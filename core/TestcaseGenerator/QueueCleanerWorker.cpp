/*
Copyright 2017-2020 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "QueueCleanerWorker.h"
#include "CommInt.h"
#include "TGWorkerThreadStateBuilder.h"
#include "TGTestcaseManager.h"
#include "TestcaseDescriptor.h"
#include "Util.h"
#include "TGWorkerThreadState.h"
#include "GarbageCollectorWorker.h"
#include "FluffiSetting.h"

QueueCleanerWorker::QueueCleanerWorker(CommInt* commInt, TGWorkerThreadStateBuilder* workerThreadStateBuilder, int intervallBetweenTwoCleaningRoundsInMillisec, int maxRetriesBeforeReport, std::string testcaseDir, TGTestcaseManager*  testcaseManager, GarbageCollectorWorker* garbageCollectorWorker)
	: m_commInt(commInt),
	m_workerThreadStateBuilder(workerThreadStateBuilder),
	m_intervallBetweenTwoCleaningRoundsInMillisec(intervallBetweenTwoCleaningRoundsInMillisec),
	m_intervallToWaitForReinsertionInMillisec(0),
	m_maxRetriesBeforeReport(maxRetriesBeforeReport),
	m_testcaseDir(testcaseDir),
	m_testcaseManager(testcaseManager),
	m_serverTimestampOfNewestEvaluationResult(0),
	m_garbageCollectorWorker(garbageCollectorWorker)
{
}

QueueCleanerWorker::~QueueCleanerWorker()
{
	delete m_thread;
	m_thread = nullptr;
}

void QueueCleanerWorker::stop() {
	if (m_workerThreadState != nullptr) {
		m_workerThreadState->m_stopRequested = true;
	}
}

void QueueCleanerWorker::workerMain() {
	m_workerThreadState = dynamic_cast<TGWorkerThreadState*>(m_workerThreadStateBuilder->constructState());
	if (m_workerThreadState == nullptr) {
		LOG(ERROR) << "QueueCleanerWorker::workerMain - m_workerThreadStateBuilder->constructState() failed";
		return;
	}

	int checkAgainMS = 500;
	int waitedMS = 0;

	while (!m_workerThreadState->m_stopRequested)
	{
		if (waitedMS < m_intervallBetweenTwoCleaningRoundsInMillisec) {
			std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
			waitedMS += checkAgainMS;
			continue;
		}
		else {
			waitedMS = 0;
		}

		//Actual loop body
		removeCompletedTestcases();
		reportTestcasesWithMultipleNoResponse();
		reinsertTestcasesWithNoResponse();
	}
	m_workerThreadStateBuilder->destructState(m_workerThreadState);
}

void QueueCleanerWorker::removeCompletedTestcases()
{
	LOG(DEBUG) << "Sending sendGetCompletedTestcaseIDsRequest";

	FLUFFIMessage req;
	FLUFFIMessage resp;

	ServiceDescriptor* ptMySelfServiceDescriptor = new ServiceDescriptor();
	ptMySelfServiceDescriptor->CopyFrom(m_commInt->getOwnServiceDescriptor().getProtobuf());

	GetNewCompletedTestcaseIDsRequest* getRequest = new GetNewCompletedTestcaseIDsRequest();
	getRequest->set_lastupdatetimestamp(m_serverTimestampOfNewestEvaluationResult);

	req.set_allocated_getnewcompletedtestcaseidsrequest(getRequest);

	bool success = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, CommInt::timeoutNormalMessage);
	if (!success) {
		LOG(ERROR) << "Sending sendGetCompletedTestcaseIDsRequest failed!";
		return;
	}

	m_serverTimestampOfNewestEvaluationResult = resp.getnewcompletedtestcaseidsresponse().updatetimestamp();
	LOG(DEBUG) << "New timestamp: " << m_serverTimestampOfNewestEvaluationResult;

	m_testcaseManager->removeEvaluatedTestcases(resp.getnewcompletedtestcaseidsresponse().ids());
}

void QueueCleanerWorker::reinsertTestcasesWithNoResponse()
{
	if (m_intervallToWaitForReinsertionInMillisec == 0) {
		m_intervallToWaitForReinsertionInMillisec = 5 * 60 * 1000; // should be well above LM.forceCompletedTestcasesCacheFlushAfterMS

		//Get hang timeout and make sure m_intervallToWaitForReinsertionInMillisec is at least 2* m_intervallToWaitForReinsertionInMillisec
		{
			FLUFFIMessage req;
			FLUFFIMessage resp;
			GetFuzzJobConfigurationRequest* getfuzzjobconfigurationrequest = new GetFuzzJobConfigurationRequest();
			ServiceDescriptor* ptMySelfServiceDescriptor = new ServiceDescriptor();
			ptMySelfServiceDescriptor->CopyFrom(m_commInt->getOwnServiceDescriptor().getProtobuf());
			getfuzzjobconfigurationrequest->set_allocated_servicedescriptor(ptMySelfServiceDescriptor);
			req.set_allocated_getfuzzjobconfigurationrequest(getfuzzjobconfigurationrequest);

			bool respReceived = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, CommInt::timeoutNormalMessage);
			if (!respReceived) {
				LOG(ERROR) << "No GetFuzzJobConfigurationResponse received!";
				return;
			}

			LOG(DEBUG) << "GetFuzzJobConfigurationResponse successfully received!";
			const GetFuzzJobConfigurationResponse* fjcr = &(resp.getfuzzjobconfigurationresponse());

			std::map<std::string, std::string> settings;

			for (auto i : fjcr->settings()) {
				FluffiSetting fs = FluffiSetting(i);
				settings.insert(std::make_pair(fs.m_settingName, fs.m_settingValue));
			}

			if (settings.count("hangTimeout") == 0) {
				LOG(WARNING) << "QueueCleanerWorker :: No hangTimeout value received!";
				return;
			}
			else {
				try {
					int hangTimeout = std::stoi(settings["hangTimeout"]);
					if (m_intervallToWaitForReinsertionInMillisec < 2 * hangTimeout) {
						m_intervallToWaitForReinsertionInMillisec = 2 * hangTimeout;
					}
				}
				catch (...) {
					LOG(ERROR) << "std::stoi failed";
					google::protobuf::ShutdownProtobufLibrary();
					_exit(EXIT_FAILURE); //make compiler happy
				}
			}
		}
		LOG(INFO) << "Setting the intervall to wait for reinsertion in millisec to " << m_intervallToWaitForReinsertionInMillisec;
	}

	std::chrono::time_point<std::chrono::steady_clock> timeBeforeWhichShouldBeReinserted = std::chrono::steady_clock::now() - std::chrono::milliseconds(m_intervallToWaitForReinsertionInMillisec);

	m_testcaseManager->reinsertTestcasesHavingNoAnswerSince(timeBeforeWhichShouldBeReinserted);
}

void QueueCleanerWorker::reportTestcasesWithMultipleNoResponse() {
	std::vector<TestcaseDescriptor> testcasesThatNeedToBeReported =
		m_testcaseManager->handMeAllTestcasesWithTooManyRetries(m_maxRetriesBeforeReport);

	while (!testcasesThatNeedToBeReported.empty()) {
		TestcaseDescriptor tcToReport = testcasesThatNeedToBeReported.back();
		testcasesThatNeedToBeReported.pop_back();

		FLUFFIMessage req;
		FLUFFIMessage resp;

		TestcaseID* mutableTestcaseId = new TestcaseID();
		mutableTestcaseId->CopyFrom(tcToReport.getId().getProtobuf());
		TestcaseID* mutableParentId = new TestcaseID();
		mutableParentId->CopyFrom(tcToReport.getparentId().getProtobuf());

		bool isLastChunk;
		FluffiTestcaseID testcaseID = tcToReport.getId();
		std::string data = Util::loadTestcaseChunkInMemory(testcaseID, m_testcaseDir, 0, &isLastChunk);

		ReportTestcaseWithNoResultRequest* reportRequest = new ReportTestcaseWithNoResultRequest();
		reportRequest->set_allocated_id(mutableTestcaseId);
		reportRequest->set_allocated_parentid(mutableParentId);
		reportRequest->set_testcasefirstchunk(data);
		reportRequest->set_islastchunk(isLastChunk);

		req.set_allocated_reporttestcasewithnoresultrequest(reportRequest);

		bool success = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, CommInt::timeoutFileTransferMessage);
		if (!success) {
			LOG(ERROR) << "Sending ReportTestcaseWithNoResultRequest failed!";
			m_testcaseManager->pushNewGeneratedTestcase(tcToReport); //try again later
		}
		else {
			LOG(DEBUG) << "Sending ReportTestcaseWithNoResultRequest succeeded - deleting file...";
			tcToReport.deleteFile(m_garbageCollectorWorker);
		}
	}
}
