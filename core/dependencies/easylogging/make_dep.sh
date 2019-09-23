#!/bin/bash
# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier, Pascal Eckmann, Roman Bendt

THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)
§§ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')

§§rm -rf include
§§rm -rf lib/$ARCH

mkdir -p include
mkdir -p lib/$ARCH

# Getting easylogging

§§rm -rf easyloggingpp

git clone https://github.com/zuhd-org/easyloggingpp.git
cd easyloggingpp
git checkout 5181b4039c04697aac7eac0bde44352cd8901567

# Writing our config
sed -i '1s;^;#define ELPP_NO_DEFAULT_LOG_FILE\n//#define ELPP_DISABLE_INFO_LOGS\n//#define ELPP_NO_LOG_TO_FILE\n#define ELPP_THREAD_SAFE\n#if !defined(_DEBUG) \&\& !defined(DEBUG)\n#define ELPP_DISABLE_DEBUG_LOGS\n#endif\n;' src/easylogging++.h

# Building easylogging lib

mkdir -p build$ARCH
cd build$ARCH
§§cmake -Dbuild_static_lib=ON ..
make -j$THREADS
cd ../..

cp easyloggingpp/build${ARCH}/libeasyloggingpp.a lib/${ARCH}/libeasyloggingpp.a


cd easyloggingpp/src/
find . -name '*.h' -exec cp --parents \{\} ../../include/ \;
cd ../..

§§rm -rf easyloggingpp

