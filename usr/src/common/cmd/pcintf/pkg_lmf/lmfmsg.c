#ident	"@(#)pcintf:pkg_lmf/lmfmsg.c	1.2"
/* SCCSID(@(#)lmfmsg.c	7.2	LCC)	/* Modified: 17:11:16 1/30/91 */

/*
 *  LMF Message printer
 */

#include <stdio.h>
#include "lmf.h"

#ifdef MSDOS
#define	ISSWITCH(c)	((c) == '-' || (c) == '/')
#else
#define	ISSWITCH(c)	((c) == '-')
#endif

main(argc, argv)
int argc;
char **argv;
{
	char *msg;
	int raw;
	int textnewline;

	raw = 0;
	textnewline = 0;
	if (argc > 1 && ISSWITCH(argv[1][0])) {
		if (argv[1][1] == '?' || argv[1][1] == 'h' || argv[1][1] == 'H')
			usage();
#ifdef MSDOS
		else if (argv[1][1] == 'r' || argv[1][1] == 'R')
#else
		else if (argv[1][1] == 'r')
#endif
		{
			argc--;
			argv++;
			raw++;
		} else if (argv[1][1] == 'n' || argv[1][1] == 'N') {
			argc--;
			argv++;
			textnewline++;
		} else if (ISSWITCH(argv[1][1])) {
			argc--;
			argv++;
		}
	}
	if (argc < 3) {
		usage();
	}
	if (lmf_open_file(argv[1], "En", "%N") < 0) {
		fprintf(stderr, "lmfmsg: Couldn't open message file %s\n",
			argv[1]);
		exit(2);
	}
	if ((msg = lmf_get_message(argv[2], NULL)) == NULL) {
		fprintf(stderr, "lmfmsg: Couldn't get message %s: %d\n",
			argv[2], lmf_errno);
		exit(3);
	}
	argc -= 3;
	argv += 3;
	while (lmf_message_length > 0) {
		if (!raw && *msg == '%') {
			if (*++msg > '0' && *msg <= '9') {
				if ((*msg - '1') < argc)
					printf("%s", argv[*msg - '1']);
				msg++;
				lmf_message_length -= 2;
				continue;
			}
			lmf_message_length--;
		}
		if (*msg == '\n' && textnewline) {
			/* When "textnewline" is on, then replace all
			 * newline characters with a backslash and 'n' pair.
			 */
			putchar('\\');
			putchar('n');
		} else {
			putchar(*msg);
		}
		msg++;
		lmf_message_length--;
	}
	fflush(stdout);
	exit(0);
}

usage()
{
	fprintf(stderr, "usage: lmfmsg msg_file msg_id [arg] ...\n");
#ifdef MSDOS
	fprintf(stderr, "       lmfmsg /h or /? prints this message\n");
#else
	fprintf(stderr, "       lmfmsg -h or -? prints this message\n");
#endif
	exit(1);
}
