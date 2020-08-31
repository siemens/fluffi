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

Author(s): Pascal Eckmann
-->

# Ubuntu

## Prepare Ubuntu image

1. Download NetInstall version of Ubuntu, e.g., [Bionic Beaver](http://archive.ubuntu.com/ubuntu/dists/bionic/main/installer-amd64/current/images/netboot/)
    - e.g., `wget -r -np -R "index.html*" -P ./ http://archive.ubuntu.com/ubuntu/dists/bionic/main/installer-amd64/current/images/netboot/`
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
    - Add `d-i preseed/late_command string in-target smbclient '//smb.fluffi/install' -c 'cd initial; get MAC2Host.csv; get initialConfiguration.sh' -U anonymous%pass; in-target chmod 777 /initialConfiguration.sh; in-target /bin/bash /initialConfiguration.sh; in-target rm MAC2Host.csv; in-target rm initialConfiguration.sh` at the end of the preseeding file
    - An example preseeding file for Ubuntu 18.04 is located [here](https://help.ubuntu.com/lts/installation-guide/example-preseed.txt). 
        - You have to change the user name to the same which you defined for _ansible_ssh_user_ and the user password to the same as _ansible_ssh_pass_ in the section _[linux:vars]_ in your [hosts](../../srv/fluffi/data/polenext/projects/1/hosts) file for Polemarch. 
        ```
        d-i passwd/user-fullname string [username]
        d-i passwd/username string [username]
        [...]
        d-i passwd/user-password password [password]
        d-i passwd/user-password-again password [password]
        ```
        - Change `apt.fluffi` as mirror hostname: `d-i mirror/http/hostname string apt.fluffi`
        - Change `ntp.fluffi` as ntp-server: `d-i clock-setup/ntp-server string ntp.fluffi`
        - Change following lines in section `### Apt setup`:
        ```
        [..]
        d-i apt-setup/restricted boolean true
        d-i apt-setup/universe boolean true
        d-i apt-setup/backports boolean true
        [..]
        d-i apt-setup/security_host string apt.fluffi
        d-i apt-setup/security_path string /ubuntu
        ```
        - Change individual additional packages to install: `d-i pkgsel/include string openssh-server smbclient net-tools cryptsetup build-essential libssl-dev libreadline-dev zlib1g-dev linux-source dkms nfs-common`
    - _e.g. execute: `sudo ./embedPreseed.sh ./initrd.gz /home/user/ubuntu_netinstall/ubuntu-installer/amd64/initrdNew /home/user/ubuntu_netinstall/ubuntu-installer/amd64/preseed.cfg`_    
        - _Attention: The last two arguments have to be absolute paths._    
    - After that, delete the old `initrd.gz`. Rename the new RAM disk `initrd.gz` and place it into the same folder where the old one was.    

## Copy files to infrastructure
- Create a folder in `ftp.fluffi/tftp-roots/`, e.g. `ftp.fluffi/tftp-roots/ubuntu`, and copy all Ubuntu files in the new folder:
    ```
    tftp-roots 
    ├── ubuntu
    │   ├── pxelinux.cfg
    │   │   └── default
    │   ├── ubuntu-installer
    │   │   └── ...
    │   ├── boot.img.gz
    │   ├── ldlinux.c32
    │   ├── ...
    │   └── pxelinux.0
    ├── another_os
    │   └── ...
    └── ...
    ```
- Rename `pxelinux.0` to `pxeboot.n12`



