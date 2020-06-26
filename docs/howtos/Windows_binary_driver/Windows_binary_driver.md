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
As penetration testers / red teamers we occasionally face the task to find security vulnerabilities in Windows driver binaries, for which we do not have any source code. In this article, we will walk you through how this could be done with our self-developed fuzzer [FLUFFI](https://github.com/siemens/fluffi).

Prior to FLUFFI, there already were a couple of Windows driver fuzzers around, such as [ioctlbf](https://github.com/koutto/ioctlbf), and [ioctlfuzzer](https://code.google.com/archive/p/ioctlfuzzer/). Those fuzzers, however, lacked a couple of features, that we painfully missed:

-   Support for fuzzing input other than ioctl (e.g. via network, or via a file).
-   Mutational/evolutionary fuzzing (allows effective fuzzing of proprietary formats)
-   Scaling across multiple machines
-   Blue-Screen data persistence

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



# !!!!!!!!!!! The rest of this article waits for a GO from the dev team of the driver we fuzzed. It won't be published before !!!!!!!!!!!!!!!
