§§/* ***************************************************************************
§§* Copyright (c) 2017-2019 Siemens AG  All rights reserved.
§§* ***************************************************************************/
§§
§§/*
§§* Redistribution and use in source and binary forms, with or without
§§* modification, are permitted provided that the following conditions are met:
§§*
§§* * Redistributions of source code must retain the above copyright notice,
§§*   this list of conditions and the following disclaimer.
§§*
§§* * Redistributions in binary form must reproduce the above copyright notice,
§§*   this list of conditions and the following disclaimer in the documentation
§§*   and/or other materials provided with the distribution.
§§*
§§*
§§* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
§§* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
§§* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
§§* ARE DISCLAIMED. IN NO EVENT SHALL Siemens AG OR CONTRIBUTORS BE LIABLE
§§* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
§§* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
§§* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
§§* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
§§* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
§§* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
§§* DAMAGE.
§§* 
§§* Author(s): Thomas Riedmaier, Pascal Eckmann
§§*/
§§
§§/* drcovMulti: DynamoRIO Code Coverage Tool that allows multiple code coverage exports for each run
§§*
§§* Collects information about basic blocks that have been executed.
§§* It simply stores the information of basic blocks seen in bb callback event
§§* into a table without any instrumentation, and dumps the buffer into log files
§§* on thread/process exit, or on nudge event.
§§* To collect per-thread basic block execution information, run DR with
§§* a thread private code cache (i.e., -thread_private).
§§*
§§* The runtime options for this client include:
§§* -dump_text         Dumps the log file in text format
§§* -dump_binary       Dumps the log file in binary format
§§* -[no_]nudge_kills  On by default.
§§*                    Uses nudge to notify a child process being terminated
§§*                    by its parent, so that the exit event will be called.
§§* -logdir <dir>      Sets log directory, which by default is ".".
§§*/
§§
§§#include "dr_api.h"
§§#include "drx.h"
§§#include "drcovlib.h"
§§#include "../common/utils.h"
§§#include <string.h>
§§
§§static uint verbose;
§§static bool nudge_kills;
§§static client_id_t client_id;
§§static const char* fluffi_pipe = NULL;
§§static int desired_fork_level;
§§static int current_fork_level;
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§static HANDLE pipe;
§§#else
§§static int pipe;
§§#endif
§§
§§#define NOTIFY(level, ...) do {          \
§§    if (verbose >= (level))              \
§§        dr_fprintf(STDERR, __VA_ARGS__); \
§§} while (0)
§§
§§#define OPTION_MAX_LENGTH MAXIMUM_PATH
§§
§§/****************************************************************************
§§* Nudges
§§*/
§§
§§enum {
§§	NUDGE_TERMINATE_PROCESS = 1,
§§	NUDGE_DUMP_CURRENT_COVERAGE = 2,
§§	NUDGE_RESET_COVERAGE = 3,
§§	NUDGE_NOOP = 4,
§§};
§§
§§static void
§§event_nudge(void* drcontext, uint64 argument)
§§{
§§	int nudge_arg = (int)argument;
§§	int exit_arg = (int)(argument >> 32);
§§	static int nudge_term_count;
§§	uint count;
§§	unsigned long num_written;
§§	switch (nudge_arg)
§§	{
§§	case NUDGE_TERMINATE_PROCESS:
§§
§§		/* handle multiple from both NtTerminateProcess and NtTerminateJobObject */
§§		count = dr_atomic_add32_return_sum(&nudge_term_count, 1);
§§		if (count == 1) {
§§			dr_exit_process(exit_arg);
§§		}
§§		break;
§§	case NUDGE_DUMP_CURRENT_COVERAGE:
§§		dump_current_coverage();
§§#if defined(_WIN32) || defined(_WIN64)
§§		WriteFile(pipe, "D", 1, &num_written, NULL);
§§#else
§§		num_written = dr_write_file(pipe, "D", 1);
§§#endif
§§		break;
§§	case NUDGE_RESET_COVERAGE:
§§		reset_coverage();
§§#if defined(_WIN32) || defined(_WIN64)
§§		WriteFile(pipe, "R", 1, &num_written, NULL);
§§#else
§§		num_written = dr_write_file(pipe, "R", 1);
§§#endif
§§		break;
§§	case NUDGE_NOOP:
§§		//Do not do anything. This is needed for "is nudge already working" check
§§		break;
§§	default:
§§		ASSERT(nudge_arg == NUDGE_TERMINATE_PROCESS, "unsupported nudge");
§§		ASSERT(false, "should not reach"); /* should not reach */
§§		break;
§§	}
§§}
§§
§§static bool
§§event_soft_kill(process_id_t pid, int exit_code)
§§{
§§	/* we pass [exit_code, NUDGE_TERMINATE_PROCESS] to target process */
§§	dr_config_status_t res;
§§	res = dr_nudge_client_ex(pid, client_id,
§§		NUDGE_TERMINATE_PROCESS | (uint64)exit_code << 32,
§§		0);
§§	if (res == DR_SUCCESS) {
§§		/* skip syscall since target will terminate itself */
§§		return true;
§§	}
§§	/* else failed b/c target not under DR control or maybe some other
§§	* error: let syscall go through
§§	*/
§§	return false;
§§}
§§
§§/****************************************************************************
§§* Event Callbacks
§§*/
§§
§§static void
§§event_exit(void)
§§{
§§#if defined(_WIN32) || defined(_WIN64)
§§	CloseHandle(pipe);
§§#else
§§	dr_close_file(pipe);
§§#endif
§§	drcovlib_exit();
§§}
§§
§§static void
§§options_init(client_id_t id, int argc, const char* argv[], drcovlib_options_t* ops)
§§{
§§	int i;
§§	const char* token;
§§	/* default values */
§§	nudge_kills = true;
§§	desired_fork_level = -1;
§§
§§	for (i = 1/*skip client*/; i < argc; i++) {
§§		token = argv[i];
§§		if (strcmp(token, "-dump_text") == 0)
§§			ops->flags |= DRCOVLIB_DUMP_AS_TEXT;
§§		else if (strcmp(token, "-dump_binary") == 0)
§§			ops->flags &= ~DRCOVLIB_DUMP_AS_TEXT;
§§		else if (strcmp(token, "-no_nudge_kills") == 0)
§§			nudge_kills = false;
§§		else if (strcmp(token, "-nudge_kills") == 0)
§§			nudge_kills = true;
§§		else if (strcmp(token, "-logdir") == 0) {
§§			USAGE_CHECK((i + 1) < argc, "missing logdir path");
§§			ops->logdir = argv[++i];
§§		}
§§		else if (strcmp(token, "-native_until_thread") == 0) {
§§			USAGE_CHECK((i + 1) < argc, "missing -native_until_thread number");
§§			token = argv[++i];
§§			if (dr_sscanf(token, "%d", &ops->native_until_thread) != 1 ||
§§				ops->native_until_thread < 0) {
§§				ops->native_until_thread = 0;
§§				USAGE_CHECK(false, "invalid -native_until_thread number");
§§			}
§§		}
§§		else if (strcmp(token, "-fluffi_pipe") == 0) {
§§			USAGE_CHECK((i + 1) < argc, "missing name of pipe to communicate with fluffi");
§§			fluffi_pipe = argv[++i];
§§		}
§§		else if (strcmp(token, "-fluffi_fork") == 0) {
§§			desired_fork_level = 1;
§§		}
§§		else if (strcmp(token, "-verbose") == 0) {
§§			/* XXX: should drcovlib expose its internal verbose param? */
§§			USAGE_CHECK((i + 1) < argc, "missing -verbose number");
§§			token = argv[++i];
§§			if (dr_sscanf(token, "%u", &verbose) != 1) {
§§				USAGE_CHECK(false, "invalid -verbose number");
§§			}
§§		}
§§		else {
§§			NOTIFY(0, "UNRECOGNIZED OPTION: \"%s\"\n", token);
§§			USAGE_CHECK(false, "invalid option");
§§		}
§§	}
§§	if (dr_using_all_private_caches())
§§		ops->flags |= DRCOVLIB_THREAD_PRIVATE;
§§}
§§
§§static void
§§setup_pipe() {
§§#if defined(_WIN32) || defined(_WIN64)
§§	pipe = CreateFile(
§§		fluffi_pipe,   // pipe name
§§		GENERIC_READ |  // read and write access
§§		GENERIC_WRITE,
§§		0,              // no sharing
§§		NULL,           // default security attributes
§§		OPEN_EXISTING,  // opens existing pipe
§§		0,              // default attributes
§§		NULL);          // no template file
§§
§§	if (pipe == INVALID_HANDLE_VALUE) DR_ASSERT_MSG(false, "error connecting to fluffi pipe");
§§#else
§§	pipe = dr_open_file(fluffi_pipe, DR_FILE_WRITE_ONLY);
§§
§§	if (pipe == -1) DR_ASSERT_MSG(false, "error connecting to fluffi pipe");
§§#endif
§§}
§§
§§static void
§§initialize_fluffi_connection() {
§§	process_id_t ownProcID = dr_get_process_id();
§§	unsigned long num_written;
§§
§§	setup_pipe();
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§	WriteFile(pipe, &ownProcID, sizeof(process_id_t), &num_written, NULL);
§§	ASSERT(num_written == sizeof(process_id_t), "writing the process id to the fluffi pipe failed");
§§	WriteFile(pipe, &client_id, sizeof(client_id_t), &num_written, NULL);
§§	ASSERT(num_written == sizeof(client_id_t), "writing the client id to the fluffi pipe failed");
§§#else
§§	num_written = dr_write_file(pipe, &ownProcID, sizeof(process_id_t));
§§	ASSERT(num_written == sizeof(process_id_t), "writing the process id to the fluffi pipe failed");
§§	num_written = dr_write_file(pipe, &client_id, sizeof(client_id_t));
§§	ASSERT(num_written == sizeof(client_id_t), "writing the client id to the fluffi pipe failed");
§§#endif
§§}
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§#else
§§static void
§§fork_event_handler(void* drcontext)
§§{
§§	current_fork_level++;
§§	if(desired_fork_level == current_fork_level){
§§		initialize_fluffi_connection();
§§	}else{
§§		dr_close_file(pipe);
§§		pipe = -1;
§§	}
§§}
§§#endif
§§
§§DR_EXPORT void
§§dr_client_main(client_id_t id, int argc, const char* argv[])
§§{
§§	drcovlib_options_t ops = { sizeof(ops), };
§§	dr_set_client_name("DrCovMulti", "https://git.fluffi/");
§§	client_id = id;
§§
§§	options_init(id, argc, argv, &ops);
§§	if (drcovlib_init(&ops) != DRCOVLIB_SUCCESS) {
§§		NOTIFY(0, "fatal error: drcovlib failed to initialize\n");
§§		dr_abort();
§§	}
§§	if (!dr_using_all_private_caches()) {
§§		const char* logname;
§§		if (drcovlib_logfile(NULL, &logname) == DRCOVLIB_SUCCESS)
§§			NOTIFY(1, "<created log file %s>\n", logname);
§§	}
§§
§§	if (nudge_kills) {
§§		drx_register_soft_kills(event_soft_kill);
§§		dr_register_nudge_event(event_nudge, id);
§§	}
§§
§§	dr_register_exit_event(event_exit);
§§
§§#if defined(_WIN32) || defined(_WIN64)
§§	initialize_fluffi_connection();
§§#else
§§	//On linux we might want to talk to to the result of a fork instead of to the original process creation
§§	if (desired_fork_level != -1) {
§§		current_fork_level = 0;
§§		dr_register_fork_init_event(fork_event_handler);
§§	}
§§	else {
§§		initialize_fluffi_connection();
§§	}
§§#endif
§§}
