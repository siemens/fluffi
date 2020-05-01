::    zeromq build script
::    Copyright (C) 2017-2020 Siemens AG
::
::    This script is free software: you can redistribute it and/or modify
::    it under the terms of the GNU General Public License as published by
::    the Free Software Foundation, either version 3 of the License, or
::    (at your option) any later version.
::
::    This script is distributed in the hope that it will be useful,
::    but WITHOUT ANY WARRANTY; without even the implied warranty of
::    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
::    GNU General Public License for more details.
::
::    You should have received a copy of the GNU General Public License
::    along with this script.  If not, see <https://www.gnu.org/licenses/>.
::
:: Author(s): Thomas Riedmaier, Pascal Eckmann

IF NOT DEFINED VCVARSALL (
		ECHO Environment Variable VCVARSALL needs to be set!
		goto errorDone
)

RMDIR /Q/S include
RMDIR /Q/S bin
RMDIR /Q/S lib

MKDIR include
MKDIR bin
MKDIR bin\x64
MKDIR bin\x86
MKDIR lib
MKDIR lib\x64
MKDIR lib\x86

REM Getting the C binaries

RMDIR /Q/S libzmq

git clone https://github.com/zeromq/libzmq.git
cd libzmq
git checkout 2cb1240db64ce1ea299e00474c646a2453a8435b
mkdir build64
mkdir build86
cd build64
SETLOCAL
call %VCVARSALL% x64
cmake -G "Visual Studio 15 2017 Win64" -DBUILD_TESTS=OFF ..
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDebugDll', 'MultiThreadedDebug' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDll', 'MultiThreaded' } | sc $f.PSPath }"
MSBuild.exe ZeroMQ.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto errorDone
MSBuild.exe ZeroMQ.sln /m /t:Build /p:Configuration=Debug /p:Platform=x64
if errorlevel 1 goto errorDone
ENDLOCAL
cd ..
cd build86
SETLOCAL
call %VCVARSALL% x86
cmake -G "Visual Studio 15 2017" -DBUILD_TESTS=OFF ..
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDebugDll', 'MultiThreadedDebug' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDll', 'MultiThreaded' } | sc $f.PSPath }"
MSBuild.exe ZeroMQ.sln /m /t:Build /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 goto errorDone
MSBuild.exe ZeroMQ.sln /m /t:Build /p:Configuration=Debug /p:Platform=Win32
if errorlevel 1 goto errorDone
ENDLOCAL
cd ..
cd ..

copy libzmq\build64\bin\Release\libzmq-v141-mt-4_3_1.dll bin\x64
copy libzmq\build64\lib\Release\libzmq-v141-mt-4_3_1.lib lib\x64
copy libzmq\build64\lib\Release\libzmq-v141-mt-4_3_1.exp lib\x64

copy libzmq\build64\bin\Debug\libzmq-v141-mt-gd-4_3_1.dll bin\x64
copy libzmq\build64\bin\Debug\libzmq-v141-mt-gd-4_3_1.pdb bin\x64
copy libzmq\build64\lib\Debug\libzmq-v141-mt-gd-4_3_1.lib lib\x64
copy libzmq\build64\lib\Debug\libzmq-v141-mt-gd-4_3_1.exp lib\x64

copy libzmq\build86\bin\Release\libzmq-v141-mt-4_3_1.dll bin\x86
copy libzmq\build86\lib\Release\libzmq-v141-mt-4_3_1.lib lib\x86
copy libzmq\build86\lib\Release\libzmq-v141-mt-4_3_1.exp lib\x86

copy libzmq\build86\bin\Debug\libzmq-v141-mt-gd-4_3_1.dll bin\x86
copy libzmq\build86\bin\Debug\libzmq-v141-mt-gd-4_3_1.pdb bin\x86
copy libzmq\build86\lib\Debug\libzmq-v141-mt-gd-4_3_1.lib lib\x86
copy libzmq\build86\lib\Debug\libzmq-v141-mt-gd-4_3_1.exp lib\x86

copy libzmq\include\zmq.h include

waitfor SomethingThatIsNeverHappening /t 10 2>NUL
::reset errorlevel
ver > nul

RMDIR /Q/S libzmq

REM Getting the C++ bindings

RMDIR /Q/S cppzmq

git clone https://github.com/zeromq/cppzmq.git
cd cppzmq
git checkout d25c58a05dc981a63a92f8b1f7ce848e7f21d008
cd ..

copy cppzmq\zmq.hpp include
copy cppzmq\zmq_addon.hpp include

waitfor SomethingThatIsNeverHappening /t 10 2>NUL
::reset errorlevel
ver > nul

RMDIR /Q/S cppzmq

goto done

:errorDone
exit /B 1

:done
exit /B 0
