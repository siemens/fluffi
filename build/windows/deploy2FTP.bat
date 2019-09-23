:: Copyright 2017-2019 Siemens AG
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
:: 
:: Author(s): Thomas Riedmaier

echo off

del ..\..\core\x64\Release\x64.zip
del ..\..\core\Win32\Release\x86.zip

:: Creating x64 zip

FOR %%I in  (..\..\core\x64\Release\*.exe) DO "C:\Program Files\7-Zip\7z.exe" a ..\..\core\x64\Release\x64.zip %%I 
FOR %%I in  (..\..\core\x64\Release\*.dll) DO "C:\Program Files\7-Zip\7z.exe" a ..\..\core\x64\Release\x64.zip %%I

move ..\..\core\CaRRoT\CaRRoT-amd64.exe ..\..\core\CaRRoT\CaRRoT.exe
"C:\Program Files\7-Zip\7z.exe" a ..\..\core\x64\Release\x64.zip ..\..\core\CaRRoT\CaRRoT.exe
del ..\..\core\CaRRoT\CaRRoT.exe

move ..\..\core\Oedipus\Oedipus-amd64.exe ..\..\core\Oedipus\Oedipus.exe
"C:\Program Files\7-Zip\7z.exe" a ..\..\core\x64\Release\x64.zip ..\..\core\Oedipus\Oedipus.exe
del ..\..\core\Oedipus\Oedipus.exe

"C:\Program Files\7-Zip\7z.exe" a ..\..\core\x64\Release\x64.zip ..\..\core\helpers\Feeder\x64\Release\Tester.exe
"C:\Program Files\7-Zip\7z.exe" a -r ..\..\core\x64\Release\x64.zip  ..\..\core\x64\Release\dyndist64

:: Creating x86 zip

FOR %%I in  (..\..\core\Win32\Release\*.exe) DO "C:\Program Files\7-Zip\7z.exe" a ..\..\core\Win32\Release\x86.zip %%I 
FOR %%I in  (..\..\core\Win32\Release\*.dll) DO "C:\Program Files\7-Zip\7z.exe" a ..\..\core\Win32\Release\x86.zip %%I

move ..\..\core\CaRRoT\CaRRoT-386.exe ..\..\core\CaRRoT\CaRRoT.exe
"C:\Program Files\7-Zip\7z.exe" a ..\..\core\Win32\Release\x86.zip ..\..\core\CaRRoT\CaRRoT.exe
del ..\..\core\CaRRoT\CaRRoT.exe

move ..\..\core\Oedipus\Oedipus-386.exe ..\..\core\Oedipus\Oedipus.exe
"C:\Program Files\7-Zip\7z.exe" a ..\..\core\Win32\Release\x86.zip ..\..\core\Oedipus\Oedipus.exe
del ..\..\core\Oedipus\Oedipus.exe

"C:\Program Files\7-Zip\7z.exe" a ..\..\core\Win32\Release\x86.zip ..\..\core\helpers\Feeder\Win32\Release\Tester.exe
"C:\Program Files\7-Zip\7z.exe" a -r ..\..\core\Win32\Release\x86.zip  ..\..\core\Win32\Release\dyndist32

:: Pushing zips to ftp server

"C:\Program Files (x86)\WinSCP\winscp.com" /script=pushFluffiBinsToFluffiFTP.ftp
IF %ERRORLEVEL% NEQ 0  goto :err

del ..\..\core\x64\Release\x64.zip
del ..\..\core\Win32\Release\x86.zip

ECHO Pushing to FTP succeeded
goto :eof

:err
ECHO Pushing to FTP failed
exit /B 1

:eof
exit /B 0
