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
#include "KfuzzProvider.h"

namespace Debugger::DataModel::Libraries::Kfuzz
{
	KfuzzProvider *KfuzzProvider::s_pProvider = nullptr;

	// KfuzzProvider():
	//
	// The constructor for the Kfuzz provider which instantiates the necessary classes to extend the debugger
	// through the data model.
	//
	KfuzzProvider::KfuzzProvider() :
		m_stopRequested(false),
		m_numOfProcessedCommands(-1),
		m_responseIPC("kfuzz_windbg_request_response", 1 * 1024 * 1024),
		m_SharedMemIPCInterruptEvent(NULL),
		m_eventCallback()
	{
		m_spKfuzzExtension = std::make_unique<KfuzzExtension>();

		//Initialize the publish-subscribe part
		IDebugHost * dbgHost = Debugger::DataModel::ClientEx::GetHost();
		ComPtr<IUnknown> spPrivate;
		if (SUCCEEDED(dbgHost->GetHostDefinedInterface(&spPrivate))) {
			ComPtr<IDebugClient> dbgClient;
			if (SUCCEEDED(spPrivate.As(&dbgClient)))
			{
				dbgClient->SetEventCallbacks(&m_eventCallback);
			}
		}

		//Initialize the request-response part
		m_SharedMemIPCInterruptEvent = CreateEvent(NULL, false, false, NULL);
		if (m_SharedMemIPCInterruptEvent != NULL && m_responseIPC.initializeAsServer()) {
			m_commandDispatcher = std::make_unique<std::thread>(&KfuzzProvider::commandDispatcher, this);
		}
		s_pProvider = this;
	}

	KfuzzProvider::~KfuzzProvider()
	{
		m_stopRequested = true;
		SetEvent(m_SharedMemIPCInterruptEvent);

		m_commandDispatcher->join();

		if (m_SharedMemIPCInterruptEvent != NULL) {
			CloseHandle(m_SharedMemIPCInterruptEvent);
			m_SharedMemIPCInterruptEvent = NULL;
		}

		s_pProvider = nullptr;
	}

	KfuzzProvider& KfuzzProvider::Get()
	{
		return *s_pProvider;
	}

	int KfuzzProvider::GetNumOfProcessedCommands() {
		return m_numOfProcessedCommands;
	}

	void KfuzzProvider::commandDispatcher() {
		//Get the dbg host's controler interface
		IDebugHost * dbgHost = Debugger::DataModel::ClientEx::GetHost();
		ComPtr<IUnknown> spPrivate;
		if (SUCCEEDED(dbgHost->GetHostDefinedInterface(&spPrivate)))
		{
			ComPtr<IDebugControl> dbgControl;
			if (SUCCEEDED(spPrivate.As(&dbgControl)))
			{
				ComPtr<IDebugRegisters> dbgRegisters;
				if (SUCCEEDED(spPrivate.As(&dbgRegisters)))
				{
					ComPtr<IDebugDataSpaces3> dbgData;
					if (SUCCEEDED(spPrivate.As(&dbgData)))
					{
						ComPtr<IDebugSymbols> dbgSymbols;
						if (SUCCEEDED(spPrivate.As(&dbgSymbols)))
						{
							ComPtr<IDebugSystemObjects> dbgSystemObj;
							if (SUCCEEDED(spPrivate.As(&dbgSystemObj)))
							{
								m_numOfProcessedCommands = 0;
								SharedMemMessage message;
								SharedMemMessageType messageType;

								//Wait for commands from shared memory and process them
								while (!m_stopRequested)
								{
									//Wait for more commands
									m_responseIPC.waitForNewMessageToServer(&message, INFINITE, m_SharedMemIPCInterruptEvent);
									messageType = message.getMessageType();
									if (messageType == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
										continue;
									}
									else if (messageType != SHARED_MEM_MESSAGE_KFUZZ_REQUEST) {
										std::string errorMessage = "Received message was not expected";
										SharedMemMessage response{ SHARED_MEM_MESSAGE_ERROR, errorMessage.c_str(),static_cast<int>(errorMessage.length()) };
										m_responseIPC.sendMessageToClient(&response);
										continue;
									}

									//We got a SHARED_MEM_MESSAGE_KFUZZ_REQUEST! Process it!
									m_numOfProcessedCommands++;
									SharedMemMessage response = processMessage(dbgControl, dbgRegisters, dbgData, dbgSymbols, dbgSystemObj, message);
									m_responseIPC.sendMessageToClient(&response);
								}
							}
						}
					}
				}
			}
		}
	}

	SharedMemMessage KfuzzProvider::processMessage(ComPtr<IDebugControl> dbgControl, ComPtr<IDebugRegisters> dbgRegisters, ComPtr<IDebugDataSpaces3> dbgData, ComPtr<IDebugSymbols> dbgSymbols, ComPtr<IDebugSystemObjects> dbgSystemObj, const SharedMemMessage& message) {
		std::string command(message.getDataPointer(), message.getDataPointer() + message.getDataSize());
		std::stringstream responseSS;

		if (command.rfind("set {", 0) == 0) {
			handleSet(command, responseSS, dbgData);
		}
		else if (command.rfind("x/1", 0) == 0) {
			handleX(command, responseSS, dbgData);
		}
		else if (command.rfind("j ", 0) == 0) {
			handleJ(command, responseSS, dbgControl, dbgRegisters);
		}
		else if (command == "c") {
			handleC(responseSS, dbgControl);
		}
		else if (command == "ctrl+c") {
			handleCtrlC(responseSS, dbgControl);
		}
		else if (command == "i r") {
			handleIR(responseSS, dbgRegisters);
		}
		else if (command.rfind("info ", 0) == 0) {
			handleInfo(command, responseSS, dbgSymbols, dbgSystemObj, dbgData);
		}
		else if (command == "br *0x0" || command == "set" || command.rfind("d br ", 0) == 0) {
			//These are commands, that are GDB specific and can be safely ignored
			responseSS << "Command \"" << command << "\" will be ignored, as this is only a hack to handle GDB quirks.";
		}
		else {
			responseSS << "ERROR! Command \"" << command << "\" is not implemented";
		}

		std::string responseMessage = responseSS.str();
		SharedMemMessage response{ SHARED_MEM_MESSAGE_KFUZZ_RESPONSE, responseMessage.c_str(),static_cast<int>(responseMessage.length()) };
		return response;
	}

	void KfuzzProvider::handleSet(std::string command, std::stringstream& responseSS, ComPtr<IDebugDataSpaces3> dbgData) {
		try {
			size_t indexOfFirstX = command.find('x');
			if (indexOfFirstX == std::string::npos) {
				responseSS << "ERROR! Command \"" << command << "\" is not implemented, as a destination address could not be identified";
				return;
			}
			size_t indexOfSecondX = command.find('x', indexOfFirstX + 1);
			if (indexOfSecondX == std::string::npos) {
				responseSS << "ERROR! Command \"" << command << "\" is not implemented, as a value to write could not be identified";
				return;
			}

			std::string addressString = command.substr(indexOfFirstX - 1);
			unsigned long long address = std::stoull(addressString, nullptr, 16);
			std::string valueString = command.substr(indexOfSecondX - 1);
			unsigned long long value = std::stoull(valueString, nullptr, 16);

			char width = ' ';
			if (command.find("char") != std::string::npos) {
				width = 'b';
			}
			else if (command.find("short") != std::string::npos) {
				width = 'h';
			}
			else if (command.find("int") != std::string::npos) {
				width = 'w';
			}

			unsigned long bytesWritten = 0;
			HRESULT res = E_UNEXPECTED;
			while (res != S_OK) {
				switch (width) {
				case'b':
				{
					res = dbgData->WriteVirtualUncached(address, &value, 1, &bytesWritten);
					res |= dbgData->WriteVirtual(address, &value, 1, &bytesWritten);
					break;
				}
				case'h':
				{
					res = dbgData->WriteVirtualUncached(address, &value, 2, &bytesWritten);
					res |= dbgData->WriteVirtual(address, &value, 2, &bytesWritten);
					break;
				}
				case'w':
				{
					res = dbgData->WriteVirtualUncached(address, &value, 4, &bytesWritten);
					res |= dbgData->WriteVirtual(address, &value, 4, &bytesWritten);
					break;
				}
				default:
				{
					responseSS << "ERROR! Command \"" << command << "\" is not implemented, as the integer size is unknown";
					break;
				}
				}
			}
		}
		catch (...) {
			responseSS << "ERROR! Could not parse \"" << command << "\"";
		}
	}
	void KfuzzProvider::handleX(std::string command, std::stringstream& responseSS, ComPtr<IDebugDataSpaces3> dbgData) {
		try {
			std::string addressString = command.substr(command.find('0'));
			unsigned long long address = std::stoull(addressString, nullptr, 16);
			unsigned long bytesRead = 0;
			switch (command[4]) {
			case'b':
			{
				uint8_t val = 0;
				dbgData->ReadVirtualUncached(address, &val, sizeof(val), &bytesRead);
				responseSS << addressString << ":   0x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)val;
				break;
			}
			case'h':
			{
				uint16_t val = 0;
				dbgData->ReadVirtualUncached(address, &val, sizeof(val), &bytesRead);
				responseSS << addressString << ":   0x" << std::hex << std::setw(4) << std::setfill('0') << val;
				break;
			}
			case'w':
			{
				uint32_t val = 0;
				dbgData->ReadVirtualUncached(address, &val, sizeof(val), &bytesRead);
				responseSS << addressString << ":   0x" << std::hex << std::setw(8) << std::setfill('0') << val;
				break;
			}
			default:
			{
				responseSS << "ERROR! Command \"" << command << "\" is not implemented, as the integer size is unknown";
				break;
			}
			}
		}
		catch (...) {
			responseSS << "ERROR! Could not parse \"" << command << "\"";
		}
	}

	void KfuzzProvider::handleC(std::stringstream& responseSS, ComPtr<IDebugControl> dbgControl) {
		dbgControl->SetExecutionStatus(DEBUG_STATUS_GO);
		responseSS << "Continuing.";
	}
	void KfuzzProvider::handleIR(std::stringstream& responseSS, ComPtr<IDebugRegisters> dbgRegisters) {
		unsigned long numOfRegisters;
		HRESULT  success = dbgRegisters->GetNumberRegisters(&numOfRegisters);
		char nameBuffer[0x101];
		DEBUG_REGISTER_DESCRIPTION desc;
		DEBUG_VALUE val;
		for (unsigned long i = 0; i < numOfRegisters; i++) {
			dbgRegisters->GetDescription(i, nameBuffer, sizeof(nameBuffer) - 1, NULL, &desc);
			dbgRegisters->GetValue(i, &val);
			if (i != 0) {
				responseSS << std::endl;
			}

			//This is not 100% GDB's output. But it is close enough for what we need
			responseSS << nameBuffer << std::string(15 - strlen(nameBuffer), ' ') << "0x" << std::hex << val.I64 << "   " << std::dec << val.I64;

			//gdb breaks at gs
			if (strcmp(nameBuffer, "gs") == 0) {
				numOfRegisters = i;
			}
		}
	}

	void KfuzzProvider::handleJ(std::string command, std::stringstream& responseSS, ComPtr<IDebugControl> dbgControl, ComPtr<IDebugRegisters> dbgRegisters) {
		try {
			std::string addressString = command.substr(command.find('0'));
			unsigned long long address = std::stoull(addressString, nullptr, 16);
			DEBUG_VALUE addressVal;
			addressVal.Type = DEBUG_VALUE_INT64;
			addressVal.I64 = address;
			unsigned long bytesRead = 0;

			unsigned long ripIndex = 0;
			if (!SUCCEEDED(dbgRegisters->GetIndexByName("rip", &ripIndex))) {
				responseSS << "ERROR! Could not identify the RIP register \"" << command << "\"";
				return;
			}
			dbgRegisters->SetValue(ripIndex, &addressVal);

			dbgControl->SetExecutionStatus(DEBUG_STATUS_GO);
			responseSS << "Continuing at " << addressString << ".";
		}
		catch (...) {
			responseSS << "ERROR! Could not parse \"" << command << "\"";
		}
	}

	void KfuzzProvider::handleCtrlC(std::stringstream& responseSS, ComPtr<IDebugControl>  dbgControl) {
		dbgControl->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
	}

	bool KfuzzProvider::ends_with(std::string const & value, std::string const & ending)
	{
		if (ending.size() > value.size()) return false;
		return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
	}

	void KfuzzProvider::handleInfo(std::string command, std::stringstream& responseSS, ComPtr<IDebugSymbols> dbgSymbols, ComPtr<IDebugSystemObjects> dbgSystemObj, ComPtr<IDebugDataSpaces3> dbgData) {
		char nameBuffer[MAX_PATH + 1];

		if (command == "info inferior") {
			responseSS << "  Num  Description       Executable" << std::endl;

			dbgSymbols->GetModuleNames(0, NULL, nameBuffer, sizeof(nameBuffer) - 1, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			unsigned long systemID = 0;
			if (!ends_with(nameBuffer, ".sys")) {
				if (S_OK != dbgSystemObj->GetProcessIdsByIndex(0, 1, NULL, &systemID)) {
					systemID = 0;
				}
			}
			responseSS << "* 1    process " << systemID << "     " << nameBuffer;
			return;
		}
		else if (command == "info files") {
			responseSS << "Local exec file:" << std::endl;

			//Force symbol reload (required if attatching to a kernel)
			dbgSymbols->Reload("");

			//How many modules are there?
			unsigned long loadedModules;
			unsigned long unloadedModules;
			dbgSymbols->GetNumberModules(&loadedModules, &unloadedModules);

			DEBUG_MODULE_PARAMETERS modparam;
			IMAGE_NT_HEADERS64 ntheaders;
			IMAGE_SECTION_HEADER shead;
			IMAGE_DOS_HEADER dosheaders;
			unsigned long bytesRead;
			for (unsigned int i = 0; i < loadedModules; i++) {
				if (i != 0) {
					responseSS << std::endl;
				}

				dbgSymbols->GetModuleNames(i, NULL, nameBuffer, sizeof(nameBuffer) - 1, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
				dbgSymbols->GetModuleParameters(1, 0, i, &modparam);
				responseSS << "        0x" << std::hex << std::setw(16) << std::setfill('0') << modparam.Base << " - 0x" << std::setw(16) << (modparam.Base + modparam.Size) << " is .base    in " << nameBuffer;

				//Read DOS Headers of image
				dbgData->ReadVirtualUncached(modparam.Base, &dosheaders, sizeof(dosheaders), &bytesRead);

				//Read NT Headers of Image
				dbgData->ReadImageNtHeaders(modparam.Base, &ntheaders);

				UINT_PTR firstSectionAddress = 0;
				if (ntheaders.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
					firstSectionAddress = static_cast<UINT_PTR>(modparam.Base) + dosheaders.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader) + ntheaders.FileHeader.SizeOfOptionalHeader;
				}
				else {
					firstSectionAddress = static_cast<UINT_PTR>(modparam.Base) + dosheaders.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader) + ntheaders.FileHeader.SizeOfOptionalHeader;
				}

				for (int s = 0; s < ntheaders.FileHeader.NumberOfSections; s++) {
					dbgData->ReadVirtualUncached(firstSectionAddress + (sizeof(shead)*s), &shead, sizeof(shead), &bytesRead);
					std::string segnameString(shead.Name, shead.Name + 8);
					std::replace(segnameString.begin(), segnameString.end(), '\0', ' ');
					responseSS << std::endl << "        0x" << std::hex << std::setw(16) << std::setfill('0') << (modparam.Base + shead.VirtualAddress) << " - 0x" << std::setw(16) << (modparam.Base + shead.VirtualAddress + shead.Misc.VirtualSize) << " is " << segnameString << " in " << nameBuffer;
				}
			}
			return;
		}
		else {
			responseSS << "info command \"" << command << "\" is not implemented";
			return;
		}
	}
}
