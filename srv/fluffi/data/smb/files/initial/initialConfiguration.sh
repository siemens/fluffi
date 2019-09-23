# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Pascal Eckmann, Thomas Riedmaier, Roman Bendt

# Inital configuration script to prepare a fresh linux system for FLUFFI. Essentially:
# - Configure Networking
# - Set Hostname
# - Prepare for Ansible

logfile="initialConfiguration.log"
echo "Initial configuration started" &>>$logfile

# Get Interface with dhcp enabled
#interfaces=`ip link show | awk 'NR % 2 == 1' | awk '{print $2}' | sed 's/.$//'`
routeInterface=`route | grep 10.66.0.0 | awk '{print $8}'`

if [[ -z "${routeInterface// }" ]]; then
	echo "No interface connected to fluffi" &>>$logfile
else
	echo "Got interface information $routeInterface" &>>$logfile
	# Get MAC
	MAC=`cat /sys/class/net/$routeInterface/address | tr a-z A-Z`
fi

echo "Start parsing the CSV file" &>>$logfile

# Parse CSV to get hostname
while IFS=, read -r col1 col2; do
	if [ "$MAC" = "$col1" ]; then
		hostname=`echo $col2 | sed 's/^M$//'` # Remove windows line ending
	fi
done < MAC2Host.csv

if [[ -z "${hostname// }" ]]; then
	echo "No hostname found in CSV" &>>$logfile
else
	# Set hostname
	echo "Setting hostname to $hostname" &>>$logfile
	sudo hostname $hostname
	sudo sed -i "s/unassigned-hostname/$hostname/g" /etc/hosts
	sudo sed -i "s/unassigned-domain/fluffi/g" /etc/hosts
	sudo sed -i "s/unassigned-hostname/$hostname/g" /etc/hostname
fi

§§# Lemmings needing a "Extrawurst"
§§if [[ $hostname == *"lemming"* ]]; then
§§	echo "Setting odroidxu4 to $hostname"
§§	hostnamectl set-hostname $hostname
§§	sed -i "s/odroidxu4/$hostname/g" /var/run/systemd/netif/leases/*
§§	sed -i "s/odroidxu4/$hostname/g" /etc/ssh/ssh_host*
§§	sed -i "s/ odroidxu4//g" /etc/hosts
§§
	echo "Add fluffi user"
	ROOTPASSWD=$(cat odroid_rootpasswd)
	FLUFFIUSER=$(cat odroid_username)
	FLUFFIPASSWD=$(cat odroid_userpasswd)
	(echo -e "$ROOTPASSWD\n$ROOTPASSWD") | passwd root
	(echo -e "$FLUFFIPASSWD\n$FLUFFIPASSWD\ny") | adduser $FLUFFIUSER
	adduser $FLUFFIUSER sudo
§§	echo "Disabling user account creation procedure"
§§	rm -f /root/.not_logged_in_yet
§§	trap - INT
§§	
§§	#systemctl stop systemd-networkd.service
§§	#systemctl stop systemd-resolved.service
§§	#systemctl disable systemd-networkd.service
§§	#systemctl disable systemd-resolved.service
§§	#systemctl mask systemd-networkd.service
§§	#systemctl mask systemd-resolved.service
§§	
§§	rm -f /etc/resolv.conf
§§	echo "search fluffi" > /etc/resolv.conf
§§	echo "nameserver 10.66.0.1" >> /etc/resolv.conf
§§	# /etc/network/interfaces /etc/network/interfaces.unused
§§	#DEBIAN_FRONTEND=noninteractive apt-get install -yq network-manager
§§	#sed -i ':a;N;$!ba;s/\[main\]\n/[main]\ndns=default\n/g' /etc/NetworkManager/NetworkManager.conf
§§	#sed -i 's/renderer: networkd/renderer: NetworkManager/g' /etc/netplan/01-netcfg.yaml
§§	#netplan apply
§§	#systemctl restart network-manager
§§	
§§	#maclow="enx${MAC,,}"
§§	#echo "${maclow//:}"
§§	#ip link set dev "${maclow//:}" down
§§	#ip link set dev "${maclow//:}" up
§§	
§§	#systemctl restart systemd-networkd.service
§§	#systemctl restart systemd-resolved.service
§§	
§§	
§§	#dhclient -r
§§	#echo "run dhclient"
§§	#dhclient -r
§§	#systemctl restart ntp.service
§§	#service ntp start
§§	#echo "Wait 9s, because ntp needs 8s for fetching the new time"
§§	
§§	#dhclient
§§	#sleep 10
§§	apt-get update
§§	(echo -e "y") | apt-get install python
§§	dhclient
§§else
§§	# UBUNTU
§§	# Fix apt-get mirror
§§	# sudo sed -i "s/http/ftp/g" /etc/apt/sources.list
§§	# fix systemd stuff (aka kill it with fire)
§§	
§§	systemctl stop systemd-networkd.service
§§	systemctl stop systemd-resolved.service
§§	systemctl disable systemd-networkd.service
§§	systemctl disable systemd-resolved.service
§§	systemctl mask systemd-networkd.service
§§	systemctl mask systemd-resolved.service
§§	
§§	rm -f /etc/resolv.conf
§§	echo "search fluffi" > /etc/resolv.conf
§§	echo "nameserver 10.66.0.1" >> /etc/resolv.conf
§§	mv /etc/network/interfaces /etc/network/interfaces.unused
§§	DEBIAN_FRONTEND=noninteractive apt-get install -yq network-manager
§§	sed -i ':a;N;$!ba;s/\[main\]\n/[main]\ndns=default\n/g' /etc/NetworkManager/NetworkManager.conf
§§	sed -i 's/renderer: networkd/renderer: NetworkManager/g' /etc/netplan/01-netcfg.yaml
§§	netplan apply
§§	systemctl restart network-manager
§§fi
