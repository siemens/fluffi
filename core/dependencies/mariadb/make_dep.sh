#!/bin/bash
#    mariadb-connector-c compile script
#    Copyright (C) 2017-2019 Siemens AG
#
#    This script is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Lesser General Public
#    License as published by the Free Software Foundation; either
#    version 2.1 of the License, or (at your option) any later version.
#
#    This script is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public
#    License along with this script; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#    
# Author(s): Thomas Riedmaier, Pascal Eckmann, Roman Bendt

THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)
ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')

rm -rf include
rm -rf lib/$ARCH

mkdir -p include
mkdir -p lib/$ARCH

# Compiling the libmaria library

rm -rf mariadb-connector-c

git clone https://github.com/MariaDB/mariadb-connector-c.git
cd mariadb-connector-c
git checkout bce6c8013805f203b38e52c979b22b3141334f3c
mkdir -p build$ARCH
cd build$ARCH
cmake .. -DWITH_SSL=OFF
make -j$THREADS
cd ../..

cp mariadb-connector-c/build${ARCH}/libmariadb/libmariadb.so lib/${ARCH}/libmariadb.so.3
cp mariadb-connector-c/build${ARCH}/include/mariadb_version.h include

cd mariadb-connector-c/include/
find . -name '*.h' -exec cp --parents \{\} ../../include/ \;
cd ../..


rm -rf mariadb-connector-c


