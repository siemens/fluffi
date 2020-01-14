/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Pascal Eckmann
*/

#include "stdafx.h"
#include "SharedMemIPC.h"
#include "ExternalProcess.h"
#include "Util.h"

INITIALIZE_EASYLOGGINGPP

//Taken from dynamorio
#if defined(_WIN64) || defined(_WIN32)
typedef UINT_PTR process_id_t; //mfapi.h
#else
typedef pid_t process_id_t; //globals_shared.h
#endif

//copied from FluffiTestExecutor::initializeFeeder. As this is not part of shared code but of TestcaseRunner, we cannot include this
bool initializeFeeder(std::shared_ptr<ExternalProcess> feederProcess, SharedMemIPC* sharedMemIPC_toFeeder, process_id_t targetProcessID, int timeoutMS) {
	std::chrono::time_point<std::chrono::system_clock> routineEntryTimeStamp = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> latestRoutineExitTimeStamp = routineEntryTimeStamp + std::chrono::milliseconds(timeoutMS);

	if (!feederProcess->initProcess()) {
		LOG(ERROR) << "Could not initialize the feeder process";
		return false;
	}

	if (!feederProcess->run()) {
		LOG(ERROR) << "Could not run the feeder process";
		return false;
	}

	std::string targetPIDString = std::to_string(targetProcessID);
	SharedMemMessage targetCMDMessage{ SHARED_MEM_MESSAGE_TARGET_PID, targetPIDString.c_str(), static_cast<int>(targetPIDString.length()) };

	if (!sharedMemIPC_toFeeder->sendMessageToClient(&targetCMDMessage)) {
		LOG(ERROR) << "Could not send the targetCMDMessage to the feeder process";
		return false;
	}

	SharedMemMessage responseFromFeeder;
	sharedMemIPC_toFeeder->waitForNewMessageToServer(&responseFromFeeder, static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(latestRoutineExitTimeStamp - std::chrono::system_clock::now()).count()));
	if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
		LOG(ERROR) << "Recived a SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT message: starting Feeder failed!";
		return false;
	}
	else if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_READY_TO_FUZZ) {
		LOG(DEBUG) << "Recived a SHARED_MEM_MESSAGE_READY_TO_FUZZ message: feeder is ready to fuzz!";

		return true;
	}
	else if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE) {
		LOG(DEBUG) << "Recived a SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE message: feeder could not initialize. Error message :" << std::string(responseFromFeeder.getDataPointer(), responseFromFeeder.getDataPointer() + responseFromFeeder.getDataSize());

		return false;
	}
	else {
		LOG(ERROR) << "Recived an unexpeced message: starting Feeder failed!";
		return false;
	}
}

int main(int argc, char* argv[])
{
	Util::setDefaultLogOptions("Tester.log");

	if (argc < 2) {
		LOG(INFO) << "Usage: Tester <parameters> ";
		LOG(INFO) << "Currently implemented parameters:";
		LOG(INFO) << "--feederCmdline: The cmdline to start the feeder (e.g. \"TCPFeeder.exe 5001\")";
		LOG(INFO) << "--targetCmdline: The cmdline to start the target (e.g. \"miniweb.exe ..\\..\\Test\\Win32\\System ..\\..\\Test\\WWWRoot\"). OPTIONAL";
		LOG(INFO) << "--testcaseFile: The file to send to the feeder";
		return -1;
	}

	/*
	std::string feederCmdline = "TCPFeeder.exe 5001";
	std::string targetCmdline = "C:\\Users\\z002wzkf\\Desktop\\Miniweb\\Win32\\bin\\miniweb.exe ..\\..\\Test\\Win32\\System ..\\..\\Test\\WWWRoot";
	std::string testcaseFile = "C:\\Users\\z002wzkf\\Desktop\\MWEB_JSON_Dll.dll+0x000187bd-MWEB_JSON_Dll.dll+0x000187bd";
	*/

	/*
	std::string feederCmdline = "SharedMemFeeder.exe";
	std::string targetCmdline = "D:\\git repos\\CSA\\FLUFFI\\research\\tom\\Showcases\\Release\\miniweb_4fuzz.exe FLUFFI_SharedMem_4Test_2";
	std::string testcaseFile = "C:\\Users\\z002wzkf\\Desktop\\MWEB_JSON_Dll.dll+0x000187bd-MWEB_JSON_Dll.dll+0x000187bd";
	*/

	/*
	std::string feederCmdline = "SharedMemFeeder";
	std::string targetCmdline = "boringssl-2016-02-12-fsanitize_fuzzer FLUFFI_SharedMem_4Test_2";
	std::string testcaseFile = "crash-ffb22c3101db1e53e38fdf630efd3dfd19cbeb84";
	*/

	/*
	std::string feederCmdline = "ShowcaseSipass 4343";
	std::string targetCmdline = "";
	std::string testcaseFile = argv[1];// "C:\\SipassHello.bin";
	*/

	std::string feederCmdline = "";
	std::string targetCmdline = "";
	std::string testcaseFile = "";
	std::string feederSharedMemName = "FLUFFI_SharedMem_4Test";

	for (int i = 1 /*skip program name*/; i < argc - 1 /*skip gdb file*/; i++) {
		if (std::string(argv[i]) == "--feederCmdline") {
			i++;
			feederCmdline = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--targetCmdline") {
			i++;
			targetCmdline = std::string(argv[i]);
		}
		if (std::string(argv[i]) == "--testcaseFile") {
			i++;
			testcaseFile = std::string(argv[i]);
		}
	}

	if (feederCmdline == "" || testcaseFile == "") {
		LOG(ERROR) << "Incomplete parameter specification";
		return -1;
	}

	LOG(INFO) << "feederCmdline: " << feederCmdline;
	LOG(INFO) << "targetCmdline: " << targetCmdline;
	LOG(INFO) << "testcaseFile: " << testcaseFile;

	SharedMemIPC sharedMemIPC_toFeeder(feederSharedMemName.c_str());
	if (!sharedMemIPC_toFeeder.initializeAsServer()) {
		LOG(ERROR) << "Could not create the shared memory ipc connection to talk to the feeder ";
		return -1;
	}

	std::shared_ptr<ExternalProcess> feederProcess = std::make_shared<ExternalProcess>(feederCmdline + " " + feederSharedMemName, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
	int targetPID = 0;
	if (targetCmdline != "") {
		ExternalProcess targetProcess(targetCmdline, ExternalProcess::CHILD_OUTPUT_TYPE::OUTPUT);
		targetProcess.initProcess();
		targetProcess.run();
		targetPID = targetProcess.getProcessID();
	}

	bool success = initializeFeeder(feederProcess, &sharedMemIPC_toFeeder, targetPID, 10000);
	if (!success) {
		LOG(ERROR) << "initializeFeeder failed! ";
		return -1;
	}

	//send "go" to feeder
	SharedMemMessage fuzzFilenameMsg{ SHARED_MEM_MESSAGE_FUZZ_FILENAME, testcaseFile.c_str(),static_cast<int>(testcaseFile.length()) };
	if (!sharedMemIPC_toFeeder.sendMessageToClient(&fuzzFilenameMsg)) {
		LOG(ERROR) << "Problem while sending a fuzzing filename to the feeder!";
		return -1;
	}

	//wait for "done" from feeder
	SharedMemMessage responseFromFeeder;
	sharedMemIPC_toFeeder.waitForNewMessageToServer(&responseFromFeeder, 10000);
	if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_FUZZ_DONE) {
		//feeder successfully forwarded the test case to the target, which responed in time
		LOG(INFO) << "Got FUZZ_DONE frome the feeder :) The setup appears to work!";
	}
	else if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_WAIT_INTERRUPTED) {
		LOG(ERROR) << "Communication with feeder was interrupted!";
		return -1;
	}
	else if (responseFromFeeder.getMessageType() == SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT) {
		LOG(ERROR) << "Communication with feeder timed out!";
		return -1;
	}
	else {
		LOG(ERROR) << "Communication with feeder yielded an error!";
		return -1;
	}

	return 0;
}
