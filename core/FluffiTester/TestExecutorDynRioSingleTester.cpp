/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Pascal Eckmann
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "TestExecutorDynRioSingle.h"
#include "DebugExecutionOutput.h"
#include "GarbageCollectorWorker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FluffiTester
{
	TEST_CLASS(TestExecutorDynRioSingleTest)
	{
	public:

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(TestExecutorDynRioSingle_DebugCommandLine)
		{
			DWORD preHandles = 0;
			DWORD postHandles = 0;
			GetProcessHandleCount(GetCurrentProcess(), &preHandles);
			std::shared_ptr<DebugExecutionOutput> texOut = std::make_shared<DebugExecutionOutput>();
			for (int i = 0; i < 50; i++) {
				TestExecutorDynRioSingle::DebugCommandLine("NotExistant.exe 0", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, 5000, texOut, false, false, "");
				Assert::IsTrue(texOut->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR, L"DebuggerTester.exe 3 did not yield an ERROR");

				TestExecutorDynRioSingle::DebugCommandLine("DebuggerTester.exe 0", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, 5000, texOut, false, false, "");
				Assert::IsTrue(texOut->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN, L"DebuggerTester.exe 3 did not yield a Clean exit");

				TestExecutorDynRioSingle::DebugCommandLine("DebuggerTester.exe 1", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, 5000, texOut, false, false, "");
				Assert::IsTrue(texOut->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION, L"DebuggerTester.exe 3 did not yield an Access violation");

				TestExecutorDynRioSingle::DebugCommandLine("DebuggerTester.exe 2", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, 5000, texOut, false, false, "");
				Assert::IsTrue(texOut->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER, L"DebuggerTester.exe 2 did not yield an Exception");

				TestExecutorDynRioSingle::DebugCommandLine("DebuggerTester.exe 3", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, 1000, texOut, false, false, "");
				Assert::IsTrue(texOut->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::TIMEOUT, L"DebuggerTester.exe 3 did not yield a TIMEOUT");
			}

			//ProcessHandle count is extremely fluctuating. Idea: If there is a leak, it should occour every time ;)
			GetProcessHandleCount(GetCurrentProcess(), &postHandles);
			Assert::IsTrue(preHandles + 25 > postHandles, L"There seems to be a handle leak");

			TestExecutorDynRioSingle::DebugCommandLine("DebuggerTester.exe 1", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, 5000, texOut, true, false, "");
			Assert::IsTrue(texOut->m_lastCrash == "DebuggerTester.exe+0x000011a3", L"The crash location detection did not work.");
		}

		TEST_METHOD(TestExecutorDynRioSingle_copyCoveredModulesToDebugExecutionOutput)
		{
			std::shared_ptr<DebugExecutionOutput> texOut = std::make_shared<DebugExecutionOutput>();
			{
				GarbageCollectorWorker garbageCollector(0);
				TestExecutorDynRioSingle::DebugCommandLine("dyndist64\\bin64\\drrun.exe -v -t drcov -dump_binary -logdir . -- DebuggerTester.exe 0", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, 5000, texOut, false, false, "");
				std::string drCovFilename = TestExecutorDynRioSingle("", 0, std::set<Module>(), ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false).getDrCovOutputFile();

				std::vector<char> drcovOutput = Util::readAllBytesFromFile(drCovFilename);
				std::set<Module> modulesToCover;
				modulesToCover.emplace("DebuggerTester.exe", "*", 1);
				TestExecutorDynRio::copyCoveredModulesToDebugExecutionOutput(&drcovOutput, &modulesToCover, texOut);

				garbageCollector.markFileForDelete(drCovFilename);
			}

			if (true)
			{
				std::string filename = "tempListOfBlocksS.txt";
				std::ofstream fout;
				fout.open(filename, std::ios::binary | std::ios::out);
				for (int i = 0; i < texOut->getCoveredBasicBlocks().size(); i++) {
					fout << "0x" << std::hex << std::setw(8) << std::setfill('0') << texOut->getCoveredBasicBlocks().at(i).m_rva << std::endl;
				}
				fout.close();
			}

			uint32_t allowedBasicBlocks[] = { 0x00001000, 0x00001035, 0x00001058, 0x00001060, 0x00001069, 0x00001073, 0x0000108b, 0x000010a1, 0x000010b8, 0x000010be, 0x000010c7, 0x00001123, 0x00001128, 0x0000112d, 0x000011ae, 0x000011c0, 0x000011cd, 0x000015e0, 0x000015fd, 0x0000160c, 0x00001614, 0x0000161d, 0x00001657, 0x0000166b, 0x00001685, 0x00001691, 0x000016d0, 0x000016d8, 0x000016e3, 0x000016f3, 0x00001720, 0x0000172a, 0x00001736, 0x00001788, 0x00001798, 0x0000179d, 0x000017a4, 0x000017a9, 0x000017b1, 0x000017bd, 0x000017c1, 0x000017c6, 0x000017d2, 0x000017d7, 0x000017de, 0x000017e2, 0x000017e7, 0x000017ec, 0x000017fc, 0x00001801, 0x00001806, 0x0000180b, 0x00001812, 0x00001817, 0x0000181b, 0x00001820, 0x00001825, 0x00001844, 0x0000184d, 0x00001854, 0x0000185d, 0x00001862, 0x00001870, 0x00001889, 0x00001898, 0x000018a5, 0x000018bc, 0x000018c0, 0x000018dd, 0x000018eb, 0x000018fe, 0x00001912, 0x00001919, 0x0000191e, 0x00001949, 0x0000194e, 0x0000196b, 0x00001970, 0x00001978, 0x00001980, 0x0000198d, 0x00001994, 0x00001998, 0x000019e8, 0x000019f1, 0x00001d1c, 0x00001d25, 0x00001d29, 0x00001d3d, 0x00001d4a, 0x00001d58, 0x00001d7a, 0x00001d7f, 0x00001d87, 0x00001d8c, 0x00001d99, 0x00001da4, 0x00001dbc, 0x00001dc1, 0x00001dc5, 0x00001df0, 0x00001f0c, 0x00001f19, 0x00001f1f, 0x00001f23, 0x00001f5c, 0x00001f81, 0x00001f89, 0x00001f9a, 0x00001fac, 0x00001fb5, 0x00001fc4, 0x0000205b, 0x00002070, 0x00002078, 0x00002080, 0x00002090, 0x00002098, 0x000020a0, 0x000020a9, 0x000020b2, 0x000020bc, 0x000020c8, 0x000020d0, 0x00002220, 0x00002224, 0x00002230, 0x0000223c, 0x00002246, 0x00002255, 0x00002260, 0x00002269, 0x00002278, 0x000022c0, 0x000022f5, 0x000022fa, 0x0000230c, 0x00002341, 0x00002346, 0x00002394, 0x0000242f, 0x0000244e, 0x00002455, 0x0000245c, 0x00002481, 0x0000249f, 0x000024af, 0x000024c8, 0x000024da, 0x000024f5, 0x000024fc, 0x00002516, 0x00002535, 0x0000255c, 0x00002568, 0x000025a6, 0x000025b2, 0x000025b8, 0x000025be, 0x000025c4, 0x000025ca, 0x000025d0, 0x000025dc, 0x000025e2, 0x000025e8, 0x00002600, 0x00002606, 0x0000260c, 0x0000261e, 0x00002630, 0x00002716 };
			Assert::IsTrue(sizeof(allowedBasicBlocks) / sizeof(uint32_t) == texOut->getCoveredBasicBlocks().size(), L"The amount of covered blocks does not match");

			for (int i = 0; i < texOut->getCoveredBasicBlocks().size(); i++) {
				uint32_t* el = std::find(std::begin(allowedBasicBlocks), std::end(allowedBasicBlocks), texOut->getCoveredBasicBlocks().at(i).m_rva);
				Assert::IsTrue(el != std::end(allowedBasicBlocks), L" Not all blocks are expected");
			}
		}

		TEST_METHOD(TestExecutorDynRioSingle_getDrCovOutputFile)
		{
			std::string filename = "testfile.log";
			std::ofstream fout;
			fout.open(filename, std::ios::binary | std::ios::out);
			for (int i = 0; i < 256; i++) {
				fout.write((char*)&i, 1);
			}
			fout.close();

			{
				GarbageCollectorWorker garbageCollector(0);
				std::string foundDRCovfile = TestExecutorDynRioSingle("", 0, std::set<Module>(), ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false).getDrCovOutputFile();

				Assert::IsTrue(std::experimental::filesystem::path(foundDRCovfile).filename() == filename, L"The found filename is not the expected one");

				garbageCollector.markFileForDelete(filename);
			}

			Assert::IsFalse(std::experimental::filesystem::exists(filename), L"Could not delete testfile.log");
		}

		TEST_METHOD(TestExecutorDynRioSingle_deleteDRCovFile)
		{
			std::string filename = "testfile.log";
			std::ofstream fout;
			fout.open(filename, std::ios::binary | std::ios::out);
			for (int i = 0; i < 256; i++) {
				fout.write((char*)&i, 1);
			}
			fout.close();

			{
				GarbageCollectorWorker garbageCollector(0);
				garbageCollector.markFileForDelete(filename);
			}

			Assert::IsFalse(std::experimental::filesystem::exists(filename));
		}

		TEST_METHOD(TestExecutorDynRioSingle_isSetupFunctionable)
		{
			GarbageCollectorWorker garbageCollector(0);
			TestExecutorDynRioSingle* te = new TestExecutorDynRioSingle(".\\DebuggerTester.exe 0", 0, std::set<Module>{}, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false);
			Assert::IsTrue(te->isSetupFunctionable());
			delete te;

			te = new TestExecutorDynRioSingle("\".\\DebuggerTester.exe\" 0", 0, std::set<Module>{}, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false);
			Assert::IsTrue(te->isSetupFunctionable());
			delete te;

			te = new TestExecutorDynRioSingle("\".\\wrongpath\\DebuggerTester.exe\" 0", 0, std::set<Module>{}, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false);
			Assert::IsFalse(te->isSetupFunctionable());
			delete te;

			te = new TestExecutorDynRioSingle("\".\\DebuggerTester.exe\" 0", 0, std::set<Module>{}, "\\wrongpath", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, "", &garbageCollector, false);
			Assert::IsFalse(te->isSetupFunctionable());
			delete te;
		}
	};
}
