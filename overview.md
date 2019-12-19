<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
-->

# High level overview

FLUFFI is a distributed system. Within this system, so-called agents (e.g., TestcaseRunners, TestcaseGenerators) are distributed to executor machines. The fuzzing target needs to be installed on these machines.

The agents are coordinated by a GlobalManager running on a central server. A DHCP/DNS server, a solution to deploy software to executor machines (consisting of a polemarch installation, an ftp and an smb server), a monitoring solution, and some helper services all run on the same central server.

FLUFFI is supposed to be set up in its own subnet with the central server having the static IP address 10.66.0.1.

This repository contains 
- the code of the distributed system ([here](core))
- a build environment to build the distributed system agents for Windows(x64/x86) and Linux(x64/x86/ARM/ARMarch64) ([here](build))
- the central server infrastructure in the form of docker containers ([here](srv))
- a monitoring plugin to monitor agent systems ([here](monitoring_client))
- some IDA Pro scripts that help you analyze FLUFFI's fuzzing process ([here](ida_scripts))
