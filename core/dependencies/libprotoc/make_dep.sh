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

THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)
ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')

rm -rf include
rm -rf bin/$ARCH
rm -rf lib/$ARCH

mkdir -p include/siemens/cpp
mkdir -p include/siemens/csharp
mkdir -p include/google/protobuf
mkdir -p bin/$ARCH
mkdir -p lib/$ARCH

# Building linux protobof staticly. 

rm -rf protobuf

git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git checkout 106ffc04be1abf3ff3399f54ccf149815b287dd9
mkdir -p build$ARCH
cd build$ARCH
cmake -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON ../cmake
make -j$THREADS
cd ../..

cp protobuf/build${ARCH}/libprotobuf.a lib/${ARCH}/libprotobuf.a
cp protobuf/build${ARCH}/protoc bin/${ARCH}/protoc

cd protobuf/src/
find . -name '*.h' -exec cp --parents \{\} ../../include/ \;
cd ../..

rm -rf protobuf

bin/$ARCH/protoc --cpp_out="include/siemens/cpp" --csharp_out="include/siemens/csharp" FLUFFI.proto
