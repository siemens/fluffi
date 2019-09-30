<!---
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
-->

## Getting Started
This section will guide you through the process of setting up your own FLUFFI installation.


### 1) Prerequisites
You need to have at least two machines to run FLUFFI: A machine that will be the central server, and another machine that runs the agents.

For performance reasons it is recommended to use more than one server machine (at least use a dedicated database server), plus as many as possible executor machines.

Furthermore, you need to have a subnet that is entirely under your control: No other DNS/DHCP server must interfere.

### 2) Compiling FLUFFI core

#### Windows
On Windows use  [the windows build file](build/windows/buildAll.bat). If you compile FLUFFI for the first time call 
```
buildAll.bat -WITH_DEPS TRUE
```
Please keep in mind that this will take a long time as it will compile all FLUFFI dependencies from sources.

For all future compiles you can use 
```
buildAll.bat -WITH_DEPS FALSE
```

If you already set up the FUN (see next section), you can directly upload your binaries to FUN by calling
```
buildAll.bat -DEPLOY_TO_FTP TRUE
```

#### Linux
On Linux use  [the linux build file](build/ubuntu_based/buildAll.sh). If you compile FLUFFI for the first time call 
```
./buildAll.sh PREPARE_ENV=TRUE WITH_DEPS=TRUE
```
Please keep in mind that this will take a long time as it will compile all FLUFFI dependencies from sources.

For all future compiles you can use 
```
./buildAll.sh PREPARE_ENV=FALSE WITH_DEPS=FALSE
```

If you already set up the FUN (see next section), you can directly upload your binaries to FUN by calling
```
./buildAll.sh PREPARE_ENV=FALSE WITH_DEPS=FALSE DEPLOY_TO_FTP=TRUE
```

### 3) Setting up the FUN
The FLUFFI Utility Network (FUN) contains all the infrastructure for fuzzing.

To set it up you firstly need to set up the central server with docker installed. Give that server the static IP address 10.66.0.1.

Copy the FLUFFI repo to that machine (or clone it there).

```
git clone  --depth 1 --branch master https://git.fluffi/team-fluffi/fluffi-private.git ./FLUFFIandFUN
```

Copy the server part to /srv/fluffi

```
rsync -ai --delete --exclude={'data/ftp/files/archive','data/ftp/files/fluffi','data/ftp/files/initial','data/ftp/files/odroid','data/ftp/files/SUT','data/ftp/files/tftp-roots','data/ftp/files/ubuntu-mirror','data/ftp/files/deploy','data/tftp','data/smb/files/server2008','data/smb/files/server2016','data/dnsmasq/dnsmasq.leases','data/gitea/varlib','data/gitea/home','data/mon/grafana','data/mon/influxdb'} ./FLUFFIandFUN/public/srv/fluffi/ /srv/fluffi/
```

Apply changes to the default configuration. We recommend you to:
- change /srv/fluffi/data/dnsmasq/ethers if you want to give some systems host names based on their mac addresses
- change /srv/fluffi/data/dnsmasq/hosts e.g., if you want to give some systems static ip addresses or if you want to move services to different machines
- change /srv/fluffi/data/polenext/projects/1/hosts to contain login information to your executor machines
- change /srv/fluffi/data/smb/files/initial/odroid_rootpasswd to contain the actual root password for your odroid executor machines
- change /srv/fluffi/data/smb/files/initial/odroid_username to contain the actual odroid username for your odroid executor machines
- change /srv/fluffi/data/smb/files/initial/odroid_userpasswd to contain the actual odroid password for your odroid executor machines
- change /srv/fluffi/data/smb/files/initial/MAC2Host.csv to contain the mac addresses and hosts of your executor machines (if you want to set host names according to mac addresses)
- replace the files in /srv/fluffi/data/smb/files/initial/updatesWS2008 with the actual files. The files stored there in the git are just text files with links to where you can download the actual files.

Copy the GlobalManager to /srv/fluffi/data/fluffigm

Set file permissions correctly:

```
chown -R 1000:1000 /srv/fluffi/data/ftp/files
chown -R 1000:1000 /srv/fluffi/data/smb/files
chown -R 1000:1000 /srv/fluffi/data/tftp
find /srv/fluffi/data/smb/files -type d -exec chmod 777 {} \;
find /srv/fluffi/data/ftp/files -type d -exec chmod 777 {} \;
find /srv/fluffi/data/tftp -type d -exec chmod 777 {} \;
chmod 555 /srv/fluffi/data/ftp/files
```

Get FLUFFI's web dependencies:
```
cd srv/fluffi/data/fluffiweb/app
./get_static_dependencies.sh
```

Finally start all server services

```
cd /srv/fluffi
docker-compose up -d --force-recreate
```

You are now ready to use FLUFFI. See [the usage section](usage.md) for information on how to do so.

# Operational Security

FLUFFI was never meant to be used in hostile environments, which is why FLUFFI's internal protocol is not hardened against attacks at all. It is therefore recommended, to operate FLUFFI in an isolated environment, to which you can restrict access.

Furthermore, it is strongly recommended to change Polemarch's user credentials, which are initially admin:admin.
