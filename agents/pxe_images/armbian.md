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

# Armbian (for Odroid XU4 boards)

1. Download and extract [Armbian](https://www.armbian.com/odroid-xu4/)
2. Mount/open downloaded image
    - `sudo fdisk -l [image]`
    - Calculate the offset between sector size * start: e.g. 512 * 8192 = 4194304
    - `sudo mount -o loop,offset=[offset] [Image] /mnt/tmp`
3. Navigate to `/mnt/tmp` and copy following files to another location, (you will need them later):
    - `/boot/initrd.img-X.XX.XX-odroidxu4` (RAM disk)   
    - `/boot/vmlinuz-X.XX.XX-odroidxu4` (Kernel)   
    - `/boot/dtb--X.XX.XX-odroidxu4/exynos5422-odroidxu4.dtb` (Device Tree Blob)
    - The following two executables, which are needed for automatic deployment
        - `/bin/tar` 
        - `/sbin/mke2fs` 
4. Prepare RAM disk image `initrd.img-X.XX.XX-odroidxu4`
    - Rename the image to `initrd.gz`
    - Create directory `initrd` next to `initrd.gz`
    - Navigate to `initrd` 
    - Execute: `zcat ../initrd.gz | fakeroot -s ../initrd.fakeroot cpio -i`
5. If everything worked, you should now see the typical unix folders in `initrd` and in the directory above `initrd.fakeroot`
6. Insert missing text `# format eMMC ...` and `# load Armbian ...` in `/initrd/init`
    ```
    [...]
    
    # Don't do log messages here to avoid confusing graphical boots
    run_scripts /scripts/init-top
    ```
    
    ```
    # format eMMC
    echo "format mmcblk0p2"
    (echo y) | mke2fs -t ext4 /dev/mmcblk0p2
    ```
    
    ```
    maybe_break modules
    [ "$quiet" != "y" ] && log_begin_msg "Loading essential drivers"
    [ -n "${netconsole}" ] && modprobe netconsole netconsole="${netconsole}"
    load_modules
    [ "$quiet" != "y" ] && log_end_msg
    
    [...]
    
    # Mount cleanup
    mount_bottom
    nfs_bottom
    local_bottom
    ```
    
    ```
    # load Armbian
    echo "load Armbian"
    configure_networking
    rm -rf /root/*
    mkdir /root/storage
    wget -t 10 ftp://ftp.fluffi/odroid/armbian.tar -P /root/storage
    tar xf /root/storage/armbian.tar -C /root/ --warning=no-timestamp
    rm /root/storage/armbian.tar
    rmdir /root/storage
    echo "Armbian filesystem here"
    ```

    ```
    maybe_break bottom
    [ "$quiet" != "y" ] && log_begin_msg "Running /scripts/init-bottom"
    # We expect udev's init-bottom script to move /dev to ${rootmnt}/dev
    run_scripts /scripts/init-bottom
    [ "$quiet" != "y" ] && log_end_msg
    
    [...]
    ```
7. Copy the `tar` and `mke2fs` into `initrd/sbin` and set their permissions
    - Execute: `chmod 755 tar`
    - Execute: `chmod 755 mke2fs`
8. Create new RAM disk image
    - Execute: `find | fakeroot -i ../initrd.fakeroot cpio -o -H newc | gzip -c > ../initrd.gz`
9. Rename kernel `vmlinuz-X.XX.XX-odroidxu4` to `zImage`
10. Navigate into the mounted Armbian image `/mnt/tmp` and copy the following *.deb files to `/usr/local` of the image (Links of these files for Debian Buster)
    - [libtalloc2_XX.deb](https://packages.debian.org/buster/armhf/libtalloc2/download)
        - Rename the file to `libtalloc2.deb`
    - [libwbclient0_XX.deb](https://packages.debian.org/buster/armhf/libwbclient0/download)
        - Rename the file to `libwbclient0.deb`
    - [cifs-utils_XX.deb](https://packages.debian.org/buster/armhf/cifs-utils/download)
        - Rename the file to `cifs-utils.deb`
    - [samba-common_XX.deb](https://packages.debian.org/buster/armhf/samba-common/download)
        - Rename the file to `samba-common.deb`
    - Note: You can also download these packages with `sudo apt-get install --download-only <package_name>`
11. Change `/etc/rc.local`
    ```
    #!/bin/sh -e
    [...]
    # By default this script does nothing.
    
    echo "nameserver 10.66.0.1" >> /etc/resolv.conf
    dpkg -i /usr/local/libtalloc2.deb
    dpkg -i /usr/local/libwbclient0.deb
    dpkg -i /usr/local/samba-common.deb
    dpkg -i /usr/local/cifs-utils.deb
    apt-get install -f
    mkdir -p /mnt/smb
    mount -t cifs //smb.fluffi/install -o username=anonymous,password=pass /mnt/smb
    cp -R /mnt/smb/initial/* /
    bash /initialConfiguration.sh
    dhclient -r
    dhclient
    
    exit 0
    ```
12. Change repository to `apt.fluffi`
    - Modify `/etc/apt/sources.list`
        ```
        deb http://apt.fluffi/mirror/ports.ubuntu.com/ bionic main restricted universe multiverse
        #deb http://ports.ubuntu.com/ bionic main restricted universe multiverse
        #deb-src http://ports.ubuntu.com/ bionic main restricted universe multiverse
        
        deb http://apt.fluffi/mirror/ports.ubuntu.com/ bionic-security main restricted universe multiverse
        #deb http://ports.ubuntu.com/ bionic-security main restricted universe multiverse
        #deb-src http://ports.ubuntu.com/ bionic-security main restricted universe multiverse
        
        deb http://apt.fluffi/mirror/ports.ubuntu.com/ bionic-updates main restricted universe multiverse
        #deb http://ports.ubuntu.com/ bionic-updates main restricted universe multiverse
        #deb-src http://ports.ubuntu.com/ bionic-updates main restricted universe multiverse
        
        deb http://apt.fluffi/mirror/ports.ubuntu.com/ bionic-backports main restricted universe multiverse
        #deb http://ports.ubuntu.com/ bionic-backports main restricted universe multiverse
        #deb-src http://ports.ubuntu.com/ bionic-backports main restricted universe multiverse
        ```
    - Modify `/etc/apt/sources.list.d/armbian.list`
        ```
        deb http://apt.fluffi/mirror/apt.armbian.com/ bionic main bionic-utils bionic-desktop
        #deb http://apt.armbian.com bionic main bionic-utils bionic-desktop
        ```
13. Archive the mounted image
    - Execute: `sudo tar --owner=root --group=root -p -cvf /home/armbian.tar */` (at / of mounted image)
    
## Copy/configure files to infrastructure
- Create file `default-arm-exynos` with the following content
    ```
    DEFAULT odroidxu4_default

    LABEL odroidxu4_default
    kernel odroidxu4/zImage
    fdt odroidxu4/exynos5422-odroidxu4.dtb
    append initrd=odroidxu4/initrd.gz console=ttySAC2,115200n8 consoleblank=0 panic=10 root=/dev/mmcblk0p2 rootwait rw

    PROMPT 1
    TIMEOUT 0
    ```
- Create a folder in `ftp.fluffi/tftp-roots/`, e.g. `ftp.fluffi/tftp-roots/armbian` and copy all Armbian files (including the files from the image) to the new folder:
    ```
    tftp-roots 
    ├── armbian
    │   ├── pxelinux.cfg
    │   │   └── default-arm-exynos
    │   └── odroidxu4
    │       ├── exynos5422-odroidxu4.dtb
    │       ├── initrd.gz
    │       └── zImage
    ├── another_os
    │   └── ...
    └── ...
    ```
- Copy your generated `armbian.tar` to `ftp.fluffi/odroid/`
    ```
    odroid 
    ├── armbian.tar
    └── ...
    ```



