/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

//g++ --std=c++11 DebuggerTesterTCP.cpp -o DebuggerTesterTCP

#include "stdafx.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define LINKTOUSER32

#if defined(_WIN32) || defined(_WIN64)
#define SOCKETTYPE SOCKET
#else
#define SOCKETTYPE int
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SD_SEND SHUT_WR

int WSAGetLastError(){
return errno;
}
#endif

void parseInput(char* recvbuf) {
	int whattoDo = std::stoi(recvbuf);

	switch (whattoDo)
	{
	case 0: //do nothing
		break;
	case 1: //access violation
	{
		int * test = (int *)0x12345678;
		*test = 0x11223344;
	}
	break;
	case 2: //div by zero
	{
		int a = 0;
		int b = 1 / a;
		*(int*)b = 0x182;
	}
	break;
	case 3: //hang
		std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));
		break;

	default:
		exit(0);
	}

	return;
}

int handleSingleConnection() {
	int iResult;

	SOCKETTYPE ListenSocket = INVALID_SOCKET;
	SOCKETTYPE ClientSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
#if defined(_WIN32) || defined(_WIN64)
		WSACleanup();
#endif
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
#if defined(_WIN32) || defined(_WIN64)
		WSACleanup();
#endif
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
#if defined(_WIN32) || defined(_WIN64)
		WSACleanup();
#endif
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
#if defined(_WIN32) || defined(_WIN64)
		WSACleanup();
#endif
		return 1;
	}

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
#if defined(_WIN32) || defined(_WIN64)
		WSACleanup();
#endif
		return 1;
	}

	// No longer need server socket
	closesocket(ListenSocket);

	// Receive until the peer shuts down the connection
	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			parseInput(recvbuf);

			// Echo the buffer back to the sender
			iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
#if defined(_WIN32) || defined(_WIN64)
				WSACleanup();
#endif
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
#if defined(_WIN32) || defined(_WIN64)
			WSACleanup();
#endif
			return 1;
		}
	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
#if defined(_WIN32) || defined(_WIN64)
		WSACleanup();
#endif
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
}

int main(int argc, char* argv[])
{
#if defined(_WIN32) || defined(_WIN64)
#ifdef LINKTOUSER32
	char input[] = "tomtest";
	char* re = CharLower(input);
#endif
#endif
	
	int iResult;

#if defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
#endif

	while (true) {
		handleSingleConnection();
	}

#if defined(_WIN32) || defined(_WIN64)
	WSACleanup();
#endif

	return 0;
}
