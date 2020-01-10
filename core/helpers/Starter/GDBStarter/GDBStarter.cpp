/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Pascal Eckmann
*/

//g++ --std=c++11 -I ../../../dependencies/base64/include -o GDBStarter GDBStarter.cpp ../../../dependencies/base64/lib/x86-64/base64.a -lstdc++fs

#include "stdafx.h"
#include "base64.h"

// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

#if defined(_WIN32) || defined(_WIN64)
std::string wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}
#else
char** split_commandline(const std::string cmdline)
{
	char** argv = nullptr;

	wordexp_t p;

	// Note! This expands shell variables.
	int success = wordexp(cmdline.c_str(), &p, WRDE_NOCMD);
	if (success != 0)
	{
		std::cout << "wordexp \"" << cmdline << "\" failed: " << success;
		return nullptr;
	}

	if (!(argv = static_cast<char**>(calloc(p.we_wordc + 1, sizeof(char*)))))
	{
		wordfree(&p);
		std::cout << "calloc failed";
		return nullptr;
	}

	for (size_t i = 0; i < p.we_wordc; i++)
	{
		argv[i] = strdup(p.we_wordv[i]);
	}
	argv[p.we_wordc] = NULL;

	wordfree(&p);

	return argv;
}
#endif

void execCommand(std::string command) {
	printf("executing command %s\n", command.c_str());

#if defined(_WIN32) || defined(_WIN64)
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));
	CreateProcess(NULL, const_cast<LPSTR>(command.c_str()), NULL, NULL, false, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);

	// Successfully created the process. Wait for it to finish.
	WaitForSingleObject(pi.hProcess, INFINITE);

#else

	pid_t pID = fork();
	if (pID == 0) // child
	{
		char** argv = split_commandline(command);

		execv(argv[0], &argv[1]);
		printf("Failed to call execv!");
		return;
	}
	else if (pID < 0) // failed to fork
	{
		printf("Failed to fork to new process!");
		return;
	}
	else // Code only executed by parent process
	{
		int status = 0;
		waitpid(pID, &status, 0);
	}

#endif
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

int startvbox(std::string nameOfVbox, std::string pathToVboxManage, std::string additionalCommands, bool restorecurrent, std::string gdbInitFile, int sleepMS) {
	if (nameOfVbox == "") {
		std::cout << "vboxManagePath needs to be specified" << std::endl;
		return -1;
	}

	if (!std::experimental::filesystem::exists(pathToVboxManage)) {
		std::cout << "pathToVboxManage needs to exist" << std::endl;
		return -1;
	}

	int waittime = 1000;

	//Switch off vm (if already running)
	execCommand(std::string(pathToVboxManage + " controlvm " + nameOfVbox + " poweroff"));

	//Sleep a little bit
	std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

	//Unlock vm (if locked)
	execCommand(std::string(pathToVboxManage + " startvm " + nameOfVbox + " --type emergencystop"));

	//Restore the current snapshot (if desired)
	if (restorecurrent) {
		//Sleep a little bit
		std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

		execCommand(std::string(pathToVboxManage + " snapshot  " + nameOfVbox + " restorecurrent "));
	}

	//Sleep a little bit
	std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

	//Start vm
	execCommand(std::string(pathToVboxManage + " startvm " + nameOfVbox));

	//Write startup commands to the gdb initialization file
	std::ofstream initfile;
	initfile.open(gdbInitFile, std::ios::out);
	if (!initfile.is_open()) {
		std::cout << "could not write to the gdb initialization file" << std::endl;
		return -1;
	}

	//initfile << "file C:\\\\FLUFFI\\\\Morpheus_Mweb\\\\s7p_cpu1500_morpheus_x86_64_standard_config.elf" << std::endl;
	//initfile << "set architecture i386:x86-64" << std::endl;
	//initfile << "target remote :4444" << std::endl;
	initfile << additionalCommands << std::endl;
	initfile.close();

	//Sleep before returning
	std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));

	return 0;
}

int startlocal(std::string pathToLocalExecutableAndArgs, std::string additionalCommands, std::string gdbInitFile, int sleepMS) {
	if (pathToLocalExecutableAndArgs == "") {
		std::cout << "pathToLocalExecutableAndArgs needs to be specified" << std::endl;
		return -1;
	}

	//Write startup commands to the gdb initialization file
	std::ofstream initfile;
	initfile.open(gdbInitFile, std::ios::out);
	if (!initfile.is_open()) {
		std::cout << "could not write to the gdb initialization file" << std::endl;
		return -1;
	}

#if defined(_WIN32) || defined(_WIN64)
	std::wstring wpathToLocalExecutableAndArgs = std::wstring(pathToLocalExecutableAndArgs.begin(), pathToLocalExecutableAndArgs.end());
	int numOfBlocks;
	LPWSTR* szArglist = CommandLineToArgvW(wpathToLocalExecutableAndArgs.c_str(), &numOfBlocks); //for some reasons this does not exist for Ascii :(
	if (NULL == szArglist || numOfBlocks < 1) {
		std::cout << "pathToLocalExecutableAndArgs command line invalid";
		return -1;
	}

	if (!std::experimental::filesystem::exists(szArglist[0])) {
		LocalFree(szArglist);
		std::cout << "pathToLocalExecutableAndArgs executable file does not exist";
		return -1;
	}

	std::string exFile = wstring_to_utf8(szArglist[0]);
	replaceAll(exFile, "\\", "\\\\");
	initfile << "file \"" << exFile << "\"" << std::endl;
	initfile << "set args ";
	for (int i = 1; i < numOfBlocks; i++) {
		initfile << wstring_to_utf8(szArglist[i]) << " ";
	}

	initfile << additionalCommands << std::endl;
	initfile << std::endl;

	LocalFree(szArglist);
#else
	std::string tmp = pathToLocalExecutableAndArgs;
	char** argv = split_commandline(tmp);
	if (argv == NULL || argv[0] == NULL) {
		std::cout << "Splitting pathToLocalExecutableAndArgs \"" << tmp << "\" failed.";
		if (argv != NULL) {
			free(argv);
		}
		return -1;
	}

	if (!std::experimental::filesystem::exists(argv[0])) {
		//free argv
		int i = 0;
		while (argv[i] != NULL) {
			free(argv[i]);
			i++;
		}
		free(argv);
		std::cout << "pathToLocalExecutableAndArgs executable not found";
		return -1;
	}

	initfile << "file \"" << argv[0] << "\"" << std::endl;
	initfile << "set args ";
	int i = 1;
	while (argv[i] != NULL) {
		initfile << argv[i] << " ";
	}

	initfile << additionalCommands << std::endl;
	initfile << std::endl;

	//free argv
	i = 0;
	while (argv[i] != NULL) {
		free(argv[i]);
		i++;
	}
	free(argv);
#endif

	initfile.close();

	//Sleep before returning
	std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << "Usage: GDBStarter <parameters> <gdb initialization file>" << std::endl;
		std::cout << "Currently implemented parameters:" << std::endl;
		std::cout << "--startVbox <Name of vbox>: start a virtual box" << std::endl;
		std::cout << "--vboxManagePath <Path to VBoxManage>: Path to VBoxManage Default: \"C:\\Program Files\\Oracle\\VirtualBox\\VBoxManage.exe\"(win) \"VBoxManage\"(lin)" << std::endl;
		std::cout << "--restoreCurrentSnapshot: When using virtual box, you might want to restore the current snapshot when starting the virtual machine" << std::endl;
		std::cout << "--sleep <milliseconds>: sleep before returning. Default: 0" << std::endl;
		std::cout << "--local <process to start and args>: start a process locally" << std::endl;
		std::cout << "--additionalCommands: Base64 encoded gdb commands that will be appended to the gdb initialization file" << std::endl;
		return -1;
	}

	bool startVbox = false;
	std::string nameOfVbox = "";
	bool startLocal = false;
	std::string pathToLocalExecutableAndArgs = "";
	int sleepMS = 0;
	std::string additionalCommands = "";
	bool restorecurrent = false;
#if defined(_WIN32) || defined(_WIN64)
	std::string pathToVboxManage = "C:\\Program Files\\Oracle\\VirtualBox\\VBoxManage.exe";
#else
	std::string pathToVboxManage = "VBoxManage";
#endif

	for (int i = 1 /*skip program name*/; i < argc - 1 /*skip gdb file*/; i++) {
		if (std::string(argv[i]) == "--startVbox") {
			startVbox = true;
			i++;
			nameOfVbox = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--vboxManagePath") {
			startVbox = true;
			i++;
			pathToVboxManage = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--local") {
			startLocal = true;
			i++;
			pathToLocalExecutableAndArgs = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--sleep") {
			startVbox = true;
			i++;
			sleepMS = std::stoi(std::string(argv[i]));
		}
		if (std::string(argv[i]) == "--additionalCommands") {
			i++;
			additionalCommands = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--restoreCurrentSnapshot") {
			restorecurrent = true;
		}
	}

	additionalCommands = base64_decode(additionalCommands);
	trim(additionalCommands);

	if (startVbox) {
		return startvbox(nameOfVbox, pathToVboxManage, additionalCommands, restorecurrent, argv[argc - 1], sleepMS);
	}
	else if (startLocal) {
		return startlocal(pathToLocalExecutableAndArgs, additionalCommands, argv[argc - 1], sleepMS);
	}

	std::cout << "I don't know what to do " << std::endl;
	return -1;
}
