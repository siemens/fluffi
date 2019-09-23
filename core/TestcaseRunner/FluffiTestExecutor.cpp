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
§§#include "stdafx.h"
§§#include "FluffiTestExecutor.h"
§§#include "GarbageCollectorWorker.h"
§§#include "SharedMemIPC.h"
§§
§§FluffiTestExecutor::FluffiTestExecutor(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const std::string additionalEnvParam, GarbageCollectorWorker* garbageCollectorWorker)
§§	: m_targetCMDline(targetCMDline),
§§	m_hangTimeoutMS(hangTimeoutMS),
§§	m_modulesToCover(modulesToCover),
§§	m_testcaseDir(testcaseDir),
§§	m_child_output_mode(child_output_mode),
§§	m_additionalEnvParam(additionalEnvParam),
§§	m_garbageCollectorWorker(garbageCollectorWorker)
§§
§§{
§§	//Make sure we are working on absolute paths. This prevents bugs, e.g., when the target application changes the working directory
§§#if defined(_WIN32) || defined(_WIN64)
§§	char full[MAX_PATH];
§§	_fullpath(full, m_testcaseDir.c_str(), MAX_PATH);
§§#else
§§	char full[PATH_MAX];
§§	char* res = realpath(m_testcaseDir.c_str(), full);
§§	if (res == NULL) {
§§		LOG(ERROR) << "realpath failed: " << errno;
§§	}
§§#endif
§§	m_testcaseDir = std::string(full);
§§}
§§
§§FluffiTestExecutor::~FluffiTestExecutor()
§§{
§§}
§§
§§bool FluffiTestExecutor::initializeFeeder(std::shared_ptr<ExternalProcess> feederProcess, SharedMemIPC* sharedMemIPC_toFeeder, process_id_t targetProcessID, int timeoutMS) {
§§	std::chrono::time_point<std::chrono::steady_clock> routineEntryTimeStamp = std::chrono::steady_clock::now();
§§	std::chrono::time_point<std::chrono::steady_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(timeoutMS);
§§
§§	if (!feederProcess->initProcess()) {
§§		LOG(ERROR) << "Could not initialize the feeder process";
§§		return false;
§§	}
§§
§§	if (!feederProcess->run()) {
§§		LOG(ERROR) << "Could not run the feeder process";
§§		return false;
§§	}
§§
§§	std::string targetPIDString = std::to_string(targetProcessID);
§§	SharedMemMessage targetCMDMessage{ SHARED_MEM_MESSAGE_TARGET_PID, targetPIDString.c_str(), static_cast<int>(targetPIDString.length()) };
§§
§§	if (!sharedMemIPC_toFeeder->sendMessageToClient(&targetCMDMessage)) {
§§		LOG(ERROR) << "Could not send the targetCMDMessage to the feeder process";
§§		return false;
§§	}
§§
§§	SharedMemMessage responseFromFeeder;
§§	sharedMemIPC_toFeeder->waitForNewMessageToServer(&responseFromFeeder, static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::steady_clock::now()).count()));
§§	if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
§§		LOG(ERROR) << "Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message: starting Feeder failed!";
§§		return false;
§§	}
§§	else if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_READY_TO_FUZZ) {
§§		LOG(DEBUG) << "Recived a SHARED_MEM_MESSAGE_READY_TO_FUZZ message: feeder is ready to fuzz!";
§§
§§		return true;
§§	}
§§	else if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE) {
§§		LOG(DEBUG) << "Recived a SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE message: feeder could not initialize. Error message :" << std::string(responseFromFeeder.getDataPointer(), responseFromFeeder.getDataPointer() + responseFromFeeder.getDataSize());
§§
§§		return false;
§§	}
§§	else {
§§		LOG(ERROR) << "Recived an unexpeced message: starting Feeder failed!";
§§		return false;
§§	}
§§}
