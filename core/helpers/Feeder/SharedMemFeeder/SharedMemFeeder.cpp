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
#include "SharedMemIPC.h"

#if defined(_WIN32) || defined(_WIN64)
#else
// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}
#endif

std::wstring getCMDLineFromPID(int targetPID) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE targetHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, targetPID);
	if (targetHandle == NULL) {
		std::cout << "SharedMemFeeder: Error opening process  " << targetPID << std::endl;
		return L"";
	}

	PROCESS_BASIC_INFORMATION pbi = { 0 };
	NTSTATUS success = NtQueryInformationProcess(targetHandle, ProcessBasicInformation, &pbi, sizeof(PROCESS_BASIC_INFORMATION), NULL);
	if (success != 0) {
		std::cout << "SharedMemFeeder: Error quering information from process  " << targetPID << std::endl;
		return L"";
	}

	PEB targetPEB = { 0 };
	SIZE_T numberOfBytesRead = 0;
	ReadProcessMemory(targetHandle, pbi.PebBaseAddress, &targetPEB, sizeof(PEB), &numberOfBytesRead);
	if (numberOfBytesRead != sizeof(PEB)) {
		std::cout << "SharedMemFeeder: Error reading the target's PEB  " << std::endl;
		return L"";
	}

	RTL_USER_PROCESS_PARAMETERS targetProcessParameters = { 0 };
	ReadProcessMemory(targetHandle, targetPEB.ProcessParameters, &targetProcessParameters, sizeof(RTL_USER_PROCESS_PARAMETERS), &numberOfBytesRead);
	if (numberOfBytesRead != sizeof(RTL_USER_PROCESS_PARAMETERS)) {
		std::cout << "SharedMemFeeder: Error reading the target's RTL_USER_PROCESS_PARAMETERS  " << std::endl;
		return L"";
	}

	char* cmdlineBytes = new char[targetProcessParameters.CommandLine.Length + 4];
	memset(cmdlineBytes, 0, targetProcessParameters.CommandLine.Length + 4);
	ReadProcessMemory(targetHandle, targetProcessParameters.CommandLine.Buffer, cmdlineBytes, targetProcessParameters.CommandLine.Length, &numberOfBytesRead);
	if (numberOfBytesRead != targetProcessParameters.CommandLine.Length) {
		std::cout << "SharedMemFeeder: Error reading the target's CMDline  " << std::endl;
		delete[] cmdlineBytes;
		return L"";
	}

	std::wstring re = std::wstring((wchar_t*)cmdlineBytes);
	delete[] cmdlineBytes;
	CloseHandle(targetHandle);

	return re;
#else
	// Read contents of virtual /proc/{pid}/cmdline file
	std::string cmdPath = std::string("/proc/") + std::to_string(targetPID) + "/cmdline";
	std::ifstream cmdFile(cmdPath.c_str());
	std::string cmdLine;
	getline(cmdFile, cmdLine);
	if (!cmdLine.empty())
	{
		std::replace(cmdLine.begin(), cmdLine.end(), '\0', ' ');
		std::cout << "SharedMemFeeder: Got target's command line: \"" << cmdLine << "\"" << std::endl;

		rtrim(cmdLine);

		std::wstring re(cmdLine.begin(), cmdLine.end());

		return re;
	}
	else {
		std::cout << "SharedMemFeeder: Error reading the target's CMDline  " << std::endl;
		return L"";
	}

#endif
}

std::vector<std::string> splitString(std::string str, std::string token) {
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

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << "Usage: SharedMemFeeder.exe <shared memory name>";
		return -1;
	}

	SharedMemIPC sharedMemIPC_ToRunner(argv[1]);
	bool success = sharedMemIPC_ToRunner.initializeAsClient();
	std::cout << "SharedMemFeeder: initializeAsClient for runner" << success << std::endl;
	if (!success) {
		return -1;
	}

	int feederTimeoutMS = 10 * 60 * 1000;

	std::string pipeToTarget = "";
	SharedMemMessage message__FromRunner;
	sharedMemIPC_ToRunner.waitForNewMessageToClient(&message__FromRunner, feederTimeoutMS);
	SharedMemMessageType mt = message__FromRunner.getMessageType();
	if (mt == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
		std::cout << "SharedMemFeeder: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message, terminating!" << std::endl;
		return -1;
	}
	else if (mt == SHARED_MEM_MESSAGE_TARGET_PID) {
		std::cout << "SharedMemFeeder: Recived a SHARED_MEM_MESSAGE_TARGET_PID message!" << std::endl;
		std::string targetPIDAsString = std::string(message__FromRunner.getDataPointer(), message__FromRunner.getDataPointer() + message__FromRunner.getDataSize());
		std::cout << "SharedMemFeeder: target pid is \"" << targetPIDAsString << "\"" << std::endl;
		std::wstring targetcmdline = getCMDLineFromPID(std::stoi(targetPIDAsString));
		if (targetcmdline == L"") {
			std::string errorMessage = "Command line of target(PID " + targetPIDAsString + ") could not be read!";
			std::cout << "SharedMemFeeder: " << errorMessage << std::endl;
			SharedMemMessage tmp(SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE, errorMessage.c_str(), static_cast<int>(errorMessage.length()));
			sharedMemIPC_ToRunner.sendMessageToServer(&tmp);
			return -1;
		}

		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		std::string targetcmdline_s = converter.to_bytes(targetcmdline);
		size_t actualLineStart = targetcmdline_s.find_first_not_of(" \t\f\v\n\r", 0);
		size_t actualLineEnd = targetcmdline_s.find_last_not_of(" \t\f\v\n\r", targetcmdline_s.length());
		if (actualLineStart == std::string::npos || actualLineEnd == std::string::npos) {
			std::string errorMessage = "Command line of target(PID " + targetPIDAsString + ") seems to be empty!";
			std::cout << "SharedMemFeeder: " << errorMessage << std::endl;
			SharedMemMessage tmp(SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE, errorMessage.c_str(), static_cast<int>(errorMessage.length()));
			sharedMemIPC_ToRunner.sendMessageToServer(&tmp);
			return -1;
		}
		std::string trimmedLine = targetcmdline_s.substr(actualLineStart, actualLineEnd - actualLineStart + 1);

		std::vector<std::string> targetcmdline_splitted = splitString(trimmedLine, " ");
		pipeToTarget = targetcmdline_splitted[targetcmdline_splitted.size() - 1];

		std::cout << "SharedMemFeeder: pipe to target is is \"" << pipeToTarget << "\"" << std::endl;
	}
	else {
		std::cout << "SharedMemFeeder: Recived an unexpeced message from runner(" << mt << "), terminating!" << std::endl;
		return -1;
	}

	std::chrono::time_point<std::chrono::system_clock> timeOfFirstConnectAttempt = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> timeOfLastConnectAttempt = timeOfFirstConnectAttempt + std::chrono::milliseconds(feederTimeoutMS);

	SharedMemIPC sharedMemIPC_ToTarget(pipeToTarget.c_str());
	success = false;
	while (success == false && std::chrono::system_clock::now() < timeOfLastConnectAttempt) {
		success = sharedMemIPC_ToTarget.initializeAsClient();

		if (!success) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	std::cout << "SharedMemFeeder: initializeAsClient for target " << (success ? "true" : "false") << std::endl;
	if (!success) {
		return -1;
	}

	//wait until target is ready
	{
		SharedMemMessage message__FromTarget;
		sharedMemIPC_ToTarget.waitForNewMessageToClient(&message__FromTarget, feederTimeoutMS);
		if (message__FromTarget.getMessageType() == SHARED_MEM_MESSAGE_READY_TO_FUZZ) {
			std::cout << "SharedMemFeeder: target did send SHARED_MEM_MESSAGE_READY_TO_FUZZ. We are good to go!" << std::endl;
			SharedMemMessage tmp(SHARED_MEM_MESSAGE_READY_TO_FUZZ, nullptr, 0);
			sharedMemIPC_ToRunner.sendMessageToServer(&tmp);
		}
		else {
			std::string errorMessage = "SharedMemFeeder: Something went wrong! The target did not come up!";
			std::cout << errorMessage << std::endl;
			SharedMemMessage tmp(SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE, errorMessage.c_str(), static_cast<int>(errorMessage.length()));
			sharedMemIPC_ToRunner.sendMessageToServer(&tmp);
			return -1;
		}
	}

	SharedMemMessage message__FromTarget;
	while (true) {
		sharedMemIPC_ToRunner.waitForNewMessageToClient(&message__FromRunner, feederTimeoutMS);
		SharedMemMessageType messageFromRunnerType = message__FromRunner.getMessageType();
		if (messageFromRunnerType == SHARED_MEM_MESSAGE_FUZZ_FILENAME) {
			//std::cout << "SharedMemFeeder: Recived a SHARED_MEM_MESSAGE_FUZZ_FILENAME message :)" << std::endl;
			sharedMemIPC_ToTarget.sendMessageToServer(&message__FromRunner);

			sharedMemIPC_ToTarget.waitForNewMessageToClient(&message__FromTarget, feederTimeoutMS);
			//std::cout << "SharedMemFeeder: Forwarding a message of type " << message__FromTarget.getMessageType() << " to the runner" << std::endl;
			sharedMemIPC_ToRunner.sendMessageToServer(&message__FromTarget);
		}
		else if (messageFromRunnerType == SHARED_MEM_MESSAGE_TERMINATE) {
			sharedMemIPC_ToTarget.sendMessageToServer(&message__FromRunner);
			std::cout << "SharedMemFeeder: Recived a SHARED_MEM_MESSAGE_TERMINATE, terminating!" << std::endl;
			return 0;
		}
		else if (messageFromRunnerType == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
			std::cout << "SharedMemFeeder: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message, terminating!" << std::endl;
			return -1;
		}
		else {
			std::cout << "SharedMemFeeder: Recived an unexpeced message from runner(" << messageFromRunnerType << "), terminating!" << std::endl;
			return -1;
		}
	}

	return 0;
}
