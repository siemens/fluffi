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

Author(s): Thomas Riedmaier, Michael Kraus, Abian Blome
*/

#include "stdafx.h"
#include "CommPartnerManager.h"

CommPartnerManager::CommPartnerManager() :
	m_weigthSum(0),
	m_mutex_(),
	m_commDescriptorSet(),
	m_gen(m_rd())
{
}

CommPartnerManager::~CommPartnerManager()
{}

// Calculates the next communication partner from the set with cumulative probabilities
std::string CommPartnerManager::getNextPartner()
{
	std::unique_lock<std::mutex> mlock(m_mutex_);

	if (m_weigthSum == 0) {
		LOG(INFO) << "Next CommPartner calculation for TE or TG failed! Sum of weigths null! CommPartner Set may be empty!";
		return "";
	}

	std::uniform_int_distribution<uint64_t> dist(1, m_weigthSum);
	uint64_t randNum = dist(m_gen);

	uint64_t cummulativeProbability = 0;

	for (auto && it : m_commDescriptorSet)
	{
		uint64_t remainingSize = (std::numeric_limits<uint64_t>::max)() - cummulativeProbability;

		//overflow prevention
		if (remainingSize <= it.m_weight) {
			LOG(ERROR) << "CommPartner calculation failed! Sum of weights was overflowed! Reached uint32.MAX clients! -> Abian der Weise sprach: Eine elegante Lösung mit Normierung auf Double Werte ... die einzig wahre Lösung ist! <-";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy;
		}

		cummulativeProbability += it.m_weight;
		if (cummulativeProbability >= randNum) {
			return it.m_serviceDescriptor.m_serviceHostAndPort;
		}
	}

	LOG(INFO) << "Next CommPartner calculation for TE or TG failed! It looks like we don't have CommPartners yet!";
	return "";
}

// Saves the new received communication partners in the set, meanwhile calculates the total weight
int CommPartnerManager::updateCommPartners(google::protobuf::RepeatedPtrField<ServiceAndWeigth>* partners)
{
	std::unique_lock<std::mutex> mlock(m_mutex_);
	m_commDescriptorSet.clear();
	m_weigthSum = 0;

	uint32_t numberOfInsertedPartners = 0;
	for (auto&& it : *partners)
	{
		m_commDescriptorSet.push_back((FluffiServiceAndWeight(it)));
		m_weigthSum += it.weight();
		numberOfInsertedPartners++;
	}

	return numberOfInsertedPartners;
}
