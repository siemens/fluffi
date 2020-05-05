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

Author(s): Abian Blome, Thomas Riedmaier
*/

#include "stdafx.h"
#include "FluffiServiceDescriptor.h"

FluffiServiceDescriptor::FluffiServiceDescriptor(const std::string serviceHostAndPort, const std::string guid)
	: m_serviceHostAndPort(serviceHostAndPort),
	m_guid(guid)
{}

FluffiServiceDescriptor::FluffiServiceDescriptor(const ServiceDescriptor & serviceDescriptor)
{
	m_serviceHostAndPort = serviceDescriptor.servicehostandport();
	m_guid = serviceDescriptor.guid();
}

FluffiServiceDescriptor::FluffiServiceDescriptor()
	: m_serviceHostAndPort(""),
	m_guid(""),
	m_isNull(true)
{}

FluffiServiceDescriptor::~FluffiServiceDescriptor()
{}

FluffiServiceDescriptor FluffiServiceDescriptor::getNullObject()
{
	return FluffiServiceDescriptor();
}

ServiceDescriptor FluffiServiceDescriptor::getProtobuf() const
{
	ServiceDescriptor serviceDescriptor{};

	serviceDescriptor.set_guid(m_guid);
	serviceDescriptor.set_servicehostandport(m_serviceHostAndPort);

	return serviceDescriptor;
}

bool FluffiServiceDescriptor::isNullObject()
{
	return m_isNull;
}
