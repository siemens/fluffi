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
#include "FluffiBasicBlock.h"

class FluffiTestResult
{
public:
	FluffiTestResult(const ExitType exitType, const std::vector<FluffiBasicBlock> blocks, const std::string crashFootprint, bool hasFullCoverage, const std::string edgeCoverageHash);
	FluffiTestResult(const TestResult& testResult);
	virtual ~FluffiTestResult();

	TestResult getProtobuf() const;
	void setProtobuf(TestResult* tr) const;

	ExitType m_exitType;
	std::vector<FluffiBasicBlock> m_blocks;
	std::string m_crashFootprint;
	bool m_hasFullCoverage;
	std::string m_edgeCoverageHash;
};
