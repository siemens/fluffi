/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Fabian Russwurm, Pascal Eckmann
*/

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#else
#define gmtime_s(a,b) gmtime_r(b,a)
#define _mkgmtime timegm
#define _strtoui64 strtoull
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define strcpy_s(a,b,c) strlcpy(a,c,b)
#endif

class StatusOfInstance;
class FluffiServiceDescriptor;
class FluffiSetting;
class FluffiTestcaseID;
class FluffiModuleNameToID;
class GarbageCollectorWorker;
class FluffiBasicBlock;
class LMDatabaseManager
{
public:
	LMDatabaseManager(GarbageCollectorWorker* garbageCollectorWorker);
	~LMDatabaseManager();

	static void setDBConnectionParameters(std::string host, std::string user, std::string pwd, std::string db);

	//Instance management
	bool addNewManagedInstanceStatus(std::string ServiceDescriptorGUID, std::string newStatus);
§§	std::vector<FluffiServiceDescriptor> getRegisteredInstancesOfAgentType(AgentType type, std::string location);
	std::string getRegisteredInstanceSubType(std::string ServiceDescriptorGUID);
	std::vector<std::pair<std::string, int>> getRegisteredInstancesNumOfSubTypes();
	std::vector<std::pair<FluffiServiceDescriptor, AgentType>> getAllRegisteredInstances(std::string location);
	bool removeManagedInstance(std::string ServiceDescriptorGUID, std::string location);
	bool deleteManagedInstanceStatusOlderThanXSec(int olderThanInSeconds);
	bool writeManagedInstance(const FluffiServiceDescriptor serviceDescriptor, int type, std::string subtype, std::string location);
	std::vector<StatusOfInstance> getStatusOfManagedInstances(std::string location);

	//Testcase management
	enum TestCaseType {
		Population = 0,
		Hang = 1,
		Exception_AccessViolation = 2,
		Exception_Other = 3,
		NoResponse = 4,
		Inactive_Population = 5,
		Locked = 6
	};
	static const int defaultInitialPopulationRating = 10000;

	bool addBlocksToCoveredBlocks(const FluffiTestcaseID tcID, const std::set<FluffiBasicBlock>* basicBlocks); //in order to prevent deadlocks, the basic blocks need to be inserted in a sorted order. Therefore a set (which is soted) is used to pass the basicBlocks
	bool addDeltaToTestcaseRating(const FluffiTestcaseID tcID, int ratingDelta);
	bool addEntriesToCompletedTestcasesTable(const std::set<FluffiTestcaseID>* testcaseIDs);
	bool addEntryToCompletedTestcasesTable(const FluffiTestcaseID tcID);
	bool addEntryToCrashDescriptionsTable(const FluffiTestcaseID tcID, const std::string crashFootprint);
	bool addEntryToInterestingTestcasesTable(const FluffiTestcaseID tcID, const FluffiTestcaseID tcparentID, int rating, const std::string testcaseDir, TestCaseType tcType);

	bool dropTestcaseTypeIFMoreThan(LMDatabaseManager::TestCaseType type, int instances);
	bool dropTestcaseIfCrashFootprintAppearedMoreThanXTimes(int times, std::string crashFootprint);
	std::deque<std::string> getAllCrashFootprints();

	GetCurrentBlockCoverageResponse* generateGetCurrentBlockCoverageResponse();
	GetNewCompletedTestcaseIDsResponse* generateGetNewCompletedTestcaseIDsResponse(time_t lastUpdateTimeStamp);
	GetTestcaseToMutateResponse* generateGetTestcaseToMutateResponse(std::string testcaseTempDir, int ratingAdaption);

	bool setTestcaseType(const FluffiTestcaseID tcID, TestCaseType tcType);

	//Configuration
	std::deque<FluffiSetting> getAllSettings();
	std::deque<FluffiModuleNameToID> getTargetModules();
	std::deque<FluffiBasicBlock> getTargetBlocks();

	//Misc
	bool initializeBillingTable();
	bool addRunnerSeconds(unsigned int secondsToAdd);
§§	bool addRunTestcasesNoLongerListed(unsigned int numberOfTestcasesNoLongerListed);
	unsigned long long cleanCompletedTestcasesTableOlderThanXSec(int olderThanInSeconds);

#ifdef _VSTEST
	std::string EXECUTE_TEST_STATEMENT(const std::string query);
#endif // _VSTEST

private:
	static MYSQL* establishDBConnection();
	static bool setSessionParameters(MYSQL* conn);

	MYSQL* m_DBConnection;
	GarbageCollectorWorker* m_garbageCollectorWorker;

	MYSQL* getDBConnection();

	static std::string s_dbHost;
	static std::string s_dbUser;
	static std::string s_dbPassword;
	static std::string s_dbName;

	//Performance watching
	std::chrono::steady_clock::time_point performanceWatchTimePoint;
#define PERFORMANCE_WATCH_FUNCTION_ENTRY performanceWatchTimePoint = std::chrono::steady_clock::now();
#define PERFORMANCE_WATCH_FUNCTION_EXIT(X)  if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - performanceWatchTimePoint).count()  > CommInt::timeoutNormalMessage/2) {		LOG(WARNING) << X << " is taking dangerously long to respond: "<< std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - performanceWatchTimePoint).count() ;	}
};
