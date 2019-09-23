§§:: Copyright 2017-2019 Siemens AG
§§:: 
§§:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§:: 
§§:: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§:: 
§§:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§:: 
§§:: Author(s): Thomas Riedmaier
§§
§§If NOT exist "%~dp0\gdb.exe" (
§§powershell -Command "Invoke-WebRequest https://datapacket.dl.sourceforge.net/project/mingw-w64/External%%20binary%%20packages%%20%%28Win64%%20hosted%%29/gdb/x86_64-w64-mingw32-gdb-7.1.90.20100730.zip -OutFile x86_64-w64-mingw32-gdb-7.1.90.20100730.zip -UserAgent 'Mozilla/5.0 (Windows NT 10.0; WOW64; rv:60.0) Gecko/20100101 Firefox/60.0'"
§§
§§powershell -Command "Add-Type -Assembly System.IO.Compression.FileSystem; [IO.Compression.ZipFile]::OpenRead(\""$(resolve-path x86_64-w64-mingw32-gdb-7.1.90.20100730.zip)\"").Entries | where {$_.FullName -like 'mingw64/bin/gdb.exe'} | foreach {[System.IO.Compression.ZipFileExtensions]::ExtractToFile($_, \""$(resolve-path %~dp0)\gdb.exe\"", $true)}"
§§
§§powershell -Command "Add-Type -Assembly System.IO.Compression.FileSystem; [IO.Compression.ZipFile]::OpenRead(\""$(resolve-path x86_64-w64-mingw32-gdb-7.1.90.20100730.zip)\"").Entries | where {$_.FullName -like 'mingw64/bin/libiconv-2.dll'} | foreach {[System.IO.Compression.ZipFileExtensions]::ExtractToFile($_, \""$(resolve-path %~dp0)\libiconv-2.dll\"", $true)}"
§§
§§del x86_64-w64-mingw32-gdb-7.1.90.20100730.zip
§§
§§)
§§
