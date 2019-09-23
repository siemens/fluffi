/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier
*/

§§#include "stdafx.h"
§§#include "LMMonitorWorker.h"
#include "GMWorkerThreadStateBuilder.h"
#include "GMWorkerThreadState.h"
#include "GMDatabaseManager.h"
#include "CommInt.h"
§§
§§LMMonitorWorker::LMMonitorWorker(CommInt* commInt, GMWorkerThreadStateBuilder* workerThreadStateBuilder, int loopIntervalMS)
§§{
§§	m_commInt = commInt;
§§	m_workerThreadStateBuilder = workerThreadStateBuilder;
§§	m_loopIntervalMS = loopIntervalMS;
§§}
§§
§§LMMonitorWorker::~LMMonitorWorker()
§§{
§§	delete m_thread;
§§	m_thread = nullptr;
§§}
§§
void LMMonitorWorker::stop() {
	if (m_workerThreadState != nullptr) {
		m_workerThreadState->m_stopRequested = true;
	}
}

§§void LMMonitorWorker::workerMain()
§§{
	m_workerThreadState = static_cast<GMWorkerThreadState*>(m_workerThreadStateBuilder->constructState());
	if (m_workerThreadState == nullptr) {
		LOG(ERROR) << "LMMonitorWorker::workerMain - m_workerThreadStateBuilder->constructState() failed";
		return;
	}
§§
§§	int checkAgainMS = 500;
§§	int waitedMS = 0;
§§
	while (!m_workerThreadState->m_stopRequested)
§§	{
§§		if (waitedMS < m_loopIntervalMS) {
§§			std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
§§			waitedMS += checkAgainMS;
§§			continue;
§§		}
§§		else {
§§			waitedMS = 0;
§§		}
§§
§§		for (auto& newCommand : m_workerThreadState->dbManager->getNewCommands())
§§		{
§§			int commandID;
§§			std::string command, argument;

§§			std::tie(commandID, command, argument) = newCommand;
§§
§§			if (commandHandler(command, argument))
§§			{
§§				m_workerThreadState->dbManager->setCommandAsDone(commandID, "");
§§			}
§§			else
§§			{
§§				m_workerThreadState->dbManager->setCommandAsDone(commandID, "Could not perform command");
§§			}
§§		}
§§
		m_workerThreadState->dbManager->deleteDoneCommandsOlderThanXSec(60 * 30);
		m_workerThreadState->dbManager->deleteManagedLMStatusOlderThanXSec(60 * 3);
		m_workerThreadState->dbManager->deleteWorkersNotSeenSinceXSec(60 * 1);
§§
§§		// call get Status to all registered instances
§§		for (auto& managedInstance : m_workerThreadState->dbManager->getAllRegisteredLMs()) {
§§			LOG(DEBUG) << "Sending getStatus to: " << managedInstance.first << " - " << managedInstance.second;
§§
§§			FLUFFIMessage req;
§§			FLUFFIMessage resp;
§§			GetStatusRequest* statusRequest = new GetStatusRequest();

			ServiceDescriptor* ptrToServiceDescriptor = new ServiceDescriptor();
			ptrToServiceDescriptor->CopyFrom(m_commInt->getOwnServiceDescriptor().getProtobuf());
			statusRequest->set_allocated_requesterservicedescriptor(ptrToServiceDescriptor);

§§			req.set_allocated_getstatusrequest(statusRequest);
§§
§§			if (!m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, managedInstance.second, CommInt::timeoutNormalMessage)) {
§§				//delete no longer responding instances
§§				LOG(INFO) << managedInstance.first << " - " << managedInstance.second << " does not respond! Deleting it!";
§§				m_workerThreadState->dbManager->removeManagedLM(managedInstance.first);
§§				continue;
§§			}
§§
§§			if (resp.fluff_case() != FLUFFIMessage::FluffCase::kGetStatusResponse) {
§§				LOG(ERROR) << "A getStatus request was not answered with a getStatus response";
§§				continue;
§§			}
§§
§§			m_workerThreadState->dbManager->addNewManagedLMStatus(managedInstance.first, resp.getstatusresponse().status());
§§		}
§§	}
§§
§§	m_workerThreadStateBuilder->destructState(m_workerThreadState);
§§}
§§
§§bool LMMonitorWorker::commandHandler(std::string command, std::string argument)
§§{
§§	if (command == "kill")
§§	{
§§		FLUFFIMessage req;
§§		FLUFFIMessage resp;
§§		KillInstanceRequest* killInstanceRequest = new KillInstanceRequest();
§§		req.set_allocated_killinstancerequest(killInstanceRequest);
		if (!m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, argument, 1000))
§§		{
§§			LOG(INFO) << "Could not kill " << argument;
§§			return false;
§§		}
§§		if (resp.fluff_case() != FLUFFIMessage::FluffCase::kKillInstanceResponse)
§§		{
§§			LOG(ERROR) << "A killInstance request was not answered with a killInstance response";
§§			return false;
§§		}
§§		return true;
§§	}
§§	else // not a recognized command
§§	{
§§		return false;
§§	}
}
