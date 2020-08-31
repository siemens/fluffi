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
# Author(s): Thomas Riedmaier, Roman Bendt

# Make sure only root can run our script
if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root. If it isn't we cannot create the zip archives in the docker-created folders (docker runs as root)" 1>&2
   exit 1
fi

rm arm32.zip
rm arm64.zip
rm x64.zip
rm x86.zip

if [ -d "../../core/x86-64/bin" ]; then cd ../../core/x86-64/bin; zip -x "*.o" -x "*.make" -x "*.cmake" -r x64.zip .; mv x64.zip ../../../build/ubuntu_based/; cd ../../../build/ubuntu_based/; else touch x64.zip; fi
if [ -d "../../core/Intel80386/bin" ]; then cd ../../core/Intel80386/bin; zip -x "*.o" -x "*.make" -x "*.cmake" -r x86.zip .; mv x86.zip ../../../build/ubuntu_based/; cd ../../../build/ubuntu_based/; else touch x86.zip; fi
if [ -d "../../core/ARM/bin" ]; then cd ../../core/ARM/bin; zip -x "*.o" -x "*.make" -x "*.cmake" -r arm32.zip .; mv arm32.zip ../../../build/ubuntu_based/; cd ../../../build/ubuntu_based/; else touch arm32.zip; fi
if [ -d "../../core/ARMaarch64/bin" ]; then cd ../../core/ARMaarch64/bin; zip -x "*.o" -x "*.make" -x "*.cmake" -r arm64.zip .; mv arm64.zip ../../../build/ubuntu_based/; cd ../../../build/ubuntu_based/; else touch arm64.zip; fi

ftp -inv ftp.fluffi << EOF
user anonymous anonymous
passive
cd /fluffi/linux/x64
delete fluffi.zip
put x64.zip fluffi.zip
cd /fluffi/linux/x86
delete fluffi.zip
put x86.zip fluffi.zip
cd /fluffi/linux/arm32
delete fluffi.zip
put arm32.zip fluffi.zip
cd /fluffi/linux/arm64
delete fluffi.zip
put arm64.zip fluffi.zip

bye

EOF

rm arm32.zip
rm arm64.zip
rm x64.zip
rm x86.zip

