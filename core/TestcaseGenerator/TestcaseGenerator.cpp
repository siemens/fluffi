/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Michael Kraus, Roman Bendt
*/

#include "stdafx.h"
#include "CommInt.h"
§§#include "GetTestcaseRequestHandler.h"
§§#include "TestcaseDescriptor.h"
#include "TGWorkerThreadStateBuilder.h"
§§#include "GetTestcaseChunkRequestHandler.h"
#include "KillInstanceRequestHandler.h"
#include "TGGetStatusRequestHandler.h"
#include "QueueCleanerWorker.h"
#include "QueueFillerWorker.h"
#include "TGTestcaseManager.h"
#include "Util.h"
#include "GarbageCollectorWorker.h"

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
§§	Util::setDefaultLogOptions("logs" + Util::pathSeperator + "TestcaseGenerator_" + std::to_string(GETPID()) + ".log");

	if (argc < 2) {
		LOG(ERROR) << "Usage: TestcaseGenerator <LOCATION> [<CommaSeparatedListOfSubtypes>]";
		google::protobuf::ShutdownProtobufLibrary();
		return EXIT_FAILURE;
	}

	std::string location = argv[1];

	std::set<std::string> myAgentSubTypes;
	myAgentSubTypes.insert("RadamsaMutator");
	myAgentSubTypes.insert("AFLMutator");
§§	myAgentSubTypes.insert("CaRRoTMutator");
	myAgentSubTypes.insert("HonggfuzzMutator");
§§	myAgentSubTypes.insert("OedipusMutator");
	myAgentSubTypes.insert("ExternalMutator");

	if (argc >= 3) {
		//Get the Subtypes specified on the commandline as set
		std::vector<std::string> desiredSubtypes = Util::splitString(argv[2], ",");
		std::set<std::string> desiredSubtypes_set(desiredSubtypes.begin(), desiredSubtypes.end());

		//See what subtypes we can actually do
		std::set<std::string> intersect;
		set_intersection(desiredSubtypes_set.begin(), desiredSubtypes_set.end(), myAgentSubTypes.begin(), myAgentSubTypes.end(), std::inserter(intersect, intersect.begin()));

		//Update the myAgentSubTypes
		myAgentSubTypes = intersect;
	}

	int intervallBetweenTwoCleaningRoundsInMillisec = 30 * 1000;
	int desiredQueueFillLevel = 500;
	int maxRetriesBeforeReport = 3;
	int intervallBetweenTwoRegistrationRoundsInMillisec = 20 * 1000;
	int intervallBetweenTwoCollectionRoundsInMillisec = 3 * 60 * 1000;
	int delayToWaitUntilConfigIsCompleteInMS = 1000;
	unsigned long maxAllowedTimeOfManagerInactivityMS = 10 * 60 * 1000;

	//The garbage collector needs to be initialized as early as possible and deleted as late as possible
	GarbageCollectorWorker* garbageCollectorWorker = new GarbageCollectorWorker(intervallBetweenTwoCollectionRoundsInMillisec);

	TGWorkerThreadStateBuilder* workerStateBuilder = new TGWorkerThreadStateBuilder();
	CommInt* comm = new CommInt(workerStateBuilder, 10, 10);
	LOG(INFO) << std::endl << "Hey! I am TestcaseGenerator " << comm->getMyGUID() << std::endl << "My location: " << location << std::endl << "My Host and Port: " << comm->getOwnServiceDescriptor().m_serviceHostAndPort << std::endl << "I was built on: " << __DATE__;
§§
	// Specify path to testcase directory, preferable relative
	std::string testcaseDir = "." + Util::pathSeperator + "testcaseFiles" + Util::pathSeperator + comm->getMyGUID();
	std::string queueFillerTempDir = testcaseDir + Util::pathSeperator + "queueFillerTempDir";
	Util::createFolderAndParentFolders(queueFillerTempDir);

§§	// Queue for managing testcases
	TGTestcaseManager* testcaseManager = new TGTestcaseManager(garbageCollectorWorker);
§§
	// ################## End of Define / Build global objects  ##################

	// ################## Registering Message Handler  ##################
	GetTestcaseRequestHandler* m_getTestcaseRequestHandler = new GetTestcaseRequestHandler(testcaseManager, testcaseDir, comm);
	comm->registerFLUFFIMessageHandler(m_getTestcaseRequestHandler, FLUFFIMessage::FluffCase::kGetTestcaseRequest);
§§
	GetTestcaseChunkRequestHandler* m_getTestcaseChunkRequestHandler = new GetTestcaseChunkRequestHandler(testcaseDir, false, garbageCollectorWorker);
	comm->registerFLUFFIMessageHandler(m_getTestcaseChunkRequestHandler, FLUFFIMessage::FluffCase::kGetTestCaseChunkRequest);
§§
	TGGetStatusRequestHandler* m_getStatusRequestHandler = new TGGetStatusRequestHandler(comm, testcaseManager);
	comm->registerFLUFFIMessageHandler(m_getStatusRequestHandler, FLUFFIMessage::FluffCase::kGetStatusRequest);

	KillInstanceRequestHandler* m_killInstanceRequestHandler = new KillInstanceRequestHandler();
	comm->registerFLUFFIMessageHandler(m_killInstanceRequestHandler, FLUFFIMessage::FluffCase::kKillInstanceRequest);

	// ################## End of Registering Message Handler  ##################

	// ################## Main Logic  ##################

#ifdef ENV64BIT
	Util::setConsoleWindowTitle("FLUFFI TG(64bit) - NO FUZZJOB YET");
#else
	Util::setConsoleWindowTitle("FLUFFI TG(32bit) - NO FUZZJOB YET");
#endif

	// Register at Global Manager
	WorkerThreadState* workerThreadState = workerStateBuilder->constructState();
	bool didGMRegistrationSucceed = comm->waitForGMRegistration(workerThreadState, AgentType::TestcaseGenerator, myAgentSubTypes, location, intervallBetweenTwoRegistrationRoundsInMillisec);
	workerStateBuilder->destructState(workerThreadState);

	//Register at Local Manager
	bool didLMRegistrationSucceed = false;
	if (didGMRegistrationSucceed) {
		workerThreadState = workerStateBuilder->constructState();
		didLMRegistrationSucceed = comm->waitForLMRegistration(workerThreadState, AgentType::TestcaseGenerator, myAgentSubTypes, location, intervallBetweenTwoRegistrationRoundsInMillisec);
		workerStateBuilder->destructState(workerThreadState);
	}

	//Only start the main logic if no key was pressed
	if (didGMRegistrationSucceed && didLMRegistrationSucceed) {
		//Start worker threads

		//The Garbage Collector thread
		LOG(DEBUG) << "Starting the GarbageCollectorWorker Thread";
		garbageCollectorWorker->m_thread = new std::thread(&GarbageCollectorWorker::workerMain, garbageCollectorWorker);

		// Thread that cleans the testcase queue
		LOG(DEBUG) << "Starting QueueCleanerWorker Thread";
		QueueCleanerWorker* queueCleanerWorker = new QueueCleanerWorker(comm, workerStateBuilder, intervallBetweenTwoCleaningRoundsInMillisec, maxRetriesBeforeReport, testcaseDir, testcaseManager, garbageCollectorWorker);
		queueCleanerWorker->m_thread = new std::thread(&QueueCleanerWorker::workerMain, queueCleanerWorker);

		// Thread that generates testcaes and puts them in the queue
		LOG(DEBUG) << "Starting QueueFillerWorker Thread";
		QueueFillerWorker* queueFillerWorker = new QueueFillerWorker(comm, workerStateBuilder, delayToWaitUntilConfigIsCompleteInMS, desiredQueueFillLevel, testcaseDir, queueFillerTempDir, testcaseManager, myAgentSubTypes, garbageCollectorWorker);
		queueFillerWorker->m_thread = new std::thread(&QueueFillerWorker::workerMain, queueFillerWorker);

		// Wait for a keypress or a kill message
		int checkAgainMS = 250;
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
				LOG(INFO) << "Could not reach manager -> Shutting down ...";
				break;
			}
			if (m_getStatusRequestHandler->wasManagerReplaced()) {
				LOG(INFO) << "It looks like my LocalManager was replaced -> Shutting down ...";
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
		}

		//Stop worker threads
		LOG(DEBUG) << "Stoping QueueCleanerWorker";
		queueCleanerWorker->stop();

		LOG(DEBUG) << "Stoping QueueFillerWorker";
		queueFillerWorker->stop();

		LOG(DEBUG) << "Stoping GarbageCollectorWorker";
		garbageCollectorWorker->stop();

		queueCleanerWorker->m_thread->join();
		delete queueCleanerWorker;
		queueFillerWorker->m_thread->join();
		delete queueFillerWorker;
		garbageCollectorWorker->m_thread->join();
		//delete garbageCollectorWorker IS NOT DONE HERE ON PURPOSE. It is done at the very end as it's destructor is supposed to remove all files
	}

	// ################## End of Main Logic  ##################

	// ################## Destruct global objects  ##################

	delete comm; //Destruct comm and stop all handlers
	comm = nullptr;

	//Destruct all handlers
	delete m_getTestcaseRequestHandler;
	m_getTestcaseRequestHandler = nullptr;
	delete m_getTestcaseChunkRequestHandler;
	m_getTestcaseChunkRequestHandler = nullptr;
	delete m_getStatusRequestHandler;
	m_getStatusRequestHandler = nullptr;
	delete m_killInstanceRequestHandler;
	m_killInstanceRequestHandler = nullptr;

	//Freeing the testcase Queue
	delete testcaseManager;
	testcaseManager = nullptr;
§§
	delete workerStateBuilder;
	workerStateBuilder = nullptr;

	//attempt to delete all remainig files
	delete garbageCollectorWorker;
	garbageCollectorWorker = nullptr;

	//delete the queueFillerTempDir folder if it is empty
	std::error_code errorcode;
	if (!std::experimental::filesystem::remove(queueFillerTempDir, errorcode)) {
		LOG(ERROR) << "Could not delete the queueFillerTempDir directory: Most likely it's not empty!";
	}
	else {
		LOG(DEBUG) << "Deleting the queueFillerTempDir directory succeeded ;)";
	}

	//delete the temp folder if it is empty
	if (!std::experimental::filesystem::remove(testcaseDir, errorcode)) {
		LOG(ERROR) << "Could not delete the temp directory: Most likely it's not empty!";
	}
	else {
		LOG(DEBUG) << "Deleting the temp directory succeeded ;)";
	}

	// Optional:  Delete all global objects allocated by libprotobuf.
	google::protobuf::ShutdownProtobufLibrary();

	// ################## End of Destruct global objects  ##################

	LOG(DEBUG) << "Program terminated normally :) ";

	return 0;
}
