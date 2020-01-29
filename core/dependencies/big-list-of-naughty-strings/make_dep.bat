:: Copyright 2017-2019 Siemens AG
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
:: 
:: Author(s): Thomas Riedmaier

RMDIR /Q/S include

MKDIR include


REM Getting big-list-of-naughty-strings

RMDIR /Q/S big-list-of-naughty-strings

git clone https://github.com/minimaxir/big-list-of-naughty-strings.git
cd big-list-of-naughty-strings
git checkout e1968d982126f74569b7a2c8e50fe272d413a310
cd ..


REM Getting all lines that are not comments
findstr /b "[^#]" big-list-of-naughty-strings\blns.txt > big-list-of-naughty-strings\blns-noComments.txt

REM Transform it into something that can be included as a header file
echo std::deque^<std::string^> nasty { > include\blns.h
@echo off
for /f "tokens=*" %%a in (big-list-of-naughty-strings\blns-noComments.txt) do (echo std::string^(R"FLUFFI(%%a)FLUFFI"^), >> include\blns.h)
@echo on
echo std::string^("")}; >> include\blns.h

REM encode EOF characters differently (as VS can not handle this)
powershell -Command "(Get-Content include\blns.h) | ForEach-Object {$_ -replace [char]0x001A,')FLUFFI"")+std::string(1,(char)0x1a)+std::string(R""""FLUFFI('} | Set-Content include\blns.h"


waitfor SomethingThatIsNeverHappening /t 10 2>NUL
::reset errorlevel
ver > nul

RMDIR /Q/S big-list-of-naughty-strings

goto done

:errorDone
exit /B 1

:done
exit /B 0
