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

Author(s): Thomas Riedmaier
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "LMGetStatusRequestHandler.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FLUFFILogTester
{
	TEST_CLASS(FLUFFILogTest)
	{
	public:

		inline bool ends_with(std::string const & value, std::string const & ending)
		{
			if (ending.size() > value.size()) return false;
			return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
		}

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(FLUFFILog_writeAndStoreLogEntry)
		{
			//Reset log messages from previous test cases
			{
				LMGetStatusRequestHandler rh(nullptr);
				GetStatusResponse resp;
				rh.setLogMessages(&resp);
			}

			LOG(ERROR) << "TestError1";
			LOG(WARNING) << "TestWarning";
			LOG(INFO) << "TestInfo";
			LOG(ERROR) << "TestError2";

			LMGetStatusRequestHandler rh(nullptr);

			GetStatusResponse resp;
			rh.setLogMessages(&resp);

			//Check if the logs are stored correctly
			google::protobuf::RepeatedPtrField< std::string > messagesField = resp.logmessages();
			Assert::IsTrue(messagesField.size() == 2, L"An unexpected number of messages was stored by FLUFFILog");
			Assert::IsTrue(ends_with(messagesField.Get(0), "TestError1"), L"The first stored message was not the expected one");
			Assert::IsTrue(ends_with(messagesField.Get(1), "TestError2"), L"The second stored message was not the expected one");

			//Check if normal logging is still operational
			HANDLE hLogFile = CreateFileA(std::string("." + Util::pathSeperator + "logs" + Util::pathSeperator + "Test.log").c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			Assert::IsFalse(hLogFile == NULL, L"Could not open log file");

			std::vector<char> logFileBytes;
			char ReadBuffer[0x1000];
			DWORD bytesRead = 1;
			while (bytesRead != 0) {
				memset(ReadBuffer, 0, 0x1000);
				bytesRead = 0;
				ReadFile(hLogFile, ReadBuffer, 0x1000, &bytesRead, NULL);
				logFileBytes.insert(logFileBytes.end(), ReadBuffer, ReadBuffer + bytesRead);
			}

			CloseHandle(hLogFile);
			std::string logFileString(logFileBytes.begin(), logFileBytes.end());
			std::vector<std::string> logFileStringArray = Util::splitString(logFileString, "\r\n");
			Assert::IsTrue(ends_with(logFileStringArray[logFileStringArray.size() - 1 - 1], "TestError2"), L"Log file did not contain the expected log entry(-1)");
			Assert::IsTrue(ends_with(logFileStringArray[logFileStringArray.size() - 1 - 2], "TestInfo"), L"Log file did not contain the expected log entry(-2)");
			Assert::IsTrue(ends_with(logFileStringArray[logFileStringArray.size() - 1 - 3], "TestWarning"), L"Log file did not contain the expected log entry(-3)");
			Assert::IsTrue(ends_with(logFileStringArray[logFileStringArray.size() - 1 - 4], "TestError1"), L"Log file did not contain the expected log entry(-4)");
		}
	};
}
