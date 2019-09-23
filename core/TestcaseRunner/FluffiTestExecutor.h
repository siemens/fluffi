/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

§§Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once
#include "Module.h"
#include "ExternalProcess.h"

#if defined(_WIN64) || defined(_WIN32)
//Define process id in conformance with dynamorio
typedef UINT_PTR process_id_t;
#else
typedef pid_t process_id_t;
#endif

class FluffiTestcaseID;
class DebugExecutionOutput;
class GarbageCollectorWorker;
class SharedMemIPC;
class FluffiTestExecutor
{
public:
	FluffiTestExecutor(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const std::string additionalEnvParam, GarbageCollectorWorker* garbageCollectorWorker);
	virtual ~FluffiTestExecutor();

	virtual std::shared_ptr<DebugExecutionOutput> execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage) = 0;

	virtual bool isSetupFunctionable() = 0;

#ifndef _VSTEST
protected:
#else
public:
#endif //_VSTEST

	std::string m_targetCMDline;
	int m_hangTimeoutMS;
	std::set<Module> m_modulesToCover;
	std::string m_testcaseDir;
	ExternalProcess::CHILD_OUTPUT_TYPE m_child_output_mode;
	std::string m_additionalEnvParam;
§§	GarbageCollectorWorker* m_garbageCollectorWorker;

§§	static bool initializeFeeder(std::shared_ptr<ExternalProcess> feederProcess, SharedMemIPC* sharedMemIPC_toFeeder, process_id_t targetProcessID, int timeoutMS);
};
