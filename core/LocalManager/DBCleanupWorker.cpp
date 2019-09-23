/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "DBCleanupWorker.h"
#include "LMWorkerThreadStateBuilder.h"
#include "FluffiServiceDescriptor.h"
#include "LMWorkerThreadState.h"

DBCleanupWorker::DBCleanupWorker(CommInt* commInt, LMWorkerThreadStateBuilder* workerThreadStateBuilder, int timeBetweenTwoCleanupRoundsInMS, std::string location) :
	m_commInt(commInt), m_workerThreadStateBuilder(workerThreadStateBuilder), m_timeBetweenTwoCleanupRoundsInMS(timeBetweenTwoCleanupRoundsInMS), m_location(location)
{
}

DBCleanupWorker::~DBCleanupWorker()
{
	delete m_thread;
	m_thread = nullptr;
}

void DBCleanupWorker::stop() {
	if (m_workerThreadState != nullptr) {
		m_workerThreadState->m_stopRequested = true;
	}
}

void DBCleanupWorker::workerMain() {
	m_workerThreadState = dynamic_cast<LMWorkerThreadState*>(m_workerThreadStateBuilder->constructState());
	if (m_workerThreadState == nullptr) {
		LOG(ERROR) << "DBCleanupWorker::workerMain - m_workerThreadStateBuilder->constructState() failed";
		return;
	}

	m_workerThreadState->dbManager->initializeBillingTable();

	int checkAgainMS = 500;
	int waitedMS = 0;

	while (!m_workerThreadState->m_stopRequested)
	{
		if (waitedMS < m_timeBetweenTwoCleanupRoundsInMS) {
			std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
			waitedMS += checkAgainMS;
			continue;
		}
		else {
			waitedMS = 0;
		}

		//Actual loop body

		LOG(DEBUG) << "DBCleanupWorker running!";
		updateRunnerSeconds();
		updateRunTestcasesNoLongerShowing();
		dropCrashesThatAppearedMoreThanXTimes(1000);
		dropTestcaseTypeIFMoreThan(LMDatabaseManager::TestCaseType::Hang, 10000);
		dropTestcaseTypeIFMoreThan(LMDatabaseManager::TestCaseType::NoResponse, 10000);
	}
	m_workerThreadStateBuilder->destructState(m_workerThreadState);
}

void DBCleanupWorker::updateRunnerSeconds() {
	unsigned int activeRunners = static_cast<unsigned int>(m_workerThreadState->dbManager->getRegisteredInstancesOfAgentType(AgentType::TestcaseRunner, m_location).size());
	m_workerThreadState->dbManager->addRunnerSeconds(activeRunners* m_timeBetweenTwoCleanupRoundsInMS / 1000);
}

void DBCleanupWorker::updateRunTestcasesNoLongerShowing() {
	unsigned int deletedRows = static_cast<unsigned int>(m_workerThreadState->dbManager->cleanCompletedTestcasesTableOlderThanXSec(3 * 60));
	m_workerThreadState->dbManager->addRunTestcasesNoLongerListed(deletedRows);
}

void DBCleanupWorker::dropCrashesThatAppearedMoreThanXTimes(int times) {
	for (auto& crashFootprint : m_workerThreadState->dbManager->getAllCrashFootprints()) {
		m_workerThreadState->dbManager->dropTestcaseIfCrashFootprintAppearedMoreThanXTimes(times, crashFootprint);
	}
}

void DBCleanupWorker::dropTestcaseTypeIFMoreThan(LMDatabaseManager::TestCaseType type, int instances) {
	m_workerThreadState->dbManager->dropTestcaseTypeIFMoreThan(type, instances);
}
