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
#include "CoveragePullerWorker.h"
#include "TEWorkerThreadStateBuilder.h"
#include "CommInt.h"
#include "BlockCoverageCache.h"
#include "TEWorkerThreadState.h"
#include "FluffiBasicBlock.h"

CoveragePullerWorker::CoveragePullerWorker(CommInt* commInt, TEWorkerThreadStateBuilder* workerThreadStateBuilder, int intervallBetweenTwoPullingRoundsInMillisec, BlockCoverageCache* localBlockCoverageCache)
	: m_commInt(commInt),
	m_workerThreadStateBuilder(workerThreadStateBuilder),
	m_intervallBetweenTwoPullingRoundsInMillisec(intervallBetweenTwoPullingRoundsInMillisec),
	m_localBlockCoverageCache(localBlockCoverageCache)
{
}

CoveragePullerWorker::~CoveragePullerWorker()
{
	delete m_thread;
	m_thread = nullptr;
}

void CoveragePullerWorker::stop() {
	if (m_workerThreadState != nullptr) {
		m_workerThreadState->m_stopRequested = true;
	}
}

void CoveragePullerWorker::workerMain() {
	m_workerThreadState = dynamic_cast<TEWorkerThreadState*>(m_workerThreadStateBuilder->constructState());

	int checkAgainMS = 500;
	int waitedMS = m_intervallBetweenTwoPullingRoundsInMillisec; //Start right after creation

	while (!m_workerThreadState->m_stopRequested)
	{
		if (waitedMS < m_intervallBetweenTwoPullingRoundsInMillisec) {
			std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
			waitedMS += checkAgainMS;
			continue;
		}
		else {
			waitedMS = 0;
		}

		//Actual loop body
		pullCoverageFromLM();
	}
	m_workerThreadStateBuilder->destructState(m_workerThreadState);
}

void CoveragePullerWorker::pullCoverageFromLM() {
	LOG(DEBUG) << "Pulling current coverage from LM";

	// Build and send message for GetTestcaseRequest
	FLUFFIMessage req;
	FLUFFIMessage resp;

	GetCurrentBlockCoverageRequest* getCurrentBlockCoverageRequest = new GetCurrentBlockCoverageRequest();

	req.set_allocated_getcurrentblockcoveragerequest(getCurrentBlockCoverageRequest);

	bool respReceived = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, CommInt::timeoutNormalMessage);
	if (respReceived) {
		LOG(DEBUG) << "GetCurrentBlockCoverageResponse successfully received!";
	}
	else {
		LOG(ERROR) << "No GetCurrentBlockCoverageResponse received!";
		return;
	}

	const GetCurrentBlockCoverageResponse* receivedTestcase = &(resp.getcurrentblockcoverageresponse());

	LOG(DEBUG) << "Adding " << receivedTestcase->blocks().size() << " new basic blocks";
	std::set<FluffiBasicBlock> blocksAsSet;
	for (int i = 0; i < receivedTestcase->blocks().size(); i++) {
		blocksAsSet.insert(FluffiBasicBlock(receivedTestcase->blocks().Get(i)));
	}
	m_localBlockCoverageCache->addBlocksToCache(&blocksAsSet);

	LOG(DEBUG) << "The cache has now " << m_localBlockCoverageCache->getSizeOfCache() << " entries.";
}
