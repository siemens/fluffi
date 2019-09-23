§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Abian Blome, Thomas Riedmaier
§§*/
§§
§§#include "stdafx.h"
§§#include "CppUnitTest.h"
§§#include "Util.h"
§§#include "FluffiServiceDescriptor.h"
§§#include "RadamsaMutator.h"
§§#include "FluffiTestcaseID.h"
§§#include "TestcaseDescriptor.h"
§§#include "GarbageCollectorWorker.h"
§§
§§using namespace Microsoft::VisualStudio::CppUnitTestFramework;
§§
§§namespace FluffiTester
§§{
§§	TEST_CLASS(RadamsaMutatorTest)
§§	{
§§	public:
§§
§§		TEST_METHOD_INITIALIZE(ModuleInitialize)
§§		{
§§			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
§§		}
§§
§§		TEST_METHOD(RadamsaMutator_RadamsaMutator)
§§		{
§§			std::string tempDir = "c:\\windows\\temp";
§§
§§			// Clean up
§§			for (int i = 0; i < 100; ++i)
§§			{
§§				std::string filename = tempDir + Util::pathSeperator + "blub-" + std::to_string(i);
§§				if (std::experimental::filesystem::exists(filename))
§§				{
§§					std::remove(filename.c_str());
§§				}
§§			}
§§
§§			FluffiServiceDescriptor svc{ "bla", "blub" };
§§			RadamsaMutator rad{ svc, tempDir };
§§
§§			FluffiTestcaseID parent{ svc, 4242 };
§§
§§			std::ofstream fh;
§§			std::string parentPathAndFileName = Util::generateTestcasePathAndFilename(parent, tempDir);
§§			fh.open(parentPathAndFileName);
§§			fh << "asdf";
§§			fh.close();
§§
§§			auto result = rad.batchMutate(0, parent, parentPathAndFileName);
§§			Assert::AreEqual(result.size(), (size_t)0, L"Mutating with size 0 does not return empty vector");
§§
§§			result = rad.batchMutate(1, parent, parentPathAndFileName);
§§			Assert::AreEqual(result.size(), (size_t)1, L"Mutating with size 1 does not return a vector with 1 element");
§§			std::string path = Util::generateTestcasePathAndFilename(result[0].getId(), tempDir);
§§			Assert::IsTrue(std::experimental::filesystem::exists(path), L"Radamsa file does not exist");
§§			{
§§				GarbageCollectorWorker garbageCollector(0);
§§				result[0].deleteFile(&garbageCollector);
§§			}
§§
§§			result = rad.batchMutate(75, parent, parentPathAndFileName);
§§			Assert::AreEqual(result.size(), (size_t)75, L"Mutating with size 75 does not return a vector with 75 elements");
§§			{
§§				GarbageCollectorWorker garbageCollector(0);
§§				for (auto&& it : result)
§§				{
§§					path = Util::generateTestcasePathAndFilename(it.getId(), tempDir);
§§					Assert::IsTrue(std::experimental::filesystem::exists(path), L"Radamsa files do not exist");
§§					it.deleteFile(&garbageCollector);
§§				}
§§			}
§§
§§			// We delete our parent and try again, expecting an exception
§§			std::remove(parentPathAndFileName.c_str());
§§			try
§§			{
§§				rad.batchMutate(1, parent, parentPathAndFileName);
§§				Assert::Fail(L"Mutating non-existing parent did not throw exception");
§§			}
§§			catch (std::runtime_error & e)
§§			{
§§				LOG(ERROR) << "batchMutate failed:" << e.what();
§§			}
§§		}
§§	};
§§}
