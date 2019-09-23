/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier
*/

#include "stdafx.h"
#include "ExternalMutator.h"
#include "FluffiTestcaseID.h"
#include "Util.h"
#include "TestcaseDescriptor.h"
#include "TGWorkerThreadState.h"
#include "CommInt.h"
#include "FluffiLMConfiguration.h"

ExternalMutator::ExternalMutator(FluffiServiceDescriptor serviceDescriptor, std::string testcaseDirectory, std::string extDirectory, CommInt* commInt, TGWorkerThreadState* workerThreadState)
	: FluffiMutator(serviceDescriptor, testcaseDirectory), m_extTestcaseDir(extDirectory), m_commInt(commInt), m_workerThreadState(workerThreadState)
{
}

ExternalMutator::~ExternalMutator()
{
}

bool ExternalMutator::isSetupFunctionable() {
	//1) check if the testcase directory  exists
	if (!std::experimental::filesystem::exists(m_testcaseDir)) {
		return false;
	}

	//2) check if the external testcase directory  exists
	if (!std::experimental::filesystem::exists(m_extTestcaseDir)) {
		return false;
	}

	//3) Give out the name of our testcase in the testcase directory
	// ask lm for creds
	if (m_commInt != nullptr && m_workerThreadState != nullptr)
	{
		FLUFFIMessage req = FLUFFIMessage();
		FLUFFIMessage resp = FLUFFIMessage();
		GetLMConfigurationRequest* glmcr = new GetLMConfigurationRequest();
		req.set_allocated_getlmconfigurationrequest(glmcr);
		bool respReceived = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, CommInt::timeoutNormalMessage);
		if (!respReceived) {
			LOG(ERROR) << "External mutator failed to get fuzzjob details";
			return false;
		}

		const FluffiLMConfiguration lmConfig = FluffiLMConfiguration(resp.getlmconfigurationresponse().lmconfiguration());

		std::experimental::filesystem::path outFilePath = std::experimental::filesystem::path(m_extTestcaseDir);
		outFilePath.append("fuzzjob.name");

		std::ofstream nameFile(outFilePath);
		if (!nameFile)
		{
			LOG(ERROR) << "Could not open: " << outFilePath;
			return false;
		}

		nameFile << lmConfig.m_fuzzJobName << std::endl;;
		nameFile.close();
	}

	return true;
}

std::deque<TestcaseDescriptor> ExternalMutator::batchMutate(unsigned int numToGenerate, const FluffiTestcaseID parentID, const std::string parentPathAndFilename)
{
	(void)(parentID); //avoid unused parameter warning
	(void)(parentPathAndFilename); //avoid unused parameter warning

	std::deque<TestcaseDescriptor> generatedMutations{};

	for (auto& file : std::experimental::filesystem::recursive_directory_iterator(m_extTestcaseDir)) {
		if (generatedMutations.size() == numToGenerate) {
			return generatedMutations;
		}
		if (std::experimental::filesystem::is_regular_file(file.path()) && file.path().filename().generic_string() != "fuzzjob.name") {
			std::string generatorGUID = file.path().parent_path().filename().generic_string();
			auto parentGUIDAndLocalID = Util::splitString(file.path().filename().generic_string(), "_");
			if (parentGUIDAndLocalID.size() != 3) {
				LOG(ERROR) << "File not in expected format (parentGUID_parentLocalID_localID): " << file.path().filename().generic_string();
				break;
			}
			std::string parentGUID = parentGUIDAndLocalID[0];
			uint64_t parentLocalID, localID;
			try {
				parentLocalID = std::stoull(parentGUIDAndLocalID[1]);
				localID = std::stoull(parentGUIDAndLocalID[2]);
			}
			catch (const std::exception& e) {
				LOG(ERROR) << "Error extracting localIDs from file: " << e.what();
				break;
			}

			FluffiServiceDescriptor parentServiceDescriptor{ "", parentGUID };
			FluffiTestcaseID parentTestcaseID{ parentServiceDescriptor, parentLocalID };

			FluffiServiceDescriptor extServiceDescriptor{ "", generatorGUID };
			FluffiTestcaseID testcaseID{ extServiceDescriptor, localID };

			std::string pathAndFilename = Util::generateTestcasePathAndFilename(testcaseID, m_testcaseDir);

			if (Util::attemptRenameFile(file.path().generic_string(), pathAndFilename))
			{
				TestcaseDescriptor childTestcaseDescriptor{ testcaseID, parentTestcaseID, pathAndFilename, false };
				generatedMutations.push_back(childTestcaseDescriptor);
			}
			else
			{
				LOG(ERROR) << "Error renaming generated file: " << file.path().generic_string();
				break;
			}
		}
	}

	return generatedMutations;
}
