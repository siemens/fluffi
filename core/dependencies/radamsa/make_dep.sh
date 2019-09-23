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
ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')

rm -rf bin/$ARCH

mkdir -p bin/$ARCH


# Getting & building OWL

rm -rf owl

git clone https://gitlab.com/owl-lisp/owl.git
cd owl
git checkout e31f1c268da8f1e9d84cdbbec4ce5b222d72127a
make owl 
cd ..

# Getting & building radamsa

rm -rf radamsa

git clone https://gitlab.com/akihe/radamsa
cd radamsa
git checkout 0bc6b489c895ce7c610b3d11a651a6b870143d6f
cp ../owl/bin/ol bin/ol
make everything
cd ..


cp radamsa/bin/radamsa bin/$ARCH/radamsa.exe

rm -rf owl
rm -rf radamsa

