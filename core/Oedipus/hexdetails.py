#!/usr/bin/env python3
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
# Author(s): Roman Bendt, Thomas Riedmaier
import argparse

if __name__ == '__main__':
    p = argparse.ArgumentParser(prog='hexdetails', description='encode connection string for oedipus')
    p.add_argument('-u', '--user', default='fluffi_gm', help='database username')
    p.add_argument('-P', '--passwd', default='fluffi_gm', help='database password')
    p.add_argument('-m', '--host', default='db.fluffi', help='database host')
    p.add_argument('-p', '--port', default='3306', help='database port')
    p.add_argument('job', help='fuzzjob name (without prefix)')
    args = p.parse_args()

    s = args.user + ':' + '@tcp(' + args.host + ':' + args.port +')/fluffi_' + args.job
    print(s)
    st = ["{:x}".format(ord(i)) for i in s]
    er = ''
    for i in st:
        er += i

    print(er)
