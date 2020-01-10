/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier
*/

#include "stdafx.h"

void execCommand(std::string command) {
	printf("executing command %s\n", command.c_str());

	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));
	CreateProcess(NULL, const_cast<LPSTR>(command.c_str()), NULL, NULL, false, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);

	// Successfully created the process.  Wait for it to finish.
	WaitForSingleObject(pi.hProcess, INFINITE);
}

void execCommandAsync(std::string command) {
	printf("executing command %s asynchronously\n", command.c_str());

	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));
	CreateProcess(NULL, const_cast<LPSTR>(command.c_str()), NULL, NULL, false, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
}

int startvm(
	std::string nameOfVM,
	std::string pathToVMWare,
	std::string additionalCommands,
	bool restorecurrent,
	std::string snapshotName,
	std::string vmmonPath,
	std::string pathToWinDBGPrev,
	std::string pipeName,
	std::string pathTokfuzzWindbg,
	std::string gdbInitFile,
	int sleepMS
)
{
	if (nameOfVM == "") {
		std::cout << "nameOfVM needs to be specified" << std::endl;
		return -1;
	}

	if (!std::experimental::filesystem::exists(pathToVMWare)) {
		std::cout << "pathToVMWare needs to exist" << std::endl;
		return -1;
	}

	int waittime = 1000;
	int waittimeWinDbg = 6000;
	std::experimental::filesystem::path kfuzzWindbg(pathTokfuzzWindbg);
	std::experimental::filesystem::path windbg(pathToWinDBGPrev);
	std::experimental::filesystem::path vmmon(vmmonPath);
	std::experimental::filesystem::path vmrun(pathToVMWare);
	vmrun /= "vmrun.exe";

	//Write startup commands to the "gdb" initialization file (if any)
	std::ofstream initfile;
	initfile.open(gdbInitFile, std::ios::out);
	if (!initfile.is_open()) {
		std::cout << "could not write to the gdb initialization file" << std::endl;
		return -1;
	}
	initfile.close();

	// Kill old instance of vmmon
	execCommand("taskkill /f /im \"" + vmmon.filename().generic_string() + "\"");
	std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

	// Kill old instance of windbg
	execCommand("taskkill /f /im \"" + windbg.filename().generic_string() + "\"");
	std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

	// Execute vmmon
	execCommandAsync(vmmon.generic_string());
	std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

	//Switch off vm (if already running)
	execCommand(vmrun.generic_string() + " stop \"" + nameOfVM + "\" hard");
	std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

	//Restore the current snapshot (if desired)
	if (restorecurrent) {
		//Sleep a little bit
		std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

		execCommand(vmrun.generic_string() + " revertToSnapshot \"" + nameOfVM + "\" \"" + snapshotName + "\"");
	}

	//Sleep a little bit
	std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

	//Start vm
	execCommand(vmrun.generic_string() + " start \"" + nameOfVM + "\"");
	std::this_thread::sleep_for(std::chrono::milliseconds(waittime));

	//Sleep a little bit
	std::this_thread::sleep_for(std::chrono::milliseconds(waittimeWinDbg));

	//Start winDBG
	execCommandAsync(windbg.generic_string() + " -k \"com:pipe,resets=0,reconnect,port=\\\\.\\pipe\\" + pipeName + "\" -c \".load " + kfuzzWindbg.generic_string() + "\"");

	std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << "Usage: kFuzzStarter <parameters> <initgdb file>" << std::endl;
		std::cout << "Currently implemented parameters:" << std::endl;
		std::cout << "--startVM <Name of vm>: start a virtual machine" << std::endl;
		std::cout << "--pipeName <Name of pipe>: Name of the vkd pipe" << std::endl;
		std::cout << "--kfuzzWindbg <Path to kfuzz-windbg.dll>: Path to kfuzz-windbg.dll" << std::endl;
		std::cout << "--winDbgPrevPath <Path to WinDBG Preview>: Path to WinDBG Preview. Default: \"C:\\windbg\\DbgX.Shell.exe\"" << std::endl;
		std::cout << "--vmmonPath <Path to VirtualKD>: Path to VirtualKD vmmon. Default: \"D:\\Siemens\\Tools\\VirtualKD-3.0\\vmmon64.exe\"" << std::endl;
		std::cout << "--vmwarePath <Path to VMWare>: Path to VBoxManage. Default: \"C:\\Program Files (x86)\\VMware\\VMware Workstation\"" << std::endl;
		std::cout << "--restoreSnapshot <name>: When using vmware, you might want to restore the current snapshot when starting the virtual machine" << std::endl;
		std::cout << "--sleep <milliseconds>: sleep before returning. Default: 0" << std::endl;
		std::cout << "--local <process to start and args>: start a process locally" << std::endl;
		return -1;
	}

	bool startVM = false;
	std::string nameOfVM = "";
	std::string pathToLocalExecutableAndArgs = "";
	int sleepMS = 0;
	std::string additionalCommands = "";
	bool restorecurrent = false;
	std::string snapshotName = "";
	std::string pipeName = "";
	std::string kfuzzWindbg = "";
	std::string pathToVMWare = "C:\\Program Files (x86)\\VMware\\VMware Workstation";
	std::string vmmonPath = "D:\\Siemens\\Tools\\VirtualKD-3.0\\vmmon64.exe";
	std::string winDbgPrevPath = "C:\\windbg\\DbgX.Shell.exe";
	std::string gdbInitFile = argv[argc - 1];

	for (int i = 1 /*skip program name*/; i < argc - 1; i++) {
		if (std::string(argv[i]) == "--pipeName") {
			startVM = true;
			i++;
			pipeName = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--startVM") {
			startVM = true;
			i++;
			nameOfVM = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--kfuzzWindbg") {
			startVM = true;
			i++;
			kfuzzWindbg = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--vmwarePath") {
			startVM = true;
			i++;
			pathToVMWare = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--vmmonPath") {
			startVM = true;
			i++;
			vmmonPath = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--winDbgPrevPath") {
			startVM = true;
			i++;
			winDbgPrevPath = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--local") {
			i++;
			pathToLocalExecutableAndArgs = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--sleep") {
			startVM = true;
			i++;
			sleepMS = std::stoi(std::string(argv[i]));
		}
		if (std::string(argv[i]) == "--additionalCommands") {
			i++;
			additionalCommands = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--restoreSnapshot") {
			restorecurrent = true;
			i++;
			snapshotName = std::string(argv[i]);
		}
	}

	if (startVM) {
		startvm(nameOfVM, pathToVMWare, additionalCommands, restorecurrent, snapshotName, vmmonPath, winDbgPrevPath, pipeName, kfuzzWindbg, gdbInitFile, sleepMS);
	}
	if (!pathToLocalExecutableAndArgs.empty()) {
		execCommandAsync(pathToLocalExecutableAndArgs);
	}

	return 0;
}
