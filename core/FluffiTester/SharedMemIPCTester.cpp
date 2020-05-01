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

Author(s): Thomas Riedmaier
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "SharedMemIPC.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SharedMemIPCTester
{
	TEST_CLASS(SharedMemIPCTest)
	{
	public:

		static void repeater(int receivetimeout, int numofmessages) {
			SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
			Assert::IsTrue(SharedMemIPC.initializeAsClient(), L"initializeAsClient failed");

			for (int i = 0; i < numofmessages; i++) {
				SharedMemMessage message;
				Assert::IsTrue(SharedMemIPC.waitForNewMessageToClient(&message, receivetimeout), L"waitForNewMessageToClient failed");
				Assert::IsTrue(SharedMemIPC.sendMessageToServer(&message), L"sendMessageToServer failed");
			}
		}

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(SharedMemIPC_SendSimpleMessage)
		{
			SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
			Assert::IsTrue(SharedMemIPC.initializeAsServer(), L"initializeAsServer failed");

			std::thread repeaterthread(repeater, 1000, 1);

			char data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
			SharedMemMessage messageToSend(SharedMemMessageType::SHARED_MEM_MESSAGE_FUZZ_FILENAME, data, sizeof(data));
			Assert::IsTrue(SharedMemIPC.sendMessageToClient(&messageToSend), L"sendMessageToClient failed");
			SharedMemMessage receivedMessage;
			Assert::IsTrue(SharedMemIPC.waitForNewMessageToServer(&receivedMessage, 1000), L"waitForNewMessageToServer failed");

			Assert::IsTrue(messageToSend.getMessageType() == receivedMessage.getMessageType(), L"Message type of the received message did not match the expected one");
			Assert::IsTrue(std::equal(messageToSend.getDataPointer(), messageToSend.getDataPointer() + messageToSend.getDataSize(), receivedMessage.getDataPointer()), L"Message content of the received message did not match the expected one");

			repeaterthread.join();
		}

		TEST_METHOD(SharedMemIPC_SendEmptyMessage)
		{
			SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
			Assert::IsTrue(SharedMemIPC.initializeAsServer(), L"initializeAsServer failed");

			std::thread repeaterthread(repeater, 1000, 1);

			SharedMemMessage messageToSend(SharedMemMessageType::SHARED_MEM_MESSAGE_FUZZ_FILENAME, nullptr, 0);
			Assert::IsTrue(SharedMemIPC.sendMessageToClient(&messageToSend), L"sendMessageToClient failed");
			SharedMemMessage receivedMessage;
			Assert::IsTrue(SharedMemIPC.waitForNewMessageToServer(&receivedMessage, 1000), L"waitForNewMessageToServer failed");

			Assert::IsTrue(messageToSend.getMessageType() == receivedMessage.getMessageType(), L"Message type of the received message did not match the expected one");
			Assert::IsTrue(std::equal(messageToSend.getDataPointer(), messageToSend.getDataPointer() + messageToSend.getDataSize(), receivedMessage.getDataPointer()), L"Message content of the received message did not match the expected one");

			repeaterthread.join();
		}

		TEST_METHOD(SharedMemIPC_SendTooBigMessage)
		{
			SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
			Assert::IsTrue(SharedMemIPC.initializeAsServer(), L"initializeAsServer failed");

			char data[0x800];
			SharedMemMessage messageToSend(SharedMemMessageType::SHARED_MEM_MESSAGE_FUZZ_FILENAME, data, sizeof(data));
			Assert::IsFalse(SharedMemIPC.sendMessageToClient(&messageToSend), L"sendMessageToClient succedded although it should have failed");

			Assert::IsFalse(SharedMemIPC.sendMessageToServer(&messageToSend), L"sendMessageToServer succedded although it should have failed");
		}

		TEST_METHOD(SharedMemIPC_SendMultipleMessages)
		{
			std::vector<char> message[] = { std::vector<char>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
			 std::vector<char>{ 0},
			 std::vector<char>{ 0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
			 std::vector<char>{ (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff },
			std::vector<char>{ 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'} };

			SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
			Assert::IsTrue(SharedMemIPC.initializeAsServer(), L"initializeAsServer failed");

			std::thread repeaterthread(repeater, 1000, 5);

			for (int i = 0; i < 5; i++) {
				SharedMemMessage messageToSend(SharedMemMessageType::SHARED_MEM_MESSAGE_FUZZ_FILENAME, &message[i].front(), (int)message[i].size());
				Assert::IsTrue(SharedMemIPC.sendMessageToClient(&messageToSend), L"sendMessageToClient failed");
				SharedMemMessage receivedMessage;
				Assert::IsTrue(SharedMemIPC.waitForNewMessageToServer(&receivedMessage, 1000), L"waitForNewMessageToServer failed");

				Assert::IsTrue(messageToSend.getMessageType() == receivedMessage.getMessageType(), L"Message type of the received message did not match the expected one");
				Assert::IsTrue(std::equal(messageToSend.getDataPointer(), messageToSend.getDataPointer() + messageToSend.getDataSize(), receivedMessage.getDataPointer()), L"Message content of the received message did not match the expected one");
			}
			repeaterthread.join();
		}

		TEST_METHOD(SharedMemIPC_Timeout)
		{
			SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
			Assert::IsTrue(SharedMemIPC.initializeAsServer(), L"initializeAsServer failed");

			SharedMemMessage receivedMessage;
			Assert::IsFalse(SharedMemIPC.waitForNewMessageToServer(&receivedMessage, 1), L"waitForNewMessageToServer did not experience a timeout although it should have!");
		}

		TEST_METHOD(SharedMemIPC_TestHandleLeak)
		{
			DWORD preHandles = 0;
			DWORD postHandles = 0;
			GetProcessHandleCount(GetCurrentProcess(), &preHandles);

			for (int i = 0; i < 50; i++) {
				SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
				Assert::IsTrue(SharedMemIPC.initializeAsServer(), L"initializeAsServer failed");

				std::thread repeaterthread(repeater, 1000, 1);

				char data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
				SharedMemMessage messageToSend(SharedMemMessageType::SHARED_MEM_MESSAGE_FUZZ_FILENAME, data, sizeof(data));
				Assert::IsTrue(SharedMemIPC.sendMessageToClient(&messageToSend), L"sendMessageToClient failed");
				SharedMemMessage receivedMessage;
				Assert::IsTrue(SharedMemIPC.waitForNewMessageToServer(&receivedMessage, 1000), L"waitForNewMessageToServer failed");

				Assert::IsTrue(messageToSend.getMessageType() == receivedMessage.getMessageType(), L"Message type of the received message did not match the expected one");
				Assert::IsTrue(std::equal(messageToSend.getDataPointer(), messageToSend.getDataPointer() + messageToSend.getDataSize(), receivedMessage.getDataPointer()), L"Message content of the received message did not match the expected one");

				repeaterthread.join();
			}

			//ProcessHandle count is extremely fluctuating. Idea: If there is a leak, it should occour every time ;)
			GetProcessHandleCount(GetCurrentProcess(), &postHandles);
			Assert::IsTrue(preHandles + 25 > postHandles, L"There seems to be a handle leak");
		}

		TEST_METHOD(SharedMemIPC_InterruptWait)
		{
			SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
			Assert::IsTrue(SharedMemIPC.initializeAsServer(), L"initializeAsServer failed");

			std::thread repeaterthread(repeater, 1000, 1);

			HANDLE interruptEvent = CreateEvent(NULL, false, false, "InterruptEvent");
			SetEvent(interruptEvent);

			SharedMemMessage messageToSend(SharedMemMessageType::SHARED_MEM_MESSAGE_FUZZ_FILENAME, nullptr, 0);
			Assert::IsTrue(SharedMemIPC.sendMessageToClient(&messageToSend), L"sendMessageToClient failed");
			SharedMemMessage receivedMessage;
			Assert::IsFalse(SharedMemIPC.waitForNewMessageToServer(&receivedMessage, 0, interruptEvent), L"waitForNewMessageToServer succeeded although it should have failed");

			CloseHandle(interruptEvent);

			Assert::IsTrue(SharedMemMessageType::SHARED_MEM_MESSAGE_WAIT_INTERRUPTED == receivedMessage.getMessageType(), L"Message type of the received message did not match the expected one");

			repeaterthread.join();
		}

		TEST_METHOD(SharedMemIPC_TimeoutWithInterruptEvent)
		{
			SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
			Assert::IsTrue(SharedMemIPC.initializeAsServer(), L"initializeAsServer failed");

			std::thread repeaterthread(repeater, 1000, 1);

			HANDLE interruptEvent = CreateEvent(NULL, false, false, "InterruptEvent");

			SharedMemMessage messageToSend(SharedMemMessageType::SHARED_MEM_MESSAGE_FUZZ_FILENAME, nullptr, 0);
			Assert::IsTrue(SharedMemIPC.sendMessageToClient(&messageToSend), L"sendMessageToClient failed");
			SharedMemMessage receivedMessage;
			Assert::IsFalse(SharedMemIPC.waitForNewMessageToServer(&receivedMessage, 0, interruptEvent), L"waitForNewMessageToServer succeeded although it should have failed");

			CloseHandle(interruptEvent);

			Assert::IsTrue(SharedMemMessageType::SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT == receivedMessage.getMessageType(), L"Message type of the received message did not match the expected one");

			repeaterthread.join();
		}

		TEST_METHOD(SharedMemIPC_NormalWithInterruptEvent)
		{
			SharedMemIPC SharedMemIPC(std::string("FLUFFI_SharedMemForTest").c_str(), 0x1000);
			Assert::IsTrue(SharedMemIPC.initializeAsServer(), L"initializeAsServer failed");

			std::thread repeaterthread(repeater, 1000, 1);

			HANDLE interruptEvent = CreateEvent(NULL, false, false, "InterruptEvent");

			SharedMemMessage messageToSend(SharedMemMessageType::SHARED_MEM_MESSAGE_FUZZ_FILENAME, nullptr, 0);
			Assert::IsTrue(SharedMemIPC.sendMessageToClient(&messageToSend), L"sendMessageToClient failed");
			SharedMemMessage receivedMessage;
			Assert::IsTrue(SharedMemIPC.waitForNewMessageToServer(&receivedMessage, 1000, interruptEvent), L"waitForNewMessageToServer failed");

			CloseHandle(interruptEvent);

			Assert::IsTrue(messageToSend.getMessageType() == receivedMessage.getMessageType(), L"Message type of the received message did not match the expected one");
			Assert::IsTrue(std::equal(messageToSend.getDataPointer(), messageToSend.getDataPointer() + messageToSend.getDataSize(), receivedMessage.getDataPointer()), L"Message content of the received message did not match the expected one");

			repeaterthread.join();
		}
	};
}
