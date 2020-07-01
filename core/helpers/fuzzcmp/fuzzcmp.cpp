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


#include "stdafx.h"
#include "fuzzcmp.h"
#include "trampoline.h"

#if defined(_WIN32) || defined(_WIN64)

void installIATHook(std::vector<std::tuple<std::string, size_t>> replacements) {
	HMODULE hMods[1024];
	DWORD cbNeeded;
	HANDLE hProcess = GetCurrentProcess();
	TCHAR szPath[MAX_PATH];
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
	{
		for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			HMODULE  currModule = hMods[i];
			PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)currModule;
			PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((size_t)currModule + pDosHeader->e_lfanew);

			GetModuleFileName(hMods[i], szPath, MAX_PATH);
			//printf("module %s\n", szPath);

			//Are there any imports?
			if (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != 0) {
				//Iterate over modules
				PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((char*)currModule + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
				while (pImportDescriptor->Name != NULL)
				{
					char * libraryName = (char *)currModule + pImportDescriptor->Name;
					//printf("library %s\n", libraryName);

					//Iterate over functions
					PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)((char *)currModule + pImportDescriptor->FirstThunk);
					PIMAGE_THUNK_DATA pIATNames = (PIMAGE_THUNK_DATA)((char *)currModule + pImportDescriptor->OriginalFirstThunk);
					while (pIAT->u1.AddressOfData != NULL) {
						//If the function is imported by ordinal, there is no function name
						if ((size_t)((char *)currModule + pIATNames->u1.Ordinal) & IMAGE_ORDINAL_FLAG)
						{
							//printf("function imported by ordinal\n");
						}
						else {
							char * funcName = ((PIMAGE_IMPORT_BY_NAME)((char *)currModule + pIATNames->u1.AddressOfData))->Name;
							PSIZE_T funcAddr = (PSIZE_T)&(pIAT->u1.AddressOfData);
							//printf("function %s -> 0x%X\n", funcName, funcAddr);

							for (std::vector<std::tuple<std::string, size_t>>::iterator it = replacements.begin(); it != replacements.end(); ++it) {
								if (strcmp(funcName, std::get<0>(*it).c_str()) == 0) {
									printf("Redirecting %s: %s->%s from 0x%llX to 0x%llX\n", szPath, libraryName, funcName, (unsigned long long)*funcAddr, (unsigned long long)std::get<1>(*it));

									DWORD originalPermissions = 0;
									VirtualProtect(funcAddr, sizeof(size_t), PAGE_EXECUTE_READWRITE, &originalPermissions);
									pIAT->u1.AddressOfData = std::get<1>(*it);
									VirtualProtect(funcAddr, sizeof(size_t), originalPermissions, 0);
								}
							}
						}

						pIAT++;
						pIATNames++;
					}

					pImportDescriptor++;
				}
			}
		}
	}
	CloseHandle(hProcess);
}

void installIATHooks() {
	printf("Installing API Hooks\n");

	std::vector<std::tuple<std::string, size_t>> replacements;
	replacements.push_back(std::make_tuple("strcmp", (size_t)&mystrcmp));
	replacements.push_back(std::make_tuple("memcmp", (size_t)&mymemcmp));
	replacements.push_back(std::make_tuple("_stricmp", (size_t)&my_stricmp));
	replacements.push_back(std::make_tuple("strcmpi", (size_t)&mystrcmpi));
	replacements.push_back(std::make_tuple("stricmp", (size_t)&mystricmp));
	installIATHook(replacements);
	printf("Done installing API Hooks\n");
}

size_t addrToRVA(size_t addr) {
	HMODULE hMods[1024];
	DWORD cbNeeded;
	HANDLE curProcess = GetCurrentProcess();

	if (EnumProcessModules(curProcess, hMods, sizeof(hMods), &cbNeeded) != 0)
	{
		for (unsigned long i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			MODULEINFO mi;
			if (GetModuleInformation(curProcess, hMods[i], &mi, sizeof(mi))) {
				if (((size_t)(mi.lpBaseOfDll) < addr) && ((size_t)(mi.lpBaseOfDll) + mi.SizeOfImage > addr)) {
					return addr - (size_t)(mi.lpBaseOfDll);
				}
			}
			else {
				printf("GetModuleInformation failed.");
			}
		}

		//apparently, the address is not part of any loaded module
		return addr;
	}
	else {
		printf("EnumProcessModules failed.");
	}

	return 0;
}

#else

std::vector<std::string> splitString(std::string str, const char token) {
	if (str.empty())        return std::vector<std::string> { "" };

	std::vector<std::string>result;
	while (str.size()) {
		size_t index = str.find(token);
		if (index != std::string::npos) {
			result.push_back(str.substr(0, index));
			str = str.substr(index + 1);
			if (str.size() == 0)result.push_back(str);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}

size_t addrToRVA(std::uintptr_t addr) {
	size_t re = UINT_MAX;
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen("/proc/self/maps", "r");
	if (fp == NULL) {
		printf("Failed opening /proc/self/maps for reading");
	}

	while ((read = getline(&line, &len, fp)) != -1)
	{
		std::vector<std::string>  lineElements = splitString(line, ' ');
		if (lineElements.size() < 2) {
			printf("Splitting line %s failed", line);;
			continue;
		}
		std::vector<std::string>  addresses = splitString(lineElements[0], '-');
		if (addresses.size() < 2) {
			printf("Splitting addresses %s failed", lineElements[0].c_str());
			continue;
		}
		if (stoull(addresses[0], 0, 16) < addr && stoull(addresses[1], 0, 16) > addr)
		{
			re = addr - stoull(addresses[0], 0, 16);
			break;
		}
	}

	fclose(fp);
	if (line != NULL) {
		free(line);
	}

	if (re != UINT_MAX) {
		return re;
	}
	else {
		//apparently, the address is not part of any loaded module
		return addr;
	}
}

#endif
