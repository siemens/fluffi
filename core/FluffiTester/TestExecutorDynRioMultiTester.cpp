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
#include "DebugExecutionOutput.h"
#include "Module.h"
#include "TestExecutorDynRioMulti.h"
#include "FluffiServiceDescriptor.h"
#include "FluffiTestcaseID.h"
#include "ExternalProcess.h"
#include "GarbageCollectorWorker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FluffiTester
{
	TEST_CLASS(TestExecutorDynRioMultiTest)
	{
	public:

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		bool IsProcessRunning(DWORD pid)
		{
			HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
			DWORD ret = WaitForSingleObject(process, 0);
			CloseHandle(process);
			return ret == WAIT_TIMEOUT;
		}

§§		bool matchesBlocklist(uint32_t* basicBlocks, int elementsInList, std::shared_ptr<DebugExecutionOutput> exout) {
			std::vector<FluffiBasicBlock>tmp = exout->getCoveredBasicBlocks();
			std::set<FluffiBasicBlock> uniqueCoveredBlocks(tmp.begin(), tmp.end());

			if (elementsInList != uniqueCoveredBlocks.size()) {
				return false;
			}
			for (auto elem : uniqueCoveredBlocks) {
§§				uint32_t* el = std::find(&basicBlocks[0], &basicBlocks[elementsInList], elem.m_rva);
				if (el == &basicBlocks[elementsInList]) {
					return false;
				}
			}
			return true;
		}

		TEST_METHOD(TestExecutorDynRioMulti_isSetupFunctionable)
		{
			std::string cmdline = "InstrumentedDebuggerTester.exe <RANDOM_SHAREDMEM>";
			int hangTimeoutMS = 1000;
			std::set<Module> modulesToCover({ Module("InstrumentedDebuggerTester.exe", "*", 0) });
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir";
			std::string feedercmdline = "SharedMemFeeder.exe";
			int initializationTimeoutMS = 1000;

			Util::createFolderAndParentFolders(testcaseDir);

			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.");
			} //trigger TestExecutorDynRioMulti destructor

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}

		TEST_METHOD(TestExecutorDynRioMulti_attemptStartTargetAndFeeder)
		{
			std::string cmdline = "InstrumentedDebuggerTester.exe <RANDOM_SHAREDMEM>";
			int hangTimeoutMS = 1000;
			std::set<Module> modulesToCover({ Module("InstrumentedDebuggerTester.exe", "*", 0) });
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir";
			std::string feedercmdline = "SharedMemFeeder.exe";

			Util::createFolderAndParentFolders(testcaseDir);

			//trigger timeout
			{
				int initializationTimeoutMS = 0;
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.(1)");

				Assert::IsFalse(multiEx.attemptStartTargetAndFeeder(true), L"The feeder could be initialized although it should have timeouted.(1)");
			} //trigger TestExecutorDynRioMulti destructor

			//normal setup
			{
				int initializationTimeoutMS = 10000;
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.(2)");

				Assert::IsTrue(multiEx.attemptStartTargetAndFeeder(true), L"The feeder could not be initialized although this should have happened.(2)");
			} //trigger TestExecutorDynRioMulti destructor

			//attemptStartTargetAndFeeder 50 times but call desctructor in between
			{
				DWORD preHandles = 0;
				DWORD postHandles = 0;
				int initializationTimeoutMS = 10000;
				GetProcessHandleCount(GetCurrentProcess(), &preHandles);

				for (int i = 0; i < 60; i++) {
					GarbageCollectorWorker garbageCollector(0);
					TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

					Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.(3)");

					Assert::IsTrue(multiEx.attemptStartTargetAndFeeder(true), L"The feeder could not be initialized although this should have happened.(3)");
				}

				//ProcessHandle count is extremely fluctuating. Idea: If there is a leak, it should occour every time ;)
				GetProcessHandleCount(GetCurrentProcess(), &postHandles);
				Assert::IsTrue(preHandles + 30 > postHandles, (L"There seems to be a handle leak(3):" + std::to_wstring(postHandles - preHandles)).c_str());
			}

			//attemptStartTargetAndFeeder 50 times but DO NOT call desctructor
			{
				DWORD preHandles = 0;
				DWORD postHandles = 0;
				int initializationTimeoutMS = 10000;
				GetProcessHandleCount(GetCurrentProcess(), &preHandles);
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.(4)");

				for (int i = 0; i < 80; i++) {
					Assert::IsTrue(multiEx.attemptStartTargetAndFeeder(true), L"The feeder could not be initialized although this should have happened.(4)");
				}

				//ProcessHandle count is extremely fluctuating. Idea: If there is a leak, it should occour every time ;)
				GetProcessHandleCount(GetCurrentProcess(), &postHandles);
				Assert::IsTrue(preHandles + 40 > postHandles, (L"There seems to be a handle leak(4):" + std::to_wstring(postHandles - preHandles)).c_str());
			}

			//without dynamo rio
			{
				int initializationTimeoutMS = 10000;
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.(5)");

				Assert::IsTrue(multiEx.attemptStartTargetAndFeeder(false), L"The feeder could not be initialized although this should have happened.(5)");
			} //trigger TestExecutorDynRioMulti destructor

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}

		TEST_METHOD(TestExecutorDynRioMulti_getDrCovOutputFile)
		{
			std::string cmdline = "InstrumentedDebuggerTester.exe <RANDOM_SHAREDMEM>";
			int hangTimeoutMS = 1000;
			std::set<Module> modulesToCover({ Module("InstrumentedDebuggerTester.exe", "*", 0) });
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir6";
			std::string feedercmdline = "SharedMemFeeder.exe";
			int initializationTimeoutMS = 10000;

			Util::createFolderAndParentFolders(testcaseDir);

			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				{
					std::ofstream fout;
					fout.open(testcaseDir + Util::pathSeperator + "abc.00000.0000.proc.log", std::ios::binary | std::ios::out);
					fout << (char)2;
					fout.close();
				}

				{
					std::ofstream fout;
					fout.open(testcaseDir + Util::pathSeperator + "def.00000.0001.proc.log", std::ios::binary | std::ios::out);
					fout << (char)2;
					fout.close();
				}

				char full[MAX_PATH];
				_fullpath(full, testcaseDir.c_str(), MAX_PATH);
				testcaseDir = std::string(full);

				Assert::IsTrue(multiEx.getDrCovOutputFile(0) == testcaseDir + Util::pathSeperator + "abc.00000.0000.proc.log", L"Testcasefile 0 was not found");
				Assert::IsTrue(multiEx.getDrCovOutputFile(1) == testcaseDir + Util::pathSeperator + "def.00000.0001.proc.log", L"Testcasefile 1 was not found");
			}

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}

		TEST_METHOD(TestExecutorDynRioMulti_execute_CLEAN)
		{
			std::string cmdline = "InstrumentedDebuggerTester.exe <RANDOM_SHAREDMEM>";
			int hangTimeoutMS = 1000;
			std::set<Module> modulesToCover({ Module("InstrumentedDebuggerTester.exe", "*", 0) });
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir2";
			std::string feedercmdline = "SharedMemFeeder.exe";
			int initializationTimeoutMS = 10000;

			Util::createFolderAndParentFolders(testcaseDir);
			int targetProcessID;
			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.");

				FluffiServiceDescriptor sd{ "hap","guid" };
				FluffiTestcaseID tid{ sd,0 };
				std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

				std::ofstream fout;
				fout.open(testcaseFile, std::ios::binary | std::ios::out);
				fout << (char)0;
				fout.close();

§§				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed");
				if (true)
				{
					std::string filename = "tempListOfBlocks.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
						//fout << "0x" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				uint32_t allowedBasicBlocks_1[] = { 0x0001010, 0x0001067, 0x0001073, 0x000108a, 0x0001099, 0x00010aa, 0x00010ca, 0x00010da, 0x00010f6, 0x0001114, 0x000111e, 0x0001127, 0x0001139, 0x0001360, 0x000136a, 0x0001375, 0x000139a, 0x00013a3, 0x00013ad, 0x00013b6, 0x00013c0, 0x00013cd, 0x00013da, 0x00013ef, 0x0001401, 0x0001415, 0x0001421, 0x0001431, 0x0001465, 0x0001473, 0x0001492, 0x000149e, 0x00014ae, 0x0001641, 0x0001683, 0x000168b, 0x00016a1, 0x00016e0, 0x00016e5, 0x00016f8, 0x0001703, 0x0001713, 0x000171c, 0x0001721, 0x0001726, 0x00017bc, 0x00017cf, 0x00017df, 0x00017f3, 0x0001802, 0x000180d, 0x0001818, 0x0001822, 0x0001a20, 0x0001a42, 0x0001a6e, 0x0001a76, 0x0001a97, 0x0001ab0, 0x0001b16, 0x0001b45, 0x0001b4b, 0x0001b53, 0x0001b5d, 0x0001b80, 0x0001bc3, 0x0001be2, 0x0001c14, 0x0001c30, 0x0001c55, 0x0001c63, 0x0001c7f, 0x0001e90, 0x0001ed3, 0x0001ee1, 0x0001ee6, 0x0001eea, 0x0001eef, 0x0001ef4, 0x0001f07, 0x0001f0b, 0x0001f1d, 0x0001f21, 0x0001f4c, 0x0001f81, 0x0001f91, 0x0001fa0, 0x0002008, 0x000201a, 0x0002047, 0x000204e, 0x000205b, 0x0002060, 0x000223b, 0x0002249, 0x0002660, 0x000266c, 0x0002680, 0x000268c, 0x0002890, 0x0002a3e, 0x0002a4c, 0x0002a70, 0x0002aa8, 0x0002aaf, 0x0002ad6, 0x0002b20, 0x0002b30, 0x0002b52, 0x0002b57, 0x0002b6f, 0x0002b86, 0x0002bb0, 0x0002bd8, 0x0002bea, 0x0002bef, 0x0002c00, 0x0002c0e, 0x0002c17, 0x0002c23, 0x0002c27, 0x0002c3f, 0x0002c49, 0x0002c4f, 0x0002c63, 0x0002c6a, 0x0002cb0, 0x0002ce7, 0x0002cf0, 0x0002cf9, 0x0002d14, 0x0002d28, 0x0002d2e, 0x0002d3d, 0x0002d52, 0x0002d73, 0x0002d81, 0x0002da7, 0x0002dab, 0x0002dc3, 0x0002dc8, 0x0002dfc, 0x0002e1c, 0x0002e31, 0x0002e38, 0x0002e3c, 0x0002e45, 0x0002e58, 0x0002e5e, 0x0002e80, 0x0002ecd, 0x0002ee1, 0x0002ee7, 0x0002ef6, 0x0002f0b, 0x0002f2c, 0x0002f3a, 0x0002f71, 0x0002f75, 0x0002f7c, 0x0002f97, 0x0002fa3, 0x0002fd2, 0x0002ff2, 0x0003007, 0x000300e, 0x0003012, 0x000301b, 0x000302d, 0x0003033, 0x0003050, 0x000306b, 0x0003077, 0x0003080, 0x0003090, 0x00030c6, 0x00030e0, 0x0003100, 0x000311c, 0x0003121, 0x000312b, 0x0003139, 0x0003170, 0x0003185, 0x000318d, 0x0003199, 0x00031a9, 0x00034d0, 0x0003508, 0x000359c, 0x00035b0, 0x00035b6, 0x00035c2, 0x00035c7, 0x00035ce, 0x00035d8, 0x0003600, 0x0003605, 0x0003611, 0x000361c, 0x0003624, 0x00036b0, 0x00036e9, 0x000371e, 0x000372b, 0x0003759, 0x000375e, 0x0003777, 0x000379b, 0x00037f4, 0x0003810, 0x0003828, 0x0003840, 0x000385d, 0x000386c, 0x0003874, 0x000387d, 0x00038b7, 0x00038cb, 0x00038d1, 0x00038e0, 0x00038e5, 0x00038ec, 0x0003933, 0x0003938, 0x0003943, 0x000394e, 0x0003956, 0x0003970, 0x0003983, 0x00039cd, 0x00039d2, 0x00039ec, 0x0003a84, 0x0003a97, 0x0003aa1, 0x0003abc, 0x0003ae0, 0x0003aea, 0x0003af6, 0x0003b04, 0x0003b0c, 0x0003b38, 0x0003b3d, 0x0003b42, 0x0004904, 0x000490a, 0x0004916, 0x0004aac, };

				Assert::IsTrue(matchesBlocklist(allowedBasicBlocks_1, sizeof(allowedBasicBlocks_1) / sizeof(allowedBasicBlocks_1[0]), exout), L"Blocks didn't match (1)");

				garbageCollector.markFileForDelete(testcaseFile);

				targetProcessID = multiEx.m_debuggeeProcess->getProcessID();
				Assert::IsTrue(IsProcessRunning(targetProcessID), L"The debuggee target is not running, although it should");
			} //trigger TestExecutorDynRioMulti destructor
			Assert::IsFalse(IsProcessRunning(targetProcessID), L"The debuggee target is still running, although it should have been killed!");

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}

		TEST_METHOD(TestExecutorDynRioMulti_execute_MULTICLEAN)
		{
			std::string cmdline = "InstrumentedDebuggerTester.exe <RANDOM_SHAREDMEM>";
			int hangTimeoutMS = 1000;
			std::set<Module> modulesToCover({ Module("InstrumentedDebuggerTester.exe", "*", 0) });
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir5";
			std::string feedercmdline = "SharedMemFeeder.exe";
			int initializationTimeoutMS = 10000;

			Util::createFolderAndParentFolders(testcaseDir);
			int targetProcessID;
			{
				bool saveBlocksToFile = true;
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.");

				FluffiServiceDescriptor sd{ "hap","guid" };
				FluffiTestcaseID tid1{ sd,1 };
				FluffiTestcaseID tid2{ sd,2 };
				FluffiTestcaseID tid3{ sd,3 };
				FluffiTestcaseID tid4{ sd,4 };
				FluffiTestcaseID tid5{ sd,5 };
				FluffiTestcaseID tid6{ sd,6 };
				std::string testcaseFile1 = Util::generateTestcasePathAndFilename(tid1, testcaseDir);
				std::string testcaseFile2 = Util::generateTestcasePathAndFilename(tid2, testcaseDir);
				std::string testcaseFile3 = Util::generateTestcasePathAndFilename(tid3, testcaseDir);
				std::string testcaseFile4 = Util::generateTestcasePathAndFilename(tid4, testcaseDir);
				std::string testcaseFile5 = Util::generateTestcasePathAndFilename(tid5, testcaseDir);
				std::string testcaseFile6 = Util::generateTestcasePathAndFilename(tid6, testcaseDir);

				uint32_t basicBlocksInitial0_1[] = { 0x0001010, 0x0001067, 0x0001073, 0x000108a, 0x0001099, 0x00010aa, 0x00010ca, 0x00010da, 0x00010f6, 0x0001114, 0x000111e, 0x0001127, 0x0001139, 0x0001360, 0x000136a, 0x0001375, 0x000139a, 0x00013a3, 0x00013ad, 0x00013b6, 0x00013c0, 0x00013cd, 0x00013da, 0x00013ef, 0x0001401, 0x0001415, 0x0001421, 0x0001431, 0x0001465, 0x0001473, 0x0001492, 0x000149e, 0x00014ae, 0x0001641, 0x0001683, 0x000168b, 0x00016a1, 0x00016e0, 0x00016e5, 0x00016f8, 0x0001703, 0x0001713, 0x000171c, 0x0001721, 0x0001726, 0x00017bc, 0x00017cf, 0x00017df, 0x00017f3, 0x0001802, 0x000180d, 0x0001818, 0x0001822, 0x0001a20, 0x0001a42, 0x0001a6e, 0x0001a76, 0x0001a97, 0x0001ab0, 0x0001b16, 0x0001b45, 0x0001b4b, 0x0001b53, 0x0001b5d, 0x0001b80, 0x0001bc3, 0x0001be2, 0x0001c14, 0x0001c30, 0x0001c55, 0x0001c63, 0x0001c7f, 0x0001e90, 0x0001ed3, 0x0001ee1, 0x0001ee6, 0x0001eea, 0x0001eef, 0x0001ef4, 0x0001f07, 0x0001f0b, 0x0001f1d, 0x0001f21, 0x0001f4c, 0x0001f81, 0x0001f91, 0x0001fa0, 0x0002008, 0x000201a, 0x0002047, 0x000204e, 0x000205b, 0x0002060, 0x000223b, 0x0002249, 0x0002660, 0x000266c, 0x0002680, 0x000268c, 0x0002890, 0x0002a3e, 0x0002a4c, 0x0002a70, 0x0002aa8, 0x0002aaf, 0x0002ad6, 0x0002b20, 0x0002b30, 0x0002b52, 0x0002b57, 0x0002b6f, 0x0002b86, 0x0002bb0, 0x0002bd8, 0x0002bea, 0x0002bef, 0x0002c00, 0x0002c0e, 0x0002c17, 0x0002c23, 0x0002c27, 0x0002c3f, 0x0002c49, 0x0002c4f, 0x0002c63, 0x0002c6a, 0x0002cb0, 0x0002ce7, 0x0002cf0, 0x0002cf9, 0x0002d14, 0x0002d28, 0x0002d2e, 0x0002d3d, 0x0002d52, 0x0002d73, 0x0002d81, 0x0002da7, 0x0002dab, 0x0002dc3, 0x0002dc8, 0x0002dfc, 0x0002e1c, 0x0002e31, 0x0002e38, 0x0002e3c, 0x0002e45, 0x0002e58, 0x0002e5e, 0x0002e80, 0x0002ecd, 0x0002ee1, 0x0002ee7, 0x0002ef6, 0x0002f0b, 0x0002f2c, 0x0002f3a, 0x0002f71, 0x0002f75, 0x0002f7c, 0x0002f97, 0x0002fa3, 0x0002fd2, 0x0002ff2, 0x0003007, 0x000300e, 0x0003012, 0x000301b, 0x000302d, 0x0003033, 0x0003050, 0x000306b, 0x0003077, 0x0003080, 0x0003090, 0x00030c6, 0x00030e0, 0x0003100, 0x000311c, 0x0003121, 0x000312b, 0x0003139, 0x0003170, 0x0003185, 0x000318d, 0x0003199, 0x00031a9, 0x00034d0, 0x0003508, 0x000359c, 0x00035b0, 0x00035b6, 0x00035c2, 0x00035c7, 0x00035ce, 0x00035d8, 0x0003600, 0x0003605, 0x0003611, 0x000361c, 0x0003624, 0x00036b0, 0x00036e9, 0x000371e, 0x000372b, 0x0003759, 0x000375e, 0x0003777, 0x000379b, 0x00037f4, 0x0003810, 0x0003828, 0x0003840, 0x000385d, 0x000386c, 0x0003874, 0x000387d, 0x00038b7, 0x00038cb, 0x00038d1, 0x00038e0, 0x00038e5, 0x00038ec, 0x0003933, 0x0003938, 0x0003943, 0x000394e, 0x0003956, 0x0003970, 0x0003983, 0x00039cd, 0x00039d2, 0x00039ec, 0x0003a84, 0x0003a97, 0x0003aa1, 0x0003abc, 0x0003ae0, 0x0003aea, 0x0003af6, 0x0003b04, 0x0003b0c, 0x0003b38, 0x0003b3d, 0x0003b42, 0x0004904, 0x000490a, 0x0004916, 0x0004aac };
				uint32_t basicBlocksInitial0_2[] = { 0x0001010, 0x0001067, 0x0001073, 0x000108a, 0x0001099, 0x00010aa, 0x00010ca, 0x00010da, 0x00010f6, 0x0001114, 0x000111e, 0x0001127, 0x0001139, 0x0001360, 0x000136a, 0x0001375, 0x000139a, 0x00013a3, 0x00013ad, 0x00013b6, 0x00013c0, 0x00013cd, 0x00013da, 0x00013ef, 0x0001401, 0x0001415, 0x0001421, 0x0001431, 0x0001465, 0x0001473, 0x0001492, 0x000149e, 0x00014ae, 0x0001641, 0x0001683, 0x000168b, 0x00016a1, 0x00016e0, 0x00016e5, 0x00016f8, 0x0001703, 0x0001713, 0x000171c, 0x0001721, 0x0001726, 0x00017bc, 0x00017cf, 0x00017df, 0x00017f3, 0x0001802, 0x000180d, 0x0001818, 0x0001822, 0x0001a20, 0x0001a42, 0x0001a6e, 0x0001a76, 0x0001a97, 0x0001ab0, 0x0001b16, 0x0001b45, 0x0001b4b, 0x0001b53, 0x0001b5d, 0x0001b80, 0x0001bc3, 0x0001be2, 0x0001c14, 0x0001c30, 0x0001c55, 0x0001c63, 0x0001c7f, 0x0001e90, 0x0001ed3, 0x0001ee1, 0x0001ee6, 0x0001eea, 0x0001eef, 0x0001ef4, 0x0001f07, 0x0001f0b, 0x0001f1d, 0x0001f21, 0x0001f4c, 0x0001f81, 0x0001f91, 0x0001fa0, 0x0002008, 0x000201a, 0x0002047, 0x000204e, 0x000205b, 0x0002060, 0x000223b, 0x0002249, 0x0002660, 0x000266c, 0x0002680, 0x000268c, 0x0002890, 0x0002a3e, 0x0002a4c, 0x0002a70, 0x0002aa8, 0x0002aaf, 0x0002ad6, 0x0002b20, 0x0002b30, 0x0002b52, 0x0002b57, 0x0002b6f, 0x0002b86, 0x0002bb0, 0x0002bd8, 0x0002bea, 0x0002bef, 0x0002c00, 0x0002c0e, 0x0002c17, 0x0002c23, 0x0002c27, 0x0002c3f, 0x0002c49, 0x0002c4f, 0x0002c63, 0x0002c6a, 0x0002cb0, 0x0002ce7, 0x0002cf0, 0x0002cf9, 0x0002d14, 0x0002d28, 0x0002d2e, 0x0002d3d, 0x0002d52, 0x0002d73, 0x0002d81, 0x0002da7, 0x0002dab, 0x0002dc3, 0x0002dc8, 0x0002dfc, 0x0002e1c, 0x0002e31, 0x0002e38, 0x0002e3c, 0x0002e45, 0x0002e58, 0x0002e5e, 0x0002e80, 0x0002ecd, 0x0002ee1, 0x0002ee7, 0x0002ef6, 0x0002f0b, 0x0002f2c, 0x0002f3a, 0x0002f71, 0x0002f75, 0x0002f7c, 0x0002f97, 0x0002fa3, 0x0002fd2, 0x0002ff2, 0x0003007, 0x000300e, 0x0003012, 0x000301b, 0x000302d, 0x0003033, 0x0003050, 0x000306b, 0x0003077, 0x0003080, 0x0003090, 0x00030c6, 0x00030e0, 0x0003100, 0x000311c, 0x0003121, 0x000312b, 0x0003139, 0x0003170, 0x0003185, 0x000318d, 0x0003199, 0x00031a9, 0x00034d0, 0x0003508, 0x000359c, 0x00035b0, 0x00035b6, 0x00035c2, 0x00035c7, 0x00035ce, 0x00035d8, 0x0003600, 0x0003605, 0x0003611, 0x000361c, 0x0003624, 0x00036b0, 0x00036e9, 0x000371e, 0x000372b, 0x0003759, 0x000375e, 0x0003777, 0x000379b, 0x00037f4, 0x0003810, 0x0003828, 0x0003840, 0x000385d, 0x000386c, 0x00038b7, 0x00038cb, 0x00038d1, 0x00038e0, 0x00038e5, 0x00038ec, 0x0003933, 0x0003938, 0x0003943, 0x000394e, 0x0003956, 0x0003970, 0x0003983, 0x00039cd, 0x00039d2, 0x00039ec, 0x0003a84, 0x0003a97, 0x0003aa1, 0x0003abc, 0x0003ae0, 0x0003aea, 0x0003af6, 0x0003b04, 0x0003b0c, 0x0003b38, 0x0003b3d, 0x0003b42, 0x0004904, 0x000490a, 0x0004916, 0x0004aac };
				uint32_t basicBlocksFurther0_1[] = { 0x0001010, 0x0001067, 0x0001073, 0x000108a, 0x0001099, 0x00010aa, 0x00010ca, 0x00010da, 0x00010f6, 0x0001114, 0x000111e, 0x0001127, 0x0001139, 0x0001390, 0x000139a, 0x00013a3, 0x00013ad, 0x00013b6, 0x00013c0, 0x00013cd, 0x00013da, 0x00013ef, 0x0001401, 0x0001415, 0x0001421, 0x0001431, 0x0001465, 0x0001473, 0x0001492, 0x000149e, 0x00014ae, 0x0001641, 0x0001683, 0x000168b, 0x00016a1, 0x00016e0, 0x00016e5, 0x00016f8, 0x0001703, 0x0001713, 0x000171c, 0x0001721, 0x0001726, 0x00017bc, 0x00017cf, 0x00017df, 0x00017f3, 0x0001802, 0x000180d, 0x0001818, 0x0001822, 0x0001837, 0x0001841, 0x000184c, 0x0001a20, 0x0001a42, 0x0001a6e, 0x0001a76, 0x0001a97, 0x0001ab0, 0x0001b16, 0x0001b45, 0x0001b4b, 0x0001b53, 0x0001b5d, 0x0001b80, 0x0001bc3, 0x0001be2, 0x0001c14, 0x0001c30, 0x0001c55, 0x0001c63, 0x0001c7f, 0x0001e90, 0x0001ed3, 0x0001ee1, 0x0001ee6, 0x0001eea, 0x0001eef, 0x0001ef4, 0x0001f07, 0x0001f0b, 0x0001f1d, 0x0001f21, 0x0001f4c, 0x0001f81, 0x0001f91, 0x0001fa0, 0x0002008, 0x000201a, 0x0002047, 0x000204e, 0x000205b, 0x0002060, 0x000223b, 0x0002249, 0x0002660, 0x000266c, 0x0002680, 0x000268c, 0x0002890, 0x0002a3e, 0x0002a4c, 0x0002a70, 0x0002aa8, 0x0002aaf, 0x0002ad6, 0x0002b20, 0x0002b30, 0x0002b52, 0x0002b57, 0x0002b6f, 0x0002b86, 0x0002bb0, 0x0002bd8, 0x0002bea, 0x0002bef, 0x0002c00, 0x0002c0e, 0x0002c17, 0x0002c23, 0x0002c27, 0x0002c3f, 0x0002c49, 0x0002c4f, 0x0002c63, 0x0002c6a, 0x0002cb0, 0x0002ce7, 0x0002cf0, 0x0002cf9, 0x0002d14, 0x0002d28, 0x0002d2e, 0x0002d3d, 0x0002d52, 0x0002d73, 0x0002d81, 0x0002da7, 0x0002dab, 0x0002dc3, 0x0002dc8, 0x0002dfc, 0x0002e1c, 0x0002e31, 0x0002e38, 0x0002e3c, 0x0002e45, 0x0002e58, 0x0002e5e, 0x0002e80, 0x0002ecd, 0x0002ee1, 0x0002ee7, 0x0002ef6, 0x0002f0b, 0x0002f2c, 0x0002f3a, 0x0002f71, 0x0002f75, 0x0002f7c, 0x0002f97, 0x0002fa3, 0x0002fd2, 0x0002ff2, 0x0003007, 0x000300e, 0x0003012, 0x000301b, 0x000302d, 0x0003033, 0x0003050, 0x000306b, 0x0003077, 0x0003080, 0x0003090, 0x00030c6, 0x00030e0, 0x0003100, 0x000311c, 0x0003121, 0x000318e, 0x0003199, 0x00031a9, 0x00034d0, 0x0003508, 0x000359c, 0x00035b0, 0x00035b6, 0x00035c2, 0x00035c7, 0x00035ce, 0x00035d8, 0x0003600, 0x0003605, 0x0003611, 0x000361c, 0x0003624, 0x00036b0, 0x00036e9, 0x000371e, 0x000372b, 0x0003759, 0x000375e, 0x0003777, 0x000379b, 0x00037f4, 0x0003810, 0x0003828, 0x0003840, 0x000385d, 0x000386c, 0x0003874, 0x000387d, 0x00038b7, 0x00038cb, 0x00038d1, 0x00038e0, 0x00038e5, 0x00038ec, 0x0003933, 0x0003938, 0x0003943, 0x000394e, 0x0003956, 0x0003970, 0x0003983, 0x00039cd, 0x00039d2, 0x00039ec, 0x0003abc, 0x0003ae0, 0x0003aea, 0x0003af6, 0x0003b04, 0x0003b0c, 0x0003b38, 0x0003b3d, 0x0003b42, 0x0004904, 0x000490a, 0x0004916, 0x0004aac };
				uint32_t basicBlocksFurther0_3[] = { 0x0001010, 0x0001067, 0x0001073, 0x000108a, 0x0001099, 0x00010aa, 0x00010ca, 0x00010da, 0x00010f6, 0x0001114, 0x000111e, 0x0001127, 0x0001139, 0x0001390, 0x000139a, 0x00013a3, 0x00013ad, 0x00013b6, 0x00013c0, 0x00013cd, 0x00013da, 0x00013ef, 0x0001401, 0x0001415, 0x0001421, 0x0001431, 0x0001465, 0x0001473, 0x0001492, 0x000149e, 0x00014ae, 0x0001641, 0x0001683, 0x000168b, 0x00016a1, 0x00016e0, 0x00016e5, 0x00016f8, 0x0001703, 0x0001713, 0x000171c, 0x0001721, 0x0001726, 0x00017bc, 0x00017cf, 0x00017df, 0x00017f3, 0x0001802, 0x000180d, 0x0001837, 0x0001841, 0x000184c, 0x0001a20, 0x0001a42, 0x0001a6e, 0x0001a76, 0x0001a97, 0x0001ab0, 0x0001b16, 0x0001b45, 0x0001b4b, 0x0001b53, 0x0001b5d, 0x0001b80, 0x0001bc3, 0x0001be2, 0x0001c14, 0x0001c30, 0x0001c55, 0x0001c63, 0x0001c7f, 0x0001e90, 0x0001ed3, 0x0001ee1, 0x0001ee6, 0x0001eea, 0x0001eef, 0x0001ef4, 0x0001f07, 0x0001f0b, 0x0001f1d, 0x0001f21, 0x0001f4c, 0x0001f81, 0x0001f91, 0x0001fa0, 0x0002008, 0x000201a, 0x0002047, 0x000204e, 0x000205b, 0x0002060, 0x000223b, 0x0002249, 0x0002660, 0x000266c, 0x0002680, 0x000268c, 0x0002890, 0x0002a3e, 0x0002a4c, 0x0002a70, 0x0002aa8, 0x0002aaf, 0x0002ad6, 0x0002b20, 0x0002b30, 0x0002b52, 0x0002b57, 0x0002b6f, 0x0002b86, 0x0002bb0, 0x0002bd8, 0x0002bea, 0x0002bef, 0x0002c00, 0x0002c0e, 0x0002c17, 0x0002c23, 0x0002c27, 0x0002c3f, 0x0002c49, 0x0002c4f, 0x0002c63, 0x0002c6a, 0x0002cb0, 0x0002ce7, 0x0002cf0, 0x0002cf9, 0x0002d14, 0x0002d28, 0x0002d2e, 0x0002d3d, 0x0002d52, 0x0002d73, 0x0002d81, 0x0002da7, 0x0002dab, 0x0002dc3, 0x0002dc8, 0x0002dfc, 0x0002e1c, 0x0002e31, 0x0002e38, 0x0002e3c, 0x0002e45, 0x0002e58, 0x0002e5e, 0x0002e80, 0x0002ecd, 0x0002ee1, 0x0002ee7, 0x0002ef6, 0x0002f0b, 0x0002f2c, 0x0002f3a, 0x0002f71, 0x0002f75, 0x0002f7c, 0x0002f97, 0x0002fa3, 0x0002fd2, 0x0002ff2, 0x0003007, 0x000300e, 0x0003012, 0x000301b, 0x000302d, 0x0003033, 0x0003050, 0x000306b, 0x0003077, 0x0003080, 0x0003090, 0x00030c6, 0x00030e0, 0x0003100, 0x000311c, 0x0003121, 0x000318e, 0x0003199, 0x00031a9, 0x00034d0, 0x0003508, 0x000359c, 0x00035b0, 0x00035b6, 0x00035c2, 0x00035c7, 0x00035ce, 0x00035d8, 0x0003600, 0x0003605, 0x0003611, 0x000361c, 0x0003624, 0x00036b0, 0x00036e9, 0x000371e, 0x000372b, 0x0003759, 0x000375e, 0x0003777, 0x000379b, 0x00037f4, 0x0003810, 0x0003828, 0x0003840, 0x000385d, 0x000386c, 0x0003874, 0x000387d, 0x00038b7, 0x00038cb, 0x00038d1, 0x00038e0, 0x00038e5, 0x00038ec, 0x0003933, 0x0003938, 0x0003943, 0x000394e, 0x0003956, 0x0003970, 0x0003983, 0x00039cd, 0x00039d2, 0x00039ec, 0x0003abc, 0x0003ae0, 0x0003aea, 0x0003af6, 0x0003b04, 0x0003b0c, 0x0003b38, 0x0003b3d, 0x0003b42, 0x0004904, 0x000490a, 0x0004916, 0x0004aac };
				uint32_t basicBlocksFurther0_2[] = { 0 };
				uint32_t basicBlocksFurther0_4[] = { 0 };
				//hello world case
				uint32_t basicBlocksFurther4_1[] = { 0x0001010, 0x0001067, 0x0001073, 0x000108a, 0x0001099, 0x00010aa, 0x00010ca, 0x00010da, 0x00010f6, 0x0001114, 0x000111e, 0x0001127, 0x0001139, 0x0001390, 0x000139a, 0x00013a3, 0x00013ad, 0x00013b6, 0x00013c0, 0x00013cd, 0x00013da, 0x00013ef, 0x0001401, 0x0001415, 0x0001421, 0x0001431, 0x0001465, 0x0001473, 0x0001492, 0x000149e, 0x00014ae, 0x0001641, 0x0001683, 0x000168b, 0x00016a1, 0x00016e0, 0x00016e5, 0x00016f8, 0x0001703, 0x0001713, 0x000171c, 0x0001721, 0x0001726, 0x000172f, 0x0001742, 0x0001752, 0x00017bc, 0x00017cf, 0x00017df, 0x00017f3, 0x0001802, 0x000180d, 0x0001818, 0x0001822, 0x0001837, 0x0001841, 0x000184c, 0x0001a20, 0x0001a42, 0x0001a6e, 0x0001a76, 0x0001a97, 0x0001ab0, 0x0001b16, 0x0001b45, 0x0001b4b, 0x0001b53, 0x0001b5d, 0x0001b80, 0x0001bc3, 0x0001be2, 0x0001c14, 0x0001c30, 0x0001c55, 0x0001c63, 0x0001c7f, 0x0001e90, 0x0001ed3, 0x0001ee1, 0x0001ee6, 0x0001eea, 0x0001eef, 0x0001ef4, 0x0001f07, 0x0001f0b, 0x0001f1d, 0x0001f21, 0x0001f4c, 0x0001f81, 0x0001f91, 0x0001fa0, 0x0002008, 0x000201a, 0x0002047, 0x000204e, 0x000205b, 0x0002060, 0x000223b, 0x0002249, 0x0002660, 0x000266c, 0x0002680, 0x000268c, 0x0002890, 0x0002a3e, 0x0002a4c, 0x0002a70, 0x0002aa8, 0x0002aaf, 0x0002ad6, 0x0002b20, 0x0002b30, 0x0002b52, 0x0002b57, 0x0002b6f, 0x0002b86, 0x0002bb0, 0x0002bd8, 0x0002bea, 0x0002bef, 0x0002c00, 0x0002c0e, 0x0002c17, 0x0002c23, 0x0002c27, 0x0002c3f, 0x0002c49, 0x0002c4f, 0x0002c63, 0x0002c6a, 0x0002cb0, 0x0002ce7, 0x0002cf0, 0x0002cf9, 0x0002d14, 0x0002d28, 0x0002d2e, 0x0002d3d, 0x0002d52, 0x0002d73, 0x0002d81, 0x0002da7, 0x0002dab, 0x0002dc3, 0x0002dc8, 0x0002dfc, 0x0002e1c, 0x0002e31, 0x0002e38, 0x0002e3c, 0x0002e45, 0x0002e58, 0x0002e5e, 0x0002e80, 0x0002ecd, 0x0002ee1, 0x0002ee7, 0x0002ef6, 0x0002f0b, 0x0002f2c, 0x0002f3a, 0x0002f71, 0x0002f75, 0x0002f7c, 0x0002f97, 0x0002fa3, 0x0002fd2, 0x0002ff2, 0x0003007, 0x000300e, 0x0003012, 0x000301b, 0x000302d, 0x0003033, 0x0003050, 0x000306b, 0x0003077, 0x0003080, 0x0003090, 0x00030c6, 0x00030e0, 0x0003100, 0x000311c, 0x0003121, 0x000318e, 0x0003199, 0x00031a9, 0x00034d0, 0x0003508, 0x000359c, 0x00035b0, 0x00035b6, 0x00035c2, 0x00035c7, 0x00035ce, 0x00035d8, 0x0003600, 0x0003605, 0x0003611, 0x000361c, 0x0003624, 0x00036b0, 0x00036e9, 0x000371e, 0x000372b, 0x0003759, 0x000375e, 0x0003777, 0x000379b, 0x00037f4, 0x0003810, 0x0003828, 0x0003840, 0x000385d, 0x000386c, 0x0003874, 0x000387d, 0x00038b7, 0x00038cb, 0x00038d1, 0x00038e0, 0x00038e5, 0x00038ec, 0x0003933, 0x0003938, 0x0003943, 0x000394e, 0x0003956, 0x0003970, 0x0003983, 0x00039cd, 0x00039d2, 0x00039ec, 0x0003abc, 0x0003ae0, 0x0003aea, 0x0003af6, 0x0003b04, 0x0003b0c, 0x0003b38, 0x0003b3d, 0x0003b42, 0x0004904, 0x000490a, 0x0004916, 0x0004aac };
				uint32_t basicBlocksFurther4_2[] = { 0 };
				uint32_t basicBlocksFurther4_3[] = { 0 };
				uint32_t basicBlocksFurther4_4[] = { 0 };

				{
					std::ofstream fout;
					fout.open(testcaseFile1, std::ios::binary | std::ios::out);
					fout << (char)0;
					fout.close();
				}
				{
					std::ofstream fout;
					fout.open(testcaseFile2, std::ios::binary | std::ios::out);
					fout << (char)0;
					fout.close();
				}
				{
					std::ofstream fout;
					fout.open(testcaseFile3, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();
				}
				{
					std::ofstream fout;
					fout.open(testcaseFile4, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();
				}
				{
					std::ofstream fout;
					fout.open(testcaseFile5, std::ios::binary | std::ios::out);
					fout << (char)100;
					fout.close();
				}
				{
					std::ofstream fout;
					fout.open(testcaseFile6, std::ios::binary | std::ios::out);
					fout << (char)2;
					fout.close();
				}

				//1---------------------------------------------------------
§§				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid1, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(1)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks1.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
						//fout << "0x" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksInitial0_1, sizeof(basicBlocksInitial0_1) / sizeof(basicBlocksInitial0_1[0]), exout) || matchesBlocklist(basicBlocksInitial0_2, sizeof(basicBlocksInitial0_2) / sizeof(basicBlocksInitial0_2[0]), exout), L"Blocks didn't match (1)");

				//2
				exout = multiEx.execute(tid1, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(2)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks2.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
						//fout << "0x" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}

				Assert::IsTrue(matchesBlocklist(basicBlocksFurther0_1, sizeof(basicBlocksFurther0_1) / sizeof(basicBlocksFurther0_1[0]), exout) || matchesBlocklist(basicBlocksFurther0_2, sizeof(basicBlocksFurther0_2) / sizeof(basicBlocksFurther0_2[0]), exout) || matchesBlocklist(basicBlocksFurther0_3, sizeof(basicBlocksFurther0_3) / sizeof(basicBlocksFurther0_3[0]), exout) || matchesBlocklist(basicBlocksFurther0_4, sizeof(basicBlocksFurther0_4) / sizeof(basicBlocksFurther0_4[0]), exout), L"Blocks didn't match (2)");

				//3
				exout = multiEx.execute(tid1, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(3)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks3.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksFurther0_1, sizeof(basicBlocksFurther0_1) / sizeof(basicBlocksFurther0_1[0]), exout) || matchesBlocklist(basicBlocksFurther0_2, sizeof(basicBlocksFurther0_2) / sizeof(basicBlocksFurther0_2[0]), exout) || matchesBlocklist(basicBlocksFurther0_3, sizeof(basicBlocksFurther0_3) / sizeof(basicBlocksFurther0_3[0]), exout) || matchesBlocklist(basicBlocksFurther0_4, sizeof(basicBlocksFurther0_4) / sizeof(basicBlocksFurther0_4[0]), exout), L"Blocks didn't match (3)");

				//4
				exout = multiEx.execute(tid2, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(4)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks4.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksFurther0_1, sizeof(basicBlocksFurther0_1) / sizeof(basicBlocksFurther0_1[0]), exout) || matchesBlocklist(basicBlocksFurther0_2, sizeof(basicBlocksFurther0_2) / sizeof(basicBlocksFurther0_2[0]), exout) || matchesBlocklist(basicBlocksFurther0_3, sizeof(basicBlocksFurther0_3) / sizeof(basicBlocksFurther0_3[0]), exout) || matchesBlocklist(basicBlocksFurther0_4, sizeof(basicBlocksFurther0_4) / sizeof(basicBlocksFurther0_4[0]), exout), L"Blocks didn't match (4)");

				//5
				exout = multiEx.execute(tid2, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(5)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks5.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksFurther0_1, sizeof(basicBlocksFurther0_1) / sizeof(basicBlocksFurther0_1[0]), exout) || matchesBlocklist(basicBlocksFurther0_2, sizeof(basicBlocksFurther0_2) / sizeof(basicBlocksFurther0_2[0]), exout) || matchesBlocklist(basicBlocksFurther0_3, sizeof(basicBlocksFurther0_3) / sizeof(basicBlocksFurther0_3[0]), exout) || matchesBlocklist(basicBlocksFurther0_4, sizeof(basicBlocksFurther0_4) / sizeof(basicBlocksFurther0_4[0]), exout), L"Blocks didn't match (5)");

				//6
				exout = multiEx.execute(tid3, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(6)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks6.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
						//fout << "0x" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksFurther4_1, sizeof(basicBlocksFurther4_1) / sizeof(basicBlocksFurther4_1[0]), exout) || matchesBlocklist(basicBlocksFurther4_2, sizeof(basicBlocksFurther4_2) / sizeof(basicBlocksFurther4_2[0]), exout) || matchesBlocklist(basicBlocksFurther4_3, sizeof(basicBlocksFurther4_3) / sizeof(basicBlocksFurther4_3[0]), exout) || matchesBlocklist(basicBlocksFurther4_4, sizeof(basicBlocksFurther4_4) / sizeof(basicBlocksFurther4_4[0]), exout), L"Blocks didn't match (6)");

				//7
				exout = multiEx.execute(tid3, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(7)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks7.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
						//fout << "0x" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksFurther4_1, sizeof(basicBlocksFurther4_1) / sizeof(basicBlocksFurther4_1[0]), exout) || matchesBlocklist(basicBlocksFurther4_2, sizeof(basicBlocksFurther4_2) / sizeof(basicBlocksFurther4_2[0]), exout) || matchesBlocklist(basicBlocksFurther4_3, sizeof(basicBlocksFurther4_3) / sizeof(basicBlocksFurther4_3[0]), exout) || matchesBlocklist(basicBlocksFurther4_4, sizeof(basicBlocksFurther4_4) / sizeof(basicBlocksFurther4_4[0]), exout), L"Blocks didn't match (7)");

				//8
				exout = multiEx.execute(tid4, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(8)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks8.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksFurther4_1, sizeof(basicBlocksFurther4_1) / sizeof(basicBlocksFurther4_1[0]), exout) || matchesBlocklist(basicBlocksFurther4_2, sizeof(basicBlocksFurther4_2) / sizeof(basicBlocksFurther4_2[0]), exout) || matchesBlocklist(basicBlocksFurther4_3, sizeof(basicBlocksFurther4_3) / sizeof(basicBlocksFurther4_3[0]), exout) || matchesBlocklist(basicBlocksFurther4_4, sizeof(basicBlocksFurther4_4) / sizeof(basicBlocksFurther4_4[0]), exout), L"Blocks didn't match (8)");

				//9
				exout = multiEx.execute(tid4, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(9)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks9.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksFurther4_1, sizeof(basicBlocksFurther4_1) / sizeof(basicBlocksFurther4_1[0]), exout) || matchesBlocklist(basicBlocksFurther4_2, sizeof(basicBlocksFurther4_2) / sizeof(basicBlocksFurther4_2[0]), exout) || matchesBlocklist(basicBlocksFurther4_3, sizeof(basicBlocksFurther4_3) / sizeof(basicBlocksFurther4_3[0]), exout) || matchesBlocklist(basicBlocksFurther4_4, sizeof(basicBlocksFurther4_4) / sizeof(basicBlocksFurther4_4[0]), exout), L"Blocks didn't match (9)");

				//10
				exout = multiEx.execute(tid5, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(10)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks10.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksFurther0_1, sizeof(basicBlocksFurther0_1) / sizeof(basicBlocksFurther0_1[0]), exout) || matchesBlocklist(basicBlocksFurther0_2, sizeof(basicBlocksFurther0_2) / sizeof(basicBlocksFurther0_2[0]), exout) || matchesBlocklist(basicBlocksFurther0_3, sizeof(basicBlocksFurther0_3) / sizeof(basicBlocksFurther0_3[0]), exout) || matchesBlocklist(basicBlocksFurther0_4, sizeof(basicBlocksFurther0_4) / sizeof(basicBlocksFurther0_4[0]), exout), L"Blocks didn't match (10)");

				//11
				exout = multiEx.execute(tid5, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(11)");
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks11.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksFurther0_1, sizeof(basicBlocksFurther0_1) / sizeof(basicBlocksFurther0_1[0]), exout) || matchesBlocklist(basicBlocksFurther0_2, sizeof(basicBlocksFurther0_2) / sizeof(basicBlocksFurther0_2[0]), exout) || matchesBlocklist(basicBlocksFurther0_3, sizeof(basicBlocksFurther0_3) / sizeof(basicBlocksFurther0_3[0]), exout) || matchesBlocklist(basicBlocksFurther0_4, sizeof(basicBlocksFurther0_4) / sizeof(basicBlocksFurther0_4[0]), exout), L"Blocks didn't match (11)");

				//12 - crash
				exout = multiEx.execute(tid6, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::EXCEPTION_OTHER, L"The expected crash execution was not observed(12)");

				//13 - start fresh
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
				exout = multiEx.execute(tid1, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, (L"The expected clean execution was not observed(13). What happened was " + converter.from_bytes(exout->m_terminationDescription)).c_str());
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocks13.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}
				Assert::IsTrue(matchesBlocklist(basicBlocksInitial0_1, sizeof(basicBlocksInitial0_1) / sizeof(basicBlocksInitial0_1[0]), exout) || matchesBlocklist(basicBlocksInitial0_2, sizeof(basicBlocksInitial0_2) / sizeof(basicBlocksInitial0_2[0]), exout), L"Blocks didn't match (13)");

				garbageCollector.markFileForDelete(testcaseFile1);
				garbageCollector.markFileForDelete(testcaseFile2);
				garbageCollector.markFileForDelete(testcaseFile3);
				garbageCollector.markFileForDelete(testcaseFile4);
				garbageCollector.markFileForDelete(testcaseFile5);
				garbageCollector.markFileForDelete(testcaseFile6);

				targetProcessID = multiEx.m_debuggeeProcess->getProcessID();
				Assert::IsTrue(IsProcessRunning(targetProcessID), L"The debuggee target is not running, although it should");
			} //trigger TestExecutorDynRioMulti destructor
			Assert::IsFalse(IsProcessRunning(targetProcessID), L"The debuggee target is still running, although it should have been killed!");

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}

		TEST_METHOD(TestExecutorDynRioMulti_execute_HANG)
		{
			std::string cmdline = "InstrumentedDebuggerTester.exe <RANDOM_SHAREDMEM>";
			int hangTimeoutMS = 1000;
			std::set<Module> modulesToCover({ Module("InstrumentedDebuggerTester.exe", "*", 0) });
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir4";
			std::string feedercmdline = "SharedMemFeeder.exe";
			int initializationTimeoutMS = 10000;

			Util::createFolderAndParentFolders(testcaseDir);
			int targetProcessID;
			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.");

				FluffiServiceDescriptor sd{ "hap","guid" };
				FluffiTestcaseID tid{ sd,0 };
				std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

				std::ofstream fout;
				fout.open(testcaseFile, std::ios::binary | std::ios::out);
				fout << (char)3;
				fout.close();

§§				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::TIMEOUT, L"The expected timeout was not detected");

				garbageCollector.markFileForDelete(testcaseFile);

				targetProcessID = multiEx.m_debuggeeProcess->getProcessID();
				Assert::IsTrue(IsProcessRunning(targetProcessID), L"The debuggee target is not running, although it should");
			} //trigger TestExecutorDynRioMulti destructor
			Assert::IsFalse(IsProcessRunning(targetProcessID), L"The debuggee target is still running, although it should have been killed!");

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}

		TEST_METHOD(TestExecutorDynRioMulti_execute_CRASH)
		{
			std::string cmdline = "InstrumentedDebuggerTester.exe <RANDOM_SHAREDMEM>";
			int hangTimeoutMS = 1000;
			std::set<Module> modulesToCover({ Module("InstrumentedDebuggerTester.exe", "*", 0) });
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir3";
			std::string feedercmdline = "SharedMemFeeder.exe";
			int initializationTimeoutMS = 10000;

			Util::createFolderAndParentFolders(testcaseDir);

			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.");

				FluffiServiceDescriptor sd{ "hap","guid" };
				FluffiTestcaseID tid{ sd,0 };
				std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

				std::ofstream fout;
				fout.open(testcaseFile, std::ios::binary | std::ios::out);
				fout << (char)2;
				fout.close();

§§				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::EXCEPTION_OTHER, L"The expected crash was not detected");
				Assert::IsTrue(exout->m_lastCrash == "InstrumentedDebuggerTester.exe+0x000017a4", L"The expected crash address was not the expected one");

				garbageCollector.markFileForDelete(testcaseFile);
			} //trigger TestExecutorDynRioMulti destructor

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}

		TEST_METHOD(TestExecutorDynRioMulti_execute_ACCESSVIOLATION)
		{
			std::string cmdline = "InstrumentedDebuggerTester.exe <RANDOM_SHAREDMEM>";
			int hangTimeoutMS = 1000;
			std::set<Module> modulesToCover({ Module("InstrumentedDebuggerTester.exe", "*", 0) });
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir1";
			std::string feedercmdline = "SharedMemFeeder.exe";
			int initializationTimeoutMS = 10000;

			Util::createFolderAndParentFolders(testcaseDir);

			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.");

				FluffiServiceDescriptor sd{ "hap","guid" };
				FluffiTestcaseID tid{ sd,0 };
				std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

				std::ofstream fout;
				fout.open(testcaseFile, std::ios::binary | std::ios::out);
				fout << (char)1;
				fout.close();

§§				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::EXCEPTION_ACCESSVIOLATION, L"The expected access violation was not detected");
				Assert::IsTrue(exout->m_lastCrash == "InstrumentedDebuggerTester.exe+0x000017b1", L"The expected crash address was not the expected one");

				garbageCollector.markFileForDelete(testcaseFile);
			} //trigger TestExecutorDynRioMulti destructor

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}

		TEST_METHOD(TestExecutorDynRioMulti_setDynamoRioRegistration)
		{
			std::string cmdline = "c:\\windows\\system32\\notepad.exe c:\\windows\\system32\\notepad.exe";
			int hangTimeoutMS = 1000;
			std::set<Module> modulesToCover({});
			std::string testcaseDir = "testtestcaseDir";
			std::string pipeName = "testpipe";
			std::string feedercmdline = "";
§§			std::string errormsg = "";
			int initializationTimeoutMS = 0;
			std::string dynregfileName = std::string(getenv("USERPROFILE")) + "\\dynamorio\\notepad.exe.config64";

			std::experimental::filesystem::remove(dynregfileName);
			Assert::IsFalse(std::experimental::filesystem::exists(dynregfileName), L"The dynamorio registration file already exists and cannot be deleted before we start with the test");

			GarbageCollectorWorker garbageCollector(0);
			TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);

			Assert::IsTrue(multiEx.setDynamoRioRegistration(true, pipeName, &errormsg), L"setDynamoRioRegistration to true failed");

			Assert::IsTrue(std::experimental::filesystem::exists(dynregfileName), L"The dynamorio registration file was not created");

			char full[MAX_PATH];
			_fullpath(full, testcaseDir.c_str(), MAX_PATH);
			testcaseDir = std::string(full);

			std::ifstream infile(dynregfileName);
			std::string line;
			std::getline(infile, line);
			Assert::IsTrue(line == "DYNAMORIO_RUNUNDER=1", L"Line 1 of registration file is not as expected");
			std::getline(infile, line);
			Assert::IsTrue(line == "DYNAMORIO_AUTOINJECT=" + std::experimental::filesystem::current_path().string() + "\\dyndist64\\lib64\\release\\dynamorio.dll", L"Line 2 of registration file is not as expected");
			std::getline(infile, line);
			Assert::IsTrue(line == "DYNAMORIO_LOGDIR=" + std::experimental::filesystem::current_path().string() + "\\dyndist64\\logs", L"Line 3 of registration file is not as expected");
			std::getline(infile, line);
			Assert::IsTrue(line == "DYNAMORIO_OPTIONS=-code_api -probe_api \"-no_follow_children\" \"-nop_initial_bblock\" -client_lib '" + std::experimental::filesystem::current_path().string() + "\\dyndist64\\clients\\lib64\\release\\drcovMulti.dll;0;\"-dump_binary\" \"-logdir\" \"" + testcaseDir + "\" \"-fluffi_pipe\" \"" + pipeName + "\"'", L"Line 4 of registration file is not as expected");
			infile.close();

			Assert::IsTrue(multiEx.setDynamoRioRegistration(false, pipeName, &errormsg), L"setDynamoRioRegistration to false failed");

			Assert::IsFalse(std::experimental::filesystem::exists(dynregfileName), L"The dynamorio registration file already exists and was therefore not deleted by setDynamoRioRegistration(false)");
		}

		TEST_METHOD(TestExecutorDynRioMulti_attachToTarget)
		{
			std::string cmdline = "DebuggerTesterTCP.exe";
			int hangTimeoutMS = 5000;
			std::set<Module> modulesToCover({ Module("DebuggerTesterTCP.exe", "*", 0) });
			std::string testcaseDir = "." + Util::pathSeperator + "testcaseDirMultiAttach";
			std::string feedercmdline = "TCPFeeder.exe 27015";
			std::string startercmdline = "ProcessStarter.exe";
			int initializationTimeoutMS = 10000;
			bool saveBlocksToFile = true;

			uint32_t basicBlocksInitial0_1[] = { 11046,4096,4164,4176,4185,4198,4222,4244,4267,4273,4282,4374,4550,4563,4592,4715,4768,4797,4851,4873,4934,4945,4959,5009,5031,5068,5077,5088,5111,5117,5131,5141,5161,5166,5180,5197,5201,5213,5225,5245,5254,5296,5328,5416,5421,5472,5488,5534,5542,5565,6608,6637,6652,6660,6669,6727,6747,6773,6785,6848,6856,6867,6883,6928,6938,6950 };
			uint32_t basicBlocksInitial0_2[] = { 11046,4096,4164,4176,4185,4198,4222,4244,4267,4273,4282,4374,4550,4563,4592,4959,5009,5031,5068,5077,5088,5111,5117,5131,5141,5161,5166,5180,5197,5201,5213,5225,5245,5254,5296,5328,5416,5421,5472,5488,5534,5542,5565,6608,6637,6652,6660,6669,6727,6747,6773,6785,6848,6856,6867,6883,6928,6938,6950 };
			uint32_t basicBlocksInitial0_3[] = { 11046,4096,4164,4176,4185,4198,4222,4244,4267,4273,4282,4374,4550,4563,4592,4715,4768,4959,5009,5031,5068,5077,5088,5111,5117,5131,5141,5161,5166,5180,5197,5201,5213,5225,5245,5254,5296,5328,5416,5421,5472,5488,5534,5542,5565,6608,6637,6652,6660,6669,6727,6747,6773,6785,6848,6856,6867,6883,6928,6938,6950 };
			uint32_t basicBlocksInitial0_4[] = { 11046,4096,4164,4176,4185,4198,4222,4244,4267,4273,4282,4374,4550,4563,4592,4715,4768,4797,4851,4873,4934,4945,5031,5068,5077,5088,5111,5117,5131,5141,5161,5166,5180,5197,5201,5213,5225,5245,5254,5296,5328,5416,5421,5472,5488,5534,5542,5565,6608,6637,6652,6660,6669,6727,6747,6773,6785,6848,6856,6867,6883,6928,6938,6950 };
			uint32_t basicBlocksInitial0_5[] = { 11046,4096,4164,4176,4185,4198,4222,4244,4267,4273,4282,4374,4550,4563,4592,4715,4768,4797,4851,4959,5009,5031,5068,5077,5088,5111,5117,5131,5141,5161,5166,5180,5197,5201,5213,5225,5245,5254,5296,5328,5416,5421,5472,5488,5534,5542,5565,6608,6637,6652,6660,6669,6727,6747,6773,6785,6848,6856,6867,6883,6928,6938,6950 };

			Util::createFolderAndParentFolders(testcaseDir);

			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, startercmdline, initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Setup for attaching to a target is not functionable. 1) YOU NEED TO RUN THIS TEST AT LEAST ONCE AS ADMINISTRATOR; 2) YOU NEED TO DISABLE SECURE BOOT");

				FluffiServiceDescriptor sd{ "hap","guid" };
				FluffiTestcaseID tid{ sd,0 };
				std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

				std::ofstream fout;
				fout.open(testcaseFile, std::ios::binary | std::ios::out);
				fout << "0";
				fout.close();

				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::CLEAN, std::wstring(L"The expected clean execution was not observed. It was " + std::to_wstring(exout->m_terminationType)).c_str());
				if (saveBlocksToFile)
				{
					std::string filename = "tempListOfBlocksattach.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < exout->getCoveredBasicBlocks().size(); i++) {
						fout << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << exout->getCoveredBasicBlocks().at(i).m_rva << ";" << std::dec << exout->getCoveredBasicBlocks().at(i).m_rva << std::endl;
					}
					fout.close();
				}

				Assert::IsTrue(matchesBlocklist(basicBlocksInitial0_1, sizeof(basicBlocksInitial0_1) / sizeof(basicBlocksInitial0_1[0]), exout) || matchesBlocklist(basicBlocksInitial0_2, sizeof(basicBlocksInitial0_2) / sizeof(basicBlocksInitial0_2[0]), exout) || matchesBlocklist(basicBlocksInitial0_3, sizeof(basicBlocksInitial0_3) / sizeof(basicBlocksInitial0_3[0]), exout) || matchesBlocklist(basicBlocksInitial0_4, sizeof(basicBlocksInitial0_4) / sizeof(basicBlocksInitial0_4[0]), exout) || matchesBlocklist(basicBlocksInitial0_5, sizeof(basicBlocksInitial0_5) / sizeof(basicBlocksInitial0_5[0]), exout), L"Blocks didn't match!");

				garbageCollector.markFileForDelete(testcaseFile);
			}

			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, startercmdline, initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Setup for attaching to a target is not functionable. YOU NEED TO RUN THIS TEST AT LEAST ONCE AS ADMINISTRATOR");

				FluffiServiceDescriptor sd{ "hap","guid" };
				FluffiTestcaseID tid{ sd,0 };
				std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

				std::ofstream fout;
				fout.open(testcaseFile, std::ios::binary | std::ios::out);
				fout << "1";
				fout.close();

				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::EXCEPTION_ACCESSVIOLATION, L"The expected access violation was not detected");
				Assert::IsTrue(exout->m_lastCrash == "DebuggerTesterTCP.exe+0x000011ab", L"The expected crash address was not the expected one");

				garbageCollector.markFileForDelete(testcaseFile);
			}

			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, startercmdline, initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Setup for attaching to a target is not functionable. YOU NEED TO RUN THIS TEST AT LEAST ONCE AS ADMINISTRATOR");

				FluffiServiceDescriptor sd{ "hap","guid" };
				FluffiTestcaseID tid{ sd,0 };
				std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

				std::ofstream fout;
				fout.open(testcaseFile, std::ios::binary | std::ios::out);
				fout << "2";
				fout.close();

				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::EXCEPTION_OTHER, L"The expected exception not detected");
				Assert::IsTrue(exout->m_lastCrash == "DebuggerTesterTCP.exe+0x0000119e", L"The expected crash address was not the expected one");

				garbageCollector.markFileForDelete(testcaseFile);
			}

			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false, feedercmdline, startercmdline, initializationTimeoutMS, nullptr, false, -1);

				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Setup for attaching to a target is not functionable. YOU NEED TO RUN THIS TEST AT LEAST ONCE AS ADMINISTRATOR");

				FluffiServiceDescriptor sd{ "hap","guid" };
				FluffiTestcaseID tid{ sd,0 };
				std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

				std::ofstream fout;
				fout.open(testcaseFile, std::ios::binary | std::ios::out);
				fout << "3";
				fout.close();

				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid, true);
				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::TIMEOUT, L"The expected timeout was not detected");

				garbageCollector.markFileForDelete(testcaseFile);
			}

			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir), L"could not delete testcase directory");
		}

		TEST_METHOD(TestExecutorDynRioMulti_crashReportedByFeeder)
		{
§§			std::string cmdline = "InstrumentedDebuggerTester.exe <RANDOM_SHAREDMEM>";
§§			int hangTimeoutMS = 2000;
§§			std::set<Module> modulesToCover({ Module("InstrumentedDebuggerTester.exe", "*", 0) });
§§			std::string testcaseDir = "." + Util::pathSeperator + "tcdir1";
§§			std::string feedercmdline = "ReportingCrashSharedMemFeeder.exe"; // Always reports a crash
§§			int initializationTimeoutMS = 10000;
§§
§§			Util::createFolderAndParentFolders(testcaseDir);
§§
§§			{
§§				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioMulti multiEx(cmdline, hangTimeoutMS, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT, "", &garbageCollector, false, feedercmdline, "", initializationTimeoutMS, nullptr, false, -1);
§§
§§				Assert::IsTrue(multiEx.isSetupFunctionable(), L"Apparently the fuzzing setup is not working, although it should be.");
§§
§§				FluffiServiceDescriptor sd{ "hap","guid" };
§§				FluffiTestcaseID tid{ sd,0 };
§§				std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);
§§
§§				std::ofstream fout;
§§				fout.open(testcaseFile, std::ios::binary | std::ios::out);
§§				fout << (char)0;
§§				fout.close();
§§
§§				std::shared_ptr<DebugExecutionOutput> exout = multiEx.execute(tid, true);
§§				Assert::IsTrue(exout->m_terminationType == DebugExecutionOutput::EXCEPTION_OTHER, L"The expected crash reported by feeder was not detected");
§§				Assert::IsTrue(exout->m_lastCrash == "FEEDER CLAIMS CRASH", L"The expected crash address was not the expected one");
§§
§§				garbageCollector.markFileForDelete(testcaseFile);
§§			} //trigger TestExecutorDynRioMulti destructor
§§
§§			std::error_code errorcode;
§§			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}
	};
}
