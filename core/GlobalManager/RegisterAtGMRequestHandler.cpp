§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Abian Blome, Pascal Eckmann
§§*/
§§
§§#include "stdafx.h"
§§#include "RegisterAtGMRequestHandler.h"
§§#include "CommInt.h"
§§#include "Util.h"
§§#include "GMDatabaseManager.h"
§§#include "GMWorkerThreadState.h"
§§#include "FluffiLMConfiguration.h"
§§
§§RegisterAtGMRequestHandler::RegisterAtGMRequestHandler(CommInt* commPtr) :
§§	m_comm(commPtr)
§§{
§§}
§§
§§RegisterAtGMRequestHandler::~RegisterAtGMRequestHandler()
§§{
§§}
§§
§§void RegisterAtGMRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp)
§§{
§§	GMWorkerThreadState* gmWorkerThreadState = dynamic_cast<GMWorkerThreadState*>(workerThreadState);
§§	if (gmWorkerThreadState == nullptr) {
§§		LOG(ERROR) << "RegisterAtGMRequestHandler::handleFLUFFIMessage - workerThreadState cannot be accessed";
§§		return;
§§	}
§§
§§	AgentType type = req->registeratgmrequest().type();
§§	std::string location = req->registeratgmrequest().location();
§§	google::protobuf::RepeatedPtrField< std::string > sts = req->registeratgmrequest().implementedagentsubtypes();
§§	std::stringstream oss;
§§	for (int i = 0; i < sts.size(); i++) {
§§		if (i != 0) {
§§			oss << "|";
§§		}
§§		oss << sts.Get(i);
§§	}
§§	std::string subtypes = oss.str();
§§	FluffiServiceDescriptor fsd(req->registeratgmrequest().servicedescriptor());
§§	GMDatabaseManager* dbManager = gmWorkerThreadState->dbManager;
§§	LOG(DEBUG) << "Incoming registration";
§§	LOG(DEBUG) << "Type=" << Util::agentTypeToString(type);
§§	LOG(DEBUG) << "SubTypes=" << subtypes;
§§	LOG(DEBUG) << "ServiceHAP=" << fsd.m_serviceHostAndPort;
§§	LOG(DEBUG) << "ServiceGUID=" << fsd.m_guid;
§§	LOG(DEBUG) << "Location=" << location;
§§
§§	RegisterAtGMResponse* registerResponse = new RegisterAtGMResponse();
§§	FluffiServiceDescriptor lmServiceDescriptor = FluffiServiceDescriptor::getNullObject();
§§
§§	switch (type)
§§	{
§§	case TestcaseGenerator:
§§	case TestcaseRunner:
§§	case TestcaseEvaluator:
§§
§§		//Frist check, if the user assigned a lm by hand
§§		lmServiceDescriptor = dbManager->getLMServiceDescriptorForWorker(fsd);//returns NullObject upon error
§§
§§		//check if the agent is welcomed there
§§		if (!lmServiceDescriptor.isNullObject())
§§		{
§§			if (!agentIsWelcomedAt(workerThreadState, type, sts, lmServiceDescriptor)) {
§§				lmServiceDescriptor = FluffiServiceDescriptor::getNullObject();
§§				dbManager->removeWorkerFromDatabase(fsd); //remove it, if it was already in the db. It will be added later with "no fuzzjob" and will therefore re-appear in the GUI
§§			}
§§		}
§§
§§		//if it is decided, what the agent should do: tell it about it
§§		if (!lmServiceDescriptor.isNullObject())
§§		{
§§			ServiceDescriptor* ptrToServiceDescriptor = new ServiceDescriptor();
§§			ptrToServiceDescriptor->CopyFrom(lmServiceDescriptor.getProtobuf());
§§			registerResponse->set_allocated_lmservicedescriptor(ptrToServiceDescriptor);
§§			LOG(DEBUG) << "Sending message with LMServiceDescriptor" << lmServiceDescriptor.m_guid << "/" << lmServiceDescriptor.m_serviceHostAndPort;
§§		}
§§		else
§§		{
§§			//store the worker in the database and let the user decide by hand
§§			bool success = dbManager->addWorkerToDatabase(fsd, type, subtypes, location);
§§			if (!success)
§§			{
§§				LOG(ERROR) << "Could not register worker in DB";
§§			}
§§
§§			registerResponse->set_retrylater(true);
§§			LOG(DEBUG) << "Sending message with retryLater";
§§		}
§§
§§		break;
§§	case LocalManager:
§§		try
§§		{
§§			// Check if we still need a local manager
§§			int fuzzjob = static_cast<int>(dbManager->getFuzzJobWithoutLM(location));
§§			if (fuzzjob == -1)
§§			{
§§				//Store LocalManager in database so we know which idle local managers there are
§§				bool success = dbManager->addWorkerToDatabase(fsd, type, subtypes, location);
§§				if (!success)
§§				{
§§					LOG(ERROR) << "Could not register worker in DB";
§§				}
§§
§§				LOG(DEBUG) << "Sending message with retryLater";
§§				registerResponse->set_retrylater(true);
§§			}
§§			else
§§			{
§§				//A race condition could happen here, if multiple LocalManagers request at the same time. Currently the last one wins due to constraints in the database
§§				FluffiLMConfiguration lmConfiguration = dbManager->getLMConfigurationForFuzzJob(fuzzjob);
§§				LMConfiguration* ptrToLMConfiguration = new LMConfiguration();
§§				ptrToLMConfiguration->CopyFrom(lmConfiguration.getProtobuf());
§§				registerResponse->set_allocated_lmconfiguration(ptrToLMConfiguration);
§§				LOG(DEBUG) << "Sending message with LMConfiguration";
§§				gmWorkerThreadState->dbManager->setLMForLocationAndFuzzJob(location, req->registeratgmrequest().servicedescriptor(), fuzzjob);
§§				LOG(DEBUG) << "Registering LM";
§§			}
§§		}
§§		catch (std::exception &e)
§§		{
§§			registerResponse->set_retrylater(true);
§§			LOG(ERROR) << "An error occured while trying to get a configuration for the local manager" << e.what();
§§		}
§§		break;
§§	case GlobalManager:
§§	default:
§§		registerResponse->set_retrylater(true);
§§		LOG(ERROR) << "An invalid agent type tried to register at the global manager";
§§		break;
§§	}
§§
§§	resp->set_allocated_registeratgmresponse(registerResponse);
§§}
§§
§§bool RegisterAtGMRequestHandler::agentIsWelcomedAt(WorkerThreadState* workerThreadState,
§§	AgentType type,
§§	const google::protobuf::RepeatedPtrField<std::string> sts,
§§	const FluffiServiceDescriptor lmServiceDescriptor
§§) {
§§	FLUFFIMessage req;
§§	FLUFFIMessage resp;
§§	IsAgentWelcomedRequest* isAgentWelcomedRequest = new IsAgentWelcomedRequest();
§§	isAgentWelcomedRequest->set_type(type);
§§	for (int i = 0; i < sts.size(); i++) {
§§		isAgentWelcomedRequest->add_implementedagentsubtypes(sts.Get(i));
§§	}
§§	req.set_allocated_isagentwelcomedrequest(isAgentWelcomedRequest);
§§	if (!m_comm->sendReqAndRecvResp(&req, &resp, workerThreadState, lmServiceDescriptor.m_serviceHostAndPort, CommInt::timeoutNormalMessage))
§§	{
§§		LOG(INFO) << "Could not contact " << lmServiceDescriptor.m_serviceHostAndPort;
§§		return false;
§§	}
§§	if (resp.fluff_case() != FLUFFIMessage::FluffCase::kIsAgentWelcomedResponse)
§§	{
§§		LOG(ERROR) << "A IsAgentWelcomedRequest request was not answered with a IsAgentWelcomedResponse response";
§§		return false;
§§	}
§§
§§	LOG(DEBUG) << "A IsAgentWelcomedRequest returned " << (resp.isagentwelcomedresponse().iswelcomed() ? "true" : "false");
§§	return resp.isagentwelcomedresponse().iswelcomed();
§§}
