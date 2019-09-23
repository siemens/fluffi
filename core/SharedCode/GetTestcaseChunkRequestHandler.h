/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Michael Kraus, Abian Blome
*/

§§#pragma once
§§#include "IFLUFFIMessageHandler.h"

class GarbageCollectorWorker;
§§class GetTestcaseChunkRequestHandler :
§§	public IFLUFFIMessageHandler
§§{
§§public:
	GetTestcaseChunkRequestHandler(std::string testcaseDir, bool deleteFileOnLastChunk, GarbageCollectorWorker* garbageCollectorWorker);
§§	virtual ~GetTestcaseChunkRequestHandler();
§§
	void handleFLUFFIMessage(WorkerThreadState* workerThreadState, FLUFFIMessage* req, FLUFFIMessage* resp);
§§
§§private:
	std::string m_testcaseDir;
	bool m_deleteFileOnLastChunk;
	GarbageCollectorWorker* m_garbageCollectorWorker;
§§};