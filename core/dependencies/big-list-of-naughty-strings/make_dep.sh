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
# Author(s): Thomas Riedmaier

THREADS=$(nproc)
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
