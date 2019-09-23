/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once

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

enum SharedMemMessageType {
	SHARED_MEM_MESSAGE_TERMINATE = 0x1,
	SHARED_MEM_MESSAGE_FUZZ_FILENAME = 0x2,
	SHARED_MEM_MESSAGE_FUZZ_DONE = 0x3,

	SHARED_MEM_MESSAGE_TARGET_PID = 0x5,
	SHARED_MEM_MESSAGE_READY_TO_FUZZ = 0x6,

	SHARED_MEM_MESSAGE_TARGET_CRASHED = 0x10,

	SHARED_MEM_MESSAGE_NOT_INITIALIZED = 0x100,
	SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT = 0x101,
	SHARED_MEM_MESSAGE_FUZZ_ERROR = 0x102,
	SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE = 0x103,
	SHARED_MEM_MESSAGE_WAIT_INTERRUPTED = 0x104,
	SHARED_MEM_MESSAGE_TRANSMISSION_ERROR = 0x105
};

class SHAREDMEMIPC_API  SharedMemMessage
{
public:

§§	SharedMemMessage(SharedMemMessageType messageType, const char* data, int dataLength);
	SharedMemMessage();
	virtual ~SharedMemMessage();

§§	const char* getDataPointer() const;
	int getDataSize() const;
§§	void replaceDataWith(const char* data, int dataLength);

	SharedMemMessageType getMessageType() const;
	void setMessageType(SharedMemMessageType messageType);

private:
	SharedMemMessageType m_messageType;
	int m_data_size;
§§	char* m_data;

#if defined(_WIN32) || defined(_WIN64)
#else
§§	int memcpy_s(void* a, size_t  b, const void* c, size_t d);
#endif
};

/*
Please note that if you ever want to use this code, e.g. from python, you need to use this c wrapper:
http://www.auctoris.co.uk/2017/04/29/calling-c-classes-from-python-with-ctypes/*/
extern "C"
{
§§	SHAREDMEMIPC_API SharedMemMessage* SharedMemMessage_new1(SharedMemMessageType messageType, const char* data, int dataLength);
	SHAREDMEMIPC_API SharedMemMessage* SharedMemMessage_new2();
	SHAREDMEMIPC_API void SharedMemMessage_delete(SharedMemMessage* thisp);

§§	SHAREDMEMIPC_API const char* SharedMemMessage_getDataPointer(SharedMemMessage* thisp);
	SHAREDMEMIPC_API int SharedMemMessage_getDataSize(SharedMemMessage* thisp);
§§	SHAREDMEMIPC_API void SharedMemMessage_replaceDataWith(SharedMemMessage* thisp, const char* data, int dataLength);

	SHAREDMEMIPC_API SharedMemMessageType SharedMemMessage_getMessageType(SharedMemMessage* thisp);
	SHAREDMEMIPC_API void SharedMemMessage_setMessageType(SharedMemMessage* thisp, SharedMemMessageType messageType);
}
