# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Abian Blome, Thomas Riedmaier

import idaapi
import ida_nalt
import idautils
import idc
from sqlalchemy import Column, String, Integer, Boolean, ForeignKey, Date, create_engine, func
from sqlalchemy.orm import relationship, sessionmaker                
from sqlalchemy.ext.declarative import declarative_base
import csv

Base = declarative_base()

class BasicBlocks(Base):
    __tablename__ = 'basic_blocks'

    id = Column(Integer, primary_key=True)
    offset = Column(Integer)
    reached = Column(Boolean)
    when = Column(Date)
    testcase = Column(String)

    def __init__(self, offset, reached, when, testcase):
        self.offset = offset
        self.reached = reached
        self.when = when
        self.testcase = testcase

filename = idaapi.ask_file(False, '*.csv', 'Please select csv')

blocks = []

with open(filename) as csvfile:
    readCSV = csv.reader(csvfile, delimiter=";", quotechar='"')
    next(readCSV)
    for row in readCSV:
        print row
        blocks.append(int(row[4]))



for bb in blocks:
    absPos = bb + ida_nalt.get_imagebase()
    f = idaapi.get_func(absPos)
    if f is None:
        continue
    fc = idaapi.FlowChart(f)
    # Highlight the complete function
    idc.SetColor(absPos, idc.CIC_FUNC, 0xFFF000)
    for block in fc:
        if block.startEA <= absPos and block.endEA > absPos:
            #print "Setting colour for %x" % absPos
            for i in Heads(block.startEA, block.endEA):
                idc.set_color(i, CIC_ITEM, 0xFFAA00)

