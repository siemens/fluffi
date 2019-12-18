/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Roman Bendt, Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "Util.h"
#include "CommInt.h"
#include "ExternalProcess.h"
#include "FluffiTestcaseID.h"
#include "WorkerThreadState.h"
#include "GarbageCollectorWorker.h"

Util::Util() {
}

Util::~Util() {
}

#if defined(_WIN32) || defined(_WIN64)

const std::string Util::pathSeperator = "\\";

std::string Util::newGUID() {
	UUID uuid;
	RPC_STATUS success = UuidCreate(&uuid);

	unsigned char* str;
	success = UuidToStringA(&uuid, &str);

	std::string s((char*)str);

	success = RpcStringFreeA(&str);

	return s;
}

int Util::kbhit() {
	return _kbhit();
}

void Util::setConsoleWindowTitle(const std::string title) {
	SetConsoleTitle(title.c_str());
	return;
}

//According to the Microsoft documentation at https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/debug-privilege
bool Util::enableDebugPrivilege()
{
	LUID luid;
	bool bRet = false;
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hToken;

	if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		{
			TOKEN_PRIVILEGES tp;

			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			//
			//  Enable the privilege or disable all privileges.
			//
			if (AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
			{
				//
				//  Check to see if you have proper access.
				//  You may get "ERROR_NOT_ALL_ASSIGNED".
				//
				bRet = (GetLastError() == ERROR_SUCCESS);
			}
		}
		CloseHandle(hToken);
	}

	return bRet;
}

#else

const std::string Util::pathSeperator = "/";

std::string Util::newGUID() {
	uuid_t uuid;
	uuid_generate_random(uuid);
	char s[37];
	uuid_unparse(uuid, s);

	return s;
}

/**
Linux (POSIX) implementation of _kbhit().
Morgan McGuire, morgan@cs.brown.edu
*/
int Util::kbhit() {
	static const int STDIN = 0;
	static bool initialized = false;

	if (!initialized) {
		// Use termios to turn off line buffering
		termios term;
		tcgetattr(STDIN, &term);
		term.c_lflag &= ~ICANON;
		tcsetattr(STDIN, TCSANOW, &term);
		setbuf(stdin, NULL);
		initialized = true;
	}

	int bytesWaiting;
	if (ioctl(STDIN, FIONREAD, &bytesWaiting) == 0) {
		return bytesWaiting;
	}
	else {
		LOG(ERROR) << "Util::kbhit:ioctl failed with errno " << errno;
		return 0; //we did not detect a keypress
	}
}

void Util::setConsoleWindowTitle(const std::string title) {
	printf("%c]0;%s%c", '\033', title.c_str(), '\007');
	return;
}

//set /proc/sys/kernel/yama/ptrace_scope to 0 (see https://www.kernel.org/doc/Documentation/security/Yama.txt)
bool Util::enableDebugPrivilege()
{
	//Read the current value
	{
		int fh = open("/proc/sys/kernel/yama/ptrace_scope", O_RDONLY);
		if (fh == -1) {
			LOG(DEBUG) << "It seems like /proc/sys/kernel/yama/ptrace_scope does not exist. We seem to be on a system without CONFIG_SECURITY_YAMA.";
			return true;
		}

		char buffer;
		if (1 != read(fh, &buffer, 1)) {
			LOG(ERROR) << "Failed reading from /proc/sys/kernel/yama/ptrace_scope";
			close(fh);
			return false;
		}

		if (buffer == '0') {
			LOG(DEBUG) << "/proc/sys/kernel/yama/ptrace_scope is already set to 0";
			close(fh);
			return true;
		}

		close(fh);
	}

	//Try setting /proc/sys/kernel/yama/ptrace_scope to 1
	{
		int fh = open("/proc/sys/kernel/yama/ptrace_scope", O_WRONLY);
		if (fh == -1) {
			LOG(ERROR) << "It seems like /proc/sys/kernel/yama/ptrace_scope is set to 0 but we cannot write it. ATTACHING WON'T WORK IF WE ARE NOT ROOT!";
			return false;
		}

		char buffer = '0';
		if (1 != write(fh, &buffer, 1)) {
			LOG(ERROR) << "Failed writing to /proc/sys/kernel/yama/ptrace_scope";
			close(fh);
			return false;
		}

		LOG(DEBUG) << "Setting /proc/sys/kernel/yama/ptrace_scope to 0 completed.";
		close(fh);
		return true;
	}
}

#endif

std::vector<std::string> Util::splitString(std::string str, std::string token) {
	if (str.empty())	return std::vector<std::string> { "" };

	if (token.empty()) return std::vector<std::string> { str };

	std::vector<std::string>result;
	size_t curIndex = 0;
	size_t lenOfToken = token.size();
	while (true) {
		size_t nextIndex = str.find(token, curIndex);
		if (nextIndex != std::string::npos) {
			result.push_back(str.substr(curIndex, nextIndex - curIndex));
		}
		else {
			result.push_back(str.substr(curIndex));
			break;
		}
		curIndex = nextIndex + lenOfToken;
	}
	return result;
}

// AV safe rm -rf
void Util::attemptDeletePathRecursive(std::experimental::filesystem::v1::path dir)
{
	while (true) {
		try {
			std::experimental::filesystem::v1::remove_all(dir);
			break;
		}
		catch (std::experimental::filesystem::v1::filesystem_error & e) {
			LOG(ERROR) << "Could not delete " << dir << ". retrying. (" << e.what() << ")";
		}
	}
}

bool Util::appendRemainingChunks(FILE* outstream, const FluffiTestcaseID testcaseID, const std::string sourceHAP, CommInt* commPtr, WorkerThreadState* workerThreadState) {
	bool testcaseComplete = false;
	int chunkID = 1;

	// While further chunks pending, get next chunk and append in filesystem
	while (testcaseComplete == false) {
		FLUFFIMessage req;
		GetTestCaseChunkRequest* nextChunkRequest = new GetTestCaseChunkRequest();

		TestcaseID* mutableTestcaseID = new TestcaseID();
		mutableTestcaseID->CopyFrom(testcaseID.getProtobuf());

		nextChunkRequest->set_allocated_id(mutableTestcaseID);
		nextChunkRequest->set_chunkid(chunkID);

		req.set_allocated_gettestcasechunkrequest(nextChunkRequest);
		FLUFFIMessage resp;

		bool result = commPtr->sendReqAndRecvResp(&req, &resp, workerThreadState, sourceHAP, CommInt::timeoutNormalMessage);
		if (!result) {
			LOG(ERROR) << "AppendRemainingChunks could not receive chunk  " << chunkID << " from " << sourceHAP;
			return false;
		}

		const GetTestCaseChunkResponse* receivedTestcaseChunk = &resp.gettestcasechunkresponse();
		fwrite(receivedTestcaseChunk->testcasechunk().c_str(), sizeof(char), receivedTestcaseChunk->testcasechunk().size(), outstream);
		fflush(outstream);

		if (receivedTestcaseChunk->islastchunk() == true) {
			testcaseComplete = true;
		}
		chunkID++;
	}

	return true;
}

std::string  Util::loadTestcaseChunkInMemory(const FluffiTestcaseID testcaseId, const std::string testcaseDir, int chunkNum, bool* isLastChunk) {
	// Stream to load file into memory
	std::string testfile = Util::generateTestcasePathAndFilename(testcaseId, testcaseDir);
	std::ifstream ifs(testfile, std::ios::binary | std::ios::in);
	std::streamoff size = CommInt::chunkSizeInBytes;
	if (!(ifs.good())) {
		LOG(ERROR) << "loadTestcaseChunkInMemory: Could not open file " << testfile;
		return "";
	}

	// file length calculation for last-chunk calculation
	ifs.seekg(0, ifs.end);
	std::streamoff completeFilelength = ifs.tellg();
	if (chunkNum * CommInt::chunkSizeInBytes > completeFilelength) {
		LOG(ERROR) << "loadTestcaseChunkInMemory: Requested invalid chunk number!";
		return "";
	}

	ifs.seekg(chunkNum * CommInt::chunkSizeInBytes, std::ios::beg);
	std::streamoff  sizeOfAlreadyTransferedParts = ifs.tellg();

	if (completeFilelength > sizeOfAlreadyTransferedParts + CommInt::chunkSizeInBytes) {
		*isLastChunk = false;
	}
	else {
		size = completeFilelength - sizeOfAlreadyTransferedParts;
		*isLastChunk = true;
	}

	// Load Testcase into memory

	char* buffer = new char[static_cast<unsigned int>(size)];
	ifs.read(buffer, static_cast<std::streamsize>(size));
	std::string data = std::string(buffer, static_cast<unsigned int>(size));

	delete[] buffer;

	ifs.close();

	return data;
}

//Meant for use by database exclusively
std::string  Util::loadTestcaseInMemory(const FluffiTestcaseID testcaseId, const std::string testcaseDir) {
	// Stream to load file into memory
	std::string testfile = Util::generateTestcasePathAndFilename(testcaseId, testcaseDir);
	std::ifstream ifs(testfile, std::ios::binary | std::ios::in);
	if (!(ifs.good())) {
		LOG(ERROR) << "loadTestcaseInMemory: FileStream in use or file not found!";
		return "";
	}

	// file length calculation for last-chunk calculation
	ifs.seekg(0, ifs.end);
	std::streamoff completeFilelength = ifs.tellg();

	ifs.seekg(0, std::ios::beg);

	// Load Testcase into memory

	char* buffer = new char[static_cast<unsigned int>(completeFilelength)];
	ifs.read(buffer, static_cast<std::streamsize>(completeFilelength));
	std::string data = std::string(buffer, static_cast<unsigned int>(completeFilelength));

	delete[] buffer;

	ifs.close();

	return data;
}

void Util::markAllFilesOfTypeInPathForDeletion(const std::string path, const std::string type, GarbageCollectorWorker* garbageCollectorWorker) {
	// find all files matching *.type
	for (auto& p : std::experimental::filesystem::directory_iterator(path)) {
		if (std::experimental::filesystem::is_regular_file(p.status())) {
			if (p.path().extension().compare(type) == 0) {
				// delete every  one
				garbageCollectorWorker->markFileForDelete(p.path().string());
			}
		}
	}
}

bool Util::storeTestcaseFileOnDisk(const FluffiTestcaseID testcaseId,
	const std::string testcaseDir,
	const std::string* firstChunk,
	bool isLastChunk,
	const std::string sourceHAP,
	CommInt* commPtr,
	WorkerThreadState* workerThreadState,
	GarbageCollectorWorker* garbageCollectorWorker
) {
	std::string filename = Util::generateTestcasePathAndFilename(testcaseId, testcaseDir);

	if (std::experimental::filesystem::exists(filename)) {
		LOG(DEBUG) << "Util::storeTestcaseFileOnDisk: is supposed to write " << filename << " which already exists.";
		if (garbageCollectorWorker->isFileMarkedForDeletion(filename)) {
			int deletionAttemtps = 0;
			while (!workerThreadState->m_stopRequested && std::experimental::filesystem::exists(filename)) {
				//Thetestcase file must be deleted before continuing. If this is not properly done, bugs will appear due to double reference / double delete
				if (deletionAttemtps > 0) {
					LOG(WARNING) << "Performance warning: storeTestcaseFileOnDisk is waiting for the GarbageCollector";
				}
				deletionAttemtps++;
				garbageCollectorWorker->collectNow(); //Dear garbage collctor: Stop sleeping and try deleting files!
				garbageCollectorWorker->collectGarbage(); //This is threadsafe. As the garbage collector could not yet delete the file yet, let's help him!
			}

			if (std::experimental::filesystem::exists(filename)) {
				LOG(ERROR) << "Util::storeTestcaseFileOnDisk could not store the file " << filename << "on disk as it already existed and could not be deleted";
				return false;
			}
		}
		else {
			//The file exists and is NOT marked for deletion
			LOG(ERROR) << "Util::storeTestcaseFileOnDisk attempts to overwrite the file " << filename << " which is not marked for deletion";
			return false;
		}
	}

	// Write received testcase into filesystem
	//apparently, fwrite performs much better than ofstream
	FILE* testcaseFile;
	if (0 != fopen_s(&testcaseFile, filename.c_str(), "wb")) {
		LOG(ERROR) << "Util::storeTestcaseFileOnDisk failed to open the file " << filename;
		return false;
	}
	fwrite(firstChunk->c_str(), sizeof(char), firstChunk->size(), testcaseFile);
	fflush(testcaseFile);

	// Check if file complete and if not: get the remaining chunks and append them to the file
	if (isLastChunk != true) {
		LOG(DEBUG) << "FileSize too big! Division into chunks!";

		bool success = Util::appendRemainingChunks(testcaseFile, testcaseId, sourceHAP, commPtr, workerThreadState);
		if (!success) {
			LOG(ERROR) << "appendRemainingChunks failed!";
			fclose(testcaseFile);
			return false;
		}
	}

	fclose(testcaseFile);

	return true;
}

void Util::createFolderAndParentFolders(std::experimental::filesystem::v1::path folder) {
	std::experimental::filesystem::v1::path parent = folder.parent_path();

	if (!std::experimental::filesystem::exists(parent)) {
		createFolderAndParentFolders(parent);
	}

	if (!std::experimental::filesystem::exists(folder)) {
		std::experimental::filesystem::create_directory(folder);
	}
}

std::string Util::generateTestcasePathAndFilename(const FluffiTestcaseID testcaseID, std::string testcaseDir) {
	std::string pathAndFilename = testcaseDir + Util::pathSeperator +
		testcaseID.m_serviceDescriptor.m_guid + "-" + std::to_string(testcaseID.m_localID);
	return pathAndFilename;
}

void Util::setDefaultLogOptions(std::string filename) {
#if defined(_WIN32) || defined(_WIN64)
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
	//Windows 10 supports colorful console, but it needs to be activated
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD consoleMode;
	GetConsoleMode(hConsole, &consoleMode);
	consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hConsole, consoleMode);
#endif

	// Set logging options
	el::Configurations defaultConf;
	el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
	defaultConf.setToDefault();
	defaultConf.set(el::Level::Debug, el::ConfigurationType::Format, "%datetime %level %fbase : %msg");
	if (!filename.empty())
	{
		defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
		defaultConf.setGlobally(el::ConfigurationType::Filename, filename);
	}
	el::Loggers::reconfigureLogger("default", defaultConf);
}

std::vector<char> Util::readAllBytesFromFile(const std::string filename)
{
	FILE* inputFile;
	if (0 != fopen_s(&inputFile, filename.c_str(), "rb")) {
		LOG(ERROR) << "Util::readAllBytesFromFile failed to open the file " << filename;
		return{};
	}

	fseek(inputFile, 0, SEEK_END);
	long fileSize = ftell(inputFile);
	fseek(inputFile, 0, SEEK_SET);

	if (fileSize == 0) {
		fclose(inputFile);
		return{};
	}

	std::vector<char>  result(static_cast<unsigned int>(fileSize));

	size_t  bytesRead = fread(&result[0], 1, fileSize, inputFile);
	if (bytesRead != static_cast<size_t>(fileSize)) {
		LOG(WARNING) << "readAllBytesFromFile failed to read all bytes! Bytes read: " << bytesRead << ". Filesize " << fileSize;
	}

	fclose(inputFile);

	return result;
}

bool Util::attemptRenameFile(const std::string from, const std::string to)
{
	std::error_code ec;
	if (!std::experimental::filesystem::exists(from, ec)) {
		LOG(WARNING) << "Util::attemptRenameFile is supposed to rename the non-existant file " << from << " (" << ec.message() << ")";
		return false;
	}

	const unsigned int timesToRetry = 100;

	for (unsigned int i = 0; i < timesToRetry; ++i)
	{
		if (std::rename(from.c_str(), to.c_str()) == 0)
		{
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	if (!std::experimental::filesystem::exists(from, ec)) {
		LOG(ERROR) << "Util::attemptRenameFile failed, and the file " << from << " does still not exist (" << ec.message() << ")";
	}
	else {
		LOG(ERROR) << "Util::attemptRenameFile failed, but the file " << from << " does exist (" << ec.message() << ")";
	}

	return false;
}

std::string Util::agentTypeToString(AgentType a) {
	switch (a) {
	case AgentType::TestcaseGenerator:
		return "TG";
		break;
	case AgentType::TestcaseRunner:
		return "TR";
		break;
	case AgentType::TestcaseEvaluator:
		return "TE";
		break;
	case AgentType::GlobalManager:
		return "GM";
		break;
	case AgentType::LocalManager:
		return "LM";
		break;
	default:
		return "INVALID AgentType";
	}
}

//Needed for Umlaute in database
std::string Util::wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

bool Util::stringHasEnding(std::string const &fullString, std::string const &ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

std::tuple<std::string, std::string> Util::splitPathIntoDirectoryAndFileName(std::string path) {
#if defined(_WIN32) || defined(_WIN64)
	std::replace(path.begin(), path.end(), '/', '\\');
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath_s(path.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
	std::string directoryname = std::string(drive) + std::string(dir);
	std::string filename = std::string(fname) + std::string(ext);
	directoryname = directoryname.substr(0, directoryname.size() - 1);
#else
	std::replace(path.begin(), path.end(), '\\', '/');
	std::string filename = basename(const_cast<char*>(path.c_str())); // yes, const_cast is dirty as shit and might crash everything.
	std::string directoryname = dirname(const_cast<char*>(path.c_str())); // yes, const_cast is dirty as shit and might crash everything.

#endif
	return std::tuple<std::string, std::string>(directoryname, filename);
}

void Util::replaceAll(std::string &s, const std::string &search, const std::string &replace) {
	for (size_t pos = 0; ; pos += replace.length()) {
		// Locate the substring to replace
		pos = s.find(search, pos);
		if (pos == std::string::npos) break;
		// Replace by erasing and inserting
		s.erase(pos, search.length());
		s.insert(pos, replace);
	}
}
