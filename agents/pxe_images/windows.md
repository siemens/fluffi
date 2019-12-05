<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Pascal Eckmann
-->

# Microsoft Windows

This instruction is tested with Microsoft Windows Server 2016.

## Prepare WinPE image
1. Install the [Windows Assessment and Deployment Kit (ADK)](https://docs.microsoft.com/en-us/windows-hardware/get-started/adk-install) (preferably on a freshly installed Windows) with standard settings
    - Install ADK
    - Install ADK-Windows PE-Addon 
2. Run `Windows Kit/Deployment and Imaging Tools Environment` from the start menu to open the CLI ...
3. ... and execute there following commands to create Windows PE step for step
    - `copype.cmd amd64 C:\winpe_amd64`
    - `dism /mount-image /imagefile:c:\winpe_amd64\media\sources\boot.wim /index:1 /mountdir:C:\winpe_amd64\mount`
    - `xcopy c:\winpe_amd64\mount\windows\boot\pxe\*.* c:\PXE\boot\`
    - `copy C:\winpe_amd64\media\boot\boot.sdi c:\PXE\boot\`
    - `copy C:\winpe_amd64\media\sources\boot.wim c:\PXE\boot\`
    - `bcdedit /createstore C:\PXE\BCD`    
    _-> maybe use other command line to get other version of bcedit_
    - `bcdedit /store c:\PXE\BCD /create {ramdiskoptions} /d "Ramdisk options"`
    - `bcdedit /store C:\PXE\BCD /set {ramdiskoptions} ramdisksdidevice boot`
    - `bcdedit /store C:\PXE\BCD /set {ramdiskoptions} ramdisksdipath \boot\boot.sdi`
    - `bcdedit /store C:\PXE\BCD /create /d "winpe boot image" /application osloader`    
    _-> use the GUID resulted in here, e.g. `bcdedit /store C:\PXE\BCD /set {00000000-0000-0000-0000-000000000000} device [...]`_

    - `bcdedit /store C:\PXE\BCD /set {GUID} device ramdisk=[boot]\boot\boot.wim,{ramdiskoptions}`
    - `bcdedit /store C:\PXE\BCD /set {GUID} path \windows\system32\winload.exe`
    - `bcdedit /store C:\PXE\BCD /set {GUID} osdevice ramdisk=[boot]\boot\boot.wim,{ramdiskoptions}`
    - `bcdedit /store C:\PXE\BCD /set {GUID} systemroot \windows`
    - `bcdedit /store C:\PXE\BCD /set {GUID} detecthal Yes`
    - `bcdedit /store c:\PXE\BCD /set {GUID} winpe Yes`
       
    - `bcdedit /store C:\PXE\BCD /set {GUID} device ramdisk=[boot]\boot\boot.wim,{ramdiskoptions}`
    - `bcdedit /store C:\PXE\BCD /set {GUID} path \windows\system32\winload.exe`
    - `bcdedit /store C:\PXE\BCD /set {GUID} osdevice ramdisk=[boot]\boot\boot.wim,{ramdiskoptions}`
    - `bcdedit /store C:\PXE\BCD /set {GUID} systemroot \windows`
    - `bcdedit /store C:\PXE\BCD /set {GUID} detecthal Yes`
    - `bcdedit /store C:\PXE\BCD /set {GUID} winpe Yes`
    
    - `bcdedit /store C:\PXE\BCD /create {bootmgr} /d "boot manager"`
    - `bcdedit /store C:\PXE\BCD /set {bootmgr} timeout 30`
    - `bcdedit /store C:\PXE\BCD -displayorder {GUID} -addlast`
4. Manipulating the `boot.wim` of the new Windows PE image
	- Mount the image
	    - Execute: `mkdir C:\mount\boot\`
	    - Execute: `dism /Mount-Wim /wimfile:C:\PXE\boot\boot.wim /index:1 /MountDir:C:\mount\boot\`
	- Navigate to `C:\mount\boot\windows\system32\` and add/change a file `Startnet.cmd` with following content, based on your settings ([windows] is a placeholder for your folder containing the windows image on the SMB share):
		```
		wpeinit
		net use y: \\smb.fluffi\install\[windows] /user:nobody pass
		Y:
		setup.exe /Unattend:Y:\unattend.xml
		```
	- Unmout the image and commit changes by executing `dism /Unmount-Wim /MountDir:C:\mount\boot /commit`
    
## Download Windows
__! work in progress !__

## Copy Windows image to infrastructure
__! work in progress !__
	
## Configure Windows for unattended installation
- To automate the Windows installation an `unattend.xml` file should be created and copied in the [windows] directory on the SMB share, where `setup.exe` is.
- You can use the provided `unattend.xml` files for [Windows Server 2008](windows/server2008/unattend.xml) and [Windows Server 2016](windows/server2016/unattend.xml)
    - Change [username] to the same which you defined for _ansible_user_ and [password] to the same as _ansible_password_ in the section _[windows:vars]_ in your [hosts](../../srv/fluffi/data/polenext/projects/1/hosts) file for Polemarch and [key] to your product key
   
## Copy Windows PE image to infrastructure
__! work in progress !__
