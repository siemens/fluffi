#!/bin/bash
# ***************************************************************************
# Copyright (c) 2017-2020 Siemens AG  All rights reserved.
# ***************************************************************************/
#
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL Siemens AG OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
# 
# Author(s): Thomas Riedmaier, Pascal Eckmann, Roman Bendt

THREADS=$(nproc)
ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')

# delete old build results (but do not throw away include/build$ARCH of other architectures)
mkdir -p include
find include -mindepth 1 ! -regex '^include/build\(.*\)?' -delete
rm -rf include/build$ARCH
rm -rf dyndist$ARCH

mkdir -p include
mkdir -p dyndist$ARCH

rm -rf dynamorio

# Getting dynamorio from git

git clone https://github.com/DynamoRIO/dynamorio.git
cd dynamorio
git checkout 1ddc1a79579c6efce1baa264de600654d00f6bda

# Copy files for the drcovMulti module
mkdir -p clients/drcovMulti
cp ../drcovMulti/drcovMulti.c clients/drcovMulti/
cp ../drcovMulti/CMakeLists.txt clients/drcovMulti/

#Apply patches for drcov that enables drcovMulti and edge coverage
patch -p0 < ../patches/CMakeLists.txt.patch
patch -p0 < ../patches/drcov.c.patch
patch -p0 < ../patches/drcovlib.c.patch
patch -p0 < ../patches/drcovlib.h.patch
patch -p0 < ../patches/modules.c.patch
cp ../patches/xxhash.h ext/drcovlib/

#Make dynamorio
mkdir -p build$ARCH
cd build$ARCH
cmake ..
make -j$THREADS drcov drcovMulti drrun drpreload drconfig
cd ../..

rsync -r dynamorio/build$ARCH/ dyndist$ARCH/

cd dynamorio/
find . -name '*.h' -exec cp --parents \{\} ../include/ \;
cd ..

rm -rf dynamorio

#Create a minimal drcovlib.h, that is easy to include
awk '/typedef struct _bb_entry_t/,/bb_entry_t;/' include/ext/drcovlib/drcovlib.h > include/ext/drcovlib/drcovlib_min.h
