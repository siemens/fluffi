/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Michael Kraus, Thomas Riedmaier, Abian Blome, Roman Bendt
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "GMDatabaseManager.h"
#include "FluffiServiceDescriptor.h"
#include "FluffiLMConfiguration.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GMDatabaseManagerTester
{
	TEST_CLASS(GMDatabaseManagerTest)
	{
	public:

		GMDatabaseManager* dbman = nullptr;

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
			dbman = setupTestDB();
		}

		TEST_METHOD_CLEANUP(ModuleCleanup)
		{
			// Freeing the databaseManager
			delete dbman;
			dbman = nullptr;

			//mysql_library_end();
		}

		TEST_METHOD(GMDatabaseManager_getLMServiceDescriptorForWorker)
		{
			// Build DB
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "0");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from locations") == "0");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 21, '21','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 22, '22','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mittelerde');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Auenland');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('1111-2222', '127.0.0.1:4444', (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('3333-4444', '127.0.0.1:5555', (SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from locations") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from location_fuzzjobs") == "2");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('9999-9999', 'System9', 21, (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 1, 'DefaultRunner');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('8888-8888', 'System8',22, (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 2, 'DefaultEvaluator');");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "2");

			std::string testGUID1 = "9999-9999";
			std::string testHAP1 = "testhap1";
			FluffiServiceDescriptor sd1{ testHAP1 ,testGUID1 };

			// Exists, there is a lm in the location
			FluffiServiceDescriptor ret = dbman->getLMServiceDescriptorForWorker(sd1);
			Assert::AreEqual(ret.isNullObject(), false, L"Error getting LM SD for specific Worker: (getLMServiceDescriptorForWorker)");
			Assert::AreEqual(ret.m_guid, std::string("1111-2222"), L"Error getting LM SD for specific Worker: (getLMServiceDescriptorForWorker)");
			Assert::AreEqual(ret.m_serviceHostAndPort, std::string("127.0.0.1:4444"), L"Error getting LM SD for specific Worker: (getLMServiceDescriptorForWorker)");

			// Exists, there is no lm in the location
			//'3333-4444', '127.0.0.1:5555' is in location auenland, and responsible for fuzzjob 22
			//agent 8888-8888 has fuzzjob 22, but is located in location Mitteleerde
			std::string testGUID3 = "8888-8888";
			std::string testHAP3 = "testhap3";
			FluffiServiceDescriptor sd3{ testHAP3 ,testGUID3 };
			ret = dbman->getLMServiceDescriptorForWorker(sd3);
			Assert::IsTrue(ret.isNullObject(), L"A localmanager was returned for a worker that was in a different location");

			// Not exists
			std::string testGUID2 = "1234-5678";
			std::string testHAP2 = "testhap2";
			FluffiServiceDescriptor sd2{ testHAP2 ,testGUID2 };
			ret = dbman->getLMServiceDescriptorForWorker(sd2);
			Assert::IsTrue(ret.isNullObject(), L"A localmanager was returned for a worker that was not in the workers table");
		}

		TEST_METHOD(GMDatabaseManager_getLMConfigurationForFuzzJob)
		{
			// Fill DB
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from fuzzjob") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost, DBUser, DBPass, DBName) VALUES (888, 'Default1', 'fluffiLMDBHost1', 'root1', 'toor1', 'fluffi_gm1');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost, DBUser, DBPass, DBName) VALUES (999, 'Default2', 'fluffiLMDBHost2', 'root2', 'toor2', 'fluffi_gm2');");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from fuzzjob") == "2");

			// Exists
			FluffiLMConfiguration conf1 = dbman->getLMConfigurationForFuzzJob(888);
			Assert::AreEqual(conf1.m_dbHost, std::string("fluffiLMDBHost1"), L"Error getting correct LM Configuration for FuzzJob: (getLMConfigurationForFuzzJob)");
			Assert::AreEqual(conf1.m_dbName, std::string("fluffi_gm1"), L"Error getting correct LM Configuration for FuzzJob: (getLMConfigurationForFuzzJob)");
			Assert::AreEqual(conf1.m_dbUser, std::string("root1"), L"Error getting correct LM Configuration for FuzzJob: (getLMConfigurationForFuzzJob)");
			Assert::AreEqual(conf1.m_dbPassword, std::string("toor1"), L"Error getting correct LM Configuration for FuzzJob: (getLMConfigurationForFuzzJob)");

			// Exists 2
			FluffiLMConfiguration conf2 = dbman->getLMConfigurationForFuzzJob(999);
			Assert::AreEqual(conf2.m_dbHost, std::string("fluffiLMDBHost2"), L"Error getting correct LM Configuration for FuzzJob: (getLMConfigurationForFuzzJob)");
			Assert::AreEqual(conf2.m_dbName, std::string("fluffi_gm2"), L"Error getting correct LM Configuration for FuzzJob: (getLMConfigurationForFuzzJob)");
			Assert::AreEqual(conf2.m_dbUser, std::string("root2"), L"Error getting correct LM Configuration for FuzzJob: (getLMConfigurationForFuzzJob)");
			Assert::AreEqual(conf2.m_dbPassword, std::string("toor2"), L"Error getting correct LM Configuration for FuzzJob: (getLMConfigurationForFuzzJob)");

			GMDatabaseManager* dbman2 = dbman;

			// Does not exist
			auto f = [dbman2] { dbman2->getLMConfigurationForFuzzJob(777); };
			Assert::ExpectException<std::invalid_argument>(f);
		}

		TEST_METHOD(GMDatabaseManager_addWorkerToDatabase)
		{
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mittelerde');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Auenland');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mordor');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('CheeseCakeFactory');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('9999-9999', 'System9', 21, (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 1, 'DefaultRunner');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('8888-8888', 'System8',22, (SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 2, 'DefaultEvaluator');");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "2");

			std::string testGUID2 = "7777-7777";
			std::string testHAP2 = "System7";
			FluffiServiceDescriptor sd1{ testHAP2 ,testGUID2 };

			// new worker
			Assert::AreEqual(dbman->addWorkerToDatabase(sd1, AgentType::TestcaseRunner, "DefaultRunner", std::string("Mordor")), true, L"Error Adding new Worker to Database: (addWorkerToDatabase)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "3");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Location from workers WHERE ServiceDescriptorGUID = '7777-7777'") == "3");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorHostAndPort from workers WHERE ServiceDescriptorGUID = '7777-7777'") == "System7");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT AgentType from workers WHERE ServiceDescriptorGUID = '7777-7777'") == "1");

			std::string testGUID3 = "7777-7777";
			std::string testHAP3 = "System7-2";
			FluffiServiceDescriptor sd2{ testHAP3 ,testGUID3 };

			// new worker with same GUID but other data   -> Overwrite
			Assert::AreEqual(dbman->addWorkerToDatabase(sd2, AgentType::TestcaseEvaluator, "DefaultEvaluator", std::string("CheeseCakeFactory")), true, L"Error Adding new Worker to Database: (addWorkerToDatabase)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "3");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Location from workers WHERE ServiceDescriptorGUID = '7777-7777'") == dbman->EXECUTE_TEST_STATEMENT("SELECT locations.ID FROM locations WHERE locations.Name = 'CheeseCakeFactory'"));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT AgentType from workers WHERE ServiceDescriptorGUID = '7777-7777'") == "2");

			std::string testGUID4 = "6666-6666";
			std::string testHAP4 = "System6";
			FluffiServiceDescriptor sd3{ testHAP4 ,testGUID4 };

			// new worker with new GUID but same data  -> Insert
			Assert::AreEqual(dbman->addWorkerToDatabase(sd3, AgentType::TestcaseEvaluator, "DefaultEvaluator", std::string("CheeseCakeFactory")), true, L"Error Adding new Worker to Database: (addWorkerToDatabase)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "4");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT Location from workers WHERE ServiceDescriptorGUID = '6666-6666'") == dbman->EXECUTE_TEST_STATEMENT("SELECT locations.ID FROM locations WHERE locations.Name = 'CheeseCakeFactory'"));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT AgentType from workers WHERE ServiceDescriptorGUID = '6666-6666'") == "2");
		}

		TEST_METHOD(GMDatabaseManager_removeWorkerFromDatabase)
		{
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mittelerde');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Auenland');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mordor');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('CheeseCakeFactory');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('9999-9999', 'System9', 21, (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 1, 'DefaultRunner');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('8888-8888', 'System8',22, (SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 2, 'DefaultEvaluator');");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "2");

			std::string testGUID1 = "9999-9999";
			std::string testHAP1 = "testhap1";
			FluffiServiceDescriptor sd1{ testHAP1 ,testGUID1 };

			//Removing an existing one should decrease the amount of workers
			Assert::IsTrue(dbman->removeWorkerFromDatabase(sd1), L"could not remove worker from database");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "1");

			//Removing an non-existing one shouldn't change anything
			Assert::IsTrue(dbman->removeWorkerFromDatabase(sd1), L"could not remove worker from database");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "1");
		}

		TEST_METHOD(GMDatabaseManager_setLMForLocation)
		{
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 21, '21','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 22, '22','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 23, '23','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mittelerde');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Auenland');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('1111-2222', '127.0.0.1:4444', (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('3333-4444', '127.0.0.1:5555', (SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from location_fuzzjobs") == "2");

			std::string testGUID1 = "1111-1111";
			std::string testHAP1 = "testhap1";
			FluffiServiceDescriptor sd1{ testHAP1 ,testGUID1 };

			// Update with new LM with same data -> Error, new GUID should not overwrite existing LM
			GMDatabaseManager* dbmanPtrLM = dbman;
			auto fx = [dbmanPtrLM, sd1] { dbmanPtrLM->setLMForLocationAndFuzzJob(std::string("Auenland"), sd1, 22); };
			Assert::ExpectException<std::runtime_error>(fx);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "2");

			// Existing LM
			GMDatabaseManager* dbmanPtr = dbman;
			auto f = [dbmanPtr, sd1] { dbmanPtr->setLMForLocationAndFuzzJob(std::string("Mordor"), sd1, 22); };
			Assert::ExpectException<std::runtime_error>(f);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "2");

			// Update new LM for new Job
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Auenland');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 23);");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from location_fuzzjobs") == "3");

			std::string testGUID2 = "2222-2222";
			std::string testHAP2 = "testhap1";
			FluffiServiceDescriptor sd2{ testHAP2 ,testGUID2 };
			Assert::AreEqual(dbman->setLMForLocationAndFuzzJob(std::string("Auenland"), sd2, 23), true, L"Error setting new LM for specific Location: (setLMForLocationAndFuzzJob)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "3");

			// Update new LM but Location does not exist
			std::string testGUID3 = "2222-2222";
			std::string testHAP3 = "testhap1";
			FluffiServiceDescriptor sd3{ testHAP3 ,testGUID3 };
			GMDatabaseManager* dbmanPtr2 = dbman;
			auto f2 = [dbmanPtr2, sd3] { dbmanPtr2->setLMForLocationAndFuzzJob(std::string("Auenland"), sd3, 24); };
			Assert::ExpectException<std::runtime_error>(f2);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "3");
		}

		TEST_METHOD(GMDatabaseManager_deleteManagedLMStatusOlderThanXSec)
		{
			// Build DB
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers_statuses (ServiceDescriptorGUID, TimeOfStatus, Status) VALUES ('1111-2222', CURRENT_TIMESTAMP(), 'Teststatus1');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers_statuses (ServiceDescriptorGUID, TimeOfStatus, Status) VALUES ('3333-4444', CURRENT_TIMESTAMP(), 'Teststatus2');");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "2");

			// Test Method
			Assert::AreEqual(dbman->deleteManagedLMStatusOlderThanXSec(1), true, L"Error deleting old LM status: (deleteManagedLMStatusOlderThanXSec)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "2");
			Sleep(1000);
			// Test Method
			Assert::AreEqual(dbman->deleteManagedLMStatusOlderThanXSec(0), true, L"Error deleting old LM status: (deleteManagedLMStatusOlderThanXSec)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers_statuses (ServiceDescriptorGUID, TimeOfStatus, Status) VALUES ('1111-2222', CURRENT_TIMESTAMP(), 'Teststatus1');");
			Sleep(2000);
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers_statuses (ServiceDescriptorGUID, TimeOfStatus, Status) VALUES ('3333-4444', CURRENT_TIMESTAMP(), 'Teststatus2');");
			// Test Method
			Assert::AreEqual(dbman->deleteManagedLMStatusOlderThanXSec(1), true, L"Error deleting old LM status: (deleteManagedLMStatusOlderThanXSec)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "1");
			// Test Method
			Sleep(3000);
			Assert::AreEqual(dbman->deleteManagedLMStatusOlderThanXSec(2), true, L"Error deleting old LM status: (deleteManagedLMStatusOlderThanXSec)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "0");
		}

		TEST_METHOD(GMDatabaseManager_deleteWorkersStatusOlderThanXSec)
		{
			// Build DB
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mittelerde');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Auenland');");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('1111-2222','System-1-2', 21, (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 1, 'DefaultRunner');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('3333-4444','System-3-4', 22, (SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 2, 'DefaultEvaluator');");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "2");

			// Test Method
			Assert::AreEqual(dbman->deleteWorkersNotSeenSinceXSec(1), true, L"Error deleting old worker status: (deleteWorkersNotSeenSinceXSec)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "2");
			Sleep(1000);
			// Test Method
			Assert::AreEqual(dbman->deleteWorkersNotSeenSinceXSec(0), true, L"Error deleting old worker status: (deleteWorkersNotSeenSinceXSec)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('1111-2222','System-1-2', 21, (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 1, 'DefaultRunner');");
			Sleep(2000);
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes) VALUES ('3333-4444','System-3-4', 22, (SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 2, 'DefaultEvaluator');");
			// Test Method
			Assert::AreEqual(dbman->deleteWorkersNotSeenSinceXSec(1), true, L"Error deleting old worker status: (deleteWorkersNotSeenSinceXSec)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "1");
			// Test Method
			Sleep(3000);
			Assert::AreEqual(dbman->deleteWorkersNotSeenSinceXSec(2), true, L"Error deleting old worker status: (deleteWorkersNotSeenSinceXSec)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from workers") == "0");
		}

		TEST_METHOD(GMDatabaseManager_getAllRegisteredLMs)
		{
			// Empty set
			// Test Method
			std::vector<std::pair<std::string, std::string>> lmSet = dbman->getAllRegisteredLMs();
			Assert::AreEqual(lmSet.size(), (size_t)0, L"Error getting all registered LMs: (getAllRegisteredLMs)");

			// Build DB
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 21, '21','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 22, '22','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 24, '24','','','','');");

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mittelerde');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Auenland');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 24);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('1111-2222', '127.0.0.1:4444', (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('3333-4444', '127.0.0.1:5555', (SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from location_fuzzjobs") == "3");

			// Test Method
			lmSet = dbman->getAllRegisteredLMs();
			Assert::AreEqual(lmSet.size(), (size_t)2, L"Error getting all registered LMs: (getAllRegisteredLMs)");

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('5555-6666', '127.0.0.1:4444', (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 24);");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "3");

			// Test Method
			lmSet = dbman->getAllRegisteredLMs();

			Assert::AreEqual(lmSet.back().first, std::string("5555-6666"), L"Error getting all registered LMs: (getAllRegisteredLMs)");
			Assert::AreEqual(lmSet.back().second, std::string("127.0.0.1:4444"), L"Error getting all registered LMs: (getAllRegisteredLMs)");
			lmSet.pop_back();
			Assert::AreEqual(lmSet.back().first, std::string("3333-4444"), L"Error getting all registered LMs: (getAllRegisteredLMs)");
			Assert::AreEqual(lmSet.back().second, std::string("127.0.0.1:5555"), L"Error getting all registered LMs: (getAllRegisteredLMs)");
			lmSet.pop_back();

			Assert::AreEqual(lmSet.size(), (size_t)1, L"Error getting all registered LMs: (getAllRegisteredLMs)");
		}

		TEST_METHOD(GMDatabaseManager_removeManagedLM)
		{
			// Empty DB
			Assert::AreEqual(dbman->removeManagedLM(std::string("1111-2222")), true, L"Error removing managed LM: (removeManagedLM)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "0");

			// Build DB
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 21, '21','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 22, '22','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 24, '24','','','','');");

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mittelerde');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Auenland');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 24);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('1111-2222', '127.0.0.1:4444', (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('3333-4444', '127.0.0.1:5555', (SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from location_fuzzjobs") == "3");

			// Delete not existing
			Assert::AreEqual(dbman->removeManagedLM(std::string("7897-2222")), true, L"Error removing managed LM: (removeManagedLM)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "2");

			// Delete existing
			Assert::AreEqual(dbman->removeManagedLM(std::string("1111-2222")), true, L"Error removing managed LM: (removeManagedLM)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "1");

			Assert::AreEqual(dbman->removeManagedLM(std::string("3333-4444")), true, L"Error removing managed LM: (removeManagedLM)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "0");
		}

		TEST_METHOD(GMDatabaseManager_addNewManagedLMStatus)
		{
			// Empty DB
			Assert::AreEqual(dbman->addNewManagedLMStatus(std::string("1111-2222"), std::string("Teststatus1")), true, L"Error adding new managed LM status: (addNewManagedLMStatus)");

			// Build DB
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "1");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers_statuses (ServiceDescriptorGUID, TimeOfStatus, Status) VALUES ('1111-2222', CURRENT_TIMESTAMP(), 'Teststatus1');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers_statuses (ServiceDescriptorGUID, TimeOfStatus, Status) VALUES ('3333-4444', CURRENT_TIMESTAMP(), 'Teststatus2');");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "3");

			// No GUID
			Assert::AreEqual(dbman->addNewManagedLMStatus(std::string(""), std::string("Teststatus1")), true, L"Error adding new managed LM status: (addNewManagedLMStatus)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "4");

			// Absolutely empty
			Assert::AreEqual(dbman->addNewManagedLMStatus(std::string(""), std::string("")), true, L"Error adding new managed LM status: (addNewManagedLMStatus)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "5");

			// Duplicate
			Assert::AreEqual(dbman->addNewManagedLMStatus(std::string(""), std::string("")), true, L"Error adding new managed LM status: (addNewManagedLMStatus)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_statuses") == "6");
		}

		TEST_METHOD(GMDatabaseManager_getFuzzJobWithoutLM)
		{
			// Empty DB
			long fuzzJob = dbman->getFuzzJobWithoutLM(std::string("Mittelerde"));
			Assert::AreEqual(fuzzJob, (long)-1, L"Error getting FuzzJob that has no LM in Database: (getFuzzJobWithoutLM)");

			// Build DB
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "0");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from locations") == "0");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 21, '21','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 22, '22','','','','');");

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mittelerde');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Auenland');");

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('1111-2222', '127.0.0.1:4444', (SELECT locations.ID FROM locations WHERE locations.Name = 'Mittelerde'), 21);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob) VALUES ('3333-4444', '127.0.0.1:5555', (SELECT locations.ID FROM locations WHERE locations.Name = 'Auenland'), 22);");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from locations") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from fuzzjob") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from location_fuzzjobs") == "2");

			// Full DB but no FuzzJob without LM
			fuzzJob = dbman->getFuzzJobWithoutLM(std::string("Mittelerde"));
			Assert::AreEqual(fuzzJob, (long)-1, L"Error getting FuzzJob that has no LM in Database: (getFuzzJobWithoutLM)");

			fuzzJob = dbman->getFuzzJobWithoutLM(std::string("Auenland"));
			Assert::AreEqual(fuzzJob, (long)-1, L"Error getting FuzzJob that has no LM in Database: (getFuzzJobWithoutLM)");

			// Insert FuzzJob without LM
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 24, '24','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO locations (Name) VALUES ('Mordor');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Mordor'), 24);");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from locations") == "3");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from location_fuzzjobs") == "3");

			// One FuzzJob without LM
			fuzzJob = dbman->getFuzzJobWithoutLM(std::string("Mordor"));
			Assert::AreEqual(fuzzJob, (long)24, L"Error getting FuzzJob that has no LM in Database: (getFuzzJobWithoutLM)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from locations") == "3");

			// Insert another FuzzJob without LM
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO fuzzjob (ID, name, DBHost,DBUser,DBPass,DBName) VALUES ( 25, '25','','','','');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO location_fuzzjobs (Location, Fuzzjob) VALUES ((SELECT locations.ID FROM locations WHERE locations.Name = 'Mordor'), 25);");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from locations") == "3");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from location_fuzzjobs") == "4");

			// Two FuzzJobs without LM
			fuzzJob = dbman->getFuzzJobWithoutLM(std::string("Mordor"));
			Assert::AreEqual(fuzzJob, (long)24, L"Error getting FuzzJob that has no LM in Database: (getFuzzJobWithoutLM)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from locations") == "3");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from location_fuzzjobs") == "4");
		}

		TEST_METHOD(GMDatabaseManager_getNewCommands)
		{
			// Empty DB
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE command_queue;");

			std::vector<std::tuple<int, std::string, std::string>> commands = dbman->getNewCommands();
			Assert::IsTrue(commands.empty(), L"Error fetching empty set of commands");

			// Simple tests
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (command, argument) VALUES ( 'asdf', 'qwer');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (command, argument) VALUES ( 'asdf2', '');");
			commands = dbman->getNewCommands();
			Assert::AreEqual((size_t)2, commands.size(), L"getNewCommands does not return all new commands");

			int id;
			std::string cmd, arg;
			tie(id, cmd, arg) = commands[0];
			Assert::AreEqual(std::string("asdf"), cmd, L"getNewCommands does not return expected cmd");
			Assert::AreEqual(std::string("qwer"), arg, L"getNewCommands does not return expected arg");

			tie(id, cmd, arg) = commands[1];
			Assert::AreEqual(std::string("asdf2"), cmd, L"getNewCommands does not return expected cmd");
			Assert::AreEqual(std::string(""), arg, L"getNewCommands does not return expected arg");

			// We shouldn't get the 'Done' ones
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (Command, Argument, Done) VALUES ( 'blue', '', 1);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (Command, Argument, Done) VALUES ( 'red', '', 1);");
			commands = dbman->getNewCommands();
			Assert::AreEqual((size_t)2, commands.size(), L"getNewCommands returns commands marked as Done");

			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE command_queue;");
		}

		TEST_METHOD(GMDatabaseManager_setCommandAsDone)
		{
			// Empty DB
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE command_queue;");

			// Simple tests
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (command, argument) VALUES ( 'asdf', 'qwer');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (command, argument) VALUES ( 'asdf2', '');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (Command, Argument, Done) VALUES ( 'blue', '', 0);");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (Command, Argument, Done) VALUES ( 'red', '', 0);");
			std::vector<std::tuple<int, std::string, std::string>>commands = dbman->getNewCommands();

			int id;
			std::string cmd, arg;
			tie(id, cmd, arg) = commands[0];
			Assert::IsTrue(dbman->setCommandAsDone(id, ""));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue WHERE Done=1") == "1");

			tie(id, cmd, arg) = commands[1];
			Assert::IsTrue(dbman->setCommandAsDone(id, "ASDF"));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue WHERE Done=1") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue WHERE Error='ASDF'") == "1");

			tie(id, cmd, arg) = commands[0];
			Assert::IsTrue(dbman->setCommandAsDone(id, ""));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue WHERE Done=1") == "2");

			tie(id, cmd, arg) = commands[1];
			Assert::IsTrue(dbman->setCommandAsDone(id, "ASDF"));
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue WHERE Done=1") == "2");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue WHERE Error='ASDF'") == "1");

			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE command_queue;");
		}

		TEST_METHOD(GMDatabaseManager_deleteDoneCommandsOlderThanXSec)
		{
			// Empty DB
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE command_queue;");

			// Simple tests
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (command, argument) VALUES ( 'asdf', 'qwer');");
			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO command_queue (command, argument) VALUES ( 'asdf2', '');");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue") == "2");
			Sleep(2200);

			//If no command is marked as done, none should be removed
			dbman->deleteDoneCommandsOlderThanXSec(1);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue") == "2");
			std::vector<std::tuple<int, std::string, std::string>>commands = dbman->getNewCommands();

			int id;
			std::string cmd, arg;
			tie(id, cmd, arg) = commands[0];
			Assert::IsTrue(dbman->setCommandAsDone(id, ""));

			//The first 'done' command should be removed
			dbman->deleteDoneCommandsOlderThanXSec(1);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue") == "1");

			tie(id, cmd, arg) = commands[1];
			Assert::IsTrue(dbman->setCommandAsDone(id, "ASDF"));

			//The second 'done' command should not yet be removed, as three second did not pass yet
			dbman->deleteDoneCommandsOlderThanXSec(3);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue") == "1");

			Sleep(2000);

			//The second 'done' command should be removed
			dbman->deleteDoneCommandsOlderThanXSec(3);
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from command_queue") == "0");

			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE command_queue;");
		}

		TEST_METHOD(GMDatabaseManager_addNewManagedLMLogMessages)
		{
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE localmanagers_logmessages;");

			std::string tooLongGUID(60, 'a');
			Assert::IsFalse(dbman->addNewManagedLMLogMessages(tooLongGUID, {}), L"Inserting a log message with a too long guid succeeded for some reason");

			std::string validGUID = "testguid";
			std::vector<std::string> invalidLogMessages;
			invalidLogMessages.push_back(std::string(2000, 'b'));
			Assert::IsFalse(dbman->addNewManagedLMLogMessages(validGUID, invalidLogMessages), L"Inserting a log message with a too long content succeeded for some reason");

			std::vector<std::string> validLogMessages;
			validLogMessages.push_back("a");
			validLogMessages.push_back("b");
			validLogMessages.push_back("c");
			Assert::IsTrue(dbman->addNewManagedLMLogMessages(validGUID, validLogMessages), L"Inserting new valid log messages failed");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_logmessages") == "3", L"We got more or less log messages than expected");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorGUID from localmanagers_logmessages WHERE ID=1") == "testguid", L"Invalid guid stored (1)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorGUID from localmanagers_logmessages WHERE ID=2") == "testguid", L"Invalid guid stored (2)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT ServiceDescriptorGUID from localmanagers_logmessages WHERE ID=3") == "testguid", L"Invalid guid stored (3)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from localmanagers_logmessages WHERE ID=1") == "a", L"Invalid log message stored (1)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from localmanagers_logmessages WHERE ID=2") == "b", L"Invalid log message stored (2)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from localmanagers_logmessages WHERE ID=3") == "c", L"Invalid log message stored (3)");

			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE localmanagers_logmessages;");
		}
		TEST_METHOD(GMDatabaseManager_deleteManagedLMLogMessagesIfMoreThan)
		{
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE localmanagers_logmessages;");

			std::string validGUID = "testguid";
			std::vector<std::string> validLogMessages;
			validLogMessages.push_back("a");
			validLogMessages.push_back("b");
			validLogMessages.push_back("c");
			Assert::IsTrue(dbman->addNewManagedLMLogMessages(validGUID, validLogMessages), L"Inserting new valid log messages failed");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_logmessages") == "3", L"We got more or less log messages than expected (1)");

			Assert::IsTrue(dbman->deleteManagedLMLogMessagesIfMoreThan(3), L"Calling deleteManagedLMLogMessagesIfMoreThan failed (1)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_logmessages") == "3", L"We got more or less log messages than expected (2)");

			Assert::IsTrue(dbman->deleteManagedLMLogMessagesIfMoreThan(2), L"Calling deleteManagedLMLogMessagesIfMoreThan failed (2)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_logmessages") == "2", L"We got more or less log messages than expected (3)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from localmanagers_logmessages WHERE ID=2") == "b", L"Invalid log message stored (2)");
			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT LogMessage from localmanagers_logmessages WHERE ID=3") == "c", L"Invalid log message stored (3)");

			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE localmanagers_logmessages;");
		}
		TEST_METHOD(GMDatabaseManager_deleteManagedLMLogMessagesOlderThanXSec)
		{
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE localmanagers_logmessages;");

			std::string validGUID = "testguid";
			std::vector<std::string> validLogMessages;
			validLogMessages.push_back("a");
			validLogMessages.push_back("b");
			validLogMessages.push_back("c");
			Assert::IsTrue(dbman->addNewManagedLMLogMessages(validGUID, validLogMessages), L"Inserting new valid log messages failed (1)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_logmessages") == "3", L"We got more or less log messages than expected (1)");

			Assert::IsTrue(dbman->deleteManagedLMLogMessagesOlderThanXSec(5), L"Calling deleteManagedLMLogMessagesOlderThanXSec failed (1)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_logmessages") == "3", L"We got more or less log messages than expected (2)");

			Sleep(2000);

			Assert::IsTrue(dbman->addNewManagedLMLogMessages(validGUID, validLogMessages), L"Inserting new valid log messages failed (2)");

			Assert::IsTrue(dbman->deleteManagedLMLogMessagesOlderThanXSec(5), L"Calling deleteManagedLMLogMessagesOlderThanXSec failed (2)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_logmessages") == "6", L"We got more or less log messages than expected (3)");

			Sleep(4000);

			Assert::IsTrue(dbman->deleteManagedLMLogMessagesOlderThanXSec(5), L"Calling deleteManagedLMLogMessagesOlderThanXSec failed (3)");

			Assert::IsTrue(dbman->EXECUTE_TEST_STATEMENT("SELECT COUNT(*) from localmanagers_logmessages") == "3", L"We got more or less log messages than expected (4)");

			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE localmanagers_logmessages;");
		}

	private:

		const std::string testdbUser = "root";
		const std::string testdbPass = "toor";
		const std::string testdbHost = "fluffiLMDBHost";
		const std::string testdbName = "fluffi_gm_test";

		GMDatabaseManager* setupTestDB() {
			std::ifstream createDatabaseFile("..\\..\\..\\srv\\fluffi\\data\\fluffiweb\\app\\sql_files\\createGMDB.sql");
			std::string dbCreateFileContent;

			createDatabaseFile.seekg(0, std::ios::end);
			dbCreateFileContent.reserve(createDatabaseFile.tellg());
			createDatabaseFile.seekg(0, std::ios::beg);

			dbCreateFileContent.assign((std::istreambuf_iterator<char>(createDatabaseFile)),
				std::istreambuf_iterator<char>());

			//adapt database name
			Util::replaceAll(dbCreateFileContent, "fluffi_gm", testdbName);

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

			GMDatabaseManager* dbman = new GMDatabaseManager();
			GMDatabaseManager::setDBConnectionParameters(testdbHost, testdbUser, testdbPass, "information_schema");

			dbman->EXECUTE_TEST_STATEMENT("DROP DATABASE IF EXISTS " + testdbName);
			dbman->EXECUTE_TEST_STATEMENT("CREATE DATABASE IF NOT EXISTS fluffi_gm;");

			dbman->EXECUTE_TEST_STATEMENT("USE " + testdbName);

			for (auto const& command : SQLcommands) {
				if (ltrim_copy(command).substr(0, 6) == "CREATE") {
					dbman->EXECUTE_TEST_STATEMENT(command);
				}
			}

			dbman->EXECUTE_TEST_STATEMENT("USE " + testdbName);
			GMDatabaseManager::setDBConnectionParameters(testdbHost, testdbUser, testdbPass, testdbName);

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