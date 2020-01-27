#!/bin/bash
# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier, Roman Bendt

# Getting bootswatch

(
rm -rf bootswatch static/3rdParty/bootswatch
mkdir -p static/3rdParty/bootswatch
git clone https://github.com/thomaspark/bootswatch.git
cd bootswatch
git checkout 0836a73018fd2c80409160f9ad6988670d1becd5
cd ..
cp bootswatch/dist/darkly/bootstrap.min.css static/3rdParty/bootswatch/bootstrap.min.css
rm -rf  bootswatch
)&

# Getting cytoscape

(
rm -rf cytoscape.js static/3rdParty/cytoscape
mkdir -p static/3rdParty/cytoscape
git clone https://github.com/cytoscape/cytoscape.js.git
cd cytoscape.js
git checkout 83ba3ceea8946bfe5b757b2345e69bf7105bfbe5
cd ..
cp cytoscape.js/dist/cytoscape.min.js static/3rdParty/cytoscape/cytoscape.min.js
rm -rf  cytoscape.js
)&

# Getting cytoscape-dagre

(
rm -rf cytoscape.js-dagre static/3rdParty/cytoscape-dagre
mkdir -p static/3rdParty/cytoscape-dagre
git clone https://github.com/cytoscape/cytoscape.js-dagre.git
cd cytoscape.js-dagre
git checkout ed130887870f8e057329390e47fdda735f7ec942
cd ..
cp cytoscape.js-dagre/cytoscape-dagre.js static/3rdParty/cytoscape-dagre/cytoscape-dagre.js
rm -rf  cytoscape.js-dagre
)&

# Getting dagre

(
rm -rf dagre static/3rdParty/dagre
mkdir -p static/3rdParty/dagre
git clone https://github.com/dagrejs/dagre.git
cd dagre
git checkout 6c65e75ad68f29c924bd0cd8f2e855bb551c46ee
cd ..
cp dagre/dist/dagre.min.js static/3rdParty/dagre/dagre.min.js
rm -rf dagre
)&

# Getting jquery

(
rm -rf jquery static/3rdParty/jquery
mkdir -p static/3rdParty/jquery
git clone https://github.com/jquery/jquery.git
cd jquery
git checkout 7751e69b615c6eca6f783a81e292a55725af6b85
cd ..
cp jquery/dist/jquery.js static/3rdParty/jquery/jquery-2.1.4.js
rm -rf  jquery
)&

# Getting socket.io-client

(
rm -rf socket.io-client static/3rdParty/socket.io-client
mkdir -p static/3rdParty/socket.io-client
git clone https://github.com/socketio/socket.io-client.git
cd socket.io-client
git checkout d30914d11b13e51ce1c1419d5cc99a74df72c2a7
cd ..
cp socket.io-client/dist/socket.io.slim.js static/3rdParty/socket.io-client/socket.io.slim.js
rm -rf  socket.io-client
)&

wait
