from xmlrpc.client import ServerProxy
import sys, socket

def waitForProxy(proxy):
    ctrlc = False
    socket.setdefaulttimeout(1)
    while True:
        try:
            if proxy.ping():
                break
        except KeyboardInterrupt:
            ctrlc = True
            break
        except Exception:
            pass
    if ctrlc:
        raise KeyboardInterrupt
    socket.setdefaulttimeout(15)
    return True

def breakDebugger(proxy):
    pass
    #waitForProxy(proxy)
    #print("Program received signal SIGTRAP, Trace/breakpoint trap.")
    #registers = proxy.getRegisters()
    #print("%s in ?? ()" % registers["rip"])
    #return True

if len(sys.argv) > 1 and sys.argv[1] == "--version":
    print("GNU gdb emulator for kFuzz/FLUFFI interaction")
    sys.exit(0)

proxy = ServerProxy("http://localhost:9442")
socket.setdefaulttimeout(15)

print("Trying to establish a connection to WinDBG")
waitForProxy(proxy)
print("Connected")

while True:
    try:
        waitForProxy(proxy)
        status, rip = proxy.getStatus()
        if status == "EXCEPTION":
            print("Program received signal SIGSEV")
            print("%s in undefined" % rip)
            while True:
                pass
        elif status == "NOCLUE":
            print("Program received signal SIGNOIDEA")
            print("%s in undefined" % rip)
            while True:
                pass
        elif status == "STOP":
            print("Program received signal SIGTRAP")
            print("%s in undefined" % rip)
        try:
            line = input("(gdb) ")
        except EOFError:
            pass
        cmd = line.rstrip()
        if cmd == "help":
            print(" Native Commands")
            print("  quitPython")
            print("  quit")
            #print("  target <ip> - set ip of target for breaks")
            print(" GDB emulation commands")
            print("  [Ctrl+C]")
            print("  run")
            print("  c")
            print("  j")
            print("  i r") 
            print("  br *0x0 - do nothing")
            print("  d br - do nothing") 
            print("  set {unsigned char}")
            print("  set {unsigned short}")
            print("  set {unsigned int}")
            print("  set")
            print("  set new-console on")
            print("  info files")
            print("  info inferior")
            print("  x/1xb")
            print("  x/1xh")
            print("  x/1xw")
        elif cmd == "quitPython":
            print(proxy.quitPython())
        elif cmd == "ping":
            print(proxy.ping())
        elif cmd == "quit":
            break
        #elif "target " in cmd:
        #    target = cmd.split(" ")[1]
        elif cmd == "run" or cmd == "c":
            proxy.go()
            print("Continuing")
        elif "j *0x" in cmd:
            proxy.jump(cmd[3:])
        elif cmd =="i r":
            registers = proxy.getRegisters()
            for reg in registers.keys():
                regValue = registers[reg]
                if reg in ["rsp", "rbp"]:
                    print("%s\t\t%s\t\t%s" % (reg, regValue, regValue)) # GDB outputs hex in 3rd column for rsp/rbp for whatever reason....
                else:
                    print("%s\t\t%s\t\t%d" % (reg, regValue, int(regValue, 16)))
        elif "d br" in cmd:
            print("")
        elif  "br *" in cmd:
            print("Breakpoint 1 at 0x0")
        elif "set {unsigned char}" in cmd:
           ea = cmd.split(" ")[2].split("}")[1]
           value = int(cmd.split(" ")[4], 16)
           proxy.setByte(ea, value)
        elif "set {unsigned short}" in cmd:
           ea = cmd.split(" ")[2].split("}")[1]
           value = int(cmd.split(" ")[4], 16)
           proxy.setWord(ea, value)
        elif "set {unsigned int}" in cmd:
           ea = cmd.split(" ")[2].split("}")[1]
           value = int(cmd.split(" ")[4], 16)
           proxy.setDWord(ea, value)
        elif cmd == "set":
            print("Argument required (expression to compute).")
        elif cmd == "set new-console on":
            pass
        elif cmd == "info files":
            modules = proxy.getModules()
            print("Symbols from \"ASDF\".")
            print("Win32 child process:")
            print("\tSome kind of info text")
            print("Local exec file:")
            for module in modules:
                print("        %s - %s is .driver in %s" % (module["startEA"], module["stopEA"], module["modName"]))
        elif cmd == "info inferior":
            print("  Num  Description       Executable")
            print("* 1    process 0         OS")
        elif "x/1xb" in cmd:
            ea = int(cmd.split(" ")[1], 16)
            value = int(proxy.getByte(cmd.split(" ")[1]), 16)
            print("0x%x:\t\t%x" % (ea, value))
        elif "x/1xh" in cmd:
            ea = int(cmd.split(" ")[1], 16)
            value = int(proxy.getWord(cmd.split(" ")[1]), 16)
            print("0x%x:\t\t%x" % (ea, value))
        elif "x/1xw" in cmd:
            ea = int(cmd.split(" ")[1], 16)
            value = int(proxy.getDWord(cmd.split(" ")[1]), 16)
            print("0x%x:\t\t%x" % (ea, value))
        else:
            print("Undefined command: \"%s\". Try \"help\"." % cmd)
    except KeyboardInterrupt:
        # Send break!
        breakDebugger(proxy)
    except Exception as e:
        # Send break!
        print(e)
        breakDebugger(proxy)