/*
 * File main.c
 * This is the dlpi unit test (dlpi) program.
 *
 * This single binary contains both a client and a server.
 * the first invocation automatically starts the server in the background.
 *
 * our ipc mechanism is named pipes.  We currently limit ourselves to
 * one server per system by using a fixed named pipe name.
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) @(#)main.c	24.1")

#include "dlpiut.h"

int
main(int argc, const char * const *argv, const char * const *envp)
{
	argc--;
	argv++;

	if (argc == 0)
		usage();
	else {
		/* -n must be first argument to dlpiut */
		if (strcmp(argv[0], "-n") != 0) {
			setuid(0);	/* must be root for SETMCA ioctl */
		}
		client(argc, argv);
	}
	return 0;
}

