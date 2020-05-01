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
#include "FluffiTestcaseID.h"

FluffiTestcaseID::FluffiTestcaseID(const FluffiServiceDescriptor serviceDescriptor, uint64_t localID)
	: m_serviceDescriptor(serviceDescriptor),
	m_localID(localID)
{}

FluffiTestcaseID::FluffiTestcaseID(const TestcaseID& testcaseID)
	: m_serviceDescriptor(testcaseID.servicedescriptor()),
	m_localID(testcaseID.localid())
{
	if (!testcaseID.has_servicedescriptor())
	{
		LOG(ERROR) << "Attempting to abstract an invalid testcaseID";
	}
}

FluffiTestcaseID::~FluffiTestcaseID()
{}

TestcaseID FluffiTestcaseID::getProtobuf() const
{
	TestcaseID protoTestcaseID{};
	ServiceDescriptor* protoServiceDescriptor = new ServiceDescriptor();
	protoServiceDescriptor->CopyFrom(m_serviceDescriptor.getProtobuf());
	protoTestcaseID.set_allocated_servicedescriptor(protoServiceDescriptor);
	protoTestcaseID.set_localid(m_localID);

	return protoTestcaseID;
}
