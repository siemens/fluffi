§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Michael Kraus, Abian Blome
§§*/
§§
§§#include "stdafx.h"
§§#include "TETestResultManager.h"
§§#include "TestOutcomeDescriptor.h"
§§#include "Util.h"
§§#include "GarbageCollectorWorker.h"
§§#include "FluffiTestcaseID.h"
§§
§§TETestResultManager::TETestResultManager(std::string testcaseDir, GarbageCollectorWorker* garbageCollectorWorker) :
§§	m_testOutcomeQueue(),
§§	m_mutex_(),
§§	m_testcaseDir(testcaseDir),
§§	m_garbageCollectorWorker(garbageCollectorWorker)
§§{
§§}
§§
§§TETestResultManager::~TETestResultManager()
§§{
§§	while (!m_testOutcomeQueue.empty()) {
§§		std::string filename = Util::generateTestcasePathAndFilename(m_testOutcomeQueue.front()->getId(), m_testcaseDir);
§§		m_garbageCollectorWorker->markFileForDelete(filename);
§§		delete m_testOutcomeQueue.front();
§§		m_testOutcomeQueue.pop_front();
§§	}
§§}
§§
§§void TETestResultManager::pushNewTestOutcomeFromTCRunner(TestOutcomeDescriptor* newTestcaseOutcome)
§§{
§§	std::unique_lock<std::mutex> mlock(m_mutex_);
§§	m_testOutcomeQueue.push_back(newTestcaseOutcome);
§§	LOG(DEBUG) << "Pushed a new received TestcaseOutcome from TestcaseRunner into the TestcaseOutcomeQueue";
§§}
§§
§§bool TETestResultManager::isThereAlreadyAToDFor(FluffiTestcaseID id) {
§§	std::unique_lock<std::mutex> mlock(m_mutex_);
§§
§§	for (auto& testOutcome : m_testOutcomeQueue) {
§§		if (testOutcome->getId() == id) {
§§			return true;
§§		}
§§	}
§§	return false;
§§}
§§
§§TestOutcomeDescriptor* TETestResultManager::popTestOutcomeForEvaluation()
§§{
§§	std::unique_lock<std::mutex> mlock(m_mutex_);
§§
§§	if (!m_testOutcomeQueue.empty()) {
§§		// Get next TestOutcomeDescriptor from testOutcomeQueue, then remove
§§		TestOutcomeDescriptor* testcase = m_testOutcomeQueue.front();
§§		m_testOutcomeQueue.pop_front();
§§
§§		return testcase;
§§	}
§§
§§	return nullptr;
§§}
§§
§§size_t TETestResultManager::getTestOutcomeDequeSize()
§§{
§§	return m_testOutcomeQueue.size();
§§}
