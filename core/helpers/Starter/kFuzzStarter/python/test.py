import pykd
import multiprocessing
import time, sys, os

def run_program():
    print("start debugger process")
    program = "C:\\windows\\system32\\notepad.exe"
    proc = pykd.startProcess(program, pykd.ProcessDebugOptions.BreakOnStart | pykd.ProcessDebugOptions.DebugChildren)
    while pykd.executionStatus.Break == pykd.go():
        print("break in debugger process")
    print("stop debugger process")
    while True:
        pass

if __name__ == "__main__":
    #pykd.initialize()
    multiprocessing.freeze_support()
    multiprocessing.set_executable(os.path.join(sys.exec_prefix, 'pythonw.exe')) 
    #multiprocessing.set_executable("C:\\Program Files (x86)\\Microsoft Visual Studio\\Shared\\Python37_64\\pythonw.exe")
    #multiprocessing.set_executable("C:\\Users\\z0030rzw\\AppData\\Local\\Programs\\Python\\Python37-32\\pythonw.exe")
    print("start main process")
    p = multiprocessing.Process(target=run_program, args=())
    p.start()
    p.join()
    print("stop main process")
    while True:
        pass
