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

#if defined(_WIN64) || defined(_WIN32)

#if defined(_WIN64)
//64 bit Windows
#include <build64/configure.h>
#include <core/lib/globals_shared.h>
#include <build64/include/dr_config.h>

#else
//32 bit Windows
#include <build86/configure.h>
#include <core/lib/globals_shared.h>
#include <build86/include/dr_config.h>

#endif

#else

#include _DR_BUILD_CONFIGURE_
#include <core/lib/globals_shared.h>
#include _DR_BUILD_CONFIG_

#endif

//Import definition of bb_entry
#include <ext/drcovlib/drcovlib_min.h>

#if defined(_WIN32) || defined(_WIN64)
#define strcasecmp _stricmp
#else
#endif

class TestExecutorDynRio :
	public FluffiTestExecutor
{
public:
	TestExecutorDynRio(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const std::string additionalEnvParam, GarbageCollectorWorker* garbageCollectorWorker, bool treatAnyAccessViolationAsFatal);
	virtual ~TestExecutorDynRio();

#ifndef _VSTEST
protected:
#endif //_VSTEST

	bool m_treatAnyAccessViolationAsFatal;

	static void copyCoveredModulesToDebugExecutionOutput(const std::vector<char>* input, const std::set<Module>* modulesToCover, std::shared_ptr<DebugExecutionOutput> exOutput, const std::string edgeCoverageModule);
};
