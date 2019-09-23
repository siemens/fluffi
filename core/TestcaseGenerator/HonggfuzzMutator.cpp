/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Abian Blome, Thomas Riedmaier
*/

§§#include "stdafx.h"
§§#include "HonggfuzzMutator.h"
§§#include "FluffiServiceDescriptor.h"
§§#include "TestcaseDescriptor.h"
§§#include "Util.h"
§§
#include "mangle.h"

§§HonggfuzzMutator::HonggfuzzMutator(FluffiServiceDescriptor selfServiceDescriptor, std::string testcaseDirectory)
	: FluffiMutator(selfServiceDescriptor, testcaseDirectory)
§§{
§§}
§§
§§HonggfuzzMutator::~HonggfuzzMutator()
§§{
§§}
§§
§§bool HonggfuzzMutator::isSetupFunctionable() {
§§	bool setupFunctionable = true;
§§
§§	//1) check if the testcase directory  exists
§§	setupFunctionable &= std::experimental::filesystem::exists(m_testcaseDir);
§§
§§	return setupFunctionable;
§§}
§§
§§std::deque<TestcaseDescriptor> HonggfuzzMutator::batchMutate(unsigned int numToGenerate, const FluffiTestcaseID parentID, const std::string parentPathAndFilename) {
§§	std::deque<TestcaseDescriptor> generatedMutations{};

§§	std::error_code ec;
§§	if (!std::experimental::filesystem::exists(parentPathAndFilename, ec))
§§	{
§§		LOG(ERROR) << "Parent does not exist (" << ec.message() << ")";
§§		throw std::runtime_error("Parent does not exist");
§§	}

§§	//Depending on file size this might cause trouble!
§§	std::vector<char> parentBytes = Util::readAllBytesFromFile(parentPathAndFilename);
§§
§§	if (parentBytes.size() == 0) {
§§		parentBytes.push_back(static_cast<char>(42));
§§	}
§§
§§	// We assume a max filesize of HONGGFUZZ_MAXSIZE
§§	std::vector<uint8_t> fileBuffer(HONGGFUZZ_MAXSIZE);
§§	run_t run;
§§	run.dynamicFile = &fileBuffer[0];
§§	run.global.cfg.only_printable = false;
	run.global.mutate.maxFileSz = (HONGGFUZZ_MAXSIZE - 1) > (static_cast<double>(parentBytes.size())*1.1) ? static_cast<size_t>(static_cast<double>(parentBytes.size())*1.1) : (HONGGFUZZ_MAXSIZE - 1);
§§	run.global.mutate.mutationsPerRun = 6;
§§	run.dynamicFileSz = parentBytes.size();
§§
§§	for (unsigned int i = 1; i <= numToGenerate; ++i)
§§	{
§§		if (parentBytes.size() < HONGGFUZZ_MAXSIZE)
§§		{
			for (size_t j = 0; j < parentBytes.size(); j++)
§§			{
				fileBuffer[j] = parentBytes[j];
§§			}
§§		}
§§		else
§§		{
§§			LOG(WARNING) << "Parent file too large, skipping...";
§§			break;
§§		}
§§
§§		FluffiTestcaseID testcaseID = genNewLocalFluffiTestcaseID();
§§		std::string pathAndFilename = Util::generateTestcasePathAndFilename(testcaseID, m_testcaseDir);
§§
§§		mangle_mangleContent(&run);
§§
§§		//write mutated bytes to disk
§§		//apparently, fwrite performs much better than ofstream
§§		FILE* testcaseFile;
§§		if (0 != fopen_s(&testcaseFile, pathAndFilename.c_str(), "wb")) {
§§			LOG(ERROR) << "HonggfuzzMutator::batchMutate failed to open the file " << pathAndFilename;
§§			continue;
§§		}
§§		fwrite(run.dynamicFile, sizeof(uint8_t), run.dynamicFileSz, testcaseFile);
§§		fflush(testcaseFile);
§§
§§		fclose(testcaseFile);
§§
§§		TestcaseDescriptor childTestcaseDescriptor{ testcaseID, parentID, pathAndFilename ,false };
§§		generatedMutations.push_back(childTestcaseDescriptor);
§§	}
§§
§§	return generatedMutations;
}
