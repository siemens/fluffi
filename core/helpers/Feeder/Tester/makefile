# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah

ARCH := $(shell file /bin/bash | awk -F',' '{print $$2}' | tr -d ' ')

WARNINGS = -Wno-psabi -Wall -Wextra -Wconversion -Wsign-compare -Wlogical-op -Wnull-dereference -Wold-style-cast
DEBUG = $(WARNIGNS) -g3 -O0 -DDEBUG
RELEASE = $(WARNINGS) -s -g0 -O2

#By default compile as release (uncomment here for debug build)
#AFLAGS = $(DEBUG)
AFLAGS = $(RELEASE)
CC = g++ -std=c++11

all:
	cp ../../../$(ARCH)/bin/libsharedmemipc.so .
	cp ../../../$(ARCH)/bin/libzmq.so.5 .
	$(CC) $(AFLAGS) -o Tester -L . -I../../../SharedCode -I../../../dependencies/libprotoc/include/siemens/cpp -I../../../dependencies/libprotoc/include -I../../../SharedMemIPC  -I ../../../dependencies/easylogging/include -I ../../../dependencies/libuuid/include Tester.cpp ../../../$(ARCH)/lib/sharedcode.a  ../../../dependencies/libprotoc/lib/$(ARCH)/libprotobuf.a  ../../../dependencies/easylogging/lib/x86-64/libeasyloggingpp.a -pthread -lstdc++fs -ldl -l:libzmq.so.5 -lsharedmemipc  -Wl,-rpath='$${ORIGIN}'

clean:
	rm -f libsharedmemipc.so libzmq.so.5 Tester
