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

Author(s): Thomas Riedmaier, Abian Blome, Fabian Russwurm, Roman Bendt, Pascal Eckmann
*/

#include "stdafx.h"
#include "CommInt.h"
#include "RegisterAtLMRequestHandler.h"
#include "GetTestcaseToMutateRequestHandler.h"
#include "ReportTestcaseWithNoResultRequestHandler.h"
#include "GetNewCompletedTestcaseIDsRequestHandler.h"
#include "GetCurrentBlockCoverageRequestHandler.h"
#include "PutTestEvaluationRequestHandler.h"
#include "LMWorkerThreadStateBuilder.h"
#include "InstanceMonitorWorker.h"
#include "InstanceControlWorker.h"
#include "GetTestcaseChunkRequestHandler.h"
#include "KillInstanceRequestHandler.h"
#include "GetFuzzJobConfigurationRequestHandler.h"
#include "FluffiLMConfiguration.h"
#include "LMGetStatusRequestHandler.h"
#include "DBCleanupWorker.h"
#include "IsAgentWelcomedRequestHandler.h"
#include "CommProxyWorker.h"
#include "LMDatabaseManager.h"
#include "Util.h"
#include "GarbageCollectorWorker.h"
#include "GetLMConfigurationRequestHandler.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char* argv[])
{
#ifdef HUNTMEMLEAKS
	// dump all leaks after exit, so globals, statics and stuff will already be deleted
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// this can be used to include the MSVCRT in leak detection
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_CRT_DF);

	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

	char* b = new char[14]{ "LEAK DETECTOR" };  //Trigger a memory leak for NEW
#endif

	// ################## Define / Build global objects  ##################
	Util::setDefaultLogOptions("logs" + Util::pathSeperator + "LocalManager_" + std::to_string(GETPID()) + ".log");

	if (argc < 2) {
		LOG(ERROR) << "Usage: LocalManager <LOCATION>";
		google::protobuf::ShutdownProtobufLibrary();
		return EXIT_FAILURE;
	}

	std::string location = argv[1];

	int timeBetweenTwoStatusFetchRoundsinMS = 10 * 1000;
	int timeBetweenTwoSetTGsAndTEsRoundsinMS = 10 * 1000;
	int timeBetweenTwoCleanupRoundsInMS = 3 * 60 * 1000;
	int timeBetweenTwoAttemptsToGetConfigFromGMMS = 10 * 1000;
	int intervallBetweenTwoCollectionRoundsInMillisec = 3 * 60 * 1000;
	int forceCompletedTestcasesCacheFlushAfterMS = 10 * 1000; // should be well below TG.intervallToWaitForReinsertionInMillisec
	unsigned long maxAllowedTimeOfManagerInactivityMS = 10 * 60 * 1000;

	//The garbage collector needs to be initialized as early as possible and deleted as late as possible
	GarbageCollectorWorker* garbageCollectorWorker = new GarbageCollectorWorker(intervallBetweenTwoCollectionRoundsInMillisec);

	LMWorkerThreadStateBuilder* workerStateBuilder = new LMWorkerThreadStateBuilder(garbageCollectorWorker);

	CommInt* comm = new CommInt(workerStateBuilder, 10, 10);
	LOG(INFO) << std::endl << "Hey! I am LocalManager " << comm->getMyGUID() << std::endl << "My location: " << location << std::endl << "My Host and Port: " << comm->getOwnServiceDescriptor().m_serviceHostAndPort << std::endl << "I was built on: " << __DATE__;

	// Specify path to testcase directory, preferable relative
	std::string testcaseTempDir = "." + Util::pathSeperator + "testcaseFiles" + Util::pathSeperator + comm->getMyGUID();
	Util::createFolderAndParentFolders(testcaseTempDir);

	CommProxyWorker localManagerServer(comm);

	// ################## End of Define / Build global objects  ##################

	// ################## Registering Message Handler  ##################
	LMGetStatusRequestHandler* m_getStatusRequestHandler = new LMGetStatusRequestHandler(comm);
	comm->registerFLUFFIMessageHandler(m_getStatusRequestHandler, FLUFFIMessage::FluffCase::kGetStatusRequest);

	RegisterAtLMRequestHandler* m_RegisterRequestHandler = new RegisterAtLMRequestHandler(location);
	comm->registerFLUFFIMessageHandler(m_RegisterRequestHandler, FLUFFIMessage::FluffCase::kRegisterAtLMRequest);

	GetTestcaseToMutateRequestHandler* m_GetTestcaseToMutateRequestHandler = new GetTestcaseToMutateRequestHandler(testcaseTempDir);
	comm->registerFLUFFIMessageHandler(m_GetTestcaseToMutateRequestHandler, FLUFFIMessage::FluffCase::kGetTestcaseToMutateRequest);

	ReportTestcaseWithNoResultRequestHandler* m_GetReportWithNoResultRequestHandler = new ReportTestcaseWithNoResultRequestHandler(testcaseTempDir, comm, garbageCollectorWorker);
	comm->registerFLUFFIMessageHandler(m_GetReportWithNoResultRequestHandler, FLUFFIMessage::FluffCase::kReportTestcaseWithNoResultRequest);

	GetNewCompletedTestcaseIDsRequestHandler* m_GetNewCompletedTestcaseIDsHandler = new GetNewCompletedTestcaseIDsRequestHandler();
	comm->registerFLUFFIMessageHandler(m_GetNewCompletedTestcaseIDsHandler, FLUFFIMessage::FluffCase::kGetNewCompletedTestcaseIDsRequest);

	GetCurrentBlockCoverageRequestHandler* m_GetCurrentBlockCoverageHandler = new GetCurrentBlockCoverageRequestHandler();
	comm->registerFLUFFIMessageHandler(m_GetCurrentBlockCoverageHandler, FLUFFIMessage::FluffCase::kGetCurrentBlockCoverageRequest);

	PutTestEvaluationRequestHandler* m_PutTestEvaluationHandler = new PutTestEvaluationRequestHandler(testcaseTempDir, comm, garbageCollectorWorker, forceCompletedTestcasesCacheFlushAfterMS);
	comm->registerFLUFFIMessageHandler(m_PutTestEvaluationHandler, FLUFFIMessage::FluffCase::kPutTestEvaluationRequest);

	GetTestcaseChunkRequestHandler* m_getTestcaseChunkRequestHandler = new GetTestcaseChunkRequestHandler(testcaseTempDir, true, garbageCollectorWorker);
	comm->registerFLUFFIMessageHandler(m_getTestcaseChunkRequestHandler, FLUFFIMessage::FluffCase::kGetTestCaseChunkRequest);

	GetFuzzJobConfigurationRequestHandler* m_getFuzzJobConfigurationRequestHandler = new GetFuzzJobConfigurationRequestHandler();
	comm->registerFLUFFIMessageHandler(m_getFuzzJobConfigurationRequestHandler, FLUFFIMessage::FluffCase::kGetFuzzJobConfigurationRequest);

	KillInstanceRequestHandler* m_killInstanceRequestHandler = new KillInstanceRequestHandler();
	comm->registerFLUFFIMessageHandler(m_killInstanceRequestHandler, FLUFFIMessage::FluffCase::kKillInstanceRequest);

	IsAgentWelcomedRequestHandler* m_isAgentWelcomedRequestHandler = new IsAgentWelcomedRequestHandler();
	comm->registerFLUFFIMessageHandler(m_isAgentWelcomedRequestHandler, FLUFFIMessage::FluffCase::kIsAgentWelcomedRequest);

	GetLMConfigurationRequestHandler* m_getLMConfigurationRequestHandler = new GetLMConfigurationRequestHandler();
	comm->registerFLUFFIMessageHandler(m_getLMConfigurationRequestHandler, FLUFFIMessage::FluffCase::kGetLMConfigurationRequest);
	// ################## End of Registering Message Handler  ##################

	// ################## Main Logic  ##################

#ifdef ENV64BIT
	Util::setConsoleWindowTitle("FLUFFI LM(64bit) - NO FUZZJOB YET");
#else
	Util::setConsoleWindowTitle("FLUFFI LM(32bit) - NO FUZZJOB YET");
#endif

	//Register at Global Manager
	WorkerThreadState* workerForLMConfig = workerStateBuilder->constructState();
	FluffiLMConfiguration* lmConfig = nullptr;
	bool wasKeyPressed = false;
	while (!wasKeyPressed)
	{
		bool isConfigAvailable = comm->getLMConfigFromGM(workerForLMConfig, location, CommInt::GlobalManagerHAP, &lmConfig);
		if (isConfigAvailable)
		{
			LMDatabaseManager::setDBConnectionParameters(lmConfig->m_dbHost, lmConfig->m_dbUser, lmConfig->m_dbPassword, lmConfig->m_dbName);
			LOG(INFO) << "The fuzz job we will be working on is " << lmConfig->m_fuzzJobName;
			m_RegisterRequestHandler->setMyFuzzjob(lmConfig->m_fuzzJobName);
			m_getLMConfigurationRequestHandler->setMyLMConfig(*lmConfig);
#ifdef ENV64BIT
			Util::setConsoleWindowTitle("FLUFFI LM(64bit) - " + lmConfig->m_fuzzJobName);
#else
			Util::setConsoleWindowTitle("FLUFFI LM(32bit) - " + lmConfig->m_fuzzJobName);
#endif
			break;
		}
		else
		{
			LOG(INFO) << "Could not retrieve LM Configuration from GM, retrying in " << timeBetweenTwoAttemptsToGetConfigFromGMMS / 1000 << " seconds";
			int checkAgainMS = 500;
			int waitedMS = 0;

			//Wait for timeBetweenTwoAttemptsToGetConfigFromGMMS but break if a key is pressed
			while (!wasKeyPressed)
			{
				wasKeyPressed = Util::kbhit() != 0;
				if (waitedMS < timeBetweenTwoAttemptsToGetConfigFromGMMS) {
					std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
					waitedMS += checkAgainMS;
					continue;
				}
				else {
					break;
				}
			}
		}
	}
	if (lmConfig != nullptr) {
		delete lmConfig;
	}
	workerStateBuilder->destructState(workerForLMConfig);

	//Only start the main logic if no key was pressed
	if (!wasKeyPressed) {
		//Start worker threads

		//The Garbage Collector thread
		LOG(DEBUG) << "Starting the GarbageCollectorWorker Thread";
		garbageCollectorWorker->m_thread = new std::thread(&GarbageCollectorWorker::workerMain, garbageCollectorWorker);

		// Thread for database cleanup (e.g. collection of billing information)
		LOG(DEBUG) << "Starting Database Cleanup Thread";
		DBCleanupWorker* dbCleanupWorker = new DBCleanupWorker(comm, workerStateBuilder, timeBetweenTwoCleanupRoundsInMS, location);
		dbCleanupWorker->m_thread = new std::thread(&DBCleanupWorker::workerMain, dbCleanupWorker);

		// Thread for monitoring instances (TG, TR, TE)
		LOG(DEBUG) << "Starting Thread for monitoring instances";
		InstanceMonitorWorker* instanceMonitorWorker = new InstanceMonitorWorker(comm, workerStateBuilder, timeBetweenTwoStatusFetchRoundsinMS, location);
		instanceMonitorWorker->m_thread = new std::thread(&InstanceMonitorWorker::workerMain, instanceMonitorWorker);

		// Thread for controlling instances (TG, TR, TE)
		LOG(DEBUG) << "Starting Thread for controlling instances";
		InstanceControlWorker* instanceControlWorker = new InstanceControlWorker(comm, workerStateBuilder, timeBetweenTwoSetTGsAndTEsRoundsinMS, location);
		instanceControlWorker->m_thread = new std::thread(&InstanceControlWorker::workerMain, instanceControlWorker);

		// Wait for a keypress or a kill message
		int checkAgainMS = 250;
		m_getStatusRequestHandler->resetManagerActiveDetector();
		while (true)
		{
			if (Util::kbhit() != 0) {
				LOG(INFO) << "Key Pressed -> Shutting down ...";
				break;
			}
			if (m_killInstanceRequestHandler->shouldCommitSuicide()) {
				LOG(INFO) << "Should Commit Suicide -> Shutting down ...";
				break;
			}
			if (!m_getStatusRequestHandler->isManagerActive(maxAllowedTimeOfManagerInactivityMS)) {
				LOG(ERROR) << "My manager did not contact me (Maybe I am overloaded!) -> Shutting down ...";
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
		}

		//Stop worker threads
		LOG(DEBUG) << "Stoping instanceMonitorWorker";
		instanceMonitorWorker->stop();

		LOG(DEBUG) << "Stoping instanceControlWorker";
		instanceControlWorker->stop();

		LOG(DEBUG) << "Stoping dbCleanupWorker";
		dbCleanupWorker->stop();

		LOG(DEBUG) << "Stoping GarbageCollectorWorker";
		garbageCollectorWorker->stop();

		instanceMonitorWorker->m_thread->join();
		delete instanceMonitorWorker;
		instanceControlWorker->m_thread->join();
		delete instanceControlWorker;
		dbCleanupWorker->m_thread->join();
		delete dbCleanupWorker;
		garbageCollectorWorker->m_thread->join();
		//delete garbageCollectorWorker IS NOT DONE HERE ON PURPOSE. It is done at the very end as it's destructor is supposed to remove all files
	}

	// ################## End of Main Logic  ##################

	// ################## Destruct global objects  ##################

	delete comm; //Destruct comm and stop all handlers
	comm = nullptr;

	//Destruct all handlers
	delete m_getStatusRequestHandler;
	m_getStatusRequestHandler = nullptr;
	delete m_RegisterRequestHandler;
	m_RegisterRequestHandler = nullptr;
	delete m_GetTestcaseToMutateRequestHandler;
	m_GetTestcaseToMutateRequestHandler = nullptr;
	delete m_GetReportWithNoResultRequestHandler;
	m_GetReportWithNoResultRequestHandler = nullptr;
	delete m_GetNewCompletedTestcaseIDsHandler;
	m_GetNewCompletedTestcaseIDsHandler = nullptr;
	delete m_GetCurrentBlockCoverageHandler;
	m_GetCurrentBlockCoverageHandler = nullptr;
	delete m_PutTestEvaluationHandler;
	m_PutTestEvaluationHandler = nullptr;
	delete m_getTestcaseChunkRequestHandler;
	m_getTestcaseChunkRequestHandler = nullptr;
	delete m_getFuzzJobConfigurationRequestHandler;
	m_getFuzzJobConfigurationRequestHandler = nullptr;
	delete m_killInstanceRequestHandler;
	m_killInstanceRequestHandler = nullptr;
	delete m_isAgentWelcomedRequestHandler;
	m_isAgentWelcomedRequestHandler = nullptr;
	delete m_getLMConfigurationRequestHandler;
	m_getLMConfigurationRequestHandler = nullptr;

	delete workerStateBuilder;
	workerStateBuilder = nullptr;

	//attempt to delete all remainig files
	delete garbageCollectorWorker;
	garbageCollectorWorker = nullptr;

	//delete the temp folder if it is empty
	std::error_code errorcode;
	if (!std::experimental::filesystem::remove(testcaseTempDir, errorcode)) {
		LOG(ERROR) << "Could not delete the temp directory: Most likely it's not empty!";
	}
	else {
		LOG(DEBUG) << "Deleting the temp directory succeeded ;)";
	}

	// Free mysql global objects
	mysql_library_end();

	// Delete all global objects allocated by libprotobuf (Optional).
	google::protobuf::ShutdownProtobufLibrary();

	// ################## End of Destruct global objects  ##################

	LOG(DEBUG) << "Program terminated normally :) ";

	return 0;
}
