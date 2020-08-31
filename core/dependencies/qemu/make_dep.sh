#!/bin/bash
#    qemu compile script
#    Copyright (C) 2017-2020 Siemens AG
#
#    This script is free software; you can redistribute it and/or
#    modify it under the terms of the GNU General Public License
#    as published by the Free Software Foundation; either version 2
#    of the License, or (at your option) any later version.
#
#    This script is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this script; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#    
# Author(s): Roman Bendt, Thomas Riedmaier

set -x

THREADS=$(nproc)
ARCH=$(file /bin/bash | awk -F',' '{print $2}' | tr -d ' ')

rm -rf upstream
rm -rf $ARCH
mkdir $ARCH

export GIT_DISCOVERY_ACROSS_FILESYSTEM=yes

#
# -___-"
#
# https://stackoverflow.com/a/22317479

git config --global core.compression 0
git clone --depth 1 https://git.qemu.org/git/qemu.git upstream
cd upstream
git fetch --unshallow
git pull --all

git checkout 474f3938d79ab36b9231c9ad3b5a9314c2aeacde
git apply ../474f3938d79ab36b9231c9ad3b5a9314c2aeacde.patch
./fluffi-build.sh
cd ..

cp ./upstream/build-arm/arm-linux-user/qemu-arm $ARCH/qemu-arm
cp ./upstream/build-mips/mips-linux-user/qemu-mips $ARCH/qemu-mips
cp ./upstream/build-ppc/ppc-linux-user/qemu-ppc $ARCH/qemu-ppc

rm -rf upstream

