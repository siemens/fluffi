/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Michael Kraus, Abian Blome
*/

§§#include "stdafx.h"
§§#include "TestcaseDescriptor.h"
#include "Util.h"
#include "GarbageCollectorWorker.h"
§§
TestcaseDescriptor::TestcaseDescriptor(FluffiTestcaseID id, FluffiTestcaseID parentId, std::string pathAndFileName, bool forceFullCoverage)
	: m_id(id.m_serviceDescriptor, id.m_localID),
	m_parentId(parentId.m_serviceDescriptor, parentId.m_localID),
	m_pathAndFileName(pathAndFileName),
	m_numOfProcessingAttempts(0),
	m_forceFullCoverage(forceFullCoverage)
{}
§§
§§TestcaseDescriptor::~TestcaseDescriptor()
{}
§§
bool TestcaseDescriptor::getForceFullCoverage() const {
	return m_forceFullCoverage;
}

FluffiTestcaseID TestcaseDescriptor::getId() const
§§{
	return m_id;
§§}
§§
FluffiTestcaseID TestcaseDescriptor::getparentId() const
§§{
	return m_parentId;
§§}
§§
void TestcaseDescriptor::increaseNumOfProcessingAttempts() {
	m_numOfProcessingAttempts++;
}
§§
int TestcaseDescriptor::getNumOfProcessingAttempts() const {
	return m_numOfProcessingAttempts;
}

void TestcaseDescriptor::updateTimeOfLastSend() {
	m_timeOfLastSend = std::chrono::steady_clock::now();
}

void TestcaseDescriptor::deleteFile(GarbageCollectorWorker* garbageCollectorWorker)
{
	garbageCollectorWorker->markFileForDelete(m_pathAndFileName);
}

std::chrono::time_point<std::chrono::steady_clock> TestcaseDescriptor::getTimeOfLastSend() const {
	return m_timeOfLastSend;
}
