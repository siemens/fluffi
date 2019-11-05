from pykd import *
from xmlrpc.server import SimpleXMLRPCServer

version = "Win10"
state = "STOP"
status = "NORMAL"



def initEnvironment():
    global version

    # Update module analysis
    dbgCommand(".reload")

    # Check version
    vertarget =  dbgCommand("vertarget")
    if "Windows 10" in vertarget:
        version = "Win10"
    # Win7 check

    # Create break possibility -> Beep
    dbgCommand("bp Beep!BeepStartIo")


def ping():
    return True

def go():
    global state
    state = "RUN"
    return True

def quitPython():
    global state
    state = "QUIT"
    return True

def getModules():
    lm = dbgCommand("lm")
    modules = []
    for line in lm.split("\n")[1:]:
        if "Unloaded modules" in line or line == '':
            continue
        splitLine = line.split(" ")
        module = {}
        # Stored as string because XMLRPC cannot transfer 64 bit ints...
        module["startEA"] = "0x%x" % expr(splitLine[0])
        module["stopEA"] = "0x%x" % expr(splitLine[1])
        module["modName"] = splitLine[4]
        modules.append(module)
    return modules

def getRegisters():
    registers = {}
    # Stored as string because XMLRPC cannot transfer 64 bit ints...
    registers["rax"] = "0x%x" % expr("rax")
    registers["rcx"] = "0x%x" % expr("rcx")
    registers["rdx"] = "0x%x" % expr("rdx")
    registers["rbx"] = "0x%x" % expr("rbx")
    registers["rsp"] = "0x%x" % expr("rsp")
    registers["rbp"] = "0x%x" % expr("rbp")
    registers["rsi"] = "0x%x" % expr("rsi")
    registers["rdi"] = "0x%x" % expr("rdi")
    registers["rip"] = "0x%x" % expr("rip")
    registers["eflags"] = "0x%x" % expr("efl")
    registers["cs"] = "0x%x" % expr("cs")
    registers["ss"] = "0x%x" % expr("ss")
    registers["ds"] = "0x%x" % expr("ds")
    registers["es"] = "0x%x" % expr("es")
    registers["fs"] = "0x%x" % expr("fs")
    registers["gs"] = "0x%x" % expr("gs")
    registers["r8"] = "0x%x" % expr("r8")
    registers["r9"] = "0x%x" % expr("r9")
    registers["r10"] = "0x%x" % expr("r10")
    registers["r11"] = "0x%x" % expr("r11")
    registers["r12"] = "0x%x" % expr("r12")
    registers["r13"] = "0x%x" % expr("r13")
    registers["r14"] = "0x%x" % expr("r14")
    registers["r15"] = "0x%x" % expr("r15")
    return registers

def setByte(ea, value):
    try:
        dbgCommand("eb 0x%x 0x%x" % (int(ea, 16), value))
    except Exception as e:
        print(e)
        return False
    return True

def setWord(ea, value):
    try:
        dbgCommand("ew 0x%x 0x%x" % (int(ea, 16), value))
    except Exception as e:
        print(e)
        return False
    return True
    
def setDWord(ea, value):
    try:
        dbgCommand("ed 0x%x 0x%x" % (int(ea, 16), value))
    except Exception as e:
        print(e)
        return False
    return True

def getByte(ea):
    try:
        return "0x%x" % loadBytes(int(ea, 16), 1)[0]
    except Exception as e:
        print(e)
        return False

def getWord(ea):
    try:
        return "0x%x" % loadWords(int(ea, 16), 1)[0]
    except Exception as e:
        print(e)
        return False

def getDWord(ea):
    try:
        return "0x%x" % loadDWords(int(ea, 16), 1)[0]
    except Exception as e:
        print(e)
        return False

def jump(ea):
    global state
    try:
        addr = int(ea, 16)
        setIP(addr)
        state = "RUN"
    except Exception as e:
        print(e)
        return False
    return True

def getStatus():
    global state
    return (state, "0x%x" % expr("rip"))

def initServer():
    print("kFuzz - FLUFFI powered")
    server = SimpleXMLRPCServer( ("localhost", 9442) )
    server.register_function(ping)
    server.register_function(go)
    server.register_function(quitPython)
    server.register_function(getModules)
    server.register_function(getRegisters)
    server.register_function(setByte)
    server.register_function(setWord)
    server.register_function(setDWord)
    server.register_function(getByte)
    server.register_function(getWord)
    server.register_function(getDWord)
    server.register_function(jump)
    server.register_function(getStatus)
    return server

initEnvironment()
server = initServer()

while True:
    try:
        server.handle_request()
        if state == "QUIT":
            break
        while state == "RUN":
            dbgCommand("g")
            
            evt = getLastEvent()
            # Did we reach our breakpoint for interrupting program flow?
            if evt.type == eventType.Breakpoint:
                if version == "Win10":
                    ip = expr("rip")
                    bp = expr("Beep!BeepStartIo")
                    if ip == bp:
                        # Check length and frequency to see whther it is a "normal" beep:
                        beepArgs = expr("poi(poi(rdx+0x18))")
                        if beepArgs == 0x2a00001092:
                            state = "STOP"
                            break
                        else:
                            continue
                    else:
                        print("Breakpoint at %x" % expr("rip"))
            elif evt.type == eventType.Exception:
                print("Exception at %x" % expr("rip"))
                status = "EXCEPTION"
                pass #DO STUFF
            else:
                print("Event at %x" % expr("rip"))
                status = "NOCLUE"
    except Exception as e:
        print(e)
        server = initServer()