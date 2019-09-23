/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "TEMainWorker.h"
#include "TEWorkerThreadStateBuilder.h"
#include "TETestResultManager.h"
#include "BlockCoverageCache.h"
#include "CommInt.h"
#include "FluffiTestcaseID.h"
#include "TestOutcomeDescriptor.h"
#include "Util.h"
#include "TEWorkerThreadState.h"
#include "GarbageCollectorWorker.h"
#include "CoverageEvaluator.h"
#include "FluffiSetting.h"

TEMainWorker::TEMainWorker(CommInt* commInt,
	TEWorkerThreadStateBuilder* workerThreadStateBuilder,
	int delayToWaitUntilConfigIsCompleteInMS,
	TETestResultManager* teTestResultManager,
	std::string testcaseDir,
	BlockCoverageCache* localBlockCoverageCache,
	std::set<std::string> myAgentSubTypes,
	GarbageCollectorWorker* garbageCollectorWorker
) :
	m_gotConfigFromLM(false),
	m_commInt(commInt),
	m_workerThreadStateBuilder(workerThreadStateBuilder),
	m_teTestResultManager(teTestResultManager),
	m_testcaseDir(testcaseDir),
	m_localBlockCoverageCache(localBlockCoverageCache),
	m_garbageCollectorWorker(garbageCollectorWorker),
	m_delayToWaitUntilConfigIsCompleteInMS(delayToWaitUntilConfigIsCompleteInMS),
	m_workerThreadState(nullptr),
	m_myAgentSubTypes(myAgentSubTypes),
	m_evaluator(nullptr)
{
}

TEMainWorker::~TEMainWorker()
{
	delete m_thread;
	m_thread = nullptr;

	if (m_evaluator != nullptr) {
		delete m_evaluator;
		m_evaluator = nullptr;
	}
}

void TEMainWorker::stop() {
	if (m_workerThreadState != nullptr) {
		m_workerThreadState->m_stopRequested = true;
	}
}

void TEMainWorker::workerMain() {
	m_workerThreadState = dynamic_cast<TEWorkerThreadState*>(m_workerThreadStateBuilder->constructState());
	int checkAgainMS = 500;

	while (!m_workerThreadState->m_stopRequested)
	{
		if (!m_gotConfigFromLM) {
			m_gotConfigFromLM = tryGetConfigFromLM();
			if (!m_gotConfigFromLM) {
				std::this_thread::sleep_for(std::chrono::milliseconds(m_delayToWaitUntilConfigIsCompleteInMS));
				continue;
			}
		}

		TestOutcomeDescriptor* tod = m_teTestResultManager->popTestOutcomeForEvaluation();
		if (tod == nullptr) {
			std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
			continue;
		}

		m_evaluator->processTestOutcomeDescriptor(tod);
		delete tod;
	}
	m_workerThreadStateBuilder->destructState(m_workerThreadState);
}

bool TEMainWorker::tryGetConfigFromLM() {
	// get config from lm
	FLUFFIMessage req;
	FLUFFIMessage resp;
	GetFuzzJobConfigurationRequest* getfuzzjobconfigurationrequest = new GetFuzzJobConfigurationRequest();
	ServiceDescriptor* ptMySelfServiceDescriptor = new ServiceDescriptor();
	ptMySelfServiceDescriptor->CopyFrom(m_commInt->getOwnServiceDescriptor().getProtobuf());
	getfuzzjobconfigurationrequest->set_allocated_servicedescriptor(ptMySelfServiceDescriptor);
	req.set_allocated_getfuzzjobconfigurationrequest(getfuzzjobconfigurationrequest);

	bool respReceived = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, CommInt::timeoutNormalMessage);
	if (!respReceived) {
		LOG(ERROR) << "No GetFuzzJobConfigurationResponse received!";
		return false;
	}

	LOG(DEBUG) << "GetFuzzJobConfigurationResponse successfully received!";
	const GetFuzzJobConfigurationResponse* fjcr = &(resp.getfuzzjobconfigurationresponse());

	std::map<std::string, std::string> settings;

	for (auto i : fjcr->settings()) {
		FluffiSetting fs = FluffiSetting(i);
		settings.insert(std::make_pair(fs.m_settingName, fs.m_settingValue));
		LOG(INFO) << "Setting: " << fs.m_settingName << " - " << fs.m_settingValue;
	}

	if (settings.count("chosenSubtype") == 0) {
		LOG(ERROR) << "No evaluatorType/chosenSubtype was specified. This is not a valid fuzz job configuration!";
		return false;
	}

	if (m_myAgentSubTypes.count(settings["chosenSubtype"]) == 0) {
		LOG(ERROR) << "The agent was requested to use an agent type for which it was not compiled. This should never happen! Check agent assignment!";
		return false;
	}

	//Initializing the testcase executor here allows to chose from different executors via config options
	if (settings["chosenSubtype"] == "CoverageEvaluator")
	{
		m_evaluator = new CoverageEvaluator(m_testcaseDir, m_localBlockCoverageCache, m_garbageCollectorWorker, m_commInt, m_workerThreadState);
	}
	else {
		LOG(ERROR) << "The specified chosenSubtype/evaluatorType \"" << settings["chosenSubtype"] << "\" is not implemented but m_myAgentSubTypes.count(settings[\"chosenSubtype\"]) was >0. This should never happen!";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy;
	}

	//check if the setup is actually working
	bool isSetupFunctionable = m_evaluator->isSetupFunctionable();
	if (!isSetupFunctionable) {
		LOG(ERROR) << "The setup is not working! Are all files where they are supposed to be?";
		delete m_evaluator;
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy;
	}

	return true;
}
