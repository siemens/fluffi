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
§§#include "InstanceControlWorker.h"
§§#include "LMWorkerThreadStateBuilder.h"
§§#include "CommInt.h"
§§#include "WorkerWeightCalculator.h"
§§#include "FluffiServiceAndWeight.h"
§§#include "StatusOfInstance.h"
§§#include "LMWorkerThreadState.h"
§§#include "LMDatabaseManager.h"
§§
§§InstanceControlWorker::InstanceControlWorker(CommInt* commInt, LMWorkerThreadStateBuilder* workerThreadStateBuilder, int loopIntervalMS, std::string location)
§§{
§§	m_commInt = commInt;
§§	m_workerThreadStateBuilder = workerThreadStateBuilder;
§§	m_loopIntervalMS = loopIntervalMS;
§§	m_location = location;
§§}
§§
§§InstanceControlWorker::~InstanceControlWorker()
§§{
§§	delete m_thread;
§§	m_thread = nullptr;
§§}
§§
§§void InstanceControlWorker::stop() {
§§	if (m_workerThreadState != nullptr) {
§§		m_workerThreadState->m_stopRequested = true;
§§	}
§§}
§§
§§void InstanceControlWorker::workerMain() {
§§	m_workerThreadState = dynamic_cast<LMWorkerThreadState*>(m_workerThreadStateBuilder->constructState());
§§	if (m_workerThreadState == nullptr) {
§§		LOG(ERROR) << "InstanceControlWorker::workerMain - m_workerThreadStateBuilder->constructState() failed";
§§		return;
§§	}
§§
§§	int checkAgainMS = 500;
§§	int waitedMS = 0;
§§
§§	while (!m_workerThreadState->m_stopRequested)
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
§§		//Actual loop body
§§
§§		LOG(DEBUG) << "Set TGs and TEs on all TRs!";
§§		setTGsAndTEsAtTRs();
§§	}
§§	m_workerThreadStateBuilder->destructState(m_workerThreadState);
§§}
§§
§§void InstanceControlWorker::setTGsAndTEsAtTRs()
§§{
§§	WorkerWeightCalculator weightCalculator = WorkerWeightCalculator(m_workerThreadState->dbManager, m_location);
§§	weightCalculator.updateStatusInformation();
§§
§§	FLUFFIMessage req;
§§	FLUFFIMessage resp;
§§
§§	SetTGsAndTEsRequest* setTGsAndTEsReqest = new SetTGsAndTEsRequest();
§§
§§	//Gather all testcase evaluators and weight them
§§	auto evaluators = weightCalculator.getEvaluatorStatus();
§§	for (auto&& it : evaluators)
§§	{
§§		FluffiServiceDescriptor serviceDescriptor{ it.serviceDescriptorHostAndPort, it.serviceDescriptorGUID };
§§		FluffiServiceAndWeight serviceAndWeight{ serviceDescriptor, it.weight };
§§
§§		ServiceAndWeigth* serviceAndWeightOfTE = setTGsAndTEsReqest->add_tes();
§§		serviceAndWeightOfTE->CopyFrom(serviceAndWeight.getProtobuf());
§§	}
§§
§§	auto generators = weightCalculator.getGeneratorStatus();
§§	for (auto&& it : generators)
§§	{
§§		FluffiServiceDescriptor serviceDescriptor{ it.serviceDescriptorHostAndPort, it.serviceDescriptorGUID };
§§		FluffiServiceAndWeight serviceAndWeight{ serviceDescriptor, it.weight };
§§
§§		ServiceAndWeigth* serviceAndWeightOfTG = setTGsAndTEsReqest->add_tgs();
§§		serviceAndWeightOfTG->CopyFrom(serviceAndWeight.getProtobuf());
§§	}
§§
§§	req.set_allocated_settgsandtesrequest(setTGsAndTEsReqest);
§§
§§	//inform all testcase runners (for now: all get the same TEs, TGs, and weights)
§§	std::vector<FluffiServiceDescriptor> registeredTestcaseRunners = m_workerThreadState->dbManager->getRegisteredInstancesOfAgentType(AgentType::TestcaseRunner, m_location);
§§
§§	//shuffle runners to improve performance when many runners are stopped and new are started in a short timeframe
§§	std::random_shuffle(registeredTestcaseRunners.begin(), registeredTestcaseRunners.end());
§§
§§	for (auto&& it : registeredTestcaseRunners) {
§§		//Allow termination
§§		if (m_workerThreadState->m_stopRequested) {
§§			break;
§§		}
§§
§§		bool success = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, it.m_serviceHostAndPort, CommInt::timeoutNormalMessage);
§§		if (success)
§§		{
§§			LOG(DEBUG) << "Set TGs and TEs on " << it.m_guid << " - " << it.m_serviceHostAndPort << " successful!";
§§		}
§§		else
§§		{
§§			LOG(DEBUG) << "Set TGs and TEs on " << it.m_guid << " - " << it.m_serviceHostAndPort << " failed! Deleting it!";
§§			m_workerThreadState->dbManager->removeManagedInstance(it.m_guid, m_location);
§§		}
§§	}
§§}
