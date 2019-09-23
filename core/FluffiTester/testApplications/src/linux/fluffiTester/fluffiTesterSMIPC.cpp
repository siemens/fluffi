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
§§#include <vector>
§§#include <string.h>
§§#include <string>
§§#include <fstream>
§§#include <iostream>
§§#include "SharedMemIPC.h"
§§
§§//g++ --std=c++11 -o fluffiTesterSMIPC -I../../../../../SharedMemIPC -L../../../../../bin fluffiTesterSMIPC.cpp -lsharedmemipc -Wl,-rpath,.
§§
§§std::vector<char> readAllBytesFromFile(std::string filename)
§§{
§§	std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
§§	if (!ifs.is_open()) {
§§		return{};
§§	}
§§
§§	std::ifstream::pos_type fileSize = ifs.tellg();
§§
§§	if ((unsigned int)fileSize == 0) {
§§		return{};
§§	}
§§
§§	std::vector<char> result((unsigned int)fileSize);
§§
§§	ifs.seekg(0, std::ios::beg);
§§	ifs.read((char*)&result[0], fileSize);
§§
§§	return result;
§§}
§§
§§void parseInput(char* recvbuf) {
§§
§§	char bufferThatIsTooSmall[0x42];
§§
§§	//cause access violation
§§	if (recvbuf[0]==0x41) {
§§		strcpy(bufferThatIsTooSmall, recvbuf);
§§
§§		for (int i = 0; i < sizeof(bufferThatIsTooSmall); i++) {
§§			bufferThatIsTooSmall[i]++;
§§		}
§§
§§		strcpy( recvbuf, bufferThatIsTooSmall);
§§	}
§§
§§	//cause hang
§§/*	if (recvbuf[0]==0x61) {
§§		int sum = 0;
§§		for(int i=0;i<0x7FFFFFFF;i++){
§§		sum += i;
§§		}
§§		printf("%d\n",sum);
§§	}*/
§§
§§
§§
§§}
§§
§§int main(int argc, char* argv[])
§§{
§§	int communicationTimeout = 100 * 1000;
§§	if(argc<2){
§§		printf("Usage: fluffiTesterSMIPC <shared memory name>");
§§		return -1;
§§	}
§§
§§	SharedMemIPC sharedMemIPC(argv[1]);
§§	bool success = sharedMemIPC.initializeAsServer();
§§	printf("fluffiTesterSMIPC: initializeAsServer %s(%s)\n", (success?"true":"false"), argv[1] );
§§	if (!success) {
§§		return -1;
§§	}
§§
§§	printf("fluffiTesterSMIPC: sending SHARED_MEM_MESSAGE_READY_TO_FUZZ to the feeder\n");
§§	SharedMemMessage messageToFeeder(SHARED_MEM_MESSAGE_READY_TO_FUZZ, nullptr, 0);
§§	sharedMemIPC.sendMessageToClient(&messageToFeeder);
§§
§§	SharedMemMessage fuzzJobDoneMessage(SHARED_MEM_MESSAGE_FUZZ_DONE, nullptr, 0);
§§	SharedMemMessage message;
§§	while (true) {
§§		sharedMemIPC.waitForNewMessageToServer(&message, communicationTimeout);
§§		SharedMemMessageType messageType = message.getMessageType();
§§		if (messageType == SHARED_MEM_MESSAGE_FUZZ_FILENAME) {
§§			int fileNameSize = message.getDataSize() + 1;
§§			char* targetFile = new char[fileNameSize];
§§			memset(targetFile, 0, fileNameSize);
§§			memcpy(targetFile, message.getDataPointer(), fileNameSize - 1);
§§
§§			std::vector<char> fileContent = readAllBytesFromFile(targetFile);
§§			parseInput(&fileContent[0]);
§§			delete[] targetFile;
§§		}
§§		else if (messageType == SHARED_MEM_MESSAGE_TERMINATE) {
§§			std::cout << "fluffiTesterSMIPC: Recived a SHARED_MEM_MESSAGE_TERMINATE, terminating!" << std::endl;
§§			return 0;
§§		}
§§		else if (messageType == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
§§			std::cout << "fluffiTesterSMIPC: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message, terminating!" << std::endl;
§§			return -1;
§§		}
§§		else {
§§			std::cout << "fluffiTesterSMIPC: Recived an unexpeced message("<<messageType<<"), terminating!" << std::endl;
§§			return -1;
§§		}
§§
§§		std::cout << "fluffiTesterSMIPC: sending SHARED_MEM_MESSAGE_FUZZ_DONE to the feeder " << std::endl;
§§
§§		sharedMemIPC.sendMessageToClient(&fuzzJobDoneMessage);
§§	}
§§
§§
§§	return 0;
§§}
