#!/bin/sh
# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Pascal Eckmann, Thomas Riedmaier

case $1 in
	"API"*)
		echo "FLUFFIMONITORING"
		;;
	"TYPE"*)
		echo "STATUS"
		;;
	"NAME"*)
		echo "cpuload"
		;;
	"RUN"*)
		if [ -f "/etc/issue" ] ; then
			NUMCPUS=$(grep -c '^processor' /proc/cpuinfo)
			JMP=7
			COL=9

			# grep returns 1 on not found. if catches if one, fails if 0
			#grep -c 'Ubuntu' "/etc/issue" 1>/dev/null 2>&1
			#if [ $? = 0 ] ; then
			#	#echo "Ubuntu"
			#fi
			#grep -c 'Mint' "/etc/issue" 1>/dev/null 2>&1
			#if [ $?  = 0 ] ; then
			#	#echo "Mint"
			#fi
			grep -c 'Alpine' "/etc/issue" 1>/dev/null 2>&1
			if [ $? = 0 ] ; then
				JMP=4
				COL=8
			fi
			top -bn 1 | awk "NR>${JMP}{s+=\$${COL}} END {print s/${NUMCPUS}}"
		fi
		;;
esac


