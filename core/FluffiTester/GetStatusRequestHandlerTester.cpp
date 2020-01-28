/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "CommInt.h"
#include "LMGetStatusRequestHandler.h"
#include "TRGetStatusRequestHandler.h"
#include "TRWorkerThreadStateBuilder.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GetStatusRequestHandlerTester
{
	TEST_CLASS(GetStatusRequestHandlerTest)
	{
	public:

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(GetStatusRequestHandler_getCurrentSystemCPUUtilization)
		{
			LMGetStatusRequestHandler u = LMGetStatusRequestHandler(nullptr);
			for (int i = 0; i < 100; ++i)
			{
				Assert::IsTrue((u.getCurrentSystemCPUUtilization() >= 0.0) && (u.getCurrentSystemCPUUtilization() <= 100.0), L"Error getting current System CPU utilization: (getCurrentSystemCPUUtilization)");
			}
		}

		TEST_METHOD(GetStatusRequestHandler_getCurrentProcessCPUUtilization)
		{
			LMGetStatusRequestHandler u = LMGetStatusRequestHandler(nullptr);
			Sleep(200);
			double a = u.getCurrentProcessCPUUtilization();
			Sleep(200);
			a = u.getCurrentProcessCPUUtilization();
			Assert::IsTrue(a < 0.1, L"Error getting current Process CPU utilization: (getCurrentProcessCPUUtilization) 1");
			int j = 0;
			for (int i = 0; i < 10000000; i++) {
				j += i;
			}
			std::cout << j << std::endl;
			a = u.getCurrentProcessCPUUtilization();
			Assert::IsTrue(a > 2.0, L"Error: current CPU utilization is unrealisticly low.");

			// The CPUUtilization is very low in the test environment. Mostly 0.
			for (int i = 0; i < 5; i++) {
				LMGetStatusRequestHandler u = LMGetStatusRequestHandler(nullptr);
				double a = u.getCurrentProcessCPUUtilization();
				a = u.getCurrentProcessCPUUtilization();
				Assert::IsTrue((a >= 0.0) && (a <= 100.0), L"Error getting current Process CPU utilization: (getCurrentProcessCPUUtilization) 2");

				for (int i = 0; i < 10; ++i)
				{
					Sleep(10);
					a = u.getCurrentProcessCPUUtilization();
					Assert::IsTrue((a >= 0.0) && (a <= 100.0), std::wstring(L"Error getting current Process CPU utilization: (getCurrentProcessCPUUtilization) 3:" + std::to_wstring(a)).c_str());
				}
			}
		}

		TEST_METHOD(GetStatusRequestHandler_generateGeneralStatus)
		{
			TRWorkerThreadStateBuilder workerStateBuilder;
			CommInt comm(&workerStateBuilder, 1, 1);
			int n = 0;
			TRGetStatusRequestHandler u = TRGetStatusRequestHandler(&comm, &n);

			for (int i = 0; i < 100; i++) {
				std::string s = u.generateGeneralStatus();

				Assert::AreEqual(std::string(typeid(s).name()), std::string("class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >"), L"Error generating a new GeneralStatus, Is not a std::string: (generateGeneralStatus)");
				Assert::IsTrue(s.find("SysCPUUtil") != std::string::npos, L"Error checking the existence of SysCPUUtil: (generateGeneralStatus)");
				Assert::IsTrue(s.find("ProcCPUUtil") != std::string::npos, L"Error checking the existence of ProcCPUUtil: (generateGeneralStatus)");
				Assert::IsTrue(s.find("AverageRTT") != std::string::npos, L"Error checking the existence of AverageRTT: (generateGeneralStatus)");
			}
		}
	};
}
