/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

§§Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once

class CommInt;
class CommProxyWorker
{
public:
§§	CommProxyWorker(CommInt* commInt);
§§	virtual ~CommProxyWorker();

	void workerMain();
	bool isReady();

	std::thread* m_thread = nullptr;

private:
§§	void wrap(zmq::multipart_t* mmsg, const std::string* address, const std::string* delim);
	std::string unwrap(zmq::multipart_t* mmsg);
§§	int fluffi_proxy_steerable(zmq::socket_t* frontend_, zmq::socket_t* backend_, zmq::socket_t* control_);

§§	CommInt* m_commInt;
	bool m_isready;
	std::deque<std::string> m_worker_queue;
};
