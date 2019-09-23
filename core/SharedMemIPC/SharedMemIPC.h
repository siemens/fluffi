/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once
#include "SharedMemMessage.h"

#ifndef SHAREDMEMIPC_API

#if defined(_WIN32) || defined(_WIN64)

#ifdef SHAREDMEMIPC_EXPORTS
#define SHAREDMEMIPC_API __declspec(dllexport)
#else
#define SHAREDMEMIPC_API __declspec(dllimport)
#endif

#else
#define SHAREDMEMIPC_API __attribute__ ((visibility ("default")))
#endif

#endif

//Linking on linux: -lsharedmemipc -luuid -lrt

class SHAREDMEMIPC_API SharedMemIPC {
public:
	SharedMemIPC(const char* sharedMemName, int sharedMemorySize = 0x1000);
	~SharedMemIPC();

	bool initializeAsServer();
	bool initializeAsClient();

	bool sendMessageToServer(const SharedMemMessage* message);
	bool sendMessageToClient(const SharedMemMessage* message);

#if defined(_WIN32) || defined(_WIN64)
	bool waitForNewMessageToClient(SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds, HANDLE interruptEventHandle);
	bool waitForNewMessageToServer(SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds, HANDLE interruptEventHandle);
#else
	bool waitForNewMessageToClient(SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds, int interruptFD);
	bool waitForNewMessageToServer(SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds, int interruptFD);
#endif

	bool waitForNewMessageToClient(SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds);
	bool waitForNewMessageToServer(SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds);

private:
#if defined(_WIN32) || defined(_WIN64)
	HANDLE m_hMapFile;
	char* m_pView;
	HANDLE m_hEventForNewMessageForClient;
	HANDLE m_hEventForNewMessageForServer;
#else
	int m_fdSharedMem;
	char* m_pView;
	int m_fdFIFOForNewMessageForClient;
	int m_fdFIFOForNewMessageForServer;
#endif

	std::string* m_sharedMemName;
	int m_sharedMemorySize;

#if defined(_WIN32) || defined(_WIN64)
#else
	int memcpy_s(void* a, size_t  b, const void* c, size_t d);
#endif

	bool notifyClientAboutNewMessage();
	bool notifyServerAboutNewMessage();

	bool placeMessageForClient(const SharedMemMessage*  message);
	bool placeMessageForServer(const SharedMemMessage* message);

	static std::string newGUID();
};

/*
Please note that if you ever want to use this code, e.g. from python, you need to use this c wrapper:
http://www.auctoris.co.uk/2017/04/29/calling-c-classes-from-python-with-ctypes/*/
extern "C"
{
	SHAREDMEMIPC_API SharedMemIPC* SharedMemIPC_new(const char* sharedMemName, int sharedMemorySize = 0x1000);
	SHAREDMEMIPC_API void SharedMemIPC_delete(SharedMemIPC* thisp);
	SHAREDMEMIPC_API bool SharedMemIPC_initializeAsServer(SharedMemIPC* thisp);
	SHAREDMEMIPC_API bool SharedMemIPC_initializeAsClient(SharedMemIPC* thisp);

	SHAREDMEMIPC_API bool SharedMemIPC_sendMessageToServer(SharedMemIPC* thisp, const SharedMemMessage* message);
	SHAREDMEMIPC_API bool SharedMemIPC_sendMessageToClient(SharedMemIPC* thisp, const SharedMemMessage* message);

	SHAREDMEMIPC_API bool SharedMemIPC_waitForNewMessageToClient(SharedMemIPC* thisp, SharedMemMessage* messagePointer, unsigned long timeoutMilliseconds);
	SHAREDMEMIPC_API bool SharedMemIPC_waitForNewMessageToServer(SharedMemIPC* thisp, SharedMemMessage* messagePointer, unsigned long timeoutMilliseconds);
}
