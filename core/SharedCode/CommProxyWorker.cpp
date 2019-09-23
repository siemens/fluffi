§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Abian Blome, Roman Bendt
§§*/
§§
§§#include "stdafx.h"
§§#include "CommProxyWorker.h"
§§#include "CommInt.h"
§§
§§CommProxyWorker::CommProxyWorker(CommInt* commInt) :
§§	m_commInt(commInt),
§§	m_isready(false),
§§	m_worker_queue()
§§{
§§}
§§
§§CommProxyWorker::~CommProxyWorker()
§§{
§§	delete m_thread;
§§	m_thread = nullptr;
§§}
§§
§§bool CommProxyWorker::isReady() {
§§	return m_isready;
§§}
§§
§§//adapted from zmsg.hpp
§§void CommProxyWorker::wrap(zmq::multipart_t* mmsg, const std::string* address, const std::string* delim) {
§§	if (delim) {
§§		mmsg->pushstr(*delim);
§§	}
§§	mmsg->pushstr(*address);
§§}
§§
§§//adapted from zmsg.hpp
§§std::string CommProxyWorker::unwrap(zmq::multipart_t* mmsg) {
§§	if (mmsg->size() == 0) {
§§		errno = 0x182;
§§		throw zmq::error_t();
§§	}
§§	std::string addr = mmsg->popstr();
§§	if (mmsg->size() > 0 && mmsg->at(0).size() == 0) {
§§		mmsg->popstr();
§§	}
§§	return addr;
§§}
§§
§§//adapted from zmq proxy_steerable(
§§int CommProxyWorker::fluffi_proxy_steerable(zmq::socket_t* frontend_, zmq::socket_t* backend_, zmq::socket_t* control_)
§§{
§§	zmq_pollitem_t items[] = { { *frontend_, 0, ZMQ_POLLIN, 0 },
§§	{ *backend_, 0, ZMQ_POLLIN, 0 },
§§	{ *control_, 0, ZMQ_POLLIN, 0 } };
§§	zmq_pollitem_t itemsout[] = { { *frontend_, 0, ZMQ_POLLOUT, 0 },
§§	{ *backend_, 0, ZMQ_POLLOUT, 0 } };
§§	std::string emptyString = "";
§§
§§	//  Proxy can be in these three states
§§	enum
§§	{
§§		active,
§§		paused,
§§		terminated
§§	} state = active;
§§
§§	while (state != terminated) {
§§		//  Wait while there are either requests or replies to process.
§§		zmq::poll(&items[0], 3, -1);
§§
§§		//  Get the pollout separately because when combining this with pollin it maxes the CPU
§§		//  because pollout shall most of the time return directly.
§§		//  POLLOUT is only checked when frontend and backend sockets are not the same.
§§		if (zmq::socket_ref(*frontend_) != zmq::socket_ref(*backend_)) {
§§			zmq::poll(&itemsout[0], 2, 0);
§§		}
§§
§§		//  Process a control command if any
§§		if (control_ && items[2].revents & ZMQ_POLLIN) {
§§			zmq::message_t  msg;
§§			control_->recv(msg, zmq::recv_flags::none);
§§
§§			if (msg.size() == 5 && memcmp(msg.data(), "PAUSE", 5) == 0)
§§				state = paused;
§§			else if (msg.size() == 6 && memcmp(msg.data(), "RESUME", 6) == 0)
§§				state = active;
§§			else if (msg.size() == 9
§§				&& memcmp(msg.data(), "TERMINATE", 9) == 0)
§§				state = terminated;
§§			else {
§§				LOG(ERROR) << "E: invalid command sent to proxy";
§§				google::protobuf::ShutdownProtobufLibrary();
§§				_exit(EXIT_FAILURE); //make compiler happy
§§			}
§§		}
§§
§§		//  Process a reply
§§		if (state == active && zmq::socket_ref(*frontend_) != zmq::socket_ref(*backend_) && items[1].revents & ZMQ_POLLIN && itemsout[0].revents & ZMQ_POLLOUT) {
§§			//LOG(DEBUG) << "fluffi_proxy_steerable: Process a reply";
§§			zmq::multipart_t  mmsg;
§§			mmsg.recv(*backend_);
§§
§§			std::string address = unwrap(&mmsg);
§§			m_worker_queue.push_back(address);
§§
§§			//  Return reply to client if it's not a READY
§§			if (mmsg.peekstr(0) != "READY") {
§§				mmsg.send(*frontend_);
§§			}
§§			else {
§§				LOG(DEBUG) << "Worker " << address << " successfully registered at Proxyworker";
§§			}
§§		}
§§
§§		//  Process a request
§§		if (state == active && items[0].revents & ZMQ_POLLIN && (zmq::socket_ref(*frontend_) == zmq::socket_ref(*backend_) || itemsout[1].revents & ZMQ_POLLOUT)) {
§§			if (m_worker_queue.size() > 0) {
§§				//LOG(DEBUG) << "fluffi_proxy_steerable: Process a request";
§§				zmq::multipart_t  mmsg;
§§				mmsg.recv(*frontend_);
§§				std::string nextworker = m_worker_queue.front();
§§				wrap(&mmsg, &nextworker, &emptyString);
§§				mmsg.send(*backend_);
§§
§§				//  Dequeue and drop the next worker address
§§				m_worker_queue.pop_front();
§§			}
§§			else {
§§				LOG(WARNING) << "NO workers to forward requests to! Messages will get delayed !";
§§				std::this_thread::sleep_for(std::chrono::milliseconds(10));
§§			}
§§		}
§§	}
§§	return 0;
§§}
§§
§§void CommProxyWorker::workerMain() {
§§	//  Connect work threads to client threads via a queue
§§	zmq::socket_t control(*m_commInt->m_zeroMQContext, ZMQ_SUB);
§§	control.connect("inproc://proxyControl");
§§	control.setsockopt(ZMQ_SUBSCRIBE, "", 0);
§§	//there might be a race condition if the interface shuts down quickly after system initialisation. In this case the TERMINATE is send before the proxy subscribed to it :(
§§	//To avoid this always wait until the worker is ready!
§§	m_isready = true;
§§
§§	LOG(DEBUG) << "CommProxyWorker will start now";
§§	try {
§§		fluffi_proxy_steerable(m_commInt->m_commIntServerSocket, m_commInt->m_commIntWorkerSocket, &control);
§§	}
§§	catch (zmq::error_t& e) {
§§		LOG(ERROR) << "CommProxyWorker::proxy_steerable: Something went wrong while forwarding messages:" << zmq_errno() << "-" << e.what();
§§		google::protobuf::ShutdownProtobufLibrary();
§§		_exit(EXIT_FAILURE); //make compiler happy
§§	}
§§	LOG(DEBUG) << "CommProxyWorker terminated";
§§}
