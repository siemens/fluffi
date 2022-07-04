<!---
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

Author(s): Thomas Riedmaier, Junes Najah, Abian Blome, Roman Bendt
-->

# Usage

## 1) Adding runner systems to FUN
All FuzzJobs are run on dedicated Runner systems in the FLUFFI Utility Network (FUN). You can bring your own system or use the FLUFFI PXE images (recommended). How to build these images is documented [here](agents/pxe_images).

To add new systems:

* Physically plug your system into FUN
* Check connectivity to `gm.fluffi`. For problems: check the firewall
* Decide on the host name for that system, and put it in the [MAC2Host.csv](srv/fluffi/data/smb/files/initial/MAC2Host.csv).  Other systems within FUN must be able to reach your system with that hostname.
* Create a user for ansible. To do so you need to create a user that matches whatever username and password you specified in ansible's [hosts](srv/fluffi/data/polenext/projects/1/hosts) file (see [the getting started section](./getting_started.md)). On Linux, this user needs to be a sudoer. On Windows, this user needs to be local administrator.
* Prepare the system for ansible. To do so run the following commands on the new agent system:
  * Windows:
    * `net use y: \\smb.fluffi\install\initial /user:nobody pass`
    * `y:\initialConfiguration.bat`
    * `net use y: /Delete /yes`
  * Linux:
    * Make sure, your system is able to reach a packet mirror from within FUN (see also [the package mirror section in the getting_started.md](./getting_started.md)).
    * `apt-get install openssh-server smbclient net-tools`
    * `smbclient '//smb.fluffi/install' -c 'cd initial; get MAC2Host.csv; get initialConfiguration.sh' -U anonymous%pass;`
    * `chmod 777 initialConfiguration.sh`
    * `/bin/bash initialConfiguration.sh`
    * `rm MAC2Host.csv`
    * `rm initialConfiguration.sh`
* Tell FLUFFI about the system. To do so you have two options: either add it as a new windows/linux/odroid host to ansible's [hosts](srv/fluffi/data/polenext/projects/1/hosts) file (persistent), or use the `Add System` button in FLUFFI's web GUI. When adding it via the web GUI just use the system's host name without any domain suffix. When adding it via the hosts file, the line needs to look like this: `<hostname> ansible_ssh_host=<hostname>.fluffi`. Remember: When modifying the hosts file, you need to restart the polemarch and fluffi_web containers.
+ Assign a location to the system in the `Systems` tab. An explication of the location concept can be found [here](./technical_details.md).

## 1.1) Deploying FLUFFI
Now that the new agent system is part of the FUN, the system needs to be configured for FLUFFI. Furthermore, the FLUFFI agent binaries need to be copied to the agent system.

You can do this by navigating to the 'Systems' tab in FLUFFI's web GUI, and clicking on the system's name. Now you should click on 'Initial Setup' and let Pelemarch/Ansible configure the target system for FLUFFI.

Once this is done, you should go to the 'Deploy FLUFFI' tab and deploy those FLUFFI binaries that correspond to the target's architecture. If your target is an X64 system, you might also want to copy the x86 binaries, to be able to fuzz both architectures on a single machine.

## 2) Preparing your target

### 2.1) Getting the input into your target

Currently, FLUFFI supports the following types of targets:

#### 2.1.1) File-parsing binaries that run natively
FUN supports x64/x86 (Linux/Windows) systems and ARM/ARMarch64 systems (Linux). If your binary parses files and runs on one of these systems, no further action is required.

**PLEASE NOTE** Many targets can be converted to this type, either by changing the source code or by patching the binary.

#### 2.1.2) File-parsing binaries that need to be emulated

If your binary parses files and you know how to emulate it with QEMU user mode emulation, no further action is required.

**PLEASE NOTE** Many targets can be converted to this type, either by changing the source code, or by patching the binary.


#### 2.1.3) Server binaries that run natively

You need to write a so-called Feeder that allows FLUFFI to feed test cases to your target. This feeder can be written in whichever language you prefer. The interface towards FLUFFI that you need to call is implemented by the `SharedMemIPC.dll`/`libsharedmemipc.so`.

There are already some feeders pre-implemented [here](core/helpers/Feeder):
* A C++ Feeder that feeds input to TCP servers (such as HTTP servers) is already implemented at [TCPFeeder](core/helpers/Feeder/TCPFeeder). If you want, you can use [sslwrap](core/helpers/Feeder/sslwrap) to connect to SSL/TLS servers.
* A C++ Feeder that feeds input over shared memory is already implemented at  [SharedMemFeeder](core/helpers/Feeder/SharedMemFeeder). This Feeder is very useful especially if you want to efficiently fuzz file-parsers. To do so, first make sure that the file parser reads the file to parse via SharedMemory and not from the command line. This allows you to fuzz many test cases without needing to restart the target after each test case. To do so, either build the target in a suitable way or replace the main function of your binary (e.g., with main4fuzz / PrepareExe4Fuzz).
* A C++ Feeder that feeds input to Ethernet servers is already implemented at [EthernetFeeder](core/helpers/Feeder/EthernetFeeder).
* A C++ Feeder that feeds input to UDP servers is already implemented at [UDPFeeder](core/helpers/Feeder/UDPFeeder).
* A python Feeder that feeds input to TCP servers (such as HTTP servers) is already implemented at [NeFu](core/helpers/Feeder/NeFu).

If you want to test your feeder, you can use the feeder tester implemented at [Tester](core/helpers/Feeder/Tester).

#### 2.1.4) GDB Targets

You need a Feeder (see last section). Furthermore, you need a GDB client that speaks the same protocol version as your GDB server.

#### 2.1.5) Windows kernel modules (e.g. Windows drivers)

You need a Feeder (see last section). Furthermore, you need to set up [kFuzz for FLUFFI](core/helpers/kfuzz).

#### 2.1.6) Everything else

Contact us if you need anything else. FLUFFI can be extended to a wide variety of targets types, if needed.

### 2.2) Replacing compare functions with fuzzable versions

Fuzzing of compare functions such as memcmp and strcmp is generally a very difficult task for a black-box coverage-based fuzzer such as FLUFFI.

To help FLUFFI handle this we added the [fuzzcmp](core/helpers/fuzzcmp) helper. What this helper does is - in one sentence - replacing standard compare functions with something that FLUFFI can nicely handle.

We recommend using it always!


## 3) Create a FLUFFI FuzzJob


If you want to create a FuzzJob go to the FLUFFI web page (currently reachable on `http://web.fluffi/`).


Having done so, you need to create a FuzzJob via `FuzzJobs->Create FuzzJob`.

![The dialog to create a new FuzzJob](CreateNewProject.png)

**Name**

The name should be something like "MiniWeb" or "MyXMLParser". Note: for now the name can not contain [-_ ].

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

Which one to choose depends on how you prepared your target. The `QemuUserSingle` runner is used for emulated binaries (see section 2.1.2), `DynRioMulti` runners are used for server binaries (see section 2.1.3), `DynRioSingle` runners are used for native file parsers (see section 2.1.1), and `ALL_GDB` is used if you use GDB (see section 2.1.4), or WinDBG (see section 2.1.5).

**Generator Types**

FLUFFI supports various test case generators. You can use as many of them as you like. The following are currently implemented:
- `RadamsaMutator`
- `AFLMutator`
- [CarrotMutator](core/CaRRoT)
- `HonggfuzzMutator`
- [OedipusMutator](core/Oedipus)
- `ExternalMutator` (Allows easy addition of custom mutators):
   in this case you need to specify an additional setting: `extGeneratorDirectory`
   This setting points to a directory, where FLUFFI will insert a file called `FuzzJob.name` containing the name of the current FuzzJob.
   It is the job of the external mutator to:
  1. Come up with a unique ID (UUID)
  2. Create a subdirectory in the `extGeneratorDirectory` named like that UUID
  3. Connect to the GM database and extracts the connection parameters for the FuzzJob's database from the `fuzzjob` table
  4. Connect to the FuzzJob's database and place a nice name in the FuzzJob's `nice_names_managed_instance` table for the chosen UUID
  5. Place new mutations in the UUID subdirectory following this schema: `ParentGUID_ParentLocalID_GeneratorLocalID`. `GeneratorLocalID` needs to be a decimal number that is unique for the current mutator instance (i.e. you must ensure that the touple (`UUID`,`GeneratorLocalID`) is unique). If you want you can use any information from the FuzzJob's database to generate good testcases.
  6. Ensure that the hard drive does not fill up (e.g. by implementing a upper limit of files in the directory)
  7. Adapt ratings of Testcases that were used for mutations. Rule of thumb: each mutation that was done based on a parent should decrease the parent's rating by one.

You need to set the percentage of how many generators should have which generator type. For example, if you only want `RadamsaMutators`, set `RadamsaMutator`=100 and all others to 0.

**Evaluator Types**

FLUFFI supports various test case evaluators. You can use as many of them as you like. The following are currently implemented:
- `CoverageEvaluator`

You need to set the percentage of how many evaluators should have which evaluator type. For example, if you only want `CoverageEvaluators` set `CoverageEvaluator`=100 and all others to 0.

**Location**

The location(s) in which you want to run your FuzzJob. This can be changed later.

**Options**

Options for the FuzzJob. 

### LocalManager

- **searchStrategy**: determines which testcase is chosen to be mutated. Defaults to `rating`.
  - `rating`: the testcase with the highest rating is chosen.
  - `roundRobin`: the testcase that has not been chosen for the longest time is chosen, and new testcases are added to the back of the queue.
  - `FAST`: the testcase with the lowest number of times chosen, then lowest number of paths exercised, then the highest rating is chosen. Edge coverage must be enabled using the **edgeCoverageModule** option.

### TestcaseGenerator

- **powerSchedule**: determines how many new testcases (*energy*) are generated for each mutation. Defaults to `constant`.
  - `constant`: Constant energy. On successful mutation, energy increments by one until maximum. On error, energy decrements by one until 1.
    - **constantFuzz**: The starting energy (default: 50).
    - **maxFuzz**: The maximum possible energy (default: equal to **constantFuzz**).
  - `FAST`: $$\min\left(\frac{\alpha(i)}{\beta}\cdot\frac{2^{s(i)}}{f(i)},M\right)$$
    - where $\alpha(i)$ is the rating,
    - $\beta$ is a constant,
    - $s(i)$ is the number of times the testcase has been chosen,
    - $f(i)$ is the number of paths that the testcase exercises,
    - and $M$ is the maximum possible energy.
    - Edge coverage must be enabled using the **edgeCoverageModule** option.
    - **minFuzz**: The minimum possible energy (default: 30).
    - **maxFuzz**: The maximum possible energy (default: 500).
    - **fastConstant**: The value of $\beta$ in the above equation (default: 100).

### TestcaseRunner

The runnerType parameter sets which runner will be used. For possible values see `myAgentSubTypes` in [TestcaseRunner.cpp](core/TestcaseRunner/TestcaseRunner.cpp).

For *_DynRioSingle:
- **targetCMDLine**: The command line to start the target (e.g. `"C:\FLUFFI\SUT\my target\mytarget.exe" -startnormal` on Windows or `/home/<FluffiUser>/fluffi/persistent/SUT/my target/mytargetbin` on Linux). It needs to be absolute! If you specify \<INPUT_FILE\> somewhere in the command line, FLUFFI will replace it with the filename of the test case. Otherwise, FLUFFI will append the filename of the test case to the end of the command line.
- **hangTimeout**: Duration in milliseconds after which a test case execution will be considered to have timed out if it did not yet complete.
- **suppressChildOutput**: Many targets output either to stdout or open windows. For debugging purposes, it makes sense to set this to false. For production it should be set to true.
- **populationMinimization**: The database will mark population items with exactly the same covered blocks as duplicates and ignore them from then on. By default this is set to *false*, but can be set to *true*.
- **treatAnyAccessViolationAsFatal**: Some targets catch access violations. These access violations might be part of normal operation, or not. Setting this parameter to true makes FLUFFI treat each access violation as an access violation crash - even if the application would catch and handle it (defaults to false).
- **additionalEnvParam**: Parameter to append to the target process's environment (Linux only), e.g. `LD_PRELOAD=/my/shared/obj.so`.
- **edgeCoverageModule**: set to a single module name to enable tracking path coverage via AFL's method of edge coverage. A 64-bit xxHash of the edges is computed and stored. Only supports x86.

For *_DynRioMulti:
- **targetCMDLine**: The command line to start the target (e.g. `"C:\FLUFFI\SUT\my target\mytarget.exe" -startnormal` on Windows or `/home/<FluffiUser>/fluffi/persistent/SUT/my target/mytargetbin` on Linux). It needs to be absolute! FLUFFI will replace \<RANDOM_SHAREDMEM\> with a randomly generated shared memory name. This makes sense when using the [SharedMemFeeder](core/helpers/Feeder/SharedMemFeeder), which expects a shared memory name as the last parameter of the target. Furthermore, FLUFFI will replace \<RANDOM_PORT\> with a random free port. This makes sense when applications bind to a TCP port that can be specified via command line. Setting it makes it possible to launch multiple target instances on a single machine.
- **hangTimeout**: Duration in milliseconds after which a test case execution will be considered to have timed out if it did not yet complete.
- **suppressChildOutput**: Many targets and feeders output either to stdout or open windows. For debugging purposes, it makes sense to set this to false. For production it should be set to true.
- **populationMinimization**: The database will mark population items with exactly the same covered blocks as duplicates and ignore them from then on. By default this is set to *false*, but can be set to *true*.
- **feederCMDLine**: The command line to start the feeder (e.g. `"C:\FLUFFI\SUT\my target\TCPFeeder.exe" 80`,  `"C:\FLUFFI\SUT\my target\TCPFeeder.exe"` (not setting a port will cause the feeder to scan the target for the listening port), or `"C:\FLUFFI\SUT\my target\SharedMemFeeder.exe"`). The name for the SharedMemory Location (as accessed in the Feeder's source code) will be added automatically (for the interface between FLUFFI<->Feeder).
- **initializationTimeout**: Duration in milliseconds after which the feeder needs to have reported to the runner that it can talk to the target. If the timeout has passed, both target and feeder will be restarted.
- **starterCMDLine**: Leave this empty ("") if you want the TestaceRunner to start the target directly via the *targetCMDLine*. That's enough in most cases. Examples: Servers, APIs ... If you need the TR to attach to the target (e.g. because the target is a service), you need to specify a program here that starts the target (i.e. starts the service und signals "started"). Examples for such starters are [ProcessStarter](core/helpers/Starter/ProcessStarter), and [ServiceStarter](core/helpers/Starter/ServiceStarter). In either case, the target will be started only once until a crash occurs (not once for each test case). In the case the target is a service, the TR needs to be attached to the service. In this case, the TR waits until the starter has finished, and then look for a process with the same path and name as the one specified in the *targetCMDLine*.
- **targetForks**: If your target is Linux AND you start it with a starter AND your target forks, you might want to set this parameter to true. Otherwise, leave this parameter alone. This is necessary for certain situations, such as if you want to fuzz a service that forks, closes the main process, and continues in the child process.
- **forceRestartAfter**: If your target tends to stop functioning after a while but your feeder doesn't realize this (which could happen if there is no backchannel, such as when fuzzing UDP), you can specify this value to force a restart of the target and the feeder after X iterations.
- **treatAnyAccessViolationAsFatal**: Some targets catch access violations. These access violations might be part of normal operation, or not. Setting this parameter to true makes FLUFFI treat each access violation as an access violation crash - even if the application would catch and handle it (defaults to false).
- **additionalEnvParam**: Parameter to append to the target process's environment (Linux only), e.g. `LD_PRELOAD=/my/shared/obj.so`. Only has an effect if no starter is used.

For ALL_Lin_QemuUserSingle:
- **changerootTemplate**: The path to the root filesystem you deployed. You can specify it relative to the location of the TestcaseRunner OR absolute. It will be used to changeroot into for running the emulated binary, and should therefore contain all necessary libraries and files, including the target binary in a known location. Currently, it must also contain the qemu-fluffi binary that will be used for emulation. Please note that the directory will not be directly used as changeroot target. Instead it will be copied to a separate location. This way, the original will not be modified and can be used again.
- **targetCMDLine**: The command line to execute INSIDE THE CHROOT environment AFTER CHROOTing. use ABSOLUTE PATHS inside the chroot.
	- **qemu binary path**: RELATIVE TO ROOTFS ususally `/bin/qemu-<ARCH>`
	- **qemu trace options(DEPRECATED)**: `-trace events=/etc/qemu-events -d nochain` (usualy no need to change this)
	- **qemu-fluffi parameter to forward signals to the target ( crash-on-signal else)**: `-f`
	- **qemu switch to pass environment vars to the target**: `-E LD_PRELOAD=/my/shared/obj.so`
	- **path to the target executable**: `/home/user/fuzztarget`
	- **parameters to pass to the target**: `-v -d --please-do-not-crash --config /home/user/conf.conf --read-in`
	- note that the system will pass the path to the test case file as a last parameter
	(eg. `[...] --read-in /fluffi/testcase/00235.bin` in above example)

- **hangTimeout**: Duration in milliseconds after which a test case execution will be considered to have timed out if it did not yet complete.
- **suppressChildOutput**: Many targets output either to stdout or open windows. For debugging purposes, it makes sense to set this to false. For production it should be set to true.
- **populationMinimization**: The database will mark population items with exactly the same covered blocks as duplicates and ignore them from then on. By default this is set to *false*, but can be set to *true*.
- **treatAnyAccessViolationAsFatal**: Some targets catch access violations. These access violations might be part of normal operation, or not. Setting this parameter to true makes FLUFFI treat each access violation as an access violation crash - even if the application would catch and handle it (defaults to false).

For ALL_GDB:
- **targetCMDLine**: The command line to start the GDB (e.g. `"C:\FLUFFI\SUT\my target\gdb.exe"` on Windows or `/home/<FluffiUser>/fluffi/persistent/SUT/my target/gdb` on Linux). If you you use [kFuzz for FLUFFI](core/helpers/kfuzz), specify the path of the GDBEmulator binary. Remember: The command line needs to be absolute in any case!
- **hangTimeout**: Duration in milliseconds after which a test case execution will be considered to have timed out if it did not yet complete.
- **suppressChildOutput**: Many targets and feeders output either to stdout or open windows. For debugging purposes, it makes sense to set this to false. For production it should be set to true.
- **populationMinimization**: The database will mark population items with exactly the same covered blocks as duplicates and ignore them from then on. By default this is set to *false*, but can be set to *true*.
- **feederCMDLine**: The command line to start the feeder (e.g. `"C:\FLUFFI\SUT\my target\TCPFeeder.exe" 80`). The name for the SharedMemory Location (as accessed in the Feeder's source code) will be added automatically (for the interface between FLUFFI<->Feeder). IMPORTANT: Due to how the communication with GDB is implemented, the feeder MUST ignore Ctrl+C on Windows (`SetConsoleCtrlHandler(NULL, true);` in C); 
- **initializationTimeout**: Duration in milliseconds after which the feeder needs to have reported to the runner that it can talk to the target. If the timeout has passed, both target and feeder will be restarted.
- **starterCMDLine**: You need to specify a program here that starts the target (e.g. `"C:\FLUFFI\SUT\my target\GDBStarter.exe -local C:\FLUFFI\SUT\my target\actual target.exe"`), and creates a GDB initialization file so that the GDB can connect to the target. The starter will be executed only once until a crash occurs (not in each runner pass). Examples for GDB starters are implemented in [GDBStarter](core/helpers/Starter/GDBStarter).
- **forceRestartAfter**: If your target tends to stop functioning after a while but your feeder doesn't realize this (which could happen if there is no backchannel, e.g. when fuzzing UDP), you can specify this value to force a restart of the target and the feeder after X iterations.
- **breakPointInstruction**: The target architecture's break point instruction IN HEXADECIMAL ENCODING (e.g. 0xCC on x86/x64). If you don't know yours, grep the gdb server sources for your architecture, for example [here](https://chromium.googlesource.com/chromiumos/third_party/gdb/+/3a0081862f33c17f3779e30ba71aacde6e7ab1bd/gdb/gdbserver)
- **breakPointInstructionBytes**: Number of bytes of breakPointInstruction. Must be 1,2, or 4

**PLEASE NOTE**: All options can be changed while the job is running. You will however need to restart the agents as they only pull the options upon start.

**Target Modules**

FLUFFI is a coverage-based evolutionary fuzzer. In order to reduce the noise in the coverage, only those modules that are listed here will be used to calculate the coverage. Examples for modules are `test.dll`, or `target.exe`.

When creating or modifying a project, you can upload the target modules as files. Their filename and binary will be saved and their path will be initialized with a `*`.
Therefore, you can edit them in the project view by renaming the filename, path and/or uploading a new binary. 
FLUFFI itself only uses the filename but some generators will use the binary. 

If there are several modules with the same name but different paths, you can specify which one should be used for coverage calculation by specifying the path. If it is left to `*`, the path of a module is ignored.

**PLEASE NOTE**: If you are using a GDB runner, the module name is actually a segment name. An example therefore is `target.exe/.text` 

**Target Upload**
See section 4) below.

**Population**

Each evolutionary fuzzer needs a set of known-to-be-good input samples to start with. These need to be uploaded here. You can specify more than one at a time. Furthermore, you can always add more while the project is running.

**Basic Blocks**

Only relevant for ALL_GDB runner.
These are the blocks that will be considered for coverage collection. FLUFFI expects a file with the following format:

```
target.exe/.text,0x1 
target.exe/.text,0x2 
helper.dll/.text,0x100 
```

If you want, you can alternatively insert the blocks in the database table `blocks_to_cover` yourself (e.g. by using [this](ida_scripts/exportBBs.py) IDA python script).

### TestcaseGenerator

Which generator types should be used is set by the generatorTypes parameter. It is a string of all types to use followed by their percentage. If you want to use only one generator of type A, set `A=100`. If you want to use two with the same probability, set `A=50|B=50`. For possible type values see `myAgentSubTypes` in [TestcaseGenerator.cpp](core/TestcaseGenerator/TestcaseGenerator.cpp).

**PLEASE NOTE** that the LocalManagers will try to stick as close to your setting as possible. However, if that is not possible (for example, if only generators that have only type A implemented register), the ratio might not be as desired.

### TestcaseEvaluator

Which evaluator types should be used is set by the evaluatorTypes parameter. It is a string of all types to use followed by their percentage. If you want to use only one evaluator of type A, set `A=100`. If you want to use two with the same probability, set `A=50|B=50`. For possible type values see `myAgentSubTypes` in [TestcaseEvaluator.cpp](core/TestcaseEvaluator/TestcaseEvaluator.cpp).

**PLEASE NOTE** that the LocalManagers will try to stick as close to your setting as possible. However, if that is not possible (for example, if only evaluators that have only type A implemented register), the ratio might not be as desired.

## 4) Deploy the FuzzJob on the target machines


All FuzzJobs are run on dedicated Runner systems in the FLUFFI Utility Network (FUN). Your test target, which should be wrapped as a *package*, needs to be deployed to these Runner systems.

A package is a zip file containing either an `install.bat`, an `install.ps1`, or an `install.sh` file as well as arbitrary data. When you deploy this package, it is copied to the FLUFFI runner systems, extracted (on Windows to  `C:\fluffi\SUT\<ZipFileName>\`, on Linux to `/home/<FluffiUser>/fluffi/persistent/SUT/<ZipfileName>\`), and the corresponding install script is executed. You should prepare the package so that it installs all the required dependencies for the target as well as the target itself.

If you create a Windows package, you should add page heap checks using gflags for the target binary. This allows FLUFFI to better detect access violations in the heap. As gflags is by default copied to all agent systems, you only need to add a line to your `install.ps1` or `install.bat` such as: `C:\utils\GFlags\x86\gflags.exe /p /enable TargetBinary.exe`

The package needs to be copied somehow to the FLUFFI ftp server (ftp.fluffi). You have two options to do so. Option one is you connect directly to the ftp server using anonymous login, and place the package in the SUT folder. Option two is, you upload the package while creating the FuzzJob. It will then be placed in that very folder. Additionally, it will be associated with the FuzzJob.


Once the package is on the FTP server, you can deploy it to runner systems. You can do so in the `Systems` tab by selecting a system or group and chosing the 'Deploy SUT/Dependency' option (SUT stands for Software Under Test).


## 5) Start the FLUFFI Agents

Currently there are two ways of how you can manage how many agents (LM, TR, TE, TG)  run on which system, and  work on which FuzzJob.

### Manually

You can connect to the systems directly (SSH, RDP), and start the agents there.

The credentials to do so can be looked up in polemarch's [hosts](srv/fluffi/data/polenext/projects/1/hosts) file.
On Windows, start one LM (if there is not already one for your FuzzJob, such as on another machine in the same location) and as many TRs / TGs / TEs as you like, e.g. by clicking on the icons on the Desktop. On Linux, just start the appropriate agent binary from the command line, ensuring that you add the location name as the argument for the agent.

After having been started, LMs register at the GlobalManager (GM) and keep asking for a yet unmanaged FuzzJob in their location. As soon as such a FuzzJob becomes available, the LM will start managing it.

TRs / TGs / TEs will connect to the GM and wait until they are assigned a FuzzJob. This can be done in the Location page of the web application. Once a FuzzJob is assigned, they will connect to the LM in their location managing that FuzzJob.

IMPORTANT: When managing agents manually, you need to disable the agent manager by setting it to `INACTIVE` in the `Settings` button on the `Systems` tab.

### Automatically

FLUFFI implements a so called agent manager. This manager checks how many agents should be run on which machine and will enforce that this is actually the case. It runs periodicly every few minutes (so you need a little patience after configuring new instances).


You can control the number of running FLUFFI agents (LM, TR, TE, TG) via the web GUI.

To do so you need go to one of the following locations: 
- the `Config System Instances` section on the FuzzJob overview page
- the properties of a group in the `Systems` tab (click on a group name)
- the properties of a system in the `Systems` tab (click on a system name)

IMPORTANT:  When managing agents with the agent manager, you need to enable the agent manager by setting it to `ACTIVE` in the `Settings` button on the `Systems` tab. Setting it to `KILL` will kill all currently running agents. As a result, you can easily replace FLUFFI or targets binaries.

Note: The agent manager is implemented by two components: A periodic [polemarch task](srv/fluffi/data/polenext/manageAgents.py), and a [python REST server](srv/fluffi/data/ftp/files/restStarter) that is deployed to the runner machines and waits for commands to execute. The latter is one of the reasons why you should NEVER run FLUFFI in an untrusted environment.

## 6) Monitoring FLUFFI Agents

You should always monitor your FuzzJob.

First of all, you should make sure that the number of executions and the number of covered blocks is rising. If one of them doesn't there is something wrong with your setup.

Furthermore, in the `Managed Instances` view make sure that:
1. The systems are not overloaded (e.g. CPU>90%),
2. The TG queues are not close to 0, and
3. The TE queues are not growing and growing.

You can stop running agents by clicking the `Managed Instances` button when viewing the FuzzJob overview page.

Finally, you should keep one eye on log messages. Currenlty, FLUFFI agents store all ERROR messages in the database. They can then be displayed in the web application (for agents in the `Managed Instances` view, for LMs there is a global tab).
