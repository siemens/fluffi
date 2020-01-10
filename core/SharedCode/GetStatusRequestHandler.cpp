/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "GetStatusRequestHandler.h"
#include "CommInt.h"
#include "ExternalProcess.h"
#include "FLUFFILogHandler.h"

GetStatusRequestHandler::GetStatusRequestHandler(CommInt* comm) :
	m_comm(comm),
	m_timeOfLastManagerRequest(std::chrono::steady_clock::now()),
	m_ManagerWasReplaced(false)
{
	initSystemCPUUtilization();
	initProcessCPUUtilization();
}

GetStatusRequestHandler::~GetStatusRequestHandler()
{
}

bool GetStatusRequestHandler::isManagerActive(unsigned long maxAllowedTimeOfInactivityMS) {
	return maxAllowedTimeOfInactivityMS > static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_timeOfLastManagerRequest).count());
}

void GetStatusRequestHandler::resetManagerActiveDetector() {
	m_timeOfLastManagerRequest = std::chrono::steady_clock::now();
}

bool GetStatusRequestHandler::wasManagerReplaced() {
	return m_ManagerWasReplaced;
}

void GetStatusRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp) {
	(void)(workerThreadState); //avoid unused parameter warning

	FluffiServiceDescriptor requesterServiceDescriptor = FluffiServiceDescriptor(req->getstatusrequest().requesterservicedescriptor());
	if (!(m_comm->getMyLMServiceDescriptor() == FluffiServiceDescriptor("", "")) && !(m_comm->getMyLMServiceDescriptor() == requesterServiceDescriptor)) {
		LOG(ERROR) << "It looks as if my LocalManager was replaced with some other LocalManager. I was not built for this!";
		m_ManagerWasReplaced = true;
	}

	m_timeOfLastManagerRequest = std::chrono::steady_clock::now();

	GetStatusResponse* statusResponse = new GetStatusResponse();
	setLogMessages(statusResponse);
	statusResponse->set_status(generateStatus());
	resp->set_allocated_getstatusresponse(statusResponse);
}

void GetStatusRequestHandler::setLogMessages(GetStatusResponse* statusResponse) {
	FLUFFILogHandler* fluffiLogger = el::Helpers::logDispatchCallback<FLUFFILogHandler>("FLUFFILog");
	std::deque<std::string> messages = fluffiLogger->getAllMessages();

	for (auto it = messages.begin(); it != messages.end(); ++it) {
		statusResponse->add_logmessages(*it);
	}
}

std::string GetStatusRequestHandler::generateGeneralStatus() {
	std::stringstream ss;
	ss << "SysCPUUtil " << getCurrentSystemCPUUtilization() << " | ProcCPUUtil " << getCurrentProcessCPUUtilization() << " | AverageRTT " << m_comm->getAverageRoundTripTime() << " |";
	return ss.str();
}

#if defined(_WIN32) || defined(_WIN64)

double GetStatusRequestHandler::getCurrentSystemCPUUtilization() {
	PDH_FMT_COUNTERVALUE counterVal;

	PdhCollectQueryData(m_cpuQuery);
	PdhGetFormattedCounterValue(m_cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
	return counterVal.doubleValue;
}
double GetStatusRequestHandler::getCurrentProcessCPUUtilization() {
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	double percent;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	if (GetProcessTimes(m_self, &ftime, &ftime, &fsys, &fuser) == 0) {
		LOG(ERROR) << "GetProcessTimes failed";
		return 0.0;
	}

	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));

	double passedTime = static_cast<double>((now.QuadPart - m_lastCPU.QuadPart));
	percent = static_cast<double>(((sys.QuadPart - m_lastSysCPU.QuadPart) + (user.QuadPart - m_lastUserCPU.QuadPart) + ExternalProcess::getAllTimeSpentSinceLastCall()) / passedTime);
	percent /= static_cast<double>(m_numProcessors);

	m_lastCPU = now;
	m_lastUserCPU = user;
	m_lastSysCPU = sys;

	if (passedTime == 0.0) {
		//By definition: If no time passed we did not consume any resources (avoid nan return, which might cause problems)
		return 0.0;
	}
	else {
		return percent * static_cast<double>(100);
	}
}

void GetStatusRequestHandler::initSystemCPUUtilization() {
	PdhOpenQuery(NULL, NULL, &m_cpuQuery);
	// You can also use L"\\Processor(*)\\% Processor Time" and get individual CPU values with PdhGetFormattedCounterArray()
	PdhAddEnglishCounter(m_cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &m_cpuTotal);
	PdhCollectQueryData(m_cpuQuery);
}
void GetStatusRequestHandler::initProcessCPUUtilization() {
	SYSTEM_INFO sysInfo;
	FILETIME ftime, fsys, fuser;

	GetSystemInfo(&sysInfo);
	m_numProcessors = sysInfo.dwNumberOfProcessors;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&m_lastCPU, &ftime, sizeof(FILETIME));

	m_self = GetCurrentProcess();
	if (GetProcessTimes(m_self, &ftime, &ftime, &fsys, &fuser) == 0) {
		LOG(ERROR) << "GetProcessTimes failed - could not initialize!";
		return;
	}

	memcpy(&m_lastSysCPU, &fsys, sizeof(FILETIME));
	memcpy(&m_lastUserCPU, &fuser, sizeof(FILETIME));
}

#else

//implement this e.g. based on https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
double GetStatusRequestHandler::getCurrentSystemCPUUtilization() {
	double percent;
	FILE* file;
	unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

	file = fopen("/proc/stat", "r");
	int elementsParsed = fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow, &totalSys, &totalIdle);
	fclose(file);
	if (elementsParsed != 4) {
		LOG(ERROR) << "Could not parse /proc/stat";
		return -1.0;
	}

	if (totalUser < m_lastTotalUser || totalUserLow < m_lastTotalUserLow ||
		totalSys < m_lastTotalSys || totalIdle < m_lastTotalIdle) {
		//Overflow detection. Just skip this value.
		percent = -1.0;
	}
	else {
		total = (totalUser - m_lastTotalUser) + (totalUserLow - m_lastTotalUserLow) + (totalSys - m_lastTotalSys);
		percent = static_cast<double>(total);
		total += (totalIdle - m_lastTotalIdle);
		percent /= static_cast<double>(total);
		percent *= 100;
	}

	m_lastTotalUser = totalUser;
	m_lastTotalUserLow = totalUserLow;
	m_lastTotalSys = totalSys;
	m_lastTotalIdle = totalIdle;

	return percent;
}

double GetStatusRequestHandler::getCurrentProcessCPUUtilization() {
	struct tms timeSample;
	clock_t now;
	double percent;

	now = times(&timeSample);
	if (now <= m_lastCPU || timeSample.tms_stime < m_lastSysCPU ||
		timeSample.tms_utime < m_lastUserCPU) {
		//Overflow detection. Just skip this value.
		percent = -1.0;
	}
	else {
		percent = static_cast<double>((timeSample.tms_stime - m_lastSysCPU) + (timeSample.tms_utime - m_lastUserCPU) + ExternalProcess::getAllTimeSpentSinceLastCall());
		percent /= static_cast<double>(now - m_lastCPU);
		percent /= static_cast<double>(m_numProcessors);
		percent *= 100;
	}
	m_lastCPU = now;
	m_lastSysCPU = timeSample.tms_stime;
	m_lastUserCPU = timeSample.tms_utime;

	return percent;
}

void GetStatusRequestHandler::initSystemCPUUtilization() {
	FILE* file = fopen("/proc/stat", "r");
	int elementsParsed = fscanf(file, "cpu %llu %llu %llu %llu", &m_lastTotalUser, &m_lastTotalUserLow, &m_lastTotalSys, &m_lastTotalIdle);
	fclose(file);
	if (elementsParsed != 4) {
		LOG(ERROR) << "Could not parse /proc/stat";
	}
}

void GetStatusRequestHandler::initProcessCPUUtilization() {
	FILE* file;
	struct tms timeSample;
	char line[128];

	m_lastCPU = times(&timeSample);
	m_lastSysCPU = timeSample.tms_stime;
	m_lastUserCPU = timeSample.tms_utime;

	file = fopen("/proc/cpuinfo", "r");
	m_numProcessors = 0;
	while (fgets(line, 128, file) != NULL) {
		if (strncmp(line, "processor", 9) == 0) m_numProcessors++;
	}
	fclose(file);
}

#endif
