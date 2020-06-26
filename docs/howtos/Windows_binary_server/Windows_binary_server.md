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

Author(s): Thomas Riedmaier
-->

# Introduction
As penetration testers / red teamers we occasionally face the task to find security vulnerabilities in Windows server binaries, for which we do not have any source code. In this article, we will walk you through how this could be done with our self-developed fuzzer [FLUFFI](https://github.com/siemens/fluffi).

Prior to FLUFFI, there already were Windows binary fuzzers around, such as [WinAFL](https://github.com/googleprojectzero/winafl). However, at least at the time we started our development, all of them lacked a couple of features, that we painfully missed (and tbh: most of them still seem to be FLUFFI-unique features):

- Support for custom Feeders
- Support for fuzzing of Windows services
- Support for programs that don't have a `parse(input)` function (e.g. as the parse function itself calls `read` multiple times)
- Scaling across multiple machines
- Blue-Screen data persistence

FLUFFI on the other hand is a mutational/evolutionary fuzzer, that was designed as some kind of swiss-army knife with respect to fuzz targets. With FLUFFI a tester can fuzz

-   Windows, as well as Linux targets
-   File parsing binaries, as well as servers or APIs
-   User-mode targets, as well as kernel drivers
-   A huge variety of processor architectures, including x64, x86, ARM, MIPs, and PowerPC

In short: FLUFFI can fuzz pretty much anything. BONUS: If you give FLUFFI a way to get code coverage for your target, FLUFFI will use mutational/evolutionary testcase generation to do so.

# Some background on fuzzing with FLUFFI
Before fuzzing with FLUFFI, one first needs to [set up a FLUFFI fuzzing environment](../../getting_started.md). Admittedly, this is a time consuming first step, as several machines and services need to be configured. However, once this is done, one can start fuzzing almost anything within a few hours (easy targets, such as file parsers, within a few minutes).

With FLUFFI, fuzzing is never done on your own machine but on dedicated runner machines. The reasoning behind that is that binary fuzzing is an extremely messy thing to do. The machines doing so tend to act funny after a while (parts of the Desktop not working any more on Windows, no more file handles available on Linux, ...).

As a result, FLUFFI uses the concept of `runner` machines. These are machines (physical or virtual) that run / execute the actual fuzz tests. In other words: these are the machines on which testcases are somehow processed by fuzz targets. All other steps of the fuzzing process, such as testcase generation, testcase evaluation, or testcase management take place on different machines.

FLUFFI comes with a set of mechanisms that allow automated setup of those runner machines. Once this is done, the fuzz targets need to be installed on the runner machines. In order to support scaling to multiple machines, FLUFFI uses so called `deployment packages` to do so. These packages are essentially zip files that are copied to the runner machines. Once there, they are unpacked and an installation script (e.g. an `install.bat`, or an `install.sh`) is executed.

# Fuzzing ftpdmin
So, what's a good binary server to start with? While searching for a good example to write a tutorial about, I stumbled over Matthias Wandel's [ftpdmin](http://www.sentex.net/~mwandel/ftpdmin/ftpdmin.exe). It's a super simple FTP server. FTP is a nice protocol do demonstrate the usage of Feeders. So why not giving it a try?

I hear you crying: "But you've got sources for that!". Yes, I know, I know. But for the sake of this tutorial let's assume we only got the compiled version of ftpdmin: `ftpdmin.exe`.

### Creating a Feeder
First, we need to tell FLUFFI how to hand testcases to the target. To hand testcases to long running processes (such as servers and drivers), FLUFFI uses the concept of Feeders. A Feeder is a piece of software, that implements a standardized FLUFFI interface on the one hand and the target's interface on the other hand. As FTP is based on TCP, we will create a feeder based on the [TCPFeeder](../../../core/helpers/Feeder/TCPFeeder). 

Network protocols are usually a state machine: The initial state is reached after the connection is established. The next state might subsequently be reached after encryption is set up. A further step might then be reached after the client authenticated to the server. Each state accepts different input messages.

In theory, one could now write a Feeder for each state and start seperate fuzz jobs, one for each state. This, however, would be rather inefficient. What we will do instead, is implementing the code to fuzz multiple states in a single Feeder.

This can be done by prepending each testcase with one byte that is used as a testcase type identifier. The Feeder will strip this testcase type identifier from each input, connect to the target server, send all necessary messages to enter the state specified by the testcase type identifier, and finaly send the Feeder's input minus the testcase type identifier.

The code for all of this is already implemented in the TCPFeeder. All that we need to do is define the testcase type identifier we want to use and implement the code to reach the states.

To do so let's have a look at this sample FTP session:

![Sample FTP connection](SampleConnection.png )

As you can see, the client first logs in with his username and password. Afterwards, the session is in some kind of 'authenticated' state, in which various different commands can be sent by the client and will be processed by the server.

For our Feeder we are going to define two test case types: Type `0`, which means 'Send the input directly to the server', and type `1`, which means 'Log in as anonymous user and then send the input to the server'.


This requires merely minimal changes to the default [TCPFeeder.cpp](../../../core/helpers/Feeder/TCPFeeder/TCPFeeder.cpp): just replace the body of the `sendTestcaseToHostAndPort` with the following:

```
Packet h0(std::vector<char>{(char)0x55, (char)0x53, (char)0x45, (char)0x52, (char)0x20, (char)0x61, (char)0x6e, (char)0x6f, (char)0x6e, (char)0x79, (char)0x6d, (char)0x6f, (char)0x75, (char)0x73, (char)0x0d, (char)0x0a}, WHAT_TODO_AFTER_SEND::WAIT_FOR_BYTE_SEQUENCE, std::vector<char>{(char)0x61, (char)0x63, (char)0x63, (char)0x65, (char)0x70, (char)0x74, (char)0x65, (char)0x64, (char)0x0d, (char)0x0a});
Packet h1(std::vector<char>{(char)0x50, (char)0x41, (char)0x53, (char)0x53, (char)0x20, (char)0x61, (char)0x6e, (char)0x6f, (char)0x6e, (char)0x79, (char)0x6d, (char)0x6f, (char)0x75, (char)0x73, (char)0x40, (char)0x65, (char)0x78, (char)0x61, (char)0x6d, (char)0x70, (char)0x6c, (char)0x65, (char)0x2e, (char)0x63, (char)0x6f, (char)0x6d, (char)0x0d, (char)0x0a}, WHAT_TODO_AFTER_SEND::WAIT_FOR_BYTE_SEQUENCE, std::vector<char>{(char)0x6c, (char)0x6f, (char)0x67, (char)0x67, (char)0x65, (char)0x64, (char)0x20, (char)0x69, (char)0x6e, (char)0x0d, (char)0x0a});
bool sendTestcaseToHostAndPort(std::vector<char> fuzzBytes, std::string targethost, int serverport) {
		//The first byte tells us what to do.
		//This allows us combining multiple testcases into a single feeder, that can even be adjusted over time :)

		if (fuzzBytes.size() < 1) {
			//Invalid mutation
			return true;
		}

		char firstbyte = fuzzBytes[0];
		fuzzBytes.erase(fuzzBytes.begin());

		switch (firstbyte) { //range -127 ... 128
		case 0:
			//Send directly
			return sendBytesToHostAndPort(fuzzBytes, targethost, serverport);
		case 1:
		{
			//Do a login first, then send the packet
			Packet p(fuzzBytes, WAIT_FOR_ANY_RESPONSE);
			std::vector<Packet> pvec{ h0, h1, p };
			return sendPacketSequenceToHostAndPort(pvec, targethost, serverport);
		}
		default:
			//Not implemented
			return true;
		}
}d

```

Now let's test our Feeder. Testing a FLUFFI feeder can be done best with the [Feeder Tester](../../../core/helpers/Feeder/Tester). It simulates the way FLUFFI calls a Feeder but is a light weight stand-alone console application.

To test if test case type `0` is working properly, we create the following initial population element:

![An example input of testcase type 0](direct_user_with_packet.png )

Please note, that its first byte is indeed a `0`, followed by the bytes that were sent by the client in the sample FTP session. It can be sent to the target with the following command:

```
Tester.exe --feederCmdline "TCPFeeder.exe 21" --testcaseFile .\direct_user.bin
```

The resulting network traffic will look like this:

![The network traffic caused by sending the first example input](direct_user_with_tester.png )

As you can see, the intended packet is sent by the client, and the server processes it as expected.

To test if test case type `1` is working properly, we create the following initial population element:

![An example input of testcase type 1](after_login_list_packet.png )

Please note, that its first byte is indeed a `1`, followed by the bytes of a valid FTP command. It can be sent to the target with the following command:

```
Tester.exe --feederCmdline "TCPFeeder.exe 21" --testcaseFile .\after_login_list.bin
```

The resulting network traffic will look like this:

![The network traffic caused by sending the second example input](after_login_list_with_tester.png )

As you can see, the intended packet is sent by the client, and the server processes it as expected.


### Creating the fuzz setup deployment package
Now that we have all puzzle pieces at hand, let's build the deployment package for our fuzzjob. To do so, we need to create a folder with the following content:

![The file structure of our deployment package](deploymentPackage.png)

1.  The target binary (`ftpdmin.exe`)
2.  install.bat (see below)
3.  TCPFeeder.exe: The binary we created above
4.  SharedMemIPC.dll: Copied from `core\x86\Release`

The `install.bat` should enable page heap on the target binary. To do so create a new file, name it `install.bat`, and put the following line in it:

```
C:\utils\GFlags\x86\gflags.exe /p /enable ftpdmin.exe
```

All that is left to do now is zip all files and folders (not the folder containing them) into a `ftpdmin.zip` 


### Creating the FLUFFI fuzz job
What is left to do now is creating the actual FLUFFI fuzzjob and starting it. The process of creating such a fuzz job is documented [here](../../../docs/usage.md). Let's just walk through this, shall we?

Firstly, we point our browser to [web.fluffi](http://web.fluffi) and click on `Fuzzjobs` -> `Create Fuzzjob`:

![Creating a new Fuzzjob in the web interface](newFuzzjob.png)

Now let's walk through all the options:

-   Name: ftpdmin
-   Runner Type: `X86_Win_DynRioMulti`.
-   Generator Type: Select which generators should be used how often. In our experience the smartest thing to do is using as many different generator types as possible, as they complement each other. Make sure that the usage percentage adds up to 100.
-   Evaluator Types: Currently there is only one. Make its usage 100 per cent.
-   Location: Select the location of your runner machines
-   Target Command Line: `C:\FLUFFI\SUT\ftpdmin\ftpdmin.exe -p <RANDOM_PORT> .`. Do not replace `<RANDOM_PORT>` with anything. Leave it as it is. It will be replaced with a random available port at runtime. This allows running multiple runner instances on a single runner machine. Servers that do not have this flexibility need to be either patched or run on a dedicated runner machine each.
-   Options:
    -   hangTimeout: 1000
    -   suppressChildOutput: true
    -   populationMinimization: true
    -   feederCMDLine: The location of our feeder. Should be `C:\FLUFFI\SUT\ftpdmin\TCPFeeder.exe` (note: no port parameter required. The Feeder will determine the port automaticly.)
    -   initializationTimeout: 3000
-   Target Modules: We want to fuzz the `ftpdmin.exe`. So select this file here.
-   Target Upload: Select the `ftpdmin.zip` you created earlier
-   Population: Select the initial testcases we created above. This will be the starting population that will be mutated until crashes are found. The results will be even better if you give FLUFI more starting points to start mutating from (e.g. more testcases of type 1 with different FTP commands).


Now Press the green `FLUFFI FUZZ` button.

FLUFFI will now create a fuzz job for you, and store all of the information you entered either in its database, or its ftp server (the deplyoment zip).

Please keep in mind that you can always update the deployment zip and redeploy it, in case you want to make updates, e.g. to the Feeder.

### Deploying and starting the FLUFFI fuzz job
Please keep in mind, that there is still no fuzzing going on, as FLUFFI does not know yet, on which runner machines you want to fuzz on. 

Talking about runner machines: Your deployment package was not yet deployed to your runner machines. You should do this now by clicking on `Systems` in FLUFFI's web GUI, selecting your target machine (or its group), and then deploy your fuzz job's deployment package by selecting your fuzzjob in the `Deploy SUT/Dependency` tab.

![Deploying the deployment package from the web page](deployPackageToSystem.png)

Once this is done, we need to configure where the agents needed by your fuzz job should be run. At the very least, each fuzz job requires a LocalManager, a TestcaseEvaluator, a couple of TestcaseRunners (in our setup MULTIPLE PER MACHINE are fine), and a couple of TestcaseGenerators.

As already mentioned above, binary fuzzing is a really messy thing to do. Consequently, we recommend running the LocalManager, the TestcaseGenerators as well as the TestcaseEvaluator on a different machine than the TestcaseRunners. If you are running into performance issues, you can distribute your fuzz job even further.

You can configure, what agents should be run on what machine either via `Systems`->`<Machine Name>`->`Configure Instances`, or via `Fuzzjobs`->`<Fuzz job name>`->`Config System Instances`.

Once you configured this, your fuzz job is actually being executed.

# Monitoring the fuzzing process
As your fuzz job is now running, you can monitor its progress in the fuzz job's page. You will see the number of executed testcases going up, as well as the population, and the covered blocks.

If you are looking for even more information, you can click on `Project Diagrams`. Here you will be shown the progress of your fuzz process over time. 

![Monitoring the fuzzing process](PerformanceGraphs.png)

By clicking on `Population Graph`, you can see what the mutational / evolutionary algorithm is doing under the hood:

![The fuzzing process' population](PopulationGraph.png)


If you are in the lucky position to have an IDA Pro license, you can even visualize which code blocks were already covered during your fuzzing run. To do so open a module for which coverage is being collected (e.g. the `ftpdmin.exe` in IDA Pro. Then go to `File` -> `Script File` and select the [cov2ida.py](../../../ida_scripts/cov2ida.py). Now you have to select the fuzz job you are working on and the correct module.

The result will look similar to this:

![Visualizing the code coverage in IDA](coverageInIDA.png)

Light-blue functions are functions in which some code blocks have been covered. Dark-blue blocks are blocks that have been covered. Light-blue blocks are blocks that belong to a function in which code blocks have been covered but were not covered themselves.

