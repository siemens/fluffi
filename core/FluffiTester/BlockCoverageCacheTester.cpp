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

Author(s): Michael Kraus, Thomas Riedmaier, Pascal Eckmann
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "BlockCoverageCache.h"
#include "FluffiBasicBlock.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BlockCoverageCacheTester
{
	TEST_CLASS(BlockCoverageCacheTest)
	{
	public:

		BlockCoverageCache* localBlockCoverageCache = nullptr;

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
			localBlockCoverageCache = new BlockCoverageCache();
		}

		TEST_METHOD_CLEANUP(ModuleCleanup)
		{
			//Freeing the testcase Queue
			delete localBlockCoverageCache;
			localBlockCoverageCache = nullptr;
		}

		TEST_METHOD(BlockCoverageCache_addBlockToCache)
		{
			FluffiBasicBlock fbb1 = FluffiBasicBlock(0, 0);
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb1), L"Error checking if specific BasicBlock is in Cache yet (addBlockToCache)");
			// Test Method
			localBlockCoverageCache->addBlockToCache(fbb1);
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb1), L"Error checking if specific BasicBlock is in Cache yet (addBlockToCache)");

			FluffiBasicBlock fbb2 = FluffiBasicBlock(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max());
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb2), L"Error checking if specific BasicBlock is in Cache yet (addBlockToCache)");
			// Test Method
			localBlockCoverageCache->addBlockToCache(fbb2);
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb2), L"Error checking if specific BasicBlock is in Cache yet (addBlockToCache)");

			FluffiBasicBlock fbb3 = FluffiBasicBlock(std::numeric_limits<uint32_t>::max() + 1, std::numeric_limits<uint32_t>::max());
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb3), L"Error checking if specific BasicBlock is in Cache yet (addBlockToCache)");
			// Test Method
			localBlockCoverageCache->addBlockToCache(fbb3);
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb3), L"Error checking if specific BasicBlock is in Cache yet (addBlockToCache)");

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock fbb = FluffiBasicBlock(i, 100 * i);
				Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (addBlockToCache)");

				localBlockCoverageCache->addBlockToCache(fbb);
				Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (addBlockToCache)");
			}

			//Check64 bit
			FluffiBasicBlock fbb64 = FluffiBasicBlock(0x12345678ab, 0);
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb64), L"Error checking if specific BasicBlock is already in the cache (x64) 1");
			localBlockCoverageCache->addBlockToCache(fbb64);
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb64), L"Error checking if specific BasicBlock is already in the cache (x64) 2");
		}

		TEST_METHOD(BlockCoverageCache_addBlocksToCache)
		{
			std::set<FluffiBasicBlock> basicBlockList;

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock theblock = FluffiBasicBlock(100 * i, i);
				basicBlockList.insert(theblock);

				Assert::IsFalse(localBlockCoverageCache->isBlockInCache(theblock), L"Error checking if specific BasicBlock was correctly added (addBlocksToCache)");
			}

			// Test Method
			localBlockCoverageCache->addBlocksToCache(&basicBlockList);

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock theblock = FluffiBasicBlock(100 * i, i);
				Assert::IsTrue(localBlockCoverageCache->isBlockInCache(theblock), L"Error checking if specific BasicBlock was correctly added (addBlocksToCache)");
			}

			//Check64 bit
			std::set<FluffiBasicBlock> basicBlockList64;
			FluffiBasicBlock fbb64_1 = FluffiBasicBlock(0x12345678ab, 0);
			FluffiBasicBlock fbb64_2 = FluffiBasicBlock(0x12345678ab, 1);
			basicBlockList64.insert(fbb64_1);
			basicBlockList64.insert(fbb64_2);
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb64_1), L"Error checking if specific BasicBlock is already in the cache (x64) 1");
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb64_2), L"Error checking if specific BasicBlock is already in the cache (x64) 2");
			localBlockCoverageCache->addBlocksToCache(&basicBlockList64);
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb64_1), L"Error checking if specific BasicBlock is already in the cache (x64) 3");
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb64_2), L"Error checking if specific BasicBlock is already in the cache (x64) 4");
		}

		TEST_METHOD(BlockCoverageCache_isBlockInCache)
		{
			FluffiBasicBlock fbb1 = FluffiBasicBlock(0, 0);
			// Test Method
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb1), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");
			localBlockCoverageCache->addBlockToCache(fbb1);
			// Test Method
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb1), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock fbb = FluffiBasicBlock(i, 100 * i);
				// Test Method
				Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");
				localBlockCoverageCache->addBlockToCache(fbb);
				// Test Method
				Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");
			}

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock fbb = FluffiBasicBlock(i, (100 * i) + 1);
				// Test Method
				Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");
				localBlockCoverageCache->addBlockToCache(fbb);
				// Test Method
				Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");
			}

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock fbb = FluffiBasicBlock(100 * i, i);
				// Test Method
				Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");
				localBlockCoverageCache->addBlockToCache(fbb);
				// Test Method
				Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");
			}

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock fbb = FluffiBasicBlock(i, i);
				// Test Method
				Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");
				localBlockCoverageCache->addBlockToCache(fbb);
				// Test Method
				Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCache)");
			}

			//Check64 bit
			FluffiBasicBlock fbb64 = FluffiBasicBlock(0x12345678ab, 0);
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb64), L"Error checking if specific BasicBlock is already in the cache (x64) 1");
			localBlockCoverageCache->addBlockToCache(fbb64);
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb64), L"Error checking if specific BasicBlock is already in the cache (x64) 2");
		}

		TEST_METHOD(BlockCoverageCache_isBlockInCacheAndAddItIfNot)
		{
			FluffiBasicBlock fbb1 = FluffiBasicBlock(0, 0);
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb1), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
			// Test Method
			Assert::IsFalse(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb1), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
			// Test Method
			Assert::IsTrue(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb1), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb1), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");

			FluffiBasicBlock fbb2 = FluffiBasicBlock(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max());
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb2), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
			// Test Method
			Assert::IsFalse(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb2), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
			// Test Method
			Assert::IsTrue(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb2), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb2), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");

			FluffiBasicBlock fbb3 = FluffiBasicBlock(std::numeric_limits<uint32_t>::max() + 1, std::numeric_limits<uint32_t>::max());
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb3), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
			// Test Method
			Assert::IsFalse(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb3), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
			// Test Method
			Assert::IsTrue(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb3), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb3), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock fbb = FluffiBasicBlock(i, 100 * i);
				Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
				// Test Method
				Assert::IsFalse(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
				// Test Method
				Assert::IsTrue(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
				Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
			}

			FluffiBasicBlock fbb4 = FluffiBasicBlock(-232323, -4566578);
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb4), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
			// Test Method
			Assert::IsFalse(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb4), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
			// Test Method
			Assert::IsTrue(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb4), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb4), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock fbb5 = FluffiBasicBlock(i, 200 * i);
				Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb5), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
				// Test Method
				Assert::IsFalse(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb5), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
				// Test Method
				Assert::IsTrue(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb5), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
				Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb5), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
			}

			for (int i = 1; i < 100; i++) {
				FluffiBasicBlock fbb6 = FluffiBasicBlock(i, (100 * i) + 1);
				Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb6), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
				// Test Method
				Assert::IsFalse(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb6), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
				// Test Method
				Assert::IsTrue(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb6), L"Error checking if specific BasicBlock is added to Cache yet (isBlockInCacheAndAddItIfNot)");
				Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb6), L"Error checking if specific BasicBlock is in Cache yet (isBlockInCacheAndAddItIfNot)");
			}

			//Check64 bit
			FluffiBasicBlock fbb64 = FluffiBasicBlock(0x12345678ab, 0);
			Assert::IsFalse(localBlockCoverageCache->isBlockInCache(fbb64), L"Error checking if specific BasicBlock is already in the cache (x64) 1");
			Assert::IsFalse(localBlockCoverageCache->isBlockInCacheAndAddItIfNot(fbb64), L"Error checking if specific BasicBlock is already in the cache (x64) 2");
			Assert::IsTrue(localBlockCoverageCache->isBlockInCache(fbb64), L"Error checking if specific BasicBlock is already in the cache (x64) 3");
		}
	};
}
