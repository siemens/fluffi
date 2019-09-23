:: Copyright 2017-2019 Siemens AG
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
:: 
:: Author(s): Thomas Riedmaier, Pascal Eckmann
RMDIR /Q/S bin

MKDIR bin
MKDIR bin\x64
MKDIR bin\x86

SET "mypath=%~dp0"
set "mypath=%mypath:\=/%"
set "mypath=%mypath:C:=/cygdrive/C%"
set "mypath=%mypath:D:=/cygdrive/D%"
set "mypath=%mypath:E:=/cygdrive/E%"
set "mypath=%mypath:F:=/cygdrive/F%"
set "mypath=%mypath:G:=/cygdrive/G%"


REM Getting OWL

RMDIR /Q/S owl

git clone https://gitlab.com/owl-lisp/owl.git
cd owl
git checkout e31f1c268da8f1e9d84cdbbec4ce5b222d72127a
cd ..


REM Getting radamsa

RMDIR /Q/S radamsa

git clone https://gitlab.com/akihe/radamsa
cd radamsa
git checkout 0bc6b489c895ce7c610b3d11a651a6b870143d6f
cd ..

REM Building OWL 64

C:\cygwin64\bin\bash --login -c "cd %mypath%/owl && sed -i -e 's/\r\+$//' ./tests/* && make owl "

REM Building radamsa 64

copy owl\bin\ol.exe radamsa\bin\ol.exe
C:\cygwin64\bin\bash --login -c "cd %mypath%/radamsa && LDFLAGS=-static make everything "

copy radamsa\bin\radamsa.exe bin\x64
copy C:\cygwin64\bin\cygwin1.dll bin\x64

REM Resetting for x86

C:\cygwin64\bin\bash --login -c "cd %mypath%/radamsa && git reset --hard && git clean -dXf && git clean -f"


REM Building radamsa 86

copy owl\bin\ol.exe radamsa\bin\ol.exe
C:\cygwin64\bin\bash --login -c "cd %mypath%/radamsa && CC=i686-pc-cygwin-gcc.exe LDFLAGS=-static make everything "

copy radamsa\bin\radamsa.exe bin\x86
copy C:\cygwin64\usr\i686-pc-cygwin\sys-root\usr\bin\cygwin1.dll bin\x86


waitfor SomethingThatIsNeverHappening /t 10 2>NUL
::reset errorlevel
ver > nul

RMDIR /Q/S radamsa
RMDIR /Q/S owl

goto :eof

:err
exit /B 1

:eof
exit /B 0
