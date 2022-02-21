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

#include "stdafx.h"
#include "FluffiTestResult.h"

FluffiTestResult::FluffiTestResult(const ExitType exitType, const std::vector<FluffiBasicBlock> blocks, const std::string crashFootprint, bool hasFullCoverage, const std::string edgeCoverageHash) :
	m_exitType(exitType), m_blocks(blocks), m_crashFootprint(crashFootprint), m_hasFullCoverage(hasFullCoverage), m_edgeCoverageHash(edgeCoverageHash)
{
}

FluffiTestResult::FluffiTestResult(const TestResult& testResult) :
	m_exitType(testResult.exittype()), m_crashFootprint(testResult.crashfootprint()), m_hasFullCoverage(testResult.hasfullcoverage()), m_edgeCoverageHash(testResult.edgecoveragehash())
{
	google::protobuf::RepeatedPtrField< ::BasicBlock > b = testResult.blocks();
	for (int i = 0; i < b.size(); i++) {
		m_blocks.push_back(FluffiBasicBlock(b.Get(i)));
	}
}

FluffiTestResult::~FluffiTestResult()
{
}

TestResult FluffiTestResult::getProtobuf() const
{
	TestResult tr{};
	tr.set_exittype(m_exitType);
	tr.set_crashfootprint(m_crashFootprint);
	tr.set_hasfullcoverage(m_hasFullCoverage);
	tr.set_edgecoveragehash(m_edgeCoverageHash);

	for (auto const& block : m_blocks) {
		BasicBlock* newblock = tr.add_blocks();
		newblock->CopyFrom(block.getProtobuf());
	}

	return tr;
}

void FluffiTestResult::setProtobuf(TestResult* tr) const
{
	tr->set_exittype(m_exitType);
	tr->set_crashfootprint(m_crashFootprint);
	tr->set_hasfullcoverage(m_hasFullCoverage);
	tr->set_edgecoveragehash(m_edgeCoverageHash);

	tr->clear_blocks();
	for (auto const& block : m_blocks) {
		BasicBlock* newblock = tr->add_blocks();
		newblock->CopyFrom(block.getProtobuf());
	}
}
