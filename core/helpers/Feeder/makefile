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
# Author(s): Junes Najah

ARCH := $(shell file /bin/bash | awk -F',' '{print $$2}' | tr -d ' ')

WARNINGS = -Wno-psabi -Wall -Wextra -Wconversion -Wsign-compare -Wlogical-op -Wnull-dereference -Wold-style-cast
DEBUG = $(WARNIGNS) -g3 -O0 -DDEBUG
RELEASE = $(WARNINGS) -s -g0 -O2

#By default compile as release (uncomment here for debug build)
#AFLAGS = $(DEBUG)
AFLAGS = $(RELEASE)
CC = g++ -std=c++11

.PHONY: sslwrap tcpfeeder tester udpfeeder

all: deps
	@$(MAKE) --no-print-directory ef smf sslwrap tcpfeeder tester udpfeeder

deps: 
	@$(MAKE) --no-print-directory depcopy

ef:
	$(CC) $(AFLAGS) -o EthernetFeeder/EthernetFeeder -I ../../SharedMemIPC/  EthernetFeeder/stdafx.cpp EthernetFeeder/EthernetFeeder.cpp -L EthernetFeeder -l:libsharedmemipc.so -Wl,-rpath='$${ORIGIN}'

smf:	
	$(CC) $(AFLAGS) -o SharedMemFeeder/SharedMemFeeder -I ../../SharedMemIPC/  SharedMemFeeder/stdafx.cpp SharedMemFeeder/SharedMemFeeder.cpp -L SharedMemFeeder -l:libsharedmemipc.so -Wl,-rpath='$${ORIGIN}'

sslwrap:
	$(CC) $(AFLAGS) -o sslwrap/libsslwrap.so  -I ../../dependencies/openssl/include -I ../../dependencies/openssl/include$(ARCH) sslwrap/sslwrap.cpp  -fPIC -shared -L ../../dependencies/openssl/lib/$(ARCH)/ -lssl -lcrypto -lpthread -ldl   -Wl,-z,defs  -Wl,--version-script=sslwrap/libsslwrap.version

tcpfeeder: sslwrap
	@cp sslwrap/libsslwrap.so TCPFeeder
	$(CC) $(AFLAGS) -o TCPFeeder/TCPFeeder -I ../../SharedMemIPC/  -DUSE_SSL TCPFeeder/stdafx.cpp TCPFeeder/http.cpp TCPFeeder/Packet.cpp TCPFeeder/TCPFeeder.cpp TCPFeeder/utils.cpp -L TCPFeeder -l:libsharedmemipc.so -l:libsslwrap.so -Wl,-rpath='$${ORIGIN}'

tester:		
	$(CC) $(AFLAGS) -o Tester/Tester  -I ../../SharedMemIPC -I ../../SharedCode -I ../../dependencies/libprotoc/include/siemens/cpp -I ../../dependencies/libprotoc/include   -I ../../dependencies/easylogging/include -I ../../dependencies/libuuid/include Tester/Tester.cpp ../../$(ARCH)/lib/sharedcode.a  ../../dependencies/libprotoc/lib/$(ARCH)/libprotobuf.a  ../../dependencies/easylogging/lib/x86-64/libeasyloggingpp.a -pthread -lstdc++fs -ldl -L Tester -l:libzmq.so.5 -lsharedmemipc  -Wl,-rpath='$${ORIGIN}'

udpfeeder:
	$(CC) $(AFLAGS) -o UDPFeeder/UDPFeeder  -I ../../SharedMemIPC/ UDPFeeder/stdafx.cpp UDPFeeder/UDPFeeder.cpp -L UDPFeeder -l:libsharedmemipc.so -Wl,-rpath='$${ORIGIN}'

depcopy:
	@cp ../../$(ARCH)/bin/libsharedmemipc.so EthernetFeeder 
	@cp ../../$(ARCH)/bin/libsharedmemipc.so SharedMemFeeder
	@cp ../../$(ARCH)/bin/libsharedmemipc.so TCPFeeder
	@cp ../../$(ARCH)/bin/libsharedmemipc.so Tester
	@cp ../../$(ARCH)/bin/libzmq.so.5 Tester
	@cp ../../$(ARCH)/bin/libsharedmemipc.so UDPFeeder

clean:
	@rm -f EthernetFeeder/EthernetFeeder EthernetFeeder/libsharedmemipc.so
	@rm -f SharedMemFeeder/SharedMemFeeder SharedMemFeeder/libsharedmemipc.so
	@rm -f sslwrap/libsslwrap.so
	@rm -f TCPFeeder/libsharedmemipc.so TCPFeeder/libsslwrap.so TCPFeeder/TCPFeeder
	@rm -f Tester/libsharedmemipc.so Tester/libzmq.so.5 Tester/Tester
	@rm -f UDPFeeder/libsharedmemipc.so UDPFeeder/libzmq.so.5 UDPFeeder/UDPFeeder
