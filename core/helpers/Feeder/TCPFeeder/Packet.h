/*
Copyright 2017-2020 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

#pragma once
typedef enum { CONTINUE, WAIT_FOR_ANY_RESPONSE, WAIT_FOR_BYTE_SEQUENCE, WAIT_N_MILLISECONDS } WHAT_TODO_AFTER_SEND;

class Packet
{
public:
	Packet(const std::vector<char> & packetBytes, WHAT_TODO_AFTER_SEND whatTodo); //CONTINUE, WAIT_FOR_ANY_RESPONSE
	Packet(const std::vector<char> & packetBytes, WHAT_TODO_AFTER_SEND whatTodo, int waitMS); //WAIT_N_MILLISECONDS
	Packet(const std::vector<char> & packetBytes, WHAT_TODO_AFTER_SEND whatTodo, std::vector<char> waitForBytes); //WAIT_FOR_BYTE_SEQUENCE
	virtual ~Packet();

	std::vector<char> m_packetBytes;
	std::vector<char> m_waitForBytes;
	WHAT_TODO_AFTER_SEND m_whatTodo;
	int m_waitMS;
};
