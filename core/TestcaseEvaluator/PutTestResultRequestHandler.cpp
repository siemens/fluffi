§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Michael Kraus, Abian Blome
§§*/
§§
§§#include "stdafx.h"
§§#include "PutTestResultRequestHandler.h"
§§#include "FluffiTestcaseID.h"
§§#include "FluffiTestResult.h"
§§#include "TestOutcomeDescriptor.h"
§§#include "Util.h"
§§#include "TETestResultManager.h"
§§#include "GarbageCollectorWorker.h"
§§
§§PutTestResultRequestHandler::PutTestResultRequestHandler(std::string testcaseDir, CommInt* commPtr, TETestResultManager* teTestResultManager, GarbageCollectorWorker* garbageCollectorWorker) :
§§	m_testcaseDir(testcaseDir),
§§	m_comm(commPtr),
§§	m_teTestResultManager(teTestResultManager),
§§	m_garbageCollectorWorker(garbageCollectorWorker)
§§{
§§}
§§
§§PutTestResultRequestHandler::~PutTestResultRequestHandler()
§§{
§§}
§§
§§void PutTestResultRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp) {
§§	const PutTestResultRequest* receivedPutTestResultRequest = &req->puttestresultrequest();
§§
§§	FluffiTestcaseID testcaseId{ receivedPutTestResultRequest->id() };
§§	FluffiTestcaseID parentTestcaseId{ receivedPutTestResultRequest->parentid() };
§§	FluffiTestResult testResult{ receivedPutTestResultRequest->result() };
§§
§§	bool success = Util::storeTestcaseFileOnDisk(testcaseId, m_testcaseDir, &receivedPutTestResultRequest->testcasefirstchunk(), receivedPutTestResultRequest->islastchunk(), receivedPutTestResultRequest->sdofrunner().servicehostandport(), m_comm, workerThreadState, m_garbageCollectorWorker);
§§
§§	if (success) {
§§		TestOutcomeDescriptor* outcomeDesc = new TestOutcomeDescriptor(testcaseId, parentTestcaseId, testResult);
§§
§§		m_teTestResultManager->pushNewTestOutcomeFromTCRunner(outcomeDesc);
§§	}
§§	else {
§§		if (m_teTestResultManager->isThereAlreadyAToDFor(testcaseId)) {
§§			success = true;
§§			LOG(INFO) << "PutTestResultRequestHandler failed to store a testcase on disk but it seems that we already got that one in our queue. Strange but we can handle that!";
§§		}
§§		else {
§§			LOG(ERROR) << "PutTestResultRequestHandler failed to store a testcase on disk and isThereAlreadyAToDFor returned false!";
§§		}
§§	}
§§
§§	// Build Response
§§	PutTestResultResponse* PutTcResp = new PutTestResultResponse();
§§	PutTcResp->set_success(success);
§§
§§	resp->set_allocated_puttestresultresponse(PutTcResp);
§§}
