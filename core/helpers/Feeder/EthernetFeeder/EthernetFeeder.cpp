/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Pascal Eckmann
*/

#include "stdafx.h"
#include "SharedMemIPC.h"

#define RESP_TIMEOUT_MS 3000
unsigned char KNOWN_GOOD[] = { 0x00, 0x88, 0x92, 0xFE, 0xFE, 0x05, 0x00, 0x07, 0x01, 0x00, 0x06, 0x00, 0x80, 0x00, 0x04, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#if defined(_WIN32) || defined(_WIN64)
#define SOCKETTYPE SOCKET
#else
#define SOCKETTYPE int
#define closesocket close
#define ETHER_TYPE	0x0800
#endif

void preprocess(std::vector<char>* bytes) {
	//add preprocession steps here as needed
	return;
}

void printMacToStream(std::ostream& os, unsigned char MACData[])
{
	// Possibly add length assertion
	char oldFill = os.fill('0');

	os << std::setw(2) << std::hex << static_cast<unsigned int>(MACData[0]);
	for (int i = 1; i < 6; ++i) {
		os << '-' << std::setw(2) << std::hex << static_cast<unsigned int>(MACData[i]);
	}

	os.fill(oldFill);

	// Possibly add:
	// os << std::endl;
}

bool sendBytesToTarget(std::vector<char>* fuzzBytes, unsigned char* targetMAC, bool waitForResponse) {
#if defined(_WIN32) || defined(_WIN64)
	std::cout << "EthernetFeeder::sendBytesToTarget is not yet implemented on Windows." << std::endl;
	return false;
#else

	//first byte tells us what to do
	char whattodo = fuzzBytes->at(0);

	switch (whattodo)
	{
	case 0:
	{
		//Inspired by https://gist.github.com/austinmarton/1922600

		int sockfd;
		struct ifreq if_idx;
		struct ifreq if_mac;
		char* sendbuf = new char[fuzzBytes->size() - 1 + 12];
		struct ether_header* eh = reinterpret_cast<struct ether_header*>(sendbuf);
		struct sockaddr_ll socket_address;
		const char* ifName = "eth0";

		std::chrono::time_point<std::chrono::system_clock> routineEntryTimeStamp = std::chrono::system_clock::now();
		std::chrono::time_point<std::chrono::system_clock> timestampToLeave = routineEntryTimeStamp + std::chrono::milliseconds(RESP_TIMEOUT_MS);

		/* Open RAW socket to send on */
		if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
			std::cout << "EthernetFeeder::socket failed:" << errno << std::endl;
			delete[] sendbuf;
			return false;
		}

		/* Get the index of the interface to send on */
		memset(&if_idx, 0, sizeof(struct ifreq));
		strncpy(if_idx.ifr_name, ifName, IFNAMSIZ - 1);
		if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
			std::cout << "EthernetFeeder::SIOCGIFINDEX failed:" << errno << std::endl;
			closesocket(sockfd);
			delete[] sendbuf;
			return false;
		}
		/* Get the MAC address of the interface to send on */
		memset(&if_mac, 0, sizeof(struct ifreq));
		strncpy(if_mac.ifr_name, ifName, IFNAMSIZ - 1);
		if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
			std::cout << "EthernetFeeder::SIOCGIFHWADDR failed:" << errno << std::endl;
			closesocket(sockfd);
			delete[] sendbuf;
			return false;
		}

		/* Construct the Ethernet header */
		memset(sendbuf, 0, fuzzBytes->size() - 1 + 12);
		/* Ethernet header */
		eh->ether_shost[0] = (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[0];
		eh->ether_shost[1] = (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[1];
		eh->ether_shost[2] = (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[2];
		eh->ether_shost[3] = (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[3];
		eh->ether_shost[4] = (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[4];
		eh->ether_shost[5] = (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[5];
		eh->ether_dhost[0] = targetMAC[0];
		eh->ether_dhost[1] = targetMAC[1];
		eh->ether_dhost[2] = targetMAC[2];
		eh->ether_dhost[3] = targetMAC[3];
		eh->ether_dhost[4] = targetMAC[4];
		eh->ether_dhost[5] = targetMAC[5];

		/* Packet data */
		memcpy(&sendbuf[12], &(*fuzzBytes)[1], fuzzBytes->size() - 1);

		/* Index of the network device */
		socket_address.sll_ifindex = if_idx.ifr_ifindex;
		/* Address length*/
		socket_address.sll_halen = ETH_ALEN;
		/* Destination MAC */
		socket_address.sll_addr[0] = targetMAC[0];
		socket_address.sll_addr[1] = targetMAC[1];
		socket_address.sll_addr[2] = targetMAC[2];
		socket_address.sll_addr[3] = targetMAC[3];
		socket_address.sll_addr[4] = targetMAC[4];
		socket_address.sll_addr[5] = targetMAC[5];

		/* Send packet */
		if (sendto(sockfd, sendbuf, fuzzBytes->size() - 1 + 12, 0, reinterpret_cast<struct sockaddr*>(&socket_address), sizeof(struct sockaddr_ll)) < 0) {
			std::cout << "EthernetFeeder::Sendto failed:" << errno << std::endl;
			closesocket(sockfd);
			delete[] sendbuf;
			return false;
		}

		closesocket(sockfd);
		delete[] sendbuf;

		/* Wait for response */
		if (waitForResponse) {
			//inspired by http://www.microhowto.info/howto/capture_ethernet_frames_using_an_af_packet_socket_in_c.html
			// and https://gist.github.com/austinmarton/2862515

			/* Open AF_PACKET socket, listening for EtherType ETH_P_ALL */
			if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
				std::cout << "EthernetFeeder::socket failed (2):" << errno << std::endl;
				return false;
			}

			/* Put the interface into promiscuous mode */
			struct packet_mreq mreq;
			memset(&mreq, 0, sizeof(mreq));
			mreq.mr_ifindex = if_idx.ifr_ifindex;
			mreq.mr_type = PACKET_MR_PROMISC;
			if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
				std::cout << "EthernetFeeder::setsockopt failed:" << errno << std::endl;
				closesocket(sockfd);
				return false;
			}

			/* Bind to device eth0 */
			memset(&socket_address, 0, sizeof(socket_address));
			socket_address.sll_family = AF_PACKET;
			socket_address.sll_ifindex = if_idx.ifr_ifindex;
			socket_address.sll_protocol = htons(ETH_P_ALL);
			if (bind(sockfd, reinterpret_cast<struct sockaddr*>(&socket_address), sizeof(socket_address)) == -1) {
				std::cout << "EthernetFeeder::bind failed:" << errno << std::endl;
				closesocket(sockfd);
				return false;
			}

			uint8_t buf[65537];
			eh = reinterpret_cast<struct ether_header*>(buf);
			while (std::chrono::system_clock::now() < timestampToLeave) {
				/* set timeout */
				long timeleft = std::chrono::duration_cast<std::chrono::milliseconds>(timestampToLeave - std::chrono::system_clock::now()).count();
				struct timeval tv;
				tv.tv_sec = timeleft / 1000;
				tv.tv_usec = ((timeleft) % 1000) * 1000;
				if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
					std::cout << "EthernetFeeder::setsockopt failed (2):" << errno << std::endl;
					closesocket(sockfd);
					return false;
				}

				memset(buf, 0, sizeof(buf));
				ssize_t  numbytes = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);

				/* Check the packet is for me AND its from the target */
				if (eh->ether_dhost[0] == (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[0] &&
					eh->ether_dhost[1] == (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[1] &&
					eh->ether_dhost[2] == (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[2] &&
					eh->ether_dhost[3] == (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[3] &&
					eh->ether_dhost[4] == (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[4] &&
					eh->ether_dhost[5] == (reinterpret_cast<uint8_t*>(&if_mac.ifr_hwaddr.sa_data))[5] &&
					eh->ether_shost[0] == targetMAC[0] &&
					eh->ether_shost[1] == targetMAC[1] &&
					eh->ether_shost[2] == targetMAC[2] &&
					eh->ether_shost[3] == targetMAC[3] &&
					eh->ether_shost[4] == targetMAC[4] &&
					eh->ether_shost[5] == targetMAC[5])
				{
					//For me - from the target
					std::cout << "EthernetFeeder::received a response packet of length " << numbytes;
					closesocket(sockfd);
					return true;
				}
				else {
					//Not for me, or not from the target (noise)
					std::cout << "EthernetFeeder::received packet from ";
					printMacToStream(std::cout, eh->ether_shost);
					std::cout << " for ";
					printMacToStream(std::cout, eh->ether_dhost);
					std::cout << std::endl;

					std::cout << "EthernetFeeder::However, we are looking for a packet from ";
					printMacToStream(std::cout, targetMAC);
					std::cout << " for ";
					printMacToStream(std::cout, reinterpret_cast<unsigned char *>(if_mac.ifr_hwaddr.sa_data));
					std::cout << std::endl;

					std::cout << "EthernetFeeder::Let's see, if there are more packets!" << std::endl;

					continue;
				}
			}

			//We did not receive an answer
			closesocket(sockfd);
			return false;
		}
		else {
			//We are done
			std::cout << "EthernetFeeder::received the expected server resonse packet :)" << std::endl;
			return true;
		}
	}
	break;
	default:
		return true;
	}

#endif
}

std::vector<char> readAllBytesFromFile(const std::string filename)
{
	FILE* inputFile;
	if (0 != fopen_s(&inputFile, filename.c_str(), "rb")) {
		std::cout << "EthernetFeeder::readAllBytesFromFile failed to open the file " << filename << std::endl;
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

	size_t bytesRead = static_cast<size_t>(fread(&result[0], 1, fileSize, inputFile));
	if (bytesRead != static_cast<size_t>(fileSize)) {
		std::cout << "readAllBytesFromFile failed to read all bytes! Bytes read: " << bytesRead << ". Filesize " << fileSize << std::endl;
	}

	fclose(inputFile);

	return result;
}

bool isServerAlive(unsigned char* targetMAC) {
	int n = sizeof(KNOWN_GOOD) / sizeof(KNOWN_GOOD[0]);
	std::vector<char> knownGood(KNOWN_GOOD, KNOWN_GOOD + n);
	return sendBytesToTarget(&knownGood, targetMAC, true);
}

bool waitUntilServerResponds(unsigned char* targetMAC) {
	while (true) {
		bool isalive = isServerAlive(targetMAC);
		if (isalive) {
			break;
		}
		else {
			std::cout << "EthernetFeeder:isServerAlive returned false" << std::endl;
		}
	}
	std::cout << "EthernetFeeder:Server responds!" << std::endl;
	return true;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << "Usage: EthernetFeeder.exe <shared memory name>" << std::endl;
		return -1;
	}

	unsigned char targetMAC[] = { 0x3a ,0xe9 ,0x58,0x64 ,0x29 ,0xd1 };

	SharedMemIPC sharedMemIPC_ToRunner(argv[argc - 1]);
	bool success = sharedMemIPC_ToRunner.initializeAsClient();
	std::cout << "EthernetFeeder: initializeAsClient for runner" << success << std::endl;
	if (!success) {
		return -1;
	}

	int feederTimeoutMS = 10 * 60 * 1000;

	std::string targetPIDAsString;
	SharedMemMessage message__FromRunner;
	sharedMemIPC_ToRunner.waitForNewMessageToClient(&message__FromRunner, feederTimeoutMS);
	if (message__FromRunner.getMessageType() == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
		std::cout << "EthernetFeeder: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message, terminating!" << std::endl;
		return -1;
	}
	else if (message__FromRunner.getMessageType() == SHARED_MEM_MESSAGE_TARGET_PID) {
		std::cout << "EthernetFeeder: Recived a SHARED_MEM_MESSAGE_TARGET_PID message!" << std::endl;
		std::string targetPIDAsString = std::string(message__FromRunner.getDataPointer(), message__FromRunner.getDataPointer() + message__FromRunner.getDataSize());
		std::cout << "EthernetFeeder: target pid is \"" << targetPIDAsString << "\"" << std::endl;
	}
	else {
		std::cout << "EthernetFeeder: Recived an unexpeced message, terminating!" << std::endl;
		return -1;
	}

	waitUntilServerResponds(targetMAC);

	SharedMemMessage readyToFuzzMessage = SharedMemMessage(SHARED_MEM_MESSAGE_READY_TO_FUZZ, nullptr, 0);
	sharedMemIPC_ToRunner.sendMessageToServer(&readyToFuzzMessage);

	SharedMemMessage message__FromTarget;
	SharedMemMessage fuzzDoneMessage(SHARED_MEM_MESSAGE_FUZZ_DONE, nullptr, 0);
	while (true) {
		sharedMemIPC_ToRunner.waitForNewMessageToClient(&message__FromRunner, feederTimeoutMS);
		SharedMemMessageType messageFromRunnerType = message__FromRunner.getMessageType();
		if (messageFromRunnerType == SHARED_MEM_MESSAGE_FUZZ_FILENAME) {
			std::cout << "EthernetFeeder: Recived a SHARED_MEM_MESSAGE_FUZZ_FILENAME message :)" << std::endl;
			std::string fuzzFileName = std::string(message__FromRunner.getDataPointer(), message__FromRunner.getDataPointer() + message__FromRunner.getDataSize());
			std::vector<char> fuzzBytes = readAllBytesFromFile(fuzzFileName);

			preprocess(&fuzzBytes);

			if (fuzzBytes.size() <= 2 || fuzzBytes.size() >= 1504) {
				std::cout << "EthernetFeeder: sending SHARED_MEM_MESSAGE_FUZZ_DONE to the runner as empty payloads and too big payloads are not forwarded" << std::endl;
				sharedMemIPC_ToRunner.sendMessageToServer(&fuzzDoneMessage);
			}
			else if (sendBytesToTarget(&fuzzBytes, targetMAC, false) && waitUntilServerResponds(targetMAC)) {
				std::cout << "EthernetFeeder: sending SHARED_MEM_MESSAGE_FUZZ_DONE to the runner " << std::endl;
				sharedMemIPC_ToRunner.sendMessageToServer(&fuzzDoneMessage);
			}
			else {
				std::string errorDesc = "EthernetFeeder:Failed sending the fuzz file bytes to the target";
				std::cout << errorDesc << std::endl;
				SharedMemMessage messageToFeeder(SHARED_MEM_MESSAGE_ERROR, errorDesc.c_str(), static_cast<int>(errorDesc.length()));
				sharedMemIPC_ToRunner.sendMessageToServer(&messageToFeeder);
			}
		}
		else if (messageFromRunnerType == SHARED_MEM_MESSAGE_TERMINATE) {
			std::cout << "EthernetFeeder: Recived a SHARED_MEM_MESSAGE_TERMINATE, terminating!" << std::endl;

			return 0;
		}
		else if (messageFromRunnerType == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
			std::cout << "EthernetFeeder: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message, terminating!" << std::endl;

			return -1;
		}
		else {
			std::cout << "EthernetFeeder: Recived an unexpeced message, terminating!" << std::endl;

			return -1;
		}
	}

	return 0;
}
