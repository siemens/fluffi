§§:: Copyright 2017-2019 Siemens AG
§§:: 
§§:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§:: 
§§:: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§:: 
§§:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§:: 
§§:: Author(s): Thomas Riedmaier
§§
§§:: Requirements for building these dependencies:
§§:: - Visual Studio 2015 compile chain (needed for almost everything)
§§:: - Visual Studio 2013 compile chain (needed for dynamorio)
§§:: - Strawberry Perl (needed for dynamorio and openssl)
§§:: - Cygwin64 with 32 and 64 bit gcc and make (radamsa)
§§:: - Network access to github and gitlab
§§
§§cd zeromq
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§cd libprotoc
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§cd dynamorio
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§cd mariadb
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§cd radamsa
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§cd easylogging
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§cd openssl
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§cd base64
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§cd afl
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§cd honggfuzz
§§call make_dep.bat
§§IF errorlevel 1 goto :err
§§cd ..
§§
§§goto :eof
§§
§§:err
§§exit /B 1
§§
§§:eof
§§exit /B 0
