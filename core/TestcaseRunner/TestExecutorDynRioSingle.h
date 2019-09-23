/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once
#include "TestExecutorDynRio.h"

class TestExecutorDynRioSingle :
	public TestExecutorDynRio
{
public:
	TestExecutorDynRioSingle(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const std::string additionalEnvParam, GarbageCollectorWorker* garbageCollectorWorker, bool treatAnyAccessViolationAsFatal);
	virtual ~TestExecutorDynRioSingle();

	std::shared_ptr<DebugExecutionOutput> execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage);
	bool isSetupFunctionable();

#ifndef _VSTEST
private:
#endif //_VSTEST

	static void DebugCommandLine(std::string commandline, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, int timeoutMS, std::shared_ptr<DebugExecutionOutput> exOutput, bool doPostMortemAnalysis, bool treatAnyAccessViolationAsFatal, const std::string additionalEnvParam);

	std::string getDrCovOutputFile();
};
