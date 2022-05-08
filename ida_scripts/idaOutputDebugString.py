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
# Author(s): Abian Blome, Thomas Riedmaier

import ida_loader
import ida_nalt
import idc

def patchFile(ea, offset):
    filename = ida_nalt.get_input_file_path()
    print "Patching file %s" % filename
    with open(filename, "r+b") as f:
        f.seek(offset)
        f.write("\x90" * 6)

xrefs = set()

def imp_cb(ea, name, ord):
    if name in ["OutputDebugStringA", "OutputDebugStringW"]:
        for xref in XrefsTo(ea, 0):
            xrefs.add((xref.frm, xref.to))
    return True

nimps = ida_nalt.get_import_module_qty()

print "Found %d import(s)..." % nimps

for i in xrange(0, nimps):
    name = ida_nalt.get_import_module_name(i)
    if name.upper() == "KERNEL32":
        print "Walking-> %s" % name
        ida_nalt.enum_import_names(i, imp_cb)
        break

for frm, to in xrefs:
    offset = ida_loader.get_fileregion_offset(frm)
    print "File location: %08x (%08x -> %08x)" % (offset, frm, to)
    try:
        patchFile(frm, offset)
    except:
        print(e)

