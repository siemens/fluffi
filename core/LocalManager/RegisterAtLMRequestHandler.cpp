/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "RegisterAtLMRequestHandler.h"
#include "FluffiServiceDescriptor.h"
#include "FluffiSetting.h"
#include "Util.h"
#include "LMDatabaseManager.h"
#include "LMWorkerThreadState.h"

RegisterAtLMRequestHandler::RegisterAtLMRequestHandler(std::string mylocation) :
	m_mylocation(mylocation),
	m_myfuzzjob("NOTYETDEFINED")
{
}

RegisterAtLMRequestHandler::~RegisterAtLMRequestHandler()
{
}

void RegisterAtLMRequestHandler::setMyFuzzjob(const std::string myfuzzjob) {
	m_myfuzzjob = myfuzzjob;
}

void RegisterAtLMRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp) {
	LMWorkerThreadState* lmWorkerThreadState = dynamic_cast<LMWorkerThreadState*>(workerThreadState);
	if (lmWorkerThreadState == nullptr) {
		LOG(ERROR) << "RegisterAtLMRequestHandler::handleFLUFFIMessage - workerThreadState cannot be accessed";
		return;
	}

	AgentType type = req->registeratlmrequest().type();
	google::protobuf::RepeatedPtrField< std::string > implementedSubTypes = req->registeratlmrequest().implementedagentsubtypes();
	std::stringstream oss;
	for (int i = 0; i < implementedSubTypes.size(); i++) {
		if (i != 0) {
			oss << "|";
		}
		oss << implementedSubTypes.Get(i);
	}
	std::string subtypes = oss.str();
	FluffiServiceDescriptor fsd(req->registeratlmrequest().servicedescriptor());
	bool success;
	LOG(DEBUG) << "Incoming registration";
	LOG(DEBUG) << "Type=" << Util::agentTypeToString(type);
	LOG(DEBUG) << "SubTypes=" << subtypes;
	LOG(DEBUG) << "ServiceDescriptor=" << fsd.m_serviceHostAndPort;
	LOG(DEBUG) << "Location=" << req->registeratlmrequest().location();

	RegisterAtLMResponse* registerResponse = new RegisterAtLMResponse();

	if (req->registeratlmrequest().location() != m_mylocation) {
		//only accept registrations in my location
		success = false;
	}
	else {
		if ("" != lmWorkerThreadState->dbManager->getRegisteredInstanceSubType(fsd.m_guid)) {
			//This agent is already in the managed instance database and it is already decided which subagent type it should have
			success = true;
		}
		else {
			//Decide which subagent type it should have and add the agent to the managed instances database
			switch (type)
			{
			case TestcaseGenerator:
			case TestcaseRunner:
			case TestcaseEvaluator:
			{
				std::string selectedSubAgentType = decideSubAgentType(lmWorkerThreadState->dbManager, type, implementedSubTypes);
				if (selectedSubAgentType == "") {
					LOG(ERROR) << "decideSubAgentType was not able to determine a valid subagenttype";
					success = false;
				}
				else {
					success = lmWorkerThreadState->dbManager->writeManagedInstance(fsd, type, selectedSubAgentType, req->registeratlmrequest().location());
				}
			}
			break;
			case GlobalManager:
			case LocalManager:
			case AgentType_INT_MIN_SENTINEL_DO_NOT_USE_:
			case AgentType_INT_MAX_SENTINEL_DO_NOT_USE_:
			default:
				success = false;
				break;
			}
		}

		if (success) {
			registerResponse->set_fuzzjobname(m_myfuzzjob);
		}
	}

	// response
	LOG(DEBUG) << "Sending success message with status " << (success ? "true" : "false");
	registerResponse->set_success(success);
	resp->set_allocated_registeratlmresponse(registerResponse);
}

std::string RegisterAtLMRequestHandler::decideSubAgentType(LMDatabaseManager* dbManager, AgentType type, const google::protobuf::RepeatedPtrField<std::string>& implementedSubtypes) {
	std::vector<std::pair<std::string, int>> desiredSubTypes;
	std::deque<FluffiSetting> settings = dbManager->getAllSettings();
	switch (type) {
	case AgentType::TestcaseGenerator:
		for (auto& setting : settings) {
			if (setting.m_settingName == "generatorTypes") {
				//generatorTypes is of format "A=50|B=50"
				std::vector<std::string> acceptedGenTypesAndPercentages = Util::splitString(setting.m_settingValue, "|");
				for (size_t i = 0; i < acceptedGenTypesAndPercentages.size(); i++) {
					std::vector<std::string> acceptedGenTypesAndPercentagesAsVector = Util::splitString(acceptedGenTypesAndPercentages[i], "=");
					if (acceptedGenTypesAndPercentagesAsVector.size() == 2) {
						try {
							desiredSubTypes.push_back(std::make_pair(acceptedGenTypesAndPercentagesAsVector[0], std::stoi(acceptedGenTypesAndPercentagesAsVector[1])));
						}
						catch (...) {
							LOG(ERROR) << "std::stoi failed";
							google::protobuf::ShutdownProtobufLibrary();
							_exit(EXIT_FAILURE); //make compiler happy
						}
					}
				}
				break;
			}
		}
		break;
	case AgentType::TestcaseEvaluator:
		for (auto& setting : settings) {
			if (setting.m_settingName == "evaluatorTypes") {
				//evaluatorTypes is of format "A=50|B=50"
				std::vector<std::string> acceptedEvalTypesAndPercentages = Util::splitString(setting.m_settingValue, "|");
				for (size_t i = 0; i < acceptedEvalTypesAndPercentages.size(); i++) {
					std::vector<std::string> acceptedEvalTypesAndPercentagesAsVector = Util::splitString(acceptedEvalTypesAndPercentages[i], "=");
					if (acceptedEvalTypesAndPercentagesAsVector.size() == 2) {
						try {
							desiredSubTypes.push_back(std::make_pair(acceptedEvalTypesAndPercentagesAsVector[0], std::stoi(acceptedEvalTypesAndPercentagesAsVector[1])));
						}
						catch (...) {
							LOG(ERROR) << "std::stoi failed";
							google::protobuf::ShutdownProtobufLibrary();
							_exit(EXIT_FAILURE); //make compiler happy
						}
					}
				}
				break;
			}
		}
		break;
	case AgentType::TestcaseRunner:
		for (auto& setting : settings) {
			if (setting.m_settingName == "runnerType") {
				desiredSubTypes.push_back(std::make_pair(setting.m_settingValue, 100));
				break;
			}
		}
		break;
	default:
		LOG(ERROR) << "An agent of type " << Util::agentTypeToString(type) << " wanted to have an decicion on its agent sub type. This is not implemented!";
		return "";
	}

	if (desiredSubTypes.size() == 0) {
		LOG(ERROR) << "The configuration is invalid - there is no desired agent sub type for agents of type " << Util::agentTypeToString(type);
		return "";
	}

	//normalize desiredSubTypes to 100
	{
		int sumOfAllSubtypes = 0;
		for (auto& subType : desiredSubTypes) {
			sumOfAllSubtypes += subType.second;
		}

		for (auto& subType : desiredSubTypes) {
			subType.second = subType.second * 100 / sumOfAllSubtypes;
		}
	}

	std::vector<std::pair<std::string, int>> currentActiveSubtypes = dbManager->getRegisteredInstancesNumOfSubTypes();

	//normalize currentActiveSubtypes to 100
	{
		int sumOfAllSubtypes = 0;
		for (size_t i = 0; i < currentActiveSubtypes.size(); i++) {
			sumOfAllSubtypes += currentActiveSubtypes[i].second;
		}

		for (size_t i = 0; i < currentActiveSubtypes.size(); i++) {
			currentActiveSubtypes[i].second = currentActiveSubtypes[i].second * 100 / sumOfAllSubtypes;
		}
	}

	std::map <std::string, int> desiredSubTypesAsMap;
	for (size_t i = 0; i < desiredSubTypes.size(); i++) {
		desiredSubTypesAsMap.insert(desiredSubTypes[i]);
	}
	std::map <std::string, int> currentActiveSubtypesAsMap;
	for (size_t i = 0; i < currentActiveSubtypes.size(); i++) {
		currentActiveSubtypesAsMap.insert(currentActiveSubtypes[i]);
	}

	std::map <std::string, int> subtypeDiffs;
	for (int i = 0; i < implementedSubtypes.size(); i++) {
		int currentVal = 0;
		if (currentActiveSubtypesAsMap.find(implementedSubtypes[i]) != currentActiveSubtypesAsMap.end()) {
			currentVal = currentActiveSubtypesAsMap[implementedSubtypes[i]];
		}

		if (desiredSubTypesAsMap.find(implementedSubtypes[i]) != desiredSubTypesAsMap.end()) {
			int desiredVal = desiredSubTypesAsMap[implementedSubtypes[i]];
			subtypeDiffs.insert(std::make_pair(implementedSubtypes[i], desiredVal - currentVal));
		}
	}

	//find the subagenttype that is needed most
	std::string maxIndex = "";
	int maxValue = -200;
	for (auto it = subtypeDiffs.begin(); it != subtypeDiffs.end(); ++it) {
		if (it->second > maxValue) {
			maxIndex = it->first;
			maxValue = it->second;
		}
	}

	return maxIndex;
}
