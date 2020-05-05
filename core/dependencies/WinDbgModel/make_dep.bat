:: Copyright 2017-2020 Siemens AG
:: 
:: Permission is hereby granted, free of charge, to any person
:: obtaining a copy of this software and associated documentation files (the
:: "Software"), to deal in the Software without restriction, including without
:: limitation the rights to use, copy, modify, merge, publish, distribute,
:: sublicense, and/or sell copies of the Software, and to permit persons to whom the
:: Software is furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be
:: included in all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
:: EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
:: MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
:: SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
:: OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
:: ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
:: DEALINGS IN THE SOFTWARE.
:: 
:: Author(s): Thomas Riedmaier

RMDIR /Q/S include

MKDIR include


REM Getting WinDbg-Libraries

RMDIR /Q/S WinDbg-Libraries

git clone https://github.com/microsoft/WinDbg-Libraries.git
cd WinDbg-Libraries
git checkout 8b1ba64c8cabb961f8a8cc8e46f7ce33cc09b83f
cd ..



copy WinDbg-Libraries\DbgModelCppLib\DbgModelClientEx.h include

::reset errorlevel
ver > nul

RMDIR /Q/S WinDbg-Libraries

goto done

:errorDone
exit /B 1

:done
exit /B 0
