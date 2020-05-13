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

#include "http.h"
#include "utils.h"
#include "Packet.h"

#if defined(_WIN32) || defined(_WIN64)
#define SOCKETTYPE SOCKET
#else
#define SOCKETTYPE int
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

bool sendPacketSequenceToHostAndPort(std::vector<Packet> packetSequence, std::string targethost, int serverport) {
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
		connectSocket = INVALID_SOCKET;
		return false;
	}

	// Connect to server.
	volatile int iResult = connect(connectSocket, reinterpret_cast<struct sockaddr*>(&saServer), sizeof(saServer));
	if (iResult == SOCKET_ERROR) {
		std::cout << "TCPFeeder: Failed to connect to the server" << std::endl;
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
		return false;
	}

	// Mark socket as non-blocking
#if defined(_WIN32) || defined(_WIN64)
	u_long mode = 1;  // 1 to enable non-blocking socket
	iResult = ioctlsocket(connectSocket, FIONBIO, &mode);
#else

	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == SOCKET_ERROR)
	{
		std::cout << "TCPFeeder: fcntl failed" << std::endl;
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
		return false;
	}
	flags = (flags | O_NONBLOCK);
	iResult = fcntl(fd, F_SETFL, flags);
#endif
	if (iResult == SOCKET_ERROR) {
		std::cout << "TCPFeeder: Failed to make the socket non-blocking" << std::endl;
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
		return false;
	}

	for (size_t pi = 0; pi < packetSequence.size(); pi++) {
		//Send packet
		if (packetSequence[pi].m_packetBytes.size() > 0) {
			iResult = static_cast<int>(send(connectSocket, &(packetSequence[pi].m_packetBytes)[0], static_cast<int>(packetSequence[pi].m_packetBytes.size()), 0));
			if (iResult == SOCKET_ERROR) {
				std::cout << "TCPFeeder: Failed to send packet " << pi << " to the server" << std::endl;
				closesocket(connectSocket);
				connectSocket = INVALID_SOCKET;
				return false;
			}
		}

		//Do what there should be done (wait, receive answer, ...)
		switch (packetSequence[pi].m_whatTodo) {
		case CONTINUE:
			break;
		case WAIT_FOR_ANY_RESPONSE:
		case WAIT_FOR_BYTE_SEQUENCE:
		{
			//recv response (at least one byte is required)
			std::vector<char>fullresponse;
			char outbuf[0x2000];
			while (true) {
				iResult = static_cast<int>(recv(connectSocket, outbuf, sizeof(outbuf), 0));
				if (iResult == SOCKET_ERROR || iResult == 0) {
#if defined(_WIN32) || defined(_WIN64)
					int lasterr = WSAGetLastError();
					if (lasterr == WSAEWOULDBLOCK) {
#else
					int lasterr = errno;
					if (lasterr == EAGAIN || lasterr == EWOULDBLOCK) {
#endif
						//No new bytes
						if (packetSequence[pi].m_whatTodo == WAIT_FOR_ANY_RESPONSE && fullresponse.size() > 0) {
							//as we already have seen some response: we are done
							break;
						}
						else if (packetSequence[pi].m_whatTodo == WAIT_FOR_BYTE_SEQUENCE && (std::search(fullresponse.begin(), fullresponse.end(), packetSequence[pi].m_waitForBytes.begin(), packetSequence[pi].m_waitForBytes.end()) != fullresponse.end())) {
							//we have seen the desired response: we are done
							break;
						}
						else {
							//as we have not yet seen the desired response: Wait
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
							continue;
						}
					}
#if defined(_WIN32) || defined(_WIN64)
					else if (lasterr == WSAECONNRESET) {
#else
					else if (iResult == 0) {
#endif
						//server shut down the connection.
						//Did we already send the last package?
						if (pi == packetSequence.size() - 1) {
							//Fair enough
							break;
						}
						else {
							std::cout << "TCPFeeder: Failed to receive a response for packet " << pi << " as the server prematurely closed the connection" << std::endl;
							closesocket(connectSocket);
							connectSocket = INVALID_SOCKET;
							return false;
						}
					}

					//Something went wrong!
					std::cout << "TCPFeeder: Failed to receive a response for packet " << pi << std::endl;
					closesocket(connectSocket);
					connectSocket = INVALID_SOCKET;
					return false;
				}
				fullresponse.insert(fullresponse.end(), outbuf, outbuf + iResult);
			}

			break;
		}

		case WAIT_N_MILLISECONDS:
			std::this_thread::sleep_for(std::chrono::milliseconds(packetSequence[pi].m_waitMS));
			break;
		default:
			std::cout << "TCPFeeder: Invalid whatTodo for packet " << pi << std::endl;
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			return false;
		}
	}

	//successfully sent all packages
	closesocket(connectSocket);
	connectSocket = INVALID_SOCKET;
	return true;
}

bool sendBytesToHostAndPort(std::vector<char> fuzzBytes, std::string targethost, int serverport) {
	// Returning true, as empty payloads are not forwarded
	if (fuzzBytes.size() == 0) {
		//std::cout << "TCPFeeder: sending SHARED_MEM_MESSAGE_FUZZ_DONE to the runner as empty payloads are not forwarded" << std::endl;
		return true;
	}

	Packet p(fuzzBytes, WHAT_TODO_AFTER_SEND::WAIT_FOR_ANY_RESPONSE);
	std::vector<Packet> pvec{ p };
	return sendPacketSequenceToHostAndPort(pvec, targethost, serverport);
}

bool firstByteMarksTestcaseType = false;
bool sendTestcaseToHostAndPort(std::vector<char> fuzzBytes, std::string targethost, int serverport) {
	if (firstByteMarksTestcaseType) {
		//The first byte tells us what to do.
		//This allows us combining multiple testcases into a single feeder, that can even be adjusted over time :)

		if (fuzzBytes.size() < 1) {
			//Invalid mutation
			return true;
		}

		char firstbyte = fuzzBytes[0];
		fuzzBytes.erase(fuzzBytes.begin());

		switch (firstbyte) { //range -127 ... 128
		case 0:
			return sendBytesToHostAndPort(fuzzBytes, targethost, serverport);

		default:
			//Not implemented
			return true;
		}
	}
	else {
		//Just send whatever came in to the server
		return sendBytesToHostAndPort(fuzzBytes, targethost, serverport);
	}
}

void preprocess(std::vector<char>* bytes) {
	// Add preprocession steps here as needed
	if (firstByteMarksTestcaseType) {
	}
	else {
		// Sample preprocessing for HTTP:
		// dropNoDoubleLinebreak(&bytes);
		// fixHTTPContentLength(&bytes);
		// performHTTPAuthAndManageSession();
	}
	return;
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
		if (sendTestcaseToHostAndPort(fuzzBytes, targethost, targetport)) {
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