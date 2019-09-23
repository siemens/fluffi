§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Abian Blome
§§*/
§§
§§#include "stdafx.h"
§§#include "SharedMemIPC.h"
§§
§§/*
§§Memory Layout:
§§0
§§MAX_PATH  Windows: NameOfEventForNewMessageForClient Linux: NameOfFIFOForNewMessageForClien
§§UINT32 ForClient::MessageType
§§UINT32 ForClient::dataLength
§§UINT32 ForClient::data
§§
§§sharedMemorySize/2
§§MAX_PATH Windows: NameOfEventForNewMessageForServer Linux: NameOfFIFOForNewMessageForServer
§§UINT32 ForServer::MessageType
§§UINT32 ForServer::dataLength
§§UINT32 ForServer::data
§§
§§*/
§§
§§SharedMemIPC::SharedMemIPC(const char* sharedMemName, int sharedMemorySize) :
§§#if defined(_WIN32) || defined(_WIN64)
§§	m_hMapFile(NULL),
§§	m_pView(NULL),
§§	m_hEventForNewMessageForClient(NULL),
§§	m_hEventForNewMessageForServer(NULL),
§§#else
§§	m_fdSharedMem(-1),
§§	m_pView(reinterpret_cast<char*>(-1)),
§§	m_fdFIFOForNewMessageForClient(-1),
§§	m_fdFIFOForNewMessageForServer(-1),
§§#endif
§§	m_sharedMemName(new std::string(sharedMemName)),
§§	m_sharedMemorySize(sharedMemorySize)
§§{
§§}
§§
§§SharedMemIPC::~SharedMemIPC() {
§§#if defined(_WIN32) || defined(_WIN64)
§§
§§	if (m_hEventForNewMessageForServer != NULL)
§§	{
§§		// Close the event handle for the server
§§		CloseHandle(m_hEventForNewMessageForServer);
§§		m_hEventForNewMessageForServer = NULL;
§§	}
§§
§§	if (m_hEventForNewMessageForClient != NULL)
§§	{
§§		// Close the event handle for the client
§§		CloseHandle(m_hEventForNewMessageForClient);
§§		m_hEventForNewMessageForClient = NULL;
§§	}
§§
§§	if (m_pView != NULL)
§§	{
§§		// Unmap the file view.
§§		UnmapViewOfFile(m_pView);
§§		m_pView = NULL;
§§	}
§§
§§	if (m_hMapFile != NULL)
§§	{
§§		// Close the file mapping object.
§§		CloseHandle(m_hMapFile);
§§		m_hMapFile = NULL;
§§	}
§§
§§#else
§§	//Close the fifo that signals new messages for the server
§§	if (m_fdFIFOForNewMessageForServer != -1) {
§§		close(m_fdFIFOForNewMessageForServer);
§§		m_fdFIFOForNewMessageForServer = -1;
§§	}
§§
§§	//Close the fifo that signals new messages for the client
§§	if (m_fdFIFOForNewMessageForClient != -1) {
§§		close(m_fdFIFOForNewMessageForClient);
§§		m_fdFIFOForNewMessageForClient = -1;
§§	}
§§
§§	if (m_pView != reinterpret_cast<char*>(-1))
§§	{
§§		//Remove the fifo that signals new messages for the server
§§		unlink(m_pView + (m_sharedMemorySize / 2));
§§
§§		//Remove the fifo that signals new messages for the client
§§		unlink(m_pView);
§§
§§		// Unmap the file view.
§§		munmap(m_pView, m_sharedMemorySize);
§§	}
§§
§§	if (m_fdSharedMem != -1)
§§	{
§§		// Close the file descriptor for the shared memory
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§	}
§§
§§	//try to unlink the shared memory file
§§	shm_unlink(m_sharedMemName->c_str());
§§
§§#endif
§§
§§	delete m_sharedMemName;
§§}
§§
§§bool SharedMemIPC::initializeAsServer() {
§§	//at the very least we need enough space to place the two guids, twice the message type, twice the message length and a byte of data
§§	if (m_sharedMemorySize <= 2 * (MAX_PATH + 9)) {
§§		printf("SMIPC: initializeAsServer:m_sharedMemorySize is too small\n");
§§		return false;
§§	}
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§
§§	// Create the file mapping object.
§§	m_hMapFile = CreateFileMapping(
§§		INVALID_HANDLE_VALUE,   // Use paging file - shared memory
§§		NULL,                   // Default security attributes
§§		PAGE_READWRITE,         // Allow read and write access
§§		0,                      // High-order DWORD of file mapping max size
§§		m_sharedMemorySize,               // Low-order DWORD of file mapping max size
§§		m_sharedMemName->c_str()           // Name of the file mapping object
§§	);
§§	if (m_hMapFile == NULL)
§§	{
§§		printf("SMIPC: initializeAsServer:CreateFileMapping failed\n");
§§		return false;
§§	}
§§
§§	m_pView = (char*)MapViewOfFile(
§§		m_hMapFile,               // Handle of the map object
§§		FILE_MAP_ALL_ACCESS,    // Read and write access
§§		0,                      // High-order DWORD of the file offset
§§		NULL,            // Low-order DWORD of the file offset
§§		NULL               // The number of bytes to map to view
§§	);
§§	if (m_pView == NULL)
§§	{
§§		CloseHandle(m_hMapFile);
§§		m_hMapFile = NULL;
§§		printf("SMIPC: initializeAsServer:MapViewOfFile failed\n");
§§		return false;
§§	}
§§
§§	//Zero the newly created shared memory
§§	memset(m_pView, 0, m_sharedMemorySize);
§§
§§	//Create the event that signals new messages for the client
§§	std::string nameOfEventForNewMessageForClient = "FLUFFI_SharedMem_Event_" + newGUID();
§§	memcpy_s(m_pView, MAX_PATH, nameOfEventForNewMessageForClient.c_str(), nameOfEventForNewMessageForClient.length());
§§	m_hEventForNewMessageForClient = CreateEvent(NULL, false, false, m_pView);
§§	if (m_hEventForNewMessageForClient == NULL)
§§	{
§§		UnmapViewOfFile(m_pView);
§§		m_pView = NULL;
§§		CloseHandle(m_hMapFile);
§§		m_hMapFile = NULL;
§§		printf("SMIPC: initializeAsServer:CreateEvent 1 failed\n");
§§		return false;
§§	}
§§
§§	//Create the event that signals new messages for the server
§§	std::string nameOfEventForNewMessageForServer = "FLUFFI_SharedMem_Event_" + newGUID();
§§	memcpy_s(m_pView + (m_sharedMemorySize / 2), MAX_PATH, nameOfEventForNewMessageForServer.c_str(), nameOfEventForNewMessageForServer.length());
§§	m_hEventForNewMessageForServer = CreateEvent(NULL, false, false, m_pView + (m_sharedMemorySize / 2));
§§	if (m_hEventForNewMessageForServer == NULL)
§§	{
§§		CloseHandle(m_hEventForNewMessageForClient);
§§		m_hEventForNewMessageForClient = NULL;
§§		UnmapViewOfFile(m_pView);
§§		m_pView = NULL;
§§		CloseHandle(m_hMapFile);
§§		m_hMapFile = NULL;
§§		printf("SMIPC: initializeAsServer:CreateEvent 2 failed\n");
§§		return false;
§§	}
§§
§§	return true;
§§
§§#else
§§	//create shared memory
§§	m_fdSharedMem = shm_open(m_sharedMemName->c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
§§	if (m_fdSharedMem == -1)
§§	{
§§		printf("SMIPC: initializeAsServer:shm_open failed: %d (%s)\n", errno, m_sharedMemName->c_str());
§§		return false;
§§	}
§§
§§	//prune shared memory to m_sharedMemorySize
§§	if (ftruncate(m_fdSharedMem, m_sharedMemorySize) == -1) {
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		shm_unlink(m_sharedMemName->c_str());
§§		printf("SMIPC: initializeAsServer:ftruncate failed\n");
§§		return false;
§§	}
§§
§§	//map shared memory into process
§§	m_pView = static_cast<char*>(mmap(NULL, m_sharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fdSharedMem, 0));
§§	if (m_pView == reinterpret_cast<char*>(-1))
§§	{
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		shm_unlink(m_sharedMemName->c_str());
§§		printf("SMIPC: initializeAsServer:mmap failed\n");
§§		return false;
§§	}
§§
§§	//Zero the newly created shared memory
§§	memset(m_pView, 0, m_sharedMemorySize);
§§
§§	//Create the fifo that signals new messages for the client
§§	std::string nameOfFIFOForNewMessageForClient = "/tmp/FLUFFI_SharedMem_FIFO_" + newGUID();
§§	memcpy_s(m_pView, MAX_PATH, nameOfFIFOForNewMessageForClient.c_str(), static_cast<int>(nameOfFIFOForNewMessageForClient.length()));
§§	if (mkfifo(m_pView, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == -1) {
§§		munmap(m_pView, m_sharedMemorySize);
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		shm_unlink(m_sharedMemName->c_str());
§§		printf("SMIPC: initializeAsServer:mkfifo 1 failed\n");
§§		return false;
§§	}
§§
§§	//Create the fifo that signals new messages for the server
§§	std::string nameOfFIFOForNewMessageForServer = "/tmp/FLUFFI_SharedMem_FIFO_" + newGUID();
§§	memcpy_s(m_pView + (m_sharedMemorySize / 2), MAX_PATH, nameOfFIFOForNewMessageForServer.c_str(), static_cast<int>(nameOfFIFOForNewMessageForServer.length()));
§§	if (mkfifo(m_pView + (m_sharedMemorySize / 2), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == -1) {
§§		unlink(m_pView);
§§		munmap(m_pView, m_sharedMemorySize);
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		shm_unlink(m_sharedMemName->c_str());
§§		printf("SMIPC: initializeAsServer:mkfifo 2 failed\n");
§§		return false;
§§	}
§§
§§	//Open the fifo that signals new messages for the client
§§	m_fdFIFOForNewMessageForClient = open(m_pView, O_RDWR | O_NONBLOCK);// writeonly fails if there is not yet somebody on the other end
§§	if (m_fdFIFOForNewMessageForClient == -1) {
§§		unlink(m_pView + (m_sharedMemorySize / 2));
§§		unlink(m_pView);
§§		munmap(m_pView, m_sharedMemorySize);
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		shm_unlink(m_sharedMemName->c_str());
§§		printf("SMIPC: initializeAsServer:open 1 failed\n");
§§		return false;
§§	}
§§
§§	//Open the fifo that signals new messages for the server
§§	m_fdFIFOForNewMessageForServer = open(m_pView + (m_sharedMemorySize / 2), O_RDONLY | O_NONBLOCK);
§§	if (m_fdFIFOForNewMessageForServer == -1) {
§§		close(m_fdFIFOForNewMessageForClient);
§§		m_fdFIFOForNewMessageForClient = -1;
§§		unlink(m_pView + (m_sharedMemorySize / 2));
§§		unlink(m_pView);
§§		munmap(m_pView, m_sharedMemorySize);
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		shm_unlink(m_sharedMemName->c_str());
§§		printf("SMIPC: initializeAsServer:open 2 failed\n");
§§		return false;
§§	}
§§
§§	return true;
§§
§§#endif
§§}
§§bool SharedMemIPC::initializeAsClient() {
§§#if defined(_WIN32) || defined(_WIN64)
§§
§§	// Try to open the named file mapping identified by the map name.
§§	m_hMapFile = OpenFileMapping(
§§		FILE_MAP_WRITE,         // Request read and write access
§§		FALSE,                  // Do not inherit the name
§§		m_sharedMemName->c_str()           // File mapping name
§§	);
§§	if (m_hMapFile == NULL)
§§	{
§§		printf("SMIPC: initializeAsClient:OpenFileMapping failed\n");
§§		return false;
§§	}
§§
§§	m_pView = (char*)MapViewOfFile(
§§		m_hMapFile,               // Handle of the map object
§§		FILE_MAP_ALL_ACCESS,    // Read and write access
§§		0,                      // High-order DWORD of the file offset
§§		NULL,            // Low-order DWORD of the file offset
§§		NULL               // The number of bytes to map to view
§§	);
§§	if (m_pView == NULL)
§§	{
§§		CloseHandle(m_hMapFile);
§§		m_hMapFile = NULL;
§§		printf("SMIPC: initializeAsClient:MapViewOfFile failed\n");
§§		return false;
§§	}
§§
§§	m_hEventForNewMessageForClient = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, false, m_pView);
§§	if (m_hEventForNewMessageForClient == NULL)
§§	{
§§		UnmapViewOfFile(m_pView);
§§		m_pView = NULL;
§§		CloseHandle(m_hMapFile);
§§		m_hMapFile = NULL;
§§		printf("SMIPC: initializeAsClient:OpenEvent 1 failed\n");
§§		return false;
§§	}
§§
§§	m_hEventForNewMessageForServer = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, false, m_pView + (m_sharedMemorySize / 2));
§§	if (m_hEventForNewMessageForServer == NULL)
§§	{
§§		CloseHandle(m_hEventForNewMessageForClient);
§§		m_hEventForNewMessageForClient = NULL;
§§		UnmapViewOfFile(m_pView);
§§		m_pView = NULL;
§§		CloseHandle(m_hMapFile);
§§		m_hMapFile = NULL;
§§		printf("SMIPC: initializeAsClient:OpenEvent 2 failed\n");
§§		return false;
§§	}
§§
§§	return true;
§§
§§#else
§§
§§	//open shared memory
§§	m_fdSharedMem = shm_open(m_sharedMemName->c_str(), O_RDWR, 0);
§§	if (m_fdSharedMem == -1)
§§	{
§§		printf("SMIPC: initializeAsClient:shm_open failed: %d (%s)\n", errno, m_sharedMemName->c_str());
§§		return false;
§§	}
§§
§§	//prune shared memory to m_sharedMemorySize
§§	if (ftruncate(m_fdSharedMem, m_sharedMemorySize) == -1) {
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		printf("SMIPC: initializeAsClient:ftruncate failed\n");
§§		return false;
§§	}
§§
§§	//map shared memory into process
§§	m_pView = static_cast<char*>(mmap(NULL, m_sharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fdSharedMem, 0));
§§	if (m_pView == reinterpret_cast<char*>(-1))
§§	{
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		printf("SMIPC: initializeAsClient:mmap failed\n");
§§		return false;
§§	}
§§
§§	//Open the fifo that signals new messages for the client
§§	m_fdFIFOForNewMessageForClient = open(m_pView, O_RDONLY | O_NONBLOCK);
§§	if (m_fdFIFOForNewMessageForClient == -1) {
§§		munmap(m_pView, m_sharedMemorySize);
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		printf("SMIPC: initializeAsClient:open 1 failed\n");
§§		return false;
§§	}
§§
§§	//Open the fifo that signals new messages for the server
§§	m_fdFIFOForNewMessageForServer = open(m_pView + (m_sharedMemorySize / 2), O_WRONLY | O_NONBLOCK);
§§	if (m_fdFIFOForNewMessageForServer == -1) {
§§		close(m_fdFIFOForNewMessageForClient);
§§		m_fdFIFOForNewMessageForClient = -1;
§§		munmap(m_pView, m_sharedMemorySize);
§§		close(m_fdSharedMem);
§§		m_fdSharedMem = -1;
§§		printf("SMIPC: initializeAsClient:open 2 failed\n");
§§		return false;
§§	}
§§
§§	return true;
§§
§§#endif
§§}
§§
§§bool SharedMemIPC::notifyClientAboutNewMessage() {
§§#if defined(_WIN32) || defined(_WIN64)
§§
§§	return SetEvent(m_hEventForNewMessageForClient) != 0;
§§
§§#else
§§	const char* buff = "M";
§§	return write(m_fdFIFOForNewMessageForClient, buff, 1) != -1;
§§#endif
§§}
§§bool SharedMemIPC::notifyServerAboutNewMessage() {
§§#if defined(_WIN32) || defined(_WIN64)
§§
§§	return SetEvent(m_hEventForNewMessageForServer) != 0;
§§
§§#else
§§	const char* buff = "M";
§§	return write(m_fdFIFOForNewMessageForServer, buff, 1) != -1;
§§#endif
§§}
§§
§§bool SharedMemIPC::placeMessageForClient(const SharedMemMessage* message) {
§§	*(reinterpret_cast<uint32_t*>(m_pView + MAX_PATH)) = message->getMessageType();
§§	*(reinterpret_cast<uint32_t*>(m_pView + MAX_PATH + 4)) = static_cast<uint32_t>(message->getDataSize());
§§
§§	if (message->getDataSize() == 0) {
§§		return true;
§§	}
§§	if (message->getDataSize() >= (m_sharedMemorySize / 2) - (MAX_PATH + 8)) {
§§		return false;
§§	}
§§
§§	return 0 == memcpy_s((m_pView + MAX_PATH + 8), (m_sharedMemorySize / 2) - (MAX_PATH + 8), message->getDataPointer(), message->getDataSize());
§§}
§§bool SharedMemIPC::placeMessageForServer(const SharedMemMessage* message) {
§§	*(reinterpret_cast<uint32_t*>(m_pView + (m_sharedMemorySize / 2) + MAX_PATH)) = message->getMessageType();
§§	*(reinterpret_cast<uint32_t*>(m_pView + (m_sharedMemorySize / 2) + MAX_PATH + 4)) = static_cast<uint32_t>(message->getDataSize());
§§
§§	if (message->getDataSize() == 0) {
§§		return true;
§§	}
§§	if (message->getDataSize() >= (m_sharedMemorySize / 2) - (MAX_PATH + 8)) {
§§		return false;
§§	}
§§	return 0 == memcpy_s((m_pView + (m_sharedMemorySize / 2) + MAX_PATH + 8), (m_sharedMemorySize / 2) - (MAX_PATH + 8), message->getDataPointer(), message->getDataSize());
§§}
§§
§§bool SharedMemIPC::sendMessageToServer(const SharedMemMessage* message) {
§§	return placeMessageForServer(message) && notifyServerAboutNewMessage();
§§}
§§bool SharedMemIPC::sendMessageToClient(const SharedMemMessage* message) {
§§	return placeMessageForClient(message) && notifyClientAboutNewMessage();
§§}
§§
§§bool SharedMemIPC::waitForNewMessageToClient(SharedMemMessage* messagePointer, unsigned long timeoutMilliseconds) {
§§#if defined(_WIN32) || defined(_WIN64)
§§	return waitForNewMessageToClient(messagePointer, timeoutMilliseconds, NULL);
§§#else
§§	return waitForNewMessageToClient(messagePointer, timeoutMilliseconds, -1);
§§#endif
§§}
§§
§§bool SharedMemIPC::waitForNewMessageToServer(SharedMemMessage* messagePointer, unsigned long timeoutMilliseconds) {
§§#if defined(_WIN32) || defined(_WIN64)
§§	return waitForNewMessageToServer(messagePointer, timeoutMilliseconds, NULL);
§§#else
§§	return waitForNewMessageToServer(messagePointer, timeoutMilliseconds, -1);
§§#endif
§§}
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§
§§bool SharedMemIPC::waitForNewMessageToClient(SharedMemMessage* messagePointer, unsigned long timeoutMilliseconds, HANDLE interruptEventHandle) {
§§	if (interruptEventHandle == NULL) {
§§		DWORD success = WaitForSingleObject(m_hEventForNewMessageForClient, timeoutMilliseconds);
§§		if (success != WAIT_OBJECT_0) {
§§			messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT);
§§			messagePointer->replaceDataWith(nullptr, 0);
§§			return false;
§§		}
§§	}
§§	else {
§§		HANDLE hEvent[2];
§§		hEvent[0] = m_hEventForNewMessageForClient;
§§		hEvent[1] = interruptEventHandle;
§§
§§		DWORD dwRet = WaitForMultipleObjects(2, hEvent, false, timeoutMilliseconds);
§§		if (dwRet == WAIT_OBJECT_0) {
§§			//a message was received
§§		}
§§		else if (dwRet == WAIT_OBJECT_0 + 1) {
§§			messagePointer->setMessageType(SHARED_MEM_MESSAGE_WAIT_INTERRUPTED);
§§			messagePointer->replaceDataWith(nullptr, 0);
§§			return false;
§§		}
§§		else {
§§			messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT);
§§			messagePointer->replaceDataWith(nullptr, 0);
§§			return false;
§§		}
§§	}
§§
§§	SharedMemMessageType messageType = *(SharedMemMessageType*)(m_pView + MAX_PATH);
§§	uint32_t dataLength = *(uint32_t*)(m_pView + MAX_PATH + 4);
§§
§§	messagePointer->setMessageType(messageType);
§§	messagePointer->replaceDataWith((m_pView + MAX_PATH + 8), dataLength);
§§	return true;
§§}
§§
§§bool SharedMemIPC::waitForNewMessageToServer(SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds, HANDLE interruptEventHandle) {
§§	if (interruptEventHandle == NULL) {
§§		DWORD success = WaitForSingleObject(m_hEventForNewMessageForServer, timeoutMilliseconds);
§§		if (success != WAIT_OBJECT_0) {
§§			messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT);
§§			messagePointer->replaceDataWith(nullptr, 0);
§§			return false;
§§		}
§§	}
§§	else {
§§		HANDLE hEvent[2];
§§		hEvent[0] = m_hEventForNewMessageForServer;
§§		hEvent[1] = interruptEventHandle;
§§
§§		DWORD dwRet = WaitForMultipleObjects(2, hEvent, false, timeoutMilliseconds);
§§		if (dwRet == WAIT_OBJECT_0) {
§§			//a message was received
§§		}
§§		else if (dwRet == WAIT_OBJECT_0 + 1) {
§§			messagePointer->setMessageType(SHARED_MEM_MESSAGE_WAIT_INTERRUPTED);
§§			messagePointer->replaceDataWith(nullptr, 0);
§§			return false;
§§		}
§§		else {
§§			messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT);
§§			messagePointer->replaceDataWith(nullptr, 0);
§§			return false;
§§		}
§§	}
§§
§§	SharedMemMessageType messageType = *(SharedMemMessageType*)(m_pView + (m_sharedMemorySize / 2) + MAX_PATH);
§§	uint32_t dataLength = *(uint32_t*)(m_pView + (m_sharedMemorySize / 2) + MAX_PATH + 4);
§§
§§	messagePointer->setMessageType(messageType);
§§	messagePointer->replaceDataWith((m_pView + (m_sharedMemorySize / 2) + MAX_PATH + 8), dataLength);
§§	return true;
§§}
§§
§§#else
§§
§§bool SharedMemIPC::waitForNewMessageToClient(SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds, int interruptFD) {
§§	fd_set readfds;
§§	FD_ZERO(&readfds);
§§	FD_SET(m_fdFIFOForNewMessageForClient, &readfds);
§§
§§	if (interruptFD != -1) {
§§		FD_SET(interruptFD, &readfds);
§§	}
§§
§§	struct timespec timeout;
§§	timeout.tv_sec = timeoutMilliseconds / 1000;
§§	timeout.tv_nsec = (timeoutMilliseconds % 1000) * 1000000;
§§
§§	int res = -1;
§§	while (true) {
§§		res = pselect(std::max(interruptFD, m_fdFIFOForNewMessageForClient) + 1, &readfds, NULL, NULL, &timeout, NULL);
§§		//check for error
§§		if (res == -1) {
§§			//pselect returned an error
§§			if (errno == EINTR) {
§§				//the error is a signal. This is usually a dynamorio nudge. In that case: continue
§§				continue;
§§			}
§§			//the error is something else
§§			messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_ERROR);
§§			messagePointer->replaceDataWith(nullptr, 0);
§§			return false;
§§		}
§§		else {
§§			//there was no error
§§			break;
§§		}
§§	}
§§
§§	//check for timeout
§§	if (res == 0) {
§§		messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT);
§§		messagePointer->replaceDataWith(nullptr, 0);
§§		return false;
§§	}
§§
§§	//check for interrupt
§§	if (interruptFD != -1 && FD_ISSET(interruptFD, &readfds)) {
§§		//clear the interrupt pipe
§§		int nbytes = 0;
§§		ioctl(interruptFD, FIONREAD, &nbytes);
§§		char* buff = new char[nbytes];
§§		ssize_t readChars = read(interruptFD, buff, nbytes);
§§		if (readChars != nbytes) {
§§			printf("SMIPC: Failed to cleanup interruptFD\n");
§§		}
§§		delete[] buff;
§§
§§		messagePointer->setMessageType(SHARED_MEM_MESSAGE_WAIT_INTERRUPTED);
§§		messagePointer->replaceDataWith(nullptr, 0);
§§		return false;
§§	}
§§
§§	//check for error again ;)
§§	if (!FD_ISSET(m_fdFIFOForNewMessageForClient, &readfds)) {
§§		messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_ERROR);
§§		messagePointer->replaceDataWith(nullptr, 0);
§§		return false;
§§	}
§§
§§	//looks like we got a message!
§§
§§	//clear the m_fdFIFOForNewMessageForClient pipe
§§	{
§§		int nbytes = 0;
§§		ioctl(m_fdFIFOForNewMessageForClient, FIONREAD, &nbytes);
§§		char* buff = new char[nbytes];
§§		ssize_t readChars = read(m_fdFIFOForNewMessageForClient, buff, nbytes);
§§		if (readChars != nbytes) {
§§			printf("SMIPC: Failed to cleanup m_fdFIFOForNewMessageForClient\n");
§§		}
§§		delete[] buff;
§§	}
§§
§§	SharedMemMessageType messageType = *(reinterpret_cast<SharedMemMessageType*>(m_pView + MAX_PATH));
§§	uint32_t dataLength = *(reinterpret_cast<uint32_t*>(m_pView + MAX_PATH + 4));
§§
§§	messagePointer->setMessageType(messageType);
§§	messagePointer->replaceDataWith((m_pView + MAX_PATH + 8), dataLength);
§§	return true;
§§}
§§
§§bool SharedMemIPC::waitForNewMessageToServer(SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds, int interruptFD) {
§§	fd_set readfds;
§§	FD_ZERO(&readfds);
§§	FD_SET(m_fdFIFOForNewMessageForServer, &readfds);
§§
§§	if (interruptFD != -1) {
§§		FD_SET(interruptFD, &readfds);
§§	}
§§
§§	struct timespec timeout;
§§	timeout.tv_sec = timeoutMilliseconds / 1000;
§§	timeout.tv_nsec = (timeoutMilliseconds % 1000) * 1000000;
§§
§§	int res = -1;
§§	while (true) {
§§		res = pselect(std::max(interruptFD, m_fdFIFOForNewMessageForServer) + 1, &readfds, NULL, NULL, &timeout, NULL);
§§		//check for error
§§		if (res == -1) {
§§			//pselect returned an error
§§			if (errno == EINTR) {
§§				//the error is a signal. This is usually a dynamorio nudge. In that case: continue
§§				continue;
§§			}
§§			//the error is something else
§§			messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_ERROR);
§§			messagePointer->replaceDataWith(nullptr, 0);
§§			return false;
§§		}
§§		else {
§§			//there was no error
§§			break;
§§		}
§§	}
§§
§§	//check for timeout
§§	if (res == 0) {
§§		messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT);
§§		messagePointer->replaceDataWith(nullptr, 0);
§§		return false;
§§	}
§§
§§	//check for interrupt
§§	if (interruptFD != -1 && FD_ISSET(interruptFD, &readfds)) {
§§		//clear the interrupt pipe
§§		int nbytes = 0;
§§		ioctl(interruptFD, FIONREAD, &nbytes);
§§		char* buff = new char[nbytes];
§§		ssize_t readChars = read(interruptFD, buff, nbytes);
§§		if (readChars != nbytes) {
§§			printf("SMIPC: Failed to cleanup interruptFD\n");
§§		}
§§		delete[] buff;
§§
§§		messagePointer->setMessageType(SHARED_MEM_MESSAGE_WAIT_INTERRUPTED);
§§		messagePointer->replaceDataWith(nullptr, 0);
§§		return false;
§§	}
§§
§§	//check for error again ;)
§§	if (!FD_ISSET(m_fdFIFOForNewMessageForServer, &readfds)) {
§§		messagePointer->setMessageType(SHARED_MEM_MESSAGE_TRANSMISSION_ERROR);
§§		messagePointer->replaceDataWith(nullptr, 0);
§§		return false;
§§	}
§§
§§	//looks like we got a message!
§§
§§	//clear the m_fdFIFOForNewMessageForServer pipe
§§	{
§§		int nbytes = 0;
§§		ioctl(m_fdFIFOForNewMessageForServer, FIONREAD, &nbytes);
§§		char* buff = new char[nbytes];
§§		ssize_t readChars = read(m_fdFIFOForNewMessageForServer, buff, nbytes);
§§		if (readChars != nbytes) {
§§			printf("SMIPC: Failed to cleanup m_fdFIFOForNewMessageForServer\n");
§§		}
§§		delete[] buff;
§§	}
§§
§§	SharedMemMessageType messageType = *(reinterpret_cast<SharedMemMessageType*>(m_pView + (m_sharedMemorySize / 2) + MAX_PATH));
§§	uint32_t dataLength = *(reinterpret_cast<uint32_t*>(m_pView + (m_sharedMemorySize / 2) + MAX_PATH + 4));
§§
§§	messagePointer->setMessageType(messageType);
§§	messagePointer->replaceDataWith((m_pView + (m_sharedMemorySize / 2) + MAX_PATH + 8), dataLength);
§§	return true;
§§}
§§
§§#endif
§§
§§std::string SharedMemIPC::newGUID() {
§§#if defined(_WIN32) || defined(_WIN64)
§§
§§	UUID uuid;
§§	RPC_STATUS success = UuidCreate(&uuid);
§§
§§	unsigned char* str;
§§	success = UuidToStringA(&uuid, &str);
§§
§§	std::string s((char*)str);
§§
§§	success = RpcStringFreeA(&str);
§§
§§	return s;
§§
§§#else
§§
§§	uuid_t uuid;
§§	uuid_generate_random(uuid);
§§	char s[37];
§§	uuid_unparse(uuid, s);
§§
§§	return s;
§§
§§#endif
§§}
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§#else
§§int SharedMemIPC::memcpy_s(void* a, size_t  b, const void* c, size_t d) {
§§	return ((memcpy(a, c, std::min(b, d)) == a) ? 0 : -1);
§§}
§§#endif
§§
§§//C wrapper
§§SharedMemIPC* SharedMemIPC_new(const char* sharedMemName, int sharedMemorySize) { return new SharedMemIPC(sharedMemName, sharedMemorySize); }
§§void SharedMemIPC_delete(SharedMemIPC* thisp) { delete thisp; }
§§bool SharedMemIPC_initializeAsServer(SharedMemIPC* thisp) { return thisp->initializeAsServer(); }
§§bool SharedMemIPC_initializeAsClient(SharedMemIPC* thisp) { return thisp->initializeAsClient(); }
§§
§§bool SharedMemIPC_sendMessageToServer(SharedMemIPC* thisp, const SharedMemMessage* message) { return thisp->sendMessageToServer(message); }
§§bool SharedMemIPC_sendMessageToClient(SharedMemIPC* thisp, const SharedMemMessage* message) { return  thisp->sendMessageToClient(message); }
§§
§§bool SharedMemIPC_waitForNewMessageToClient(SharedMemIPC* thisp, SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds) { return thisp->waitForNewMessageToClient(messagePointer, timeoutMilliseconds); }
§§bool SharedMemIPC_waitForNewMessageToServer(SharedMemIPC* thisp, SharedMemMessage* messagePointer, unsigned long  timeoutMilliseconds) { return thisp->waitForNewMessageToServer(messagePointer, timeoutMilliseconds); }
