:: Copyright 2017-2019 Siemens AG
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
:: 
:: Author(s): Thomas Riedmaier

IF NOT DEFINED VCVARSALL (
		ECHO Environment Variable VCVARSALL needs to be set!
		goto errorDone
)

RMDIR /Q/S lib

MKDIR lib
MKDIR lib\x64
MKDIR lib\x86


REM Getting big-list-of-naughty-strings

RMDIR /Q/S big-list-of-naughty-strings

git clone https://github.com/minimaxir/big-list-of-naughty-strings.git
cd big-list-of-naughty-strings
git checkout e1968d982126f74569b7a2c8e50fe272d413a310



REM Extracting all lines that are not comments
findstr /b "[^#]" blns.txt > blns-noComments.txt

REM Transform it into something that can be compiled
echo #include ^<deque^>  > blns.cpp
echo #include ^<string^>  >> blns.cpp
echo #include "../include/blns.h"  >> blns.cpp
echo std::deque^<std::string^> blns { >> blns.cpp
@echo off
for /f "tokens=*" %%a in (blns-noComments.txt) do (echo std::string^(R"FLUFFI(%%a)FLUFFI"^), >> blns.cpp)
@echo on
echo std::string^("")}; >> blns.cpp

REM encode EOF characters differently (as VS can not handle this)
powershell -Command "(Get-Content blns.cpp) | ForEach-Object {$_ -replace [char]0x001A,')FLUFFI"")+std::string(1,(char)0x1a)+std::string(R""""FLUFFI('} | Set-Content blns.cpp"


REM Building  lib

mkdir build64
mkdir build86
SETLOCAL
cd build64
call %VCVARSALL% x64

cl.exe /MT /c /EHsc ..\blns.cpp
LIB.EXE /OUT:blns.LIB blns.obj
cl.exe /MTd /Debug /c /EHsc ..\blns.cpp
LIB.EXE /OUT:blnsd.LIB blns.obj
cd ..
ENDLOCAL

SETLOCAL
cd build86
call %VCVARSALL% x86

cl.exe /MT /c /EHsc ..\blns.cpp
LIB.EXE /OUT:blns.LIB blns.obj
cl.exe /MTd /Debug /c /EHsc ..\blns.cpp
LIB.EXE /OUT:blnsd.LIB blns.obj
cd ..
ENDLOCAL
cd ..

copy big-list-of-naughty-strings\build64\blns.LIB lib\x64

copy big-list-of-naughty-strings\build64\blnsd.LIB lib\x64

copy big-list-of-naughty-strings\build86\blns.LIB lib\x86

copy big-list-of-naughty-strings\build86\blnsd.LIB lib\x86



waitfor SomethingThatIsNeverHappening /t 10 2>NUL
::reset errorlevel
ver > nul

RMDIR /Q/S big-list-of-naughty-strings

goto done

:errorDone
exit /B 1

:done
exit /B 0
