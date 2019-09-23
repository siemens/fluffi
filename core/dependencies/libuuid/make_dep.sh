§§#!/bin/bash
§§# Copyright 2017-2019 Siemens AG
§§# 
§§# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
§§# 
§§# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
§§# 
§§# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
§§# 
§§# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
§§# 
§§# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
§§# Getting util-linux (contains libuuid)
§§
§§rm -rf util-linux
§§
§§git clone https://github.com/karelzak/util-linux.git
§§cd util-linux
§§git checkout e3bb9bfb76c17b1d05814436ced62c05c4011f48
§§
§§
§§# Compiling libuuid manualy (don't want to get autotools to run in our build environment)
§§mkdir -p build$ARCH
§§cd build$ARCH
§§
§§DEFINES="-DHAVE_ERR -DHAVE_ERRX -DHAVE_ERR_H -DHAVE_GETRANDOM -DHAVE_JRAND48 -DHAVE_LANGINFO_ALTMON -DHAVE_LANGINFO_H -DHAVE_LANGINFO_NL_ABALTMON -DHAVE_LOCALE_H -DHAVE_LOFF_T -DHAVE_MEMPCPY -DHAVE_NANOSLEEP -DHAVE_NETINET_IN_H -DHAVE_NET_IF_H -DHAVE_PROGRAM_INVOCATION_SHORT_NAME -DHAVE_SRANDOM -DHAVE_STDLIB_H -DHAVE_STRNDUP -DHAVE_STRNLEN -DHAVE_SYS_FILE_H -DHAVE_SYS_IOCTL_H -DHAVE_SYS_SOCKET_H -DHAVE_SYS_SYSCALL_H -DHAVE_SYS_SYSMACROS_H -DHAVE_SYS_TIME_H -DHAVE_SYS_UN_H -DHAVE_TLS -DHAVE_UNISTD_H -DHAVE_USLEEP -DHAVE_UUIDD -DHAVE_WARN -DHAVE_WARNX -DHAVE___PROGNAME -DSTDC_HEADERS -D_GNU_SOURCE -D_PATH_RUNSTATEDIR=\"/usr/var/run\" "
§§
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/clear.c -o clear.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/compare.c -o compare.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/copy.c -o copy.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/gen_uuid.c -o gen_uuid.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/isnull.c -o isnull.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/pack.c -o pack.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/parse.c -o parse.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/predefined.c -o predefined.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/unpack.c -o unpack.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/unparse.c -o unparse.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../libuuid/src/uuid_time.c -o uuid_time.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../lib/randutils.c -o randutils.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../lib/md5.c -o md5.o)&
§§(gcc -std=c99 $DEFINES -c -fPIC -I ../include ../lib/sha1.c -o sha1.o)&
§§
§§
§§wait
§§
§§ar rcs libuuid.a compare.o copy.o gen_uuid.o isnull.o pack.o parse.o predefined.o unpack.o unparse.o uuid_time.o randutils.o md5.o sha1.o
§§cd ../..
§§
§§cp util-linux/build${ARCH}/libuuid.a lib/${ARCH}/libuuid.a
§§cp util-linux/libuuid/src/uuid.h include/uuid.h
§§
§§rm -rf util-linux
