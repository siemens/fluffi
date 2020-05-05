#!/bin/bash
# Copyright 2017-2020 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including without
# limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
# SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier, Roman Bendt

# example call: sudo ./buildAll.sh PREPARE_ENV=TRUE WITH_DEPS=TRUE DEPLOY_TO_FTP=FALSE

[ "$JENKINS_DEBUG" == 'true' ] && set -x

# Make sure only root can run our script
if [ "$(id -u)" != "0" ]; then
	echo "As this script uses docker, it must be run as root." 1>&2
	exit 1
fi

PREPARE_ENV=
WITH_DEPS=
DEPLOY_TO_FTP=

for ARGUMENT in "$@"
do

	KEY=$(echo $ARGUMENT | cut -f1 -d=)
	VALUE=$(echo $ARGUMENT | cut -f2 -d=)   

	case "$KEY" in
		PREPARE_ENV)              PREPARE_ENV=${VALUE} ;; 
		WITH_DEPS)              WITH_DEPS=${VALUE} ;; 
		DEPLOY_TO_FTP)              DEPLOY_TO_FTP=${VALUE} ;; 
		*)   
	esac    


done

if [ "$PREPARE_ENV" = TRUE ] ; then
	echo "Preparing Docker Build environment"
	./prepare.sh
	STAT=$?
	if [ $STAT != 0 ]
	then
		echo "Preparing docker environment failed"
		exit $STAT
	fi
fi

USER=$(logname)
GROUP=$(groups $USER | awk '{print $3}')
USERID=$(id $USER -u)
GROUPID=$(id $GROUP -g)

# Building FLUFFI C
for ARCH in "Intel80386" "x86-64" "ARM" "ARMaarch64"; do
	docker run --rm -e FLUFFI_DEPS="$WITH_DEPS" --user $USERID:$GROUPID -v $(pwd)/../../core:/fluffi -v $(pwd)/build.sh:/build.sh fluffi${ARCH,,} /build.sh
	STAT=$?
	if [ $STAT != 0 ]
	then
		echo "Sdocker run failed"
		exit $STAT
	fi
done

# Building FLUFFI GO: Carrot
docker run --rm --user $USERID:$GROUPID -v $(pwd)/../../core:/fluffi -v $(pwd)/buildCaRRoT.sh:/build.sh flufficarrot /build.sh
STAT=$?
if [ $STAT != 0 ]
then
	echo "Sdocker run failed"
	exit $STAT
fi

# Building FLUFFI GO: Oedipus
docker run --rm --user $USERID:$GROUPID -v $(pwd)/../../core:/fluffi -v $(pwd)/buildOedipus.sh:/build.sh fluffioedipus /build.sh
STAT=$?
if [ $STAT != 0 ]
then
	echo "Sdocker run failed"
	exit $STAT
fi


if [[ -d "../../core/Intel80386/bin" ]] ; then
	cp ../../core/CaRRoT/CaRRoT-386 ../../core/Intel80386/bin/CaRRoT
	cp ../../core/Oedipus/Oedipus-386 ../../core/Intel80386/bin/Oedipus
fi
if [[ -d "../../core/x86-64/bin" ]] ; then
	cp ../../core/CaRRoT/CaRRoT-amd64 ../../core/x86-64/bin/CaRRoT
	cp ../../core/Oedipus/Oedipus-amd64 ../../core/x86-64/bin/Oedipus
fi
if [[ -d "../../core/ARM/bin" ]] ; then
	cp ../../core/CaRRoT/CaRRoT-arm ../../core/ARM/bin/CaRRoT
	cp ../../core/Oedipus/Oedipus-arm ../../core/ARM/bin/Oedipus
fi
if [[ -d "../../core/ARMaarch64/bin" ]] ; then
	cp ../../core/CaRRoT/CaRRoT-arm64 ../../core/ARMaarch64/bin/Oedipus
	cp ../../core/Oedipus/Oedipus-arm64 ../../core/ARMaarch64/bin/CaRRoT
fi

if [ "$DEPLOY_TO_FTP" = TRUE ] ; then
	echo "Deploying FLUFFI to ftp.fluffi"
	./deploy2FTP.sh
	STAT=$?
	if [ $STAT != 0 ]
	then
		echo "deploying to ftp failed"
		exit $STAT
	fi
fi

