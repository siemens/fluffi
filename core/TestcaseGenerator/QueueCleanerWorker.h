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

#pragma once

class TGWorkerThreadStateBuilder;
class TGTestcaseManager;
class TGWorkerThreadState;
class CommInt;
class GarbageCollectorWorker;
class QueueCleanerWorker
{
public:
	QueueCleanerWorker(CommInt* commInt, TGWorkerThreadStateBuilder* workerThreadStateBuilder, int intervallBetweenTwoCleaningRoundsInMillisec, int maxRetriesBeforeReport, std::string testcaseDir, TGTestcaseManager* testcaseManager, GarbageCollectorWorker* garbageCollectorWorker);
	virtual ~QueueCleanerWorker();

	void workerMain();
	void stop();

	std::thread* m_thread = nullptr;

private:
	void reinsertTestcasesWithNoResponse();
	void removeCompletedTestcases();
	void reportTestcasesWithMultipleNoResponse();

	CommInt* m_commInt;
	TGWorkerThreadStateBuilder* m_workerThreadStateBuilder;
	int m_intervallBetweenTwoCleaningRoundsInMillisec;
	int m_intervallToWaitForReinsertionInMillisec;
	int m_maxRetriesBeforeReport;
	std::string m_testcaseDir;
	TGTestcaseManager* m_testcaseManager;
	uint64_t m_serverTimestampOfNewestEvaluationResult;
	GarbageCollectorWorker* m_garbageCollectorWorker;

	TGWorkerThreadState* m_workerThreadState = nullptr;
};
