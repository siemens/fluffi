§§#!/usr/bin/env python3
§§# Copyright 2017-2019 Siemens AG
§§# 
§§# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§# 
§§# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§# 
§§# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§# 
§§# Author(s): Roman Bendt, Thomas Riedmaier
§§
§§import math
§§import matplotlib.pyplot as plt
§§
§§fd = open('./positions.csv', 'r')
§§t = fd.read()
§§fd.close()
§§vals = t[1:-1].split()[::100]
§§
§§# fd = open('./breaks56250.csv', 'r')
§§# t = fd.read()
§§# fd.close()
§§# breaks100 = t[1:-1].split() #[::100]
§§
§§fd = open('./breaks100000.csv', 'r')
§§t = fd.read()
§§fd.close()
§§breaks1000 = t[1:-1].split() #[::100]
§§
§§plt.scatter([x*100 for x in range(len(vals))], [int(x) for x in vals], marker='+')
§§
§§# for i in breaks100:
§§#     plt.plot( (int(i),int(i)) , (0,int(vals[-1])), 'r-')
§§for i in breaks1000:
§§    plt.plot( (int(i),int(i)) , (0,int(vals[-1])), 'k-', dashes=[3,3])
§§
§§plt.show()
§§
