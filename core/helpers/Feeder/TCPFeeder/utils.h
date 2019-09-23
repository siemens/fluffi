/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier, Pascal Eckmann
*/

§§#pragma once
§§#include "stdafx.h"
§§
§§#if !defined(_WIN32) && !defined(_WIN64)
§§#define SOCKETTYPE int
§§#define closesocket close
§§#define INVALID_SOCKET -1
§§#define SOCKET_ERROR -1
§§
§§std::string execCommandAndGetOutput(std::string command);
§§char** split_commandline(const std::string cmdline);
§§int memcpy_s(void* a, size_t b, const void* c, size_t d);
§§#endif
§§
§§bool isPortOpen(std::string target, uint16_t port);
§§void initializeIPC(SharedMemIPC& sharedMemIPC_ToRunner);
§§std::vector<std::string> splitString(std::string str, std::string token);
§§std::vector<char> readAllBytesFromFile(const std::string filename);
§§int getServerPortFromPID(int targetPID);
§§void initFromArgs(int argc, char* argv[], int& targetport, std::string& ipcName);
§§uint16_t getTargetPID(SharedMemIPC& sharedMemIPC_ToRunner, int feederTimeoutMS);
§§uint16_t getServerPortFromPIDOrTimeout(SharedMemIPC& sharedMemIPC_ToRunner, uint16_t targetPID, std::chrono::time_point<std::chrono::system_clock> timeLimit);
§§void waitForPortOpenOrTimeout(SharedMemIPC& sharedMemIPC_ToRunner, std::string targethost, uint16_t targetport, std::chrono::time_point<std::chrono::system_clock> timeOfLastConnectAttempt);
std::string getNewFuzzFileOrDie(SharedMemIPC& sharedMemIPC_ToRunner, int feederTimeoutMS);
