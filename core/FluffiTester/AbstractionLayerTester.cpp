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

Author(s): Thomas Riedmaier, Pascal Eckmann
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "FluffiBasicBlock.h"
#include "FluffiModuleNameToID.h"
#include "FluffiServiceDescriptor.h"
#include "FluffiLMConfiguration.h"
#include "FluffiServiceAndWeight.h"
#include "FluffiTestcaseID.h"
#include "FluffiTestResult.h"
#include "FluffiSetting.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AbstractionLayerTester
{
	TEST_CLASS(AbstractionLayerTest)
	{
	public:

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(AbstractionLayer_FluffiBasicBlock)
		{
			uint64_t rva = 123;
			int32_t moduleID = 456;

			FluffiBasicBlock fbb{ rva,moduleID };
			BasicBlock bb = fbb.getProtobuf();
			FluffiBasicBlock fbb2{ bb };

			Assert::AreEqual(moduleID, fbb2.m_moduleID);
			Assert::AreEqual(fbb.m_moduleID, fbb2.m_moduleID);

			Assert::AreEqual(rva, fbb2.m_rva);
			Assert::AreEqual(fbb.m_rva, fbb2.m_rva);
		}

		TEST_METHOD(AbstractionLayer_FluffiModuleNameToID)
		{
			std::string moduleName = "a";
			std::string modulePath = "b";
			uint32_t moduleID = 123;

			FluffiModuleNameToID fm2i{ moduleName ,modulePath , moduleID };
			ModuleNameToID m2i = fm2i.getProtobuf();
			FluffiModuleNameToID fm2i2{ m2i };

			Assert::AreEqual(moduleName, fm2i.m_moduleName);
			Assert::AreEqual(fm2i.m_moduleName, fm2i2.m_moduleName);

			Assert::AreEqual(modulePath, fm2i.m_modulePath);
			Assert::AreEqual(fm2i.m_modulePath, fm2i2.m_modulePath);

			Assert::AreEqual(moduleID, fm2i.m_moduleID);
			Assert::AreEqual(fm2i.m_moduleID, fm2i2.m_moduleID);
		}

		TEST_METHOD(AbstractionLayer_FluffiServiceAndWeight)
		{
			std::string hap = "a";
			std::string guid = "b";

			FluffiServiceDescriptor serviceDescriptor{ hap ,guid };
			uint32_t weight = 2;

			FluffiServiceAndWeight fsaw{ serviceDescriptor ,weight };
			ServiceAndWeigth saw = fsaw.getProtobuf();
			FluffiServiceAndWeight fsaw2{ saw };

			Assert::AreEqual(hap, fsaw.m_serviceDescriptor.m_serviceHostAndPort);
			Assert::AreEqual(fsaw.m_serviceDescriptor.m_serviceHostAndPort, fsaw2.m_serviceDescriptor.m_serviceHostAndPort);

			Assert::AreEqual(guid, fsaw.m_serviceDescriptor.m_guid);
			Assert::AreEqual(fsaw.m_serviceDescriptor.m_guid, fsaw2.m_serviceDescriptor.m_guid);

			Assert::AreEqual(weight, fsaw.m_weight);
			Assert::AreEqual(fsaw.m_weight, fsaw2.m_weight);
		}

		TEST_METHOD(AbstractionLayer_FluffiServiceDescriptor)
		{
			std::string hap = "a";
			std::string guid = "b";

			FluffiServiceDescriptor fsd{ hap ,guid };
			ServiceDescriptor sd = fsd.getProtobuf();
			FluffiServiceDescriptor fsd2{ sd };

			Assert::AreEqual(hap, fsd.m_serviceHostAndPort);
			Assert::AreEqual(fsd.m_serviceHostAndPort, fsd2.m_serviceHostAndPort);

			Assert::AreEqual(guid, fsd.m_guid);
			Assert::AreEqual(fsd.m_guid, fsd2.m_guid);
		}

		TEST_METHOD(AbstractionLayer_FluffiTestcaseID)
		{
			std::string hap = "a";
			std::string guid = "b";

			FluffiServiceDescriptor serviceDescriptor{ hap ,guid };
			uint64_t localID = 1;

			FluffiTestcaseID fti{ serviceDescriptor ,1 };
			TestcaseID ti = fti.getProtobuf();
			FluffiTestcaseID fti2{ ti };

			Assert::AreEqual(hap, fti.m_serviceDescriptor.m_serviceHostAndPort);
			Assert::AreEqual(fti.m_serviceDescriptor.m_serviceHostAndPort, fti2.m_serviceDescriptor.m_serviceHostAndPort);

			Assert::AreEqual(guid, fti.m_serviceDescriptor.m_guid);
			Assert::AreEqual(fti.m_serviceDescriptor.m_guid, fti2.m_serviceDescriptor.m_guid);

			Assert::AreEqual(localID, fti.m_localID);
			Assert::AreEqual(fti.m_localID, fti2.m_localID);
		}

		TEST_METHOD(AbstractionLayer_FluffiTestResult)
		{
			uint64_t rva = 123;
			int32_t moduleID = 456;

			FluffiBasicBlock fbb{ rva,moduleID };

			ExitType exitType = ExitType::CleanExit;
			std::vector<FluffiBasicBlock> blocks;
			blocks.push_back(fbb);
			std::string crashFootprint = "myFootprint";

			FluffiTestResult ftr{ exitType,blocks,crashFootprint,false };
			TestResult tr = ftr.getProtobuf();
			FluffiTestResult ftr2{ tr };

			Assert::AreEqual((int)exitType, (int)ftr.m_exitType);
			Assert::AreEqual((int)ftr.m_exitType, (int)ftr2.m_exitType);

			Assert::AreEqual(false, ftr.m_hasFullCoverage);
			Assert::AreEqual(ftr.m_hasFullCoverage, ftr2.m_hasFullCoverage);

			Assert::AreEqual(crashFootprint, ftr.m_crashFootprint);
			Assert::AreEqual(ftr.m_crashFootprint, ftr2.m_crashFootprint);

			Assert::AreEqual(moduleID, ftr.m_blocks[0].m_moduleID);
			Assert::AreEqual(ftr.m_blocks[0].m_moduleID, ftr2.m_blocks[0].m_moduleID);

			Assert::AreEqual(rva, ftr.m_blocks[0].m_rva);
			Assert::AreEqual(ftr.m_blocks[0].m_rva, ftr2.m_blocks[0].m_rva);

			TestResult tr3;
			ftr.setProtobuf(&tr3);
			FluffiTestResult ftr4{ tr3 };

			Assert::AreEqual((int)ftr.m_exitType, (int)ftr4.m_exitType);

			Assert::AreEqual(ftr.m_hasFullCoverage, ftr4.m_hasFullCoverage);

			Assert::AreEqual(ftr.m_crashFootprint, ftr4.m_crashFootprint);

			Assert::AreEqual(ftr.m_blocks[0].m_moduleID, ftr4.m_blocks[0].m_moduleID);

			Assert::AreEqual(ftr.m_blocks[0].m_rva, ftr4.m_blocks[0].m_rva);
		}

		TEST_METHOD(AbstractionLayer_FluffiLMConfiguration)
		{
			std::string dbHost = "dbHost";
			std::string dbUser = "dbUser";
			std::string dbPassword = "dbPassword";
			std::string dbName = "dbName";
			std::string fuzzJobName = "jobName";

			FluffiLMConfiguration flc{ dbHost, dbUser,dbPassword, dbName,fuzzJobName };
			LMConfiguration lc = flc.getProtobuf();
			FluffiLMConfiguration flc2{ lc };

			Assert::AreEqual(dbHost, flc2.m_dbHost);
			Assert::AreEqual(flc.m_dbHost, flc2.m_dbHost);

			Assert::AreEqual(dbUser, flc2.m_dbUser);
			Assert::AreEqual(flc.m_dbUser, flc2.m_dbUser);

			Assert::AreEqual(dbPassword, flc2.m_dbPassword);
			Assert::AreEqual(flc.m_dbPassword, flc2.m_dbPassword);

			Assert::AreEqual(dbName, flc2.m_dbName);
			Assert::AreEqual(flc.m_dbName, flc2.m_dbName);

			Assert::AreEqual(fuzzJobName, flc2.m_fuzzJobName);
			Assert::AreEqual(flc.m_fuzzJobName, flc2.m_fuzzJobName);
		}

		TEST_METHOD(AbstractionLayer_FluffiSetting)
		{
			std::string name = "settingName";
			std::string value = "settingValue";

			FluffiSetting fs{ name,value };
			Setting s = fs.getProtobuf();
			FluffiSetting fs2{ s };

			Assert::AreEqual(name, fs2.m_settingName);
			Assert::AreEqual(fs.m_settingName, fs2.m_settingName);

			Assert::AreEqual(value, fs2.m_settingValue);
			Assert::AreEqual(fs.m_settingValue, fs2.m_settingValue);
		}
	};
}
