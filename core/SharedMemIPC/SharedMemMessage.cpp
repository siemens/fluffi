/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "SharedMemMessage.h"

SharedMemMessage::SharedMemMessage(SharedMemMessageType messageType, const char* data, int dataLength) :
	m_messageType(messageType),
	m_data_size(dataLength),
	m_data(new char[m_data_size])
{
	memcpy_s(m_data, m_data_size, data, dataLength);
}

SharedMemMessage::SharedMemMessage() :
	m_messageType(SHARED_MEM_MESSAGE_NOT_INITIALIZED),
	m_data_size(0),
	m_data(new char[0])
{
}

SharedMemMessage::SharedMemMessage(const SharedMemMessage &other) :
	m_messageType(other.m_messageType),
	m_data_size(other.m_data_size),
	m_data(new char[m_data_size])
{
	memcpy_s(m_data, m_data_size, other.m_data, m_data_size);
}

SharedMemMessage::~SharedMemMessage()
{
	delete[] m_data;
}

const char* SharedMemMessage::getDataPointer()  const {
	if (m_data_size == 0) {
		return nullptr;
	}
	return m_data;
}

int SharedMemMessage::getDataSize() const {
	return m_data_size;
}

void SharedMemMessage::replaceDataWith(const char* data, int dataLength) {
	delete[] m_data;
	m_data_size = dataLength;
	m_data = new char[m_data_size];
	memcpy_s(m_data, m_data_size, data, dataLength);
}

SharedMemMessageType SharedMemMessage::getMessageType() const {
	return m_messageType;
}

void SharedMemMessage::setMessageType(SharedMemMessageType messageType) {
	m_messageType = messageType;
}

#if defined(_WIN32) || defined(_WIN64)
#else
int SharedMemMessage::memcpy_s(void* a, size_t  b, const void* c, size_t d) {
	return ((memcpy(a, c, std::min(b, d)) == a) ? 0 : -1);
}
#endif

//C wrapper
SharedMemMessage* SharedMemMessage_new1(SharedMemMessageType messageType, const char* data, int dataLength) { return new SharedMemMessage(messageType, data, dataLength); }
SharedMemMessage* SharedMemMessage_new2() { return new SharedMemMessage(); }
void SharedMemMessage_delete(SharedMemMessage* thisp) { delete thisp; }

const char* SharedMemMessage_getDataPointer(SharedMemMessage* thisp) { return thisp->getDataPointer(); }
int SharedMemMessage_getDataSize(SharedMemMessage* thisp) { return thisp->getDataSize(); }
void SharedMemMessage_replaceDataWith(SharedMemMessage* thisp, const char* data, int dataLength) { thisp->replaceDataWith(data, dataLength); }

SharedMemMessageType SharedMemMessage_getMessageType(SharedMemMessage* thisp) { return thisp->getMessageType(); }
void SharedMemMessage_setMessageType(SharedMemMessage* thisp, SharedMemMessageType messageType) { thisp->setMessageType(messageType); };
