/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

// g++ --std=c++11 InstrumentedDebuggerTester.cpp -o InstrumentedDebuggerTester -I ../../../../SharedMemIPC -L ../../../../bin -lsharedmemipc


#include "stdafx.h"
#include "SharedMemIPC.h"

std::vector<char> readAllBytesFromFile(const std::string filename)
{
	std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
	if (!ifs.is_open()) {
		return{};
	}

	std::ifstream::pos_type fileSize = ifs.tellg();

	if ((unsigned int)fileSize == 0) {
		return{};
	}

	std::vector<char> result((unsigned int)fileSize);

	ifs.seekg(0, std::ios::beg);
	ifs.read((char*)&result[0], fileSize);

	return result;
}

int main(int argc, char* argv[])
{
	int communicationTimeout = 100 * 1000;
	if (argc < 2) {
		std::cout << "Usage: InstrumentedDebuggerTester.exe <shared memory name>";
		return -1;
	}

	SharedMemIPC sharedMemIPC(argv[1]);
	bool success = sharedMemIPC.initializeAsServer();
	std::cout << "InstrumentedDebuggerTester: initializeAsServer " << success << "(" << std::string(argv[1]) << ")" << std::endl;
	if (!success) {
		return -1;
	}

	std::cout << "InstrumentedDebuggerTester: sending SHARED_MEM_MESSAGE_READY_TO_FUZZ to the feeder " << std::endl;
	SharedMemMessage messageToFeeder(SHARED_MEM_MESSAGE_READY_TO_FUZZ, nullptr, 0);
	sharedMemIPC.sendMessageToClient(&messageToFeeder);

	int whattoDo;
	while (true) {
		SharedMemMessage message;
		sharedMemIPC.waitForNewMessageToServer(&message, communicationTimeout);
		if (message.getMessageType() == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
			std::cout << "InstrumentedDebuggerTester: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message, terminating!" << std::endl;
			return -1;
		}
		else if (message.getMessageType() == SHARED_MEM_MESSAGE_TERMINATE) {
			std::cout << "InstrumentedDebuggerTester: Recived a SHARED_MEM_MESSAGE_TERMINATE, terminating!" << std::endl;
			return 0;
		}
		else if (message.getMessageType() == SHARED_MEM_MESSAGE_FUZZ_FILENAME) {
			std::string fuzzFileName = std::string(message.getDataPointer(), message.getDataPointer() + message.getDataSize());
			std::cout << "InstrumentedDebuggerTester: fuzzFileName " << fuzzFileName << std::endl;
			std::vector<char> fuzzBytes = readAllBytesFromFile(fuzzFileName);
			std::cout << "InstrumentedDebuggerTester: fuzzBytes length " << fuzzBytes.size() << std::endl;

			if (fuzzBytes.size() < 1) {
				std::string errorDesc = "Could not open the target file";
				SharedMemMessage messageToFeeder(SHARED_MEM_MESSAGE_ERROR, errorDesc.c_str(), (int)errorDesc.length());
				sharedMemIPC.sendMessageToClient(&messageToFeeder);
				continue;
			}
			whattoDo = fuzzBytes[0];
		}
		else {
			std::cout << "InstrumentedDebuggerTester: Recived an unexpeced message, terminating!" << std::endl;
			return -1;
		}
		std::cout << "InstrumentedDebuggerTester: whattoDo " << whattoDo << std::endl;

		switch (whattoDo)
		{
		case 0: //do nothing
		{
			break;
		}
		case 1: //access violation
		{
			int * test = (int *)0x12345678;
			*test = 0x11223344;

			break;
		}
		case 2: //div by zero
		{
			int a = 0;
			int b = 1 / a;
			*(int*)b = 0x182;

			break;
		}
		case 3: //hang
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));
			break;
		}
		case 4: //print something
		{
			std::cout << "Hello world! " << std::endl;
			break;
		}
		default:
		{
			break;
		}
		}

		std::cout << "InstrumentedDebuggerTester: sending SHARED_MEM_MESSAGE_FUZZ_DONE to the feeder " << std::endl;
		SharedMemMessage messageToFeeder(SHARED_MEM_MESSAGE_FUZZ_DONE, nullptr, 0);
		sharedMemIPC.sendMessageToClient(&messageToFeeder);
	}

	return 0;
}
