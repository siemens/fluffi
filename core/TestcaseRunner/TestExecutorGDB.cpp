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
#include "TestExecutorGDB.h"
#include "FluffiTestcaseID.h"
#include "DebugExecutionOutput.h"
#include "Util.h"
#include "SharedMemIPC.h"
#include "GarbageCollectorWorker.h"
#include "GDBBreakpoint.h"

TestExecutorGDB::TestExecutorGDB(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover,
	const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, GarbageCollectorWorker* garbageCollectorWorker,
	const std::string  feederCmdline, const std::string starterCmdline, int initializationTimeoutMS, int forceRestartAfterXTCs,
	std::set<FluffiBasicBlock> blocksToCover, uint32_t bpInstr, int bpInstrBytes) :
	FluffiTestExecutor(targetCMDline, hangTimeoutMS, modulesToCover, testcaseDir, child_output_mode, "", garbageCollectorWorker), //As we rely on starters, the Environment of the target process cannot be influenced here
	m_feederCmdline(feederCmdline),
	m_starterCmdline(starterCmdline),
	m_initializationTimeoutMS(initializationTimeoutMS),
	m_forceRestartAfterXTCs(forceRestartAfterXTCs),
	m_executionsSinceLastRestart(0),
	m_sharedMemIPC_toFeeder(nullptr),
	m_feederProcess(std::make_shared<ExternalProcess>("", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS)),
	m_gDBThreadCommunication{ std::make_shared<GDBThreadCommunication>() },
	m_blocksToCover(blocksToCover),
	m_target_and_feeder_okay(false),
	m_bpInstr(bpInstr),
	m_bpInstrBytes(bpInstrBytes),
#if defined(_WIN32) || defined(_WIN64)
	m_SharedMemIPCInterruptEvent(NULL)
#else
	m_SharedMemIPCInterruptFD{
	-1, -1
}
#endif
{
#if defined(_WIN32) || defined(_WIN64)
	LOG(DEBUG) << "Setting the SharedMemIPCInterruptEvent to stop the current execution";
	m_SharedMemIPCInterruptEvent = CreateEvent(NULL, false, false, NULL);
	if (m_SharedMemIPCInterruptEvent == NULL) {
		LOG(ERROR) << "failed to create the SharedMemIPCInterruptEvent";
	}
#else
	LOG(DEBUG) << "Creating the SharedMemIPCInterruptFD to stop the current execution";
	if (pipe(m_SharedMemIPCInterruptFD) == -1) {
		LOG(ERROR) << "failed to create the SharedMemIPCInterruptFD";
	}
#endif
}

TestExecutorGDB::~TestExecutorGDB()
{
	m_gDBThreadCommunication->set_gdbThreadShouldTerminate(); //Stop the gdb debugging thread

#if defined(_WIN32) || defined(_WIN64)
	if (m_sharedMemIPC_toFeeder != nullptr) {
		delete m_sharedMemIPC_toFeeder;
		m_sharedMemIPC_toFeeder = nullptr;
	}

	if (m_SharedMemIPCInterruptEvent != NULL) {
		CloseHandle(m_SharedMemIPCInterruptEvent);
		m_SharedMemIPCInterruptEvent = NULL;
	}

#else
	if (m_sharedMemIPC_toFeeder != nullptr) {
		delete m_sharedMemIPC_toFeeder;
		m_sharedMemIPC_toFeeder = nullptr;
	}

	//Try closing the interrupt pipe
	close(m_SharedMemIPCInterruptFD[0]);
	m_SharedMemIPCInterruptFD[0] = -1;
	close(m_SharedMemIPCInterruptFD[1]);
	m_SharedMemIPCInterruptFD[1] = -1;
#endif

	waitForDebuggerToTerminate(); // Avoid ugly memleaks
}

std::shared_ptr<DebugExecutionOutput> TestExecutorGDB::execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage) {
	LOG(DEBUG) << "Executing: " << testcaseId;
	if (m_forceRestartAfterXTCs > 0 && m_executionsSinceLastRestart++ > m_forceRestartAfterXTCs) {
		LOG(INFO) << "Forcing a restart of the target as we reached the defined maximum of TCs without restart";
		m_target_and_feeder_okay = false;
	}

	bool firstRun = false;
	if (!m_target_and_feeder_okay) {
		m_target_and_feeder_okay = attemptStartTargetAndFeeder();
		if (!m_target_and_feeder_okay) {
			//Either target or feeder could not be started!
			std::shared_ptr<DebugExecutionOutput> exResult = std::make_shared<DebugExecutionOutput>();
			exResult->m_hasFullCoverage = false;
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exResult->m_terminationDescription = "Either target or feeder could not be started!";
			return exResult;
		}
		firstRun = true;
	}

	std::shared_ptr<DebugExecutionOutput> exOutput_FROM_FEEDER = std::make_shared<DebugExecutionOutput>();
	exOutput_FROM_FEEDER->m_hasFullCoverage = forceFullCoverage | firstRun;
	m_target_and_feeder_okay = runSingleTestcase(testcaseId, exOutput_FROM_FEEDER, exOutput_FROM_FEEDER->m_hasFullCoverage ? CoverageMode::FULL : CoverageMode::PARTIAL);

	//m_target_and_feeder_okay will be set to false always but on clean exit. This will lead to a restart of fuzzer and target on next testcase by attemptStartTargetAndFeeder
	switch (m_gDBThreadCommunication->m_exOutput.m_terminationType)
	{
	case DebugExecutionOutput::CLEAN:
		//Check if the target still runs. Clean may mean "Target still running - no problem so far" and "Target terminated without an Exception"
		if (m_gDBThreadCommunication->get_exitStatus() == -1) {
			//Case "Target still running - no problem so far" (exit status was not yet set to a meaningfull value)
			switch (exOutput_FROM_FEEDER->m_terminationType)
			{
			case DebugExecutionOutput::CLEAN:
				LOG(DEBUG) << "Testcase execution successfull! Reporting coverage...";
				return exOutput_FROM_FEEDER;
			case DebugExecutionOutput::TIMEOUT:
				LOG(DEBUG) << "Testcase execution yielded a timeout";
				m_target_and_feeder_okay = false;
				return exOutput_FROM_FEEDER;
			case DebugExecutionOutput::EXCEPTION_OTHER:
				LOG(DEBUG) << "Feeder claims that the target encountered an exception.";
				m_target_and_feeder_okay = false;
				return exOutput_FROM_FEEDER;
			case DebugExecutionOutput::EXCEPTION_ACCESSVIOLATION:
			case DebugExecutionOutput::ERR:
			default:
				LOG(WARNING) << "Testcase execution yielded an internal error (1): " << exOutput_FROM_FEEDER->m_terminationDescription;
				m_target_and_feeder_okay = false;
				return exOutput_FROM_FEEDER;
			}
			LOG(ERROR) << "The testcase exectution did not trigger a case in the switch statement (1)";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy;
		}
		//Case "Target terminated without an Exception" - treat this like an Exception
		/* fall through */

	case DebugExecutionOutput::EXCEPTION_OTHER:
	case DebugExecutionOutput::EXCEPTION_ACCESSVIOLATION:
		LOG(DEBUG) << "Testcase execution yielded an exception - or the target terminated \"cleanly\" by itself. Let's try to reproduce this without breakpoints.";
		{
			//Do a second run without setting breakpoints
			DebugExecutionOutput::PROCESS_TERMINATION_TYPE originalCrashType = m_gDBThreadCommunication->m_exOutput.m_terminationType;
			m_target_and_feeder_okay = attemptStartTargetAndFeeder();
			if (!m_target_and_feeder_okay) {
				//Either target or feeder could not be started!
				std::shared_ptr<DebugExecutionOutput> exResult = std::make_shared<DebugExecutionOutput>();
				exResult->m_hasFullCoverage = false;
				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
				exResult->m_terminationDescription = "Either target or feeder coould not be started to reproduce the exception!";
				return exResult;
			}

			std::shared_ptr<DebugExecutionOutput> second_exOutput_FROM_FEEDER = std::make_shared<DebugExecutionOutput>();
			second_exOutput_FROM_FEEDER->m_hasFullCoverage = false;
			runSingleTestcase(testcaseId, second_exOutput_FROM_FEEDER, CoverageMode::NONE);

			//If the feeder timed out - i.e. there was no response from target - it crashed and we need to wait for the debugger thread
			if (second_exOutput_FROM_FEEDER->m_terminationType != DebugExecutionOutput::CLEAN) {
				waitForDebuggerToTerminate();
			}

			//Check if the target still runs. Clean may mean "Target still running - no problem so far" and "Target terminated without an Exception"
			if (m_gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::CLEAN && m_gDBThreadCommunication->get_exitStatus() != -1) {
				//Case "Target terminated without an Exception"
				std::shared_ptr<DebugExecutionOutput> exResult = std::make_shared<DebugExecutionOutput>();
				exResult->m_terminationType = originalCrashType;
				exResult->m_terminationDescription = "The request reproducibly causes the target to terminate!";
				exResult->m_firstCrash = "TARGET_TERMINATED";
				exResult->m_lastCrash = "TARGET_TERMINATED";
				m_target_and_feeder_okay = false; //force reinitialization (dyn rio needs to be reactivated)
				return exResult;
			}

			if (m_gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::CLEAN || m_gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::TIMEOUT || m_gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::ERR) {
				std::shared_ptr<DebugExecutionOutput> exResult = std::make_shared<DebugExecutionOutput>();
				exResult->m_hasFullCoverage = false;
				exResult->m_terminationType = originalCrashType;
				exResult->m_terminationDescription = "The found exception could not be reproduced!";
				exResult->m_firstCrash = "NOT_REPRODUCIBLE";
				exResult->m_lastCrash = "NOT_REPRODUCIBLE";
				m_target_and_feeder_okay = false; //force reinitialization
				return exResult;
			}
		}
		m_target_and_feeder_okay = false; //force reinitialization
		return  std::make_shared<DebugExecutionOutput>(m_gDBThreadCommunication->m_exOutput);
	case DebugExecutionOutput::ERR:
	case DebugExecutionOutput::TIMEOUT:
	default:
		LOG(WARNING) << "Testcase execution yielded an internal error (2): " << m_gDBThreadCommunication->m_exOutput.m_terminationDescription;
		m_target_and_feeder_okay = false;
		return std::make_shared<DebugExecutionOutput>(m_gDBThreadCommunication->m_exOutput);
	}

	LOG(ERROR) << "The testcase exectution did not trigger a case in the switch statement (2)";
	google::protobuf::ShutdownProtobufLibrary();
	_exit(EXIT_FAILURE); //make compiler happy;
}

bool TestExecutorGDB::isSetupFunctionable() {
	//1)Check if targetCMDline is a functional GDB
	{
		std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
		std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(500);

		ExternalProcess ep(m_targetCMDline + " --version", ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL);
		bool success = ep.initProcess();
		if (!success) {
			LOG(ERROR) << "targetCMDline " << m_targetCMDline << "could not be initialized";
			return false;
		}

		std::istream* is = ep.getStdOutIstream();
		success = ep.run();
		if (!success) {
			LOG(ERROR) << "targetCMDline " << m_targetCMDline << "could not be run";
			return false;
		}

		char identifier[8];
		identifier[7] = 0;
		std::streamsize charsRead = 0;
		while (charsRead < 7 && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
			std::streamsize charsToRead = 7 - charsRead;
			if (is->peek() != EOF) {
				charsRead += is->readsome(&identifier[charsRead], charsToRead);
			}
		}

		if (charsRead < 7 || std::string(identifier) != "GNU gdb") {
			LOG(ERROR) << "targetCMDline seems not to point to a valid GDB";
			return false;
		}
	}

	//2) check if the feeder executable file exists
	{
#if defined(_WIN32) || defined(_WIN64)
		std::wstring wfeedercmdline = std::wstring(m_feederCmdline.begin(), m_feederCmdline.end());
		int numOfBlocks;
		LPWSTR* szArglist = CommandLineToArgvW(wfeedercmdline.c_str(), &numOfBlocks); //for some reasons this does not exist for Ascii :(
		if (NULL == szArglist || numOfBlocks < 1) {
			LOG(ERROR) << "feeder command line invalid";
			return false;
		}

		if (!std::experimental::filesystem::exists(szArglist[0])) {
			LocalFree(szArglist);
			LOG(ERROR) << "feeder executable file does not exist";
			return false;
		}
		LocalFree(szArglist);
#else
		std::string tmp = m_feederCmdline;
		std::replace(tmp.begin(), tmp.end(), '<', '_'); // replace all '<' to '_' (handle <RANDOM_SHAREDMEM> et al)
		std::replace(tmp.begin(), tmp.end(), '>', '_'); // replace all '>' to '_' (handle <RANDOM_SHAREDMEM> et al)
		char** argv = ExternalProcess::split_commandline(tmp);
		if (argv == NULL || argv[0] == NULL) {
			LOG(ERROR) << "Splitting feeder command line \"" << tmp << "\" failed.";
			if (argv != NULL) {
				free(argv);
			}
			return false;
		}

		bool setupFunctionable = std::experimental::filesystem::exists(argv[0]);

		//free argv
		int i = 0;
		while (argv[i] != NULL) {
			free(argv[i]);
			i++;
		}
		free(argv);

		if (!setupFunctionable) {
			LOG(WARNING) << "feeder executable not found";
			return false;
		}
#endif
	}

	//3) check if the starter executable file exists
#if defined(_WIN32) || defined(_WIN64)
	std::wstring wstartercmdline = std::wstring(m_starterCmdline.begin(), m_starterCmdline.end());
	int numOfBlocks;
	LPWSTR* szArglist = CommandLineToArgvW(wstartercmdline.c_str(), &numOfBlocks); //for some reasons this does not exist for Ascii :(
	if (NULL == szArglist || numOfBlocks < 1) {
		LOG(ERROR) << "starter command line invalid";
		return false;
	}

	if (!std::experimental::filesystem::exists(szArglist[0])) {
		LocalFree(szArglist);
		LOG(ERROR) << "starter executable file does not exist";
		return false;
	}
	LocalFree(szArglist);
#else
	std::string tmp = m_starterCmdline;
	std::replace(tmp.begin(), tmp.end(), '<', '_'); // replace all '<' to '_' (handle <RANDOM_SHAREDMEM> et al)
	std::replace(tmp.begin(), tmp.end(), '>', '_'); // replace all '>' to '_' (handle <RANDOM_SHAREDMEM> et al)
	char** argv = ExternalProcess::split_commandline(tmp);
	if (argv == NULL || argv[0] == NULL) {
		LOG(ERROR) << "Splitting starter command line \"" << tmp << "\" failed.";
		if (argv != NULL) {
			free(argv);
		}
		return false;
	}

	bool setupFunctionable = std::experimental::filesystem::exists(argv[0]);

	//free argv
	int i = 0;
	while (argv[i] != NULL) {
		free(argv[i]);
		i++;
	}
	free(argv);

	if (!setupFunctionable) {
		LOG(WARNING) << "starter executable not found";
		return false;
	}
#endif

	//4) check if the testcase directory  exists
	if (!std::experimental::filesystem::exists(m_testcaseDir)) {
		LOG(ERROR) << "testcase directory does not exist";
		return false;
	}

	//5) check if we have at least one block to cover
	if (m_blocksToCover.size() == 0) {
		LOG(ERROR) << "There are no blocks to cover!";
		return false;
	}

	//6) check if we have a moduleid for all blocks to cover have
	for (const FluffiBasicBlock& fluffiBasicBlockit : m_blocksToCover) {
		bool isThereAModule = false;
		for (const Module& modIt : m_modulesToCover) {
			if (fluffiBasicBlockit.m_moduleID == modIt.m_moduleid) {
				isThereAModule = true;
				break;
			}
		}

		if (!isThereAModule) {
			LOG(ERROR) << "There are blocks we should cover for which we do not have a module!";
			return false;
		}
	}

	//7) check if m_bpInstr and m_bpInstrBytes are set and valid
	switch (m_bpInstrBytes)
	{
	case 1:
		if (m_bpInstr > 255) {
			LOG(ERROR) << "m_bpInstr cannot be represented in 1 byte, as it is " << m_bpInstr;
			return false;
		}
		break;
	case 2:
		if (m_bpInstr > 65535) {
			LOG(ERROR) << "m_bpInstr cannot be represented in 2 bytes, as it is " << m_bpInstr;
			return false;
		}
		break;
	case 4:
		//Currently, m_bpInstr is an uint32, which can always be represented as 4 bytes
		break;
	default:
		LOG(ERROR) << "bpInstrBytes must be 1,2, or 4";
		return false;
	}

	return true;
}

bool TestExecutorGDB::waitUntilTargetIsBeingDebugged(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, int timeoutMS) {
	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(timeoutMS);

	gDBThreadCommunication->waitForDebuggingReadyTimeout(latestRoutineExitTimeStamp - std::chrono::steady_clock::now());

	return gDBThreadCommunication->get_debuggingReady();
}

bool TestExecutorGDB::waitUntilCoverageState(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, GDBThreadCommunication::COVERAGE_STATE desiredState, int timeoutMS) {
	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(timeoutMS);

	while (!gDBThreadCommunication->get_gdbThreadShouldTerminate() && gDBThreadCommunication->get_coverageState() != desiredState && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
		gDBThreadCommunication->waitForTerminateMessageOrCovStateChangeTimeout(latestRoutineExitTimeStamp - std::chrono::steady_clock::now(), &desiredState, 1);
	}

	return gDBThreadCommunication->get_coverageState() == desiredState;
}

bool TestExecutorGDB::attemptStartTargetAndFeeder() {
	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(m_initializationTimeoutMS);
	LOG(DEBUG) << "Attempting to start target and feeder";

	/*Design decission: Target is ALWAYS started before feeder.
	- feeder can be told the target's PID
	- from a feeder's perspective the target always looks like an already running server (no matter what it actually is)
	*/

	m_executionsSinceLastRestart = 0;

	//Target part -------------------------------------------------

	m_gDBThreadCommunication->set_gdbThreadShouldTerminate(); //Stop the gdb debugging thread

		//Start the target by starting the starter (starter is a class member, so the destructor is called when a new starter is created or on class destruct)
	std::string gdbInitFile = m_testcaseDir + Util::pathSeperator + "GDBInit_" + Util::newGUID();
	std::shared_ptr<ExternalProcess> starterProcess = std::make_shared<ExternalProcess>(m_starterCmdline + " \"" + gdbInitFile + "\"", m_child_output_mode);
#if defined(_WIN32) || defined(_WIN64)
	starterProcess->setAllowBreakAway(); //children of the starter process should run outside of the starter job.
#else
	//No need to set allow break away as on Linux we trace only the direct child anyway. It is free to create new processes as it likes
#endif
	if (starterProcess->initProcess()) {
		if (!starterProcess->runAndWaitForCompletion(static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()))) {
			LOG(ERROR) << "The starter process was not able to start the target in time";
			m_garbageCollectorWorker->markFileForDelete(gdbInitFile);
			return false;
		}
	}
	else {
		LOG(ERROR) << "Could not initialize starter process";
		m_garbageCollectorWorker->markFileForDelete(gdbInitFile);
		return false;
	}

	m_gDBThreadCommunication = std::make_shared<GDBThreadCommunication>();
#if defined(_WIN32) || defined(_WIN64)
	std::thread debuggerThread(&TestExecutorGDB::debuggerThreadMain, m_targetCMDline, m_gDBThreadCommunication, m_child_output_mode, gdbInitFile, m_SharedMemIPCInterruptEvent, m_modulesToCover, m_blocksToCover, m_bpInstr, m_bpInstrBytes);
#else
	std::thread debuggerThread(&TestExecutorGDB::debuggerThreadMain, m_targetCMDline, m_gDBThreadCommunication, m_child_output_mode, gdbInitFile, m_SharedMemIPCInterruptFD[1], m_modulesToCover, m_blocksToCover, m_bpInstr, m_bpInstrBytes);
#endif
	debuggerThread.detach(); //we do not want to join this. It will terminate as we set m_gDBThreadCommunication->m_gdbThreadShouldTerminate to true
	LOG(DEBUG) << "started debugger thread";

	//We need to wait for the target's initialization ("ready to debug").
	if (!waitUntilTargetIsBeingDebugged(m_gDBThreadCommunication, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()))) {
		LOG(ERROR) << "Failed to wait for the target's/debugger's initialization.";
		m_garbageCollectorWorker->markFileForDelete(gdbInitFile);
		return false;
	}

	//Initialization is done - we don't need this any longer
	m_garbageCollectorWorker->markFileForDelete(gdbInitFile);

	//End of Target part -------------------------------------------------

	//Feeder part -------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
	std::string feederSharedMemName = "FLUFFI_SharedMem_" + Util::newGUID();
#else
	std::string feederSharedMemName = "/FLUFFI_SharedMem_" + Util::newGUID();
#endif

	if (m_sharedMemIPC_toFeeder != nullptr) {
		delete m_sharedMemIPC_toFeeder;
		m_sharedMemIPC_toFeeder = nullptr;
	}
	m_sharedMemIPC_toFeeder = new SharedMemIPC(feederSharedMemName.c_str());
	if (!m_sharedMemIPC_toFeeder->initializeAsServer()) {
		LOG(ERROR) << "Could not create the shared memory ipc connection to talk to the feeder ";
		return false;
	}

	m_feederProcess = std::make_shared<ExternalProcess>(m_feederCmdline + " " + feederSharedMemName, m_child_output_mode);
	if (!initializeFeeder(m_feederProcess, m_sharedMemIPC_toFeeder, m_gDBThreadCommunication->m_exOutput.m_PID, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()))) {
		LOG(ERROR) << "Could not initialize the feeder ";
		return false;
	}

#if defined(_WIN32) || defined(_WIN64)
	if (0 == ResetEvent(m_SharedMemIPCInterruptEvent)) {
		LOG(ERROR) << "Could not reset the m_SharedMemIPCInterruptEvent!";
		return false;
	}
	else {
		LOG(DEBUG) << "The SharedMemIPCInterruptEvent was resetted";
	}
#else
	//clear the interrupt pipe
	int nbytes = 0;
	ioctl(m_SharedMemIPCInterruptFD[0], FIONREAD, &nbytes);
	if (nbytes > 0) {
		char* buff = new char[nbytes];
		ssize_t bytesRead = read(m_SharedMemIPCInterruptFD[0], buff, nbytes);
		delete[] buff;
		if (bytesRead == nbytes) {
			LOG(DEBUG) << "The m_SharedMemIPCInterruptFD was resetted";
		}
		else {
			LOG(ERROR) << "Could not reset the m_SharedMemIPCInterruptFD!";
			return false;
		}
	}
	else {
		LOG(DEBUG) << "The m_SharedMemIPCInterruptFD was not resetted as it was not set";
	}
#endif

	//End of Feeder part -------------------------------------------------

	return true;
}

#if defined(_WIN32) || defined(_WIN64)
void TestExecutorGDB::debuggerThreadMain(const std::string targetCMDline, std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication,
	ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const  std::string  gdbInitFile, HANDLE sharedMemIPCInterruptEvent,
	const std::set<Module> modulesToCover, const std::set<FluffiBasicBlock> blocksToCover, uint32_t bpInstr, int bpInstrBytes) {
#else
void TestExecutorGDB::debuggerThreadMain(const std::string targetCMDline, std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const  std::string  gdbInitFile, int sharedMemIPCInterruptWriteFD, const std::set<Module> modulesToCover, const std::set<FluffiBasicBlock> blocksToCover, unsigned int bpInstr, int bpInstrBytes) {
#endif
	gDBThreadCommunication->m_exOutput.m_debuggerThreadDone = false;

	gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
	gDBThreadCommunication->m_exOutput.m_terminationDescription = "The target did not run!";

	//Start gdb
	ExternalProcess gdbProc = ExternalProcess(targetCMDline, ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL);
	bool success = gdbProc.initProcess();
	if (!success) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "debuggerThreadMain: Calling \"initProcess\" on the GDB process failed!";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		gDBThreadCommunication->set_gdbThreadShouldTerminate();
		gDBThreadCommunication->m_exOutput.m_debuggerThreadDone = true;
		return;
	}
	gDBThreadCommunication->m_ostreamToGdb = gdbProc.getStdInOstream();
	HANDLETYPE inputHandleFromGdb = gdbProc.getStdOutHandle();
#if defined(_WIN32) || defined(_WIN64)
#else
	gDBThreadCommunication->m_gdbPid = gdbProc.getProcessID();
#endif
	success = gdbProc.run();
	if (!success) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "debuggerThreadMain: Calling \"run\" on the GDB process failed!";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		gDBThreadCommunication->set_gdbThreadShouldTerminate();
		gDBThreadCommunication->m_exOutput.m_debuggerThreadDone = true;
		return;
	}

	//Init gdb
	std::ifstream initCommandFile(gdbInitFile, std::ifstream::in);
	if (!initCommandFile.is_open()) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "debuggerThreadMain: Could not open the gdbInitFile!";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		gDBThreadCommunication->set_gdbThreadShouldTerminate();
		gDBThreadCommunication->m_exOutput.m_debuggerThreadDone = true;
		return;
	}
	if (initCommandFile.rdbuf()->pubseekoff(0, std::ios_base::end) > 0) {
		//Only pipe GDBInitfile if it has any content
		initCommandFile.rdbuf()->pubseekpos(std::ios_base::beg);
		*gDBThreadCommunication->m_ostreamToGdb << initCommandFile.rdbuf();
	}
	//*gDBThreadCommunication->m_ostreamToGdb << "set stop-on-solib-events 1" << std::endl; //stop on module load
	*gDBThreadCommunication->m_ostreamToGdb << "set new-console on" << std::endl; //do not print the debugees output in our console
	gDBThreadCommunication->m_ostreamToGdb->flush();

	initCommandFile.close();

	//Init gdbLinereader
	std::thread gdbLinereaderThread(&TestExecutorGDB::gdbLinereaderThread, gDBThreadCommunication, inputHandleFromGdb, child_output_mode);
	gdbLinereaderThread.detach(); //we do not want to join this. It will terminate as soon as gDBThreadCommunication->m_gdbThreadShouldTerminate is set to true

	//Remove the gdb headers from the gdbOutputQueue by sending a dummy command (when it's response arrives, we now we are done)
	//As there might be leftover commands we might need to clean that first
	std::string resp;
	bool noProblem = false;
	while (true) {
		noProblem = sendCommandToGDBAndWaitForResponse("set", &resp, gDBThreadCommunication);
		if (noProblem) {
			break;
		}
		else if (resp == "LEFTOVER") {
			while (gDBThreadCommunication->get_gdbOutputQueue_size() > 0) {
				gDBThreadCommunication->gdbOutputQueue_pop_front();
			}
			continue;
		}
		break;
	}
	if (!noProblem) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "debuggerThreadMain: clearing the gdb output queue failed";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		gDBThreadCommunication->set_gdbThreadShouldTerminate();
		gDBThreadCommunication->m_exOutput.m_debuggerThreadDone = true;
		return;
	}

	gdbDebug(gDBThreadCommunication, modulesToCover, blocksToCover, bpInstr, bpInstrBytes);

	if (gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION || gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER) {
		//In case an exception occured, set sharedMemIPCInterruptEvent / write to the sharedMemIPCInterruptWriteFD, so the shared mem communication does not need to wait for an timeout but terminates early
#if defined(_WIN32) || defined(_WIN64)
		if (0 == SetEvent(sharedMemIPCInterruptEvent)) {
			LOG(ERROR) << "Failed setting the  sharedMemIPCInterruptEvent";
		}
#else
		char buf[] = "X";
		ssize_t charsWritten = write(sharedMemIPCInterruptWriteFD, buf, 1);
		if (charsWritten != 1) {
			LOG(ERROR) << "Failed setting the  sharedMemIPCInterruptWriteFD";
		}
#endif
	}

	gDBThreadCommunication->set_gdbThreadShouldTerminate();
	gDBThreadCommunication->m_exOutput.m_debuggerThreadDone = true;
	return;
}

bool TestExecutorGDB::sendCommandToGDBAndWaitForResponse(const std::string command, std::string* response, std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, bool sendCtrlZ) {
	//LOG(DEBUG) << "sendCommandToGDBAndWaitForResponse:\"" << command << "\"";
	if (command == "") {
		//Nothing to do
		return true;
	}

	if (gDBThreadCommunication->get_gdbOutputQueue_size() != 0) {
		//There is something left over from the last call. This is most likely a bug!
		*response = "LEFTOVER";
		return false;
	}

	if (sendCtrlZ) {
#if defined(_WIN32) || defined(_WIN64)
		// Disable Ctrl-C handling for our program
		if (SetConsoleCtrlHandler(NULL, true) == 0) {
			*response = "ERROR";
			return false;
		}

		//Send Ctrl-C to all programs in our Console
		if (GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0) == 0) {
			SetConsoleCtrlHandler(NULL, false);
			*response = "ERROR";
			return false;
		}

		//Avoid race conditions (command processed before Ctrl+z is processed)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		//Re-enable Ctrl-C handling or any subsequently started programs will inherit the disabled state.
		SetConsoleCtrlHandler(NULL, false);

#else
		kill(gDBThreadCommunication->m_gdbPid, SIGINT);
#endif
	}

	//Send the actual command
	for (size_t i = 0; i < command.length(); i += 1000) {
		//Handle gdbThreadShouldTerminate
		if (gDBThreadCommunication->get_gdbThreadShouldTerminate()) {
			*response = "INTERRUPTED";
			return false;
		}

		*gDBThreadCommunication->m_ostreamToGdb << command.substr(i, 1000);
		gDBThreadCommunication->m_ostreamToGdb->flush();
	}
	*gDBThreadCommunication->m_ostreamToGdb << std::endl << "fluffi" << std::endl;

	std::stringstream responseSS;
	bool firstLine = true;
	while (true) {
		//Wait for response
		gDBThreadCommunication->waitForTerminateMessageOrCovStateChange(nullptr, 0);

		//Handle gdbThreadShouldTerminate
		if (gDBThreadCommunication->get_gdbThreadShouldTerminate()) {
			*response = "INTERRUPTED";
			return false;
		}

		//Handle gdbOutputQueue
		while (gDBThreadCommunication->get_gdbOutputQueue_size() > 0) {
			std::string line = gDBThreadCommunication->gdbOutputQueue_pop_front();
			if (line.find("Undefined command") != std::string::npos) {
				*response = responseSS.str();
				return true;
			}
			else {
				if (firstLine) {
					firstLine = false;
				}
				else {
					responseSS << "\n";
				}
				responseSS << line;
			}
		}
	}
}

std::string TestExecutorGDB::getCrashRVA(std::vector<tModuleAddressesAndSizes>& baseAddressesAndSizes, uint64_t signalAddress) {
	for (tModuleAddressesAndSizes& baseAddressesAndSizesit : baseAddressesAndSizes) {
		if (std::get<1>(baseAddressesAndSizesit) < signalAddress &&std::get<1>(baseAddressesAndSizesit) + std::get<2>(baseAddressesAndSizesit) > signalAddress) {
			std::stringstream oss;
			oss << std::get<0>(baseAddressesAndSizesit) << "+0x" << std::hex << std::setw(8) << std::setfill('0') << signalAddress - std::get<1>(baseAddressesAndSizesit);
			return oss.str();
		}
	}

	std::stringstream oss;
	oss << "unknown+0x" << std::hex << std::setw(8) << std::setfill('0') << signalAddress;
	return oss.str();
}

bool TestExecutorGDB::handleSignal(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, std::string signalmessage,
	std::map<uint64_t, GDBBreakpoint>* allBreakpoints, std::set<FluffiBasicBlock>* blocksCoveredSinceLastReset, int bpInstrBytes) {
	std::string signalName;
	std::string::size_type pos = signalmessage.find(',');
	if (pos != std::string::npos)
	{
		signalName = signalmessage.substr(24, pos - 24);
	}
	else
	{
		signalName = signalmessage.substr(24, 9);
	}

	std::string addressLine;
	bool gotAddressLine = false;
	while (!gotAddressLine) {
		if (gDBThreadCommunication->get_gdbOutputQueue_size() == 0) {
			//Try to wait for the message that contains the current ip
			gDBThreadCommunication->waitForTerminateMessageOrCovStateChange(nullptr, 0);

			if (gDBThreadCommunication->get_gdbOutputQueue_size() == 0) {
				//waiting failed: we should terminate
				return false;
			}
		}

		//read a line from the gdb input
		addressLine = gDBThreadCommunication->gdbOutputQueue_pop_front();

		//Is it the address line?
		if (addressLine.substr(0, 2) == "0x") {
			gotAddressLine = true;
		}
		else if (addressLine.find("Switching to Thread") != std::string::npos) {
			//These are well known and can be ignored
		}
		else {
			LOG(DEBUG) << "handleSignal: Unexpected message from gdb:" << addressLine;
		}
	}

	uint64_t signalAddress;
	try {
		signalAddress = std::stoull(addressLine.c_str(), 0, 0x10);

		if (signalAddress == 0) {
			LOG(WARNING) << "handleSignal could not parse line " << addressLine << " as address";
		}
	}
	catch (...) {
		LOG(ERROR) << "handleSignal could not parse line " << addressLine << " as address: std::stoi/stoull failed";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy
	}

	if (signalName.substr(0, 7) == "SIGTRAP") {
		//possibly a breakpoint - see if we can handle it

		bool breakPointKnown = (allBreakpoints->count(signalAddress) > 0);
		bool breakPointKnownOffByOne = (allBreakpoints->count(signalAddress - 1) > 0);

		if (breakPointKnown || breakPointKnownOffByOne) {
			//Yep, this is one of our breakpoints
			LOG(DEBUG) << "This is one of our breakpoints";

			GDBBreakpoint gdbb = allBreakpoints->at(signalAddress - (breakPointKnown ? 0 : 1));
			blocksCoveredSinceLastReset->insert(gdbb.m_fbb);

			//Mark the breakpoint as disabled
			allBreakpoints->at(signalAddress - (breakPointKnown ? 0 : 1)).m_isEnabled = false;

			//Remove the breakpoint
			switch (bpInstrBytes)
			{
			case 1:
				*gDBThreadCommunication->m_ostreamToGdb << "set {unsigned char}0x" << std::hex << gdbb.m_absoluteAdress << " = 0x" << std::hex << gdbb.m_originalBytes << std::endl;
				break;
			case 2:
				*gDBThreadCommunication->m_ostreamToGdb << "set {unsigned short}0x" << std::hex << gdbb.m_absoluteAdress << " = 0x" << std::hex << gdbb.m_originalBytes << std::endl;
				break;
			case 4:
				*gDBThreadCommunication->m_ostreamToGdb << "set {unsigned int}0x" << std::hex << gdbb.m_absoluteAdress << " = 0x" << std::hex << gdbb.m_originalBytes << std::endl;
				break;
			default:
				LOG(ERROR) << "Reached a code path in generateEnableAllCommand, that should be unreachable";
				google::protobuf::ShutdownProtobufLibrary();
				_exit(EXIT_FAILURE); //make compiler happy
			}

			//Jump to the recently written byte (equivaltent to decrementing the instruction pointer and continuing - but that would be architecture dependant)
			*gDBThreadCommunication->m_ostreamToGdb << "j *0x" << std::hex << gdbb.m_absoluteAdress << std::endl;

			return true;
		}
	}
	//This is not one of our breakpoints!
	LOG(DEBUG) << "This is some strange bp";

	std::vector<tModuleAddressesAndSizes> baseAddressesAndSizes;
	std::string infoFilesResp;
	if (!sendCommandToGDBAndWaitForResponse("info files", &infoFilesResp, gDBThreadCommunication)) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "handleSignal: \"info files\" failed";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}
	if (!getBaseAddressesAndSizes(baseAddressesAndSizes, infoFilesResp)) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "handleSignal: \"getBaseAddressesAndSizes\" failed";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}

	std::string crashRVA = getCrashRVA(baseAddressesAndSizes, signalAddress);

	if (gDBThreadCommunication->m_exOutput.m_firstCrash == "") {
		//first crash
		gDBThreadCommunication->m_exOutput.m_firstCrash = crashRVA;
		gDBThreadCommunication->m_exOutput.m_lastCrash = crashRVA;

		//Try to continue the signal
		*gDBThreadCommunication->m_ostreamToGdb << "c" << std::endl;
	}
	else {
		if (gDBThreadCommunication->m_exOutput.m_lastCrash == crashRVA) {
			//Apparently, the program failed to handle the last signal

			if (signalName.substr(0, 7) == "SIGSEGV") {
				gDBThreadCommunication->m_exOutput.m_terminationDescription = "Detected an EXCEPTION_ACCESS_VIOLATION";
				gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION;
			}
			else {
				gDBThreadCommunication->m_exOutput.m_terminationDescription = "Detected an Unhandled signal: " + signalName;
				gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
			}

			gDBThreadCommunication->set_gdbThreadShouldTerminate();
		}
		else {
			//Apparently, the program managed to handle the last signal

			gDBThreadCommunication->m_exOutput.m_lastCrash = crashRVA;

			//Try to continue the signal
			*gDBThreadCommunication->m_ostreamToGdb << "c" << std::endl;
		}
	}

	return true;
}

bool TestExecutorGDB::consumeGDBOutputQueue(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication,
	std::set<FluffiBasicBlock>* blocksCoveredSinceLastReset, std::map<uint64_t, GDBBreakpoint>* allBreakpoints, int bpInstrBytes) {
	while (gDBThreadCommunication->get_gdbOutputQueue_size() > 0) {
		std::string line = gDBThreadCommunication->gdbOutputQueue_pop_front();
		if (line.find_first_not_of(" \t\f\v\n\r", 0) != std::string::npos) {
			if (line.substr(0, 23) == "Program received signal") {
				if (!handleSignal(gDBThreadCommunication, line, allBreakpoints, blocksCoveredSinceLastReset, bpInstrBytes)) {
					gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
					gDBThreadCommunication->m_exOutput.m_terminationDescription = "consumeGDBOutputQueue: Failed to handle a signal";
					LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
					return false;
				}
			}
			else if (line.find("exited with code") != std::string::npos || line.substr(0, 24) == "Program exited normally.") {
				size_t exitCodePos = line.find("exited with code");
				int exitCode = 0;

				if (exitCodePos != std::string::npos) {
					try {
						exitCode = std::stoi(line.c_str() + exitCodePos + 17, 0, 0x10);
					}
					catch (...) {
						exitCode = 0;
						LOG(WARNING) << "consumeGDBOutputQueue: std::stoi on process exit code failed";
					}
				}

				gDBThreadCommunication->set_exitStatus(exitCode);

				//If no exception was observed: Set exit type to normal
				if (gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN) {
					gDBThreadCommunication->m_exOutput.m_terminationDescription = "Process terminated normally: " + std::to_string(exitCode);
				}
				return false;
			}

			else if (line.substr(0, 10) == "Continuing" || line.find("New Thread") != std::string::npos || line.find("Switching to Thread") != std::string::npos) {
				//These are well known and can be ignored
			}
			else {
				LOG(DEBUG) << "Unexpected message from gdb:" << line;
			}
		}
	}

	return true;
}

std::string TestExecutorGDB::generateEnableAllCommand(std::map<uint64_t, GDBBreakpoint>& allBreakpoints, uint32_t bpInstr, int bpInstrBytes) {
	std::stringstream oss;

	typedef std::pair<const uint64_t, GDBBreakpoint> breakpointPair;
	for (breakpointPair& it : allBreakpoints)
	{
		if (!it.second.m_isEnabled) {
			//It will be enabled now
			it.second.m_isEnabled = true;

			switch (bpInstrBytes)
			{
			case 1:
				oss << "set {unsigned char}0x" << std::hex << it.second.m_absoluteAdress << " = 0x" << std::hex << bpInstr << std::endl;
				break;
			case 2:
				oss << "set {unsigned short}0x" << std::hex << it.second.m_absoluteAdress << " = 0x" << std::hex << bpInstr << std::endl;
				break;
			case 4:
				oss << "set {unsigned int}0x" << std::hex << it.second.m_absoluteAdress << " = 0x" << std::hex << bpInstr << std::endl;
				break;
			default:
				LOG(ERROR) << "Reached a code path in generateEnableAllCommand, that should be unreachable";
				google::protobuf::ShutdownProtobufLibrary();
				_exit(EXIT_FAILURE); //make compiler happy
			}
		}
	}

	return oss.str();
}

void TestExecutorGDB::gdbDebug(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, const std::set<Module> modulesToCover,
	const std::set<FluffiBasicBlock> blocksToCover, uint32_t bpInstr, int bpInstrBytes) {
	std::map<uint64_t, GDBBreakpoint> allBreakpoints;
	if (!gdbPrepareForDebug(gDBThreadCommunication, modulesToCover, blocksToCover, &allBreakpoints, bpInstrBytes)) {
		return;
	}

	*gDBThreadCommunication->m_ostreamToGdb << "c" << std::endl;

	gDBThreadCommunication->set_debuggingReady();

	//So far there is no exception
	gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN;
	gDBThreadCommunication->m_exOutput.m_terminationDescription = "The target executed without any exception";

	std::set<FluffiBasicBlock> blocksCoveredSinceLastReset;
	GDBThreadCommunication::COVERAGE_STATE statesToWaitFor[] = { GDBThreadCommunication::COVERAGE_STATE::SHOULD_DUMP, GDBThreadCommunication::COVERAGE_STATE::SHOULD_RESET_HARD, GDBThreadCommunication::COVERAGE_STATE::SHOULD_RESET_SOFT };
	while (true) {
		gDBThreadCommunication->waitForTerminateMessageOrCovStateChange(statesToWaitFor, 2);

		//Handle gdbThreadShouldTerminate
		if (gDBThreadCommunication->get_gdbThreadShouldTerminate()) {
			return;
		}

		//Handle gdbOutputQueue
		if (!consumeGDBOutputQueue(gDBThreadCommunication, &blocksCoveredSinceLastReset, &allBreakpoints, bpInstrBytes)) {
			return;
		}

		//Handle coverageState
		GDBThreadCommunication::COVERAGE_STATE cstate = gDBThreadCommunication->get_coverageState();
		switch (cstate) {
		case GDBThreadCommunication::COVERAGE_STATE::SHOULD_RESET_HARD: //Reset coverage to NULL and enable all breakpoints
		{
			std::string enableAllCommand = generateEnableAllCommand(allBreakpoints, bpInstr, bpInstrBytes);

			//As there might be leftover commands we might need to clean that first
			std::string resp;
			bool noProblem = false;
			while (true) {
				noProblem = sendCommandToGDBAndWaitForResponse(enableAllCommand, &resp, gDBThreadCommunication, true);
				if (noProblem) {
					break;
				}
				else if (resp == "LEFTOVER") {
					//Handle gdbOutputQueue
					if (!consumeGDBOutputQueue(gDBThreadCommunication, &blocksCoveredSinceLastReset, &allBreakpoints, bpInstrBytes)) {
						return;
					}
					continue;
				}
				break;
			}
			if (!noProblem) {
				gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
				gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbDebug: Failed enabling all breakpoints";
				LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
				return;
			}

			blocksCoveredSinceLastReset.clear();

			*gDBThreadCommunication->m_ostreamToGdb << "c" << std::endl;

			gDBThreadCommunication->set_coverageState(GDBThreadCommunication::COVERAGE_STATE::RESETTED);

			break;
		}
		case GDBThreadCommunication::COVERAGE_STATE::SHOULD_RESET_SOFT://Reset coverage to NULL
		{
			blocksCoveredSinceLastReset.clear();

			gDBThreadCommunication->set_coverageState(GDBThreadCommunication::COVERAGE_STATE::RESETTED);

			break;
		}
		case GDBThreadCommunication::COVERAGE_STATE::SHOULD_DUMP:
		{
			if (gDBThreadCommunication->m_exOutput.getCoveredBasicBlocks().size() > 0) {
				LOG(WARNING) << "gDBThreadCommunication->m_exOutput.getCoveredBasicBlocks was not empty. This should not happen!";
				DebugExecutionOutput t = DebugExecutionOutput{};
				gDBThreadCommunication->m_exOutput.swapBlockVectorWith(t);
			}

			for (const FluffiBasicBlock& blockIt : blocksCoveredSinceLastReset) {
				gDBThreadCommunication->m_exOutput.addCoveredBasicBlock(blockIt);
			}

			gDBThreadCommunication->set_coverageState(GDBThreadCommunication::COVERAGE_STATE::DUMPED);

			break;
		}
		default:
			break;
		}
	}
}

bool TestExecutorGDB::gdbPrepareForDebug(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, const std::set<Module> modulesToCover,
	const std::set<FluffiBasicBlock> blocksToCover, std::map<uint64_t, GDBBreakpoint>* allBreakpoints, int bpInstrBytes) {
	if (gDBThreadCommunication->get_gdbOutputQueue_size() != 0) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "GDB initialization failed: we got a message when we did not expect it:" + gDBThreadCommunication->gdbOutputQueue_pop_front();
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}

	std::string resp;
	//Check, if the gdb is waiting for us (we attached)
	bool noProblem = sendCommandToGDBAndWaitForResponse("i r", &resp, gDBThreadCommunication);
	if (!noProblem) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: first \"i r\" failed";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}

	if (resp.find("no registers now") != std::string::npos) {
		//If the GDB did not wait for us, try to start the target
		noProblem = sendCommandToGDBAndWaitForResponse("br *0x0", &resp, gDBThreadCommunication);
		if (!noProblem) {
			gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: \"br *0x0\" failed";
			LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
			return false;
		}

		int numOfDummyBreakpoint;
		try {
			numOfDummyBreakpoint = std::stoi(resp.c_str() + 11);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi of " << (resp.c_str() + 11) << " failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}

		//Program will load but complain that it cannot set fake breakpont at 0 - we got a breakpoint at system entry
		noProblem = sendCommandToGDBAndWaitForResponse("run", &resp, gDBThreadCommunication);
		if (!noProblem) {
			gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: \"run\" failed";
			LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
			return false;
		}

		noProblem = sendCommandToGDBAndWaitForResponse("d br " + std::to_string(numOfDummyBreakpoint), &resp, gDBThreadCommunication);
		if (!noProblem) {
			gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: deleting 0 breakpoint failed";
			LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
			return false;
		}

		//check if we the program is now loaded
		noProblem = sendCommandToGDBAndWaitForResponse("i r", &resp, gDBThreadCommunication);
		if (!noProblem) {
			gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: second \"i r\" failed";
			LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
			return false;
		}
	}

	if (resp.find("rax") == std::string::npos && resp.find("eax") == std::string::npos && resp.find("r0") == std::string::npos) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: The gdb command \"i r\" does not print registers ";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}

	//get a list of all loaded files and their addresses
	std::map<int, uint64_t> baseMap;
	std::string infoFilesResp;
	if (!sendCommandToGDBAndWaitForResponse("info files", &infoFilesResp, gDBThreadCommunication)) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: \"info files\" failed";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}
	if (!getBaseAddressesForModules(&baseMap, modulesToCover, infoFilesResp)) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: getBaseAddressesForModules failed ";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}

	if (baseMap.size() != modulesToCover.size()) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: It was not possible to determine the addresses of all breakpoints to set (Dynamic module loading is currently not supported)";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}

	//Get the target's PID (if possible)
	noProblem = sendCommandToGDBAndWaitForResponse("info inferior", &resp, gDBThreadCommunication);
	if (!noProblem) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: \"info inferior\" failed";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}
	else {
		size_t isPos = resp.find("process");
		if (isPos != std::string::npos) {
			try {
				gDBThreadCommunication->m_exOutput.m_PID = std::stoi(resp.c_str() + isPos + 7);
			}
			catch (...) {
				LOG(ERROR) << "std::stoi of " << (resp.c_str() + isPos + 7) << " failed";
				google::protobuf::ShutdownProtobufLibrary();
				_exit(EXIT_FAILURE); //make compiler happy
			}
		}
	}

	//Fill the allBreakpoints map: 1 - Generate the statement to get all "real" bytes where we will set the breakpoints
	std::stringstream breakpointsToSetSS;
	bool isFirstBreak = true;
	for (const FluffiBasicBlock& basicBlockIt : blocksToCover) {
		if (isFirstBreak) {
			isFirstBreak = false;
		}
		else {
			breakpointsToSetSS << std::endl;
		}
		switch (bpInstrBytes)
		{
		case 1:
			breakpointsToSetSS << "x/1xb 0x" << std::hex << basicBlockIt.m_rva + baseMap[basicBlockIt.m_moduleID];
			break;
		case 2:
			breakpointsToSetSS << "x/1xh 0x" << std::hex << basicBlockIt.m_rva + baseMap[basicBlockIt.m_moduleID];
			break;
		case 4:
			breakpointsToSetSS << "x/1xw 0x" << std::hex << basicBlockIt.m_rva + baseMap[basicBlockIt.m_moduleID];
			break;
		default:
			LOG(ERROR) << "Reached a code path in gdbPrepareForDebug, that should be unreachable";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}
	}
	noProblem = sendCommandToGDBAndWaitForResponse(breakpointsToSetSS.str(), &resp, gDBThreadCommunication);
	if (!noProblem) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: getting all real breakpoint bytes failed";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}

	//Fill the allBreakpoints map: 2 - Store which "breakpoint_bytes" are at which address, and belongs to which Fluffi basic block
	allBreakpoints->clear();
	std::vector<std::string> lines = Util::splitString(resp, "\n");
	std::set<FluffiBasicBlock>::iterator fbbIt = blocksToCover.begin();
	for (std::string& linesIt : lines) {
		try {
			uint64_t absAddress = (*fbbIt).m_rva + baseMap[(*fbbIt).m_moduleID];
			if (std::stoull(linesIt.c_str(), 0, 16) == absAddress && linesIt.find_first_of(":") != std::string::npos) {
				allBreakpoints->insert(std::pair<uint64_t, GDBBreakpoint>(absAddress, GDBBreakpoint(absAddress, static_cast<unsigned int>(std::stoul(linesIt.c_str() + linesIt.find_first_of(":") + 1, 0, 16)), *fbbIt)));

				if (fbbIt == blocksToCover.end()) {
					break;
				}
				else {
					++fbbIt;
				}
			}
			else {
				LOG(DEBUG) << "Read line \"" << linesIt << "\" when parsing the getting-all-real-breakpoint-bytes output.";
				continue;
			}
		}
		catch (...) {
			LOG(ERROR) << "std::stoi/stoull of " << linesIt << " failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}
	}

	if (allBreakpoints->size() != blocksToCover.size()) {
		gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbPrepareForDebug: Failed getting all real breakpoint bytes - at least one could not be read";
		LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
		return false;
	}

	return true;
}

bool TestExecutorGDB::getBaseAddressesForModules(std::map<int, uint64_t>* modmap, const std::set<Module> modulesToCover, std::string parseInfoOutput) {
	//Check for the NULL module
	for (std::set<Module>::iterator it = modulesToCover.begin(); it != modulesToCover.end(); ++it) {
		if ((*it).m_modulename == "" || (*it).m_modulename == "NULL" || (*it).m_modulename == "0") {
			modmap->insert(std::pair<int, uint64_t>((*it).m_moduleid, 0));
		}
	}

	std::vector<tModuleInformation>  loadedFiles;
	if (!parseInfoFiles(loadedFiles, parseInfoOutput)) {
		return false;
	}

	for (std::vector<tModuleInformation>::iterator loadedFilesIT = loadedFiles.begin(); loadedFilesIT != loadedFiles.end(); ++loadedFilesIT) {
		//Check, if this is something, we are interested in
		for (std::set<Module>::iterator mod2CoverIT = modulesToCover.begin(); mod2CoverIT != modulesToCover.end(); ++mod2CoverIT) {
			if ((*mod2CoverIT).m_modulename == std::get<2>((*loadedFilesIT)) + "/" + std::get<3>((*loadedFilesIT)) && ((*mod2CoverIT).m_modulepath == "*" || (*mod2CoverIT).m_modulepath == std::get<4>((*loadedFilesIT)))) {
				modmap->insert(std::pair<int, uint64_t>((*mod2CoverIT).m_moduleid, std::get<0>((*loadedFilesIT))));
			}
		}
	}

	return true;
}

bool TestExecutorGDB::getBaseAddressesAndSizes(std::vector<tModuleAddressesAndSizes>& outBaseAddressesAndSizes, std::string parseInfoOutput) {
	// start, end, name, segmentname, path
	std::vector<tModuleInformation> loadedFiles;

	if (!parseInfoFiles(loadedFiles, parseInfoOutput)) {
		return false;
	}

	using std::get;
	for (tModuleInformation& loadedFilesIT : loadedFiles) {
		outBaseAddressesAndSizes.push_back(
			std::make_tuple(
				get<2>(loadedFilesIT) + "/" + get<3>(loadedFilesIT),
				get<0>(loadedFilesIT),
				get<1>(loadedFilesIT) - get<0>(loadedFilesIT)
			)
		);
	}

	return true;
}

bool TestExecutorGDB::parseInfoFiles(std::vector<tModuleInformation>& loadedFiles, std::string infoFilesResp) {
	std::vector<std::string> lines = Util::splitString(infoFilesResp, "\n");

	bool reachedMainPart = false;
	std::string procName = "";
	std::string procPath = "";
	for (std::string& lineIt : lines) {
		//Skip all lines until the list of files
		if (!reachedMainPart) {
			if (lineIt.substr(0, 5) == "Local") {
				reachedMainPart = true;
			}
			continue;
		}

		//Is this the line that contains the file name? If yes: extract main module name and path
		if (lineIt.find("file type") != std::string::npos) {
			size_t start = lineIt.find_first_of(0x60, 0);
			size_t end = lineIt.find_first_of(0x27, 0);
			if (start != std::string::npos && end != std::string::npos) {
				std::string mainPathAndName = lineIt.substr(start + 1, end - start - 1);
				std::tuple<std::string, std::string> directoryAndFilename = Util::splitPathIntoDirectoryAndFileName(mainPathAndName);
				procName = std::get<1>(directoryAndFilename);
				procPath = std::get<0>(directoryAndFilename);
			}
			continue;
		}

		//Parse each line
		size_t actualLineStart = lineIt.find_first_not_of(" \t\f\v\n\r", 0);
		size_t actualLineEnd = lineIt.find_last_not_of(" \t\f\v\n\r", lineIt.length());
		if (actualLineStart == std::string::npos || actualLineEnd == std::string::npos) {
			continue;
		}
		std::string trimmedLine = lineIt.substr(actualLineStart, actualLineEnd - actualLineStart + 1);
		size_t isPos = trimmedLine.find("is ");
		if (isPos != std::string::npos) {
			std::string afterIs = trimmedLine.substr(isPos + 3);
			size_t firstSpaceAfterIs = afterIs.find_first_of(' ', 0);
			if (firstSpaceAfterIs == std::string::npos) {
				firstSpaceAfterIs = afterIs.length();
			}
			std::string currentSegmentName = afterIs.substr(0, firstSpaceAfterIs);

			std::string currentName = "";
			std::string currentPath = "";
			size_t inPos = afterIs.find("in ");
			if (inPos != std::string::npos) {
				std::string afterIn = afterIs.substr(inPos + 3);
				std::tuple<std::string, std::string> currentDirectoryAndFilename = Util::splitPathIntoDirectoryAndFileName(afterIn);
				currentName = std::get<1>(currentDirectoryAndFilename);
				currentPath = std::get<0>(currentDirectoryAndFilename);
			}
			else {
				currentName = procName;
				currentPath = procPath;
			}

			uint64_t currentStartAddress, currentEndAddress;
			try {
				currentStartAddress = std::stoull(trimmedLine, NULL, 16);
				currentEndAddress = std::stoull(trimmedLine.substr(trimmedLine.find("-") + 1), NULL, 16);
			}
			catch (...) {
				LOG(ERROR) << "std::stoi/stoull of " << trimmedLine << ", or of " << trimmedLine.substr(trimmedLine.find("-") + 1) << " failed";
				google::protobuf::ShutdownProtobufLibrary();
				_exit(EXIT_FAILURE); //make compiler happy
			}

			loadedFiles.push_back(std::make_tuple(currentStartAddress,
				currentEndAddress,
				currentName,
				currentSegmentName,
				currentPath));
		}
	}

	return true;
}

void TestExecutorGDB::gdbLinereaderThread(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, HANDLETYPE inputHandleFromGdb, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode) {
	std::vector<char> charsReadButNotReturned;

#if defined(_WIN32) || defined(_WIN64)
	DWORD totalBytesAvail = 0;
	DWORD bytesRead = 0;
	while (!gDBThreadCommunication->get_gdbThreadShouldTerminate() && 0 != PeekNamedPipe(inputHandleFromGdb, NULL, NULL, NULL, &totalBytesAvail, NULL)) {
		if (totalBytesAvail == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		char* buff = new char[totalBytesAvail];
		ReadFile(inputHandleFromGdb, buff, totalBytesAvail, &bytesRead, NULL);
		if (totalBytesAvail != bytesRead) {
#else
	int nbytes = 0;
	while (!gDBThreadCommunication->get_gdbThreadShouldTerminate() && -1 != ioctl(inputHandleFromGdb, FIONREAD, &nbytes)) {
		if (nbytes == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		char* buff = new char[nbytes];
		ssize_t bytesRead = read(inputHandleFromGdb, buff, nbytes);
		if (bytesRead != nbytes) {
#endif
			gDBThreadCommunication->m_exOutput.m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			gDBThreadCommunication->m_exOutput.m_terminationDescription = "gdbLinereaderThread: Failed reading from gdb output pipe";
			gDBThreadCommunication->set_gdbThreadShouldTerminate();
			LOG(ERROR) << gDBThreadCommunication->m_exOutput.m_terminationDescription;
			return;
		}

		charsReadButNotReturned.insert(charsReadButNotReturned.end(), buff, buff + bytesRead);
		delete[] buff;

		while (true) {
			auto newlinePos = std::find(charsReadButNotReturned.begin(), charsReadButNotReturned.end(), '\n');
			if (newlinePos != charsReadButNotReturned.end()) {
				/* charsReadButNotReturned contains newline */

				std::string line(charsReadButNotReturned.begin(), newlinePos);

				charsReadButNotReturned.erase(charsReadButNotReturned.begin(), newlinePos + 1);

				//delete the "(gdb) ". It seems to stem from a different stream and is more confusing than helping
				while (true) {
					std::string::size_type gdbPos = line.find("(gdb) ");
					if (gdbPos == std::string::npos) {
						break;
					}
					line.erase(gdbPos, 6);
				}

				if (child_output_mode == ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT) {
					//Simulate external program's "white" output
					printf("%s\n", line.c_str());
				}

				gDBThreadCommunication->gdbOutputQueue_push_back(line);
			}
			else {
				/* charsReadButNotReturned DOES NOT contain newline */
				break;
			}
		}
	}

	LOG(DEBUG) << "gdbLinereaderThread terminating";

	gDBThreadCommunication->set_gdbThreadShouldTerminate();
	return;
}

bool TestExecutorGDB::runSingleTestcase(const FluffiTestcaseID testcaseId, std::shared_ptr<DebugExecutionOutput> exResult, CoverageMode covMode) {
	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(m_hangTimeoutMS);
	LOG(DEBUG) << "runSingleTestcase:" << testcaseId << " in covMode " << ((covMode == CoverageMode::FULL) ? "FULL" : ((covMode == CoverageMode::NONE) ? "NONE" : "PARTIAL"));

	if (covMode != CoverageMode::NONE) {
		//reset coverage
		m_gDBThreadCommunication->set_coverageState((covMode == CoverageMode::FULL) ? GDBThreadCommunication::COVERAGE_STATE::SHOULD_RESET_HARD : GDBThreadCommunication::COVERAGE_STATE::SHOULD_RESET_SOFT);
		if (!TestExecutorGDB::waitUntilCoverageState(m_gDBThreadCommunication, GDBThreadCommunication::COVERAGE_STATE::RESETTED, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()))) {
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exResult->m_terminationDescription = "Problem while resetting the coverage (a timeout occured): Increase hangTimeout!";
			return false;
		}
		LOG(DEBUG) << "The coverage state RESETTED was reached";
	}

	//send "go" to feeder
	std::string testcaseFile = Util::generateTestcasePathAndFilename(testcaseId, m_testcaseDir);
	SharedMemMessage fuzzFilenameMsg{ SHARED_MEM_MESSAGE_FUZZ_FILENAME, testcaseFile.c_str(),static_cast<int>(testcaseFile.length()) };
	if (!m_sharedMemIPC_toFeeder->sendMessageToClient(&fuzzFilenameMsg)) {
		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		exResult->m_terminationDescription = "Problem while sending a fuzzing filename to the feeder!";
		return false;
	}

	//wait for "done" from feeder
	SharedMemMessage responseFromFeeder;
#if defined(_WIN32) || defined(_WIN64)
	m_sharedMemIPC_toFeeder->waitForNewMessageToServer(&responseFromFeeder, static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()), m_SharedMemIPCInterruptEvent);
#else
	m_sharedMemIPC_toFeeder->waitForNewMessageToServer(&responseFromFeeder, static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()), m_SharedMemIPCInterruptFD[0]);
#endif
	if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_FUZZ_DONE) {
		//feeder successfully forwarded the test case to the target, which responed in time
	}
	else if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_WAIT_INTERRUPTED) {
		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		exResult->m_terminationDescription = "Communication with feeder was interrupted!";
		LOG(DEBUG) << exResult->m_terminationDescription;
		return false;
	}
	else if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::TIMEOUT;
		exResult->m_terminationDescription = "Communication with feeder timed out!";
		return false;
	}
	else if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_TARGET_CRASHED) {
		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
		exResult->m_terminationDescription = "Feeder claims that target crashed!";
		exResult->m_firstCrash = "FEEDER CLAIMS CRASH";
		exResult->m_lastCrash = exResult->m_firstCrash;
		return true;
	}
	else {
		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		exResult->m_terminationDescription = "Feeder returned a not expected message. We received a message of type " + std::to_string(responseFromFeeder.getMessageType());
		return false;
	}

	if (covMode != CoverageMode::NONE) {
		//get coverage
		m_gDBThreadCommunication->set_coverageState(GDBThreadCommunication::COVERAGE_STATE::SHOULD_DUMP);
		if (!TestExecutorGDB::waitUntilCoverageState(m_gDBThreadCommunication, GDBThreadCommunication::COVERAGE_STATE::DUMPED, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()))) {
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exResult->m_terminationDescription = "Problem while dumping the coverage!";
			return false;
		}
		LOG(DEBUG) << "The coverage state DUMPED was reached";

		//Move coverage to m_gDBThreadCommunication execution result (for compatibility of the execution function with dynrio multi runner - allows better code reusability)
		m_gDBThreadCommunication->m_exOutput.swapBlockVectorWith(*exResult);
	}

	exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN;
	exResult->m_terminationDescription = "Fuzzcase executed normally!";

	return true;
}

void TestExecutorGDB::waitForDebuggerToTerminate() {
	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> timestampToKillTheTarget = routineEntryTimeStamp + std::chrono::milliseconds(m_hangTimeoutMS);
	bool targetKilled = false;
	LOG(DEBUG) << "Waiting for the debugger thread to terminate.";
	while (!m_gDBThreadCommunication->m_exOutput.m_debuggerThreadDone) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (!targetKilled && std::chrono::steady_clock::now() > timestampToKillTheTarget) {
			LOG(DEBUG) << "Waiting sucks - killing the target.";
			m_gDBThreadCommunication->set_gdbThreadShouldTerminate(); //In case the crash is not reproducible, terminate gdb to avoid race conditions. This should also kill the target.
			targetKilled = true;
		}
	}
}
