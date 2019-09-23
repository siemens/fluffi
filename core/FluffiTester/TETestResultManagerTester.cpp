/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Michael Kraus, Thomas Riedmaier, Abian Blome, Pascal Eckmann
*/

§§#include "stdafx.h"
§§#include "CppUnitTest.h"
#include "Util.h"
#include "TETestResultManager.h"
#include "FluffiServiceDescriptor.h"
#include "FluffiTestResult.h"
#include "FluffiTestcaseID.h"
#include "TestOutcomeDescriptor.h"
#include "GarbageCollectorWorker.h"
§§
§§using namespace Microsoft::VisualStudio::CppUnitTestFramework;
§§
§§namespace FluffiTester
§§{
§§	TEST_CLASS(TETestResultManagerTest)
§§	{
§§	public:
§§
		TETestResultManager* testResultManager = nullptr;
		GarbageCollectorWorker* garbageCollector = nullptr;
§§
§§		TEST_METHOD_INITIALIZE(ModuleInitialize)
§§		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
			garbageCollector = new GarbageCollectorWorker(200);
			testResultManager = new TETestResultManager(".", garbageCollector);
§§		}
§§
§§		TEST_METHOD_CLEANUP(ModuleCleanup)
§§		{
§§			//Freeing the testcase Queue
§§			delete testResultManager;
§§			testResultManager = nullptr;

			// Freeing the garbageCollector
			delete garbageCollector;
			garbageCollector = nullptr;
§§		}
§§
		TEST_METHOD(TETestResultManager_pushNewTestOutcomeFromTCRunner)
§§		{
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)0, L"Error initializing TETestResultManager with empty testOutcomeQueue (pushNewTestOutcomeFromTCRunner)");
§§
§§			const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
§§			FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 111);
§§			FluffiTestcaseID parentId1 = FluffiTestcaseID(sd1, 222);
			FluffiTestResult tR1 = FluffiTestResult(ExitType::Exception_Other, std::vector<FluffiBasicBlock>(), std::string("testMessage1"), true);
			TestOutcomeDescriptor* outcomeDesc1 = new TestOutcomeDescriptor(id1, parentId1, tR1);
§§
§§			// Test Method
§§			testResultManager->pushNewTestOutcomeFromTCRunner(outcomeDesc1);
§§
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)1, L"Error pushing TestResult into testOutcomeQueue of TETestResultManager (pushNewTestOutcomeFromTCRunner)");
§§
§§			const FluffiServiceDescriptor sd2 = FluffiServiceDescriptor("testServiceDescriptor2", "testGuiID2");
§§			FluffiTestcaseID id2 = FluffiTestcaseID(sd2, 333);
§§			FluffiTestcaseID parentId2 = FluffiTestcaseID(sd2, 444);
			FluffiTestResult tR2 = FluffiTestResult(ExitType::Exception_Other, std::vector<FluffiBasicBlock>(), std::string("testMessage2"), true);
			TestOutcomeDescriptor* outcomeDesc2 = new TestOutcomeDescriptor(id2, parentId2, tR2);
§§
§§			// Test Method
§§			testResultManager->pushNewTestOutcomeFromTCRunner(outcomeDesc2);
§§
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)2, L"Error pushing TestResult into testOutcomeQueue of TETestResultManager (pushNewTestOutcomeFromTCRunner)");
§§		}
§§
		TEST_METHOD(TETestResultManager_popTestOutcomeForEvaluation)
§§		{
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)0, L"Error initializing TETestResultManager with empty testOutcomeQueue (popTestOutcomeForEvaluation)");
§§
§§			const FluffiServiceDescriptor sd = FluffiServiceDescriptor("testServiceDescriptor", "testGuiID");
§§			FluffiTestcaseID id = FluffiTestcaseID(sd, 111);
§§			FluffiTestcaseID parentId = FluffiTestcaseID(sd, 222);
			FluffiTestResult tR = FluffiTestResult(ExitType::Exception_Other, std::vector<FluffiBasicBlock>(), std::string("testMessage"), true);
§§
			TestOutcomeDescriptor* outcomeDesc = new TestOutcomeDescriptor(id, parentId, tR);
§§			testResultManager->pushNewTestOutcomeFromTCRunner(outcomeDesc);
§§
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)1, L"Error calculating correct size of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
§§
§§			// Test Method
§§			TestOutcomeDescriptor* tOD = testResultManager->popTestOutcomeForEvaluation();
§§			Assert::AreEqual(tOD->getId().m_localID, (size_t)111, L"Error popping correct item of of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
§§			Assert::AreEqual(tOD->getparentId().m_localID, (size_t)222, L"Error popping correct item of of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
§§			Assert::AreEqual(tOD->getTestResult().m_crashFootprint, std::string("testMessage"), L"Error popping correct item of of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
§§
§§			int runningId = 0;
§§			int runningPId = 100;
§§
§§			for (int i = 1; i < 21; i++) {
§§				runningId++;
§§				runningPId++;
§§
§§				const FluffiServiceDescriptor sd = FluffiServiceDescriptor("testServiceDescriptor", "testGuiID");
§§				FluffiTestcaseID id = FluffiTestcaseID(sd, runningId);
§§				FluffiTestcaseID parentId = FluffiTestcaseID(sd, runningPId);
				FluffiTestResult tR = FluffiTestResult(ExitType::Exception_Other, std::vector<FluffiBasicBlock>(), std::string("testMessage" + runningId), true);
§§
				TestOutcomeDescriptor* outcomeDesc = new TestOutcomeDescriptor(id, parentId, tR);
§§				testResultManager->pushNewTestOutcomeFromTCRunner(outcomeDesc);
§§
§§				Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)(i), L"Error calculating correct size of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
§§			}
§§
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)20, L"Error calculating correct size of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
§§
§§			runningId = 0;
§§			runningPId = 100;
§§
§§			for (int i = 21; i > 1; i--) {
§§				runningId++;
§§				runningPId++;
§§
§§				// Test Method
§§				TestOutcomeDescriptor* tOD = testResultManager->popTestOutcomeForEvaluation();
§§				Assert::AreEqual(tOD->getId().m_localID, (size_t)runningId, L"Error popping correct item of of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
§§				Assert::AreEqual(tOD->getparentId().m_localID, (size_t)runningPId, L"Error popping correct item of of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
				Assert::AreEqual(tOD->getTestResult().m_crashFootprint, std::string("testMessage" + runningId), L"Error popping correct item of of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
§§			}
§§
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)0, L"Error calculating correct size of testOutcomeQueue of TETestResultManager (popTestOutcomeForEvaluation)");
§§
§§			for (int i = 0; i < 5; i++) {
§§				TestOutcomeDescriptor* tOD = testResultManager->popTestOutcomeForEvaluation();
§§				Assert::IsNull(tOD);
§§			}
§§		}
§§
		TEST_METHOD(TETestResultManager_getTestOutcomeDequeSize)
§§		{
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)0, L"Error initializing TETestResultManager with empty testOutcomeQueue (getTestOutcomeDequeSize)");
§§
§§			const FluffiServiceDescriptor sd = FluffiServiceDescriptor("testServiceDescriptor", "testGuiID");
§§			FluffiTestcaseID id = FluffiTestcaseID(sd, 190);
§§			FluffiTestcaseID parentId = FluffiTestcaseID(sd, 111);
			FluffiTestResult tR = FluffiTestResult(ExitType::Exception_Other, std::vector<FluffiBasicBlock>(), std::string("testMessage"), true);
§§
			TestOutcomeDescriptor* outcomeDesc = new TestOutcomeDescriptor(id, parentId, tR);
§§			testResultManager->pushNewTestOutcomeFromTCRunner(outcomeDesc);
§§
§§			// Test Method
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)1, L"Error calculating correct size of testOutcomeQueue of TETestResultManager (getTestOutcomeDequeSize)");
§§
§§			for (int i = 0; i < 20; i++) {
§§				const FluffiServiceDescriptor sd = FluffiServiceDescriptor("testServiceDescriptor", "testGuiID");
§§				FluffiTestcaseID id = FluffiTestcaseID(sd, 190);
§§				FluffiTestcaseID parentId = FluffiTestcaseID(sd, 111);
				FluffiTestResult tR = FluffiTestResult(ExitType::Exception_Other, std::vector<FluffiBasicBlock>(), std::string("testMessage"), true);
§§
				TestOutcomeDescriptor* outcomeDesc = new TestOutcomeDescriptor(id, parentId, tR);
§§				testResultManager->pushNewTestOutcomeFromTCRunner(outcomeDesc);
§§
§§				// Test Method
				Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)(i + 2), L"Error calculating correct size of testOutcomeQueue of TETestResultManager (getTestOutcomeDequeSize)");
§§			}
§§
§§			// Test Method
§§			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)21, L"Error calculating correct size of testOutcomeQueue of TETestResultManager (getTestOutcomeDequeSize)");
§§		}

		TEST_METHOD(TETestResultManager_isThereAlreadyAToDFor)
		{
			Assert::AreEqual(testResultManager->getTestOutcomeDequeSize(), (size_t)0, L"Error initializing TETestResultManager with empty testOutcomeQueue");

			const FluffiServiceDescriptor sd = FluffiServiceDescriptor("testServiceDescriptor", "testGuiID");
			FluffiTestcaseID id = FluffiTestcaseID(sd, 190);
			FluffiTestcaseID parentId = FluffiTestcaseID(sd, 111);
			FluffiTestResult tR = FluffiTestResult(ExitType::Exception_Other, std::vector<FluffiBasicBlock>(), std::string("testMessage"), true);

§§			TestOutcomeDescriptor* outcomeDesc = new TestOutcomeDescriptor(id, parentId, tR);
			testResultManager->pushNewTestOutcomeFromTCRunner(outcomeDesc);

			Assert::IsTrue(testResultManager->isThereAlreadyAToDFor(id), L"The TETestResultManager failed to find the ToD we just pushed");

			FluffiTestcaseID id2 = FluffiTestcaseID(sd, 191);
			Assert::IsFalse(testResultManager->isThereAlreadyAToDFor(id2), L"The TETestResultManager falsely claims the presence of a ToD that was never inserted.");
		}
§§	};
}
