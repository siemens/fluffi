/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

#include "stdafx.h"

int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("Usage: ProcessStarter <Process To start>");
		return -1;
	}

#if defined(_WIN32) || defined(_WIN64)
	std::stringstream oss;
	for (int i = 1; i < argc; i++) {
		oss << argv[i] << " ";
	}

	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));
	CreateProcess(NULL, const_cast<LPSTR>(oss.str().c_str()), NULL, NULL, false, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);

#else
	pid_t pID = fork();
	if (pID == 0) // child
	{
		setenv("LD_PRELOAD", "./dynamorio/lib64/release/libdynamorio.so ./dynamorio/lib64/release/libdrpreload.so", 1);
		execv(argv[1], &argv[1]);
		printf("Failed to call execv!");
		return -1;
	}
	else if (pID < 0) // failed to fork
	{
		printf("Failed to fork to new process!");
		return -1;
	}
	else // Code only executed by parent process
	{
	}

#endif

	//If you want to wait before returning until your target initialized use this line here :)
	//std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	return 0;
}