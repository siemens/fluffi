/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Pascal Eckmann
*/

#include "stdafx.h"
#include "InstanceMonitorWorker.h"
#include "LMWorkerThreadStateBuilder.h"
#include "LMDatabaseManager.h"
#include "CommInt.h"
#include "Util.h"
#include "LMWorkerThreadState.h"

InstanceMonitorWorker::InstanceMonitorWorker(CommInt* commInt, LMWorkerThreadStateBuilder* workerThreadStateBuilder, int loopIntervalMS, std::string location)
{
	m_commInt = commInt;
	m_workerThreadStateBuilder = workerThreadStateBuilder;
	m_loopIntervalMS = loopIntervalMS;
	m_location = location;
}

InstanceMonitorWorker::~InstanceMonitorWorker()
{
	delete m_thread;
	m_thread = nullptr;
}

void InstanceMonitorWorker::stop() {
	if (m_workerThreadState != nullptr) {
		m_workerThreadState->m_stopRequested = true;
	}
}

void InstanceMonitorWorker::workerMain() {
	m_workerThreadState = dynamic_cast<LMWorkerThreadState*>(m_workerThreadStateBuilder->constructState());
	if (m_workerThreadState == nullptr) {
		LOG(ERROR) << "InstanceMonitorWorker::workerMain - m_workerThreadStateBuilder->constructState() failed";
		return;
	}

	int checkAgainMS = 500;
	int waitedMS = 0;

	while (!m_workerThreadState->m_stopRequested)
	{
		if (waitedMS < m_loopIntervalMS) {
			std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
			waitedMS += checkAgainMS;
			continue;
		}
		else {
			waitedMS = 0;
		}

		m_workerThreadState->dbManager->deleteManagedInstanceStatusOlderThanXSec(60);
		m_workerThreadState->dbManager->deleteManagedInstanceLogMessagesIfMoreThan(1000);
		m_workerThreadState->dbManager->deleteManagedInstanceLogMessagesOlderThanXSec(60 * 60 * 24 * 7);

		// call get Status to all registered instances
		for (std::pair<FluffiServiceDescriptor, AgentType>& managedInstance : m_workerThreadState->dbManager->getAllRegisteredInstances(m_location)) {
			//Allow termination
			if (m_workerThreadState->m_stopRequested) {
				break;
			}

			LOG(DEBUG) << "Sending getStatus to: " << managedInstance.first.m_guid << " - " << managedInstance.first.m_serviceHostAndPort;

			FLUFFIMessage req;
			FLUFFIMessage resp;
			GetStatusRequest* statusRequest = new GetStatusRequest();

			ServiceDescriptor* ptrToServiceDescriptor = new ServiceDescriptor();
			ptrToServiceDescriptor->CopyFrom(m_commInt->getOwnServiceDescriptor().getProtobuf());
			statusRequest->set_allocated_requesterservicedescriptor(ptrToServiceDescriptor);

			req.set_allocated_getstatusrequest(statusRequest);

			if (!m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, managedInstance.first.m_serviceHostAndPort, CommInt::timeoutNormalMessage)) {
				//delete no longer responding instances
				LOG(INFO) << managedInstance.first.m_guid << " - " << managedInstance.first.m_serviceHostAndPort << " does not respond! Deleting it!";
				m_workerThreadState->dbManager->removeManagedInstance(managedInstance.first.m_guid, m_location);
				continue;
			}

			if (resp.fluff_case() != FLUFFIMessage::FluffCase::kGetStatusResponse) {
				LOG(ERROR) << "A getStatus request was not answered with a getStatus response";
				continue;
			}

			storeLogMessages(managedInstance, resp.getstatusresponse().logmessages());
			storeStatus(managedInstance, resp.getstatusresponse().status());
		}
	}

	m_workerThreadStateBuilder->destructState(m_workerThreadState);
}

void InstanceMonitorWorker::storeStatus(const std::pair<FluffiServiceDescriptor, AgentType>& managedInstance, std::string statusToStore) {
	//Transform total executions into exections / sec
	if (managedInstance.second == AgentType::TestcaseRunner) {
		std::vector<std::string> statusElements = Util::splitString(statusToStore, "|");

		statusToStore = "";
		for (size_t i = 0; i < statusElements.size(); i++) {
			if (i != 0) {
				statusToStore = statusToStore + "|";
			}
			if (statusElements[i].substr(0, 32) == " TestcasesSinceLastStatusRequest") {
				std::vector<std::string> testcasesSinceLastRequestElements = Util::splitString(statusElements[i], " ");
				int testcasesSinceLastRequestElementsAsInt;
				try {
					testcasesSinceLastRequestElementsAsInt = std::stoi(testcasesSinceLastRequestElements[2]);
				}
				catch (...) {
					LOG(ERROR) << "std::stoi failed";
					google::protobuf::ShutdownProtobufLibrary();
					_exit(EXIT_FAILURE); //make compiler happy
				}
				statusToStore = statusToStore + " TestcasesPerSecond " + std::to_string(static_cast<double>(testcasesSinceLastRequestElementsAsInt * 1000) / static_cast<double>(m_loopIntervalMS)) + " ";
			}
			else {
				statusToStore = statusToStore + statusElements[i];
			}
		}
	}

	m_workerThreadState->dbManager->addNewManagedInstanceStatus(managedInstance.first.m_guid, statusToStore);
}

void InstanceMonitorWorker::storeLogMessages(const std::pair<FluffiServiceDescriptor, AgentType>& managedInstance, const google::protobuf::RepeatedPtrField<std::string> & logMessages) {
	std::vector<std::string> MessagesAsVector;
	for (int i = 0; i < logMessages.size(); i++) {
		MessagesAsVector.push_back(logMessages.Get(i));
	}
	m_workerThreadState->dbManager->addNewManagedInstanceLogMessages(managedInstance.first.m_guid, MessagesAsVector);
}
