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

Author(s): Thomas Riedmaier, Pascal Eckmann
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "CommInt.h"
#include "TRWorkerThreadStateBuilder.h"
#include "NotImplementedHandler.h"
#include "KillInstanceRequestHandler.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CommIntTester
{
	TEST_CLASS(CommIntTest)
	{
	public:

		bool CheckPortTCP(unsigned short int dwPort, const char* ipAddressStr)
		{
			struct hostent* remoteHost = gethostbyname(ipAddressStr);
			if (remoteHost == NULL) {
				return false;
			}

			struct in_addr remoteHostAddr;
			remoteHostAddr.s_addr = *(u_long*)remoteHost->h_addr_list[0];

			struct sockaddr_in client;

			int sock;

			client.sin_family = AF_INET;
			client.sin_port = htons(dwPort);
			client.sin_addr = remoteHostAddr;

			sock = (int)socket(AF_INET, SOCK_STREAM, 0);

			int result = connect(sock, (struct sockaddr*) &client, sizeof(client));

			if (result == 0) return true; // port is active and used
			else return false;
		}

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(CommInt_getFLUFFIMessageHandler)
		{
			TRWorkerThreadStateBuilder workerStateBuilder;
			CommInt ci((IWorkerThreadStateBuilder*)&workerStateBuilder, 1, 1);
			IFLUFFIMessageHandler* h = ci.getFLUFFIMessageHandler(FLUFFIMessage::FluffCase::kRequestTypeNotImplementedRequest);
			boolean debug = dynamic_cast<NotImplementedHandler*>(h) != nullptr;;
			Assert::IsTrue(dynamic_cast<NotImplementedHandler*>(h) != nullptr, L"The handler for kRequestTypeNotImplementedRequest was not the expected one");

			h = ci.getFLUFFIMessageHandler(FLUFFIMessage::FluffCase::kKillInstanceRequest);
			Assert::IsTrue(dynamic_cast<NotImplementedHandler*>(h) != nullptr, L"The handler for a not implemented request was not the expected one");

			KillInstanceRequestHandler killHandler;
			ci.registerFLUFFIMessageHandler(&killHandler, FLUFFIMessage::FluffCase::kKillInstanceRequest);
			h = ci.getFLUFFIMessageHandler(FLUFFIMessage::FluffCase::kKillInstanceRequest);
			Assert::IsTrue(dynamic_cast<KillInstanceRequestHandler*>(h) != nullptr, L"The handler for a kKillInstanceRequest was not the expected one");
		}

		TEST_METHOD(CommInt_getMyListeningPort)
		{
			TRWorkerThreadStateBuilder workerStateBuilder;
			CommInt ci((IWorkerThreadStateBuilder*)&workerStateBuilder, 1, 1);
			int port = ci.getMyListeningPort();
			Assert::IsTrue(CheckPortTCP(port, "127.0.0.1"), L"The port that the commint claimed to open cannot be connected");
		}

		TEST_METHOD(CommInt_getMyGUID)
		{
			TRWorkerThreadStateBuilder workerStateBuilder;
			CommInt ci((IWorkerThreadStateBuilder*)&workerStateBuilder, 1, 1);
			std::string myGUID = ci.getMyGUID();
			Assert::IsTrue(myGUID != "", L"MyGUID is empty!");
		}

		TEST_METHOD(CommInt_getOwnServiceDescriptor)
		{
			TRWorkerThreadStateBuilder workerStateBuilder;
			CommInt ci((IWorkerThreadStateBuilder*)&workerStateBuilder, 1, 1);
			FluffiServiceDescriptor mySD = ci.getOwnServiceDescriptor();
			std::vector<std::string> hapParts = Util::splitString(mySD.m_serviceHostAndPort, ":");
			Assert::IsTrue(hapParts.size() == 2, L"My own service desciptor has a host and port string that does not have the format aa:bb");
			Assert::IsTrue(hapParts[0].substr(hapParts[0].size() - 7) == ".fluffi", L"The hostname of my own service desciptor does not end in \".fluffi\"");

			Assert::IsTrue(CheckPortTCP(std::stoi(hapParts[1]), hapParts[0].substr(0, hapParts[0].size() - 7).c_str()), L"The host and port that the commint claimed to open cannot be connected");
		}

		TEST_METHOD(CommInt_getFreeListeningPort)
		{
			TRWorkerThreadStateBuilder workerStateBuilder;
			CommInt ci((IWorkerThreadStateBuilder*)&workerStateBuilder, 1, 1);
			int freePort = ci.getFreeListeningPort();

			//Check if socket can be bound
			SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

			struct sockaddr_in mySockaddr;
			mySockaddr.sin_family = AF_INET;
			mySockaddr.sin_port = htons(freePort);
			InetPton(AF_INET, "127.0.0.1", &(mySockaddr.sin_addr));
			int result = bind(sock, (sockaddr*)&mySockaddr, sizeof(mySockaddr));
			closesocket(sock);
			Assert::IsTrue(result == 0, L"Could not bind to the allegedly free listen port");
		}
	};
}
