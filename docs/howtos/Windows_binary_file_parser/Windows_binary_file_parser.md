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
As penetration testers / red teamers we occasionally face the task to find security vulnerabilities in Windows file parser binaries, for which we do not have any source code. In this article, we will walk you through how this could be done with our self-developed fuzzer [FLUFFI](https://github.com/siemens/fluffi).

Prior to FLUFFI, there already were Windows binary fuzzers around, such as [WinAFL](https://github.com/googleprojectzero/winafl). However, at least at the time we started our development, all of them lacked a couple of features, that we painfully missed (and tbh: most of them still seem to be FLUFFI-unique features):

- Support for custom Feeders
- Support for fuzzing of Windows services
- Support for programs that don't have a `parse(input)` function (e.g. as the parse function itself calls `read` multiple times)
- Scaling accross multiple machines
- Blue-Screen data persistance

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

# Fuzzing trailofbits' dump-pe
So, what's a good binary file parser to start with? While searching for a good example to write a tutorial about, I stumbled over trailofbits' [dump-pe](https://github.com/trailofbits/pe-parse/tree/master/dump-pe). It's a fairly light wight parser of a rather complex file format (PE files). So why not giving it a try?

I hear you crying: "But this is not a binary! You've got sources for that!". Yes, I know, I know. But for the sake of this tutorial let's assume we only got the compiled version of dump-pe: `dump-pe.exe`, and `pe-parser-library.dll`.

### Creating the fuzz setup deployment package
Building the deployment package for our fuzzjob is quite simple. First you need to create a `install.bat` that enables page heap for `dump-pe.exe`. To do so create a new file, name it `install.bat`, and put the following line in it:

```
C:\utils\GFlags\x64\gflags.exe /p /enable dump-pe.exe
```

Now, just put the `install.bat`, the `dump-pe.exe`, and the `pe-parser-library.dll` (not the folder containing them) into a `dump-pe.zip`:

![The file structure of our deployment package](deployment-package.png )

### Creating the FLUFFI fuzz job
What is left to do now is creating the actual FLUFFI fuzzjob and starting it. The process of creating such a fuzz job is documented [here](../../../docs/usage.md). Let's just walk through this, shall we?

Firstly, we point our browser to [web.fluffi](http://web.fluffi) and click on `Fuzzjobs` -> `Create Fuzzjob`:

![Creating a new Fuzzjob in the web interface](newFuzzjob.png)

Now let's walk through all the options:

-   Name: pedump
-   Runner Type: `X64_Win_DynRioSingle`.
-   Generator Type: Select which generators should be used how often. In our experience the smartest thing to do is using as many different generator types as possible, as they complement each other. Make sure that the usage percentage adds up to 100.
-   Evaluator Types: Currently there is only one. Make its usage 100 per cent.
-   Location: Select the location of your runner machines
-   Target Command Line: The location of the `dump-pe.exe` Should be `C:\FLUFFI\SUT\dump-pe\dump-pe.exe`
-   Options:
    -   hangTimeout: 1000
    -   suppressChildOutput: true
    -   populationMinimization: true
-   Target Modules: We want to fuzz the  `dump-pe.exe`, and the `pe-parser-library.dll`. So select these file here.
-   Target Upload: Select the `dump-pe.zip` you created earlier
-   Population: Select a bunch of valid pe files. This will be the starting population that will be mutated until crashes are found

Now Press the green `FLUFFI FUZZ` button.

FLUFFI will now create a fuzz job for you, and store all of the information you entered either in its database, or its ftp server (the deplyoment zip).

### Starting the FLUFFI fuzz job
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


If you are in the lucky position to have an IDA Pro license, you can even visualize which code blocks were already covered during your fuzzing run. To do so open a module for which coverage is being collected (e.g. the  `pe-parser-library.dll` in IDA Pro. Then go to `File` -> `Script File` and select the [cov2ida.py](../../../ida_scripts/cov2ida.py). Now you have to select the fuzz job you are working on and the correct module.

The result will look similar to this:

![Visualizing the code coverage in IDA](coverageInIDA.png)

Light-blue functions are functions in which some code blocks have been covered. Dark-blue blocks are blocks that have been covered. Light-blue blocks are blocks that belong to a function in which code blocks have been covered but were not covered themselves.

