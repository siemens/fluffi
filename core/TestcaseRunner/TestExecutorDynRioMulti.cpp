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
#include "TestExecutorDynRioMulti.h"
#include "ExternalProcess.h"
#include "FluffiTestcaseID.h"
#include "Util.h"
#include "DebugExecutionOutput.h"
#include "GarbageCollectorWorker.h"
#include "CommInt.h"

#if defined(_WIN32) || defined(_WIN64)
TestExecutorDynRioMulti::TestExecutorDynRioMulti(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const std::string additionalEnvParam, GarbageCollectorWorker* garbageCollectorWorker, bool treatAnyAccessViolationAsFatal, const std::string  feederCmdline, const std::string  starterCmdline, int initializationTimeoutMS, CommInt* commInt, bool target_forks, int forceRestartAfterXTCs)
	: TestExecutorDynRio(targetCMDline, hangTimeoutMS, modulesToCover, testcaseDir, child_output_mode, additionalEnvParam, garbageCollectorWorker, treatAnyAccessViolationAsFatal),
	m_target_and_feeder_okay(false),
	m_sharedMemIPC_toFeeder(nullptr),
	m_debuggeeProcess(std::make_shared<ExternalProcess>("", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS)),
	m_targetProcessID(0),
	m_dynrio_clientID(0),
	m_feederProcess(std::make_shared<ExternalProcess>("", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS)),
	m_exOutput_FROM_TARGET_DEBUGGING(std::make_shared<DebugExecutionOutput>()),
	m_initializationTimeoutMS(initializationTimeoutMS),
	m_iterationNumMod2(0),
	m_feederCmdline(feederCmdline),
	m_starterCmdline(starterCmdline),
	m_attachInsteadOfStart(!starterCmdline.empty()),
	m_commInt(commInt),
	m_dynrioPipeName(""),
	m_forceRestartAfterXTCs(forceRestartAfterXTCs),
	m_executionsSinceLastRestart(0),
	m_hdrconfiglib(NULL),
	m_dr_nudge_pid(nullptr),
	m_dr_register_process(nullptr),
	m_dr_unregister_process(nullptr),
	m_dr_register_client(nullptr),
	m_dr_process_is_registered(nullptr),
	m_dr_register_syswide(nullptr),
	m_dr_syswide_is_on(nullptr),
	m_pipe_to_dynrio(INVALID_HANDLE_VALUE),
	m_SharedMemIPCInterruptEvent(NULL)
{
	m_hdrconfiglib = LoadLibrary(DRCONFIGLIB_PATH);
	if (m_hdrconfiglib != NULL) {
		m_dr_nudge_pid = (dr_nudge_pid_func*)GetProcAddress(m_hdrconfiglib, "dr_nudge_pid");
		m_dr_register_process = (dr_register_process_func*)GetProcAddress(m_hdrconfiglib, "dr_register_process");
		m_dr_unregister_process = (dr_unregister_process_func*)GetProcAddress(m_hdrconfiglib, "dr_unregister_process");
		m_dr_register_client = (dr_register_client_func*)GetProcAddress(m_hdrconfiglib, "dr_register_client");
		m_dr_process_is_registered = (dr_process_is_registered_func*)GetProcAddress(m_hdrconfiglib, "dr_process_is_registered");
		m_dr_register_syswide = (dr_register_syswide_func*)GetProcAddress(m_hdrconfiglib, "dr_register_syswide");
		m_dr_syswide_is_on = (dr_syswide_is_on_func*)GetProcAddress(m_hdrconfiglib, "dr_syswide_is_on");
	}

	LOG(DEBUG) << "Setting the SharedMemIPCInterruptEvent to stop the current execution";
	m_SharedMemIPCInterruptEvent = CreateEvent(NULL, false, false, NULL);
	if (m_SharedMemIPCInterruptEvent == NULL) {
		LOG(ERROR) << "failed to create the SharedMemIPCInterruptEvent";
	}

	if (m_attachInsteadOfStart) {
		//Try setting the debug privilege. Doesn't hurt if we don't get it, but might be required (e.g. when attaching to a SYSTEM process)
		bool debugPriv = Util::enableDebugPrivilege();
		LOG(DEBUG) << "enableDebugPrivilege returned " << (debugPriv ? "true" : "false");

		//Try to remove registration for the target. It might be there due to a dirty exit
		std::string errormsg = "";
		bool dynRioReg = setDynamoRioRegistration(false, "", &errormsg);
		LOG(DEBUG) << "initial setDynamoRioRegistration(false) returned " << (dynRioReg ? "true" : "false") << "Error message: " << errormsg;
	}
}

TestExecutorDynRioMulti::~TestExecutorDynRioMulti()
{
	if (m_hdrconfiglib != NULL) {
		FreeLibrary(m_hdrconfiglib);
	}

	if (m_sharedMemIPC_toFeeder != nullptr) {
		delete m_sharedMemIPC_toFeeder;
		m_sharedMemIPC_toFeeder = nullptr;
	}

	if (m_pipe_to_dynrio != INVALID_HANDLE_VALUE) {
		CloseHandle(m_pipe_to_dynrio);
		m_pipe_to_dynrio = INVALID_HANDLE_VALUE;
	}

	m_debuggeeProcess->die(); //send kill to an hanging process. Doing so will cause the debugging thread do terminate

	if (m_SharedMemIPCInterruptEvent != NULL) {
		CloseHandle(m_SharedMemIPCInterruptEvent);
		m_SharedMemIPCInterruptEvent = NULL;
	}

	Util::markAllFilesOfTypeInPathForDeletion(m_testcaseDir, ".log", m_garbageCollectorWorker);

	waitForDebuggerToTerminate(); // Avoid ugly memleaks
}

std::string TestExecutorDynRioMulti::getDrCovOutputFile(int iterationNumMod2) {
	std::stringstream ss;
	ss << m_testcaseDir << Util::pathSeperator << "*." << std::setw(5) << std::setfill('0') << m_targetProcessID << "." << std::setw(4) << std::setfill('0') << iterationNumMod2 << ".proc.log";
	std::string targetFile = ss.str();

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(targetFile.c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		return "";
	}

	std::string re = m_testcaseDir + Util::pathSeperator + std::string(ffd.cFileName);

	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;

	return re;
}

#else
TestExecutorDynRioMulti::TestExecutorDynRioMulti(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const std::string additionalEnvParam, GarbageCollectorWorker* garbageCollectorWorker, bool treatAnyAccessViolationAsFatal, const std::string  feederCmdline, const std::string  starterCmdline, int initializationTimeoutMS, CommInt * commInt, bool target_forks, int forceRestartAfterXTCs)
	: TestExecutorDynRio(targetCMDline, hangTimeoutMS, modulesToCover, testcaseDir, child_output_mode, additionalEnvParam, garbageCollectorWorker, treatAnyAccessViolationAsFatal),
	m_target_and_feeder_okay(false),
	m_sharedMemIPC_toFeeder(nullptr),
	m_debuggeeProcess(std::make_shared<ExternalProcess>("", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS)),
	m_targetProcessID(0),
	m_dynrio_clientID(0),
	m_feederProcess(std::make_shared<ExternalProcess>("", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS)),
	m_exOutput_FROM_TARGET_DEBUGGING(std::make_shared<DebugExecutionOutput>()),
	m_initializationTimeoutMS(initializationTimeoutMS),
	m_iterationNumMod2(0),
	m_feederCmdline(feederCmdline),
	m_starterCmdline(starterCmdline),
	m_attachInsteadOfStart(!starterCmdline.empty()),
	m_commInt(commInt),
	m_dynrioPipeName(""),
	m_forceRestartAfterXTCs(forceRestartAfterXTCs),
	m_executionsSinceLastRestart(0),
	m_pipe_to_dynrio(-1),
	m_SharedMemIPCInterruptFD{ -1,-1 },
	m_target_forks(target_forks)

{
	LOG(DEBUG) << "Creating the SharedMemIPCInterruptFD to stop the current execution";
	if (pipe(m_SharedMemIPCInterruptFD) == -1) {
		LOG(ERROR) << "failed to create the SharedMemIPCInterruptFD";
	}

	if (m_attachInsteadOfStart) {
		//make sure /proc/sys/kernel/yama/ptrace_scope is set to 0
		bool debugPriv = Util::enableDebugPrivilege();
		LOG(DEBUG) << "enableDebugPrivilege returned " << (debugPriv ? "true" : "false");

		//Try to remove registration for the target. It might be there due to a dirty exit
		std::string errormsg = "";
		bool dynRioReg = setDynamoRioRegistration(false, "", &errormsg);
		LOG(DEBUG) << "initial setDynamoRioRegistration(false) returned " << (dynRioReg ? "true" : "false");
	}
}

TestExecutorDynRioMulti::~TestExecutorDynRioMulti()
{
	if (m_sharedMemIPC_toFeeder != nullptr) {
		delete m_sharedMemIPC_toFeeder;
		m_sharedMemIPC_toFeeder = nullptr;
	}

	if (m_pipe_to_dynrio != -1) {
		close(m_pipe_to_dynrio);
		m_pipe_to_dynrio = -1;
	}

	if (m_dynrioPipeName != "") {
		unlink(m_dynrioPipeName.c_str());
		m_dynrioPipeName = "";
	}

	m_debuggeeProcess->die(); //send kill to an hanging process. Doing so will cause the debugging thread do terminate

	//Try closing the interrupt pipe
	close(m_SharedMemIPCInterruptFD[0]);
	m_SharedMemIPCInterruptFD[0] = -1;
	close(m_SharedMemIPCInterruptFD[1]);
	m_SharedMemIPCInterruptFD[1] = -1;

	Util::markAllFilesOfTypeInPathForDeletion(m_testcaseDir, ".log", m_garbageCollectorWorker);

	waitForDebuggerToTerminate(); // Avoid ugly memleaks
}

std::string TestExecutorDynRioMulti::getDrCovOutputFile(int iterationNumMod2) {
	std::stringstream ss;
	ss << "." << std::setw(5) << std::setfill('0') << m_targetProcessID << "." << std::setw(4) << std::setfill('0') << iterationNumMod2 << ".proc.log";
	std::string targetFileEnd = ss.str();

	struct dirent* dp;
	DIR* dirp = opendir(m_testcaseDir.c_str());
	while ((dp = readdir(dirp)) != NULL)
		if (Util::stringHasEnding(dp->d_name, targetFileEnd)) {
			std::string re = m_testcaseDir + Util::pathSeperator + dp->d_name;
			closedir(dirp);
			return re;
		}
	closedir(dirp);
	return "";
}

dr_config_status_t TestExecutorDynRioMulti::m_dr_nudge_pid(process_id_t process_id, client_id_t client_id, uint64_t client_arg, uint timeout_ms) {
	(void)(timeout_ms); //avoid unused parameter warning: timeout_ms cannot be used given how nudging is implemented on unix

	/* construct the payload */
	nudge_arg_t* arg;
	siginfo_t info;

	memset(&info, 0, sizeof(info));
	info.si_signo = NUDGESIG_SIGNUM;
	info.si_code = SI_QUEUE;

	arg = reinterpret_cast<nudge_arg_t*>(&info);
	arg->version = NUDGE_ARG_CURRENT_VERSION;
	arg->nudge_action_mask = NUDGE_GENERIC(client);
	arg->flags = 0;
	arg->client_id = client_id;
	arg->client_arg = client_arg;

	/* ensure nudge_arg_t overlays how we expect it to */
	if (info.si_signo != NUDGESIG_SIGNUM || info.si_code != SI_QUEUE) {
		LOG(ERROR) << "nudge FAILED. It looks like the kernel's sigqueueinfo has changed ";
		return DR_FAILURE;
	}

	/* send the nudge */
	long ret = syscall(SYS_rt_sigqueueinfo, process_id, NUDGESIG_SIGNUM, &info);
	if (ret < 0) {
		LOG(ERROR) << "nudge FAILED with error " << ret << "(errno: " << errno << ")";
		return DR_FAILURE;
	}

	return DR_SUCCESS;
}

#endif

bool TestExecutorDynRioMulti::attemptStartTargetAndFeeder(bool use_dyn_rio) {
	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(m_initializationTimeoutMS);
	LOG(DEBUG) << "Attempting to start target and feeder";

	/*Design decission: Target is ALWAYS started before feeder.
	- feeder can be told the target's PID
	- from a feeder's perspective the target always looks like an already running server (no matter what it actually is)
	*/

	m_executionsSinceLastRestart = 0;

	//Target part -------------------------------------------------

	m_debuggeeProcess->die(); //send kill to an hanging process. Doing so will cause the debugging thread do terminate
	m_exOutput_FROM_TARGET_DEBUGGING = std::make_shared<DebugExecutionOutput>(); //Reset debug output

	//Mark all .log files for deletion and wait until the log file with iteration num 0 is deleted (dynamo rio needs to create this file!)
	Util::markAllFilesOfTypeInPathForDeletion(m_testcaseDir, ".log", m_garbageCollectorWorker);
	m_garbageCollectorWorker->collectNow();

	if (use_dyn_rio) {
		m_iterationNumMod2 = 0;
		std::string nextRunDrcovLogFile = getDrCovOutputFile(m_iterationNumMod2);
		int deletionAttemtps = 0;
		while (std::experimental::filesystem::exists(nextRunDrcovLogFile) && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
			//It needs to be ensured, that the first dynamorio log file does not yet exist.
			if (deletionAttemtps > 0) {
				LOG(WARNING) << "Performance warning: TestExecurotDynRioMulti is waiting for the GarbageCollector (1). We are waiting for the deletion of " << nextRunDrcovLogFile;
			}
			deletionAttemtps++;
			m_garbageCollectorWorker->collectNow(); //Dear garbage collctor: Stop sleeping and try deleting files!
			m_garbageCollectorWorker->collectGarbage(); //This is threadsafe. As the garbage collector could not yet delete the file yet, let's help him!
		}
		if (std::experimental::filesystem::exists(nextRunDrcovLogFile)) {
			LOG(ERROR) << "Could not delete the initial drcov file in time!";
			return false;
		}
	}

	std::string cmdlineWithReplacements = m_targetCMDline;
	if (cmdlineWithReplacements.find("<RANDOM_SHAREDMEM>") != std::string::npos) {
#if defined(_WIN32) || defined(_WIN64)
		std::string targetSharedMemName = "FLUFFI_SharedMem_" + Util::newGUID();
#else
		std::string targetSharedMemName = "/FLUFFI_SharedMem_" + Util::newGUID();
#endif
		Util::replaceAll(cmdlineWithReplacements, "<RANDOM_SHAREDMEM>", targetSharedMemName);
	}
	if (cmdlineWithReplacements.find("<RANDOM_PORT>") != std::string::npos) {
		Util::replaceAll(cmdlineWithReplacements, "<RANDOM_PORT>", std::to_string(m_commInt->getFreeListeningPort()));
	}

	if (use_dyn_rio) {
#if defined(_WIN32) || defined(_WIN64)
		if (m_pipe_to_dynrio != INVALID_HANDLE_VALUE) {
			CloseHandle(m_pipe_to_dynrio);
			m_pipe_to_dynrio = INVALID_HANDLE_VALUE;
		}

		m_dynrioPipeName = "\\\\.\\pipe\\drcovMulti_" + Util::newGUID();
		m_pipe_to_dynrio = CreateNamedPipe(
			m_dynrioPipeName.c_str(),                      // pipe name
			PIPE_ACCESS_DUPLEX,       // read/write access
			0,
			1,                        // max. instances
			512,                      // output buffer size
			512,                      // input buffer size
			m_hangTimeoutMS,                    // client time-out
			NULL);                    // default security attribute

		if (m_pipe_to_dynrio == INVALID_HANDLE_VALUE) {
			LOG(ERROR) << "Creating the communication pipe to dynamo rio failed: " << GetLastError();
			return false;
		}
#else
		if (m_pipe_to_dynrio != -1) {
			close(m_pipe_to_dynrio);
			m_pipe_to_dynrio = -1;
		}

		if (m_dynrioPipeName != "") {
			unlink(m_dynrioPipeName.c_str());
			m_dynrioPipeName = "";
		}

		m_dynrioPipeName = "/tmp/drcovMulti_" + Util::newGUID();
		if (mkfifo(m_dynrioPipeName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == -1) {
			m_dynrioPipeName = "";
			LOG(ERROR) << "Creating the communication pipe to dynamo rio failed: " << errno;
			return false;
		}

		m_pipe_to_dynrio = open(m_dynrioPipeName.c_str(), O_RDONLY | O_NONBLOCK);
		if (m_pipe_to_dynrio == -1) {
			unlink(m_dynrioPipeName.c_str());
			m_dynrioPipeName = "";
			LOG(ERROR) << "Opening the communication pipe to dynamo rio failed: " << errno;
			return false;
		}
#endif

		if (!m_attachInsteadOfStart) {
			cmdlineWithReplacements = std::string(DRRUN_PATH) + " -v -no_follow_children -t drcovMulti -dump_binary -logdir \"" + m_testcaseDir + "\" -fluffi_pipe " + m_dynrioPipeName + " -- " + cmdlineWithReplacements;
		}
	}

	if (m_attachInsteadOfStart) {
		//make sure that the dynamo rio registration is correct
		std::string errormsg = "";
		if (!setDynamoRioRegistration(use_dyn_rio, m_dynrioPipeName, &errormsg)) {
			LOG(ERROR) << "Failed to set the dynamo rio registration to " << use_dyn_rio << ". Error message: " << errormsg;
			return false;
		}

		//Start the target by starting the starter (starter is a class member, so the destructor is called when a new starter is created or on class destruct)
		std::shared_ptr<ExternalProcess> starterProcess = std::make_shared<ExternalProcess>(m_starterCmdline, m_child_output_mode);

#if defined(_WIN32) || defined(_WIN64)
		starterProcess->setAllowBreakAway(); //children of the starter process should run outside of the starter job. So we can attach to them as a target
#else
		//No need to set allow break away as on Linux we trace only the direct child anyway. It is free to create new processes as it likes
#endif
		if (starterProcess->initProcess()) {
			if (!starterProcess->runAndWaitForCompletion(static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()))) {
				LOG(ERROR) << "The starter process was not able to start the target in time";
				return false;
			}
		}
		else {
			LOG(ERROR) << "Could not initialize starter process";
			return false;
		}
	}

#if defined(_WIN32) || defined(_WIN64)
	//Appending a parameter to the Environment is currently not implemented in Windows, so additionalEnvParam is ignored
	m_debuggeeProcess = std::make_shared<ExternalProcess>(cmdlineWithReplacements, m_child_output_mode);
	std::thread debuggerThread(&TestExecutorDynRioMulti::debuggerThreadMain, m_exOutput_FROM_TARGET_DEBUGGING, m_debuggeeProcess, m_SharedMemIPCInterruptEvent, m_attachInsteadOfStart, m_treatAnyAccessViolationAsFatal);
#else
	m_debuggeeProcess = std::make_shared<ExternalProcess>(cmdlineWithReplacements, m_child_output_mode, m_additionalEnvParam, use_dyn_rio);
	std::thread debuggerThread(&TestExecutorDynRioMulti::debuggerThreadMain, m_exOutput_FROM_TARGET_DEBUGGING, m_debuggeeProcess, m_SharedMemIPCInterruptFD[1], m_attachInsteadOfStart, m_treatAnyAccessViolationAsFatal);
#endif
	debuggerThread.detach(); //we do not want to join this. It will terminate as soon as the debugee terminates
	LOG(DEBUG) << "started debugger thread";

	//We need to wait for the target's initialization even if dynamo rio is initialized as the target may be an attached-to target.
	if (!m_debuggeeProcess->waitUntilProcessIsBeingDebugged(static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()))) {
		LOG(ERROR) << "Failed to wait for the target's initialization.";
		return false;
	}

	if (use_dyn_rio) {
		if (!waitForDynRioInitialization(static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()))) {
			LOG(ERROR) << "Dynamo rio or the plugin drcovMulti could not be initialized in time";
			return false;
		}
	}

	if (!use_dyn_rio) {
		m_targetProcessID = m_debuggeeProcess->getProcessID();
	}
	else {
		//no need to set m_targetProcessID, as waitForDynRioInitialization sets it
		if (m_attachInsteadOfStart && m_targetProcessID != m_debuggeeProcess->getProcessID()) {
			LOG(ERROR) << "It looks like we attached to a different process than the one in which our drcovMulti plugin is loaded";
			return false;
		}
	}

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
	if (!initializeFeeder(m_feederProcess, m_sharedMemIPC_toFeeder, m_targetProcessID, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()))) {
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

bool TestExecutorDynRioMulti::setDynamoRioRegistration(bool active, const std::string dynrioPipeName, std::string* errormsg) {
#if defined(_WIN32) || defined(_WIN64)
	//get procNameAndPath from targetCMDLine
	std::wstring wtargetCMDLine = std::wstring(m_targetCMDline.begin(), m_targetCMDline.end());
	int numOfBlocks;
	LPWSTR* szArglist = CommandLineToArgvW(wtargetCMDLine.c_str(), &numOfBlocks); //for some reasons this does not exist for Ascii :(
	if (NULL == szArglist || numOfBlocks < 1) {
		*errormsg = "targetCMDLine  invalid";
		return false;
	}

	//get procName from procNameAndPath
	std::tuple<std::string, std::string> directoryAndFilename = Util::splitPathIntoDirectoryAndFileName(Util::wstring_to_utf8(szArglist[0]));
	std::string procName = std::get<1>(directoryAndFilename);

	LocalFree(szArglist);
#else
	//get procNameAndPath from targetCMDLine
	std::string tmp = m_targetCMDline;
	std::replace(tmp.begin(), tmp.end(), '<', '_'); // replace all '<' to '_' (handle <RANDOM_SHAREDMEM> et al)
	std::replace(tmp.begin(), tmp.end(), '>', '_'); // replace all '>' to '_' (handle <RANDOM_SHAREDMEM> et al)
	char** argv = ExternalProcess::split_commandline(tmp);
	if (argv == NULL || argv[0] == NULL) {
		*errormsg = "Splitting target command line \"" + tmp + "\" failed.";
		if (argv != NULL) {
			free(argv);
		}
		return false;
	}

	//get procName from procNameAndPath
	std::tuple<std::string, std::string> directoryAndFilename = Util::splitPathIntoDirectoryAndFileName(argv[0]);
	std::string procName = std::get<1>(directoryAndFilename);

	//free argv
	int i = 0;
	while (argv[i] != NULL) {
		free(argv[i]);
		i++;
	}
	free(argv);
#endif

	std::string dynrioPath = std::experimental::filesystem::current_path().string() + DYNRIO_DIR;

	if (active) {
#if defined(_WIN32) || defined(_WIN64)

		//If the process is registered try unregistering it first
		bool isRegistered = m_dr_process_is_registered(procName.c_str(), 0, false, DR_PLATFORM_DEFAULT, NULL, NULL, NULL, NULL);
		if (isRegistered) {
			m_dr_unregister_process(procName.c_str(), 0, false, DR_PLATFORM_DEFAULT);
		}

		dr_config_status_t status = m_dr_register_process(procName.c_str(), 0, false, dynrioPath.c_str(), DR_MODE_CODE_MANIPULATION, false, DR_PLATFORM_DEFAULT, "\"-no_follow_children\" \"-nop_initial_bblock\"");
		if (status != DR_SUCCESS) {
			*errormsg = "dr_register_process failed:" + std::to_string(status);
			return  false;
		}

		status = m_dr_register_client(procName.c_str(), 0, false, DR_PLATFORM_DEFAULT, 0, 0, (dynrioPath + Util::pathSeperator + "clients" + Util::pathSeperator + DYNRIO_LIBDIR + Util::pathSeperator + "release" + Util::pathSeperator + "drcovMulti.dll").c_str(), ("\"-dump_binary\" \"-logdir\" \"" + m_testcaseDir + "\" \"-fluffi_pipe\" \"" + dynrioPipeName + "\"").c_str());
		if (status != DR_SUCCESS) {
			*errormsg = "dr_register_client failed:" + std::to_string(status);
			return false;
		}

		isRegistered = m_dr_process_is_registered(procName.c_str(), 0, false, DR_PLATFORM_DEFAULT, NULL, NULL, NULL, NULL);
		if (!isRegistered) {
			*errormsg = "We tried to register the process with dynamo rio but failed!";
			return false;
		}
#else
		//unfortunately, it appears to be impossible to compile drconfiglib as a static ".so". as drconfig is static, and we want to support static dynamorio, we use it for now
		std::string fluffi_fork_param = (m_target_forks) ? " \"-fluffi_fork\"" : "";
		std::string drconfigParams = " -reg " + procName + " -root \"" + dynrioPath + "\" -ops \"-no_follow_children\" -ops \"-nop_initial_bblock\" -c \"" + (dynrioPath + Util::pathSeperator + "clients" + Util::pathSeperator + DYNRIO_LIBDIR + Util::pathSeperator + "release" + Util::pathSeperator + "libdrcovMulti.so") + "\" " + ("\"-dump_binary\" \"-logdir\" \"" + m_testcaseDir + "\"" + fluffi_fork_param + " \"-fluffi_pipe\" \"" + dynrioPipeName + "\"");
		ExternalProcess ep(std::string(DRCONFIG_PATH) + drconfigParams, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
		if (ep.initProcess()) {
			ep.runAndWaitForCompletion(1000);
			if (ep.getExitStatus() != 0) {
				*errormsg = "drconfig -reg failed!";
				return false;
			}
		}
		else {
			*errormsg = "could not initialize \"drconfig -reg\"";
			return false;
		}
#endif
	}
	else {
#if defined(_WIN32) || defined(_WIN64)
		dr_config_status_t status = m_dr_unregister_process(procName.c_str(), 0, false, DR_PLATFORM_DEFAULT);
		if (status != DR_SUCCESS) {
			*errormsg = "dr_unregister_process failed!";
			return false;
		}

		bool isRegistered = m_dr_process_is_registered(procName.c_str(), 0, false, DR_PLATFORM_DEFAULT, NULL, NULL, NULL, NULL);
		if (isRegistered) {
			*errormsg = "We tried to unregister the process from dynamo rio but failed!";
			return false;
		}
#else
		//unfortunately, it appears to be impossible to compile drconfiglib as a static ".so". as drconfig is static, and we want to support static dynamorio, we use it for now
		ExternalProcess ep(std::string(DRCONFIG_PATH) + " -unreg " + procName, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
		if (ep.initProcess()) {
			ep.runAndWaitForCompletion(1000);
			if (ep.getExitStatus() != 0) {
				*errormsg = "drconfig -unreg failed!";
				return false;
			}
		}
		else {
			*errormsg = "could not initialize \"drconfig -unreg\"";
			return false;
		}
#endif
	}

	return true;
}

bool TestExecutorDynRioMulti::isSetupFunctionable() {
	//1) check if the target executable file exists
	{
#if defined(_WIN32) || defined(_WIN64)
		std::wstring wcmdline = std::wstring(m_targetCMDline.begin(), m_targetCMDline.end());
		int numOfBlocks;
		LPWSTR* szArglist = CommandLineToArgvW(wcmdline.c_str(), &numOfBlocks); //for some reasons this does not exist for Ascii :(
		if (NULL == szArglist || numOfBlocks < 1) {
			LOG(ERROR) << "target command line invalid";
			return false;
		}

		if (!std::experimental::filesystem::exists(szArglist[0])) {
			LocalFree(szArglist);
			LOG(ERROR) << "target executable file does not exist";
			return false;
		}
		LocalFree(szArglist);
#else
		std::string tmp = m_targetCMDline;
		std::replace(tmp.begin(), tmp.end(), '<', '_'); // replace all '<' to '_' (handle <RANDOM_SHAREDMEM> et al)
		std::replace(tmp.begin(), tmp.end(), '>', '_'); // replace all '>' to '_' (handle <RANDOM_SHAREDMEM> et al)
		char** argv = ExternalProcess::split_commandline(tmp);
		if (argv == NULL || argv[0] == NULL) {
			LOG(ERROR) << "Splitting target command line \"" << tmp << "\" failed.";
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
			LOG(WARNING) << "target executable not found";
			return false;
		}
#endif
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
	if (m_attachInsteadOfStart) {
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
	}

	//4) check if the testcase directory  exists
	if (!std::experimental::filesystem::exists(m_testcaseDir)) {
		LOG(ERROR) << "testcase directory does not exist";
		return false;
	}

	//5) check if the dynamo rio executable is present at the expected location
	if (!std::experimental::filesystem::exists(DRRUN_PATH)) {
		LOG(ERROR) << "dynamo rio directory does not exist";
		return false;
	}

	//6) check if the drconfiglib / the drconfig tool is present at the expected location
#if defined(_WIN32) || defined(_WIN64)
	if (!std::experimental::filesystem::exists(DRCONFIGLIB_PATH)) {
		LOG(ERROR) << "dynamo rio config lib does not exist";
		return false;
	}
#else
	if (!std::experimental::filesystem::exists(DRCONFIG_PATH)) {
		LOG(ERROR) << "drconfig tool does not exist";
		return false;
	}
#endif

	//7) check if the drcovMulti client is present at the expected location
	if (!std::experimental::filesystem::exists(DRCOVMULTI_PATH)) {
		LOG(ERROR) << "drcovMulti client does not exist";
		return false;
	}

	//8) check if we could load the drconfiglib.dll and identify the required dynamo rio functions
#if defined(_WIN32) || defined(_WIN64)
	if (m_hdrconfiglib == NULL) {
		LOG(ERROR) << "failed loading the necessary dynamo rio config library";
		return false;
	}

	if ((m_dr_nudge_pid == NULL) || (m_dr_register_process == NULL) || (m_dr_unregister_process == NULL) || (m_dr_register_client == NULL) || (m_dr_process_is_registered == NULL) || (m_dr_register_syswide == NULL) || (m_dr_syswide_is_on == NULL)) {
		LOG(ERROR) << "failed resolving the necessary dynamo rio functions";
		return false;
	}
#endif

	//9) Check if we have syswide injection (Windows)/ Check if libdrpreload.so exists
	if (m_attachInsteadOfStart) {
#if defined(_WIN32) || defined(_WIN64)
		std::string dynrioPath = std::experimental::filesystem::current_path().string() + DYNRIO_DIR;
		if (m_dr_syswide_is_on(DR_PLATFORM_DEFAULT, dynrioPath.c_str())) {
			LOG(DEBUG) << "syswide injection is already turned on!";
		}
		else {
			LOG(DEBUG) << "syswide injection is not yet turned on. Trying to do so ...";
			if ((m_dr_register_syswide(DR_PLATFORM_DEFAULT, dynrioPath.c_str()) == DR_SUCCESS) && m_dr_syswide_is_on(DR_PLATFORM_DEFAULT, dynrioPath.c_str())) {
				LOG(DEBUG) << "syswide injection is now turned on!";
			}
			else {
				LOG(ERROR) << "Could not turn syswide injection on. Are you admin?";
				return false;
			}
		}
#else
		if (!std::experimental::filesystem::exists(DRPRELOAD_PATH)) {
			LOG(ERROR) << "libdrpreload.so does not exist";
			return false;
		}
#endif
	}

	//10) Check if the application links to user32 statically (which is for some reason currently required by dynamorio if used in preinject mode)
#if defined(_WIN32) || defined(_WIN64)
	if (m_attachInsteadOfStart) {
		std::wstring wcmdline = std::wstring(m_targetCMDline.begin(), m_targetCMDline.end());
		int numOfBlocks;
		LPWSTR* szArglist = CommandLineToArgvW(wcmdline.c_str(), &numOfBlocks); //for some reasons this does not exist for Ascii :(
		if (NULL == szArglist || numOfBlocks < 1) {
			LOG(ERROR) << "target command line invalid";
			return false;
		}
		HANDLE  hFile = CreateFileW(szArglist[0], GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		LocalFree(szArglist);

		HANDLE  hFileMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
		LPVOID lpFile = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);

		PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpFile;
		PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((SIZE_T)lpFile + pDosHeader->e_lfanew);
		DWORD dwImportDirectoryVA = pNtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress;

		//get the section header of the virtualaddress table (we need to get the raw addresses)
		DWORD dwSectionCount = pNtHeaders->FileHeader.NumberOfSections;
		DWORD dwSection = 0;
		PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)((SIZE_T)pNtHeaders + sizeof(IMAGE_NT_HEADERS));
		for (; dwSection < dwSectionCount && pSectionHeader->VirtualAddress <= dwImportDirectoryVA; pSectionHeader++, dwSection++);
		pSectionHeader--;

		SIZE_T dwRawOffset = (SIZE_T)lpFile + pSectionHeader->PointerToRawData;
		PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(dwRawOffset + (dwImportDirectoryVA - pSectionHeader->VirtualAddress));
		bool usesUser32 = false;
		for (; pImportDescriptor->Name != 0; pImportDescriptor++)
		{
			char* dllName = (char*)(dwRawOffset + (pImportDescriptor->Name - pSectionHeader->VirtualAddress));

			if (_stricmp(dllName, "user32.dll") == 0) {
				LOG(DEBUG) << "The target application links staticly against user32.dll. This is required by dynamorio";
				usesUser32 = true;
				break;
			}
		}
		UnmapViewOfFile(lpFile);
		CloseHandle(hFileMap);
		CloseHandle(hFile);

		if (!usesUser32) {
			LOG(ERROR) << "The application does not staticly link against user32.dll. Dynamorio requires this. Possible workaround: add dependency to user32 by modifying IAT (not yet implemented).";
			return false;
		}
	}

	//11) Check if secure boot is enabled (secure boot prevents us from using syswide injection)
	if (m_attachInsteadOfStart) {
		HKEY    hKey;
		LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State", 0, KEY_READ, &hKey);
		if (res == ERROR_SUCCESS) {
			DWORD isSecureBootEnabled;
			DWORD size_isSecureBootEnabled = sizeof(isSecureBootEnabled);
			res = RegQueryValueEx(hKey, "UEFISecureBootEnabled", 0, NULL, reinterpret_cast<LPBYTE>(&isSecureBootEnabled), &size_isSecureBootEnabled);
			if (res == ERROR_SUCCESS && isSecureBootEnabled == 1) {
				//Secure boot is enabled!
				LOG(ERROR) << "Secure boot is enabled. We wont't be able to attach to a target (as dynamorio uses AppInit_DLL).";
				return false;
			}
			res = RegCloseKey(hKey);
		}
	}

#endif

	return true;
}

std::shared_ptr<DebugExecutionOutput> TestExecutorDynRioMulti::execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage) {
	(void)(forceFullCoverage); //avoid unused parameter warning: forceFullCoverage is not relevant for Dynamorio runners

	LOG(DEBUG) << "Executing: " << testcaseId;
	if (m_forceRestartAfterXTCs > 0 && m_executionsSinceLastRestart++ > m_forceRestartAfterXTCs) {
		LOG(INFO) << "Forcing a restart of the target as we reached the defined maximum of TCs without restart";
		m_target_and_feeder_okay = false;
	}

	if (!m_target_and_feeder_okay) {
		m_target_and_feeder_okay = attemptStartTargetAndFeeder(true);
		if (!m_target_and_feeder_okay) {
			//Either target or feeder could not be started!
			std::shared_ptr<DebugExecutionOutput> exResult = std::make_shared<DebugExecutionOutput>();
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exResult->m_terminationDescription = "Either target or feeder could not be started!";
			return exResult;
		}
	}

	std::shared_ptr<DebugExecutionOutput> exOutput_FROM_FEEDER = std::make_shared<DebugExecutionOutput>();
	m_target_and_feeder_okay = runSingleTestcase(testcaseId, exOutput_FROM_FEEDER, true);
	//m_target_and_feeder_okay will be set to false always but on clean exit. This will lead to a restart of fuzzer and target on next testcase by attemptStartTargetAndFeeder
	switch (m_exOutput_FROM_TARGET_DEBUGGING->m_terminationType)
	{
	case DebugExecutionOutput::CLEAN:
		//Check if the target still runs. Clean may mean "Target still running - no problem so far" and "Target terminated without an Exception"
		if (m_debuggeeProcess->getExitStatus() == -1) {
			//Case "Target still running - no problem so far" (exit status was not yet set to a meaningfull value)
			switch (exOutput_FROM_FEEDER->m_terminationType)
			{
			case DebugExecutionOutput::CLEAN:
				LOG(DEBUG) << "Testcase execution successfull! Reporting coverage...";
				m_iterationNumMod2 = (m_iterationNumMod2 + 1) % 2;
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
		LOG(DEBUG) << "Target terminated \"cleanly\" although it should not have";
	case DebugExecutionOutput::EXCEPTION_OTHER:
	case DebugExecutionOutput::EXCEPTION_ACCESSVIOLATION:
		LOG(DEBUG) << "Testcase execution yielded an exception - or the target terminated \"cleanly\" by itself. Let's try to reproduce this without dynamorio";
		{
			//Do a second run without dynamo rio to get the real crash address
			DebugExecutionOutput::PROCESS_TERMINATION_TYPE originalCrashType = m_exOutput_FROM_TARGET_DEBUGGING->m_terminationType;
			m_target_and_feeder_okay = attemptStartTargetAndFeeder(false);
			if (!m_target_and_feeder_okay) {
				//Either target or feeder could not be started!
				std::shared_ptr<DebugExecutionOutput> exResult = std::make_shared<DebugExecutionOutput>();
				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
				exResult->m_terminationDescription = "Either target or feeder coould not be started to reproduce the exception!";
				return exResult;
			}

			std::shared_ptr<DebugExecutionOutput> second_exOutput_FROM_FEEDER = std::make_shared<DebugExecutionOutput>();
			runSingleTestcase(testcaseId, second_exOutput_FROM_FEEDER, false);

			//If the feeder timed out - i.e. there was no response from target - it crashed and we need to wait for the debugger thread
			if (second_exOutput_FROM_FEEDER->m_terminationType != DebugExecutionOutput::CLEAN) {
				waitForDebuggerToTerminate();
			}

			//Check if the target still runs. Clean may mean "Target still running - no problem so far" and "Target terminated without an Exception"
			if (m_exOutput_FROM_TARGET_DEBUGGING->m_terminationType == DebugExecutionOutput::CLEAN && m_debuggeeProcess->getExitStatus() != -1) {
				//Case "Target terminated without an Exception"
				std::shared_ptr<DebugExecutionOutput> exResult = std::make_shared<DebugExecutionOutput>();
				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
				exResult->m_terminationDescription = "The request reproducibly causes the target to terminate!";
				exResult->m_firstCrash = "TARGET_TERMINATED";
				exResult->m_lastCrash = "TARGET_TERMINATED";
				m_target_and_feeder_okay = false; //force reinitialization (dyn rio needs to be reactivated)
				return exResult;
			}

			if (m_exOutput_FROM_TARGET_DEBUGGING->m_terminationType == DebugExecutionOutput::CLEAN || m_exOutput_FROM_TARGET_DEBUGGING->m_terminationType == DebugExecutionOutput::TIMEOUT || m_exOutput_FROM_TARGET_DEBUGGING->m_terminationType == DebugExecutionOutput::ERR) {
				std::shared_ptr<DebugExecutionOutput> exResult = std::make_shared<DebugExecutionOutput>();
				exResult->m_terminationType = originalCrashType;
				exResult->m_terminationDescription = "The found exception could not be reproduced!";
				exResult->m_firstCrash = "NOT_REPRODUCIBLE";
				exResult->m_lastCrash = "NOT_REPRODUCIBLE";
				m_target_and_feeder_okay = false; //force reinitialization (dyn rio needs to be reactivated)
				return exResult;
			}
		}
		m_target_and_feeder_okay = false; //force reinitialization (dyn rio needs to be reactivated)
		return m_exOutput_FROM_TARGET_DEBUGGING;
	case DebugExecutionOutput::ERR:
	case DebugExecutionOutput::TIMEOUT:
	default:
		LOG(WARNING) << "Testcase execution yielded an internal error (2): " << m_exOutput_FROM_TARGET_DEBUGGING->m_terminationDescription;
		m_target_and_feeder_okay = false;
		return m_exOutput_FROM_TARGET_DEBUGGING;
	}

	LOG(ERROR) << "The testcase exectution did not trigger a case in the switch statement (2)";
	google::protobuf::ShutdownProtobufLibrary();
	_exit(EXIT_FAILURE); //make compiler happy;
}

void TestExecutorDynRioMulti::waitForDebuggerToTerminate() {
	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> timestampToKillTheTarget = routineEntryTimeStamp + std::chrono::milliseconds(m_hangTimeoutMS);
	bool targetKilled = false;
	LOG(DEBUG) << "Waiting for the debugger thread to terminate.";
	while (!m_exOutput_FROM_TARGET_DEBUGGING->m_debuggerThreadDone) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (!targetKilled && std::chrono::steady_clock::now() > timestampToKillTheTarget) {
			LOG(DEBUG) << "Waiting sucks - killing the target.";
			m_debuggeeProcess->die(); //In case the crash is not reproducible, make sure the target dies!
			targetKilled = true;
		}
	}
}

#if defined(_WIN32) || defined(_WIN64)
void TestExecutorDynRioMulti::debuggerThreadMain(std::shared_ptr<DebugExecutionOutput> exOutput_FROM_TARGET_DEBUGGING, std::shared_ptr<ExternalProcess> debuggeeProcess, HANDLE sharedMemIPCInterruptEvent, bool attachInsteadOfStart, bool treatAnyAccessViolationAsFatal) {
#else
#define INFINITE            0xFFFFFFFF  // Infinite timeout
void TestExecutorDynRioMulti::debuggerThreadMain(std::shared_ptr<DebugExecutionOutput> exOutput_FROM_TARGET_DEBUGGING, std::shared_ptr<ExternalProcess> debuggeeProcess, int sharedMemIPCInterruptWriteFD, bool attachInsteadOfStart, bool treatAnyAccessViolationAsFatal) {
#endif
	exOutput_FROM_TARGET_DEBUGGING->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
	exOutput_FROM_TARGET_DEBUGGING->m_terminationDescription = "The target did not run!";
	exOutput_FROM_TARGET_DEBUGGING->m_debuggerThreadDone = false;

	if (attachInsteadOfStart) {
		if (!debuggeeProcess->attachToProcess()) {
			exOutput_FROM_TARGET_DEBUGGING->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exOutput_FROM_TARGET_DEBUGGING->m_terminationDescription = "Error attatching to the target process";
			LOG(ERROR) << "Failed attaching to the target process ";
			exOutput_FROM_TARGET_DEBUGGING->m_debuggerThreadDone = true;
			return;
		}
	}
	else {
		if (!debuggeeProcess->initProcess()) {
			exOutput_FROM_TARGET_DEBUGGING->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exOutput_FROM_TARGET_DEBUGGING->m_terminationDescription = "Error creating the target process";
			LOG(ERROR) << "Error initializing the target process ";
			exOutput_FROM_TARGET_DEBUGGING->m_debuggerThreadDone = true;
			return;
		}
	}

	LOG(DEBUG) << "Current target process ID " << debuggeeProcess->getProcessID();

	debuggeeProcess->debug(INFINITE, exOutput_FROM_TARGET_DEBUGGING, true, treatAnyAccessViolationAsFatal);

	if (exOutput_FROM_TARGET_DEBUGGING->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION || exOutput_FROM_TARGET_DEBUGGING->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER) {
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

	exOutput_FROM_TARGET_DEBUGGING->m_debuggerThreadDone = true;
	return;
}

bool TestExecutorDynRioMulti::waitForDynRioInitialization(int timeoutMS) {
	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(timeoutMS);

#if defined(_WIN32) || defined(_WIN64)
	DWORD  numBytesRead = 0;
#else
	ssize_t  numBytesRead = 0;
#endif

	while (numBytesRead != sizeof(process_id_t) && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
#if defined(_WIN32) || defined(_WIN64)
		ReadFile(m_pipe_to_dynrio, &m_targetProcessID, sizeof(process_id_t), &numBytesRead, NULL);
#else
		numBytesRead = read(m_pipe_to_dynrio, &m_targetProcessID, sizeof(process_id_t));
		if (numBytesRead == -1) {
			if (errno == EAGAIN) {
				numBytesRead = 0;
			}
			else {
				LOG(ERROR) << "reading from the dynamorio pipe failed (2) with errno " << errno;
			}
		}
#endif
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	if (numBytesRead != sizeof(process_id_t)) {
		LOG(ERROR) << "Could not read the process Id from the dynamo rio pipe! It looks like the client did not connect";
#if defined(_WIN32) || defined(_WIN64)
		LOG(ERROR) << "Please verify if the target process is running as the same user that runs this process. If that is not the case dynamo rio won't be initialized correctly." << std::endl << "What you can do to handle this situation: use mklink /j to create a symbolic link at <TargetUserFolder>\\dynamorio that points to <ThisUserFolder>\\dynamorio. For NT\\AUTHORITY SYSTEM its C:\\Windows\\System32\\config\\systemprofile or C:\\Windows\\SysWOW64\\config\\systemprofile.";
#endif
		return false;
	}
	LOG(DEBUG) << "Process Id delivered via dynamo rio pipe: " << m_targetProcessID;

	numBytesRead = 0;
	while (numBytesRead != sizeof(client_id_t) && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
#if defined(_WIN32) || defined(_WIN64)
		ReadFile(m_pipe_to_dynrio, &m_dynrio_clientID, sizeof(client_id_t), &numBytesRead, NULL);
#else
		numBytesRead = read(m_pipe_to_dynrio, &m_dynrio_clientID, sizeof(client_id_t));
		if (numBytesRead == -1) {
			if (errno == EAGAIN) {
				numBytesRead = 0;
			}
			else {
				LOG(ERROR) << "reading from the dynamorio pipe failed (3) with errno " << errno;
			}
		}
#endif
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	if (numBytesRead != sizeof(client_id_t)) {
		LOG(ERROR) << "Could not read the client Id from the dynamo rio pipe! It looks like the client did not connect";
		return false;
	}
	LOG(DEBUG) << "Client Id delivered via dynamo rio pipe: " << m_dynrio_clientID;

#if defined(_WIN32) || defined(_WIN64)
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	dr_config_status_t nudgePIDResult = DR_NUDGE_TIMEOUT;
	while (std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
		nudgePIDResult = m_dr_nudge_pid(m_targetProcessID, m_dynrio_clientID, NUDGE_NOOP, 100);
		if (nudgePIDResult == DR_SUCCESS) {
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	LOG(ERROR) << "Could not nudge the target process: Final Error code " << nudgePIDResult << " (8 means DR_NUDGE_TIMEOUT)";
	return false;
#else
	return true;
#endif
}

bool TestExecutorDynRioMulti::runSingleTestcase(const FluffiTestcaseID testcaseId, std::shared_ptr<DebugExecutionOutput> exResult, bool use_dyn_rio) {
	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(m_hangTimeoutMS);
	LOG(DEBUG) << "runSingleTestcase:" << testcaseId;

	//first part: Make sure the next log file does not exist, Reset Coverage, Send "Go" to Feeder, Wait for "Done" from feeder, and get Coverage from dynrio plugin
	if (use_dyn_rio) {
		std::string nextRunDrcovLogFile = getDrCovOutputFile((m_iterationNumMod2 + 1) % 2);
		int deletionAttemtps = 0;
		while (std::experimental::filesystem::exists(nextRunDrcovLogFile) && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
			//dumping the coverage later will trigger the switch to a new log file. It needs to be ensured, that that file does not yet exist.
			//this is done before reseting the coverage in order not to spoil the block coverage by waiting
			if (deletionAttemtps > 0) {
				LOG(WARNING) << "Performance warning: TestExecurotDynRioMulti is waiting for the GarbageCollector (2). We wait for the deletion of " << nextRunDrcovLogFile;
			}
			deletionAttemtps++;
			m_garbageCollectorWorker->collectNow(); //Dear garbage collctor: Stop sleeping and try deleting files!
			m_garbageCollectorWorker->collectGarbage(); //This is threadsafe. As the garbage collector could not yet delete the file yet, let's help him!
		}
		if (std::experimental::filesystem::exists(nextRunDrcovLogFile)) {
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exResult->m_terminationDescription = "Could not delete the next drcov file in time!";
			return false;
		}

		//reset coverage
		dr_config_status_t nudgePIDResult = m_dr_nudge_pid(m_targetProcessID, m_dynrio_clientID, NUDGE_RESET_COVERAGE, static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()));
		if (DR_SUCCESS != nudgePIDResult) {
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exResult->m_terminationDescription = "dr_nudge_pid failed (1):" + std::to_string(nudgePIDResult) + " PID:" + std::to_string(m_targetProcessID);
			LOG(ERROR) << exResult->m_terminationDescription;
			return false;
		}

#if defined(_WIN32) || defined(_WIN64)
		DWORD  numBytesRead = 0;
#else
		ssize_t  numBytesRead = 0;
#endif
		char resp = 0;
		while (numBytesRead < 1 && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
#if defined(_WIN32) || defined(_WIN64)
			ReadFile(m_pipe_to_dynrio, &resp, sizeof(resp), &numBytesRead, NULL);
#else
			numBytesRead = read(m_pipe_to_dynrio, &resp, sizeof(resp));
			if (numBytesRead == -1) {
				if (errno == EAGAIN) {
					numBytesRead = 0;
				}
				else {
					LOG(ERROR) << "reading from the dynamorio pipe failed (1) with errno " << errno;
				}
			}
#endif
		}
		if (resp != 'R') {
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exResult->m_terminationDescription = std::string("Problem while communicating with dynamoRio (1)!") +
				((std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) ? "no timeout" : "timeout");
			return false;
		}
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
	m_sharedMemIPC_toFeeder->waitForNewMessageToServer(&responseFromFeeder,
		(DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count(), m_SharedMemIPCInterruptEvent);
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

	if (use_dyn_rio) {
		//get coverage
		dr_config_status_t nudgePIDResult = m_dr_nudge_pid(m_targetProcessID, m_dynrio_clientID, NUDGE_DUMP_CURRENT_COVERAGE,
			static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()));
		if (DR_SUCCESS != nudgePIDResult) {
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exResult->m_terminationDescription = "dr_nudge_pid failed (2)" + std::to_string(nudgePIDResult) + " PID:" + std::to_string(m_targetProcessID);
			LOG(ERROR) << exResult->m_terminationDescription;
			return false;
		}

#if defined(_WIN32) || defined(_WIN64)
		DWORD  numBytesRead = 0;
#else
		ssize_t  numBytesRead = 0;
#endif
		char resp = 0;
		while (numBytesRead < 1 && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
#if defined(_WIN32) || defined(_WIN64)
			ReadFile(m_pipe_to_dynrio, &resp, sizeof(resp), &numBytesRead, NULL);
#else
			numBytesRead = read(m_pipe_to_dynrio, &resp, sizeof(resp));
			if (numBytesRead == -1) {
				if (errno == EAGAIN) {
					numBytesRead = 0;
				}
				else {
					LOG(ERROR) << "reading from the dynamorio pipe failed (2) with errno " << errno;
				}
			}
#endif
		}
		if (resp != 'D') {
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			exResult->m_terminationDescription = std::string("Problem while communicating with dynamoRio (2)!") +
				((std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) ? "no timeout" : "timeout");
			return false;
		}
	}
	//end of first part

	//second part: parse output file

	if (use_dyn_rio) {
		std::string drcovLogFile = getDrCovOutputFile(m_iterationNumMod2);
		if (drcovLogFile == "") {
			exResult->m_terminationDescription = " Could not find DrCov File!";
			LOG(ERROR) << exResult->m_terminationDescription << drcovLogFile;
			exResult->m_terminationType = DebugExecutionOutput::ERR;
			return false;
		}
		else {
			LOG(DEBUG) << "Parsing log file " << drcovLogFile;
		}

		std::vector<char> drcovOutput = Util::readAllBytesFromFile(drcovLogFile);

		//delete the drcovLogFile
		if (std::remove(drcovLogFile.c_str()) != 0) {
			LOG(DEBUG) << "Marking " << drcovLogFile << " for lazy delete";
			// mark the pdrcovLogFile for lazy delete
			m_garbageCollectorWorker->markFileForDelete(drcovLogFile);
			m_garbageCollectorWorker->collectNow();
		}
		else {
			LOG(DEBUG) << "Sucessfully deleted " << drcovLogFile;
		}

		if (drcovOutput.size() == 0) {
			exResult->m_terminationDescription = "drcovOutput is of length 0!";
			LOG(ERROR) << exResult->m_terminationDescription;
			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			return false;
		}

		copyCoveredModulesToDebugExecutionOutput(&drcovOutput, &m_modulesToCover, exResult);
	}
	exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN;
	exResult->m_terminationDescription = "Fuzzcase executed normally!";

	return true;

	//end of second part
}
