/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once
#include "FluffiEvaluator.h"

class TestOutcomeDescriptor;
class FluffiTestResult;
class BlockCoverageCache;
class CoverageEvaluator :
	public FluffiEvaluator
{
public:
	CoverageEvaluator(std::string testcaseDir, BlockCoverageCache* localBlockCoverageCache, GarbageCollectorWorker* garbageCollectorWorker, CommInt* commInt, TEWorkerThreadState* workerThreadState);
	virtual ~CoverageEvaluator();

	bool isSetupFunctionable();
	void processTestOutcomeDescriptor(TestOutcomeDescriptor* tod);

private:
	int generateParentRatingDelta(ExitType exitType, bool hasNewBlocks, bool isSpecial);
	FluffiTestResult copyTestResultButStripDuplicateBlocks(const FluffiTestResult originalTestResult);
	void updateTestcaseEvaluationSendTimeout(bool wasLastTransferSuccessful, TestEvaluation eval, ExitType exitType);

	int m_deltaNewBlocks = 100;
	int m_deltaHang = -10;
	int m_deltaAccessViolation = -10;
	int m_deltaOtherException = -10;

	int m_currentTestcaseEvaluationSendTimeout;
};