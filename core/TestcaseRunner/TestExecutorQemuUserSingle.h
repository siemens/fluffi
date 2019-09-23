/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Roman Bendt, Thomas Riedmaier, Abian Blome
*/

§§#pragma once
§§#include "FluffiTestExecutor.h"
§§#if defined(_WIN32) || defined(_WIN64)
§§#else
§§#include <sys/ptrace.h>
§§#include <sys/mount.h>
§§#include <sys/wait.h>
§§#include <fcntl.h>
§§#endif
§§
§§class TestExecutorQemuUserSingle :
§§	public FluffiTestExecutor
§§{
§§public:
	TestExecutorQemuUserSingle(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, GarbageCollectorWorker* garbageCollectorWorker, bool treatAnyAccessViolationAsFatal, const std::string rootfs);
§§	virtual ~TestExecutorQemuUserSingle();
	std::shared_ptr<DebugExecutionOutput> execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage);
§§	bool isSetupFunctionable();
§§
§§private:
§§	bool doInit();

	bool m_treatAnyAccessViolationAsFatal;
§§	std::experimental::filesystem::path m_rootfs;
§§	bool m_was_initialized;
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§#else
	static void DebugCommandLineInChrootEnv(std::string commandline, std::string rootfs, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, int timeoutMS, std::shared_ptr<DebugExecutionOutput> exResult, bool treatAnyAccessViolationAsFatal);
§§#endif
§§};
