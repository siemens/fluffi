import sys, os, pykd, multiprocessing

def debuggerProcess(name, pipeName, queue):    
    queue.put("Running %s" % name)
    connectToKd(pipeName, queue)
    
def connectToKd(pipeName, queue, attempts=3):
    queue.put("Attempting to connect to vkd (%s)" % pipeName)
    for i in range(0, attempts+1):
        try:
            pykd.attachKernel("com:pipe,resets=0,port=\\\\.\\pipe\\%s" % pipeName)
            break
        except Exception as e:
            queue.put("Connection not succesfull: %s" % str(e))
            if i == attempts:
                raise(e)
            continue
    pykd.go()

if __name__ == "__main__":
    multiprocessing.set_executable(os.path.join(sys.exec_prefix, 'pythonw.exe')) 
    multiprocessing.freeze_support()

    # TODO: validate args
    pipeName = sys.argv[1]

    print("asdf")
    queue = multiprocessing.Queue()
    p = multiprocessing.Process(target=debuggerProcess, args=("debugerProcess", pipeName, queue))

    p.start()
    p.join()

    while not queue.empty():
        print(queue.get())
