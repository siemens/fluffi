/*
Copyright 2017-2020 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Roman Bendt
*/

#include "stdafx.h"
#include "QueueFillerWorker.h"
#include "CommInt.h"
#include "Util.h"
#include "FluffiTestcaseID.h"
#include "TestcaseDescriptor.h"
#include "TGTestcaseManager.h"
#include "TGWorkerThreadStateBuilder.h"
#include "TGWorkerThreadState.h"
#include "GarbageCollectorWorker.h"
#include "RadamsaMutator.h"
#include "CaRRoTMutator.h"
#include "OedipusMutator.h"
#include "HonggfuzzMutator.h"
#include "ExternalMutator.h"
#include "FluffiSetting.h"
#include "FluffiMutator.h"
#include "AFLMutator.h"

QueueFillerWorker::QueueFillerWorker(CommInt* commInt, TGWorkerThreadStateBuilder* workerThreadStateBuilder, int delayToWaitUntilConfigIsCompleteInMS, size_t desiredQueueFillLevel, std::string testcaseDirectory, std::string queueFillerTempDir, TGTestcaseManager* testcaseManager, std::set<std::string> myAgentSubTypes, GarbageCollectorWorker* garbageCollectorWorker, int maxBulkGenerationSize) :
	m_gotConfigFromLM(false),
	m_commInt(commInt),
	m_workerThreadStateBuilder(workerThreadStateBuilder),
	m_desiredQueueFillLevel(desiredQueueFillLevel),
	m_queueFillerTempDir(queueFillerTempDir),
	m_testcaseManager(testcaseManager),
	m_bulkGenerationSize(maxBulkGenerationSize),
	m_maxBulkGenerationSize(maxBulkGenerationSize),
	m_mySelfServiceDescriptor(commInt->getOwnServiceDescriptor()),
	m_workerThreadState(nullptr),
	m_garbageCollectorWorker(garbageCollectorWorker),
	m_testcaseDirectory(testcaseDirectory),
	m_delayToWaitUntilConfigIsCompleteInMS(delayToWaitUntilConfigIsCompleteInMS),
	m_myAgentSubTypes(myAgentSubTypes),
	m_mutator(nullptr),
	m_mutatorNeedsParents(true)
{}

QueueFillerWorker::~QueueFillerWorker()
{
	delete m_thread;
	m_thread = nullptr;

	if (m_mutator != nullptr) {
		delete m_mutator;
		m_mutator = nullptr;
	}
}

void QueueFillerWorker::stop() {
	if (m_workerThreadState != nullptr) {
		m_workerThreadState->m_stopRequested = true;
	}
}

void QueueFillerWorker::workerMain() {
	m_workerThreadState = dynamic_cast<TGWorkerThreadState*>(m_workerThreadStateBuilder->constructState());
	if (m_workerThreadState == nullptr) {
		LOG(ERROR) << "QueueFillerWorker::workerMain - m_workerThreadStateBuilder->constructState() failed";
		return;
	}

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

		if (m_testcaseManager->getPendingTestcaseQueueSize() > m_desiredQueueFillLevel) {
			std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
			continue;
		}

		FluffiTestcaseID parentID{ FluffiServiceDescriptor{"",""},0 };
		try
		{
			if (m_mutatorNeedsParents) {
				parentID = getNewParent();
			}
		}
		catch (const std::runtime_error& e) {
			LOG(ERROR) << "Failed to get a new parent (" << e.what() << ") we'll try again!";
			continue;
		}

		//from this point on there is a parent testcase file that we have to take care of!
		std::string parentPathAndFileName = Util::generateTestcasePathAndFilename(parentID, m_queueFillerTempDir);

		try
		{
			std::deque<TestcaseDescriptor> children = m_mutator->batchMutate(m_bulkGenerationSize, parentID, parentPathAndFileName);

			if (children.size() > 0)
			{
				m_testcaseManager->pushNewGeneratedTestcases(children);
				reportNewMutations(parentID, static_cast<int>(children.size()));

				//adapt bulk generation size
				if (m_bulkGenerationSize < m_maxBulkGenerationSize) {
					m_bulkGenerationSize++;
				}
			}
			else
			{
				LOG(DEBUG) << "Batch queue generation returned 0 elements";
			}
		}
		catch (const std::runtime_error& e) {
			LOG(ERROR) << "batchMutate failed (" << e.what() << ")!";

			//adapt bulk generation size
			if (m_bulkGenerationSize > 1) {
				m_bulkGenerationSize--;
			}
		}

		//delete the parent testcase file
		if (m_mutatorNeedsParents) {
			if (std::remove(parentPathAndFileName.c_str()) != 0) {
				// mark the parent file for lazy delete
				m_garbageCollectorWorker->markFileForDelete(parentPathAndFileName);
				m_garbageCollectorWorker->collectNow();
			}
		}
	}

	m_workerThreadStateBuilder->destructState(m_workerThreadState);
}

FluffiTestcaseID QueueFillerWorker::getNewParent()
{
	FLUFFIMessage req;
	GetTestcaseToMutateRequest* getTestcaseToMutateRequest = new GetTestcaseToMutateRequest();
	req.set_allocated_gettestcasetomutaterequest(getTestcaseToMutateRequest);
	FLUFFIMessage resp = FLUFFIMessage();

	bool respReceived = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, CommInt::timeoutNormalMessage);

	if (respReceived)
	{
		const GetTestcaseToMutateResponse* receivedTestcase = &(resp.gettestcasetomutateresponse());
		LOG(DEBUG) << "GetTestcaseToMutateResponse successfully received (first part of Testcase): " << FluffiTestcaseID(receivedTestcase->id());

		//parents are stored in the m_queueFillerTempDir and NOT in the m_testcaseDirectory as race conditions might mess everything up! ("attempting to delete no longer existing file" if a file is already inserted as interesting_tc but not yet cleaned from queue)
		bool fileStored = Util::storeTestcaseFileOnDisk(receivedTestcase->id(), m_queueFillerTempDir, &receivedTestcase->testcasefirstchunk(), receivedTestcase->islastchunk(), m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, m_commInt, m_workerThreadState, m_garbageCollectorWorker);
		if (fileStored)
		{
			LOG(DEBUG) << "File successfully stored!";

			FluffiTestcaseID parentID{ receivedTestcase->id() };

			//Check if the LM wants us to also run the unmodified (original) testcase
			if (receivedTestcase->alsorunwithoutmutation()) {
				//Careful: Time of check is not time of use! This works as long as there is only one queue filler thread!
				//check if the testcase it is already in the queue
				std::string filenameInTestcaseDir = Util::generateTestcasePathAndFilename(parentID, m_testcaseDirectory);
				if (!std::experimental::filesystem::exists(filenameInTestcaseDir)) {
					//add it to the queue
					std::string filenameInTemp = Util::generateTestcasePathAndFilename(parentID, m_queueFillerTempDir);
					std::experimental::filesystem::copy(filenameInTemp, filenameInTestcaseDir);
					FluffiServiceDescriptor specialSD{ "special","special" };
					FluffiTestcaseID specialTestcase{ specialSD,0 };
					TestcaseDescriptor td{ parentID, specialTestcase, filenameInTestcaseDir,true };
					m_testcaseManager->pushNewGeneratedTestcase(td);
				}
			}

			return parentID;
		}
		else
		{
			LOG(ERROR) << "File storage of parent testcase from LM unsuccessful";
			throw std::runtime_error("File storage unsuccesful");
		}
	}
	LOG(ERROR) << "No GetTestcaseToMutateResponse received, we retry next cycle";
	throw std::runtime_error("GetTestcase unsuccesful");
}

bool QueueFillerWorker::tryGetConfigFromLM() {
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
		LOG(ERROR) << "No generatorType/chosenSubtype was specified. This is not a valid fuzz job configuration!";
		return false;
	}

	if (m_myAgentSubTypes.count(settings["chosenSubtype"]) == 0) {
		LOG(ERROR) << "The agent was requested to use an agent type for which it was not compiled. This should never happen! Check agent assignment!";
		return false;
	}

	//Initializing the testcase executor here allows to chose from different executors via config options
	if (settings["chosenSubtype"] == "RadamsaMutator")
	{
		m_mutator = new RadamsaMutator{ m_mySelfServiceDescriptor, m_testcaseDirectory };
	}
	else if (settings["chosenSubtype"] == "AFLMutator")
	{
		m_mutator = new AFLMutator{ m_mySelfServiceDescriptor, m_testcaseDirectory };
	}
	else if (settings["chosenSubtype"] == "CaRRoTMutator")
	{
		m_mutator = new CaRRoTMutator{ m_mySelfServiceDescriptor, m_testcaseDirectory };
	}
	else if (settings["chosenSubtype"] == "HonggfuzzMutator")
	{
		m_mutator = new HonggfuzzMutator{ m_mySelfServiceDescriptor, m_testcaseDirectory };
	}
	else if (settings["chosenSubtype"] == "OedipusMutator")
	{
		m_mutator = new OedipusMutator{ m_mySelfServiceDescriptor, m_testcaseDirectory, m_commInt, m_workerThreadState };
	}
	else if (settings["chosenSubtype"] == "ExternalMutator")
	{
		m_mutator = new ExternalMutator{ m_mySelfServiceDescriptor, m_testcaseDirectory, settings["extGeneratorDirectory"], m_commInt, m_workerThreadState };
		m_mutatorNeedsParents = false;
	}
	else {
		LOG(ERROR) << "The specified chosenSubtype/generatorType \"" << settings["chosenSubtype"] << "\" is not implemented but m_myAgentSubTypes.count(settings[\"chosenSubtype\"]) was >0. This should never happen!";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy;
	}

	//check if the setup is actually working
	bool isSetupFunctionable = m_mutator->isSetupFunctionable();
	if (!isSetupFunctionable) {
		LOG(ERROR) << "The setup is not working! Are all files where they are supposed to be?";
		delete m_mutator;
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy;
	}

	return true;
}

void QueueFillerWorker::reportNewMutations(FluffiTestcaseID id, int numOfNewMutations) {
	FLUFFIMessage req;
	ReportNewMutationsRequest* reportNewMutationsRequest = new ReportNewMutationsRequest();
	LOG(DEBUG) << "Reporting " << numOfNewMutations << " new mutations on " << id;

	TestcaseID* mutableTestcaseId = new TestcaseID();
	mutableTestcaseId->CopyFrom(id.getProtobuf());

	reportNewMutationsRequest->set_allocated_id(mutableTestcaseId);
	reportNewMutationsRequest->set_numofnewmutationsinqueue(numOfNewMutations);
	req.set_allocated_reportnewmutationsrequest(reportNewMutationsRequest);
	FLUFFIMessage resp = FLUFFIMessage();

	bool respReceived = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, CommInt::timeoutNormalMessage);
	if (!respReceived) {
		LOG(ERROR) << "No response to our sending of a new ReportNewMutationsRequest received!";
		return;
	}
}