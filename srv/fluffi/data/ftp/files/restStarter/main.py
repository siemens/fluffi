§§# Copyright 2017-2019 Siemens AG
§§# 
§§# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§# 
§§# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§# 
§§# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§# 
§§# Author(s): Michael Kraus, Thomas Riedmaier
§§
§§import time
§§from http.server import BaseHTTPRequestHandler, HTTPServer
§§from urllib.parse import urlparse
§§from urllib.parse import parse_qs
§§import json
§§import subprocess
§§import sys
§§from pathlib import Path, PureWindowsPath
§§import os
§§import socket
§§import errno
§§
§§HOST_NAME = '0.0.0.0'
§§PORT_NUMBER = 9000
§§SOURCE_FOLDER = ''
§§
§§
§§class InstanceHandler(BaseHTTPRequestHandler):
§§    def do_HEAD(self):
§§        self.send_response(200)
§§        self.send_header('Content-type', 'text/html')
§§        self.end_headers()
§§
§§    def do_GET(self):
§§        paths = {
§§            '/start': {'status': 200},
§§        }
§§        if self.path.split("?")[0] in paths:
§§            self.respond(paths[self.path.split("?")[0]])
§§        else:
§§            self.respond({'status': 500})
§§
§§    def handle_http(self, status_code, path):
§§        self.send_response(status_code)
§§        self.send_header('Content-type', 'application/json')
§§        self.end_headers()
§§        content = {"status":'ok'}
§§        o = urlparse(path)
§§        data = parse_qs(o.query)
§§        if data:
§§            arch=data["cmd"][0].split("T")[0]
§§            restPath=data["cmd"][0].split("/")[1]
§§            cmdPath=os.path.join(SOURCE_FOLDER,arch.split("/")[0], restPath)
§§            if os.name == 'nt':
§§                cmdPath= "start /MIN " + cmdPath
§§            try:
§§                p = subprocess.Popen(cmdPath, shell=True, stdin=subprocess.PIPE, cwd=os.path.join(SOURCE_FOLDER,arch.split("/")[0]))
§§            except:
§§                content = {"status":'error'}
§§        return bytes(json.dumps(content), 'UTF-8')
§§
§§    def respond(self, opts):
§§        response = self.handle_http(opts['status'], self.path)
§§        self.wfile.write(response)
§§
§§if __name__ == '__main__':
§§    if len(sys.argv) < 2:
§§        print("Specify fluffi source folder!")
§§        sys.exit()
§§    SOURCE_FOLDER = sys.argv[1]
§§    print("Start fluffi agents in " + SOURCE_FOLDER)
§§    server_class = HTTPServer
§§    try:
§§        httpd = server_class((HOST_NAME, PORT_NUMBER), InstanceHandler)
§§        print(time.asctime(), 'Server Starts - %s:%s' % (HOST_NAME, PORT_NUMBER))
§§        httpd.serve_forever()
§§    except socket.error as e:
§§        if e.errno == errno.EADDRINUSE:
§§            print("Port is already in use! There must be an already running instance of the RESTInstanceStarter!")
§§            sys.exit(1)
§§    except KeyboardInterrupt:
§§        pass
§§    httpd.server_close()
§§    print(time.asctime(), 'Server Stops - %s:%s' % (HOST_NAME, PORT_NUMBER))
