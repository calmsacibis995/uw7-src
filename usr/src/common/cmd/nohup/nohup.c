/*	copyright	"%c%"	*/
#ident	"@(#)nohup:nohup.c	1.5.1.5"

#include	<stdio.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>

char	nout[100] = "nohup.out";
char	*getenv();

static char badopen[] = ":3:Cannot open %s: %s\n";
static char badchmod[] = ":321:Cannot chmod %s: %s\n";

main(argc, argv)
char **argv;
{
	struct stat sb;
	char	*home;
	FILE *temp;
	int	err;
	int	fileexists = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxue");
	(void)setlabel("UX:nohup");

	/* allow 'end-of-options' delimiter */
	if (argc>1 && strcmp(argv[1],"--") == 0){
		argc--;
		argv++;
	}

	if(argc < 2) {
		pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":4:Usage: nohup command arg ...\n");
		exit(127);
	}
	argv[argc] = 0;
	signal(1, 1);
	signal(3, 1);
	if(isatty(1)) {
		/*
		 * Check if the nohup.out file exists in the current
		 * directory already, so we know whether or not to
		 * perform the chmod() after creation, as per the
		 * POSIX.2 specification.
		 */
		if (stat(nout, &sb) == 0) {
			fileexists = 1;
		}
		if( (temp = fopen(nout, "a")) == NULL) {
			if((home=getenv("HOME")) == NULL) {
				pfmt(stderr, MM_ERROR, badopen, nout,
					strerror(errno));
				exit(127);
			}
			strcpy(nout,home);
			strcat(nout,"/nohup.out");
			if (stat(nout, &sb) == 0) {
				fileexists = 1;
			} else {
				fileexists = 0;
			}
			if(freopen(nout, "a", stdout) == NULL) {
				pfmt(stderr, MM_ERROR, badopen, nout,
					strerror(errno));
				exit(127);
			}
		}
		else {
			fclose(temp);
			freopen(nout, "a", stdout);
		}
		if (!fileexists) {
			if (chmod(nout, S_IRUSR | S_IWUSR) == -1) {
				pfmt(stderr, MM_ERROR, badchmod, nout,
					strerror(errno));
			}
		}
		pfmt(stderr, MM_INFO, ":5:Sending output to %s\n", nout);
	}
	if(isatty(2)) {
		close(2);
		dup(1);
	}

	execvp(argv[1], &argv[1]);
	err = errno;

/* It failed, so print an error */
	freopen("/dev/tty", "w", stderr);
	pfmt(stderr, MM_ERROR, ":6:%s: %s\n", argv[1], strerror(err)); 
	if (err == ENOENT) {
		exit(127);
	} else {
		exit(126);
	}
}
