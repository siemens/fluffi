<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Junes Najah, Pascal Eckmann, Roman Bendt
-->

# Getting Started
This section will guide you through the process of setting up your own FLUFFI installation.


## 1) Prerequisites
You need to have at least two machines to run FLUFFI: A machine that will be the central server, and another machine that runs the agents.

For performance reasons, it is recommended to use more than one server machine (or at least a dedicated database server), plus as many executor machines as possible.

Furthermore, you need to have a subnet that is entirely under your control, no other DNS/DHCP server may interfere.

## 2) Compiling FLUFFI core

### Windows
On Windows use  [the windows build file](build/windows/buildAll.bat). When you compile FLUFFI for the first time, call 
```
buildAll.bat -WITH_DEPS TRUE
```
PREPARE_ENV=TRUE will build the docker continers.  This needs to be done only once.

WITH_DEPS=TRUE will build all of FLUFFI's dependencies from sources. Please keep in mind that this will take a long time.

For all future compiles you can use 
```
buildAll.bat -WITH_DEPS FALSE
```

If you have already set up the FUN (see next section), you can directly upload your binaries to FUN by calling
```
buildAll.bat -DEPLOY_TO_FTP TRUE
```

### Linux
On Linux use  [the linux build file](build/ubuntu_based/buildAll.sh). When you compile FLUFFI for the first time, call 
```
./buildAll.sh PREPARE_ENV=TRUE WITH_DEPS=TRUE
```
PREPARE_ENV=TRUE will build the docker continers.  This needs to be done only once.

WITH_DEPS=TRUE will build all of FLUFFI's dependencies from sources. Please keep in mind that this will take a long time.

For all future compiles you can use 
```
./buildAll.sh PREPARE_ENV=FALSE WITH_DEPS=FALSE
```

If you have already set up the FUN (see next section), you can directly upload your binaries to FUN by calling
```
./buildAll.sh PREPARE_ENV=FALSE WITH_DEPS=FALSE DEPLOY_TO_FTP=TRUE
```

## 3) Setting up the FUN
The FLUFFI Utility Network (FUN) contains all the infrastructure needed for fuzzing.

To set it up, you first need to set up the central server with docker and docker-compose installed. Give that server the static IP address 10.66.0.1.

To install docker and docker-compose see [here](https://docs.docker.com/install/linux/docker-ce/ubuntu/), and [here](https://docs.docker.com/compose/install/).

Having done so, copy the FLUFFI repo to that machine (or clone it there). For git clone, make sure to have [git lfs](https://git-lfs.github.com/) installed and properly configured(run `git lfs install`).

```
git clone --depth 1 --branch master https://github.com/siemens/fluffi.git ./fluffigit
```

If you are doing this for the very first time, create an empty dnsmasq.leases file and create directories for FTP (Do not do this if you just update FLUFFI): 
``` 
touch /srv/fluffi/data/dnsmasq/dnsmasq.leases
mkdir -p /srv/fluffi/data/ftp/files/fluffi/linux/{x64,x86,arm32,arm64} /srv/fluffi/data/ftp/files/fluffi/windows/{x64,x86} /srv/fluffi/data/tftp
```

Copy the server part to /srv/fluffi (You can execute this command even after you set up FLUFFI,e.g., to update server components):

```
rsync -ai --delete --exclude={'data/ftp/files/archive','data/ftp/files/deploy','data/ftp/files/fluffi','data/ftp/files/initial/linux/activePackages','data/ftp/files/initial/linux/inactivePackages','data/ftp/files/initial/windows/activePackages','data/ftp/files/initial/windows/inactivePackages','data/ftp/files/initial/windows/ansible/Driver','data/ftp/files/initial/windows/ansible/VCRedistributables','data/ftp/files/odroid','data/ftp/files/SUT','data/ftp/files/tftp-roots','data/ftp/files/ubuntu-mirror','data/tftp','data/smb/files/server*','data/dnsmasq/dnsmasq.leases','data/mon/grafana','data/mon/influxdb'} ./fluffigit/srv/fluffi/ /srv/fluffi/
```

Apply changes to the default configuration. We recommend you to:
- change /srv/fluffi/data/dnsmasq/ethers if you want to give some systems host names based on their MAC addresses
- change /srv/fluffi/data/dnsmasq/hosts e.g., if you want to give some systems static IP addresses or if you want to move services to different machines
- change /srv/fluffi/data/polenext/projects/1/hosts to contain the names of and login information to your executor machines. Make sure that you are not using any underscores in your hostname, 
as mentioned in this [issue](https://github.com/ansible/ansible/issues/56930).
- change /srv/fluffi/data/smb/files/initial/odroid_rootpasswd to contain the actual root password for your odroid executor machines
- change /srv/fluffi/data/smb/files/initial/odroid_username to contain the actual odroid username for your odroid executor machines
- change /srv/fluffi/data/smb/files/initial/odroid_userpasswd to contain the actual odroid password for your odroid executor machines
- change /srv/fluffi/data/smb/files/initial/MAC2Host.csv to contain the MAC addresses and hosts of your executor machines (if you want to set host names according to MAC addresses)
- replace the files in /srv/fluffi/data/smb/files/initial/updatesWS2008 and /srv/fluffi/data/ftp/files/initial/windows/ansible with the actual files. The files stored there in the git are just text files with links to where you can download the actual files.

Copy the GlobalManager to /srv/fluffi/data/fluffigm. To be precise, you need to copy `core/{{arch}}/bin/{GlobalManager,lib*.so*}`.

Set file permissions correctly:

```
chown -R 1000:1000 /srv/fluffi/data/ftp/files
chown -R 1000:1000 /srv/fluffi/data/smb/files
chown -R 1000:1000 /srv/fluffi/data/tftp
find /srv/fluffi/data/smb/files -type d -exec chmod 777 {} \;
find /srv/fluffi/data/ftp/files -type d -exec chmod 777 {} \;
find /srv/fluffi/data/tftp -type d -exec chmod 777 {} \;
chmod 555 /srv/fluffi/data/ftp/files
chmod -R +x /srv/fluffi/data/fluffigm
```

For the following two steps, an internet connection is necessary at the first time.
Get FLUFFI's web dependencies:
```
cd /srv/fluffi/data/fluffiweb/app
./get_static_dependencies.sh
```

Finally, build all server services:

```
cd /srv/fluffi
docker-compose build
```

Now switch your environment form Internet-connected to offline:
```
systemctl stop systemd-resolved.service
rm -f /etc/resolv.conf
echo "search fluffi" > /etc/resolv.conf
echo "nameserver 10.66.0.1" >> /etc/resolv.conf
```

Now you don't need an internet connection anymore. To start the containers and keep them running, run:
```
docker-compose up -d --force-recreate
```

You are now ready to use FLUFFI. See [the usage section](usage.md) for information on how to do so.

## 4) Operational Security

FLUFFI was never meant to be used in hostile environments, which is why FLUFFI's internal protocol is not hardened against attacks at all. It is therefore recommended to operate FLUFFI in an isolated environment, to which you can restrict access.

Furthermore, it is strongly recommended to change Polemarch's user credentials, which are initially admin:admin.
