/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Pascal Eckmann
*/

#include "stdafx.h"

int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("Usage: ServiceStarter <Name and Path of the Service binary to start>");
		return -1;
	}

	std::string sserviceBinary = argv[1];
	std::wstring wserviceBinary = std::wstring(sserviceBinary.begin(), sserviceBinary.end());

	SC_HANDLE hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
	if (hSCManager == 0) {
		printf("couldn't open service manager.\n");
		return -1;
	}

	DWORD bytesNeeded = 0;
	DWORD numOfServices = 0;
	DWORD resumeHandle = 0;
	BOOL success = EnumServicesStatus(hSCManager, SERVICE_WIN32_OWN_PROCESS, SERVICE_STATE_ALL, NULL, 0, &bytesNeeded, &numOfServices, &resumeHandle);
	if (success != FALSE) {
		printf("EnumServicesStatus did not fail: %d.\n", GetLastError());
		return -1;
	}

§§	char* serviceBuf = new char[bytesNeeded];
	DWORD bytesNeeded2 = 0;
	numOfServices = 0;
	resumeHandle = 0;
§§	success = EnumServicesStatus(hSCManager, SERVICE_WIN32_OWN_PROCESS, SERVICE_STATE_ALL, (ENUM_SERVICE_STATUS*)serviceBuf, bytesNeeded, &bytesNeeded2, &numOfServices, &resumeHandle);
	if (success == FALSE) {
		printf("EnumServicesStatus failed: %d.\n", GetLastError());
		return -1;
	}

	//Search for the target service
	for (DWORD i = 0; i < numOfServices; i++) {
§§		printf("\nService %s \n", ((ENUM_SERVICE_STATUS*)serviceBuf)[i].lpServiceName);
§§		SC_HANDLE hService = OpenService(hSCManager, ((ENUM_SERVICE_STATUS*)serviceBuf)[i].lpServiceName, SERVICE_START | SERVICE_STOP | SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
		if (hService == NULL) {
			continue;
		}

		DWORD bytesNeededForQuery = 0;
		success = QueryServiceConfig(hService, NULL, 0, &bytesNeededForQuery);

§§		char* serviceConfig = new char[bytesNeededForQuery];
		DWORD bytesNeededForQuery2 = 0;
		success = QueryServiceConfig(hService, (LPQUERY_SERVICE_CONFIG)serviceConfig, bytesNeededForQuery, &bytesNeededForQuery2);

		std::string scmdline = ((LPQUERY_SERVICE_CONFIG)serviceConfig)->lpBinaryPathName;
		std::wstring wcmdline = std::wstring(scmdline.begin(), scmdline.end());
		int numOfBlocks;
§§		LPWSTR* szArglist = CommandLineToArgvW(wcmdline.c_str(), &numOfBlocks); //for some reasons this does not exist for Ascii :(
		if (NULL != szArglist && numOfBlocks >= 1) {
			wprintf(L"Found service binary %s\n", szArglist[0]);
			if (std::wstring(szArglist[0]) == wserviceBinary) {
				printf("This is the service you want to start!\n");

				//Check if service is already running
				SERVICE_STATUS_PROCESS ssp;
				DWORD dwBytesNeeded;
				if (0 != QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
				{
					if (ssp.dwCurrentState != SERVICE_STOPPED)
					{
						printf("Service is NOT stopped. Trying to stop it.\n");
						if (!ControlService(hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp))
						{
							printf("ControlService failed (%d)\n", GetLastError());
						}

						//Wait until the service is stopped
						while (true) {
							SERVICE_STATUS_PROCESS ssp;
							DWORD dwBytesNeeded;
							if (0 != QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
							{
								if (ssp.dwCurrentState == SERVICE_STOPPED)
								{
									printf("Service is now stopped.\n");
									break;
								}
								else {
									printf("Service is not stopped yet: %d\n", ssp.dwCurrentState);
									std::this_thread::sleep_for(std::chrono::milliseconds(100));
								}
							}
							else {
								printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
							}
						}
					}
				}
				else {
					printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
				}

				//Actualy start the service
§§				LPCSTR* args4Start = new LPCSTR[argc - 1];
§§				args4Start[0] = ((ENUM_SERVICE_STATUS*)serviceBuf)[i].lpServiceName;
				for (int j = 2; j < argc; j++) {
					args4Start[j - 1] = argv[j];
				}

				printf("Trying to start the service.\n");
				StartService(hService, argc - 1, args4Start);

				delete args4Start;

				//Wait until the service is running
				while (true) {
					SERVICE_STATUS_PROCESS ssp;
					DWORD dwBytesNeeded;
					if (0 != QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
					{
						if (ssp.dwCurrentState == SERVICE_RUNNING)
						{
							printf("Service is Started.\n");
							break;
						}
						else {
							printf("Service is not started yet: %d\n", ssp.dwCurrentState);
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
						}
					}
					else {
						printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
					}
				}

				LocalFree(szArglist);
				delete serviceConfig;
				CloseServiceHandle(hService);
				break;
			}
		}
		LocalFree(szArglist);
		delete serviceConfig;
		CloseServiceHandle(hService);
	}
	delete serviceBuf;

	CloseServiceHandle(hSCManager);

	//If you want to wait before returning until your target initialized use this line here :)
	//std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	return 0;
}
