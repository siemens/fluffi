<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Pascal Eckmann
-->

# PXE

> __! work in progress !__
> Some of the instructions aren't completely finished yet.

## Infrastructure

All required systems are executed when _docker-compose_ is started with the specified [configuration](../../srv/fluffi/docker-compose.yaml).    
__On _web.fluffi_ you can select the OS, which should be loaded over PXE.__    

- ##### FTP
    - `ftp.fluffi/tftp-roots/...`    
        - e.g. `ubuntuRoot/...`   
        -> copy your systems here, detailed steps are in the instructions for each operating system
- ##### TFTP
    - If you select an OS on _web.fluffi_, it will be copied here and the system which should get a fresh OS will pick the necessary data from here
- ##### SMB
    - `smb.fluffi/install/...`
        - `initial/` -> hostname, etc. configuration for fresh deployed systems
        - e.g. `server2016/` -> Windows image, detailed steps are in the instruction for [Windows](windows.md)

## Agent systems OS
- ##### Windows __! work in progress !__
    - [Create and configure Windows image for PXE](windows.md)
- ##### Ubuntu
    - [Create and configure Ubuntu image for PXE](ubuntu.md)
- ##### Armbian/Odroid-XU4 __! work in progress !__
    - [Install and configure Odroid-XU4 for PXE](odroid.md)
    - [Create and configure Armbian image for PXE](armbian.md)