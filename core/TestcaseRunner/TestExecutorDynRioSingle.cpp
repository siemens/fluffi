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
#include "TestExecutorDynRioSingle.h"
#include "FluffiTestcaseID.h"
#include "DebugExecutionOutput.h"
#include "Util.h"
#include "ExternalProcess.h"
#include "GarbageCollectorWorker.h"

TestExecutorDynRioSingle::TestExecutorDynRioSingle(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover, const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, const std::string additionalEnvParam, GarbageCollectorWorker* garbageCollectorWorker, bool treatAnyAccessViolationAsFatal)
	: TestExecutorDynRio(targetCMDline, hangTimeoutMS, modulesToCover, testcaseDir, child_output_mode, additionalEnvParam, garbageCollectorWorker, treatAnyAccessViolationAsFatal)
{
	Util::markAllFilesOfTypeInPathForDeletion(m_testcaseDir, ".log", m_garbageCollectorWorker);
}

TestExecutorDynRioSingle::~TestExecutorDynRioSingle()
{
	Util::markAllFilesOfTypeInPathForDeletion(m_testcaseDir, ".log", m_garbageCollectorWorker);
}

void TestExecutorDynRioSingle::DebugCommandLine(std::string commandline, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, int timeoutMS, std::shared_ptr<DebugExecutionOutput> exResult, bool doPostMortemAnalysis, bool treatAnyAccessViolationAsFatal, const std::string additionalEnvParam) {
	exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
	exResult->m_terminationDescription = "The target did not run!";

#if defined(_WIN32) || defined(_WIN64)
	//Appending a parameter to the Environment is currently not implemented in Windows, so additionalEnvParam is ignored
	ExternalProcess debuggeeProcess(commandline, child_output_mode);
#else
	ExternalProcess debuggeeProcess(commandline, child_output_mode, additionalEnvParam, false);
#endif
	if (!debuggeeProcess.initProcess()) {
		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		exResult->m_terminationDescription = "Error creating the Process";
		LOG(ERROR) << "Error initializing process " << commandline;
		return;
	}

	debuggeeProcess.debug(timeoutMS, exResult, doPostMortemAnalysis, treatAnyAccessViolationAsFatal);

	return;
}

bool TestExecutorDynRioSingle::isSetupFunctionable() {
	bool setupFunctionable = true;

	//1) check if the executable file exists
#if defined(_WIN32) || defined(_WIN64)
	std::wstring wcmdline = std::wstring(m_targetCMDline.begin(), m_targetCMDline.end());
	int numOfBlocks;
	LPWSTR* szArglist = CommandLineToArgvW(wcmdline.c_str(), &numOfBlocks); //for some reasons this does not exist for Ascii :(
	if (NULL == szArglist || numOfBlocks < 1) {
		return false;
	}

	setupFunctionable &= std::experimental::filesystem::exists(szArglist[0]);

	LocalFree(szArglist);
#else
	std::string tmp = m_targetCMDline;
	std::replace(tmp.begin(), tmp.end(), '<', '_'); // replace all '<' to '_' (handle <INPUT_FILE> et al)
	std::replace(tmp.begin(), tmp.end(), '>', '_'); // replace all '>' to '_' (handle <INPUT_FILE> et al)
	char** argv = ExternalProcess::split_commandline(tmp);
	if (argv == NULL || argv[0] == NULL) {
		LOG(ERROR) << "Splitting target command line \"" << tmp << "\" failed.";
		if (argv != NULL) {
			free(argv);
		}
		return false;
	}

	setupFunctionable &= std::experimental::filesystem::exists(argv[0]);

	//free argv
	int i = 0;
	while (argv[i] != NULL) {
		free(argv[i]);
		i++;
	}
	free(argv);
#endif
	if (!setupFunctionable) {
		LOG(WARNING) << "executable not found";
		return setupFunctionable;
	}

	//2) check if the testcase directory  exists
	setupFunctionable &= std::experimental::filesystem::exists(m_testcaseDir);
	if (!setupFunctionable) {
		LOG(WARNING) << "testcase directory not found";
		return setupFunctionable;
	}

	//3) check if the dynamo rio executable is present at the expected location
#if defined(_WIN64)
	//64 bit Windows
	setupFunctionable &= std::experimental::filesystem::exists("dyndist64\\bin64\\drrun.exe");
#elif defined(_WIN32)
	//32 bit Windows
	setupFunctionable &= std::experimental::filesystem::exists("dyndist32\\bin32\\drrun.exe");
#elif (__WORDSIZE == 64 )
	//64 bit Linux
	setupFunctionable &= std::experimental::filesystem::exists("dynamorio/bin64/drrun");
#else
	//32 bit Linux
	setupFunctionable &= std::experimental::filesystem::exists("dynamorio/bin32/drrun");
#endif
	if (!setupFunctionable) {
		LOG(WARNING) << "dynamorio binary not found";
		return setupFunctionable;
	}

	return setupFunctionable;
}

/**
Execute child with dynamorio and return trace information.
User is responsible for freeing the returned Trace object.
*/
std::shared_ptr<DebugExecutionOutput> TestExecutorDynRioSingle::execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage)
{
	(void)(forceFullCoverage); //avoid unused parameter warning: forceFullCoverage is not relevant for Dynamorio runners

	// create result object
	std::shared_ptr<DebugExecutionOutput> firstExecutionOutput = std::make_shared<DebugExecutionOutput>();

	std::string drcovLogFile = getDrCovOutputFile(); //All drcov ".log" files in the testcase directory should be marked for deletion
	for (int i = 0; i < 100; i++) {
		if (std::experimental::filesystem::exists(drcovLogFile)) {
			//The file must be deleted before continuing. If this is not properly done, future testcase runs might use the wrong file
			LOG(WARNING) << "Performance warning: TestExecutorDynRioSingle is waiting for the GarbageCollector";
			m_garbageCollectorWorker->collectNow(); //Dear garbage collctor: Stop sleeping and try deleting files!
			m_garbageCollectorWorker->collectGarbage(); //This is threadsafe. As the garbage collector could not yet delete the file yet, let's help him!
		}
		else {
			break;
		}
	}
	if (std::experimental::filesystem::exists(drcovLogFile)) {
		firstExecutionOutput->m_terminationDescription = "Could not delete the drcov file after trying 100 times!";
		LOG(ERROR) << firstExecutionOutput->m_terminationDescription;
		firstExecutionOutput->m_terminationType = DebugExecutionOutput::ERR;
		return firstExecutionOutput;
	}

	// build command line to execute
	std::stringstream firstCMDLine;
#if defined(_WIN64)
	//64 bit Windows
	firstCMDLine << "dyndist64\\bin64\\drrun.exe -v -t drcov -dump_binary -logdir \"" + m_testcaseDir + "\" -- ";
#elif defined(_WIN32)
	//32 bit Windows
	firstCMDLine << "dyndist32\\bin32\\drrun.exe -v -t drcov -dump_binary -logdir \"" + m_testcaseDir + "\" -- ";
#elif (__WORDSIZE == 64 )
	//64 bit Linux
	firstCMDLine << "dynamorio/bin64/drrun -v -t drcov -dump_binary -logdir \"" + m_testcaseDir + "\" -- ";
#else
	//32 bit Linux
	firstCMDLine << "dynamorio/bin32/drrun -v -t drcov -dump_binary -logdir \"" + m_testcaseDir + "\" -- ";
#endif

	if (m_targetCMDline.find("<INPUT_FILE>") != std::string::npos) {
		std::string tmp = m_targetCMDline;
		Util::replaceAll(tmp, "<INPUT_FILE>", Util::generateTestcasePathAndFilename(testcaseId, m_testcaseDir));
		firstCMDLine << tmp;
	}
	else {
		firstCMDLine << m_targetCMDline << " " << Util::generateTestcasePathAndFilename(testcaseId, m_testcaseDir);
	}

	DebugCommandLine(firstCMDLine.str(), m_child_output_mode, m_hangTimeoutMS, firstExecutionOutput, false, m_treatAnyAccessViolationAsFatal, m_additionalEnvParam);
	LOG(DEBUG) << "Execution result: " << firstExecutionOutput->m_terminationDescription;
	if (firstExecutionOutput->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR) {
		return firstExecutionOutput;
	}

	drcovLogFile = getDrCovOutputFile();
	if (firstExecutionOutput->m_terminationType == DebugExecutionOutput::CLEAN) {
		if (drcovLogFile == "") {
			firstExecutionOutput->m_terminationDescription = " Could not find DrCov File!";
			LOG(ERROR) << firstExecutionOutput->m_terminationDescription;
			firstExecutionOutput->m_terminationType = DebugExecutionOutput::ERR;
			return firstExecutionOutput;
		}

		std::vector<char> drcovOutput = Util::readAllBytesFromFile(drcovLogFile);

		//delete the drcovLogFile
		if (std::remove(drcovLogFile.c_str()) != 0) {
			// mark the drcovLogFile for lazy delete
			m_garbageCollectorWorker->markFileForDelete(drcovLogFile);
			m_garbageCollectorWorker->collectNow();
		}

		if (drcovOutput.size() == 0) {
			firstExecutionOutput->m_terminationDescription = "drcovOutput is of length 0!";
			LOG(ERROR) << firstExecutionOutput->m_terminationDescription;
			firstExecutionOutput->m_terminationType = DebugExecutionOutput::ERR;
			return firstExecutionOutput;
		}
		copyCoveredModulesToDebugExecutionOutput(&drcovOutput, &m_modulesToCover, firstExecutionOutput);
	}
	else {
		if (drcovLogFile != "") {
			//delete the drcovLogFile
			if (std::remove(drcovLogFile.c_str()) != 0) {
				// mark the drcovLogFile for lazy delete
				m_garbageCollectorWorker->markFileForDelete(drcovLogFile);
				m_garbageCollectorWorker->collectNow();
			}
		}
	}

	if (firstExecutionOutput->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_OTHER ||
		firstExecutionOutput->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::EXCEPTION_ACCESSVIOLATION) {
		//Crashtrace run without dynamo rio to get the actual crash adress

		std::shared_ptr<DebugExecutionOutput> secondExecutionOutput = std::make_shared<DebugExecutionOutput>();
		std::stringstream secondCMDLine;
		secondCMDLine << m_targetCMDline << " " << Util::generateTestcasePathAndFilename(testcaseId, m_testcaseDir);

		DebugCommandLine(secondCMDLine.str(), m_child_output_mode, m_hangTimeoutMS, secondExecutionOutput, true, m_treatAnyAccessViolationAsFatal, m_additionalEnvParam);

		return secondExecutionOutput;
	}
	else {
		return firstExecutionOutput;
	}
}

std::string TestExecutorDynRioSingle::getDrCovOutputFile() {
	for (auto & p : std::experimental::filesystem::directory_iterator(m_testcaseDir))
		if (std::experimental::filesystem::path(p).extension().string() == ".log") {
			return std::experimental::filesystem::path(p).string();
		}

	return "";
}
