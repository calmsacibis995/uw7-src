/*	copyright	"%c%"	*/

#ident	"@(#)logger.c	1.2"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <pfmt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static char conslog[]	= "/dev/conslog";
/*static char conslog[]	= "/dev/tty";*/
static char Usage_err[]	= ":844:Missing arguments\n";
static char Usage[]	= ":845:Usage: logger string ...\n";
static char Open_err[]	= ":846:Could not open log-file: %s: %s\n";
static char Mem_err[]	= ":847:Out of memory\n";
static char Write_err[]	= ":848:Write to %s failed: %s\n";
static char prefix[]	= "UX:logger:INFO:";
				/* note that 'logger' is hardcoded */
static char delim	= ' ';
static char postfix	= '\n';

main(int argc, char *argv[])
{
	int i, start; 			/* loop and start variables */
	int log;			/* log file descriptor */
	char *out, *tmp;		/* output buffer */
	int bytes;			/* total number of bytes */
	char *in;			/* pointer to input strings */

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:logger");

	/*
	 *	This strange test is to cater for the POSIX.2 guideline
	 *	regarding the handling of '--' even when a command
	 *	supports no options. getopt() is undesirable, so this
	 *	test makes certain that if the first argument is '--',
	 *	it is ignored, as per POSIX.2 spec.
	 */

	if (argc > 1 && strcmp(argv[1], "--") == 0)
		start = 2;
	else
		start = 1;

	if (argc < start+1) {
		pfmt(stderr, MM_ERROR, Usage_err);
		pfmt(stderr, MM_ACTION, Usage);
		exit(1);
	}

	/*
	 *	Find the length of all the arguments, so that they can
	 *	be stuffed into a single buffer, which can be written
	 *	by a single (ie. atomic) write call.
	 */

	bytes = strlen(prefix);
	for (i=start; i < argc; i++) {
		bytes += strlen(argv[i]) + 1;
	}

	/* And malloc the buffer to write from. */

	if ((out = malloc(bytes)) == 0) {
		pfmt(stderr, MM_ERROR, Mem_err);
		exit(1);
	}

	errno = 0;
	log = open(conslog, O_WRONLY | O_APPEND | O_CREAT);
	if (errno != 0) {
		lfmt(stderr, MM_ERROR, Open_err, conslog, strerror(errno));
		/*
		 *	Note: it is dangerous to assume lfmt goes to the same
		 *	place as logger. It is also dangerous to assume the
		 *	opposite. Let's live dangerously...
		 */
		exit(1);
	}

	/*
	 *	Stuff the arguments into a single buffer.
	 *	Using strcpy and strcat would be too slow here.
	 */

	for (tmp=out, in=prefix; *in;)
		*tmp++ = *in++;

	/* loop until the second last argument */
	argc--;
	for (i=start; i < argc; i++) {
		for (in=argv[i]; *in;)
			*tmp++ = *in++;
		*tmp++ = delim;
	}
	/* do the last argument */
	for (in=argv[i]; *in;)
		*tmp++ = *in++;
	*tmp++ = postfix;

	if (write(log, out, bytes) != bytes) {
		pfmt(stderr, MM_ERROR, Write_err, conslog, strerror(errno));
		exit(1);
	}

	(void)close(log);
	return 0;
}
