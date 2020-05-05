# Copyright 2017-2020 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including without
# limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
# SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# 
# Author(s): Abian Blome, Thomas Riedmaier

import ctypes
import os 
dir_path = os.path.dirname(os.path.realpath(__file__))

lib = ctypes.cdll.LoadLibrary(os.path.join(dir_path, 'SharedMemIPC.dll'))


class MessageTypes():
    SHARED_MEM_MESSAGE_TERMINATE = 1
    SHARED_MEM_MESSAGE_FUZZ_FILENAME = 2
    SHARED_MEM_MESSAGE_FUZZ_DONE = 3
    SHARED_MEM_MESSAGE_TARGET_PID = 5
    SHARED_MEM_MESSAGE_READY_TO_FUZZ = 6
    SHARED_MEM_MESSAGE_TARGET_CRASHED = 0x10
    SHARED_MEM_MESSAGE_NOT_INITIALIZED = 0x100
    SHARED_MEM_MESSAGE_TRANSMISSION_TIMEOUT = 0x101
    SHARED_MEM_MESSAGE_ERROR = 0x102
    SHARED_MEM_MESSAGE_FEEDER_COULD_NOT_INITIALIZE = 0x103
    SHARED_MEM_MESSAGE_WAIT_INTERRUPTED = 0x104
    SHARED_MEM_MESSAGE_TRANSMISSION_ERROR = 0x105

class SharedMemMessage(object):
    def _settypes(self):
        lib.SharedMemMessage_new1.argtypes = [ctypes.c_uint32, ctypes.c_char_p, ctypes.c_int]
        lib.SharedMemMessage_new1.restype = ctypes.c_void_p

        lib.SharedMemMessage_new2.argtypes = []
        lib.SharedMemMessage_new2.restype = ctypes.c_void_p

        lib.SharedMemMessage_delete.argtypes = [ctypes.c_void_p]
        lib.SharedMemMessage_delete.restype = ctypes.c_void_p

        lib.SharedMemMessage_getDataPointer.argtypes = [ctypes.c_void_p]
        lib.SharedMemMessage_getDataPointer.restype = ctypes.c_char_p

        lib.SharedMemMessage_getDataSize.argtypes = [ctypes.c_void_p]
        lib.SharedMemMessage_getDataSize.restype = ctypes.c_int

        lib.SharedMemMessage_replaceDataWith.argtypes = [ctypes.c_uint32, ctypes.c_char_p, ctypes.c_int]
        lib.SharedMemMessage_replaceDataWith.restype = ctypes.c_void_p

        lib.SharedMemMessage_getMessageType.argtypes = [ctypes.c_void_p]
        lib.SharedMemMessage_getMessageType.restype = ctypes.c_uint32

        lib.SharedMemMessage_setMessageType.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
        lib.SharedMemMessage_setMessageType.restype = ctypes.c_void_p

    def __initNoParams(self):
        self._settypes()
        

    def __init__(self, messageType=None, data=None):
        self._settypes()

        if messageType is None or data is None:
            self.obj = lib.SharedMemMessage_new2()
        else:
            cData = ctypes.create_string_buffer(data.encode('ascii'))
            self.obj = lib.SharedMemMessage_new1(messageType, cData, len(data)+1)

    def __del__(self):
        lib.SharedMemMessage_delete(self.obj)

    def getRawObject(self):
        return self.obj

    def getData(self):
        size = lib.SharedMemMessage_getDataSize(self.obj)
        cData = lib.SharedMemMessage_getDataPointer(self.obj)
        return ctypes.string_at(cData, size)

    def setData(self, data):
        cData = ctypes.create_string_buffer(data.encode('ascii'))
        lib.SharedMemMessage_replaceDataWith(self.obj, cData, len(data)+1)

    def getType(self):
        return lib.SharedMemMessage_getMessageType(self.obj)

    def setType(self, type):
        lib.SharedMemMessage_setMessageType(self.obj, type)


class SharedMemIPC(object):
    def __init__(self, sharedMemName, sharedMemorySize=0x1000):
        lib.SharedMemIPC_new.argtypes = [ctypes.c_char_p, ctypes.c_int]
        lib.SharedMemIPC_new.restype = ctypes.c_void_p

        lib.SharedMemIPC_delete.argtypes = [ctypes.c_void_p]
        lib.SharedMemIPC_delete.restype = ctypes.c_void_p

        lib.SharedMemIPC_initializeAsServer.argtypes = [ctypes.c_void_p]
        lib.SharedMemIPC_initializeAsServer.restype = ctypes.c_bool

        lib.SharedMemIPC_initializeAsClient.argtypes = [ctypes.c_void_p]
        lib.SharedMemIPC_initializeAsClient.restype = ctypes.c_bool

        lib.SharedMemIPC_sendMessageToServer.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        lib.SharedMemIPC_sendMessageToServer.restype = ctypes.c_bool

        lib.SharedMemIPC_sendMessageToClient.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        lib.SharedMemIPC_sendMessageToClient.restype = ctypes.c_bool

        lib.SharedMemIPC_waitForNewMessageToClient.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32]
        lib.SharedMemIPC_waitForNewMessageToClient.restype = ctypes.c_bool

        lib.SharedMemIPC_waitForNewMessageToServer.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32]
        lib.SharedMemIPC_waitForNewMessageToServer.restype = ctypes.c_bool

        cSharedMemName = ctypes.create_string_buffer(sharedMemName.encode('ascii'))

        self.obj = lib.SharedMemIPC_new(cSharedMemName, sharedMemorySize)

    def getRawObject(self):
        return self.obj

    def __del__(self):
        lib.SharedMemIPC_delete(self.obj)

    def initializeAsServer(self):
        return lib.SharedMemIPC_initializeAsServer(self.obj)

    def initializeAsClient(self):
        return lib.SharedMemIPC_initializeAsClient(self.obj)

    def sendMessageToServer(self, sharedMemMessage):
        return lib.SharedMemIPC_sendMessageToServer(self.obj, sharedMemMessage.getRawObject())

    def sendMessageToClient(self, sharedMemMessage):
        return lib.SharedMemIPC_sendMessageToClient(self.obj, sharedMemMessage.getRawObject())

    def waitForNewMessageToClient(self, timeoutMilliseconds):
        sharedMemMessage = SharedMemMessage()
        lib.SharedMemIPC_waitForNewMessageToClient(self.obj, sharedMemMessage.getRawObject(), timeoutMilliseconds)
        return sharedMemMessage

    def waitForNewMessageToServer(self, timeoutMilliseconds):
        sharedMemMessage = SharedMemMessage()
        lib.SharedMemIPC_waitForNewMessageToServer(self.obj, sharedMemMessage.getRawObject(), timeoutMilliseconds)
        return sharedMemMessage
