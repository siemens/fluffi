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

#pragma once
namespace Debugger::DataModel::Libraries::Kfuzz
{
	// KfuzzOutputCallback:
	//
	// A class to handle Outputs sent by the debugging engine
	class KfuzzPublisher;
	class KfuzzOutputCallback : public  IDebugOutputCallbacks2
	{
	public:
		KfuzzOutputCallback(KfuzzPublisher * publisher);
		virtual ~KfuzzOutputCallback();

		ULONG STDMETHODCALLTYPE AddRef(void);
		HRESULT STDMETHODCALLTYPE GetInterestMask(PULONG Mask);
		HRESULT STDMETHODCALLTYPE Output(ULONG Mask, PCSTR Text);
		HRESULT STDMETHODCALLTYPE Output2(ULONG Which, ULONG Flags, ULONG64 Arg, PCWSTR Text);
		HRESULT STDMETHODCALLTYPE QueryInterface(const IID & InterfaceId, PVOID * Interface);
		ULONG STDMETHODCALLTYPE Release(void);

	private:
		volatile ULONG m_cRef;

		KfuzzPublisher * m_publisher;
	};
}
