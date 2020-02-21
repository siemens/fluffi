/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

#pragma once
#include "KfuzzExtension.h"
#include "KfuzzEventCallback.h"

namespace Debugger::DataModel::Libraries::Kfuzz
{
	// KfuzzProvider:
	//
	// A class which provides the "kfuzz" set of functionality.  This is a singleton instance which encapsulates
	// all of the extension classes and factories for this part of the debugger extension.
	class KfuzzProvider
	{
	public:

		KfuzzProvider();
		~KfuzzProvider();

		static KfuzzProvider& Get();

		int GetNumOfProcessedCommands();

	private:

		static KfuzzProvider *s_pProvider;

		// Extensions managed by this provider
		std::unique_ptr<KfuzzExtension> m_spKfuzzExtension;
		std::unique_ptr<std::thread> m_commandDispatcher;
		bool m_stopRequested;
		int m_numOfProcessedCommands;
		SharedMemIPC m_responseIPC;
		HANDLE m_SharedMemIPCInterruptEvent;
		KfuzzEventCallback m_eventCallback;

		static SharedMemMessage processMessage(ComPtr<IDebugControl> dbgControl, ComPtr<IDebugRegisters> dbgRegisters, ComPtr<IDebugDataSpaces3> dbgData, ComPtr<IDebugSymbols> dbgSymbols, ComPtr<IDebugSystemObjects> dbgSystemObj, const SharedMemMessage& message);
		static bool ends_with(std::string const & value, std::string const & ending);
		static void handleSet(std::string command, std::stringstream& responseSS, ComPtr<IDebugDataSpaces3> dbgData);
		static void handleX(std::string command, std::stringstream& responseSS, ComPtr<IDebugDataSpaces3> dbgData);
		static void handleC(std::stringstream& responseSS, ComPtr<IDebugControl> dbgControl);
		static void handleIR(std::stringstream& responseSS, ComPtr<IDebugRegisters> dbgRegisters);
		static void handleJ(std::string command, std::stringstream& responseSS, ComPtr<IDebugControl> dbgControl, ComPtr<IDebugRegisters> dbgRegisters);
		static void handleCtrlC(std::stringstream& responseSS, ComPtr<IDebugControl>  dbgControl);
		static void handleInfo(std::string command, std::stringstream& responseSS, ComPtr<IDebugSymbols> dbgSymbols, ComPtr<IDebugSystemObjects> dbgSystemObj, ComPtr<IDebugDataSpaces3> dbgData);

		void commandDispatcher();
	};
}
