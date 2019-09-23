§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Abian Blome, Roman Bendt
§§*/
§§
§§#include "stdafx.h"
§§#include "ExternalProcess.h"
§§#include "DebugExecutionOutput.h"
§§#include "Util.h"
§§
§§std::atomic<TIMETYPE> ExternalProcess::s_allTimeSpent = { 0 };
§§std::unordered_set<ExternalProcess*> ExternalProcess::s_runningProcessSet;
§§std::mutex ExternalProcess::s_runningProcessSet_mutex_;
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§
§§ExternalProcess::ExternalProcess(std::string commandline, CHILD_OUTPUT_TYPE child_output_mode) :
§§	m_hasBeenRun(false),
§§	m_hasBeenInitialized(false),
§§	m_isBeingDebugged(false),
§§	m_exitStatus(-1),
§§	m_stdInOstream(nullptr),
§§	m_stdOutIstream(nullptr),
§§	m_commandline(commandline),
§§	m_child_output_mode(child_output_mode),
§§	m_inputPipe{ NULL,NULL },
§§	m_outputPipe{ NULL,NULL },
§§	m_pi{ 0 },
§§	m_job_handle(0),
§§	m_lastTimeSpentInUsermode{ 0 },
§§	m_lastTimeSpentInKernelmode{ 0 },
§§	m_allowBreakaway{ false }
§§{
§§	addToRunningProcessesSet(this);
§§
§§	if (m_child_output_mode == CHILD_OUTPUT_TYPE::SPECIAL) {
§§		//https://support.microsoft.com/de-de/help/190351/how-to-spawn-console-processes-with-redirected-standard-handles
§§
§§		SECURITY_ATTRIBUTES sa;
§§
§§		// Set up the security attributes struct.
§§		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
§§		sa.lpSecurityDescriptor = NULL;
§§		sa.bInheritHandle = TRUE;
§§
§§		// Create the child output pipe.
§§		if (!CreatePipe(&m_outputPipe[0], &m_outputPipe[1], &sa, 0)) {
§§			LOG(ERROR) << "CreatePipe for the child output pipe failed";
§§		}
§§
§§		// Create the child input pipe.
§§		if (!CreatePipe(&m_inputPipe[0], &m_inputPipe[1], &sa, 0)) {
§§			LOG(ERROR) << "CreatePipe for the child input pipe failed";
§§		}
§§
§§		// Ensure the read handle to the pipe for STDOUT is not inherited.
§§		if (!SetHandleInformation(m_outputPipe[0], HANDLE_FLAG_INHERIT, 0)) {
§§			LOG(ERROR) << " making sure the read handle to the pipe for stdout is not inherited failed";
§§		}
§§
§§		// Ensure the write handle to the pipe for STDIN is not inherited.
§§		if (!SetHandleInformation(m_inputPipe[1], HANDLE_FLAG_INHERIT, 0)) {
§§			LOG(ERROR) << " making sure the write handle to the pipe for stdin is not inherited failed";
§§		}
§§	}
§§}
§§
§§ExternalProcess::~ExternalProcess()
§§{
§§	die();
§§
§§	//Close the output / input redirection pipes
§§	if (m_outputPipe[0] != NULL) {
§§		CloseHandle(m_outputPipe[0]);
§§		m_outputPipe[0] = NULL;
§§	}
§§	if (m_outputPipe[1] != NULL) {
§§		CloseHandle(m_outputPipe[1]);
§§		m_outputPipe[1] = NULL;
§§	}
§§	if (m_inputPipe[0] != NULL) {
§§		CloseHandle(m_inputPipe[0]);
§§		m_inputPipe[0] = NULL;
§§	}
§§	if (m_inputPipe[1] != NULL) {
§§		CloseHandle(m_inputPipe[1]);
§§		m_inputPipe[1] = NULL;
§§	}
§§
§§	if (m_stdInOstream != nullptr) {
§§		delete m_stdInOstream;
§§		m_stdInOstream = nullptr;
§§	}
§§
§§	if (m_stdOutIstream != nullptr) {
§§		delete m_stdOutIstream;
§§		m_stdOutIstream = nullptr;
§§	}
§§
§§	removeFromRunningProcessesSet(this);
§§}
§§
§§void ExternalProcess::updateTimeSpent() {
§§	if (m_hasBeenInitialized && m_job_handle != NULL) {
§§		JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobInfo;
§§		BOOL ret = QueryInformationJobObject(m_job_handle, JobObjectBasicAccountingInformation, &jobInfo, sizeof(jobInfo), NULL);
§§		if (ret != FALSE) {
§§			s_allTimeSpent += (jobInfo.TotalUserTime.QuadPart - m_lastTimeSpentInUsermode.QuadPart) + (jobInfo.TotalKernelTime.QuadPart - m_lastTimeSpentInKernelmode.QuadPart);
§§			m_lastTimeSpentInUsermode.QuadPart = jobInfo.TotalUserTime.QuadPart;
§§			m_lastTimeSpentInKernelmode.QuadPart = jobInfo.TotalKernelTime.QuadPart;
§§		}
§§		else {
§§			LOG(ERROR) << "QueryInformationJobObject failed";
§§		}
§§	}
§§}
§§
§§void ExternalProcess::die() {
§§	updateTimeSpent();
§§
§§	LOG(DEBUG) << "Telling PID " << m_pi.dwProcessId << " (\"" + m_commandline + "\") to die...";
§§
§§	if (m_pi.dwProcessId != 0) {
§§		//if we debug the target (e.g. because it has not yet been run) - stop doing so
§§		DebugActiveProcessStop(m_pi.dwProcessId);
§§	}
§§
§§	if (m_pi.hProcess != NULL) {
§§		if (TerminateProcess(m_pi.hProcess, 0) == 0) {
§§			LOG(DEBUG) << "Could not terminate " << m_commandline << " (" << GetLastError() << "). It might already be terminated.";
§§		}
§§	}
§§	if (m_pi.hThread != NULL) {
§§		CloseHandle(m_pi.hThread);
§§		m_pi.hThread = NULL;
§§	}
§§	if (m_pi.hProcess != NULL) {
§§		CloseHandle(m_pi.hProcess);
§§		m_pi.hProcess = NULL;
§§	}
§§	if (m_job_handle != NULL) {
§§		CloseHandle(m_job_handle);
§§		m_job_handle = NULL;
§§	}
§§}
§§
§§int ExternalProcess::getProcessID() {
§§	return m_pi.dwProcessId;
§§}
§§
§§//adapted from https://www.codeproject.com/Questions/78801/How-to-get-the-main-thread-ID-of-a-process-known-b
§§bool ExternalProcess::fillInMainThread(PROCESS_INFORMATION* m_pi)
§§{
§§	DWORD dwMainThreadID = 0;
§§	ULONGLONG ullMinCreateTime = MAXULONGLONG;
§§
§§	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
§§	if (hThreadSnap != INVALID_HANDLE_VALUE) {
§§		THREADENTRY32 th32;
§§		th32.dwSize = sizeof(THREADENTRY32);
§§		BOOL bOK = TRUE;
§§		for (bOK = Thread32First(hThreadSnap, &th32); bOK == TRUE; bOK = Thread32Next(hThreadSnap, &th32)) {
§§			if (th32.th32OwnerProcessID == m_pi->dwProcessId) {
§§				HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, TRUE, th32.th32ThreadID);
§§				if (hThread) {
§§					FILETIME afTimes[4] = { 0 };
§§					if (GetThreadTimes(hThread,
§§						&afTimes[0], &afTimes[1], &afTimes[2], &afTimes[3])) {
§§						ULONGLONG ullTest = static_cast<ULONGLONG>(afTimes[0].dwHighDateTime) << 32 | afTimes[0].dwLowDateTime;
§§						if (ullTest && ullTest < ullMinCreateTime) {
§§							ullMinCreateTime = ullTest;
§§							dwMainThreadID = th32.th32ThreadID; // let it be main... :)
§§						}
§§					}
§§					CloseHandle(hThread);
§§				}
§§			}
§§		}
§§		CloseHandle(hThreadSnap);
§§	}
§§
§§	if (dwMainThreadID != 0) {
§§		m_pi->dwThreadId = dwMainThreadID;
§§		m_pi->hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, m_pi->dwThreadId);
§§		return true;
§§	}
§§	else {
§§		LOG(ERROR) << "Could not get the main thread for process " << m_pi->dwProcessId;
§§		return false;
§§	}
§§}
§§
§§void ExternalProcess::setAllowBreakAway() {
§§	m_allowBreakaway = true;
§§}
§§
§§DWORD ExternalProcess::getPIDForProcessName(std::string processName) {
§§	//get procNameAndPath from targetCMDLine
§§	std::wstring wtargetCMDLine = std::wstring(processName.begin(), processName.end());
§§	int numOfBlocks;
§§	LPWSTR* szArglist = CommandLineToArgvW(wtargetCMDLine.c_str(), &numOfBlocks); //for some reasons this does not exist for Ascii :(
§§	if (NULL == szArglist || numOfBlocks < 1) {
§§		LOG(ERROR) << "Invalid processName: " << processName;
§§		return 0;
§§	}
§§
§§	//get procName from procNameAndPath
§§	std::tuple<std::string, std::string> directoryAndFilename = Util::splitPathIntoDirectoryAndFileName(Util::wstring_to_utf8(szArglist[0]));
§§	LocalFree(szArglist);
§§	LOG(DEBUG) << "getPIDForProcessName is searching for " << std::get<1>(directoryAndFilename);
§§
§§	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
§§	DWORD re = 0;
§§	if (hSnapshot) {
§§		PROCESSENTRY32 pe32;
§§		pe32.dwSize = sizeof(PROCESSENTRY32);
§§		BOOL bOK = TRUE;
§§		for (bOK = Process32First(hSnapshot, &pe32); bOK == TRUE; bOK = Process32Next(hSnapshot, &pe32)) {
§§			//LOG(DEBUG) << "getPIDForProcessName found process " << pe32.szExeFile;
§§			if (pe32.szExeFile == std::get<1>(directoryAndFilename)) {
§§				if (re == 0) {
§§					LOG(DEBUG) << "Process " << pe32.szExeFile << "(" << pe32.th32ProcessID << ") matches our search pattern";
§§					re = pe32.th32ProcessID;
§§				}
§§				else {
§§					LOG(ERROR) << "getPIDForProcessName: There are multiple processes with the process name " << processName;
§§					CloseHandle(hSnapshot);
§§					return 0;
§§				}
§§			}
§§		}
§§		CloseHandle(hSnapshot);
§§	}
§§
§§	if (re == 0) {
§§		LOG(ERROR) << "getPIDForProcessName: Could not find a process with name " << processName;
§§	}
§§
§§	return re;
§§}
§§
§§bool ExternalProcess::attachToProcess() {
§§	//Opening target Process
§§	m_pi = { 0 };
§§	m_pi.dwProcessId = getPIDForProcessName(m_commandline);
§§	m_pi.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_pi.dwProcessId);
§§	if (m_pi.hProcess == NULL) {
§§		LOG(ERROR) << "Could not Open Process  " << m_pi.dwProcessId << " (Error " << GetLastError() << ")";
§§		return false;
§§	}
§§
§§	// supress message box on crash, inheritable, vital for core functionality
§§	if (m_child_output_mode == CHILD_OUTPUT_TYPE::SUPPRESS) {
§§		UINT EMode = (SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
§§		ULONG ProcessDefaultHardErrorMode = 12;
§§		NtSetInformationProcess(m_pi.hProcess, ProcessDefaultHardErrorMode, (PVOID)&EMode, sizeof(EMode));
§§	}
§§
§§	if (!fillInMainThread(&m_pi)) {
§§		if (m_pi.hProcess != NULL) {
§§			TerminateProcess(m_pi.hProcess, 0);
§§		}
§§		if (m_pi.hThread != NULL) {
§§			CloseHandle(m_pi.hThread);
§§			m_pi.hThread = NULL;
§§		}
§§		if (m_pi.hProcess != NULL) {
§§			CloseHandle(m_pi.hProcess);
§§			m_pi.hProcess = NULL;
§§		}
§§
§§		LOG(ERROR) << "Error calling fillInMainThread for " << m_commandline;
§§		return false;
§§	}
§§
§§	// create Job Object for child process
§§	m_job_handle = CreateJobObject(nullptr, nullptr);
§§
§§	if (m_job_handle == NULL) {
§§		if (m_pi.hProcess != NULL) {
§§			TerminateProcess(m_pi.hProcess, 0);
§§		}
§§		if (m_pi.hThread != NULL) {
§§			CloseHandle(m_pi.hThread);
§§			m_pi.hThread = NULL;
§§		}
§§		if (m_pi.hProcess != NULL) {
§§			CloseHandle(m_pi.hProcess);
§§			m_pi.hProcess = NULL;
§§		}
§§
§§		LOG(ERROR) << "Error calling CreateJobObject for " << m_commandline;
§§		return false;
§§	}
§§
§§	// enable kill on job close
§§	JOBOBJECT_EXTENDED_LIMIT_INFORMATION joelim = { 0 };
§§	joelim.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
§§	if (m_allowBreakaway) {
§§		//As we cannot attach to a process that is already in a job, we need to allow starters to create subprocesses that break away from the starter job
§§		joelim.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
§§	}
§§
§§	int callresult = SetInformationJobObject(m_job_handle, JobObjectExtendedLimitInformation, &joelim, sizeof(joelim));
§§	if (callresult == 0) {
§§		if (m_pi.hProcess != NULL) {
§§			TerminateProcess(m_pi.hProcess, 0);
§§		}
§§		if (m_pi.hThread != NULL) {
§§			CloseHandle(m_pi.hThread);
§§			m_pi.hThread = NULL;
§§		}
§§		if (m_pi.hProcess != NULL) {
§§			CloseHandle(m_pi.hProcess);
§§			m_pi.hProcess = NULL;
§§		}
§§		if (m_job_handle != NULL) {
§§			CloseHandle(m_job_handle);
§§			m_job_handle = NULL;
§§		}
§§
§§		LOG(ERROR) << "Error calling SetInformationJobObject for " << m_commandline;
§§		return false;
§§	}
§§
§§	callresult = AssignProcessToJobObject(m_job_handle, m_pi.hProcess);
§§	if (callresult == 0) {
§§		DWORD lastError = GetLastError();
§§		if (m_pi.hProcess != NULL) {
§§			TerminateProcess(m_pi.hProcess, 0);
§§		}
§§		if (m_pi.hThread != NULL) {
§§			CloseHandle(m_pi.hThread);
§§			m_pi.hThread = NULL;
§§		}
§§		if (m_pi.hProcess != NULL) {
§§			CloseHandle(m_pi.hProcess);
§§			m_pi.hProcess = NULL;
§§		}
§§		if (m_job_handle != NULL) {
§§			CloseHandle(m_job_handle);
§§			m_job_handle = NULL;
§§		}
§§
§§		LOG(ERROR) << "Error calling AssignProcessToJobObject for " << m_commandline << " (Error code" << lastError << ")";
§§		return false;
§§	}
§§
§§	//Activate debugging and suspend the target process
§§	callresult = DebugActiveProcess(m_pi.dwProcessId);
§§	if (callresult == 0) {
§§		if (m_pi.hProcess != NULL) {
§§			TerminateProcess(m_pi.hProcess, 0);
§§		}
§§		if (m_pi.hThread != NULL) {
§§			CloseHandle(m_pi.hThread);
§§			m_pi.hThread = NULL;
§§		}
§§		if (m_pi.hProcess != NULL) {
§§			CloseHandle(m_pi.hProcess);
§§			m_pi.hProcess = NULL;
§§		}
§§		if (m_job_handle != NULL) {
§§			CloseHandle(m_job_handle);
§§			m_job_handle = NULL;
§§		}
§§
§§		LOG(ERROR) << "Error calling DebugActiveProcess for " << m_commandline;
§§		return false;
§§	}
§§
§§	LOG(DEBUG) << "Attaching to PID " << m_pi.dwProcessId << " succeeded.";
§§	m_hasBeenInitialized = true;
§§	return true;
§§}
§§
§§bool ExternalProcess::initProcess() {
§§	LOG(DEBUG) << "Trying to init  " << m_commandline;
§§
§§	// supress message box on crash, inheritable, vital for core functionality
§§	if (m_child_output_mode == CHILD_OUTPUT_TYPE::SUPPRESS) {
§§		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
§§	}
§§
§§	// create Job Object for child process
§§	m_job_handle = CreateJobObject(nullptr, nullptr);
§§
§§	if (m_job_handle == NULL) {
§§		LOG(ERROR) << "Error calling CreateJobObject for " << m_commandline;
§§		return false;
§§	}
§§
§§	// enable kill on job close
§§	JOBOBJECT_EXTENDED_LIMIT_INFORMATION joelim = { 0 };
§§	joelim.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
§§	if (m_allowBreakaway) {
§§		//As we cannot attach to a process that is already in a job, we need to allow starters to create subprocesses that break away from the starter job
§§		joelim.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
§§	}
§§
§§	int callresult = SetInformationJobObject(
§§		m_job_handle,
§§		JobObjectExtendedLimitInformation,
§§		&joelim,
§§		sizeof(joelim)
§§	);
§§	if (callresult == 0) {
§§		LOG(ERROR) << "Error calling SetInformationJobObject for " << m_commandline;
§§		CloseHandle(m_job_handle);
§§		m_job_handle = NULL;
§§		return false;
§§	}
§§
§§	// create process for job
§§	STARTUPINFOA si = { sizeof(STARTUPINFOA), };
§§	BOOL inheritHandles = FALSE;
§§	if (m_child_output_mode == CHILD_OUTPUT_TYPE::SUPPRESS) {
§§		si.dwFlags |= STARTF_USESTDHANDLES;
§§		si.hStdInput = si.hStdOutput = si.hStdError = nullptr;
§§
§§		inheritHandles = FALSE; // do not inherit handles (process isolation)
§§	}
§§	else if (m_child_output_mode == CHILD_OUTPUT_TYPE::SPECIAL) {
§§		si.dwFlags |= STARTF_USESTDHANDLES;
§§		si.hStdInput = m_inputPipe[0];
§§		si.hStdOutput = m_outputPipe[1];
§§		// Create a duplicate of the output write handle for the std error
§§		// write handle. This is necessary in case the child application
§§		// closes one of its std output handles.
§§		if (!DuplicateHandle(GetCurrentProcess(), m_outputPipe[1], GetCurrentProcess(), &si.hStdError, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
§§			LOG(ERROR) << "Failed calling DuplicateHandle on m_outputPipe";
§§			CloseHandle(m_job_handle);
§§			m_job_handle = NULL;
§§			return false;
§§		}
§§
§§		inheritHandles = TRUE; // inherit inheritable handles
§§	}
§§	else {
§§		/* std handles are inherited by default even inherit is false
§§		si.hStdOutput = stdout;
§§		si.hStdError = stderr;
§§		si.hStdInput = nullptr;
§§		*/
§§
§§		inheritHandles = FALSE; // do not inherit handles (process isolation)
§§	}
§§
§§	DWORD crfl = INHERIT_PARENT_AFFINITY | CREATE_SUSPENDED | DEBUG_PROCESS;
§§	if (m_child_output_mode == CHILD_OUTPUT_TYPE::SUPPRESS) {
§§		//Some applications might behave different if they cannot open their precious window. It might therefore make sense to remove the following line.
§§		crfl |= CREATE_NO_WINDOW;
§§	}
§§
§§	callresult = CreateProcessInJob(m_job_handle, NULL, const_cast<char*>(m_commandline.c_str()), nullptr, nullptr, inheritHandles, crfl, nullptr, nullptr, &si, &m_pi);
§§	if (callresult == 0) {
§§		if (m_pi.hProcess != NULL) {
§§			TerminateProcess(m_pi.hProcess, 0);
§§		}
§§		if (m_pi.hThread != NULL) {
§§			CloseHandle(m_pi.hThread);
§§			m_pi.hThread = NULL;
§§		}
§§		if (m_pi.hProcess != NULL) {
§§			CloseHandle(m_pi.hProcess);
§§			m_pi.hProcess = NULL;
§§		}
§§		if (m_job_handle != NULL) {
§§			CloseHandle(m_job_handle);
§§			m_job_handle = NULL;
§§		}
§§
§§		LOG(ERROR) << "Error calling CreateProcessInJob for " << m_commandline;
§§		return false;
§§	}
§§
§§	if (m_child_output_mode == CHILD_OUTPUT_TYPE::SPECIAL) {
§§		// Close pipe handles (do not continue to modify the parent).
§§		// You need to make sure that no handles to the write end of the
§§		// output pipe are maintained in this process or else the pipe will
§§		// not close when the child process exits and the ReadFile will hang.
§§
§§		//Close the duplicated handle (error pipe)
§§		CloseHandle(si.hStdError);
§§		si.hStdError = NULL;
§§
§§		//Close the writing end of the output pipe
§§		CloseHandle(m_outputPipe[1]);
§§		m_outputPipe[1] = NULL;
§§
§§		//Close the reading end of the input pipe
§§		CloseHandle(m_inputPipe[0]);
§§		m_inputPipe[0] = NULL;
§§	}
§§
§§	LOG(DEBUG) << "We successfully created PID " << m_pi.dwProcessId;
§§
§§	m_hasBeenInitialized = true;
§§	return true;
§§}
§§
§§//Taken from https://blogs.msdn.microsoft.com/oldnewthing/20131209-00/?p=2433
§§BOOL ExternalProcess::CreateProcessInJob(
§§	HANDLE hJob,
§§	LPCSTR lpApplicationName,
§§	LPSTR lpCommandLine,
§§	LPSECURITY_ATTRIBUTES lpProcessAttributes,
§§	LPSECURITY_ATTRIBUTES lpThreadAttributes,
§§	BOOL bInheritHandles,
§§	DWORD dwCreationFlags,
§§	LPVOID lpEnvironment,
§§	LPCSTR lpCurrentDirectory,
§§	LPSTARTUPINFOA lpStartupInfo,
§§	LPPROCESS_INFORMATION ppi)
§§{
§§	BOOL fRc = CreateProcess(
§§		lpApplicationName,
§§		lpCommandLine,
§§		lpProcessAttributes,
§§		lpThreadAttributes,
§§		bInheritHandles,
§§		dwCreationFlags | CREATE_SUSPENDED,
§§		lpEnvironment,
§§		lpCurrentDirectory,
§§		lpStartupInfo,
§§		ppi);
§§	if (fRc) {
§§		fRc = AssignProcessToJobObject(hJob, ppi->hProcess);
§§		if (fRc && !(dwCreationFlags & CREATE_SUSPENDED)) {
§§			fRc = ResumeThread(ppi->hThread) != (DWORD)-1;
§§		}
§§		if (!fRc) {
§§			DWORD lasterror = GetLastError();
§§
§§			TerminateProcess(ppi->hProcess, 0);
§§			CloseHandle(ppi->hProcess);
§§			CloseHandle(ppi->hThread);
§§			ppi->hProcess = ppi->hThread = nullptr;
§§
§§			LOG(ERROR) << "AssignProcessToJobObject or ResumeThread, error " << lasterror;
§§		}
§§	}
§§	else {
§§		DWORD lasterror = GetLastError();
§§		LOG(ERROR) << "CreateProcess, error " << lasterror;
§§	}
§§	return fRc;
§§}
§§
§§bool ExternalProcess::runAndWaitForCompletion(unsigned long timeoutMilliseconds) {
§§	if (m_pi.hThread == NULL || m_pi.hProcess == NULL || m_hasBeenRun || !m_hasBeenInitialized) {
§§		LOG(ERROR) << "The process " << m_commandline << " cannot run, as it is not in a runnable state";
§§		return false;
§§	}
§§
§§	if (!DebugActiveProcessStop(m_pi.dwProcessId)) {
§§		LOG(ERROR) << "Detaching from " << m_commandline << " failed (" << GetLastError() << ")";
§§		return false;
§§	}
§§
§§	if (ResumeThread(m_pi.hThread) == (DWORD)-1) {
§§		LOG(ERROR) << "Resuming " << m_commandline << " failed (" << GetLastError() << ")";
§§		return false;
§§	}
§§
§§	// Successfully created the process.  Wait for it to finish.
§§	DWORD waitResult = WaitForSingleObject(m_pi.hProcess, timeoutMilliseconds);
§§	if (waitResult == WAIT_TIMEOUT || waitResult == WAIT_FAILED) {
§§		LOG(WARNING) << "Executing " << m_commandline << " resulted in a timeout!";
§§		return false;
§§	}
§§	else if (waitResult != WAIT_OBJECT_0) {
§§		LOG(ERROR) << "Executing " << m_commandline << " triggered an error while executing WaitForSingleObject: " << waitResult;
§§		return false;
§§	}
§§
§§	DWORD exitCode;
§§	if (0 != GetExitCodeProcess(m_pi.hProcess, &exitCode)) {
§§		m_exitStatus = exitCode;
§§	}
§§
§§	m_hasBeenRun = true;
§§	return true;
§§}
§§bool ExternalProcess::run() {
§§	if (m_pi.hThread == NULL || m_pi.dwProcessId == NULL || m_hasBeenRun || !m_hasBeenInitialized) {
§§		LOG(ERROR) << "The process " << m_commandline << " cannot run, as it is not in a runnable state";
§§		return false;
§§	}
§§
§§	if (!DebugActiveProcessStop(m_pi.dwProcessId)) {
§§		LOG(ERROR) << "Detaching from " << m_commandline << " failed (" << GetLastError() << ")";
§§		return false;
§§	}
§§
§§	if (ResumeThread(m_pi.hThread) == (DWORD)-1) {
§§		LOG(ERROR) << "Resuming " << m_commandline << " failed (" << GetLastError() << ")";
§§		return false;
§§	}
§§
§§	m_hasBeenRun = true;
§§	return true;
§§}
§§
§§void ExternalProcess::debug(unsigned long timeoutMilliseconds, std::shared_ptr<DebugExecutionOutput> exResult, bool doPostMortemAnalysis, bool treatAnyAccessViolationAsFatal) {
§§	if (m_pi.hThread == NULL || m_pi.dwProcessId == NULL || m_hasBeenRun || !m_hasBeenInitialized) {
§§		std::stringstream oss;
§§		oss << "The process " << m_commandline << " cannot run, as it is not in a runnable state" << std::endl;
§§		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§		exResult->m_terminationDescription = oss.str();
§§		return;
§§	}
§§
§§	int numberOfExceptions = 0;
§§	int numberOfCreatedProcesses = 0;
§§
§§	if (ResumeThread(m_pi.hThread) == (DWORD)-1) {
§§		std::stringstream oss;
§§		oss << "Could not start the target's main thread " << GetLastError() << std::endl;
§§		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§		exResult->m_terminationDescription = oss.str();
§§		return;
§§	}
§§
§§	//Ready to debug
§§	m_isBeingDebugged = true;
§§
§§	//So far there is no exception
§§	exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN;
§§	exResult->m_terminationDescription = "The target executed without any exception";
§§
§§	//It turns out that if a process hangs and we start a new one too soon, we receive debugging messages for processes that are no longer actively debugged
§§	std::vector<DWORD> myProcessIDs;
§§	myProcessIDs.push_back(m_pi.dwProcessId);
§§
§§	DEBUG_EVENT debug_event;
§§	ZeroMemory(&debug_event, sizeof(debug_event));
§§	DWORD dwContinueStatus = DBG_CONTINUE; // exception continuation
§§	while (TRUE)
§§	{
§§		if (WaitForDebugEvent(&debug_event, timeoutMilliseconds))
§§		{
§§			dwContinueStatus = DBG_CONTINUE;
§§			switch (debug_event.dwDebugEventCode)
§§			{
§§			case EXCEPTION_DEBUG_EVENT:
§§				// Process the exception code. When handling
§§				// exceptions, remember to set the continuation
§§				// status parameter (dwContinueStatus). This value
§§				// is used by the ContinueDebugEvent function.
§§				dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
§§				numberOfExceptions++;
§§				if (debug_event.u.Exception.ExceptionRecord.ExceptionCode == DBG_CONTROL_C) {
§§					exResult->m_terminationDescription = "Target was interrupted! ";
§§					exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§				}
§§				// only access violations (if treatAnyAccessViolationAsFatal) and last chance exception:
§§				if (!debug_event.u.Exception.dwFirstChance || (debug_event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && treatAnyAccessViolationAsFatal))
§§				{
§§					if (debug_event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
§§						exResult->m_lastCrash = "RACE";
§§						exResult->m_terminationDescription = "Detected an EXCEPTION_ACCESS_VIOLATION";
§§						exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION;
§§					}
§§					else if (debug_event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {
§§						exResult->m_lastCrash = "RACE";
§§						exResult->m_terminationDescription = "Detected an EXCEPTION_BREAKPOINT";
§§						exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
§§					}
§§					else if (debug_event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION) {
§§						exResult->m_lastCrash = "RACE";
§§						exResult->m_terminationDescription = "Detected an EXCEPTION_ILLEGAL_INSTRUCTION";
§§						exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
§§					}
§§					else if (debug_event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO) {
§§						exResult->m_lastCrash = "RACE";
§§						exResult->m_terminationDescription = "Detected an EXCEPTION_INT_DIVIDE_BY_ZERO";
§§						exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
§§					}
§§					else if (debug_event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
§§						exResult->m_lastCrash = "RACE";
§§						exResult->m_terminationDescription = "Detected an EXCEPTION_STACK_OVERFLOW";
§§						exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
§§					}
§§					else {
§§						exResult->m_lastCrash = "RACE";
§§						std::stringstream oss;
§§						oss << "Detected a not yet defined exception type: Exception " << debug_event.u.Exception.ExceptionRecord.ExceptionCode << std::endl;
§§						exResult->m_terminationDescription = oss.str();
§§						exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
§§					}
§§
§§					if (doPostMortemAnalysis) {
§§						exResult->m_lastCrash = addrToRVAString(m_pi, reinterpret_cast<std::uintptr_t>(debug_event.u.Exception.ExceptionRecord.ExceptionAddress));
§§					}
§§
§§					//We got an access violation but it's not said that the program will terminate because of it. We need to return here forcefully (just as with a hang)
§§					if (debug_event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && treatAnyAccessViolationAsFatal) {
§§						//Stop debugging (also for all subprocesses)
§§						for (DWORD myProcessID : myProcessIDs)
§§						{
§§							DebugActiveProcessStop(myProcessID);
§§						}
§§						m_hasBeenRun = true;
§§						return;
§§					}
§§				}
§§				//The first real exception after  system breakpoint is numberOfExceptions==2. However, if we attach, we will miss that. In that case, store the first exception
§§				if (doPostMortemAnalysis &&  numberOfExceptions <= 2) {
§§					exResult->m_firstCrash = addrToRVAString(m_pi, reinterpret_cast<std::uintptr_t>(debug_event.u.Exception.ExceptionRecord.ExceptionAddress));
§§				}
§§
§§				break;
§§
§§			case RIP_EVENT:
§§				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§				exResult->m_terminationDescription = "RIP Event";
§§				ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, dwContinueStatus);
§§				DebugActiveProcessStop(m_pi.dwProcessId);
§§				m_hasBeenRun = true;
§§				return;
§§			case CREATE_PROCESS_DEBUG_EVENT:
§§				numberOfCreatedProcesses++;
§§				if (debug_event.u.CreateProcessInfo.hFile != 0) {
§§					CloseHandle(debug_event.u.CreateProcessInfo.hFile);
§§				}
§§				else {
§§					exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§					exResult->m_terminationDescription = "debug_event.u.CreateProcessInfo.hFile was NULL!";
§§					LOG(ERROR) << exResult->m_terminationDescription;
§§					m_hasBeenRun = true;
§§					return;
§§				}
§§				myProcessIDs.push_back(debug_event.dwProcessId);
§§				break;
§§			case CREATE_THREAD_DEBUG_EVENT:
§§				break;
§§			case EXIT_PROCESS_DEBUG_EVENT:
§§				//This exit will be taken even after an exception.
§§				numberOfCreatedProcesses--;
§§				if (numberOfCreatedProcesses == 0) {
§§					//If no exception was observed: Set exit type to normal
§§					if (exResult->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN) {
§§						m_exitStatus = debug_event.u.ExitProcess.dwExitCode;
§§						exResult->m_terminationDescription = "Process terminated normally: " + std::to_string(debug_event.u.ExitProcess.dwExitCode);
§§					}
§§
§§					ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, dwContinueStatus);
§§					DebugActiveProcessStop(m_pi.dwProcessId);
§§					m_hasBeenRun = true;
§§					return;
§§				}
§§				break;
§§
§§			case EXIT_THREAD_DEBUG_EVENT:
§§				break;
§§			case OUTPUT_DEBUG_STRING_EVENT:
§§				if (!doPostMortemAnalysis) {
§§					//https://github.com/DynamoRIO/dynamorio/wiki/Debugging
§§					LOG(WARNING) << "The application uses OutputDebugString - Dynamo Rio cannot handle this! Please patch the target application if you get the message that drcovOutput is of length 0 (if not - you are fine - this is a known wtf)!";
§§				}
§§				else {
§§					LOG(INFO) << "The application uses OutputDebugString - No problem as long as Dynamo Rio is not used.";
§§				}
§§				break;
§§			case LOAD_DLL_DEBUG_EVENT:
§§				if (debug_event.u.LoadDll.hFile != 0) {
§§					CloseHandle(debug_event.u.LoadDll.hFile);
§§				}
§§				else {
§§					exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§					exResult->m_terminationDescription = "debug_event.u.LoadDll.hFile was NULL!";
§§					LOG(ERROR) << exResult->m_terminationDescription;
§§					m_hasBeenRun = true;
§§					return;
§§				}
§§				break;
§§			case UNLOAD_DLL_DEBUG_EVENT:
§§				break;
§§			default:
§§				std::stringstream oss;
§§				oss << "Debug Event unknown: " << debug_event.dwDebugEventCode;
§§				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§				exResult->m_terminationDescription = oss.str();
§§				LOG(ERROR) << exResult->m_terminationDescription;
§§				m_hasBeenRun = true;
§§				return;
§§			}
§§			ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, dwContinueStatus);
§§		}
§§		else
§§		{
§§			// WaitForDebugEvent failed...
§§			// Is it because of timeout ?
§§			DWORD ErrCode = GetLastError();
§§			if (ErrCode == ERROR_SEM_TIMEOUT)
§§			{
§§				// Yes, report hang
§§				exResult->m_terminationDescription = "Process hang detected";
§§				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::TIMEOUT;
§§
§§				//Stop debugging (also for all subprocesses)
§§				for (DWORD myProcessID : myProcessIDs)
§§				{
§§					DebugActiveProcessStop(myProcessID);
§§				}
§§				m_hasBeenRun = true;
§§				return;
§§			}
§§			else
§§			{
§§				// No, something went wrong!
§§				std::stringstream oss;
§§				oss << "WaitForDebugEvent() failed. Error: " << GetLastError() << std::endl;
§§				exResult->m_terminationDescription = oss.str();
§§				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§				//Stop debugging (also for all subprocesses)
§§				for (DWORD myProcessID : myProcessIDs)
§§				{
§§					DebugActiveProcessStop(myProcessID);
§§				}
§§				m_hasBeenRun = true;
§§				return;
§§			}
§§		}
§§	}
§§}
§§
§§std::string ExternalProcess::addrToRVAString(PROCESS_INFORMATION pi, std::uintptr_t addr) {
§§	HMODULE hMods[1024];
§§	DWORD cbNeeded;
§§
§§	if (EnumProcessModules(pi.hProcess, hMods, sizeof(hMods), &cbNeeded) != 0)
§§	{
§§		for (unsigned long i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
§§		{
§§			MODULEINFO mi;
§§			if (GetModuleInformation(pi.hProcess, hMods[i], &mi, sizeof(mi))) {
§§				if ((reinterpret_cast<std::uintptr_t>(mi.lpBaseOfDll) < addr) && (reinterpret_cast<std::uintptr_t>(mi.lpBaseOfDll) + mi.SizeOfImage > addr)) {
§§					char szModName[MAX_PATH];
§§					if (GetModuleFileNameEx(pi.hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(char)))
§§					{
§§						std::stringstream oss;
§§						oss << std::experimental::filesystem::path(szModName).filename().string() << "+0x" << std::hex << std::setw(8) << std::setfill('0') << addr - reinterpret_cast<std::uintptr_t>(mi.lpBaseOfDll);
§§						return oss.str();
§§					}
§§					else {
§§						LOG(ERROR) << "GetModuleFileNameEx failed.";
§§					}
§§				}
§§			}
§§			else {
§§				LOG(ERROR) << "GetModuleInformation failed.";
§§			}
§§		}
§§
§§		//apparently, the address is not part of any loaded module
§§		std::stringstream oss;
§§		oss << "unknown+0x" << std::hex << std::setw(8) << std::setfill('0') << addr;
§§		return oss.str();
§§	}
§§	else {
§§		LOG(ERROR) << "EnumProcessModules failed.";
§§	}
§§
§§	return "";
§§}
§§
§§#else
§§
§§ExternalProcess::ExternalProcess(std::string commandline, CHILD_OUTPUT_TYPE child_output_mode, std::string additionalEnvParam, bool targetIsSentNudges) :
§§	m_child_output_mode(child_output_mode),
§§	m_inputPipe{ -1,-1 },
§§	m_outputPipe{ -1,-1 },
§§	m_hasBeenRun(false),
§§	m_hasBeenInitialized(false),
§§	m_isBeingDebugged(false),
§§	m_exitStatus(-1),
§§	m_stdInOstream(nullptr),
§§	m_stdOutIstream(nullptr),
§§	m_argv(split_commandline(commandline)),
§§	m_envp(addToEnv(additionalEnvParam)),
§§	m_childPID(0),
§§	m_lastutime(0),
§§	m_laststime(0),
§§	m_lastcutime(0),
§§	m_lastcstime(0),
§§	m_originalMask{ 0 },
§§	m_debugSignalMask{ 0 },
§§	m_fd_pipe{ -1,-1 },
§§	m_targetIsSentNudges{ targetIsSentNudges },
§§	m_stdInOstream_buf(nullptr),
§§	m_stdOutIstream_buf(nullptr)
§§{
§§	addToRunningProcessesSet(this);
§§
§§	sigemptyset(&m_debugSignalMask);
§§	sigaddset(&m_debugSignalMask, SIGCHLD);//the current mask may or may not be set for SIGCHLD - depending on whether this is the first external process or not
§§
§§	//Apparently this call is neccesary in order to receive SIGCHLD events
§§	if (sigprocmask(SIG_BLOCK, &m_debugSignalMask, &m_originalMask) < 0) {
§§		LOG(ERROR) << "sigprocmask failed (const): " << strerror(errno);
§§	}
§§
§§	if (m_child_output_mode == CHILD_OUTPUT_TYPE::SPECIAL) {
§§		// Create the child output pipe.
§§		if (pipe(m_outputPipe) == -1) {
§§			LOG(ERROR) << "CreatePipe for the child output pipe failed";
§§		}
§§
§§		// Create the child input pipe.
§§		if (pipe(m_inputPipe) == -1) {
§§			LOG(ERROR) << "CreatePipe for the child input pipe failed";
§§		}
§§	}
§§}
§§
§§ExternalProcess::~ExternalProcess()
§§{
§§	die();
§§
§§	//Close the output / input redirection pipes
§§	if (m_outputPipe[0] != -1) {
§§		close(m_outputPipe[0]);
§§		m_outputPipe[0] = -1;
§§	}
§§	if (m_outputPipe[1] != -1) {
§§		close(m_outputPipe[1]);
§§		m_outputPipe[1] = -1;
§§	}
§§	if (m_inputPipe[0] != -1) {
§§		close(m_inputPipe[0]);
§§		m_inputPipe[0] = -1;
§§	}
§§	if (m_inputPipe[1] != -1) {
§§		close(m_inputPipe[1]);
§§		m_inputPipe[1] = -1;
§§	}
§§
§§	if (m_stdInOstream != nullptr) {
§§		delete m_stdInOstream;
§§		m_stdInOstream = nullptr;
§§	}
§§
§§	if (m_stdOutIstream != nullptr) {
§§		delete m_stdOutIstream;
§§		m_stdOutIstream = nullptr;
§§	}
§§
§§	if (m_stdInOstream_buf != nullptr) {
§§		delete m_stdInOstream_buf;
§§		m_stdInOstream_buf = nullptr;
§§	}
§§
§§	if (m_stdOutIstream_buf != nullptr) {
§§		delete m_stdOutIstream_buf;
§§		m_stdOutIstream_buf = nullptr;
§§	}
§§
§§	removeFromRunningProcessesSet(this);
§§
§§	//free m_argv
§§	if (m_argv != nullptr) {
§§		int i = 0;
§§		while (m_argv[i] != NULL) {
§§			free(m_argv[i]);
§§			m_argv[i] = NULL;
§§			i++;
§§		}
§§		free(m_argv);
§§		m_argv = nullptr;
§§	}
§§
§§	//free m_envp
§§	if (m_envp != nullptr) {
§§		int i = 0;
§§		while (m_envp[i] != NULL) {
§§			free(m_envp[i]);
§§			m_envp[i] = NULL;
§§			i++;
§§		}
§§		free(m_envp);
§§		m_envp = nullptr;
§§	}
§§}
§§
§§int ExternalProcess::getProcessID() {
§§	return m_childPID;
§§}
§§
§§char** ExternalProcess::split_commandline(const std::string  cmdline)
§§{
§§	char** argv = nullptr;
§§
§§	wordexp_t p;
§§
§§	// Note! This expands shell variables.
§§	int success = wordexp(cmdline.c_str(), &p, WRDE_NOCMD);
§§	if (success != 0)
§§	{
§§		LOG(DEBUG) << "wordexp \"" << cmdline << "\" failed: " << success;
§§		return nullptr;
§§	}
§§
§§	if (!(argv = static_cast<char**>(calloc(p.we_wordc + 1, sizeof(char*)))))
§§	{
§§		wordfree(&p);
§§		LOG(DEBUG) << "calloc failed";
§§		return nullptr;
§§	}
§§
§§	for (size_t i = 0; i < p.we_wordc; i++)
§§	{
§§		argv[i] = strdup(p.we_wordv[i]);
§§	}
§§	argv[p.we_wordc] = NULL;
§§
§§	wordfree(&p);
§§
§§	return argv;
§§}
§§
§§char** ExternalProcess::addToEnv(const std::string  additionalEnvParam)
§§{
§§	if (additionalEnvParam == "") {
§§		//Do not set m_envp, if there is nothing to add to the default environment
§§		return nullptr;
§§	}
§§
§§	unsigned int numOfEnvVariables = 0;
§§	while (__environ[numOfEnvVariables] != NULL) {
§§		numOfEnvVariables++;
§§	}
§§
§§	char** envp = static_cast<char**>(calloc(numOfEnvVariables + 2, sizeof(char*)));
§§
§§	if (!(envp))
§§	{
§§		LOG(DEBUG) << "calloc failed";
§§		return nullptr;
§§	}
§§
§§	for (size_t i = 0; i < numOfEnvVariables; i++)
§§	{
§§		envp[i] = strdup(__environ[i]);
§§	}
§§	envp[numOfEnvVariables] = strdup(additionalEnvParam.c_str());
§§	envp[numOfEnvVariables + 1] = NULL;
§§
§§	return envp;
§§}
§§
§§bool ExternalProcess::initProcess()
§§{
§§	return initProcessInChrootEnv("");
§§}
§§
§§bool ExternalProcess::initProcessInChrootEnv(const std::string  chrootPath) {
§§	// Creating child process. It will execute ExecuteCommandLine().
§§
§§	if (pipe(m_fd_pipe) == -1)
§§	{
§§		LOG(DEBUG) << "Creating a pipe between debugee and debugger failed";
§§		return false;
§§	}
§§
§§	// Parent process will continue to DebugChild().
§§	m_childPID = fork();
§§	if (m_childPID == 0) {
§§		//this is executed in the new process, in which easylogging is not available (at least it seems so)
§§
§§		//restore signal mask
§§		sigdelset(&m_originalMask, SIGCHLD); //the current mask may or may not be set for SIGCHLD - depending on whether this is the first external process or not
§§		sigset_t mask;
§§		if (sigprocmask(SIG_SETMASK, &m_originalMask, &mask) < 0) {
§§			printf("sigprocmask failed (fork): %s\n", strerror(errno));
§§			google::protobuf::ShutdownProtobufLibrary();
§§			_exit(EXIT_FAILURE); //Make compiler happy
§§		}
§§
§§		if (m_child_output_mode == CHILD_OUTPUT_TYPE::SUPPRESS) {
§§			/* open /dev/null for writing */
§§			int fd = open("/dev/null", O_WRONLY);
§§			if (fd < 0) {
§§				printf("open /dev/null failed: %s\n", strerror(errno));
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //Make compiler happy
§§			}
§§
§§			if (-1 == dup2(fd, STDOUT_FILENO)) {    /* make stdout a copy of fd (> /dev/null) */
§§				printf("Failed setting STDOUT_FILENO with dup2\n");
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //Make compiler happy
§§			}
§§			if (-1 == dup2(fd, STDERR_FILENO)) {    /* ...and same with stderr */
§§				printf("Failed setting STDERR_FILENO with dup2\n");
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //Make compiler happy
§§			}
§§			close(fd);      /* close fd */
§§			fd = -1;
§§
§§			/* stdout and stderr now write to /dev/null */
§§		}
§§		else if (m_child_output_mode == CHILD_OUTPUT_TYPE::SPECIAL) {
§§			if (-1 == dup2(m_inputPipe[0], STDIN_FILENO)) {
§§				printf("Failed setting STDIN_FILENO with dup2\n");
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //Make compiler happy
§§			}
§§			if (-1 == dup2(m_outputPipe[1], STDOUT_FILENO)) {
§§				printf("Failed setting STDOUT_FILENO with dup2\n");
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //Make compiler happy
§§			}
§§			if (-1 == dup2(m_outputPipe[1], STDERR_FILENO)) {
§§				printf("Failed setting STDERR_FILENO with dup2\n");
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //Make compiler happy
§§			}
§§
§§			//Close all input redirection pipe ends. They are not needed by the client (any more)
§§
§§			close(m_outputPipe[0]);
§§			m_outputPipe[0] = -1;
§§
§§			close(m_outputPipe[1]);
§§			m_outputPipe[1] = -1;
§§
§§			close(m_inputPipe[0]);
§§			m_inputPipe[0] = -1;
§§
§§			close(m_inputPipe[1]);
§§			m_inputPipe[1] = -1;
§§		}
§§		else {
§§			//forward stdin and stdout
§§		}
§§
§§		if (chrootPath != "") {
§§			if (chroot(chrootPath.c_str()) != 0) {
§§				printf("chroot failed: %s\n", strerror(errno));
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //Make compiler happy
§§			}
§§			if (chdir("/") != 0) {
§§				printf("chdir failed: %s\n", strerror(errno));
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //Make compiler happy
§§			}
§§		}
§§
§§		{
§§			//Wait until the parent thread is ready
§§			close(m_fd_pipe[1]);  // Close writing  end of debuggee / debugger pipe
§§			m_fd_pipe[1] = -1;
§§			char buf[1];
§§			ssize_t  bytesRead = read(m_fd_pipe[0], buf, 1);
§§			if (bytesRead != 1) {
§§				printf("Waiting for the parent thread failed! read returned %d", static_cast<int>(bytesRead));
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //Make compiler happy
§§			}
§§			close(m_fd_pipe[0]);  // Close reading  end of debuggee / debugger pipe
§§			m_fd_pipe[0] = -1;
§§		}
§§
§§		if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
§§		{
§§			printf("ptrace PTRACE_TRACEME failed: %s\n", strerror(errno));
§§			google::protobuf::ShutdownProtobufLibrary();
§§			_exit(EXIT_FAILURE); //Make compiler happy
§§		}
§§
§§		if (raise(SIGSTOP) != 0) {
§§			printf("raise(SIGSTOP) failed: %s\n", strerror(errno));
§§			google::protobuf::ShutdownProtobufLibrary();
§§			_exit(EXIT_FAILURE); //Make compiler happy
§§		}
§§
§§		if (m_envp != nullptr) {
§§			execve(m_argv[0], m_argv, m_envp);
§§		}
§§		else {
§§			execv(m_argv[0], m_argv);
§§		}
§§
§§		//This code is only executed if execve failed, e.g. if the file does not exist
§§		printf("execve failed: %s; %s\n", strerror(errno), m_argv[0]);
§§		google::protobuf::ShutdownProtobufLibrary();
§§		_exit(EXIT_FAILURE); //Make compiler happy
§§	}
§§	else if (m_childPID > 0) {
§§		close(m_fd_pipe[0]);  // Close reading end of debuggee / debugger pipe
§§		m_fd_pipe[0] = -1;
§§
§§		//Close input output redirection pipe ends that are only needed by child
§§		if (m_child_output_mode == CHILD_OUTPUT_TYPE::SPECIAL) {
§§			close(m_outputPipe[1]);
§§			m_outputPipe[1] = -1;
§§
§§			close(m_inputPipe[0]);
§§			m_inputPipe[0] = -1;
§§		}
§§
§§		LOG(DEBUG) << "Initialization for " << m_argv[0] << " completed";
§§
§§		m_hasBeenInitialized = true;
§§		return true;
§§	}
§§	else {
§§		LOG(ERROR) << "Fork failed:" << errno;
§§		return false;
§§	}
§§}
§§
§§bool ExternalProcess::runAndWaitForCompletion(unsigned long timeoutMilliseconds) {
§§	if (m_hasBeenRun || !m_hasBeenInitialized) {
§§		LOG(ERROR) << "The process cannot run, as it is not in a runnable state";
§§		return false;
§§	}
§§	m_hasBeenRun = true;
§§
§§	struct timespec timeout;
§§
§§	timeout.tv_sec = timeoutMilliseconds / 1000;
§§	timeout.tv_nsec = (timeoutMilliseconds % 1000) * 1000000;
§§
§§	int status = 0;
§§	bool isfirstStop = true;
§§
§§	//tell the debuggee to continue
§§	{
§§		char buf[] = "X";
§§		ssize_t  bytesWritten = write(m_fd_pipe[1], buf, 1);
§§		if (bytesWritten != 1) {
§§			LOG(ERROR) << "Failed writing an X to m_fd_pipe[1]";
§§		}
§§		close(m_fd_pipe[1]);  // Close writing  end of debuggee / debugger pipe
§§		m_fd_pipe[1] = -1;
§§	}
§§
§§	do {
§§		//Wait for a SIGCHLD event
§§		errno = 0;
§§		int sig = sigtimedwait(&m_debugSignalMask, NULL, &timeout);
§§		if (sig < 0) {
§§			if (errno == EINTR) {
§§				/* Interrupted by a signal other than SIGCHLD. */
§§				LOG(DEBUG) << "The wait on process " << m_childPID << " for SIGCHLD was interrupted by a signal other than SIGCHLD: " << sig;
§§				ptrace(PTRACE_CONT, m_childPID, NULL, NULL);
§§				continue;
§§			}
§§			else if (errno == EAGAIN) {
§§				LOG(WARNING) << "Process hang detected";
§§				die();
§§				return false;
§§			}
§§			else {
§§				LOG(ERROR) << "sigtimedwait failed: " << strerror(errno);
§§				die();
§§				return false;
§§			}
§§		}
§§
§§		updateTimeSpent();//This might be a significant performance overhead! Remove if that turns out to be true (or change to "once every 100 times...")
§§
§§		//Once SIGCHLD was received - the next signal will tell us what happened
§§		pid_t signalSourcePID = waitpid(m_childPID, &status, WNOHANG);
§§		if (signalSourcePID == -1) {
§§			die();
§§			return false;
§§		}
§§		else if (signalSourcePID == 0) {
§§			LOG(DEBUG) << "We got a sigchild but there is no signal for " << m_childPID;
§§			continue;
§§		}
§§
§§		LOG(DEBUG) << "Debuggee " << m_childPID << " got signal " << status;
§§		if (signalSourcePID != m_childPID) {
§§			LOG(DEBUG) << "The signal we got is not actually from our child (PID " << m_childPID << "). It is from PID " << signalSourcePID << ". We ignore this for now (1)";
§§			continue;
§§		}
§§
§§		if (WIFSTOPPED(status))
§§		{
§§			//program stoped. Continue on first stop, kill on second (if it's not a sigtrap)
§§			if (isfirstStop) {
§§				isfirstStop = false;
§§				LOG(DEBUG) << "Continuing on initial Stop ...";
§§				ptrace(PTRACE_CONT, m_childPID, NULL, NULL);
§§				continue;
§§			}
§§
§§			if (WSTOPSIG(status) == SIGTRAP) {
§§				LOG(DEBUG) << "Continuing SIGTRAP (most likely caused by execve) ...";
§§				ptrace(PTRACE_CONT, m_childPID, NULL, NULL);
§§				continue;
§§			}
§§			else if (WSTOPSIG(status) == SIGCHLD) {
§§				LOG(DEBUG) << "Our child received a SIGCHLD. Most likely, the child executed a fork";
§§				ptrace(PTRACE_CONT, m_childPID, NULL, NULL);
§§				continue;
§§			}
§§			else {
§§				LOG(DEBUG) << "Debuggee " << m_childPID << " stopped with signal " << WSTOPSIG(status) << " as this is not the initial stop we kill it!";
§§				die();
§§				break;
§§			}
§§
§§			LOG(ERROR) << "Reached a code path that should never be reached!";
§§			break;
§§		}
§§
§§		if (WIFSIGNALED(status))
§§		{
§§			//process exited as it was handed a signal that it could not handle
§§			LOG(DEBUG) << "Debuggee " << m_childPID << " exited as it could not handle signal " << WTERMSIG(status);
§§			break;
§§		}
§§		if (WIFEXITED(status))
§§		{
§§			//process exited normally
§§			LOG(DEBUG) << "Debuggee " << m_childPID << " exited cleanly with status " << WEXITSTATUS(status);
§§			m_exitStatus = WEXITSTATUS(status);
§§			break;
§§		}
§§	} while (true);
§§
§§	return true;
§§}
§§
§§bool ExternalProcess::run() {
§§	if (m_hasBeenRun || !m_hasBeenInitialized) {
§§		LOG(ERROR) << "The process cannot run, as it is not in a runnable state";
§§		return false;
§§	}
§§
§§	m_hasBeenRun = true;
§§
§§	struct timespec timeout; //timeout for the initial "stop" signal. Once we received that-> detach
§§	timeout.tv_sec = 100;
§§	timeout.tv_nsec = 0;
§§
§§	int status = 0;
§§
§§	//tell the debuggee to continue
§§	{
§§		char buf[] = "X";
§§		ssize_t  bytesWritten = write(m_fd_pipe[1], buf, 1);
§§		if (bytesWritten != 1) {
§§			LOG(ERROR) << "Failed writing an X to m_fd_pipe[1]";
§§		}
§§		close(m_fd_pipe[1]);  // Close writing  end of debuggee / debugger pipe
§§		m_fd_pipe[1] = -1;
§§	}
§§
§§	do {
§§		//Wait for a SIGCHLD event
§§		errno = 0;
§§		int sig = sigtimedwait(&m_debugSignalMask, NULL, &timeout);
§§		if (sig < 0) {
§§			if (errno == EINTR) {
§§				/* Interrupted by a signal other than SIGCHLD. */
§§				LOG(DEBUG) << "The wait on process " << m_childPID << " for SIGCHLD was interrupted by a signal other than SIGCHLD: " << sig;
§§				ptrace(PTRACE_CONT, m_childPID, NULL, NULL);
§§				continue;
§§			}
§§			else if (errno == EAGAIN) {
§§				LOG(WARNING) << "Process hang detected";
§§				die();
§§				return false;
§§			}
§§			else {
§§				LOG(ERROR) << "sigtimedwait failed: " << strerror(errno);
§§				die();
§§				return false;
§§			}
§§		}
§§
§§		updateTimeSpent();//This might be a significant performance overhead! Remove if that turns out to be true (or change to "once every 100 times...")
§§
§§		//Once SIGCHLD was received - the next signal will tell us what happened
§§		pid_t signalSourcePID = waitpid(m_childPID, &status, WNOHANG);
§§		if (signalSourcePID == -1) {
§§			die();
§§			return false;
§§		}
§§		else if (signalSourcePID == 0) {
§§			LOG(DEBUG) << "We got a sigchild but there is no signal for " << m_childPID;
§§			continue;
§§		}
§§
§§		LOG(DEBUG) << "Debuggee " << m_childPID << " got signal " << status;
§§		if (signalSourcePID != m_childPID) {
§§			LOG(DEBUG) << "The signal we got is not actually from our child (PID " << m_childPID << "). It is from PID " << signalSourcePID << ". We ignore this for now(2)";
§§			continue;
§§		}
§§
§§		if (WIFSTOPPED(status))
§§		{
§§			//first stop
§§			LOG(DEBUG) << "Detaching " << m_childPID << " on initial Stop ...";
§§			ptrace(PTRACE_DETACH, m_childPID, NULL, NULL);
§§			break;
§§		}
§§	} while (true);
§§
§§	return true;
§§}
§§
§§void ExternalProcess::die() {
§§	if (m_hasBeenInitialized) {
§§		LOG(DEBUG) << "Telling PID " << m_childPID << " (\"" << m_argv[0] << "\") to die...";
§§
§§		updateTimeSpent();
§§		if (m_childPID != 0) {
§§			kill(m_childPID, SIGKILL); // idea: Send kill to -pid to kill the entire process group (if we need this)
§§
§§			int status;
§§			pid_t retunedPID = 0;
§§			do {
§§				retunedPID = waitpid(m_childPID, &status, 0);
§§			} while (retunedPID >= 0);
§§		}
§§		LOG(DEBUG) << "PID " << m_childPID << " Is finally gone :)";
§§	}
§§}
§§
§§std::string ExternalProcess::addrToRVAString(pid_t childPID, std::uintptr_t addr) {
§§	std::stringstream fileNameSS;
§§	fileNameSS << "/proc/" << childPID << "/maps";
§§	std::ifstream mapsFile(fileNameSS.str(), std::ifstream::in);
§§
§§	std::string re = "";
§§	std::string line;
§§	while (std::getline(mapsFile, line))
§§	{
§§		std::vector<std::string>  lineElements = Util::splitString(line, " ");
§§		if (lineElements.size() < 2) {
§§			LOG(INFO) << "Splitting line " << line << "failed";
§§			continue;
§§		}
§§		std::vector<std::string>  addresses = Util::splitString(lineElements[0], "-");
§§		if (addresses.size() < 2) {
§§			LOG(INFO) << "Splitting addresses " << lineElements[0] << "failed";
§§			continue;
§§		}
§§		if (stoull(addresses[0], 0, 16) < addr && stoull(addresses[1], 0, 16) > addr)
§§		{
§§			std::stringstream oss;
§§			oss << std::experimental::filesystem::path(lineElements[lineElements.size() - 1]).filename().string() << "+0x" << std::hex << std::setw(8) << std::setfill('0') << addr - stoull(addresses[0], 0, 16);
§§			re = oss.str();
§§			break;
§§		}
§§	}
§§
§§	mapsFile.close();
§§
§§	if (re != "") {
§§		return re;
§§	}
§§	else {
§§		//apparently, the address is not part of any loaded module
§§		std::stringstream oss;
§§		oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr;
§§		return oss.str();
§§	}
§§}
§§
§§//taken from http://proswdev.blogspot.com/2012/02/get-process-id-by-name-in-linux-using-c.html?_sm_au_=iVVN0HW56nFrsHSr
§§pid_t ExternalProcess::getPIDForProcessName(std::string processName) {
§§	LOG(DEBUG) << "processName: " << processName;
§§
§§	pid_t pid = -1;
§§
§§	// Open the /proc directory
§§	DIR* dp = opendir("/proc");
§§	if (dp != NULL)
§§	{
§§		// Enumerate all entries in directory until process found
§§		struct dirent* dirp;
§§		while ((dirp = readdir(dp)))
§§		{
§§			// Skip non-numeric entries
§§			int id = atoi(dirp->d_name);
§§			if (id > 0)
§§			{
§§				// Read contents of virtual /proc/{pid}/cmdline file
§§				std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
§§				std::ifstream cmdFile(cmdPath.c_str());
§§				std::string cmdLine;
§§				getline(cmdFile, cmdLine);
§§				if (!cmdLine.empty())
§§				{
§§					// Keep first cmdline item which contains the program path
§§					size_t pos = cmdLine.find('\0');
§§					if (pos != std::string::npos) {
§§						cmdLine = cmdLine.substr(0, pos);
§§					}
§§					/*
§§					// Keep program name only, removing the path
§§					pos = cmdLine.rfind('/');
§§					if (pos != std::string::npos) {
§§						cmdLine = cmdLine.substr(pos + 1);
§§					}
§§					*/
§§					// Compare against requested process name
§§					if (processName == cmdLine) {
§§						LOG(DEBUG) << "PID " << id << " matches process name " << processName;
§§						if (pid != -1) {
§§							LOG(ERROR) << "getPIDForProcessName: There are multiple processes with the process name " << processName;
§§							closedir(dp);
§§							return -1;
§§						}
§§						pid = id;
§§					}
§§				}
§§			}
§§		}
§§	}
§§
§§	closedir(dp);
§§
§§	if (pid == -1) {
§§		LOG(ERROR) << "getPIDForProcessName: Could not find a process with name " << processName;
§§	}
§§
§§	return pid;
§§}
§§
§§bool ExternalProcess::attachToProcess() {
§§	m_childPID = getPIDForProcessName(m_argv[0]);
§§	LOG(DEBUG) << "attaching to PID " << m_childPID;
§§	if (m_childPID < 0) {
§§		m_childPID = 0;
§§		LOG(ERROR) << "attachToProcess: Could not attach to target process ";
§§		return false;
§§	}
§§
§§	if (ptrace(PTRACE_ATTACH, m_childPID, 0, 0) == -1)
§§	{
§§		LOG(ERROR) << "ptrace PTRACE_ATTACH failed: " << strerror(errno);
§§		return false;
§§	}
§§
§§	// there is no communication pipe to the debuggee. Right now let the write calls write to /dev/null. Alternatively: Store if we attached in a member variable and act based on it
§§	m_fd_pipe[1] = open("/dev/null", O_WRONLY);
§§
§§	m_hasBeenInitialized = true;
§§	return true;
§§}
§§
§§//For list of signals see https://de.wikipedia.org/wiki/Signal_(Unix)
§§void ExternalProcess::debug(unsigned long timeoutMilliseconds, std::shared_ptr<DebugExecutionOutput> exResult, bool doPostMortemAnalysis, bool treatAnyAccessViolationAsFatal)
§§{
§§	if (m_hasBeenRun || !m_hasBeenInitialized) {
§§		std::stringstream oss;
§§		oss << "The process " << m_childPID << " cannot run, as it is not in a runnable state" << std::endl;
§§		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§		exResult->m_terminationDescription = oss.str();
§§		return;
§§	}
§§
§§	uint64_t firstCrash = 0;
§§	uint64_t lastCrash = 0;
§§
§§	//So far there is no exception
§§	exResult->m_PID = m_childPID;
§§	exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN;
§§	exResult->m_terminationDescription = "The target executed without any exception";
§§
§§	struct timespec timeout;
§§	timeout.tv_sec = timeoutMilliseconds / 1000;
§§	timeout.tv_nsec = (timeoutMilliseconds % 1000) * 1000000;
§§
§§	int status = 0;
§§	bool isfirstStop = true;
§§
§§	//tell the debuggee to continue
§§	{
§§		char buf[] = "X";
§§		ssize_t  bytesWritten = write(m_fd_pipe[1], buf, 1);
§§		if (bytesWritten != 1) {
§§			LOG(ERROR) << "Failed writing an X to m_fd_pipe[1]";
§§		}
§§		close(m_fd_pipe[1]);  // Close writing  end of debuggee / debugger pipe
§§		m_fd_pipe[1] = -1;
§§	}
§§
§§	//Ready to debug
§§	m_isBeingDebugged = true;
§§
§§	do {
§§		//Wait for a SIGCHLD event
§§		errno = 0;
§§		int sig = sigtimedwait(&m_debugSignalMask, NULL, &timeout);
§§		if (sig < 0) {
§§			if (errno == EINTR) {
§§				/* Interrupted by a signal other than SIGCHLD. */
§§				LOG(DEBUG) << "The wait on process " << m_childPID << " for SIGCHLD was interrupted by a signal other than SIGCHLD: " << sig;
§§				ptrace(PTRACE_CONT, m_childPID, NULL, NULL);
§§				continue;
§§			}
§§			else if (errno == EAGAIN) {
§§				exResult->m_terminationDescription = "Process hang detected!";
§§				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::TIMEOUT;
§§				die();
§§				m_hasBeenRun = true;
§§				return;
§§			}
§§			else {
§§				std::stringstream oss;
§§				oss << "sigtimedwait failed: " << strerror(errno);
§§				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§				exResult->m_terminationDescription = oss.str();
§§				LOG(ERROR) << exResult->m_terminationDescription;
§§				die();
§§				m_hasBeenRun = true;
§§				return;
§§			}
§§		}
§§		updateTimeSpent();//This might be a significant performance overhead! Remove if that turns out to be true (or change to "once every 100 times...")
§§
§§		//Once SIGCHLD was received - the next signal will tell us what happened
§§		pid_t signalSourcePID = waitpid(m_childPID, &status, WNOHANG);
§§		if (signalSourcePID == -1) {
§§			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
§§			exResult->m_terminationDescription = "waitpid encountered an error";
§§			die();
§§			m_hasBeenRun = true;
§§			return;
§§		}
§§		else if (signalSourcePID == 0) {
§§			LOG(DEBUG) << "We got a sigchild but there is no signal for " << m_childPID;
§§			continue;
§§		}
§§
§§		LOG(DEBUG) << "Debuggee " << m_childPID << " got signal " << status;
§§		if (signalSourcePID != m_childPID) {
§§			LOG(DEBUG) << "The signal we got is not actually from our child (PID " << m_childPID << "). It is from PID " << signalSourcePID << ". We ignore this for now (3)";
§§			continue;
§§		}
§§
§§		if (WIFSTOPPED(status))
§§		{
§§			LOG(DEBUG) << "Child " << m_childPID << " has stopped due to signal " << WSTOPSIG(status);
§§
§§			//Continue on first stop. This stems from attach or create
§§			if (isfirstStop) {
§§				isfirstStop = false;
§§				LOG(DEBUG) << "Continuing " << m_childPID << " on initial Stop ...";
§§				ptrace(PTRACE_CONT, m_childPID, NULL, NULL);
§§				continue;
§§			}
§§
§§			if (WSTOPSIG(status) == SIGTRAP) {
§§				LOG(DEBUG) << "Continuing " << m_childPID << " on SIGTRAP (most likely caused by execve)";
§§				ptrace(PTRACE_CONT, m_childPID, NULL, NULL);
§§			}
§§			else if (WSTOPSIG(status) == SIGCHLD) {
§§				LOG(DEBUG) << "Our child received a SIGCHLD. Most likely, the child executed a fork";
§§				ptrace(PTRACE_CONT, m_childPID, NULL, NULL);
§§				continue;
§§			}
§§			else if (WSTOPSIG(status) == SIGILL && m_targetIsSentNudges) {
§§				LOG(DEBUG) << "Continuing " << m_childPID << " on SIGILL as we run under dynamorio (SIGILL - is used to nudge dynamorio on linux)";
§§				ptrace(PTRACE_CONT, m_childPID, NULL, WSTOPSIG(status));
§§				continue;
§§			}
§§			else {
§§				//Target is stoped but not due to SIGTRAP and not due to the initial SIGSTOP
§§#if defined(__ia64__) || defined(__x86_64__) || defined(__i386__)
§§				user_regs_struct regs;
§§#elif defined(__arm__)
§§				struct user regs;
§§#elif defined(__aarch64__)
§§				struct user_pt_regs regs;
§§				struct iovec io;
§§				io.iov_base = &regs;
§§				io.iov_len = sizeof(regs);
§§#endif
§§				memset(&regs, 0, sizeof(regs));
§§
§§#if defined(__aarch64__)
§§				ptrace(PTRACE_GETREGSET, m_childPID, (void*)NT_PRSTATUS, &io);
§§#else
§§				ptrace(PTRACE_GETREGS, m_childPID, NULL, &regs);
§§#endif
§§
§§#if defined(__ia64__) || defined(__x86_64__)
§§				std::uintptr_t instructionPointer = regs.rip;
§§#elif defined(__i386__)
§§				std::uintptr_t instructionPointer = regs.eip;
§§#elif defined(__arm__)
§§				//see https://undo.io/resources/blog-articles/arm64-vs-arm32-whats-different-linux-programmers/
§§				std::uintptr_t instructionPointer = regs.regs.uregs[15];
§§#elif defined(__aarch64__)
§§				//see https://undo.io/resources/blog-articles/arm64-vs-arm32-whats-different-linux-programmers/
§§				std::uintptr_t instructionPointer = regs.pc;
§§#endif
§§
§§				if (WSTOPSIG(status) == SIGSEGV) {
§§					LOG(DEBUG) << "Detected an EXCEPTION_ACCESS_VIOLATION";
§§					exResult->m_terminationDescription = "Detected an EXCEPTION_ACCESS_VIOLATION";
§§					exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION;
§§				}
§§				else {
§§					std::stringstream oss;
§§					oss << "Detected exception " << WSTOPSIG(status);
§§					exResult->m_terminationDescription = oss.str();
§§					exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
§§				}
§§
§§				if (firstCrash == 0) {
§§					firstCrash = instructionPointer;
§§					if (doPostMortemAnalysis) {
§§						exResult->m_firstCrash = addrToRVAString(m_childPID, instructionPointer);
§§					}
§§				}
§§
§§				//The big question here is what should happen upon an exception. Should we try to let the program solve this situation or report the exception
§§				if (WSTOPSIG(status) == SIGSEGV  && treatAnyAccessViolationAsFatal) {
§§					// Option 1) Report the exception, as it might be an exploitable access violation
§§					if (doPostMortemAnalysis) {
§§						exResult->m_lastCrash = addrToRVAString(m_childPID, instructionPointer);
§§					}
§§					LOG(DEBUG) << "The application received a SIGSEGV and we treat any access violation as fatal";
§§					die();
§§				}
§§				else {
§§					//Option 2) Forward the exception and see if the application can handle it. Kill it if not
§§					if (lastCrash != instructionPointer) {
§§						lastCrash = instructionPointer;
§§						if (doPostMortemAnalysis) {
§§							exResult->m_lastCrash = addrToRVAString(m_childPID, instructionPointer);
§§						}
§§						LOG(DEBUG) << "Let's see if the application " << m_childPID << " can handle this exception";
§§						ptrace(PTRACE_CONT, m_childPID, NULL, WSTOPSIG(status));
§§					}
§§					else {
§§						//The application was unable to handle this exception
§§						LOG(DEBUG) << "The application was unable to handle this exception - killing it";
§§						die();
§§					}
§§				}
§§			}
§§		}
§§
§§		if (WIFSIGNALED(status))
§§		{
§§			LOG(DEBUG) << "Debuggee terminated because it received a signal that was not handled: " << WTERMSIG(status);
§§			if (exResult->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN) {
§§				LOG(DEBUG) << "Updating m_terminationType as there was no exception observed before";
§§				std::stringstream oss;
§§				oss << "Detected exception " << WTERMSIG(status);
§§				exResult->m_terminationDescription = oss.str();
§§				exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER;
§§			}
§§			break;
§§		}
§§
§§		if (WIFEXITED(status))
§§		{
§§			m_exitStatus = WEXITSTATUS(status);
§§
§§			LOG(DEBUG) << "Debuggee terminated normally: " << WEXITSTATUS(status);
§§			exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN;
§§			exResult->m_terminationDescription = "The target terminated normally";
§§			break;
§§		}
§§	} while (true);
§§
§§	m_hasBeenRun = true;
§§	return;
§§}
§§
§§void ExternalProcess::updateTimeSpent() {
§§	if (m_hasBeenInitialized && m_childPID != 0) {
§§		//TBH I don't know how to implement this recursively without blowing it up imensely. Just thinking about child processes that terminated, which might calculate "negative" ticks since last count.
§§		//We stick to performance of the specified process for now
§§
§§		unsigned long utime = 0, stime = 0, cutime = 0, cstime = 0; //divide by  sysconf(_SC_CLK_TCK) to get seconds
§§		FILE* file = fopen(("/proc/" + std::to_string(m_childPID) + "/stat").c_str(), "r");
§§		if (file == NULL) {
§§			LOG(ERROR) << "Could not open /proc/[pid]/stat for process " << m_childPID;
§§		}
§§		else {
§§			int elementsParsed = fscanf(file, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %ld %ld", &utime, &stime, &cutime, &cstime);
§§			fclose(file);
§§			if (elementsParsed != 4) {
§§				LOG(WARNING) << "updateTimeSpent failed parsing /proc/stat:" << elementsParsed;
§§			}
§§
§§			s_allTimeSpent += (utime - m_lastutime) + (stime - m_laststime) + (cutime - m_lastcutime) + (cstime - m_lastcstime);
§§			m_lastutime = utime;
§§			m_laststime = stime;
§§			m_lastcutime = cutime;
§§			m_lastcstime = cstime;
§§		}
§§	}
§§}
§§
§§#endif
§§
§§bool ExternalProcess::waitForProcessInitialization(int timeoutMS) {
§§	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
§§	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(timeoutMS);
§§
§§	while (!m_hasBeenInitialized && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
§§		std::this_thread::sleep_for(std::chrono::milliseconds(1));
§§	}
§§
§§	return m_hasBeenInitialized;
§§}
§§
§§bool ExternalProcess::waitUntilProcessIsBeingDebugged(int timeoutMS) {
§§	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
§§	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(timeoutMS);
§§
§§	while (!m_isBeingDebugged && std::chrono::steady_clock::now() < latestRoutineExitTimeStamp) {
§§		std::this_thread::sleep_for(std::chrono::milliseconds(1));
§§	}
§§
§§	return m_isBeingDebugged;
§§}
§§
§§TIMETYPE ExternalProcess::getAllTimeSpentSinceLastCall() {
§§	updateTimeSpentOnAllInRunningProcessesSet();
§§	TIMETYPE re = s_allTimeSpent;
§§	s_allTimeSpent = { 0 };
§§	return re;
§§}
§§
§§void ExternalProcess::addToRunningProcessesSet(ExternalProcess* thisp) {
§§	std::unique_lock<std::mutex> mlock(s_runningProcessSet_mutex_);
§§	s_runningProcessSet.insert(thisp);
§§}
§§
§§bool ExternalProcess::isInRunningProcessesSet(int pid) {
§§	std::unique_lock<std::mutex> mlock(s_runningProcessSet_mutex_);
§§
§§	for (ExternalProcess* runningProcess : s_runningProcessSet) {
§§		if (runningProcess->getProcessID() == pid) {
§§			return true;
§§		}
§§	}
§§	return false;
§§}
§§
§§void ExternalProcess::removeFromRunningProcessesSet(ExternalProcess* thisp) {
§§	std::unique_lock<std::mutex> mlock(s_runningProcessSet_mutex_);
§§	if (s_runningProcessSet.count(thisp) > 0) {
§§		s_runningProcessSet.erase(thisp);
§§	}
§§	else {
§§		LOG(ERROR) << "Attempting to remove a external process from the s_runningProcessSet, which has never been inserted";
§§	}
§§}
§§void ExternalProcess::updateTimeSpentOnAllInRunningProcessesSet() {
§§	std::unique_lock<std::mutex> mlock(s_runningProcessSet_mutex_);
§§
§§	for (ExternalProcess* runningProcess : s_runningProcessSet)
§§	{
§§		runningProcess->updateTimeSpent();
§§	}
§§}
§§
§§int ExternalProcess::getExitStatus() {
§§	return m_exitStatus;
§§}
§§
§§HANDLETYPE ExternalProcess::getStdInHandle() {
§§	return m_inputPipe[1];
§§}
§§HANDLETYPE ExternalProcess::getStdOutHandle() {
§§	return m_outputPipe[0];
§§}
§§
§§std::istream* ExternalProcess::getStdOutIstream() {
§§	if (m_child_output_mode != SPECIAL) {
§§		return nullptr;
§§	}
§§
§§	if (m_stdOutIstream == nullptr) {
§§#if defined(_WIN32) || defined(_WIN64)
§§		int fd = _open_osfhandle((intptr_t)getStdOutHandle(), _O_RDONLY);
§§		if (fd == -1) {
§§			LOG(ERROR) << "open_osfhandle failed";
§§			return nullptr;
§§		}
§§		FILE* fp = _fdopen(fd, "r");
§§		if (fp == NULL) {
§§			LOG(ERROR) << "_fdopen failed";
§§			_close(fd);
§§			return nullptr;
§§		}
§§
§§		m_stdOutIstream = new std::ifstream(fp);
§§#else
§§
§§		m_stdOutIstream_buf = new __gnu_cxx::stdio_filebuf<char>(getStdOutHandle(), std::ios::in);
§§		m_stdOutIstream = new  std::istream(m_stdOutIstream_buf);
§§
§§#endif
§§	}
§§	return m_stdOutIstream;
§§}
§§
§§std::ostream* ExternalProcess::getStdInOstream() {
§§	if (m_child_output_mode != SPECIAL) {
§§		return nullptr;
§§	}
§§
§§	if (m_stdInOstream == nullptr) {
§§#if defined(_WIN32) || defined(_WIN64)
§§		int fd = _open_osfhandle((std::intptr_t)getStdInHandle(), _O_WRONLY);
§§		if (fd == -1) {
§§			LOG(ERROR) << "open_osfhandle failed";
§§			return nullptr;
§§		}
§§		FILE* fp = _fdopen(fd, "a");
§§		if (fp == NULL) {
§§			LOG(ERROR) << "_fdopen failed";
§§			_close(fd);
§§			return nullptr;
§§		}
§§
§§		m_stdInOstream = new std::ofstream(fp);
§§#else
§§
§§		m_stdInOstream_buf = new __gnu_cxx::stdio_filebuf<char>(getStdInHandle(), std::ios::out);
§§		m_stdInOstream = new  std::ostream(m_stdInOstream_buf);
§§
§§#endif
§§	}
§§	return m_stdInOstream;
§§}
