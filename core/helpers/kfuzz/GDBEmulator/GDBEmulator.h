/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

#pragma once

class GDBEmulator
{
public:
	GDBEmulator();
	virtual ~GDBEmulator();

	bool init();
	int handleConsoleCommands();

	static std::shared_ptr<GDBEmulator>  getInstance();

private:

	SharedMemMessage stringToMessage(std::string command);
	std::string  messageToString(const SharedMemMessage& message);
	std::string sendCommandToWinDbgAndGetResponse(std::string command);
	void winDbgMessageDispatcher();

	static BOOL WINAPI consoleHandler(DWORD dwCtrlType);

	SharedMemIPC m_requestIPC;
	SharedMemIPC m_subscriberIPC;
	std::mutex m_mutex_;
	std::unique_ptr<std::thread> m_WinDbgMessageDispatcher;
	bool m_stopRequested;
	HANDLE m_SharedMemIPCInterruptEvent;

	static std::shared_ptr<GDBEmulator> instance;
};
