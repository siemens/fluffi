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

Author(s): Thomas Riedmaier, Abian Blome
*/

#pragma once
#include "FluffiServiceDescriptor.h"

class FluffiTestcaseID
{
public:
	FluffiTestcaseID(const FluffiServiceDescriptor serviceDescriptor, uint64_t localID);
	FluffiTestcaseID(const TestcaseID& testcaseID);
	virtual ~FluffiTestcaseID();

	TestcaseID getProtobuf() const;

	FluffiServiceDescriptor m_serviceDescriptor;
	uint64_t m_localID;

	inline friend std::ostream & operator<<(std::ostream & os, FluffiTestcaseID const & v) {
		os << v.m_serviceDescriptor.m_guid << "-" << std::dec << v.m_localID;

		return os;
	}

	inline friend bool operator==(const FluffiTestcaseID& lhs, const FluffiTestcaseID& rhs) {
		return (lhs.m_serviceDescriptor.m_guid == rhs.m_serviceDescriptor.m_guid) && (lhs.m_localID == rhs.m_localID);
	}

	bool operator< (const FluffiTestcaseID& rhs) const {
		if (this->m_serviceDescriptor.m_guid == rhs.m_serviceDescriptor.m_guid) {
			return this->m_localID < rhs.m_localID;
		}
		return this->m_serviceDescriptor.m_guid < rhs.m_serviceDescriptor.m_guid;
	}
};
