§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier
§§*/
§§
§§#pragma once
§§#include <condition_variable>
§§
§§class GarbageCollectorWorker
§§{
§§public:
§§	GarbageCollectorWorker(int intervallBetweenTwoCollectionRoundsInMillisec);
§§	virtual ~GarbageCollectorWorker();
§§
§§	void workerMain();
§§
§§	void markFileForDelete(std::string fileToDelete);
§§
§§	void collectGarbage(); //The calling thread will collect garbage itself
§§	void collectNow(); // The GarbageCollectorWorker will start collecting garbage
§§
§§	bool isFileMarkedForDeletion(const std::string candidate);
§§
§§	void stop();
§§
§§	std::thread* m_thread = nullptr;
§§
§§#ifndef _VSTEST
§§private:
§§#endif //_VSTEST
§§
§§	int m_intervallBetweenTwoCollectionRoundsInMillisec;
§§	std::mutex m_setOfFilesToDelete_mutex_;
§§	std::mutex m_wakep_mutex_;
§§	std::condition_variable m_wakeup_cv_;
§§	std::set<std::string> m_setOfFilesToDelete;
§§	int m_waitedMS;
§§	bool m_stopRequested;
§§
§§	std::string getRandomFileToDelete();
§§};
