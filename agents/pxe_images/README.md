<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Pascal Eckmann
-->

# PXE

## Infrastructure

All required systems (FTP, TFTP, SMB) are available if _docker-compose_ has been started with the specified [configuration](../../srv/fluffi/docker-compose.yaml).    
__On _web.fluffi_ you can select the OS, which should be loaded over PXE.__    

- ##### FTP
    - USE `Binary` as transfer type for copying files to FTP
        - If you use _Filezilla_, open _Settings \> Transfers \> File Types_ and set the _Default transfer type_ to _Binary_
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
- ##### Windows
    - [Create and configure Windows image for PXE](windows.md)
- ##### Ubuntu
    - [Create and configure Ubuntu image for PXE](ubuntu.md)
- ##### Armbian/Odroid-XU4
    - [Install and configure Odroid-XU4 for PXE](odroid.md)
    - [Create and configure Armbian image for PXE](armbian.md)
    
## Add MAC address and hostname for autmatic configuration
- Add the MAC address with the associated hostname to the [MAC2Host.csv](../../srv/fluffi/data/smb/files/initial/MAC2Host.csv) file on SMB share
    - _Attention: Only use upper case letters in MAC address_
    - __Odroid__: Additional add your MAC adress and your hostname to the [ethers](../../srv/fluffi/data/dnsmasq/ethers) file from _dnsmasq_ with following style: `00:11:22:33:44:55 odroidHostname`
    
## Add virtual machine to Fluffi
- Setup VM (tested with VMware Workstation Pro)
    - Use BIOS instead of UEFI
    - Bridge your network device to the VM to use PXE properly
    - Select OS in _webgui_ and boot your emtpy VM (it should now load the OS over PXE)
- Now you can integrate the system in Fluffi according to [usage.md](../../usage.md)
