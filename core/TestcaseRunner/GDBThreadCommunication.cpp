§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Roman Bendt
§§*/
§§
§§#include "stdafx.h"
§§#include "GDBThreadCommunication.h"
§§
§§GDBThreadCommunication::GDBThreadCommunication() :
§§#if defined(_WIN32) || defined(_WIN64)
§§#else
§§	m_gdbPid(-1),
§§#endif
§§	m_exOutput(),
§§	m_ostreamToGdb(nullptr),
§§	m_gdbThreadShouldTerminate(false),
§§	m_debuggingReady(false),
§§	m_coverageState(DUMPED),
§§	m_gdbOutputQueue(),
§§	m_exitStatus(-1)
§§{
§§	m_exOutput.m_hasFullCoverage = false;
§§}
§§
§§GDBThreadCommunication::~GDBThreadCommunication()
§§{
§§}
§§
§§bool GDBThreadCommunication::get_gdbThreadShouldTerminate() {
§§	return m_gdbThreadShouldTerminate;
§§}
§§void GDBThreadCommunication::set_gdbThreadShouldTerminate() {
§§	std::unique_lock<std::mutex> lk(m_gdb_mutex);
§§	m_gdbThreadShouldTerminate = true;
§§	m_gdb_mutex_cv.notify_all();
§§}
§§bool GDBThreadCommunication::get_debuggingReady() {
§§	return m_debuggingReady;
§§}
§§void GDBThreadCommunication::set_debuggingReady() {
§§	std::unique_lock<std::mutex> lk(m_readyDebug_mutex);
§§	m_debuggingReady = true;
§§	m_readyDebug_cv.notify_all();
§§}
§§GDBThreadCommunication::COVERAGE_STATE GDBThreadCommunication::get_coverageState() {
§§	return m_coverageState;
§§}
§§void GDBThreadCommunication::set_coverageState(GDBThreadCommunication::COVERAGE_STATE val) {
§§	std::unique_lock<std::mutex> lk(m_gdb_mutex);
§§	m_coverageState = val;
§§	m_gdb_mutex_cv.notify_all();
§§}
§§
§§void GDBThreadCommunication::waitForTerminateMessageOrCovStateChange(GDBThreadCommunication::COVERAGE_STATE statesToWaitFor[], int numOfStatesToWaitFor) {
§§	std::unique_lock<std::mutex> lk(m_gdb_mutex);
§§	//Dont wait, if the state we want to wait for is already reached
§§	if (m_gdbThreadShouldTerminate || m_gdbOutputQueue.size() > 0) {
§§		return;
§§	}
§§	for (int i = 0; i < numOfStatesToWaitFor; i++) {
§§		if (statesToWaitFor[i] == m_coverageState) {
§§			return;
§§		}
§§	}
§§
§§	m_gdb_mutex_cv.wait(lk);
§§}
§§
§§void GDBThreadCommunication::waitForDebuggingReady() {
§§	std::unique_lock<std::mutex> lk(m_readyDebug_mutex);
§§	//Dont wait, if the state we want to wait for is already reached
§§	if (m_debuggingReady) {
§§		return;
§§	}
§§
§§	m_readyDebug_cv.wait(lk);
§§}
§§
§§size_t GDBThreadCommunication::get_gdbOutputQueue_size() {
§§	return m_gdbOutputQueue.size();
§§}
§§
§§void GDBThreadCommunication::gdbOutputQueue_push_back(const std::string val) {
§§	std::unique_lock<std::mutex> lk(m_gdb_mutex);
§§	m_gdbOutputQueue.push_back(val);
§§	m_gdb_mutex_cv.notify_all();
§§}
§§
§§std::string GDBThreadCommunication::gdbOutputQueue_pop_front() {
§§	std::unique_lock<std::mutex> lk(m_gdb_mutex);
§§	std::string line = m_gdbOutputQueue.front();
§§	m_gdbOutputQueue.pop_front();
§§	return line;
§§}
§§
§§int GDBThreadCommunication::get_exitStatus() {
§§	return m_exitStatus;
§§}
§§
§§void GDBThreadCommunication::set_exitStatus(int exitStatus) {
§§	m_exitStatus = exitStatus;
§§}
