/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Pascal Eckmann, Abian Blome
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "LMDatabaseManager.h"
#include "GarbageCollectorWorker.h"
#include "RegisterAtLMRequestHandler.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace RegisterAtLMRequestHandlerTester
{
	TEST_CLASS(RegisterAtLMRequestHandlerTest)
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

		TEST_METHOD(RegisterAtLMRequestHandler_decideSubAgentType)
		{
			//-----------------------'TypeA=50|TypeB=50'---------------------------
			dbman->EXECUTE_TEST_STATEMENT("REPLACE INTO settings (SettingName,SettingValue) VALUES ('generatorTypes','TypeA=50|TypeB=50')");

			//No Agents running. I can do TypeA
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeA");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "TypeA", L"I can only do TypeA. TypeA should have been returned");
			}

			//No Agents running. I can do TypeB
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeB");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "TypeB", L"I can only do TypeB. TypeB should have been returned");
			}

			//No Agents running. I can do TypeA AND TypeB
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeA");
				implementedSubtypes.Add("TypeB");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "TypeA", L"I can do TypeA and TypeB. TypeA should have been returned");
			}

			//No Agents running. I can do TypeC
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeC");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "", L"I can only do TypeC. This should return in an empty result");
			}

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO managed_instances (ServiceDescriptorGUID,ServiceDescriptorHostAndPort,AgentType,Location,AgentSubType) VALUES ('GUID1','HAP1',1,'home','TypeA')");

			//One TypeA agent running. I can do TypeA
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeA");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "TypeA", L"I can only do TypeA. TypeA should have been returned (2)");
			}

			//One TypeA agent running. I can do TypeB
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeB");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "TypeB", L"I can only do TypeB. TypeB should have been returned (2)");
			}

			//One TypeA agent running. I can do TypeA AND TypeB
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeA");
				implementedSubtypes.Add("TypeB");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "TypeB", L"I can do TypeA and TypeB. TypeB should have been returned");
			}

			//One TypeA agent running. I can do TypeC
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeC");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "", L"I can only do TypeC. This should return in an empty result (2)");
			}

			//-----------------------'TypeA=30|TypeB=70'---------------------------
			dbman->EXECUTE_TEST_STATEMENT("TRUNCATE TABLE managed_instances");
			dbman->EXECUTE_TEST_STATEMENT("REPLACE INTO settings (SettingName,SettingValue) VALUES ('generatorTypes','TypeA=30|TypeB=70')");

			//No Agents running. I can do TypeA AND TypeB
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeA");
				implementedSubtypes.Add("TypeB");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "TypeB", L"I can do TypeA and TypeB. TypeB should have been returned(3)");
			}

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO managed_instances (ServiceDescriptorGUID,ServiceDescriptorHostAndPort,AgentType,Location,AgentSubType) VALUES ('GUID1','HAP1',1,'home','TypeB')");

			//One TypeB running. I can do TypeA AND TypeB
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeA");
				implementedSubtypes.Add("TypeB");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "TypeA", L"I can do TypeA and TypeB. TypeA should have been returned(4)");
			}

			dbman->EXECUTE_TEST_STATEMENT("INSERT INTO managed_instances (ServiceDescriptorGUID,ServiceDescriptorHostAndPort,AgentType,Location,AgentSubType) VALUES ('GUID2','HAP2',1,'home','TypeA')");

			//One TypeA, One TypeB running. I can do TypeA AND TypeB
			{
				AgentType type = AgentType::TestcaseGenerator;
				google::protobuf::RepeatedPtrField< std::string > implementedSubtypes;
				implementedSubtypes.Add("TypeA");
				implementedSubtypes.Add("TypeB");
				std::string re = RegisterAtLMRequestHandler::decideSubAgentType(dbman, type, implementedSubtypes);
				Assert::IsTrue(re == "TypeB", L"I can do TypeA and TypeB. TypeB should have been returned(5)");
			}
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
