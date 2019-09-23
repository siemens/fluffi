/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Michael Kraus, Thomas Riedmaier, Pascal Eckmann, Abian Blome
*/

§§#include "stdafx.h"
§§#include "CppUnitTest.h"
#include "Util.h"
#include "TGTestcaseManager.h"
#include "GarbageCollectorWorker.h"
§§
§§using namespace Microsoft::VisualStudio::CppUnitTestFramework;
§§
§§namespace FluffiTester
§§{
§§	TEST_CLASS(TGTestcaseManagerTest)
§§	{
§§	public:
§§
§§		TGTestcaseManager* testcaseManager = nullptr;
§§		GarbageCollectorWorker* garbageCollector = nullptr;
§§
§§		TEST_METHOD_INITIALIZE(ModuleInitialize)
§§		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
			garbageCollector = new GarbageCollectorWorker(200);
			testcaseManager = new TGTestcaseManager(garbageCollector);
§§		}
§§
		TEST_METHOD_CLEANUP(ModuleCleanup)
§§		{
§§			//Freeing the testcase Queue
§§			delete testcaseManager;
§§			testcaseManager = nullptr;

			// Freeing the garbageCollector
			delete garbageCollector;
			garbageCollector = nullptr;
§§		}
§§
		TEST_METHOD(TGTestcaseManager_pushNewGeneratedTestcase_getQueueSize)
§§		{
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error initializing pendingTestcaseQueue (pushNewGeneratedTestcase)");

			const FluffiServiceDescriptor sd = FluffiServiceDescriptor("testServiceDescriptor", "testGuiID");
§§			FluffiTestcaseID id = FluffiTestcaseID(sd, 190);
§§			FluffiTestcaseID parentId = FluffiTestcaseID(sd, 111);
§§			std::string pathAndFileName = "testpath";
			TestcaseDescriptor testcase = TestcaseDescriptor(id, parentId, pathAndFileName, true);
§§
§§			// Test Method
§§			testcaseManager->pushNewGeneratedTestcase(testcase);
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)1, L"Error pushing Testcase into pendingTestcaseQueue: (pushNewGeneratedTestcase)");
§§		}
§§
		TEST_METHOD(TGTestcaseManager_pushNewGeneratedTestcases)
§§		{
§§			std::deque<TestcaseDescriptor> testcases = std::deque<TestcaseDescriptor>();

§§			const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
§§			FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 222);
§§			FluffiTestcaseID parentId1 = FluffiTestcaseID(sd1, 333);
§§			std::string pathAndFileName1 = "testpath";
			TestcaseDescriptor testcase1 = TestcaseDescriptor(id1, parentId1, pathAndFileName1, true);
§§
§§			const FluffiServiceDescriptor sd2 = FluffiServiceDescriptor("testServiceDescriptor2", "testGuiID2");
§§			FluffiTestcaseID id2 = FluffiTestcaseID(sd2, 444);
§§			FluffiTestcaseID parentId2 = FluffiTestcaseID(sd2, 555);
§§			std::string pathAndFileName2 = "testpath";
			TestcaseDescriptor testcase2 = TestcaseDescriptor(id2, parentId2, pathAndFileName2, true);
§§
§§			testcases.push_back(testcase1);
§§			testcases.push_back(testcase2);
§§
§§			// Test Method
§§			testcaseManager->pushNewGeneratedTestcases(testcases);
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)2, L"Error pushing TestcaseS into pendingTestcaseQueue: (pushNewGeneratedTestcases)");
§§
§§			int oldNumOfProcessingAttempts = testcase1.getNumOfProcessingAttempts();
§§			TestcaseDescriptor poppedTestcase1 = testcaseManager->popPendingTCForProcessing();
§§			Assert::AreEqual(poppedTestcase1.getId().m_localID, id1.m_localID, L"Error testcases in pendingTestcaseQueue corrupted: (pushNewGeneratedTestcases)");
§§			Assert::AreEqual(poppedTestcase1.getparentId().m_localID, parentId1.m_localID, L"Error testcases in pendingTestcaseQueue corrupted: (pushNewGeneratedTestcases)");
§§			Assert::AreEqual(poppedTestcase1.getparentId().m_localID, parentId1.m_localID, L"Error testcases in pendingTestcaseQueue corrupted: (pushNewGeneratedTestcases)");
			Assert::AreEqual(poppedTestcase1.getNumOfProcessingAttempts(), oldNumOfProcessingAttempts + 1, L"Error testcases in pendingTestcaseQueue corrupted: (pushNewGeneratedTestcases)");

§§			int oldNumOfProcessingAttempts2 = testcase2.getNumOfProcessingAttempts();
§§			TestcaseDescriptor poppedTestcase2 = testcaseManager->popPendingTCForProcessing();
§§			Assert::AreEqual(poppedTestcase2.getId().m_localID, id2.m_localID, L"Error testcases in pendingTestcaseQueue corrupted: (pushNewGeneratedTestcases)");
§§			Assert::AreEqual(poppedTestcase2.getparentId().m_localID, parentId2.m_localID, L"Error testcases in pendingTestcaseQueue corrupted: (pushNewGeneratedTestcases)");
§§			Assert::AreEqual(poppedTestcase2.getparentId().m_localID, parentId2.m_localID, L"Error testcases in pendingTestcaseQueue corrupted: (pushNewGeneratedTestcases)");
§§			Assert::AreEqual(poppedTestcase2.getNumOfProcessingAttempts(), oldNumOfProcessingAttempts2 + 1, L"Error testcases in pendingTestcaseQueue corrupted: (pushNewGeneratedTestcases)");
§§
§§			// Proove Exception if Queue is empty
§§			TGTestcaseManager* testcaseManagerLocal = testcaseManager;
			auto f = [testcaseManagerLocal] { testcaseManagerLocal->popPendingTCForProcessing(); };
§§			Assert::ExpectException<std::runtime_error>(f);
§§		}
§§
		TEST_METHOD(TGTestcaseManager_popPendingTCForProcessing)
§§		{
§§			std::deque<TestcaseDescriptor> testcases = std::deque<TestcaseDescriptor>();
§§
§§			const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
§§			FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 222);
§§			FluffiTestcaseID parentId1 = FluffiTestcaseID(sd1, 333);
§§			std::string pathAndFileName1 = "testpath";
			TestcaseDescriptor testcase1 = TestcaseDescriptor(id1, parentId1, pathAndFileName1, true);
§§
§§			const FluffiServiceDescriptor sd2 = FluffiServiceDescriptor("testServiceDescriptor2", "testGuiID2");
§§			FluffiTestcaseID id2 = FluffiTestcaseID(sd2, 444);
§§			FluffiTestcaseID parentId2 = FluffiTestcaseID(sd2, 555);
§§			std::string pathAndFileName2 = "testpath";
			TestcaseDescriptor testcase2 = TestcaseDescriptor(id2, parentId2, pathAndFileName2, true);
§§
§§			testcases.push_back(testcase1);
§§			testcases.push_back(testcase2);
§§
§§			testcaseManager->pushNewGeneratedTestcases(testcases);
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)2, L"Error pushing TestcaseS into pendingTestcaseQueue: (pushNewGeneratedTestcases)");
§§
§§			int oldNumOfProcessingAttempts = testcase1.getNumOfProcessingAttempts();
§§
§§			// Test Method
§§			TestcaseDescriptor poppedTestcase1 = testcaseManager->popPendingTCForProcessing();
§§			Assert::AreEqual(poppedTestcase1.getId().m_localID, id1.m_localID, L"Error popping testcases from pendingTestcaseQueue: (popPendingTCForProcessing)");
§§			Assert::AreEqual(poppedTestcase1.getparentId().m_localID, parentId1.m_localID, L"Error popping testcases from pendingTestcaseQueue: (popPendingTCForProcessing)");
§§			Assert::AreEqual(poppedTestcase1.getparentId().m_localID, parentId1.m_localID, L"Error popping testcases from pendingTestcaseQueue: (popPendingTCForProcessing)");
§§			Assert::AreEqual(poppedTestcase1.getNumOfProcessingAttempts(), oldNumOfProcessingAttempts + 1, L"Error popping testcases from pendingTestcaseQueue: (popPendingTCForProcessing)");
§§
§§			int oldNumOfProcessingAttempts2 = testcase2.getNumOfProcessingAttempts();

§§			// Test Method
§§			TestcaseDescriptor poppedTestcase2 = testcaseManager->popPendingTCForProcessing();
§§			Assert::AreEqual(poppedTestcase2.getId().m_localID, id2.m_localID, L"Error popping testcases from pendingTestcaseQueue: (popPendingTCForProcessing)");
§§			Assert::AreEqual(poppedTestcase2.getparentId().m_localID, parentId2.m_localID, L"Error popping testcases from pendingTestcaseQueue: (popPendingTCForProcessing)");
§§			Assert::AreEqual(poppedTestcase2.getparentId().m_localID, parentId2.m_localID, L"Error popping testcases from pendingTestcaseQueue: (popPendingTCForProcessing)");
§§			Assert::AreEqual(poppedTestcase2.getNumOfProcessingAttempts(), oldNumOfProcessingAttempts2 + 1, L"Error popping testcases from pendingTestcaseQueue: (popPendingTCForProcessing)");
§§
§§			// Proove Exception if Queue is empty
§§			TGTestcaseManager* testcaseManagerLocal = testcaseManager;
			auto f = [testcaseManagerLocal] { testcaseManagerLocal->popPendingTCForProcessing(); };
§§			Assert::ExpectException<std::runtime_error>(f);
§§		}
§§
		TEST_METHOD(TGTestcaseManager_removeEvaluatedTestcases)
§§		{
§§			std::deque<TestcaseDescriptor> testcases = std::deque<TestcaseDescriptor>();
§§
§§			const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
§§			FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 222);
§§			FluffiTestcaseID parentId1 = FluffiTestcaseID(sd1, 333);
§§			std::string pathAndFileName1 = "testpath";
			TestcaseDescriptor testcase1 = TestcaseDescriptor(id1, parentId1, pathAndFileName1, true);
§§
§§			const FluffiServiceDescriptor sd2 = FluffiServiceDescriptor("testServiceDescriptor2", "testGuiID2");
§§			FluffiTestcaseID id2 = FluffiTestcaseID(sd2, 444);
§§			FluffiTestcaseID parentId2 = FluffiTestcaseID(sd2, 555);
§§			std::string pathAndFileName2 = "testpath";
			TestcaseDescriptor testcase2 = TestcaseDescriptor(id2, parentId2, pathAndFileName2, true);
§§
§§			testcases.push_back(testcase1);
§§			testcases.push_back(testcase2);
§§
§§			testcaseManager->pushNewGeneratedTestcases(testcases);
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)2, L"Error pushing TestcaseS into pendingTestcaseQueue: (removeEvaluatedTestcases)");

§§			testcaseManager->popPendingTCForProcessing();
§§			testcaseManager->popPendingTCForProcessing();
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error popping TestcaseS from pendingTestcaseQueue into sentButNotEvaluatedSet: (removeEvaluatedTestcases)");
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)2, L"Error popping TestcaseS from pendingTestcaseQueue into sentButNotEvaluatedSet: (removeEvaluatedTestcases)");
§§
§§			google::protobuf::RepeatedPtrField< TestcaseID > evaluatedTestcases = google::protobuf::RepeatedPtrField< TestcaseID >();
§§			TestcaseID* tmp1 = new TestcaseID();
			tmp1->CopyFrom(id1.getProtobuf());
			evaluatedTestcases.AddAllocated(tmp1);

§§			TestcaseID* tmp2 = new TestcaseID();
			tmp2->CopyFrom(id2.getProtobuf());
			evaluatedTestcases.AddAllocated(tmp2);
§§
§§			// Test Method
§§			testcaseManager->removeEvaluatedTestcases(evaluatedTestcases);
§§
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)0, L"Error removing evaluated TestcaseS from sentButNotEvaluatedTestcaseSet: (removeEvaluatedTestcases)");
§§		}
§§
		TEST_METHOD(TGTestcaseManager_getPendingTestcaseQueueSize)
§§		{
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error initializing pendingTestcaseQueue (pushNewGeneratedTestcase)");
§§
§§			const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
§§			FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 222);
§§			FluffiTestcaseID parentId1 = FluffiTestcaseID(sd1, 333);
§§			std::string pathAndFileName1 = "testpath";
			TestcaseDescriptor testcase1 = TestcaseDescriptor(id1, parentId1, pathAndFileName1, true);
§§
§§			testcaseManager->pushNewGeneratedTestcase(testcase1);
§§
§§			const FluffiServiceDescriptor sd2 = FluffiServiceDescriptor("testServiceDescriptor2", "testGuiID2");
§§			FluffiTestcaseID id2 = FluffiTestcaseID(sd2, 444);
§§			FluffiTestcaseID parentId2 = FluffiTestcaseID(sd2, 555);
§§			std::string pathAndFileName2 = "testpath";
			TestcaseDescriptor testcase2 = TestcaseDescriptor(id2, parentId2, pathAndFileName2, true);
§§
§§			testcaseManager->pushNewGeneratedTestcase(testcase2);
§§
§§			// Test Method
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)2, L"Error getting Queue size of pendingTestcaseQueue: (getPendingTestcaseQueueSize)");
§§		}
§§
		TEST_METHOD(TGTestcaseManager_getSentButNotEvaluatedTestcaseSetSize)
§§		{
§§			std::deque<TestcaseDescriptor> testcases = std::deque<TestcaseDescriptor>();
§§
§§			const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
§§			FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 222);
§§			FluffiTestcaseID parentId1 = FluffiTestcaseID(sd1, 333);
§§			std::string pathAndFileName1 = "testpath";
			TestcaseDescriptor testcase1 = TestcaseDescriptor(id1, parentId1, pathAndFileName1, true);
§§
§§			const FluffiServiceDescriptor sd2 = FluffiServiceDescriptor("testServiceDescriptor2", "testGuiID2");
§§			FluffiTestcaseID id2 = FluffiTestcaseID(sd2, 444);
§§			FluffiTestcaseID parentId2 = FluffiTestcaseID(sd2, 555);
§§			std::string pathAndFileName2 = "testpath";
			TestcaseDescriptor testcase2 = TestcaseDescriptor(id2, parentId2, pathAndFileName2, true);
§§
§§			const FluffiServiceDescriptor sd3 = FluffiServiceDescriptor("testServiceDescriptor3", "testGuiID3");
§§			FluffiTestcaseID id3 = FluffiTestcaseID(sd3, 666);
§§			FluffiTestcaseID parentId3 = FluffiTestcaseID(sd3, 777);
§§			std::string pathAndFileName3 = "testpath";
			TestcaseDescriptor testcase3 = TestcaseDescriptor(id3, parentId3, pathAndFileName3, true);
§§
§§			testcases.push_back(testcase1);
§§			testcases.push_back(testcase2);
§§			testcases.push_back(testcase3);
§§
§§			testcaseManager->pushNewGeneratedTestcases(testcases);
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)3, L"Error pushing TestcaseS into pendingTestcaseQueue: (getSentButNotEvaluatedTestcaseSetSize)");
§§
§§			testcaseManager->popPendingTCForProcessing();
§§			testcaseManager->popPendingTCForProcessing();
§§			testcaseManager->popPendingTCForProcessing();
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error popping TestcaseS from pendingTestcaseQueue into sentButNotEvaluatedSet: (getSentButNotEvaluatedTestcaseSetSize)");
§§
§§			// Test Method
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)3, L"Error calculating size of sentButNotEvaluatedTestcaseSet: (getSentButNotEvaluatedTestcaseSetSize)");
§§
§§			google::protobuf::RepeatedPtrField< TestcaseID > evaluatedTestcases = google::protobuf::RepeatedPtrField< TestcaseID >();
§§			TestcaseID* tmp2 = new TestcaseID();
			tmp2->CopyFrom(id2.getProtobuf());
			evaluatedTestcases.AddAllocated(tmp2);
§§
§§			Assert::AreEqual(evaluatedTestcases.size(), 1, L"Error calculating size of evaluatedTestcases: (getSentButNotEvaluatedTestcaseSetSize)");
§§
§§			testcaseManager->removeEvaluatedTestcases(evaluatedTestcases);
§§
§§			// Test Method
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)2, L"Error calculating size of sentButNotEvaluatedTestcaseSet or Error in removeEvaluatedTestcases: (getSentButNotEvaluatedTestcaseSetSize)");
§§		}
§§
		TEST_METHOD(TGTestcaseManager_reinsertTestcasesHavingNoAnswerSince)
§§		{
§§			std::deque<TestcaseDescriptor> testcases = std::deque<TestcaseDescriptor>();
§§
§§			const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
§§			FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 222);
§§			FluffiTestcaseID parentId1 = FluffiTestcaseID(sd1, 333);
§§			std::string pathAndFileName1 = "testpath";
			TestcaseDescriptor testcase1 = TestcaseDescriptor(id1, parentId1, pathAndFileName1, true);
§§
§§			const FluffiServiceDescriptor sd2 = FluffiServiceDescriptor("testServiceDescriptor2", "testGuiID2");
§§			FluffiTestcaseID id2 = FluffiTestcaseID(sd2, 444);
§§			FluffiTestcaseID parentId2 = FluffiTestcaseID(sd2, 555);
§§			std::string pathAndFileName2 = "testpath";
			TestcaseDescriptor testcase2 = TestcaseDescriptor(id2, parentId2, pathAndFileName2, true);
§§
§§			testcases.push_back(testcase1);
§§			testcases.push_back(testcase2);
§§
§§			testcaseManager->pushNewGeneratedTestcases(testcases);
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)2, L"Error pushing TestcaseS into pendingTestcaseQueue: (reinsertTestcasesHavingNoAnswerSince)");
§§
§§			testcaseManager->popPendingTCForProcessing();
§§			testcaseManager->popPendingTCForProcessing();
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error popping TestcaseS from pendingTestcaseQueue into sentButNotEvaluatedSet: (reinsertTestcasesHavingNoAnswerSince)");
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)2, L"Error popping TestcaseS from pendingTestcaseQueue into sentButNotEvaluatedSet: (reinsertTestcasesHavingNoAnswerSince)");

			std::chrono::time_point<std::chrono::steady_clock> timeBeforeWhichShouldBeReinserted = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);

§§			// Test Method
§§			testcaseManager->reinsertTestcasesHavingNoAnswerSince(timeBeforeWhichShouldBeReinserted);
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)2, L"Error reinserting TestcaseS from sentButNotEvaluatedSet due to no answer since..: (reinsertTestcasesHavingNoAnswerSince)");
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)0, L"Error reinserting TestcaseS from sentButNotEvaluatedSet due to no answer since..: (reinsertTestcasesHavingNoAnswerSince)");
§§		}
§§
		TEST_METHOD(TGTestcaseManager_handMeAllTestcasesWithTooManyRetries)
§§		{
§§			std::deque<TestcaseDescriptor> testcases = std::deque<TestcaseDescriptor>();
§§
§§			const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
§§			FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 222);
§§			FluffiTestcaseID parentId1 = FluffiTestcaseID(sd1, 333);
§§			std::string pathAndFileName1 = "testpath";
			TestcaseDescriptor testcase1 = TestcaseDescriptor(id1, parentId1, pathAndFileName1, true);
§§
§§			const FluffiServiceDescriptor sd2 = FluffiServiceDescriptor("testServiceDescriptor2", "testGuiID2");
§§			FluffiTestcaseID id2 = FluffiTestcaseID(sd2, 444);
§§			FluffiTestcaseID parentId2 = FluffiTestcaseID(sd2, 555);
§§			std::string pathAndFileName2 = "testpath";
			TestcaseDescriptor testcase2 = TestcaseDescriptor(id2, parentId2, pathAndFileName2, true);
§§
§§			testcases.push_back(testcase1);
§§			testcases.push_back(testcase2);
§§
§§			testcaseManager->pushNewGeneratedTestcases(testcases);
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)2, L"Error pushing TestcaseS into pendingTestcaseQueue: (handMeAllTestcasesWithTooManyRetries)");
§§
§§			testcaseManager->popPendingTCForProcessing();
§§			testcaseManager->popPendingTCForProcessing();
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error popping TestcaseS from pendingTestcaseQueue into sentButNotEvaluatedSet: (handMeAllTestcasesWithTooManyRetries)");
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)2, L"Error popping TestcaseS from pendingTestcaseQueue into sentButNotEvaluatedSet: (handMeAllTestcasesWithTooManyRetries)");
§§
			std::chrono::time_point<std::chrono::steady_clock> timeBeforeWhichShouldBeReinserted = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
§§
§§			testcaseManager->reinsertTestcasesHavingNoAnswerSince(timeBeforeWhichShouldBeReinserted);
§§
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)2, L"Error reinserting TestcaseS from sentButNotEvaluatedSet due to no answer since..: (handMeAllTestcasesWithTooManyRetries)");
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)0, L"Error reinserting TestcaseS from sentButNotEvaluatedSet due to no answer since..: (handMeAllTestcasesWithTooManyRetries)");
§§
§§			testcaseManager->popPendingTCForProcessing();
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)1, L"Error popping TestcaseS from pendingTestcaseQueue into sentButNotEvaluatedSet: (handMeAllTestcasesWithTooManyRetries)");
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)1, L"Error popping TestcaseS from pendingTestcaseQueue into sentButNotEvaluatedSet: (handMeAllTestcasesWithTooManyRetries)");
			timeBeforeWhichShouldBeReinserted = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
§§			testcaseManager->reinsertTestcasesHavingNoAnswerSince(timeBeforeWhichShouldBeReinserted);
§§
§§			testcaseManager->popPendingTCForProcessing();
§§			testcaseManager->popPendingTCForProcessing();
§§
§§			// Test Method
§§			std::vector<TestcaseDescriptor> testcasesThatNeedToBeReported = testcaseManager->handMeAllTestcasesWithTooManyRetries(4);
§§			Assert::AreEqual(testcasesThatNeedToBeReported.size(), (size_t)0, L"Error calculating size of handed out vector of testcases with too many retires..: (handMeAllTestcasesWithTooManyRetries)");
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error handing out all testcases that need to be reported due to too many send retries..: (handMeAllTestcasesWithTooManyRetries)");
§§
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)2, L"Error handing out all testcases that need to be reported due to too many send retries..: (handMeAllTestcasesWithTooManyRetries)");
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error handing out all testcases that need to be reported due to too many send retries..: (handMeAllTestcasesWithTooManyRetries)");
§§
§§			// Test Method
§§			std::vector<TestcaseDescriptor> testcasesThatNeedToBeReported2 = testcaseManager->handMeAllTestcasesWithTooManyRetries(3);
§§			Assert::AreEqual(testcasesThatNeedToBeReported2.size(), (size_t)1, L"Error calculating size of handed out vector of testcases with too many retires..: (handMeAllTestcasesWithTooManyRetries)");
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)1, L"Error handing out all testcases that need to be reported due to too many send retries..: (handMeAllTestcasesWithTooManyRetries)");
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error handing out all testcases that need to be reported due to too many send retries..: (handMeAllTestcasesWithTooManyRetries)");
§§
§§			// Test Method
§§			std::vector<TestcaseDescriptor> testcasesThatNeedToBeReported3 = testcaseManager->handMeAllTestcasesWithTooManyRetries(2);
§§			Assert::AreEqual(testcasesThatNeedToBeReported3.size(), (size_t)1, L"Error calculating size of handed out vector of testcases with too many retires..: (handMeAllTestcasesWithTooManyRetries)");
§§			Assert::AreEqual(testcaseManager->getSentButNotEvaluatedTestcaseSetSize(), (size_t)0, L"Error handing out all testcases that need to be reported due to too many send retries..: (handMeAllTestcasesWithTooManyRetries)");
§§			Assert::AreEqual(testcaseManager->getPendingTestcaseQueueSize(), (size_t)0, L"Error handing out all testcases that need to be reported due to too many send retries..: (handMeAllTestcasesWithTooManyRetries)");
§§		}
§§	};
}
