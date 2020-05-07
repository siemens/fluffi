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

Author(s): Thomas Riedmaier, Roman Bendt, Abian Blome
*/

#pragma once
#include "DebugExecutionOutput.h"

class GDBThreadCommunication
{
public:
	typedef enum { SHOULD_RESET_HARD, SHOULD_RESET_SOFT, RESETTED, SHOULD_DUMP, DUMPED } COVERAGE_STATE;

#if defined(_WIN32) || defined(_WIN64)
#else
	int m_gdbPid;
#endif

	DebugExecutionOutput m_exOutput;
	std::ostream* m_ostreamToGdb;

	GDBThreadCommunication();
	virtual ~GDBThreadCommunication();

	bool get_gdbThreadShouldTerminate();
	void set_gdbThreadShouldTerminate(); //triggers waitForRelevantStateChange
	bool get_debuggingReady();
	void set_debuggingReady(); //triggers waitForRelevantStateChange
	COVERAGE_STATE get_coverageState();
	void set_coverageState(COVERAGE_STATE val); //triggers waitForRelevantStateChange
	size_t get_gdbOutputQueue_size();

	void gdbOutputQueue_push_back(const std::string val); //triggers waitForRelevantStateChange
	std::string gdbOutputQueue_pop_front();

	int get_exitStatus();
	void set_exitStatus(int exitStatus);

	std::cv_status waitForTerminateOrNewMessageOrCovStateChangeWithTimeout(int timeoutMS, GDBThreadCommunication::COVERAGE_STATE statesToWaitFor[], int numOfStatesToWaitFor);
	void waitForTerminateOrNewMessageOrCovStateChange(GDBThreadCommunication::COVERAGE_STATE statesToWaitFor[], int numOfStatesToWaitFor);

	std::cv_status waitForDebuggingReadyWithTimeout(int timeoutMS);
	void waitForDebuggingReady();

private:

	std::mutex m_gdb_mutex, m_readyDebug_mutex;
	std::condition_variable m_gdb_mutex_cv, m_readyDebug_cv;

	bool m_gdbThreadShouldTerminate;
	bool m_debuggingReady;
	COVERAGE_STATE m_coverageState;
	std::deque<std::string> m_gdbOutputQueue;
	int m_exitStatus;
};
