/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:xx/vsig.c	1.6.3.3"

/* 
 * This short program is called by a co-process if it wishes to 
 * communicate asynchronously with the controlling FMLI process during
 * the course of its execution.  It blocks til FMLI is ready for a
 * signal then sends a SIGUSR2. 
 */

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include "sizes.h"


/* the path of the FIFO for process synchronization */
char Semaphore[PATHSIZ] = "/tmp/FMLIsem.";

main(argc, argv)
int argc;
char *argv[];
{
	FILE *fp;
	char name[PATHSIZ];
	char *vpid;
	char *getenv();

	if ((vpid = getenv("VPID")) == NULL)
		exit(1);
	strcat(Semaphore, vpid);
	fflush(stdout);
	fflush(stderr);
	/*
	 * The reason for the close(open) is to
	 * block until FACE says its is OK
	 * to send a signal ... A signal when
	 * FACE is updating the screen
	 * can create garbage .....
	 */
	close(open(Semaphore, O_WRONLY));
	kill(strtol(vpid, (char **)NULL, 0), SIGUSR2); /* EFT abs k16 */
	exit(0);
}
