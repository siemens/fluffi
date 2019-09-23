§§#!/bin/bash
§§# Copyright 2017-2019 Siemens AG
§§# 
§§# This source code is provided 'as-is', without any express or implied
§§# warranty. In no event will the author be held liable for any damages
§§# arising from the use of this software.
§§#                                                                               
§§# Permission is granted to anyone to use this software for any purpose,
§§# including commercial applications, and to alter it and redistribute it
§§# freely, subject to the following restrictions:
§§#                                                                               
§§# 1. The origin of this source code must not be misrepresented; you must not
§§#    claim that you wrote the original source code. If you use this source code
§§#    in a product, an acknowledgment in the product documentation would be
§§#    appreciated but is not required.
§§#                                                                               
§§# 2. Altered source versions must be plainly marked as such, and must not be
§§#    misrepresented as being the original source code.
§§#                                                                               
§§# 3. This notice may not be removed or altered from any source distribution.
§§# 
§§# Author(s): Thomas Riedmaier, Pascal Eckmann
§§
§§THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)
§§ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')
§§
§§rm -rf lib/$ARCH
§§rm -rf include
§§
§§mkdir -p lib/$ARCH
§§mkdir -p include
§§
§§# Getting base64
§§
§§rm -rf cpp-base64
§§
§§git clone https://github.com/ReneNyffenegger/cpp-base64.git
§§cd cpp-base64
§§git checkout a8aae956a2f07df9aac25b064cf4cd92d56aac45
§§
§§
§§# Building base64 lib
§§mkdir -p build$ARCH
§§cd build$ARCH
§§
§§g++ -c -fPIC ../base64.cpp -o base64.o
§§
§§ar rcs base64.a base64.o
§§cd ../..
§§
§§cp cpp-base64/build${ARCH}/base64.a lib/${ARCH}/base64.a
§§cp cpp-base64/base64.h include/base64.h
§§
§§rm -rf cpp-base64
