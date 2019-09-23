/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once
#include "IFLUFFIMessageHandler.h"

#if defined(_WIN32) || defined(_WIN64)
#include "pdh.h"
#else
#endif

class CommInt;
class GetStatusRequestHandler :
	public IFLUFFIMessageHandler
{
public:
	GetStatusRequestHandler(CommInt* comm);
	virtual ~GetStatusRequestHandler();

	void handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp);

	bool isManagerActive(unsigned long maxAllowedTimeOfInactivityMS);
	bool wasManagerReplaced();

#ifndef _VSTEST
protected:
#else
public:
#endif //_VSTEST
	CommInt* m_comm;
	std::chrono::time_point<std::chrono::steady_clock> m_timeOfLastManagerRequest;
	bool m_ManagerWasReplaced;

#if defined(_WIN32) || defined(_WIN64)
	PDH_HQUERY m_cpuQuery;
	PDH_HCOUNTER m_cpuTotal;

	ULARGE_INTEGER m_lastCPU, m_lastSysCPU, m_lastUserCPU;
	int m_numProcessors;
	HANDLE m_self;
#else
	unsigned long long m_lastTotalUser, m_lastTotalUserLow, m_lastTotalSys, m_lastTotalIdle;
	clock_t m_lastCPU, m_lastSysCPU, m_lastUserCPU;
	int m_numProcessors;

#endif

	virtual std::string generateStatus() = 0;
	std::string generateGeneralStatus();
	double getCurrentSystemCPUUtilization(); //needs to be called multiple times. Every call gives an average since the last call
	double getCurrentProcessCPUUtilization(); //needs to be called multiple times. Every call gives an average since the last call

	//Code taken from https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
	void initSystemCPUUtilization();
	void initProcessCPUUtilization();
};
