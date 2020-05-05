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
#include "GetLMConfigurationRequestHandler.h"

GetLMConfigurationRequestHandler::GetLMConfigurationRequestHandler() :
	m_myLMConfiguration("", "", "", "", "")
{
}

GetLMConfigurationRequestHandler::~GetLMConfigurationRequestHandler()
{
}

void GetLMConfigurationRequestHandler::setMyLMConfig(FluffiLMConfiguration theConf) {
	m_myLMConfiguration = theConf;
}

void GetLMConfigurationRequestHandler::handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp)
{
	(void)(workerThreadState); //avoid unused parameter warning
	(void)(req); //avoid unused parameter warning

	GetLMConfigurationResponse* getLMConfigurationResponse = new GetLMConfigurationResponse();

	LMConfiguration* ptrToLMConfiguration = new LMConfiguration();
	ptrToLMConfiguration->CopyFrom(m_myLMConfiguration.getProtobuf());
	getLMConfigurationResponse->set_allocated_lmconfiguration(ptrToLMConfiguration);

	resp->set_allocated_getlmconfigurationresponse(getLMConfigurationResponse);
}
