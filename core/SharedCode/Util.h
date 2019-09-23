/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Roman Bendt
*/

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <Rpc.h>
#include "conio.h"
#else
#include "uuid.h"
#include "sys/times.h"
//#include "sys/vtimes.h"
#include <sys/ioctl.h>
#include <termios.h>
#endif

// define bitness windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENV64BIT
#else
#define ENV32BIT
#endif
#endif

// define bitness linux
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENV64BIT
#else
#define ENV32BIT
#endif
#endif

class CommInt;
class WorkerThreadState;
class FluffiTestcaseID;
class GarbageCollectorWorker;
class Util
{
public:
	Util();
	~Util();

	static std::string agentTypeToString(AgentType a);
	static void markAllFilesOfTypeInPathForDeletion(const std::string path, const std::string type, GarbageCollectorWorker* garbageCollectorWorker);
	static void attemptDeletePathRecursive(std::experimental::filesystem::v1::path dir);
	static bool attemptRenameFile(const std::string from, const std::string to);
	static void createFolderAndParentFolders(std::experimental::filesystem::v1::path folder);
	static bool enableDebugPrivilege();
	static std::string generateTestcasePathAndFilename(const FluffiTestcaseID testcaseID, std::string testcaseDir);
	static int kbhit();
	static std::string loadTestcaseChunkInMemory(const FluffiTestcaseID testcaseId, const std::string testcaseDir, int chunkNum, bool* isLastChunk);
	static std::string loadTestcaseInMemory(const FluffiTestcaseID testcaseId, const std::string testcaseDir); //Meant for use by database exclusively
	static std::string newGUID();
	static std::vector<char> readAllBytesFromFile(const std::string filename);
	static void setConsoleWindowTitle(const std::string title);
	static void setDefaultLogOptions(std::string filename = "");
	static std::vector<std::string> splitString(std::string str, std::string token);
	static std::tuple<std::string, std::string> splitPathIntoDirectoryAndFileName(std::string path);
	static bool storeTestcaseFileOnDisk(const FluffiTestcaseID testcaseId, const std::string testcaseDir, const std::string* firstChunk, bool isLastChunk, const  std::string sourceHAP, CommInt* commPtr, WorkerThreadState* workerThreadState, GarbageCollectorWorker* garbageCollectorWorker);
	static bool stringHasEnding(std::string const &fullString, std::string const &ending);
	static void replaceAll(std::string &s, const std::string &search, const std::string &replace);
	static std::string wstring_to_utf8(const std::wstring& str);

	const static std::string pathSeperator;

private:

	static bool appendRemainingChunks(FILE* outstream, const FluffiTestcaseID testcaseID, const  std::string sourceHAP, CommInt* commPtr, WorkerThreadState* workerThreadState);
};
