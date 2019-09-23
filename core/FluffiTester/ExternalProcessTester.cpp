/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Pascal Eckmann
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "ExternalProcess.h"
#include "GarbageCollectorWorker.h"
#include "DebugExecutionOutput.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ExternalProcessTester
{
	TEST_CLASS(ExternalProcessTest)
	{
	public:

		bool IsProcessRunning(DWORD pid)
		{
			HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
			DWORD ret = WaitForSingleObject(process, 0);
			CloseHandle(process);
			return ret == WAIT_TIMEOUT;
		}

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(ExternalProcess_runAndWaitForCompletion)
		{
			ExternalProcess ep1("cmd /c ping localhost -n 2", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
			bool success = ep1.runAndWaitForCompletion(3000);
			Assert::IsTrue(!success, L"A process that was not initialized could be run(1)");
			success = ep1.initProcess();
			Assert::IsTrue(success, L"Error initializing the process (1)");
			success = ep1.runAndWaitForCompletion(3000);
			Assert::IsTrue(success, L"Error running the process (1)");

			success = ep1.runAndWaitForCompletion(3000);
			Assert::IsTrue(!success, L"For some reason a process can be run twice(1)");

			ExternalProcess ep2("cmd /c ping localhost -n 2", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
			success = ep2.initProcess();
			Assert::IsTrue(success, L"Error initializing the process (2)");
			success = ep2.runAndWaitForCompletion(300);
			Assert::IsTrue(!success, L"The target process did not yield the expected timeout(2)");

			{
				GarbageCollectorWorker garbageCollector(0);
				garbageCollector.markFileForDelete("EmptyFile.txt");
			}
			Assert::IsTrue(!std::experimental::filesystem::exists("EmptyFile.txt"), L"Error the expected file was already there (3)");
			ExternalProcess ep3("cmd /c copy NUL EmptyFile.txt", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
			success = ep3.initProcess();
			Assert::IsTrue(success, L"Error initializing the process (3)");
			success = ep3.runAndWaitForCompletion(500);
			Assert::IsTrue(success, L"Error running the process (3)");
			Assert::IsTrue(std::experimental::filesystem::exists("EmptyFile.txt"), L"Error the expected file was not created (3)");
			{
				GarbageCollectorWorker garbageCollector(0);
				garbageCollector.markFileForDelete("EmptyFile.txt");
			}
		}

		TEST_METHOD(ExternalProcess_run)
		{
			{
				GarbageCollectorWorker garbageCollector(0);
				garbageCollector.markFileForDelete("EmptyFile.txt");
			}
			Assert::IsTrue(!std::experimental::filesystem::exists("EmptyFile.txt"), L"Error the expected file was already there (1)");
			ExternalProcess ep1("cmd /c copy NUL EmptyFile.txt", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
			bool success = ep1.run();
			Assert::IsTrue(!success, L"A process that was not initialized could be run(1)");
			success = ep1.initProcess();
			Assert::IsTrue(success, L"Error initializing the process (1)");
			success = ep1.run();
			Assert::IsTrue(success, L"Error running the process (1)");

			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			Assert::IsTrue(std::experimental::filesystem::exists("EmptyFile.txt"), L"Error the expected file was not created (1)");

			{
				GarbageCollectorWorker garbageCollector(0);
				garbageCollector.markFileForDelete("EmptyFile.txt");
			}
		}

		TEST_METHOD(ExternalProcess_addrToRVAString)
		{
			PROCESS_INFORMATION procInfo;
			procInfo.dwProcessId = GetCurrentProcessId();
			procInfo.dwThreadId = GetCurrentThreadId();
			procInfo.hProcess = GetCurrentProcess();
			procInfo.hThread = GetCurrentThread();

§§			HINSTANCE kernel32Handle = LoadLibrary("kernel32.dll");
			SIZE_T llAddress = (SIZE_T)GetProcAddress(kernel32Handle, "LoadLibraryA");
			SIZE_T llRVA = llAddress - (SIZE_T)kernel32Handle;
			std::stringstream oss;
			oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << llRVA;

			std::string rvaString = ExternalProcess::addrToRVAString(procInfo, llAddress);
			Assert::IsTrue(rvaString == "KERNEL32.dll+" + oss.str());

			rvaString = ExternalProcess::addrToRVAString(procInfo, 0x12345678);
			Assert::IsTrue(rvaString == "unknown+0x12345678");
		}

		TEST_METHOD(ExternalProcess_fillInMainThread)
		{
			PROCESS_INFORMATION outerpi{ 0 };

			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(NULL, "ExternalProcessTester.exe 1", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(si.hStdInput);
				CloseHandle(si.hStdOutput);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				std::this_thread::sleep_for(std::chrono::milliseconds(200));

				outerpi.dwProcessId = pi.dwProcessId;
				ExternalProcess::fillInMainThread(&outerpi);

				Assert::IsTrue(pi.dwThreadId == outerpi.dwThreadId, L"pi.dwThreadId != outerpi.dwThreadId");

				ExternalProcess ep("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
				bool success = ep.attachToProcess();
				Assert::IsTrue(success, L"Failed to attach to the ExternalProcessTester.exe");
			}

			std::vector<char> outbytes = Util::readAllBytesFromFile("out.txt");
			outbytes.resize(outbytes.size() + 1);

			int expectedThreadID = std::stoi(&outbytes[0]);

			Assert::IsTrue(expectedThreadID == outerpi.dwThreadId, L"expectedThreadID != outerpi.dwThreadId");

			std::experimental::filesystem::remove("out.txt");
		}

		TEST_METHOD(ExternalProcess_getPIDForProcessName)
		{
			int outerPID = 0;
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(NULL, "ExternalProcessTester.exe 2", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(si.hStdInput);
				CloseHandle(si.hStdOutput);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				std::this_thread::sleep_for(std::chrono::milliseconds(200));

				outerPID = ExternalProcess::getPIDForProcessName("ExternalProcessTester.exe");

				Assert::IsTrue(pi.dwProcessId == outerPID, L"pi.dwProcessId != outerPID");

				ExternalProcess ep("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
				bool success = ep.attachToProcess();
				Assert::IsTrue(success, L"Failed to attach to the ExternalProcessTester.exe");
			}

			std::vector<char> outbytes = Util::readAllBytesFromFile("out.txt");
			outbytes.resize(outbytes.size() + 1);

			int expectedProcessID = std::stoi(&outbytes[0]);

			Assert::IsTrue(expectedProcessID == outerPID, L"expectedProcessID  != outerPID");

			std::experimental::filesystem::remove("out.txt");

			Assert::IsTrue(0 == ExternalProcess::getPIDForProcessName("notexisting.exe"), L"somehow we got a pid for a non existing process.exe");
		}

		TEST_METHOD(ExternalProcess_attachToProcess)
		{
			{
				//check if we can attach to non existing targets
				ExternalProcess ep("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
				bool success = ep.attachToProcess();
				Assert::IsFalse(success, L"Somehow we were able to attach to an inexisting process");

				//check if we can attach to an existing target
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(NULL, "ExternalProcessTester.exe 1", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(si.hStdInput);
				CloseHandle(si.hStdOutput);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				success = ep.attachToProcess();
				Assert::IsTrue(success, L"we weren't able to attach to the external process");
				Assert::IsTrue(IsProcessRunning(ep.getProcessID()), L"the external process we attached to is not running");

				//check if we can kill a target
				ep.die();
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
				Assert::IsFalse(IsProcessRunning(ep.getProcessID()), L"the external process we tried to kill is still running");
			}

			//check for handle leaks
			DWORD preHandles = 0;
			DWORD postHandles = 0;
			GetProcessHandleCount(GetCurrentProcess(), &preHandles);
			for (int i = 0; i < 60; i++) {
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(NULL, "ExternalProcessTester.exe 1", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(si.hStdInput);
				CloseHandle(si.hStdOutput);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				ExternalProcess ep2("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
				bool success = ep2.attachToProcess();

				Assert::IsTrue(ep2.run(), L"Couldn't run the target");
			}
			GetProcessHandleCount(GetCurrentProcess(), &preHandles);
			Assert::IsTrue(preHandles + 30 > postHandles, (L"There seems to be a handle leak:" + std::to_wstring(postHandles - preHandles)).c_str());

			{
				//make sure that all targets were properly killed
				ExternalProcess ep("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
				bool success = ep.attachToProcess();
				Assert::IsFalse(success, L"Apparently not all targets got killed!");
			}

			//check if the target actually starts suspended and if we can continue it (runAndWaitForCompletion / run)
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(NULL, "ExternalProcessTester.exe 3", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(si.hStdInput);
				CloseHandle(si.hStdOutput);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				ExternalProcess ep("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
				bool success = ep.attachToProcess();
				Assert::IsTrue(success, L"could not attach to process!");

				Assert::IsTrue(IsProcessRunning(ep.getProcessID()), L"the external process appears not to be running");
				std::this_thread::sleep_for(std::chrono::milliseconds(2000));
				Assert::IsTrue(IsProcessRunning(ep.getProcessID()), L"the external process stopped although it should have been running");
				Assert::IsTrue(ep.runAndWaitForCompletion(2000), L"The process didn't complete as expected");
				Assert::IsFalse(IsProcessRunning(ep.getProcessID()), L"the external process is somehow still alive!");
			}
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(NULL, "ExternalProcessTester.exe 3", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(si.hStdInput);
				CloseHandle(si.hStdOutput);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				ExternalProcess ep("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
				bool success = ep.attachToProcess();
				Assert::IsTrue(success, L"could not attach to process!");

				Assert::IsTrue(IsProcessRunning(ep.getProcessID()), L"the external process appears not to be running");
				std::this_thread::sleep_for(std::chrono::milliseconds(2000));
				Assert::IsTrue(IsProcessRunning(ep.getProcessID()), L"the external process stopped although it should have been running");
				Assert::IsTrue(ep.run(), L"The process could not be resumed");
				std::this_thread::sleep_for(std::chrono::milliseconds(2000));
				Assert::IsFalse(IsProcessRunning(ep.getProcessID()), L"the external process is somehow still alive!");
			}

			//check if debugging works
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(NULL, "ExternalProcessTester.exe 4", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(si.hStdInput);
				CloseHandle(si.hStdOutput);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				ExternalProcess ep("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
				bool success = ep.attachToProcess();
				Assert::IsTrue(success, L"could not attach to process!");

				Assert::IsTrue(IsProcessRunning(ep.getProcessID()), L"the external process appears not to be running");
§§				std::shared_ptr<DebugExecutionOutput> t = std::make_shared<DebugExecutionOutput>();
				ep.debug(2000, t, false, false);
				Assert::IsTrue(t->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION, L"Access violation was not observed! It was " + t->m_terminationType);
			}

			//check if we can catch an exeption when using dynamorio
			{
				std::string testcaseDir = ".\\extprocTemp";
				Util::createFolderAndParentFolders(testcaseDir);
				{
					STARTUPINFO si;
					PROCESS_INFORMATION pi;

					ZeroMemory(&si, sizeof(si));
					si.cb = sizeof(si);
					ZeroMemory(&pi, sizeof(pi));

					CreateProcess(NULL, "dyndist64\\bin64\\drrun.exe -v -t drcov -dump_binary -logdir extprocTemp -- ExternalProcessTester.exe 4", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
					CloseHandle(si.hStdInput);
					CloseHandle(si.hStdOutput);
					CloseHandle(si.hStdError);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);

					std::this_thread::sleep_for(std::chrono::milliseconds(200));

					ExternalProcess ep("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
					bool success = ep.attachToProcess();
					Assert::IsTrue(success, L"could not attach to process!");

					Assert::IsTrue(IsProcessRunning(ep.getProcessID()), L"the external process appears not to be running");
§§					std::shared_ptr<DebugExecutionOutput> t = std::make_shared<DebugExecutionOutput>();
					ep.debug(3000, t, false, false);
					Assert::IsTrue(t->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION);
				}
				for (auto & p : std::experimental::filesystem::directory_iterator(testcaseDir)) {
					std::experimental::filesystem::remove(p);
				}

				Util::attemptDeletePathRecursive(testcaseDir);
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"testcaseDir could not be deleted.");
			}

			//check if we get coverage when using dynamorio
			{
				std::string testcaseDir = ".\\extprocTemp";
				Util::createFolderAndParentFolders(testcaseDir);
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(NULL, "dyndist64\\bin64\\drrun.exe -v -t drcov -dump_binary -logdir extprocTemp -- ExternalProcessTester.exe 3", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(si.hStdInput);
				CloseHandle(si.hStdOutput);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				std::this_thread::sleep_for(std::chrono::milliseconds(200));

				ExternalProcess ep("ExternalProcessTester.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS);
				bool success = ep.attachToProcess();
				Assert::IsTrue(success, L"could not attach to process!");

				Assert::IsTrue(IsProcessRunning(ep.getProcessID()), L"the external process appears not to be running");
§§				std::shared_ptr<DebugExecutionOutput> t = std::make_shared<DebugExecutionOutput>();
				ep.debug(3000, t, false, false);
				Assert::IsTrue(t->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN);

				for (auto & p : std::experimental::filesystem::directory_iterator(testcaseDir)) {
					Assert::IsTrue(std::experimental::filesystem::file_size(p) > 30000, L"The generated drcov file is too small /empty");
					std::experimental::filesystem::remove(p);
				}

				Util::attemptDeletePathRecursive(testcaseDir);
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"testcaseDir could not be deleted.");
			}

			//check debugging of subprocesses - WONT WORK WITHOUT A LOT OF WORK, AS WINAPI IS NOT MEANT FOR THIS!!!!
			/*
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(NULL, "ExternalProcessTester.exe 5", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(si.hStdInput);
				CloseHandle(si.hStdOutput);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				ExternalProcess ep("ExternalProcessTester.exe", true);
				bool success = ep.attachToProcess();
				Assert::IsTrue(success, L"could not attach to process!");

				Assert::IsTrue(IsProcessRunning(ep.getProcessID()), L"the external process appears not to be running");
§§				std::shared_ptr<DebugExecutionOutput> t = std::make_shared<DebugExecutionOutput>();
				ep.debug(2000, t, false,false);
				Assert::IsTrue(t->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION);
			}*/
		}

		TEST_METHOD(ExternalProcess_testRedirectedIO)
		{
			ExternalProcess ep1("ConsoleReflector.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL);
			bool success = ep1.initProcess();
			Assert::IsTrue(success, L"Failed to initialize the target process");

			success = ep1.run();
			Assert::IsTrue(success, L"Failed to run the target process");

			std::string mytestString = "myteststring\r\n";
			DWORD nBytesWritten = 0;
§§			WriteFile(ep1.getStdInHandle(), mytestString.c_str(), (DWORD)mytestString.length(), &nBytesWritten, NULL);
			Assert::IsTrue(nBytesWritten == mytestString.length(), (std::wstring(L"Failed to write the test input to the target's stdin:") + std::to_wstring(GetLastError())).c_str());

			std::this_thread::sleep_for(std::chrono::milliseconds(200));

			char buf[100];
			memset(buf, 0, sizeof(buf));
			DWORD nBytesRead = 0;
			ReadFile(ep1.getStdOutHandle(), buf, sizeof(buf), &nBytesRead, NULL);
			Assert::IsTrue(nBytesRead == 24, L"Failed to read the test input from the target's stdout");

			Assert::IsTrue(std::string(buf) == "myteststringmyteststring", L"What we read from the target's stdout was not what we expected");

			mytestString = "myteststring2\r\n";
			nBytesWritten = 0;
§§			WriteFile(ep1.getStdInHandle(), mytestString.c_str(), (DWORD)mytestString.length(), &nBytesWritten, NULL);
			Assert::IsTrue(nBytesWritten == mytestString.length(), (std::wstring(L"Failed to write the second test input to the target's stdin:") + std::to_wstring(GetLastError())).c_str());

			std::this_thread::sleep_for(std::chrono::milliseconds(200));

			memset(buf, 0, sizeof(buf));
			nBytesRead = 0;
			ReadFile(ep1.getStdOutHandle(), buf, sizeof(buf), &nBytesRead, NULL);
			Assert::IsTrue(nBytesRead == 39, L"Failed to read the test input from the target's stderr (redirected to stdout)");

			Assert::IsTrue(std::string(buf) == "myteststring2myteststring2myteststring2", L"What we read from the target's stdout was not what we expected");
		}

		TEST_METHOD(ExternalProcess_testRedirectedIO_Stream)
		{
			ExternalProcess ep1("ConsoleReflector.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL);
			bool success = ep1.initProcess();
			Assert::IsTrue(success, L"Failed to initialize the target process");

			success = ep1.run();
			Assert::IsTrue(success, L"Failed to run the target process");

§§			std::istream* is = ep1.getStdOutIstream();
§§			std::ostream* os = ep1.getStdInOstream();

			std::string mytestString = "myteststring\r\n";

			os->write(mytestString.c_str(), mytestString.length());
			os->flush();

			char buf[100];
			memset(buf, 0, sizeof(buf));

			int nbytesRead = 0;
			while (is->peek() == EOF) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			nbytesRead = is->readsome(buf, sizeof(buf));

			Assert::IsTrue(nbytesRead == 24, L"Failed to read the test input from the target's stdout");

			Assert::IsTrue(std::string(buf) == "myteststringmyteststring", L"What we read from the target's stdout was not what we expected");

			mytestString = "myteststring2\r\n";
			os->write(mytestString.c_str(), mytestString.length());
			os->flush();

			memset(buf, 0, sizeof(buf));
			nbytesRead = 0;
			while (is->peek() == EOF) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			nbytesRead = is->readsome(buf, sizeof(buf));

			Assert::IsTrue(nbytesRead == 39, L"Failed to read the test input from the target's stderr (redirected to stdout)");

			Assert::IsTrue(std::string(buf) == "myteststring2myteststring2myteststring2", L"What we read from the target's stdout was not what we expected");
		}

		TEST_METHOD(ExternalProcess_testHandleLeaks)
		{
			//Detect handle leaks when starting target with ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL
			{
				DWORD preHandles = 0;
				DWORD postHandles = 0;
				GetProcessHandleCount(GetCurrentProcess(), &preHandles);

				for (int i = 0; i < 50; i++) {
					ExternalProcess ep1("ConsoleReflector.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL);
					bool success = ep1.initProcess();
					Assert::IsTrue(success, L"Failed to initialize the target process");

					success = ep1.run();
					Assert::IsTrue(success, L"Failed to run the target process");
				}

				//ProcessHandle count is extremely fluctuating. Idea: If there is a leak, it should occour every time ;)
				GetProcessHandleCount(GetCurrentProcess(), &postHandles);
				Assert::IsTrue(preHandles + 25 > postHandles, L"There seems to be a handle leak (1)");
			}

§§			//Detect handle leaks when starting target with ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT
			{
				DWORD preHandles = 0;
				DWORD postHandles = 0;
				GetProcessHandleCount(GetCurrentProcess(), &preHandles);

				for (int i = 0; i < 50; i++) {
					ExternalProcess ep1("ConsoleReflector.exe", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
					bool success = ep1.initProcess();
					Assert::IsTrue(success, L"Failed to initialize the target process");

					success = ep1.run();
					Assert::IsTrue(success, L"Failed to run the target process");
				}

				//ProcessHandle count is extremely fluctuating. Idea: If there is a leak, it should occour every time ;)
				GetProcessHandleCount(GetCurrentProcess(), &postHandles);
				Assert::IsTrue(preHandles + 25 > postHandles, L"There seems to be a handle leak (2)");
			}

			//Detect handle leaks when attaching to target
			{
				DWORD preHandles = 0;
				DWORD postHandles = 0;
				GetProcessHandleCount(GetCurrentProcess(), &preHandles);

				for (int i = 0; i < 50; i++) {
					ExternalProcess ep1("ConsoleReflector.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL);
					//attach doesn't do anything with the handles. So "attaching" is currently equivalent to just calling the constructor and the destructor
				}

				//ProcessHandle count is extremely fluctuating. Idea: If there is a leak, it should occour every time ;)
				GetProcessHandleCount(GetCurrentProcess(), &postHandles);
				Assert::IsTrue(preHandles + 25 > postHandles, L"There seems to be a handle leak (3)");
			}

			//Detect handle leaks when converting the handles to streams
			{
				DWORD preHandles = 0;
				DWORD postHandles = 0;
				GetProcessHandleCount(GetCurrentProcess(), &preHandles);

				for (int i = 0; i < 50; i++) {
					ExternalProcess ep1("ConsoleReflector.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL);
					bool success = ep1.initProcess();
					Assert::IsTrue(success, L"Failed to initialize the target process");

					success = ep1.run();
					Assert::IsTrue(success, L"Failed to run the target process");

§§					std::istream* is = ep1.getStdOutIstream();
§§					std::ostream* os = ep1.getStdInOstream();
				}

				//ProcessHandle count is extremely fluctuating. Idea: If there is a leak, it should occour every time ;)
				GetProcessHandleCount(GetCurrentProcess(), &postHandles);
				Assert::IsTrue(preHandles + 25 > postHandles, L"There seems to be a handle leak (4)");
			}
		}

		TEST_METHOD(ExternalProcess_isInRunningProcessesSet) {
			int processID = 0;
			{
				ExternalProcess ep1("ConsoleReflector.exe", ExternalProcess::CHILD_OUTPUT_TYPE::SPECIAL);
				bool success = ep1.initProcess();
				Assert::IsTrue(success, L"Failed to initialize the target process");

				success = ep1.run();
				processID = ep1.getProcessID();
				Assert::IsTrue(ExternalProcess::isInRunningProcessesSet(processID), L"Process is not in the set of running processes");
			}
			Assert::IsFalse(ExternalProcess::isInRunningProcessesSet(processID), L"Process is still in the set of running processes");
		}
	};
}
