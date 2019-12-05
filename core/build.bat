:: Copyright 2017-2019 Siemens AG
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
:: 
:: Author(s): Thomas Riedmaier, Roman Bendt


IF [%1]==[2017] (

	REM Requires Visual STUD 2017 Update 3	
	cd FluffiTester
	powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace '<PlatformToolset>v140</PlatformToolset>', '<PlatformToolset>v141</PlatformToolset>' } | sc $f.PSPath }"
	cd ..

	cd GlobalManager
	powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace '<PlatformToolset>v140</PlatformToolset>', '<PlatformToolset>v141</PlatformToolset>' } | sc $f.PSPath }"
	cd ..

	cd LocalManager
	powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace '<PlatformToolset>v140</PlatformToolset>', '<PlatformToolset>v141</PlatformToolset>' } | sc $f.PSPath }"
	cd ..
	
	cd SharedCode
	powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace '<PlatformToolset>v140</PlatformToolset>', '<PlatformToolset>v141</PlatformToolset>' } | sc $f.PSPath }"
	cd ..
	
	cd SharedMemIPC
	powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace '<PlatformToolset>v140</PlatformToolset>', '<PlatformToolset>v141</PlatformToolset>' } | sc $f.PSPath }"
	cd ..

	cd TestcaseEvaluator
	powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace '<PlatformToolset>v140</PlatformToolset>', '<PlatformToolset>v141</PlatformToolset>' } | sc $f.PSPath }"
	cd ..

	cd TestcaseGenerator
	powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace '<PlatformToolset>v140</PlatformToolset>', '<PlatformToolset>v141</PlatformToolset>' } | sc $f.PSPath }"
	cd ..

	cd TestcaseRunner
	powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace '<PlatformToolset>v140</PlatformToolset>', '<PlatformToolset>v141</PlatformToolset>' } | sc $f.PSPath }"
	cd ..

	cd Helpers
	powershell -Command "ls *.vcxproj -rec | %%{ $f=$_; (gc $f.PSPath) | %%{ $_ -replace '<PlatformToolset>v140</PlatformToolset>', '<PlatformToolset>v141</PlatformToolset>' } | sc $f.PSPath }"
	cd ..

	:: Fluffi x64
	"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" FluffiV1.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCTargets"
	if errorlevel 1 goto errorDone

	:: Fluffi x86
	"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" FluffiV1.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86 /property:VCTargetsPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCTargets"
	if errorlevel 1 goto errorDone

	:: Starter x64
	"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" helpers\Starter\Starter.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCTargets"
	if errorlevel 1 goto errorDone

	:: Starter x86
	"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" helpers\Starter\Starter.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86 /property:VCTargetsPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCTargets"
	if errorlevel 1 goto errorDone

	:: Feeder x64
	"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" helpers\Feeder\Feeder.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCTargets"
	if errorlevel 1 goto errorDone

	:: Feeder x86
	"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" helpers\Feeder\Feeder.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86 /property:VCTargetsPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCTargets"
	if errorlevel 1 goto errorDone
	
	:: Fuzzcmp x64
	"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" helpers\fuzzcmp\fuzzcmp.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCTargets"
	if errorlevel 1 goto errorDone

	:: Fuzzcmp x86
	"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" helpers\fuzzcmp\fuzzcmp.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86 /property:VCTargetsPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\VC\VCTargets"
	if errorlevel 1 goto errorDone
	
) ELSE (

	REM Requires Visual CPP Build Tools 2015 Update 3

	:: Fluffi x64
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" FluffiV1.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
	if errorlevel 1 goto errorDone

	:: Fluffi x86
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" FluffiV1.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
	if errorlevel 1 goto errorDone

	:: Starter x64
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" helpers\Starter\Starter.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
	if errorlevel 1 goto errorDone

	:: Starter x86
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" helpers\Starter\Starter.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
	if errorlevel 1 goto errorDone

	:: Feeder x64
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" helpers\Feeder\Feeder.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
	if errorlevel 1 goto errorDone

	:: Feeder x86
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" helpers\Feeder\Feeder.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
	if errorlevel 1 goto errorDone
	
	:: Fuzzcmp x64
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" helpers\fuzzcmp\fuzzcmp.sln /m  /t:Build /p:Configuration=Release /p:Platform=x64 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
	if errorlevel 1 goto errorDone

	:: Fuzzcmp x86
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" helpers\fuzzcmp\fuzzcmp.sln /m  /t:Build /p:Configuration=Release /p:Platform=x86 /property:VCTargetsPath="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\v140"
	if errorlevel 1 goto errorDone
	
)

:: Requires at least go 1.12

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
EXIT /B  1

:done
EXIT /B  0
