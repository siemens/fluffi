/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier
*/

§§#include "stdafx.h"
§§#include "FluffiServiceAndWeight.h"
§§
FluffiServiceAndWeight::FluffiServiceAndWeight(const FluffiServiceDescriptor serviceDescriptor, uint32_t weight) :
	m_serviceDescriptor(serviceDescriptor), m_weight(weight)
§§{
§§}
§§
FluffiServiceAndWeight::FluffiServiceAndWeight(const ServiceAndWeigth& serviceAndWeight) :
	m_serviceDescriptor(FluffiServiceDescriptor(serviceAndWeight.servicedescriptor())), m_weight(serviceAndWeight.weight())
§§{
§§}
§§
§§FluffiServiceAndWeight::~FluffiServiceAndWeight()
§§{
§§}
§§
§§ServiceAndWeigth FluffiServiceAndWeight::getProtobuf() const
§§{
§§	ServiceAndWeigth serviceAndWeight = ServiceAndWeigth();
§§
§§	ServiceDescriptor* protoServiceDescriptor = new ServiceDescriptor();
	protoServiceDescriptor->CopyFrom(m_serviceDescriptor.getProtobuf());
§§
§§	serviceAndWeight.set_allocated_servicedescriptor(protoServiceDescriptor);
	serviceAndWeight.set_weight(m_weight);

§§	return serviceAndWeight;
}
