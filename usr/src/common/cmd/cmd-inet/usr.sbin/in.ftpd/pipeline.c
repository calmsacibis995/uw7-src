#ident	"@(#)pipeline.c	1.3"

/*
 * pipeline - helper program for ftpd used in the ftpconversions file.
 *
 * usage: pipeline [-n pipes] command args... [| command args...]...
 *
 * The pipeline command connects the standard output of each command but the
 * last to the standard input of the next command by a pipe(2) and is used
 * when the conversion requires more than one command.
 *
 * For example the following command could be specified in the ftpconversions
 * file to create a compressed tar archive:
 * /etc/inet/pipeline /bin/tar -cf - %s | /bin/compress -c
 *
 * %s is replaced by a filename derived from the user supplied filename.
 *
 * As a filename may contain the '|' character, to stop the creation of a pipe
 * to a program whose name was specified as part of the filename (e.g. "| foo"),
 * pipeline needs a way to distinguish genuine '|' characters from those which
 * were part of the filename.
 *
 * Fortunately in the ftpconversions file the filename is usually supplied as
 * an argument to the first command so appears before the first genuine '|'.
 * Thus providing pipeline is told the number of pipes to create (which is done
 * using the -n option which defaults to 1 if not specified) and the filename
 * does appear before the first '|' character, pipeline can parse its options
 * backwards looking for the specified number of '|' characters and avoid
 * being tricked into executing a program specified as part of the filename.
 */

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef	INTL
#  include <locale.h>
#  include "pipeline_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num, str)	(str)
#endif	/* INTL */

static char *command_name = "pipeline";
static char errbuf[64];

static void
usage(void /* no args */)
{
	(void) fprintf(stderr, MSGSTR(MSG_USAGE,
	"usage: %s [-n pipes] command args... [| command args...]...\n"),
		command_name);
	exit(1);
}

static char *
errstr(char *str)
{
	(void) sprintf(errbuf, MSGSTR(MSG_FAILED,
			"%s: %s failed"), command_name, str);
	return(errbuf);
}

static int
execute(char **args, int readfd) {
	int pfd[2];

	if (pipe(pfd) < 0) {
		perror(errstr("pipe"));
		return(-1);
	}
	switch (fork()) {
	case -1:
		perror(errstr("fork"));
		return(-1);
	case 0: /* child */
		(void) close(pfd[0]);
		if (pfd[1] != 1) {
			if (dup2(pfd[1], 1) < 0) {
				perror(errstr("dup2"));
				exit(2);
			}
			(void) close(pfd[1]);
		}
		if (readfd != 0) {
			if (dup2(readfd, 0) < 0) {
				perror(errstr("dup2"));
				exit(2);
			}
			(void) close(readfd);
		}
		(void) execv(args[0], args);
		perror(errstr("execv"));
		exit(2);
	}
	(void) close(pfd[1]);
	return(pfd[0]);
}

int
main(int argc, char **argv)
{
	int i, nargs, option, pipes = 1, readfd = 0;
	char **cmdv, **basev;

#ifdef	INTL
	setlocale(LC_ALL, "");
	catd = catopen(MF_PIPELINE, MC_FLAGS);
#endif	/* INTL */

	if (argc > 0)
		command_name = basename(argv[0]);

	while ((option = getopt(argc, argv, "n:")) != -1) {
		switch (option) {
		case 'n':
			pipes = atoi(optarg);
			break;
		default:
			usage();
			break;
		}
	}
	if ((cmdv = (char **)malloc(argc * sizeof(char *))) == (char **)0) {
		(void) fprintf(stderr, MSGSTR(MSG_MEMORY,
				"Unable to obtain memory\n"));
		exit(2);
	}
	/* Copy the arguments into cmdv */
	for (nargs = 0; nargs < argc - optind; nargs++)
		cmdv[nargs] = argv[nargs + optind];
	cmdv[nargs] = (char *)0;

	/*
	 * Walk back through the arguments counting pipes, skip the first
	 * and last arguments as they shouldn't be a pipe.
	 */
	for (i = nargs - 2; i > 0; i--) {
		if (cmdv[i] && cmdv[i][0] == '|' && cmdv[i][1] == '\0'
		    && --pipes == 0)
			break;
	}
	if (pipes != 0)
		usage();

	/* Walk through the arguments executing the commands */
	for (basev=&cmdv[0]; i < nargs; i++) {
		if (cmdv[i] && cmdv[i][0] == '|' && cmdv[i][1] == '\0') {
			cmdv[i] = (char *)0;
			if ((readfd = execute(basev, readfd)) < 0) {
				exit(2);
			}
			basev = &cmdv[++i];
		}
	}

	if (readfd != 0) {
		if (dup2(readfd, 0) < 0) {
			perror(errstr("dup2"));
			exit(2);
		}
		(void) close(readfd);
	}
	(void) execv(basev[0], basev);
	perror(errstr("execv"));
	return(2);
}
