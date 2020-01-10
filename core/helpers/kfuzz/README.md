<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome
-->

# kFuzz for FLUFFI

kFuzz (short for kernel space fuzzer) is a coverage based fuzzer for Windows kernel space modules, such as Windows drivers.

`kFuzz for FLUFFI` is the integration of the principles behind kFuzz into FLUFFI.

## Internals

The idea is to use WinDBG as a backend to collect coverage, just in the same way GDB is used when targeting devices. However, instead of implementing a full WinDBG-FLUFFI integration, a GDB-to-WinDBG adapter is used. This adapter consists of two parts: [GDBEmulator](GDBEmulator), a binary simulationg a GDB interface, and [kfuzz-windbg](kfuzz-windbg), a plugin for WinDBG. GDBEmulator forwards all requests it receives over a shared memory communication channel to kfuzz-windbg. Furthermore, all responses and exceptions are forwarded from kfuzz-windbg to GDBEmulator and displayed there in a GDB-like syntax.

With the help of this adapter, Windows kernel space modules can be fuzzed with FLUFFI's ALL_GDB runner.

## Dependencies
 - [VMWare Workstation](https://www.vmware.com/de/products/workstation-pro.html), or [VirtualBox](https://www.virtualbox.org/) (we prefer VMWare Workstation over VirtualBox, as the VirtualKD integration seems to be more stable)
 - [VirtualKD](https://sysprogs.com/legacy/virtualkd/)
 - [WinDBG Preview](https://www.microsoft.com/store/productId/9PGJGD53TN86) (important - not the old WinDBG). To distribute WinDBG Preview to FLUFFI machines download it once with any internet-connected Windows 10 machine, and copy its folder to your target machines.

## Usage

1. Create a starter (e.g. by  adapting the [kFuzzStarter](../Starter/kFuzzStarter)), that 
  - Kills vmmon
  - Kills windbg
  - Starts vmmon
  - Switches of your target vm
  - Restores a target vm snapshot (if desired)
  - Starts your target vm
  - Starts WinDBG Preview with the kfuzz-windbg plugin and connects it to the target vm
2. Create a Feeder (e.g. by adapting the [TCPFeeder](../Feeder/TCPFeeder), or by using Shared Folders), that
  - Runs outside the target vm
  - Causes the target vm to process a testcase
  - Notifies FLUFFI, as soon as the processing is done
  - Ignores Ctrl+C (`SetConsoleCtrlHandler(NULL, true);` in C)
3. Set up a Fuzzjob
  - Select **runner_type** ALL_GDB
  - Set all options required for that runner type
