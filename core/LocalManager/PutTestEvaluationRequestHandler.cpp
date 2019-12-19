/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "PutTestEvaluationRequestHandler.h"
#include "LMWorkerThreadState.h"
#include "Util.h"
#include "LMDatabaseManager.h"
#include "GarbageCollectorWorker.h"
#include "FluffiBasicBlock.h"
#include "FluffiTestcaseID.h"

//forceFlushAfterMS should be well below intervallToWaitForReinsertionInMillisec
PutTestEvaluationRequestHandler::PutTestEvaluationRequestHandler(std::string testcaseDir, CommInt* commPtr, GarbageCollectorWorker* garbageCollectorWorker, int forceFlushAfterMS) :
	m_testcaseDir(testcaseDir),
	m_comm(commPtr),
	m_garbageCollectorWorker(garbageCollectorWorker),
	m_completedTestcases_notYetPushedToDB(),
	m_completedTestcasesCache_mutex_(),
	m_timeOfLastFlush(std::chrono::steady_clock::now()),
	m_forceFlushAfterMS(forceFlushAfterMS)
{
}

PutTestEvaluationRequestHandler::~PutTestEvaluationRequestHandler()
{
}

bool PutTestEvaluationRequestHandler::markTestcaseAsCompleted(LMDatabaseManager* dbManager, FluffiTestcaseID testcaseID) {
	//add one by one
	//return dbManager->addEntryToCompletedTestcasesTable(testcaseID);

	//adding the tcid to the completed_testcases table is the most frequent db operation. for performance reasons, this is clustered in a bulk insert.
	//When shutting down the LM, all entries that have not yet been flushed will be lost. This is, however, not a problem as the TGs, that depend on this information
	//will need to restart anyway once the LM is gone
	std::unique_lock<std::mutex> mlock(m_completedTestcasesCache_mutex_);
	m_completedTestcases_notYetPushedToDB.insert(testcaseID);

	if (m_completedTestcases_notYetPushedToDB.size() > 100 || std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_timeOfLastFlush).count() > m_forceFlushAfterMS) {
		LOG(DEBUG) << "flushing the set of cashed completed testcases to the database";
		//flush completed testcases to db
		bool success = dbManager->addEntriesToCompletedTestcasesTable(m_completedTestcases_notYetPushedToDB);
		if (success) {
			m_completedTestcases_notYetPushedToDB.clear();
			m_timeOfLastFlush = std::chrono::steady_clock::now();
		}
		return success;
	}
	else {
		return true;
	}
}

void PutTestEvaluationRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp)
{
	LMWorkerThreadState* lmWorkerThreadState = dynamic_cast<LMWorkerThreadState*>(workerThreadState);
	if (lmWorkerThreadState == nullptr) {
		LOG(ERROR) << "PutTestEvaluationRequestHandler::handleFLUFFIMessage - workerThreadState cannot be accessed";
		return;
	}

	const PutTestEvaluationRequest* putTestEvaluationRequest = &req->puttestevaluationrequest();

	FluffiTestcaseID testcaseID = FluffiTestcaseID(putTestEvaluationRequest->id());
	FluffiTestcaseID parentTestcaseID = FluffiTestcaseID(putTestEvaluationRequest->parentid());

	bool success = markTestcaseAsCompleted(lmWorkerThreadState->dbManager, testcaseID);
	if (!success) {
		LOG(ERROR) << "PutTestEvaluationRequestHandler: addEntryToCompletedTestcasesTable failed";
		goto fail;
	}

	//Modify rating of parent
	success = lmWorkerThreadState->dbManager->addDeltaToTestcaseRating(parentTestcaseID, putTestEvaluationRequest->parentratingdelta());
	if (!success) {
		LOG(ERROR) << "PutTestEvaluationRequestHandler: addDeltaToTestcaseRating failed";
		goto fail;
	}

	switch (putTestEvaluationRequest->eval()) {
	case TestEvaluation::Discard:
		//Discard the testcase - no more actions required
		LOG(DEBUG) << "PutTestEvaluationRequestHandler - Discard";
		break;
	case TestEvaluation::Keep:
		LOG(DEBUG) << "PutTestEvaluationRequestHandler - Keep";

		//Get & store testcase bin
		success = Util::storeTestcaseFileOnDisk(FluffiTestcaseID(putTestEvaluationRequest->id()), m_testcaseDir, &putTestEvaluationRequest->testcasefirstchunk(), putTestEvaluationRequest->islastchunk(), putTestEvaluationRequest->sdofevaluator().servicehostandport(), m_comm, workerThreadState, m_garbageCollectorWorker);
		if (!success) {
			LOG(ERROR) << "PutTestEvaluationRequestHandler: storeTestcaseFileOnDisk failed";
			goto fail;
		}

		//Translate testcase type
		LMDatabaseManager::TestCaseType tcType;
		switch (putTestEvaluationRequest->result().exittype())
		{
		case ExitType::CleanExit:
			tcType = LMDatabaseManager::TestCaseType::Population;
			break;
		case ExitType::Hang:
			tcType = LMDatabaseManager::TestCaseType::Hang;
			break;
		case ExitType::Exception_AccessViolation:
			tcType = LMDatabaseManager::TestCaseType::Exception_AccessViolation;
			break;
		case ExitType::Exception_Other:
			tcType = LMDatabaseManager::TestCaseType::Exception_Other;
			break;
		default:
			tcType = LMDatabaseManager::TestCaseType::Exception_Other;
			break;
		}

		//Add testcase to database (Testacase file gets deleted here)
		success = lmWorkerThreadState->dbManager->addEntryToInterestingTestcasesTable(
			testcaseID,
			parentTestcaseID,
			(tcType == LMDatabaseManager::TestCaseType::Population) ? LMDatabaseManager::defaultInitialPopulationRating : 0,
			m_testcaseDir,
			(tcType == LMDatabaseManager::TestCaseType::Population) ? LMDatabaseManager::TestCaseType::Locked : tcType);
		if (!success) {
			LOG(ERROR) << "PutTestEvaluationRequestHandler: addEntryToInterestingTestcasesTable failed";
			goto fail;
		}

		if (tcType == LMDatabaseManager::TestCaseType::Exception_AccessViolation || tcType == LMDatabaseManager::TestCaseType::Exception_Other) {
			//Insert crash details
			success = lmWorkerThreadState->dbManager->addEntryToCrashDescriptionsTable(testcaseID, putTestEvaluationRequest->result().crashfootprint());
			if (!success) {
				LOG(ERROR) << "PutTestEvaluationRequestHandler: addEntryToCrashDescriptionsTable failed";
				goto fail;
			}
		}
		else if (tcType == LMDatabaseManager::TestCaseType::Population) {
			//Add blocks to covered blocks
			//Insert blocks. Apparently this may fail due to multithreading. To avoid this, the blocks need to be sorted. We will do this by passing them as set
			std::set<FluffiBasicBlock> blocksAsSet;
			for (int i = 0; i < putTestEvaluationRequest->result().blocks().size(); i++) {
				blocksAsSet.insert(FluffiBasicBlock(putTestEvaluationRequest->result().blocks().Get(i)));
			}
			success = lmWorkerThreadState->dbManager->addBlocksToCoveredBlocks(testcaseID, blocksAsSet);
			if (!success) {
				LOG(ERROR) << "PutTestEvaluationRequestHandler: addBlocksToCoveredBlocks failed";
				goto fail;
			}

			//Now that all blocks are inserted unlock the population entry (it was inserted locked to avoid handler hangs due to getTestcaseToMutateRequest trying to access the population entry while addBlocksToCoveredBlocks is still running)
			success = lmWorkerThreadState->dbManager->setTestcaseType(testcaseID, LMDatabaseManager::TestCaseType::Population);
			if (!success) {
				LOG(ERROR) << "PutTestEvaluationRequestHandler: setTestcaseType failed";
				goto fail;
			}
		}

		break;
	default:
		break;
	}

fail:
	PutTestEvaluationResponse* putTestEvaluationResponse = new PutTestEvaluationResponse();
	putTestEvaluationResponse->set_success(success);
	LOG(DEBUG) << "PutTestEvaluationRequestHandler returns " << success;
	resp->set_allocated_puttestevaluationresponse(putTestEvaluationResponse);
}