/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Michael Kraus
*/

§§#include "stdafx.h"
§§#include "TGTestcaseManager.h"
#include "GarbageCollectorWorker.h"
§§
TGTestcaseManager::TGTestcaseManager(GarbageCollectorWorker* garbageCollectorWorker) :
	m_garbageCollectorWorker(garbageCollectorWorker),
	m_mutex_()
{}
§§
§§TGTestcaseManager::~TGTestcaseManager()
{
	for (auto&& it : m_pendingTestcaseQueue)
	{
		it.deleteFile(m_garbageCollectorWorker);
	}

	for (auto&&it : m_sentButNotEvaluatedTestcaseSet)
	{
		it.deleteFile(m_garbageCollectorWorker);
	}
}
§§
void TGTestcaseManager::pushNewGeneratedTestcase(TestcaseDescriptor newTestcase)
§§{
	std::unique_lock<std::mutex> mlock(m_mutex_);
	m_pendingTestcaseQueue.push_back(newTestcase);
	LOG(DEBUG) << "Pushed a new generated Testcase into the Testcase Queue for pending Testcases.";
§§}
§§
void TGTestcaseManager::pushNewGeneratedTestcases(std::deque<TestcaseDescriptor> newTestcases)
{
	std::unique_lock<std::mutex> mlock(m_mutex_);
	m_pendingTestcaseQueue.insert(m_pendingTestcaseQueue.end(), newTestcases.begin(), newTestcases.end());
	LOG(DEBUG) << "Pushed " << newTestcases.size() << " new Testcase into the Testcase Queue for pending Testcases.";
}

TestcaseDescriptor TGTestcaseManager::popPendingTCForProcessing()
§§{
	std::unique_lock<std::mutex> mlock(m_mutex_);
§§
	if (!m_pendingTestcaseQueue.empty()) {
		// Get next TestcaseDescriptor from pendingTestcaseQueue, then remove
		TestcaseDescriptor testcase = m_pendingTestcaseQueue.front();
		m_pendingTestcaseQueue.pop_front();

		//Increase number of processing attempts
		testcase.increaseNumOfProcessingAttempts();
§§
		//Update the timestamp of when the testcase was last sent
		testcase.updateTimeOfLastSend();

§§		// Move testcase from pending to waitForEvaluation set
		m_sentButNotEvaluatedTestcaseSet.push_back(testcase);
§§
§§		return testcase;
§§	}
§§
	throw std::runtime_error("No pending testcases");
§§}
§§
void TGTestcaseManager::removeEvaluatedTestcases(const google::protobuf::RepeatedPtrField<TestcaseID>  evaluatedTestcases)
§§{
	std::unique_lock<std::mutex> mlock(m_mutex_);
§§
	for (auto it = m_sentButNotEvaluatedTestcaseSet.begin(); it != m_sentButNotEvaluatedTestcaseSet.end(); )
	{
		if (std::find(evaluatedTestcases.begin(), evaluatedTestcases.end(), it->getId()) != evaluatedTestcases.end())
		{
			LOG(DEBUG) << "Testcase: " << it->getId() << " successfully evaluated! Removed from set!";
			it->deleteFile(m_garbageCollectorWorker);
			it = m_sentButNotEvaluatedTestcaseSet.erase(it);
		}
		else
		{
			++it;
§§		}
§§	}
}
§§
size_t TGTestcaseManager::getPendingTestcaseQueueSize() {
	return m_pendingTestcaseQueue.size();
§§}

size_t TGTestcaseManager::getSentButNotEvaluatedTestcaseSetSize() {
	return m_sentButNotEvaluatedTestcaseSet.size();
}

void TGTestcaseManager::reinsertTestcasesHavingNoAnswerSince(std::chrono::time_point<std::chrono::steady_clock> timeBeforeWhichShouldBeReinserted) {
	std::unique_lock<std::mutex> mlock(m_mutex_);

	for (auto it = m_sentButNotEvaluatedTestcaseSet.begin(); it != m_sentButNotEvaluatedTestcaseSet.end(); ) {
		if (it->getTimeOfLastSend() < timeBeforeWhichShouldBeReinserted) {
			m_pendingTestcaseQueue.push_back(*it);
			LOG(DEBUG) << "Testcase: " << it->getId() << " reinserted into pending test case queue.";
			it = m_sentButNotEvaluatedTestcaseSet.erase(it);
		}
		else {
			++it;
		}
	}
}

std::vector<TestcaseDescriptor> TGTestcaseManager::handMeAllTestcasesWithTooManyRetries(int maxRetriesBeforeReport) {
	std::unique_lock<std::mutex> mlock(m_mutex_);

	std::vector<TestcaseDescriptor> theReturnSet{};

	for (auto it = m_sentButNotEvaluatedTestcaseSet.begin(); it != m_sentButNotEvaluatedTestcaseSet.end(); ) {
		if (it->getNumOfProcessingAttempts() >= maxRetriesBeforeReport) {
			theReturnSet.push_back(*it);
			LOG(INFO) << "Testcase: " << it->getId() << " was removed from sentButNotEvaluatedTestcaseSet as it had too many reties.";
			it = m_sentButNotEvaluatedTestcaseSet.erase(it);
		}
		else {
			++it;
		}
	}

	return theReturnSet;
}
