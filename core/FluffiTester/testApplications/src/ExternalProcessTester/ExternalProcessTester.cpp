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

Author(s): Thomas Riedmaier
*/

// g++ --std=c++11 ExternalProcessTester.cpp -o ExternalProcessTester

#include "stdafx.h"

int waitForever() {
	HANDLE dummyevent = CreateEvent(NULL, TRUE, FALSE, "");
	WaitForSingleObject(dummyevent, INFINITE); //wait forever
	CloseHandle(dummyevent);

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		return -1;
	}

	int whattoDo = std::stoi(argv[1]);

	switch (whattoDo)
	{
	case 0: //do nothing
		break;
	case 1: // get main thread id
	{
		std::thread t1(waitForever);
		std::thread t2(waitForever);
		std::thread t3(waitForever);
		FILE* f;
		f = fopen("out.txt", "w");
		fprintf(f, "%d\r\n", GetCurrentThreadId());
		fclose(f);

		t1.join();//will block. Kill is needed
		t2.join();
		t3.join();
	}
	break;
	case 2: // get proc id
	{
		FILE* f;
		f = fopen("out.txt", "w");
		fprintf(f, "%d\r\n", GetCurrentProcessId());
		fclose(f);
		waitForever();//will block. Kill is needed
	}
	case 3: //sleep
		std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000));
		break;
	case 4: //sleep, then access violation
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000));
		int * test = (int *)0x12345678;
		*test = 0x11223344;
	}
	break;
	case 5: //create another thread that sleeps and then caues an access violation
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		CreateProcess(NULL, "ExternalProcessTester.exe 4", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(si.hStdInput);
		CloseHandle(si.hStdOutput);
		CloseHandle(si.hStdError);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	break;

	default:
		break;
	}

	return 0;
}
