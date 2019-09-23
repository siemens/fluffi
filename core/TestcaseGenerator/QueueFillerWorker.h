/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once
#include "FluffiServiceDescriptor.h"

class CommInt;
class TGTestcaseManager;
class FluffiTestcaseID;
class TGWorkerThreadStateBuilder;
class TGWorkerThreadState;
class GarbageCollectorWorker;
class FluffiMutator;
class QueueFillerWorker
{
public:
	QueueFillerWorker(CommInt* commInt, TGWorkerThreadStateBuilder* workerThreadStateBuilder, int delayToWaitUntilConfigIsCompleteInMS, size_t desiredQueueFillLevel, std::string testcaseDirectory, std::string queueFillerTempDir, TGTestcaseManager*  testcaseManager, std::set<std::string> myAgentSubTypes, GarbageCollectorWorker* garbageCollectorWorker, int howManyWillBeGeneratedFromParent = 50);
	virtual ~QueueFillerWorker();

	void workerMain();
	void stop();

	std::thread* m_thread = nullptr;

private:
	FluffiTestcaseID getNewParent();
	bool tryGetConfigFromLM();

	bool m_gotConfigFromLM;
	CommInt* m_commInt;
	TGWorkerThreadStateBuilder* m_workerThreadStateBuilder;
	size_t m_desiredQueueFillLevel;
	std::string m_queueFillerTempDir;
	TGTestcaseManager* m_testcaseManager;
	int m_howManyWillBeGeneratedFromParent;
	FluffiServiceDescriptor m_mySelfServiceDescriptor;
	TGWorkerThreadState* m_workerThreadState;
	GarbageCollectorWorker* m_garbageCollectorWorker;
	std::string m_testcaseDirectory;
	int m_delayToWaitUntilConfigIsCompleteInMS;
	std::set<std::string> m_myAgentSubTypes;
	FluffiMutator* m_mutator;
	bool m_mutatorNeedsParents;
};
