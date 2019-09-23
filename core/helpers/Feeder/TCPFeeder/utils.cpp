/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier, Pascal Eckmann, Roman Bendt
*/

§§#include "utils.h"
§§#include "stdafx.h"
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§#define SOCKETTYPE SOCKET
§§#else
§§#define SOCKETTYPE int
§§#define closesocket close
§§#define INVALID_SOCKET -1
§§#define SOCKET_ERROR -1
§§int memcpy_s(void* a, size_t b, const void* c, size_t d) {
§§	return ((memcpy(a, c, std::min(b, d)) == a) ? 0 : -1);
§§}
§§
§§char** split_commandline(const std::string cmdline)
§§{
§§	char** argv = NULL;
§§
§§	wordexp_t p;
§§
§§	// Note! This expands shell variables.
§§	int success = wordexp(cmdline.c_str(), &p, WRDE_NOCMD);
§§	if (success != 0)
§§	{
§§		std::cout << "TCPFeeder: wordexp \"" << cmdline << "\" failed: " << success << std::endl;
§§		return NULL;
§§	}
§§
§§	if (!(argv = (char**)calloc(p.we_wordc + 1, sizeof(char*))))
§§	{
§§		wordfree(&p);
§§		std::cout << "TCPFeeder: calloc failed" << std::endl;
§§		return NULL;
§§	}
§§
§§	for (size_t i = 0; i < p.we_wordc; i++)
§§	{
§§		argv[i] = strdup(p.we_wordv[i]);
§§	}
§§	argv[p.we_wordc] = NULL;
§§
§§	wordfree(&p);
§§
§§	return argv;
§§}
§§
§§std::string execCommandAndGetOutput(std::string command) {
§§	std::cout << "TCPFeeder: executing command " << command << std::endl;
§§	int link[2];
§§	if (pipe(link) == -1) {
§§		std::cout << "TCPFeeder: pipe failed " << std::endl;
§§		return "ERROR";
§§	}
§§
§§	pid_t pID = fork();
§§	if (pID == 0) // child
§§	{
§§		dup2(link[1], STDOUT_FILENO);
§§		close(link[0]);
§§		close(link[1]);
§§
§§		char** argv = split_commandline(command);
§§
§§		execvp(argv[0], &argv[1]);
§§		std::cout << "TCPFeeder: Failed to call execv:" << errno << std::endl;
§§		return "ERROR";
§§	}
§§	else if (pID < 0) // failed to fork
§§	{
§§		std::cout << "TCPFeeder: Failed to fork to new process!" << std::endl;
§§		return "ERROR";
§§	}
§§	else // Code only executed by parent process
§§	{
§§		close(link[1]);
§§		char buff[4096];
§§		int nbytes = 0;
§§		std::string totalStr;
§§		while (0 != (nbytes = read(link[0], buff, sizeof(buff)))) {
§§			totalStr = totalStr + buff;
§§			memset(buff, 0, sizeof(buff));
§§		}
§§		close(link[0]);
§§
§§		int status = 0;
§§		waitpid(pID, &status, 0);
§§
§§		return totalStr;
§§	}
§§}
§§#endif
§§
§§
§§bool isPortOpen(std::string target, uint16_t port)
§§{
§§	int iResult = 0;
§§
§§	// Create a SOCKET for connecting to server
§§	SOCKETTYPE connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
§§	if (connectSocket == INVALID_SOCKET) {
§§		std::cout << "TCPFeeder: socket failed!" << std::endl;
§§		throw std::runtime_error("Socket failed");
§§	}
§§
§§	struct sockaddr_in saServer;
§§	memset(&saServer, 0, sizeof(saServer));
§§	saServer.sin_family = AF_INET;
§§	saServer.sin_port = htons(port);
§§#if defined(_WIN32) || defined(_WIN64)
§§	if (inet_pton(AF_INET, target.c_str(), &saServer.sin_addr) == 0) {
§§#else
§§	if (inet_aton(target.c_str(), &saServer.sin_addr) == 0) {
§§#endif
§§		std::cout << "Error using inet_aton" << std::endl;
§§		closesocket(connectSocket);
§§		throw std::runtime_error("Error using inet_aton");
§§	}
§§
§§	// Connect to server.
§§	iResult = connect(connectSocket, (struct sockaddr*)&saServer, sizeof(saServer));
§§	if (iResult == SOCKET_ERROR) {
§§		closesocket(connectSocket);
§§		connectSocket = INVALID_SOCKET;
§§		return false;
§§	}
§§
§§	closesocket(connectSocket);
§§	connectSocket = INVALID_SOCKET;
§§	return true;
§§	}
§§
§§std::vector<std::string> splitString(std::string str, std::string token) {
§§	if (str.empty())	return std::vector<std::string> { "" };
§§
§§	if (token.empty()) return std::vector<std::string> { str };
§§
§§	std::vector<std::string>result;
§§	while (str.size()) {
§§		size_t index = str.find(token);
§§		if (index != std::string::npos) {
§§			result.push_back(str.substr(0, index));
§§			str = str.substr(index + token.size());
§§			if (str.size() == 0)result.push_back(str);
§§		}
§§		else {
§§			result.push_back(str);
§§			str = "";
§§		}
§§	}
§§	return result;
§§}
§§
§§std::vector<char> readAllBytesFromFile(const std::string filename)
§§{
§§	FILE* inputFile;
§§	if (0 != fopen_s(&inputFile, filename.c_str(), "rb")) {
§§		std::cout << "TCPFeeder: readAllBytesFromFile failed to open the file " << filename << std::endl;
§§		return{};
§§	}
§§
§§	fseek(inputFile, 0, SEEK_END);
§§	long fileSize = ftell(inputFile);
§§	fseek(inputFile, 0, SEEK_SET);
§§
§§	if ((unsigned int)fileSize == 0) {
§§		fclose(inputFile);
§§		return{};
§§	}
§§
§§	std::vector<char> result((unsigned int)fileSize);
§§
§§	fread((char*)&result[0], fileSize, 1, inputFile);
§§
§§	fclose(inputFile);
§§
§§	return result;
§§}
§§
§§int getServerPortFromPID(int targetPID) {
§§#if defined(_WIN32) || defined(_WIN64)
§§	// Declare and initialize variables
§§	PMIB_TCPTABLE2 pTcpTable;
§§
§§	pTcpTable = (MIB_TCPTABLE2*)malloc(sizeof(MIB_TCPTABLE2));
§§	if (pTcpTable == NULL) {
§§		std::cout << "TCPFeeder.getServerPortFromPID: Error allocating memory (1)" << std::endl;
§§		return 0;
§§	}
§§
§§	unsigned long ulSize = sizeof(MIB_TCPTABLE);
§§	// Make an initial call to GetTcpTable2 to
§§	// get the necessary size into the ulSize variable
§§	if (GetTcpTable2(pTcpTable, &ulSize, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
§§		free(pTcpTable);
§§		pTcpTable = (MIB_TCPTABLE2*)malloc(ulSize);
§§		if (pTcpTable == NULL) {
§§			std::cout << "TCPFeeder.getServerPortFromPID: Error allocating memory (2)" << std::endl;
§§			return 0;
§§		}
§§	}
§§
§§	int re = 0;
§§	// Make a second call to GetTcpTable2 to get
§§	// the actual data we require
§§	if (GetTcpTable2(pTcpTable, &ulSize, TRUE) == NO_ERROR) {
§§		for (int i = 0; i < (int)pTcpTable->dwNumEntries; i++) {
§§			if (pTcpTable->table[i].dwState == MIB_TCP_STATE_LISTEN && pTcpTable->table[i].dwOwningPid == targetPID) {
§§				re = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
§§			}
§§		}
§§	}
§§	else {
§§		std::cout << "TCPFeeder.getServerPortFromPID:GetTcpTable2 failed." << std::endl;
§§		free(pTcpTable);
§§		return 0;
§§	}
§§
§§	if (pTcpTable != NULL) {
§§		free(pTcpTable);
§§		pTcpTable = NULL;
§§	}
§§
§§	return re;
§§#else
§§	std::string netstatOut = execCommandAndGetOutput("netstat -4 -lntp");
§§	std::vector<std::string> netstatLines = splitString(netstatOut, "\n");
§§	int i = 0;
§§	size_t pidPOS = std::string::npos;
§§	for (; i < netstatLines.size(); i++) {
§§		pidPOS = netstatLines[i].find("PID", 0);
§§		if (pidPOS != std::string::npos) {
§§			i++;
§§			break;
§§		}
§§	}
§§	for (; i < netstatLines.size(); i++) {
§§		try {
§§			int pid = std::stoi(netstatLines[i].substr(pidPOS, std::string::npos));
§§
§§			if (pid == targetPID) {
§§				size_t portPOS = netstatLines[i].find(":", 0);
§§				return std::stoi(netstatLines[i].substr(portPOS + 1, std::string::npos));
§§			}
§§		}
§§		catch (...) {
§§			//stoi failed
§§		}
§§	}
§§
§§	return 0;
§§#endif
§§}
§§
§§void initializeIPC(SharedMemIPC& sharedMemIPC_ToRunner)
§§{
§§	bool success = sharedMemIPC_ToRunner.initializeAsClient();
§§	std::cout << "TCPFeeder: initializeAsClient for runner: " << success << std::endl;
§§	if (!success) {
§§		exit(-1);
§§	}
§§}
§§
§§
§§
§§void initFromArgs(int argc, char* argv[], int& targetport, std::string& ipcName)
§§{
§§	if (argc < 2) {
§§		std::cout << "Usage: TCPFeeder.exe [port] <shared memory name>";
§§		exit(-1);
§§	}
§§#if defined(_WIN32) || defined(_WIN64)
§§	//When using GDB as target; We need to ignore Ctrl+C
§§	SetConsoleCtrlHandler(NULL, true);
§§	// Initialize Winsock
§§	WSADATA wsaData;
§§	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
§§	if (result != 0) {
§§		std::cout << "TCPFeeder: WSAStartup failed, terminating!" << std::endl;
§§		exit(-1);
§§	}
§§#endif
§§	if (argc == 3) {
§§		targetport = std::stoi(argv[1]);
§§	}
§§
§§	ipcName = std::string(argv[argc - 1]);
§§}
§§
§§uint16_t getTargetPID(SharedMemIPC& sharedMemIPC_ToRunner, int feederTimeoutMS)
§§{
§§	std::string targetPIDAsString;
§§	SharedMemMessage message_FromRunner;
§§	std::cout << message_FromRunner.getMessageType() << std::endl;
§§	sharedMemIPC_ToRunner.waitForNewMessageToClient(&message_FromRunner, feederTimeoutMS);
§§	if (message_FromRunner.getMessageType() == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
§§		std::cout << "TCPFeeder: Received a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message in getTargetPID, terminating!" << std::endl;
§§#if defined(_WIN32) || defined(_WIN64)
§§		WSACleanup();
§§#endif
§§		exit(-1);
§§	}
§§	else if (message_FromRunner.getMessageType() == SHARED_MEM_MESSAGE_TARGET_PID) {
§§		std::cout << "TCPFeeder: Recived a SHARED_MEM_MESSAGE_TARGET_PID message!" << std::endl;
§§		targetPIDAsString = std::string(message_FromRunner.getDataPointer(), message_FromRunner.getDataPointer() + message_FromRunner.getDataSize());
§§		std::cout << "TCPFeeder: target pid is \"" << targetPIDAsString << "\"" << std::endl;
§§	}
§§	else {
§§		std::cout << "TCPFeeder: Recived an unexpeced message, terminating!" << std::endl;
§§#if defined(_WIN32) || defined(_WIN64)
§§		WSACleanup();
§§#endif
§§		exit(-1);
§§	}
§§
§§	return std::stoi(targetPIDAsString);
§§}
§§
§§uint16_t getServerPortFromPIDOrTimeout(SharedMemIPC& sharedMemIPC_ToRunner, uint16_t targetPID, std::chrono::time_point<std::chrono::system_clock> timeLimit)
§§{
§§	uint16_t targetport = 0;
§§	while (targetport == 0 && std::chrono::system_clock::now() < timeLimit) {
§§		targetport = getServerPortFromPID(targetPID);
§§	}
§§	if (targetport == 0) {
§§		std::string errorMessage = "Server port of target(PID " + std::to_string(targetPID) + ") could not be identified!";
§§		std::cout << "TCPFeeder: " << errorMessage << std::endl;
§§		SharedMemMessage couldNotInitializeMsg(SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE, errorMessage.c_str(), (int)errorMessage.length());
§§		sharedMemIPC_ToRunner.sendMessageToServer(&couldNotInitializeMsg);
§§#if defined(_WIN32) || defined(_WIN64)
§§		WSACleanup();
§§#endif
§§		exit(-1);
§§	}
§§}
§§
§§void waitForPortOpenOrTimeout(SharedMemIPC& sharedMemIPC_ToRunner, std::string targethost, uint16_t targetport, std::chrono::time_point<std::chrono::system_clock> timeOfLastConnectAttempt)
§§{
§§	bool success = false;
§§	try
§§	{
§§		while (success == false && std::chrono::system_clock::now() < timeOfLastConnectAttempt) {
§§			success = isPortOpen(targethost, targetport);
§§		}
§§	}
§§	catch (const std::runtime_error& e) {
§§		std::cout << e.what() << std::endl;
§§	}
§§
§§	std::cout << "TCPFeeder: initializeAsClient for target " << success << std::endl;
§§
§§	if (!success) {
§§		std::string errorMessage = "It was not possible to connect to the target on port " + std::to_string(targetport) + "!";
§§		std::cout << "TCPFeeder: " << errorMessage << std::endl;
§§		SharedMemMessage couldNotInitializeMsg(SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE, errorMessage.c_str(), (int)errorMessage.length());
§§		sharedMemIPC_ToRunner.sendMessageToServer(&couldNotInitializeMsg);
§§#if defined(_WIN32) || defined(_WIN64)
§§		WSACleanup();
§§#endif
§§		exit(-1);
§§	}
§§}
§§
§§std::string getNewFuzzFileOrDie(SharedMemIPC& sharedMemIPC_ToRunner, int feederTimeoutMS)
§§{
§§	SharedMemMessage message_FromRunner;
§§	sharedMemIPC_ToRunner.waitForNewMessageToClient(&message_FromRunner, feederTimeoutMS);
§§	SharedMemMessageType messageFromRunnerType = message_FromRunner.getMessageType();
§§	if (messageFromRunnerType == SHARED_MEM_MESSAGE_FUZZ_FILENAME) {
§§		//std::cout << "TCPFeeder: Recived a SHARED_MEM_MESSAGE_FUZZ_FILENAME message :)" << std::endl;
§§		std::string fuzzFileName = std::string(message_FromRunner.getDataPointer(), message_FromRunner.getDataPointer() + message_FromRunner.getDataSize());
§§		//std::cout << "TCPFeeder: fuzzFileName " << fuzzFileName << std::endl;
§§
§§		return fuzzFileName;
§§	}
§§	else if (messageFromRunnerType == SHARED_MEM_MESSAGE_TERMINATE) {
§§		std::cout << "TCPFeeder: Recived a SHARED_MEM_MESSAGE_TERMINATE, terminating!" << std::endl;
§§#if defined(_WIN32) || defined(_WIN64)
§§		WSACleanup();
§§#endif
§§		exit(0);
§§	}
§§	else if (messageFromRunnerType == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
§§		std::cout << "TCPFeeder: Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message in getNewFuzzFileOrDie, terminating!" << std::endl;
§§#if defined(_WIN32) || defined(_WIN64)
§§		WSACleanup();
§§#endif
§§		exit(-1);
§§	}
§§	else {
§§		std::cout << "TCPFeeder: Recived an unexpeced message, terminating!" << std::endl;
§§#if defined(_WIN32) || defined(_WIN64)
§§		WSACleanup();
§§#endif
§§		exit(-1);
§§	}
§§}
