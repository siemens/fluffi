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
import idc

from sqlalchemy import create_engine

class SelectBox(idaapi.Choose2):
    def __init__(self, title, elements):
        idaapi.Choose2.__init__(self, title, [
            ['ID', 8],
            ['Name', 64],
        ])
        self.elements = [
            [fuzzjob, name]
            for (fuzzjob, name) in elements
        ]

    def OnClose(self):
        pass

    def OnGetSize(self):
        return len(self.elements)

    def OnGetLine(self, n):
        return self.elements[n]

database_string = 'mysql+pymysql://fluffi_gm:fluffi_gm@db.fluffi:3306/fluffi_gm'

# Step Nr. 1: Connect to db.fluffi and retrieve a list
engine = create_engine(database_string)
with engine.connect() as con:
    fuzzjobsDB = con.execute('SELECT * FROM fuzzjob')

#print "Fuzzjobs:"
fuzzjobs = []
for row in fuzzjobsDB:
    #print " %s" % row
    fuzzjobs.append(row)

# Step Nr. 2: Let user select fuzzjob
fuzzjobList = []
for fuzzjob in fuzzjobs:
    fuzzjobList.append( (str(fuzzjob[0]), fuzzjob[1]) )

a = SelectBox("Select a fuzzjob", fuzzjobList)
selected_fuzzjob = fuzzjobs[a.Show(True)]

# Step Nr. 3: Retrieve Module List (remember, GDB need to be handled different, module 0 means raw address, otherwise it has the segment name included)
hostname = selected_fuzzjob[2]
username = selected_fuzzjob[3]
password = selected_fuzzjob[4]
dbname = selected_fuzzjob[5]

database_string = "mysql+pymysql://%s:%s@%s/%s" % (username, password, hostname, dbname)
#print database_string

engine = create_engine(database_string)
with engine.connect() as con:
    modulesDB = con.execute('SELECT * FROM target_modules')
    settingsDB = con.execute('SELECT * FROM settings')

#print "Modules:"
modules = {}
for row in modulesDB:
    #print " %s" % row
    modules[row[0]] = row

#print "Settings:"
#for row in settingsDB:
#    print " %s" % row

# Step Nr. 4: Let user select module
moduleList = []
for module in modules.values():
    moduleList.append( (str(module[0]), module[1]) )

a = SelectBox("Select a module", moduleList)
selected_module = a.Show(True) + 1
print "Selected module: %d" % selected_module

rawModule = False
if moduleList[selected_module-1][1] == 'NULL':
    rawModule = True

# Step Nr. 5: Let user change offset (optional)
offset = idaapi.asklong(0, "Add offset")

# Step Nr. 6: Retrieve covered blocks
engine = create_engine(database_string)
with engine.connect() as con:
    #blocksDB = con.execute('SELECT Offset FROM covered_blocks WHERE ModuleID = %d' % selected_module)
    blocksDistinctDB = con.execute('SELECT DISTINCT Offset FROM covered_blocks WHERE ModuleID = %d' % selected_module)
print "Found ? block(s) (%d distinct)" % (blocksDistinctDB.rowcount)

# Step Nr. 7: Color the currently loaded binary
for (bb,) in blocksDistinctDB:
    absPos = bb + offset
    if not rawModule:
        absPos += ida_nalt.get_imagebase()
    print absPos
        
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
