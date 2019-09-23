/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier
*/

§§#include "stdafx.h"
§§#include "WorkerWeightCalculator.h"
#include "LMDatabaseManager.h"
§§
§§WorkerWeightCalculator::WorkerWeightCalculator(LMDatabaseManager* dbManager, std::string location)
	: m_dbManager(dbManager),
§§	m_location(location)
§§{}
§§
§§WorkerWeightCalculator::~WorkerWeightCalculator()
§§{}
§§
§§void WorkerWeightCalculator::updateStatusInformation()
§§{
	std::vector<StatusOfInstance> statusVector = m_dbManager->getStatusOfManagedInstances(m_location);
§§
§§	m_generatorStatus.clear();
§§	m_evaluatorStatus.clear();
§§
	if (statusVector.empty())
	{
		LOG(INFO) << "Retrieval of status information from DB failed! It looks like there are no agents in my location! ";
		return;
	}

§§	while (!statusVector.empty())
§§	{
§§		StatusOfInstance status = statusVector.back();
§§		statusVector.pop_back();
§§
§§		switch (status.type)
§§		{
§§		case(AgentType::TestcaseGenerator):
§§			status.weight = calculateGeneratorWeight(status);
§§			m_generatorStatus.push_back(status);
§§			break;
§§		case(AgentType::TestcaseEvaluator):
§§			status.weight = calculateEvaluatorWeight(status);
§§			m_evaluatorStatus.push_back(status);
§§			break;
		default:
			//we are only interested in TGs and TEs
			break;
§§		}
§§	}
§§}
§§
§§std::vector<StatusOfInstance> WorkerWeightCalculator::getGeneratorStatus()
§§{
§§	return m_generatorStatus;
§§}
§§
§§std::vector<StatusOfInstance> WorkerWeightCalculator::getEvaluatorStatus()
§§{
§§	return m_evaluatorStatus;
§§}
§§
uint32_t WorkerWeightCalculator::calculateGeneratorWeight(StatusOfInstance status)
§§{
§§	// Current heuristic: The more cases are in the queue, the better.
	uint32_t score = 100;
§§
§§	std::map<std::string, std::string>::iterator it = status.status.find("TestCasesQueueSize");
§§	if (it != status.status.end())
§§	{
		try {
			score += std::stoi(it->second);
		}
		catch (...) {
			LOG(WARNING) << "calculateGeneratorWeight: std::stoi failed";
		}
§§	}
§§	else
§§	{
		LOG(ERROR) << "Status of TG does not contain a TestCasesQueueSize: Using default weight of 100";
§§	}
§§	return score;
§§}
§§
uint32_t WorkerWeightCalculator::calculateEvaluatorWeight(StatusOfInstance status)
§§{
§§	// Current heuristic: The fewer cases are in the queue, the better
	uint32_t score = 100;
§§
§§	std::map<std::string, std::string>::iterator it = status.status.find("TestEvaluationsQueueSize");
§§	if (it != status.status.end())
§§	{
		try {
			score = std::max(100, 1000 - std::stoi(it->second));
		}
		catch (...) {
			LOG(WARNING) << "calculateEvaluatorWeight: std::stoi failed";
		}
§§	}
§§	else
§§	{
		LOG(ERROR) << "Status of TG does not contain a TestCasesQueueSize: Using default weight of 100";
§§	}
§§	return score;
}
