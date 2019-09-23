§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Pascal Eckmann
§§*/
§§
§§#include "stdafx.h"
§§#include "CppUnitTest.h"
§§#include "Util.h"
§§#include "TRWorkerThreadStateBuilder.h"
§§#include "TRWorkerThreadState.h"
§§
§§using namespace Microsoft::VisualStudio::CppUnitTestFramework;
§§
§§namespace FluffiTester
§§{
§§	TEST_CLASS(TRWorkerThreadStateBuilderTest)
§§	{
§§	public:
§§
§§		TEST_METHOD_INITIALIZE(ModuleInitialize)
§§		{
§§			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
§§		}
§§
§§		TEST_METHOD(TRWorkerThreadStateBuilder_TRWorkerThreadStateBuilder)
§§		{
§§			TRWorkerThreadStateBuilder stateBuilder;
§§
§§			TRWorkerThreadState* thestate = (TRWorkerThreadState*)stateBuilder.constructState();
§§			Assert::IsNotNull(thestate);
§§
§§			Assert::IsNotNull(thestate->m_zeroMQSockets);
§§
§§			zmq::context_t* zeroMQContext = new zmq::context_t();
§§			zmq::socket_t* sock = new zmq::socket_t(*zeroMQContext, ZMQ_REQ); //ZMQ_REQ<->ZMQ_DEALER
§§			zmq::socket_t* sock2 = new zmq::socket_t(*zeroMQContext, ZMQ_REQ); //ZMQ_REQ<->ZMQ_DEALER
§§			zmq::socket_t* sock3 = new zmq::socket_t(*zeroMQContext, ZMQ_REQ); //ZMQ_REQ<->ZMQ_DEALER
§§
§§			Assert::IsTrue(thestate->m_zeroMQSockets->insert(std::make_pair(std::make_pair("test", 0), sock)).second);
§§			Assert::IsTrue(thestate->m_zeroMQSockets->insert(std::make_pair(std::make_pair("test", 2), sock2)).second);
§§			Assert::IsFalse(thestate->m_zeroMQSockets->insert(std::make_pair(std::make_pair("test", 2), sock3)).second);
§§			sock3->close();
§§			delete sock3;
§§			Assert::IsTrue(thestate->m_zeroMQSockets->insert(std::make_pair(std::make_pair("test", 1), nullptr)).second);
§§
§§			stateBuilder.destructState(thestate);
§§
§§			delete zeroMQContext;
§§
§§			//if this testcase terminates all sockets are actually closed :D
§§		}
§§	};
§§}
