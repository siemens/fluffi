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
#include "GDBEmulator.h"

std::shared_ptr<GDBEmulator> GDBEmulator::instance = nullptr;

GDBEmulator::GDBEmulator() :
	m_requestIPC("kfuzz_windbg_request_response", 1 * 1024 * 1024),
	m_subscriberIPC("kfuzz_windbg_publish_subscribe"),
	m_mutex_(),
	m_WinDbgMessageDispatcher(nullptr),
	m_stopRequested(false),
	m_SharedMemIPCInterruptEvent(NULL),
	m_waitingForForcedBreak(false),
	m_fatalSystemErrorMessage()
{
}

GDBEmulator:: ~GDBEmulator() {
	m_stopRequested = true;
	if (m_SharedMemIPCInterruptEvent != NULL) {
		SetEvent(m_SharedMemIPCInterruptEvent);
	}

	if (m_WinDbgMessageDispatcher != nullptr) {
		m_WinDbgMessageDispatcher->join();
	}

	if (m_SharedMemIPCInterruptEvent != NULL) {
		CloseHandle(m_SharedMemIPCInterruptEvent);
		m_SharedMemIPCInterruptEvent = NULL;
	}
}

bool GDBEmulator::init() {
	std::cout << "GNU gdb emulator starting up ..." << std::endl;

	if (!SetConsoleCtrlHandler(GDBEmulator::consoleHandler, TRUE)) {
		std::cout << "... failed! Could not set control handler";
		return false;
	}

	if (!m_requestIPC.initializeAsClient() || !m_subscriberIPC.initializeAsClient()) {
		std::cout << "... failed! Could not connect to WinDbg" << std::endl;
		return false;
	}

	m_SharedMemIPCInterruptEvent = CreateEvent(NULL, false, false, NULL);
	if (m_SharedMemIPCInterruptEvent != NULL) {
		m_WinDbgMessageDispatcher = std::make_unique<std::thread>(&GDBEmulator::winDbgMessageDispatcher, this);
	}

	std::cout << "... succeeded. Ready to go!" << std::endl;

	return true;
}

std::string GDBEmulator::sendCommandToWinDbgAndGetResponse(std::string command) {
	//Make sure only one thread at a time communicates with WinDbg (Ctrl+C handling is asynchronous)
	std::unique_lock<std::recursive_mutex> mlock(m_mutex_);

	int timeoutMilliseconds = 10000;
	SharedMemMessage response;
	SharedMemMessage request = stringToMessage(command);
	if (!m_requestIPC.sendMessageToServer(&request) || !m_requestIPC.waitForNewMessageToClient(&response, timeoutMilliseconds)) {
		return "ERROR: Could not forward a command to WinDbg, or WinDbg did not respond in time for command \"" + command + "\"";
	}
	SharedMemMessageType messageType = response.getMessageType();
	if (messageType != SHARED_MEM_MESSAGE_KFUZZ_RESPONSE) {
		return "ERROR: Received server response type was not expected";
	}

	return messageToString(response);
}

int GDBEmulator::handleConsoleCommands() {
	std::string command = "";

	while (command != "quit") {
		//Get next Command
		std::getline(std::cin, command);
		if (std::cin.fail() || std::cin.eof()) {
			std::cin.clear(); // reset cin state
		}

		if (command == "") {
			//skip empty commands
			continue;
		}
		else if (command == "fluffi") {
			//implement hack to know if all data has been read
			std::unique_lock<std::recursive_mutex> mlock(m_mutex_); //must wait for potential ctrl+c
			std::cout << "Undefined command" << std::endl;
			continue;
		}
		else if (command == "help") {
			std::cout << " Native Commands" << std::endl;
			std::cout << "  help" << std::endl;
			std::cout << "  quit" << std::endl;
			std::cout << "  fluffi" << std::endl;

			std::cout << " GDB emulation commands" << std::endl;
			std::cout << "  [Ctrl+C]" << std::endl;
			std::cout << "  c" << std::endl;
			std::cout << "  j" << std::endl;
			std::cout << "  i r" << std::endl;
			std::cout << "  br *0x0 - do nothing" << std::endl;
			std::cout << "  d br - do nothing" << std::endl;
			std::cout << "  set {unsigned char}" << std::endl;
			std::cout << "  set {unsigned short}" << std::endl;
			std::cout << "  set {unsigned int}" << std::endl;
			std::cout << "  set - do nothing" << std::endl;
			std::cout << "  info files" << std::endl;
			std::cout << "  info inferior" << std::endl;
			std::cout << "  x/1xb" << std::endl;
			std::cout << "  x/1xh" << std::endl;
			std::cout << "  x/1xw" << std::endl;
			continue;
		}
		else if (!m_fatalSystemErrorMessage.empty() && command == "c") {
			//A fatal system error occured do not forward the continue request. Instead, replay the fatal error message
			std::cout << m_fatalSystemErrorMessage << std::endl;
			continue;
		}
		else {
			//Forward command to WinDbg
			std::string respTranslated = sendCommandToWinDbgAndGetResponse(command);
			std::cout << respTranslated << std::endl;
			continue;
		}
	}

	std::cout << "Good bye!" << std::endl;
	return 0;
}

SharedMemMessage GDBEmulator::stringToMessage(std::string command) {
	SharedMemMessage request{ SHARED_MEM_MESSAGE_KFUZZ_REQUEST, command.c_str(),static_cast<int>(command.length()) };
	return request;
}

std::string  GDBEmulator::messageToString(const SharedMemMessage& message) {
	return std::string(message.getDataPointer(), message.getDataPointer() + message.getDataSize());
}

void GDBEmulator::winDbgMessageDispatcher() {
	SharedMemMessage message;
	SharedMemMessageType messageType;

	//Wait for commands from shared memory and process them
	while (!m_stopRequested)
	{
		//Wait for more commands
		m_subscriberIPC.waitForNewMessageToClient(&message, INFINITE, m_SharedMemIPCInterruptEvent);
		messageType = message.getMessageType();
		if (messageType == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
			continue;
		}
		else if (messageType != SHARED_MEM_MESSAGE_KFUZZ_REQUEST) {
			std::string errorMessage = "Received message was not expected";
			SharedMemMessage response{ SHARED_MEM_MESSAGE_ERROR, errorMessage.c_str(),static_cast<int>(errorMessage.length()) };
			m_subscriberIPC.sendMessageToServer(&response);
			continue;
		}
		std::string newMsg = messageToString(message);

		if (newMsg.find("FATAL_SYSTEM_ERROR") != std::string::npos) {
			//Store fatal system error message, so it can be replayed later
			m_fatalSystemErrorMessage = newMsg;
		}

		if (newMsg.find("Program received signal SIGTRAP") != std::string::npos) {
			//Check if we hit a ctrl+c induced breakpoint while not waiting for a forced break (happens if we request a break with ctrl+c but then hit another breakpoint)
			if (newMsg.find("(DbgBreakPoint)") != std::string::npos && !m_waitingForForcedBreak) {
				sendCommandToWinDbgAndGetResponse("c");
				newMsg = "Continuing obsolete debug break";
			}

			//The target is in breaked state - it is now safe to send more messages
			m_waitingForForcedBreak = false;
			m_mutex_.unlock();
		}
		std::cout << newMsg << std::endl;

		//We got a SHARED_MEM_MESSAGE_KFUZZ_REQUEST! Respond to it!
		SharedMemMessage response{ SHARED_MEM_MESSAGE_KFUZZ_RESPONSE,nullptr,0 };
		m_subscriberIPC.sendMessageToServer(&response);
	}
}

std::shared_ptr<GDBEmulator> GDBEmulator::getInstance() {
	if (instance == nullptr) {
		instance = std::make_shared<GDBEmulator>();
	}

	return instance;
}

BOOL WINAPI GDBEmulator::consoleHandler(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_C_EVENT) {
		std::shared_ptr<GDBEmulator> gdbEmu = GDBEmulator::getInstance();
		gdbEmu->m_mutex_.lock();
		gdbEmu->m_waitingForForcedBreak = true;
		gdbEmu->sendCommandToWinDbgAndGetResponse("ctrl+c");
		return TRUE;
	}

	return FALSE;
}

int main(int argc, char* argv[])
{
	if (argc == 2 && strcmp(argv[1], "--version") == 0) {
		std::cout << "GNU gdb emulator for kfuzz build on " << __DATE__ << " - " << __TIME__ << std::endl;
		return 0;
	}

	std::shared_ptr<GDBEmulator> gdbEmu = GDBEmulator::getInstance();

	if (!gdbEmu->init()) {
		return -1;
	}

	return gdbEmu->handleConsoleCommands();
}
