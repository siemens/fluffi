/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Michael Kraus, Thomas Riedmaier, Abian Blome, Pascal Eckmann
*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "Util.h"
#include "CommInt.h"
#include "FluffiServiceDescriptor.h"
#include "FluffiTestcaseID.h"
#include "FluffiBasicBlock.h"
#include "GarbageCollectorWorker.h"
#include "TRWorkerThreadStateBuilder.h"
#include "WorkerThreadState.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilTester
{
	TEST_CLASS(UtilTest)
	{
	public:

		TEST_METHOD_INITIALIZE(ModuleInitialize)
		{
			Util::setDefaultLogOptions("logs" + Util::pathSeperator + "Test.log");
		}

		TEST_METHOD(Util_newGUID)
		{
			std::set<std::string> uniqueGUIDSet = std::set<std::string>();
			Assert::AreEqual(uniqueGUIDSet.size(), (size_t)0, L"Error counting unique GUIDs, Problem initialising counting set: (newGUID)");
			uniqueGUIDSet.insert("");
			uniqueGUIDSet.insert("AAAAAAAAAABBBBBBBBBBCCCCCCCCCC");
			Assert::AreEqual(uniqueGUIDSet.size(), (size_t)2, L"Error counting unique GUIDs, Problem inserting item into counting set: (newGUID)");

			for (int i = 0; i < 500; i++) {
				std::string guid = Util::newGUID();
				Assert::AreEqual(std::string(typeid(guid).name()), std::string("class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >"), L"Error generating a new GUID, Is not a std::string: (newGUID)");
				Assert::AreEqual(guid.length(), (size_t)36, L"Error generating a new GUID, Not the correct length: (newGUID)");
				uniqueGUIDSet.insert(guid);
			}
			uniqueGUIDSet.insert("AAAAAAAAAABBBBBBBBBBCCCCCCCCCC");
			Assert::AreEqual(uniqueGUIDSet.size(), (size_t)502, L"Error checking correct number of GUIDs, perhaps not all unique: (newGUID)");
		}

		TEST_METHOD(Util_testsplitString)
		{
			std::vector<std::string> v = Util::splitString("", "asdf");
			std::vector<std::string> emptyStringVector{ "" };
			std::vector<std::string> asdfStringVector{ "asdf" };

			bool is_equal = false;
			if (v.size() == emptyStringVector.size())
				is_equal = std::equal(v.begin(), v.end(), emptyStringVector.begin());

			Assert::IsTrue(is_equal, L"splitString(\"\", \"asdf\") does not return an empty String Vector");

			is_equal = false;
			v = Util::splitString("asdf", "");
			if (v.size() == asdfStringVector.size())
				is_equal = std::equal(v.begin(), v.end(), asdfStringVector.begin());
			Assert::IsTrue(is_equal, L"splitString(\"asdf\", \"\") does not return an asdf String Vector");

			v = Util::splitString("asdf1 asdf2 asdf3 qwer", " ");
			Assert::AreEqual(v.size(), (size_t)4, L"Correct separation of spaces");
			Assert::IsTrue(v[0] == "asdf1");
			Assert::IsTrue(v[1] == "asdf2");
			Assert::IsTrue(v[2] == "asdf3");
			Assert::IsTrue(v[3] == "qwer");
		}

		TEST_METHOD(Util_attemptDeletePathRecursive)
		{
			for (int i = 0; i < 10; i++) {
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");

				Util::createFolderAndParentFolders(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\");

				std::ofstream testFile;
				testFile.open(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\test.txt");
				testFile << "Writing this to a file.\n";
				testFile.close();

				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\test.txt"), L"Error checking the existence of file in file system: (attemptDeletePathRecursive)");

				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");

				Util::attemptDeletePathRecursive(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\");

				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");

				Util::attemptDeletePathRecursive(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\");

				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");

				Util::attemptDeletePathRecursive(".\\550e8400-11111\\550e8400-22222\\");

				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-11111\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");

				Util::attemptDeletePathRecursive(".\\550e8400-11111\\");

				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\550e8400-44444\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\550e8400-33333\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\550e8400-22222\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111\\"), L"Error checking the existence of folder in file system: (attemptDeletePathRecursive)");
			}
		}

		TEST_METHOD(Util_loadTestcaseChunkInMemory)
		{
			// File exists, first chunk, also last chunk
			for (int i = 0; i < 3; i++) {
				std::string testcaseDir = ".\\1244550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");
				std::ofstream testFile;
				testFile.open(testcaseDir + "\\testGuiID1-444");
				testFile << "sdfsdfsdfsdfsdfsdfsdfsdfsdfAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCC";
				testFile.close();
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");

				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				std::string pathAndFileName = Util::generateTestcasePathAndFilename(id1, testcaseDir);
				Assert::AreEqual(pathAndFileName, std::string(testcaseDir + "\\testGuiID1-444"), L"Error generating path and filename for testcase : (loadTestcaseChunkInMemory)");

				bool isLastChunk;
				std::string data = Util::loadTestcaseChunkInMemory(id1, testcaseDir, 0, &isLastChunk);
				Assert::AreEqual(data, std::string("sdfsdfsdfsdfsdfsdfsdfsdfsdfAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCC"), L"Error generating path and filename for testcase : (loadTestcaseChunkInMemory)");
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of file in file system: (loadTestcaseChunkInMemory)");
				Assert::AreEqual(isLastChunk, true, L"Error checking if last Chunk reached : (loadTestcaseChunkInMemory)");
				{
					GarbageCollectorWorker garbageCollector(0);
					garbageCollector.markFileForDelete(testcaseDir + "\\testGuiID1-444");
				}

				Util::attemptDeletePathRecursive(".\\1244550e8400-114567123456789999");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\1244550e8400-114567123456789999"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}

			// File not exists
			for (int i = 0; i < 3; i++) {
				std::string testcaseDir = ".\\1244550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");

				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				std::string pathAndFileName = Util::generateTestcasePathAndFilename(id1, testcaseDir);
				Assert::AreEqual(pathAndFileName, std::string(testcaseDir + "\\testGuiID1-444"), L"Error generating path and filename for testcase : (loadTestcaseChunkInMemory)");

				bool isLastChunk;
				std::string data = Util::loadTestcaseChunkInMemory(id1, testcaseDir, 0, &isLastChunk);
				Assert::AreEqual(data, std::string(""), L"Error generating path and filename for testcase : (loadTestcaseChunkInMemory)");
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of file in file system: (loadTestcaseChunkInMemory)");

				Util::attemptDeletePathRecursive(".\\1244550e8400-114567123456789999");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\1244550e8400-114567123456789999"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}

			// File exists, chunknum to high
			for (int i = 0; i < 3; i++) {
				GarbageCollectorWorker garbageCollector(0);
				std::string testcaseDir = ".\\41244550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");
				std::ofstream testFile;
				testFile.open(testcaseDir + "\\testGuiID1-444");
				testFile << "sdfsdfsdfsdfsdfsdfsdfsdfsdfAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCC";
				testFile.close();
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");

				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				std::string pathAndFileName = Util::generateTestcasePathAndFilename(id1, testcaseDir);
				Assert::AreEqual(pathAndFileName, std::string(testcaseDir + "\\testGuiID1-444"), L"Error generating path and filename for testcase : (loadTestcaseChunkInMemory)");

				bool isLastChunk;
				std::string data = Util::loadTestcaseChunkInMemory(id1, testcaseDir, 1, &isLastChunk);
				Assert::AreEqual(data, std::string(""), L"Error generating path and filename for testcase : (loadTestcaseChunkInMemory)");
				{
					GarbageCollectorWorker garbageCollector(0);
					garbageCollector.markFileForDelete(testcaseDir + "\\testGuiID1-444");
				}

				Util::attemptDeletePathRecursive(".\\41244550e8400-114567123456789999");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\41244550e8400-114567123456789999"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}

			// File exists, first chunk, but not last chunk
			for (int i = 0; i < 2; i++) {
				std::string testcaseDir = ".\\241244550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir),
					L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");
				std::ofstream testFile;
				testFile.open(testcaseDir + "\\testGuiID1-444");

				std::string stuffA(100 * 1000 * 1000, 'A');
				std::string stuffB(100 * 1000 * 1000, 'B');

				testFile << stuffA;
				testFile << stuffB;

				testFile.close();
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseChunkInMemory)");

				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				std::string pathAndFileName = Util::generateTestcasePathAndFilename(id1, testcaseDir);
				Assert::AreEqual(pathAndFileName, std::string(testcaseDir + "\\testGuiID1-444"), L"Error generating path and filename for testcase : (loadTestcaseChunkInMemory)");

				bool isLastChunk;
				std::string data = Util::loadTestcaseChunkInMemory(id1, testcaseDir, 0, &isLastChunk);
				Assert::AreEqual(data, std::string(100000000, 'A'), L"Error generating path and filename for testcase : (loadTestcaseChunkInMemory)");
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of file in file system: (loadTestcaseChunkInMemory)");
				Assert::AreEqual(isLastChunk, false, L"Error checking if last Chunk reached : (loadTestcaseChunkInMemory)");

				data = Util::loadTestcaseChunkInMemory(id1, testcaseDir, 1, &isLastChunk);
				{
					GarbageCollectorWorker garbageCollector(0);
					garbageCollector.markFileForDelete(testcaseDir + "\\testGuiID1-444");
				}
				Assert::AreEqual(data, std::string(100000000, 'B'), L"Error generating path and filename for testcase : (loadTestcaseChunkInMemory)");
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of file in file system: (loadTestcaseChunkInMemory)");

				Util::attemptDeletePathRecursive(".\\241244550e8400-114567123456789999");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\241244550e8400-114567123456789999"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}
		}

		TEST_METHOD(Util_loadTestcaseInMemory)
		{
			// File exists
			for (int i = 0; i < 3; i++) {
				std::string testcaseDir = ".\\44550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseInMemory)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseInMemory)");

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseInMemory)");
				std::ofstream testFile;
				testFile.open(testcaseDir + "\\testGuiID1-444");
				testFile << "sdfsdfsdfsdfsdfsdfsdfsdfsdfAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCC";
				testFile.close();
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseInMemory)");

				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				std::string pathAndFileName = Util::generateTestcasePathAndFilename(id1, testcaseDir);
				Assert::AreEqual(pathAndFileName, std::string(testcaseDir + "\\testGuiID1-444"), L"Error generating path and filename for testcase : (loadTestcaseInMemory)");

				std::string data = Util::loadTestcaseInMemory(id1, testcaseDir);
				Assert::AreEqual(data, std::string("sdfsdfsdfsdfsdfsdfsdfsdfsdfAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCC"), L"Error generating path and filename for testcase : (loadTestcaseInMemory)");
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of file in file system: (loadTestcaseInMemory)");
				{
					GarbageCollectorWorker garbageCollector(0);
					garbageCollector.markFileForDelete(testcaseDir + "\\testGuiID1-444");
				}

				Util::attemptDeletePathRecursive(".\\44550e8400-114567123456789999");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\44550e8400-114567123456789999"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}

			// File not exists
			for (int i = 0; i < 3; i++) {
				std::string testcaseDir = ".\\44550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseInMemory)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (loadTestcaseInMemory)");

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseInMemory)");

				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "testGuiID1");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				std::string pathAndFileName = Util::generateTestcasePathAndFilename(id1, testcaseDir);
				Assert::AreEqual(pathAndFileName, std::string(testcaseDir + "\\testGuiID1-444"), L"Error generating path and filename for testcase : (loadTestcaseInMemory)");

				std::string data = Util::loadTestcaseInMemory(id1, testcaseDir);
				Assert::AreEqual(data, std::string(""), L"Error generating path and filename for testcase : (loadTestcaseInMemory)");
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testGuiID1-444"), L"Error checking the existence of folder in file system: (loadTestcaseInMemory)");

				Util::attemptDeletePathRecursive(".\\44550e8400-114567123456789999");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\44550e8400-114567123456789999"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}
		}

		TEST_METHOD(Util_storeTestcaseFileOnDisk)
		{
			GarbageCollectorWorker garbageCollector(0);
			TRWorkerThreadStateBuilder workerStateBuilder;
			WorkerThreadState* state = workerStateBuilder.constructState();

			// file not exists, only 1 chunk
			{
				std::string testcaseDir = ".\\67241244550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testcase"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");

				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "test");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				std::string data = "AAAAAAAAAABBBBBBBBBBCCCCCCCCCC";
				std::string HAP = "127.0.0.1:78911";

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\test-444"), L"Error checking the existence of file in file system: (storeTestcaseFileOnDisk)");
				Assert::AreEqual(Util::storeTestcaseFileOnDisk(id1, testcaseDir, &data, true, HAP, nullptr, state, &garbageCollector), true, L"Error store first chunk in file system: (storeTestcaseFileOnDisk)");
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\test-444"), L"Error checking the existence of file in file system: (storeTestcaseFileOnDisk)");

				std::vector<char> data2 = Util::readAllBytesFromFile(".\\67241244550e8400-114567123456789999\\550e8400-1113465467989999\\test-444");
				Assert::AreEqual(data2.size(), (size_t)30, L"Error reading correct Bytes from File: (storeTestcaseFileOnDisk)");
				std::string testString;
				testString.assign(data2.data(), data2.size());
				Assert::AreEqual(testString, std::string("AAAAAAAAAABBBBBBBBBBCCCCCCCCCC"), L"Error reading correct Bytes from File: (storeTestcaseFileOnDisk)");

				Util::attemptDeletePathRecursive(".\\67241244550e8400-114567123456789999");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\67241244550e8400-114567123456789999"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}

			//file exists, 2 chunks cannot be tested offline

			// file exists, only 1 chunk, not marked for deletion
			{
				std::string testcaseDir = ".\\67241244550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testcase"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");

				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "test");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				std::string data = "AAAAAAAAAABBBBBBBBBBCCCCCCCCCC";
				std::string HAP = "127.0.0.1:78911";

				std::ofstream testFile;
				testFile.open(".\\67241244550e8400-114567123456789999\\550e8400-1113465467989999\\test-444");
				testFile << "EEEEEEEEEE";
				testFile.close();

				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\test-444"), L"Error checking the existence of file in file system: (storeTestcaseFileOnDisk)");
				Assert::AreEqual(Util::storeTestcaseFileOnDisk(id1, testcaseDir, &data, true, HAP, nullptr, state, &garbageCollector), false, L"Succeded storing file on disk although the file already existed and was not marked for deletion");
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\test-444"), L"Error checking the existence of file in file system: (storeTestcaseFileOnDisk)");

				Util::attemptDeletePathRecursive(".\\67241244550e8400-114567123456789999");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\67241244550e8400-114567123456789999"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}

			// file exists, only 1 chunk, marked for deletion
			{
				std::string testcaseDir = ".\\67241244550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testcase"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");

				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "test");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				std::string data = "AAAAAAAAAABBBBBBBBBBCCCCCCCCCC";
				std::string HAP = "127.0.0.1:78911";

				std::ofstream testFile;
				testFile.open(".\\67241244550e8400-114567123456789999\\550e8400-1113465467989999\\test-444");
				testFile << "EEEEEEEEEE";
				testFile.close();

				garbageCollector.markFileForDelete(".\\67241244550e8400-114567123456789999\\550e8400-1113465467989999\\test-444");

				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\test-444"), L"Error checking the existence of file in file system: (storeTestcaseFileOnDisk)");
				Assert::AreEqual(Util::storeTestcaseFileOnDisk(id1, testcaseDir, &data, true, HAP, nullptr, state, &garbageCollector), true, L"Failed storing file on disk although the file already existing file was marked for deletion");
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\test-444"), L"Error checking the existence of file in file system: (storeTestcaseFileOnDisk)");

				std::vector<char> data2 = Util::readAllBytesFromFile(".\\67241244550e8400-114567123456789999\\550e8400-1113465467989999\\test-444");
				Assert::AreEqual(data2.size(), (size_t)30, L"Error reading correct Bytes from File: (storeTestcaseFileOnDisk)");
				std::string testString;
				testString.assign(data2.data(), data2.size());
				Assert::AreEqual(testString, std::string("AAAAAAAAAABBBBBBBBBBCCCCCCCCCC"), L"Error reading correct Bytes from File: (storeTestcaseFileOnDisk)");

				Util::attemptDeletePathRecursive(".\\67241244550e8400-114567123456789999");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\67241244550e8400-114567123456789999"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}
		}

		TEST_METHOD(Util_createFolderAndParentFolders)
		{
			for (int i = 0; i < 10; i++) {
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\550e8400-4444\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-1111\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");

				// Test method
				Util::createFolderAndParentFolders(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\550e8400-4444\\");

				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\550e8400-4444\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-1111\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");

				Util::attemptDeletePathRecursive(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\550e8400-4444\\");

				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\550e8400-4444\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsTrue(std::experimental::filesystem::exists(".\\550e8400-1111\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");

				Util::attemptDeletePathRecursive(".\\550e8400-1111\\");

				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\550e8400-4444\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\550e8400-3333\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-1111\\550e8400-2222\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-1111\\"), L"Error checking the existence of folder in file system: (createFolderAndParentFolders)");
			}
		}

		TEST_METHOD(Util_generateTestcasePathAndFilename)
		{
			for (int i = 0; i < 10; i++) {
				const FluffiServiceDescriptor sd1 = FluffiServiceDescriptor("testServiceDescriptor1", "");
				FluffiTestcaseID id1 = FluffiTestcaseID(sd1, 444);
				Assert::AreEqual(Util::generateTestcasePathAndFilename(id1, ".\\1111\\22222"), std::string(".\\1111\\22222\\-444"), L"Error generating path and filename for testcase : (generateTestcasePathAndFilename)");

				const FluffiServiceDescriptor sd3 = FluffiServiceDescriptor("testServiceDescriptor1", "");
				FluffiTestcaseID id3 = FluffiTestcaseID(sd3, 444);
				Assert::AreEqual(Util::generateTestcasePathAndFilename(id3, ""), std::string("\\-444"), L"Error generating path and filename for testcase : (generateTestcasePathAndFilename)");

				const FluffiServiceDescriptor sd5 = FluffiServiceDescriptor("testServiceDescriptor1", "test");
				FluffiTestcaseID id5 = FluffiTestcaseID(sd5, std::numeric_limits<uint64_t>::max());
				Assert::AreEqual(Util::generateTestcasePathAndFilename(id5, ".\\AAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCC\\CCCCCCCCCCCCDDDDDDDDDDDDEEEEEEEEEE"), std::string(".\\AAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCC\\CCCCCCCCCCCCDDDDDDDDDDDDEEEEEEEEEE\\test-18446744073709551615"), L"Error generating path and filename for testcase : (generateTestcasePathAndFilename)");

				const FluffiServiceDescriptor sd4 = FluffiServiceDescriptor("testServiceDescriptor1", "");
				FluffiTestcaseID id4 = FluffiTestcaseID(sd4, 0);
				Assert::AreEqual(Util::generateTestcasePathAndFilename(id4, "\\\\"), std::string("\\\\\\-0"), L"Error generating path and filename for testcase : (generateTestcasePathAndFilename)");

				const FluffiServiceDescriptor sd2 = FluffiServiceDescriptor("testServiceDescriptor2", "testGuiID1");
				FluffiTestcaseID id2 = FluffiTestcaseID(sd2, 444);
				Assert::AreEqual(Util::generateTestcasePathAndFilename(id2, ".\\1111\\22222"), std::string(".\\1111\\22222\\testGuiID1-444"), L"Error generating path and filename for testcase : (generateTestcasePathAndFilename)");
			}
		}

		TEST_METHOD(Util_setDefaultLogOptions)
		{
			// no testcase needed
			Util::setDefaultLogOptions();
		}

		TEST_METHOD(Util_readAllBytesFromFile)
		{
			for (int i = 0; i < 3; i++) {
				// Test Method -> Not existing file
				Assert::AreEqual(typeid(Util::readAllBytesFromFile(".\\550e8400-9876\\550e9876-9876\\xxx.txt")).name(), "class std::vector<char,class std::allocator<char> >", L"Error reading correct Bytes from File: (readAllBytesFromFile)");
				std::vector<char> data = Util::readAllBytesFromFile(".\\550e8400-9876\\550e9876-9876\\xxx.txt");
				std::vector<char> data2 = std::vector<char>();
				Assert::AreEqual(data.size(), data2.size(), L"Error reading correct Bytes from File: (readAllBytesFromFile)");
				Assert::IsTrue(data.empty(), L"Error reading correct Bytes from File: (readAllBytesFromFile)");

				// Test Method -> FileSize 0
				Util::createFolderAndParentFolders(".\\550e8400-11111123456789\\");
				std::ofstream testFile;
				testFile.open(".\\550e8400-11111123456789\\test.txt");
				testFile << "";
				testFile.close();
				std::vector<char> data3 = Util::readAllBytesFromFile(".\\550e8400-11111123456789\\test.txt");
				Assert::AreEqual(data3.size(), data2.size(), L"Error reading correct Bytes from File: (readAllBytesFromFile)");
				Assert::IsTrue(data.empty(), L"Error reading correct Bytes from File: (readAllBytesFromFile)");
				{
					GarbageCollectorWorker garbageCollectorWorker(200);
					garbageCollectorWorker.markFileForDelete(".\\550e8400-11111123456789\\test.txt");
				}
				Util::attemptDeletePathRecursive(".\\550e8400-11111123456789\\");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111123456789\\"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");

				// Test Method -> With FileSize
				Util::createFolderAndParentFolders(".\\550e8400-11111123456789999\\");
				std::ofstream testFile2;
				testFile.open(".\\550e8400-11111123456789999\\test.txt");
				testFile << "sdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdf";
				testFile.close();
				std::vector<char> data4 = Util::readAllBytesFromFile(".\\550e8400-11111123456789999\\test.txt");
				Assert::AreEqual(data4.size(), (size_t)54, L"Error reading correct Bytes from File: (readAllBytesFromFile)");
				std::string testString;
				testString.assign(data4.data(), data4.size());
				Assert::AreEqual(testString, std::string("sdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdfsdf"), L"Error reading correct Bytes from File: (readAllBytesFromFile)");
				Assert::IsTrue(data.empty(), L"Error reading correct Bytes from File: (readAllBytesFromFile)");
				{
					GarbageCollectorWorker garbageCollectorWorker(200);
					garbageCollectorWorker.markFileForDelete(".\\550e8400-11111123456789999\\test.txt");
				}
				Util::attemptDeletePathRecursive(".\\550e8400-11111123456789999\\");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\550e8400-11111123456789999\\"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}
		}

		TEST_METHOD(Util_attemptRenameFile)
		{
			// We need to clean up after other testcases
			Util::attemptDeletePathRecursive(".\\647241244550e8400-114567123456789999\\");
			// File does not exist
			for (int i = 0; i < 3; i++) {
				std::string testcaseDir = ".\\647241244550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error in renaming file method, folder uncorrectly exists: (attemptRenameFile)");

				// Test Method
				Assert::AreEqual(Util::attemptRenameFile(testcaseDir + "\\notExists.txt", testcaseDir + "\\eigentlichehwurst.txt"), false, L"Error in renaming file method, renaming should not work: (attemptRenameFile)");

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error in renaming file method, folder should not exist: (attemptRenameFile)");
			}

			// File exists -> no problems
			for (int i = 0; i < 3; i++) {
				std::string testcaseDir = ".\\647241244550e8400-114567123456789999\\550e8400-1113465467989999";
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error in renaming file method, folder should not exist: (attemptRenameFile)");
				Util::createFolderAndParentFolders(testcaseDir);
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (attemptRenameFile)");
				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testcaseNOTrenamed"), L"Error checking the existence of file in file system: (attemptRenameFile)");

				std::ofstream testFile;
				testFile.open(".\\647241244550e8400-114567123456789999\\550e8400-1113465467989999\\testcaseNOTrenamed");
				testFile << "AAAAAAAAAABBBBBBBBBBCCCCCCCCCC";
				testFile.close();
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testcaseNOTrenamed"), L"Error checking the existence of file in file system: (attemptRenameFile)");

				// Test Method
				Assert::AreEqual(Util::attemptRenameFile(testcaseDir + "\\testcaseNOTrenamed", testcaseDir + "\\testcaseRenamed"), true, L"Error in renaming file method: (attemptRenameFile)");

				Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testcaseNOTrenamed"), L"Error in renaming file method, file should no more exist: (attemptRenameFile)");
				Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testcaseRenamed"), L"Error in renaming file method, renamed file should exist: (attemptRenameFile)");

				// Also check the contents of the file
				std::vector<char> data = Util::readAllBytesFromFile(testcaseDir + "\\testcaseRenamed");
				Assert::AreEqual(data.size(), (size_t)30, L"Error reading correct Bytes from File: (attemptRenameFile)");
				std::string testString;
				testString.assign(data.data(), data.size());
				Assert::AreEqual(testString, std::string("AAAAAAAAAABBBBBBBBBBCCCCCCCCCC"), L"Error reading correct Bytes from File: (attemptRenameFile)");

				// Clean up
				Util::attemptDeletePathRecursive(".\\647241244550e8400-114567123456789999\\");
				Assert::IsFalse(std::experimental::filesystem::exists(".\\647241244550e8400-114567123456789999\\"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
			}

			// Try to rename file that is currently used -> Error -> 100 failed attempts
			std::string testcaseDir = ".\\647241244550e8400-114567123456789999\\550e8400-1113465467989999";
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error in renaming file method, folder should not exist: (attemptRenameFile)");
			Util::createFolderAndParentFolders(testcaseDir);
			Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (attemptRenameFile)");
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testcaseNOTrenamed"), L"Error checking the existence of file in file system: (attemptRenameFile)");

			std::ofstream testFile;
			testFile.open("647241244550e8400-114567123456789999\\550e8400-1113465467989999\\testcaseNOTrenamed");
			testFile << "AAAAAAAAAABBBBBBBBBBCCCCCCCCCC";

			// Test Method
			Assert::AreEqual(Util::attemptRenameFile(testcaseDir + "\\testcaseNOTrenamed", testcaseDir + "\\testcaseRenamed"), false, L"Error in renaming file method, should not be able to rename file in use: (attemptRenameFile)");

			testFile << "AAAAAAAAAABBBBBBBBBBCCCCCCCCCC2";
			testFile.close();
			Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir + "\\testcaseNOTrenamed"), L"Error in renaming file method, file must exist: (attemptRenameFile)");
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir + "\\testcaseRenamed"), L"Error in renaming file method, renamed file should not exist: (attemptRenameFile)");

			// Clean up
			Util::attemptDeletePathRecursive(".\\647241244550e8400-114567123456789999\\");
			Assert::IsFalse(std::experimental::filesystem::exists(".\\647241244550e8400-114567123456789999\\"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");

			// Try to rename current folder (.) -> Error -> 100 failed attempts
			testcaseDir = ".\\647241244550e8400-114567123456789999\\550e8400-1113465467989999";
			Assert::IsFalse(std::experimental::filesystem::exists(testcaseDir), L"Error in renaming file method, folder should not exist: (attemptRenameFile)");
			Util::createFolderAndParentFolders(testcaseDir);
			Assert::IsTrue(std::experimental::filesystem::exists(testcaseDir), L"Error checking the existence of folder in file system: (attemptRenameFile)");

			// Test Method
			Assert::AreEqual(Util::attemptRenameFile(testcaseDir + "\\.", testcaseDir + "\\.."), false, L"Error in renaming file method, should not be able to rename .: (attemptRenameFile)");

			// Clean up
			Util::attemptDeletePathRecursive(".\\647241244550e8400-114567123456789999\\");
			Assert::IsFalse(std::experimental::filesystem::exists(".\\647241244550e8400-114567123456789999\\"), L"Error checking the existence of folder in file system: (storeTestcaseFileOnDisk)");
		}

		TEST_METHOD(Util_stringHasEnding)
		{
			Assert::IsTrue(Util::stringHasEnding("12345", "12345"), L"1");
			Assert::IsTrue(Util::stringHasEnding("12345", "2345"), L"2");
			Assert::IsTrue(Util::stringHasEnding("12345", "345"), L"3");
			Assert::IsTrue(Util::stringHasEnding("12345", "45"), L"4");
			Assert::IsTrue(Util::stringHasEnding("12345", "5"), L"5");

			Assert::IsFalse(Util::stringHasEnding("12345.", "12345"), L"6");
		}

		TEST_METHOD(Util_replaceAll)
		{
			std::string test = "";
			Util::replaceAll(test, "a", "bb");
			Assert::IsTrue(test == "", L"1");

			test = "a";
			Util::replaceAll(test, "a", "bb");
			Assert::IsTrue(test == "bb", L"2");

			test = "aa";
			Util::replaceAll(test, "a", "bb");
			Assert::IsTrue(test == "bbbb", L"3");

			test = "aa";
			Util::replaceAll(test, "a", "b");
			Assert::IsTrue(test == "bb", L"4");

			test = "aa";
			Util::replaceAll(test, "aa", "b");
			Assert::IsTrue(test == "b", L"5");
		}
	};
}