/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "CommWorker.h"
#include "CommInt.h"
#include "IFLUFFIMessageHandler.h"
#include "WorkerThreadState.h"
#include "IWorkerThreadStateBuilder.h"

§§CommWorker::CommWorker(CommInt* commInt, IWorkerThreadStateBuilder* workerThreadStateBuilder, unsigned short workerNum) :
	m_commInt(commInt),
	m_workerThreadStateBuilder(workerThreadStateBuilder),
	m_isready(false),
	m_workerNum(workerNum)
{
}

CommWorker::~CommWorker()
{
	delete m_thread;
	m_thread = nullptr;
}

void CommWorker::stop() {
	if (m_workerThreadState != nullptr) {
		m_workerThreadState->m_stopRequested = true;
	}
}

bool CommWorker::isReady() {
	return m_isready;
}

void CommWorker::workerMain() {
	m_workerThreadState = m_workerThreadStateBuilder->constructState();

	zmq::socket_t sock(*m_commInt->m_zeroMQContext, ZMQ_REQ); //ZMQ_ROUTER <->ZMQ_REQ
	sock.setsockopt(ZMQ_RCVTIMEO, 500);
	sock.setsockopt(ZMQ_SNDTIMEO, 500);
	sock.setsockopt(ZMQ_LINGER, 500);
	std::string workerNumAsString = std::to_string(m_workerNum);
	sock.setsockopt(ZMQ_IDENTITY, workerNumAsString.c_str(), workerNumAsString.length());
	sock.connect("inproc://workers");

	//Tell the proxy worker about this comm worker
	std::string ready = "READY";
	zmq::message_t readymsg(ready.size());
	memcpy(readymsg.data(), ready.data(), ready.size());
	sock.send(readymsg, zmq::send_flags::none);

	m_isready = true;

	while (!m_workerThreadState->m_stopRequested) {
		zmq::multipart_t  mrequest;
		try {
			bool isThereMore = mrequest.recv(sock);
			if (!isThereMore) {
				//no new message for about half a second: retry (needed to be able to terminate this thread)
				continue;
			}
		}
		catch (zmq::error_t& e) {
			LOG(ERROR) << "CommWorker::workerMain: Something went wrong while receiving messages:" << zmq_errno() << "-" << e.what();
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}
		zmq::message_t request = mrequest.remove();

§§		FLUFFIMessage incomingMessage;
§§		incomingMessage.ParseFromArray(request.data(), static_cast<int>(request.size()));

§§		LOG(DEBUG) << "Received request of type " << incomingMessage.GetDescriptor()->oneof_decl(0)->field(incomingMessage.fluff_case() - 1)->camelcase_name() << " of length " << incomingMessage.ByteSize();
§§
		FLUFFIMessage outgoingMessage;
§§		m_commInt->getFLUFFIMessageHandler(incomingMessage.fluff_case())->handleFLUFFIMessage(m_workerThreadState, &incomingMessage, &outgoingMessage);

§§		char* responseAsArray = new char[outgoingMessage.ByteSize()];
		outgoingMessage.SerializeToArray(responseAsArray, outgoingMessage.ByteSize());

		LOG(DEBUG) << "Sending response of type " << outgoingMessage.GetDescriptor()->oneof_decl(0)->field(outgoingMessage.fluff_case() - 1)->camelcase_name() << " of length " << outgoingMessage.ByteSize();
§§
		zmq::message_t zeroMQRequest(responseAsArray, outgoingMessage.ByteSize());
		mrequest.add(std::move(zeroMQRequest));
		try {
			bool success = mrequest.send(sock);
			if (!success) {
				throw zmq::error_t();
			}
		}
		catch (zmq::error_t& e) {
			LOG(ERROR) << "CommWorker::workerMain: Something went wrong while sending messages:" << zmq_errno() << "-" << e.what();
			google::protobuf::ShutdownProtobufLibrary();
§§			_exit(EXIT_FAILURE); //make compiler happy
		}

		delete[] responseAsArray;
§§		responseAsArray = nullptr;
	}

	m_workerThreadStateBuilder->destructState(m_workerThreadState);
}
