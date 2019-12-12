:: Copyright 2017-2019 Siemens AG. All Rights Reserved.
::
:: Licensed under the Apache License 2.0 (the "License").  You may not use
:: this file except in compliance with the License.  You can obtain a copy
:: in the file LICENSE in the source distribution or at
:: https://www.openssl.org/source/license.html
::
:: Author(s): Thomas Riedmaier, Pascal Eckmann

RMDIR /Q/S include
RMDIR /Q/S include64
RMDIR /Q/S include86
RMDIR /Q/S lib

MKDIR include
MKDIR include64
MKDIR include64\openssl
MKDIR include86
MKDIR include86\openssl
MKDIR lib
MKDIR lib\x64
MKDIR lib\x86

REM Getting Openssl

RMDIR /Q/S openssl

git clone https://github.com/openssl/openssl.git
cd openssl
git checkout 97ace46e11dba4c4c2b7cb67140b6ec152cfaaf4

Rem Apply patch that allows naming the libraries according to a suffix

"C:\Program Files\Git\usr\bin\patch.exe" -p0 -i ..\suffix.patch

REM Actually build openssl

mkdir build64
mkdir build86
mkdir build64d
mkdir build86d
SETLOCAL
cd build64
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
set CL=/MP
set __CNF_LDLIBS=-static
perl ..\Configure VC-WIN64A --release no-tests no-unit-test no-asm enable-static-engine no-shared
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\nmake.exe"
cd ..
cd build64d
perl ..\Configure VC-WIN64A --debug no-tests no-unit-test no-asm enable-static-engine no-shared
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\nmake.exe"
cd ..
ENDLOCAL


SETLOCAL
cd build86
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
set CL=/MP
set __CNF_LDLIBS=-static
perl ..\Configure VC-WIN32 --release no-tests no-unit-test no-asm enable-static-engine no-shared
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\nmake.exe"
cd ..
cd build86d
perl ..\Configure VC-WIN32 --debug no-tests no-unit-test no-asm enable-static-engine no-shared
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\nmake.exe"
cd ..
ENDLOCAL
cd ..


copy openssl\build64\libcryptoMT.lib lib\x64
copy openssl\build64\libsslMT.lib lib\x64

copy openssl\build64d\libcryptoMTd.lib lib\x64
copy openssl\build64d\libsslMTd.lib lib\x64

copy openssl\build86\libcryptoMT.lib lib\x86
copy openssl\build86\libsslMT.lib lib\x86


copy openssl\build86d\libcryptoMTd.lib lib\x86
copy openssl\build86d\libsslMTd.lib lib\x86

copy openssl\build64\include\openssl\opensslconf.h include64\openssl
copy openssl\build86\include\openssl\opensslconf.h include86\openssl
xcopy openssl\include\*.h include /sy

copy openssl\ms\applink.c include

waitfor SomethingThatIsNeverHappening /t 10 2>NUL
::reset errorlevel
ver > nul

RMDIR /Q/S openssl

goto :eof

:err
exit /B 1

:eof
exit /B 0
