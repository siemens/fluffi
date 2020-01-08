<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Pascal Eckmann
-->

# Ubuntu

## Prepare Ubuntu image

1. Download netinstall version of Ubuntu, e.g. [Bionic Beaver](http://archive.ubuntu.com/ubuntu/dists/bionic/main/installer-amd64/current/images/netboot/)
    - e.g. `wget -r -np -R "index.html*" -P ./ http://archive.ubuntu.com/ubuntu/dists/bionic/main/installer-amd64/current/images/netboot/`
2. Modify the file `\pxelinux.cfg\default` to boot the installer with a preseeding file
    ```
    prompt 0
    timeout 0
    DEFAULT ubuntu
    
    label ubuntu
        menu label automated installation of ubuntu
        kernel ubuntu-installer/amd64/linux
        append initrd=ubuntu-installer/amd64/initrd.gz auto=true priority=critical keyboard-configuration/layout=de keyboard-configuration/variant=de preseed/file=/preseed.cfg ---
    ```
3. To automate the installation, prepare a preseeding file and copy it to the root of the file `\ubuntu-installer\amd64\initrd.gz`. You can use the [script](ubuntu/embedPreseed.sh) `embedPreseed.sh` to copy the preseed file in the `initrd.gz`.    
_e.g. execute: `sudo ./embedPreseed.sh ./initrd.gz /home/user/ubuntu_netinstall/ubuntu-installer/amd64/initrdNew /home/user/ubuntu_netinstall/ubuntu-installer/amd64/preseed.cfg`_    
_Attention: The last two arguments have to be absolute paths._    
After that, delete the old `initrd.gz` and rename the new ramdisk to `ìnitrd.gz` and place it into the same folder, where the old one was.    
A example preseeding file for Ubuntu 18.04 is located in this [repository](ubuntu/preseed.cfg). You have only to change [username] to the same which you defined for _ansible_ssh_user_ and [password] to the same as _ansible_ssh_pass_ in the section _[linux:vars]_ in your [hosts](../../srv/fluffi/data/polenext/projects/1/hosts) file for Polemarch. If you want to create a new one, copy all necessary data from the provided example.    

## Copy files to infrastructure
- Create a folder in `ftp.fluffi/tftp-root/`, e.g. `ftp.fluffi/tftp-root/ubuntu` and copy all Ubuntu files in the new folder:
    >&gt; __tftp-root__    
    >|&emsp;&gt; __ubuntu__    
    >|&emsp;|&emsp;&gt; __pxelinux.cfg__    
    >|&emsp;|&emsp;|&emsp;&gt; default    
    >|&emsp;|&emsp;&gt; __ubuntu-installer__    
    >|&emsp;|&emsp;|&emsp;&gt; ...    
    >|&emsp;|&emsp;&gt; boot.img.gz    
    >|&emsp;|&emsp;&gt; ldlinux.c32    
    >|&emsp;|&emsp;&gt; ...    
    >|&emsp;|&emsp;&gt; pxelinux.0    
    >|&emsp;&gt;	 __another_os__    
    >|&emsp;|&emsp;&gt; ...    
    >|&emsp;&gt; ...   
- Rename `pxelinux.0` to `pxeboot.n12`



