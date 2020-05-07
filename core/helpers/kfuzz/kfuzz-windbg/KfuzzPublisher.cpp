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
#include "KfuzzPublisher.h"

namespace Debugger::DataModel::Libraries::Kfuzz
{
	KfuzzPublisher::KfuzzPublisher() :
		m_mutex_(),
		m_publisherIPC("kfuzz_windbg_publish_subscribe")
	{
		m_publisherIPC.initializeAsServer();
	}

	KfuzzPublisher::~KfuzzPublisher() {
	}

	SharedMemMessage KfuzzPublisher::stringToMessage(std::string command) {
		SharedMemMessage request{ SHARED_MEM_MESSAGE_KFUZZ_REQUEST, command.c_str(),static_cast<int>(command.length()) };
		return request;
	}

	bool KfuzzPublisher::publish(std::string command) {
		//Make sure only one thread at a time uses the shared memory IPC
		std::unique_lock<std::mutex> mlock(m_mutex_);

		int timeoutMilliseconds = 1000;
		SharedMemMessage response;
		SharedMemMessage request = stringToMessage(command);
		bool success = m_publisherIPC.sendMessageToClient(&request);

		if (success) {
			success = m_publisherIPC.waitForNewMessageToServer(&response, timeoutMilliseconds);
		}
		return success;
	}
}