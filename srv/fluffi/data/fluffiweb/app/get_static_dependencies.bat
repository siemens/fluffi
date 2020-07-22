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
:: Author(s): Thomas Riedmaier, Junes Najah

:: Getting bootstrap

RMDIR /Q/S  bootstrap
RMDIR /Q/S static\bootstrap
MKDIR static\bootstrap

git clone https://github.com/twbs/bootstrap.git
cd bootstrap
git checkout 0b9c4a4007c44201dce9a6cc1a38407005c26c86
cd ..

xcopy bootstrap\dist\* static\bootstrap /sy

RMDIR /Q/S  bootstrap

:: Getting bootswatch

RMDIR /Q/S  bootswatch
RMDIR /Q/S static\3rdParty\bootswatch
MKDIR static\3rdParty\bootswatch

git clone https://github.com/thomaspark/bootswatch.git
cd bootswatch
git checkout 0836a73018fd2c80409160f9ad6988670d1becd5
cd ..

copy bootswatch\dist\darkly\bootstrap.min.css static\3rdParty\bootswatch\bootstrap.min.css

RMDIR /Q/S  bootswatch

:: Getting cytoscape

RMDIR /Q/S  cytoscape.js
RMDIR /Q/S static\3rdParty\cytoscape
MKDIR static\3rdParty\cytoscape

git clone https://github.com/cytoscape/cytoscape.js.git
cd cytoscape.js
git checkout 83ba3ceea8946bfe5b757b2345e69bf7105bfbe5
cd ..

copy cytoscape.js\dist\cytoscape.min.js static\3rdParty\cytoscape\cytoscape.min.js


RMDIR /Q/S  cytoscape.js

:: Getting cytoscape-dagre

RMDIR /Q/S  cytoscape.js-dagre
RMDIR /Q/S static\3rdParty\cytoscape-dagre
MKDIR static\3rdParty\cytoscape-dagre


git clone https://github.com/cytoscape/cytoscape.js-dagre.git
cd cytoscape.js-dagre
git checkout ed130887870f8e057329390e47fdda735f7ec942
cd ..

copy cytoscape.js-dagre\cytoscape-dagre.js static\3rdParty\cytoscape-dagre\cytoscape-dagre.js


RMDIR /Q/S  cytoscape.js-dagre


:: Getting dagre

RMDIR /Q/S  dagre
RMDIR /Q/S static\3rdParty\dagre
MKDIR static\3rdParty\dagre


git clone https://github.com/dagrejs/dagre.git
cd dagre
git checkout 6c65e75ad68f29c924bd0cd8f2e855bb551c46ee
cd ..

copy dagre\dist\dagre.min.js static\3rdParty\dagre\dagre.min.js


RMDIR /Q/S  dagre

:: Getting jquery

RMDIR /Q/S  jquery
RMDIR /Q/S static\3rdParty\jquery
MKDIR static\3rdParty\jquery


git clone https://github.com/jquery/jquery.git
cd jquery
git checkout 7751e69b615c6eca6f783a81e292a55725af6b85
cd ..

copy jquery\dist\jquery.js static\3rdParty\jquery\jquery-2.1.4.js
copy jquery\dist\jquery.min.js static\bootstrap\jquery.min.js


RMDIR /Q/S  jquery

:: Getting socket.io-client

RMDIR /Q/S  socket.io-client
RMDIR /Q/S static\3rdParty\socket.io-client
MKDIR static\3rdParty\socket.io-client


git clone https://github.com/socketio/socket.io-client.git
cd socket.io-client
git checkout d30914d11b13e51ce1c1419d5cc99a74df72c2a7
cd ..

copy socket.io-client\dist\socket.io.slim.js static\3rdParty\socket.io-client\socket.io.slim.js


RMDIR /Q/S  socket.io-client
