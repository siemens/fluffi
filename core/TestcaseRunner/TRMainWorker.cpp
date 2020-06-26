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
#include "TRMainWorker.h"
#include "FluffiTestcaseID.h"
#include "DebugExecutionOutput.h"
#include "TRWorkerThreadStateBuilder.h"
#include "CommPartnerManager.h"
#include "FluffiTestResult.h"
#include "CommInt.h"
#include "FluffiSetting.h"
#include "TestExecutorDynRioSingle.h"
#include "TestExecutorDynRioMulti.h"
#include "TestExecutorQemuUserSingle.h"
#include "TestExecutorGDB.h"
#include "Util.h"
#include "TRWorkerThreadState.h"
#include "GarbageCollectorWorker.h"

TRMainWorker::TRMainWorker(CommInt* commInt,
	TRWorkerThreadStateBuilder*  workerThreadStateBuilder,
	int delayToWaitUntilConfigIsCompleteInMS,
	CommPartnerManager* tGManager,
	CommPartnerManager* tEManager,
	std::string testcaseDir,
	int* numberOfProcessedTestcases,
	std::set<std::string> myAgentSubTypes,
	GarbageCollectorWorker* garbageCollectorWorker
)
	:
	m_gotConfigFromLM(false),
	m_commInt(commInt),
	m_workerThreadStateBuilder(workerThreadStateBuilder),
	m_delayToWaitUntilConfigIsCompleteInMS(delayToWaitUntilConfigIsCompleteInMS),
	m_tGManager(tGManager),
	m_tEManager(tEManager),
	m_testcaseDir(testcaseDir),
	m_numberOfProcessedTestcases(numberOfProcessedTestcases),
	m_myAgentSubTypes(myAgentSubTypes),
	m_garbageCollectorWorker(garbageCollectorWorker),
	m_workerThreadState(nullptr),
	m_executor(nullptr)
{}

TRMainWorker::~TRMainWorker()
{
	delete m_thread;
	m_thread = nullptr;

	if (m_executor != nullptr) {
		delete m_executor;
		m_executor = nullptr;
	}
}

void TRMainWorker::stop() {
	if (m_workerThreadState != nullptr) {
		m_workerThreadState->m_stopRequested = true;
	}
}

void TRMainWorker::workerMain() {
	m_workerThreadState = dynamic_cast<TRWorkerThreadState*>(m_workerThreadStateBuilder->constructState());

	const int warningHistSize = 5;
	bool warnhist_pre[warningHistSize];
	bool warnhist_post[warningHistSize];
	for (int i = 0; i < warningHistSize; i++) {
		warnhist_pre[i] = false;
		warnhist_post[i] = false;
	}

	while (!m_workerThreadState->m_stopRequested)
	{
		std::chrono::time_point<std::chrono::steady_clock> p1 = std::chrono::steady_clock::now();// needed for performance waring calculation
		if (!m_gotConfigFromLM) {
			m_gotConfigFromLM = tryGetConfigFromLM();
			if (!m_gotConfigFromLM) {
				std::this_thread::sleep_for(std::chrono::milliseconds(m_delayToWaitUntilConfigIsCompleteInMS));
				continue;
			}
		}

		std::string generatorHAP = m_tGManager->getNextPartner();
		if (generatorHAP.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(m_delayToWaitUntilConfigIsCompleteInMS));
			continue;
		}

		std::string evaluatorHAP = m_tEManager->getNextPartner();
		if (evaluatorHAP.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(m_delayToWaitUntilConfigIsCompleteInMS));
			continue;
		}

		LOG(DEBUG) << "Executing TestcaseRunner with TestcaseGenerator on: " << generatorHAP << " and TestcaseEvaluator on: " << evaluatorHAP;

		// Get TestcaseFromGenerator
		FLUFFIMessage respMessage;
		bool success = getTestcaseFromGenerator(&respMessage, generatorHAP);
		if (!success) {
			LOG(WARNING) << "Receiving Testcase from " << generatorHAP << " failed!";
			continue;
		}
		const GetTestcaseResponse* receivedTestcase = &(respMessage.gettestcaseresponse());

		// Start Fuzzing testcase
		FluffiTestcaseID testcaseID{ receivedTestcase->id() };
		FluffiTestcaseID parentTestcaseID{ receivedTestcase->parentid() };
		std::chrono::time_point<std::chrono::steady_clock> p2 = std::chrono::steady_clock::now(); // needed for performance waring calculation
		FluffiTestResult testResult = fuzzTestCase(testcaseID, receivedTestcase->forcefullcoverage());
		std::chrono::time_point<std::chrono::steady_clock> p3 = std::chrono::steady_clock::now(); // needed for performance waring calculation
		(*m_numberOfProcessedTestcases)++;

		// Do not send internal errors to evaluator. The generator will detect this and will resend the testcase to somebody
		if (testResult.m_exitType == ExitType::Exception_Other && testResult.m_crashFootprint.substr(0, 16) == "Internal Error: ") {
			LOG(ERROR) << "Executing Testcase " << testcaseID << " failed with an internal error! We will have to do that again!";
			// Delete testcase file in filesystem
			std::string testfile = Util::generateTestcasePathAndFilename(testcaseID, m_testcaseDir);
			m_garbageCollectorWorker->markFileForDelete(testfile);
			continue;
		}

		// Push Testcase and results to Evaluator
		success = pushTestcaseWithResultsToEvaluator(evaluatorHAP, testResult, testcaseID, parentTestcaseID);
		if (!success) {
			LOG(ERROR) << "Pushing Testcase result of Testcase " << testcaseID << " to " << evaluatorHAP << " failed! We will have to do that again!";
			// Delete testcase file in filesystem
			std::string testfile = Util::generateTestcasePathAndFilename(testcaseID, m_testcaseDir);
			m_garbageCollectorWorker->markFileForDelete(testfile);
			continue;
		}

		//Performance waring calculation start
		std::chrono::time_point<std::chrono::steady_clock> p4 = std::chrono::steady_clock::now();
		//std::cout << std::chrono::duration_cast<std::chrono::milliseconds>((p2 - p1)).count() << ":" << std::chrono::duration_cast<std::chrono::milliseconds>((p3 - p2)).count() << ":" << std::chrono::duration_cast<std::chrono::milliseconds>((p4 - p3)).count() << "(" << testResult.m_blocks.size() << ")" << std::endl;
		bool warnPre = true;
		bool warnPost = true;
		for (int i = 0; i < warningHistSize - 1; i++) {
			warnhist_pre[i] = warnhist_pre[i + 1];
			warnhist_post[i] = warnhist_post[i + 1];
			warnPre &= warnhist_pre[i];
			warnPost &= warnhist_post[i];
		}
		warnhist_pre[warningHistSize - 1] = 10 < std::chrono::duration_cast<std::chrono::milliseconds>((p2 - p1)).count();
		warnhist_post[warningHistSize - 1] = static_cast<long long>(testResult.m_blocks.size() / 1000) + 15 < std::chrono::duration_cast<std::chrono::milliseconds>((p4 - p3)).count();
		warnPre &= warnhist_pre[warningHistSize - 1];
		warnPost &= warnhist_post[warningHistSize - 1];

		if (warnPre) {
			LOG(WARNING) << "Getting Testcases from TGs is slow. Maybe start more TGs?";
		}
		if (warnPost) {
			LOG(WARNING) << "Sending Testresults to TEs is slow. Maybe start more TEs?";
		}
		//Performance waring calculation end
	}
	m_workerThreadStateBuilder->destructState(m_workerThreadState);
}

bool TRMainWorker::getTestcaseFromGenerator(FLUFFIMessage* resp, std::string generatorHAP) {
	// Build and send message for GetTestcaseRequest
	FLUFFIMessage req;
	req.set_allocated_gettestcaserequest(new GetTestcaseRequest());

	bool respReceived = m_commInt->sendReqAndRecvResp(&req, resp, m_workerThreadState, generatorHAP, CommInt::timeoutNormalMessage);
	if (respReceived) {
		LOG(DEBUG) << "GetTestcaseResponse successfully received (first part of Testcase)!";
	}
	else {
		LOG(ERROR) << "No GetTestcaseResponse received, check timeout and the accessibility of the TestcaseGenerator!";
		return false;
	}

	const GetTestcaseResponse* receivedTestcase = &(resp->gettestcaseresponse());
	if (!receivedTestcase->success()) {
		LOG(WARNING) << "Testcase generator appears to be overloaded!";
		return false;
	}

	return Util::storeTestcaseFileOnDisk(receivedTestcase->id(), m_testcaseDir, &receivedTestcase->testcasefirstchunk(),
		receivedTestcase->islastchunk(), receivedTestcase->id().servicedescriptor().servicehostandport(), m_commInt, m_workerThreadState, m_garbageCollectorWorker);
}

bool TRMainWorker::pushTestcaseWithResultsToEvaluator(std::string evaluatorTargetHAP, const FluffiTestResult testResult, const FluffiTestcaseID testcaseId, const FluffiTestcaseID parentId) {
	bool isLastChunk;
	std::string data = Util::loadTestcaseChunkInMemory(testcaseId, m_testcaseDir, 0, &isLastChunk);
	if (isLastChunk) {
		//If the last chunk was just read: delete the file on all systems but the testcase generator (as the files are only temporary files)
		std::string testfile = Util::generateTestcasePathAndFilename(testcaseId, m_testcaseDir);
		m_garbageCollectorWorker->markFileForDelete(testfile);
	}

	TestcaseID* mutableTestcaseId = new TestcaseID();
	mutableTestcaseId->CopyFrom(testcaseId.getProtobuf());

	TestcaseID* mutableParentId = new TestcaseID();
	mutableParentId->CopyFrom(parentId.getProtobuf());

	TestResult* mutableTestResult = new TestResult();
	testResult.setProtobuf(mutableTestResult); // This one is potentially slow as it depends on the number of covered blocks

	// Build PutTestResultRequest
	FLUFFIMessage putTcReq;
	PutTestResultRequest* putTcRequest = new PutTestResultRequest();

	ServiceDescriptor* mySd = new ServiceDescriptor();
	mySd->CopyFrom(m_commInt->getOwnServiceDescriptor().getProtobuf());

	// Build Response with loaded Testcase
	putTcRequest->set_allocated_id(mutableTestcaseId);
	putTcRequest->set_allocated_parentid(mutableParentId);
	putTcRequest->set_allocated_result(mutableTestResult);
	putTcRequest->set_testcasefirstchunk(data);
	putTcRequest->set_islastchunk(isLastChunk);
	putTcRequest->set_allocated_sdofrunner(mySd);
	putTcReq.set_allocated_puttestresultrequest(putTcRequest);

	// Get Response from evaluator
	FLUFFIMessage putTcResp;
	bool respReceived = m_commInt->sendReqAndRecvResp(&putTcReq, &putTcResp, m_workerThreadState, evaluatorTargetHAP, CommInt::timeoutFileTransferMessage);
	if (!respReceived)
	{
		LOG(ERROR) << "PutTestResultRequest failed, no response received! check timeout and the accessibility of the TestcaseEvaluator!";
		return false;
	}

	bool success = putTcResp.puttestresultresponse().success();
	if (success)
	{
		LOG(DEBUG) << "PutTestResultRequest successfull!";
	}
	else
	{
		LOG(ERROR) << "PutTestResultRequest failed, TestcaseEvaluator has problems receiving the testcase!";
		return false;
	}

	return true;
}

bool TRMainWorker::tryGetConfigFromLM() {
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

	std::set<Module> modulesToCover;
	for (auto i : fjcr->targetmodules()) {
		modulesToCover.emplace(i.modulename(), i.modulepath(), i.moduleid());
		LOG(INFO) << "MOD: [" << i.moduleid() << "] " << i.modulename();
	}
	if (modulesToCover.size() == 0) {
		LOG(ERROR) << "Interesting modules list is empty. This is not a valid fuzz job configuration!";
		return false;
	}

	if (settings.count("chosenSubtype") == 0) {
		LOG(ERROR) << "No runnerType/chosenSubtype was specified. This is not a valid fuzz job configuration!";
		return false;
	}

	if (m_myAgentSubTypes.count(settings["chosenSubtype"]) == 0) {
		LOG(ERROR) << "The agent was requested to use an agent type for which it was not compiled. This should never happen! Check agent assignment!";
		return false;
	}

	//Initializing the testcase executor here allows to chose from different executors via config options
	if (Util::stringHasEnding(settings["chosenSubtype"], "DynRioSingle"))
	{
		LOG(INFO) << "Using TestExecutorDynRioSingle for running testcases";

		if (settings.count("targetCMDLine") == 0) {
			LOG(ERROR) << "No targetCMDLine received!";
			return false;
		}
		std::string targetcmdline = settings["targetCMDLine"];

		if (settings.count("hangTimeout") == 0) {
			LOG(ERROR) << "No hangTimeout value received!";
			return false;
		}

		int hangTimeout;
		try {
			hangTimeout = std::stoi(settings["hangTimeout"]);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}

		ExternalProcess::CHILD_OUTPUT_TYPE suppressChildOutput = ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS;
		if (settings.count("suppressChildOutput") == 0) {
			LOG(INFO) << "No suppressChildOutput value received - using default (true)";
		}
		else {
			suppressChildOutput = (settings["suppressChildOutput"] == "true") ? ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS : ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT;
		}

		bool treatAnyAccessViolationAsFatal = false;
		if (settings.count("treatAnyAccessViolationAsFatal") == 0) {
			LOG(INFO) << "No treatAnyAccessViolationAsFatal value received - using default (false)";
		}
		else {
			treatAnyAccessViolationAsFatal = (settings["treatAnyAccessViolationAsFatal"] == "true") ? true : false;
		}

#if defined(_WIN32) || defined(_WIN64)
		std::string additionalEnvParam = "";
#else
		std::string additionalEnvParam = "";
		if (settings.count("additionalEnvParam") == 0) {
			LOG(INFO) << "No additionalEnvParam value received - using default (\"\")";
		}
		else {
			additionalEnvParam = settings["additionalEnvParam"];
		}
#endif

		m_executor = new TestExecutorDynRioSingle(targetcmdline, hangTimeout, modulesToCover, m_testcaseDir, suppressChildOutput, additionalEnvParam, m_garbageCollectorWorker, treatAnyAccessViolationAsFatal);
	}
	else if (Util::stringHasEnding(settings["chosenSubtype"], "DynRioMulti")) {
		LOG(INFO) << "Using TestExecutorDynRioMulti for running testcases";

		if (settings.count("targetCMDLine") == 0) {
			LOG(ERROR) << "No targetCMDLine received!";
			return false;
		}
		std::string targetcmdline = settings["targetCMDLine"];

		if (settings.count("hangTimeout") == 0) {
			LOG(ERROR) << "No hangTimeout value received!";
			return false;
		}

		int hangTimeout;
		try {
			hangTimeout = std::stoi(settings["hangTimeout"]);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}

		ExternalProcess::CHILD_OUTPUT_TYPE suppressChildOutput = ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS;
		if (settings.count("suppressChildOutput") == 0) {
			LOG(INFO) << "No suppressChildOutput value received - using default (true)";
		}
		else {
			suppressChildOutput = (settings["suppressChildOutput"] == "true") ? ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS : ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT;
		}

		if (settings.count("feederCMDLine") == 0) {
			LOG(ERROR) << "No feederCMDLine received!";
			return false;
		}
		std::string feederCmdline = settings["feederCMDLine"];

		if (settings.count("initializationTimeout") == 0) {
			LOG(ERROR) << "No initializationTimeout value received!";
			return false;
		}

		int initializationTimeoutMS;
		try {
			initializationTimeoutMS = std::stoi(settings["initializationTimeout"]);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}

		std::string starterCMDLine = "";
		if (settings.count("starterCMDLine") == 0) {
			LOG(INFO) << "No starterCMDLine value received - using default (\"\")";
		}
		else {
			starterCMDLine = settings["starterCMDLine"];
		}

#if defined(_WIN32) || defined(_WIN64)
		bool targetForks = false;
#else
		bool targetForks = false;
		if (settings.count("targetForks") == 0) {
			LOG(INFO) << "No targetForks value received - using default (false)";
		}
		else {
			targetForks = (settings["targetForks"] == "true") ? true : false;
		}
#endif
		bool treatAnyAccessViolationAsFatal = false;
		if (settings.count("treatAnyAccessViolationAsFatal") == 0) {
			LOG(INFO) << "No treatAnyAccessViolationAsFatal value received - using default (false)";
		}
		else {
			treatAnyAccessViolationAsFatal = (settings["treatAnyAccessViolationAsFatal"] == "true") ? true : false;
		}

		int forceRestartAfter = -1;
		if (settings.count("forceRestartAfter") == 0) {
			LOG(INFO) << "No forceRestartAfter value received - using default \"Not forced\"";
		}
		else {
			try {
				forceRestartAfter = std::stoi(settings["forceRestartAfter"]);
			}
			catch (...) {
				LOG(ERROR) << "std::stoi failed";
				google::protobuf::ShutdownProtobufLibrary();
				_exit(EXIT_FAILURE); //make compiler happy
			}
		}

#if defined(_WIN32) || defined(_WIN64)
		std::string additionalEnvParam = "";
#else
		std::string additionalEnvParam = "";
		if (settings.count("additionalEnvParam") == 0) {
			LOG(INFO) << "No additionalEnvParam value received - using default (\"\")";
		}
		else {
			additionalEnvParam = settings["additionalEnvParam"];
		}
#endif

		m_executor = new TestExecutorDynRioMulti(targetcmdline, hangTimeout, modulesToCover, m_testcaseDir, suppressChildOutput, additionalEnvParam, m_garbageCollectorWorker, treatAnyAccessViolationAsFatal, feederCmdline, starterCMDLine, initializationTimeoutMS, m_commInt, targetForks, forceRestartAfter);
	}
	else if (Util::stringHasEnding(settings["chosenSubtype"], "QemuUserSingle")) {
		LOG(INFO) << "Using TestExecutorQemuUserSingle for running testcases";

		if (settings.count("targetCMDLine") == 0) {
			LOG(ERROR) << "No targetCMDLine received!";
			return false;
		}
		std::string targetcmdline = settings["targetCMDLine"];

		if (settings.count("hangTimeout") == 0) {
			LOG(ERROR) << "No hangTimeout value received!";
			return false;
		}
		int hangTimeout;
		try {
			hangTimeout = std::stoi(settings["hangTimeout"]);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}

		ExternalProcess::CHILD_OUTPUT_TYPE suppressChildOutput = ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS;
		if (settings.count("suppressChildOutput") == 0) {
			LOG(INFO) << "No suppressChildOutput value received - using default (true)";
		}
		else {
			suppressChildOutput = (settings["suppressChildOutput"] == "true") ? ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS : ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT;
		}

		bool treatAnyAccessViolationAsFatal = false;
		if (settings.count("treatAnyAccessViolationAsFatal") == 0) {
			LOG(INFO) << "No treatAnyAccessViolationAsFatal value received - using default (false)";
		}
		else {
			treatAnyAccessViolationAsFatal = (settings["treatAnyAccessViolationAsFatal"] == "true") ? true : false;
		}

		if (settings.count("changerootTemplate") == 0) {
			LOG(ERROR) << "No changerootTemplate received!";
			return false;
		}
		std::string changerootTemplate = settings["changerootTemplate"];

		m_executor = new TestExecutorQemuUserSingle(targetcmdline, hangTimeout, modulesToCover, m_testcaseDir, suppressChildOutput, m_garbageCollectorWorker, treatAnyAccessViolationAsFatal, changerootTemplate);
	}
	else if (Util::stringHasEnding(settings["chosenSubtype"], "GDB")) {
		LOG(INFO) << "Using TestExecutorGDB for running testcases";

		if (settings.count("targetCMDLine") == 0) {
			LOG(ERROR) << "No targetCMDLine received!";
			return false;
		}
		std::string targetcmdline = settings["targetCMDLine"];

		if (settings.count("hangTimeout") == 0) {
			LOG(ERROR) << "No hangTimeout value received!";
			return false;
		}
		int hangTimeout;
		try {
			hangTimeout = std::stoi(settings["hangTimeout"]);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}

		ExternalProcess::CHILD_OUTPUT_TYPE suppressChildOutput = ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS;
		if (settings.count("suppressChildOutput") == 0) {
			LOG(INFO) << "No suppressChildOutput value received - using default (true)";
		}
		else {
			suppressChildOutput = (settings["suppressChildOutput"] == "true") ? ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS : ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT;
		}

		if (settings.count("feederCMDLine") == 0) {
			LOG(ERROR) << "No feederCMDLine received!";
			return false;
		}
		std::string feederCmdline = settings["feederCMDLine"];

		if (settings.count("initializationTimeout") == 0) {
			LOG(ERROR) << "No initializationTimeout value received!";
			return false;
		}

		int initializationTimeoutMS;
		try {
			initializationTimeoutMS = std::stoi(settings["initializationTimeout"]);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}

		std::string starterCMDLine = "";
		if (settings.count("starterCMDLine") == 0) {
			LOG(INFO) << "No starterCMDLine value received - using default (\"\")";
		}
		else {
			starterCMDLine = settings["starterCMDLine"];
		}

		int forceRestartAfter = -1;
		if (settings.count("forceRestartAfter") == 0) {
			LOG(INFO) << "No forceRestartAfter value received - using default \"Not forced\"";
		}
		else {
			try {
				forceRestartAfter = std::stoi(settings["forceRestartAfter"]);
			}
			catch (...) {
				LOG(ERROR) << "std::stoi failed";
				google::protobuf::ShutdownProtobufLibrary();
				_exit(EXIT_FAILURE); //make compiler happy
			}
		}

		if (settings.count("breakPointInstruction") == 0) {
			LOG(ERROR) << "No breakPointInstruction value received!";
			return false;
		}

		int breakPointInstruction;
		try {
			breakPointInstruction = std::stoi(settings["breakPointInstruction"], 0, 0x10);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}

		if (settings.count("breakPointInstructionBytes") == 0) {
			LOG(ERROR) << "No breakPointInstructionBytes value received!";
			return false;
		}

		int breakPointInstructionBytes;
		try {
			breakPointInstructionBytes = std::stoi(settings["breakPointInstructionBytes"]);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}

		std::set<FluffiBasicBlock> blocksToCover;
		for (auto i : fjcr->targetblocks()) {
			blocksToCover.insert(FluffiBasicBlock(i.rva(), i.moduleid()));
		}
		LOG(INFO) << "Got  " << blocksToCover.size() << " blocks to cover";

		m_executor = new TestExecutorGDB(targetcmdline, hangTimeout, modulesToCover, m_testcaseDir, suppressChildOutput, m_garbageCollectorWorker, feederCmdline, starterCMDLine, initializationTimeoutMS, forceRestartAfter, blocksToCover, breakPointInstruction, breakPointInstructionBytes);
	}
	else {
		LOG(ERROR) << "The specified runnerType/chosenSubtype \"" << settings["chosenSubtype"] << "\" is not implemented but m_myAgentSubTypes.count(settings[\"chosenSubtype\"]) was >0. This should never happen!";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy;
	}

	//check if the setup is actually working
	bool isSetupFunctionable = m_executor->isSetupFunctionable();
	if (!isSetupFunctionable) {
		LOG(ERROR) << "The setup is not working! Are all files where they are supposed to be?";
		delete m_executor;
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy;
	}

	return true;
}

FluffiTestResult TRMainWorker::fuzzTestCase(const FluffiTestcaseID testcaseId, bool forceFullCoverage) {
	LOG(DEBUG) << "Fuzzing testcase " << testcaseId;

	LOG(DEBUG) << "starting execution";
	std::shared_ptr<DebugExecutionOutput> output = m_executor->execute(testcaseId, forceFullCoverage);
	LOG(DEBUG) << "finished execution";

	LOG(DEBUG) << "Termination description:" << output->m_terminationDescription;
	if (output->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN) {
		LOG(DEBUG) << "exit normal";
		return FluffiTestResult(ExitType::CleanExit, output->getCoveredBasicBlocks(), "", output->m_hasFullCoverage);
	}
	else {
		switch (output->m_terminationType) {
		case DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR:
			LOG(INFO) << "ERROR: " << output->m_terminationDescription;
			return FluffiTestResult(ExitType::Exception_Other, std::vector<FluffiBasicBlock>(), "Internal Error: " + output->m_terminationDescription, output->m_hasFullCoverage);
		case DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER:
			LOG(INFO) << "CRASH @ " << std::hex << output->m_lastCrash << " !";
			return FluffiTestResult(ExitType::Exception_Other, std::vector<FluffiBasicBlock>(), output->m_firstCrash + "-" + output->m_lastCrash, output->m_hasFullCoverage);
		case DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION:
			LOG(INFO) << "ACCESS VIOLATION @ " << std::hex << output->m_lastCrash << " !";
			return FluffiTestResult(ExitType::Exception_AccessViolation, std::vector<FluffiBasicBlock>(), output->m_firstCrash + "-" + output->m_lastCrash, output->m_hasFullCoverage);
		case DebugExecutionOutput::PROCESS_TERMINATION_TYPE::TIMEOUT:
			LOG(INFO) << "HANG!";
			return FluffiTestResult(ExitType::Hang, std::vector<FluffiBasicBlock>(), "", output->m_hasFullCoverage);
		default:
			LOG(ERROR) << "Observed non-implemented termination type!";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}
	}
}