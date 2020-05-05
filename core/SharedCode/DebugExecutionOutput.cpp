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

Author(s): Thomas Riedmaier
*/

#include "stdafx.h"
#include "DebugExecutionOutput.h"

DebugExecutionOutput::DebugExecutionOutput() :
	m_firstCrash(""),
	m_lastCrash(""),
	m_terminationType(PROCESS_TERMINATION_TYPE::ERR),
	m_terminationDescription("NOT SET"),
	m_PID(0),
	m_debuggerThreadDone(true), //defaults to true to allow "waitfordebugger" to return if the debugger was never started. It will be set to false as first action within the debuggerthread
	m_hasFullCoverage(true) //defaults to true, as most techniques capture full coverage by default
{
}

DebugExecutionOutput::DebugExecutionOutput(const DebugExecutionOutput &obj) :
	m_firstCrash(obj.m_firstCrash),
	m_lastCrash(obj.m_lastCrash),
	m_terminationType(obj.m_terminationType),
	m_terminationDescription(obj.m_terminationDescription),
	m_PID(obj.m_PID),
	m_debuggerThreadDone(obj.m_debuggerThreadDone),
	m_hasFullCoverage(obj.m_hasFullCoverage)
{
	m_nonUniqueuUsortedListOfCoveredBasicBlocks = obj.m_nonUniqueuUsortedListOfCoveredBasicBlocks;
}

DebugExecutionOutput::~DebugExecutionOutput()
{
}

void DebugExecutionOutput::addCoveredBasicBlock(FluffiBasicBlock bb) {
	m_nonUniqueuUsortedListOfCoveredBasicBlocks.push_back(bb);
}

std::vector<FluffiBasicBlock> DebugExecutionOutput::getCoveredBasicBlocks() {
	return m_nonUniqueuUsortedListOfCoveredBasicBlocks;
}

void DebugExecutionOutput::swapBlockVectorWith(DebugExecutionOutput & other) {
	m_nonUniqueuUsortedListOfCoveredBasicBlocks.swap(other.m_nonUniqueuUsortedListOfCoveredBasicBlocks);
}
