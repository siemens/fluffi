/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

#include <vector>
#include <string>
#include <iostream>
#include <stdio.h>
#include <string.h>

#include "SharedMemIPC.h"

void testOneInput(char * data, std::vector::size_type size) {
	//TODO
}

std::vector<char> readAllBytesFromFile(const std::string filename)
{
	FILE* inputFile;
	if (0 != fopen_s(&inputFile, filename.c_str(), "rb")) {
		std::cout << "readAllBytesFromFile failed to open the file " << filename << std::endl;
		return{};
	}

	fseek(inputFile, 0, SEEK_END);
	long fileSize = ftell(inputFile);
	fseek(inputFile, 0, SEEK_SET);

	if (fileSize == 0) {
		fclose(inputFile);
		return{};
	}

	std::vector<char> result(fileSize);

	size_t  bytesRead = fread(&result[0], 1, fileSize, inputFile);
	if (bytesRead != static_cast<size_t>(fileSize)) {
		std::cout << "readAllBytesFromFile failed to read all bytes! Bytes read: " << bytesRead << ". Filesize " << fileSize << std::endl;
	}

	fclose(inputFile);

	return result;
}

#define FLUFFI_MULTIRUNNER

#if defined(FLUFFI_MULTIRUNNER)

int main(int argc, char* argv[]) {
	int communicationTimeout = 100 * 1000;
	if (argc < 2) {
		std::cout << "Usage: SharedMemStub.exe <shared memory name>";
		return -1;
	}

	SharedMemIPC sharedMemIPC(argv[1]);
	bool success = sharedMemIPC.initializeAsServer();
	std::cout << "SharedMemStub: initializeAsServer " << success << std::endl;
	if (!success) {
		return -1;
	}

	std::cout << "SharedMemStub: sending SHARED_MEM_MESSAGE_READY_TO_FUZZ to the feeder " << std::endl;
	SharedMemMessage messageToFeeder(SHARED_MEM_MESSAGE_READY_TO_FUZZ, nullptr, 0);
	sharedMemIPC.sendMessageToClient(&messageToFeeder);

	SharedMemMessage fuzzJobDoneMessage(SHARED_MEM_MESSAGE_FUZZ_DONE, nullptr, 0);
	SharedMemMessage message;
	while (true) {
		sharedMemIPC.waitForNewMessageToServer(&message, communicationTimeout);
		SharedMemMessageType messageType = message.getMessageType();
		if (messageType == SHARED_MEM_MESSAGE_FUZZ_FILENAME) {
			int fileNameSize = message.getDataSize() + 1;
			char*  targetFile = new char[fileNameSize];
			memset(targetFile, 0, fileNameSize);
			memcpy(targetFile, message.getDataPointer(), fileNameSize - 1);

			std::vector<char> input = readAllBytesFromFile(targetFile);

			testOneInput(input.data(), input.size());
			delete[] targetFile;
		}
		else if (messageType == SHARED_MEM_MESSAGE_TERMINATE) {
			//std::cout << "SharedMemStub: Recived a SHARED_MEM_MESSAGE_TERMINATE, terminating!" << std::endl;
			return 0;
		}
		else if (messageType == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
			//std::cout << "SharedMemStub: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message, terminating!" << std::endl;
			return -1;
		}
		else {
			//std::cout << "SharedMemStub: Recived an unexpeced message, terminating!" << std::endl;
			return -1;
		}

		//std::cout << "SharedMemStub: sending SHARED_MEM_MESSAGE_FUZZ_DONE to the feeder " << std::endl;

		sharedMemIPC.sendMessageToClient(&fuzzJobDoneMessage);
	}
}

#else

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << "Usage: SharedMemStub.exe <fuzz file name>";
		return -1;
	}

	std::vector<char> input = readAllBytesFromFile(argv[1]);

	testOneInput(input.data(), input.size());

	return 0;
}

#endif