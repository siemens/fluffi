:: Copyright 2017-2019 Siemens AG
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
:: 
:: Author(s): Thomas Riedmaier, Roman Bendt

:: Requires Visual Studio 2017 build tools

SETLOCAL
IF NOT DEFINED VCVARSALL (
		ECHO Environment Variable VCVARSALL needs to be set!
		goto errorDone
)
ENDLOCAL

SETLOCAL
call %VCVARSALL% x64

:: Fluffi x64
MSBuild.exe FluffiV1.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto errorDone

:: Starter x64
MSBuild.exe helpers\Starter\Starter.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto errorDone

:: Feeder x64
MSBuild.exe helpers\Feeder\Feeder.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto errorDone

:: Fuzzcmp x64
MSBuild.exe helpers\fuzzcmp\fuzzcmp.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto errorDone

ENDLOCAL

SETLOCAL
call %VCVARSALL% x86

:: Fluffi x86
MSBuild.exe FluffiV1.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86
if errorlevel 1 goto errorDone

:: Starter x86
MSBuild.exe helpers\Starter\Starter.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86
if errorlevel 1 goto errorDone

:: Feeder x86
MSBuild.exe helpers\Feeder\Feeder.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86
if errorlevel 1 goto errorDone

:: Fuzzcmp x86
MSBuild.exe helpers\fuzzcmp\fuzzcmp.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86
if errorlevel 1 goto errorDone

ENDLOCAL

:: Requires at least go 1.12
SETLOCAL
set CGO_ENABLED=0
cd CaRRoT\
set GOARCH=386
go build -o CaRRoT-386.exe -v -a -ldflags "-s -w -extldflags ^"-static^"" .
if errorlevel 1 goto errorDone

set GOARCH=amd64
go build -o CaRRoT-amd64.exe -v -a -ldflags "-s -w -extldflags ^"-static^"" .
if errorlevel 1 goto errorDone

set CGO_ENABLED=0
cd ..\Oedipus\
set GOARCH=386
go build -o Oedipus-386.exe -v -a -ldflags "-s -w -extldflags ^"-static^"" .\cmd\Oedipus
if errorlevel 1 goto errorDone

set GOARCH=amd64
go build -o Oedipus-amd64.exe -v -a -ldflags "-s -w -extldflags ^"-static^"" .\cmd\Oedipus
if errorlevel 1 goto errorDone

cd ..
goto done

:errorDone
ENDLOCAL
EXIT /B  1

:done
ENDLOCAL
EXIT /B  0
