#ident	"@(#)lprof:bblk/common/bblk.c	1.3"
/*
* bblk.c - startup code
*/
#include <locale.h>
#include <pfmt.h>
#include <sgs.h>
#include <stdio.h>
#include <unistd.h>
#include "bblk.h"

int
main(int argc, char **argv)
{
	static const char usagemsg[] =
		":1225:usage:  basicblk [-Q] [-x|l] [in] [out]\n";
	char labstr[256];
	int ch, qflag;

	setlocale(LC_ALL, "");
	setcat("uxcds");
	snprintf(labstr, sizeof(labstr), "UX:%sbasicblk", SGS);
	setlabel(labstr);
	qflag = 0;
	while ((ch = getopt(argc, argv, "Q:lx")) != EOF) {
		switch (ch) {
		case 'Q': /* -Q y/n */
			if (*optarg != 'n' && *optarg != 'N')
				qflag = 1;
			break;
		case 'l':
		case 'x':
			break;
		default:
		usage:;
			pfmt(stderr, MM_ACTION, usagemsg);
			return 1;
		}
	}
	if (optind < argc) {
		if (freopen(argv[optind], "r", stdin) == 0) {
		fail:;
			pfmt(stderr, MM_ERROR, ":1394:can't open file %s\n",
				argv[optind]);
			goto usage;
		}
		optind++;
	}
	if (optind < argc) {
		if (freopen(argv[optind], "w", stdout) == 0)
			goto fail;
		optind++;
	}
	if (optind < argc) {
		pfmt(stderr, MM_ERROR, ":1661:too many operands\n");
		goto usage;
	}
	init();
	parse();
	if (qflag)
		printf("\t.ident\t\"basicblk: %s\"\n", CPPT_REL);
	return 0;
}
