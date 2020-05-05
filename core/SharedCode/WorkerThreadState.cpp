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

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "WorkerThreadState.h"

WorkerThreadState::WorkerThreadState() :
	m_stopRequested(false),
	m_resetClock(std::chrono::steady_clock::now()),
	m_zeroMQSockets(new std::map<std::pair<std::string, int>, zmq::socket_t*>())
{
}

WorkerThreadState::~WorkerThreadState()
{
	clearSocketCache();

	delete m_zeroMQSockets;
	m_zeroMQSockets = nullptr;
}

void  WorkerThreadState::clearSocketFor(const std::string targetHAP, int timeoutMS) {
	std::pair<std::string, int> socketIdentifier(targetHAP, timeoutMS);
	auto socketCacheEntry = m_zeroMQSockets->find(socketIdentifier);
	if (socketCacheEntry != m_zeroMQSockets->end()) {
		//Delete entry from socket cache
		if (socketCacheEntry->second != nullptr) {
			socketCacheEntry->second->close();
			delete socketCacheEntry->second;
		}
		m_zeroMQSockets->erase(socketCacheEntry);
	}
}

zmq::socket_t* WorkerThreadState::getSocketFor(zmq::context_t* zeroMQContext, const std::string targetHAP, int timeoutMS) {
	//reset the socket cache every 10 minutes socket cache to get rid of no longer used connections
	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_resetClock).count() > 1000 * 10 * 60) {
		LOG(DEBUG) << "Resetting the socket cache!";
		clearSocketCache();
		m_resetClock = std::chrono::steady_clock::now();
	}

	zmq::socket_t* sock = nullptr;
	std::pair<std::string, int> socketIdentifier(targetHAP, timeoutMS);
	auto socketCacheEntry = m_zeroMQSockets->find(socketIdentifier);
	if (socketCacheEntry != m_zeroMQSockets->end()) {
		//we already have a socket for our target
		sock = socketCacheEntry->second;
	}
	else {
		//there is not yet a socket for our target
		sock = new zmq::socket_t(*zeroMQContext, ZMQ_REQ); //ZMQ_REQ<->ZMQ_DEALER
		sock->setsockopt(ZMQ_RCVTIMEO, timeoutMS);
		sock->setsockopt(ZMQ_SNDTIMEO, timeoutMS);
		sock->setsockopt(ZMQ_LINGER, 1000);
		sock->setsockopt(ZMQ_SNDHWM, 0); //Set the send and receive high water marks to unlimeted. This means memory usage might go -> inf if the workers cannot handle the load
		sock->setsockopt(ZMQ_RCVHWM, 0); //Set the send and receive high water marks to unlimeted. This means memory usage might go -> inf if the workers cannot handle the load
		sock->connect(std::string("tcp://") + targetHAP);

		auto insertionResponse = m_zeroMQSockets->insert(std::make_pair(socketIdentifier, sock));
		if (!insertionResponse.second) {
			LOG(ERROR) << "The socket we just tried to insert to the workerThreadState->m_zeroMQSockets structure already existed. THIS SHOULD NEVER HAPPEN!";
		}
	}

	return sock;
}

void WorkerThreadState::clearSocketCache() {
	for (auto it = m_zeroMQSockets->begin(); it != m_zeroMQSockets->end(); it++) {
		if (it->second != nullptr) {
			it->second->close();
			delete it->second;
		}
	}

	m_zeroMQSockets->clear();
}
