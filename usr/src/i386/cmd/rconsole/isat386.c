/*		copyright	"%c%" 	*/

#ident	"@(#)rconsole:i386/cmd/rconsole/isat386.c	1.1.2.2"
#ident  "$Header$"

/* isat386 exits with a status of 0 if the controlling terminal is an
 * AT386(-M) and with a 1 otherwise.
 */

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>

void usage();
char *cmd;

main(argc, argv)
int	argc;
char	**argv;
{
	int retval;

	if (argc != 1)
	{
		fprintf(stderr, "usage: %s\n", argv[0]);
		exit(1);
	}

	if ((retval = ioctl(fileno(stdin), KIOCINFO)) == -1)
		exit(1);

	if (retval != (('k' << 8) | 'd'))
		exit(1);

	exit(0);
}
