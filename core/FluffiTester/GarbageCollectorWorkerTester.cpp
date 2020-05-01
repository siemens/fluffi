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
#include "GarbageCollectorWorker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GarbageCollectorWorkerTester
{
	TEST_CLASS(GarbageCollectorWorkerTest)
	{
	public:

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(GarbageCollectorWorker_getRandomFileToDelete)
		{
			GarbageCollectorWorker garbageCollectorWorker(200);
			garbageCollectorWorker.markFileForDelete("A");
			garbageCollectorWorker.markFileForDelete("B");
			garbageCollectorWorker.markFileForDelete("C");

			bool gota = false;
			bool gotb = false;
			bool gotc = false;
			for (int i = 0; i < 3; i++) {
				std::string toDelete = garbageCollectorWorker.getRandomFileToDelete();
				if (toDelete == "A") {
					gota = true;
				}
				else if (toDelete == "B") {
					gotb = true;
				}
				else if (toDelete == "C") {
					gotc = true;
				}
			}

			Assert::IsTrue("" == garbageCollectorWorker.getRandomFileToDelete(), L"getRandomFileToDelete returned 4 entries although only 4 were pushed!");

			Assert::IsTrue(gota && gotb && gotc, L"Did not observe all expected files while querying them randomly");
		}

		TEST_METHOD(GarbageCollectorWorker_markFileForDelete)
		{
			std::string testcaseDir = ".\\gctemp1";
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"testcaseDir could not be deleted. Most likely its not empty(1)!");
			Util::createFolderAndParentFolders(testcaseDir);

			//Create testfile
			std::ofstream testFile;
			testFile.open(testcaseDir + "\\testfile");
			testFile << "ABC";
			testFile.close();

			Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testfile"), L"testFile could not be created!");

			{
				GarbageCollectorWorker garbageCollectorWorker(200);
				garbageCollectorWorker.markFileForDelete(testcaseDir + "\\testfile");
			}

			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testfile"), L"testFile was not deleted!");

			Util::attemptDeletePathRecursive(testcaseDir);
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"testcaseDir could not be deleted. Most likely its not empty(2)!");
		}

		TEST_METHOD(GarbageCollectorWorker_collectGarbage)
		{
			std::string testcaseDir = ".\\gctemp2";
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"testcaseDir exists from last bogous execution!");
			Util::createFolderAndParentFolders(testcaseDir);

			//Create testfile1
			std::ofstream testFile1;
			testFile1.open(testcaseDir + "\\testfile1");
			testFile1 << "ABC";
			testFile1.close();

			//Create testfile2
			std::ofstream testFile2;
			testFile2.open(testcaseDir + "\\testfile2");
			testFile2 << "ABC";
			testFile2.close();

			Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testfile1"), L"testFile1 could not be created!");
			Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testfile2"), L"testFile2 could not be created!");

			{
				GarbageCollectorWorker garbageCollectorWorker(200);
				garbageCollectorWorker.markFileForDelete(testcaseDir + "\\testfile1");
				garbageCollectorWorker.markFileForDelete(testcaseDir + "\\testfile2");
				garbageCollectorWorker.markFileForDelete("C:\\Windows\\notepad.exe"); //This file cannot be deleted and should therefore remain in the set

				garbageCollectorWorker.collectGarbage();

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testfile1"), L"testFile1 was not deleted!");
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testfile2"), L"testFile1 was not deleted!");

				Assert::IsTrue(garbageCollectorWorker.getRandomFileToDelete() == "C:\\Windows\\notepad.exe", L"Apparently, the Garbage collector managed to delete notpad.exe. This is not good!");
			}

			Util::attemptDeletePathRecursive(testcaseDir);
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"testcaseDir could not be deleted. Most likely its not empty!");
		}

		TEST_METHOD(GarbageCollectorWorker_collectNow)
		{
			std::string testcaseDir = ".\\gctemp3";
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"testcaseDir exists from last bogous execution!");
			Util::createFolderAndParentFolders(testcaseDir);

			//Create testfile1
			std::ofstream testFile1;
			testFile1.open(testcaseDir + "\\testfile1");
			testFile1 << "ABC";
			testFile1.close();

			//Create testfile2
			std::ofstream testFile2;
			testFile2.open(testcaseDir + "\\testfile2");
			testFile2 << "ABC";
			testFile2.close();

			Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testfile1"), L"testFile1 could not be created!");
			Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testfile2"), L"testFile2 could not be created!");

			bool check1, check2 = false;
			{
				GarbageCollectorWorker garbageCollectorWorker(10 * 1000);
				garbageCollectorWorker.markFileForDelete(testcaseDir + "\\testfile1");
				garbageCollectorWorker.markFileForDelete(testcaseDir + "\\testfile2");
				garbageCollectorWorker.m_thread = new std::thread(&GarbageCollectorWorker::workerMain, &garbageCollectorWorker);

				garbageCollectorWorker.collectNow();

				std::this_thread::sleep_for(std::chrono::milliseconds(2000));

				check1 = std::experimental::filesystem::exists(testcaseDir + "\\testfile1");
				check2 = std::experimental::filesystem::exists(testcaseDir + "\\testfile2");

				garbageCollectorWorker.stop();
				garbageCollectorWorker.m_thread->join();
			}
			Assert::IsFalse(check1, L"testFile1 was not deleted!");
			Assert::IsFalse(check2, L"testFile1 was not deleted!");

			Util::attemptDeletePathRecursive(testcaseDir);
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"testcaseDir could not be deleted. Most likely its not empty!");
		}

		TEST_METHOD(GarbageCollectorWorker_isFileMarkedForDeletion)
		{
			GarbageCollectorWorker garbageCollectorWorker(200);
			garbageCollectorWorker.markFileForDelete("A");

			Assert::IsFalse(garbageCollectorWorker.isFileMarkedForDeletion("B"), L"File that is not in the set is somehow marked for deletion");

			Assert::IsTrue(garbageCollectorWorker.isFileMarkedForDeletion("A"), L"File that is in the set is somehow NOT marked for deletion(1)");

			Assert::IsTrue(garbageCollectorWorker.isFileMarkedForDeletion("A"), L"File that is in the set is somehow NOT marked for deletion(2)");
		}
	};
}
