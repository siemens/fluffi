:: ***************************************************************************
:: Copyright (c) 2017-2020 Siemens AG  All rights reserved.
:: ***************************************************************************/
::
::
:: Redistribution and use in source and binary forms, with or without
:: modification, are permitted provided that the following conditions are met:
::
:: * Redistributions of source code must retain the above copyright notice,
::   this list of conditions and the following disclaimer.
::
:: * Redistributions in binary form must reproduce the above copyright notice,
::   this list of conditions and the following disclaimer in the documentation
::   and/or other materials provided with the distribution.
::
:: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
:: AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
:: IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
:: ARE DISCLAIMED. IN NO EVENT SHALL Siemens AG OR CONTRIBUTORS BE LIABLE
:: FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
:: DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
:: SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
:: CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
:: LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
:: OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
:: DAMAGE.
:: 
:: Author(s): Thomas Riedmaier, Pascal Eckmann, Roman Bendt

IF NOT DEFINED VCVARSALL (
		ECHO Environment Variable VCVARSALL needs to be set!
		goto errorDone
)

RMDIR /Q/S include
RMDIR /Q/S dyndist32
RMDIR /Q/S dyndist64


MKDIR include
MKDIR dyndist32
MKDIR dyndist64


RMDIR /Q/S dynamorio

REM Getting dynamorio from git

git clone https://github.com/DynamoRIO/dynamorio.git
cd dynamorio
git checkout b9d0d9efebcc05cbc63811337cb687ccfeda149c

REM Copy files for the drcovMulti module
MKDIR clients\drcovMulti
copy ..\drcovMulti\drcovMulti.c clients\drcovMulti\
copy ..\drcovMulti\CMakeLists.txt clients\drcovMulti\

REM Apply patches for drcovlib that enables drcovMulti

"C:\Program Files\Git\usr\bin\patch.exe" ext\drcovlib\drcovlib.c ..\patch_for_drmulticov\drcovlib.c.patch
"C:\Program Files\Git\usr\bin\patch.exe" ext\drcovlib\drcovlib.h ..\patch_for_drmulticov\drcovlib.h.patch

REM Building dynamorio

mkdir build64
mkdir build86
cd build64
SETLOCAL
call %VCVARSALL% x64
SET PATH=%VCToolsInstallDir%bin\Hostx86\x86;%PATH%
cmake -G "Visual Studio 15 2017 Win64" -DBUILD_CORE=ON -DBUILD_DOCS=OFF -DBUILD_DRSTATS=OFF -DBUILD_TOOLS=ON -DBUILD_SAMPLES=OFF -DBUILD_TESTS=OFF -DDEBUG=OFF -DCMAKE_WARN_DEPRECATED=OFF  ..
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDebugDll', 'MultiThreadedDebug' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDll', 'MultiThreaded' } | sc $f.PSPath }"
MSBuild.exe DynamoRIO.sln /m /t:Build /p:Configuration=RelWithDebInfo /p:Platform=x64
if errorlevel 1 goto errorDone
ENDLOCAL
cd ..
cd build86
SETLOCAL
call %VCVARSALL% x86
SET PATH=%VCToolsInstallDir%bin\Hostx86\x86;%PATH%
cmake -G "Visual Studio 15 2017"  -DBUILD_CORE=ON -DBUILD_DOCS=OFF -DBUILD_DRSTATS=OFF -DBUILD_TOOLS=ON -DBUILD_SAMPLES=OFF -DBUILD_TESTS=OFF -DDEBUG=OFF -DCMAKE_WARN_DEPRECATED=OFF  ..
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDebugDll', 'MultiThreadedDebug' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDll', 'MultiThreaded' } | sc $f.PSPath }"
MSBuild.exe DynamoRIO.sln /m /t:Build /p:Configuration=RelWithDebInfo /p:Platform=Win32
if errorlevel 1 goto errorDone
ENDLOCAL
cd ..
cd ..

xcopy dynamorio\build64\*.dll dyndist64 /sy
xcopy dynamorio\build64\*.exe dyndist64 /sy
xcopy dynamorio\build64\*.drrun64 dyndist64 /sy

xcopy dynamorio\build86\*.dll dyndist32 /sy
xcopy dynamorio\build86\*.exe dyndist32 /sy
xcopy dynamorio\build86\*.drrun32 dyndist32 /sy

xcopy dynamorio\*.h include /sy

waitfor SomethingThatIsNeverHappening /t 10 2>NUL
::reset errorlevel
ver > nul

RMDIR /Q/S dynamorio

REM create a minimal drcovlib.h, that is easy to include
powershell -Command "([string]::Join(\"`n\", $(cat include\ext\drcovlib\drcovlib.h)) -replace '(?ms)^.*typedef struct _bb_entry_t', 'typedef struct _bb_entry_t') -replace '(?ms)} bb_entry_t;.*', '} bb_entry_t;' | Out-file -encoding ASCII include\ext\drcovlib\drcovlib_min.h"

goto done

:errorDone
exit /B 1

:done
exit /B 0
