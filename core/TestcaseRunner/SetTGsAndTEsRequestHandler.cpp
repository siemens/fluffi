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
§§#include "SetTGsAndTEsRequestHandler.h"
§§#include "CommPartnerManager.h"
§§
§§SetTGsAndTEsRequestHandler::SetTGsAndTEsRequestHandler(CommPartnerManager* tGManager, CommPartnerManager* tEManager)
§§{
§§	this->m_tGManager = tGManager;
§§	this->m_tEManager = tEManager;
§§}
§§
§§SetTGsAndTEsRequestHandler::~SetTGsAndTEsRequestHandler()
§§{
§§}
§§
§§void SetTGsAndTEsRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp) {
§§	(void)(workerThreadState); //avoid unused parameter warning
§§
§§	const SetTGsAndTEsRequest* setTGsAndTEsReqest = &req->settgsandtesrequest();
§§	google::protobuf::RepeatedPtrField<ServiceAndWeigth> tgs = setTGsAndTEsReqest->tgs();
§§	google::protobuf::RepeatedPtrField<ServiceAndWeigth> tes = setTGsAndTEsReqest->tes();
§§
§§	// Update datastructure of registered TGs and TEs for TestcaseRunner
§§	int newTGs = m_tGManager->updateCommPartners(&tgs);
§§	LOG(DEBUG) << "Registered " << newTGs << " new TestcaseGenerators!";
§§
§§	int newTEs = m_tEManager->updateCommPartners(&tes);
§§	LOG(DEBUG) << "Registered " << newTEs << " new TestcaseEvaluators!";
§§
§§	SetTGsAndTEsResponse* setTGsAndTEsResponse = new SetTGsAndTEsResponse();
§§	resp->set_allocated_settgsandtesresponse(setTGsAndTEsResponse);
§§}
