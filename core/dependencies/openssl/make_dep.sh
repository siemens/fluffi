#!/bin/bash
# Copyright 2017-2020 Siemens AG. All Rights Reserved.
#
# Licensed under the Apache License 2.0 (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.openssl.org/source/license.html
#
# Author(s): Thomas Riedmaier, Roman Bendt

THREADS=$(nproc)
ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')

rm -rf include
rm -rf include$ARCH
rm -rf lib/$ARCH

mkdir -p include
mkdir -p include$ARCH/openssl
mkdir -p lib/$ARCH

# Getting Openssl

rm -rf openssl

git clone https://github.com/openssl/openssl.git
cd openssl
git checkout 97ace46e11dba4c4c2b7cb67140b6ec152cfaaf4

# Transalting arch to something openssl understands

if [[ $ARCH == "Intel80386" ]] ; then
	OSSLARCH="linux-x86"
elif [[ $ARCH == "x86-64" ]] ; then
	OSSLARCH="linux-x86_64"
elif [[ $ARCH == "ARM" ]] ; then
	OSSLARCH="linux-armv4"
elif [[ $ARCH == "ARMaarch64" ]] ; then
	OSSLARCH="linux-aarch64"
fi

# Actually build openssl
mkdir -p build$ARCH
cd build$ARCH
perl ../Configure $OSSLARCH --release no-tests no-unit-test no-asm enable-static-engine no-shared
make -j$THREADS
cd ../..


cp openssl/build${ARCH}/libcrypto.a lib/${ARCH}/libcrypto.a
cp openssl/build${ARCH}/libssl.a lib/${ARCH}/libssl.a

cp openssl/build${ARCH}/include/openssl/opensslconf.h include$ARCH/openssl
cd openssl/include/
find . -name '*.h' -exec cp --parents \{\} ../../include/ \;
cd ../..

cp openssl/ms/applink.c include



rm -rf openssl


