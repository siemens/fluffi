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
§§#pragma once
§§#include "FluffiServiceDescriptor.h"
§§
§§class CommWorker;
§§class CommProxyWorker;
§§class IFLUFFIMessageHandler;
§§class NotImplementedHandler;
§§class FluffiLMConfiguration;
§§class IWorkerThreadStateBuilder;
§§class WorkerThreadState;
§§class CommInt
§§{
§§public:
§§	CommInt(IWorkerThreadStateBuilder*  workerThreadStateBuilder, unsigned char workerThreadCount, unsigned char ioThreadCount, const std::string& bindAddr = "tcp://*:*");
§§	virtual ~CommInt();
§§
§§	IFLUFFIMessageHandler* getFLUFFIMessageHandler(FLUFFIMessage::FluffCase whatfor);
§§	int getMyListeningPort();
§§	std::string getMyGUID();
§§	FluffiServiceDescriptor getMyLMServiceDescriptor();
§§	FluffiServiceDescriptor getOwnServiceDescriptor();
§§	bool waitForGMRegistration(WorkerThreadState* workerThreadState, AgentType type, const std::set<std::string> implementedAgentSubTypes, const std::string location, int intervallBetweenTwoPullingRoundsInMillisec);
§§	bool waitForLMRegistration(WorkerThreadState* workerThreadState, AgentType type, const std::set<std::string> implementedAgentSubTypes, const std::string location, int intervallBetweenTwoPullingRoundsInMillisec);
§§
§§	std::string getAverageRoundTripTime();
§§	int getFreeListeningPort();
§§	bool getLMConfigFromGM(WorkerThreadState* workerThreadState, const std::string location, const std::string GMHAP, FluffiLMConfiguration** theConfig);
§§	void registerFLUFFIMessageHandler(IFLUFFIMessageHandler* handler, FLUFFIMessage::FluffCase whatfor);
§§	bool sendReqAndRecvResp(FLUFFIMessage* req, FLUFFIMessage* resp, WorkerThreadState* workerThreadState, const std::string  destinationHAP, int timeoutMS);
§§
§§	zmq::context_t* m_zeroMQContext;
§§	zmq::socket_t* m_commIntServerSocket;
§§	zmq::socket_t* m_commIntWorkerSocket;
§§
§§	static const std::string GlobalManagerHAP;
§§	static const std::streamoff chunkSizeInBytes;
§§	static const int timeoutGMRegisterMessage;
§§	static const int timeoutNormalMessage;
§§	static const int timeoutFileTransferMessage;
§§
§§private:
§§	bool registerAtGM(WorkerThreadState* workerThreadState, AgentType type, const std::set<std::string> implementedAgentSubTypes, const std::string location, const std::string GMHAP);
§§	bool registerAtLM(WorkerThreadState* workerThreadState, AgentType type, const std::set<std::string> implementedAgentSubTypes, const std::string location, const std::string LMHAP);
§§
§§	FluffiServiceDescriptor m_lmServiceDescriptor;
§§	int m_listeningPort;
§§
§§	NotImplementedHandler* m_notImplementedHandler;
§§	zmq::socket_t* m_commIntProxyControlSocket;
§§	std::vector<CommWorker*> m_workerThreads;
§§	CommProxyWorker* m_proxyWorker;
§§	std::string m_ownHAP;
§§	std::string m_ownGUID;
§§	std::map <FLUFFIMessage::FluffCase, IFLUFFIMessageHandler*> m_registeredHandlers;
§§
§§	unsigned int m_lastRoundTripTimes[20];
§§	int m_lastUpdatedRoundTripTime;
§§};
