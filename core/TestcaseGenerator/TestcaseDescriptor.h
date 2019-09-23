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
§§#pragma once
§§#include "FluffiTestcaseID.h"
§§
§§class GarbageCollectorWorker;
§§class TestcaseDescriptor
§§{
§§public:
§§	TestcaseDescriptor(FluffiTestcaseID id, FluffiTestcaseID parentId, std::string pathAndFileName, bool forceFullCoverage);
§§	~TestcaseDescriptor();
§§
§§	FluffiTestcaseID getId() const;
§§	FluffiTestcaseID getparentId() const;
§§	int getNumOfProcessingAttempts() const;
§§	std::chrono::time_point<std::chrono::steady_clock> getTimeOfLastSend() const;
§§	void increaseNumOfProcessingAttempts();
§§	void updateTimeOfLastSend();
§§	void deleteFile(GarbageCollectorWorker* garbageCollectorWorker);
§§	bool getForceFullCoverage() const;
§§
§§private:
§§	FluffiTestcaseID m_id;
§§	FluffiTestcaseID m_parentId;
§§	std::string m_pathAndFileName;
§§	int m_numOfProcessingAttempts;
§§	std::chrono::time_point<std::chrono::steady_clock> m_timeOfLastSend = std::chrono::steady_clock::time_point();
§§	bool m_forceFullCoverage;
§§};
