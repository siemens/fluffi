/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Michael Kraus, Thomas Riedmaier, Pascal Eckmann, Abian Blome
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "LMDatabaseManager.h"
#include "Util.h"
#include "WorkerWeightCalculator.h"
#include "FluffiServiceDescriptor.h"
#include "StatusOfInstance.h"
#include "GarbageCollectorWorker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WorkerWeightCalculatorTester
{
	TEST_CLASS(WorkerWeightCalculatorTest)
	{
	public:

		WorkerWeightCalculator* weightCalculator = nullptr;
		LMDatabaseManager* dbman = nullptr;
		GarbageCollectorWorker* garbageCollector = nullptr;

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
			garbageCollector = new GarbageCollectorWorker(200);
			dbman = setupTestDB(garbageCollector);
			weightCalculator = new WorkerWeightCalculator(dbman, "testlocation");
		}

		TEST_METHOD_CLEANUP(ModuleCleanup)
		{
			//Freeing the testcase Queue
			delete weightCalculator;
			weightCalculator = nullptr;

			// Freeing the databaseManager
			delete dbman;
			dbman = nullptr;

			// Freeing the garbageCollector
			delete garbageCollector;
			garbageCollector = nullptr;
		}

		TEST_METHOD(WorkerWeightCalculator_updateStatusInformation)
		{
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "0");

			std::string testGUID1 = "testguid1";
			std::string testHAP1 = "testhap1";
			std::string testStatus1 = "TestCasesQueueSize 100";

			FluffiServiceDescriptor sd1{ testHAP1 ,testGUID1 };
			dbman->writeManagedInstance(sd1, AgentType::TestcaseEvaluator, "TestSubType", "testlocation");

			Assert::IsTrue(dbman->addNewManagedInstanceStatus(testGUID1, testStatus1));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "1");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses") == testStatus1);

			// Test Method
			weightCalculator->updateStatusInformation();

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "1");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses") == testStatus1);

			Assert::AreEqual(weightCalculator->getEvaluatorStatus().size(), (size_t)1, L"Error checking number of inserted InstanceStatus: (updateStatusInformation)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().size(), (size_t)0, L"Error checking number of inserted InstanceStatus: (updateStatusInformation)");

			std::string testGUID2 = "testguid2";
			std::string testHAP2 = "testhap2";
			std::string testStatus2 = "TestCasesQueueSize 200";

			FluffiServiceDescriptor sd2{ testHAP2 ,testGUID2 };
			dbman->writeManagedInstance(sd2, AgentType::TestcaseEvaluator, "TestSubType", "testlocation");

			Assert::IsTrue(dbman->addNewManagedInstanceStatus(testGUID2, testStatus2));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses where ServiceDescriptorGUID = 'testguid2'") == testStatus2);

			// Test Method
			weightCalculator->updateStatusInformation();

			Assert::AreEqual(weightCalculator->getEvaluatorStatus().size(), (size_t)2, L"Error checking number of inserted InstanceStatus: (updateStatusInformation)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().size(), (size_t)0, L"Error checking number of inserted InstanceStatus: (updateStatusInformation)");

			std::string testGUID3 = "testguid3";
			std::string testHAP3 = "testhap3";
			std::string testStatus3 = "TestCasesQueueSize 300";

			FluffiServiceDescriptor sd3{ testHAP3 ,testGUID3 };
			dbman->writeManagedInstance(sd3, AgentType::TestcaseGenerator, "TestSubType", "testlocation");

			Assert::IsTrue(dbman->addNewManagedInstanceStatus(testGUID3, testStatus3));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "3");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses where ServiceDescriptorGUID = 'testguid3'") == testStatus3);

			// Test Method
			weightCalculator->updateStatusInformation();

			Assert::AreEqual(weightCalculator->getEvaluatorStatus().size(), (size_t)2, L"Error checking number of inserted InstanceStatus: (updateStatusInformation)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().size(), (size_t)1, L"Error checking number of inserted InstanceStatus: (updateStatusInformation)");
		}

		TEST_METHOD(WorkerWeightCalculator_getGeneratorStatus)
		{
			// Test Method
			Assert::AreEqual(weightCalculator->getGeneratorStatus().size(), (size_t)0, L"Error checking number of inserted InstanceStatuses: (getGeneratorStatus)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "0");

			std::string testGUID1 = "testguid1";
			std::string testHAP1 = "testhap1";
			std::string testStatus1 = "Test 11 1111|TestCasesQueueSize 11 1111";

			FluffiServiceDescriptor sd1{ testHAP1 ,testGUID1 };
			dbman->writeManagedInstance(sd1, AgentType::TestcaseGenerator, "TestSubType", "testlocation");

			Assert::IsTrue(dbman->addNewManagedInstanceStatus(testGUID1, testStatus1));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "1");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses") == testStatus1);

			weightCalculator->updateStatusInformation();

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "1");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses") == testStatus1);

			// Test Method
			Assert::AreEqual(weightCalculator->getGeneratorStatus().size(), (size_t)1, L"Error checking number of inserted InstanceStatus: (getGeneratorStatus)");

			std::string testGUID2 = "testguid2";
			std::string testHAP2 = "testhap2";
			std::string testStatus2 = "Test 22 2222|TestCasesQueueSize 22 2222";

			FluffiServiceDescriptor sd2{ testHAP2 ,testGUID2 };
			dbman->writeManagedInstance(sd2, AgentType::TestcaseGenerator, "TestSubType", "testlocation");

			Assert::IsTrue(dbman->addNewManagedInstanceStatus(testGUID2, testStatus2));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses where ServiceDescriptorGUID = 'testguid2'") == testStatus2);

			weightCalculator->updateStatusInformation();

			// Test Methods
			Assert::AreEqual(weightCalculator->getGeneratorStatus().size(), (size_t)2, L"Wrong number of statuses in vector: (getGeneratorStatus)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().back().serviceDescriptorGUID, std::string("testguid1"), L"Wrong data (GUID) in called StatusOfInstance: (getGeneratorStatus)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().front().serviceDescriptorGUID, std::string("testguid2"), L"Wrong data (GUID) in called StatusOfInstance: (getGeneratorStatus)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().back().serviceDescriptorHostAndPort, std::string("testhap1"), L"Wrong data (HaP) in called StatusOfInstance: (getGeneratorStatus)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().front().serviceDescriptorHostAndPort, std::string("testhap2"), L"Wrong data (HaP) in called StatusOfInstance: (getGeneratorStatus)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().back().weight, (unsigned int)111, L"Wrong data (Weight) in called StatusOfInstance: (getGeneratorStatus)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().front().weight, (unsigned int)122, L"Wrong data (Weight) in called StatusOfInstance: (getGeneratorStatus)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().size(), (size_t)2, L"Wrong number of statuses in vector: (getGeneratorStatus)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().front().status.empty(), false, L"Empty data (Status) in called StatusOfInstance: (getGeneratorStatus)");
			Assert::AreEqual(weightCalculator->getGeneratorStatus().back().status.empty(), false, L"Empty data (Status) in called StatusOfInstance: (getGeneratorStatus)");
			std::map<std::string, std::string> statusMap = weightCalculator->getGeneratorStatus().front().status;
			Assert::AreEqual(statusMap["TestCasesQueueSize"], std::string("22"), L"Wrong data (Weight in Status) in called StatusOfInstance: (getGeneratorStatus)");
			std::map<std::string, std::string> statusMap2 = weightCalculator->getGeneratorStatus().back().status;
			Assert::AreEqual(statusMap2["TestCasesQueueSize"], std::string("11"), L"Wrong data (Weight in Status) in called StatusOfInstance: (getGeneratorStatus)");
		}

		TEST_METHOD(WorkerWeightCalculator_getEvaluatorStatus)
		{
			// Test Method
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().size(), (size_t)0, L"Error checking number of inserted InstanceStatuses: (getEvaluatorStatus)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "0");

			std::string testGUID1 = "testguid1";
			std::string testHAP1 = "testhap1";
			std::string testStatus1 = "Test 11 1111|TestEvaluationsQueueSize 11 1111";

			FluffiServiceDescriptor sd1{ testHAP1 ,testGUID1 };
			dbman->writeManagedInstance(sd1, AgentType::TestcaseEvaluator, "TestSubType", "testlocation");

			Assert::IsTrue(dbman->addNewManagedInstanceStatus(testGUID1, testStatus1));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "1");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses") == testStatus1);

			weightCalculator->updateStatusInformation();

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "1");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses") == testStatus1);

			// Test Method
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().size(), (size_t)1, L"Error checking number of inserted InstanceStatuses: (getEvaluatorStatus)");

			std::string testGUID2 = "testguid2";
			std::string testHAP2 = "testhap2";
			std::string testStatus2 = "Test 22 2222|TestEvaluationsQueueSize 22 2222";

			FluffiServiceDescriptor sd2{ testHAP2 ,testGUID2 };
			dbman->writeManagedInstance(sd2, AgentType::TestcaseEvaluator, "TestSubType", "testlocation");

			Assert::IsTrue(dbman->addNewManagedInstanceStatus(testGUID2, testStatus2));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from managed_instances_statuses") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Status from managed_instances_statuses where ServiceDescriptorGUID = 'testguid2'") == testStatus2);

			weightCalculator->updateStatusInformation();

			// Test Methods
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().size(), (size_t)2, L"Wrong number of statuses in vector: (getEvaluatorStatus)");
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().back().serviceDescriptorGUID, std::string("testguid1"), L"Wrong data (GUID) in called StatusOfInstance: (getEvaluatorStatus)");
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().front().serviceDescriptorGUID, std::string("testguid2"), L"Wrong data (GUID) in called StatusOfInstance: (getEvaluatorStatus)");
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().back().serviceDescriptorHostAndPort, std::string("testhap1"), L"Wrong data (HaP) in called StatusOfInstance: (getEvaluatorStatus)");
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().front().serviceDescriptorHostAndPort, std::string("testhap2"), L"Wrong data (HaP) in called StatusOfInstance: (getEvaluatorStatus)");
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().back().weight, (unsigned int)(1000 - 11), L"Wrong data (Weight) in called StatusOfInstance: (getEvaluatorStatus)");
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().front().weight, (unsigned int)(1000 - 22), L"Wrong data (Weight) in called StatusOfInstance: (getEvaluatorStatus)");
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().size(), (size_t)2, L"Wrong number of statuses in vector: (getEvaluatorStatus)");
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().front().status.empty(), false, L"Empty data (Status) in called StatusOfInstance: (getEvaluatorStatus)");
			Assert::AreEqual(weightCalculator->getEvaluatorStatus().back().status.empty(), false, L"Empty data (Status) in called StatusOfInstance: (getEvaluatorStatus)");
			std::map<std::string, std::string> statusMap = weightCalculator->getEvaluatorStatus().front().status;
			Assert::AreEqual(statusMap["TestEvaluationsQueueSize"], std::string("22"), L"Wrong data (Weight in Status) in called StatusOfInstance: (getEvaluatorStatus)");
			std::map<std::string, std::string> statusMap2 = weightCalculator->getEvaluatorStatus().back().status;
			Assert::AreEqual(statusMap2["TestEvaluationsQueueSize"], std::string("11"), L"Wrong data (Weight in Status) in called StatusOfInstance: (getEvaluatorStatus)");
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
	};
}
