/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Pascal Eckmann
*/

#include <stdio.h>
#include <string>
#include <mutex>
#include <unordered_set>
#include <memory>
#include <vector>
#include <experimental/filesystem>
#include <unistd.h>

#include "FLUFFI.pb.h"

#define ELPP_NO_DEFAULT_LOG_FOLE
#define ELPP_THREAD_SAFE
#include "easylogging++.h"
#include "Util.h"
#include "ExternalProcess.h"
#include "DebugExecutionOutput.h"

#define DEBUGGERTESTER std::string("./DebuggerTester")
#define CONSOLEREFLECTOR std::string("./ConsoleReflector")


//g++ -std=c++11 mytester.c -I ../../../../SharedCode -I ../../../../dependencies/easylogging/include -I ../../../../dependencies/libprotoc/include/siemens/cpp -I ../../../../dependencies/libprotoc/include -I ../../../../dependencies/libuuid/include -L ../../../../bin  -o mytester ../../../../lib/sharedcode.a ../../../../dependencies/libprotoc/lib/x86-64/libprotobuf.a  ../../../../dependencies/easylogging/lib/x86-64/libeasyloggingpp.a ../../../../dependencies/libuuid/lib/x86-64/libuuid.a  -pthread -lstdc++fs  -ldl -l:libzmq.so.5

INITIALIZE_EASYLOGGINGPP

int main() {

	Util::setDefaultLogOptions("mytest.log");

§§	LOG(INFO) << "Starting RUN tests!";

	//normal execution should print "option 0 do nothing"
	{
		ExternalProcess ep(DEBUGGERTESTER + " 0", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 1:" << success;

		success =ep.run();
§§		LOG(INFO) << "run 1:" << success;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//exception execution should print "option 1 pre" as the post wont be seen
	{
		ExternalProcess ep(DEBUGGERTESTER + " 1", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 2:" << success;

		success =ep.run();
§§		LOG(INFO) << "run 2:" << success;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//timeout execution should print "option 3 pre", as the process is killed before post
	{
		ExternalProcess ep(DEBUGGERTESTER + " 3", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 3:" << success;

		success =ep.run();
§§		LOG(INFO) << "run 3:" << success;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}



§§	LOG(INFO) << "RUN Tests completed!";


§§	LOG(INFO) << "Starting RUNANDWAIT tests!";

	//normal execution should print "option 0 do nothing"
	{
		ExternalProcess ep(DEBUGGERTESTER + " 0", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 1:" << success;

		success =ep.runAndWaitForCompletion(20000);
§§		LOG(INFO) << "runAndWaitForCompletion 1:" << success;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//exception execution should print "option 1 pre" as the post wont be seen
	{
		ExternalProcess ep(DEBUGGERTESTER + " 1", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 2:" << success;

		success =ep.runAndWaitForCompletion(20000);
§§		LOG(INFO) << "runAndWaitForCompletion 2:" << success;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//looong execution should print "option 3 pre", and "option 3 post" 10 seconds later
	{
		ExternalProcess ep(DEBUGGERTESTER + " 3", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 3:" << success;

		success =ep.runAndWaitForCompletion(20000);
§§		LOG(INFO) << "runAndWaitForCompletion 3:" << success;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//timeout execution should print "option 3 pre" but not "option 3: post sleep", as it should get killed before
	{
		ExternalProcess ep(DEBUGGERTESTER + " 3", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 4:" << success;

		success =ep.runAndWaitForCompletion(5000);
§§		LOG(INFO) << "runAndWaitForCompletion 4:" << success;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}



§§	LOG(INFO) << "RUNANDWAIT Tests completed!";





§§	LOG(INFO) << "Starting DEBUG tests!";

	//normal execution should print "option 0 do nothing"
	{
		ExternalProcess ep(DEBUGGERTESTER + " 0", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 1 :" << success;

		std::shared_ptr<DebugExecutionOutput> dex = std::make_shared<DebugExecutionOutput>();
		ep.debug(20000,dex,false,false);
§§		LOG(INFO) << "debug 1:" << dex->m_terminationDescription;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//exception execution should print "option 1 pre" as the post wont be seen an EXCEPTION_ACCESS_VIOLATION is expected
	{
		ExternalProcess ep(DEBUGGERTESTER + " 1", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 2:" << success;

		std::shared_ptr<DebugExecutionOutput> dex = std::make_shared<DebugExecutionOutput>();
		ep.debug(20000,dex,false,false);
§§		LOG(INFO) << "debug 2:" << dex->m_terminationDescription;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//exception execution should print "option 2 pre" as the post wont be seen an signal 8 is expected
	{
		ExternalProcess ep(DEBUGGERTESTER + " 2", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 3:" << success;

		std::shared_ptr<DebugExecutionOutput> dex = std::make_shared<DebugExecutionOutput>();
		ep.debug(20000,dex,false,false);
§§		LOG(INFO) << "debug 3:" << dex->m_terminationDescription;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//looong execution should print "option 3 pre", and "option 3 post" 10 seconds later
	{
		ExternalProcess ep(DEBUGGERTESTER + " 3", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 4:" << success;

		std::shared_ptr<DebugExecutionOutput> dex = std::make_shared<DebugExecutionOutput>();
		ep.debug(20000,dex,false,false);
§§		LOG(INFO) << "debug 4:" << dex->m_terminationDescription;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//timeout execution should print "option 3 pre" but nothing else as it gets killed after a "Process hang detected" is expected
	{
		ExternalProcess ep(DEBUGGERTESTER + " 3", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.initProcess();
§§		LOG(INFO) << "init 5:" << success;

		std::shared_ptr<DebugExecutionOutput> dex = std::make_shared<DebugExecutionOutput>();
		ep.debug(5000,dex,false,false);
§§		LOG(INFO) << "debug 5:" << dex->m_terminationDescription;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}



§§	LOG(INFO) << "DEBUG Tests completed!";


§§	LOG(INFO) << "Starting ATTACH tests!";

	//attach to sleeping program - then run
	{
		std::system((DEBUGGERTESTER + " 3 &").c_str());
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		ExternalProcess ep(DEBUGGERTESTER, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.attachToProcess();
§§		LOG(INFO) << "attach 1:" << success;

		success =ep.run();
§§		LOG(INFO) << "run 1:" << success;


§§		LOG(INFO) << "done with run - waiting";

		std::this_thread::sleep_for(std::chrono::milliseconds(20000));

§§		LOG(INFO) << "done with waiting";
	}



	//attach to sleeping program - then runAndWaitForCompletion
	{
		std::system((DEBUGGERTESTER + " 3 &").c_str());
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		ExternalProcess ep(DEBUGGERTESTER, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.attachToProcess();
§§		LOG(INFO) << "attach 2:" << success;

		success =ep.runAndWaitForCompletion(20000);
§§		LOG(INFO) << "runAndWaitForCompletion 2:" << success;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//attach to sleeping program - then debug + kill (a hang is expected)
	{
		std::system((DEBUGGERTESTER + " 3 &").c_str());
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		ExternalProcess ep(DEBUGGERTESTER, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.attachToProcess();
§§		LOG(INFO) << "attach 3:" << success;

		std::shared_ptr<DebugExecutionOutput> dex = std::make_shared<DebugExecutionOutput>();
		ep.debug(5000,dex,false,false);
§§		LOG(INFO) << "debug 3:" << dex->m_terminationDescription;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//attach to sleeping program - then debug + no kill (long enough timeout)
	{
		std::system((DEBUGGERTESTER + " 3 &").c_str());
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		ExternalProcess ep(DEBUGGERTESTER, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		bool success =ep.attachToProcess();
§§		LOG(INFO) << "attach 4:" << success;

		std::shared_ptr<DebugExecutionOutput> dex = std::make_shared<DebugExecutionOutput>();
		ep.debug(20000,dex,false,false);
§§		LOG(INFO) << "debug 4:" << dex->m_terminationDescription;




		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}


§§	LOG(INFO) << "ATTACH tests completed!";

§§	LOG(INFO) << "Starting CONSOLE Redirect tests!";

	{
		ExternalProcess ep1(CONSOLEREFLECTOR, ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL);
		bool success = ep1.initProcess();
		if(!success){
§§			LOG(ERROR) << "Failed to initialize the target process";
		}

		success = ep1.run();
		if(!success){
§§			LOG(ERROR) << "Failed to run the target process";
		}

§§		std::istream* is = ep1.getStdOutIstream();
§§		std::ostream* os = ep1.getStdInOstream();

		std::string mytestString = "myteststring\n";

		LOG(INFO) << "writing " << mytestString << " to target's stdout"; 

		os->write(mytestString.c_str(), mytestString.length());
		os->flush();


		char buf[100];
		memset(buf, 0, sizeof(buf));

		int nbytesRead = 0;
		while (is->peek() == EOF) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		nbytesRead = is->readsome(buf, sizeof(buf));

		if(nbytesRead != 24){
§§			LOG(ERROR) << "Failed to read the test input from the target's stdout";
		}

		if(std::string(buf) != "myteststringmyteststring"){
			LOG(ERROR)<<"What we read from the target's stdout was not what we expected";
		}
		LOG(INFO)<<"Read the expected string from stdout";

		mytestString = "myteststring2\n";
		os->write(mytestString.c_str(), mytestString.length());
		os->flush();

		LOG(INFO) << "writing " << mytestString << " to target's stdout"; 

		memset(buf, 0, sizeof(buf));
		nbytesRead = 0;
		while (is->peek() == EOF) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		nbytesRead = is->readsome(buf, sizeof(buf));

		if(nbytesRead != 39){ 
			LOG(ERROR)<<"Failed to read the test input from the target's stderr (redirected to stdout)";
		}

		if(std::string(buf) != "myteststring2myteststring2myteststring2"){
			LOG(ERROR)<<"What we read from the target's stdout was not what we expected";
		}
		LOG(INFO)<<"Read the expected string from stderr";

	}

	LOG(INFO) <<"CONSOLE Redirect completed!";

	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
