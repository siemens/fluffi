§§#!/bin/bash
§§#    zeromq build script
§§#    Copyright (C) 2017-2019 Siemens AG
§§#
§§#    This script is free software: you can redistribute it and/or modify
§§#    it under the terms of the GNU General Public License as published by
§§#    the Free Software Foundation, either version 3 of the License, or
§§#    (at your option) any later version.
§§#
§§#    This script is distributed in the hope that it will be useful,
§§#    but WITHOUT ANY WARRANTY; without even the implied warranty of
§§#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
§§#    GNU General Public License for more details.
§§#
§§#    You should have received a copy of the GNU General Public License
§§#    along with this script.  If not, see <https://www.gnu.org/licenses/>.
§§#
§§# Author(s): Thomas Riedmaier, Roman Bendt, Pascal Eckmann
§§
§§THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)
§§ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')
§§
§§rm -rf include
§§rm -rf bin/$ARCH
§§rm -rf lib/$ARCH
§§
§§mkdir -p include
§§mkdir -p bin/$ARCH
§§mkdir -p lib/$ARCH
§§
§§# Getting the C binaries
§§
§§rm -rf libzmq
§§
§§git clone https://github.com/zeromq/libzmq.git
§§cd libzmq
§§git checkout 2cb1240db64ce1ea299e00474c646a2453a8435b
§§
§§#Apply patch that links libgc++ staticly
§§patch -p0 < ../staticgcc.patch
§§
§§# Building the C libraries
§§mkdir -p build$ARCH
§§cd build$ARCH
§§cmake -DBUILD_TESTS=OFF ..
§§make -j$THREADS
§§cd ../..
§§
§§cp libzmq/build${ARCH}/lib/libzmq.so.5.2.1 lib/${ARCH}/libzmq.so.5
§§cp libzmq/include/zmq.h include
§§
§§
§§rm -rf libzmq
§§
§§# Getting the C++ bindings
§§
§§rm -rf cppzmq
§§
§§git clone https://github.com/zeromq/cppzmq.git
§§cd cppzmq
§§git checkout d25c58a05dc981a63a92f8b1f7ce848e7f21d008
§§cd ..
§§
§§cp cppzmq/zmq.hpp include
§§cp cppzmq/zmq_addon.hpp include
§§
§§rm -rf cppzmq
§§
