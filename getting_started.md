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

Author(s): Roman Bendt, Thomas Riedmaier, Pascal Eckmann, Junes Najah
-->

# Getting Started
This section will guide you through the process of setting up your own FLUFFI installation.
To make reproducing the FLUFFI environment as easy as possible, the following instructions are written to build a virtual FLUFFI network. However, you can easily adapt them for actual hardware.

- [Prerequisites](#prerequisites)
- [Common Steps](#common-steps)
- [Compiling FLUFFI core](#compiling-fluffi-core)
    - [Windows](#windows)
    - [Linux](#linux)
- [Setting up the FUN](#setting-up-the-fun)
    - [Virtual Network Configuration](#virtual-network-configuration)
    - [FUN Services](#fun-services)
- [A Note On Operational Security](#a-note-on-operational-security)

## Prerequisites
You need to have at least two machines to run FLUFFI: a master machine that will host the infrastructure services and the controlling web interface, and another machine that runs the agents for actual fuzzing work.
Furthermore, you need to create a subnet that is entirely under your control, where no other DNS/DHCP server may interfere. 

If you want to move to a production grade FLUFFI installation, it is recommended to distribute the master services to more than one server for performance reasons. At the very least, you should consider a dedicated database server.
Then you can further scale your fuzzing capabilities by adding more and more agent machines.

## Common Steps
These steps are needed to compile FLUFFI core, as well as to build the FLUFFI master for the FUN (FLUFFI utility network).
Should you want to compile FLUFFI on a server other than your master, ensure that both machines are prepared this way.
These instructions assume you have a fresh and clean install of **Ubuntu 18.04**.

Become root and setup the environment:
```bash
sudo -s # to run these steps as root
apt update
apt install git git-lfs python3-pip apt-transport-https ca-certificates curl gnupg-agent software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
apt update
apt install docker-ce docker-ce-cli containerd.io
pip3 install --upgrade docker-compose
```
Drop root privileges now, so file permissions are correct afterwards. Then we clone the repository:
```bash
exit # to get back to normal user
git lfs install
git clone https://github.com/siemens/fluffi FLUFFI
cd FLUFFI
git lfs pull
```

## Compiling FLUFFI core
### Linux
Make sure you run the build on a machine/VM that has **at least 1.5 GB of memory for every core** installed, otherwise the system will not be able to build.
For example, a machine/VM with 4 cpu cores should have at least 6 GB of RAM allocated to it.
Note that the build is optimized to make heavy use of parallel processing, so the more cores the system has, the faster your build will be done.

Now use the [Linux build file](build/ubuntu_based/buildAll.sh) to compile the core binaries. 
Please note that we rely on _sudo_ here to correctly set things like the effective `uid` and `gid`.
If you directly run this as root (without sudo), you will get errors and the build will likely not work correctly.
```bash
cd build/ubuntu_based
sudo ./buildAll.sh PREPARE_ENV=TRUE WITH_DEPS=TRUE
```

When you compile FLUFFI for the first time, use the parameters `PREPARE_ENV=TRUE` and `WITH_DEPS=TRUE`:
- `PREPARE_ENV` will build the docker containers. This needs to be done only once.
- `WITH_DEPS` will build all of FLUFFI's dependencies from sources - please keep in mind that this will take a long time. This step needs to be repeated only if the dependencies change or get updated.
For any other future compile run you can simply do:
```
sudo ./buildAll.sh
```
If you have already set up the [FUN](#setting-up-the-fun) and it is reachable from the machine you use to compile, you can directly upload your binaries to the FLUFFI ftp server (ftp.fluffi) by calling
```
sudo ./buildAll.sh DEPLOY_TO_FTP=TRUE
```

Please note that the FLUFFI binaries must be uploaded to the ftp server in order for the automatic deployment to work.

### Windows
These instructions assume you have a fresh and clean install of **Windows 10 64-bit**.
The following tools are needed to build and deploy a FLUFFI installation:
- git and the [git-lfs extention](https://git-lfs.github.com/)
- Visual Studio 2017 or the standalone [build tools](https://download.visualstudio.microsoft.com/download/pr/3e542575-929e-4297-b6c6-bef34d0ee648/639c868e1219c651793aff537a1d3b77/vs_buildtools.exe) for Visual Studio 2017
- cmake
- perl (e.g. [Strawberry Perl for Windows](http://strawberryperl.com/))
- cygwin64 with make, gcc-i86, and gcc installed
- go (1.12 or newer) with [mysql driver](https://github.com/go-sql-driver/mysql) and [godiff](github.com/sergi/go-diff)
- 7zip
- Winscp

On Windows use [the Windows build file](build/windows/buildAll.bat) to start the compilation of FLUFFI core. When you compile FLUFFI for the first time, call
```
cd build/windows
buildAll.bat -WITH_DEPS TRUE
```
WITH_DEPS=TRUE will build all of FLUFFI's dependencies from sources. Please keep in mind that this will take quite some time.

For all future compile runs you can use 
```
buildAll.bat
```

If you have already set up the [FUN](#setting-up-the-fun) and it is reachable from the machine you use to compile, you can directly upload your binaries to the FLUFFI ftp server (ftp.fluffi) by calling
```
buildAll.bat -DEPLOY_TO_FTP TRUE
```

Please note that the FLUFFI binaries must be uploaded to the ftp server in order for the automatic deployment to work.

## Setting up the FUN
The FLUFFI Utility Network (FUN) contains all the infrastructure needed for fuzzing. 
It is governed by a master server with the static network internal IP address 10.66.0.1 and runs all the basic networking services in containers.
Before continuing, make sure you have run the necessary steps listed under [Common Steps](#common-steps) on the machine that will be your master.

### Virtual Network Configuration
Set up your master machine with two network interfaces, one as your method of internet access and one to connect to the separate FLUFFI network.
In these exemplary instructions we use `ens33` for internet access and `ens38` for the FUN.

![insert image here](virtualfluffi.png "virtual FLUFFI utility network")

Then configure the interfaces on the master:
```bash
sudo cat > /etc/systemd/network/default.network <<EOF
[Match]
Name=ens33
[Network]
DHCP=ipv4
EOF

sudo cat > /etc/systemd/network/fluffi.network <<EOF
[Match]
Name=ens38
[Network]
Address=10.66.0.1/23
Domains=fluffi
DNS=10.66.0.1
EOF

# this step is only needed on ubuntu desktop installations, since
# on headless installs systemd is already taking care of networking.
sed -i 's/NetworkManager/networkd/' /etc/netplan/01-network-manager-all.yaml
sudo systemctl restart systemd-networkd
```

### FUN Services
Next, create the live environment that serves all the core services:

#### Preparing the master machine to run the FLUFFI docker containers

```
sudo mkdir -p \
	/srv/fluffi/data/ftp/files/{SUT,archive} \
	/srv/fluffi/data/ftp/files/initial/linux/activePackages \
	/srv/fluffi/data/ftp/files/fluffi/linux/{x64,x86,arm32,arm64} \
	/srv/fluffi/data/ftp/files/fluffi/windows/{x64,x86} \
	/srv/fluffi/data/tftp \
	/srv/fluffi/data/dnsmasq

sudo touch /srv/fluffi/data/dnsmasq/dnsmasq.leases
```
Change to the base of the FLUFFI git folder and copy the services to /srv/fluffi.
You can execute this command again to update the server components in the future.

```
sudo rsync -ai --delete --exclude={'data/ftp/files/archive','data/ftp/files/deploy','data/ftp/files/fluffi','data/ftp/files/initial/linux/activePackages','data/ftp/files/initial/linux/inactivePackages','data/ftp/files/initial/windows/activePackages','data/ftp/files/initial/windows/inactivePackages','data/ftp/files/initial/windows/ansible/Driver','data/ftp/files/initial/windows/ansible/VCRedistributables','data/ftp/files/odroid','data/ftp/files/SUT','data/ftp/files/tftp-roots','data/ftp/files/ubuntu-mirror','data/tftp','data/smb/files/server*','data/dnsmasq/dnsmasq.leases','data/mon/grafana','data/mon/influxdb'} ./srv/fluffi/ /srv/fluffi/
```

Now you need to apply your individual changes to the default configuration.
We recommend you customize the following files:
- `/srv/fluffi/data/dnsmasq/ethers` - assign MAC based hostnames
- `/srv/fluffi/data/dnsmasq/hosts` - assign static IP addresses or split the master services between different machines
- `/srv/fluffi/data/polenext/projects/1/hosts` - amend the names and login credentials of your executor machines. Make sure that you are not using any underscores in your hostname, see [this upstream issue](https://github.com/ansible/ansible/issues/56930)
- `/srv/fluffi/data/smb/files/initial/odroid_rootpasswd` - add root password for odroid executor machines if applicable
- `/srv/fluffi/data/smb/files/initial/odroid_username` - add username for odroid executor machines if applicable
- `/srv/fluffi/data/smb/files/initial/odroid_userpasswd` - add user password for your odroid executor machines applicable
- `/srv/fluffi/data/smb/files/initial/MAC2Host.csv` - list MAC addresses and hostnames of your executor machines if you want to set hostnames during automatic deployment

There are several necessary binary files we cannot provide in this repository.
The following directories and their subdirectories contain a number of placeholder files that need to be replaced:
- `/srv/fluffi/data/smb/files/initial/updatesWS2008`
- `/srv/fluffi/data/ftp/files/initial/windows/ansible`

All the files in there are dummy files named like the real files that have to go in there, but they are actually just textfiles that contain a download link.
Download the binaries from these links and replace the placeholder textfiles with their respective binaries.

Get the GlobalManager components from where you [compiled FLUFFI core](#compiling-fluffi-core) - specifically, you need to copy
`core/{ARCH}/bin/{GlobalManager,lib*.so*}` to `/srv/fluffi/data/fluffigm/`


Set file permissions correctly:
```
sudo chown -R 1000:1000 /srv/fluffi/data/{ftp/files,tftp,smb/files}

sudo find /srv/fluffi/data/smb/files -type d -exec chmod 777 {} \;
sudo find /srv/fluffi/data/ftp/files -type d -exec chmod 777 {} \;
sudo find /srv/fluffi/data/tftp -type d -exec chmod 777 {} \;

sudo chmod 555 /srv/fluffi/data/ftp/files
sudo chmod -R +x /srv/fluffi/data/fluffigm
```

Get FLUFFI's web dependencies:
```
( cd /srv/fluffi/data/fluffiweb/app/; ./get_static_dependencies.sh; )
```

Now you can start the build of the containers and bring up the infrastructure services:
```
sudo docker-compose -f /srv/fluffi/docker-compose.yaml build --parallel --no-cache --pull
sudo docker-compose -f /srv/fluffi/docker-compose.yaml up -d --force-recreate
```
At this point you are ready to use FLUFFI, and can go to [the usage section](usage.md) for more information.
You should be able to connect to the web interface at `web.fluffi:8880`.

#### Reverse Proxy for comfortable service access

We recommend setting up a reverse proxy for the different services, so you can omit the port and use the network with DNS names only.
An example configuration for a reverse proxy setup using `nginx` could look like this: 
```
server {
	listen 80;
	server_name web.fluffi;
	client_max_body_size 0;
	location / {
		proxy_pass http://localhost:8880;
	}
}
server {
	listen 80;
	server_name pole.fluffi;
	location / {
		proxy_pass http://localhost:8888;
	}
}
server {
	listen 80;
	server_name mon.fluffi;
	location / {
		proxy_pass http://localhost:8885;
	}
}
server {
	listen 80;
	server_name apt.fluffi;
	location / {
		autoindex on;
		root /srv/ftp/ubuntu-mirror/;
	}
}
```

#### Package mirror


When using Ubuntu or Odroid runner machines, the runner machines somehow need to be able to reach a packet mirror. If you use the [FLUFFI pxe images](agents/pxe_images), the mirror used will be apt.fluffi. We recommend setting up a local packet mirror by installing apt-mirror on your master machine with `apt-get install apt-mirror`.

An example mirror.list you can use is
```
############# config ##################
#
set base_path    /srv/ftp/ubuntu-mirror
#
set mirror_path  $base_path/mirror
set skel_path    $base_path/skel
set var_path     $base_path/var
set cleanscript $var_path/clean.sh
# set defaultarch  <running host architecture>
set postmirror_script $var_path/postmirror.sh
# set run_postmirror 0
set nthreads     20
set _tilde 0
#
############# end config ##############

### AMD64
deb [arch=amd64] http://archive.ubuntu.com/ubuntu bionic main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu bionic main/debian-installer restricted/debian-installer universe/debian-installer multiverse/debian-installer
deb [arch=amd64] http://archive.ubuntu.com/ubuntu bionic-security main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu bionic-updates main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu bionic-backports main restricted universe multiverse

### i386 - was necessary due to PXE installer requiring some i386 package
deb [arch=i386] http://archive.ubuntu.com/ubuntu bionic main restricted universe multiverse
deb [arch=i386] http://archive.ubuntu.com/ubuntu bionic main/debian-installer restricted/debian-installer universe/debian-installer multiverse/debian-installer
deb [arch=i386] http://archive.ubuntu.com/ubuntu bionic-security main restricted universe multiverse
deb [arch=i386] http://archive.ubuntu.com/ubuntu bionic-updates main restricted universe multiverse
deb [arch=i386] http://archive.ubuntu.com/ubuntu bionic-backports main restricted universe multiverse

### ARM64 - not absolutely necessary in current environment
deb [arch=arm64] http://apt.armbian.com bionic main bionic-utils bionic-desktop
deb [arch=arm64] http://ports.ubuntu.com bionic main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com bionic-security main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com bionic-updates main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com bionic-backports main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com bionic-backports main/debian-installer restricted/debian-installer universe/debian-installer multiverse/debian-installer

### ARM-hf - Lemmings lair running on it
deb [arch=armhf] http://apt.armbian.com bionic main bionic-utils bionic-desktop
deb [arch=armhf] http://ports.ubuntu.com bionic main restricted universe multiverse
deb [arch=armhf] http://ports.ubuntu.com bionic-security main restricted universe multiverse
deb [arch=armhf] http://ports.ubuntu.com bionic-updates main restricted universe multiverse
deb [arch=armhf] http://ports.ubuntu.com bionic-backports main restricted universe multiverse
deb [arch=armhf] http://ports.ubuntu.com bionic-backports main/debian-installer restricted/debian-installer universe/debian-installer multiverse/debian-installer
#deb http://archive.ubuntu.com/ubuntu bionic-proposed main restricted universe multiverse

### Source because we can
deb-src http://archive.ubuntu.com/ubuntu bionic main restricted universe multiverse
deb-src http://archive.ubuntu.com/ubuntu bionic-security main restricted universe multiverse
deb-src http://archive.ubuntu.com/ubuntu bionic-updates main restricted universe multiverse
#deb-src http://archive.ubuntu.com/ubuntu bionic-proposed main restricted universe multiverse
deb-src http://archive.ubuntu.com/ubuntu bionic-backports main restricted universe multiverse
deb-src http://ports.ubuntu.com bionic main restricted universe multiverse
deb-src http://ports.ubuntu.com bionic-security main restricted universe multiverse
deb-src http://ports.ubuntu.com bionic-updates main restricted universe multiverse
deb-src http://ports.ubuntu.com bionic-backports main restricted universe multiverse

clean http://archive.ubuntu.com/ubuntu
clean http://apt.armbian.com
clean http://ports.ubuntu.com
```

#### Accessing the Internet from FUN

To actually access the internet from the 10.66.0.0/23 subnet, you can set up routing on the master machine.
Here, ens33 is the internet facing interface controlled by systemd, and ens38 is connected to the FUN, controlled by dnsmasq:
```
sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -A FORWARD -o ens33 -i ens38 -s 10.66.0.0/23 ! -d 10.66.0.0/23 -m conntrack --ctstate NEW -j ACCEPT
sudo iptables -A FORWARD -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
sudo iptables -t nat -A POSTROUTING -o ens33 -j MASQUERADE
```
Note that iptables rules do not typically survive a reboot of the machine and you have to either persist these, or run the above commands again after restarting your master.

Please note: We strongly discourage connecting FLUFFI to an untrusted network (see also the Operational Security section below).

## A Note on Operational Security

FLUFFI was not meant to be used in hostile environments, which is why FLUFFI's internal protocol is not hardened against attacks. It is therefore recommended to operate FLUFFI in an isolated environment, to which you can restrict access, and only allow the master to be connected to the internet via NAT.
Furthermore, it is strongly recommended to change the Polemarch user credentials, which are initially set to be `admin:admin`.
