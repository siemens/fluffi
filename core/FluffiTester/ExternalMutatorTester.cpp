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
#include "FluffiServiceDescriptor.h"
#include "ExternalMutator.h"
#include "FluffiTestcaseID.h"
#include "TestcaseDescriptor.h"
#include "GarbageCollectorWorker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FluffiTester
{
	TEST_CLASS(ExternalMutatorTest)
	{
	public:

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(ExternalMutator_ExternalMutator)
		{
			std::string tempDir = "c:\\windows\\temp\\temp";
			std::string extDir = "c:\\windows\\temp\\ext\\TEST";

			// Clean up
			Util::attemptDeletePathRecursive(tempDir);
			Util::attemptDeletePathRecursive(extDir);

			Util::createFolderAndParentFolders(tempDir);
			Util::createFolderAndParentFolders(extDir);

			FluffiServiceDescriptor svc{ "bla", "blub" };
			ExternalMutator extMut{ svc, tempDir, extDir };

			FluffiTestcaseID parent{ svc, 4242 };

			auto result = extMut.batchMutate(42, parent, "");
			Assert::AreEqual((size_t)0, result.size(), L"Mutating with 0 files does not return empty vector");

			for (int i = 0; i < 100; ++i)
			{
				std::ofstream fh;
				std::string targetFile = extDir;
				targetFile.append("\\UUID_42_" + std::to_string(i));
				fh.open(targetFile);
				fh << "asdf";
				fh.close();
			}

			result = extMut.batchMutate(0, parent, "");
			Assert::AreEqual((size_t)0, result.size(), L"Mutating with size 0 does not return empty vector");

			result = extMut.batchMutate(1, parent, "");
			Assert::AreEqual((size_t)1, result.size(), L"Mutating with size 1 does not return a vector with 1 element");
			std::string path = Util::generateTestcasePathAndFilename(result[0].getId(), tempDir);
			Assert::IsTrue(std::experimental::filesystem::exists(path), L"External file does not exist");
			{
				GarbageCollectorWorker garbageCollector(0);
				result[0].deleteFile(&garbageCollector);
			}

			result = extMut.batchMutate(75, parent, "");
			Assert::AreEqual(result.size(), (size_t)75, L"Mutating with size 75 does not return a vector with 75 elements");
			{
				GarbageCollectorWorker garbageCollector(0);
				for (auto&& it : result)
				{
					path = Util::generateTestcasePathAndFilename(it.getId(), tempDir);
					Assert::IsTrue(std::experimental::filesystem::exists(path), L"External files do not exist");
					it.deleteFile(&garbageCollector);
				}
			}
		}
	};
}
