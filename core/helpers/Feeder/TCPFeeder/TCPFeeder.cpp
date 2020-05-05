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

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "SharedMemIPC.h"

#include "http.h"
#include "utils.h"

#if defined(_WIN32) || defined(_WIN64)
#define SOCKETTYPE SOCKET
#else
#define SOCKETTYPE int
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

void preprocess(std::vector<char>* bytes) {
	// Add preprocession steps here as needed, e.g. for HTTP:
	// dropNoDoubleLinebreak(&bytes);
	// fixHTTPContentLength(&bytes);
	// performHTTPAuthAndManageSession();
	return;
}

bool sendBytesToHostAndPort(std::vector<char> fuzzBytes, std::string targethost, int serverport) {
	// Returning true, as empty payloads are not forwarded
	if (fuzzBytes.size() == 0) {
		//std::cout << "TCPFeeder: sending SHARED_MEM_MESSAGE_FUZZ_DONE to the runner as empty payloads are not forwarded" << std::endl;
		return true;
	}
	//try until the runner kills us
	while (true) {
		// Create a SOCKET for connecting to server
		SOCKETTYPE connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (connectSocket == INVALID_SOCKET) {
			std::cout << "TCPFeeder: socket failed!" << std::endl;
			return false;
		}

		struct sockaddr_in saServer;
		memset(&saServer, 0, sizeof(saServer));
		saServer.sin_family = AF_INET;
		saServer.sin_port = htons(serverport);
#if defined(_WIN32) || defined(_WIN64)
		if (inet_pton(AF_INET, targethost.c_str(), &saServer.sin_addr) == 0) {
#else
		if (inet_aton(targethost.c_str(), &saServer.sin_addr) == 0) {
#endif
			std::cout << "TCPFeeder: Error using inet_aton" << std::endl;
			closesocket(connectSocket);
			return false;
		}

		// Connect to server.
		volatile int iResult = connect(connectSocket, reinterpret_cast<struct sockaddr*>(&saServer), sizeof(saServer));
		if (iResult == SOCKET_ERROR) {
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}

		iResult = static_cast<int>(send(connectSocket, &(fuzzBytes)[0], static_cast<int>(fuzzBytes.size()), 0));
		if (iResult == SOCKET_ERROR) {
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		char outbuf[1];
		iResult = static_cast<int>(recv(connectSocket, outbuf, sizeof(outbuf), 0)); //recv  one byte (if possible)
		if (iResult == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
			int lasterr = WSAGetLastError();
#else
			//int lasterr = errno;
#endif
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;

#if defined(_WIN32) || defined(_WIN64)
			if (lasterr == WSAECONNRESET) {
				//server shut down the connection. Fair enough
				break;
			}
#endif

			continue;
		}

		//success
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
		break;
	}

	return true;
}

int main(int argc, char* argv[])
{
	int feederTimeoutMS = 10 * 60 * 1000;
	std::string targethost = "127.0.0.1";

	auto timeOfLastConnectAttempt = std::chrono::system_clock::now() + std::chrono::milliseconds(feederTimeoutMS);
	uint16_t targetport = 0;
	std::string ipcName;
	initFromArgs(argc, argv, targetport, ipcName);

	SharedMemIPC sharedMemIPC_ToRunner(ipcName.c_str());
	initializeIPC(sharedMemIPC_ToRunner);

	uint16_t targetPID = getTargetPID(sharedMemIPC_ToRunner, feederTimeoutMS);

	if (targetport == 0) {
		targetport = getServerPortFromPIDOrTimeout(sharedMemIPC_ToRunner, targetPID, timeOfLastConnectAttempt);
	}

	waitForPortOpenOrTimeout(sharedMemIPC_ToRunner, targethost, targetport, timeOfLastConnectAttempt);
	std::cout << "TCPFeeder: It was possible to establish a tcp connection to the target. We are good to go!" << std::endl;

	SharedMemMessage readyToFuzzMessage = SharedMemMessage(SHARED_MEM_MESSAGE_READY_TO_FUZZ, nullptr, 0);
	sharedMemIPC_ToRunner.sendMessageToServer(&readyToFuzzMessage);
	while (true) {
		std::string fuzzFileName = getNewFuzzFileOrDie(sharedMemIPC_ToRunner, feederTimeoutMS);
		std::vector<char> fuzzBytes = readAllBytesFromFile(fuzzFileName);

		preprocess(&fuzzBytes);
		if (sendBytesToHostAndPort(fuzzBytes, targethost, targetport)) {
			SharedMemMessage fuzzDoneMessage(SHARED_MEM_MESSAGE_FUZZ_DONE, nullptr, 0);
			sharedMemIPC_ToRunner.sendMessageToServer(&fuzzDoneMessage);
		}
		else {
			std::string errorDesc = "Failed sending the fuzz file bytes to the target port";
			SharedMemMessage messageToFeeder(SHARED_MEM_MESSAGE_ERROR, errorDesc.c_str(), static_cast<int>(errorDesc.length()));
			sharedMemIPC_ToRunner.sendMessageToServer(&messageToFeeder);
		}
	}

#if defined(_WIN32) || defined(_WIN64)
	WSACleanup();
#endif
	return 0;
}
