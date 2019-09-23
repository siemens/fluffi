/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include "Psapi.h"
#include <shellapi.h>
#include <tlhelp32.h>
#include <fcntl.h>
#include <io.h>

EXTERN_C SIZE_T NTAPI NtSetInformationProcess(HANDLE, ULONG, PVOID, ULONG);
#else
#include <wordexp.h>
#include <sys/ptrace.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ext/stdio_filebuf.h>

#if defined(__aarch64__)
#include <asm/ptrace.h>
#include <elf.h>
#elif defined(__arm__)
#include <sys/user.h>
#elif defined(__ia64__) || defined(__x86_64__)
#include <sys/user.h>
#elif defined(__i386__)
#include <sys/user.h>
#endif
#endif

#include <atomic>

#if defined(_WIN32) || defined(_WIN64)
#define TIMETYPE LONGLONG
#define HANDLETYPE HANDLE
#else
#define TIMETYPE unsigned long long
#define HANDLETYPE int
#endif

class DebugExecutionOutput;
class ExternalProcess
{
public:

	virtual ~ExternalProcess();

	bool attachToProcess();
	void debug(unsigned long timeoutMilliseconds, std::shared_ptr<DebugExecutionOutput> exResult, bool doPostMortemAnalysis, bool treatAnyAccessViolationAsFatal);
	void die();
	int getExitStatus();
	int getProcessID();
	HANDLETYPE getStdInHandle();
§§	std::ostream* getStdInOstream();
	HANDLETYPE getStdOutHandle();
§§	std::istream* getStdOutIstream();
	bool initProcess();
	bool run();
	bool runAndWaitForCompletion(unsigned long timeoutMilliseconds);
	bool waitForProcessInitialization(int timeoutMS);//only used if another process does the initialization
	bool waitUntilProcessIsBeingDebugged(int timeoutMS);//only used if another process does the initialization

	static TIMETYPE getAllTimeSpentSinceLastCall();

	typedef enum { SUPPRESS, OUTPUT, SPECIAL } CHILD_OUTPUT_TYPE;

#if defined(_WIN32) || defined(_WIN64)
	ExternalProcess(std::string commandline, CHILD_OUTPUT_TYPE child_output_mode);
	void setAllowBreakAway();
#else
	ExternalProcess(std::string commandline, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, std::string additionalEnvParam = "", bool targetIsSentNudges = false);
	bool initProcessInChrootEnv(const std::string  chrootPath);
	static char** split_commandline(const std::string  cmdline);
	static char** addToEnv(const std::string  additionalEnvParam);
#endif

#ifndef _VSTEST
private:
#endif //_VSTEST

	CHILD_OUTPUT_TYPE m_child_output_mode;
	HANDLETYPE m_inputPipe[2]; //[1] is the writing end - [0] the reading end
	HANDLETYPE m_outputPipe[2];//[1] is the writing end - [0] the reading end
§§	bool m_hasBeenRun;
§§	bool m_hasBeenInitialized;
§§	bool m_isBeingDebugged;
	int m_exitStatus;
§§	std::ostream* m_stdInOstream;
§§	std::istream* m_stdOutIstream;

	static std::atomic<TIMETYPE> s_allTimeSpent;
	static std::unordered_set<ExternalProcess*> s_runningProcessSet;
	static std::mutex s_runningProcessSet_mutex_;

#if defined(_WIN32) || defined(_WIN64)
	std::string m_commandline;
	PROCESS_INFORMATION m_pi;
	HANDLE m_job_handle;
	LARGE_INTEGER m_lastTimeSpentInUsermode;
	LARGE_INTEGER m_lastTimeSpentInKernelmode;
	bool m_allowBreakaway;

	static std::string addrToRVAString(PROCESS_INFORMATION pi, std::uintptr_t addr);
	static BOOL CreateProcessInJob(HANDLE hJob, LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION ppi);
§§	static bool fillInMainThread(PROCESS_INFORMATION* m_pi); //needed when attach to process is used to set the main thread
	static DWORD getPIDForProcessName(std::string processName);

#else
	char** m_argv;
	char** m_envp;
	pid_t m_childPID;
	unsigned long m_lastutime;
	unsigned long m_laststime;
	unsigned long m_lastcutime;
	unsigned long m_lastcstime;
	sigset_t m_originalMask;
	sigset_t m_debugSignalMask;
	int m_fd_pipe[2];  // Used to store two ends of  pipe
	bool m_targetIsSentNudges;
§§	__gnu_cxx::stdio_filebuf<char>* m_stdInOstream_buf;
§§	__gnu_cxx::stdio_filebuf<char>* m_stdOutIstream_buf;

	static std::string addrToRVAString(pid_t childPID, std::uintptr_t addr);
	static pid_t getPIDForProcessName(std::string processName);
#endif

	void updateTimeSpent();
§§	static void addToRunningProcessesSet(ExternalProcess* thisp);
§§	static void removeFromRunningProcessesSet(ExternalProcess* thisp);
	static void updateTimeSpentOnAllInRunningProcessesSet();
	static bool isInRunningProcessesSet(int pid);
};
