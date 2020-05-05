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
#include "CommInt.h"
#include "RegisterAtGMRequestHandler.h"
#include "GMWorkerThreadStateBuilder.h"
#include "LMMonitorWorker.h"
#include "Util.h"
#include "GMDatabaseManager.h"

INITIALIZE_EASYLOGGINGPP

int main()
{
#ifdef HUNTMEMLEAKS
	// dump all leaks after exit, so globals, statics and stuff will already be deleted
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// this can be used to include the MSVCRT in leak detection
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_CRT_DF);

	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

	char* b = new char[14]{ "LEAK DETECTOR" };  //Trigger a memory leak for NEW
#endif

	// ################## Define / Build global objects  ##################
	Util::setDefaultLogOptions("logs" + Util::pathSeperator + "GlobalManager.log");

	int timeBetweenTwoStatusFetchRoundsinMS = 5 * 1000;

	GMWorkerThreadStateBuilder* workerStateBuilder = new GMWorkerThreadStateBuilder();

	CommInt* comm = new CommInt(workerStateBuilder, 5, 5, "tcp://*:6669");
	LOG(INFO) << std::endl << "Hey! I am the GlobalManager" << std::endl << "My Host and Port: " << comm->getOwnServiceDescriptor().m_serviceHostAndPort << std::endl << "I was built on: " << __DATE__;;

	// ################## End of Define / Build global objects  ##################

	// ################## Registering Message Handler  ##################
	RegisterAtGMRequestHandler* m_RegisterRequest = new RegisterAtGMRequestHandler(comm);
	comm->registerFLUFFIMessageHandler(m_RegisterRequest, FLUFFIMessage::FluffCase::kRegisterAtGMRequest);
	// ################## End of Registering Message Handler  ##################

	// ################## Main Logic  ##################

#ifdef ENV64BIT
	Util::setConsoleWindowTitle("FLUFFI GM(64bit)");
#else
	Util::setConsoleWindowTitle("FLUFFI GM(32bit)");
#endif

	GMDatabaseManager::setDBConnectionParameters("db.fluffi", "fluffi_gm", "fluffi_gm", "fluffi_gm");

	// Start monitoring thread for LMs
	LOG(DEBUG) << "Starting thread for monitoring LM instances";
	LMMonitorWorker* lmMonitorWorker = new LMMonitorWorker(comm, workerStateBuilder, timeBetweenTwoStatusFetchRoundsinMS);

	lmMonitorWorker->m_thread = new std::thread(&LMMonitorWorker::workerMain, lmMonitorWorker);

	std::cin.ignore(); //Wait for a keypress
	LOG(INFO) << "Key Pressed -> Shutting down ...";

	// Stop worker threads
	LOG(DEBUG) << "Stoping thread for monitoring LM instances";
	lmMonitorWorker->stop();

	lmMonitorWorker->m_thread->join();
	delete lmMonitorWorker;

	// ################## End of Main Logic  ##################

	// ################## Destruct global objects  ##################

	delete comm; //Destruct comm and stop all handlers
	comm = nullptr;

	//Destruct all handlers
	delete m_RegisterRequest;
	m_RegisterRequest = nullptr;

	delete workerStateBuilder;
	workerStateBuilder = nullptr;

	// Free mysql global objects
	mysql_library_end();

	// Delete all global objects allocated by libprotobuf (Optional).
	google::protobuf::ShutdownProtobufLibrary();

	// ################## End of Destruct global objects  ##################

	LOG(DEBUG) << "Program terminated normally :) ";

	return 0;
}
