#!/bin/bash
# Copyright 2017-2020 Siemens AG
# 
# This source code is provided 'as-is', without any express or implied
# warranty. In no event will the author be held liable for any damages
# arising from the use of this software.
#                                                                               
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#                                                                               
# 1. The origin of this source code must not be misrepresented; you must not
#    claim that you wrote the original source code. If you use this source code
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
#                                                                               
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original source code.
#                                                                               
# 3. This notice may not be removed or altered from any source distribution.
# 
# Author(s): Thomas Riedmaier

THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)
ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')

rm -rf lib/$ARCH

mkdir -p lib/$ARCH


# Getting big-list-of-naughty-strings

rm -rf big-list-of-naughty-strings

git clone https://github.com/minimaxir/big-list-of-naughty-strings.git
cd big-list-of-naughty-strings
git checkout e1968d982126f74569b7a2c8e50fe272d413a310

# Extracting all lines that are not comments
grep -v '^#' blns.txt > blns-noComments.txt

# Delete empty lines or blank lines
sed -i '/^$/d' blns-noComments.txt

# Transform it into something that can be compiled
echo "#include <deque>"  > blns.cpp
echo "#include <string>"  >> blns.cpp
echo "#include \"../include/blns.h\""  >> blns.cpp
echo "std::deque<std::string> blns {" >> blns.cpp
sed -e "s/.*/std::string(R\"FLUFFI(&)FLUFFI\"),/" "blns-noComments.txt" >> blns.cpp
echo "std::string(\"\")};" >> blns.cpp


# Building lib
mkdir -p build$ARCH
cd build$ARCH

g++ -std=c++11 -c -fPIC ../blns.cpp -o blns.o

ar rcs libblns.a blns.o
cd ../..

cp big-list-of-naughty-strings/build${ARCH}/libblns.a lib/${ARCH}/libblns.a

rm -rf big-list-of-naughty-strings
