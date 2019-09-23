§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Pascal Eckmann
§§*/
§§
§§#include "stdafx.h"
§§#include "SharedMemIPC.h"
§§
§§//cp ../../../bin/libsharedmemipc.so .
§§//g++ --std=c++11 -I../../../SharedMemIPC/ -o UDPFeeder stdafx.cpp UDPFeeder.cpp libsharedmemipc.so -Wl,-rpath='${ORIGIN}'
§§
§§#define RESP_TIMEOUT_MS 60000
§§#define KNOWN_GOOD (char)0x98, (char)0x79, (char)0x01, (char)0x00, (char)0x00, (char)0x01, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x04, (char)0x74, (char)0x65, (char)0x73, (char)0x74, (char)0x04, (char)0x74, (char)0x65, (char)0x73, (char)0x74, (char)0x00, (char)0x00, (char)0x01, (char)0x00, (char)0x01
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§#define SOCKETTYPE SOCKET
§§#else
§§#define SOCKETTYPE int
§§#define closesocket close
§§#endif
§§
§§void preprocess(std::vector<char>* bytes) {
§§	//add preprocession steps here as needed
§§	return;
§§}
§§
§§bool sendBytesToPort(std::vector<char>* fuzzBytes, int serverport, bool waitForResponse) {
§§	struct sockaddr_in si_other;
§§	SOCKETTYPE s;
§§	int slen = sizeof(si_other);
§§
§§	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
§§		std::cout << "socket failed" << std::endl;
§§		return false;
§§	}
§§
§§	memset((char*)&si_other, 0, sizeof(si_other));
§§	si_other.sin_family = AF_INET;
§§	si_other.sin_port = htons(serverport);
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§	if (inet_pton(AF_INET, "127.0.0.1", &si_other.sin_addr) == 0) {
§§#else
§§	if (inet_aton("127.0.0.1", &si_other.sin_addr) == 0) {
§§#endif
§§		std::cout << "Error using inet_aton" << std::endl;
§§		closesocket(s);
§§		return false;
§§	}
§§
§§	size_t fuzzBytesSize = (size_t)fuzzBytes->size();
§§	char* fuzzByteArray = &(*fuzzBytes)[0];
§§
§§	fuzzBytesSize = fuzzBytesSize > 65506 ? 65506 : fuzzBytesSize;
§§
§§	if (sendto(s, fuzzByteArray, (int)fuzzBytesSize, 0, (sockaddr*)&si_other, slen) == -1) {
§§		std::cout << "Error using sendto" << std::endl;
§§		closesocket(s);
§§		return false;
§§	}
§§
§§	if (waitForResponse) {
§§#if defined(_WIN32) || defined(_WIN64)
§§		DWORD timeout_MS = RESP_TIMEOUT_MS;
§§		if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_MS, sizeof(timeout_MS)) < 0) {
§§#else
§§		struct timeval tv;
§§		tv.tv_sec = RESP_TIMEOUT_MS / 1000;
§§		tv.tv_usec = ((RESP_TIMEOUT_MS) % 1000) * 1000;
§§		if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
§§#endif
§§			std::cout << "setsockopt failed" << std::endl;
§§			closesocket(s);
§§			return false;
§§		}
§§
§§		char buf[1];
§§		int blen = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*) &si_other, (socklen_t*)&slen);
§§		if (blen == -1) {
§§#if defined(_WIN32) || defined(_WIN64)
§§			if (WSAGetLastError() != WSAEMSGSIZE) {
§§#endif
§§				std::cout << "recvfrom failed" << std::endl;
§§				closesocket(s);
§§				return false;
§§#if defined(_WIN32) || defined(_WIN64)
§§			}
§§#endif
§§		}
§§	}
§§
§§	closesocket(s);
§§	return true;
§§}
§§
§§bool isServerAlive(int targetPort) {
§§	std::vector<char> knownGood{ KNOWN_GOOD };
§§	return sendBytesToPort(&knownGood, targetPort, true);
§§}
§§
§§std::vector<char> readAllBytesFromFile(const std::string filename)
§§{
§§	FILE* inputFile;
§§	if (0 != fopen_s(&inputFile, filename.c_str(), "rb")) {
§§		std::cout << "UDPFeeder::readAllBytesFromFile failed to open the file " << filename;
§§		return{};
§§	}
§§
§§	fseek(inputFile, 0, SEEK_END);
§§	long fileSize = ftell(inputFile);
§§	fseek(inputFile, 0, SEEK_SET);
§§
§§	if ((unsigned int)fileSize == 0) {
§§		fclose(inputFile);
§§		return{};
§§	}
§§
§§	std::vector<char> result((unsigned int)fileSize);
§§
§§	size_t bytesRead = static_cast<size_t>(fread((char*)&result[0], 1, fileSize, inputFile));
§§	if (bytesRead != static_cast<size_t>(fileSize)) {
§§		std::cout << "readAllBytesFromFile failed to read all bytes! Bytes read: " << bytesRead << ". Filesize " << fileSize;
§§	}
§§
§§	fclose(inputFile);
§§
§§	return result;
§§}
§§
§§int main(int argc, char* argv[])
§§{
§§	if (argc < 2) {
§§		std::cout << "Usage: UDPFeeder.exe [port] <shared memory name>";
§§		return -1;
§§	}
§§
§§	int targetport = 0;
§§
§§	if (argc == 3) {
§§		targetport = std::stoi(argv[1]);
§§	}
§§
§§	SharedMemIPC sharedMemIPC_ToRunner(argv[argc - 1]);
§§	bool success = sharedMemIPC_ToRunner.initializeAsClient();
§§	std::cout << "UDPFeeder: initializeAsClient for runner" << success << std::endl;
§§	if (!success) {
§§		return -1;
§§	}
§§
§§	int feederTimeoutMS = 10 * 60 * 1000;
§§
§§	std::string targetPIDAsString;
§§	SharedMemMessage message__FromRunner;
§§	sharedMemIPC_ToRunner.waitForNewMessageToClient(&message__FromRunner, feederTimeoutMS);
§§	if (message__FromRunner.getMessageType() == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
§§		std::cout << "UDPFeeder: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message, terminating!" << std::endl;
§§		return -1;
§§	}
§§	else if (message__FromRunner.getMessageType() == SHARED_MEM_MESSAGE_TARGET_PID) {
§§		std::cout << "UDPFeeder: Recived a SHARED_MEM_MESSAGE_TARGET_PID message!" << std::endl;
§§		std::string targetPIDAsString = std::string(message__FromRunner.getDataPointer(), message__FromRunner.getDataPointer() + message__FromRunner.getDataSize());
§§		std::cout << "UDPFeeder: target pid is \"" << targetPIDAsString << "\"" << std::endl;
§§	}
§§	else {
§§		std::cout << "UDPFeeder: Recived an unexpeced message, terminating!" << std::endl;
§§		return -1;
§§	}
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§	// Initialize Winsock
§§	WSADATA wsaData;
§§	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
§§	if (iResult != 0) {
§§		std::cout << "TCPFeeder: WSAStartup failed, terminating!" << std::endl;
§§		return -1;
§§	}
§§#endif
§§
§§	SharedMemMessage readyToFuzzMessage = SharedMemMessage(SHARED_MEM_MESSAGE_READY_TO_FUZZ, nullptr, 0);
§§	sharedMemIPC_ToRunner.sendMessageToServer(&readyToFuzzMessage);
§§
§§	SharedMemMessage message__FromTarget;
§§	SharedMemMessage fuzzDoneMessage(SHARED_MEM_MESSAGE_FUZZ_DONE, nullptr, 0);
§§	while (true) {
§§		sharedMemIPC_ToRunner.waitForNewMessageToClient(&message__FromRunner, feederTimeoutMS);
§§		SharedMemMessageType messageFromRunnerType = message__FromRunner.getMessageType();
§§		if (messageFromRunnerType == SHARED_MEM_MESSAGE_FUZZ_FILENAME) {
§§			std::cout << "UDPFeeder: Recived a SHARED_MEM_MESSAGE_FUZZ_FILENAME message :)" << std::endl;
§§			std::string fuzzFileName = std::string(message__FromRunner.getDataPointer(), message__FromRunner.getDataPointer() + message__FromRunner.getDataSize());
§§			std::vector<char> fuzzBytes = readAllBytesFromFile(fuzzFileName);
§§
§§			preprocess(&fuzzBytes);
§§
§§			if (fuzzBytes.size() == 0) {
§§				std::cout << "UDPFeeder: sending SHARED_MEM_MESSAGE_FUZZ_DONE to the runner as empty payloads are not forwarded" << std::endl;
§§				sharedMemIPC_ToRunner.sendMessageToServer(&fuzzDoneMessage);
§§			}
§§			else if (sendBytesToPort(&fuzzBytes, targetport, false) & isServerAlive(targetport)) {
§§				std::cout << "UDPFeeder: sending SHARED_MEM_MESSAGE_FUZZ_DONE to the runner " << std::endl;
§§				sharedMemIPC_ToRunner.sendMessageToServer(&fuzzDoneMessage);
§§			}
§§			else {
§§				std::string errorDesc = "Failed sending the fuzz file bytes to the target port";
§§				std::cout << errorDesc << std::endl;
§§				SharedMemMessage messageToFeeder(SHARED_MEM_MESSAGE_FUZZ_ERROR, errorDesc.c_str(), (int)errorDesc.length());
§§				sharedMemIPC_ToRunner.sendMessageToServer(&messageToFeeder);
§§			}
§§		}
§§		else if (messageFromRunnerType == SHARED_MEM_MESSAGE_TERMINATE) {
§§			std::cout << "UDPFeeder: Recived a SHARED_MEM_MESSAGE_TERMINATE, terminating!" << std::endl;
§§#if defined(_WIN32) || defined(_WIN64)
§§			WSACleanup();
§§#endif
§§			return 0;
§§		}
§§		else if (messageFromRunnerType == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
§§			std::cout << "UDPFeeder: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message, terminating!" << std::endl;
§§#if defined(_WIN32) || defined(_WIN64)
§§			WSACleanup();
§§#endif
§§			return -1;
§§		}
§§		else {
§§			std::cout << "UDPFeeder: Recived an unexpeced message, terminating!" << std::endl;
§§#if defined(_WIN32) || defined(_WIN64)
§§			WSACleanup();
§§#endif
§§			return -1;
§§		}
§§	}
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§	WSACleanup();
§§#endif
§§	return 0;
§§}
