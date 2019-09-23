/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Roman Bendt
*/

#include "stdafx.h"
#include "FluffiMutator.h"
#include "FluffiTestcaseID.h"
#include "ExternalProcess.h"

uint64_t FluffiMutator::s_numMutations = 0;

FluffiMutator::FluffiMutator(FluffiServiceDescriptor selfServiceDescriptor, std::string testcaseDirectory)
	: m_selfServiceDescriptor(selfServiceDescriptor),
	m_testcaseDir(testcaseDirectory)
{
}

FluffiMutator::~FluffiMutator()
{
}

FluffiTestcaseID FluffiMutator::genNewLocalFluffiTestcaseID()
{
	FluffiTestcaseID testcaseID{ m_selfServiceDescriptor, s_numMutations };
	++s_numMutations;

	return testcaseID;
}

bool FluffiMutator::executeProcessAndWaitForCompletion(std::string cmdline, unsigned long timeoutMilliseconds) {
	ExternalProcess extProc(cmdline, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
	if (!extProc.initProcess()) {
		return false;
	}

	return extProc.runAndWaitForCompletion(timeoutMilliseconds);
}
