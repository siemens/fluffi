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

Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once
#include "FluffiTestExecutor.h"
#include "GDBThreadCommunication.h"

class SharedMemIPC;
class Module;
class FluffiBasicBlock;
class GDBBreakpoint;
class TestExecutorGDB :
	public FluffiTestExecutor
{
public:
	TestExecutorGDB(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, GarbageCollectorWorker* garbageCollectorWorker, const std::string  feederCmdline, const std::string  starterCmdline, int initializationTimeoutMS, int forceRestartAfterXTCs, std::set<FluffiBasicBlock> blocksToCover, uint32_t bpInstr, int bpInstrBytes);
	virtual ~TestExecutorGDB();

	std::shared_ptr<DebugExecutionOutput> execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage);

	bool isSetupFunctionable();

#ifndef _VSTEST
private:
#else
#endif //_VSTEST
	typedef std::tuple<std::string, uint64_t, uint64_t> tModuleAddressesAndSizes;
	typedef std::tuple<uint64_t, uint64_t, std::string, std::string, std::string> tModuleInformation;

	std::string m_feederCmdline;
	std::string m_starterCmdline;
	int m_initializationTimeoutMS;
	int m_forceRestartAfterXTCs;
	int m_executionsSinceLastRestart;
	SharedMemIPC* m_sharedMemIPC_toFeeder;
	std::shared_ptr<ExternalProcess> m_feederProcess;
	std::shared_ptr<GDBThreadCommunication> m_gDBThreadCommunication;
	std::set<FluffiBasicBlock> m_blocksToCover;
	bool m_target_and_feeder_okay;
	uint32_t m_bpInstr;
	int m_bpInstrBytes;

	enum CoverageMode { FULL, PARTIAL, NONE };

#if defined(_WIN64) || defined(_WIN32)
	HANDLE m_SharedMemIPCInterruptEvent;
#else
	int m_SharedMemIPCInterruptFD[2];
#endif

	bool attemptStartTargetAndFeeder();
	bool runSingleTestcase(const FluffiTestcaseID testcaseId, std::shared_ptr<DebugExecutionOutput> exResult, CoverageMode covMode);

#if defined(_WIN32) || defined(_WIN64)
	static void debuggerThreadMain(const std::string targetCMDline, std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const  std::string  gdbInitFile, HANDLE sharedMemIPCInterruptEvent, const std::set<Module> modulesToCover, const std::set<FluffiBasicBlock> blocksToCover, uint32_t bpInstr, int bpInstrBytes);
#else
	static void debuggerThreadMain(const std::string targetCMDline, std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const  std::string  gdbInitFile, int sharedMemIPCInterruptWriteFD, const std::set<Module> modulesToCover, const std::set<FluffiBasicBlock> blocksToCover, uint32_t bpInstr, int bpInstrBytes);
#endif

	void waitForDebuggerToTerminate();

	static bool consumeGDBOutputQueue(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, std::set<FluffiBasicBlock>* blocksCoveredSinceLastReset, std::map<uint64_t, GDBBreakpoint>* allBreakpoints, int bpInstrBytes);
	static void gdbDebug(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, const std::set<Module> modulesToCover, const std::set<FluffiBasicBlock> blocksToCover, uint32_t bpInstr, int bpInstrBytes);
	static void gdbLinereaderThread(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, HANDLETYPE inputHandleFromGdb, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode);
	static bool gdbPrepareForDebug(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, const std::set<Module> modulesToCover, const std::set<FluffiBasicBlock> blocksToCover, std::map<uint64_t, GDBBreakpoint>* allBreakpoints, int bpInstrBytes);
	static std::string generateEnableAllCommand(std::map<uint64_t, GDBBreakpoint>&  allBreakpoints, uint32_t bpInstr, int bpInstrBytes);
	static bool getBaseAddressesAndSizes(std::vector<tModuleAddressesAndSizes>& baseAddressesAndSizes, std::string parseInfoOutput);
	static bool getBaseAddressesForModules(std::map<int, uint64_t>* modmap, const std::set<Module> modulesToCover, std::string parseInfoOutput);
	static std::string getCrashRVA(std::vector<tModuleAddressesAndSizes>& baseAddressesAndSizes, uint64_t signalAddress);
	static bool handleSignal(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, std::string signalmessage, std::map<uint64_t, GDBBreakpoint>* allBreakpoints, std::set<FluffiBasicBlock>* blocksCoveredSinceLastReset, int bpInstrBytes);
	static bool parseInfoFiles(std::vector<tModuleInformation>& loadedFiles, std::string parseInfoOutput);
	static bool sendCommandToGDBAndWaitForResponse(const std::string command, std::string* response, std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, bool sendCtrlZ = false);
	static bool waitUntilTargetIsBeingDebugged(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, int timeoutMS);
	static bool waitUntilCoverageState(std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication, GDBThreadCommunication::COVERAGE_STATE desiredState, int timeoutMS);
};
