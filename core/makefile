# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier, Roman Bendt


# absolute path to the fluffi main repository base
FLUFFIPATH := $(shell pwd)
ARCH := $(shell file /bin/bash | awk -F',' '{print $$2}' | tr -d ' ')

# vars for SharedCode subproject
SHSRCS := $(wildcard $(FLUFFIPATH)/SharedCode/*.cpp)
SHOBJS := $(addprefix $(ARCH)/buildtmp/shobj/,$(notdir $(SHSRCS:.cpp=.o)))
SHINCLUDE = -I $(FLUFFIPATH)/dependencies/zeromq/include \
-I $(FLUFFIPATH)/dependencies/libprotoc/include/siemens/cpp \
-I $(FLUFFIPATH)/dependencies/libprotoc/include \
-I $(FLUFFIPATH)/dependencies/easylogging/include \
-I $(FLUFFIPATH)/dependencies/libuuid/include
SHLIBOBJS =  $(FLUFFIPATH)/dependencies/libprotoc/lib/$(ARCH)/libprotobuf.a $(FLUFFIPATH)/dependencies/libuuid/lib/$(ARCH)/libuuid.a

# vars for TestcaseRunner
RUNSRCS := $(shell find $(FLUFFIPATH)/TestcaseRunner/ -iname '*.cpp' -not -name '*Win*')
RUNOBJS := $(addprefix $(ARCH)/buildtmp/runobj/,$(notdir $(RUNSRCS:.cpp=.o)))
RUNINCLUDE = $(SHINCLUDE) \
-I $(FLUFFIPATH)/SharedCode \
-I $(FLUFFIPATH)/SharedMemIPC \
-I $(FLUFFIPATH)/dependencies/dynamorio/include
RUNLIBOBJS = $(ARCH)/lib/sharedcode.a  $(FLUFFIPATH)/dependencies/libprotoc/lib/$(ARCH)/libprotobuf.a $(FLUFFIPATH)/dependencies/easylogging/lib/$(ARCH)/libeasyloggingpp.a $(FLUFFIPATH)/dependencies/libuuid/lib/$(ARCH)/libuuid.a

# vars for TestcaseGenerator
GENSRCS := $(shell find $(FLUFFIPATH)/TestcaseGenerator/ -iname '*.cpp' -not -name '*Win*')
GENOBJS := $(addprefix $(ARCH)/buildtmp/genobj/,$(notdir $(GENSRCS:.cpp=.o)))
GENINCLUDE = $(SHINCLUDE) \
-I $(FLUFFIPATH)/SharedCode \
-I $(FLUFFIPATH)/dependencies/afl/include \
-I $(FLUFFIPATH)/dependencies/honggfuzz/include
GENLIBOBJS = $(ARCH)/lib/sharedcode.a $(FLUFFIPATH)/dependencies/libprotoc/lib/$(ARCH)/libprotobuf.a $(FLUFFIPATH)/dependencies/easylogging/lib/$(ARCH)/libeasyloggingpp.a $(FLUFFIPATH)/dependencies/afl/lib/$(ARCH)/libafl.a $(FLUFFIPATH)/dependencies/honggfuzz/lib/$(ARCH)/libhonggfuzz.a $(FLUFFIPATH)/dependencies/libuuid/lib/$(ARCH)/libuuid.a

# vars for TestcaseEvaluator
EVALSRCS := $(shell find $(FLUFFIPATH)/TestcaseEvaluator/ -iname '*.cpp' -not -name '*Win*')
EVALOBJS := $(addprefix $(ARCH)/buildtmp/evalobj/,$(notdir $(EVALSRCS:.cpp=.o)))
EVALINCLUDE = $(SHINCLUDE) \
-I $(FLUFFIPATH)/SharedCode
EVALLIBOBJS = $(ARCH)/lib/sharedcode.a $(FLUFFIPATH)/dependencies/libprotoc/lib/$(ARCH)/libprotobuf.a $(FLUFFIPATH)/dependencies/easylogging/lib/$(ARCH)/libeasyloggingpp.a $(FLUFFIPATH)/dependencies/libuuid/lib/$(ARCH)/libuuid.a

# vars for GlobalManager
GMSRCS := $(wildcard $(FLUFFIPATH)/GlobalManager/*.cpp)
GMOBJS := $(addprefix $(ARCH)/buildtmp/gmobj/,$(notdir $(GMSRCS:.cpp=.o)))
GMINCLUDE = $(SHINCLUDE) \
-I $(FLUFFIPATH)/SharedCode \
-I $(FLUFFIPATH)/dependencies/mariadb/include
GMLIBOBJS = $(ARCH)/lib/sharedcode.a  $(FLUFFIPATH)/dependencies/libprotoc/lib/$(ARCH)/libprotobuf.a $(FLUFFIPATH)/dependencies/easylogging/lib/$(ARCH)/libeasyloggingpp.a $(FLUFFIPATH)/dependencies/libuuid/lib/$(ARCH)/libuuid.a

# vars for LocalManager
LMSRCS := $(wildcard $(FLUFFIPATH)/LocalManager/*.cpp)
LMOBJS := $(addprefix $(ARCH)/buildtmp/lmobj/,$(notdir $(LMSRCS:.cpp=.o)))
LMINCLUDE = $(SHINCLUDE) \
-I $(FLUFFIPATH)/SharedCode \
-I $(FLUFFIPATH)/dependencies/mariadb/include
LMLIBOBJS = $(ARCH)/lib/sharedcode.a $(FLUFFIPATH)/dependencies/libprotoc/lib/$(ARCH)/libprotobuf.a $(FLUFFIPATH)/dependencies/easylogging/lib/$(ARCH)/libeasyloggingpp.a $(FLUFFIPATH)/dependencies/libuuid/lib/$(ARCH)/libuuid.a

# vars for Shared Mem IPC
SMIPCSRCS := $(wildcard $(FLUFFIPATH)/SharedMemIPC/*.cpp)
SMIPCOBJS := $(addprefix $(ARCH)/buildtmp/smipcobj/,$(notdir $(SMIPCSRCS:.cpp=.o)))
SMIPCINCLUDE = -I $(FLUFFIPATH)/dependencies/libuuid/include
SMIPCLIBOBJS = $(FLUFFIPATH)/dependencies/libuuid/lib/$(ARCH)/libuuid.a

WARNINGS = -Wno-psabi -Wall -Wextra -Wconversion -Wsign-compare -Wlogical-op -Wnull-dereference -Wold-style-cast
DEBUG = $(WARNIGNS) -g3 -O0 -DDEBUG
RELEASE = $(WARNINGS) -s -g0 -O2

#By default compile as release (uncomment here for debug build)
#AFLAGS = $(DEBUG)
AFLAGS = $(RELEASE)
CC = g++ -std=c++11
CFLAGS = -fPIC -c
LDFLAGS = -static-libstdc++ -static-libgcc -pthread -lstdc++fs -ldl -Wl,-fuse-ld=gold

.PHONY: dirs clean tidy depcopy re

all: deps
	@$(MAKE) --no-print-directory tr lm gm tg te
	@$(MAKE) --no-print-directory $(ARCH)/lib/libsharedcode.so

deps: dirs
	@$(MAKE) --no-print-directory depcopy

gm: $(ARCH)/bin/GlobalManager

lm: $(ARCH)/bin/LocalManager

tr: $(ARCH)/bin/TestcaseRunner

tg: $(ARCH)/bin/TestcaseGenerator

te: $(ARCH)/bin/TestcaseEvaluator

$(ARCH)/buildtmp/shobj/%.o: $(FLUFFIPATH)/SharedCode/%.cpp
	@echo -e "[\033[94mcompiling\033[0m] "$(notdir $<)"..."
	@$(CC) $(AFLAGS) $(CFLAGS) $(SHINCLUDE) -o $@ $<

$(ARCH)/buildtmp/shobj/FLUFFI.pb.o: $(FLUFFIPATH)/dependencies/libprotoc/include/siemens/cpp/FLUFFI.pb.cc
	@echo -e "[\033[94mcompiling\033[0m] "$(notdir $<)"..."
	@$(CC) $(AFLAGS) $(CFLAGS) $(SHINCLUDE) -o $@ $<

$(ARCH)/lib/sharedcode.a: $(SHOBJS) $(ARCH)/buildtmp/shobj/FLUFFI.pb.o
	@echo -e "[\033[93mlinking\033[0m] "$(notdir $@)
	@ar rcs $@ $^

$(ARCH)/lib/libsharedcode.so: $(SHOBJS) $(SHLIBOBJS) $(ARCH)/buildtmp/shobj/FLUFFI.pb.o
	@echo -e "[\033[93mlinking\033[0m] "$(notdir $@)
	@$(CC) $(AFLAGS) -shared  -L $(ARCH)/bin  -o $@ $^ $(SHLIBOBJS) $(LDFLAGS) -l:libzmq.so.5 -Wl,-rpath='$${ORIGIN}'

$(ARCH)/buildtmp/runobj/%.o: $(FLUFFIPATH)/TestcaseRunner/%.cpp
	@echo -e "[\033[94mcompiling\033[0m] "$(notdir $<)"..."
	@$(CC) $(AFLAGS) $(CFLAGS) -D DR_DO_NOT_DEFINE_MAX_MIN -D_DR_BUILD_CONFIGURE_=\<build$(ARCH)/configure.h\> -D_DR_BUILD_CONFIG_=\<build$(ARCH)/include/dr_config.h\> $(RUNINCLUDE) -o $@ $<

$(ARCH)/buildtmp/genobj/%.o: $(FLUFFIPATH)/TestcaseGenerator/%.cpp
	@echo -e "[\033[94mcompiling\033[0m] "$(notdir $<)"..."
	@$(CC) $(AFLAGS) $(CFLAGS) $(GENINCLUDE) -o $@ $<

$(ARCH)/buildtmp/evalobj/%.o: $(FLUFFIPATH)/TestcaseEvaluator/%.cpp
	@echo -e "[\033[94mcompiling\033[0m] "$(notdir $<)"..."
	@$(CC) $(AFLAGS) $(CFLAGS) $(EVALINCLUDE) -o $@ $<

$(ARCH)/bin/TestcaseRunner: $(RUNOBJS) $(RUNLIBOBJS) $(ARCH)/bin/libsharedmemipc.so $(ARCH)/lib/sharedcode.a
	@echo -e "[\033[92mlinking\033[0m] "$(notdir $@)
	@$(CC) $(AFLAGS) -L $(ARCH)/bin -o $@ $(RUNOBJS) $(RUNLIBOBJS) $(ARCH)/lib/sharedcode.a $(LDFLAGS) -lsharedmemipc -l:libzmq.so.5 -Wl,-rpath='$${ORIGIN}'

$(ARCH)/bin/TestcaseGenerator: $(GENOBJS) $(GENLIBOBJS) $(ARCH)/lib/sharedcode.a 
	@echo -e "[\033[92mlinking\033[0m] "$(notdir $@)
	@$(CC) $(AFLAGS) -L $(ARCH)/bin -o $@ $(GENOBJS) $(GENLIBOBJS) $(ARCH)/lib/sharedcode.a  $(LDFLAGS) -l:libzmq.so.5 -Wl,-rpath='$${ORIGIN}'

$(ARCH)/bin/TestcaseEvaluator: $(EVALOBJS) $(EVALLIBOBJS) $(ARCH)/lib/sharedcode.a 
	@echo -e "[\033[92mlinking\033[0m] "$(notdir $@)
	@$(CC) $(AFLAGS) -L $(ARCH)/bin -o $@ $(EVALOBJS) $(EVALLIBOBJS) $(ARCH)/lib/sharedcode.a  $(LDFLAGS) -l:libzmq.so.5 -Wl,-rpath='$${ORIGIN}'

$(ARCH)/buildtmp/gmobj/%.o: $(FLUFFIPATH)/GlobalManager/%.cpp
	@echo -e "[\033[94mcompiling\033[0m] "$(notdir $<)"..."
	@$(CC) $(AFLAGS) $(CFLAGS) $(GMINCLUDE) -o $@ $<

$(ARCH)/bin/GlobalManager: $(GMOBJS) $(GMLIBOBJS) $(ARCH)/lib/sharedcode.a 
	@echo -e "[\033[92mlinking\033[0m] "$(notdir $@)
	@$(CC) $(AFLAGS) -L $(ARCH)/bin  -o $@ $(GMOBJS) $(GMLIBOBJS) $(ARCH)/lib/sharedcode.a  $(LDFLAGS) $(MYSQLLINK) -l:libzmq.so.5 -l:libmariadb.so.3 -Wl,-rpath='$${ORIGIN}'

$(ARCH)/buildtmp/lmobj/%.o: $(FLUFFIPATH)/LocalManager/%.cpp
	@echo -e "[\033[94mcompiling\033[0m] "$(notdir $<)"..."
	@$(CC) $(AFLAGS) $(CFLAGS) $(LMINCLUDE) -o $@ $<

$(ARCH)/bin/LocalManager: $(LMOBJS) $(LMLIBOBJS) $(ARCH)/lib/sharedcode.a 
	@echo -e "[\033[92mlinking\033[0m] "$(notdir $@)
	@$(CC) $(AFLAGS) -L $(ARCH)/bin  -o $@ $(LMOBJS) $(LMLIBOBJS) $(ARCH)/lib/sharedcode.a $(LDFLAGS) $(MYSQLLINK) -lbsd -l:libzmq.so.5 -l:libmariadb.so.3 -Wl,-rpath='$${ORIGIN}'

$(ARCH)/buildtmp/smipcobj/%.o: $(FLUFFIPATH)/SharedMemIPC/%.cpp
	@echo -e "[\033[94mcompiling\033[0m] "$(notdir $<)"..."
	@$(CC) $(AFLAGS) $(CFLAGS) $(SMIPCINCLUDE) -o $@ $<

$(ARCH)/bin/libsharedmemipc.so: $(SMIPCOBJS) $(SMIPCLIBOBJS)
	@echo -e "[\033[93mlinking\033[0m] "$(notdir $@)
	@$(CC) $(AFLAGS) -shared -o $@ $(SMIPCOBJS) $(SMIPCLIBOBJS) $(LDFLAGS) -lrt


dirs:
	@mkdir -p $(ARCH)/buildtmp/shobj $(ARCH)/buildtmp/runobj $(ARCH)/buildtmp/genobj $(ARCH)/buildtmp/evalobj $(ARCH)/buildtmp/gmobj $(ARCH)/buildtmp/lmobj $(ARCH)/buildtmp/smipcobj $(ARCH)/lib $(ARCH)/bin/dynamorio

tidy: 
	@echo -e "[\033[94mtidying project\033[0m]"
	@rm -rf $(ARCH)/buildtmp

depcopy: dirs
	@cp $(FLUFFIPATH)/dependencies/zeromq/lib/$(ARCH)/libzmq.so.5 $(ARCH)/bin/.
	@rsync -r $(FLUFFIPATH)/dependencies/dynamorio/dyndist$(ARCH)/ $(ARCH)/bin/dynamorio/ >/dev/null 2>&1
	@cp $(FLUFFIPATH)/dependencies/mariadb/lib/$(ARCH)/libmariadb.so.3 $(ARCH)/bin/.
	@cp $(FLUFFIPATH)/dependencies/radamsa/bin/$(ARCH)/radamsa.exe $(ARCH)/bin/.
	@cp $(FLUFFIPATH)/dependencies/qemu/${ARCH}/qemu-arm $(ARCH)/bin/.
	@cp $(FLUFFIPATH)/dependencies/qemu/${ARCH}/qemu-mips $(ARCH)/bin/.
	@cp $(FLUFFIPATH)/dependencies/qemu/${ARCH}/qemu-ppc $(ARCH)/bin/.

clean: tidy
	@echo -e "[\033[94mcleaning project\033[0m]"
	@rm -rf $(ARCH)/lib $(ARCH)/bin

re:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory deps
	@$(MAKE) --no-print-directory all

