/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "LMWorkerThreadStateBuilder.h"
#include "Util.h"
#include "LMDatabaseManager.h"
#include "LMWorkerThreadState.h"
#include "GarbageCollectorWorker.h"

LMWorkerThreadStateBuilder::LMWorkerThreadStateBuilder(GarbageCollectorWorker* garbageCollectorWorker) :
	m_garbageCollectorWorker(garbageCollectorWorker)
{
}

LMWorkerThreadStateBuilder::~LMWorkerThreadStateBuilder()
{
}

WorkerThreadState* LMWorkerThreadStateBuilder::constructState() {
	LMWorkerThreadState* state = new LMWorkerThreadState();
	state->dbManager = new LMDatabaseManager(m_garbageCollectorWorker);

	return state;
}

void LMWorkerThreadStateBuilder::destructState(WorkerThreadState* theState) {
	LMWorkerThreadState* lmWorkerThreadState = dynamic_cast<LMWorkerThreadState*>(theState);
	if (lmWorkerThreadState == nullptr) {
		LOG(ERROR) << "LMWorkerThreadStateBuilder::destructState - theState cannot be accessed";
		return;
	}

	delete lmWorkerThreadState->dbManager;
	delete theState;
}
