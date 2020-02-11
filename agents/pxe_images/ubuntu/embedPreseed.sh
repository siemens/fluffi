#!/bin/sh
# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Pascal Eckmann

# $1 = path to an basic initrd.gz file
# $2 = dir for the new initrd file
# $3 = preseeding file to copy to initrd
# this script adds the given preseeding file to the initrd.gz

function print_usage {
 echo "Usage: sudo embedPreseed.sh <FILEPATH_TO_BASE_INITRD> <FILEPATH_TO_NEW_INITRD_FILE> <FILEPATH_TO_INSTALLFILE>\nThe last two arguments have to be absolute paths.\nExample: sudo ./embedPreseed.sh ./initrd.gz /home/user/ubuntu_netinstall/ubuntu-installer/amd64/initrdNew /home/user/ubuntu_netinstall/ubuntu-installer/amd64/preseed.cfg"
}

# check arguments
if [ $# -lt 3 ];then
  print_usage
  exit 1
fi

if ! [ -f "$1" ];then
  print_usage
  echo "File not exists: $1"
  exit 1
fi 

dirname=$(dirname "${2}")
if ! [ -d "$dirname" ];then
  print_usage
  echo "Directory not exists: $dirname"
  exit 1
fi

if ! [ -f "$3" ];then
  print_usage
  echo "File not exists: $3"
  exit 1
fi

echo "Trying to extract $1 ..." 
# extract initrd file
sudo gunzip -ckf $1 >> ./orginal_initrd 
if [ ! -d "tmp" ]; then
  sudo mkdir tmp 
fi
cd tmp
sudo cpio -id < ../orginal_initrd

# copy preseed 
echo "Copy preceeding $3 ..."
sudo cp "$3" preseed.cfg

# compress to initrd
echo "Compressing to $2 ..."
find . | cpio --create --format='newc' > $2 
gzip -f $2 

# clean up
cd ..
sudo rm -r tmp	
sudo rm ./orginal_initrd
