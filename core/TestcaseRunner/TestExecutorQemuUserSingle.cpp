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
#include "TestExecutorQemuUserSingle.h"
#include "FluffiTestcaseID.h"
#include "DebugExecutionOutput.h"
#include "Util.h"
#include "ExternalProcess.h"
#include "GarbageCollectorWorker.h"

TestExecutorQemuUserSingle::TestExecutorQemuUserSingle(const std::string targetCMDline, int hangTimeoutMS, const std::set<Module> modulesToCover,
	const std::string testcaseDir, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, GarbageCollectorWorker* garbageCollectorWorker,
	bool treatAnyAccessViolationAsFatal, const std::string rootfs)
	: FluffiTestExecutor(targetCMDline, hangTimeoutMS, modulesToCover, testcaseDir, child_output_mode, "", garbageCollectorWorker), //Environment parameters are handled by QEMU's -E switch
	m_treatAnyAccessViolationAsFatal(treatAnyAccessViolationAsFatal),
	m_rootfs(rootfs),
	m_was_initialized{ false }
{
}

#if defined(_WIN32) || defined(_WIN64)
TestExecutorQemuUserSingle::~TestExecutorQemuUserSingle() {
}
bool TestExecutorQemuUserSingle::isSetupFunctionable() {
	return false;
}
std::shared_ptr<DebugExecutionOutput> TestExecutorQemuUserSingle::execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage) {
	(void)(forceFullCoverage); //avoid unused parameter warning: forceFullCoverage is not relevant for qemu runners

	std::shared_ptr<DebugExecutionOutput>  t = std::make_shared<DebugExecutionOutput>();
	t->m_terminationType = DebugExecutionOutput::ERR;
	t->m_terminationDescription = "Windows not supported by qemu runner!";
	return t;
}
#else

TestExecutorQemuUserSingle::~TestExecutorQemuUserSingle()
{
	for (int i = 0; i < 100; ++i) {
		sync();
		int ret = umount((m_rootfs / "dev").string().c_str());
		if (ret == 0) {
			break;
		}
		else {
			LOG(ERROR) << "umount dev failed, retrying";
			if (i == 99) return;
		}
	}
	for (int i = 0; i < 100; ++i) {
		sync();
		int ret = umount((m_rootfs / "testcaseFiles").string().c_str());
		if (ret == 0) {
			Util::attemptDeletePathRecursive(m_testcaseDir + "/rootfs");
			break;
		}
		else {
			LOG(ERROR) << "umount testcaseFiles failed, retrying";
		}
	}
}

// try to set up the needed environment (copy stuff, mount stuff)
bool TestExecutorQemuUserSingle::doInit() {
	bool success = true;
	// copy rootfs to workdir
	Util::createFolderAndParentFolders(m_testcaseDir + "/rootfs/testcaseFiles");

	// rsync -a rootfs/ testcaseFiles/<ID>/rootfs/
	std::stringstream cmdss;
	cmdss << "rsync -a " << m_rootfs.string() << "/ " << m_testcaseDir << "/rootfs/";
	int ret = system(cmdss.str().c_str()); // dear reader. if this hurts your eye, please go and implement it some other way. I dare you.
	success &= (ret == 0);
	LOG(DEBUG) << (success ? "rsync finished" : "rsync failed");

	// set new rootfs prefix
	m_rootfs = std::experimental::filesystem::path(m_testcaseDir) / "rootfs";

	// mount --bind workdir to rootfs inside workdir
	ret = mount(m_testcaseDir.c_str(), (m_rootfs / "testcaseFiles").string().c_str(), NULL, MS_BIND, NULL);
	success &= (ret == 0);
	LOG(DEBUG) << (success ? "mount of tcF finished" : "testcaseFiles mount failed");

	// mount --bind /dev to chroot fs
	ret = mount("/dev", (m_rootfs / "dev").string().c_str(), NULL, MS_BIND, NULL);
	success &= (ret == 0);
	LOG(DEBUG) << (success ? "mount of dev finished" : "dev mount failed");

	// write events file
	std::ofstream ofs;
	ofs.open(m_rootfs / "etc/qemu-events", std::ofstream::out | std::ofstream::trunc);
	ofs << "exec_tb\n";
	ofs.close();

	return m_was_initialized = success;
}

bool TestExecutorQemuUserSingle::isSetupFunctionable() {
	LOG(DEBUG) << "isSetupFunctionable on qemu-linux called";
	bool setupFunctionable = true;

	// check if we are root for chroot
	setupFunctionable &= geteuid() == 0;
	LOG(DEBUG) << (setupFunctionable ? "everything fine" : "not root");
	if (!setupFunctionable) return false;

	// check if necessary files exists
	setupFunctionable &= std::experimental::filesystem::is_directory(m_rootfs);
	LOG(DEBUG) << (setupFunctionable ? "everything fine" : "no rootfs");
	if (!setupFunctionable) return false;

	// check that we can safely call system
	setupFunctionable &= (system(NULL) ? true : false);
	LOG(DEBUG) << (setupFunctionable ? "everything fine" : "no shell callable");
	if (!setupFunctionable) return false;

	// check if the testcase directory exists
	setupFunctionable &= std::experimental::filesystem::exists(m_testcaseDir);
	LOG(DEBUG) << (setupFunctionable ? "everything fine" : "no testcasedir");
	if (!setupFunctionable) return false;

	return setupFunctionable;
}

void TestExecutorQemuUserSingle::DebugCommandLineInChrootEnv(std::string commandline, std::string rootfs, ExternalProcess::CHILD_OUTPUT_TYPE child_output_mode, int timeoutMS, std::shared_ptr<DebugExecutionOutput> exResult, bool treatAnyAccessViolationAsFatal) {
	exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
	exResult->m_terminationDescription = "The target did not run!";

	ExternalProcess debuggeeProcess(commandline, child_output_mode);
	if (!debuggeeProcess.initProcessInChrootEnv(rootfs)) {
		exResult->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		exResult->m_terminationDescription = "Error creating the Process";
		LOG(ERROR) << "Error initializing process " << commandline;
		return;
	}

	debuggeeProcess.debug(timeoutMS, exResult, false, treatAnyAccessViolationAsFatal);

	return;
}

/**
  Execute child with qemu and return trace information.
  User is responsible for freeing the returned Trace object.
  */
std::shared_ptr<DebugExecutionOutput> TestExecutorQemuUserSingle::execute(const FluffiTestcaseID testcaseId, bool forceFullCoverage)
{
	(void)(forceFullCoverage); //avoid unused parameter warning: forceFullCoverage is not relevant for qemu runners

	std::shared_ptr<DebugExecutionOutput> result = std::make_shared<DebugExecutionOutput>();

	if (!m_was_initialized) {
		if (!doInit()) {
			result->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
			return result;
		}
	}

	int lret = link(Util::generateTestcasePathAndFilename(testcaseId, m_rootfs / "testcaseFiles").c_str(), (m_rootfs / "testcaseFiles/fuzzcase").c_str());
	if (lret != 0) {
		LOG(ERROR) << "link failed: " << strerror(errno);
		result->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		return result;
	}

	DebugCommandLineInChrootEnv(m_targetCMDline + " /testcaseFiles/fuzzcase", m_rootfs, m_child_output_mode, m_hangTimeoutMS, result, m_treatAnyAccessViolationAsFatal);
	lret = unlink((m_rootfs / "testcaseFiles/fuzzcase").c_str());
	if (result->m_terminationType == DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR) {
		return result;
	}
	if (lret != 0) {
		LOG(ERROR) << "unlink failed: " << strerror(errno);
		result->m_terminationType = DebugExecutionOutput::PROCESS_TERMINATION_TYPE::ERR;
		return result;
	}

	// delete core files
	Util::markAllFilesOfTypeInPathForDeletion(m_rootfs, ".core", m_garbageCollectorWorker);

	std::vector<std::pair<uint32_t, std::pair<uint64_t, uint64_t>>> modt;

	if (std::experimental::filesystem::exists(m_rootfs / "modules.txt")) {
		std::ifstream is(m_rootfs / "modules.txt", std::ios::in);

		std::string line;
		std::stringstream ss;
		int signal1 = 0;
		int signal2 = 0;
		uint64_t pc1 = 0, pc2 = 0, start = 0, stop = 0;
		if (std::getline(is, line)) {
#if ( __WORDSIZE == 64 )
			int ret = sscanf(line.c_str(), "FLUFFI: first signal %d pc %lx", &signal1, &pc1);
#else
			int ret = sscanf(line.c_str(), "FLUFFI: first signal %d pc %llx", &signal1, &pc1);
#endif
			if (ret != 2) {
				LOG(ERROR) << "sscanf failed to parse the first signal";
			}
		}
		if (std::getline(is, line)) {
#if ( __WORDSIZE == 64 )
			int ret = sscanf(line.c_str(), "FLUFFI: last signal %d pc %lx", &signal2, &pc2);
#else
			int ret = sscanf(line.c_str(), "FLUFFI: last signal %d pc %llx", &signal2, &pc2);
#endif
			if (ret != 2) {
				LOG(ERROR) << "sscanf failed to parse the last signal";
			}
		}

		std::getline(is, line);
		while (std::getline(is, line)) {
			// yes, this could be leveraged for a buffer overflow.
			// well, security was not a requirement.
			std::string mod(1024, ' ');
#if ( __WORDSIZE == 64 )
			int ret = sscanf(line.c_str(), "FLUFFI: %lx - %lx %s", &start, &stop, &mod[0]);
#else
			int ret = sscanf(line.c_str(), "FLUFFI: %llx - %llx %s", &start, &stop, &mod[0]);
#endif
			if (ret != 3) {
				LOG(ERROR) << "sscanf failed parsing";
			}

			mod = std::string(mod.c_str());

			if (result->m_terminationType != DebugExecutionOutput::PROCESS_TERMINATION_TYPE::CLEAN) {
				if (pc1 <= stop && pc1 >= start) {
					ss << std::experimental::filesystem::path(mod).filename() << "+";
					ss << std::hex << std::setw(8) << std::setfill('0') << (pc1 - start);
					result->m_firstCrash = ss.str();
					ss.str(""); ss.clear();
				}
				if (pc2 <= stop && pc2 >= start) {
					ss << std::experimental::filesystem::path(mod).filename() << "+";
					ss << std::hex << std::setw(8) << std::setfill('0') << (pc2 - start);
					result->m_lastCrash = ss.str();
					ss.str(""); ss.clear();
				}
			}

			for (auto& m : this->m_modulesToCover) {
				if (std::experimental::filesystem::path(m.m_modulename).filename() == std::experimental::filesystem::path(mod).filename()) {
					if (m.m_modulepath == "*" || m.m_modulepath == mod) {
						LOG(DEBUG) << "found interesting module " << mod << " with id " << m.m_moduleid << " from " << start << " to " << stop;
						modt.push_back(std::pair<uint32_t, std::pair<uint64_t, uint64_t>>(m.m_moduleid, std::pair<uint64_t, uint64_t>(start, stop)));
					}
				}
			}
		}
		is.close();
		if (result->m_firstCrash == "") {
			if (signal1 == 0) {
				ss << "nosig+";
			}
			else {
				ss << "unknown+";
			}
			ss << std::hex << std::setw(8) << std::setfill('0') << pc1;
			result->m_firstCrash = ss.str();
			ss.str(""); ss.clear();
		}
		if (result->m_lastCrash == "") {
			if (signal2 == 0) {
				ss << "nosig+";
			}
			else {
				ss << "unknown+";
			}
			ss << std::hex << std::setw(8) << std::setfill('0') << pc2;
			result->m_lastCrash = ss.str();
			ss.str(""); ss.clear();
		}
		close(open((m_rootfs / "modules.txt").c_str(), O_WRONLY | O_TRUNC));
		//Util::attemptDeleteFile(m_rootfs / "modules.txt", 100);
	}
	else {
		LOG(DEBUG) << "no module table was created";
	}

	std::string tracefile = m_rootfs / ("trace-" + std::to_string(result->m_PID));
	// file format:
	//
	// HEADER
	// magic:
	// ff ff ff ff ff ff ff ff
	// b4 29 a4 0a cb 77 b1 f2
	// version:
	// 04 00 00 00 00 00 00 00
	// END OF HEADER
	//
	// FUNC TABLE
	// 8 byte record type all zeros: record type 0=mapping, 1=event
	// 8 byte mod_num
	// 4 byte name length
	// name length byte ascii name
	//
	// EVENT TABLE
	// 8 byte record type: 01 followed by 7x00
	// QQII
	// 8 byte event id
	// 8 byte timestamp
	// 4 byte ???
	// 4 byte pid
	//
	// for len(type.args)
	//   if type(arg) == string
	//     4 byte strlen
	//     strlen byte ascii
	//   else
	//     8 byte arg

	if (modt.size() == 0) {
		LOG(DEBUG) << "mod talbe empty, deleting trace";
		m_garbageCollectorWorker->markFileForDelete(tracefile);
		return result;
	}

	std::ifstream ifs(tracefile, std::ios::binary | std::ios::in);
	if (!ifs.is_open()) {
		LOG(ERROR) << "trace file open failed";
		return result;
	}
	// check header

	// aparently, gcc stack protection does not accept
	// reinterpretcast on stack arrays,
	// so we need to new this sh*t
	char* b8f = new char[8];
	ifs.read(b8f, 8);

	uint64_t magic = *reinterpret_cast<uint64_t*>(b8f);
	if (magic != 0xffffffffffffffff) {
		LOG(DEBUG) << "trace file header magic mismatch1";
		delete[] b8f;
		return result;
	}

	ifs.read(b8f, 8);
	magic = *reinterpret_cast<uint64_t*>(b8f);
	if (magic != 0xf2b177cb0aa429b4) {
		LOG(DEBUG) << "trace file header magic mismatch2";
		delete[] b8f;
		return result;
	}
	ifs.read(b8f, 8);
	magic = *reinterpret_cast<uint64_t*>(b8f);
	if (magic != 0x04) {
		LOG(DEBUG) << "trace file header version mismatch";
		delete[] b8f;
		return result;
	}

	std::map<uint64_t, std::string> eventmap;
	eventmap.insert(std::pair<uint64_t, std::string>(0xfffffffffffffffe, std::string("dropped")));
	std::set<uint64_t> bbs;

	// parse event table
	uint64_t num_events = 0;
	for (;;) {
		// 8 byte record type: 0=mapping, 1=event
		ifs.read(b8f, 8);
		if (!ifs) {
			if (ifs.eof()) {
				LOG(DEBUG) << "trace file end reached";
				break;
			}
			LOG(DEBUG) << "trace file read got an error1";
			break;
		}
		magic = *reinterpret_cast<uint64_t*>(b8f);
		if (magic == 0) { // mapping
			// 8 byte mod_num
			ifs.read(b8f, 8);
			if (!ifs) {
				LOG(DEBUG) << "trace file read got an error2";
				break;
			}
			uint64_t eventnum = *reinterpret_cast<uint64_t*>(b8f);
			// 4 byte namelen
			ifs.read(b8f, 4);
			if (!ifs) {
				LOG(DEBUG) << "trace file read got an error3";
				break;
			}
			uint32_t namelen = *reinterpret_cast<uint32_t*>(b8f);
			char* name = new char[namelen + 1];
			// namelen byte ascii name
			ifs.read(name, namelen);
			name[namelen] = '\0';
			if (!ifs) {
				LOG(DEBUG) << "trace file read got an error4";
				delete[] name;
				break;
			}
			eventmap.insert(std::pair<uint64_t, std::string>(eventnum, std::string(name)));
			delete[] name;
		}//endmapping
		else if (magic == 1) { // event
			++num_events;
			// 8 byte event id
			ifs.read(b8f, 8);
			if (!ifs) {
				LOG(DEBUG) << "trace file read got an error5";
				break;
			}
			uint64_t eventnum = *reinterpret_cast<uint64_t*>(b8f);
			// 8 byte timestamp: ns since start. overflow????
			ifs.read(b8f, 8);
			if (!ifs) {
				LOG(DEBUG) << "trace file read got an error6";
				break;
			}
			//uint64_t timestamp = *reinterpret_cast<uint64_t*>(b8f);
			// 4 byte ???
			// 4 byte pid
			ifs.read(b8f, 8);
			if (!ifs) {
				LOG(DEBUG) << "trace file read got an error7";
				break;
			}
			uint32_t pid = *reinterpret_cast<uint32_t*>(b8f + 4);
			if (static_cast<int32_t>(pid) != result->m_PID) {
				LOG(DEBUG) << "trace file pid does not match pid";
			}

			if (eventmap[eventnum] == "exec_tb") {
				// 8 byte tb addr
				ifs.read(b8f, 8);
				if (!ifs) {
					LOG(DEBUG) << "trace file read got an error8";
					break;
				}
				//uint64_t tb_addr = *reinterpret_cast<uint64_t*>(b8f);
				// 8 byte program counter
				ifs.read(b8f, 8);
				if (!ifs) {
					LOG(DEBUG) << "trace file read got an error9";
					break;
				}
				uint64_t progcnt = *reinterpret_cast<uint64_t*>(b8f);

				// add basic block to list of covered basic blocks
				bbs.insert(progcnt);
			}
			else if (eventmap[eventnum] == "dropped") {
				// 8 byte dropped event count
				ifs.read(b8f, 8);
				if (!ifs) {
					LOG(DEBUG) << "trace file read got an error10";
					break;
				}
				uint64_t dropcnt = *reinterpret_cast<uint64_t*>(b8f);
				LOG(DEBUG) << "trace file contains " << dropcnt << " dropped events";
			}
			else {
				LOG(ERROR) << "trace file contains unsolicited event entries";
				break;
			}
		}//endevent
		else { // error
			LOG(DEBUG) << "trace file packet type error";
			break;
		}
	}// endfor

	ifs.close();
	delete[] b8f;
	b8f = nullptr;
	close(open(tracefile.c_str(), O_WRONLY | O_TRUNC));
	//Util::attemptDeleteFile(tracefile, 100);
	LOG(DEBUG) << "Num Events: " << num_events << " Num UNIQUE BBs: " << bbs.size();

	// match bbs to modules, fill covered block table
	for (auto& me : modt) {
		uint32_t id = me.first;
		uint64_t start = me.second.first;
		uint64_t stop = me.second.second;
		for (auto i : bbs) {
			if (i <= stop && i >= start) {
				result->addCoveredBasicBlock(FluffiBasicBlock(i - start, id));
				bbs.erase(i);
			}
		}
	}
	LOG(DEBUG) << "unique bbs outside of interesting modules: " << bbs.size();
	return result;
}
#endif
