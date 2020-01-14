<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Pascal Eckmann
-->

# [Odroid XU4](https://magazine.odroid.com/odroid-xu4)

1. Flash [Armbian](https://www.armbian.com/odroid-xu4/) on a Micro-SD card.
2. Turn the white switch on Odroid-XU4 to _&#181;SD_.
3. Boot Odroid-XU4 with the inserted Micro-SD card and attached eMMC flash. Ensure it is connected to the internet.
4. Log in.    
    user: _root_    
    password: _(odroid-)1234_    
## Configurate U-Boot on Odroid-XU4
1. Execute the following command to create new partitions on eMMC
    - `sudo fdisk /dev/mmcblk0` and press/type the following values:    
    `d`, `n`, `p`, `1`, `2048`, `264191`, `(y)`, `n`, `p`, `2`, `264192`, `30535679`, `(y)`, `w`   
2. The result should now be two partitions:
    - 128 MB: 2048 - 264191  
    - Remaining space: 264192 - default (30535679)  
3. Run the following commands to format your partitions and copy U-Boot to your device
    - `sudo mkfs.vfat -n boot /dev/mmcblk0p1`  
    - `sudo mkfs.ext4 /dev/mmcblk0p2`  
    - `mount -t vfat /dev/mmcblk0p1 /mnt/`  
    - `sudo wget -O /mnt/boot.ini https://git.io/v5ZNg`
    --> [boot.ini](https://github.com/mdrjr/5422_bootini/blob/8771a589360c093de23c78dfb5543d690cf86343/boot.ini)
4. Edit `/mnt/boot.ini`
    - `sudo nano /mnt/boot.ini`  
    - _Attention:_ Comment out the last line `#bootz 0x40008000 0x42000000 0x44000000` and insert the IP address of the TFTP server for the placeholder [server_ip]
        ```
        #Boot the board   
        #bootz 0x40008000 0x42000000 0x44000000   
        setenv serverip [server_ip]   
        setenv bootfile default-arm-exynos   
        run bootcmd_pxe   
        ```
6. Clone the U-Boot and run the configuration script
    - `git clone --depth 1 https://github.com/hardkernel/u-boot.git -b odroidxu4-v2017.05`   
    - `cd ../u-boot/sd_fuse`   
    - `sudo ./sd_fusing.sh /dev/mmcblk0` 
    
## Prepare [Armbian](./armbian.md) and then continue here
- Turn the white switch on Odroid-XU4 to _eMMC_
- Remove the Micro-SD card 
- Connect Odroid-XU4 to your Fluffi network
- Turn it on. It should now boot with PXE and get its image over TFTP and FTP

## Good to know
- Every time you restart your Odroid-XU4, it will reinstall its OS
- UART on Odroid-XU4 for debugging
    ```
     ___________
    |           |
    |Pin 4 - GND|
    |Pin 3 - RXD|
    |Pin 2 - TXD|
    |Pin 1 - VCC|
    |           |
      |_________|
    ```
