/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Fabian Russwurm, Pascal Eckmann
*/

#include "stdafx.h"
#include "LMDatabaseManager.h"
#include "FluffiTestcaseID.h"
#include "CommInt.h"
#include "Util.h"
#include "StatusOfInstance.h"
#include "FluffiSetting.h"
#include "FluffiModuleNameToID.h"
#include "FluffiBasicBlock.h"
#include "GarbageCollectorWorker.h"

//Init static members
std::string LMDatabaseManager::s_dbHost = "";
std::string LMDatabaseManager::s_dbUser = "";
std::string LMDatabaseManager::s_dbPassword = "";
std::string LMDatabaseManager::s_dbName = "";

LMDatabaseManager::LMDatabaseManager(GarbageCollectorWorker* garbageCollectorWorker) :
	m_DBConnection(nullptr),
	m_garbageCollectorWorker(garbageCollectorWorker),
	performanceWatchTimePoint(std::chrono::steady_clock::now())
{
	if (!mysql_thread_safe()) {
		LOG(ERROR) << "The maria db connector was not compiled in a thread safe way! This won't work!";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy
	}
}

LMDatabaseManager::~LMDatabaseManager()
{
	if (m_DBConnection != nullptr) {
		mysql_close(m_DBConnection);
		m_DBConnection = nullptr;
	}

	mysql_thread_end();
}

void LMDatabaseManager::setDBConnectionParameters(std::string host, std::string user, std::string pwd, std::string db) {
	s_dbHost = host;
	s_dbUser = user;
	s_dbPassword = pwd;
	s_dbName = db;
}

MYSQL* LMDatabaseManager::establishDBConnection() {
	MYSQL* re = mysql_init(NULL);

	my_bool reconnect = true;
	mysql_options(re, MYSQL_OPT_RECONNECT, &reconnect);
	mysql_options(re, MYSQL_SET_CHARSET_NAME, "utf8");

	if (!mysql_real_connect(re, s_dbHost.c_str(), s_dbUser.c_str(), s_dbPassword.c_str(), s_dbName.c_str(), 3306, "", 0))
	{
		LOG(ERROR) << "Failed to connect to database";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy
	}
	else
	{
		LOG(DEBUG) << "Thread " << std::this_thread::get_id() << " connected successful to database " << re->db << " with version " << re->server_version << " at host " << re->host_info;
	}

	setSessionParameters(re);

	return re;
}

MYSQL* LMDatabaseManager::getDBConnection() {
	if (m_DBConnection == nullptr) {
		m_DBConnection = establishDBConnection();
	}

	return m_DBConnection;
}

#ifdef _VSTEST
std::string LMDatabaseManager::EXECUTE_TEST_STATEMENT(const std::string query) {
	if (mysql_query(getDBConnection(), query.c_str()) != 0) {
		return "";
	}

	MYSQL_RES* result = mysql_store_result(getDBConnection());
	if (result == NULL) {
		return "";
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	if (row == NULL) {
		mysql_free_result(result);
		return "";
	}

	std::string re = row[0];
	mysql_free_result(result);

	return re;
}
#endif // _VSTEST

bool LMDatabaseManager::writeManagedInstance(const FluffiServiceDescriptor serviceDescriptor, int type, std::string subtype, std::string location)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedType = type;
	const char* cStrServiceDescriptorGUID = serviceDescriptor.m_guid.c_str();
	unsigned long GUIDLength = static_cast<unsigned long>(serviceDescriptor.m_guid.length());

	const char* cStrServiceDescriptorHAP = serviceDescriptor.m_serviceHostAndPort.c_str();
	unsigned long HAPLength = static_cast<unsigned long>(serviceDescriptor.m_serviceHostAndPort.length());

	const char* cSubtype = subtype.c_str();
	unsigned long subtypeLength = static_cast<unsigned long>(subtype.length());

	int preparedType2 = type;

	const char* cStrServiceDescriptorHAP2 = serviceDescriptor.m_serviceHostAndPort.c_str();
	unsigned long HAPLength2 = static_cast<unsigned long>(serviceDescriptor.m_serviceHostAndPort.length());

	const char* cSubtype2 = subtype.c_str();
	unsigned long subtypeLength2 = static_cast<unsigned long>(subtype.length());

	const char* cStrLocation2 = location.c_str();
	unsigned long locationLength2 = static_cast<unsigned long>(location.length());

	{
		//// prepared Statement to insert the managed instance into the managed_instances table
		MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

		const char* stmt = "INSERT INTO managed_instances (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, AgentType, AgentSubType, Location) values (?, ? , ?, ?, ?) ON DUPLICATE KEY UPDATE ServiceDescriptorHostAndPort  = ?, AgentType  = ?, AgentSubType  = ?, Location = ?";
		mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

		//params
		MYSQL_BIND bind[9];
		memset(bind, 0, sizeof(bind));

		bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[0].buffer = const_cast<char*>(cStrServiceDescriptorGUID);
		bind[0].buffer_length = GUIDLength;
		bind[0].length = &GUIDLength;

		bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[1].buffer = const_cast<char*>(cStrServiceDescriptorHAP);
		bind[1].buffer_length = HAPLength;
		bind[1].length = &HAPLength;

		bind[2].buffer_type = MYSQL_TYPE_LONG;
		bind[2].buffer = &preparedType;
		bind[2].is_null = 0;
		bind[2].is_unsigned = true;
		bind[2].length = NULL;

		bind[3].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[3].buffer = const_cast<char*>(cSubtype);
		bind[3].buffer_length = subtypeLength;
		bind[3].length = &subtypeLength;

		bind[4].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[4].buffer = const_cast<char*>(cStrLocation2);
		bind[4].buffer_length = locationLength2;
		bind[4].length = &locationLength2;

		bind[5].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[5].buffer = const_cast<char*>(cStrServiceDescriptorHAP2);
		bind[5].buffer_length = HAPLength2;
		bind[5].length = &HAPLength2;

		bind[6].buffer_type = MYSQL_TYPE_LONG;
		bind[6].buffer = &preparedType2;
		bind[6].is_null = 0;
		bind[6].is_unsigned = true;
		bind[6].length = NULL;

		bind[7].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[7].buffer = const_cast<char*>(cSubtype2);
		bind[7].buffer_length = subtypeLength2;
		bind[7].length = &subtypeLength2;

		bind[8].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[8].buffer = const_cast<char*>(cStrLocation2);
		bind[8].buffer_length = locationLength2;
		bind[8].length = &locationLength2;

		mysql_stmt_bind_param(sql_stmt, bind);
		bool re = mysql_stmt_execute(sql_stmt) == 0;
		if (!re) {
			LOG(ERROR) << "writeManagedInstance encountered the following error (1): " << mysql_stmt_error(sql_stmt);
		}

		mysql_stmt_close(sql_stmt);

		if (!re) {
			return false;
		}
	}

	{
		//// prepared Statement to insert to store the nice name of the instance (for gui and evaluation purposes)
		MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

		const char* stmt = "INSERT IGNORE INTO nice_names_managed_instance (ServiceDescriptorGUID,NiceName ) SELECT ?, CONCAT(?, COUNT(*)) FROM nice_names_managed_instance FOR UPDATE";
		mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

		//params
		MYSQL_BIND bind[2];
		memset(bind, 0, sizeof(bind));

		bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[0].buffer = const_cast<char*>(cStrServiceDescriptorGUID);
		bind[0].buffer_length = GUIDLength;
		bind[0].length = &GUIDLength;

		bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[1].buffer = const_cast<char*>(cSubtype);
		bind[1].buffer_length = subtypeLength;
		bind[1].length = &subtypeLength;

		mysql_stmt_bind_param(sql_stmt, bind);
		bool re = mysql_stmt_execute(sql_stmt) == 0;
		if (!re) {
			LOG(ERROR) << "writeManagedInstance encountered the following error (2): " << mysql_stmt_error(sql_stmt);
		}

		mysql_stmt_close(sql_stmt);

		if (!re) {
			return false;
		}
	}

	PERFORMANCE_WATCH_FUNCTION_EXIT("writeManagedInstance")
		return true;
}

GetNewCompletedTestcaseIDsResponse* LMDatabaseManager::generateGetNewCompletedTestcaseIDsResponse(time_t lastUpdateTimeStamp)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		GetNewCompletedTestcaseIDsResponse* re = new GetNewCompletedTestcaseIDsResponse();

	struct tm structCurTime;
	gmtime_s(&structCurTime, &lastUpdateTimeStamp);
	MYSQL_TIME timeStamp;
	memset(&timeStamp, 0, sizeof(timeStamp));
	timeStamp.year = structCurTime.tm_year + 1900;
	timeStamp.month = structCurTime.tm_mon + 1;
	timeStamp.day = structCurTime.tm_mday;
	timeStamp.hour = structCurTime.tm_hour;
	timeStamp.minute = structCurTime.tm_min;
	timeStamp.second = structCurTime.tm_sec;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	// fetch local IDs and TimeOFCompletion to get new timestamp without race condition
	const char* stmt = "SELECT CreatorServiceDescriptorGUID, CreatorLocalID, TimeOfCompletion FROM completed_testcases WHERE TimeOfCompletion >= ?"; //>= in order to avoid that some do not appear at all (drawback - some will appear twice)
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];

	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_TIMESTAMP;
	bind[0].buffer = &timeStamp;
	bind[0].is_null = 0;
	bind[0].length = 0;

	mysql_stmt_bind_param(sql_stmt, bind);
	if (mysql_stmt_execute(sql_stmt) != 0) {
		LOG(ERROR) << "generateGetNewCompletedTestcaseIDsResponse encountered the following error: " << mysql_stmt_error(sql_stmt);
		mysql_stmt_close(sql_stmt);
		return re;
	}

	MYSQL_TIME outputTimeStamp;
	long long int	creatorLocalID;

	MYSQL_BIND resultBIND[3];
	memset(resultBIND, 0, sizeof(resultBIND));

	unsigned long lineLengthCol0 = 0;

	resultBIND[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[1].buffer_type = MYSQL_TYPE_LONGLONG;
	resultBIND[2].buffer_type = MYSQL_TYPE_TIMESTAMP;

	resultBIND[0].length = &lineLengthCol0;

	resultBIND[1].buffer = &(creatorLocalID);
	resultBIND[1].buffer_length = sizeof(creatorLocalID);

	resultBIND[2].buffer = &(outputTimeStamp);
	resultBIND[2].buffer_length = sizeof(outputTimeStamp);

	mysql_stmt_bind_result(sql_stmt, resultBIND);

	mysql_stmt_store_result(sql_stmt);
	unsigned long long resultRows = mysql_stmt_num_rows(sql_stmt);

	time_t latestTimeSTamp = 0;

	for (unsigned long long i = 0; i < resultRows; i++) {
		mysql_stmt_fetch(sql_stmt);

		char* responseCol0 = new char[lineLengthCol0 + 1];
		memset(responseCol0, 0, lineLengthCol0 + 1);

		resultBIND[0].buffer = responseCol0;
		resultBIND[0].buffer_length = lineLengthCol0;

		mysql_stmt_fetch_column(sql_stmt, &resultBIND[0], 0, 0);

		struct tm timeinfo = tm();

		timeinfo.tm_year = outputTimeStamp.year - 1900;
		timeinfo.tm_mon = outputTimeStamp.month - 1;
		timeinfo.tm_mday = outputTimeStamp.day;
		timeinfo.tm_hour = outputTimeStamp.hour;
		timeinfo.tm_min = outputTimeStamp.minute;
		timeinfo.tm_sec = outputTimeStamp.second;
		timeinfo.tm_isdst = -1;

		time_t newTimestamp = _mkgmtime(&timeinfo);

		if (latestTimeSTamp == 0)
		{
			latestTimeSTamp = newTimestamp;
			LOG(DEBUG) << "Setting initial timestamp";
		}
		else if (difftime(newTimestamp, latestTimeSTamp) > 0.0)
		{
			latestTimeSTamp = newTimestamp;
			LOG(DEBUG) << "Found newer timestamp";
		}
		else
		{
			LOG(DEBUG) << "Timestamp not newer";
		}

		ServiceDescriptor* creatorSd = new ServiceDescriptor();
		creatorSd->set_guid(responseCol0);
		//the service's hostandPort is unknown and therefore not set (and irrelevant anyways)

		TestcaseID* tcID = re->add_ids();
		tcID->set_allocated_servicedescriptor(creatorSd);
		tcID->set_localid(creatorLocalID);

		delete[] responseCol0;
	}

	mysql_stmt_free_result(sql_stmt);
	mysql_stmt_close(sql_stmt);

	re->set_updatetimestamp(latestTimeSTamp);

	PERFORMANCE_WATCH_FUNCTION_EXIT("generateGetNewCompletedTestcaseIDsResponse")
		return re;
}

std::vector<std::pair<FluffiServiceDescriptor, AgentType>> LMDatabaseManager::getAllRegisteredInstances(std::string location)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::vector<std::pair<FluffiServiceDescriptor, AgentType>> re;

	const char* cStrLocation = location.c_str();
	unsigned long locationLength = static_cast<unsigned long>(location.length());

	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "SELECT  ServiceDescriptorHostAndPort, ServiceDescriptorGUID, AgentType FROM managed_instances WHERE Location = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrLocation);
	bind[0].buffer_length = locationLength;
	bind[0].length = &locationLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	if (mysql_stmt_execute(sql_stmt) != 0) {
		LOG(ERROR) << "getAllRegisteredInstances encountered the following error: " << mysql_stmt_error(sql_stmt);
		mysql_stmt_close(sql_stmt);
		return re;
	}

	MYSQL_BIND resultBIND[3];
	memset(resultBIND, 0, sizeof(resultBIND));

	unsigned long lineLengthCol0 = 0;
	unsigned long lineLengthCol1 = 0;

	resultBIND[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[2].buffer_type = MYSQL_TYPE_LONG;

	resultBIND[0].length = &lineLengthCol0;
	resultBIND[1].length = &lineLengthCol1;

	int outputAgentType;

	resultBIND[2].buffer = &(outputAgentType);
	resultBIND[2].buffer_length = sizeof(outputAgentType);

	mysql_stmt_bind_result(sql_stmt, resultBIND);

	mysql_stmt_store_result(sql_stmt);
	unsigned long long resultRows = mysql_stmt_num_rows(sql_stmt);
	for (unsigned long long i = 0; i < resultRows; i++) {
		mysql_stmt_fetch(sql_stmt);

		char* responseCol0 = new char[lineLengthCol0 + 1];
		memset(responseCol0, 0, lineLengthCol0 + 1);

		char* responseCol1 = new char[lineLengthCol1 + 1];
		memset(responseCol1, 0, lineLengthCol1 + 1);

		resultBIND[0].buffer = responseCol0;
		resultBIND[1].buffer = responseCol1;

		resultBIND[0].buffer_length = lineLengthCol0;
		resultBIND[1].buffer_length = lineLengthCol1;

		mysql_stmt_fetch_column(sql_stmt, &resultBIND[0], 0, 0);
		mysql_stmt_fetch_column(sql_stmt, &resultBIND[1], 1, 0);

		std::pair<FluffiServiceDescriptor, AgentType> newPair(FluffiServiceDescriptor(responseCol0, responseCol1), static_cast<AgentType>(outputAgentType));
		re.push_back(newPair);

		delete[] responseCol0;
		delete[] responseCol1;
	}

	mysql_stmt_free_result(sql_stmt);
	mysql_stmt_close(sql_stmt);
	PERFORMANCE_WATCH_FUNCTION_EXIT("getAllRegisteredInstances")
		return re;
}

std::vector<FluffiServiceDescriptor> LMDatabaseManager::getRegisteredInstancesOfAgentType(AgentType type, std::string location)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::vector<FluffiServiceDescriptor> re;
	int preparedType = type;

	const char* cStrLocation = location.c_str();
	unsigned long locationLength = static_cast<unsigned long>(location.length());

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "SELECT ServiceDescriptorHostAndPort,  ServiceDescriptorGUID FROM managed_instances WHERE AgentType = ? AND Location = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedType;
	bind[0].is_null = 0;
	bind[0].is_unsigned = true;
	bind[0].length = NULL;

	bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[1].buffer = const_cast<char*>(cStrLocation);
	bind[1].buffer_length = locationLength;
	bind[1].length = &locationLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	if (mysql_stmt_execute(sql_stmt) != 0) {
		LOG(ERROR) << "getRegisteredInstancesOfAgentType encountered the following error: " << mysql_stmt_error(sql_stmt);
		mysql_stmt_close(sql_stmt);
		return re;
	}

	MYSQL_BIND resultBIND[2];
	memset(resultBIND, 0, sizeof(resultBIND));

	unsigned long lineLengthCol0 = 0;
	unsigned long lineLengthCol1 = 0;

	resultBIND[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[1].buffer_type = MYSQL_TYPE_VAR_STRING;

	resultBIND[0].length = &lineLengthCol0;
	resultBIND[1].length = &lineLengthCol1;

	mysql_stmt_bind_result(sql_stmt, resultBIND);

	mysql_stmt_store_result(sql_stmt);
	unsigned long long resultRows = mysql_stmt_num_rows(sql_stmt);
	for (unsigned long long i = 0; i < resultRows; i++) {
		mysql_stmt_fetch(sql_stmt);

		char* responseCol0 = new char[lineLengthCol0 + 1];
		memset(responseCol0, 0, lineLengthCol0 + 1);

		char* responseCol1 = new char[lineLengthCol1 + 1];
		memset(responseCol1, 0, lineLengthCol1 + 1);

		resultBIND[0].buffer = responseCol0;
		resultBIND[1].buffer = responseCol1;

		resultBIND[0].buffer_length = lineLengthCol0;
		resultBIND[1].buffer_length = lineLengthCol1;

		mysql_stmt_fetch_column(sql_stmt, &resultBIND[0], 0, 0);
		mysql_stmt_fetch_column(sql_stmt, &resultBIND[1], 1, 0);

		re.push_back(FluffiServiceDescriptor(responseCol0, responseCol1));

		delete[] responseCol0;
		delete[] responseCol1;
	}

	mysql_stmt_free_result(sql_stmt);
	mysql_stmt_close(sql_stmt);
	PERFORMANCE_WATCH_FUNCTION_EXIT("getRegisteredInstancesOfAgentType")
		return re;
}

bool LMDatabaseManager::addNewManagedInstanceStatus(std::string ServiceDescriptorGUID, std::string newStatus) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrNewStatus = newStatus.c_str();
	unsigned long nsStrLength = static_cast<unsigned long>(strlen(cStrNewStatus));

	const char* cStrServiceDescriptorGUID = ServiceDescriptorGUID.c_str();
	unsigned long serviceDescriptorGUIDStrLength = static_cast<unsigned long>(ServiceDescriptorGUID.length());

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	//params
	MYSQL_BIND bind[2];

	const char* stmt = "INSERT INTO managed_instances_statuses (ServiceDescriptorGUID, TimeOfStatus, Status) values (?, CURRENT_TIMESTAMP() ,?)";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrServiceDescriptorGUID);
	bind[0].buffer_length = serviceDescriptorGUIDStrLength;
	bind[0].length = &serviceDescriptorGUIDStrLength;

	bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[1].buffer = const_cast<char*>(cStrNewStatus);
	bind[1].buffer_length = nsStrLength;
	bind[1].length = &nsStrLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "addNewManagedInstanceStatus encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("addNewManagedInstanceStatus")
		return re;
}

bool LMDatabaseManager::removeManagedInstance(std::string ServiceDescriptorGUID, std::string location) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrServiceDescriptorGUID = ServiceDescriptorGUID.c_str();
	unsigned long serviceDescriptorGUIDLength = static_cast<unsigned long>(ServiceDescriptorGUID.length());

	const char* cStrLocation = location.c_str();
	unsigned long locationLength = static_cast<unsigned long>(location.length());

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "DELETE FROM managed_instances WHERE ServiceDescriptorGUID = ? AND Location = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrServiceDescriptorGUID);
	bind[0].buffer_length = serviceDescriptorGUIDLength;
	bind[0].length = &serviceDescriptorGUIDLength;

	bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[1].buffer = const_cast<char*>(cStrLocation);
	bind[1].buffer_length = locationLength;
	bind[1].length = &locationLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "removeManagedInstance encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("removeManagedInstance")
		return re;
}

std::string LMDatabaseManager::getRegisteredInstanceSubType(std::string ServiceDescriptorGUID) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::string re = "";

	const char* cStrServiceDescriptorGUID = ServiceDescriptorGUID.c_str();
	unsigned long serviceDescriptorGUIDLength = static_cast<unsigned long>(ServiceDescriptorGUID.length());

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "SELECT AgentSubType FROM managed_instances WHERE ServiceDescriptorGUID = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrServiceDescriptorGUID);
	bind[0].buffer_length = serviceDescriptorGUIDLength;
	bind[0].length = &serviceDescriptorGUIDLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	if (mysql_stmt_execute(sql_stmt) != 0) {
		LOG(ERROR) << "getRegisteredInstanceSubType encountered the following error: " << mysql_stmt_error(sql_stmt);
		mysql_stmt_close(sql_stmt);
		return re;
	}

	MYSQL_BIND resultBIND[1];
	memset(resultBIND, 0, sizeof(resultBIND));

	unsigned long lineLengthCol0 = 0;

	resultBIND[0].buffer_type = MYSQL_TYPE_VAR_STRING;

	resultBIND[0].length = &lineLengthCol0;

	mysql_stmt_bind_result(sql_stmt, resultBIND);

	mysql_stmt_store_result(sql_stmt);
	unsigned long long resultRows = mysql_stmt_num_rows(sql_stmt);
	if (resultRows == 0) {
		LOG(DEBUG) << "getRegisteredInstanceSubType was supposed to return a AgentSubType for a managed_instance that seems not to be there!";
		//It's not there! Return ""
	}
	else if (resultRows != 1) {
		LOG(ERROR) << "getRegisteredInstanceSubType received more than one result line!";
	}
	else {
		mysql_stmt_fetch(sql_stmt);

		char* responseCol0 = new char[lineLengthCol0 + 1];
		memset(responseCol0, 0, lineLengthCol0 + 1);

		resultBIND[0].buffer = responseCol0;

		resultBIND[0].buffer_length = lineLengthCol0;

		mysql_stmt_fetch_column(sql_stmt, &resultBIND[0], 0, 0);

		re = std::string(responseCol0);

		delete[] responseCol0;
	}

	mysql_stmt_free_result(sql_stmt);
	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getRegisteredInstanceSubType")
		return re;
}

GetTestcaseToMutateResponse* LMDatabaseManager::generateGetTestcaseToMutateResponse(std::string testcaseTempDir, int ratingAdaption) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		GetTestcaseToMutateResponse* response = new GetTestcaseToMutateResponse();
	long long int testcaseID = 0;

	//#################### First part: get testcase ####################
	{
		if (mysql_query(getDBConnection(), "SELECT ID, CreatorServiceDescriptorGUID, CreatorLocalID, RawBytes  FROM interesting_testcases WHERE TestCaseType = 0 ORDER BY Rating DESC LIMIT 1") != 0) {
			LOG(ERROR) << "LMDatabaseManager::generateGetTestcaseToMutateResponse could not get a testcase from the database";
			return response;
		}

		MYSQL_RES* result = mysql_store_result(getDBConnection());
		if (result == NULL) {
			return response;
		}

		MYSQL_ROW row = mysql_fetch_row(result);
		if (row == NULL) {
			mysql_free_result(result);
			return response;
		}

		unsigned long* lengths = mysql_fetch_lengths(result);
		if (lengths == NULL) {
			mysql_free_result(result);
			return response;
		}

		testcaseID = _strtoui64(row[0], NULL, 10);
		std::string creatorGUID = row[1];
		long long int creatorLocalID = _strtoui64(row[2], NULL, 10);
		std::string* rawBytesFirstChunk = new std::string(row[3], min(lengths[3], static_cast<unsigned int>(CommInt::chunkSizeInBytes)));
		bool isOnlyOneChunk = lengths[3] <= static_cast<unsigned long>(CommInt::chunkSizeInBytes);

		if (!isOnlyOneChunk) {
			std::string* rawBytes = new std::string(row[3], lengths[3]);

			std::stringstream ss;
			ss << creatorGUID << "-" << creatorLocalID;
			//apparently, fwrite performs much better than ofstream
			FILE* testcaseFile;
			if (0 != fopen_s(&testcaseFile, (testcaseTempDir + Util::pathSeperator + ss.str()).c_str(), "wb")) {
				LOG(ERROR) << "LMDatabaseManager::generateGetTestcaseToMutateResponse failed to open the file " << (testcaseTempDir + Util::pathSeperator + ss.str());
				delete rawBytes;
				delete rawBytesFirstChunk;
				return response;
			}
			fwrite(rawBytes->c_str(), sizeof(char), lengths[3], testcaseFile);
			fclose(testcaseFile);

			delete rawBytes;
		}

		ServiceDescriptor* creatorSd = new ServiceDescriptor();
		creatorSd->set_guid(creatorGUID);
		//the service's hostandPort is unknown and therefore not set (and irrelevant anyways)

		TestcaseID* tcID = new TestcaseID();
		tcID->set_allocated_servicedescriptor(creatorSd);
		tcID->set_localid(creatorLocalID);

		response->set_allocated_id(tcID);
		response->set_allocated_testcasefirstchunk(rawBytesFirstChunk);
		response->set_islastchunk(isOnlyOneChunk);

		mysql_free_result(result);
	}
	//#################### Second part: decrease testcase rating ####################
	{
		int preparedRatingAdaption = ratingAdaption;

		//// prepared Statement
		MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

		const char* stmt = "UPDATE interesting_testcases SET Rating = Rating - ? WHERE ID = ?";
		mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

		//params
		MYSQL_BIND bind[2];
		memset(bind, 0, sizeof(bind));

		bind[0].buffer_type = MYSQL_TYPE_LONG;
		bind[0].buffer = &preparedRatingAdaption;
		bind[0].is_null = 0;
		bind[0].is_unsigned = false;
		bind[0].length = NULL;

		bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
		bind[1].buffer = &testcaseID;
		bind[1].is_null = 0;
		bind[1].is_unsigned = true;
		bind[1].length = NULL;

		mysql_stmt_bind_param(sql_stmt, bind);
		bool re = mysql_stmt_execute(sql_stmt) == 0;
		if (!re) {
			LOG(ERROR) << "generateGetTestcaseToMutateResponse encountered the following error (1): " << mysql_stmt_error(sql_stmt);
		}

		mysql_stmt_close(sql_stmt);
	}
	//#################### Third part: check if we already have coverage for this testcase ####################
	{
		//// prepared Statement
		MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

		const char* stmt = "SELECT COUNT(1) FROM covered_blocks WHERE CreatorTestcaseID = ?";
		mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

		//params
		MYSQL_BIND bind[1];
		memset(bind, 0, sizeof(bind));

		bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
		bind[0].buffer = &testcaseID;
		bind[0].is_null = 0;
		bind[0].is_unsigned = true;
		bind[0].length = NULL;

		mysql_stmt_bind_param(sql_stmt, bind);
		bool re = mysql_stmt_execute(sql_stmt) == 0;
		if (!re) {
			LOG(ERROR) << "generateGetTestcaseToMutateResponse encountered the following error (2): " << mysql_stmt_error(sql_stmt);
		}

		long long int	countOfBlocks;

		MYSQL_BIND resultBIND[1];
		memset(resultBIND, 0, sizeof(resultBIND));

		resultBIND[0].buffer_type = MYSQL_TYPE_LONGLONG;
		resultBIND[0].buffer = &(countOfBlocks);
		resultBIND[0].buffer_length = sizeof(countOfBlocks);

		mysql_stmt_bind_result(sql_stmt, resultBIND);

		mysql_stmt_store_result(sql_stmt);
		unsigned long long resultRows = mysql_stmt_num_rows(sql_stmt);
		if (resultRows != 1) {
			LOG(ERROR) << "generateGetTestcaseToMutateResponse failed to set alsoRunWithoutMutation as it could not be determined, if there is blockcoverage";
		}
		else {
			mysql_stmt_fetch(sql_stmt);

			response->set_alsorunwithoutmutation(countOfBlocks == 0);
		}

		mysql_stmt_free_result(sql_stmt);
		mysql_stmt_close(sql_stmt);
	}

	PERFORMANCE_WATCH_FUNCTION_EXIT("generateGetTestcaseToMutateResponse")
		return response;
}

std::vector<StatusOfInstance> LMDatabaseManager::getStatusOfManagedInstances(std::string location)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::vector<StatusOfInstance> re{};

	const char* cStrLocation = location.c_str();
	unsigned long locationLength = static_cast<unsigned long>(location.length());

	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "SELECT ServiceDescriptorHostAndPort, managed_instances.ServiceDescriptorGUID,  AgentType, TimeOfStatus, Status FROM managed_instances INNER JOIN (SELECT * FROM managed_instances_statuses AS a WHERE TimeOfStatus = (SELECT MAX(TimeOfStatus) FROM managed_instances_statuses AS b WHERE a.ServiceDescriptorGUID = b.ServiceDescriptorGUID) GROUP BY ServiceDescriptorGUID ) AS statuses ON managed_instances.ServiceDescriptorGUID = statuses.ServiceDescriptorGUID  WHERE Location = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrLocation);
	bind[0].buffer_length = locationLength;
	bind[0].length = &locationLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	if (mysql_stmt_execute(sql_stmt) != 0) {
		LOG(ERROR) << "getStatusOfManagedInstances encountered the following error: " << mysql_stmt_error(sql_stmt);
		mysql_stmt_close(sql_stmt);
		return re;
	}

	MYSQL_BIND resultBIND[5];
	memset(resultBIND, 0, sizeof(resultBIND));

	unsigned long lineLengthCol0 = 0;
	unsigned long lineLengthCol1 = 0;
	unsigned long lineLengthCol4 = 0;

	resultBIND[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[2].buffer_type = MYSQL_TYPE_LONG;
	resultBIND[3].buffer_type = MYSQL_TYPE_TIMESTAMP;;
	resultBIND[4].buffer_type = MYSQL_TYPE_VAR_STRING;

	resultBIND[0].length = &lineLengthCol0;
	resultBIND[1].length = &lineLengthCol1;
	resultBIND[4].length = &lineLengthCol4;

	int outputAgentType;
	MYSQL_TIME outputTimeStamp;

	resultBIND[2].buffer = &(outputAgentType);
	resultBIND[3].buffer = &(outputTimeStamp);

	resultBIND[2].buffer_length = sizeof(outputAgentType);
	resultBIND[3].buffer_length = sizeof(outputTimeStamp);

	mysql_stmt_bind_result(sql_stmt, resultBIND);

	mysql_stmt_store_result(sql_stmt);
	unsigned long long resultRows = mysql_stmt_num_rows(sql_stmt);
	for (unsigned long long i = 0; i < resultRows; i++) {
		mysql_stmt_fetch(sql_stmt);

		char* responseCol0 = new char[lineLengthCol0 + 1];
		memset(responseCol0, 0, lineLengthCol0 + 1);

		char* responseCol1 = new char[lineLengthCol1 + 1];
		memset(responseCol1, 0, lineLengthCol1 + 1);

		char* responseCol4 = new char[lineLengthCol4 + 1];
		memset(responseCol4, 0, lineLengthCol4 + 1);

		resultBIND[0].buffer = responseCol0;
		resultBIND[1].buffer = responseCol1;
		resultBIND[4].buffer = responseCol4;

		resultBIND[0].buffer_length = lineLengthCol0;
		resultBIND[1].buffer_length = lineLengthCol1;
		resultBIND[4].buffer_length = lineLengthCol4;

		mysql_stmt_fetch_column(sql_stmt, &resultBIND[0], 0, 0);
		mysql_stmt_fetch_column(sql_stmt, &resultBIND[1], 1, 0);
		mysql_stmt_fetch_column(sql_stmt, &resultBIND[4], 4, 0);

		{
			struct tm timeinfo = tm();

			timeinfo.tm_year = outputTimeStamp.year - 1900;
			timeinfo.tm_mon = outputTimeStamp.month - 1;
			timeinfo.tm_mday = outputTimeStamp.day;
			timeinfo.tm_hour = outputTimeStamp.hour;
			timeinfo.tm_min = outputTimeStamp.minute;
			timeinfo.tm_sec = outputTimeStamp.second;
			timeinfo.tm_isdst = -1;

			StatusOfInstance newStatus{};
			newStatus.serviceDescriptorHostAndPort = responseCol0;
			newStatus.serviceDescriptorGUID = responseCol1;
			newStatus.type = static_cast<AgentType>(outputAgentType);
			newStatus.lastStatusUpdate = _mkgmtime(&timeinfo);

			for (auto&& element : Util::splitString(responseCol4, "|"))
			{
				// Warning: Ugly hack, we need something neater. Probably obsolete once we actually store the things in a DB table and not as a string
				std::vector<std::string> pair = Util::splitString(element, " ");
				if (pair.size() == 3)
				{
					newStatus.status[pair[0]] = pair[1];
				}
				else if (pair.size() == 4)
				{
					newStatus.status[pair[1]] = pair[2];
				}
			}
			re.push_back(newStatus);
		}

		delete[] responseCol0;
		delete[] responseCol1;
		delete[] responseCol4;
	}

	mysql_stmt_free_result(sql_stmt);
	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getStatusOfManagedInstances")
		return re;
}

bool LMDatabaseManager::addEntryToInterestingTestcasesTable(const FluffiTestcaseID tcID, const FluffiTestcaseID tcparentID, int rating, const std::string testcaseDir, TestCaseType tcType) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrCreatorServiceDescriptorGUID = tcID.m_serviceDescriptor.m_guid.c_str();
	unsigned long creatorGUIDLength = static_cast<unsigned long>(tcID.m_serviceDescriptor.m_guid.length());

	uint64_t preparedCreatorLocalID = tcID.m_localID;

	const char* cStrParentServiceDescriptorGUID = tcparentID.m_serviceDescriptor.m_guid.c_str();
	unsigned long parentGUIDLength = static_cast<unsigned long>(tcparentID.m_serviceDescriptor.m_guid.length());

	uint64_t preparedParentLocalID = tcparentID.m_localID;

	int preparedRating = rating;

	std::string rawBytes = Util::loadTestcaseInMemory(tcID, testcaseDir);
	std::string testfile = Util::generateTestcasePathAndFilename(tcID, testcaseDir);
	m_garbageCollectorWorker->markFileForDelete(testfile);

	const char* cStrRawBytes = rawBytes.c_str();
	unsigned long rawBytesLength = static_cast<unsigned long>(rawBytes.length());

	int preparedType = tcType;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "REPLACE INTO interesting_testcases (CreatorServiceDescriptorGUID, CreatorLocalID, ParentServiceDescriptorGUID, ParentLocalID, Rating, RawBytes, TestCaseType, TimeOfInsertion) values (?, ?, ?, ?, ?, ?, ?,CURRENT_TIMESTAMP())";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[9];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrCreatorServiceDescriptorGUID);
	bind[0].buffer_length = creatorGUIDLength;
	bind[0].length = &creatorGUIDLength;

	bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[1].buffer = &preparedCreatorLocalID;
	bind[1].is_null = 0;
	bind[1].is_unsigned = true;
	bind[1].length = NULL;

	bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[2].buffer = const_cast<char*>(cStrParentServiceDescriptorGUID);
	bind[2].buffer_length = parentGUIDLength;
	bind[2].length = &parentGUIDLength;

	bind[3].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[3].buffer = &preparedParentLocalID;
	bind[3].is_null = 0;
	bind[3].is_unsigned = true;
	bind[3].length = NULL;

	bind[4].buffer_type = MYSQL_TYPE_LONG;
	bind[4].buffer = &preparedRating;
	bind[4].is_null = 0;
	bind[4].is_unsigned = false;
	bind[4].length = NULL;

	bind[5].buffer_type = MYSQL_TYPE_LONG_BLOB;
	bind[5].buffer = const_cast<char*>(cStrRawBytes);
	bind[5].buffer_length = rawBytesLength;
	bind[5].length = &rawBytesLength;

	bind[6].buffer_type = MYSQL_TYPE_LONG;
	bind[6].buffer = &preparedType;
	bind[6].is_null = 0;
	bind[6].is_unsigned = true;
	bind[6].length = NULL;

	bind[7].buffer_type = MYSQL_TYPE_LONG;
	bind[7].buffer = &preparedRating;
	bind[7].is_null = 0;
	bind[7].is_unsigned = false;
	bind[7].length = NULL;

	bind[8].buffer_type = MYSQL_TYPE_LONG;
	bind[8].buffer = &preparedType;
	bind[8].is_null = 0;
	bind[8].is_unsigned = true;
	bind[8].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);
	//if problems with big files arise, mysql_stmt_send_long_data might be the solution
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "addEntryToInterestingTestcasesTable encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);
	PERFORMANCE_WATCH_FUNCTION_EXIT("addEntryToInterestingTestcasesTable")
		return re;
}

bool LMDatabaseManager::setSessionParameters(MYSQL* conn) {
	//Set timezone to UTC
	return mysql_query(conn, "SET SESSION time_zone = '+0:00'") == 0;
}

GetCurrentBlockCoverageResponse* LMDatabaseManager::generateGetCurrentBlockCoverageResponse() {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		GetCurrentBlockCoverageResponse* resp = new GetCurrentBlockCoverageResponse();

	if (mysql_query(getDBConnection(), "SELECT DISTINCT Offset, ModuleID FROM covered_blocks") != 0) {
		return resp;
	}

	MYSQL_RES* result = mysql_store_result(getDBConnection());
	if (result == NULL) {
		return resp;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		BasicBlock* block = resp->add_blocks();
		block->set_rva(std::stoull(row[0]));
		block->set_moduleid(std::stoi(row[1]));
	}
	mysql_free_result(result);

	return resp;

	PERFORMANCE_WATCH_FUNCTION_EXIT("generateGetCurrentBlockCoverageResponse")
		return resp;
}

bool LMDatabaseManager::addEntryToCompletedTestcasesTable(const FluffiTestcaseID tcID) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrCreatorServiceDescriptorGUID = tcID.m_serviceDescriptor.m_guid.c_str();
	unsigned long creatorGUIDLength = static_cast<unsigned long>(tcID.m_serviceDescriptor.m_guid.length());

	uint64_t preparedCreatorLocalID = tcID.m_localID;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "INSERT IGNORE INTO completed_testcases (CreatorServiceDescriptorGUID, CreatorLocalID, TimeOfCompletion) values (?, ? , CURRENT_TIMESTAMP())";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrCreatorServiceDescriptorGUID);
	bind[0].buffer_length = creatorGUIDLength;
	bind[0].length = &creatorGUIDLength;

	bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[1].buffer = &preparedCreatorLocalID;
	bind[1].is_null = 0;
	bind[1].is_unsigned = true;
	bind[1].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool success = mysql_stmt_execute(sql_stmt) == 0;
	if (!success) {
		LOG(ERROR) << "addEntryToCompletedTestcasesTable encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("addEntryToCompletedTestcasesTable")
		return success;
}

bool LMDatabaseManager::addEntriesToCompletedTestcasesTable(const std::set<FluffiTestcaseID>* testcaseIDs) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY

		const int maxGUIDLength = 50;
	for (std::set<FluffiTestcaseID>::const_iterator it = testcaseIDs->begin(); it != testcaseIDs->end(); ++it) {
		//As we use bulk insert, we need to assume a maximum string length. We assume it to be 50. Longer guids  will cause an error. Please note: this value can be adjusted
		if ((*it).m_serviceDescriptor.m_guid.length() >= maxGUIDLength) {
			return false;
		}
	}

	//bulk insert as documented in https://mariadb.com/kb/en/library/bulk-insert-row-wise-binding/

	struct st_data {
		char guid[maxGUIDLength];
		char guid_ind;
		uint64_t localid;
		char localid_ind;
	};

	size_t array_size = testcaseIDs->size();

	if (array_size == 0) {
		//Nothing to do here
		return true;
	}

	size_t row_size = sizeof(struct st_data);
	struct st_data* data = new struct st_data[array_size];
	unsigned int i = 0;
	for (std::set<FluffiTestcaseID>::iterator it = testcaseIDs->begin(); it != testcaseIDs->end(); i++, ++it) {
		strcpy_s(data[i].guid, maxGUIDLength, (*it).m_serviceDescriptor.m_guid.c_str()); //check above ensures that tcID->servicedescriptor().guid().length() < maxGUIDLength (which is the size of the target buffer)

		data[i].guid_ind = STMT_INDICATOR_NTS;
		data[i].localid = (*it).m_localID;
		data[i].localid_ind = STMT_INDICATOR_NONE;
	}

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "INSERT IGNORE INTO completed_testcases (CreatorServiceDescriptorGUID, CreatorLocalID, TimeOfCompletion) values (?, ? , CURRENT_TIMESTAMP())";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = &data[0].guid;
	bind[0].u.indicator = &data[0].guid_ind;

	bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[1].buffer = &data[0].localid;
	bind[1].u.indicator = &data[0].localid_ind;

	/* set array size */
	mysql_stmt_attr_set(sql_stmt, STMT_ATTR_ARRAY_SIZE, &array_size);

	/* set row size */
	mysql_stmt_attr_set(sql_stmt, STMT_ATTR_ROW_SIZE, &row_size);

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "addEntriesToCompletedTestcasesTable encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	delete[] data;

	PERFORMANCE_WATCH_FUNCTION_EXIT("addEntriesToCompletedTestcasesTable")
		return re;
}

bool LMDatabaseManager::addDeltaToTestcaseRating(const FluffiTestcaseID tcID, int ratingDelta) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		if (ratingDelta == 0) {
			return true;
		}

	int preparedRatingDelta = ratingDelta;

	const char* cStrCreatorServiceDescriptorGUID = tcID.m_serviceDescriptor.m_guid.c_str();
	unsigned long creatorGUIDLength = static_cast<unsigned long>(tcID.m_serviceDescriptor.m_guid.length());

	uint64_t preparedCreatorLocalID = tcID.m_localID;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "UPDATE interesting_testcases SET Rating = Rating + ? WHERE CreatorLocalID = ? AND CreatorServiceDescriptorGUID = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[3];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedRatingDelta;
	bind[0].is_null = 0;
	bind[0].is_unsigned = false;
	bind[0].length = NULL;

	bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[1].buffer = &preparedCreatorLocalID;
	bind[1].is_null = 0;
	bind[1].is_unsigned = true;
	bind[1].length = NULL;

	bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[2].buffer = const_cast<char*>(cStrCreatorServiceDescriptorGUID);
	bind[2].buffer_length = creatorGUIDLength;
	bind[2].length = &creatorGUIDLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "addDeltaToTestcaseRating encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("addDeltaToTestcaseRating")
		return re;
}

bool LMDatabaseManager::addBlocksToCoveredBlocks(const FluffiTestcaseID tcID, const std::set<FluffiBasicBlock>* basicBlocks) { //in order to prevent deadlocks, the basic blocks need to be inserted in a sorted order. Therefore a set (which is soted) is used to pass the basicBlocks
	PERFORMANCE_WATCH_FUNCTION_ENTRY

		//#################### First part: get testcase ID ####################
		long long int testcaseID = 0;
	{
		const char* cStrCreatorServiceDescriptorGUID = tcID.m_serviceDescriptor.m_guid.c_str();
		unsigned long creatorGUIDLength = static_cast<unsigned long>(tcID.m_serviceDescriptor.m_guid.length());

		uint64_t preparedCreatorLocalID = tcID.m_localID;

		//// prepared Statement
		MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
		const char* stmt = "SELECT ID FROM interesting_testcases WHERE CreatorServiceDescriptorGUID = ? AND CreatorLocalID = ?";
		mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

		//params
		MYSQL_BIND bind[2];
		memset(bind, 0, sizeof(bind));

		bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
		bind[0].buffer = const_cast<char*>(cStrCreatorServiceDescriptorGUID);
		bind[0].buffer_length = creatorGUIDLength;
		bind[0].length = &creatorGUIDLength;

		bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
		bind[1].buffer = &preparedCreatorLocalID;
		bind[1].is_null = 0;
		bind[1].is_unsigned = true;
		bind[1].length = NULL;

		mysql_stmt_bind_param(sql_stmt, bind);
		if (mysql_stmt_execute(sql_stmt) != 0) {
			LOG(ERROR) << "addBlocksToCoveredBlocks encountered the following error: " << mysql_stmt_error(sql_stmt);
			mysql_stmt_close(sql_stmt);
			return false;
		}

		MYSQL_BIND resultBIND[1];
		memset(resultBIND, 0, sizeof(resultBIND));

		resultBIND[0].buffer_type = MYSQL_TYPE_LONGLONG;
		resultBIND[0].buffer = &(testcaseID);
		resultBIND[0].buffer_length = sizeof(testcaseID);

		mysql_stmt_bind_result(sql_stmt, resultBIND);

		mysql_stmt_store_result(sql_stmt);

		if (mysql_stmt_num_rows(sql_stmt) != 1) {
			LOG(ERROR) << "addBlocksToCoveredBlocks failed to retreive exactly one testcaseID!";
			mysql_stmt_free_result(sql_stmt);
			mysql_stmt_close(sql_stmt);
			return false;
		}

		mysql_stmt_fetch(sql_stmt);

		mysql_stmt_free_result(sql_stmt);
		mysql_stmt_close(sql_stmt);
	}
	//#################### Second part: bulk insert blocks ####################
	{
		//In order to avoid deadlocks we set the transaction level (see https://www.percona.com/blog/2012/03/27/innodbs-gap-locks/)
		if (mysql_query(getDBConnection(), "SET TRANSACTION ISOLATION LEVEL READ COMMITTED") != 0) {
			LOG(WARNING) << "\"SET TRANSACTION ISOLATION LEVEL READ COMMITTED\" failed";
		}

		//bulk insert as documented in https://mariadb.com/kb/en/library/bulk-insert-row-wise-binding/

		struct st_data {
			uint64_t testcaseid;
			char testcaseid_ind;
			unsigned char moduleid;
			char moduleid_ind;
			uint64_t offset;
			char offset_ind;
		};

		unsigned int array_size = static_cast<unsigned int>(basicBlocks->size());

		if (array_size == 0) {
			//Nothing to do here
			return true;
		}

		size_t row_size = sizeof(struct st_data);
		struct st_data* data = new struct st_data[array_size];
		unsigned int i = 0;
		for (auto& basicBlock : *basicBlocks) {
			data[i].testcaseid = testcaseID;
			data[i].testcaseid_ind = STMT_INDICATOR_NONE;
			data[i].moduleid = static_cast<unsigned char>(basicBlock.m_moduleID);
			data[i].moduleid_ind = STMT_INDICATOR_NONE;
			data[i].offset = basicBlock.m_rva;
			data[i].offset_ind = STMT_INDICATOR_NONE;

			i++;
		}

		//// prepared Statement
		MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

		const char* stmt = "INSERT INTO covered_blocks (CreatorTestcaseID, ModuleID, Offset, TimeOfInsertion) values (? , ? , ? , CURRENT_TIMESTAMP()) ON DUPLICATE KEY UPDATE TimeOfInsertion=CURRENT_TIMESTAMP()";
		mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

		//params
		MYSQL_BIND bind[3];
		memset(bind, 0, sizeof(bind));

		bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
		bind[0].buffer = &data[0].testcaseid;
		bind[0].u.indicator = &data[0].testcaseid_ind;

		bind[1].buffer_type = MYSQL_TYPE_TINY;
		bind[1].buffer = &data[0].moduleid;
		bind[1].is_unsigned = true;
		bind[1].u.indicator = &data[0].moduleid_ind;

		bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
		bind[2].buffer = &data[0].offset;
		bind[2].u.indicator = &data[0].offset_ind;

		/* set array size */
		mysql_stmt_attr_set(sql_stmt, STMT_ATTR_ARRAY_SIZE, &array_size);

		/* set row size */
		mysql_stmt_attr_set(sql_stmt, STMT_ATTR_ROW_SIZE, &row_size);

		mysql_stmt_bind_param(sql_stmt, bind);
		bool re = mysql_stmt_execute(sql_stmt) == 0;
		if (!re) {
			LOG(ERROR) << "addBlocksToCoveredBlocks encountered the following error: " << mysql_stmt_error(sql_stmt);
		}

		mysql_stmt_close(sql_stmt);

		delete[] data;

		PERFORMANCE_WATCH_FUNCTION_EXIT("addBlocksToCoveredBlocks")
			return re;
	}
}

std::deque<FluffiSetting> LMDatabaseManager::getAllSettings() {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::deque<FluffiSetting> re{};

	if (mysql_query(getDBConnection(), "SELECT SettingName, SettingValue FROM settings") != 0) {
		return re;
	}

	MYSQL_RES* result = mysql_store_result(getDBConnection());
	if (result == NULL) {
		return re;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		FluffiSetting s{ row[0], row[1] };
		re.push_back(s);
	}
	mysql_free_result(result);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getSettingValue")
		return re;
}
std::deque<FluffiBasicBlock> LMDatabaseManager::getTargetBlocks() {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::deque<FluffiBasicBlock> re{};

	if (mysql_query(getDBConnection(), "SELECT Offset, ModuleID  FROM blocks_to_cover") != 0) {
		return re;
	}

	MYSQL_RES* result = mysql_store_result(getDBConnection());
	if (result == NULL) {
		return re;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		FluffiBasicBlock block{ std::stoull(row[0]), std::stoi(row[1]) };
		re.push_back(block);
	}
	mysql_free_result(result);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getTargetBlocks")
		return re;
}

std::deque<FluffiModuleNameToID> LMDatabaseManager::getTargetModules() {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::deque<FluffiModuleNameToID> re{};

	if (mysql_query(getDBConnection(), "SELECT ID, ModuleName, ModulePath FROM target_modules") != 0) {
		return re;
	}

	MYSQL_RES* result = mysql_store_result(getDBConnection());
	if (result == NULL) {
		return re;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		FluffiModuleNameToID mod{ row[1], row[2],static_cast<uint32_t>(std::stoi(row[0])) };
		re.push_back(mod);
	}
	mysql_free_result(result);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getTargetModules")
		return re;
}

bool LMDatabaseManager::deleteManagedInstanceStatusOlderThanXSec(int olderThanInSeconds) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedSec = olderThanInSeconds;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "DELETE FROM managed_instances_statuses WHERE TimeOfStatus < (CURRENT_TIMESTAMP() - INTERVAL ? SECOND)";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedSec;
	bind[0].is_null = 0;
	bind[0].is_unsigned = true;
	bind[0].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "deleteManagedInstanceStatusOlderThanXSec encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("deleteManagedInstanceStatusOlderThanXSec")
		return re;
}

bool LMDatabaseManager::addEntryToCrashDescriptionsTable(const FluffiTestcaseID tcID, const std::string crashFootprint) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrCreatorServiceDescriptorGUID = tcID.m_serviceDescriptor.m_guid.c_str();
	unsigned long creatorGUIDLength = static_cast<unsigned long>(tcID.m_serviceDescriptor.m_guid.length());

	uint64_t preparedCreatorLocalID = tcID.m_localID;

	const char* cStrCrashFootprint = crashFootprint.c_str();
	unsigned long crashFootprintLength = static_cast<unsigned long>(crashFootprint.length());

	const char* cStrCrashFootprint2 = crashFootprint.c_str();
	unsigned long crashFootprintLength2 = static_cast<unsigned long>(crashFootprint.length());

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "INSERT INTO crash_descriptions (CreatorTestcaseID, CrashFootprint) values ((SELECT ID from interesting_testcases WHERE CreatorServiceDescriptorGUID= ? AND CreatorLocalID = ?) , ?) ON DUPLICATE KEY UPDATE CrashFootprint = ? ";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[4];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrCreatorServiceDescriptorGUID);
	bind[0].buffer_length = creatorGUIDLength;
	bind[0].length = &creatorGUIDLength;

	bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[1].buffer = &preparedCreatorLocalID;
	bind[1].is_null = 0;
	bind[1].is_unsigned = true;
	bind[1].length = NULL;

	bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[2].buffer = const_cast<char*>(cStrCrashFootprint);
	bind[2].buffer_length = crashFootprintLength;
	bind[2].length = &crashFootprintLength;

	bind[3].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[3].buffer = const_cast<char*>(cStrCrashFootprint2);
	bind[3].buffer_length = crashFootprintLength2;
	bind[3].length = &crashFootprintLength2;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "addEntryToCrashDescriptionsTable encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("addEntryToCrashDescriptionsTable")
		return re;
}

bool LMDatabaseManager::initializeBillingTable() {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		bool success = true;
	success &= mysql_query(getDBConnection(), "INSERT IGNORE INTO billing (Resource) VALUES ('RunnerSeconds')") == 0;
	success &= mysql_query(getDBConnection(), "INSERT IGNORE INTO billing (Resource) VALUES ('RunTestcasesNoLongerListed')") == 0;

	PERFORMANCE_WATCH_FUNCTION_EXIT("initializeBillingTable")
		return success;
}

bool LMDatabaseManager::addRunnerSeconds(unsigned int secondsToAdd) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		unsigned int preparedSecondsToAdd = secondsToAdd;
	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "UPDATE billing SET Amount = Amount + ? WHERE Resource = 'RunnerSeconds'";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedSecondsToAdd;
	bind[0].is_null = 0;
	bind[0].is_unsigned = true;
	bind[0].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "addRunnerSeconds encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("addRunnerSeconds")
		return re;
}

unsigned long long LMDatabaseManager::cleanCompletedTestcasesTableOlderThanXSec(int olderThanInSeconds) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedSec = olderThanInSeconds;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	//const char* stmt = "DELETE FROM completed_testcases WHERE TimeOfCompletion < (CURRENT_TIMESTAMP() - INTERVAL ? SECOND)"; // this one causes deadlocks but is fast and simple
	//Hint for deadlock debugging : "SHOW ENGINE INNODB STATUS"
	const char* stmt = "DELETE FROM completed_testcases WHERE ID IN (SELECT id from (SELECT id FROM completed_testcases WHERE TimeOfCompletion < (CURRENT_TIMESTAMP() - INTERVAL ? SECOND))as ct_t)"; //This one is slow but might prevent deadlocks
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedSec;
	bind[0].is_null = 0;
	bind[0].is_unsigned = true;
	bind[0].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);
	int success = mysql_stmt_execute(sql_stmt);
	if (!(success == 0)) {
		LOG(ERROR) << "cleanCompletedTestcasesTableOlderThanXSec encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	unsigned long long affected_rows = mysql_stmt_affected_rows(sql_stmt);

	mysql_stmt_close(sql_stmt);

	if (!(success == 0)) {
		return 0;
	}

	PERFORMANCE_WATCH_FUNCTION_EXIT("cleanCompletedTestcasesTableOlderThanXSec")
		return affected_rows;
}

bool LMDatabaseManager::addRunTestcasesNoLongerListed(unsigned int numberOfTestcasesNoLongerListed) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		unsigned int preparedNumberOfTestcasesNoLongerListed = numberOfTestcasesNoLongerListed;
	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "UPDATE billing SET Amount = Amount + ? WHERE Resource = 'RunTestcasesNoLongerListed'";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedNumberOfTestcasesNoLongerListed;
	bind[0].is_null = 0;
	bind[0].is_unsigned = true;
	bind[0].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "addRunTestcasesNoLongerListed encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("addRunTestcasesNoLongerListed")
		return re;
}

bool LMDatabaseManager::dropTestcaseTypeIFMoreThan(LMDatabaseManager::TestCaseType type, int instances) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY

		//In order to avoid deadlocks we set the transaction level (see https://www.percona.com/blog/2012/03/27/innodbs-gap-locks/)
		if (mysql_query(getDBConnection(), "SET TRANSACTION ISOLATION LEVEL READ COMMITTED") != 0) {
			LOG(WARNING) << "\"SET TRANSACTION ISOLATION LEVEL READ COMMITTED\" failed";
		}

	int preparedType1 = type;
	int preparedType2 = type;
	int preparedInstances = instances - 1;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "DELETE tc FROM interesting_testcases as tc JOIN ( SELECT ID FROM  interesting_testcases WHERE TestcaseType = ? ORDER BY ID ASC LIMIT 1 OFFSET ? ) tclimit ON tc.ID > tclimit.ID WHERE TestcaseType = ? ";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[3];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedType1;
	bind[0].is_null = 0;
	bind[0].is_unsigned = true;
	bind[0].length = NULL;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = &preparedInstances;
	bind[1].is_null = 0;
	bind[1].is_unsigned = false;
	bind[1].length = NULL;

	bind[2].buffer_type = MYSQL_TYPE_LONG;
	bind[2].buffer = &preparedType2;
	bind[2].is_null = 0;
	bind[2].is_unsigned = true;
	bind[2].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "dropTestcaseTypeIFMoreThan encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("dropTestcaseTypeIFMoreThan")
		return re;
}

std::deque<std::string> LMDatabaseManager::getAllCrashFootprints() {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::deque<std::string> re{};

	if (mysql_query(getDBConnection(), "SELECT DISTINCT CrashFootprint FROM crash_descriptions") != 0) {
		return re;
	}

	MYSQL_RES* result = mysql_store_result(getDBConnection());
	if (result == NULL) {
		return re;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		re.push_back(row[0]);
	}
	mysql_free_result(result);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getAllCrashFootprints")
		return re;
}

std::vector<std::pair<std::string, int>> LMDatabaseManager::getRegisteredInstancesNumOfSubTypes() {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::vector<std::pair<std::string, int>> re{};

	if (mysql_query(getDBConnection(), "SELECT AgentSubType, COUNT(AgentSubType) FROM managed_instances GROUP BY AgentSubType") != 0) {
		return re;
	}

	MYSQL_RES* result = mysql_store_result(getDBConnection());
	if (result == NULL) {
		return re;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		re.push_back(std::make_pair(row[0], std::stoi(row[1])));
	}
	mysql_free_result(result);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getRegisteredInstancesNumOfSubTypes")
		return re;
}

bool LMDatabaseManager::dropTestcaseIfCrashFootprintAppearedMoreThanXTimes(int times, std::string crashFootprint) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedTimes = times - 1;
	const char* cStrCrashFootprint1 = crashFootprint.c_str();
	unsigned long crashFootprint1Length = static_cast<unsigned long>(crashFootprint.length());
	const char* cStrCrashFootprint2 = crashFootprint.c_str();
	unsigned long crashFootprint2Length = static_cast<unsigned long>(crashFootprint.length());

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "DELETE tc FROM interesting_testcases as tc JOIN crash_descriptions AS cd ON tc.ID = cd.CreatorTestcaseID JOIN ( Select it.ID FROM  interesting_testcases AS it JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID WHERE cd.CrashFootprint= ? LIMIT 1 OFFSET ?  ) tclimit ON tc.ID > tclimit.ID WHERE cd.CrashFootprint= ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[3];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrCrashFootprint1);
	bind[0].buffer_length = crashFootprint1Length;
	bind[0].length = &crashFootprint1Length;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = &preparedTimes;
	bind[1].is_null = 0;
	bind[1].is_unsigned = false;
	bind[1].length = NULL;

	bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[2].buffer = const_cast<char*>(cStrCrashFootprint2);
	bind[2].buffer_length = crashFootprint2Length;
	bind[2].length = &crashFootprint2Length;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "dropTestcaseIfCrashFootprintAppearedMoreThanXTimes encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("dropTestcaseIfCrashFootprintAppearedMoreThanXTimes")
		return re;
}

bool LMDatabaseManager::setTestcaseType(const FluffiTestcaseID tcID, TestCaseType tcType) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY

		int preparedType = tcType;

	const char* cStrCreatorServiceDescriptorGUID = tcID.m_serviceDescriptor.m_guid.c_str();
	unsigned long creatorGUIDLength = static_cast<unsigned long>(tcID.m_serviceDescriptor.m_guid.length());

	uint64_t preparedCreatorLocalID = tcID.m_localID;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "UPDATE interesting_testcases SET TestCaseType = ? WHERE CreatorLocalID = ? AND CreatorServiceDescriptorGUID = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[3];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedType;
	bind[0].is_null = 0;
	bind[0].is_unsigned = true;
	bind[0].length = NULL;

	bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[1].buffer = &preparedCreatorLocalID;
	bind[1].is_null = 0;
	bind[1].is_unsigned = true;
	bind[1].length = NULL;

	bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[2].buffer = const_cast<char*>(cStrCreatorServiceDescriptorGUID);
	bind[2].buffer_length = creatorGUIDLength;
	bind[2].length = &creatorGUIDLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "setTestcaseType encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("setTestcaseType")
		return re;
}