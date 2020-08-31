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

Author(s): Abian Blome, Thomas Riedmaier
*/

#include "stdafx.h"

std::string loggingFuncBytes("\x73\x7D\x02\x00\x06\x0A\x06\x02\x7D\xBA\x01\x00\x04\x06\x03\x7D\xBB\x01\x00\x04\x06\x06\xFE\x06\x7E\x02\x00\x06\x73\x19\x01\x00\x0A\x7D\xBC\x01\x00\x04\x02\x7B\xAC\x00\x00\x04\x28\x2C\x01\x00\x0A\x6F\x29\x01\x00\x0A\x2E\x19\x02\x7B\xAB\x00\x00\x04\x06\xFE\x06\x7F\x02\x00\x06\x73\x2D\x01\x00\x0A\x14\x6F\x2E\x01\x00\x0A\x2A\x06\x7B\xBC\x01\x00\x04\x6F\x2F\x01\x00\x0A\x2A", 0x5d);
std::string loggingFuncDisabledBytes("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x2A", 0x5d);

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
	execCommandAsync(windbg.generic_string() + " -d -k \"com:pipe,resets=0,reconnect,port=\\\\.\\pipe\\" + pipeName + "\" -c \".load " + kfuzzWindbg.string() + "\"");

	std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));

	return 0;
}
int  search(std::istream_iterator<char>  first, std::istream_iterator<char>  last, std::string::iterator  s_first, std::string::iterator  s_last)
{
	int current_position = -1;

	for (; ; ++first) {
		current_position++;
		std::istream_iterator<char>  it = first;

		for (std::string::iterator s_it = s_first; ; ++it, ++s_it) {
			if (s_it == s_last) {
				return current_position - std::distance(s_first, s_last);
			}
			if (it == last) {
				return -1;
			}
			if (!(*it == *s_it)) {
				break;
			}
			current_position++;
		}
	}
}

bool doesFileContainPattern(std::experimental::filesystem::path filename, std::string pattern) {
	//Get an input stream for the filename
	std::ifstream ifs(filename.c_str(), std::ios_base::binary);

	if (!ifs) {
		std::cout << "failed to open file";
		return false;
	}

	ifs.unsetf(std::ios::skipws);

	// begin = start of file
	// end = end of file
	std::istream_iterator<char> begin(ifs), end;

	// search file for string
	if (search(begin, end, pattern.begin(), pattern.end()) != -1)
	{
		//The pattern is present
		return true;
	}

	//The pattern is missing
	return false;
}

bool isWindbgLoggingDisabled(std::string pathToWinDBGPrev) {
	std::experimental::filesystem::path windbgExePath(pathToWinDBGPrev);

	return doesFileContainPattern(windbgExePath.parent_path().append("DbgX.Services.dll"), loggingFuncDisabledBytes);
}

bool disableWindbgLogging(std::string pathToWinDBGPrev) {
	std::experimental::filesystem::path windbgExePath(pathToWinDBGPrev);

	//Check if the file can be patched
	bool canWePatchThisFile = doesFileContainPattern(windbgExePath.parent_path().append("DbgX.Services.dll"), loggingFuncBytes);
	if (!canWePatchThisFile) {
		std::cout << "We cannot disable the logging, as the DbgX.Services.dll does not contain the expected pattern." << std::endl;
		return false;
	}

	//Create a backup
	try
	{
		std::experimental::filesystem::copy(windbgExePath.parent_path().append("DbgX.Services.dll"), windbgExePath.parent_path().append("DbgX.Services.dll.orig"), std::experimental::filesystem::copy_options::skip_existing);
	}
	catch (...) {
		std::cout << "We cannot disable the logging, as the WinDBG directory does not allow creating files." << std::endl;
		return false;
	}

	//Patch the DbgX.Services.dll
	{
		std::fstream fs(windbgExePath.parent_path().append("DbgX.Services.dll").c_str(), std::fstream::in | std::fstream::out | std::fstream::binary);
		if (!fs) {
			std::cout << "We cannot disable the logging, as the DbgX.Services.dll could not be opened for writing." << std::endl;
			return false;
		}
		fs.unsetf(std::ios::skipws);
		std::istream_iterator<char> begin(fs), end;
		std::iostream::pos_type position_of_data_to_overwrite = search(begin, end, loggingFuncBytes.begin(), loggingFuncBytes.end()); //We already checked that the pattern is there
		fs.seekp(position_of_data_to_overwrite, std::ios_base::beg);
		fs.write(loggingFuncDisabledBytes.c_str(), loggingFuncDisabledBytes.size());
		fs.flush();
		fs.close();
	}

	//Check if we succeeded
	bool didPatchSucceed = !doesFileContainPattern(windbgExePath.parent_path().append("DbgX.Services.dll"), loggingFuncBytes) && doesFileContainPattern(windbgExePath.parent_path().append("DbgX.Services.dll"), loggingFuncDisabledBytes);
	if (!canWePatchThisFile) {
		std::cout << "We cannot disable the logging as patching the DbgX.Services.dll failed" << std::endl;
		return false;
	}

	return true;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cout << "Usage: kFuzzStarter <parameters> <initgdb file>" << std::endl;
		std::cout << "Currently implemented parameters:" << std::endl;
		std::cout << "--startVM <Path to vmx file>: start a virtual machine" << std::endl;
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
			i++;
			pipeName = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--startVM") {
			i++;
			nameOfVM = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--kfuzzWindbg") {
			i++;
			kfuzzWindbg = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--vmwarePath") {
			i++;
			pathToVMWare = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--vmmonPath") {
			i++;
			vmmonPath = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--winDbgPrevPath") {
			i++;
			winDbgPrevPath = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--local") {
			i++;
			pathToLocalExecutableAndArgs = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--sleep") {
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

	if (!isWindbgLoggingDisabled(winDbgPrevPath)) {
		std::cout << "WinDBG logging is not yet disabled. We need to do so in order to prevent memory resource exhaustion." << std::endl;
		if (!disableWindbgLogging(winDbgPrevPath)) {
			std::cout << "Failed to disable WinDBG logging." << std::endl;
			return -1;
		}
		std::cout << "Successfully disabled WinDBG logging." << std::endl;
	}

	startvm(nameOfVM, pathToVMWare, additionalCommands, restorecurrent, snapshotName, vmmonPath, winDbgPrevPath, pipeName, kfuzzWindbg, gdbInitFile, sleepMS);

	if (!pathToLocalExecutableAndArgs.empty()) {
		execCommandAsync(pathToLocalExecutableAndArgs);
	}

	return 0;
}
