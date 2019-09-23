/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Pascal Eckmann, Abian Blome
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "TestExecutorGDB.h"
#include "DebugExecutionOutput.h"
#include "GarbageCollectorWorker.h"
#include "GDBThreadCommunication.h"
#include "SharedMemIPC.h"
#include "FluffiServiceDescriptor.h"
#include "FluffiTestcaseID.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FluffiTester
{
	TEST_CLASS(TestExecutorGDBTest)
	{
	public:

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

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(TestExecutorGDB_isSetupFunctionable)
		{
			std::set<Module> validModulesToCover;
			validModulesToCover.emplace("", "", 1);
			std::set<FluffiBasicBlock> validBlocksToCover;
			FluffiBasicBlock b(1, 1);
			validBlocksToCover.insert(b);

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, validModulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\TcpFeeder.exe", ".\\GDBStarter.exe", 0, 0, validBlocksToCover, 0xcc, 1);
				Assert::IsTrue(te->isSetupFunctionable(), L"The setup that should be valid is somehow not considered to be valid");
				delete te;
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\radamsa.exe", 0, validModulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\TcpFeeder.exe", ".\\GDBStarter.exe", 0, 0, validBlocksToCover, 0xcc, 1);
				Assert::IsFalse(te->isSetupFunctionable(), L"radamsa.exe is somehow recognized as a valid GDB");
				delete te;
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, validModulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\NotExisting.exe", ".\\GDBStarter.exe", 0, 0, validBlocksToCover, 0xcc, 1);
				Assert::IsFalse(te->isSetupFunctionable(), L"The check for the presence of the Feeder fails");
				delete te;
			}
			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, validModulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\TcpFeeder.exe", ".\\NotExisting.exe", 0, 0, validBlocksToCover, 0xcc, 1);
				Assert::IsFalse(te->isSetupFunctionable(), L"The check for the presence of the Starter fails");
				delete te;
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, validModulesToCover, ".\\doesntExist", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\TcpFeeder.exe", ".\\GDBStarter.exe", 0, 0, validBlocksToCover, 0xcc, 1);
				Assert::IsFalse(te->isSetupFunctionable(), L"The check for the presence of the testcase dir fails");
				delete te;
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, validModulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\TcpFeeder.exe", ".\\GDBStarter.exe", 0, 0, std::set<FluffiBasicBlock>{}, 0xcc, 1);
				Assert::IsFalse(te->isSetupFunctionable(), L"The check for empty blocks to cover fails");
				delete te;
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, std::set<Module>{}, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\TcpFeeder.exe", ".\\GDBStarter.exe", 0, 0, validBlocksToCover, 0xcc, 1);
				Assert::IsFalse(te->isSetupFunctionable(), L"The check for non matched blocks to cover fails");
				delete te;
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, std::set<Module>{}, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\TcpFeeder.exe", ".\\GDBStarter.exe", 0, 0, validBlocksToCover, 0xcc, 3);
				Assert::IsFalse(te->isSetupFunctionable(), L"The check for invalid breakpoint instraction fails (1)");
				delete te;
			}
			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, std::set<Module>{}, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\TcpFeeder.exe", ".\\GDBStarter.exe", 0, 0, validBlocksToCover, 0xccc, 1);
				Assert::IsFalse(te->isSetupFunctionable(), L"The check for invalid breakpoint instraction fails (2)");
				delete te;
			}
			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, std::set<Module>{}, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\TcpFeeder.exe", ".\\GDBStarter.exe", 0, 0, validBlocksToCover, 0xccccc, 2);
				Assert::IsFalse(te->isSetupFunctionable(), L"The check for invalid breakpoint instraction fails (3)");
				delete te;
			}
		}

		TEST_METHOD(TestExecutorGDB_attemptStartTargetAndFeeder) {
			std::set<Module> modulesToCover;
			modulesToCover.emplace("InstrumentedDebuggerTester.exe/.text", "*", 0);
			std::set<FluffiBasicBlock> blocksToCover;
			int rva[] = { 0x0010,0x0056,0x0074,0x007a,0x008a,0x00aa,0x00cb,0x010b,0x011f,0x3ae0,0x3aec,0x3af8,0x2970,0x0150,0x09a0,0x0150,0x0180,0x01be,0x01da,0x01ef,0x021e,0x0223,0x0226,0x022f,0x023a,0x0296,0x02a1,0x02b1,0x02b6,0x02bd,0x02c6,0x02cd,0x02d6,0x02dd,0x02e3,0x02ea,0x02ed,0x02f2,0x02fb,0x0333,0x034c,0x0375,0x0390,0x03a3,0x03b6,0x03ef,0x0402,0x0474,0x04b8,0x04e3,0x0509,0x0518,0x0523,0x052e,0x053e,0x0547,0x0554,0x0561,0x056b,0x056e,0x0573,0x0586,0x058b,0x059a,0x05a3,0x05b0,0x05bd,0x05c7,0x05ca,0x05de,0x05e8,0x05f7,0x0600,0x060d,0x061a,0x0624,0x0627,0x062c,0x0641,0x0644,0x0653,0x065c,0x0669,0x0676,0x0680,0x0683,0x0697,0x06a1,0x06b0,0x06b9,0x06c6,0x06d3,0x06dd,0x06e0,0x06e5,0x071c,0x0721,0x0726,0x072f,0x0754,0x079e,0x07b1,0x07bc,0x07f4,0x0803,0x080e,0x0823,0x084c,0x0851,0x0858,0x085f,0x0866,0x086d,0x0874,0x087b,0x0882,0x0889,0x0890,0x0897,0x089e,0x08a5,0x08ac,0x08b3,0x08ba,0x08c1,0x08c8,0x08cf,0x08d6,0x08dd,0x0902,0x0927,0x094d,0x0950,0x095b,0x0966,0x0973,0x3b10,0x3b1e,0x3b2a,0x3b38,0x3b46,0x3b52,0x3b5e,0x3b6a,0x3b78,0x2970,0x09a0,0x09a0,0x09b1,0x09c0,0x09c5,0x09cc,0x09d5,0x09dc,0x09e5,0x09ec,0x09f2,0x09f9,0x09fc,0x0a0e,0x0a20,0x0a42,0x0a4b,0x0a54,0x0a5b,0x0a6e,0x0a79,0x0a9b,0x0ab0,0x0b02,0x0b16,0x0b23,0x0b45,0x0b4b,0x0b54,0x0b5e,0x38e0,0x0b80,0x0bc4,0x0be3,0x0c15,0x0c4e,0x0c55,0x0c58,0x0c68,0x0c7f,0x3b90,0x3ba3,0x3bb8,0x3bbe,0x3bd0,0x3bde,0x16a0,0x0ca0,0x0cc5,0x0cd8,0x0cf0,0x0d03,0x0d11,0x0d22,0x0d2b,0x0d40,0x0d58,0x0d5d,0x0d62,0x0d68,0x0d6b,0x0d75,0x0d91,0x0da0,0x0ddb,0x0de4,0x0dfa,0x0dff,0x0e17,0x0e2d,0x0e57,0x0e72,0x0e90,0x0ec3,0x0ec9,0x0ed0,0x0ed3,0x0ee1,0x0eea,0x0eef,0x0ef4,0x0f0b,0x0f21,0x0f2a,0x0f4c,0x0f66,0x0f81,0x0fa0,0x0fdc,0x0fec,0x1008,0x1012,0x101a,0x1027,0x1047,0x104e,0x1060,0x1068,0x1070,0x1084,0x109a,0x1104,0x1109,0x1112,0x1119,0x111e,0x1130,0x114b,0x1156,0x115e,0x117d,0x1181,0x1187,0x118c,0x11a8,0x11b0,0x11ca,0x11d0,0x11d4,0x11da,0x11e1,0x11eb,0x11f6,0x11fc,0x1203,0x120c,0x1213,0x121c,0x1223,0x1229,0x1230,0x1233,0x123b,0x123d,0x3bf0,0x2970,0x1260,0x1275,0x1284,0x128d,0x1293,0x12a2,0x12af,0x12d0,0x12eb,0x12f4,0x12f9,0x1301,0x1321,0x132d,0x1332,0x1339,0x1348,0x1355,0x1364,0x137c,0x1393,0x13a0,0x13b0,0x13ea,0x13f6,0x1402,0x1412,0x1430,0x143a,0x1442,0x144f,0x146f,0x1476,0x1494,0x14c2,0x14d0,0x1522,0x152b,0x1548,0x1576,0x1584,0x158d,0x1594,0x15a8,0x15ac,0x15b1,0x15d4,0x15dc,0x15df,0x15e5,0x15f1,0x15f7,0x15fe,0x1607,0x160e,0x1617,0x161e,0x1624,0x162b,0x162e,0x1636,0x1638,0x3bf0,0x2970,0x16a0,0x16c6,0x16d3,0x16f5,0x16fb,0x1704,0x1720,0x1748,0x1759,0x1770,0x1789,0x179d,0x17aa,0x17ca,0x17d0,0x17d9,0x17e3,0x17e9,0x17f7,0x38e0,0x1810,0x182d,0x1837,0x184b,0x1852,0x38e0,0x1860,0x186d,0x188f,0x1890,0x18cb,0x18d5,0x18e3,0x18ea,0x1914,0x1920,0x1956,0x195b,0x195d,0x1961,0x197e,0x19af,0x19b5,0x19be,0x19d2,0x19df,0x19e3,0x19e5,0x19eb,0x19f7,0x19fd,0x1a04,0x1a0d,0x1a14,0x1a1d,0x1a24,0x1a2a,0x1a31,0x1a34,0x1a39,0x1a3e,0x1a40,0x3c00,0x2970,0x1a70,0x1aaf,0x1afd,0x1b30,0x1b4c,0x1b52,0x1b75,0x1bb0,0x1bd8,0x1bef,0x1c0f,0x1c27,0x1c31,0x1c3f,0x1c49,0x1c54,0x1c63,0x1c68,0x1c6a,0x38e0,0x3c10,0x22e0,0x1c80,0x1ca1,0x1ca8,0x1cb0,0x1ce2,0x1ce7,0x1cf0,0x1cf9,0x1d0a,0x1d0f,0x1d14,0x1d16,0x1d28,0x1d2f,0x1d3d,0x1d47,0x1d4c,0x1d52,0x1d65,0x1d69,0x1d73,0x1d81,0x1d86,0x1da2,0x1da7,0x1dab,0x1dc8,0x1dcd,0x1de9,0x1dee,0x1df3,0x1df8,0x1dfc,0x1e0c,0x1e0e,0x1e1c,0x1e32,0x1e3c,0x1e46,0x1e58,0x1e5f,0x38e0,0x3c20,0x3c2c,0x3c38,0x3c45,0x3c62,0x1c80,0x1810,0x1e80,0x1ec3,0x1ec8,0x1ecd,0x1ecf,0x1ee1,0x1ee8,0x1ef6,0x1f00,0x1f05,0x1f0b,0x1f1e,0x1f22,0x1f2c,0x1f3a,0x1f40,0x1f45,0x1f61,0x1f6c,0x1f71,0x1f75,0x1f7c,0x1f7f,0x1f9c,0x1fa3,0x1fa8,0x1fc4,0x1fc9,0x1fce,0x1fd2,0x1fe2,0x1fe4,0x1ff2,0x2008,0x2012,0x201c,0x202d,0x2034,0x38e0,0x3c70,0x3c7c,0x3c88,0x3c95,0x3cb2,0x1c80,0x1810,0x2050,0x2090,0x20c7,0x20ed,0x20fe,0x2100,0x2102,0x2108,0x2114,0x211c,0x2121,0x212b,0x213f,0x2170,0x218e,0x3cc0,0x21bc,0x1720,0x21d0,0x21f0,0x2218,0x2225,0x2240,0x2280,0x22a0,0x22e0,0x22f6,0x2302,0x2311,0x2320,0x2342,0x2350,0x2362,0x2368,0x2376,0x237c,0x238d,0x2397,0x239e,0x23a7,0x23ac,0x23b7,0x23cd,0x23e0,0x23e9,0x23f0,0x23f5,0x23f8,0x2407,0x2412,0x2417,0x241a,0x241e,0x2430,0x2446,0x2455,0x2469,0x247a,0x247f,0x2486,0x2489,0x248e,0x249b,0x24a6,0x24bb,0x24c2,0x24d0,0x24fa,0x2508,0x251b,0x2525,0x2533,0x253e,0x2541,0x254e,0x2561,0x256c,0x2575,0x257c,0x2581,0x2584,0x258d,0x259c,0x25a2,0x25b0,0x25b6,0x25c7,0x25ce,0x25d1,0x25d8,0x25dd,0x25e2,0x25ed,0x25f5,0x25fd,0x2600,0x2605,0x2611,0x261c,0x2621,0x2624,0x2628,0x2650,0x2659,0x2662,0x266b,0x2674,0x267b,0x2694,0x26b0,0x26e4,0x26e9,0x2708,0x271a,0x271e,0x2727,0x272b,0x2734,0x273d,0x2743,0x2759,0x2761,0x2763,0x2777,0x277c,0x2783,0x2788,0x278b,0x2790,0x279b,0x27a5,0x27b3,0x27b8,0x27bf,0x27c8,0x27cf,0x27d8,0x27df,0x27e5,0x27ec,0x27ef,0x27f4,0x280b,0x2810,0x2813,0x2828,0x282b,0x3cd0,0x3cdd,0x3ce5,0x3cf1,0x3cff,0x3d0d,0x3d1b,0x3d27,0x3d2e,0x3d36,0x3d3b,0x3d42,0x3d4b,0x3d52,0x3d5b,0x3d62,0x3d68,0x3d6f,0x3d72,0x3d77,0x3d8e,0x3d91,0x2840,0x285d,0x2867,0x286c,0x286f,0x2874,0x287a,0x287d,0x2886,0x288c,0x2891,0x2894,0x28b7,0x28bd,0x28cb,0x28d1,0x28e5,0x28ec,0x28f1,0x28f6,0x2901,0x291a,0x2930,0x2933,0x2938,0x2943,0x294e,0x2953,0x2956,0x295a,0x24d0,0x2970,0x2983,0x2991,0x2996,0x299d,0x29a6,0x29ad,0x29b6,0x29bd,0x29c3,0x29ca,0x29cd,0x29d2,0x29e9,0x29ec,0x29f8,0x2a19,0x2a3b,0x2a55,0x2a5b,0x2a69,0x38e0,0x2a84,0x2aa1,0x2aaf,0x2abc,0x2ae0,0x2aea,0x2af6,0x2af8,0x2afc,0x30d4,0x2b04,0x390a,0x2b0c,0x2b17,0x2b23,0x2b29,0x2b2e,0x2b30,0x2b35,0x2b38,0x2b42,0x2b48,0x2b04,0x2b50,0x2b68,0x2b72,0x2b7c,0x2bb5,0x2bd6,0x2be4,0x2bf0,0x2c0f,0x2c14,0x2c21,0x2c2c,0x2c38,0x2c48,0x3982,0x2c64,0x2c81,0x2c8c,0x2c94,0x2ca6,0x2cb0,0x2cb4,0x2cd5,0x2cdf,0x2cfe,0x2d06,0x2d1b,0x2d27,0x2d3d,0x2d4b,0x2d57,0x2d5f,0x2d8c,0x2d93,0x2d98,0x2d9d,0x2daa,0x2db5,0x2dbd,0x2dc4,0x2dc9,0x2dcb,0x3d9e,0x3cff,0x2ddc,0x2c64,0x2df0,0x2dfd,0x2e0c,0x2e11,0x2e1e,0x2e20,0x2e25,0x2e2c,0x2e57,0x2e5b,0x2e64,0x2e6d,0x2e6f,0x2e78,0x2e90,0x2e99,0x2e9d,0x2ead,0x2eb1,0x2ec4,0x2f2b,0x2f36,0x2f44,0x2f4b,0x2f59,0x2f73,0x2f7e,0x2f98,0x2fa1,0x2fa9,0x2fb3,0x2fb9,0x2fbb,0x2fc0,0x2fc4,0x2fca,0x2fce,0x2fd2,0x2fd6,0x2fd8,0x3dbc,0x2fe0,0x2ff3,0x2ff7,0x2ffe,0x3004,0x3015,0x3019,0x3027,0x3030,0x3055,0x305f,0x306e,0x3080,0x3098,0x30a0,0x30d4,0x30eb,0x30f2,0x31a8,0x31c2,0x31d8,0x3211,0x321c,0x325c,0x327c,0x32bc,0x32dc,0x38ec,0x32f0,0x3317,0x3324,0x3334,0x3354,0x3374,0x339c,0x340b,0x3420,0x3428,0x3430,0x3434,0x3444,0x3448,0x3450,0x3458,0x3474,0x3480,0x3488,0x3490,0x34b5,0x34b9,0x34fa,0x3536,0x35ba,0x35c4,0x35d8,0x35ec,0x35f0,0x35fa,0x3609,0x3614,0x361d,0x3627,0x362c,0x363c,0x364b,0x3651,0x365f,0x3667,0x366e,0x3674,0x3693,0x369b,0x36a5,0x36a9,0x36ae,0x36c0,0x36df,0x36e7,0x36f1,0x36f5,0x36fa,0x370c,0x37a7,0x37c6,0x37cd,0x37d4,0x37de,0x37ee,0x37f9,0x37fe,0x380c,0x3817,0x3827,0x3840,0x384b,0x3852,0x386d,0x3874,0x388e,0x38ad,0x38c0,0x38d4,0x38e6,0x38ec,0x38f2,0x38f8,0x38fe,0x3904,0x390a,0x3910,0x3916,0x391c,0x3922,0x3928,0x392e,0x3934,0x393a,0x3940,0x3946,0x394c,0x3952,0x3958,0x395e,0x3964,0x396a,0x3970,0x3976,0x397c,0x3982,0x3988,0x398e,0x3994,0x399a,0x39a0,0x39a6,0x39ac,0x39b0,0x39d0,0x39e8,0x39fb,0x3a13,0x3a1d,0x2ae0,0x3a2c,0x3a7f,0x3a90,0x3aac,0x3ad0,0x3dd4,0x3dda,0x3dee,0x3dfa };
			for (int i = 0; i < sizeof(rva) / sizeof(rva[0]); i++) {
				FluffiBasicBlock b(rva[i], 0);
				blocksToCover.insert(b);
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 6000, modulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\SharedMemFeeder.exe", ".\\GDBStarter.exe --local \".\\InstrumentedDebuggerTester.exe FLUFFI_SharedMemForTest\"", 6000, 0, blocksToCover, 0xcc, 1);
				Assert::IsTrue(te->isSetupFunctionable(), L"The setup that should be valid is somehow not considered to be valid");
				Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened");
				delete te;
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 0, modulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\SharedMemFeeder.exe", ".\\GDBStarter.exe --local \".\\InstrumentedDebuggerTester.exe FLUFFI_SharedMemForTest\"", 0, 0, blocksToCover, 0xcc, 1);
				Assert::IsTrue(te->isSetupFunctionable(), L"The setup that should be valid is somehow not considered to be valid");
§§				Assert::IsFalse(te->attemptStartTargetAndFeeder(), L"Erroneously succeeded to start target and feeder, although this should have timed out");
				delete te;
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 6000, modulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\gdb.exe", ".\\GDBStarter.exe --local \".\\InstrumentedDebuggerTester.exe FLUFFI_SharedMemForTest\"", 6000, 0, blocksToCover, 0xcc, 1);
				Assert::IsTrue(te->isSetupFunctionable(), L"The setup that should be valid is somehow not considered to be valid");
§§				Assert::IsFalse(te->attemptStartTargetAndFeeder(), L"Erroneously succeeded to start target and feeder, although the Feeder should be invalid");
				delete te;
			}

			{
				GarbageCollectorWorker garbageCollector(0);
§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 6000, modulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\SharedMemFeeder.exe", ".\\gdb.exe", 6000, 0, blocksToCover, 0xcc, 1);
				Assert::IsTrue(te->isSetupFunctionable(), L"The setup that should be valid is somehow not considered to be valid");
§§				Assert::IsFalse(te->attemptStartTargetAndFeeder(), L"Erroneously succeeded to start target and feeder, although the Starter should be invalid");
				delete te;
			}

			bool runSlowHandleLeakChecks = false;

			if (runSlowHandleLeakChecks) {
				//attemptStartTargetAndFeeder 50 times but call desctructor in between
				{
					DWORD preHandles = 0;
					DWORD postHandles = 0;
					int initializationTimeoutMS = 10000;
					GetProcessHandleCount(GetCurrentProcess(), &preHandles);

					for (int i = 0; i < 60; i++) {
						GarbageCollectorWorker garbageCollector(0);
§§						TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 6000, modulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\SharedMemFeeder.exe", ".\\GDBStarter.exe --local \".\\InstrumentedDebuggerTester.exe FLUFFI_SharedMemForTest\"", 6000, 0, blocksToCover, 0xcc, 1);
						Assert::IsTrue(te->isSetupFunctionable(), L"The setup that should be valid is somehow not considered to be valid(loop1)");
						Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened (loop1)");
						delete te;
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
§§					TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 6000, modulesToCover, ".", ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\SharedMemFeeder.exe", ".\\GDBStarter.exe --local \".\\InstrumentedDebuggerTester.exe FLUFFI_SharedMemForTest\"", 6000, 0, blocksToCover, 0xcc, 1);
					Assert::IsTrue(te->isSetupFunctionable(), L"The setup that should be valid is somehow not considered to be valid(loop2)");

					for (int i = 0; i < 60; i++) {
						Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened (loop2)");
					}

					//ProcessHandle count is extremely fluctuating. Idea: If there is a leak, it should occour every time ;)
					GetProcessHandleCount(GetCurrentProcess(), &postHandles);
					Assert::IsTrue(preHandles + 30 > postHandles, (L"There seems to be a handle leak(4):" + std::to_wstring(postHandles - preHandles)).c_str());

					delete te;
				}
			}
		}

		TEST_METHOD(TestExecutorGDB_getBaseAddressesForModules) {
			std::string sampleoutput = "Symbols from \"D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test/InstrumentedDebuggerTester.exe\".\r\nWin32 child process:\r\n        Using the running image of child Thread 9712.0x2cec.\r\n        While running this, GDB does not access memory from...\r\nLocal exec file:\r\n        `D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test/InstrumentedDebuggerTester.exe', file type pei-x86-64.\r\n        Entry point: 0x140003ddc\r\n        0x0000000140001000 - 0x0000000140004dff is .text\r\n        0x0000000140005000 - 0x00000001400087c4 is .rdata\r\n        0x0000000140009000 - 0x0000000140009400 is .data\r\n        0x000000014000a000 - 0x000000014000a444 is .pdata\r\n        0x000000014000b000 - 0x000000014000b040 is .gfids\r\n        0x000000014000c000 - 0x000000014000c1e0 is .rsrc\r\n        0x00000000777c1000 - 0x00000000778e31f3 is .text in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778e4000 - 0x00000000778e41da is RT in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778e5000 - 0x00000000778eb600 is .data in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778f4000 - 0x0000000077901ddc is .pdata in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x0000000077902000 - 0x000000007795c028 is .rsrc in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x000000007795d000 - 0x000000007795e4ee is .reloc in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000776a1000 - 0x000000007773bf29 is .text in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x000000007773c000 - 0x00000000777a9d34 is .rdata in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777aa000 - 0x00000000777ab600 is .data in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777ac000 - 0x00000000777b5564 is .pdata in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777b6000 - 0x00000000777b6528 is .rsrc in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777b7000 - 0x00000000777beaf4 is .reloc in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x000007fefd611000 - 0x000007fefd659f1f is .text in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd65a000 - 0x000007fefd66ee5c is .rdata in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd66f000 - 0x000007fefd670a00 is .data in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd671000 - 0x000007fefd6771c8 is .pdata in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd678000 - 0x000007fefd678530 is .rsrc in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd679000 - 0x000007fefd67913c is .reloc in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fed39b1000 - 0x000007fed3a0c71d is .textbss in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3a0d000 - 0x000007fed3ad05ea is .text in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3ad1000 - 0x000007fed3b022ca is .rdata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b03000 - 0x000007fed3b04200 is .data in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b07000 - 0x000007fed3b11908 is .pdata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b12000 - 0x000007fed3b13330 is .idata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b14000 - 0x000007fed3b1444a is .gfids in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b15000 - 0x000007fed3b1511b is .00cfg in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b16000 - 0x000007fed3b1643c is .rsrc in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b17000 - 0x000007fed3b188a6 is .reloc in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007feff431000 - 0x000007feff50933f is .text in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff50a000 - 0x000007feff50ebd7 is .ndr64 in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff50f000 - 0x000007feff512579 is .orpc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff513000 - 0x000007feff53e188 is .rdata in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff53f000 - 0x000007feff540800 is .data in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff541000 - 0x000007feff556018 is .pdata in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff557000 - 0x000007feff55ac50 is .rsrc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff55b000 - 0x000007feff55c924 is .reloc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007fee1bb1000 - 0x000007fee1c00f6c is .text in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c01000 - 0x000007fee1c3e9ec is .rdata in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c3f000 - 0x000007fee1c40c00 is .data in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c43000 - 0x000007fee1c47104 is .pdata in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c48000 - 0x000007fee1c48068 is .didat in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c49000 - 0x000007fee1c493f8 is .rsrc in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c4a000 - 0x000007fee1c4abd0 is .reloc in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee7c71000 - 0x000007fee7c7d31f is .text in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c7e000 - 0x000007fee7c81718 is .rdata in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c82000 - 0x000007fee7c82400 is .data in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c83000 - 0x000007fee7c83954 is .pdata in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c84000 - 0x000007fee7c84408 is .rsrc in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c85000 - 0x000007fee7c85174 is .reloc in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee8141000 - 0x000007fee8142514 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-runtime-l1-1-0.dll\r\n        0x000007fee8143000 - 0x000007fee81433f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-runtime-l1-1-0.dll\r\n        0x000007fee1ab1000 - 0x000007fee1b5b120 is .text in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b5c000 - 0x000007fee1b9462a is .rdata in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b95000 - 0x000007fee1b96000 is .data in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b98000 - 0x000007fee1ba29ec is .pdata in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1ba3000 - 0x000007fee1ba3418 is .rsrc in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1ba4000 - 0x000007fee1ba4aa8 is .reloc in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee74f1000 - 0x000007fee74f13bc is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-timezone-l1-1-0.dll\r\n        0x000007fee74f2000 - 0x000007fee74f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-timezone-l1-1-0.dll\r\n        0x000007fee74d1000 - 0x000007fee74d136c is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-file-l2-1-0.dll\r\n        0x000007fee74d2000 - 0x000007fee74d23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-file-l2-1-0.dll\r\n        0x000007fee7101000 - 0x000007fee7101dac is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-localization-l1-2-0.dll\r\n        0x000007fee7102000 - 0x000007fee71023f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-localization-l1-2-0.dll\r\n        0x000007fee70f1000 - 0x000007fee70f1558 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-synch-l1-2-0.dll\r\n        0x000007fee70f2000 - 0x000007fee70f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-synch-l1-2-0.dll\r\n        0x000007fee4801000 - 0x000007fee48014c4 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-processthreads-l1-1-1.dll\r\n        0x000007fee4802000 - 0x000007fee48023f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-processthreads-l1-1-1.dll\r\n        0x000007fee47f1000 - 0x000007fee47f1228 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-file-l1-2-0.dll\r\n        0x000007fee47f2000 - 0x000007fee47f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-file-l1-2-0.dll\r\n        0x000007fee47e1000 - 0x000007fee47e2bc0 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-string-l1-1-0.dll\r\n        0x000007fee47e3000 - 0x000007fee47e33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-string-l1-1-0.dll\r\n        0x000007fee47d1000 - 0x000007fee47d15f4 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-heap-l1-1-0.dll\r\n        0x000007fee47d2000 - 0x000007fee47d23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-heap-l1-1-0.dll\r\n        0x000007fee47c1000 - 0x000007fee47c2b40 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-stdio-l1-1-0.dll\r\n        0x000007fee47c3000 - 0x000007fee47c33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-stdio-l1-1-0.dll\r\n        0x000007fee19a1000 - 0x000007fee19a22d0 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-convert-l1-1-0.dll\r\n        0x000007fee19a3000 - 0x000007fee19a33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-convert-l1-1-0.dll\r\n        0x000007fee1991000 - 0x000007fee1991548 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-locale-l1-1-0.dll\r\n        0x000007fee1992000 - 0x000007fee19923f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-locale-l1-1-0.dll\r\n        0x000007fee1981000 - 0x000007fee1983678 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-math-l1-1-0.dll\r\n        0x000007fee1984000 - 0x000007fee19843f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-math-l1-1-0.dll\r\n        0x000007fee1971000 - 0x000007fee1971c9c is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-time-l1-1-0.dll\r\n        0x000007fee1972000 - 0x000007fee19723f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-time-l1-1-0.dll\r\n        0x000007fee1961000 - 0x000007fee1961b94 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-filesystem-l1-1-0.dll\r\n        0x000007fee1962000 - 0x000007fee19623f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-filesystem-l1-1-0.dll\r\n        0x000007fee1951000 - 0x000007fee1951408 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-environment-l1-1-0.dll\r\n        0x000007fee1952000 - 0x000007fee19523f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-environment-l1-1-0.dll\r\n        0x000007fee1941000 - 0x000007fee1941540 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-utility-l1-1-0.dll\r\n        0x000007fee1942000 - 0x000007fee19423f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-utility-l1-1-0.dll\r\n";

			std::set<Module> validModulesToCover;
			validModulesToCover.emplace("ntdll.dll/.text", "C:\\WINDOWS\\system32", 1);
			validModulesToCover.emplace("InstrumentedDebuggerTester.exe/.text", "*", 2);
			validModulesToCover.emplace("kernel32.dll/.text", "*", 3);
			validModulesToCover.emplace("NULL", "", 4);

			std::map<int, uint64_t> mapped;
			Assert::IsTrue(TestExecutorGDB::getBaseAddressesForModules(&mapped, validModulesToCover, sampleoutput), L"getBaseAddressesForModules");

			Assert::IsTrue(mapped.size() == 4, L"The size of the mapped modules is wrong");

			Assert::IsTrue(mapped[1] == 0x00000000777c1000, L"Base for ntdll.dll/.text was not correctly identified");

			Assert::IsTrue(mapped[2] == 0x0000000140001000, L"Base for InstrumentedDebuggerTester.exe/.text was not correctly identified");

			Assert::IsTrue(mapped[3] == 0x00000000776a1000, L"Base for kernel32.dll/.text was not correctly identified");

			Assert::IsTrue(mapped[4] == 0x0, L"Base for NULL was not correctly identified");
		}

		TEST_METHOD(TestExecutorGDB_getBaseAddressesAndSizes) {
			std::string sampleoutput = "Symbols from \"D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test/InstrumentedDebuggerTester.exe\".\r\nWin32 child process:\r\n        Using the running image of child Thread 9712.0x2cec.\r\n        While running this, GDB does not access memory from...\r\nLocal exec file:\r\n        `D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test/InstrumentedDebuggerTester.exe', file type pei-x86-64.\r\n        Entry point: 0x140003ddc\r\n        0x0000000140001000 - 0x0000000140004dff is .text\r\n        0x0000000140005000 - 0x00000001400087c4 is .rdata\r\n        0x0000000140009000 - 0x0000000140009400 is .data\r\n        0x000000014000a000 - 0x000000014000a444 is .pdata\r\n        0x000000014000b000 - 0x000000014000b040 is .gfids\r\n        0x000000014000c000 - 0x000000014000c1e0 is .rsrc\r\n        0x00000000777c1000 - 0x00000000778e31f3 is .text in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778e4000 - 0x00000000778e41da is RT in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778e5000 - 0x00000000778eb600 is .data in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778f4000 - 0x0000000077901ddc is .pdata in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x0000000077902000 - 0x000000007795c028 is .rsrc in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x000000007795d000 - 0x000000007795e4ee is .reloc in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000776a1000 - 0x000000007773bf29 is .text in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x000000007773c000 - 0x00000000777a9d34 is .rdata in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777aa000 - 0x00000000777ab600 is .data in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777ac000 - 0x00000000777b5564 is .pdata in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777b6000 - 0x00000000777b6528 is .rsrc in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777b7000 - 0x00000000777beaf4 is .reloc in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x000007fefd611000 - 0x000007fefd659f1f is .text in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd65a000 - 0x000007fefd66ee5c is .rdata in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd66f000 - 0x000007fefd670a00 is .data in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd671000 - 0x000007fefd6771c8 is .pdata in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd678000 - 0x000007fefd678530 is .rsrc in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd679000 - 0x000007fefd67913c is .reloc in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fed39b1000 - 0x000007fed3a0c71d is .textbss in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3a0d000 - 0x000007fed3ad05ea is .text in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3ad1000 - 0x000007fed3b022ca is .rdata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b03000 - 0x000007fed3b04200 is .data in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b07000 - 0x000007fed3b11908 is .pdata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b12000 - 0x000007fed3b13330 is .idata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b14000 - 0x000007fed3b1444a is .gfids in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b15000 - 0x000007fed3b1511b is .00cfg in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b16000 - 0x000007fed3b1643c is .rsrc in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b17000 - 0x000007fed3b188a6 is .reloc in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007feff431000 - 0x000007feff50933f is .text in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff50a000 - 0x000007feff50ebd7 is .ndr64 in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff50f000 - 0x000007feff512579 is .orpc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff513000 - 0x000007feff53e188 is .rdata in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff53f000 - 0x000007feff540800 is .data in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff541000 - 0x000007feff556018 is .pdata in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff557000 - 0x000007feff55ac50 is .rsrc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff55b000 - 0x000007feff55c924 is .reloc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007fee1bb1000 - 0x000007fee1c00f6c is .text in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c01000 - 0x000007fee1c3e9ec is .rdata in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c3f000 - 0x000007fee1c40c00 is .data in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c43000 - 0x000007fee1c47104 is .pdata in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c48000 - 0x000007fee1c48068 is .didat in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c49000 - 0x000007fee1c493f8 is .rsrc in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c4a000 - 0x000007fee1c4abd0 is .reloc in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee7c71000 - 0x000007fee7c7d31f is .text in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c7e000 - 0x000007fee7c81718 is .rdata in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c82000 - 0x000007fee7c82400 is .data in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c83000 - 0x000007fee7c83954 is .pdata in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c84000 - 0x000007fee7c84408 is .rsrc in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c85000 - 0x000007fee7c85174 is .reloc in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee8141000 - 0x000007fee8142514 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-runtime-l1-1-0.dll\r\n        0x000007fee8143000 - 0x000007fee81433f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-runtime-l1-1-0.dll\r\n        0x000007fee1ab1000 - 0x000007fee1b5b120 is .text in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b5c000 - 0x000007fee1b9462a is .rdata in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b95000 - 0x000007fee1b96000 is .data in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b98000 - 0x000007fee1ba29ec is .pdata in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1ba3000 - 0x000007fee1ba3418 is .rsrc in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1ba4000 - 0x000007fee1ba4aa8 is .reloc in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee74f1000 - 0x000007fee74f13bc is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-timezone-l1-1-0.dll\r\n        0x000007fee74f2000 - 0x000007fee74f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-timezone-l1-1-0.dll\r\n        0x000007fee74d1000 - 0x000007fee74d136c is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-file-l2-1-0.dll\r\n        0x000007fee74d2000 - 0x000007fee74d23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-file-l2-1-0.dll\r\n        0x000007fee7101000 - 0x000007fee7101dac is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-localization-l1-2-0.dll\r\n        0x000007fee7102000 - 0x000007fee71023f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-localization-l1-2-0.dll\r\n        0x000007fee70f1000 - 0x000007fee70f1558 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-synch-l1-2-0.dll\r\n        0x000007fee70f2000 - 0x000007fee70f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-synch-l1-2-0.dll\r\n        0x000007fee4801000 - 0x000007fee48014c4 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-processthreads-l1-1-1.dll\r\n        0x000007fee4802000 - 0x000007fee48023f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-processthreads-l1-1-1.dll\r\n        0x000007fee47f1000 - 0x000007fee47f1228 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-file-l1-2-0.dll\r\n        0x000007fee47f2000 - 0x000007fee47f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-file-l1-2-0.dll\r\n        0x000007fee47e1000 - 0x000007fee47e2bc0 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-string-l1-1-0.dll\r\n        0x000007fee47e3000 - 0x000007fee47e33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-string-l1-1-0.dll\r\n        0x000007fee47d1000 - 0x000007fee47d15f4 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-heap-l1-1-0.dll\r\n        0x000007fee47d2000 - 0x000007fee47d23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-heap-l1-1-0.dll\r\n        0x000007fee47c1000 - 0x000007fee47c2b40 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-stdio-l1-1-0.dll\r\n        0x000007fee47c3000 - 0x000007fee47c33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-stdio-l1-1-0.dll\r\n        0x000007fee19a1000 - 0x000007fee19a22d0 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-convert-l1-1-0.dll\r\n        0x000007fee19a3000 - 0x000007fee19a33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-convert-l1-1-0.dll\r\n        0x000007fee1991000 - 0x000007fee1991548 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-locale-l1-1-0.dll\r\n        0x000007fee1992000 - 0x000007fee19923f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-locale-l1-1-0.dll\r\n        0x000007fee1981000 - 0x000007fee1983678 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-math-l1-1-0.dll\r\n        0x000007fee1984000 - 0x000007fee19843f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-math-l1-1-0.dll\r\n        0x000007fee1971000 - 0x000007fee1971c9c is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-time-l1-1-0.dll\r\n        0x000007fee1972000 - 0x000007fee19723f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-time-l1-1-0.dll\r\n        0x000007fee1961000 - 0x000007fee1961b94 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-filesystem-l1-1-0.dll\r\n        0x000007fee1962000 - 0x000007fee19623f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-filesystem-l1-1-0.dll\r\n        0x000007fee1951000 - 0x000007fee1951408 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-environment-l1-1-0.dll\r\n        0x000007fee1952000 - 0x000007fee19523f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-environment-l1-1-0.dll\r\n        0x000007fee1941000 - 0x000007fee1941540 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-utility-l1-1-0.dll\r\n        0x000007fee1942000 - 0x000007fee19423f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-utility-l1-1-0.dll\r\n";

§§			std::vector<std::tuple<std::string, uint64_t, uint64_t>> baseAddressesAndSizes;
			Assert::IsTrue(TestExecutorGDB::getBaseAddressesAndSizes(baseAddressesAndSizes, sampleoutput), L"getBaseAddressesAndSizes");

			Assert::IsTrue(baseAddressesAndSizes.size() == 95, L"The size of the mapped modules is wrong");

			Assert::IsTrue(std::get<0>(baseAddressesAndSizes[0]) == "InstrumentedDebuggerTester.exe/.text", L"Base for text was not correctly identified");
			Assert::IsTrue(std::get<1>(baseAddressesAndSizes[0]) == 0x0000000140001000, L"End for text was not correctly identified");
			Assert::IsTrue(std::get<2>(baseAddressesAndSizes[0]) == 0x0000000140004dff - 0x0000000140001000, L"Binary for text was not correctly identified");

			Assert::IsTrue(std::get<0>(baseAddressesAndSizes[6]) == "ntdll.dll/.text", L"Base for ntdll.dll/.text was not correctly identified");
			Assert::IsTrue(std::get<1>(baseAddressesAndSizes[6]) == 0x00000000777c1000, L"End for ntdll.dll/.text was not correctly identified");
			Assert::IsTrue(std::get<2>(baseAddressesAndSizes[6]) == 0x00000000778e31f3 - 0x00000000777c1000, L"Binary for ntdll.dll/.text was not correctly identified");
		}

		TEST_METHOD(TestExecutorGDB_getCrashRVA) {
			std::string sampleoutput = "Symbols from \"D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test/InstrumentedDebuggerTester.exe\".\r\nWin32 child process:\r\n        Using the running image of child Thread 9712.0x2cec.\r\n        While running this, GDB does not access memory from...\r\nLocal exec file:\r\n        `D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test/InstrumentedDebuggerTester.exe', file type pei-x86-64.\r\n        Entry point: 0x140003ddc\r\n        0x0000000140001000 - 0x0000000140004dff is .text\r\n        0x0000000140005000 - 0x00000001400087c4 is .rdata\r\n        0x0000000140009000 - 0x0000000140009400 is .data\r\n        0x000000014000a000 - 0x000000014000a444 is .pdata\r\n        0x000000014000b000 - 0x000000014000b040 is .gfids\r\n        0x000000014000c000 - 0x000000014000c1e0 is .rsrc\r\n        0x00000000777c1000 - 0x00000000778e31f3 is .text in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778e4000 - 0x00000000778e41da is RT in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778e5000 - 0x00000000778eb600 is .data in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778f4000 - 0x0000000077901ddc is .pdata in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x0000000077902000 - 0x000000007795c028 is .rsrc in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x000000007795d000 - 0x000000007795e4ee is .reloc in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000776a1000 - 0x000000007773bf29 is .text in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x000000007773c000 - 0x00000000777a9d34 is .rdata in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777aa000 - 0x00000000777ab600 is .data in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777ac000 - 0x00000000777b5564 is .pdata in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777b6000 - 0x00000000777b6528 is .rsrc in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777b7000 - 0x00000000777beaf4 is .reloc in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x000007fefd611000 - 0x000007fefd659f1f is .text in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd65a000 - 0x000007fefd66ee5c is .rdata in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd66f000 - 0x000007fefd670a00 is .data in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd671000 - 0x000007fefd6771c8 is .pdata in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd678000 - 0x000007fefd678530 is .rsrc in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd679000 - 0x000007fefd67913c is .reloc in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fed39b1000 - 0x000007fed3a0c71d is .textbss in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3a0d000 - 0x000007fed3ad05ea is .text in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3ad1000 - 0x000007fed3b022ca is .rdata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b03000 - 0x000007fed3b04200 is .data in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b07000 - 0x000007fed3b11908 is .pdata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b12000 - 0x000007fed3b13330 is .idata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b14000 - 0x000007fed3b1444a is .gfids in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b15000 - 0x000007fed3b1511b is .00cfg in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b16000 - 0x000007fed3b1643c is .rsrc in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b17000 - 0x000007fed3b188a6 is .reloc in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007feff431000 - 0x000007feff50933f is .text in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff50a000 - 0x000007feff50ebd7 is .ndr64 in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff50f000 - 0x000007feff512579 is .orpc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff513000 - 0x000007feff53e188 is .rdata in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff53f000 - 0x000007feff540800 is .data in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff541000 - 0x000007feff556018 is .pdata in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff557000 - 0x000007feff55ac50 is .rsrc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff55b000 - 0x000007feff55c924 is .reloc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007fee1bb1000 - 0x000007fee1c00f6c is .text in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c01000 - 0x000007fee1c3e9ec is .rdata in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c3f000 - 0x000007fee1c40c00 is .data in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c43000 - 0x000007fee1c47104 is .pdata in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c48000 - 0x000007fee1c48068 is .didat in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c49000 - 0x000007fee1c493f8 is .rsrc in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c4a000 - 0x000007fee1c4abd0 is .reloc in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee7c71000 - 0x000007fee7c7d31f is .text in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c7e000 - 0x000007fee7c81718 is .rdata in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c82000 - 0x000007fee7c82400 is .data in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c83000 - 0x000007fee7c83954 is .pdata in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c84000 - 0x000007fee7c84408 is .rsrc in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c85000 - 0x000007fee7c85174 is .reloc in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee8141000 - 0x000007fee8142514 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-runtime-l1-1-0.dll\r\n        0x000007fee8143000 - 0x000007fee81433f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-runtime-l1-1-0.dll\r\n        0x000007fee1ab1000 - 0x000007fee1b5b120 is .text in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b5c000 - 0x000007fee1b9462a is .rdata in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b95000 - 0x000007fee1b96000 is .data in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b98000 - 0x000007fee1ba29ec is .pdata in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1ba3000 - 0x000007fee1ba3418 is .rsrc in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1ba4000 - 0x000007fee1ba4aa8 is .reloc in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee74f1000 - 0x000007fee74f13bc is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-timezone-l1-1-0.dll\r\n        0x000007fee74f2000 - 0x000007fee74f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-timezone-l1-1-0.dll\r\n        0x000007fee74d1000 - 0x000007fee74d136c is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-file-l2-1-0.dll\r\n        0x000007fee74d2000 - 0x000007fee74d23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-file-l2-1-0.dll\r\n        0x000007fee7101000 - 0x000007fee7101dac is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-localization-l1-2-0.dll\r\n        0x000007fee7102000 - 0x000007fee71023f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-localization-l1-2-0.dll\r\n        0x000007fee70f1000 - 0x000007fee70f1558 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-synch-l1-2-0.dll\r\n        0x000007fee70f2000 - 0x000007fee70f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-synch-l1-2-0.dll\r\n        0x000007fee4801000 - 0x000007fee48014c4 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-processthreads-l1-1-1.dll\r\n        0x000007fee4802000 - 0x000007fee48023f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-processthreads-l1-1-1.dll\r\n        0x000007fee47f1000 - 0x000007fee47f1228 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-file-l1-2-0.dll\r\n        0x000007fee47f2000 - 0x000007fee47f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-file-l1-2-0.dll\r\n        0x000007fee47e1000 - 0x000007fee47e2bc0 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-string-l1-1-0.dll\r\n        0x000007fee47e3000 - 0x000007fee47e33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-string-l1-1-0.dll\r\n        0x000007fee47d1000 - 0x000007fee47d15f4 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-heap-l1-1-0.dll\r\n        0x000007fee47d2000 - 0x000007fee47d23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-heap-l1-1-0.dll\r\n        0x000007fee47c1000 - 0x000007fee47c2b40 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-stdio-l1-1-0.dll\r\n        0x000007fee47c3000 - 0x000007fee47c33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-stdio-l1-1-0.dll\r\n        0x000007fee19a1000 - 0x000007fee19a22d0 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-convert-l1-1-0.dll\r\n        0x000007fee19a3000 - 0x000007fee19a33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-convert-l1-1-0.dll\r\n        0x000007fee1991000 - 0x000007fee1991548 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-locale-l1-1-0.dll\r\n        0x000007fee1992000 - 0x000007fee19923f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-locale-l1-1-0.dll\r\n        0x000007fee1981000 - 0x000007fee1983678 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-math-l1-1-0.dll\r\n        0x000007fee1984000 - 0x000007fee19843f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-math-l1-1-0.dll\r\n        0x000007fee1971000 - 0x000007fee1971c9c is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-time-l1-1-0.dll\r\n        0x000007fee1972000 - 0x000007fee19723f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-time-l1-1-0.dll\r\n        0x000007fee1961000 - 0x000007fee1961b94 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-filesystem-l1-1-0.dll\r\n        0x000007fee1962000 - 0x000007fee19623f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-filesystem-l1-1-0.dll\r\n        0x000007fee1951000 - 0x000007fee1951408 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-environment-l1-1-0.dll\r\n        0x000007fee1952000 - 0x000007fee19523f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-environment-l1-1-0.dll\r\n        0x000007fee1941000 - 0x000007fee1941540 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-utility-l1-1-0.dll\r\n        0x000007fee1942000 - 0x000007fee19423f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-utility-l1-1-0.dll\r\n";

§§			std::vector<std::tuple<std::string, uint64_t, uint64_t>> baseAddressesAndSizes;
			Assert::IsTrue(TestExecutorGDB::getBaseAddressesAndSizes(baseAddressesAndSizes, sampleoutput), L"getBaseAddressesAndSizes");
			Assert::IsTrue(TestExecutorGDB::getCrashRVA(baseAddressesAndSizes, 0x1234) == "unknown+0x00001234", L"0x1234 was not correctly translated");

			Assert::IsTrue(TestExecutorGDB::getCrashRVA(baseAddressesAndSizes, 0x000000014000c1d0) == "InstrumentedDebuggerTester.exe/.rsrc+0x000001d0", L"0x000000014000c1d0 was not correctly translated");
			Assert::IsTrue(TestExecutorGDB::getCrashRVA(baseAddressesAndSizes, 0x000000015000c000) == "unknown+0x15000c000", L"0x000000015000c000 was not correctly translated");
		}

		TEST_METHOD(TestExecutorGDB_parseInfoFiles) {
			std::string sampleoutput = "Symbols from \"D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test/InstrumentedDebuggerTester.exe\".\r\nWin32 child process:\r\n        Using the running image of child Thread 9712.0x2cec.\r\n        While running this, GDB does not access memory from...\r\nLocal exec file:\r\n        `D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test/InstrumentedDebuggerTester.exe', file type pei-x86-64.\r\n        Entry point: 0x140003ddc\r\n        0x0000000140001000 - 0x0000000140004dff is .text\r\n        0x0000000140005000 - 0x00000001400087c4 is .rdata\r\n        0x0000000140009000 - 0x0000000140009400 is .data\r\n        0x000000014000a000 - 0x000000014000a444 is .pdata\r\n        0x000000014000b000 - 0x000000014000b040 is .gfids\r\n        0x000000014000c000 - 0x000000014000c1e0 is .rsrc\r\n        0x00000000777c1000 - 0x00000000778e31f3 is .text in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778e4000 - 0x00000000778e41da is RT in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778e5000 - 0x00000000778eb600 is .data in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000778f4000 - 0x0000000077901ddc is .pdata in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x0000000077902000 - 0x000000007795c028 is .rsrc in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x000000007795d000 - 0x000000007795e4ee is .reloc in C:\\WINDOWS\\system32\\ntdll.dll\r\n        0x00000000776a1000 - 0x000000007773bf29 is .text in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x000000007773c000 - 0x00000000777a9d34 is .rdata in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777aa000 - 0x00000000777ab600 is .data in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777ac000 - 0x00000000777b5564 is .pdata in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777b6000 - 0x00000000777b6528 is .rsrc in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x00000000777b7000 - 0x00000000777beaf4 is .reloc in C:\\WINDOWS\\system32\\kernel32.dll\r\n        0x000007fefd611000 - 0x000007fefd659f1f is .text in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd65a000 - 0x000007fefd66ee5c is .rdata in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd66f000 - 0x000007fefd670a00 is .data in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd671000 - 0x000007fefd6771c8 is .pdata in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd678000 - 0x000007fefd678530 is .rsrc in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fefd679000 - 0x000007fefd67913c is .reloc in C:\\WINDOWS\\system32\\KernelBase.dll\r\n        0x000007fed39b1000 - 0x000007fed3a0c71d is .textbss in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3a0d000 - 0x000007fed3ad05ea is .text in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3ad1000 - 0x000007fed3b022ca is .rdata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b03000 - 0x000007fed3b04200 is .data in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b07000 - 0x000007fed3b11908 is .pdata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b12000 - 0x000007fed3b13330 is .idata in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b14000 - 0x000007fed3b1444a is .gfids in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b15000 - 0x000007fed3b1511b is .00cfg in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b16000 - 0x000007fed3b1643c is .rsrc in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007fed3b17000 - 0x000007fed3b188a6 is .reloc in D:\\git repos\\CSA\\FLUFFI\\f1\\FluffiV1\\x64\\Test\\SharedMemIPC.dll\r\n        0x000007feff431000 - 0x000007feff50933f is .text in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff50a000 - 0x000007feff50ebd7 is .ndr64 in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff50f000 - 0x000007feff512579 is .orpc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff513000 - 0x000007feff53e188 is .rdata in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff53f000 - 0x000007feff540800 is .data in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff541000 - 0x000007feff556018 is .pdata in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff557000 - 0x000007feff55ac50 is .rsrc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007feff55b000 - 0x000007feff55c924 is .reloc in C:\\WINDOWS\\system32\\rpcrt4.dll\r\n        0x000007fee1bb1000 - 0x000007fee1c00f6c is .text in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c01000 - 0x000007fee1c3e9ec is .rdata in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c3f000 - 0x000007fee1c40c00 is .data in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c43000 - 0x000007fee1c47104 is .pdata in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c48000 - 0x000007fee1c48068 is .didat in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c49000 - 0x000007fee1c493f8 is .rsrc in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee1c4a000 - 0x000007fee1c4abd0 is .reloc in C:\\WINDOWS\\system32\\msvcp140.dll\r\n        0x000007fee7c71000 - 0x000007fee7c7d31f is .text in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c7e000 - 0x000007fee7c81718 is .rdata in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c82000 - 0x000007fee7c82400 is .data in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c83000 - 0x000007fee7c83954 is .pdata in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c84000 - 0x000007fee7c84408 is .rsrc in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee7c85000 - 0x000007fee7c85174 is .reloc in C:\\WINDOWS\\system32\\vcruntime140.dll\r\n        0x000007fee8141000 - 0x000007fee8142514 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-runtime-l1-1-0.dll\r\n        0x000007fee8143000 - 0x000007fee81433f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-runtime-l1-1-0.dll\r\n        0x000007fee1ab1000 - 0x000007fee1b5b120 is .text in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b5c000 - 0x000007fee1b9462a is .rdata in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b95000 - 0x000007fee1b96000 is .data in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1b98000 - 0x000007fee1ba29ec is .pdata in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1ba3000 - 0x000007fee1ba3418 is .rsrc in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee1ba4000 - 0x000007fee1ba4aa8 is .reloc in C:\\WINDOWS\\system32\\ucrtbase.dll\r\n        0x000007fee74f1000 - 0x000007fee74f13bc is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-timezone-l1-1-0.dll\r\n        0x000007fee74f2000 - 0x000007fee74f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-timezone-l1-1-0.dll\r\n        0x000007fee74d1000 - 0x000007fee74d136c is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-file-l2-1-0.dll\r\n        0x000007fee74d2000 - 0x000007fee74d23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-file-l2-1-0.dll\r\n        0x000007fee7101000 - 0x000007fee7101dac is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-localization-l1-2-0.dll\r\n        0x000007fee7102000 - 0x000007fee71023f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-localization-l1-2-0.dll\r\n        0x000007fee70f1000 - 0x000007fee70f1558 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-synch-l1-2-0.dll\r\n        0x000007fee70f2000 - 0x000007fee70f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-synch-l1-2-0.dll\r\n        0x000007fee4801000 - 0x000007fee48014c4 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-processthreads-l1-1-1.dll\r\n        0x000007fee4802000 - 0x000007fee48023f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-processthreads-l1-1-1.dll\r\n        0x000007fee47f1000 - 0x000007fee47f1228 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-core-file-l1-2-0.dll\r\n        0x000007fee47f2000 - 0x000007fee47f23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-core-file-l1-2-0.dll\r\n        0x000007fee47e1000 - 0x000007fee47e2bc0 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-string-l1-1-0.dll\r\n        0x000007fee47e3000 - 0x000007fee47e33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-string-l1-1-0.dll\r\n        0x000007fee47d1000 - 0x000007fee47d15f4 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-heap-l1-1-0.dll\r\n        0x000007fee47d2000 - 0x000007fee47d23f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-heap-l1-1-0.dll\r\n        0x000007fee47c1000 - 0x000007fee47c2b40 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-stdio-l1-1-0.dll\r\n        0x000007fee47c3000 - 0x000007fee47c33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-stdio-l1-1-0.dll\r\n        0x000007fee19a1000 - 0x000007fee19a22d0 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-convert-l1-1-0.dll\r\n        0x000007fee19a3000 - 0x000007fee19a33f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-convert-l1-1-0.dll\r\n        0x000007fee1991000 - 0x000007fee1991548 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-locale-l1-1-0.dll\r\n        0x000007fee1992000 - 0x000007fee19923f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-locale-l1-1-0.dll\r\n        0x000007fee1981000 - 0x000007fee1983678 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-math-l1-1-0.dll\r\n        0x000007fee1984000 - 0x000007fee19843f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-math-l1-1-0.dll\r\n        0x000007fee1971000 - 0x000007fee1971c9c is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-time-l1-1-0.dll\r\n        0x000007fee1972000 - 0x000007fee19723f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-time-l1-1-0.dll\r\n        0x000007fee1961000 - 0x000007fee1961b94 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-filesystem-l1-1-0.dll\r\n        0x000007fee1962000 - 0x000007fee19623f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-filesystem-l1-1-0.dll\r\n        0x000007fee1951000 - 0x000007fee1951408 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-environment-l1-1-0.dll\r\n        0x000007fee1952000 - 0x000007fee19523f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-environment-l1-1-0.dll\r\n        0x000007fee1941000 - 0x000007fee1941540 is .rdata in C:\\WINDOWS\\system32\\api-ms-win-crt-utility-l1-1-0.dll\r\n        0x000007fee1942000 - 0x000007fee19423f8 is .rsrc in C:\\WINDOWS\\system32\\api-ms-win-crt-utility-l1-1-0.dll\r\n";

§§			std::vector<std::tuple<uint64_t, uint64_t, std::string, std::string, std::string>> loadedFiles;
			Assert::IsTrue(TestExecutorGDB::parseInfoFiles(loadedFiles, sampleoutput), L"parseInfoFiles");

			Assert::IsTrue(loadedFiles.size() == 95, L"The size of the mapped modules is wrong");

			Assert::IsTrue(std::get<0>(loadedFiles[0]) == 0x0000000140001000, L"Base for text was not correctly identified");
			Assert::IsTrue(std::get<1>(loadedFiles[0]) == 0x0000000140004dff, L"End for text was not correctly identified");
			Assert::IsTrue(std::get<2>(loadedFiles[0]) == "InstrumentedDebuggerTester.exe", L"Binary for text was not correctly identified");
			Assert::IsTrue(std::get<3>(loadedFiles[0]) == ".text", L"name for text was not correctly identified");

			Assert::IsTrue(std::get<0>(loadedFiles[6]) == 0x00000000777c1000, L"Base for ntdll.dll/.text was not correctly identified");
			Assert::IsTrue(std::get<1>(loadedFiles[6]) == 0x00000000778e31f3, L"End for ntdll.dll/.text was not correctly identified");
			Assert::IsTrue(std::get<2>(loadedFiles[6]) == "ntdll.dll", L"Binary for ntdll.dll/.text was not correctly identified");
			Assert::IsTrue(std::get<3>(loadedFiles[6]) == ".text", L"name for ntdll.dll/.text was not correctly identified");
			Assert::IsTrue(std::get<4>(loadedFiles[6]) == "C:\\WINDOWS\\system32", L"Path for ntdll.dll/.text was not correctly identified");
		}

		TEST_METHOD(TestExecutorGDB_debuggerThreadMain_clean) {
			//Check coverage
			{
				HANDLE sharedMemIPCInterruptEvent = CreateEvent(NULL, false, false, NULL);
				Assert::IsFalse(sharedMemIPCInterruptEvent == NULL, L"Failed to create the SharedMemIPCInterruptEvent");

				SharedMemIPC sharedMemIPC_toTarget(std::string("FLUFFI_SharedMemForTest").c_str());

				std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication = std::make_shared<GDBThreadCommunication>();

				std::string gdbInitFile = ".\\gdbinitfile";
				bool saveBlocksToFile = true;

				std::ofstream initCommandFile(gdbInitFile, std::ofstream::out);
				initCommandFile << "file InstrumentedDebuggerTester.exe" << std::endl;
				initCommandFile << "set args FLUFFI_SharedMemForTest" << std::endl;
				initCommandFile.close();

				std::string testcaseFile = ".\\gdbtestcaseFile";

				std::ofstream testcaseFileOS(testcaseFile, std::ios::binary | std::ios::out);
				testcaseFileOS << (char)4;
				testcaseFileOS.close();

				uint32_t expectedBlocks[] = { 16,86,116,138,170,203,267,287,885,912,931,950,1007,1026,1140,1601,1604,1667,1687,1697,1760,1765,1820,1825,1830,1839,1980,2036,2592,2626,2670,2681,2715,2736,2818,2838,2885,2891,2900,2910,2944,3012,3043,3093,3150,3157,3160,3199,3728,3795,3809,3818,3823,3828,3851,3873,3916,3969,4000,4104,4122,4167,4174,4192,4667,4669,6288,6718,6720,6768,6831,6909,6960,6994,7029,7088,7128,7151,7183,7207,7231,7241,7267,7274,7344,7399,7408,7417,7444,7446,7464,7471,7485,7506,7525,7539,7553,7591,7595,7624,7676,7692,7708,7730,7740,7750,7768,7775,7808,7885,7887,7905,7912,7926,7947,7966,7980,7994,8000,8049,8053,8060,8063,8099,8146,8162,8178,8200,8210,8220,8237,8244,8272,8336,8391,8448,8450,8476,8481,8491,8560,8590,9424,9480,9628,9648,9654,9671,9678,9681,9688,9728,9733,9745,9756,9764,9768,9904,9961,10014,10027,10073,10081,10103,10139,10228,10256,10259,10280,10283,10304,10333,10348,10351,10356,10365,10423,10443,10449,10469,10476,10547,10552,10563,10574,10582,10586,10608,10627,10701,10706,10732,10884,10913,10927,10940,10976,10986,10998,11012,11020,11064,11074,14596,14602,14614,15020 };

				std::set<Module> modulesToCover;
				modulesToCover.emplace("InstrumentedDebuggerTester.exe/.text", "*", 0);
				std::set<FluffiBasicBlock>blocksToCover;
				int rva[] = { 0x0010,0x0056,0x0074,0x007a,0x008a,0x00aa,0x00cb,0x010b,0x011f,0x3ae0,0x3aec,0x3af8,0x2970,0x0150,0x09a0,0x0150,0x0180,0x01be,0x01da,0x01ef,0x021e,0x0223,0x0226,0x022f,0x023a,0x0296,0x02a1,0x02b1,0x02b6,0x02bd,0x02c6,0x02cd,0x02d6,0x02dd,0x02e3,0x02ea,0x02ed,0x02f2,0x02fb,0x0333,0x034c,0x0375,0x0390,0x03a3,0x03b6,0x03ef,0x0402,0x0474,0x04b8,0x04e3,0x0509,0x0518,0x0523,0x052e,0x053e,0x0547,0x0554,0x0561,0x056b,0x056e,0x0573,0x0586,0x058b,0x059a,0x05a3,0x05b0,0x05bd,0x05c7,0x05ca,0x05de,0x05e8,0x05f7,0x0600,0x060d,0x061a,0x0624,0x0627,0x062c,0x0641,0x0644,0x0653,0x065c,0x0669,0x0676,0x0680,0x0683,0x0697,0x06a1,0x06b0,0x06b9,0x06c6,0x06d3,0x06dd,0x06e0,0x06e5,0x071c,0x0721,0x0726,0x072f,0x0754,0x079e,0x07b1,0x07bc,0x07f4,0x0803,0x080e,0x0823,0x084c,0x0851,0x0858,0x085f,0x0866,0x086d,0x0874,0x087b,0x0882,0x0889,0x0890,0x0897,0x089e,0x08a5,0x08ac,0x08b3,0x08ba,0x08c1,0x08c8,0x08cf,0x08d6,0x08dd,0x0902,0x0927,0x094d,0x0950,0x095b,0x0966,0x0973,0x3b10,0x3b1e,0x3b2a,0x3b38,0x3b46,0x3b52,0x3b5e,0x3b6a,0x3b78,0x2970,0x09a0,0x09a0,0x09b1,0x09c0,0x09c5,0x09cc,0x09d5,0x09dc,0x09e5,0x09ec,0x09f2,0x09f9,0x09fc,0x0a0e,0x0a20,0x0a42,0x0a4b,0x0a54,0x0a5b,0x0a6e,0x0a79,0x0a9b,0x0ab0,0x0b02,0x0b16,0x0b23,0x0b45,0x0b4b,0x0b54,0x0b5e,0x38e0,0x0b80,0x0bc4,0x0be3,0x0c15,0x0c4e,0x0c55,0x0c58,0x0c68,0x0c7f,0x3b90,0x3ba3,0x3bb8,0x3bbe,0x3bd0,0x3bde,0x16a0,0x0ca0,0x0cc5,0x0cd8,0x0cf0,0x0d03,0x0d11,0x0d22,0x0d2b,0x0d40,0x0d58,0x0d5d,0x0d62,0x0d68,0x0d6b,0x0d75,0x0d91,0x0da0,0x0ddb,0x0de4,0x0dfa,0x0dff,0x0e17,0x0e2d,0x0e57,0x0e72,0x0e90,0x0ec3,0x0ec9,0x0ed0,0x0ed3,0x0ee1,0x0eea,0x0eef,0x0ef4,0x0f0b,0x0f21,0x0f2a,0x0f4c,0x0f66,0x0f81,0x0fa0,0x0fdc,0x0fec,0x1008,0x1012,0x101a,0x1027,0x1047,0x104e,0x1060,0x1068,0x1070,0x1084,0x109a,0x1104,0x1109,0x1112,0x1119,0x111e,0x1130,0x114b,0x1156,0x115e,0x117d,0x1181,0x1187,0x118c,0x11a8,0x11b0,0x11ca,0x11d0,0x11d4,0x11da,0x11e1,0x11eb,0x11f6,0x11fc,0x1203,0x120c,0x1213,0x121c,0x1223,0x1229,0x1230,0x1233,0x123b,0x123d,0x3bf0,0x2970,0x1260,0x1275,0x1284,0x128d,0x1293,0x12a2,0x12af,0x12d0,0x12eb,0x12f4,0x12f9,0x1301,0x1321,0x132d,0x1332,0x1339,0x1348,0x1355,0x1364,0x137c,0x1393,0x13a0,0x13b0,0x13ea,0x13f6,0x1402,0x1412,0x1430,0x143a,0x1442,0x144f,0x146f,0x1476,0x1494,0x14c2,0x14d0,0x1522,0x152b,0x1548,0x1576,0x1584,0x158d,0x1594,0x15a8,0x15ac,0x15b1,0x15d4,0x15dc,0x15df,0x15e5,0x15f1,0x15f7,0x15fe,0x1607,0x160e,0x1617,0x161e,0x1624,0x162b,0x162e,0x1636,0x1638,0x3bf0,0x2970,0x16a0,0x16c6,0x16d3,0x16f5,0x16fb,0x1704,0x1720,0x1748,0x1759,0x1770,0x1789,0x179d,0x17aa,0x17ca,0x17d0,0x17d9,0x17e3,0x17e9,0x17f7,0x38e0,0x1810,0x182d,0x1837,0x184b,0x1852,0x38e0,0x1860,0x186d,0x188f,0x1890,0x18cb,0x18d5,0x18e3,0x18ea,0x1914,0x1920,0x1956,0x195b,0x195d,0x1961,0x197e,0x19af,0x19b5,0x19be,0x19d2,0x19df,0x19e3,0x19e5,0x19eb,0x19f7,0x19fd,0x1a04,0x1a0d,0x1a14,0x1a1d,0x1a24,0x1a2a,0x1a31,0x1a34,0x1a39,0x1a3e,0x1a40,0x3c00,0x2970,0x1a70,0x1aaf,0x1afd,0x1b30,0x1b4c,0x1b52,0x1b75,0x1bb0,0x1bd8,0x1bef,0x1c0f,0x1c27,0x1c31,0x1c3f,0x1c49,0x1c54,0x1c63,0x1c68,0x1c6a,0x38e0,0x3c10,0x22e0,0x1c80,0x1ca1,0x1ca8,0x1cb0,0x1ce2,0x1ce7,0x1cf0,0x1cf9,0x1d0a,0x1d0f,0x1d14,0x1d16,0x1d28,0x1d2f,0x1d3d,0x1d47,0x1d4c,0x1d52,0x1d65,0x1d69,0x1d73,0x1d81,0x1d86,0x1da2,0x1da7,0x1dab,0x1dc8,0x1dcd,0x1de9,0x1dee,0x1df3,0x1df8,0x1dfc,0x1e0c,0x1e0e,0x1e1c,0x1e32,0x1e3c,0x1e46,0x1e58,0x1e5f,0x38e0,0x3c20,0x3c2c,0x3c38,0x3c45,0x3c62,0x1c80,0x1810,0x1e80,0x1ec3,0x1ec8,0x1ecd,0x1ecf,0x1ee1,0x1ee8,0x1ef6,0x1f00,0x1f05,0x1f0b,0x1f1e,0x1f22,0x1f2c,0x1f3a,0x1f40,0x1f45,0x1f61,0x1f6c,0x1f71,0x1f75,0x1f7c,0x1f7f,0x1f9c,0x1fa3,0x1fa8,0x1fc4,0x1fc9,0x1fce,0x1fd2,0x1fe2,0x1fe4,0x1ff2,0x2008,0x2012,0x201c,0x202d,0x2034,0x38e0,0x3c70,0x3c7c,0x3c88,0x3c95,0x3cb2,0x1c80,0x1810,0x2050,0x2090,0x20c7,0x20ed,0x20fe,0x2100,0x2102,0x2108,0x2114,0x211c,0x2121,0x212b,0x213f,0x2170,0x218e,0x3cc0,0x21bc,0x1720,0x21d0,0x21f0,0x2218,0x2225,0x2240,0x2280,0x22a0,0x22e0,0x22f6,0x2302,0x2311,0x2320,0x2342,0x2350,0x2362,0x2368,0x2376,0x237c,0x238d,0x2397,0x239e,0x23a7,0x23ac,0x23b7,0x23cd,0x23e0,0x23e9,0x23f0,0x23f5,0x23f8,0x2407,0x2412,0x2417,0x241a,0x241e,0x2430,0x2446,0x2455,0x2469,0x247a,0x247f,0x2486,0x2489,0x248e,0x249b,0x24a6,0x24bb,0x24c2,0x24d0,0x24fa,0x2508,0x251b,0x2525,0x2533,0x253e,0x2541,0x254e,0x2561,0x256c,0x2575,0x257c,0x2581,0x2584,0x258d,0x259c,0x25a2,0x25b0,0x25b6,0x25c7,0x25ce,0x25d1,0x25d8,0x25dd,0x25e2,0x25ed,0x25f5,0x25fd,0x2600,0x2605,0x2611,0x261c,0x2621,0x2624,0x2628,0x2650,0x2659,0x2662,0x266b,0x2674,0x267b,0x2694,0x26b0,0x26e4,0x26e9,0x2708,0x271a,0x271e,0x2727,0x272b,0x2734,0x273d,0x2743,0x2759,0x2761,0x2763,0x2777,0x277c,0x2783,0x2788,0x278b,0x2790,0x279b,0x27a5,0x27b3,0x27b8,0x27bf,0x27c8,0x27cf,0x27d8,0x27df,0x27e5,0x27ec,0x27ef,0x27f4,0x280b,0x2810,0x2813,0x2828,0x282b,0x3cd0,0x3cdd,0x3ce5,0x3cf1,0x3cff,0x3d0d,0x3d1b,0x3d27,0x3d2e,0x3d36,0x3d3b,0x3d42,0x3d4b,0x3d52,0x3d5b,0x3d62,0x3d68,0x3d6f,0x3d72,0x3d77,0x3d8e,0x3d91,0x2840,0x285d,0x2867,0x286c,0x286f,0x2874,0x287a,0x287d,0x2886,0x288c,0x2891,0x2894,0x28b7,0x28bd,0x28cb,0x28d1,0x28e5,0x28ec,0x28f1,0x28f6,0x2901,0x291a,0x2930,0x2933,0x2938,0x2943,0x294e,0x2953,0x2956,0x295a,0x24d0,0x2970,0x2983,0x2991,0x2996,0x299d,0x29a6,0x29ad,0x29b6,0x29bd,0x29c3,0x29ca,0x29cd,0x29d2,0x29e9,0x29ec,0x29f8,0x2a19,0x2a3b,0x2a55,0x2a5b,0x2a69,0x38e0,0x2a84,0x2aa1,0x2aaf,0x2abc,0x2ae0,0x2aea,0x2af6,0x2af8,0x2afc,0x30d4,0x2b04,0x390a,0x2b0c,0x2b17,0x2b23,0x2b29,0x2b2e,0x2b30,0x2b35,0x2b38,0x2b42,0x2b48,0x2b04,0x2b50,0x2b68,0x2b72,0x2b7c,0x2bb5,0x2bd6,0x2be4,0x2bf0,0x2c0f,0x2c14,0x2c21,0x2c2c,0x2c38,0x2c48,0x3982,0x2c64,0x2c81,0x2c8c,0x2c94,0x2ca6,0x2cb0,0x2cb4,0x2cd5,0x2cdf,0x2cfe,0x2d06,0x2d1b,0x2d27,0x2d3d,0x2d4b,0x2d57,0x2d5f,0x2d8c,0x2d93,0x2d98,0x2d9d,0x2daa,0x2db5,0x2dbd,0x2dc4,0x2dc9,0x2dcb,0x3d9e,0x3cff,0x2ddc,0x2c64,0x2df0,0x2dfd,0x2e0c,0x2e11,0x2e1e,0x2e20,0x2e25,0x2e2c,0x2e57,0x2e5b,0x2e64,0x2e6d,0x2e6f,0x2e78,0x2e90,0x2e99,0x2e9d,0x2ead,0x2eb1,0x2ec4,0x2f2b,0x2f36,0x2f44,0x2f4b,0x2f59,0x2f73,0x2f7e,0x2f98,0x2fa1,0x2fa9,0x2fb3,0x2fb9,0x2fbb,0x2fc0,0x2fc4,0x2fca,0x2fce,0x2fd2,0x2fd6,0x2fd8,0x3dbc,0x2fe0,0x2ff3,0x2ff7,0x2ffe,0x3004,0x3015,0x3019,0x3027,0x3030,0x3055,0x305f,0x306e,0x3080,0x3098,0x30a0,0x30d4,0x30eb,0x30f2,0x31a8,0x31c2,0x31d8,0x3211,0x321c,0x325c,0x327c,0x32bc,0x32dc,0x38ec,0x32f0,0x3317,0x3324,0x3334,0x3354,0x3374,0x339c,0x340b,0x3420,0x3428,0x3430,0x3434,0x3444,0x3448,0x3450,0x3458,0x3474,0x3480,0x3488,0x3490,0x34b5,0x34b9,0x34fa,0x3536,0x35ba,0x35c4,0x35d8,0x35ec,0x35f0,0x35fa,0x3609,0x3614,0x361d,0x3627,0x362c,0x363c,0x364b,0x3651,0x365f,0x3667,0x366e,0x3674,0x3693,0x369b,0x36a5,0x36a9,0x36ae,0x36c0,0x36df,0x36e7,0x36f1,0x36f5,0x36fa,0x370c,0x37a7,0x37c6,0x37cd,0x37d4,0x37de,0x37ee,0x37f9,0x37fe,0x380c,0x3817,0x3827,0x3840,0x384b,0x3852,0x386d,0x3874,0x388e,0x38ad,0x38c0,0x38d4,0x38e6,0x38ec,0x38f2,0x38f8,0x38fe,0x3904,0x390a,0x3910,0x3916,0x391c,0x3922,0x3928,0x392e,0x3934,0x393a,0x3940,0x3946,0x394c,0x3952,0x3958,0x395e,0x3964,0x396a,0x3970,0x3976,0x397c,0x3982,0x3988,0x398e,0x3994,0x399a,0x39a0,0x39a6,0x39ac,0x39b0,0x39d0,0x39e8,0x39fb,0x3a13,0x3a1d,0x2ae0,0x3a2c,0x3a7f,0x3a90,0x3aac,0x3ad0,0x3dd4,0x3dda,0x3dee,0x3dfa };
				for (int i = 0; i < sizeof(rva) / sizeof(rva[0]); i++) {
					FluffiBasicBlock b(rva[i], 0);
					blocksToCover.insert(b);
				}

				std::thread debuggerThread(&TestExecutorGDB::debuggerThreadMain, ".\\gdb.exe", gDBThreadCommunication, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT, gdbInitFile, sharedMemIPCInterruptEvent, modulesToCover, blocksToCover, 0xcc, 1);
				debuggerThread.detach();

				bool isDebugged = TestExecutorGDB::waitUntilTargetIsBeingDebugged(gDBThreadCommunication, 5000);
				Assert::IsTrue(isDebugged, L"The target failed to reach the debugged stage");

				//Wait until the target is up and running
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));

				Assert::IsTrue(sharedMemIPC_toTarget.initializeAsClient(), L"initializeAsClient failed");

				SharedMemMessage message;
				Assert::IsTrue(sharedMemIPC_toTarget.waitForNewMessageToClient(&message, 5000, sharedMemIPCInterruptEvent), L"Failed to receive the ready to fuzz message(1)");
				Assert::IsTrue(message.getMessageType() == SHARED_MEM_MESSAGE_READY_TO_FUZZ, L"Failed to receive the ready to fuzz message(2)");

				gDBThreadCommunication->set_coverageState(GDBThreadCommunication::COVERAGE_STATE::SHOULD_RESET_HARD);

				bool isResetted = TestExecutorGDB::waitUntilCoverageState(gDBThreadCommunication, GDBThreadCommunication::COVERAGE_STATE::RESETTED, 5000);
				Assert::IsTrue(isResetted, L"The target failed to reach the resetted stage");

				SharedMemMessage fuzzFilenameMsg{ SHARED_MEM_MESSAGE_FUZZ_FILENAME, testcaseFile.c_str(),(int)testcaseFile.length() };
				Assert::IsTrue(sharedMemIPC_toTarget.sendMessageToServer(&fuzzFilenameMsg), L"Failed to send the fuzzFilenameMsg to the server");

				Assert::IsTrue(sharedMemIPC_toTarget.waitForNewMessageToClient(&message, 30000, sharedMemIPCInterruptEvent), L"Failed to receive the SHARED_MEM_MESSAGE_FUZZ_DONE message(1)");
				Assert::IsTrue(message.getMessageType() == SHARED_MEM_MESSAGE_FUZZ_DONE, L"Failed to receive the SHARED_MEM_MESSAGE_FUZZ_DONE message(2)");

				gDBThreadCommunication->set_coverageState(GDBThreadCommunication::COVERAGE_STATE::SHOULD_DUMP);

				bool isDumped = TestExecutorGDB::waitUntilCoverageState(gDBThreadCommunication, GDBThreadCommunication::COVERAGE_STATE::DUMPED, 5000);
				Assert::IsTrue(isDumped, L"The target failed to reach the dumped stage");

				Assert::IsTrue(gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed");

				if (saveBlocksToFile)
				{
					std::string filename = "gdb_tempListOfBlocks.csv";
					std::ofstream fout;
					fout.open(filename, std::ios::binary | std::ios::out);
					for (int i = 0; i < gDBThreadCommunication->m_exOutput.getCoveredBasicBlocks().size(); i++) {
						fout << gDBThreadCommunication->m_exOutput.getCoveredBasicBlocks().at(i).m_rva << ";;;" << "0x14" << std::hex << std::setw(7) << std::setfill('0') << gDBThreadCommunication->m_exOutput.getCoveredBasicBlocks().at(i).m_rva + 0x1000 << ";" << std::dec << gDBThreadCommunication->m_exOutput.getCoveredBasicBlocks().at(i).m_rva + 0x1000 << std::endl;
					}
					fout.close();
				}

				Assert::IsTrue(matchesBlocklist(expectedBlocks, sizeof(expectedBlocks) / sizeof(expectedBlocks[0]), std::make_shared<DebugExecutionOutput>(gDBThreadCommunication->m_exOutput)), L"Blocks didn't match (1)");

				gDBThreadCommunication->set_gdbThreadShouldTerminate();

				//Cleanup
				CloseHandle(sharedMemIPCInterruptEvent);
				std::remove(gdbInitFile.c_str());
				std::remove(testcaseFile.c_str());
			}
		}

		TEST_METHOD(TestExecutorGDB_debuggerThreadMain_crash) {
			//Check crash
			{
				HANDLE sharedMemIPCInterruptEvent = CreateEvent(NULL, false, false, NULL);
				Assert::IsFalse(sharedMemIPCInterruptEvent == NULL, L"Failed to create the SharedMemIPCInterruptEvent");

				SharedMemIPC sharedMemIPC_toTarget(std::string("FLUFFI_SharedMemForTest").c_str());

				std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication = std::make_shared<GDBThreadCommunication>();

				std::string gdbInitFile = ".\\gdbinitfile";
				bool saveBlocksToFile = true;

				std::ofstream initCommandFile(gdbInitFile, std::ofstream::out);
				initCommandFile << "file InstrumentedDebuggerTester.exe" << std::endl;
				initCommandFile << "set args FLUFFI_SharedMemForTest" << std::endl;
				initCommandFile.close();

				std::string testcaseFile = ".\\gdbtestcaseFile";

				std::ofstream testcaseFileOS(testcaseFile, std::ios::binary | std::ios::out);
				testcaseFileOS << (char)2;
				testcaseFileOS.close();

				std::set<Module> modulesToCover;
				modulesToCover.emplace("InstrumentedDebuggerTester.exe/.text", "*", 0);
				std::set<FluffiBasicBlock>blocksToCover;
				int rva[] = { 0x0010,0x0056,0x0074,0x007a,0x008a,0x00aa,0x00cb,0x010b,0x011f,0x3ae0,0x3aec,0x3af8,0x2970,0x0150,0x09a0,0x0150,0x0180,0x01be,0x01da,0x01ef,0x021e,0x0223,0x0226,0x022f,0x023a,0x0296,0x02a1,0x02b1,0x02b6,0x02bd,0x02c6,0x02cd,0x02d6,0x02dd,0x02e3,0x02ea,0x02ed,0x02f2,0x02fb,0x0333,0x034c,0x0375,0x0390,0x03a3,0x03b6,0x03ef,0x0402,0x0474,0x04b8,0x04e3,0x0509,0x0518,0x0523,0x052e,0x053e,0x0547,0x0554,0x0561,0x056b,0x056e,0x0573,0x0586,0x058b,0x059a,0x05a3,0x05b0,0x05bd,0x05c7,0x05ca,0x05de,0x05e8,0x05f7,0x0600,0x060d,0x061a,0x0624,0x0627,0x062c,0x0641,0x0644,0x0653,0x065c,0x0669,0x0676,0x0680,0x0683,0x0697,0x06a1,0x06b0,0x06b9,0x06c6,0x06d3,0x06dd,0x06e0,0x06e5,0x071c,0x0721,0x0726,0x072f,0x0754,0x079e,0x07b1,0x07bc,0x07f4,0x0803,0x080e,0x0823,0x084c,0x0851,0x0858,0x085f,0x0866,0x086d,0x0874,0x087b,0x0882,0x0889,0x0890,0x0897,0x089e,0x08a5,0x08ac,0x08b3,0x08ba,0x08c1,0x08c8,0x08cf,0x08d6,0x08dd,0x0902,0x0927,0x094d,0x0950,0x095b,0x0966,0x0973,0x3b10,0x3b1e,0x3b2a,0x3b38,0x3b46,0x3b52,0x3b5e,0x3b6a,0x3b78,0x2970,0x09a0,0x09a0,0x09b1,0x09c0,0x09c5,0x09cc,0x09d5,0x09dc,0x09e5,0x09ec,0x09f2,0x09f9,0x09fc,0x0a0e,0x0a20,0x0a42,0x0a4b,0x0a54,0x0a5b,0x0a6e,0x0a79,0x0a9b,0x0ab0,0x0b02,0x0b16,0x0b23,0x0b45,0x0b4b,0x0b54,0x0b5e,0x38e0,0x0b80,0x0bc4,0x0be3,0x0c15,0x0c4e,0x0c55,0x0c58,0x0c68,0x0c7f,0x3b90,0x3ba3,0x3bb8,0x3bbe,0x3bd0,0x3bde,0x16a0,0x0ca0,0x0cc5,0x0cd8,0x0cf0,0x0d03,0x0d11,0x0d22,0x0d2b,0x0d40,0x0d58,0x0d5d,0x0d62,0x0d68,0x0d6b,0x0d75,0x0d91,0x0da0,0x0ddb,0x0de4,0x0dfa,0x0dff,0x0e17,0x0e2d,0x0e57,0x0e72,0x0e90,0x0ec3,0x0ec9,0x0ed0,0x0ed3,0x0ee1,0x0eea,0x0eef,0x0ef4,0x0f0b,0x0f21,0x0f2a,0x0f4c,0x0f66,0x0f81,0x0fa0,0x0fdc,0x0fec,0x1008,0x1012,0x101a,0x1027,0x1047,0x104e,0x1060,0x1068,0x1070,0x1084,0x109a,0x1104,0x1109,0x1112,0x1119,0x111e,0x1130,0x114b,0x1156,0x115e,0x117d,0x1181,0x1187,0x118c,0x11a8,0x11b0,0x11ca,0x11d0,0x11d4,0x11da,0x11e1,0x11eb,0x11f6,0x11fc,0x1203,0x120c,0x1213,0x121c,0x1223,0x1229,0x1230,0x1233,0x123b,0x123d,0x3bf0,0x2970,0x1260,0x1275,0x1284,0x128d,0x1293,0x12a2,0x12af,0x12d0,0x12eb,0x12f4,0x12f9,0x1301,0x1321,0x132d,0x1332,0x1339,0x1348,0x1355,0x1364,0x137c,0x1393,0x13a0,0x13b0,0x13ea,0x13f6,0x1402,0x1412,0x1430,0x143a,0x1442,0x144f,0x146f,0x1476,0x1494,0x14c2,0x14d0,0x1522,0x152b,0x1548,0x1576,0x1584,0x158d,0x1594,0x15a8,0x15ac,0x15b1,0x15d4,0x15dc,0x15df,0x15e5,0x15f1,0x15f7,0x15fe,0x1607,0x160e,0x1617,0x161e,0x1624,0x162b,0x162e,0x1636,0x1638,0x3bf0,0x2970,0x16a0,0x16c6,0x16d3,0x16f5,0x16fb,0x1704,0x1720,0x1748,0x1759,0x1770,0x1789,0x179d,0x17aa,0x17ca,0x17d0,0x17d9,0x17e3,0x17e9,0x17f7,0x38e0,0x1810,0x182d,0x1837,0x184b,0x1852,0x38e0,0x1860,0x186d,0x188f,0x1890,0x18cb,0x18d5,0x18e3,0x18ea,0x1914,0x1920,0x1956,0x195b,0x195d,0x1961,0x197e,0x19af,0x19b5,0x19be,0x19d2,0x19df,0x19e3,0x19e5,0x19eb,0x19f7,0x19fd,0x1a04,0x1a0d,0x1a14,0x1a1d,0x1a24,0x1a2a,0x1a31,0x1a34,0x1a39,0x1a3e,0x1a40,0x3c00,0x2970,0x1a70,0x1aaf,0x1afd,0x1b30,0x1b4c,0x1b52,0x1b75,0x1bb0,0x1bd8,0x1bef,0x1c0f,0x1c27,0x1c31,0x1c3f,0x1c49,0x1c54,0x1c63,0x1c68,0x1c6a,0x38e0,0x3c10,0x22e0,0x1c80,0x1ca1,0x1ca8,0x1cb0,0x1ce2,0x1ce7,0x1cf0,0x1cf9,0x1d0a,0x1d0f,0x1d14,0x1d16,0x1d28,0x1d2f,0x1d3d,0x1d47,0x1d4c,0x1d52,0x1d65,0x1d69,0x1d73,0x1d81,0x1d86,0x1da2,0x1da7,0x1dab,0x1dc8,0x1dcd,0x1de9,0x1dee,0x1df3,0x1df8,0x1dfc,0x1e0c,0x1e0e,0x1e1c,0x1e32,0x1e3c,0x1e46,0x1e58,0x1e5f,0x38e0,0x3c20,0x3c2c,0x3c38,0x3c45,0x3c62,0x1c80,0x1810,0x1e80,0x1ec3,0x1ec8,0x1ecd,0x1ecf,0x1ee1,0x1ee8,0x1ef6,0x1f00,0x1f05,0x1f0b,0x1f1e,0x1f22,0x1f2c,0x1f3a,0x1f40,0x1f45,0x1f61,0x1f6c,0x1f71,0x1f75,0x1f7c,0x1f7f,0x1f9c,0x1fa3,0x1fa8,0x1fc4,0x1fc9,0x1fce,0x1fd2,0x1fe2,0x1fe4,0x1ff2,0x2008,0x2012,0x201c,0x202d,0x2034,0x38e0,0x3c70,0x3c7c,0x3c88,0x3c95,0x3cb2,0x1c80,0x1810,0x2050,0x2090,0x20c7,0x20ed,0x20fe,0x2100,0x2102,0x2108,0x2114,0x211c,0x2121,0x212b,0x213f,0x2170,0x218e,0x3cc0,0x21bc,0x1720,0x21d0,0x21f0,0x2218,0x2225,0x2240,0x2280,0x22a0,0x22e0,0x22f6,0x2302,0x2311,0x2320,0x2342,0x2350,0x2362,0x2368,0x2376,0x237c,0x238d,0x2397,0x239e,0x23a7,0x23ac,0x23b7,0x23cd,0x23e0,0x23e9,0x23f0,0x23f5,0x23f8,0x2407,0x2412,0x2417,0x241a,0x241e,0x2430,0x2446,0x2455,0x2469,0x247a,0x247f,0x2486,0x2489,0x248e,0x249b,0x24a6,0x24bb,0x24c2,0x24d0,0x24fa,0x2508,0x251b,0x2525,0x2533,0x253e,0x2541,0x254e,0x2561,0x256c,0x2575,0x257c,0x2581,0x2584,0x258d,0x259c,0x25a2,0x25b0,0x25b6,0x25c7,0x25ce,0x25d1,0x25d8,0x25dd,0x25e2,0x25ed,0x25f5,0x25fd,0x2600,0x2605,0x2611,0x261c,0x2621,0x2624,0x2628,0x2650,0x2659,0x2662,0x266b,0x2674,0x267b,0x2694,0x26b0,0x26e4,0x26e9,0x2708,0x271a,0x271e,0x2727,0x272b,0x2734,0x273d,0x2743,0x2759,0x2761,0x2763,0x2777,0x277c,0x2783,0x2788,0x278b,0x2790,0x279b,0x27a5,0x27b3,0x27b8,0x27bf,0x27c8,0x27cf,0x27d8,0x27df,0x27e5,0x27ec,0x27ef,0x27f4,0x280b,0x2810,0x2813,0x2828,0x282b,0x3cd0,0x3cdd,0x3ce5,0x3cf1,0x3cff,0x3d0d,0x3d1b,0x3d27,0x3d2e,0x3d36,0x3d3b,0x3d42,0x3d4b,0x3d52,0x3d5b,0x3d62,0x3d68,0x3d6f,0x3d72,0x3d77,0x3d8e,0x3d91,0x2840,0x285d,0x2867,0x286c,0x286f,0x2874,0x287a,0x287d,0x2886,0x288c,0x2891,0x2894,0x28b7,0x28bd,0x28cb,0x28d1,0x28e5,0x28ec,0x28f1,0x28f6,0x2901,0x291a,0x2930,0x2933,0x2938,0x2943,0x294e,0x2953,0x2956,0x295a,0x24d0,0x2970,0x2983,0x2991,0x2996,0x299d,0x29a6,0x29ad,0x29b6,0x29bd,0x29c3,0x29ca,0x29cd,0x29d2,0x29e9,0x29ec,0x29f8,0x2a19,0x2a3b,0x2a55,0x2a5b,0x2a69,0x38e0,0x2a84,0x2aa1,0x2aaf,0x2abc,0x2ae0,0x2aea,0x2af6,0x2af8,0x2afc,0x30d4,0x2b04,0x390a,0x2b0c,0x2b17,0x2b23,0x2b29,0x2b2e,0x2b30,0x2b35,0x2b38,0x2b42,0x2b48,0x2b04,0x2b50,0x2b68,0x2b72,0x2b7c,0x2bb5,0x2bd6,0x2be4,0x2bf0,0x2c0f,0x2c14,0x2c21,0x2c2c,0x2c38,0x2c48,0x3982,0x2c64,0x2c81,0x2c8c,0x2c94,0x2ca6,0x2cb0,0x2cb4,0x2cd5,0x2cdf,0x2cfe,0x2d06,0x2d1b,0x2d27,0x2d3d,0x2d4b,0x2d57,0x2d5f,0x2d8c,0x2d93,0x2d98,0x2d9d,0x2daa,0x2db5,0x2dbd,0x2dc4,0x2dc9,0x2dcb,0x3d9e,0x3cff,0x2ddc,0x2c64,0x2df0,0x2dfd,0x2e0c,0x2e11,0x2e1e,0x2e20,0x2e25,0x2e2c,0x2e57,0x2e5b,0x2e64,0x2e6d,0x2e6f,0x2e78,0x2e90,0x2e99,0x2e9d,0x2ead,0x2eb1,0x2ec4,0x2f2b,0x2f36,0x2f44,0x2f4b,0x2f59,0x2f73,0x2f7e,0x2f98,0x2fa1,0x2fa9,0x2fb3,0x2fb9,0x2fbb,0x2fc0,0x2fc4,0x2fca,0x2fce,0x2fd2,0x2fd6,0x2fd8,0x3dbc,0x2fe0,0x2ff3,0x2ff7,0x2ffe,0x3004,0x3015,0x3019,0x3027,0x3030,0x3055,0x305f,0x306e,0x3080,0x3098,0x30a0,0x30d4,0x30eb,0x30f2,0x31a8,0x31c2,0x31d8,0x3211,0x321c,0x325c,0x327c,0x32bc,0x32dc,0x38ec,0x32f0,0x3317,0x3324,0x3334,0x3354,0x3374,0x339c,0x340b,0x3420,0x3428,0x3430,0x3434,0x3444,0x3448,0x3450,0x3458,0x3474,0x3480,0x3488,0x3490,0x34b5,0x34b9,0x34fa,0x3536,0x35ba,0x35c4,0x35d8,0x35ec,0x35f0,0x35fa,0x3609,0x3614,0x361d,0x3627,0x362c,0x363c,0x364b,0x3651,0x365f,0x3667,0x366e,0x3674,0x3693,0x369b,0x36a5,0x36a9,0x36ae,0x36c0,0x36df,0x36e7,0x36f1,0x36f5,0x36fa,0x370c,0x37a7,0x37c6,0x37cd,0x37d4,0x37de,0x37ee,0x37f9,0x37fe,0x380c,0x3817,0x3827,0x3840,0x384b,0x3852,0x386d,0x3874,0x388e,0x38ad,0x38c0,0x38d4,0x38e6,0x38ec,0x38f2,0x38f8,0x38fe,0x3904,0x390a,0x3910,0x3916,0x391c,0x3922,0x3928,0x392e,0x3934,0x393a,0x3940,0x3946,0x394c,0x3952,0x3958,0x395e,0x3964,0x396a,0x3970,0x3976,0x397c,0x3982,0x3988,0x398e,0x3994,0x399a,0x39a0,0x39a6,0x39ac,0x39b0,0x39d0,0x39e8,0x39fb,0x3a13,0x3a1d,0x2ae0,0x3a2c,0x3a7f,0x3a90,0x3aac,0x3ad0,0x3dd4,0x3dda,0x3dee,0x3dfa };
				for (int i = 0; i < sizeof(rva) / sizeof(rva[0]); i++) {
					FluffiBasicBlock b(rva[i], 0);
					blocksToCover.insert(b);
				}

				std::thread debuggerThread(&TestExecutorGDB::debuggerThreadMain, ".\\gdb.exe", gDBThreadCommunication, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT, gdbInitFile, sharedMemIPCInterruptEvent, modulesToCover, blocksToCover, 0xcc, 1);
				debuggerThread.detach();

				bool isDebugged = TestExecutorGDB::waitUntilTargetIsBeingDebugged(gDBThreadCommunication, 5000);
				Assert::IsTrue(isDebugged, L"The target failed to reach the debugged stage");

				//Wait until the target is up and running
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));

				Assert::IsTrue(sharedMemIPC_toTarget.initializeAsClient(), L"initializeAsClient failed");

				SharedMemMessage message;
				Assert::IsTrue(sharedMemIPC_toTarget.waitForNewMessageToClient(&message, 5000, sharedMemIPCInterruptEvent), L"Failed to receive the ready to fuzz message(1)");
				Assert::IsTrue(message.getMessageType() == SHARED_MEM_MESSAGE_READY_TO_FUZZ, L"Failed to receive the ready to fuzz message(2)");

				gDBThreadCommunication->set_coverageState(GDBThreadCommunication::COVERAGE_STATE::SHOULD_RESET_HARD);

				bool isResetted = TestExecutorGDB::waitUntilCoverageState(gDBThreadCommunication, GDBThreadCommunication::COVERAGE_STATE::RESETTED, 5000);
				Assert::IsTrue(isResetted, L"The target failed to reach the resetted stage");

				SharedMemMessage fuzzFilenameMsg{ SHARED_MEM_MESSAGE_FUZZ_FILENAME, testcaseFile.c_str(),(int)testcaseFile.length() };
				Assert::IsTrue(sharedMemIPC_toTarget.sendMessageToServer(&fuzzFilenameMsg), L"Failed to send the fuzzFilenameMsg to the server");

				Assert::IsFalse(sharedMemIPC_toTarget.waitForNewMessageToClient(&message, 30000, sharedMemIPCInterruptEvent), L"Failed to receive the SHARED_MEM_MESSAGE_WAIT_INTERRUPTED message(1)");

				Assert::IsTrue(message.getMessageType() == SHARED_MEM_MESSAGE_WAIT_INTERRUPTED, L"Failed to receive the SHARED_MEM_MESSAGE_WAIT_INTERRUPTED message(2)");

				Assert::IsTrue(gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::EXCEPTION_OTHER, L"The expected crash was not observed");

				Assert::IsTrue(gDBThreadCommunication->m_exOutput.m_firstCrash == gDBThreadCommunication->m_exOutput.m_lastCrash, L"m_firstCrash was not m_lastCrash");

				Assert::IsTrue(gDBThreadCommunication->m_exOutput.m_lastCrash == "InstrumentedDebuggerTester.exe/.text+0x000007a4", L"The observed crash was not the expected one");

				gDBThreadCommunication->set_gdbThreadShouldTerminate();

				//Cleanup
				CloseHandle(sharedMemIPCInterruptEvent);
				std::remove(gdbInitFile.c_str());
				std::remove(testcaseFile.c_str());
			}
		}

		TEST_METHOD(TestExecutorGDB_vector_swap) {
			DebugExecutionOutput exResult1;
			exResult1.addCoveredBasicBlock(FluffiBasicBlock{ 1,1 });

			DebugExecutionOutput exResult2;
			exResult2.addCoveredBasicBlock(FluffiBasicBlock{ 2,2 });
			exResult2.addCoveredBasicBlock(FluffiBasicBlock{ 3,3 });

			Assert::IsTrue(exResult1.getCoveredBasicBlocks().size() == 1 && exResult2.getCoveredBasicBlocks().size() == 2, L"sizes pre-swap are not as expected");

			exResult1.swapBlockVectorWith(exResult2);

			Assert::IsTrue(exResult1.getCoveredBasicBlocks().size() == 2 && exResult2.getCoveredBasicBlocks().size() == 1, L"sizes post-swap are not as expected");
			Assert::IsTrue(exResult2.getCoveredBasicBlocks().at(0).m_moduleID == 1 && exResult1.getCoveredBasicBlocks().at(0).m_moduleID == 2, L"values post-swap are not as expected");
		}

		TEST_METHOD(TestExecutorGDB_runSingleTestcase) {
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir_gdb";

			std::error_code errorcode;
			std::experimental::filesystem::remove(testcaseDir, errorcode);

			Util::createFolderAndParentFolders(testcaseDir);
			{
				std::set<Module> modulesToCover;
				GarbageCollectorWorker garbageCollector(0);
				modulesToCover.emplace("InstrumentedDebuggerTester.exe/.text", "*", 0);
				std::set<FluffiBasicBlock> blocksToCover;
				int rva[] = { 0x0010,0x0056,0x0074,0x007a,0x008a,0x00aa,0x00cb,0x010b,0x011f,0x3ae0,0x3aec,0x3af8,0x2970,0x0150,0x09a0,0x0150,0x0180,0x01be,0x01da,0x01ef,0x021e,0x0223,0x0226,0x022f,0x023a,0x0296,0x02a1,0x02b1,0x02b6,0x02bd,0x02c6,0x02cd,0x02d6,0x02dd,0x02e3,0x02ea,0x02ed,0x02f2,0x02fb,0x0333,0x034c,0x0375,0x0390,0x03a3,0x03b6,0x03ef,0x0402,0x0474,0x04b8,0x04e3,0x0509,0x0518,0x0523,0x052e,0x053e,0x0547,0x0554,0x0561,0x056b,0x056e,0x0573,0x0586,0x058b,0x059a,0x05a3,0x05b0,0x05bd,0x05c7,0x05ca,0x05de,0x05e8,0x05f7,0x0600,0x060d,0x061a,0x0624,0x0627,0x062c,0x0641,0x0644,0x0653,0x065c,0x0669,0x0676,0x0680,0x0683,0x0697,0x06a1,0x06b0,0x06b9,0x06c6,0x06d3,0x06dd,0x06e0,0x06e5,0x071c,0x0721,0x0726,0x072f,0x0754,0x079e,0x07b1,0x07bc,0x07f4,0x0803,0x080e,0x0823,0x084c,0x0851,0x0858,0x085f,0x0866,0x086d,0x0874,0x087b,0x0882,0x0889,0x0890,0x0897,0x089e,0x08a5,0x08ac,0x08b3,0x08ba,0x08c1,0x08c8,0x08cf,0x08d6,0x08dd,0x0902,0x0927,0x094d,0x0950,0x095b,0x0966,0x0973,0x3b10,0x3b1e,0x3b2a,0x3b38,0x3b46,0x3b52,0x3b5e,0x3b6a,0x3b78,0x2970,0x09a0,0x09a0,0x09b1,0x09c0,0x09c5,0x09cc,0x09d5,0x09dc,0x09e5,0x09ec,0x09f2,0x09f9,0x09fc,0x0a0e,0x0a20,0x0a42,0x0a4b,0x0a54,0x0a5b,0x0a6e,0x0a79,0x0a9b,0x0ab0,0x0b02,0x0b16,0x0b23,0x0b45,0x0b4b,0x0b54,0x0b5e,0x38e0,0x0b80,0x0bc4,0x0be3,0x0c15,0x0c4e,0x0c55,0x0c58,0x0c68,0x0c7f,0x3b90,0x3ba3,0x3bb8,0x3bbe,0x3bd0,0x3bde,0x16a0,0x0ca0,0x0cc5,0x0cd8,0x0cf0,0x0d03,0x0d11,0x0d22,0x0d2b,0x0d40,0x0d58,0x0d5d,0x0d62,0x0d68,0x0d6b,0x0d75,0x0d91,0x0da0,0x0ddb,0x0de4,0x0dfa,0x0dff,0x0e17,0x0e2d,0x0e57,0x0e72,0x0e90,0x0ec3,0x0ec9,0x0ed0,0x0ed3,0x0ee1,0x0eea,0x0eef,0x0ef4,0x0f0b,0x0f21,0x0f2a,0x0f4c,0x0f66,0x0f81,0x0fa0,0x0fdc,0x0fec,0x1008,0x1012,0x101a,0x1027,0x1047,0x104e,0x1060,0x1068,0x1070,0x1084,0x109a,0x1104,0x1109,0x1112,0x1119,0x111e,0x1130,0x114b,0x1156,0x115e,0x117d,0x1181,0x1187,0x118c,0x11a8,0x11b0,0x11ca,0x11d0,0x11d4,0x11da,0x11e1,0x11eb,0x11f6,0x11fc,0x1203,0x120c,0x1213,0x121c,0x1223,0x1229,0x1230,0x1233,0x123b,0x123d,0x3bf0,0x2970,0x1260,0x1275,0x1284,0x128d,0x1293,0x12a2,0x12af,0x12d0,0x12eb,0x12f4,0x12f9,0x1301,0x1321,0x132d,0x1332,0x1339,0x1348,0x1355,0x1364,0x137c,0x1393,0x13a0,0x13b0,0x13ea,0x13f6,0x1402,0x1412,0x1430,0x143a,0x1442,0x144f,0x146f,0x1476,0x1494,0x14c2,0x14d0,0x1522,0x152b,0x1548,0x1576,0x1584,0x158d,0x1594,0x15a8,0x15ac,0x15b1,0x15d4,0x15dc,0x15df,0x15e5,0x15f1,0x15f7,0x15fe,0x1607,0x160e,0x1617,0x161e,0x1624,0x162b,0x162e,0x1636,0x1638,0x3bf0,0x2970,0x16a0,0x16c6,0x16d3,0x16f5,0x16fb,0x1704,0x1720,0x1748,0x1759,0x1770,0x1789,0x179d,0x17aa,0x17ca,0x17d0,0x17d9,0x17e3,0x17e9,0x17f7,0x38e0,0x1810,0x182d,0x1837,0x184b,0x1852,0x38e0,0x1860,0x186d,0x188f,0x1890,0x18cb,0x18d5,0x18e3,0x18ea,0x1914,0x1920,0x1956,0x195b,0x195d,0x1961,0x197e,0x19af,0x19b5,0x19be,0x19d2,0x19df,0x19e3,0x19e5,0x19eb,0x19f7,0x19fd,0x1a04,0x1a0d,0x1a14,0x1a1d,0x1a24,0x1a2a,0x1a31,0x1a34,0x1a39,0x1a3e,0x1a40,0x3c00,0x2970,0x1a70,0x1aaf,0x1afd,0x1b30,0x1b4c,0x1b52,0x1b75,0x1bb0,0x1bd8,0x1bef,0x1c0f,0x1c27,0x1c31,0x1c3f,0x1c49,0x1c54,0x1c63,0x1c68,0x1c6a,0x38e0,0x3c10,0x22e0,0x1c80,0x1ca1,0x1ca8,0x1cb0,0x1ce2,0x1ce7,0x1cf0,0x1cf9,0x1d0a,0x1d0f,0x1d14,0x1d16,0x1d28,0x1d2f,0x1d3d,0x1d47,0x1d4c,0x1d52,0x1d65,0x1d69,0x1d73,0x1d81,0x1d86,0x1da2,0x1da7,0x1dab,0x1dc8,0x1dcd,0x1de9,0x1dee,0x1df3,0x1df8,0x1dfc,0x1e0c,0x1e0e,0x1e1c,0x1e32,0x1e3c,0x1e46,0x1e58,0x1e5f,0x38e0,0x3c20,0x3c2c,0x3c38,0x3c45,0x3c62,0x1c80,0x1810,0x1e80,0x1ec3,0x1ec8,0x1ecd,0x1ecf,0x1ee1,0x1ee8,0x1ef6,0x1f00,0x1f05,0x1f0b,0x1f1e,0x1f22,0x1f2c,0x1f3a,0x1f40,0x1f45,0x1f61,0x1f6c,0x1f71,0x1f75,0x1f7c,0x1f7f,0x1f9c,0x1fa3,0x1fa8,0x1fc4,0x1fc9,0x1fce,0x1fd2,0x1fe2,0x1fe4,0x1ff2,0x2008,0x2012,0x201c,0x202d,0x2034,0x38e0,0x3c70,0x3c7c,0x3c88,0x3c95,0x3cb2,0x1c80,0x1810,0x2050,0x2090,0x20c7,0x20ed,0x20fe,0x2100,0x2102,0x2108,0x2114,0x211c,0x2121,0x212b,0x213f,0x2170,0x218e,0x3cc0,0x21bc,0x1720,0x21d0,0x21f0,0x2218,0x2225,0x2240,0x2280,0x22a0,0x22e0,0x22f6,0x2302,0x2311,0x2320,0x2342,0x2350,0x2362,0x2368,0x2376,0x237c,0x238d,0x2397,0x239e,0x23a7,0x23ac,0x23b7,0x23cd,0x23e0,0x23e9,0x23f0,0x23f5,0x23f8,0x2407,0x2412,0x2417,0x241a,0x241e,0x2430,0x2446,0x2455,0x2469,0x247a,0x247f,0x2486,0x2489,0x248e,0x249b,0x24a6,0x24bb,0x24c2,0x24d0,0x24fa,0x2508,0x251b,0x2525,0x2533,0x253e,0x2541,0x254e,0x2561,0x256c,0x2575,0x257c,0x2581,0x2584,0x258d,0x259c,0x25a2,0x25b0,0x25b6,0x25c7,0x25ce,0x25d1,0x25d8,0x25dd,0x25e2,0x25ed,0x25f5,0x25fd,0x2600,0x2605,0x2611,0x261c,0x2621,0x2624,0x2628,0x2650,0x2659,0x2662,0x266b,0x2674,0x267b,0x2694,0x26b0,0x26e4,0x26e9,0x2708,0x271a,0x271e,0x2727,0x272b,0x2734,0x273d,0x2743,0x2759,0x2761,0x2763,0x2777,0x277c,0x2783,0x2788,0x278b,0x2790,0x279b,0x27a5,0x27b3,0x27b8,0x27bf,0x27c8,0x27cf,0x27d8,0x27df,0x27e5,0x27ec,0x27ef,0x27f4,0x280b,0x2810,0x2813,0x2828,0x282b,0x3cd0,0x3cdd,0x3ce5,0x3cf1,0x3cff,0x3d0d,0x3d1b,0x3d27,0x3d2e,0x3d36,0x3d3b,0x3d42,0x3d4b,0x3d52,0x3d5b,0x3d62,0x3d68,0x3d6f,0x3d72,0x3d77,0x3d8e,0x3d91,0x2840,0x285d,0x2867,0x286c,0x286f,0x2874,0x287a,0x287d,0x2886,0x288c,0x2891,0x2894,0x28b7,0x28bd,0x28cb,0x28d1,0x28e5,0x28ec,0x28f1,0x28f6,0x2901,0x291a,0x2930,0x2933,0x2938,0x2943,0x294e,0x2953,0x2956,0x295a,0x24d0,0x2970,0x2983,0x2991,0x2996,0x299d,0x29a6,0x29ad,0x29b6,0x29bd,0x29c3,0x29ca,0x29cd,0x29d2,0x29e9,0x29ec,0x29f8,0x2a19,0x2a3b,0x2a55,0x2a5b,0x2a69,0x38e0,0x2a84,0x2aa1,0x2aaf,0x2abc,0x2ae0,0x2aea,0x2af6,0x2af8,0x2afc,0x30d4,0x2b04,0x390a,0x2b0c,0x2b17,0x2b23,0x2b29,0x2b2e,0x2b30,0x2b35,0x2b38,0x2b42,0x2b48,0x2b04,0x2b50,0x2b68,0x2b72,0x2b7c,0x2bb5,0x2bd6,0x2be4,0x2bf0,0x2c0f,0x2c14,0x2c21,0x2c2c,0x2c38,0x2c48,0x3982,0x2c64,0x2c81,0x2c8c,0x2c94,0x2ca6,0x2cb0,0x2cb4,0x2cd5,0x2cdf,0x2cfe,0x2d06,0x2d1b,0x2d27,0x2d3d,0x2d4b,0x2d57,0x2d5f,0x2d8c,0x2d93,0x2d98,0x2d9d,0x2daa,0x2db5,0x2dbd,0x2dc4,0x2dc9,0x2dcb,0x3d9e,0x3cff,0x2ddc,0x2c64,0x2df0,0x2dfd,0x2e0c,0x2e11,0x2e1e,0x2e20,0x2e25,0x2e2c,0x2e57,0x2e5b,0x2e64,0x2e6d,0x2e6f,0x2e78,0x2e90,0x2e99,0x2e9d,0x2ead,0x2eb1,0x2ec4,0x2f2b,0x2f36,0x2f44,0x2f4b,0x2f59,0x2f73,0x2f7e,0x2f98,0x2fa1,0x2fa9,0x2fb3,0x2fb9,0x2fbb,0x2fc0,0x2fc4,0x2fca,0x2fce,0x2fd2,0x2fd6,0x2fd8,0x3dbc,0x2fe0,0x2ff3,0x2ff7,0x2ffe,0x3004,0x3015,0x3019,0x3027,0x3030,0x3055,0x305f,0x306e,0x3080,0x3098,0x30a0,0x30d4,0x30eb,0x30f2,0x31a8,0x31c2,0x31d8,0x3211,0x321c,0x325c,0x327c,0x32bc,0x32dc,0x38ec,0x32f0,0x3317,0x3324,0x3334,0x3354,0x3374,0x339c,0x340b,0x3420,0x3428,0x3430,0x3434,0x3444,0x3448,0x3450,0x3458,0x3474,0x3480,0x3488,0x3490,0x34b5,0x34b9,0x34fa,0x3536,0x35ba,0x35c4,0x35d8,0x35ec,0x35f0,0x35fa,0x3609,0x3614,0x361d,0x3627,0x362c,0x363c,0x364b,0x3651,0x365f,0x3667,0x366e,0x3674,0x3693,0x369b,0x36a5,0x36a9,0x36ae,0x36c0,0x36df,0x36e7,0x36f1,0x36f5,0x36fa,0x370c,0x37a7,0x37c6,0x37cd,0x37d4,0x37de,0x37ee,0x37f9,0x37fe,0x380c,0x3817,0x3827,0x3840,0x384b,0x3852,0x386d,0x3874,0x388e,0x38ad,0x38c0,0x38d4,0x38e6,0x38ec,0x38f2,0x38f8,0x38fe,0x3904,0x390a,0x3910,0x3916,0x391c,0x3922,0x3928,0x392e,0x3934,0x393a,0x3940,0x3946,0x394c,0x3952,0x3958,0x395e,0x3964,0x396a,0x3970,0x3976,0x397c,0x3982,0x3988,0x398e,0x3994,0x399a,0x39a0,0x39a6,0x39ac,0x39b0,0x39d0,0x39e8,0x39fb,0x3a13,0x3a1d,0x2ae0,0x3a2c,0x3a7f,0x3a90,0x3aac,0x3ad0,0x3dd4,0x3dda,0x3dee,0x3dfa };
				for (int i = 0; i < sizeof(rva) / sizeof(rva[0]); i++) {
					FluffiBasicBlock b(rva[i], 0);
					blocksToCover.insert(b);
				}

§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 15000, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\SharedMemFeeder.exe", ".\\GDBStarter.exe --local \".\\InstrumentedDebuggerTester.exe FLUFFI_SharedMemForTest\"", 6000, 0, blocksToCover, 0xcc, 1);
				Assert::IsTrue(te->isSetupFunctionable(), L"The setup that should be valid is somehow not considered to be valid");
				Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened (1)");

				{
					//Do the first clean run (full coverage will be reported)
					uint32_t expectedBlocks[] = { 16,86,116,138,170,203,267,287,885,912,931,950,1007,1026,1140,1601,1604,1667,1687,1697,1760,1765,1820,1825,1830,1839,1980,2036,2592,2626,2670,2681,2715,2736,2818,2838,2885,2891,2900,2910,2944,3012,3043,3093,3150,3157,3160,3199,3728,3795,3809,3818,3823,3828,3851,3873,3916,3969,4000,4104,4122,4167,4174,4192,4667,4669,6288,6718,6720,6768,6831,6909,6960,6994,7029,7088,7128,7151,7183,7207,7231,7241,7267,7274,7344,7399,7408,7417,7444,7446,7464,7471,7485,7506,7525,7539,7553,7591,7595,7624,7676,7692,7708,7730,7740,7750,7768,7775,7808,7885,7887,7905,7912,7926,7947,7966,7980,7994,8000,8049,8053,8060,8063,8099,8146,8162,8178,8200,8210,8220,8237,8244,8272,8336,8391,8448,8450,8476,8481,8491,8560,8590,9424,9480,9628,9648,9654,9671,9678,9681,9688,9728,9733,9745,9756,9764,9768,9904,9961,10014,10027,10073,10081,10103,10139,10228,10256,10259,10280,10283,10304,10333,10348,10351,10356,10365,10423,10443,10449,10469,10476,10547,10552,10563,10574,10582,10586,10608,10627,10701,10706,10732,10884,10913,10927,10940,10976,10986,10998,11012,11020,11064,11074,14596,14602,14614,15020 };

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = std::make_shared<DebugExecutionOutput>();
					Assert::IsTrue(te->runSingleTestcase(tid, exOutput, TestExecutorGDB::CoverageMode::FULL), L"Run single testcase failed");

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(1)");

					bool doBlocksMatch = matchesBlocklist(expectedBlocks, sizeof(expectedBlocks) / sizeof(expectedBlocks[0]), exOutput);
					if (!doBlocksMatch) {
						int debug = 0;
					}
					Assert::IsTrue(doBlocksMatch, L"Blocks didn't match (1)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectNow();
				}

				{
					//Do a second clean run (as partial coverage will be reported - only incremental coverage will be reported). It differs due to compiler optimization that first loop run != other loop runs
					uint32_t expectedBlocks_1[] = { 2062,2083, 2124 };
					uint32_t expectedBlocks_2[] = { 2051, 2062,2083, 2124 };
					uint32_t expectedBlocks_3[] = { 2124 };
					uint32_t expectedBlocks_4[] = { 2083, 2124 };

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = std::make_shared<DebugExecutionOutput>();
					Assert::IsTrue(te->runSingleTestcase(tid, exOutput, TestExecutorGDB::CoverageMode::PARTIAL), L"Run single testcase failed");

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(2)");

					bool doBlocksMatch = matchesBlocklist(expectedBlocks_1, sizeof(expectedBlocks_1) / sizeof(expectedBlocks_1[0]), exOutput) || matchesBlocklist(expectedBlocks_2, sizeof(expectedBlocks_2) / sizeof(expectedBlocks_2[0]), exOutput) || matchesBlocklist(expectedBlocks_3, sizeof(expectedBlocks_3) / sizeof(expectedBlocks_3[0]), exOutput) || matchesBlocklist(expectedBlocks_4, sizeof(expectedBlocks_4) / sizeof(expectedBlocks_4[0]), exOutput);
					if (!doBlocksMatch) {
						int debug = 0;
					}
					Assert::IsTrue(doBlocksMatch, L"Blocks didn't match (2)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}

				{
					//Do a third clean run (as partial coverage will be reported - only incremental coverage will be reported)

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = std::make_shared<DebugExecutionOutput>();
					Assert::IsTrue(te->runSingleTestcase(tid, exOutput, TestExecutorGDB::CoverageMode::PARTIAL), L"Run single testcase failed");

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(3)");

					Assert::IsTrue(exOutput->getCoveredBasicBlocks().size() == 0, L"Blocks didn't match (3)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}

				{
					//Do a access violation run

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)1;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = std::make_shared<DebugExecutionOutput>();
					Assert::IsFalse(te->runSingleTestcase(tid, exOutput, TestExecutorGDB::CoverageMode::PARTIAL), L"Run single testcase failed");

					Assert::IsTrue(te->m_gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::EXCEPTION_ACCESSVIOLATION, L"The expected access violation was not observed(4)");

					Assert::IsTrue(te->m_gDBThreadCommunication->m_exOutput.m_firstCrash == te->m_gDBThreadCommunication->m_exOutput.m_lastCrash, L"m_firstCrash was not m_lastCrash(4)");

					Assert::IsTrue(te->m_gDBThreadCommunication->m_exOutput.m_lastCrash == "InstrumentedDebuggerTester.exe/.text+0x000007b1", L"The observed crash was not the expected one(4)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}
				Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened (2)");
				{
					//Do a crash run

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)2;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = std::make_shared<DebugExecutionOutput>();
					Assert::IsFalse(te->runSingleTestcase(tid, exOutput, TestExecutorGDB::CoverageMode::PARTIAL), L"Run single testcase failed");

					Assert::IsTrue(te->m_gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::EXCEPTION_OTHER, L"The expected access violation was not observed(5)");

					Assert::IsTrue(te->m_gDBThreadCommunication->m_exOutput.m_firstCrash == te->m_gDBThreadCommunication->m_exOutput.m_lastCrash, L"m_firstCrash was not m_lastCrash(5)");

					Assert::IsTrue(te->m_gDBThreadCommunication->m_exOutput.m_lastCrash == "InstrumentedDebuggerTester.exe/.text+0x000007a4", L"The observed crash was not the expected one(5)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}
				Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened (3)");
				{
					//Do a hang run

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)3;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = std::make_shared<DebugExecutionOutput>();
					Assert::IsFalse(te->runSingleTestcase(tid, exOutput, TestExecutorGDB::CoverageMode::PARTIAL), L"Run single testcase failed");

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::TIMEOUT, L"The expected timeout not observed(6)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}
				Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened (4)");
				{
					//Do a clean run
					uint32_t expectedBlocks[] = { 16,86,116,138,170,203,267,287,885,912,931,950,1007,1026,1140,1601,1604,1667,1687,1697,1760,1765,1820,1825,1830,1980,2036,2592,2626,2670,2681,2715,2736,2818,2838,2885,2891,2900,2910,2944,3012,3043,3093,3150,3157,3160,3199,3728,3795,3809,3818,3823,3828,3851,3873,3916,3969,4000,4104,4122,4167,4174,4192,4667,4669,6288,6718,6720,6768,6831,6909,6960,6994,7029,7088,7128,7151,7183,7207,7231,7241,7267,7274,7344,7399,7408,7417,7444,7446,7464,7471,7485,7506,7525,7539,7553,7591,7595,7624,7676,7692,7708,7730,7740,7750,7768,7775,7808,7885,7887,7905,7912,7926,7947,7966,7980,7994,8000,8049,8053,8060,8063,8099,8146,8162,8178,8200,8210,8220,8237,8244,8272,8336,8391,8448,8450,8476,8481,8491,8560,8590,9424,9480,9628,9648,9654,9671,9678,9681,9688,9728,9733,9745,9756,9764,9768,9904,9961,10014,10027,10073,10081,10103,10139,10228,10256,10259,10280,10283,10304,10333,10348,10351,10356,10365,10423,10443,10449,10469,10476,10547,10552,10563,10574,10582,10586,10608,10627,10701,10706,10732,10884,10913,10927,10940,10976,10986,10998,11012,11020,11064,11074,14596,14602,14614,15020 };

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)0;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = std::make_shared<DebugExecutionOutput>();
					Assert::IsTrue(te->runSingleTestcase(tid, exOutput, TestExecutorGDB::CoverageMode::FULL), L"Run single testcase failed");

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(7)");

					bool doBlocksMatch = matchesBlocklist(expectedBlocks, sizeof(expectedBlocks) / sizeof(expectedBlocks[0]), exOutput);
					if (!doBlocksMatch) {
						int debug = 0;
					}
					Assert::IsTrue(doBlocksMatch, L"Blocks didn't match (7)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectNow();
				}

				{
					//Do a clean run that differs slightly due to the different whattodo value
					uint32_t expectedBlocks_1[] = { 1839,2062,2083, 2124 };
					uint32_t expectedBlocks_2[] = { 1839, 2051, 2062,2083, 2124 };
					uint32_t expectedBlocks_3[] = { 1839,2083, 2124 };
					uint32_t expectedBlocks_4[] = { 1839, 2124 };

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = std::make_shared<DebugExecutionOutput>();
					Assert::IsTrue(te->runSingleTestcase(tid, exOutput, TestExecutorGDB::CoverageMode::PARTIAL), L"Run single testcase failed");

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(8)");

					bool doBlocksMatch = matchesBlocklist(expectedBlocks_1, sizeof(expectedBlocks_1) / sizeof(expectedBlocks_1[0]), exOutput) || matchesBlocklist(expectedBlocks_2, sizeof(expectedBlocks_2) / sizeof(expectedBlocks_2[0]), exOutput) || matchesBlocklist(expectedBlocks_3, sizeof(expectedBlocks_3) / sizeof(expectedBlocks_3[0]), exOutput) || matchesBlocklist(expectedBlocks_4, sizeof(expectedBlocks_4) / sizeof(expectedBlocks_4[0]), exOutput);
					if (!doBlocksMatch) {
						int debug = 0;
					}
					Assert::IsTrue(doBlocksMatch, L"Blocks didn't match (8)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectNow();
				}

				delete te;
			}

			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode), L"testcase directory could not be deleted!");
		}

		TEST_METHOD(TestExecutorGDB_debuggerThreadMain_accessViolation) {
			//Check access violation
			{
				HANDLE sharedMemIPCInterruptEvent = CreateEvent(NULL, false, false, NULL);
				Assert::IsFalse(sharedMemIPCInterruptEvent == NULL, L"Failed to create the SharedMemIPCInterruptEvent");

				SharedMemIPC sharedMemIPC_toTarget(std::string("FLUFFI_SharedMemForTest").c_str());

				std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication = std::make_shared<GDBThreadCommunication>();

				std::string gdbInitFile = ".\\gdbinitfile";
				bool saveBlocksToFile = true;

				std::ofstream initCommandFile(gdbInitFile, std::ofstream::out);
				initCommandFile << "file InstrumentedDebuggerTester.exe" << std::endl;
				initCommandFile << "set args FLUFFI_SharedMemForTest" << std::endl;
				initCommandFile.close();

				std::string testcaseFile = ".\\gdbtestcaseFile";

				std::ofstream testcaseFileOS(testcaseFile, std::ios::binary | std::ios::out);
				testcaseFileOS << (char)1;
				testcaseFileOS.close();

				std::set<Module> modulesToCover;
				modulesToCover.emplace("InstrumentedDebuggerTester.exe/.text", "*", 0);
				std::set<FluffiBasicBlock>blocksToCover;
				int rva[] = { 0x0010,0x0056,0x0074,0x007a,0x008a,0x00aa,0x00cb,0x010b,0x011f,0x3ae0,0x3aec,0x3af8,0x2970,0x0150,0x09a0,0x0150,0x0180,0x01be,0x01da,0x01ef,0x021e,0x0223,0x0226,0x022f,0x023a,0x0296,0x02a1,0x02b1,0x02b6,0x02bd,0x02c6,0x02cd,0x02d6,0x02dd,0x02e3,0x02ea,0x02ed,0x02f2,0x02fb,0x0333,0x034c,0x0375,0x0390,0x03a3,0x03b6,0x03ef,0x0402,0x0474,0x04b8,0x04e3,0x0509,0x0518,0x0523,0x052e,0x053e,0x0547,0x0554,0x0561,0x056b,0x056e,0x0573,0x0586,0x058b,0x059a,0x05a3,0x05b0,0x05bd,0x05c7,0x05ca,0x05de,0x05e8,0x05f7,0x0600,0x060d,0x061a,0x0624,0x0627,0x062c,0x0641,0x0644,0x0653,0x065c,0x0669,0x0676,0x0680,0x0683,0x0697,0x06a1,0x06b0,0x06b9,0x06c6,0x06d3,0x06dd,0x06e0,0x06e5,0x071c,0x0721,0x0726,0x072f,0x0754,0x079e,0x07b1,0x07bc,0x07f4,0x0803,0x080e,0x0823,0x084c,0x0851,0x0858,0x085f,0x0866,0x086d,0x0874,0x087b,0x0882,0x0889,0x0890,0x0897,0x089e,0x08a5,0x08ac,0x08b3,0x08ba,0x08c1,0x08c8,0x08cf,0x08d6,0x08dd,0x0902,0x0927,0x094d,0x0950,0x095b,0x0966,0x0973,0x3b10,0x3b1e,0x3b2a,0x3b38,0x3b46,0x3b52,0x3b5e,0x3b6a,0x3b78,0x2970,0x09a0,0x09a0,0x09b1,0x09c0,0x09c5,0x09cc,0x09d5,0x09dc,0x09e5,0x09ec,0x09f2,0x09f9,0x09fc,0x0a0e,0x0a20,0x0a42,0x0a4b,0x0a54,0x0a5b,0x0a6e,0x0a79,0x0a9b,0x0ab0,0x0b02,0x0b16,0x0b23,0x0b45,0x0b4b,0x0b54,0x0b5e,0x38e0,0x0b80,0x0bc4,0x0be3,0x0c15,0x0c4e,0x0c55,0x0c58,0x0c68,0x0c7f,0x3b90,0x3ba3,0x3bb8,0x3bbe,0x3bd0,0x3bde,0x16a0,0x0ca0,0x0cc5,0x0cd8,0x0cf0,0x0d03,0x0d11,0x0d22,0x0d2b,0x0d40,0x0d58,0x0d5d,0x0d62,0x0d68,0x0d6b,0x0d75,0x0d91,0x0da0,0x0ddb,0x0de4,0x0dfa,0x0dff,0x0e17,0x0e2d,0x0e57,0x0e72,0x0e90,0x0ec3,0x0ec9,0x0ed0,0x0ed3,0x0ee1,0x0eea,0x0eef,0x0ef4,0x0f0b,0x0f21,0x0f2a,0x0f4c,0x0f66,0x0f81,0x0fa0,0x0fdc,0x0fec,0x1008,0x1012,0x101a,0x1027,0x1047,0x104e,0x1060,0x1068,0x1070,0x1084,0x109a,0x1104,0x1109,0x1112,0x1119,0x111e,0x1130,0x114b,0x1156,0x115e,0x117d,0x1181,0x1187,0x118c,0x11a8,0x11b0,0x11ca,0x11d0,0x11d4,0x11da,0x11e1,0x11eb,0x11f6,0x11fc,0x1203,0x120c,0x1213,0x121c,0x1223,0x1229,0x1230,0x1233,0x123b,0x123d,0x3bf0,0x2970,0x1260,0x1275,0x1284,0x128d,0x1293,0x12a2,0x12af,0x12d0,0x12eb,0x12f4,0x12f9,0x1301,0x1321,0x132d,0x1332,0x1339,0x1348,0x1355,0x1364,0x137c,0x1393,0x13a0,0x13b0,0x13ea,0x13f6,0x1402,0x1412,0x1430,0x143a,0x1442,0x144f,0x146f,0x1476,0x1494,0x14c2,0x14d0,0x1522,0x152b,0x1548,0x1576,0x1584,0x158d,0x1594,0x15a8,0x15ac,0x15b1,0x15d4,0x15dc,0x15df,0x15e5,0x15f1,0x15f7,0x15fe,0x1607,0x160e,0x1617,0x161e,0x1624,0x162b,0x162e,0x1636,0x1638,0x3bf0,0x2970,0x16a0,0x16c6,0x16d3,0x16f5,0x16fb,0x1704,0x1720,0x1748,0x1759,0x1770,0x1789,0x179d,0x17aa,0x17ca,0x17d0,0x17d9,0x17e3,0x17e9,0x17f7,0x38e0,0x1810,0x182d,0x1837,0x184b,0x1852,0x38e0,0x1860,0x186d,0x188f,0x1890,0x18cb,0x18d5,0x18e3,0x18ea,0x1914,0x1920,0x1956,0x195b,0x195d,0x1961,0x197e,0x19af,0x19b5,0x19be,0x19d2,0x19df,0x19e3,0x19e5,0x19eb,0x19f7,0x19fd,0x1a04,0x1a0d,0x1a14,0x1a1d,0x1a24,0x1a2a,0x1a31,0x1a34,0x1a39,0x1a3e,0x1a40,0x3c00,0x2970,0x1a70,0x1aaf,0x1afd,0x1b30,0x1b4c,0x1b52,0x1b75,0x1bb0,0x1bd8,0x1bef,0x1c0f,0x1c27,0x1c31,0x1c3f,0x1c49,0x1c54,0x1c63,0x1c68,0x1c6a,0x38e0,0x3c10,0x22e0,0x1c80,0x1ca1,0x1ca8,0x1cb0,0x1ce2,0x1ce7,0x1cf0,0x1cf9,0x1d0a,0x1d0f,0x1d14,0x1d16,0x1d28,0x1d2f,0x1d3d,0x1d47,0x1d4c,0x1d52,0x1d65,0x1d69,0x1d73,0x1d81,0x1d86,0x1da2,0x1da7,0x1dab,0x1dc8,0x1dcd,0x1de9,0x1dee,0x1df3,0x1df8,0x1dfc,0x1e0c,0x1e0e,0x1e1c,0x1e32,0x1e3c,0x1e46,0x1e58,0x1e5f,0x38e0,0x3c20,0x3c2c,0x3c38,0x3c45,0x3c62,0x1c80,0x1810,0x1e80,0x1ec3,0x1ec8,0x1ecd,0x1ecf,0x1ee1,0x1ee8,0x1ef6,0x1f00,0x1f05,0x1f0b,0x1f1e,0x1f22,0x1f2c,0x1f3a,0x1f40,0x1f45,0x1f61,0x1f6c,0x1f71,0x1f75,0x1f7c,0x1f7f,0x1f9c,0x1fa3,0x1fa8,0x1fc4,0x1fc9,0x1fce,0x1fd2,0x1fe2,0x1fe4,0x1ff2,0x2008,0x2012,0x201c,0x202d,0x2034,0x38e0,0x3c70,0x3c7c,0x3c88,0x3c95,0x3cb2,0x1c80,0x1810,0x2050,0x2090,0x20c7,0x20ed,0x20fe,0x2100,0x2102,0x2108,0x2114,0x211c,0x2121,0x212b,0x213f,0x2170,0x218e,0x3cc0,0x21bc,0x1720,0x21d0,0x21f0,0x2218,0x2225,0x2240,0x2280,0x22a0,0x22e0,0x22f6,0x2302,0x2311,0x2320,0x2342,0x2350,0x2362,0x2368,0x2376,0x237c,0x238d,0x2397,0x239e,0x23a7,0x23ac,0x23b7,0x23cd,0x23e0,0x23e9,0x23f0,0x23f5,0x23f8,0x2407,0x2412,0x2417,0x241a,0x241e,0x2430,0x2446,0x2455,0x2469,0x247a,0x247f,0x2486,0x2489,0x248e,0x249b,0x24a6,0x24bb,0x24c2,0x24d0,0x24fa,0x2508,0x251b,0x2525,0x2533,0x253e,0x2541,0x254e,0x2561,0x256c,0x2575,0x257c,0x2581,0x2584,0x258d,0x259c,0x25a2,0x25b0,0x25b6,0x25c7,0x25ce,0x25d1,0x25d8,0x25dd,0x25e2,0x25ed,0x25f5,0x25fd,0x2600,0x2605,0x2611,0x261c,0x2621,0x2624,0x2628,0x2650,0x2659,0x2662,0x266b,0x2674,0x267b,0x2694,0x26b0,0x26e4,0x26e9,0x2708,0x271a,0x271e,0x2727,0x272b,0x2734,0x273d,0x2743,0x2759,0x2761,0x2763,0x2777,0x277c,0x2783,0x2788,0x278b,0x2790,0x279b,0x27a5,0x27b3,0x27b8,0x27bf,0x27c8,0x27cf,0x27d8,0x27df,0x27e5,0x27ec,0x27ef,0x27f4,0x280b,0x2810,0x2813,0x2828,0x282b,0x3cd0,0x3cdd,0x3ce5,0x3cf1,0x3cff,0x3d0d,0x3d1b,0x3d27,0x3d2e,0x3d36,0x3d3b,0x3d42,0x3d4b,0x3d52,0x3d5b,0x3d62,0x3d68,0x3d6f,0x3d72,0x3d77,0x3d8e,0x3d91,0x2840,0x285d,0x2867,0x286c,0x286f,0x2874,0x287a,0x287d,0x2886,0x288c,0x2891,0x2894,0x28b7,0x28bd,0x28cb,0x28d1,0x28e5,0x28ec,0x28f1,0x28f6,0x2901,0x291a,0x2930,0x2933,0x2938,0x2943,0x294e,0x2953,0x2956,0x295a,0x24d0,0x2970,0x2983,0x2991,0x2996,0x299d,0x29a6,0x29ad,0x29b6,0x29bd,0x29c3,0x29ca,0x29cd,0x29d2,0x29e9,0x29ec,0x29f8,0x2a19,0x2a3b,0x2a55,0x2a5b,0x2a69,0x38e0,0x2a84,0x2aa1,0x2aaf,0x2abc,0x2ae0,0x2aea,0x2af6,0x2af8,0x2afc,0x30d4,0x2b04,0x390a,0x2b0c,0x2b17,0x2b23,0x2b29,0x2b2e,0x2b30,0x2b35,0x2b38,0x2b42,0x2b48,0x2b04,0x2b50,0x2b68,0x2b72,0x2b7c,0x2bb5,0x2bd6,0x2be4,0x2bf0,0x2c0f,0x2c14,0x2c21,0x2c2c,0x2c38,0x2c48,0x3982,0x2c64,0x2c81,0x2c8c,0x2c94,0x2ca6,0x2cb0,0x2cb4,0x2cd5,0x2cdf,0x2cfe,0x2d06,0x2d1b,0x2d27,0x2d3d,0x2d4b,0x2d57,0x2d5f,0x2d8c,0x2d93,0x2d98,0x2d9d,0x2daa,0x2db5,0x2dbd,0x2dc4,0x2dc9,0x2dcb,0x3d9e,0x3cff,0x2ddc,0x2c64,0x2df0,0x2dfd,0x2e0c,0x2e11,0x2e1e,0x2e20,0x2e25,0x2e2c,0x2e57,0x2e5b,0x2e64,0x2e6d,0x2e6f,0x2e78,0x2e90,0x2e99,0x2e9d,0x2ead,0x2eb1,0x2ec4,0x2f2b,0x2f36,0x2f44,0x2f4b,0x2f59,0x2f73,0x2f7e,0x2f98,0x2fa1,0x2fa9,0x2fb3,0x2fb9,0x2fbb,0x2fc0,0x2fc4,0x2fca,0x2fce,0x2fd2,0x2fd6,0x2fd8,0x3dbc,0x2fe0,0x2ff3,0x2ff7,0x2ffe,0x3004,0x3015,0x3019,0x3027,0x3030,0x3055,0x305f,0x306e,0x3080,0x3098,0x30a0,0x30d4,0x30eb,0x30f2,0x31a8,0x31c2,0x31d8,0x3211,0x321c,0x325c,0x327c,0x32bc,0x32dc,0x38ec,0x32f0,0x3317,0x3324,0x3334,0x3354,0x3374,0x339c,0x340b,0x3420,0x3428,0x3430,0x3434,0x3444,0x3448,0x3450,0x3458,0x3474,0x3480,0x3488,0x3490,0x34b5,0x34b9,0x34fa,0x3536,0x35ba,0x35c4,0x35d8,0x35ec,0x35f0,0x35fa,0x3609,0x3614,0x361d,0x3627,0x362c,0x363c,0x364b,0x3651,0x365f,0x3667,0x366e,0x3674,0x3693,0x369b,0x36a5,0x36a9,0x36ae,0x36c0,0x36df,0x36e7,0x36f1,0x36f5,0x36fa,0x370c,0x37a7,0x37c6,0x37cd,0x37d4,0x37de,0x37ee,0x37f9,0x37fe,0x380c,0x3817,0x3827,0x3840,0x384b,0x3852,0x386d,0x3874,0x388e,0x38ad,0x38c0,0x38d4,0x38e6,0x38ec,0x38f2,0x38f8,0x38fe,0x3904,0x390a,0x3910,0x3916,0x391c,0x3922,0x3928,0x392e,0x3934,0x393a,0x3940,0x3946,0x394c,0x3952,0x3958,0x395e,0x3964,0x396a,0x3970,0x3976,0x397c,0x3982,0x3988,0x398e,0x3994,0x399a,0x39a0,0x39a6,0x39ac,0x39b0,0x39d0,0x39e8,0x39fb,0x3a13,0x3a1d,0x2ae0,0x3a2c,0x3a7f,0x3a90,0x3aac,0x3ad0,0x3dd4,0x3dda,0x3dee,0x3dfa };
				for (int i = 0; i < sizeof(rva) / sizeof(rva[0]); i++) {
					FluffiBasicBlock b(rva[i], 0);
					blocksToCover.insert(b);
				}

				std::thread debuggerThread(&TestExecutorGDB::debuggerThreadMain, ".\\gdb.exe", gDBThreadCommunication, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT, gdbInitFile, sharedMemIPCInterruptEvent, modulesToCover, blocksToCover, 0xcc, 1);
				debuggerThread.detach();

				bool isDebugged = TestExecutorGDB::waitUntilTargetIsBeingDebugged(gDBThreadCommunication, 5000);
				Assert::IsTrue(isDebugged, L"The target failed to reach the debugged stage");

				//Wait until the target is up and running
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));

				Assert::IsTrue(sharedMemIPC_toTarget.initializeAsClient(), L"initializeAsClient failed");

				SharedMemMessage message;
				Assert::IsTrue(sharedMemIPC_toTarget.waitForNewMessageToClient(&message, 5000, sharedMemIPCInterruptEvent), L"Failed to receive the ready to fuzz message(1)");
				Assert::IsTrue(message.getMessageType() == SHARED_MEM_MESSAGE_READY_TO_FUZZ, L"Failed to receive the ready to fuzz message(2)");

				gDBThreadCommunication->set_coverageState(GDBThreadCommunication::COVERAGE_STATE::SHOULD_RESET_HARD);

				bool isResetted = TestExecutorGDB::waitUntilCoverageState(gDBThreadCommunication, GDBThreadCommunication::COVERAGE_STATE::RESETTED, 5000);
				Assert::IsTrue(isResetted, L"The target failed to reach the resetted stage");

				SharedMemMessage fuzzFilenameMsg{ SHARED_MEM_MESSAGE_FUZZ_FILENAME, testcaseFile.c_str(),(int)testcaseFile.length() };
				Assert::IsTrue(sharedMemIPC_toTarget.sendMessageToServer(&fuzzFilenameMsg), L"Failed to send the fuzzFilenameMsg to the server");

				Assert::IsFalse(sharedMemIPC_toTarget.waitForNewMessageToClient(&message, 30000, sharedMemIPCInterruptEvent), L"Failed to receive the SHARED_MEM_MESSAGE_WAIT_INTERRUPTED message(1)");

				Assert::IsTrue(message.getMessageType() == SHARED_MEM_MESSAGE_WAIT_INTERRUPTED, L"Failed to receive the SHARED_MEM_MESSAGE_WAIT_INTERRUPTED message(2)");

				Assert::IsTrue(gDBThreadCommunication->m_exOutput.m_terminationType == DebugExecutionOutput::EXCEPTION_ACCESSVIOLATION, L"The expected access violation was not observed");

				Assert::IsTrue(gDBThreadCommunication->m_exOutput.m_firstCrash == gDBThreadCommunication->m_exOutput.m_lastCrash, L"m_firstCrash was not m_lastCrash");

				Assert::IsTrue(gDBThreadCommunication->m_exOutput.m_lastCrash == "InstrumentedDebuggerTester.exe/.text+0x000007b1", L"The observed crash was not the expected one");

				gDBThreadCommunication->set_gdbThreadShouldTerminate();

				//Cleanup
				CloseHandle(sharedMemIPCInterruptEvent);
				std::remove(gdbInitFile.c_str());
				std::remove(testcaseFile.c_str());
			}
		}

		TEST_METHOD(TestExecutorGDB_get_exitStatus) {
			//debuggertester status 0
			{
				HANDLE sharedMemIPCInterruptEvent = CreateEvent(NULL, false, false, NULL);
				Assert::IsFalse(sharedMemIPCInterruptEvent == NULL, L"Failed to create the SharedMemIPCInterruptEvent");

				SharedMemIPC sharedMemIPC_toTarget(std::string("FLUFFI_SharedMemForTest").c_str());

				std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication = std::make_shared<GDBThreadCommunication>();

				std::string gdbInitFile = ".\\gdbinitfile";

				std::ofstream initCommandFile(gdbInitFile, std::ofstream::out);
				initCommandFile << "file DebuggerTester.exe" << std::endl;
				initCommandFile << "set args 0" << std::endl;
				initCommandFile.close();

				std::set<Module> modulesToCover;
				modulesToCover.emplace("DebuggerTester.exe/.text", "*", 0);
				std::set<FluffiBasicBlock>blocksToCover;
				blocksToCover.insert(FluffiBasicBlock(0, 0));

				TestExecutorGDB tgdb("", 0, {}, "", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT, nullptr, "", "", 0, 0, {}, 0xcc, 1);
				tgdb.m_hangTimeoutMS = 500000;
				tgdb.m_gDBThreadCommunication = gDBThreadCommunication;
				Assert::IsTrue(tgdb.m_gDBThreadCommunication->get_exitStatus() == -1, L"Exit status was not -1");

				std::thread debuggerThread(&TestExecutorGDB::debuggerThreadMain, ".\\gdb.exe", gDBThreadCommunication, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT, gdbInitFile, sharedMemIPCInterruptEvent, modulesToCover, blocksToCover, 0xcc, 1);
				debuggerThread.detach();

				tgdb.waitForDebuggerToTerminate();

				Assert::IsTrue(tgdb.m_gDBThreadCommunication->get_exitStatus() == 0, L"Exit status was not 0");

				gDBThreadCommunication->set_gdbThreadShouldTerminate();

				//Cleanup
				CloseHandle(sharedMemIPCInterruptEvent);
				std::remove(gdbInitFile.c_str());
			}

			//robocopy status 20
			{
				HANDLE sharedMemIPCInterruptEvent = CreateEvent(NULL, false, false, NULL);
				Assert::IsFalse(sharedMemIPCInterruptEvent == NULL, L"Failed to create the SharedMemIPCInterruptEvent");

				SharedMemIPC sharedMemIPC_toTarget(std::string("FLUFFI_SharedMemForTest").c_str());

				std::shared_ptr<GDBThreadCommunication> gDBThreadCommunication = std::make_shared<GDBThreadCommunication>();

				std::string gdbInitFile = ".\\gdbinitfile";

				std::ofstream initCommandFile(gdbInitFile, std::ofstream::out);
				initCommandFile << "file robocopy.exe" << std::endl;
				initCommandFile << "set args C:\\Windows\\System32\\notepad.exe C:\\Windows\\System32" << std::endl;
				initCommandFile.close();

				std::set<Module> modulesToCover;
				modulesToCover.emplace("robocopy.exe/.text", "*", 0);
				std::set<FluffiBasicBlock>blocksToCover;
				blocksToCover.insert(FluffiBasicBlock(0, 0));

				TestExecutorGDB tgdb("", 0, {}, "", ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT, nullptr, "", "", 0, 0, {}, 0xcc, 1);
				tgdb.m_hangTimeoutMS = 500000;
				tgdb.m_gDBThreadCommunication = gDBThreadCommunication;
				Assert::IsTrue(tgdb.m_gDBThreadCommunication->get_exitStatus() == -1, L"Exit status was not -1");

				std::thread debuggerThread(&TestExecutorGDB::debuggerThreadMain, ".\\gdb.exe", gDBThreadCommunication, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT, gdbInitFile, sharedMemIPCInterruptEvent, modulesToCover, blocksToCover, 0xcc, 1);
				debuggerThread.detach();

				tgdb.waitForDebuggerToTerminate();

				Assert::IsTrue(tgdb.m_gDBThreadCommunication->get_exitStatus() == 0x20, L"Exit status was not 0x20");

				gDBThreadCommunication->set_gdbThreadShouldTerminate();

				//Cleanup
				CloseHandle(sharedMemIPCInterruptEvent);
				std::remove(gdbInitFile.c_str());
			}
		};

		TEST_METHOD(TestExecutorGDB_execute) {
			std::string testcaseDir = "." + Util::pathSeperator + "tcdir_gdb2";
			Util::createFolderAndParentFolders(testcaseDir);
			{
				std::set<Module> modulesToCover;
				GarbageCollectorWorker garbageCollector(0);
				modulesToCover.emplace("InstrumentedDebuggerTester.exe/.text", "*", 0);
				std::set<FluffiBasicBlock> blocksToCover;
				int rva[] = { 0x0010,0x0056,0x0074,0x007a,0x008a,0x00aa,0x00cb,0x010b,0x011f,0x3ae0,0x3aec,0x3af8,0x2970,0x0150,0x09a0,0x0150,0x0180,0x01be,0x01da,0x01ef,0x021e,0x0223,0x0226,0x022f,0x023a,0x0296,0x02a1,0x02b1,0x02b6,0x02bd,0x02c6,0x02cd,0x02d6,0x02dd,0x02e3,0x02ea,0x02ed,0x02f2,0x02fb,0x0333,0x034c,0x0375,0x0390,0x03a3,0x03b6,0x03ef,0x0402,0x0474,0x04b8,0x04e3,0x0509,0x0518,0x0523,0x052e,0x053e,0x0547,0x0554,0x0561,0x056b,0x056e,0x0573,0x0586,0x058b,0x059a,0x05a3,0x05b0,0x05bd,0x05c7,0x05ca,0x05de,0x05e8,0x05f7,0x0600,0x060d,0x061a,0x0624,0x0627,0x062c,0x0641,0x0644,0x0653,0x065c,0x0669,0x0676,0x0680,0x0683,0x0697,0x06a1,0x06b0,0x06b9,0x06c6,0x06d3,0x06dd,0x06e0,0x06e5,0x071c,0x0721,0x0726,0x072f,0x0754,0x079e,0x07b1,0x07bc,0x07f4,0x0803,0x080e,0x0823,0x084c,0x0851,0x0858,0x085f,0x0866,0x086d,0x0874,0x087b,0x0882,0x0889,0x0890,0x0897,0x089e,0x08a5,0x08ac,0x08b3,0x08ba,0x08c1,0x08c8,0x08cf,0x08d6,0x08dd,0x0902,0x0927,0x094d,0x0950,0x095b,0x0966,0x0973,0x3b10,0x3b1e,0x3b2a,0x3b38,0x3b46,0x3b52,0x3b5e,0x3b6a,0x3b78,0x2970,0x09a0,0x09a0,0x09b1,0x09c0,0x09c5,0x09cc,0x09d5,0x09dc,0x09e5,0x09ec,0x09f2,0x09f9,0x09fc,0x0a0e,0x0a20,0x0a42,0x0a4b,0x0a54,0x0a5b,0x0a6e,0x0a79,0x0a9b,0x0ab0,0x0b02,0x0b16,0x0b23,0x0b45,0x0b4b,0x0b54,0x0b5e,0x38e0,0x0b80,0x0bc4,0x0be3,0x0c15,0x0c4e,0x0c55,0x0c58,0x0c68,0x0c7f,0x3b90,0x3ba3,0x3bb8,0x3bbe,0x3bd0,0x3bde,0x16a0,0x0ca0,0x0cc5,0x0cd8,0x0cf0,0x0d03,0x0d11,0x0d22,0x0d2b,0x0d40,0x0d58,0x0d5d,0x0d62,0x0d68,0x0d6b,0x0d75,0x0d91,0x0da0,0x0ddb,0x0de4,0x0dfa,0x0dff,0x0e17,0x0e2d,0x0e57,0x0e72,0x0e90,0x0ec3,0x0ec9,0x0ed0,0x0ed3,0x0ee1,0x0eea,0x0eef,0x0ef4,0x0f0b,0x0f21,0x0f2a,0x0f4c,0x0f66,0x0f81,0x0fa0,0x0fdc,0x0fec,0x1008,0x1012,0x101a,0x1027,0x1047,0x104e,0x1060,0x1068,0x1070,0x1084,0x109a,0x1104,0x1109,0x1112,0x1119,0x111e,0x1130,0x114b,0x1156,0x115e,0x117d,0x1181,0x1187,0x118c,0x11a8,0x11b0,0x11ca,0x11d0,0x11d4,0x11da,0x11e1,0x11eb,0x11f6,0x11fc,0x1203,0x120c,0x1213,0x121c,0x1223,0x1229,0x1230,0x1233,0x123b,0x123d,0x3bf0,0x2970,0x1260,0x1275,0x1284,0x128d,0x1293,0x12a2,0x12af,0x12d0,0x12eb,0x12f4,0x12f9,0x1301,0x1321,0x132d,0x1332,0x1339,0x1348,0x1355,0x1364,0x137c,0x1393,0x13a0,0x13b0,0x13ea,0x13f6,0x1402,0x1412,0x1430,0x143a,0x1442,0x144f,0x146f,0x1476,0x1494,0x14c2,0x14d0,0x1522,0x152b,0x1548,0x1576,0x1584,0x158d,0x1594,0x15a8,0x15ac,0x15b1,0x15d4,0x15dc,0x15df,0x15e5,0x15f1,0x15f7,0x15fe,0x1607,0x160e,0x1617,0x161e,0x1624,0x162b,0x162e,0x1636,0x1638,0x3bf0,0x2970,0x16a0,0x16c6,0x16d3,0x16f5,0x16fb,0x1704,0x1720,0x1748,0x1759,0x1770,0x1789,0x179d,0x17aa,0x17ca,0x17d0,0x17d9,0x17e3,0x17e9,0x17f7,0x38e0,0x1810,0x182d,0x1837,0x184b,0x1852,0x38e0,0x1860,0x186d,0x188f,0x1890,0x18cb,0x18d5,0x18e3,0x18ea,0x1914,0x1920,0x1956,0x195b,0x195d,0x1961,0x197e,0x19af,0x19b5,0x19be,0x19d2,0x19df,0x19e3,0x19e5,0x19eb,0x19f7,0x19fd,0x1a04,0x1a0d,0x1a14,0x1a1d,0x1a24,0x1a2a,0x1a31,0x1a34,0x1a39,0x1a3e,0x1a40,0x3c00,0x2970,0x1a70,0x1aaf,0x1afd,0x1b30,0x1b4c,0x1b52,0x1b75,0x1bb0,0x1bd8,0x1bef,0x1c0f,0x1c27,0x1c31,0x1c3f,0x1c49,0x1c54,0x1c63,0x1c68,0x1c6a,0x38e0,0x3c10,0x22e0,0x1c80,0x1ca1,0x1ca8,0x1cb0,0x1ce2,0x1ce7,0x1cf0,0x1cf9,0x1d0a,0x1d0f,0x1d14,0x1d16,0x1d28,0x1d2f,0x1d3d,0x1d47,0x1d4c,0x1d52,0x1d65,0x1d69,0x1d73,0x1d81,0x1d86,0x1da2,0x1da7,0x1dab,0x1dc8,0x1dcd,0x1de9,0x1dee,0x1df3,0x1df8,0x1dfc,0x1e0c,0x1e0e,0x1e1c,0x1e32,0x1e3c,0x1e46,0x1e58,0x1e5f,0x38e0,0x3c20,0x3c2c,0x3c38,0x3c45,0x3c62,0x1c80,0x1810,0x1e80,0x1ec3,0x1ec8,0x1ecd,0x1ecf,0x1ee1,0x1ee8,0x1ef6,0x1f00,0x1f05,0x1f0b,0x1f1e,0x1f22,0x1f2c,0x1f3a,0x1f40,0x1f45,0x1f61,0x1f6c,0x1f71,0x1f75,0x1f7c,0x1f7f,0x1f9c,0x1fa3,0x1fa8,0x1fc4,0x1fc9,0x1fce,0x1fd2,0x1fe2,0x1fe4,0x1ff2,0x2008,0x2012,0x201c,0x202d,0x2034,0x38e0,0x3c70,0x3c7c,0x3c88,0x3c95,0x3cb2,0x1c80,0x1810,0x2050,0x2090,0x20c7,0x20ed,0x20fe,0x2100,0x2102,0x2108,0x2114,0x211c,0x2121,0x212b,0x213f,0x2170,0x218e,0x3cc0,0x21bc,0x1720,0x21d0,0x21f0,0x2218,0x2225,0x2240,0x2280,0x22a0,0x22e0,0x22f6,0x2302,0x2311,0x2320,0x2342,0x2350,0x2362,0x2368,0x2376,0x237c,0x238d,0x2397,0x239e,0x23a7,0x23ac,0x23b7,0x23cd,0x23e0,0x23e9,0x23f0,0x23f5,0x23f8,0x2407,0x2412,0x2417,0x241a,0x241e,0x2430,0x2446,0x2455,0x2469,0x247a,0x247f,0x2486,0x2489,0x248e,0x249b,0x24a6,0x24bb,0x24c2,0x24d0,0x24fa,0x2508,0x251b,0x2525,0x2533,0x253e,0x2541,0x254e,0x2561,0x256c,0x2575,0x257c,0x2581,0x2584,0x258d,0x259c,0x25a2,0x25b0,0x25b6,0x25c7,0x25ce,0x25d1,0x25d8,0x25dd,0x25e2,0x25ed,0x25f5,0x25fd,0x2600,0x2605,0x2611,0x261c,0x2621,0x2624,0x2628,0x2650,0x2659,0x2662,0x266b,0x2674,0x267b,0x2694,0x26b0,0x26e4,0x26e9,0x2708,0x271a,0x271e,0x2727,0x272b,0x2734,0x273d,0x2743,0x2759,0x2761,0x2763,0x2777,0x277c,0x2783,0x2788,0x278b,0x2790,0x279b,0x27a5,0x27b3,0x27b8,0x27bf,0x27c8,0x27cf,0x27d8,0x27df,0x27e5,0x27ec,0x27ef,0x27f4,0x280b,0x2810,0x2813,0x2828,0x282b,0x3cd0,0x3cdd,0x3ce5,0x3cf1,0x3cff,0x3d0d,0x3d1b,0x3d27,0x3d2e,0x3d36,0x3d3b,0x3d42,0x3d4b,0x3d52,0x3d5b,0x3d62,0x3d68,0x3d6f,0x3d72,0x3d77,0x3d8e,0x3d91,0x2840,0x285d,0x2867,0x286c,0x286f,0x2874,0x287a,0x287d,0x2886,0x288c,0x2891,0x2894,0x28b7,0x28bd,0x28cb,0x28d1,0x28e5,0x28ec,0x28f1,0x28f6,0x2901,0x291a,0x2930,0x2933,0x2938,0x2943,0x294e,0x2953,0x2956,0x295a,0x24d0,0x2970,0x2983,0x2991,0x2996,0x299d,0x29a6,0x29ad,0x29b6,0x29bd,0x29c3,0x29ca,0x29cd,0x29d2,0x29e9,0x29ec,0x29f8,0x2a19,0x2a3b,0x2a55,0x2a5b,0x2a69,0x38e0,0x2a84,0x2aa1,0x2aaf,0x2abc,0x2ae0,0x2aea,0x2af6,0x2af8,0x2afc,0x30d4,0x2b04,0x390a,0x2b0c,0x2b17,0x2b23,0x2b29,0x2b2e,0x2b30,0x2b35,0x2b38,0x2b42,0x2b48,0x2b04,0x2b50,0x2b68,0x2b72,0x2b7c,0x2bb5,0x2bd6,0x2be4,0x2bf0,0x2c0f,0x2c14,0x2c21,0x2c2c,0x2c38,0x2c48,0x3982,0x2c64,0x2c81,0x2c8c,0x2c94,0x2ca6,0x2cb0,0x2cb4,0x2cd5,0x2cdf,0x2cfe,0x2d06,0x2d1b,0x2d27,0x2d3d,0x2d4b,0x2d57,0x2d5f,0x2d8c,0x2d93,0x2d98,0x2d9d,0x2daa,0x2db5,0x2dbd,0x2dc4,0x2dc9,0x2dcb,0x3d9e,0x3cff,0x2ddc,0x2c64,0x2df0,0x2dfd,0x2e0c,0x2e11,0x2e1e,0x2e20,0x2e25,0x2e2c,0x2e57,0x2e5b,0x2e64,0x2e6d,0x2e6f,0x2e78,0x2e90,0x2e99,0x2e9d,0x2ead,0x2eb1,0x2ec4,0x2f2b,0x2f36,0x2f44,0x2f4b,0x2f59,0x2f73,0x2f7e,0x2f98,0x2fa1,0x2fa9,0x2fb3,0x2fb9,0x2fbb,0x2fc0,0x2fc4,0x2fca,0x2fce,0x2fd2,0x2fd6,0x2fd8,0x3dbc,0x2fe0,0x2ff3,0x2ff7,0x2ffe,0x3004,0x3015,0x3019,0x3027,0x3030,0x3055,0x305f,0x306e,0x3080,0x3098,0x30a0,0x30d4,0x30eb,0x30f2,0x31a8,0x31c2,0x31d8,0x3211,0x321c,0x325c,0x327c,0x32bc,0x32dc,0x38ec,0x32f0,0x3317,0x3324,0x3334,0x3354,0x3374,0x339c,0x340b,0x3420,0x3428,0x3430,0x3434,0x3444,0x3448,0x3450,0x3458,0x3474,0x3480,0x3488,0x3490,0x34b5,0x34b9,0x34fa,0x3536,0x35ba,0x35c4,0x35d8,0x35ec,0x35f0,0x35fa,0x3609,0x3614,0x361d,0x3627,0x362c,0x363c,0x364b,0x3651,0x365f,0x3667,0x366e,0x3674,0x3693,0x369b,0x36a5,0x36a9,0x36ae,0x36c0,0x36df,0x36e7,0x36f1,0x36f5,0x36fa,0x370c,0x37a7,0x37c6,0x37cd,0x37d4,0x37de,0x37ee,0x37f9,0x37fe,0x380c,0x3817,0x3827,0x3840,0x384b,0x3852,0x386d,0x3874,0x388e,0x38ad,0x38c0,0x38d4,0x38e6,0x38ec,0x38f2,0x38f8,0x38fe,0x3904,0x390a,0x3910,0x3916,0x391c,0x3922,0x3928,0x392e,0x3934,0x393a,0x3940,0x3946,0x394c,0x3952,0x3958,0x395e,0x3964,0x396a,0x3970,0x3976,0x397c,0x3982,0x3988,0x398e,0x3994,0x399a,0x39a0,0x39a6,0x39ac,0x39b0,0x39d0,0x39e8,0x39fb,0x3a13,0x3a1d,0x2ae0,0x3a2c,0x3a7f,0x3a90,0x3aac,0x3ad0,0x3dd4,0x3dda,0x3dee,0x3dfa };
				for (int i = 0; i < sizeof(rva) / sizeof(rva[0]); i++) {
					FluffiBasicBlock b(rva[i], 0);
					blocksToCover.insert(b);
				}

§§				TestExecutorGDB* te = new TestExecutorGDB(".\\gdb.exe", 15000, modulesToCover, testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE::SUPPRESS, &garbageCollector, ".\\SharedMemFeeder.exe", ".\\GDBStarter.exe --local \".\\InstrumentedDebuggerTester.exe FLUFFI_SharedMemForTest\"", 6000, 0, blocksToCover, 0xcc, 1);
				Assert::IsTrue(te->isSetupFunctionable(), L"The setup that should be valid is somehow not considered to be valid");

				{
					//Do the first clean run (full coverage will be reported)
					uint32_t expectedBlocks[] = { 16,86,116,138,170,203,267,287,885,912,931,950,1007,1026,1140,1601,1604,1667,1687,1697,1760,1765,1820,1825,1830,1839,1980,2036,2592,2626,2670,2681,2715,2736,2818,2838,2885,2891,2900,2910,2944,3012,3043,3093,3150,3157,3160,3199,3728,3795,3809,3818,3823,3828,3851,3873,3916,3969,4000,4104,4122,4167,4174,4192,4667,4669,6288,6718,6720,6768,6831,6909,6960,6994,7029,7088,7128,7151,7183,7207,7231,7241,7267,7274,7344,7399,7408,7417,7444,7446,7464,7471,7485,7506,7525,7539,7553,7591,7595,7624,7676,7692,7708,7730,7740,7750,7768,7775,7808,7885,7887,7905,7912,7926,7947,7966,7980,7994,8000,8049,8053,8060,8063,8099,8146,8162,8178,8200,8210,8220,8237,8244,8272,8336,8391,8448,8450,8476,8481,8491,8560,8590,9424,9480,9628,9648,9654,9671,9678,9681,9688,9728,9733,9745,9756,9764,9768,9904,9961,10014,10027,10073,10081,10103,10139,10228,10256,10259,10280,10283,10304,10333,10348,10351,10356,10365,10423,10443,10449,10469,10476,10547,10552,10563,10574,10582,10586,10608,10627,10701,10706,10732,10884,10913,10927,10940,10976,10986,10998,11012,11020,11064,11074,14596,14602,14614,15020 };

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = te->execute(tid, false);

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(1)");

					bool doBlocksMatch = matchesBlocklist(expectedBlocks, sizeof(expectedBlocks) / sizeof(expectedBlocks[0]), exOutput);
					if (!doBlocksMatch) {
						int debug = 0;
					}
					Assert::IsTrue(doBlocksMatch, L"Blocks didn't match (1)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectNow();
				}

				{
					//Do a second clean run (as partial coverage will be reported - only incremental coverage will be reported). It differs due to compiler optimization that first loop run != other loop runs
					uint32_t expectedBlocks_1[] = { 2062,2083, 2124 };
					uint32_t expectedBlocks_2[] = { 2051, 2062,2083, 2124 };
					uint32_t expectedBlocks_3[] = { 2124 };
					uint32_t expectedBlocks_4[] = { 2083, 2124 };

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = te->execute(tid, false);

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(2)");

					bool doBlocksMatch = matchesBlocklist(expectedBlocks_1, sizeof(expectedBlocks_1) / sizeof(expectedBlocks_1[0]), exOutput) || matchesBlocklist(expectedBlocks_2, sizeof(expectedBlocks_2) / sizeof(expectedBlocks_2[0]), exOutput) || matchesBlocklist(expectedBlocks_3, sizeof(expectedBlocks_3) / sizeof(expectedBlocks_3[0]), exOutput) || matchesBlocklist(expectedBlocks_4, sizeof(expectedBlocks_4) / sizeof(expectedBlocks_4[0]), exOutput);
					if (!doBlocksMatch) {
						int debug = 0;
					}
					Assert::IsTrue(doBlocksMatch, L"Blocks didn't match (2)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}

				{
					//Do a third clean run (as partial coverage will be reported - only incremental coverage will be reported)

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = te->execute(tid, false);

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(3)");

					Assert::IsTrue(exOutput->getCoveredBasicBlocks().size() == 0, L"Blocks didn't match (3)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}

				{
					//Do a access violation run

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)1;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = te->execute(tid, false);

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::EXCEPTION_ACCESSVIOLATION, L"The expected access violation was not observed(4)");

					Assert::IsTrue(exOutput->m_firstCrash == exOutput->m_lastCrash, L"m_firstCrash was not m_lastCrash(4)");

					Assert::IsTrue(exOutput->m_lastCrash == "InstrumentedDebuggerTester.exe/.text+0x000007b1", L"The observed crash was not the expected one(4)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}
				Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened (2)");
				{
					//Do a crash run

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)2;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = te->execute(tid, false);

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::EXCEPTION_OTHER, L"The expected access violation was not observed(5)");

					Assert::IsTrue(exOutput->m_firstCrash == exOutput->m_lastCrash, L"m_firstCrash was not m_lastCrash(5)");

					Assert::IsTrue(exOutput->m_lastCrash == "InstrumentedDebuggerTester.exe/.text+0x000007a4", L"The observed crash was not the expected one(5)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}
				Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened (3)");
				{
					//Do a hang run

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)3;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = te->execute(tid, false);

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::TIMEOUT, L"The expected timeout not observed(6)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectGarbage();
				}
				Assert::IsTrue(te->attemptStartTargetAndFeeder(), L"Failed to start target and feeder, although this should have happened (4)");
				{
					//Do a clean run
					uint32_t expectedBlocks[] = { 16,86,116,138,170,203,267,287,885,912,931,950,1007,1026,1140,1601,1604,1667,1687,1697,1760,1765,1820,1825,1830,1980,2036,2592,2626,2670,2681,2715,2736,2818,2838,2885,2891,2900,2910,2944,3012,3043,3093,3150,3157,3160,3199,3728,3795,3809,3818,3823,3828,3851,3873,3916,3969,4000,4104,4122,4167,4174,4192,4667,4669,6288,6718,6720,6768,6831,6909,6960,6994,7029,7088,7128,7151,7183,7207,7231,7241,7267,7274,7344,7399,7408,7417,7444,7446,7464,7471,7485,7506,7525,7539,7553,7591,7595,7624,7676,7692,7708,7730,7740,7750,7768,7775,7808,7885,7887,7905,7912,7926,7947,7966,7980,7994,8000,8049,8053,8060,8063,8099,8146,8162,8178,8200,8210,8220,8237,8244,8272,8336,8391,8448,8450,8476,8481,8491,8560,8590,9424,9480,9628,9648,9654,9671,9678,9681,9688,9728,9733,9745,9756,9764,9768,9904,9961,10014,10027,10073,10081,10103,10139,10228,10256,10259,10280,10283,10304,10333,10348,10351,10356,10365,10423,10443,10449,10469,10476,10547,10552,10563,10574,10582,10586,10608,10627,10701,10706,10732,10884,10913,10927,10940,10976,10986,10998,11012,11020,11064,11074,14596,14602,14614,15020 };

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)0;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = te->execute(tid, false);

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(7)");

					bool doBlocksMatch = matchesBlocklist(expectedBlocks, sizeof(expectedBlocks) / sizeof(expectedBlocks[0]), exOutput);
					if (!doBlocksMatch) {
						int debug = 0;
					}
					Assert::IsTrue(doBlocksMatch, L"Blocks didn't match (7)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectNow();
				}

				{
					//Do a clean run that differs slightly due to the different whattodo value
					uint32_t expectedBlocks_1[] = { 1839,2062,2083, 2124 };
					uint32_t expectedBlocks_2[] = { 1839, 2051, 2062,2083, 2124 };
					uint32_t expectedBlocks_3[] = { 1839,2083, 2124 };
					uint32_t expectedBlocks_4[] = { 1839, 2124 };

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = te->execute(tid, false);

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(8)");

					bool doBlocksMatch = matchesBlocklist(expectedBlocks_1, sizeof(expectedBlocks_1) / sizeof(expectedBlocks_1[0]), exOutput) || matchesBlocklist(expectedBlocks_2, sizeof(expectedBlocks_2) / sizeof(expectedBlocks_2[0]), exOutput) || matchesBlocklist(expectedBlocks_3, sizeof(expectedBlocks_3) / sizeof(expectedBlocks_3[0]), exOutput) || matchesBlocklist(expectedBlocks_4, sizeof(expectedBlocks_4) / sizeof(expectedBlocks_4[0]), exOutput);
					if (!doBlocksMatch) {
						int debug = 0;
					}
					Assert::IsTrue(doBlocksMatch, L"Blocks didn't match (8)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectNow();
				}

				{
					//Do a forced full run
					uint32_t expectedBlocks[] = { 16,86,116,138,170,203,267,287,912,931,950,1007,1026,1140,1601,1604,1667,1687,1697,1760,1765,1820,1825,1830,1839,1980,2036,2124,2592,2626,2670,2681,2715,2736,2818,2838,2885,2891,2900,2910,2944,3012,3043,3093,3150,3157,3160,3199,3728,3795,3809,3818,3823,3828,3851,3873,3916,3969,4000,4104,4122,4167,4174,4192,4667,4669,6288,6718,6720,6768,6831,6909,6960,6994,7029,7088,7128,7151,7183,7207,7231,7241,7267,7274,7344,7399,7408,7417,7444,7446,7464,7471,7485,7506,7525,7539,7553,7591,7595,7624,7676,7692,7708,7730,7740,7750,7768,7775,7808,7885,7887,7905,7912,7926,7947,7966,7980,7994,8000,8049,8053,8060,8063,8099,8146,8162,8178,8200,8210,8220,8237,8244,8272,8336,8391,8448,8450,8476,8481,8590,9424,9480,9628,9648,9654,9671,9678,9681,9688,9728,9733,9745,9756,9764,9768,9904,9961,10014,10027,10073,10081,10103,10139,10228,10256,10259,10280,10283,10304,10333,10348,10351,10356,10365,10423,10443,10449,10469,10476,10547,10552,10563,10574,10582,10586,10608,10627,10701,10706,10732,10940,10976,10986,10998,11012,11020,11064,11074,14596,14602,14614,15020 };

					FluffiServiceDescriptor sd{ "hap","guid" };
					FluffiTestcaseID tid{ sd,0 };
					std::string testcaseFile = Util::generateTestcasePathAndFilename(tid, testcaseDir);

					std::ofstream fout;
					fout.open(testcaseFile, std::ios::binary | std::ios::out);
					fout << (char)4;
					fout.close();

					std::shared_ptr<DebugExecutionOutput> exOutput = te->execute(tid, true);

					Assert::IsTrue(exOutput->m_terminationType == DebugExecutionOutput::CLEAN, L"The expected clean execution was not observed(1)");

					bool doBlocksMatch = matchesBlocklist(expectedBlocks, sizeof(expectedBlocks) / sizeof(expectedBlocks[0]), exOutput);
					if (!doBlocksMatch) {
						int debug = 0;
					}

					Assert::IsTrue(doBlocksMatch, L"Blocks didn't match (9)");

					garbageCollector.markFileForDelete(testcaseFile);
					garbageCollector.collectNow();
				}

				delete te;
			}

			std::error_code errorcode;
			Assert::IsTrue(std::experimental::filesystem::remove(testcaseDir, errorcode), L"testcase directory could not be deleted!");
		};
	};
}
