:: Copyright 2017-2019 Siemens AG
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
:: 
:: Author(s): Thomas Riedmaier

ECHO off

:parseArgs
:: asks for the -WITH_DEPS argument and store the value in the variable WITH_DEPS
call:getArgWithValue "-WITH_DEPS" "WITH_DEPS" "%~1" "%~2" && SHIFT && SHIFT && goto :parseArgs

:: asks for the -DEPLOY_TO_FTP argument and store the value in the variable DEPLOY_TO_FTP
call:getArgWithValue "-DEPLOY_TO_FTP" "DEPLOY_TO_FTP" "%~1" "%~2" && SHIFT && SHIFT && goto :parseArgs

:: resetting %ERRORLEVEL%
ver > nul

ECHO About to build FLUFFI with the following options:
ECHO WITH_DEPS: %WITH_DEPS%
ECHO DEPLOY_TO_FTP: %DEPLOY_TO_FTP%
ECHO.

:: =====================================================================
:: Building FLUFFI dependencies

IF "%WITH_DEPS%" == "TRUE" (
		ECHO Checking IF all build requirements for FLUFFI dependencies are installed
		WHERE git >nul 2>nul
		IF %ERRORLEVEL% NEQ 0 (
				ECHO git wasn't found 
				goto :err
				)
		IF NOT EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
				ECHO Build tools for Visual Studio 2017 C++ weren't found 
				goto :err
				)
		IF NOT EXIST "C:\Program Files\Git\usr\bin\patch.exe" (
				ECHO patch.exe wasn't found 
				goto :err
				)
		WHERE cmake >nul 2>nul
		IF %ERRORLEVEL% NEQ 0 (
				ECHO cmake wasn't found 
				goto :err
				)
		WHERE powershell >nul 2>nul
		IF %ERRORLEVEL% NEQ 0 (
				ECHO powershell wasn't found 
				goto :err
				)
		WHERE perl >nul 2>nul
		IF %ERRORLEVEL% NEQ 0 (
				ECHO perl wasn't found 
				goto :err
				)
		IF NOT EXIST "C:\cygwin64\bin\bash.exe" (
				ECHO cygwin64 isn't installed.
				goto :err
				)
		IF NOT EXIST "C:\cygwin64\bin\make.exe" (
				ECHO cygwin64 make isn't installed.
				goto :err
				)
		IF NOT EXIST "C:\cygwin64\bin\i686-pc-cygwin-gcc.exe" (
				ECHO cygwin64 gcc-i68 isn't installed.
				goto :err
				)
		IF NOT EXIST "C:\cygwin64\bin\gcc.exe" (
				ECHO cygwin64 gcc isn't installed.
				goto :err
				)

		ECHO Building Fluffi dependencies
		CD ..\..\core\dependencies
		call make_all_dep.bat
		IF errorlevel 1 goto :err
		CD ..\..\build\windows
		)

:: =====================================================================

:: =====================================================================
:: Building FLUFFI Core

ECHO Checking IF all tools needed to build FLUFFI are installed
IF NOT EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
		ECHO Build tools for Visual Studio 2017 C++ weren't found 
		goto :err
		)
WHERE go >nul 2>nul
IF %ERRORLEVEL% NEQ 0 (
		ECHO go wasn't found 
		goto :err
		)

ECHO Building Fluffi core
CD ..\..\core
call build.bat
IF errorlevel 1 goto :err
CD ..\build\windows

:: =====================================================================

:: =====================================================================
:: Deployment to fTP

IF "%DEPLOY_TO_FTP%" == "TRUE" (
		ECHO Checking IF all tools needed to deploy fluffi to the ftp server are installed
		IF NOT EXIST "C:\Program Files\7-Zip\7z.exe" (
				ECHO 7zip wasn't found 
				goto :err
				)


		IF NOT EXIST "C:\Program Files (x86)\WinSCP\winscp.com" (
				ECHO WinSCP wasn't found 
				goto :err
				)

		ECHO Deploying FLUFFI to ftp server
		call deploy2FTP.bat
		IF errorlevel 1 goto :err

		)


:: =====================================================================

ECHO Completed sucessfully :)
goto :eof

:: =====================================================================
:: This function sets a variable from a cli arg with value
:: 1 cli argument name
:: 2 variable name
:: 3 current Argument Name
:: 4 current Argument Value
:getArgWithValue
IF "%~3"=="%~1" (
		IF "%~4"=="" (
				:: unset the variable IF value is not provided
				SET "%~2="
				exit /B 1
				)
		SET "%~2=%~4"
		exit /B 0
		)
exit /B 1
goto :eof

:err
ECHO Something went wrong :(
exit /B 1

:eof
exit /B 0
