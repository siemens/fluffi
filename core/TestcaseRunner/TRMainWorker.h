§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Abian Blome
§§*/
§§
§§#pragma once
§§
§§class CommInt;
§§class TRWorkerThreadStateBuilder;
§§class TRWorkerThreadState;
§§class CommPartnerManager;
§§class FluffiTestResult;
§§class FluffiTestExecutor;
§§class FluffiTestcaseID;
§§class GarbageCollectorWorker;
§§class TRMainWorker
§§{
§§public:
§§	TRMainWorker(CommInt* commInt, TRWorkerThreadStateBuilder*  workerThreadStateBuilder, int delayToWaitUntilConfigIsCompleteInMS, CommPartnerManager* tGManager, CommPartnerManager* tEManager, std::string testcaseDir, int* numberOfProcessedTestcases, std::set<std::string> myAgentSubTypes, GarbageCollectorWorker* garbageCollectorWorker);
§§	virtual ~TRMainWorker();
§§
§§	void workerMain();
§§	void stop();
§§
§§	std::thread* m_thread = nullptr;
§§
§§private:
§§	FluffiTestResult fuzzTestCase(const FluffiTestcaseID testcaseId, bool forceFullCoverage);
§§	bool getTestcaseFromGenerator(FLUFFIMessage* resp, std::string generatorHAP);
§§	bool pushTestcaseWithResultsToEvaluator(std::string evaluatorTargetHAP, const FluffiTestResult testResult, const FluffiTestcaseID testcaseId, const FluffiTestcaseID parentId);
§§	bool tryGetConfigFromLM();
§§
§§	bool m_gotConfigFromLM;
§§	CommInt* m_commInt;
§§	TRWorkerThreadStateBuilder* m_workerThreadStateBuilder;
§§	int m_delayToWaitUntilConfigIsCompleteInMS;
§§	CommPartnerManager* m_tGManager;
§§	CommPartnerManager* m_tEManager;
§§	std::string m_testcaseDir;
§§	int* m_numberOfProcessedTestcases;
§§	std::set<std::string> m_myAgentSubTypes;
§§	GarbageCollectorWorker* m_garbageCollectorWorker;
§§
§§	TRWorkerThreadState* m_workerThreadState;
§§	FluffiTestExecutor* m_executor;
§§};
