/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier, Pascal Eckmann
*/

§§#include "stdafx.h"
§§#include "http.h"
§§
§§void performHTTPAuthAndManageSession() {
§§	//TODO
§§	return;
§§}
§§
§§
§§void fixHTTPContentLength(std::vector<char>* bytes) {
§§	std::cout << "TCPFeeder: called fixHTTPContentLength" << std::endl;
§§
§§	std::string contentLength = "Content-Length: ";
§§	std::string doubleLinebreak = "\r\n\r\n";
§§	std::string singleLinebreak = "\r\n";
§§	std::string fullrequest(bytes->begin(), bytes->begin() + bytes->size());
§§
§§	std::size_t contentLengthPos = fullrequest.find(contentLength);
§§
§§	if (contentLengthPos == std::string::npos)
§§	{
§§		// not found
§§		std::cout << "TCPFeeder: fixHTTPContentLength not changing anything as the string \"Content-Length: \" was not found " << std::endl;
§§		return;
§§	}
§§
§§	std::size_t doubleLinebreakPos = fullrequest.find(doubleLinebreak, contentLengthPos);
§§
§§	if (doubleLinebreakPos == std::string::npos)
§§	{
§§		// not found
§§		std::cout << "TCPFeeder: fixHTTPContentLength not changing anything as the string \"\\r\\n\\r\\n\" was not found " << std::endl;
§§		return;
§§	}
§§
§§	int realContentLength = (int)fullrequest.length() - (int)doubleLinebreakPos - 4;
§§
§§	int specifiedContentLength = atoi(&fullrequest[contentLengthPos + 16]);
§§
§§	if (realContentLength >= specifiedContentLength) {
§§		//do not change the value if the required amount of bytes is sent
§§		std::cout << "TCPFeeder: fixHTTPContentLength not changing anything as realContentLength >= specifiedContentLength " << std::endl;
§§		return;
§§	}
§§
§§	//reduce the value, if not enough bytes are sent in order to avoid hangs (as the webserver will wait until the necessary amount of bytes are received)
§§	std::size_t singleLinebreakPos = fullrequest.find(singleLinebreak, contentLengthPos);
§§
§§	//Overwrite the original value with spaces
§§	for (size_t i = contentLengthPos + 16; i < singleLinebreakPos; i++) {
§§		(*bytes)[i] = ' ';
§§	}
§§
§§	//write the new value (it has less or equal digits as the old value, as it is smaller) WITHPUT A TRAILING NULL
§§	char buff[256];
§§	int strlength = snprintf(buff, sizeof(buff), "%d", realContentLength);
§§	memcpy_s(&(*bytes)[contentLengthPos + 16], fullrequest.length() - (contentLengthPos + 16), buff, strlength);
§§
§§	std::cout << "TCPFeeder: fixHTTPContentLength fixed the content length to :" << realContentLength << std::endl;
§§	//std::cout << std::string(bytes->begin(), bytes->begin() + bytes->size()) << std::endl;
§§
§§	return;
§§}
§§
§§void dropNoDoubleLinebreak(std::vector<char>* bytes) {
§§	std::string doubleLinebreak = "\r\n\r\n";
§§	std::string fullrequest(bytes->begin(), bytes->begin() + bytes->size());
§§
§§	std::size_t doubleLinebreakPos = fullrequest.find(doubleLinebreak);
§§
§§	if (doubleLinebreakPos == std::string::npos)
§§	{
§§		// not found
§§		std::cout << "TCPFeeder: dropNoDoubleLinebreak dropping the request as the string \"\\r\\n\\r\\n\" was not found " << std::endl;
§§
§§		bytes->clear();
§§
§§		return;
§§	}
§§
§§	//do nothing if we have a double linebreak
§§	return;
§§}
