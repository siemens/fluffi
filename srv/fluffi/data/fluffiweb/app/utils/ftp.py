§§# Copyright 2017-2019 Siemens AG
§§# 
§§# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§# 
§§# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§# 
§§# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§# 
§§# Author(s): Michael Kraus, Junes Najah, Fabian Russwurm, Thomas Riedmaier
§§
§§from ftplib import FTP
§§
§§
§§class FTPConnector:
§§
§§    def __init__(self, ftpURL):
§§        self.ftpURL = ftpURL
§§        self.ftpClient = FTP()
§§
§§    def getListOfFilesOnFTPServer(self, path):
§§        self.ftpClient.connect(self.ftpURL)
§§        self.ftpClient.login()
§§        self.ftpClient.cwd(path)
§§        ls = []
§§        ls = self.ftpClient.nlst()
§§        tupelsOfLS = zip(ls, ls)
§§        self.ftpClient.quit()
§§
§§        return tupelsOfLS
§§
§§    def getListOfArchitecturesOnFTPServer(self, path, group):
§§        self.ftpClient.connect(self.ftpURL)
§§        self.ftpClient.login()
§§        self.ftpClient.cwd(path)
§§        ls = []
§§        ls = self.ftpClient.nlst()
§§
§§        for i, w in enumerate(ls):
§§            ls[i] = group + "-" + w
§§
§§        tupelsOfLS = zip(ls, ls)
§§        self.ftpClient.quit()
§§
§§        return tupelsOfLS
§§
§§    def saveTargetFileOnFTPServer(self, targetFileData, name):
§§        # ftplib storbinary is programmed to read file from disk before sending to ftp server
§§        # solution is to extend the lib and rewrite storbinary...
§§        # https://stackoverflow.com/questions/2671118/can-i-upload-an-object-in-memory-to-ftp-using-python 
§§        # workaround: write file to disk
§§        # .....
§§        path = 'tmp.zip'
§§        target = open(path, 'wb')
§§        target.write(targetFileData)
§§        target.close()
§§        self.ftpClient.connect(self.ftpURL)
§§        self.ftpClient.login()
§§        f = open('tmp.zip', 'rb')
§§        self.ftpClient.storbinary("STOR /SUT/" + name.split('.', 1)[0] + ".zip", f)
§§        self.ftpClient.quit()
§§
§§        return True
§§
§§    def saveArchivedProjectOnFTPServer(self, fileName):
§§        self.ftpClient.connect(self.ftpURL)
§§        self.ftpClient.login()
§§        myFile = open(fileName, 'rb')
§§
§§        if myFile:
§§            self.ftpClient.storbinary("STOR /archive/" + fileName, myFile)
§§        else:
§§            print("-------------------------------------------")
§§            print("Error: File not found")
§§            print("-------------------------------------------")
§§            self.ftpClient.quit()
§§            return False
§§
§§        self.ftpClient.quit()
§§
§§        return True
