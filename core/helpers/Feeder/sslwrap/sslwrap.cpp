/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Michael Kraus, Thomas Riedmaier, Pascal Eckmann
*/


#include "stdafx.h"
#include "sslwrap.h"

int sendByteBufOnce(char* dstIP, int port, char* msg, int msgSize, int clientCertSize, const unsigned char * clientCert, int clientPrivateKeySize, const unsigned char * clientPrivateKey)
{
	SSL_CTX* ctx = nullptr;
	SOCKETTYPE server = INVALID_SOCKET;
	SSL* ssl = nullptr;
	BIO* buf_io = nullptr;

	/* ---------------------------------------------------------- *
	* initialize SSL library and register algorithms             *
	* ---------------------------------------------------------- */
	if (SSL_library_init() < 0) {
		std::cout << ">sslwrap.dll< - Could not initialize the OpenSSL library!" << std::endl;
		return -1;
	}

	ctx = createSSLContext();
	if (ctx == nullptr) {
		std::cout << ">sslwrap.dll< - Error creating SSL context!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	server = createSocket(dstIP, port);
	if (server == INVALID_SOCKET) {
		std::cout << ">sslwrap.dll< - Error creating SSL socket!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	ssl = establishConnection(dstIP, port, server, ctx, clientCertSize, clientCert, clientPrivateKeySize, clientPrivateKey);
	if (ssl == nullptr) {
		std::cout << ">sslwrap.dll< - SSL connection Error!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	buf_io = setUpBufIO(ssl);
	if (buf_io == nullptr) {
		std::cout << ">sslwrap.dll< - Error setting up BIO!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	// Request
	int n = BIO_puts(buf_io, msg);
	if (n < 0) {
		std::cout << ">sslwrap.dll< - Error writing bytes OR no data available, try again!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	int success = BIO_flush(buf_io);
	if (success < 1) {
		std::cout << ">sslwrap.dll< - Error flushing Message!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	int free = freeStructures(ssl, server, ctx, buf_io);
	if (free < 0) {
		std::cout << ">sslwrap.dll< - Error freeing structures!" << std::endl;
		return -1;
	}

	return 0;
}

/* ---------------------------------------------------------- *
* Saves response in bytebuffer
* Returns 0 if succesful, -1 if openssl lib error, -2 if resonse too large for buffer...
* ---------------------------------------------------------- */
int sendByteBufWithResponse(char* dstIP, int dstPort, char* msg, int msgSize, char* response, int responseSize, int clientCertSize, const unsigned char * clientCert, int clientPrivateKeySize, const unsigned char * clientPrivateKey)
{
	SSL_CTX* ctx = nullptr;
	SOCKETTYPE server = INVALID_SOCKET;
	SSL* ssl = nullptr;
	BIO* buf_io = nullptr;

	/* ---------------------------------------------------------- *
	* initialize SSL library and register algorithms             *
	* ---------------------------------------------------------- */
	if (SSL_library_init() < 0) {
		std::cout << ">sslwrap.dll< - Could not initialize the OpenSSL library!" << std::endl;
		return -1;
	}

	ctx = createSSLContext();
	if (ctx == nullptr) {
		std::cout << ">sslwrap.dll< - Error creating SSL context!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	server = createSocket(dstIP, dstPort);
	if (server == INVALID_SOCKET) {
		std::cout << ">sslwrap.dll< - Error creating SSL socket!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	ssl = establishConnection(dstIP, dstPort, server, ctx, clientCertSize, clientCert, clientPrivateKeySize, clientPrivateKey);
	if (ssl == nullptr) {
		std::cout << ">sslwrap.dll< - SSL connection Error!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	buf_io = setUpBufIO(ssl);
	if (buf_io == nullptr) {
		std::cout << ">sslwrap.dll< - Error setting up BIO!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	// Request
	int n = BIO_puts(buf_io, msg);
	if (n < 0) {
		std::cout << ">sslwrap.dll< - Error writing bytes OR no data available, try again!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	int success = BIO_flush(buf_io);
	if (success < 1) {
		std::cout << ">sslwrap.dll< - Error flushing Message!" << std::endl;
		freeStructures(ssl, server, ctx, buf_io);
		return -1;
	}

	int lenRead = 0;
	int currentLen = 0;
	int retCode = 0;
	memset(response, 0, responseSize);
	do
	{
		if (!(lenRead + 1 >= responseSize))
		{
			currentLen = BIO_gets(buf_io, response + lenRead, responseSize - lenRead);
			if (currentLen < 1)
			{
				if (BIO_should_retry(buf_io))
				{
					continue;
				}
			}
			lenRead += currentLen;
		}
		else
		{
			// Response does not fit into buffer.
			// Does not interrupt the processing currently because in most cases only the headers are important.
			// Feeder will interrupt if no cookie etc was found.
			std::cout << ">sslwrap.dll< - Warning: Server response larger than the buffer. Response cropped! Increase Buffer Size!" << std::endl;
			retCode = -2;
			break;
		}
	} while ((currentLen > 0));

	msgSize = lenRead;

	int free = freeStructures(ssl, server, ctx, buf_io);
	if (free < 0) {
		std::cout << ">sslwrap.dll< - Error freeing structures!" << std::endl;
		return -1;
	}

	return retCode;
}

/* ---------------------------------------------------------- *
* Set up buffered IO
* ---------------------------------------------------------- */
BIO* setUpBufIO(SSL* ssl)
{
	BIO* buf_io;
	BIO* ssl_bio;
	buf_io = BIO_new(BIO_f_buffer());  /* create a buffer BIO */
	if (buf_io == nullptr) {
		std::cout << ">sslwrap.dll< - Error creating new BIO!" << std::endl;
		return nullptr;
	}
	ssl_bio = BIO_new(BIO_f_ssl());           /* create an ssl BIO */
	if (ssl_bio == nullptr) {
		std::cout << ">sslwrap.dll< - Error creating new SSL BIO!" << std::endl;
		return nullptr;
	}
	BIO_set_ssl(ssl_bio, ssl, BIO_CLOSE);       /* assign the ssl BIO to SSL */
	BIO_push(buf_io, ssl_bio);          /* add ssl_bio to buf_io */

	return buf_io;
}

/* ---------------------------------------------------------- *
* Make the underlying TCP socket connection                  *
* ---------------------------------------------------------- */
SOCKETTYPE createSocket(char* dstIP, int port)
{
	SOCKETTYPE server = create_socket(dstIP, port);
	if (server != INVALID_SOCKET) {
		std::cout << ">sslwrap.dll< - Successfully made the TCP connection to: " << dstIP << ":" << port << "." << std::endl;
	}

	return server;
}

/* ---------------------------------------------------------- *
* Try to create a new SSL context                            *
* ---------------------------------------------------------- */
SSL_CTX* createSSLContext()
{
	/* ---------------------------------------------------------- *
	* Set TLSv1_2 client method      *
	* ---------------------------------------------------------- */
	const SSL_METHOD* method = TLSv1_2_client_method();

	/* ---------------------------------------------------------- *
	* Try to create a new SSL context                            *
	* ---------------------------------------------------------- */
	SSL_CTX* ctx;
	if ((ctx = SSL_CTX_new(method)) == nullptr) {
		std::cout << ">sslwrap.dll< - Unable to create a new SSL context structure." << std::endl;
		return nullptr;
	}

	return ctx;
}

int freeStructures(SSL* ssl, SOCKETTYPE server, SSL_CTX* ctx, BIO* buf_io)
{
	int re = 0;
	/* ---------------------------------------------------------- *
	* Free the structures we don't need anymore                  *
	* -----------------------------------------------------------*/

	if (buf_io != nullptr) {
		BIO_free_all(buf_io);
		ssl = nullptr;
	}

	if (ssl != nullptr) {
		SSL_free(ssl);
	}
	if (server != INVALID_SOCKET) {
		if (closesocket(server) != 0) {
			std::cout << ">sslwrap.dll< - Error closing Socket" << std::endl;
			re = -1;
		}
	}

	if (ctx != nullptr) {
		SSL_CTX_free(ctx);
	}

#if defined(_WIN32) || defined(_WIN64)
	if (WSACleanup() != 0) {
		std::cout << ">sslwrap.dll< - Error in WSACleanup(Windows)" << std::endl;
		re = -1;
	};
#endif

	return re;
}

/* ---------------------------------------------------------- *
* create_socket() creates the socket & TCP-connect to server *
* ---------------------------------------------------------- */
SOCKETTYPE create_socket(const char ip[], const int port) {
#if defined(_WIN32) || defined(_WIN64)
	//----------------------
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		std::cout << ">sslwrap.dll< - WSAStartup function failed with error: " << iResult << std::endl;
		return INVALID_SOCKET;
	}
#endif

	/* ---------------------------------------------------------- *
	* create the basic TCP socket                                *
	* ---------------------------------------------------------- */
	SOCKETTYPE sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == INVALID_SOCKET) {
#if defined(_WIN32) || defined(_WIN64)
		std::cout << ">sslwrap.dll< - Socket function failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
#else
		std::cout << ">sslwrap.dll< - Socket function failed with error: " << errno << std::endl;
#endif
		return INVALID_SOCKET;
	}

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = inet_addr(ip);

	/* ---------------------------------------------------------- *
	* Try to connect to the server                    *
	* ---------------------------------------------------------- */
	if (connect(sockfd, (struct sockaddr*) &dest_addr, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		char* tmp_ptr = inet_ntoa(dest_addr.sin_addr);
		std::cout << ">sslwrap.dll< - Error: Cannot connect to ip " << ip << " [" << tmp_ptr << "] on port " << port << "." << std::endl;
		if (closesocket(sockfd) == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
			std::cout << ">sslwrap.dll< - closesocket function failed with error: " << WSAGetLastError() << std::endl;
			WSACleanup();
#else
			std::cout << ">sslwrap.dll< - closesocket function failed with error: " << errno << std::endl;
#endif

			return INVALID_SOCKET;
		}
	}

	return sockfd;
}

/* ---------------------------------------------------------- *
* establishConnection() returns the connection to the server *
* ---------------------------------------------------------- */
SSL* establishConnection(const char* dstIP, const int dstPort, SOCKETTYPE server, SSL_CTX* ctx, int clientCertSize, const unsigned char * clientCert, int clientPrivateKeySize, const unsigned char * clientPrivateKey) {
	/* ---------------------------------------------------------- *
	* Set the client's certificate and private key                *
	* ---------------------------------------------------------- */
	if (clientCert != nullptr) {
		if (SSL_CTX_use_certificate_ASN1(ctx, clientCertSize, clientCert) != 1) {
			std::cout << ">sslwrap.dll< - Unable to set the client's certificate." << std::endl;
			return nullptr;
		}
	}
	if (clientPrivateKey != nullptr) {
		if (SSL_CTX_use_PrivateKey_ASN1(EVP_PKEY_RSA, ctx, clientPrivateKey, clientPrivateKeySize) != 1) {
			std::cout << ">sslwrap.dll< - Unable to set the client's private key." << std::endl;
			return nullptr;
		}
	}

	/* ---------------------------------------------------------- *
	* Create new SSL connection state object                     *
	* ---------------------------------------------------------- */
	SSL* ssl = SSL_new(ctx);

	/*---------------------------------------------------------- *
	* Attach the SSL session to the socket descriptor            *
	* ---------------------------------------------------------- */
	SSL_set_fd(ssl, (int)server);

	/* ---------------------------------------------------------- *
	* Try to SSL-connect here, returns 1 for success             *
	* ---------------------------------------------------------- */
	if (SSL_connect(ssl) != 1) {
		std::cout << ">sslwrap.dll< - Error: Could not build a SSL session to: " << dstIP << ":" << dstPort << "." << std::endl;
		SSL_free(ssl);
		return nullptr;
	}
	else {
		std::cout << ">sslwrap.dll< - Successfully established SSL/TLS session to: " << dstIP << ":" << dstPort << "." << std::endl;
	}

	return ssl;
}