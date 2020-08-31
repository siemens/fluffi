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

Author(s): Thomas Riedmaier, Michael Kraus, Abian Blome
*/

#include "stdafx.h"
#include "GetTestcaseRequestHandler.h"
#include "TestcaseDescriptor.h"
#include "TGTestcaseManager.h"
#include "Util.h"

GetTestcaseRequestHandler::GetTestcaseRequestHandler(TGTestcaseManager*  testcaseManager, std::string testcaseDir, CommInt* commInt)
{
	this->m_testcaseManager = testcaseManager;
	this->m_testcaseDir = testcaseDir;
	this->m_CommInt = commInt;
}

GetTestcaseRequestHandler::~GetTestcaseRequestHandler()
{
}

void GetTestcaseRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp) {
	(void)(workerThreadState); //avoid unused parameter warning
	(void)(req); //avoid unused parameter warning

	GetTestcaseResponse* TcResp = new GetTestcaseResponse();

	// Get next TestcaseDescriptor from TestcaseQueue
	try {
		TestcaseDescriptor testcase = m_testcaseManager->popPendingTCForProcessing();
		TestcaseID* mutableId = new TestcaseID();
		mutableId->CopyFrom(testcase.getId().getProtobuf());

		TestcaseID* mutableIdParent = new TestcaseID();
		mutableIdParent->CopyFrom(testcase.getparentId().getProtobuf());

		bool isLastChunk;
		std::string data = Util::loadTestcaseChunkInMemory(testcase.getId(), m_testcaseDir, 0, &isLastChunk);

		// Build Response with loaded Testcase
		TcResp->set_allocated_id(mutableId);
		TcResp->set_allocated_parentid(mutableIdParent);
		TcResp->set_islastchunk(isLastChunk);
		TcResp->set_testcasefirstchunk(data);
		TcResp->set_forcefullcoverage(testcase.getForceFullCoverage());
		TcResp->set_success(true);
	}
	catch (std::runtime_error & e)
	{
		LOG(WARNING) << "Could not get a testcase for processing: " << e.what();
		TcResp->set_success(false);
	}

	resp->set_allocated_gettestcaseresponse(TcResp);
}
