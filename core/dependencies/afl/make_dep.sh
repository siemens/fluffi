#!/bin/bash
#   Copyright 2017-2019 Siemens AG
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

mkdir -p lib/$ARCH

# Getting afl

rm -rf afl

git clone https://github.com/mirrorer/afl.git
cd afl
git checkout 2fb5a3482ec27b593c57258baae7089ebdc89043

# Apply patch that allows us to integrate the AFL Havoc mutations into FLUFFI

patch -p0 < ../afl.patch

mkdir -p build$ARCH
cd build$ARCH
g++ -c -fPIC ../afl-fuzz.c -o afl-fuzz.o
ar rcs libafl.a afl-fuzz.o
cd ../..

cp afl/build${ARCH}/libafl.a lib/${ARCH}/libafl.a


rm -rf afl
