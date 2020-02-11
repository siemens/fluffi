/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier, Michael Kraus, Pascal Eckmann
*/

#include "stdafx.h"
#include "GMDatabaseManager.h"
#include "FluffiServiceDescriptor.h"
#include "CommInt.h"
#include "FluffiLMConfiguration.h"

//Init static members
std::string GMDatabaseManager::s_dbHost = "";
std::string GMDatabaseManager::s_dbUser = "";
std::string GMDatabaseManager::s_dbPassword = "";
std::string GMDatabaseManager::s_dbName = "";

GMDatabaseManager::GMDatabaseManager() :
	m_DBConnection(nullptr),
	performanceWatchTimePoint(std::chrono::steady_clock::now())
{
	if (!mysql_thread_safe()) {
		LOG(ERROR) << "The maria db connector was not compiled in a thread safe way! This won't work!";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy
	}
}

GMDatabaseManager::~GMDatabaseManager()
{
	if (m_DBConnection != nullptr) {
		mysql_close(m_DBConnection);
		m_DBConnection = nullptr;
	}

	mysql_thread_end();
}

void GMDatabaseManager::setDBConnectionParameters(std::string host, std::string user, std::string pwd, std::string db)
{
	s_dbHost = host;
	s_dbUser = user;
	s_dbPassword = pwd;
	s_dbName = db;
}

FluffiServiceDescriptor GMDatabaseManager::getLMServiceDescriptorForWorker(FluffiServiceDescriptor fsd)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::string guid, serviceDescriptor;
	FluffiServiceDescriptor result = FluffiServiceDescriptor::getNullObject();

	const char* cStrworkerGUID = fsd.m_guid.c_str();
	unsigned long GUIDworkerLength = static_cast<unsigned long>(fsd.m_guid.length());

	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "SELECT localmanagers.ServiceDescriptorGUID, localmanagers.ServiceDescriptorHostAndPort FROM workers, localmanagers WHERE workers.ServiceDescriptorGUID = ? AND workers.Location = localmanagers.Location AND workers.FuzzJob = localmanagers.FuzzJob";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrworkerGUID);
	bind[0].buffer_length = GUIDworkerLength;
	bind[0].length = &GUIDworkerLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	if (mysql_stmt_execute(sql_stmt) != 0) {
		mysql_stmt_close(sql_stmt);
		LOG(ERROR) << "getLMServiceDescriptorForWorker Could not execute database command";
		return result;
	}

	mysql_stmt_store_result(sql_stmt);
	unsigned long long resultRows = mysql_stmt_num_rows(sql_stmt);

	if (resultRows == 1)
	{
		MYSQL_BIND resultBIND[2];
		memset(resultBIND, 0, sizeof(resultBIND));

		unsigned long lineLengthCol0 = 0;
		unsigned long lineLengthCol1 = 0;

		resultBIND[0].buffer_type = MYSQL_TYPE_VAR_STRING;
		resultBIND[1].buffer_type = MYSQL_TYPE_VAR_STRING;

		resultBIND[0].length = &lineLengthCol0;
		resultBIND[1].length = &lineLengthCol1;

		mysql_stmt_bind_result(sql_stmt, resultBIND);

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

		serviceDescriptor = responseCol1;
		guid = responseCol0;

		delete[] responseCol0;
		delete[] responseCol1;

		result = FluffiServiceDescriptor(serviceDescriptor, guid);
	}

	mysql_stmt_free_result(sql_stmt);
	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getLMServiceDescriptorForWorker")
		return result;
}

FluffiLMConfiguration GMDatabaseManager::getLMConfigurationForFuzzJob(int fuzzJob)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedFuzzJob = fuzzJob;

	std::string dbHost, dbUser, dbPassword, dbName, fuzzJobName;

	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "SELECT DBHost, DBUser, DBPass, DBName, name FROM fuzzjob WHERE ID = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedFuzzJob;
	bind[0].is_null = 0;
	bind[0].is_unsigned = true;
	bind[0].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);
	if (mysql_stmt_execute(sql_stmt) != 0) {
		mysql_stmt_close(sql_stmt);
		LOG(ERROR) << "getLMConfigurationForFuzzJob Could not execute database command getLMConfigurationForFuzzJob";
		throw std::runtime_error("Could not execute database command");
	}

	mysql_stmt_store_result(sql_stmt);
	unsigned long long resultRows = mysql_stmt_num_rows(sql_stmt);

	if (resultRows == 0)
	{
		LOG(ERROR) << "getLMConfigurationForFuzzJob: Could not find result";
		throw std::invalid_argument("Could not find result");
	}
	else if (resultRows > 1)
	{
		LOG(ERROR) << "FuzzJob has more than one DB assigned to it";
		throw std::runtime_error("FuzzJob has more than one DB assigned to it");
	}

	MYSQL_BIND resultBIND[5];
	memset(resultBIND, 0, sizeof(resultBIND));

	unsigned long lineLengthCol0 = 0;
	unsigned long lineLengthCol1 = 0;
	unsigned long lineLengthCol2 = 0;
	unsigned long lineLengthCol3 = 0;
	unsigned long lineLengthCol4 = 0;

	resultBIND[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[2].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[3].buffer_type = MYSQL_TYPE_VAR_STRING;
	resultBIND[4].buffer_type = MYSQL_TYPE_VAR_STRING;

	resultBIND[0].length = &lineLengthCol0;
	resultBIND[1].length = &lineLengthCol1;
	resultBIND[2].length = &lineLengthCol2;
	resultBIND[3].length = &lineLengthCol3;
	resultBIND[4].length = &lineLengthCol4;

	mysql_stmt_bind_result(sql_stmt, resultBIND);

	mysql_stmt_fetch(sql_stmt);

	char* responseCol0 = new char[lineLengthCol0 + 1];
	memset(responseCol0, 0, lineLengthCol0 + 1);

	char* responseCol1 = new char[lineLengthCol1 + 1];
	memset(responseCol1, 0, lineLengthCol1 + 1);

	char* responseCol2 = new char[lineLengthCol2 + 1];
	memset(responseCol2, 0, lineLengthCol2 + 1);

	char* responseCol3 = new char[lineLengthCol3 + 1];
	memset(responseCol3, 0, lineLengthCol3 + 1);

	char* responseCol4 = new char[lineLengthCol4 + 1];
	memset(responseCol4, 0, lineLengthCol4 + 1);

	resultBIND[0].buffer = responseCol0;
	resultBIND[1].buffer = responseCol1;
	resultBIND[2].buffer = responseCol2;
	resultBIND[3].buffer = responseCol3;
	resultBIND[4].buffer = responseCol4;

	resultBIND[0].buffer_length = lineLengthCol0;
	resultBIND[1].buffer_length = lineLengthCol1;
	resultBIND[2].buffer_length = lineLengthCol2;
	resultBIND[3].buffer_length = lineLengthCol3;
	resultBIND[4].buffer_length = lineLengthCol4;

	mysql_stmt_fetch_column(sql_stmt, &resultBIND[0], 0, 0);
	mysql_stmt_fetch_column(sql_stmt, &resultBIND[1], 1, 0);
	mysql_stmt_fetch_column(sql_stmt, &resultBIND[2], 2, 0);
	mysql_stmt_fetch_column(sql_stmt, &resultBIND[3], 3, 0);
	mysql_stmt_fetch_column(sql_stmt, &resultBIND[4], 4, 0);

	dbHost = responseCol0;
	dbUser = responseCol1;
	dbPassword = responseCol2;
	dbName = responseCol3;
	fuzzJobName = responseCol4;

	delete[] responseCol0;
	delete[] responseCol1;
	delete[] responseCol2;
	delete[] responseCol3;
	delete[] responseCol4;

	mysql_stmt_free_result(sql_stmt);
	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getLMConfigurationForFuzzJob")
		return FluffiLMConfiguration(dbHost, dbUser, dbPassword, dbName, fuzzJobName);
}

bool GMDatabaseManager::removeWorkerFromDatabase(FluffiServiceDescriptor fsd) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrServiceDescriptorGUID = fsd.m_guid.c_str();
	unsigned long GUIDLength = static_cast<unsigned long>(fsd.m_guid.length());

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "DELETE FROM workers WHERE ServiceDescriptorGUID = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrServiceDescriptorGUID);
	bind[0].buffer_length = GUIDLength;
	bind[0].length = &GUIDLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "removeWorkerFromDatabase encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("removeWorkerFromDatabase")
		return re;
}

bool GMDatabaseManager::addWorkerToDatabase(FluffiServiceDescriptor fsd, AgentType type, std::string subtypes, std::string location)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrServiceDescriptorGUID = fsd.m_guid.c_str();
	unsigned long GUIDLength = static_cast<unsigned long>(fsd.m_guid.length());

	const char* cStrServiceDescriptorHAP = fsd.m_serviceHostAndPort.c_str();
	unsigned long HAPLength = static_cast<unsigned long>(fsd.m_serviceHostAndPort.length());

	const char* cStrSubtypes = subtypes.c_str();
	unsigned long subtypesLength = static_cast<unsigned long>(subtypes.length());

	const char* cStrLocation = location.c_str();
	unsigned long locationLength = static_cast<unsigned long>(location.length());

	long prepAgentType = type;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "REPLACE INTO workers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, FuzzJob, Location, AgentType, AgentSubTypes, TimeOfLastRequest) VALUES (?, ?,  NULL, (SELECT locations.ID FROM locations WHERE locations.Name = ?), ?, ?, NOW())";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[5];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrServiceDescriptorGUID);
	bind[0].buffer_length = GUIDLength;
	bind[0].length = &GUIDLength;

	bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[1].buffer = const_cast<char*>(cStrServiceDescriptorHAP);
	bind[1].buffer_length = HAPLength;
	bind[1].length = &HAPLength;

	bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[2].buffer = const_cast<char*>(cStrLocation);
	bind[2].buffer_length = locationLength;
	bind[2].length = &locationLength;

	bind[3].buffer_type = MYSQL_TYPE_LONG;
	bind[3].buffer = &prepAgentType;
	bind[3].buffer_length = sizeof(prepAgentType);

	bind[4].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[4].buffer = const_cast<char*>(cStrSubtypes);
	bind[4].buffer_length = subtypesLength;
	bind[4].length = &subtypesLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "addWorkerToDatabase encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("addWorkerToDatabase")
		return re;
}

bool GMDatabaseManager::setLMForLocationAndFuzzJob(std::string location, FluffiServiceDescriptor lmServiceDescriptor, long fuzzjob)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrServiceDescriptorGUID = lmServiceDescriptor.m_guid.c_str();
	unsigned long GUIDLength = static_cast<unsigned long>(lmServiceDescriptor.m_guid.length());

	const char* cStrServiceDescriptorHostAndPort = lmServiceDescriptor.m_serviceHostAndPort.c_str();
	unsigned long HAPLength = static_cast<unsigned long>(lmServiceDescriptor.m_serviceHostAndPort.length());

	const char* cStrLocation = location.c_str();
	unsigned long locationLength = static_cast<unsigned long>(location.length());

	long prepFuzzjob = fuzzjob;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "INSERT INTO localmanagers (ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, Fuzzjob) values (?, ?, (SELECT locations.ID FROM locations WHERE locations.Name = ?), ?)";

	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[4];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrServiceDescriptorGUID);
	bind[0].buffer_length = GUIDLength;
	bind[0].length = &GUIDLength;

	bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[1].buffer = const_cast<char*>(cStrServiceDescriptorHostAndPort);
	bind[1].buffer_length = HAPLength;
	bind[1].length = &HAPLength;

	bind[2].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[2].buffer = const_cast<char*>(cStrLocation);
	bind[2].buffer_length = locationLength;
	bind[2].length = &locationLength;

	bind[3].buffer_type = MYSQL_TYPE_LONG;
	bind[3].buffer = &prepFuzzjob;
	bind[3].buffer_length = sizeof(prepFuzzjob);

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "setLMForLocationAndFuzzJob encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	if (!re)
	{
		throw std::runtime_error("Could not register LM in DB");
	}

	PERFORMANCE_WATCH_FUNCTION_EXIT("setLMForLocationAndFuzzJob")
		return re;
}

bool GMDatabaseManager::deleteManagedLMStatusOlderThanXSec(int olderThanInSeconds)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedSec = olderThanInSeconds;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "DELETE FROM localmanagers_statuses WHERE TimeOfStatus < (CURRENT_TIMESTAMP() - INTERVAL ? SECOND)";
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
		LOG(ERROR) << "deleteManagedLMStatusOlderThanXSec encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("deleteManagedLMStatusOlderThanXSec")
		return re;
}

bool GMDatabaseManager::deleteWorkersNotSeenSinceXSec(int olderThanInSeconds)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedSec = olderThanInSeconds;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "DELETE FROM workers WHERE TimeOfLastRequest < (CURRENT_TIMESTAMP() - INTERVAL ? SECOND)";
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
		LOG(ERROR) << "deleteWorkersNotSeenSinceXSec encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("deleteWorkersNotSeenSinceXSec")
		return re;
}

std::vector<std::pair<std::string, std::string>> GMDatabaseManager::getAllRegisteredLMs()
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		std::vector<std::pair<std::string, std::string>> re;

	if (mysql_query(getDBConnection(), "SELECT ServiceDescriptorGUID, ServiceDescriptorHostAndPort FROM localmanagers") != 0) {
		return re;
	}

	MYSQL_RES* result = mysql_store_result(getDBConnection());
	if (result == NULL) {
		return re;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		std::pair<std::string, std::string> newPair(row[0], row[1]);
		re.push_back(newPair);
	}
	mysql_free_result(result);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getAllRegisteredLMs")
		return re;
}

bool GMDatabaseManager::removeManagedLM(std::string ServiceDescriptorGUID)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrServiceDescriptorGUID = ServiceDescriptorGUID.c_str();
	unsigned long serviceDescriptorGUIDLength = static_cast<unsigned long>(ServiceDescriptorGUID.length());

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "DELETE FROM localmanagers WHERE ServiceDescriptorGUID = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrServiceDescriptorGUID);
	bind[0].buffer_length = serviceDescriptorGUIDLength;
	bind[0].length = &serviceDescriptorGUIDLength;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "removeManagedLM encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("removeManagedLM")
		return re;
}

bool GMDatabaseManager::addNewManagedLMStatus(std::string ServiceDescriptorGUID, std::string newStatus)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		const char* cStrNewStatus = newStatus.c_str();
	unsigned long nsStrLength = static_cast<unsigned long>(strlen(cStrNewStatus));

	const char* cStrServiceDescriptorGUID = ServiceDescriptorGUID.c_str();
	unsigned long serviceDescriptorGUIDStrLength = static_cast<unsigned long>(ServiceDescriptorGUID.length());

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	//params
	MYSQL_BIND bind[2];

	const char* stmt = "INSERT INTO localmanagers_statuses (ServiceDescriptorGUID, TimeOfStatus, Status) values (?, CURRENT_TIMESTAMP() ,?)";
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
		LOG(ERROR) << "addNewManagedLMStatus encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("addNewManagedLMStatus")
		return re;
}

long GMDatabaseManager::getFuzzJobWithoutLM(std::string location)
{
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		long fuzzJob = -1;
	const char* cStrLocation = location.c_str();
	unsigned long locationLength = static_cast<unsigned long>(location.length());

	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "SELECT location_fuzzjobs.Fuzzjob FROM location_fuzzjobs LEFT JOIN locations ON location_fuzzjobs.Location=locations.ID WHERE locations.Name = ? AND location_fuzzjobs.Fuzzjob NOT IN (SELECT FuzzJob FROM localmanagers WHERE Location = locations.ID) LIMIT 1";
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
		mysql_stmt_close(sql_stmt);
		LOG(ERROR) << "getFuzzJobWithoutLM: Could not execute database command";
		return -1;
	}

	mysql_stmt_store_result(sql_stmt);
	unsigned long long resultRows = mysql_stmt_num_rows(sql_stmt);

	if (resultRows > 0)
	{
		MYSQL_BIND resultBIND[1];
		memset(resultBIND, 0, sizeof(resultBIND));

		resultBIND[0].buffer_type = MYSQL_TYPE_LONG;
		resultBIND[0].buffer = &fuzzJob;
		resultBIND[0].buffer_length = sizeof(fuzzJob);

		mysql_stmt_bind_result(sql_stmt, resultBIND);

		mysql_stmt_fetch(sql_stmt);

		mysql_stmt_free_result(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("getFuzzJobWithoutLM")
		return fuzzJob;
}

std::vector<std::tuple<int, std::string, std::string>> GMDatabaseManager::getNewCommands()
{
	std::vector<std::tuple<int, std::string, std::string>> newCommands = std::vector<std::tuple<int, std::string, std::string>>();

	if (mysql_query(getDBConnection(), "SELECT ID, Command, Argument FROM command_queue WHERE Done = 0") != 0) {
		return newCommands;
	}

	MYSQL_RES* result = mysql_store_result(getDBConnection());
	if (result == NULL) {
		return newCommands;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		std::tuple<int, std::string, std::string> newTuple = std::make_tuple(atoi(row[0]), std::string(row[1]), std::string(row[2]));
		newCommands.push_back(newTuple);
	}
	mysql_free_result(result);

	return newCommands;
}

bool GMDatabaseManager::setCommandAsDone(int commandID, std::string errorMessage)
{
	unsigned long longCommandID = commandID;
	const char* cStrErrorMessage = errorMessage.c_str();
	unsigned long errorMessageLength = static_cast<unsigned long>(errorMessage.length());

	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());
	const char* stmt = "UPDATE command_queue SET Done = 1, Error = ? WHERE ID = ?";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = const_cast<char*>(cStrErrorMessage);
	bind[0].buffer_length = errorMessageLength;
	bind[0].length = &errorMessageLength;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = &longCommandID;
	bind[1].is_null = 0;
	bind[1].is_unsigned = true;
	bind[1].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);

	if (mysql_stmt_execute(sql_stmt) != 0) {
		LOG(ERROR) << "setCommandAsDone: Could not execute database command";
		mysql_stmt_close(sql_stmt);

		return false;
	}
	mysql_stmt_close(sql_stmt);
	return true;
}

#ifdef _VSTEST
std::string GMDatabaseManager::EXECUTE_TEST_STATEMENT(const std::string query) {
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

	if (row[0] == NULL) {
		mysql_free_result(result);
		return "";
	}

	std::string re = row[0];
	mysql_free_result(result);

	return re;
}
#endif // _VSTEST

MYSQL* GMDatabaseManager::establishDBConnection()
{
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

bool GMDatabaseManager::setSessionParameters(MYSQL* conn)
{
	//Set timezone to UTC
	return mysql_query(conn, "SET SESSION time_zone = '+0:00'") == 0;
}

MYSQL* GMDatabaseManager::getDBConnection()
{
	if (m_DBConnection == nullptr) {
		m_DBConnection = establishDBConnection();
	}

	return m_DBConnection;
}

bool GMDatabaseManager::deleteDoneCommandsOlderThanXSec(int olderThanInSeconds) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedSec = olderThanInSeconds;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "DELETE FROM command_queue WHERE Done = 1 AND CreationDate < (CURRENT_TIMESTAMP() - INTERVAL ? SECOND)";
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
		LOG(ERROR) << "deleteDoneCommandsOlderThanXSec encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("deleteDoneCommandsOlderThanXSec")
		return re;
}
bool GMDatabaseManager::addNewManagedLMLogMessages(std::string ServiceDescriptorGUID, const std::vector<std::string>& messages) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY

		const int maxGUIDLength = 50;

	//As we use bulk insert, we need to assume a maximum string length. We assume it to be maxGUIDLength. Longer guids  will cause an error. Please note: this value can be adjusted
	if (ServiceDescriptorGUID.length() >= maxGUIDLength) {
		LOG(ERROR) << "Trying to insert a GUID of invalid length with addNewManagedLMLogMessages";
		return false;
	}

	const int maxLogMessageLength = 1990;

	for (std::vector<std::string>::const_iterator it = messages.begin(); it != messages.end(); ++it) {
		//As we use bulk insert, we need to assume a maximum string length. We assume it to be maxLogMessageLength. Longer log messages  will cause an error. Please note: this value can be adjusted
		if ((*it).length() >= maxLogMessageLength) {
			LOG(ERROR) << "Trying to insert a log message of invalid length with addNewManagedLMLogMessages";
			return false;
		}
	}

	//bulk insert as documented in https://mariadb.com/kb/en/library/bulk-insert-row-wise-binding/

	struct st_data {
		char guid[maxGUIDLength];
		char guid_ind;
		char logMessage[maxLogMessageLength];
		char logMessage_ind;
	};

	unsigned int array_size = static_cast<unsigned int>(messages.size());

	if (array_size == 0) {
		//Nothing to do here
		return true;
	}

	//In order to avoid deadlocks we set the transaction level (see https://www.percona.com/blog/2012/03/27/innodbs-gap-locks/)
	if (mysql_query(getDBConnection(), "SET TRANSACTION ISOLATION LEVEL READ COMMITTED") != 0) {
		LOG(WARNING) << "\"SET TRANSACTION ISOLATION LEVEL READ COMMITTED\" failed";
	}

	size_t row_size = sizeof(struct st_data);
	struct st_data* data = new struct st_data[array_size];
	unsigned int i = 0;
	for (std::vector<std::string>::const_iterator it = messages.begin(); it != messages.end(); i++, ++it) {
		strcpy_s(data[i].guid, maxGUIDLength, ServiceDescriptorGUID.c_str()); //check above ensures that m_serviceDescriptor.length() < maxGUIDLength (which is the size of the target buffer)
		data[i].guid_ind = STMT_INDICATOR_NTS;
		strcpy_s(data[i].logMessage, maxLogMessageLength, (*it).c_str()); //check above ensures that (*it).length() < maxLogMessageLength (which is the size of the target buffer)
		data[i].logMessage_ind = STMT_INDICATOR_NTS;
	}

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "INSERT INTO localmanagers_logmessages (ServiceDescriptorGUID, TimeOfInsertion,LogMessage) values (? , CURRENT_TIMESTAMP(), ?)";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[0].buffer = &data[0].guid;
	bind[0].u.indicator = &data[0].guid_ind;

	bind[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	bind[1].buffer = &data[0].logMessage;
	bind[1].u.indicator = &data[0].logMessage_ind;

	/* set array size */
	mysql_stmt_attr_set(sql_stmt, STMT_ATTR_ARRAY_SIZE, &array_size);

	/* set row size */
	mysql_stmt_attr_set(sql_stmt, STMT_ATTR_ROW_SIZE, &row_size);

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "addNewManagedLMLogMessages encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	delete[] data;

	PERFORMANCE_WATCH_FUNCTION_EXIT("addNewManagedLMLogMessages")
		return re;
}

bool GMDatabaseManager::deleteManagedLMLogMessagesIfMoreThan(int num) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedNum = num;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "DELETE ll FROM localmanagers_logmessages as ll JOIN ( SELECT ID FROM  localmanagers_logmessages ORDER BY ID DESC LIMIT 1 OFFSET ?  ) lllimit ON ll.ID <= lllimit.ID";
	mysql_stmt_prepare(sql_stmt, stmt, static_cast<unsigned long>(strlen(stmt)));

	//params
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &preparedNum;
	bind[0].is_null = 0;
	bind[0].is_unsigned = true;
	bind[0].length = NULL;

	mysql_stmt_bind_param(sql_stmt, bind);
	bool re = mysql_stmt_execute(sql_stmt) == 0;
	if (!re) {
		LOG(ERROR) << "deleteManagedLMLogMessagesIfMoreThan encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("deleteManagedLMLogMessagesIfMoreThan")
		return re;
}

bool GMDatabaseManager::deleteManagedLMLogMessagesOlderThanXSec(int olderThanInSeconds) {
	PERFORMANCE_WATCH_FUNCTION_ENTRY
		int preparedSec = olderThanInSeconds;

	//// prepared Statement
	MYSQL_STMT* sql_stmt = mysql_stmt_init(getDBConnection());

	const char* stmt = "DELETE FROM localmanagers_logmessages WHERE TimeOfInsertion < (CURRENT_TIMESTAMP() - INTERVAL ? SECOND)";
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
		LOG(ERROR) << "deleteManagedLMLogMessagesOlderThanXSec encountered the following error: " << mysql_stmt_error(sql_stmt);
	}

	mysql_stmt_close(sql_stmt);

	PERFORMANCE_WATCH_FUNCTION_EXIT("deleteManagedLMLogMessagesOlderThanXSec")
		return re;
}
