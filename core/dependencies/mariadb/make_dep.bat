::    mariadb-connector-c compile script
::    Copyright (C) 2017-2019 Siemens AG
::
::    This script is free software; you can redistribute it and/or
::    modify it under the terms of the GNU Lesser General Public
::    License as published by the Free Software Foundation; either
::    version 2.1 of the License, or (at your option) any later version.
::
::    This script is distributed in the hope that it will be useful,
::    but WITHOUT ANY WARRANTY; without even the implied warranty of
::    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
::    Lesser General Public License for more details.
::
::    You should have received a copy of the GNU Lesser General Public
::    License along with this script; if not, write to the Free Software
::    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
::    
:: Author(s): Thomas Riedmaier, Pascal Eckmann

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

REM Compiling the libmaria library

RMDIR /Q/S mariadb-connector-c

git clone https://github.com/MariaDB/mariadb-connector-c.git
cd mariadb-connector-c
git checkout bce6c8013805f203b38e52c979b22b3141334f3c
mkdir build64
mkdir build86
cd build64
cmake -G "Visual Studio 14 2015 Win64" ..
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDebugDll', 'MultiThreadedDebug' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDll', 'MultiThreaded' } | sc $f.PSPath }"
"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" mariadb-connector-c.sln /m /t:Build /p:Configuration=Release /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'libmariadb</TargetName>', 'libmariadbd</TargetName>' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'libmariadb.pdb', 'libmariadbd.pdb' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'libmariadb.lib', 'libmariadbd.lib' } | sc $f.PSPath }"
"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" mariadb-connector-c.sln /m /t:Build /p:Configuration=Debug /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
cd ..
cd build86
cmake -G "Visual Studio 14 2015" ..
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDebugDll', 'MultiThreadedDebug' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'MultiThreadedDll', 'MultiThreaded' } | sc $f.PSPath }"
"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" mariadb-connector-c.sln /m /t:Build /p:Configuration=Release /p:Platform=Win32 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'libmariadb</TargetName>', 'libmariadbd</TargetName>' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'libmariadb.pdb', 'libmariadbd.pdb' } | sc $f.PSPath }"
powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace 'libmariadb.lib', 'libmariadbd.lib' } | sc $f.PSPath }"
"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" mariadb-connector-c.sln /m /t:Build /p:Configuration=Debug /p:Platform=Win32 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
cd ..
cd ..


copy mariadb-connector-c\build64\libmariadb\Release\libmariadb.dll bin\x64
copy mariadb-connector-c\build64\libmariadb\Release\libmariadb.lib lib\x64
copy mariadb-connector-c\build64\libmariadb\Release\libmariadb.exp lib\x64

copy mariadb-connector-c\build64\libmariadb\Debug\libmariadbd.dll bin\x64
copy mariadb-connector-c\build64\libmariadb\Debug\libmariadbd.pdb bin\x64
copy mariadb-connector-c\build64\libmariadb\Debug\libmariadbd.lib lib\x64
copy mariadb-connector-c\build64\libmariadb\Debug\libmariadbd.exp lib\x64

copy mariadb-connector-c\build86\libmariadb\Release\libmariadb.dll bin\x86
copy mariadb-connector-c\build86\libmariadb\Release\libmariadb.lib lib\x86
copy mariadb-connector-c\build86\libmariadb\Release\libmariadb.exp lib\x86

copy mariadb-connector-c\build86\libmariadb\Debug\libmariadbd.dll bin\x86
copy mariadb-connector-c\build86\libmariadb\Debug\libmariadbd.pdb bin\x86
copy mariadb-connector-c\build86\libmariadb\Debug\libmariadbd.lib lib\x86
copy mariadb-connector-c\build86\libmariadb\Debug\libmariadbd.exp lib\x86


xcopy mariadb-connector-c\include\*.h include /sy
copy mariadb-connector-c\build64\include\mariadb_version.h include

waitfor SomethingThatIsNeverHappening /t 10 2>NUL
::reset errorlevel
ver > nul

RMDIR /Q/S mariadb-connector-c

goto :eof

:err
exit /B 1

:eof
exit /B 0
