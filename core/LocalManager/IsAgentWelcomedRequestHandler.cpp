§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Abian Blome
§§*/
§§
§§#include "stdafx.h"
§§#include "IsAgentWelcomedRequestHandler.h"
§§#include "FluffiSetting.h"
§§#include "LMWorkerThreadState.h"
§§#include "LMDatabaseManager.h"
§§#include "Util.h"
§§
§§IsAgentWelcomedRequestHandler::IsAgentWelcomedRequestHandler()
§§{
§§}
§§
§§IsAgentWelcomedRequestHandler::~IsAgentWelcomedRequestHandler()
§§{
§§}
§§
§§void IsAgentWelcomedRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp) {
§§	LMWorkerThreadState* lmWorkerThreadState = dynamic_cast<LMWorkerThreadState*>(workerThreadState);
§§	if (lmWorkerThreadState == nullptr) {
§§		LOG(ERROR) << "IsAgentWelcomedRequestHandler::handleFLUFFIMessage - workerThreadState cannot be accessed";
§§		return;
§§	}
§§
§§	IsAgentWelcomedResponse* isAgentWelcomedResponse = new IsAgentWelcomedResponse();
§§
§§	google::protobuf::RepeatedPtrField< std::string > implementedSubtypes = req->isagentwelcomedrequest().implementedagentsubtypes();
§§	std::string subtypes;
§§	{
§§		std::stringstream oss;
§§		for (int i = 0; i < implementedSubtypes.size(); i++) {
§§			if (i != 0) {
§§				oss << "|";
§§			}
§§			oss << implementedSubtypes.Get(i);
§§		}
§§		subtypes = oss.str();
§§	}
§§	LOG(DEBUG) << "Incoming isAgentWelcomedRequest";
§§	LOG(DEBUG) << "Type=" << Util::agentTypeToString(req->isagentwelcomedrequest().type());
§§	LOG(DEBUG) << "SubTypes=" << subtypes;
§§
§§	std::deque<FluffiSetting> settings = lmWorkerThreadState->dbManager->getAllSettings();
§§	std::set<std::string> acceptedAgentSubTypes;
§§	switch (req->isagentwelcomedrequest().type()) {
§§	case AgentType::TestcaseGenerator:
§§		for (auto& setting : settings) {
§§			if (setting.m_settingName == "generatorTypes") {
§§				//generatorTypes is of format "A=50|B=50"
§§				std::vector<std::string> acceptedGenTypesAndPercentages = Util::splitString(setting.m_settingValue, "|");
§§				for (size_t i = 0; i < acceptedGenTypesAndPercentages.size(); i++) {
§§					acceptedAgentSubTypes.insert(Util::splitString(acceptedGenTypesAndPercentages[i], "=")[0]);
§§				}
§§				break;
§§			}
§§		}
§§		break;
§§	case AgentType::TestcaseRunner:
§§		for (auto& setting : settings) {
§§			if (setting.m_settingName == "runnerType") {
§§				acceptedAgentSubTypes.insert(setting.m_settingValue);
§§				break;
§§			}
§§		}
§§		break;
§§	case AgentType::TestcaseEvaluator:
§§		for (auto& setting : settings) {
§§			if (setting.m_settingName == "evaluatorTypes") {
§§				//evaluatorTypes is of format "A=50|B=50"
§§				std::vector<std::string> acceptedEvalTypesAndPercentages = Util::splitString(setting.m_settingValue, "|");
§§				for (size_t i = 0; i < acceptedEvalTypesAndPercentages.size(); i++) {
§§					acceptedAgentSubTypes.insert(Util::splitString(acceptedEvalTypesAndPercentages[i], "=")[0]);
§§				}
§§				break;
§§			}
§§		}
§§		break;
§§	default:
§§		LOG(ERROR) << "An agent of type " << Util::agentTypeToString(req->isagentwelcomedrequest().type()) << "asked if it is welcomed. This is not implemented!";
§§		isAgentWelcomedResponse->set_iswelcomed(false);
§§		resp->set_allocated_isagentwelcomedresponse(isAgentWelcomedResponse);
§§		return;
§§	}
§§
§§	std::string acceptedSubTypes;
§§	{
§§		std::stringstream oss;
§§		bool isFirst = true;
§§		for (auto& subtype : acceptedAgentSubTypes) {
§§			if (!isFirst) {
§§				oss << "|";
§§			}
§§			else {
§§				isFirst = false;
§§			}
§§			oss << subtype;
§§		}
§§		acceptedSubTypes = oss.str();
§§	}
§§	LOG(DEBUG) << "Accepted agent subtypes are:" << acceptedSubTypes;
§§
§§	//iff the requester implements one of the desired subagenttypes, it is welcomed :)
§§	isAgentWelcomedResponse->set_iswelcomed(false);
§§	google::protobuf::RepeatedPtrField< std::string > sts = req->isagentwelcomedrequest().implementedagentsubtypes();
§§	for (int i = 0; i < implementedSubtypes.size(); i++) {
§§		if (acceptedAgentSubTypes.find(implementedSubtypes.Get(i)) != acceptedAgentSubTypes.end()) {
§§			isAgentWelcomedResponse->set_iswelcomed(true);
§§			break;
§§		}
§§	}
§§
§§	if (!isAgentWelcomedResponse->iswelcomed()) {
§§		LOG(WARNING) << "Agent " << Util::agentTypeToString(req->isagentwelcomedrequest().type()) << " with implemented subtypes " << subtypes << " will receive a \"reject\". If this is not desired adapt generatorTypes, evaluatorTypes, and runnerType";
§§	}
§§
§§	LOG(DEBUG) << "A IsAgentWelcomedRequest will return " << (isAgentWelcomedResponse->iswelcomed() ? "true" : "false");
§§	resp->set_allocated_isagentwelcomedresponse(isAgentWelcomedResponse);
§§}
