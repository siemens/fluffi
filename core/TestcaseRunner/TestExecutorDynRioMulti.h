§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Roman Bendt, Abian Blome
§§*/
§§
§§#pragma once
§§#include "TestExecutorDynRio.h"
§§#include "SharedMemIPC.h"
§§
§§#if defined(_WIN64) || defined(_WIN32)
§§
§§#if defined(_WIN64)
§§//64 bit Windows
§§#define DRCONFIGLIB_PATH "dyndist64\\lib64\\drconfiglib.dll"
§§#define DRRUN_PATH "dyndist64\\bin64\\drrun.exe"
§§#define DRCOVMULTI_PATH "dyndist64\\clients\\lib64\\release\\drcovMulti.dll"
§§#define DYNRIO_DIR "\\dyndist64"
§§#define DYNRIO_LIBDIR "lib64"
§§#else
§§//32 bit Windows
§§#define DRCONFIGLIB_PATH "dyndist32\\lib32\\drconfiglib.dll"
§§#define DRRUN_PATH "dyndist32\\bin32\\drrun.exe"
§§#define DRCOVMULTI_PATH "dyndist32\\clients\\lib32\\release\\drcovMulti.dll"
§§#define DYNRIO_DIR "\\dyndist32"
§§#define DYNRIO_LIBDIR "lib32"
§§#endif
§§
§§#else
§§#include <sys/syscall.h>
§§
§§#if (__WORDSIZE == 64 )
§§//64 bit Linux
§§#define DRCONFIG_PATH "dynamorio/bin64/drconfig"
§§#define DRRUN_PATH "dynamorio/bin64/drrun"
§§#define DRCOVMULTI_PATH "dynamorio/clients/lib64/release/libdrcovMulti.so"
§§#define DRPRELOAD_PATH "dynamorio/lib64/release/libdrpreload.so"
§§#define DYNRIO_DIR "/dynamorio"
§§#define DYNRIO_LIBDIR "lib64"
§§#else
§§//32 bit Linux
§§#define DRCONFIG_PATH "dynamorio/bin32/drconfig"
§§#define DRRUN_PATH "dynamorio/bin32/drrun"
§§#define DRCOVMULTI_PATH "dynamorio/clients/lib32/release/libdrcovMulti.so"
§§#define DRPRELOAD_PATH "dynamorio/lib32/release/libdrpreload.so"
§§#define DYNRIO_DIR "/dynamorio"
§§#define DYNRIO_LIBDIR "lib32"
§§#endif
§§#endif
§§
§§#if defined(_WIN64) || defined(_WIN32)
§§//Typedefs for dynamorio functions defined in dr_config.h
§§typedef  dr_config_status_t  __cdecl dr_nudge_pid_func(process_id_t process_id, client_id_t client_id, uint64 arg, uint timeout_ms);
§§typedef dr_config_status_t __cdecl dr_register_process_func(const char *process_name, process_id_t pid, bool global, const char *dr_root_dir, dr_operation_mode_t dr_mode, bool debug, dr_platform_t dr_platform, const char *dr_options);
§§typedef dr_config_status_t __cdecl dr_unregister_process_func(const char *process_name, process_id_t pid, bool global, dr_platform_t dr_platform);
§§typedef dr_config_status_t __cdecl dr_register_client_func(const char *process_name, process_id_t pid, bool global, dr_platform_t dr_platform, client_id_t client_id, size_t client_pri, const char *client_path, const char *client_options);
§§typedef bool __cdecl  dr_process_is_registered_func(const char *process_name, process_id_t pid, bool global, dr_platform_t dr_platform, char *dr_root_dir /* OUT */, dr_operation_mode_t *dr_mode /* OUT */, bool *debug /* OUT */, char *dr_options /* OUT */);
§§typedef dr_config_status_t __cdecl dr_register_syswide_func(dr_platform_t dr_platform, const char *dr_root_dir);
§§typedef  bool __cdecl dr_syswide_is_on_func(dr_platform_t dr_platform, const char *dr_root_dir);
§§#endif
§§
§§// Defines needed to communicate with dynamorio multicov
§§enum {
§§	NUDGE_TERMINATE_PROCESS = 1,
§§	NUDGE_DUMP_CURRENT_COVERAGE = 2,
§§	NUDGE_RESET_COVERAGE = 3,
§§	NUDGE_NOOP = 4,
§§};
§§
§§class CommInt;
§§class ExternalProcess;
§§class TestExecutorDynRioMulti :
§§	public TestExecutorDynRio
§§{
§§#ifndef _VSTEST
§§private:
§§#else
§§public:
§§#endif //_VSTEST
§§
§§	bool m_target_and_feeder_okay;
§§	SharedMemIPC* m_sharedMemIPC_toFeeder;
§§	std::shared_ptr<ExternalProcess> m_debuggeeProcess;
§§	process_id_t m_targetProcessID;
§§	client_id_t  m_dynrio_clientID;
§§	std::shared_ptr<ExternalProcess> m_feederProcess;
§§	std::shared_ptr<DebugExecutionOutput> m_exOutput_FROM_TARGET_DEBUGGING;
§§	int m_initializationTimeoutMS;
§§	int m_iterationNumMod2;
§§	std::string m_feederCmdline;
§§	std::string m_starterCmdline;
§§	bool m_attachInsteadOfStart;
§§	CommInt* m_commInt;
§§	std::string m_dynrioPipeName;
§§	int m_forceRestartAfterXTCs;
§§	int m_executionsSinceLastRestart;
§§
§§#if defined(_WIN64) || defined(_WIN32)
§§	HMODULE m_hdrconfiglib;
§§	dr_nudge_pid_func* m_dr_nudge_pid;
§§	dr_register_process_func* m_dr_register_process;
§§	dr_unregister_process_func* m_dr_unregister_process;
§§	dr_register_client_func* m_dr_register_client;
§§	dr_process_is_registered_func* m_dr_process_is_registered;
§§	dr_register_syswide_func* m_dr_register_syswide;
§§	dr_syswide_is_on_func* m_dr_syswide_is_on;
§§
§§	HANDLE m_pipe_to_dynrio;
§§	HANDLE m_SharedMemIPCInterruptEvent;
§§
§§	static void debuggerThreadMain(std::shared_ptr<DebugExecutionOutput> exOutput_FROM_TARGET_DEBUGGING, std::shared_ptr<ExternalProcess> debuggeeProcess, HANDLE sharedMemIPCInterruptEvent, bool attachInsteadOfStart, bool treatAnyAccessViolationAsFatal);
§§
§§#else
§§	int m_pipe_to_dynrio;
§§	int m_SharedMemIPCInterruptFD[2];
§§	bool m_target_forks;
§§
§§	static void debuggerThreadMain(std::shared_ptr<DebugExecutionOutput> exOutput_FROM_TARGET_DEBUGGING, std::shared_ptr<ExternalProcess> debuggeeProcess, int sharedMemIPCInterruptWriteFD, bool attachInsteadOfStart, bool treatAnyAccessViolationAsFatal);
§§	dr_config_status_t m_dr_nudge_pid(process_id_t process_id, client_id_t client_id, uint64_t arg, uint timeout_ms);
§§#endif
§§
§§	bool setDynamoRioRegistration(bool active, const std::string dynrioPipeName, std::string* errormsg);
§§	bool runSingleTestcase(const FluffiTestcaseID testcaseId, std::shared_ptr<DebugExecutionOutput> exResult, bool use_dyn_rio);
§§	bool attemptStartTargetAndFeeder(bool use_dyn_rio);
§§	bool waitForDynRioInitialization(int timeoutMS);
§§	void waitForDebuggerToTerminate();
§§	std::string getDrCovOutputFile(int iterationNumMod2);
§§
§§public:
§§	TestExecutorDynRioMulti(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const std::string additionalEnvParam, GarbageCollectorWorker* garbageCollectorWorker, bool treatAnyAccessViolationAsFatal, const std::string  feederCmdline, const std::string  starterCmdline, int initializationTimeoutMS, CommInt* commInt, bool target_forks, int forceRestartAfterXTCs);
§§	virtual ~TestExecutorDynRioMulti();
§§
§§	std::shared_ptr<DebugExecutionOutput> execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage);
§§	bool isSetupFunctionable();
§§};
