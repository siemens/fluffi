§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Abian Blome
§§*/
§§
§§#include "stdafx.h"
§§#include "CommInt.h"
§§#include "Util.h"
§§#include "CommWorker.h"
§§#include "CommProxyWorker.h"
§§#include "IFLUFFIMessageHandler.h"
§§#include "NotImplementedHandler.h"
§§#include "FluffiLMConfiguration.h"
§§#include "WorkerThreadState.h"
§§
§§const std::string CommInt::GlobalManagerHAP = "gm.fluffi:6669";
§§const std::streamoff CommInt::chunkSizeInBytes = 100 * 1000 * 1000;
§§const int CommInt::timeoutGMRegisterMessage = 2000;
§§const int CommInt::timeoutNormalMessage = 10 * 1000;
§§const int CommInt::timeoutFileTransferMessage = 10 * CommInt::timeoutNormalMessage;
§§
§§CommInt::CommInt(IWorkerThreadStateBuilder* workerThreadStateBuilder, unsigned char workerThreadCount, unsigned char ioThreadCount, const std::string& bindAddr) :
§§	m_zeroMQContext(nullptr),
§§	m_commIntServerSocket(nullptr),
§§	m_commIntWorkerSocket(nullptr),
§§	m_lmServiceDescriptor("", ""),
§§	m_listeningPort(0),
§§	m_notImplementedHandler(nullptr),
§§	m_commIntProxyControlSocket(nullptr),
§§	m_workerThreads(),
§§	m_proxyWorker(nullptr),
§§	m_ownHAP(""),
§§	m_ownGUID(""),
§§	m_registeredHandlers(),
§§	m_lastRoundTripTimes{ 0 },
§§	m_lastUpdatedRoundTripTime(0)
§§{
§§	//Register basic message handler(s)
§§	m_notImplementedHandler = new NotImplementedHandler();
§§	registerFLUFFIMessageHandler(m_notImplementedHandler, FLUFFIMessage::FluffCase::kRequestTypeNotImplementedRequest);
§§
§§	//Initializing zeroMQ
§§	m_zeroMQContext = new zmq::context_t(ioThreadCount);
§§	m_commIntServerSocket = new zmq::socket_t(*m_zeroMQContext, ZMQ_ROUTER);
§§	m_commIntServerSocket->bind(bindAddr);
§§	m_commIntServerSocket->setsockopt(ZMQ_RCVTIMEO, 1000);
§§	m_commIntServerSocket->setsockopt(ZMQ_SNDTIMEO, 1000);
§§	m_commIntServerSocket->setsockopt(ZMQ_LINGER, 1000);
§§	m_commIntServerSocket->setsockopt(ZMQ_SNDHWM, 0); //Set the send and receive high water marks to unlimeted. This means memory usage might go -> inf if the workers cannot handle the load
§§	m_commIntServerSocket->setsockopt(ZMQ_RCVHWM, 0); //Set the send and receive high water marks to unlimeted. This means memory usage might go -> inf if the workers cannot handle the load
§§	//m_commIntServerSocket->setsockopt(ZMQ_ROUTER_MANDATORY, 1); //Activating this will cause an exception every time the "Server" fails to answer a "Client"'s request in time. Reason: If the server does not respond in time, the client closes the connection. If then later the server tries to answer via that connection, an exception is thrown.
§§
§§	m_commIntWorkerSocket = new zmq::socket_t(*m_zeroMQContext, ZMQ_ROUTER);
§§	m_commIntWorkerSocket->setsockopt(ZMQ_RCVTIMEO, 1000);
§§	m_commIntWorkerSocket->setsockopt(ZMQ_SNDTIMEO, 1000);
§§	m_commIntWorkerSocket->setsockopt(ZMQ_LINGER, 1000);
§§	m_commIntWorkerSocket->setsockopt(ZMQ_SNDHWM, 0); //Set the send and receive high water marks to unlimeted. This means memory usage might go -> inf if the workers cannot handle the load
§§	m_commIntWorkerSocket->setsockopt(ZMQ_RCVHWM, 0); //Set the send and receive high water marks to unlimeted. This means memory usage might go -> inf if the workers cannot handle the load
§§	m_commIntWorkerSocket->bind("inproc://workers");
§§
§§	m_commIntProxyControlSocket = new zmq::socket_t(*m_zeroMQContext, ZMQ_PUB);
§§	m_commIntProxyControlSocket->bind("inproc://proxyControl");
§§
§§	//  Launch pool of worker threads
§§	for (unsigned short i = 0; i < workerThreadCount; i++) {
§§		CommWorker* worker = new CommWorker(this, workerThreadStateBuilder, i);
§§		m_workerThreads.push_back(worker);
§§		worker->m_thread = new std::thread(&CommWorker::workerMain, worker);
§§	}
§§
§§	//Wait until all  worker threads are ready
§§	for (std::vector<CommWorker*>::iterator it = m_workerThreads.begin(); it != m_workerThreads.end(); ) {
§§		if (!((*it)->isReady())) {
§§			std::this_thread::sleep_for(std::chrono::milliseconds(10));
§§		}
§§		else {
§§			++it;
§§		}
§§	}
§§
§§	//Launch the proxy worker
§§	CommProxyWorker* proxyWorker = new CommProxyWorker(this);
§§	m_proxyWorker = proxyWorker;
§§	m_proxyWorker->m_thread = new std::thread(&CommProxyWorker::workerMain, proxyWorker);
§§
§§	//Wait until the proxy worker is ready
§§	while (!m_proxyWorker->isReady()) {
§§		std::this_thread::sleep_for(std::chrono::milliseconds(10));
§§	}
§§
§§	//store the randomly generated port
§§	char port[1024];
§§	size_t size = sizeof(port);
§§	m_commIntServerSocket->getsockopt(ZMQ_LAST_ENDPOINT, &port, &size);
§§
§§	m_listeningPort = atoi(strchr(port + 5, ':') + 1);
§§}
§§
§§CommInt::~CommInt()
§§{
§§	//Stopping the workers
§§	LOG(DEBUG) << "Telling workers to stop";
§§	for (CommWorker* w : m_workerThreads) {
§§		if (w != nullptr) {
§§			w->stop();
§§		}
§§	}
§§
§§	while (!m_workerThreads.empty()) {
§§		CommWorker* w = m_workerThreads.back();
§§		if (w != nullptr) {
§§			if (w->m_thread != nullptr) {
§§				w->m_thread->join();
§§			}
§§			m_workerThreads.pop_back();
§§			delete w;
§§		}
§§	}
§§
§§	LOG(DEBUG) << "All workers stopped";
§§
§§	LOG(DEBUG) << "Telling proxy thread to stop";
§§	zmq::const_buffer buf = zmq::const_buffer("TERMINATE", 9);
§§	m_commIntProxyControlSocket->send(buf, zmq::send_flags::none);
§§	if (m_proxyWorker != nullptr) {
§§		m_proxyWorker->m_thread->join();
§§	}
§§	delete m_proxyWorker;
§§	m_proxyWorker = nullptr;
§§
§§	LOG(DEBUG) << "Proxy thread stopped";
§§
§§	//Tearing down zeroMQ;
§§	delete m_commIntProxyControlSocket;
§§	m_commIntProxyControlSocket = nullptr;
§§	delete m_commIntWorkerSocket;
§§	m_commIntWorkerSocket = nullptr;
§§	delete m_commIntServerSocket;
§§	m_commIntServerSocket = nullptr;
§§	delete m_zeroMQContext;
§§	m_zeroMQContext = nullptr;
§§
§§	//Freeing message handler objects
§§	delete m_notImplementedHandler;
§§	m_notImplementedHandler = nullptr;
§§
§§	LOG(DEBUG) << "Freed all ComInt ressources";
§§}
§§
§§int CommInt::getMyListeningPort() {
§§	return m_listeningPort;
§§}
§§
§§int CommInt::getFreeListeningPort() {
§§	zmq::socket_t socket(*m_zeroMQContext, ZMQ_REQ);
§§	socket.bind("tcp://127.0.0.1:*");
§§
§§	//store the randomly generated port
§§	char port[1024];
§§	size_t size = sizeof(port);
§§	socket.getsockopt(ZMQ_LAST_ENDPOINT, &port, &size);
§§
§§	socket.unbind(port);
§§	socket.close();
§§
§§	return atoi(strchr(port + 5, ':') + 1);
§§}
§§
§§bool CommInt::registerAtLM(WorkerThreadState* workerThreadState, AgentType type, const std::set<std::string> implementedAgentSubTypes, const  std::string location, const std::string LMHAP) {
§§	FLUFFIMessage req;
§§	FLUFFIMessage resp;
§§
§§	ServiceDescriptor* ptMySelfServiceDescriptor = new ServiceDescriptor();
§§	ptMySelfServiceDescriptor->CopyFrom(getOwnServiceDescriptor().getProtobuf());
§§
§§	RegisterAtLMRequest* registerRequest = new RegisterAtLMRequest();
§§	registerRequest->set_type(type);
§§	for (auto it = implementedAgentSubTypes.begin(); it != implementedAgentSubTypes.end(); ++it) {
§§		registerRequest->add_implementedagentsubtypes(*it);
§§	}
§§	registerRequest->set_location(location);
§§	registerRequest->set_allocated_servicedescriptor(ptMySelfServiceDescriptor);
§§	req.set_allocated_registeratlmrequest(registerRequest);
§§	bool success = sendReqAndRecvResp(&req, &resp, workerThreadState, LMHAP, CommInt::timeoutNormalMessage);
§§	if (success && resp.registeratlmresponse().success())
§§	{
§§		LOG(DEBUG) << "Registration at Local Manager " << LMHAP << " successful!";
§§#ifdef ENV64BIT
§§		Util::setConsoleWindowTitle("FLUFFI " + Util::agentTypeToString(type) + "(64bit) - " + resp.registeratlmresponse().fuzzjobname());
§§#else
§§		Util::setConsoleWindowTitle("FLUFFI " + Util::agentTypeToString(type) + "(32bit) - " + resp.registeratlmresponse().fuzzjobname());
§§#endif
§§		return true;
§§	}
§§	else
§§	{
§§		LOG(ERROR) << "Registration at Local Manager " << LMHAP << " failed!";
§§		return false;
§§	}
§§}
§§
§§bool CommInt::getLMConfigFromGM(WorkerThreadState* workerThreadState, const std::string location, const std::string GMHAP, FluffiLMConfiguration** theConfig) {
§§	FLUFFIMessage req;
§§	FLUFFIMessage resp;
§§
§§	ServiceDescriptor* ptMySelfServiceDescriptor = new ServiceDescriptor();
§§	ptMySelfServiceDescriptor->CopyFrom(getOwnServiceDescriptor().getProtobuf());
§§
§§	RegisterAtGMRequest* registerRequest = new RegisterAtGMRequest();
§§	registerRequest->set_type(AgentType::LocalManager);
§§	registerRequest->set_location(location);
§§	registerRequest->set_allocated_servicedescriptor(ptMySelfServiceDescriptor);
§§	req.set_allocated_registeratgmrequest(registerRequest);
§§	bool success = sendReqAndRecvResp(&req, &resp, workerThreadState, GMHAP, CommInt::timeoutGMRegisterMessage);
§§	if (success)
§§	{
§§		if (resp.registeratgmresponse().retrylater())
§§		{
§§			LOG(INFO) << "Registration at Global Manager " << GMHAP << " was delayed as it is not yet decided what we should do!";
§§			return false;
§§		}
§§		LOG(INFO) << "Registration at Global Manager " << GMHAP << " successful!";
§§		*theConfig = new FluffiLMConfiguration(resp.registeratgmresponse().lmconfiguration());
§§		return true;
§§	}
§§	else
§§	{
§§		LOG(ERROR) << "Registration at Global Manager " << GMHAP << " failed!";
§§		return false;
§§	}
§§}
§§
§§bool CommInt::registerAtGM(WorkerThreadState* workerThreadState, AgentType type, const std::set<std::string> implementedAgentSubTypes, const std::string location, const std::string GMHAP) {
§§	FLUFFIMessage req;
§§	FLUFFIMessage resp;
§§
§§	ServiceDescriptor* ptMySelfServiceDescriptor = new ServiceDescriptor();
§§	ptMySelfServiceDescriptor->CopyFrom(getOwnServiceDescriptor().getProtobuf());
§§
§§	RegisterAtGMRequest* registerRequest = new RegisterAtGMRequest();
§§	registerRequest->set_type(type);
§§	for (auto it = implementedAgentSubTypes.begin(); it != implementedAgentSubTypes.end(); ++it) {
§§		registerRequest->add_implementedagentsubtypes(*it);
§§	}
§§	registerRequest->set_location(location);
§§	registerRequest->set_allocated_servicedescriptor(ptMySelfServiceDescriptor);
§§	req.set_allocated_registeratgmrequest(registerRequest);
§§	bool success = sendReqAndRecvResp(&req, &resp, workerThreadState, GMHAP, CommInt::timeoutGMRegisterMessage);
§§	if (success)
§§	{
§§		if (resp.registeratgmresponse().retrylater())
§§		{
§§			LOG(INFO) << "Registration at Global Manager " << GMHAP << " was delayed as it is not yet decided what we should do!";
§§			return false;
§§		}
§§		LOG(INFO) << "Registration at Global Manager " << GMHAP << " successful!";
§§		m_lmServiceDescriptor = FluffiServiceDescriptor(resp.registeratgmresponse().lmservicedescriptor());
§§		return true;
§§	}
§§	else
§§	{
§§		LOG(ERROR) << "Registration at Global Manager " << GMHAP << " failed!";
§§		return false;
§§	}
§§}
§§
§§FluffiServiceDescriptor CommInt::getOwnServiceDescriptor() {
§§	if (m_ownHAP == "" || m_ownGUID == "") {
§§		char hostname[128];
§§		gethostname(hostname, 128);
§§		hostname[127] = 0;
§§		int port = getMyListeningPort();
§§		m_ownHAP = std::string(hostname) + ".fluffi" + ":" + std::to_string(port);
§§	}
§§	if (m_ownGUID == "") {
§§		m_ownGUID = Util::newGUID();
§§	}
§§
§§	return FluffiServiceDescriptor{ m_ownHAP, m_ownGUID };
§§}
§§
§§FluffiServiceDescriptor CommInt::getMyLMServiceDescriptor() {
§§	return m_lmServiceDescriptor;
§§}
§§
§§bool CommInt::waitForGMRegistration(WorkerThreadState* workerThreadState, AgentType type, const std::set<std::string> implementedAgentSubTypes, const std::string location, int intervallBetweenTwoPullingRoundsInMillisec)
§§{
§§	bool wasKeyPressed = false;
§§	int attemptCount = 0;
§§	while (!wasKeyPressed && attemptCount < 5)
§§	{
§§		attemptCount++;
§§		bool didRegistrationSucceed = registerAtGM(workerThreadState, type, implementedAgentSubTypes, location, CommInt::GlobalManagerHAP);
§§		if (didRegistrationSucceed)
§§		{
§§			LOG(INFO) << "Registered at GM succesfull, forwarded to LM: " << m_lmServiceDescriptor.m_serviceHostAndPort;
§§			return true;
§§		}
§§		else
§§		{
§§			LOG(INFO) << "Could not retrieve my LM from GM, retrying in " << intervallBetweenTwoPullingRoundsInMillisec / 1000 << " seconds";
§§			int checkAgainMS = 500;
§§			int waitedMS = 0;
§§
§§			//Wait for intervallBetweenTwoPullingRoundsInMillisec but break if a key is pressed
§§			while (!wasKeyPressed)
§§			{
§§				wasKeyPressed = Util::kbhit() != 0;
§§				if (waitedMS < intervallBetweenTwoPullingRoundsInMillisec) {
§§					std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
§§					waitedMS += checkAgainMS;
§§					continue;
§§				}
§§				else {
§§					break;
§§				}
§§			}
§§		}
§§	}
§§	LOG(ERROR) << "Could not register at GM!";
§§
§§	return false;
§§}
§§
§§bool CommInt::waitForLMRegistration(WorkerThreadState* workerThreadState, AgentType type, const std::set<std::string> implementedAgentSubTypes, const std::string location, int intervallBetweenTwoPullingRoundsInMillisec)
§§{
§§	bool wasKeyPressed = false;
§§	while (!wasKeyPressed)
§§	{
§§		bool didRegistrationSucceed = registerAtLM(workerThreadState, type, implementedAgentSubTypes, location, m_lmServiceDescriptor.m_serviceHostAndPort);
§§		if (didRegistrationSucceed)
§§		{
§§			LOG(INFO) << "Registered at LM succesfull! ";
§§			return true;
§§		}
§§		else
§§		{
§§			LOG(INFO) << "Could not register at LM, retrying in " << intervallBetweenTwoPullingRoundsInMillisec / 1000 << " seconds";
§§			int checkAgainMS = 500;
§§			int waitedMS = 0;
§§
§§			//Wait for intervallBetweenTwoPullingRoundsInMillisec but break if a key is pressed
§§			while (!wasKeyPressed)
§§			{
§§				wasKeyPressed = Util::kbhit() != 0;
§§				if (waitedMS < intervallBetweenTwoPullingRoundsInMillisec) {
§§					std::this_thread::sleep_for(std::chrono::milliseconds(checkAgainMS));
§§					waitedMS += checkAgainMS;
§§					continue;
§§				}
§§				else {
§§					break;
§§				}
§§			}
§§		}
§§	}
§§
§§	return false;
§§}
§§
§§std::string CommInt::getMyGUID() {
§§	if (m_ownGUID == "") {
§§		m_ownGUID = Util::newGUID();
§§	}
§§
§§	return m_ownGUID;
§§}
§§
§§bool CommInt::sendReqAndRecvResp(FLUFFIMessage* req, FLUFFIMessage* resp, WorkerThreadState* workerThreadState, const std::string destinationHAP, int timeoutMS) {
§§	zmq::socket_t* sock = workerThreadState->getSocketFor(m_zeroMQContext, destinationHAP, timeoutMS);
§§
§§	char* reqAsArray = new char[req->ByteSize()];
§§	req->SerializeToArray(reqAsArray, req->ByteSize());
§§
§§	LOG(DEBUG) << "Sending request of type " << req->GetDescriptor()->oneof_decl(0)->field(req->fluff_case() - 1)->camelcase_name() << " of length " << req->ByteSize();
§§	zmq::message_t zeroMQRequest(reqAsArray, req->ByteSize());
§§	delete[] reqAsArray;
§§
§§	std::chrono::time_point<std::chrono::steady_clock> preSendTS = std::chrono::steady_clock::now();
§§	try {
§§		zmq::detail::send_result_t wasSuccessfullySend = sock->send(zeroMQRequest, zmq::send_flags::none);
§§		if (!wasSuccessfullySend.has_value()) {
§§			LOG(DEBUG) << "CommWorker::sendReqAndRecvResp send failed with timeout";
§§			//Delete entry from socket cache
§§			workerThreadState->clearSocketFor(destinationHAP, timeoutMS);
§§			return false;
§§		}
§§	}
§§	catch (zmq::error_t& e) {
§§		LOG(ERROR) << "CommWorker::sendReqAndRecvResp: Something went wrong while sending messages:" << zmq_errno() << "-" << e.what();
§§		google::protobuf::ShutdownProtobufLibrary();
§§		_exit(EXIT_FAILURE); //make compiler happy
§§	}
§§
§§	zmq::message_t zeroMQResponse;
§§	try {
§§		zmq::detail::recv_result_t wasSuccessfullyReceived = sock->recv(zeroMQResponse, zmq::recv_flags::none);
§§		if (!wasSuccessfullyReceived.has_value()) {
§§			LOG(DEBUG) << "CommWorker::sendReqAndRecvResp recv failed with timeout";
§§			//Delete entry from socket cache
§§			workerThreadState->clearSocketFor(destinationHAP, timeoutMS);
§§			return false;
§§		}
§§	}
§§	catch (zmq::error_t& e) {
§§		LOG(ERROR) << "CommWorker::sendReqAndRecvResp: Something went wrong while receiving messages:" << zmq_errno() << "-" << e.what();
§§		google::protobuf::ShutdownProtobufLibrary();
§§		_exit(EXIT_FAILURE); //make compiler happy
§§	}
§§
§§	std::chrono::time_point<std::chrono::steady_clock> postReceiveTS = std::chrono::steady_clock::now();
§§	unsigned int elapsedMilliseconds = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::milliseconds>(postReceiveTS - preSendTS).count());
§§	//Updating the RTTs like this is not thread safe. However, the worst thing that happens is that we loose RTT values. Consequently, we ignore thread safety / race conditions here
§§	m_lastRoundTripTimes[m_lastUpdatedRoundTripTime] = elapsedMilliseconds;
§§	m_lastUpdatedRoundTripTime = (m_lastUpdatedRoundTripTime + 1) % static_cast<int>(sizeof(m_lastRoundTripTimes) / sizeof(m_lastRoundTripTimes[0]));
§§
§§	resp->ParseFromArray(zeroMQResponse.data(), static_cast<int>(zeroMQResponse.size()));
§§	LOG(DEBUG) << "Received response of type " << resp->GetDescriptor()->oneof_decl(0)->field(resp->fluff_case() - 1)->camelcase_name() << " of length " << resp->ByteSize();
§§
§§	return true;
§§}
§§
§§void CommInt::registerFLUFFIMessageHandler(IFLUFFIMessageHandler* handler, FLUFFIMessage::FluffCase whatfor) {
§§	m_registeredHandlers[whatfor] = handler;
§§}
§§
§§IFLUFFIMessageHandler* CommInt::getFLUFFIMessageHandler(FLUFFIMessage::FluffCase whatfor) {
§§	if (m_registeredHandlers.count(whatfor) == 1) {
§§		return m_registeredHandlers[whatfor];
§§	}
§§	else {
§§		return m_registeredHandlers[FLUFFIMessage::FluffCase::kRequestTypeNotImplementedRequest];
§§	}
§§}
§§
§§std::string CommInt::getAverageRoundTripTime() {
§§	//Reading the RTTs like this is not thread safe. However, the worst thing that happens is that we use old RTT values. Consequently, we ignore thread safety / race conditions here
§§
§§	unsigned int sumOfRTTs = 0;
§§	for (int i = 0; i < static_cast<int>(sizeof(m_lastRoundTripTimes) / sizeof(m_lastRoundTripTimes[0])); i++) {
§§		sumOfRTTs += m_lastRoundTripTimes[i];
§§	}
§§
§§	unsigned long long averageRTT = sumOfRTTs / (sizeof(m_lastRoundTripTimes) / sizeof(m_lastRoundTripTimes[0]));
§§
§§	return std::to_string(averageRTT);
§§}
