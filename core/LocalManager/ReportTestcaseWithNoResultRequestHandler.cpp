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
§§#include "ReportTestcaseWithNoResultRequestHandler.h"
§§#include "FluffiTestcaseID.h"
§§#include "LMDatabaseManager.h"
§§#include "Util.h"
§§#include "LMWorkerThreadState.h"
§§#include "GarbageCollectorWorker.h"
§§
§§ReportTestcaseWithNoResultRequestHandler::ReportTestcaseWithNoResultRequestHandler(std::string testcaseDir, CommInt* commPtr, GarbageCollectorWorker* garbageCollectorWorker) :
§§	m_testcaseDir(testcaseDir),
§§	m_commPtr(commPtr),
§§	m_garbageCollectorWorker(garbageCollectorWorker)
§§{
§§}
§§
§§ReportTestcaseWithNoResultRequestHandler::~ReportTestcaseWithNoResultRequestHandler()
§§{
§§}
§§
§§void ReportTestcaseWithNoResultRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp)
§§{
§§	LMWorkerThreadState* lmWorkerThreadState = dynamic_cast<LMWorkerThreadState*>(workerThreadState);
§§	if (lmWorkerThreadState == nullptr) {
§§		LOG(ERROR) << "ReportTestcaseWithNoResultRequestHandler::handleFLUFFIMessage - workerThreadState cannot be accessed";
§§		return;
§§	}
§§
§§	const ReportTestcaseWithNoResultRequest* reportTestcaseWithNoResultRequest = &req->reporttestcasewithnoresultrequest();
§§
§§	bool success = Util::storeTestcaseFileOnDisk(FluffiTestcaseID(reportTestcaseWithNoResultRequest->id()), m_testcaseDir, &reportTestcaseWithNoResultRequest->testcasefirstchunk(), reportTestcaseWithNoResultRequest->islastchunk(), reportTestcaseWithNoResultRequest->id().servicedescriptor().servicehostandport(), m_commPtr, workerThreadState, m_garbageCollectorWorker);
§§	if (!success) {
§§		LOG(ERROR) << "ReportTestcaseWithNoResultRequestHandler failed to store the testcase " << FluffiTestcaseID(reportTestcaseWithNoResultRequest->id()) << " on disk.";
§§	}
§§	else {
§§		//will delete the testcase file
§§		success = lmWorkerThreadState->dbManager->addEntryToInterestingTestcasesTable(FluffiTestcaseID(reportTestcaseWithNoResultRequest->id()), FluffiTestcaseID(reportTestcaseWithNoResultRequest->parentid()), 0, m_testcaseDir, LMDatabaseManager::TestCaseType::NoResponse);
§§
§§		if (!success) {
§§			LOG(ERROR) << "ReportTestcaseWithNoResultRequestHandler failed to add the testcase " << FluffiTestcaseID(reportTestcaseWithNoResultRequest->id()) << " to the database.";
§§		}
§§	}
§§
§§	// Build Response
§§	ReportTestcaseWithNoResultResponse* reportTestcaseWithNoResultResponse = new ReportTestcaseWithNoResultResponse();
§§	reportTestcaseWithNoResultResponse->set_success(success);
§§	resp->set_allocated_reporttestcasewithnoresultresponse(reportTestcaseWithNoResultResponse);
§§}
