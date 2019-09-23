/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

§§Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "CoverageEvaluator.h"
#include "BlockCoverageCache.h"
#include "TestOutcomeDescriptor.h"
#include "Util.h"
#include "GarbageCollectorWorker.h"
#include "CommInt.h"
#include "TEWorkerThreadState.h"

§§CoverageEvaluator::CoverageEvaluator(std::string testcaseDir, BlockCoverageCache* localBlockCoverageCache, GarbageCollectorWorker* garbageCollectorWorker, CommInt* commInt, TEWorkerThreadState* workerThreadState) :
	FluffiEvaluator(testcaseDir, localBlockCoverageCache, garbageCollectorWorker, commInt, workerThreadState),
	m_currentTestcaseEvaluationSendTimeout(CommInt::timeoutFileTransferMessage)
{
}

CoverageEvaluator::~CoverageEvaluator()
{
}

bool CoverageEvaluator::isSetupFunctionable() {
	bool setupFunctionable = true;

	//No external dependencies

	return setupFunctionable;
}

§§void CoverageEvaluator::processTestOutcomeDescriptor(TestOutcomeDescriptor* tod) {
	FLUFFIMessage req;
	FLUFFIMessage resp;

	FluffiTestcaseID testcaseId = tod->getId();
	FluffiTestcaseID parentId = tod->getparentId();

§§	TestcaseID* mutableTestcaseId = new TestcaseID();
	mutableTestcaseId->CopyFrom(testcaseId.getProtobuf());

§§	TestcaseID* mutableParentTestcaseId = new TestcaseID();
	mutableParentTestcaseId->CopyFrom(parentId.getProtobuf());

§§	PutTestEvaluationRequest* putTestEvaluationRequest = new PutTestEvaluationRequest();
	putTestEvaluationRequest->set_allocated_id(mutableTestcaseId);
	putTestEvaluationRequest->set_allocated_parentid(mutableParentTestcaseId);

	bool hasNewBlocks = false;
	for (FluffiBasicBlock const& fbb : tod->getTestResult().m_blocks) {
		bool thisOneIsNew = !m_localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb);
		hasNewBlocks |= thisOneIsNew;
		if (thisOneIsNew) {
			LOG(INFO) << "Testcase " << FluffiTestcaseID(*mutableParentTestcaseId) << " got us new coverage: " << fbb;
		}
	}

	TestEvaluation eval;
	//Every testcaes that does not yet have coverage is marked "special"
	if (tod->getparentId().m_serviceDescriptor.m_guid != "special" && tod->getTestResult().m_exitType == ExitType::CleanExit && !hasNewBlocks) {
		eval = TestEvaluation::Discard;

		//If there are no new blocks and the process terminated nicely: do not set the optional parts of this message to save network bandwich  and processing time
		//Exception: if it is a "special" testcase: always report coverage to lm!

		// Instead: Delete file in filesystem (as it will not be transferred)
		std::string filename = Util::generateTestcasePathAndFilename(*mutableTestcaseId, m_testcaseDir);
		m_garbageCollectorWorker->markFileForDelete(filename);
	}
	else {
		eval = TestEvaluation::Keep;

		putTestEvaluationRequest->set_parentratingdelta(generateParentRatingDelta(tod->getTestResult().m_exitType, hasNewBlocks, tod->getparentId().m_serviceDescriptor.m_guid == "special"));

§§		TestResult* mutableTestResult = new TestResult();
		if (tod->getTestResult().m_hasFullCoverage) {
			//Report full coverage to LM
			FluffiTestResult  testResult = copyTestResultButStripDuplicateBlocks(tod->getTestResult());
			testResult.setProtobuf(mutableTestResult);
		}
		else {
			//We only got partial coverage. Report this as "empty" coverage. This will trigger a re-run of this Testcase with "forceFullCoverage"
			FluffiTestResult  testResult = FluffiTestResult(tod->getTestResult().m_exitType, std::vector<FluffiBasicBlock>(), tod->getTestResult().m_crashFootprint, false);
			testResult.setProtobuf(mutableTestResult);
		}

		putTestEvaluationRequest->set_allocated_result(mutableTestResult);

		bool isLastChunk;
		std::string data = Util::loadTestcaseChunkInMemory(testcaseId, m_testcaseDir, 0, &isLastChunk);
		if (isLastChunk) {
			//If the last chunk was just read: delete the file on all systems but the testcase generator (as the files are only temporary files)
			std::string testfile = Util::generateTestcasePathAndFilename(testcaseId, m_testcaseDir);
			m_garbageCollectorWorker->markFileForDelete(testfile);
		}
		putTestEvaluationRequest->set_islastchunk(isLastChunk);
		putTestEvaluationRequest->set_testcasefirstchunk(data);

		ServiceDescriptor* ptMySelfServiceDescriptor = new ServiceDescriptor();
		ptMySelfServiceDescriptor->CopyFrom(m_commInt->getOwnServiceDescriptor().getProtobuf());
		putTestEvaluationRequest->set_allocated_sdofevaluator(ptMySelfServiceDescriptor);
	}
	putTestEvaluationRequest->set_eval(eval);

	req.set_allocated_puttestevaluationrequest(putTestEvaluationRequest);
	bool success = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, m_currentTestcaseEvaluationSendTimeout);
	updateTestcaseEvaluationSendTimeout(success, eval, tod->getTestResult().m_exitType);
	if (!success) {
		LOG(ERROR) << "Sending PutTestEvaluationRequest failed!";
		return; //Just return - the generator will detect that there was an error and resend the testcase
	}

	if (!resp.puttestevaluationresponse().success()) {
		LOG(ERROR) << "The server was not able to process the PutTestEvaluationRequest!";
		return; //Just return - the generator will detect that there was an error and resend the testcase
	}
}

int CoverageEvaluator::generateParentRatingDelta(ExitType exitType, bool hasNewBlocks, bool isSpecial) {
	int parentRatingDelta = 0;

	if (isSpecial) {
		return 0;
	}

	switch (exitType)
	{
	case ExitType::CleanExit:
		parentRatingDelta = 0;
		break;
	case ExitType::Hang:
		parentRatingDelta = m_deltaHang;
		break;
	case ExitType::Exception_AccessViolation:
		parentRatingDelta = m_deltaAccessViolation;
		break;
	case ExitType::Exception_Other:
		parentRatingDelta = m_deltaOtherException;
		break;
	default:
		break;
	}

	if (hasNewBlocks) {
		parentRatingDelta += m_deltaNewBlocks;
	}

	return parentRatingDelta;
}

FluffiTestResult CoverageEvaluator::copyTestResultButStripDuplicateBlocks(const FluffiTestResult originalTestResult) {
	std::set<FluffiBasicBlock> strippedBlocks_s(originalTestResult.m_blocks.begin(), originalTestResult.m_blocks.end());
	std::vector<FluffiBasicBlock> strippedBlocks(strippedBlocks_s.begin(), strippedBlocks_s.end());

	return FluffiTestResult(originalTestResult.m_exitType, strippedBlocks, originalTestResult.m_crashFootprint, originalTestResult.m_hasFullCoverage);
}

void CoverageEvaluator::updateTestcaseEvaluationSendTimeout(bool wasLastTransferSuccessful, TestEvaluation eval, ExitType exitType) {
	if (wasLastTransferSuccessful) {
		if (m_currentTestcaseEvaluationSendTimeout - 1000 > CommInt::timeoutFileTransferMessage && eval == TestEvaluation::Keep && exitType == ExitType::CleanExit) {
			LOG(INFO) << "Decreasing m_currentTestcaseEvaluationSendTimeout to " << m_currentTestcaseEvaluationSendTimeout;
			m_currentTestcaseEvaluationSendTimeout -= 1000;
		}
	}
	else {
		LOG(INFO) << "Increasing m_currentTestcaseEvaluationSendTimeout to " << m_currentTestcaseEvaluationSendTimeout;
		m_currentTestcaseEvaluationSendTimeout += 5000;
	}
}
