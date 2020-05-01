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

Author(s): Thomas Riedmaier, Michael Kraus
*/

#include "stdafx.h"
#include "TRGetStatusRequestHandler.h"
#include "TRWorkerThreadState.h"
#include "Util.h"
#include "CommInt.h"

TRGetStatusRequestHandler::TRGetStatusRequestHandler(CommInt* comm, int* numberOfProcessedTestcasesSinceLastStatusRequest) :
	GetStatusRequestHandler(comm),
	m_numberOfProcessedTestcasesSinceLastStatusRequest(numberOfProcessedTestcasesSinceLastStatusRequest)
{
}

TRGetStatusRequestHandler::~TRGetStatusRequestHandler()
{
}

std::string TRGetStatusRequestHandler::generateStatus() {
	//Generate the state (including the testcases since last reset)
	std::stringstream ss;
	ss << generateGeneralStatus() << " TestcasesSinceLastStatusRequest " << *m_numberOfProcessedTestcasesSinceLastStatusRequest << " |";

	// Reset counter of processed testcases
	*m_numberOfProcessedTestcasesSinceLastStatusRequest = 0;

	return ss.str().c_str();
}
