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

Author(s): Thomas Riedmaier
*/

#include "stdafx.h"
#include "GarbageCollectorWorker.h"
#include "IWorkerThreadStateBuilder.h"

GarbageCollectorWorker::GarbageCollectorWorker(int intervallBetweenTwoCollectionRoundsInMillisec) :
	m_intervallBetweenTwoCollectionRoundsInMillisec(intervallBetweenTwoCollectionRoundsInMillisec),
	m_setOfFilesToDelete(std::set<std::string>()),
	m_waitedMS(0),
	m_stopRequested(false)
{
}

GarbageCollectorWorker::~GarbageCollectorWorker()
{
	if (m_thread != nullptr) {
		delete m_thread;
		m_thread = nullptr;
	}

	std::unique_lock<std::mutex> mlock(m_setOfFilesToDelete_mutex_); //get the lock for m_setOfFilesToDelete

	//Attempt to delete all remaining files (best effort)
	for (auto it = m_setOfFilesToDelete.begin(); it != m_setOfFilesToDelete.end(); ++it)
	{
		bool deleteSuccessful = false;
		for (int deleteTries = 0; deleteTries < 100; deleteTries++) {
			if (std::remove(it->c_str()) == 0) {
				deleteSuccessful = true;
				break;
			}
		}
		if (!deleteSuccessful) {
			LOG(ERROR) << "collectGarbage could not delete " << *it << " (it will remain on the system)";
		}
	}
}

void GarbageCollectorWorker::stop() {
	m_stopRequested = true;
}

void GarbageCollectorWorker::workerMain() {
	int checkAgainMS = 500;
	std::unique_lock<std::mutex> lk(m_wakep_mutex_);

	while (!m_stopRequested)
	{
		if (m_waitedMS < m_intervallBetweenTwoCollectionRoundsInMillisec) {
			m_wakeup_cv_.wait_for(lk, std::chrono::milliseconds(checkAgainMS));
			m_waitedMS += checkAgainMS;
			continue;
		}
		else {
			m_waitedMS = 0;
		}

		//Actual loop body
		collectGarbage();
	}
}

void GarbageCollectorWorker::collectGarbage() {
	LOG(DEBUG) << "Started with garbage collection";

	int iterations = static_cast<int>(m_setOfFilesToDelete.size()) * 10; // Try to delete each file about 10 times
	LOG(DEBUG) << "Will attempt to delete " << iterations << " files.";

	for (int i = 0; i < iterations; i++) {
		std::string fileToDelete = getRandomFileToDelete();
		if (fileToDelete == "") {
			break;
		}
		LOG(DEBUG) << "GarbageCollectorWorker::collectGarbage attempts to delete " << fileToDelete;

		std::error_code ec;
		if (!std::experimental::filesystem::exists(fileToDelete, ec)) {
			LOG(WARNING) << "GarbageCollectorWorker::collectGarbage, attempted to delete a file that no longer existed: " << fileToDelete;
			continue;
		}

		if (std::remove(fileToDelete.c_str()) != 0) {
			LOG(DEBUG) << "GarbageCollectorWorker::collectGarbage could not delete " << fileToDelete << "(file will be reinserted into delete list)";
			markFileForDelete(fileToDelete);
			continue;
		}

		LOG(DEBUG) << "GarbageCollectorWorker::succeeded to delete " << fileToDelete;
	}

	LOG(DEBUG) << "Done with garbage collection round";
}

std::string GarbageCollectorWorker::getRandomFileToDelete() {
	std::random_device rd; // obtain a random number from hardware
	std::mt19937 eng(rd()); // seed the generator

	std::unique_lock<std::mutex> mlock(m_setOfFilesToDelete_mutex_); //get the lock for m_setOfFilesToDelete

	if (m_setOfFilesToDelete.size() == 0) {
		return "";
	}

	std::set<std::string>::iterator  it(m_setOfFilesToDelete.begin());

	std::uniform_int_distribution<> distr(0, static_cast<int>(m_setOfFilesToDelete.size()) - 1); // define the range of the random number
	std::advance(it, distr(eng));

	// Check if Iterator is valid
	if (it != m_setOfFilesToDelete.end())
	{
		std::string re(*it);

		// Deletes the element pointing by iterator it
		m_setOfFilesToDelete.erase(it);

		return re;
	}
	else {
		LOG(ERROR) << "GarbageCollectorWorker::getRandomFileToDelete somehow iterated over the end of m_setOfFilesToDelete";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy;
	}
}

void GarbageCollectorWorker::markFileForDelete(std::string fileToDelete) {
	std::unique_lock<std::mutex> mlock(m_setOfFilesToDelete_mutex_);

	m_setOfFilesToDelete.insert(fileToDelete);
}

void GarbageCollectorWorker::collectNow() {
	m_waitedMS = m_intervallBetweenTwoCollectionRoundsInMillisec;
	m_wakeup_cv_.notify_all();
}

bool GarbageCollectorWorker::isFileMarkedForDeletion(const std::string candidate) {
	std::unique_lock<std::mutex> mlock(m_setOfFilesToDelete_mutex_);

	for (auto it = m_setOfFilesToDelete.begin(); it != m_setOfFilesToDelete.end(); ++it)
	{
		if (*it == candidate) {
			return true;
		}
	}

	return false;
}
