/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

#include "stdafx.h"
#include "KfuzzEventCallback.h"

namespace Debugger::DataModel::Libraries::Kfuzz
{
	KfuzzEventCallback::KfuzzEventCallback() :
		m_cRef(0),
		m_publisherIPC("kfuzz_windbg_publish_subscribe")
	{
		m_publisherIPC.initializeAsServer();
	}
	KfuzzEventCallback::~KfuzzEventCallback() {
	}

	ULONG STDMETHODCALLTYPE KfuzzEventCallback::AddRef(void) {
		InterlockedIncrement(&m_cRef);
		return m_cRef;
	}
	ULONG STDMETHODCALLTYPE KfuzzEventCallback::Release(void) {
		// Decrement the object's internal counter.
		ULONG ulRefCount = InterlockedDecrement(&m_cRef);
		if (0 == m_cRef)
		{
			delete this;
		}
		return ulRefCount;
	}
	HRESULT STDMETHODCALLTYPE KfuzzEventCallback::GetInterestMask(PULONG Mask) {
		*Mask = DEBUG_EVENT_EXCEPTION | DEBUG_EVENT_EXIT_PROCESS;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE KfuzzEventCallback::ExitProcess(ULONG ExitCode) {
		std::stringstream whatHappened;
		if (ExitCode == 0) {
			whatHappened << "Program exited normally.";
		}
		else {
			whatHappened << "program exited with code " << ExitCode << ".";
		}
		int timeoutMilliseconds = 1000;
		SharedMemMessage response;
		SharedMemMessage request = stringToMessage(whatHappened.str());
		m_publisherIPC.sendMessageToClient(&request);
		m_publisherIPC.waitForNewMessageToServer(&response, timeoutMilliseconds);

		return DEBUG_STATUS_NO_CHANGE;
	}

	HRESULT STDMETHODCALLTYPE KfuzzEventCallback::Exception(PEXCEPTION_RECORD64 Exception, ULONG FirstChance) {
		std::stringstream whatHappened;
		if (Exception->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
			whatHappened << "Program received signal SIGSEGV, Segmentation fault." << std::endl;
		}
		else {
			//It's not necessarily true, that we received a SIGTRAP. However, for FLUFFI compatibility, this is enough.
			whatHappened << "Program received signal SIGTRAP, Trace/breakpoint trap." << std::endl;
		}
		whatHappened << "0x" << std::hex << std::setw(16) << std::setfill('0') << Exception->ExceptionAddress << " in ?? ()";
		int timeoutMilliseconds = 1000;
		SharedMemMessage response;
		SharedMemMessage request = stringToMessage(whatHappened.str());
		m_publisherIPC.sendMessageToClient(&request);
		m_publisherIPC.waitForNewMessageToServer(&response, timeoutMilliseconds);

		return DEBUG_STATUS_NO_CHANGE;
	}

	SharedMemMessage KfuzzEventCallback::stringToMessage(std::string command) {
		SharedMemMessage request{ SHARED_MEM_MESSAGE_KFUZZ_REQUEST, command.c_str(),static_cast<int>(command.length()) };
		return request;
	}
}
