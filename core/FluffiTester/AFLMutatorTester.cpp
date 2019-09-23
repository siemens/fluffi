§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier
§§*/
§§
§§#include "stdafx.h"
§§#include "CppUnitTest.h"
§§#include "Util.h"
§§#include "AFLMutator.h"
§§#include "FluffiServiceDescriptor.h"
§§#include "TestcaseDescriptor.h"
§§#include "GarbageCollectorWorker.h"
§§#include "afl-fuzz.h"
§§
§§using namespace Microsoft::VisualStudio::CppUnitTestFramework;
§§
§§namespace FluffiTester
§§{
§§	TEST_CLASS(AFLMutatorTest)
§§	{
§§	public:
§§
§§		TEST_METHOD_INITIALIZE(ModuleInitialize)
§§		{
§§			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
§§		}
§§
§§		TEST_METHOD(AFLMutator_batchMutate)
§§		{
§§			std::string testcaseDir = "." + Util::pathSeperator + "afltestdir";
§§			Util::createFolderAndParentFolders(testcaseDir);
§§
§§			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");
§§			AFLMutator mut(selfServiceDescriptor, testcaseDir);
§§
§§			FluffiTestcaseID parent{ selfServiceDescriptor, 4242 };
§§
§§			std::ofstream fh;
§§			std::string parentPathAndFileName = Util::generateTestcasePathAndFilename(parent, testcaseDir);
§§			fh.open(parentPathAndFileName);
§§			fh << "asdf";
§§			fh.close();
§§
§§			std::deque<TestcaseDescriptor> mutations = mut.batchMutate(3, parent, parentPathAndFileName);
§§			Assert::IsTrue(mutations.size() == 3, L"The expected amount of testcases was not generated");
§§			Assert::IsTrue(mutations[0].getparentId().m_localID == 4242 && mutations[0].getparentId().m_serviceDescriptor.m_serviceHostAndPort == "myhap", L"The parent was not correctly set");
§§
§§			for (int i = 0; i < 3; i++) {
§§				Assert::IsTrue(std::experimental::filesystem::exists(Util::generateTestcasePathAndFilename(mutations[i].getId(), testcaseDir)), L"The generated file does not exist");
§§				{
§§					GarbageCollectorWorker gw(1000);
§§					mutations[i].deleteFile(&gw);
§§				}
§§				Assert::IsFalse(std::experimental::filesystem::exists(Util::generateTestcasePathAndFilename(mutations[i].getId(), testcaseDir)), L"The generated file still exists!");
§§			}
§§
§§			{
§§				GarbageCollectorWorker gw(1000);
§§				gw.markFileForDelete(parentPathAndFileName);
§§			}
§§
§§			std::error_code errorcode;
§§			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode));
§§		}
§§
§§		TEST_METHOD(AFLMutator_SWAP32)
§§		{
§§			uint32_t input = 0x12345678;
§§			Assert::IsTrue(0x78563412 == SWAP32(input));
§§		}
§§
§§		TEST_METHOD(AFLMutator_SWAP16)
§§		{
§§			uint16_t input = 0x1234;
§§			Assert::IsTrue(0x3412 == SWAP16(input));
§§		}
§§
§§		TEST_METHOD(AFLMutator_UR)
§§		{
§§			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");
§§			int currand = UR(2);
§§			while (currand != 0) { //will hang if 0 never shows up
§§				if (currand != 0 && currand != 1) {
§§					Assert::Fail(L"Rand produced a number that was not expected");
§§				}
§§				currand = UR(2);
§§			}
§§			while (currand != 1) {//will hang if 1 never shows up
§§				if (currand != 0 && currand != 1) {
§§					Assert::Fail(L"Rand produced a number that was not expected");
§§				}
§§				currand = UR(2);
§§			}
§§		}
§§
§§		TEST_METHOD(AFLMutator_mutateWithAFLAlgorithm)
§§		{
§§			FluffiServiceDescriptor selfServiceDescriptor("myhap", "myguid");
§§
§§			std::vector<char> input = { 1,2,3,4,5,6,7,8,9,0 };
§§
§§			//just make sure it doesn't crash ;)
§§			for (int i = 0; i < 1000; i++) {
§§				std::vector<char> output = mutateWithAFLHavoc(input);
§§			}
§§		}
§§
§§		TEST_METHOD(FluffiMutator_executeProcess)
§§		{
§§			for (int i = 0; i < 100; ++i)
§§			{
§§				Assert::AreEqual(FluffiMutator::executeProcessAndWaitForCompletion("whoami.exe", 10000), true, L"Error executing whoami");
§§			}
§§			Assert::AreNotEqual(FluffiMutator::executeProcessAndWaitForCompletion("doesnotexistforsure.exe", 10000), true, L"Succesfully executed non-existing file");
§§		}
§§	};
§§}
