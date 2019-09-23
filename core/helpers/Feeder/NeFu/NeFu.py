§§# Copyright 2017-2019 Siemens AG
§§# 
§§# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§# 
§§# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§# 
§§# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§# 
§§# Author(s): Abian Blome, Thomas Riedmaier
§§
§§import sys
§§import os
§§import socket
§§import time
§§
§§from SharedMemIPC import SharedMemIPC, SharedMemMessage, MessageTypes
§§
§§def sendTestcase(filename):
§§    try:
§§        with open(filename, "r") as f:
§§            data = f.read()
§§
§§        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
§§        s.connect(("192.168.150.135", 4444))
§§        s.sendall(data)
§§        s.close()
§§        time.sleep(0.1)
§§        return True
§§    except:
§§        time.sleep(0.1)
§§        return False
§§
§§if len(sys.argv) == 1:
§§    print("Usage: %s <shared memory name>")
§§    sys.exit(1)
§§
§§sharedMemName = sys.argv[1]
§§print("Using shared memory: %s" % sharedMemName)
§§
§§timeout = 10000
§§
§§# Step 1: Init
§§ipc = SharedMemIPC(sharedMemName)
§§ipc.initializeAsClient()
§§
§§# Step 2: Receive PID msg
§§message = ipc.waitForNewMessageToClient(timeout)
§§if not message.getType() == MessageTypes.SHARED_MEM_MESSAGE_TARGET_PID:
§§    print("Received unexpected message of type 0x%x, exiting" % message.getType())
§§    sys.exit(1)
§§
§§pid = int(message.getData())
§§print("We are analyzing PID %d, not that we care..." % pid)
§§
§§# Step 3: Notify readiness
§§message = SharedMemMessage(MessageTypes.SHARED_MEM_MESSAGE_READY_TO_FUZZ, "")
§§ipc.sendMessageToServer(message)
§§
§§# Step 4: Loop for testcases
§§while(True):
§§    # Step 4.1: Wait for a filename
§§    message = ipc.waitForNewMessageToClient(timeout)
§§    if not message.getType() == MessageTypes.SHARED_MEM_MESSAGE_FUZZ_FILENAME:
§§        print("Did not receive a filename, exiting")
§§    testcase = message.getData()
§§
§§    # Step 4.2: We do out thing
§§    if sendTestcase(testcase):
§§        # Step 4.3: We tell the runner that we're done
§§        message = SharedMemMessage(MessageTypes.SHARED_MEM_MESSAGE_FUZZ_DONE, "")
§§        ipc.sendMessageToServer(message)
§§    else:
§§        message = SharedMemMessage(MessageTypes.SHARED_MEM_MESSAGE_FUZZ_ERROR, "")
§§        ipc.sendMessageToServer(message)
§§
