<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Roman Bendt
-->

## Usage

### 1) Adding runner systems to FUN
All FuzzJobs are run on dedicated Runner systems in the FLUFFI Utility Network (FUN). To add new systems:

* Physically plug in your system into FUN
* Assign a host name. (Other systems within FUN must be able to reach your system with that hostname. The DNS server should be able to handle this though). On linux: `sudo hostnamectl set-hostname mysystem`
* Check connectivity to `gm.fluffi`
* For problems: check firewall
* Create a user for ansible. To do so you need to create user that matches whatever username and password you specified in ansible's [hosts](srv/fluffi/data/polenext/projects/1/hosts) file (see [the getting started section](getting_started.md)). On Linux, this user needs to be a sudoer. On Windows, this user needs to be local Administrator.
* Prepare the system for ansible. Linux should be fine. On Windows you can use FLUFFI's initialization script: 
  * `net use y: \\smb.fluffi\install\initial /user:nobody pass`
  * `y:\initialConfiguration.bat`
  * `net use y: /Delete /yes`
* Finally, you need to tell FLUFFI about the system. To do so you have two options: Either you add it to ansible's [hosts](srv/fluffi/data/polenext/projects/1/hosts) file. Alternatively, you can use the `Add System` button in FLUFFI's web gui.

### 2) Preparing your target

Currently, FLUFFI supports the following types of targets:

#### a) File-parsing binaries that run natively
FUN supports x64/x86 (Linux/Windows) systems and ARM/ARMarch64 systems (Linux). If your binary parses files and runs on one of these systems, you don't need to do anything more.

**PLEASE NOTE**, that many targets can be converted to this type. Either by changing the source code, or by patching the binary.

#### b) File-parsing binaries that need to be emulated

If your binary parses files and you know how to emulate it with QEMU user-mode emulation, you don't need to do anything more.

**PLEASE NOTE**, that many targets can be converted to this type. Either by changing the source code, or by patching the binary.


#### c) Server binaries that run natively

You need to write a so-called Feeder that allows FLUFFI to feed testcases to your target. This feeder can be written in any language you like. The interface towards FLUFFI that you need to call is implemented by the `SharedMemIPC.dll`/`libsharedmemipc.so`.

There are already some feeders pre-implemented [here](core/helpers/Feeder):
* A C++ Feeder that Feeds input to TCP servers (such as HTTP servers) is already implemented at [TCPFeeder](core/helpers/Feeder/TCPFeeder). If you want, you can use [sslwrap](core/helpers/Feeder/sslwrap) to connect to SSL/TLS servers.
* A C++ Feeder that Feeds input over Shared memory is already implemented at  [SharedMemFeeder](core/helpers/Feeder/SharedMemFeeder). This Feeder is very useful e.g. if you want to efficiently fuzz file-parsers. Just make sure that the file parser reads the file to parse via SharedMemory and not from command line. Doing so allows you to fuzz many testcases without the need to restart the target after each testcase. How to do this? Well, either build the target in a suitable way or replace the main function of your binary e.g. with main4fuzz / PrepareExe4Fuzz.
* A C++ Feeder that Feeds input to Ehternet servers is already implemented at [EthernetFeeder](core/helpers/Feeder/EthernetFeeder).
* A C++ Feeder that Feeds input to UDP servers is already implemented at [UDPFeeder](core/helpers/Feeder/UDPFeeder).
* A python Feeder that Feeds input to TCP servers (such as HTTP servers) is already implemented at [NeFu](core/helpers/Feeder/NeFu).

If you want to test your feeder you can use the feeder tester implemented at [Tester](core/helpers/Feeder/Tester).

#### d) GDB Targets

You need a Feeder just as with type c) targets. Furthermore, you need a GDB client that speaks the same protocol version as your GDB server.

#### e) Everything else

Contact us if you need anything else. FLUFFI can be extended to a lot of different targets types, if needed.

### 3) Create a FLUFFI Fuzz Job

All FuzzJobs are run on dedicated Runner systems in the FLUFFI Utility Network (FUN), to which you need to deploy your test target, which itself you should prepare as a *package*.

A package is a zip file containing either an `install.bat`, an `install.ps1`, or an `install.sh` file as well as arbitrary data. When you deploy this package, it is copied to the FLUFFI runner systems, extracted (on Windows to  `C:\fluffi\SUT\<ZipFileName>\`, on Linux to `/home/<FluffiUser>/fluffi/persistent/SUT/<ZipfileName>\`), and the corresponding install script is executed. You should prepare the package so that it installs all the required dependencies for the target as well as the target itself. Once you have such a zip file, you should upload it to the FLUFFI FTP server at `ftp.fluffi` using anonymous login.

In case you create a windows package, you should add page heap checks using gflags for the target binary. This allows FLUFFI to better detect access violations in the heap. As gflags is already installed on all of our systems, you only need to add a line to your `install.ps1` or `install.bat` such as: `C:\utils\GFlags\x86\gflags.exe /p /enable TargetBinary.exe`

Once the package is on the FTP server, you are ready to deploy it on the selected runner systems. You can do this in the `Systems` Tab or while creating a fuzz job.

If you want to create a FuzzJob go to the FLUFFI web page (currently listening on `http://web.fluffi/`).


Having done so, you need to create a FuzzJob via `FuzzJobs->Create FuzzJob`.

![alt text](CreateNewProject.png "The dialog to create a new FuzzJob")

**Name**

Should be something like "MiniWeb", "MyXMLParser". Note: For now the name must not contain [-_ ].

**Runner Type**

Currently the following Runner (aka Testcase Executors) are defined
- `ALL_GDB`
- `ALL_Lin_QemuUserSingle`
- `ARM_Lin_DynRioMulti`
- `ARM_Lin_DynRioSingle`
- `ARM64_Lin_DynRioMulti`
- `ARM64_Lin_DynRioSingle`
- `X64_Lin_DynRioMulti`
- `X64_Lin_DynRioSingle`
- `X64_Win_DynRioMulti`
- `X64_Win_DynRioSingle`
- `X86_Lin_DynRioMulti`
- `X86_Lin_DynRioSingle`
- `X86_Win_DynRioMulti`
- `X86_Win_DynRioSingle`

Which one to choose depends on how you prepared your target. The `QemuUserSingle` runner is used for emulated binaries (type b), `DynRioMulti` runners are used for server binaries (type c), `DynRioSingle` runners are used for native file parsers (type a), and `ALL_GDB` is used for targets of type d.

**Generator Types**
FLUFFI supports various Testcase generators. You can use as many of them as you like. Currently Implemented are:
- `RadamsaMutator`
- `AFLMutator`
- [CarrotMutator](core/CaRRoT)
- `HonggfuzzMutator`
- [OedipusMutator](core/Oedipus)
- `ExternalMutator` (Allows easy addition of custom mutators):
   In this case you need to specify an additional setting: `extGeneratorDirectory`
   This setting points to a directory. In this directory, FLUFFI will insert a file called `fuzzjob.name` containing the name of the current FuzzJob.
   It is the job of the external mutator to:
      1. Register on the GM with a new UUID (and a nice name)
      2. Create a subdirectory with that UUID
      3. Place new mutations in that subdirectory following this schema: `ParentGUID_ParentLocalID_GeneratorLocalID`
      4. Ensure that the hard drive doesn't fill up (e.g. upper limit of files in directory)

You need to set the percentage of how many generators should have which generator type. E.g. if you only want `RadamsaMutators` set `RadamsaMutator`=100 and all others to 0.

**Evaluator Types**
FLUFFI supports various Testcase evaluators. You can use as many of them as you like. Currently Implemented are:
- `CoverageEvaluator`

You need to set the percentage of how many evaluators should have which evaluator type. E.g. if you only want `CoverageEvaluators` set `CoverageEvaluator`=100 and all others to 0.

**Location**

The location(s) in which you want to run your FuzzJob (can be changed later)

**Options**

Options for the FuzzJob. 

#### TestcaseRunner

Which runner is used is set by the runnerType parameter. For possible values see `myAgentSubTypes` in [TestcaseRunner.cpp](core/TestcaseRunner/TestcaseRunner.cpp).

For *_DynRioSingle:
- **targetCMDLine**: The command line to start the target (e.g. `"C:\FLUFFI\SUT\my target\mytarget.exe" -startnormal` on Windows or `/home/<FluffiUser>/fluffi/persistent/SUT/my target/mytargetbin` on Linux). It needs to be absolute! If you specify \<INPUT_FILE\> somewhere in the command line, FLUFFI will replace it with the filename of the testcase. Otherwise, FLUFFI will append the filename of the testcase to the end of the command line.
- **hangTimeout**: Duration in milliseconds after which a Testcase execution will be considered to have timed out if it did not yet complete.
- **suppressChildOutput**: Many targets output either to stdout or open windows. For debugging purposes it makes sense to set this to false. For production it should be set to true.
- **populationMinimization**: The database will mark population with exactly the same covered blocks as duplicates and ignore them from then on. By default, set to *false*, can be set to *true*.
- **treatAnyAccessViolationAsFatal**: Some targets catch access violations. These access violations might belong to normal operation, or not. Setting this parameter to true makes FLUFFI treat each access violation as an access violation crash - even if the application would catch and handle it (defaults to false).
- **additionalEnvParam**: Parameter to append to the target process's environment (Linux only), e.g. `LD_PRELOAD=/my/shared/obj.so`.

For *_DynRioMulti:
- **targetCMDLine**: The command line to start the target (e.g. `"C:\FLUFFI\SUT\my target\mytarget.exe" -startnormal` on Windows or `/home/<FluffiUser>/fluffi/persistent/SUT/my target/mytargetbin` on Linux). It needs to be absolute! FLUFFI will replace \<RANDOM_SHAREDMEM\> with a randomly generated shared memory name. This makes sense when using the [SharedMemFeeder](core/helpers/Feeder/SharedMemFeeder), which expects a shared memory name as last parameter of the target. Furthermore, FLUFFI will replace \<RANDOM_PORT\> with a random but free port. This makes sense when applications bind to a TCP port that can be specified via command line. Setting it allows launching multiple target instances on a single machine.
- **hangTimeout**: Duration in milliseconds after which a Testcase execution will be considered to have timed out if it did not yet complete.
- **suppressChildOutput**: Many targets and feeders output either to stdout or open windows. For debugging purposes it makes sense to set this to false. For production it should be set to true.
- **populationMinimization**: The database will mark population with exactly the same covered blocks as duplicates and ignore them from then on. By default, set to *false*, can be set to *true*.
- **feederCMDLine**: The command line to start the feeder (e.g. `"C:\FLUFFI\SUT\my target\TCPFeeder.exe" 80`,  `"C:\FLUFFI\SUT\my target\TCPFeeder.exe"` (not setting a port will cause the feeder to scan the target for the listining port), or `"C:\FLUFFI\SUT\my target\SharedMemFeeder.exe"`). The name for the SharedMemory Location (as you need in Feeder SourceCode) will be added automatically (for the interface between FLUFFI<->Feeder).
- **initializationTimeout**: Duration in milliseconds after which the feeder needs to have reported to the runner that it can talk to the target. If the timeout has passed, both target and feeder will be restarted.
- **starterCMDLine**: Empty ("") if you want the TR to start the target directly via the targetCMDLine. That's enough in most cases. Examples: Servers, APIs ... If you need the TR to attach to the target (e.g. because the target is a service), you need to specify a program here that starts the target. (e.g. Starts the service und signals "started") Examples for such starters are [ProcessStarter](core/helpers/Starter/ProcessStarter), and [ServiceStarter](core/helpers/Starter/ServiceStarter). In either case, the target will be started only once until a crash occurs (not in each runner pass). In the case of a service, the TR is automatically attached to the service. In this case, the TR waits until the starter has finished, and then looks for a process with the same path and name as the one specified as the *targetCMDLine*.
- **targetForks**: If your target is Linux AND you start it with a starter AND your target forks, you might want to set this parameter to true. Leave it alone otherwise. This is needed, e.g. if you want to fuzz a service that forks, closes the main process, and continues in the child process.
- **forceRestartAfter**: If your target tends to stop functioning after a while but your feeder doesn't realize this (which could happen if there is no backchannel, e.g. when fuzzing UDP), you can specify this value to force a restart of the target and the feeder after X iterations.
- **treatAnyAccessViolationAsFatal**: Some targets catch access violations. These access violations might belong to normal operation, or not. Setting this parameter to true makes FLUFFI treat each access violation as an access violation crash - even if the application would catch and handle it (defaults to false).
- **additionalEnvParam**: Parameter to append to the target process's environment (Linux only), e.g. `LD_PRELOAD=/my/shared/obj.so`. Only has an effect if no starter is used.

For ALL_Lin_QemuUserSingle:
- **changerootTemplate**: path to the root filesystem you deployed. relative to TestcaseRunner OR absolute path is possible. will be used to changeroot into for running the emulated binary. should therefore contain all necessary libraries and files, including the target binary in a known location. Currently, must also contain the qemu-fluffi binary to use for emulation. note that the directory will not be directly used as changeroot target but copied to a separate location. This way, the original will not be modified and can be used over and over. If starterCMDLine is set, targetCMDLine should hold the command line of the target to attach to (e.g. `"C:\Program Files (x86)\myservice\myservice.exe"`). 
- **targetCMDLine**: the command line to execute INSIDE THE CHROOT environment AFTER CHROOTing. use ABSOLUTE PATHS inside the chroot.
	- **qemu binary path**: RELATIVE TO ROOTFS ususally `/bin/qemu-<ARCH>`
	- **qemu trace options(DEPRECATED)**: `-trace events=/etc/qemu-events -d nochain` (usualy no need to change this)
	- **qemu-fluffi parameter to forward signals to the target ( crash-on-signal else)**: `-f`
	- **qemu switch to pass environment vars to the target**: `-E LD_PRELOAD=/my/shared/obj.so`
	- **path to the target executable**: `/home/user/fuzztarget`
	- **parameters to pass to the target**: `-v -d --please-do-not-crash --config /home/user/conf.conf --read-in`
	- note that the system will pass the path to the testcase file after all the stuff you just defined as a last parameter
	(eg. `[...] --read-in /fluffi/testcase/00235.bin` in above example)

- **hangTimeout**: Duration in milliseconds after which a Testcase execution will be considered to have timed out if it did not yet complete.
- **suppressChildOutput**: Many targets output either to stdout or open windows. For debugging purposes it makes sense to set this to false. For production it should be set to true.
- **populationMinimization**: The database will mark population with exactly the same covered blocks as duplicates and ignore them from then on. By default, set to *false*, can be set to *true*.
- **treatAnyAccessViolationAsFatal**: Some targets catch access violations. These access violations might belong to normal operation, or not. Setting this parameter to true makes FLUFFI treat each access violation as an access violation crash - even if the application would catch and handle it (defaults to false).

For ALL_GDB:
- **targetCMDLine**: The command line to start the GDB (e.g. `"C:\FLUFFI\SUT\my target\gdb.exe"` on Windows or `/home/<FluffiUser>/fluffi/persistent/SUT/my target/gdb` on Linux). It needs to be absolute!
- **hangTimeout**: Duration in milliseconds after which a Testcase execution will be considered to have timed out if it did not yet complete.
- **suppressChildOutput**: Many targets and feeders output either to stdout or open windows. For debugging purposes it makes sense to set this to false. For production it should be set to true.
- **populationMinimization**: The database will mark population with exactly the same covered blocks as duplicates and ignore them from then on. By default, set to *false*, can be set to *true*.
- **feederCMDLine**: The command line to start the feeder (e.g. `"C:\FLUFFI\SUT\my target\TCPFeeder.exe" 80`). The name for the SharedMemory Location (as you need in Feeder SourceCode) will be added automatically (for the interface between FLUFFI<->Feeder). IMPORTANT: Due to the way of how the communication with GDB is implemented, the feeder MUST ignore Ctrl+C on Windows (`SetConsoleCtrlHandler(NULL, true);` in C); 
- **initializationTimeout**: Duration in milliseconds after which the feeder needs to have reported to the runner that it can talk to the target. If the timeout has passed, both target and feeder will be restarted.
- **starterCMDLine**: You need to specify a program here that starts the target (e.g. `"C:\FLUFFI\SUT\my target\GDBStarter.exe -local C:\FLUFFI\SUT\my target\actual target.exe"`), and creates a GDB initialization file so that the GDB can connect to the target. The starter will be executed only once until a crash occurs (not in each runner pass). Examples for GDB starters are implemented in [GDBStarter](core/helpers/Starter/GDBStarter).
- **forceRestartAfter**: If your target tends to stop functioning after a while but your feeder doesn't realize this (which could happen if there is no backchannel, e.g. when fuzzing UDP), you can specify this value to force a restart of the target and the feeder after X iterations.
- **breakPointInstruction**: The target architecture's break point instruction IN HEXADECIMAL ENCODING (e.g. 0xCC on x86/x64). If you don't know yours grep the gdb server sources for your architecture, e.g. [here](https://chromium.googlesource.com/chromiumos/third_party/gdb/+/3a0081862f33c17f3779e30ba71aacde6e7ab1bd/gdb/gdbserver)
- **breakPointInstructionBytes**: Number of bytes of breakPointInstruction. Must be 1,2, or 4

**PLEASE NOTE**: All options can be changed while the job is running. You will however need to restart the agents as they only pull the options upon start.

**Target Modules**

FLUFFI is a coverage-based evolutionary fuzzer. In order to reduce the noise in the coverage, only those modules that are listed here will be used to calculate the coverage. Examples for modules are `test.dll`, or `target.exe`.

If there are several modules with the same name but different paths, you can specify which one should be used for coverage calculation by specifying the path. If it is left to `*`, the path of a module is ignored.

**PLEASE NOTE**: If you are using a GDB runner, the module name is actually a segment name. An example therefore is `target.exe/.text` 

**Population**

Each evolutionary fuzzer needs a set of known-to-be-good input samples to start with. These need to be uploaded here. You can specify more than one at a time. Furthermore, you can always add more while the project is running.

**Basic Blocks**

Only relevant for ALL_GDB runner.
These are the blocks that will be considered for coverage collection. FLUFFI expects a file having the following format:

```
target.exe/.text,0x1 
target.exe/.text,0x2 
helper.dll/.text,0x100 
```

If you want, you can alternatively insert the blocks in the database table `blocks_to_cover` yourself (e.g. by using an IDA python script).

#### TestcaseGenerator

Which generator types should be used is set by the generatorTypes parameter. It is a string of all types to use followed by their percentage. If you want to use only one generator of type A set `A=100`. If you want to use two with the same probability set `A=50|B=50`. For possible type values see `myAgentSubTypes` in [TestcaseGenerator.cpp](core/TestcaseGenerator/TestcaseGenerator.cpp).

**PLEASE NOTE** that the LocalManagers will try to stick as close to your setting as possible. If, however, only generators register that have e.g. only type A implemented, the ratio might not be as desired.

#### TestcaseEvaluator

Which evaluator types should be used is set by the evaluatorTypes parameter. It is a string of all types to use followed by their percentage. If you want to use only one evaluator of type A set `A=100`. If you want to use two with the same probability set `A=50|B=50`. For possible type values see `myAgentSubTypes` in [TestcaseEvaluator.cpp](core/TestcaseEvaluator/TestcaseEvaluator.cpp).

**PLEASE NOTE** that the LocalManagers will try to stick as close to your setting as possible. If, however, only evaluators register that have e.g. only type A implemented, the ratio might not be as desired.


### 4) Start the FLUFFI Agents
You can control the number of running FLUFFI Agents (LM, TR, TE, TG) via the web gui.

To do so, you can click on 
- the `Config System Instances` button on the FuzzJob overview page
- a group name in the `Systems` tab
- a system name in the `Systems` tab

Furthermore, you can stop running agents by clicking the `Managed Instances` button when viewing the FuzzJob overview page.

Alternatively, you can connect to the system directly (SSH, RDP), and start the Agents there.

The credentials to do so can be looked up in polemarch's [hosts](srv/fluffi/data/polenext/projects/1/hosts) file. Once there, on Windows start one LM (if there is not already one for your fuzzJob, e.g. on another machine in the same location) and as many TRs / TGs / TEs as you like, e.g. by clicking on the icons on the Desktop. On Linux, just start the appropriate agent binary from the command line, ensuring that you add the location name as the argument for the agent.

You should always monitor your fuzzJob in the `Managed Instances` view in order to make sure that
1. The system is not overloaded (e.g. CPU>90%)
2. The TG queues are not close to 0
3. The TE queues are not growing and growing. 
