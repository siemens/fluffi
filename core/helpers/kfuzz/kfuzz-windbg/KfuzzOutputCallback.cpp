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
#include "KfuzzOutputCallback.h"
#include "KfuzzPublisher.h"

namespace Debugger::DataModel::Libraries::Kfuzz
{
	KfuzzOutputCallback::KfuzzOutputCallback(KfuzzPublisher * publisher) :
		m_cRef(0),
		m_publisher(publisher)
	{
	}

	KfuzzOutputCallback::~KfuzzOutputCallback() {
	}

	ULONG STDMETHODCALLTYPE KfuzzOutputCallback::AddRef(void) {
		InterlockedIncrement(&m_cRef);
		return m_cRef;
	}
	ULONG STDMETHODCALLTYPE KfuzzOutputCallback::Release(void) {
		// Decrement the object's internal counter.
		ULONG ulRefCount = InterlockedDecrement(&m_cRef);
		if (0 == m_cRef)
		{
			delete this;
		}
		return ulRefCount;
	}

	HRESULT STDMETHODCALLTYPE KfuzzOutputCallback::QueryInterface(const IID & InterfaceId, PVOID * Interface)
	{
		*Interface = NULL;

#if _MSC_VER >= 1100
		if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
			IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks2)))
#else
		if (IsEqualIID(InterfaceId, IID_IUnknown) ||
			IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks2))
#endif
		{
			*Interface = (IDebugOutputCallbacks2 *)this;
			AddRef();
			return S_OK;
		}
		else
		{
			return E_NOINTERFACE;
		}
	}

	HRESULT STDMETHODCALLTYPE KfuzzOutputCallback::GetInterestMask(PULONG Mask) {
		*Mask = DEBUG_OUTCBI_DML;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE KfuzzOutputCallback::Output(ULONG Mask, PCSTR Text) {
		//	This method is not used.
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE KfuzzOutputCallback::Output2(ULONG Which, ULONG Flags, ULONG64 Arg, PCWSTR Text) {
		std::wstring wstrText(Text);
		if (wstrText.find(L"*** Fatal System Error:") != std::string::npos) {
			std::stringstream whatHappened;
			whatHappened << "Program received signal SIGSEGV, Segmentation fault." << std::endl;
			size_t firstComma = wstrText.find(L",");
			if (firstComma != std::string::npos) {
				size_t secondComma = wstrText.find(L",", firstComma + 1);
				if (secondComma != std::string::npos) {
					try {
						unsigned long long crashaddress = std::stoull(wstrText.substr(secondComma + 1), 0, 16);
						whatHappened << "0x" << std::hex << std::setw(16) << std::setfill('0') << crashaddress << " in FATAL_SYSTEM_ERROR ()";
					}
					catch (...) {
						std::wstring ws = wstrText.substr(secondComma + 1);
						whatHappened << "failed to parse input: stoull of \"" << std::string((char *)ws.c_str()) << "\" at position " << secondComma << " failed";
					}
				}
				else {
					whatHappened << "failed to parse input: second comma not found";
				}
			}
			else {
				whatHappened << "failed to parse input: first comma not found";
			}

			m_publisher->publish(whatHappened.str());
		}
		return S_OK;
	}
}