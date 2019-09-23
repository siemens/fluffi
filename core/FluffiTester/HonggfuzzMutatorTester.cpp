/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "HonggfuzzMutator.h"
#include "FluffiServiceDescriptor.h"
#include "TestcaseDescriptor.h"
#include "GarbageCollectorWorker.h"

#include "mangle.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FluffiTester
{
	TEST_CLASS(HonggfuzzMutatorTest)
	{
	public:

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(HonggfuzzMutator_batchMutate)
		{
			std::string testcaseDir = "." + Util::pathSeperator + "honggfuzztestdir";
			Util::createFolderAndParentFolders(testcaseDir);

			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");
			HonggfuzzMutator mut(selfServiceDescriptor, testcaseDir);

			FluffiTestcaseID parent{ selfServiceDescriptor, 4242 };

			std::ofstream fh;
			std::string parentPathAndFileName = Util::generateTestcasePathAndFilename(parent, testcaseDir);
			fh.open(parentPathAndFileName);
			fh << "asdf";
			fh.close();

			std::deque<TestcaseDescriptor> mutations = mut.batchMutate(3, parent, parentPathAndFileName);
			Assert::IsTrue(mutations.size() == 3, L"The expected amount of testcases was not generated");
			Assert::IsTrue(mutations[0].getparentId().m_localID == 4242 && mutations[0].getparentId().m_serviceDescriptor.m_serviceHostAndPort == "myhap", L"The parent was not correctly set");

			for (int i = 0; i < 3; i++) {
				Assert::IsTrue(std::experimental::filesystem::exists(Util::generateTestcasePathAndFilename(mutations[i].getId(), testcaseDir)), L"The generated file does not exist");
				{
					GarbageCollectorWorker gw(1000);
					mutations[i].deleteFile(&gw);
				}
				Assert::IsFalse(std::experimental::filesystem::exists(Util::generateTestcasePathAndFilename(mutations[i].getId(), testcaseDir)), L"The generated file still exists!");
			}

			{
				GarbageCollectorWorker gw(1000);
				gw.markFileForDelete(parentPathAndFileName);
			}

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
		}

		TEST_METHOD(HonggfuzzMutator_rnd64)
		{
			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");
			int currand = (int)util_rndGet(0, 1);
			while (currand != 0) { //will hang if 0 never shows up
				if (currand != 0 && currand != 1) {
					Assert::Fail(L"Rand produced a number that was not expected");
				}
				currand = (int)util_rndGet(0, 1);
			}
			while (currand != 1) {//will hang if 1 never shows up
				if (currand != 0 && currand != 1) {
					Assert::Fail(L"Rand produced a number that was not expected");
				}
				currand = (int)util_rndGet(0, 1);
			}
		}

		TEST_METHOD(HonggfuzzMutator_mangleContent)
		{
			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");

			std::vector<uint8_t> input = { 1,2,3,4,5,6,7,8,9,0 };

			run_t run;
			run.dynamicFileSz = input.size();
			input.resize(128);
			run.dynamicFile = &input[0];
			run.global.cfg.only_printable = false;
			run.global.mutate.maxFileSz = 128 - 1;
			run.global.mutate.mutationsPerRun = 6;

			//just make sure it doesn't crash ;)
			for (int i = 0; i < 1000; i++) {
				mangle_mangleContent(&run);
			}
		}

		TEST_METHOD(HonggfuzzMutator_mangle_ASCIIVal)
		{
			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");

			std::vector<uint8_t> input = { 1,2,3,4,5,6,7,8,9,0 };

			run_t run;
			run.dynamicFileSz = input.size();
			input.resize(HONGGFUZZ_MAXSIZE);
			run.dynamicFile = &input[0];
			run.global.cfg.only_printable = false;
			run.global.mutate.maxFileSz = HONGGFUZZ_MAXSIZE - 1;
			run.global.mutate.mutationsPerRun = 6;

			//just make sure it doesn't crash ;)
			for (int i = 0; i < 25; i++) {
				mangle_ASCIIVal(&run);
			}
		}

		TEST_METHOD(HonggfuzzMutator_mangle_InsertRndPrintable)
		{
			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");
			std::vector<uint8_t> input = { 1,2,3,4,5,6,7,8,9,0 };

			run_t run;
			run.dynamicFileSz = input.size();
			input.resize(128);
			run.dynamicFile = &input[0];
			run.global.cfg.only_printable = false;
			run.global.mutate.maxFileSz = 128;
			run.global.mutate.mutationsPerRun = 6;

			//just make sure it doesn't crash ;)
			for (int i = 0; i < 25; i++) {
				mangle_InsertRndPrintable(&run);
			}
		}

		TEST_METHOD(HonggfuzzMutator_mangle_InsertRnd)
		{
			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");

			std::vector<uint8_t> input = { 1,2,3,4,5,6,7,8,9,0 };

			run_t run;
			run.dynamicFileSz = input.size();
			input.resize(128);
			run.dynamicFile = &input[0];
			run.global.cfg.only_printable = false;
			run.global.mutate.maxFileSz = 128 - 1;
			run.global.mutate.mutationsPerRun = 6;

			//just make sure it doesn't crash ;)
			for (int i = 0; i < 25; i++) {
				mangle_InsertRnd(&run);
			}
		}

		TEST_METHOD(HonggfuzzMutator_mangle_Shrink)
		{
			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");

			std::vector<uint8_t> input = { 1,2,3,4,5,6,7,8,9,0 };

			run_t run;
			run.dynamicFileSz = input.size();
			input.resize(HONGGFUZZ_MAXSIZE);
			run.dynamicFile = &input[0];
			run.global.cfg.only_printable = false;
			run.global.mutate.maxFileSz = HONGGFUZZ_MAXSIZE - 1;
			run.global.mutate.mutationsPerRun = 6;

			//just make sure it doesn't crash ;)
			for (int i = 0; i < 25; i++) {
				mangle_Shrink(&run);
			}
		}

		TEST_METHOD(HonggfuzzMutator_mangle_Expand)
		{
			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");

			std::vector<uint8_t> input = { 1,2,3,4,5,6,7,8,9,0 };

			run_t run;
			run.dynamicFileSz = input.size();
			input.resize(HONGGFUZZ_MAXSIZE);
			run.dynamicFile = &input[0];
			run.global.cfg.only_printable = false;
			run.global.mutate.maxFileSz = HONGGFUZZ_MAXSIZE - 1;
			run.global.mutate.mutationsPerRun = 6;

			//just make sure it doesn't crash ;)
			for (int i = 0; i < 25; i++) {
				mangle_Expand(&run);
			}
		}

		TEST_METHOD(HonggfuzzMutator_mangle_CloneByte)
		{
			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");

			std::vector<uint8_t> input = { 1,2,3,4,5,6,7,8,9,0 };

			run_t run;
			run.dynamicFileSz = input.size();
			input.resize(HONGGFUZZ_MAXSIZE);
			run.dynamicFile = &input[0];
			run.global.cfg.only_printable = false;
			run.global.mutate.maxFileSz = HONGGFUZZ_MAXSIZE - 1;
			run.global.mutate.mutationsPerRun = 6;

			//just make sure it doesn't crash ;)
			for (int i = 0; i < 25; i++) {
				mangle_CloneByte(&run);
			}
		}
	};
}
