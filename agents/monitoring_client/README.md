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

# FLUFFI monitoring agent

## Building

To build FLUFFIs monitoring:

Run the build.\* files, these will create a zip file, which can be used with the deployment via Polemarch.

###Linux:

Make sure you are not running as root
```
cd linux
./build.sh
```

###Windows:
```
cd windows
Powershell.exe -executionpolicy bypass -File "%~dp0build.ps1"
```

## Usage

To activate the plugin, you can either
- Copy the generated monitoring agent to the agent system and execute the install script or
- Place the generated zip files on ftp.fluffi in /initial/linux/activePackages and /initial/windows/activePackages and wait for the Polemarch init script to do this for you
