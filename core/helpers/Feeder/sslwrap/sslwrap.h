/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Michael Kraus
*/

#pragma once

#define NOCRYPT

//public
int __cdecl sendByteBufOnce(char* dstIP, int port, char* msg, int msgSize, int clientCertSize, const unsigned char * clientCert, int clientPrivateKeySize, const unsigned char * clientPrivateKey);
int __cdecl sendByteBufWithResponse(char* dstIP, int port, char* msg, int msgSize, char* response, int responseSize, int clientCertSize, const unsigned char * clientCert, int clientPrivateKeySize, const unsigned char * clientPrivateKey);

//private
SSL* establishConnection(const char* dstIP, const int dstPort, SOCKETTYPE server, SSL_CTX* ctx, int clientCertSize, const unsigned char * clientCert, int clientPrivateKeySize, const unsigned char * clientPrivateKey);
SOCKETTYPE createSocket(char* dstIP, int port);
SOCKETTYPE create_socket(const char ip[], const int port);
SSL_CTX* createSSLContext();
int freeStructures(SSL* ssl, SOCKETTYPE server, SSL_CTX* ctx, BIO* buf_io);
BIO* setUpBufIO(SSL* ssl);
