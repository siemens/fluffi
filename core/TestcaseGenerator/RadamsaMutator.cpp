/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Roman Bendt
*/

§§#include "stdafx.h"
§§#include "RadamsaMutator.h"
#include "FluffiTestcaseID.h"
#include "Util.h"
#include "TestcaseDescriptor.h"
§§
§§RadamsaMutator::RadamsaMutator(FluffiServiceDescriptor serviceDescriptor, std::string testcaseDirectory)
	: FluffiMutator(serviceDescriptor, testcaseDirectory)
§§{}
§§
§§RadamsaMutator::~RadamsaMutator()
§§{}
§§
bool RadamsaMutator::isSetupFunctionable() {
	bool setupFunctionable = true;
	//1) check if the radamsa executable exists
#if defined(_WIN32) || defined(_WIN64)
	setupFunctionable &= std::experimental::filesystem::exists(".\\radamsa.exe");
#else
	setupFunctionable &= std::experimental::filesystem::exists("./radamsa.exe");
#endif

	//2) check if the testcase directory  exists
	setupFunctionable &= std::experimental::filesystem::exists(m_testcaseDir);

	return setupFunctionable;
}

std::deque<TestcaseDescriptor> RadamsaMutator::batchMutate(unsigned int numToGenerate, const FluffiTestcaseID parentID, const std::string parentPathAndFilename)
{
§§	std::deque<TestcaseDescriptor> generatedMutations{};
§§
	std::error_code ec;
	if (!std::experimental::filesystem::exists(parentPathAndFilename, ec))
§§	{
		LOG(ERROR) << "Parent does not exist (" << ec.message() << ")";
§§		throw std::runtime_error("Parent does not exist");
§§	}
§§
§§	LOG(DEBUG) << "Executing radamsa.exe";
§§	std::string cmdline =
§§#if defined(_WIN32) || defined(_WIN64)
§§		"radamsa.exe --count "
§§#else
§§		"./radamsa.exe --count "
§§#endif
§§		+ std::to_string(numToGenerate) + " --output " + m_testcaseDir + Util::pathSeperator + "radamsa_%n " + parentPathAndFilename;
	if (!executeProcessAndWaitForCompletion(cmdline, 60 * 1000))
§§	{
		if (!std::experimental::filesystem::exists(parentPathAndFilename))
		{
			LOG(ERROR) << "Could not generate testcases with radamsa (" << parentPathAndFilename << " does not exist)";
		}
		else {
			LOG(ERROR) << "Could not generate testcases with radamsa (" << parentPathAndFilename << " exists)";
		}

§§		throw std::runtime_error("Could not generate testcase with radamsa");
§§	}
§§
§§	for (unsigned int i = 1; i <= numToGenerate; ++i)
§§	{
		std::string currentRadamsaFile = m_testcaseDir + Util::pathSeperator + "radamsa_" + std::to_string(i);

§§		FluffiTestcaseID testcaseID = genNewLocalFluffiTestcaseID();
§§		std::string pathAndFilename = Util::generateTestcasePathAndFilename(testcaseID, m_testcaseDir);
		if (!std::experimental::filesystem::exists(currentRadamsaFile)) {
			//Sometimes radamsa fails to generate the desired amount of mutations. In order to avoid error message polution, we will simply break here and re-run radamsa
			LOG(WARNING) << "Radamsa failed to generate the expected file " << pathAndFilename << ". We will break here.";
			break;
		}

§§		// As AVs love to lock the file, attempt to rename them several times
		if (Util::attemptRenameFile(currentRadamsaFile, pathAndFilename))
§§		{
§§			TestcaseDescriptor childTestcaseDescriptor{ testcaseID, parentID, pathAndFilename, false };
§§			generatedMutations.push_back(childTestcaseDescriptor);
§§		}
§§		else
§§		{
§§			LOG(ERROR) << "Error renaming radamsa file: " << currentRadamsaFile;
§§		}
§§	}
§§
§§	return generatedMutations;
}
