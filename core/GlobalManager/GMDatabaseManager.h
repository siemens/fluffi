/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier
*/

§§#pragma once
§§
class FluffiLMConfiguration;
class FluffiServiceDescriptor;
§§class GMDatabaseManager
§§{
§§public:
§§	GMDatabaseManager();
§§	~GMDatabaseManager();
§§
§§	static void setDBConnectionParameters(std::string host, std::string user, std::string pwd, std::string db);
	FluffiServiceDescriptor getLMServiceDescriptorForWorker(FluffiServiceDescriptor fsd);
§§	FluffiLMConfiguration getLMConfigurationForFuzzJob(int fuzzJob);
	bool addWorkerToDatabase(FluffiServiceDescriptor fsd, AgentType type, std::string subtypes, std::string location);
	bool setLMForLocationAndFuzzJob(std::string location, FluffiServiceDescriptor lmServiceDescriptor, long fuzzjob);
§§	bool deleteManagedLMStatusOlderThanXSec(int olderThanInSeconds);
	bool deleteWorkersNotSeenSinceXSec(int olderThanInSeconds);
	bool deleteDoneCommandsOlderThanXSec(int olderThanInSeconds);
§§	std::vector<std::pair<std::string, std::string>> getAllRegisteredLMs();
§§	bool removeManagedLM(std::string ServiceDescriptorGUID);
	bool removeWorkerFromDatabase(FluffiServiceDescriptor fsd);
§§	bool addNewManagedLMStatus(std::string ServiceDescriptorGUID, std::string newStatus);
§§	long getFuzzJobWithoutLM(std::string location);
§§	std::vector<std::tuple<int, std::string, std::string>> getNewCommands();
§§	bool setCommandAsDone(int commandID, std::string errorMessage);
§§
§§#ifdef _VSTEST
§§	std::string EXECUTE_TEST_STATEMENT(const std::string query);
§§#endif // _VSTEST
§§
§§private:
§§	static MYSQL* establishDBConnection();
§§	static bool setSessionParameters(MYSQL* conn);
§§
§§	MYSQL* m_DBConnection;
§§
§§	MYSQL* getDBConnection();
§§
§§	static std::string s_dbHost;
§§	static std::string s_dbUser;
§§	static std::string s_dbPassword;
§§	static std::string s_dbName;

	//Performance watching
§§	std::chrono::steady_clock::time_point performanceWatchTimePoint;
§§#define PERFORMANCE_WATCH_FUNCTION_ENTRY performanceWatchTimePoint = std::chrono::steady_clock::now();
§§#define PERFORMANCE_WATCH_FUNCTION_EXIT(X)  	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - performanceWatchTimePoint).count()  > CommInt::timeoutNormalMessage/2) {		LOG(WARNING) << X << " is taking dangerously long to respond: "<< std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - performanceWatchTimePoint).count() ;	}
§§};
