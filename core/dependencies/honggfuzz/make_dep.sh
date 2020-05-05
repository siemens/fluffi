#!/bin/bash
#   Copyright 2017-2020 Siemens AG
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
# 
# Author(s): Thomas Riedmaier, Pascal Eckmann

THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)
ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')

rm -rf lib/$ARCH
rm -rf include

mkdir -p lib/$ARCH
mkdir -p include/libhfcommon

# Getting honggfuzz

rm -rf honggfuzz

git clone https://github.com/google/honggfuzz.git
cd honggfuzz
git checkout 589a9fb92c843ce5d44645e58457f7c041a33238

# Apply patch that allows us to integrate the honggfuzz mutations into FLUFFI

patch -p0 < ../honggfuzz.patch

mkdir -p build$ARCH
cd build$ARCH
g++ -c -fPIC ../mangle.c -o mangle.o
g++ -c -fPIC ../input.c -o input.o
g++ -c -fPIC ../libhfcommon/util.c -o util.o
ar rcs libhonggfuzz.a mangle.o input.o util.o
cd ../..

cp honggfuzz/build${ARCH}/libhonggfuzz.a lib/${ARCH}/libhonggfuzz.a

cp honggfuzz/mangle.h include
cp honggfuzz/honggfuzz.h include
cp honggfuzz/libhfcommon/util.h include/libhfcommon

rm -rf honggfuzz
