#!/bin/bash
# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier, Roman Bendt

# example call: sudo ./prepare.sh DOPTS=--no-cache

# Make sure only root can run our script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root." 1>&2
   exit 1
fi

# Parse options
§§DOPTS=
for ARGUMENT in "$@"
do

    KEY=$(echo $ARGUMENT | cut -f1 -d=)
    VALUE=$(echo $ARGUMENT | cut -f2 -d=)   

    case "$KEY" in
            DOPTS)              DOPTS=${VALUE} ;; 
            *)   
    esac    


done
§§

# Installing qemu binfmt so that we can run arm dockerfiles
§§apt-get install -y --no-install-recommends qemu-user-static binfmt-support
§§update-binfmts --enable qemu-arm

§§cp $(which qemu-arm-static) .
§§cp $(which qemu-aarch64-static) .

§§(
	docker build $DOPTS -t fluffiintel80386 -f intel80386/Dockerfile .
§§)&
§§(
	docker build $DOPTS -t fluffix86-64 -f x86-64/Dockerfile .
§§)&
§§(
	docker build $DOPTS -t fluffiarm -f arm/Dockerfile .
§§)&
§§(
	docker build $DOPTS -t fluffiarmaarch64 -f armaarch64/Dockerfile .
§§)&
§§(
§§	docker build $DOPTS -t flufficarrot -f carrot/Dockerfile .
§§)&
§§(
§§	docker build $DOPTS -t fluffioedipus -f oedipus/Dockerfile .
§§)&
§§wait
