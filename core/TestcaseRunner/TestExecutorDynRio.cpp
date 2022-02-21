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

Author(s): Roman Bendt, Thomas Riedmaier, Abian Blome
*/

#include "stdafx.h"
#include "TestExecutorDynRio.h"
#include "Util.h"
#include "FluffiBasicBlock.h"
#include "DebugExecutionOutput.h"

TestExecutorDynRio::TestExecutorDynRio(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const std::string additionalEnvParam, GarbageCollectorWorker* garbageCollectorWorker, bool treatAnyAccessViolationAsFatal) :
	FluffiTestExecutor(targetCMDline, hangTimeoutMS, modulesToCover, testcaseDir, child_output_mode, additionalEnvParam, garbageCollectorWorker),
	m_treatAnyAccessViolationAsFatal(treatAnyAccessViolationAsFatal)
{
}

TestExecutorDynRio::~TestExecutorDynRio()
{
}

void TestExecutorDynRio::copyCoveredModulesToDebugExecutionOutput(const std::vector<char>* input, const std::set<Module>* modulesToCover, std::shared_ptr<DebugExecutionOutput> texOutput)
{
	// save header to member
	std::string m_header = std::string(&(*input)[0]);

	std::istringstream iss(m_header);
	std::string line;

	// filter header stuff
	std::getline(iss, line);
	if (line != "DRCOV VERSION: 2") {
		LOG(ERROR) << "Trace::copyCoveredModulesToTestResult was called but dynamo rio's output did not start with the expected header (1/2). It was " << line;
		return;
	}
	std::getline(iss, line);
	if (line != "DRCOV FLAVOR: drcov" && line != "DRCOV FLAVOR: drcov-64" && line != "DRCOV FLAVOR: drcov-32") { //windows, linux x64, linux x86
		LOG(ERROR) << "Trace::copyCoveredModulesToTestResult was called but dynamo rio's output did not start with the expected header (2/2). It was " << line;
		return;
	}

	// Get edge coverage hash
	std::getline(iss, line);
	if (line.find("Hash: ") != std::string::npos) {
		size_t pointerToHash = line.find("Hash: ");
		texOutput->m_edgeCoverageHash = line.substr(pointerToHash + 6, line.length() - pointerToHash - 6);
	} else {
		LOG(ERROR) << "Trace::copyCoveredModulesToTestResult was called but dynamo rio's output did not contain the edge coverage hash.";
		return;
	}

	int num_mods = -1;

	// get num of modules loaded
	std::getline(iss, line);
	if (line.find("count ") != std::string::npos) {
		size_t pointerToCount = line.find("count ");
		std::string NumberOfModulesAsString = line.substr(pointerToCount + 6, line.length() - pointerToCount - 6);
		try {
			num_mods = std::stoi(NumberOfModulesAsString);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}
	}
	else {
		LOG(ERROR) << "Trace::copyCoveredModulesToTestResult was called but dynamo rio's output did not contain the information on how many modules were loaded.";
		return;
	}

	// all these header checks are here, because we had an experience where
	// dynamorio changed the format in master while we were developing this.
	std::getline(iss, line);
	if (line != "Columns: id, containing_id, start, end, entry, offset, checksum, timestamp, path" && line != "Columns: id, containing_id, start, end, entry, offset, path") {
		LOG(ERROR) << "Trace::copyCoveredModulesToTestResult was called but dynamo rio's output did not have the expected column order.";
		return;
	}

	std::map<uint32_t, uint32_t> traceModIDToFluffiModID{};

	// iterate over modules and parse name-number association
	for (int i = 0; i < num_mods; ++i) {
		std::getline(iss, line);

		std::vector<std::string> lineTokenized = Util::splitString(line, ",");

		if (lineTokenized.size() < 6) {
			LOG(ERROR) << "Trace::copyCoveredModulesToTestResult was called but dynamo rio's module list did not have the expected format.";
			return;
		}

		int moduleid;
		try {
			moduleid = std::stoi(lineTokenized[0], NULL, 10);
		}
		catch (...) {
			LOG(ERROR) << "std::stoi failed";
			google::protobuf::ShutdownProtobufLibrary();
			_exit(EXIT_FAILURE); //make compiler happy
		}
		//int containing_id = std::stoi(lineTokenized[1], NULL, 10);
		//size_t start = (size_t)std::stoull(lineTokenized[2], NULL, 16);
		//size_t end = (size_t)std::stoull(lineTokenized[3], NULL, 16);
		//size_t entry = (size_t)std::stoull(lineTokenized[4], NULL, 16);
		//size_t offset = (size_t)std::stoull(lineTokenized[5], NULL, 16);
		std::string path = lineTokenized[lineTokenized.size() - 1];
		std::tuple<std::string, std::string> directoryAndFilename = Util::splitPathIntoDirectoryAndFileName(path);

		//LOG(DEBUG) << filename << " was loaded from 0x" << std::hex << start << " to 0x" << std::hex << end;

		for (auto it = modulesToCover->begin(); it != modulesToCover->end(); ++it) {
			//is the file name in the list of modules to trace?
			if (strcasecmp(it->m_modulename.c_str(), (std::get<1>(directoryAndFilename)).c_str()) != 0) {
				continue;
			}

			//additionally: is it important that the path matches and - if yes - does it?
			if (it->m_modulepath != "*" && (strcasecmp(it->m_modulepath.c_str(), (std::get<0>(directoryAndFilename)).c_str()) != 0)) {
				continue;
			}

			//This module should be traced!
			traceModIDToFluffiModID[moduleid] = it->m_moduleid;
			break;
		}
	}

	//get number of basic blocks
	std::string basicblockLine;
	std::getline(iss, basicblockLine);

	unsigned long long numbbs;
	try {
		numbbs = std::stoll(Util::splitString(basicblockLine, " ")[2]);
	}
	catch (...) {
		LOG(ERROR) << "std::stoi/stoll failed";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy
	}

	// read out bbs to separate list
	const char* firstBBEntry = &(*input)[0] + static_cast<int>(iss.tellg()); // this should be the  end of the header

																			 //do size check
	if (static_cast<int>(iss.tellg()) + numbbs * sizeof(bb_entry_t) > input->size()) {
		LOG(ERROR) << "for some reasons we try to read ahead of the drcov file!";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy
	}

	// now the next ptr should point to the first bb_table_entry
	const bb_entry_t* entry = reinterpret_cast<const bb_entry_t*>(firstBBEntry);

	unsigned long long parsedBasicBlocks = 0;
	for (; parsedBasicBlocks < numbbs; ++parsedBasicBlocks, ++entry) {
		//only relevant for dynamo rio that was modified to support multicov: was it hit since the last reset?
		if (entry->hits_since_last_reset == 0) {
			continue;
		}

		if (traceModIDToFluffiModID.find(entry->mod_id) == traceModIDToFluffiModID.end())
		{
			continue;
		}

		unsigned int fluffiModID = traceModIDToFluffiModID[entry->mod_id];

		texOutput->addCoveredBasicBlock(FluffiBasicBlock(entry->start, fluffiModID));
	}

	if (parsedBasicBlocks != numbbs) {
		LOG(ERROR) << "vecbbs size mismatch";
		google::protobuf::ShutdownProtobufLibrary();
		_exit(EXIT_FAILURE); //make compiler happy
	}
}
