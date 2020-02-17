/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Roman Bendt, Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "CaRRoTMutator.h"
#include "FluffiTestcaseID.h"
#include "Util.h"
#include "TestcaseDescriptor.h"

CaRRoTMutator::CaRRoTMutator(FluffiServiceDescriptor serviceDescriptor, std::string testcaseDirectory)
	: FluffiMutator(serviceDescriptor, testcaseDirectory)
{}

CaRRoTMutator::~CaRRoTMutator()
{}

bool CaRRoTMutator::isSetupFunctionable() {
	bool setupFunctionable = true;
#if defined(_WIN32) || defined(_WIN64)
	setupFunctionable &= std::experimental::filesystem::exists(".\\CaRRoT.exe");
#else
	setupFunctionable &= std::experimental::filesystem::exists("./CaRRoT");
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

std::deque<TestcaseDescriptor> CaRRoTMutator::batchMutate(unsigned int numToGenerate, const FluffiTestcaseID parentID, const std::string parentPathAndFilename)
{
	std::deque<TestcaseDescriptor> generatedMutations{};

	std::error_code ec;
	if (!std::experimental::filesystem::exists(parentPathAndFilename, ec))
	{
		LOG(ERROR) << "Parent does not exist (" << ec.message() << ")";
		throw std::runtime_error("Parent does not exist");
	}

	LOG(DEBUG) << "Executing CaRRoT";
	std::string cmdline = "CaRRoT -d -n " + std::to_string(numToGenerate) + " -o " + m_testcaseDir + Util::pathSeperator + "CaRRoT_ " + parentPathAndFilename;
	if (!executeProcessAndWaitForCompletion(cmdline, 5 * 60 * 1000))
	{
		if (!std::experimental::filesystem::exists(parentPathAndFilename))
		{
			LOG(ERROR) << "Could not generate testcases with CaRRoT (" << parentPathAndFilename << " does not exist)";
		}
		else {
			LOG(ERROR) << "Could not generate testcases with CaRRoT (" << parentPathAndFilename << " exists)";
		}

		throw std::runtime_error("Could not generate testcase with CaRRoT");
	}

	for (unsigned int i = 1; i <= numToGenerate; ++i)
	{
		std::string currentCaRRoTFile = m_testcaseDir + Util::pathSeperator + "CaRRoT_" + std::to_string(i);

		FluffiTestcaseID testcaseID = genNewLocalFluffiTestcaseID();
		std::string pathAndFilename = Util::generateTestcasePathAndFilename(testcaseID, m_testcaseDir);
		if (!std::experimental::filesystem::exists(currentCaRRoTFile)) {
			LOG(WARNING) << "CaRRoT failed to generate the expected file " << currentCaRRoTFile << ". We will break here.";
			break;
		}

		// As AVs love to lock the file, attempt to rename them several times
		if (Util::attemptRenameFile(currentCaRRoTFile, pathAndFilename))
		{
			TestcaseDescriptor childTestcaseDescriptor{ testcaseID, parentID, pathAndFilename, false };
			generatedMutations.push_back(childTestcaseDescriptor);
		}
		else
		{
			LOG(ERROR) << "Error renaming CaRRoT file: " << currentCaRRoTFile;
		}
	}

	return generatedMutations;
}