/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Roman Bendt, Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "OedipusMutator.h"
#include "FluffiTestcaseID.h"
#include "Util.h"
#include "TestcaseDescriptor.h"
#include "CommInt.h"
#include "TGWorkerThreadState.h"
#include "FluffiLMConfiguration.h"

OedipusMutator::OedipusMutator(FluffiServiceDescriptor serviceDescriptor, std::string testcaseDirectory, CommInt* commInt, TGWorkerThreadState* workerThreadState)
	: FluffiMutator(serviceDescriptor, testcaseDirectory),
	m_commInt(commInt),
	m_workerThreadState(workerThreadState)
{}

OedipusMutator::~OedipusMutator()
{}

bool OedipusMutator::isSetupFunctionable() {
	bool setupFunctionable = true;
#if defined(_WIN32) || defined(_WIN64)
	setupFunctionable &= std::experimental::filesystem::exists(".\\Oedipus.exe");
#else
	setupFunctionable &= std::experimental::filesystem::exists("./Oedipus");
#endif
	if (!setupFunctionable) {
		LOG(WARNING) << "generator executable not found";
		return setupFunctionable;
	}

	//2) check if the testcase directory  exists
	setupFunctionable &= std::experimental::filesystem::exists(m_testcaseDir);
	if (!setupFunctionable) {
		LOG(WARNING) << "testcasedir not found";
		return setupFunctionable;
	}

	return setupFunctionable;
}

std::deque<TestcaseDescriptor> OedipusMutator::batchMutate(unsigned int numToGenerate, const FluffiTestcaseID parentID, const std::string parentPathAndFilename)
{
	if (this->encodedDBcredentials.empty()) {
		// get db credentials by sending a getlmconfigurationrequest
		// these two need to go out of scope for protobuf to clean up all the raw pointers below.
		FLUFFIMessage req = FLUFFIMessage();
		FLUFFIMessage resp = FLUFFIMessage();

		// ask lm for creds
		GetLMConfigurationRequest* glmcr = new GetLMConfigurationRequest();
		req.set_allocated_getlmconfigurationrequest(glmcr);
		bool respReceived = m_commInt->sendReqAndRecvResp(&req, &resp, m_workerThreadState, m_commInt->getMyLMServiceDescriptor().m_serviceHostAndPort, CommInt::timeoutNormalMessage);

		std::stringstream ss;
		if (respReceived) {
			LOG(DEBUG) << "OedipusMutator::batchMutate received respose for GetLMConfigurationRequest: " << (resp.has_getlmconfigurationresponse() ? "true" : "false");
			const FluffiLMConfiguration lmConfig = FluffiLMConfiguration(resp.getlmconfigurationresponse().lmconfiguration());
			// format: "fluffi_gm:fluffi_gm@tcp(db.fluffi:3306)/fluffi_miniweb"
			ss << lmConfig.m_dbUser << ":" << lmConfig.m_dbPassword << "@tcp(";
			ss << lmConfig.m_dbHost << ":3306)/" << lmConfig.m_dbName;
		}
		else {
			LOG(ERROR) << "Oedipus could not get database credentials";
			throw std::runtime_error("cannot get db creds");
		}

		// encode creds for easy passing
		this->encodedDBcredentials = string2hex(ss.str());
		LOG(DEBUG) << "GetLMConfigurationResponse successfully received: " << ss.str();
	}

	std::error_code ec;
	if (!std::experimental::filesystem::exists(parentPathAndFilename, ec))
	{
		LOG(ERROR) << "Parent does not exist (" << ec.message() << ")";
		throw std::runtime_error("Parent does not exist");
	}

	LOG(DEBUG) << "Executing Oedipus";
	std::string cmdline = "Oedipus -r -m -d " + this->encodedDBcredentials + " -n " + std::to_string(numToGenerate) + " -o " + m_testcaseDir + Util::pathSeperator + "Oedipus_ " + parentPathAndFilename;

	LOG(DEBUG) << "CMD: " << cmdline;

	if (!executeProcessAndWaitForCompletion(cmdline, 60 * 1000))
	{
		if (!std::experimental::filesystem::exists(parentPathAndFilename))
		{
			LOG(ERROR) << "Could not generate testcases with Oedipus (" << parentPathAndFilename << " does not exist)";
		}
		else {
			LOG(ERROR) << "Could not generate testcases with Oedipus (" << parentPathAndFilename << " exists)";
		}

		throw std::runtime_error("Could not generate testcase with Oedipus");
	}

	std::deque<TestcaseDescriptor> generatedMutations{};
	for (unsigned int i = 1; i <= numToGenerate; ++i)
	{
		std::string currentOedipusFile = m_testcaseDir + Util::pathSeperator + "Oedipus_" + std::to_string(i);

		FluffiTestcaseID testcaseID = genNewLocalFluffiTestcaseID();
		std::string pathAndFilename = Util::generateTestcasePathAndFilename(testcaseID, m_testcaseDir);
		if (!std::experimental::filesystem::exists(currentOedipusFile)) {
			LOG(WARNING) << "Oedipus failed to generate the expected file " << currentOedipusFile << " which we wanted to rename to " << pathAndFilename << ". We will break here.";
			break;
		}

		// As AVs love to lock the file, attempt to rename them several times
		if (Util::attemptRenameFile(currentOedipusFile, pathAndFilename))
		{
			TestcaseDescriptor childTestcaseDescriptor{ testcaseID, parentID, pathAndFilename,false };
			generatedMutations.push_back(childTestcaseDescriptor);
		}
		else
		{
			LOG(ERROR) << "Error renaming Oedipus file: " << currentOedipusFile;
		}
	}

	return generatedMutations;
}

std::string OedipusMutator::string2hex(const std::string& s) {
	static const char* const LUT = "0123456789abcdef";
	std::string enc;
	enc.reserve(s.length() * 2);

	for (size_t i = 0; i < s.length(); ++i) {
		const unsigned char c = s[i];
		enc.push_back(LUT[c >> 4]);
		enc.push_back(LUT[c & 15]);
	}
	return enc;
}
