/*		copyright	"%c%" 	*/

#ident	"@(#)drain.output.c	1.2"
#ident	"$Header$"

#include "termio.h"
#include "unistd.h"
#include "stdlib.h"

/*
 * The following macro computes the number of seconds to sleep
 * AFTER waiting for the system buffers to be drained.
 *
 * Various choices:
 *
 *	- A percentage (perhaps even >100%) of the time it would
 *	  take to print the printer's buffer. Use this if it appears
 *	  the printers are affected if the port is closed before they
 *	  finish printing.
 *
 *	- 0. Use this to avoid any extra sleep after waiting for the
 *	  system buffers to be flushed.
 *
 *	- N > 0. Use this to have a fixed sleep after flushing the
 *	  system buffers.
 *
 * The sleep period can be overridden by a single command line argument.
 */
			/* 25% of the print-full-buffer time, plus 1 */
#define LONG_ENOUGH(BUFSZ,CPS)	 (1 + ((250 * BUFSZ) / CPS) / 1000)

extern int		tidbit();

/**
 ** main()
 **/

int			main (argc, argv)
	int			argc;
	char			*argv[];
{
	short			bufsz	= -1,
				cps	= -1;

	char			*TERM;

	int			sleep_time	= 0;


	/*
	 * Wait for the output to drain.
	 */
	(void) ioctl (1, TCSBRK, (struct termio *)1);

	/*
	 * Decide how long to sleep.
	 */
	if (argc != 2 || (sleep_time = atoi(argv[1])) < 0)
		if ((TERM = getenv("TERM"))) {
			(void) tidbit (TERM, "bufsz", &bufsz);
			(void) tidbit (TERM, "cps", &cps);
			if (cps > 0 && bufsz > 0)
				sleep_time = LONG_ENOUGH(bufsz, cps);
		} else
			sleep_time = 2;

	/*
	 * Wait ``long enough'' for the printer to finish
	 * printing what's in its buffer.
	 */
	if (sleep_time)
		(void) sleep (sleep_time);

	exit (0);
	/*NOTREACHED*/
}
