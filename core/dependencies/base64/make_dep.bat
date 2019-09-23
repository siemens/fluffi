§§:: Copyright 2017-2019 Siemens AG
§§:: 
§§:: This source code is provided 'as-is', without any express or implied
§§:: warranty. In no event will the author be held liable for any damages
§§:: arising from the use of this software.
§§::                                                                               
§§:: Permission is granted to anyone to use this software for any purpose,
§§:: including commercial applications, and to alter it and redistribute it
§§:: freely, subject to the following restrictions:
§§::                                                                               
§§:: 1. The origin of this source code must not be misrepresented; you must not
§§::    claim that you wrote the original source code. If you use this source code
§§::    in a product, an acknowledgment in the product documentation would be
§§::    appreciated but is not required.
§§::                                                                               
§§:: 2. Altered source versions must be plainly marked as such, and must not be
§§::    misrepresented as being the original source code.
§§::                                                                               
§§:: 3. This notice may not be removed or altered from any source distribution.
§§::
§§:: Author(s): Thomas Riedmaier, Pascal Eckmann
§§
§§RMDIR /Q/S include
§§RMDIR /Q/S lib
§§
§§MKDIR include
§§MKDIR lib
§§MKDIR lib\x64
§§MKDIR lib\x86
§§
§§REM Getting base64
§§
§§RMDIR /Q/S cpp-base64
§§
§§git clone https://github.com/ReneNyffenegger/cpp-base64.git
§§cd cpp-base64
§§git checkout a8aae956a2f07df9aac25b064cf4cd92d56aac45
§§
§§
§§REM Building base64 lib
§§
§§mkdir build64
§§mkdir build86
§§SETLOCAL
§§cd build64
§§call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
§§
§§cl.exe /MT /c /EHsc ..\base64.cpp
§§LIB.EXE /OUT:base64.LIB base64.obj
§§cl.exe /MTd /Debug /c /EHsc ..\base64.cpp
§§LIB.EXE /OUT:base64d.LIB base64.obj
§§cd ..
§§ENDLOCAL
§§
§§SETLOCAL
§§cd build86
§§call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
§§
§§cl.exe /MT /c /EHsc ..\base64.cpp
§§LIB.EXE /OUT:base64.LIB base64.obj
§§cl.exe /MTd /Debug /c /EHsc ..\base64.cpp
§§LIB.EXE /OUT:base64d.LIB base64.obj
§§cd ..
§§ENDLOCAL
§§cd ..
§§
§§copy cpp-base64\build64\base64.LIB lib\x64
§§
§§copy cpp-base64\build64\base64d.LIB lib\x64
§§
§§copy cpp-base64\build86\base64.LIB lib\x86
§§
§§copy cpp-base64\build86\base64d.LIB lib\x86
§§
§§
§§copy cpp-base64\base64.h include
§§
§§
§§waitfor SomethingThatIsNeverHappening /t 10 2>NUL
§§::reset errorlevel
§§ver > nul
§§
§§RMDIR /Q/S cpp-base64
§§
§§goto :eof
§§
§§:err
§§exit /B 1
§§
§§:eof
§§exit /B 0
