/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Pascal Eckmann
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "LMDatabaseManager.h"
#include "FluffiServiceDescriptor.h"
#include "FluffiTestcaseID.h"
#include "BlockCoverageCache.h"
#include "FluffiSetting.h"
#include "FluffiBasicBlock.h"
#include "FluffiModuleNameToID.h"
#include "StatusOfInstance.h"
#include "GarbageCollectorWorker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LMDatabaseManagerTester
{
	TEST_CLASS(LMDatabaseManagerTest)
	{
	public:

		LMDatabaseManager* dbman = nullptr;
		GarbageCollectorWorker* garbageCollector = nullptr;

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
			garbageCollector = new GarbageCollectorWorker(200);
			dbman = setupTestDB(garbageCollector);
		}

		TEST_METHOD_CLEANUP(ModuleCleanup)
		{
			// Freeing the databaseManager
			delete dbman;
			dbman = nullptr;

			//mysql_library_end();

			// Freeing the garbageCollector
			delete garbageCollector;
			garbageCollector = nullptr;
		}

		static void writeManagedInstance_multithread(int i) {
			GarbageCollectorWorker gc(200);
			LMDatabaseManager local_lmdb(&gc);

			FluffiServiceDescriptor sd("HAP-" + std::to_string(i), Util::newGUID());
			Assert::IsTrue(local_lmdb.writeManagedInstance(sd, 1, "subtest", "testloc"), L"Multithreaded insert failed");
		}

		TEST_METHOD(LMDatabaseManager_writeManagedInstance)
		{
			std::string testGUID = "testguid";
			std::string testHAP = "testhap";
			std::string testStatus = "teststatus";
			std::string testlocation = "testlocation";
			std::string testsubtype = "testsubtype";

			FluffiServiceDescriptor sd{ testHAP ,testGUID };
			Assert::IsTrue(dbman->writeManagedInstance(sd, AgentType::TestcaseEvaluator, testsubtype, testlocation));

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorGUID from managed_instances") == testGUID);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorHostAndPort from managed_instances") == testHAP);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT AgentType from managed_instances")) == AgentType::TestcaseEvaluator);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Location from managed_instances") == testlocation);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT AgentSubType from managed_instances") == testsubtype);

			//Testing nice names
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT NiceName from nice_names_managed_instance WHERE ServiceDescriptorGUID = 'testguid'") == testsubtype + "0");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ID from nice_names_managed_instance WHERE ServiceDescriptorGUID = 'testguid'") == "1");

			//Testing overwrite
			Assert::IsTrue(dbman->writeManagedInstance(sd, AgentType::TestcaseGenerator, testsubtype + "2", testlocation + "2")); //duplicate insert should overwrite
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Location from managed_instances") == testlocation + "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorGUID from managed_instances") == testGUID);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorHostAndPort from managed_instances") == testHAP);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT AgentType from managed_instances")) == AgentType::TestcaseGenerator);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT AgentSubType from managed_instances") == testsubtype + "2");

			//Nice name id should not change
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ID from nice_names_managed_instance WHERE ServiceDescriptorGUID = 'testguid'") == "1");

			//Test multithreading
			for (int j = 0; j < 100; j++) {
				std::vector<std::thread> threads;
				for (int i = 0; i < 10; i++)
				{
					//create threads
					threads.push_back(std::thread(writeManagedInstance_multithread, i));
				}
				//wait for them to complete
				for (auto& th : threads)
					th.join();
				dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE managed_instances");
			}
		}

		TEST_METHOD(LMDatabaseManager_addNewManagedInstanceStatus)
		{
			std::string testGUID = "testguid";
			std::string testHAP = "testhap";
			std::string testStatus = "teststatus";
			std::string testsubtype = "testsubtype";

			FluffiServiceDescriptor sd{ testHAP ,testGUID };
			dbman->writeManagedInstance(sd, AgentType::TestcaseEvaluator, testsubtype, "testlocation");

			Assert::IsTrue(dbman->addNewManagedInstanceStatus(testGUID, testStatus));

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "1");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses") == testStatus);

			//Assert timestamp is utc
			std::string dbtimestamp = dbman->EXECUTE_TEST_STATEMENT("SELECT TimeOfStatus from managed_instances_statuses");
			struct tm result;
			strptime(dbtimestamp.c_str(), "%Y-%m-%d %H:%M:%S", &result);
			long t = (long)time(NULL);
			Assert::IsTrue(abs(t - _mkgmtime(&result)) < 5 * 60);
		}

		TEST_METHOD(LMDatabaseManager_getRegisteredInstancesOfAgentType)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			std::string guid2 = "guid2";
			std::string hap2 = "hap2";
			FluffiServiceDescriptor sd2{ hap2 ,guid2 };

			std::string guid3 = "guid3";
			std::string hap3 = "hap3";
			FluffiServiceDescriptor sd3{ hap3 ,guid3 };

			std::string guid4 = "guid4";
			std::string hap4 = "hap4";
			FluffiServiceDescriptor sd4{ hap4 ,guid4 };

			std::string loc1 = "testlocation";
			std::string loc2 = "testlocation2";
			std::string testsubtype = "testsubtype";

			dbman->writeManagedInstance(sd1, AgentType::TestcaseEvaluator, testsubtype, loc1);
			dbman->writeManagedInstance(sd2, AgentType::TestcaseGenerator, testsubtype, loc1);
			dbman->writeManagedInstance(sd3, AgentType::TestcaseEvaluator, testsubtype, loc1);
			dbman->writeManagedInstance(sd4, AgentType::TestcaseEvaluator, testsubtype, loc2);

			std::vector<FluffiServiceDescriptor> resp = dbman->getRegisteredInstancesOfAgentType(AgentType::TestcaseEvaluator, loc1);
			Assert::IsTrue(resp.size() == 2);

			resp = dbman->getRegisteredInstancesOfAgentType(AgentType::TestcaseGenerator, loc1);
			Assert::IsTrue(resp.size() == 1);
			Assert::IsTrue(resp[0].m_guid == guid2);
			Assert::IsTrue(resp[0].m_serviceHostAndPort == hap2);

			resp = dbman->getRegisteredInstancesOfAgentType(AgentType::TestcaseEvaluator, loc2);
			Assert::IsTrue(resp.size() == 1);

			resp = dbman->getRegisteredInstancesOfAgentType(AgentType::TestcaseGenerator, loc2);
			Assert::IsTrue(resp.size() == 0);
		}

		TEST_METHOD(LMDatabaseManager_getAllRegisteredInstances)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			std::string guid2 = "guid2";
			std::string hap2 = "hap2";
			FluffiServiceDescriptor sd2{ hap2 ,guid2 };

			std::string guid3 = "guid3";
			std::string hap3 = "hap3";
			FluffiServiceDescriptor sd3{ hap3 ,guid3 };

			std::string guid4 = "guid4";
			std::string hap4 = "hap4";
			FluffiServiceDescriptor sd4{ hap4 ,guid4 };

			std::string loc1 = "testlocation";
			std::string loc2 = "testlocation2";
			std::string testsubtype = "testsubtype";

			dbman->writeManagedInstance(sd1, AgentType::TestcaseEvaluator, testsubtype, loc1);
			dbman->writeManagedInstance(sd2, AgentType::TestcaseGenerator, testsubtype, loc1);
			dbman->writeManagedInstance(sd3, AgentType::LocalManager, testsubtype, loc1);
			dbman->writeManagedInstance(sd4, AgentType::TestcaseEvaluator, testsubtype, loc2);

			std::vector<std::pair<FluffiServiceDescriptor, AgentType>> resp = dbman->getAllRegisteredInstances(loc1);
			Assert::IsTrue(resp.size() == 3);

			resp = dbman->getAllRegisteredInstances(loc2);
			Assert::IsTrue(resp.size() == 1);
			Assert::IsTrue(resp[0].first.m_guid == guid4);
			Assert::IsTrue(resp[0].first.m_serviceHostAndPort == hap4);
			Assert::IsTrue(resp[0].second == AgentType::TestcaseEvaluator);
		}

		TEST_METHOD(LMDatabaseManager_removeManagedInstance)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			std::string guid2 = "guid2";
			std::string hap2 = "hap2";
			FluffiServiceDescriptor sd2{ hap2 ,guid2 };

			std::string guid3 = "guid3";
			std::string hap3 = "hap3";
			FluffiServiceDescriptor sd3{ hap3 ,guid3 };

			std::string guid4 = "guid4";
			std::string hap4 = "hap4";
			FluffiServiceDescriptor sd4{ hap4 ,guid4 };

			std::string loc1 = "testlocation";
			std::string loc2 = "testlocation2";
			std::string testsubtype = "testsubtype";

			dbman->writeManagedInstance(sd1, AgentType::TestcaseEvaluator, testsubtype, loc1);
			dbman->writeManagedInstance(sd2, AgentType::TestcaseGenerator, testsubtype, loc1);
			dbman->writeManagedInstance(sd3, AgentType::LocalManager, testsubtype, loc1);
			dbman->writeManagedInstance(sd4, AgentType::TestcaseEvaluator, testsubtype, loc2);

			Assert::IsTrue(dbman->removeManagedInstance(guid1, loc1));

			Assert::IsTrue(dbman->removeManagedInstance(guid1, loc1));

			Assert::IsTrue(dbman->removeManagedInstance(guid1, loc2));

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances WHERE ServiceDescriptorGUID = \"" + guid2 + "\" AND Location = \"" + loc1 + "\"") == "1");
			dbman->removeManagedInstance(guid2, loc1);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances WHERE ServiceDescriptorGUID = \"" + guid2 + "\" AND Location = \"" + loc1 + "\"") == "0");
		}

		TEST_METHOD(LMDatabaseManager_deleteManagedInstanceStatusOlderThanXSec)
		{
			std::string testGUID = "testguid";
			std::string testHAP = "testhap";
			std::string testStatus = "teststatus";
			std::string testStatus2 = "teststatus2";
			std::string testsubtype = "testsubtype";

			FluffiServiceDescriptor sd{ testHAP ,testGUID };
			dbman->writeManagedInstance(sd, AgentType::TestcaseEvaluator, testsubtype, "testlocation");

			dbman->addNewManagedInstanceStatus(testGUID, testStatus);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "1");
			Sleep(2000);
			dbman->addNewManagedInstanceStatus(testGUID, testStatus2);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "2");

			Assert::IsTrue(dbman->deleteManagedInstanceStatusOlderThanXSec(1));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "1");
		}

		TEST_METHOD(LMDatabaseManager_getRegisteredInstanceSubType) {
			std::string testGUID = "testguid";
			std::string testHAP = "testhap";
			std::string testsubtype = "testsubtype";

			FluffiServiceDescriptor sd{ testHAP ,testGUID };
			dbman->writeManagedInstance(sd, AgentType::TestcaseEvaluator, testsubtype, "testlocation");

			Assert::IsTrue(dbman->getRegisteredInstanceSubType(testGUID) == testsubtype, L"The expected agentsubtype was not received");
		}

		TEST_METHOD(LMDatabaseManager_getStatusOfManagedInstances)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			std::string guid2 = "guid2";
			std::string hap2 = "hap2";
			FluffiServiceDescriptor sd2{ hap2 ,guid2 };

			std::string guid3 = "guid3";
			std::string hap3 = "hap3";
			FluffiServiceDescriptor sd3{ hap3 ,guid3 };

			std::string loc1 = "testlocation";
			std::string loc2 = "testlocation2";
			std::string testsubtype = "testsubtype";

			dbman->writeManagedInstance(sd1, AgentType::TestcaseEvaluator, testsubtype, loc1);
			dbman->addNewManagedInstanceStatus(guid1, "1 1 |");
			Sleep(2000);
			dbman->addNewManagedInstanceStatus(guid1, "1 2 |");

			dbman->writeManagedInstance(sd2, AgentType::TestcaseGenerator, testsubtype, loc1);
			dbman->addNewManagedInstanceStatus(guid2, "2 1 |");

			dbman->writeManagedInstance(sd3, AgentType::TestcaseEvaluator, testsubtype, loc2);
			dbman->addNewManagedInstanceStatus(guid3, "3 1 | 4 2 |");
			Sleep(2000);
			dbman->addNewManagedInstanceStatus(guid3, "3 2 |");

			std::vector<StatusOfInstance> res = dbman->getStatusOfManagedInstances(loc1);
			Assert::IsTrue(res.size() == 2);

			res = dbman->getStatusOfManagedInstances(loc2);
			Assert::IsTrue(res.size() == 1);
			Assert::IsTrue(res[0].serviceDescriptorGUID == guid3);
			Assert::IsTrue(res[0].serviceDescriptorHostAndPort == hap3);
			Assert::IsTrue(res[0].status["3"] == "2");
			Assert::IsTrue(res[0].type == AgentType::TestcaseEvaluator);

			//enforce utc
			long t = (long)time(NULL);
			Assert::IsTrue(abs(t - res[0].lastStatusUpdate) < 5 * 60);
		}

		TEST_METHOD(LMDatabaseManager_addBlocksToCoveredBlocks)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			std::string guid2 = "guid2";
			std::string hap2 = "hap2";
			FluffiServiceDescriptor sd2{ hap2 ,guid2 };

			uint64_t localid1 = 1;
			FluffiTestcaseID ftid1{ sd1,localid1 };

			uint64_t localid2 = 2;
			FluffiTestcaseID ftid2{ sd2,localid2 };

			std::set<FluffiBasicBlock> blocks;
			blocks.insert(FluffiBasicBlock(22, 11));
			blocks.insert(FluffiBasicBlock(44, 33));

			//must fail before testcase is inserted into interesting testcases table
			Assert::IsFalse(dbman->addBlocksToCoveredBlocks(ftid1, blocks));

			dbman->addEntryToInterestingTestcasesTable(ftid1, ftid1, 0, "", LMDatabaseManager::TestCaseType::Population);
			dbman->addEntryToInterestingTestcasesTable(ftid2, ftid2, 0, "", LMDatabaseManager::TestCaseType::Population);

			//Must fail as target module are not set it
			Assert::IsFalse(dbman->addBlocksToCoveredBlocks(ftid1, blocks));

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ID, ModuleName) VALUES (11, 'test1.dll')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ID, ModuleName) VALUES (33, 'test2.dll')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ID, ModuleName) VALUES (55, 'test3.dll')");

			//Now it should work
			Assert::IsTrue(dbman->addBlocksToCoveredBlocks(ftid1, blocks));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from covered_blocks") == "2");

			//Duplicate insert should merely overwrite the timestamp
			Assert::IsTrue(dbman->addBlocksToCoveredBlocks(ftid1, blocks));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from covered_blocks") == "2");

			blocks.insert(FluffiBasicBlock(66, 55));

			Assert::IsTrue(dbman->addBlocksToCoveredBlocks(ftid2, blocks));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from covered_blocks") == "5");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorTestcaseID from covered_blocks WHERE Offset = 66") == dbman->EXECUTE_TEST_STATEMENT("SELECT ID from interesting_testcases WHERE CreatorServiceDescriptorGUID = '" + guid2 + "'"));
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT ModuleID from covered_blocks WHERE Offset = 66")) == 55);

			//Assert timestamp is utc
			std::string dbtimestamp = dbman->EXECUTE_TEST_STATEMENT("SELECT TimeOfInsertion from covered_blocks WHERE Offset = 66");
			struct tm result;
			strptime(dbtimestamp.c_str(), "%Y-%m-%d %H:%M:%S", &result);
			long t = (long)time(NULL);
			Assert::IsTrue(abs(t - _mkgmtime(&result)) < 5 * 60);

			//Transaction isolation should not change
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT @@TX_ISOLATION;") == "REPEATABLE-READ");
			Assert::IsTrue(dbman->addBlocksToCoveredBlocks(ftid1, blocks));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT @@TX_ISOLATION;") == "REPEATABLE-READ");

			//64 bit rvas should be possible
			std::string guid3 = "guid3";
			std::string hap3 = "hap3";
			FluffiServiceDescriptor sd3{ hap3 ,guid3 };

			uint64_t localid3 = 3;
			FluffiTestcaseID ftid3{ sd3,localid3 };
			dbman->addEntryToInterestingTestcasesTable(ftid3, ftid3, 0, "", LMDatabaseManager::TestCaseType::Population);

			std::set<FluffiBasicBlock> blocks2;
			blocks2.insert(FluffiBasicBlock(0x12345678ab, 11));

			Assert::IsTrue(dbman->addBlocksToCoveredBlocks(ftid3, blocks2));
			Assert::IsTrue(stoull(dbman->EXECUTE_TEST_STATEMENT("SELECT Offset from covered_blocks WHERE CreatorTestcaseID = (SELECT ID from interesting_testcases WHERE CreatorServiceDescriptorGUID ='guid3') LIMIT 1")) == 0x12345678ab);
		}

		TEST_METHOD(LMDatabaseManager_addDeltaToTestcaseRating)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			uint64_t localid1 = 1;
			FluffiTestcaseID ftid1{ sd1,localid1 };

			dbman->addEntryToInterestingTestcasesTable(ftid1, ftid1, 0, "", LMDatabaseManager::TestCaseType::Population);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Rating from interesting_testcases")) == 0);

			Assert::IsTrue(dbman->addDeltaToTestcaseRating(ftid1, 10));
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Rating from interesting_testcases")) == 10);

			Assert::IsTrue(dbman->addDeltaToTestcaseRating(ftid1, -20));
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Rating from interesting_testcases")) == -10);
		}

		TEST_METHOD(LMDatabaseManager_addEntryToCompletedTestcasesTable)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			uint64_t localid1 = 1;
			FluffiTestcaseID ftid1{ sd1,localid1 };

			Assert::IsTrue(dbman->addEntryToCompletedTestcasesTable(ftid1));

			//duplicate inserts should be possible (fault tolerance)
			Assert::IsTrue(dbman->addEntryToCompletedTestcasesTable(ftid1));

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorServiceDescriptorGUID from completed_testcases") == guid1);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorLocalID from completed_testcases")) == localid1);

			//Assert timestamp is utc
			std::string dbtimestamp = dbman->EXECUTE_TEST_STATEMENT("SELECT TimeOfCompletion from completed_testcases");
			struct tm result;
			strptime(dbtimestamp.c_str(), "%Y-%m-%d %H:%M:%S", &result);
			long t = (long)time(NULL);
			Assert::IsTrue(abs(t - _mkgmtime(&result)) < 5 * 60);
		}

		static void addEntryToCrashDescriptionsTable_multithread(int i) {
			GarbageCollectorWorker gc(200);
			LMDatabaseManager local_lmdb(&gc);

			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };
			FluffiTestcaseID tid(sd1, i);
			Assert::IsTrue(local_lmdb.addEntryToInterestingTestcasesTable(tid, tid, 0, "", LMDatabaseManager::TestCaseType::Population), L"Multithreaded insert failed (1)");

			Assert::IsTrue(local_lmdb.addEntryToCrashDescriptionsTable(tid, std::to_string(rand() % 20)), L"Multithreaded insert failed (2)");
		}

		TEST_METHOD(LMDatabaseManager_addEntryToCrashDescriptionsTable)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			uint64_t localid1 = 1;
			FluffiTestcaseID ftid1{ sd1,localid1 };

			std::string footprint = "asdf";
			std::string footprint2 = "asdf2";

			//Insert without foreign key constraint should fail
			Assert::IsFalse(dbman->addEntryToCrashDescriptionsTable(ftid1, footprint));

			dbman->addEntryToInterestingTestcasesTable(ftid1, ftid1, 0, "", LMDatabaseManager::TestCaseType::Population);

			//Now it should work
			Assert::IsTrue(dbman->addEntryToCrashDescriptionsTable(ftid1, footprint));

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorTestcaseID from crash_descriptions") == dbman->EXECUTE_TEST_STATEMENT("SELECT ID from interesting_testcases WHERE CreatorServiceDescriptorGUID = '" + guid1 + "'"));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT CrashFootprint from crash_descriptions") == footprint);

			//duplicate inserts should result in an overwrite
			Assert::IsTrue(dbman->addEntryToCrashDescriptionsTable(ftid1, footprint2));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT CrashFootprint from crash_descriptions") == footprint2);

			//Test multithreading
			for (int j = 0; j < 100; j++) {
				std::vector<std::thread> threads;
				for (int i = 0; i < 10; i++)
				{
					//create threads
					threads.push_back(std::thread(addEntryToCrashDescriptionsTable_multithread, i));
				}
				//wait for them to complete
				for (auto& th : threads)
					th.join();
				dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE crash_descriptions");
			}
		}

		TEST_METHOD(LMDatabaseManager_addEntryToInterestingTestcasesTable)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			uint64_t localid1 = 1;
			FluffiTestcaseID ftid1{ sd1,localid1 };

			std::string guid2 = "guid2";
			std::string hap2 = "hap2";
			FluffiServiceDescriptor sd2{ hap2 ,guid2 };

			uint64_t localid2 = 2;
			FluffiTestcaseID ftid2{ sd2,localid2 };

			std::string testfile = Util::generateTestcasePathAndFilename(ftid1, ".");
			std::ofstream fout;
			fout.open(testfile, std::ios::binary | std::ios::out);
			for (int i = 0; i < 256; i++) {
				fout.write((char*)&i, 1);
			}
			fout.close();

			Assert::IsTrue(dbman->addEntryToInterestingTestcasesTable(ftid1, ftid1, 10, ".", LMDatabaseManager::TestCaseType::Population));
			Assert::IsTrue(dbman->addEntryToInterestingTestcasesTable(ftid2, ftid1, 20, ".", LMDatabaseManager::TestCaseType::Hang));

			garbageCollector->collectGarbage();

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorServiceDescriptorGUID from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid1 + "\"") == guid1);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorLocalID from interesting_testcases WHERE CreatorServiceDescriptorGUID= \"" + guid1 + "\"")) == localid1);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ParentServiceDescriptorGUID from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid1 + "\"") == guid1);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT ParentLocalID from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid1 + "\"")) == localid1);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Rating from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid1 + "\"")) == 10);
			Assert::IsTrue("000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF" == dbman->EXECUTE_TEST_STATEMENT("SELECT HEX(RawBytes) from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid1 + "\""));
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT TestCaseType from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid1 + "\"")) == LMDatabaseManager::TestCaseType::Population);

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorServiceDescriptorGUID from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\"") == guid2);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorLocalID from interesting_testcases WHERE CreatorServiceDescriptorGUID= \"" + guid2 + "\"")) == localid2);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ParentServiceDescriptorGUID from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\"") == guid1);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT ParentLocalID from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\"")) == localid1);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Rating from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\"")) == 20);
			Assert::IsTrue("" == dbman->EXECUTE_TEST_STATEMENT("SELECT HEX(RawBytes) from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\""));
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT TestCaseType from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\"")) == LMDatabaseManager::TestCaseType::Hang);

			//check if testfile still exists (it should be deleted)
			std::ifstream f(testfile);
			Assert::IsFalse(f.good());

			//duplicate insert should result in an update of type and delete dependants such as worst case scenarios and coverage
			Assert::IsTrue(dbman->addEntryToCrashDescriptionsTable(ftid2, "here a crash fp"));
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Count(*) from crash_descriptions")) == 1);
			std::set<FluffiBasicBlock> blocks;
			blocks.insert(FluffiBasicBlock(22, 11));
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ID, ModuleName) VALUES (11, 'test1.dll')");
			Assert::IsTrue(dbman->addBlocksToCoveredBlocks(ftid2, blocks));
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Count(*) from covered_blocks")) == 1);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Count(*) from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\"")) == 1);
			Assert::IsTrue(dbman->addEntryToInterestingTestcasesTable(ftid2, ftid1, 200, "", LMDatabaseManager::TestCaseType::Exception_AccessViolation));
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Count(*) from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\"")) == 1);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Rating from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\"")) == 200);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT TestCaseType from interesting_testcases WHERE CreatorServiceDescriptorGUID = \"" + guid2 + "\"")) == LMDatabaseManager::TestCaseType::Exception_AccessViolation);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Count(*) from crash_descriptions")) == 0);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Count(*) from covered_blocks")) == 0);
		}

		TEST_METHOD(LMDatabaseManager_generateGetCurrentBlockCoverageResponse)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			std::string guid2 = "guid2";
			std::string hap2 = "hap2";
			FluffiServiceDescriptor sd2{ hap2 ,guid2 };

			uint64_t localid1 = 1;
			FluffiTestcaseID ftid1{ sd1,localid1 };

			uint64_t localid2 = 2;
			FluffiTestcaseID ftid2{ sd2,localid2 };

			std::set<FluffiBasicBlock> blocks;
			blocks.insert(FluffiBasicBlock(22, 11));
			blocks.insert(FluffiBasicBlock(44, 33));
			blocks.insert(FluffiBasicBlock(0x12345678ab, 55));

			dbman->addEntryToInterestingTestcasesTable(ftid1, ftid1, 0, "", LMDatabaseManager::TestCaseType::Population);
			dbman->addEntryToInterestingTestcasesTable(ftid2, ftid2, 0, "", LMDatabaseManager::TestCaseType::Population);

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ID, ModuleName) VALUES (11, 'test1.dll')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ID, ModuleName) VALUES (33, 'test2.dll')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ID, ModuleName) VALUES (55, 'test3.dll')");

			dbman->addBlocksToCoveredBlocks(ftid1, blocks);

			GetCurrentBlockCoverageResponse* resp = dbman->generateGetCurrentBlockCoverageResponse();

			//compare result with input
			//1) Assert same size
			Assert::IsTrue(blocks.size() == resp->blocks().size(),
				L"Size of blocks sent to addBlocksToCoveredBlocks does not match generateGetCurrentBlockCoverageResponse");
			Assert::IsTrue(blocks.size() == 3,
				L"Size of blocks in database not equal to expected size");

			//2) Assert same content
			BlockCoverageCache bcache1;
			bcache1.addBlocksToCache(&blocks); //insert the blocks we put in the database

			for (auto it = resp->blocks().begin(); it != resp->blocks().end(); it++) {
				FluffiBasicBlock fbb(it->rva(), it->moduleid());
				Assert::IsTrue(bcache1.isBlockInCache(fbb));
			}

			//3) Check content manually
			Assert::IsTrue(FluffiBasicBlock(resp->blocks().Get(0)) == FluffiBasicBlock(22, 11), L"Failed manual content checking (1)");
			Assert::IsTrue(FluffiBasicBlock(resp->blocks().Get(1)) == FluffiBasicBlock(44, 33), L"Failed manual content checking (2)");
			Assert::IsTrue(FluffiBasicBlock(resp->blocks().Get(2)) == FluffiBasicBlock(0x12345678ab, 55), L"Failed manual content checking (3)");

			delete resp;

			//Assert nothing changes if we reinsert the blocks for a different testcase
			dbman->addBlocksToCoveredBlocks(ftid2, blocks);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) FROM covered_blocks") == "6");
			resp = dbman->generateGetCurrentBlockCoverageResponse();
			Assert::IsTrue(blocks.size() == 3);
			delete resp;
		}

		TEST_METHOD(LMDatabaseManager_generateGetNewCompletedTestcaseIDsResponse)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			std::string initial_hap = "NOT_SET";
			std::string initial_guid = "initial";
			FluffiServiceDescriptor sd_initial{ initial_hap ,initial_guid };

			uint64_t localid1 = 1;
			FluffiTestcaseID ftid1{ sd1,localid1 };

			uint64_t localid2 = 2;
			FluffiTestcaseID ftid2{ sd1,localid2 };

			std::string guid3 = "guid3";
			std::string hap3 = "hap3";
			FluffiServiceDescriptor sd3{ hap3 ,guid3 };
			uint64_t localid3 = 3;
			FluffiTestcaseID ftid3{ sd3,localid3 };

			dbman->addEntryToCompletedTestcasesTable(ftid1);
			dbman->addEntryToCompletedTestcasesTable(ftid2);
			dbman->addEntryToCompletedTestcasesTable(ftid3);

			GetNewCompletedTestcaseIDsResponse* resp = dbman->generateGetNewCompletedTestcaseIDsResponse(0);
			Assert::IsTrue(resp->ids().size() == 3);
			Assert::IsTrue(resp->ids().Get(0).localid() == localid1 && resp->ids().Get(0).servicedescriptor().guid() == sd1.m_guid);
			Assert::IsTrue(resp->ids().Get(1).localid() == localid2 && resp->ids().Get(1).servicedescriptor().guid() == sd1.m_guid);
			Assert::IsTrue(resp->ids().Get(2).localid() == localid3 && resp->ids().Get(2).servicedescriptor().guid() == sd3.m_guid);

			//Assert timestamp is utc
			long t = (long)time(NULL);
			Assert::IsTrue(abs((google::protobuf::int64)t - (google::protobuf::int64)resp->updatetimestamp()) < 5 * 60);
			time_t firstInsertTS = resp->updatetimestamp();
			delete resp;

			//Make sure that the the result always contains the entries with the very time stamp (1)
			resp = dbman->generateGetNewCompletedTestcaseIDsResponse(firstInsertTS);
			Assert::IsTrue(0 < resp->ids().size());
			delete resp;

			//Make sure that the the result always contains the entries with the very time stamp (2)
			resp = dbman->generateGetNewCompletedTestcaseIDsResponse(firstInsertTS + 1);
			Assert::IsTrue(0 == resp->ids().size());
			delete resp;

			Sleep(2000);

			//adding a new one should give exactly the new one
			uint64_t localid4 = 4;
			FluffiTestcaseID ftid4{ sd3,localid4 };
			dbman->addEntryToCompletedTestcasesTable(ftid4);
			resp = dbman->generateGetNewCompletedTestcaseIDsResponse(firstInsertTS + 1);
			Assert::IsTrue(1 == resp->ids().size());
			delete resp;
		}

		static void generateGetTestcaseToMutateResponse_multithread(int i) {
			GarbageCollectorWorker gc(200);
			LMDatabaseManager local_lmdb(&gc);

			GetTestcaseToMutateResponse* resp = local_lmdb.generateGetTestcaseToMutateResponse(".", 10);
			Assert::IsTrue(!FluffiTestcaseID(resp->id()).m_serviceDescriptor.isNullObject() && FluffiTestcaseID(resp->id()).m_serviceDescriptor.m_guid != "", L"Multithreaded gettestcase failed");
			delete resp;
		}

		TEST_METHOD(LMDatabaseManager_generateGetTestcaseToMutateResponse)
		{
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			uint64_t localid1 = 1;
			FluffiTestcaseID ftid1{ sd1,localid1 };

			std::string guid2 = "guid2";
			std::string hap2 = "hap2";
			FluffiServiceDescriptor sd2{ hap2 ,guid2 };

			uint64_t localid2 = 2;
			FluffiTestcaseID ftid2{ sd2,localid2 };

			std::string testfile = Util::generateTestcasePathAndFilename(ftid1, ".");
			std::ofstream fout;
			fout.open(testfile, std::ios::binary | std::ios::out);
			for (int i = 0; i < 256; i++) {
				fout.write((char*)&i, 1);
			}
			fout.close();

			dbman->addEntryToInterestingTestcasesTable(ftid1, ftid1, 20, ".", LMDatabaseManager::TestCaseType::Population);

			//Check that the returned response has the expected content
			GetTestcaseToMutateResponse* resp = dbman->generateGetTestcaseToMutateResponse("", 10);
			FluffiTestcaseID returnedFtid1(resp->id());
			Assert::IsTrue(returnedFtid1.m_localID == ftid1.m_localID);
			Assert::IsTrue(returnedFtid1.m_serviceDescriptor.m_guid == ftid1.m_serviceDescriptor.m_guid);
			Assert::IsTrue(returnedFtid1.m_serviceDescriptor.m_serviceHostAndPort == ""); //is not set and is not known
			Assert::IsTrue(resp->islastchunk());
			Assert::IsTrue(resp->alsorunwithoutmutation());
			for (int i = 0; i < 256; i++) {
				Assert::IsTrue(resp->testcasefirstchunk()[i] == (char)i);
			}
			delete resp;

			//Check that the rating is adapted correctly
			Assert::IsTrue(10 == stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Rating FROM interesting_testcases")));

			resp = dbman->generateGetTestcaseToMutateResponse(".", 20);
			delete resp;

			Assert::IsTrue(-10 == stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT Rating FROM interesting_testcases")));

			//Check that always the testcase with the highest rating is returned
			dbman->addEntryToInterestingTestcasesTable(ftid2, ftid2, 20, ".", LMDatabaseManager::TestCaseType::Population);

			resp = dbman->generateGetTestcaseToMutateResponse(".", 20);
			FluffiTestcaseID returnedFtid2(resp->id());
			Assert::IsTrue(returnedFtid2.m_localID == ftid2.m_localID);
			Assert::IsTrue(returnedFtid2.m_serviceDescriptor.m_guid == ftid2.m_serviceDescriptor.m_guid);
			Assert::IsTrue(returnedFtid2.m_serviceDescriptor.m_serviceHostAndPort == ""); //is not set and is not known
			Assert::IsTrue(resp->alsorunwithoutmutation());
			delete resp;

			//check if alsoRunWithoutMutation is false if there are already some blocks for the testcase
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO  target_modules (ID,ModuleName) VALUES (11,'adsf')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO  covered_blocks (CreatorTestcaseID, ModuleID, Offset) VALUES ((SELECT ID FROM interesting_testcases WHERE CreatorServiceDescriptorGUID = '" + guid2 + "' AND CreatorLocalID = " + std::to_string(localid2) + "), 11,22)");
			resp = dbman->generateGetTestcaseToMutateResponse(".", 20);
			Assert::IsFalse(resp->alsorunwithoutmutation());
			delete resp;

			//Test multithreading
			for (int j = 0; j < 100; j++) {
				std::vector<std::thread> threads;
				for (int i = 0; i < 10; i++)
				{
					//create threads
					threads.push_back(std::thread(generateGetTestcaseToMutateResponse_multithread, i));
				}
				//wait for them to complete
				for (auto& th : threads)
					th.join();
			}
		}

		TEST_METHOD(LMDatabaseManager_getSettingValue)
		{
			std::string settingName1 = "settingName1";
			std::string settingValue1 = "settingValue1";
			std::string settingName2 = "settingName2";
			std::string settingValue2 = "settingValue2";

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO settings (`SettingName`, `SettingValue`) VALUES ('" + settingName1 + "', '" + settingValue1 + "')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO settings (`SettingName`, `SettingValue`) VALUES ('" + settingName2 + "', '" + settingValue2 + "')");

			std::deque<FluffiSetting> settings = dbman->getAllSettings();

			Assert::IsTrue(settings.size() == 2, L"The returned amount of settings was not the expected amount");

			Assert::IsTrue(settings[0].m_settingName == settingName1, L"name of setting1 was not as expected");
			Assert::IsTrue(settings[0].m_settingValue == settingValue1, L"value of setting1 was not as expected");
			Assert::IsTrue(settings[1].m_settingName == settingName2, L"name of setting2 was not as expected");
			Assert::IsTrue(settings[1].m_settingValue == settingValue2, L"value setting2 was not as expected");
		}

		TEST_METHOD(LMDatabaseManager_getTargetModules)
		{
			std::string mod1 = "1.dll";
			std::string mod2 = "2.dll";
			std::string path2 = "path2";

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ModuleName) VALUES ( '" + mod1 + "')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ModuleName, ModulePath) VALUES ('" + mod2 + "', '" + path2 + "')");

			std::deque<FluffiModuleNameToID> modules = dbman->getTargetModules();
			Assert::IsTrue(modules[0].m_moduleID == 1);
			Assert::IsTrue(modules[0].m_moduleName == mod1);
			Assert::IsTrue(modules[0].m_modulePath == "*");

			Assert::IsTrue(modules[1].m_moduleID == 2);
			Assert::IsTrue(modules[1].m_moduleName == mod2);
			Assert::IsTrue(modules[1].m_modulePath == path2);
		}

		TEST_METHOD(LMDatabaseManager_getTargetBlocks)
		{
			std::string mod1 = "1.dll";
			std::string mod2 = "2.dll";
			std::string path2 = "path2";

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ModuleName) VALUES ( '" + mod1 + "')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO target_modules (ModuleName, ModulePath) VALUES ('" + mod2 + "', '" + path2 + "')");

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO blocks_to_cover (ModuleID, Offset) VALUES ( 1,3),( 2,4),(1,78187493547)");

			std::deque<FluffiBasicBlock> blocks = dbman->getTargetBlocks();
			Assert::IsTrue(blocks[0].m_moduleID == 1);
			Assert::IsTrue(blocks[0].m_rva == 3);

			Assert::IsTrue(blocks[2].m_moduleID == 2);
			Assert::IsTrue(blocks[2].m_rva == 4);

			Assert::IsTrue(blocks[1].m_moduleID == 1);
			Assert::IsTrue(blocks[1].m_rva == 0x12345678ab);
		}

		TEST_METHOD(LMDatabaseManager_initializeBillingTable)
		{
			dbman->initializeBillingTable();
			Assert::IsTrue("2" == dbman->EXECUTE_TEST_STATEMENT("SELECT Count(*) FROM billing"));
			Assert::IsTrue("0" == dbman->EXECUTE_TEST_STATEMENT("SELECT Amount FROM billing WHERE Resource = 'RunnerSeconds'"));
			Assert::IsTrue("0" == dbman->EXECUTE_TEST_STATEMENT("SELECT Amount FROM billing WHERE Resource = 'RunTestcasesNoLongerListed'"));
		}

		TEST_METHOD(LMDatabaseManager_addRunnerSeconds)
		{
			dbman->initializeBillingTable();
			dbman->addRunnerSeconds(1111);
			Assert::IsTrue("1111" == dbman->EXECUTE_TEST_STATEMENT("SELECT Amount FROM billing WHERE Resource = 'RunnerSeconds'"));
		}

		TEST_METHOD(LMDatabaseManager_cleanCompletedTestcasesTableOlderThanXSec)
		{
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO completed_testcases (CreatorServiceDescriptorGUID, CreatorLocalID, TimeOfCompletion) VALUES ('1',1,'2017-01-29 09:41:54')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO completed_testcases (CreatorServiceDescriptorGUID, CreatorLocalID, TimeOfCompletion) VALUES ('2',2,'2017-01-29 09:42:54')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO completed_testcases (CreatorServiceDescriptorGUID, CreatorLocalID, TimeOfCompletion) VALUES ('3',3,'2017-01-29 09:43:54')");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO completed_testcases (CreatorServiceDescriptorGUID, CreatorLocalID, TimeOfCompletion) VALUES ('4',4,NOW())");

			Assert::IsTrue("4" == dbman->EXECUTE_TEST_STATEMENT("SELECT Count(*) FROM completed_testcases"));

			unsigned long long cleaned = dbman->cleanCompletedTestcasesTableOlderThanXSec(100);
			Assert::IsTrue(3 == cleaned);

			Assert::IsTrue("1" == dbman->EXECUTE_TEST_STATEMENT("SELECT Count(*) FROM completed_testcases"));
		}

		TEST_METHOD(LMDatabaseManager_addRunTestcasesNoLongerListed)
		{
			dbman->initializeBillingTable();
			dbman->addRunTestcasesNoLongerListed(2222);
			Assert::IsTrue("2222" == dbman->EXECUTE_TEST_STATEMENT("SELECT Amount FROM billing WHERE Resource = 'RunTestcasesNoLongerListed'"));
		}

		TEST_METHOD(LMDatabaseManager_addEntriesToCompletedTestcasesTable)
		{
			std::set<FluffiTestcaseID> ids;

			bool success = dbman->addEntriesToCompletedTestcasesTable(ids);
			Assert::IsTrue(success, L"failed adding an empty vector");
			Assert::IsTrue("0" == dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) FROM completed_testcases"), L"Expected 0 elements in completed_testcases but that was not the case");

			FluffiServiceDescriptor sd1{ "hap1", "guid1" };
			FluffiTestcaseID tid1{ sd1, 1 };
			ids.insert(tid1);
			FluffiServiceDescriptor sd2{ "hap2", "guid2" };
			FluffiTestcaseID tid2{ sd2, 2 };
			ids.insert(tid2);
			FluffiServiceDescriptor sd3{ "hap3", "guid3" };
			FluffiTestcaseID tid3{ sd3, 3 };
			ids.insert(tid3);
			success = dbman->addEntriesToCompletedTestcasesTable(ids);
			Assert::IsTrue(success, L"failed adding an vector of three elements");
			Assert::IsTrue("3" == dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) FROM completed_testcases"), L"Expected 3 elements in completed_testcases but that was not the case (1)");

			Assert::IsTrue("guid1" == dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorServiceDescriptorGUID FROM completed_testcases WHERE CreatorLocalID=1"), L"The first entry was not created as expected");
			Assert::IsTrue("guid2" == dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorServiceDescriptorGUID FROM completed_testcases WHERE CreatorLocalID=2"), L"The second entry was not created as expected");
			Assert::IsTrue("guid3" == dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorServiceDescriptorGUID FROM completed_testcases WHERE CreatorLocalID=3"), L"The third entry was not created as expected");

			FluffiServiceDescriptor sd4{ "hap4", "guid4" };
			FluffiTestcaseID tid4{ sd4, 4 };
			ids.insert(tid4);

			success = dbman->addEntriesToCompletedTestcasesTable(ids);
			Assert::IsTrue(success, L"failed adding the vector a second time");
			Assert::IsTrue("7" == dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) FROM completed_testcases"), L"Expected 7 elements in completed_testcases but that was not the case");

			Assert::IsTrue("guid4" == dbman->EXECUTE_TEST_STATEMENT("SELECT CreatorServiceDescriptorGUID FROM completed_testcases WHERE CreatorLocalID=4"), L"The fourth entry was not created as expected");
		}

		TEST_METHOD(LMDatabaseManager_dropTestcaseTypeIFMoreThan)
		{
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO interesting_testcases (CreatorServiceDescriptorGUID,CreatorLocalID,ParentServiceDescriptorGUID,ParentLocalID,Rating,RawBytes,TestCaseType) VALUES ('GUID1',1,'GUID2',2,0,'',1),('GUID3',3,'GUID4',4,0,'',1),('GUID5',5,'GUID6',6,0,'',1)");
			Assert::IsTrue("3" == dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) FROM interesting_testcases"), L"Filling the database with dummy values didn't work");
			dbman->dropTestcaseTypeIFMoreThan(LMDatabaseManager::TestCaseType::Hang, 1);
			Assert::IsTrue("1" == dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) FROM interesting_testcases"), L"Dropping down to one entry didnt work");

			dbman->dropTestcaseTypeIFMoreThan(LMDatabaseManager::TestCaseType::Hang, 1);
			Assert::IsTrue("1" == dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) FROM interesting_testcases"), L"Somehow dropping a second time changed something!");
		}

		TEST_METHOD(LMDatabaseManager_getAllCrashFootprints)
		{
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO interesting_testcases (CreatorServiceDescriptorGUID,CreatorLocalID,ParentServiceDescriptorGUID,ParentLocalID,Rating,RawBytes,TestCaseType) VALUES ('GUID1',1,'GUID2',2,0,'',2),('GUID3',3,'GUID4',4,0,'',2),('GUID5',5,'GUID6',6,0,'',2)");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO crash_descriptions (CreatorTestcaseID,CrashFootprint) VALUES ((SELECT ID FROM interesting_testcases WHERE CreatorServiceDescriptorGUID='GUID1'),'A'),((SELECT ID FROM interesting_testcases WHERE CreatorServiceDescriptorGUID='GUID3'),'A'),((SELECT ID FROM interesting_testcases WHERE CreatorServiceDescriptorGUID='GUID5'),'B')");

			std::deque<std::string> footprints = dbman->getAllCrashFootprints();
			Assert::IsTrue(footprints.size() == 2, L"Did not retreive the two expected footprints");
			Assert::IsTrue(footprints[0] == "A", L"The first footprint was not 'A'");
			Assert::IsTrue(footprints[1] == "B", L"The second footprint was not 'B'");
		}

		TEST_METHOD(LMDatabaseManager_dropTestcaseIfCrashFootprintAppearedMoreThanXTimes)
		{
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO interesting_testcases (CreatorServiceDescriptorGUID,CreatorLocalID,ParentServiceDescriptorGUID,ParentLocalID,Rating,RawBytes,TestCaseType) VALUES ('GUID1',1,'GUID2',2,0,'',2),('GUID3',3,'GUID4',4,0,'',2),('GUID5',5,'GUID6',6,0,'',2)");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO crash_descriptions (CreatorTestcaseID,CrashFootprint) VALUES ((SELECT ID FROM interesting_testcases WHERE CreatorServiceDescriptorGUID='GUID1'),'A'),((SELECT ID FROM interesting_testcases WHERE CreatorServiceDescriptorGUID='GUID3'),'A'),((SELECT ID FROM interesting_testcases WHERE CreatorServiceDescriptorGUID='GUID5'),'B')");

			Assert::IsTrue("3" == dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) FROM interesting_testcases"), L"There weren't the expected three entries before dropping");
			Assert::IsTrue(dbman->dropTestcaseIfCrashFootprintAppearedMoreThanXTimes(1, "A"), L"dropTestcaseIfCrashFootprintAppearedMoreThanXTimes did not return ture");
			Assert::IsTrue("2" == dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) FROM interesting_testcases"), L"There weren't the expected two entries after dropping");

			std::deque<std::string> footprints = dbman->getAllCrashFootprints();
			Assert::IsTrue(footprints.size() == 2, L"Did not retreive the two expected footprints");
			Assert::IsTrue(footprints[0] == "A", L"The first footprint was not 'A'");
			Assert::IsTrue(footprints[1] == "B", L"The second footprint was not 'B'");
		}

		TEST_METHOD(LMDatabaseManager_getRegisteredInstancesNumOfSubTypes)
		{
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO managed_instances (ServiceDescriptorGUID,ServiceDescriptorHostAndPort,AgentType,Location,AgentSubType) VALUES ('GUID1','HAP1',1,'home','TypeA'),('GUID2','HAP2',1,'home','TypeA'),('GUID3','HAP3',1,'home','TypeA'),('GUID4','HAP4',1,'home','TypeB'),('GUID5','HAP5',1,'home','TypeB')");
			std::vector<std::pair<std::string, int>> registered = dbman->getRegisteredInstancesNumOfSubTypes();

			Assert::IsTrue(registered.size() == 2, L"registered was not of the expected size");
			Assert::IsTrue(registered[0].first == "TypeA", L"TypeA was not the first element");
			Assert::IsTrue(registered[0].second == 3, L"TypeA was not found to be three times in the databse");
			Assert::IsTrue(registered[1].first == "TypeB", L"TypeB was not the second element");
			Assert::IsTrue(registered[1].second == 2, L"TypeB was not found to be two times in the databse");
		}

		TEST_METHOD(LMDatabaseManager_setTestcaseType) {
			std::string guid1 = "guid1";
			std::string hap1 = "hap1";
			FluffiServiceDescriptor sd1{ hap1 ,guid1 };

			uint64_t localid1 = 1;
			FluffiTestcaseID ftid1{ sd1,localid1 };

			uint64_t localid2 = 2;
			FluffiTestcaseID ftid2{ sd1,localid2 };

			dbman->addEntryToInterestingTestcasesTable(ftid1, ftid1, 0, "", LMDatabaseManager::TestCaseType::Locked);
			dbman->addEntryToInterestingTestcasesTable(ftid2, ftid2, 0, "", LMDatabaseManager::TestCaseType::Locked);

			//Check if population type is changed
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT TestCaseType from interesting_testcases WHERE CreatorLocalID = " + std::to_string(localid1))) == LMDatabaseManager::TestCaseType::Locked);
			dbman->setTestcaseType(ftid1, LMDatabaseManager::TestCaseType::Population);
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT TestCaseType from interesting_testcases WHERE CreatorLocalID = " + std::to_string(localid1))) == LMDatabaseManager::TestCaseType::Population);

			//Check if other entries are affected
			Assert::IsTrue(stoi(dbman->EXECUTE_TEST_STATEMENT("SELECT TestCaseType from interesting_testcases WHERE CreatorLocalID = " + std::to_string(localid2))) == LMDatabaseManager::TestCaseType::Locked);
		}

		TEST_METHOD(LMDatabaseManager_addNewManagedInstanceLogMessages)
		{
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE managed_instances_logmessages;");

			std::string tooLongGUID(60, 'a');
			Assert::IsFalse(dbman->addNewManagedInstanceLogMessages(tooLongGUID, {}), L"Inserting a log message with a too long guid succeeded for some reason");

			std::string validGUID = "testguid";
			std::vector<std::string> invalidLogMessages;
			invalidLogMessages.push_back(std::string(2000, 'b'));
			Assert::IsFalse(dbman->addNewManagedInstanceLogMessages(validGUID, invalidLogMessages), L"Inserting a log message with a too long content succeeded for some reason");

			std::vector<std::string> validLogMessages;
			validLogMessages.push_back("a");
			validLogMessages.push_back("b");
			validLogMessages.push_back("c");
			Assert::IsTrue(dbman->addNewManagedInstanceLogMessages(validGUID, validLogMessages), L"Inserting new valid log messages failed");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_logmessages") == "3", L"We got more or less log messages than expected");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorGUID from managed_instances_logmessages WHERE ID=1") == "testguid", L"Invalid guid stored (1)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorGUID from managed_instances_logmessages WHERE ID=2") == "testguid", L"Invalid guid stored (2)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorGUID from managed_instances_logmessages WHERE ID=3") == "testguid", L"Invalid guid stored (3)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from managed_instances_logmessages WHERE ID=1") == "a", L"Invalid log message stored (1)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from managed_instances_logmessages WHERE ID=2") == "b", L"Invalid log message stored (2)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from managed_instances_logmessages WHERE ID=3") == "c", L"Invalid log message stored (3)");

			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE managed_instances_logmessages;");
		}
		TEST_METHOD(LMDatabaseManager_deleteManagedInstanceLogMessagesIfMoreThan)
		{
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE managed_instances_logmessages;");

			std::string validGUID = "testguid";
			std::vector<std::string> validLogMessages;
			validLogMessages.push_back("a");
			validLogMessages.push_back("b");
			validLogMessages.push_back("c");
			Assert::IsTrue(dbman->addNewManagedInstanceLogMessages(validGUID, validLogMessages), L"Inserting new valid log messages failed");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_logmessages") == "3", L"We got more or less log messages than expected (1)");

			Assert::IsTrue(dbman->deleteManagedInstanceLogMessagesIfMoreThan(3), L"Calling deleteManagedInstanceLogMessagesIfMoreThan failed (1)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_logmessages") == "3", L"We got more or less log messages than expected (2)");

			Assert::IsTrue(dbman->deleteManagedInstanceLogMessagesIfMoreThan(2), L"Calling deleteManagedInstanceLogMessagesIfMoreThan failed (2)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_logmessages") == "2", L"We got more or less log messages than expected (3)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from managed_instances_logmessages WHERE ID=2") == "b", L"Invalid log message stored (2)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from managed_instances_logmessages WHERE ID=3") == "c", L"Invalid log message stored (3)");

			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE managed_instances_logmessages;");
		}
		TEST_METHOD(LMDatabaseManager_deleteManagedInstanceLogMessagesOlderThanXSec)
		{
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE managed_instances_logmessages;");

			std::string validGUID = "testguid";
			std::vector<std::string> validLogMessages;
			validLogMessages.push_back("a");
			validLogMessages.push_back("b");
			validLogMessages.push_back("c");
			Assert::IsTrue(dbman->addNewManagedInstanceLogMessages(validGUID, validLogMessages), L"Inserting new valid log messages failed (1)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_logmessages") == "3", L"We got more or less log messages than expected (1)");

			Assert::IsTrue(dbman->deleteManagedInstanceLogMessagesOlderThanXSec(5), L"Calling deleteManagedInstanceLogMessagesOlderThanXSec failed (1)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_logmessages") == "3", L"We got more or less log messages than expected (2)");

			Sleep(2000);

			Assert::IsTrue(dbman->addNewManagedInstanceLogMessages(validGUID, validLogMessages), L"Inserting new valid log messages failed (2)");

			Assert::IsTrue(dbman->deleteManagedInstanceLogMessagesOlderThanXSec(5), L"Calling deleteManagedInstanceLogMessagesOlderThanXSec failed (2)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_logmessages") == "6", L"We got more or less log messages than expected (3)");

			Sleep(4000);

			Assert::IsTrue(dbman->deleteManagedInstanceLogMessagesOlderThanXSec(5), L"Calling deleteManagedInstanceLogMessagesOlderThanXSec failed (3)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_logmessages") == "3", L"We got more or less log messages than expected (4)");

			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE managed_instances_logmessages;");
		}

	private:

		const std::string testdbUser = "root";
		const std::string testdbPass = "toor";
		const std::string testdbHost = "fluffiLMDBHost";
		const std::string testdbName = "fluffi_test";

		LMDatabaseManager* setupTestDB(GarbageCollectorWorker* garbageCollectorWorker) {
			std::ifstream createDatabaseFile("..\\..\\..\\srv\\fluffi\\data\\fluffiweb\\app\\sql_files\\createLMDB.sql");
			std::string dbCreateFileContent;

			createDatabaseFile.seekg(0, std::ios::end);
			dbCreateFileContent.reserve(createDatabaseFile.tellg());
			createDatabaseFile.seekg(0, std::ios::beg);

			dbCreateFileContent.assign((std::istreambuf_iterator<char>(createDatabaseFile)),
				std::istreambuf_iterator<char>());

			//adapt database name
			Util::replaceAll(dbCreateFileContent, "fluffi", testdbName);

			//remove linebreaks
			std::string::size_type pos = 0; // Must initialize
			while ((pos = dbCreateFileContent.find("\r\n", pos)) != std::string::npos)
			{
				dbCreateFileContent.erase(pos, 2);
			}

			pos = 0; // Must initialize
			while ((pos = dbCreateFileContent.find("\n", pos)) != std::string::npos)
			{
				dbCreateFileContent.erase(pos, 1);
			}

			pos = 0; // Must initialize
			while ((pos = dbCreateFileContent.find("\t", pos)) != std::string::npos)
			{
				dbCreateFileContent.erase(pos, 1);
			}

			//remove comments
			std::regex commentRegEx("/\\*.*\\*/");
			dbCreateFileContent = std::regex_replace(dbCreateFileContent, commentRegEx, "");

			std::vector<std::string> SQLcommands = Util::splitString(dbCreateFileContent, ";");

			LMDatabaseManager* dbman = new LMDatabaseManager(garbageCollectorWorker);
			LMDatabaseManager::setDBConnectionParameters(testdbHost, testdbUser, testdbPass, "information_schema");

			dbman->EXECUTE_TEST_STATEMENT("DROP DATABASE IF EXISTS " + testdbName);

			for (auto const& command : SQLcommands) {
				if (ltrim_copy(command).substr(0, 6) == "CREATE") {
					dbman->EXECUTE_TEST_STATEMENT(command);
				}
			}

			dbman->EXECUTE_TEST_STATEMENT("USE " + testdbName);
			LMDatabaseManager::setDBConnectionParameters(testdbHost, testdbUser, testdbPass, testdbName);

			return dbman;
		}

		// trim from start (in place)
		static inline void ltrim(std::string &s) {
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
				return !std::isspace(ch);
			}));
		}

		// trim from start (copying)
		static inline std::string ltrim_copy(std::string s) {
			ltrim(s);
			return s;
		}

		static char* strptime(const char* s,
			const char* f,
			struct tm* tm) {
			// Isn't the C++ standard lib nice? std::get_time is defined such that its
			// format parameters are the exact same as strptime. Of course, we have to
			// create a string stream first, and imbue it with the current C locale, and
			// we also have to make sure we return the right things if it fails, or
			// if it succeeds, but this is still far simpler an implementation than any
			// of the versions in any of the C standard libraries.
			std::istringstream input(s);
			input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
			input >> std::get_time(tm, f);
			if (input.fail()) {
				return nullptr;
			}
			return (char*)(s + input.tellg());
		}
	};
}
