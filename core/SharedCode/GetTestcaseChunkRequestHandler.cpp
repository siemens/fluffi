/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Michael Kraus, Abian Blome
*/

#include "stdafx.h"
#include "GetTestcaseChunkRequestHandler.h"
#include "FluffiTestcaseID.h"
#include "Util.h"
#include "GarbageCollectorWorker.h"

GetTestcaseChunkRequestHandler::GetTestcaseChunkRequestHandler(std::string testcaseDir, bool deleteFileOnLastChunk, GarbageCollectorWorker* garbageCollectorWorker) :
	m_testcaseDir(testcaseDir),
	m_deleteFileOnLastChunk(deleteFileOnLastChunk),
	m_garbageCollectorWorker(garbageCollectorWorker)

{
}

GetTestcaseChunkRequestHandler::~GetTestcaseChunkRequestHandler()
{
}

void GetTestcaseChunkRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp) {
	(void)(workerThreadState); //avoid unused parameter warning

	bool isLastChunk;

	FluffiTestcaseID testcaseID = FluffiTestcaseID(req->gettestcasechunkrequest().id());

	std::string data = Util::loadTestcaseChunkInMemory(testcaseID, m_testcaseDir, req->gettestcasechunkrequest().chunkid(), &isLastChunk);
	if (isLastChunk && m_deleteFileOnLastChunk) {
		//If the last chunk was just read: delete the file on all systems but the testcase generator (as the files are only temporary files)
		std::string testfile = Util::generateTestcasePathAndFilename(testcaseID, m_testcaseDir);
		m_garbageCollectorWorker->markFileForDelete(testfile);
	}

	// Build Response with loaded Testcase
	GetTestCaseChunkResponse* TcResp = new GetTestCaseChunkResponse();
	TcResp->set_chunkid(req->gettestcasechunkrequest().chunkid());
	TcResp->set_islastchunk(isLastChunk);
	TcResp->set_testcasechunk(data);
	resp->set_allocated_gettestcasechunkresponse(TcResp);
}
