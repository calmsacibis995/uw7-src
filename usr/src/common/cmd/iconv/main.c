/*		copyright	"%c%" 	*/

#ident	"@(#)main.c	1.2"
#ident  "$Header$"

#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include "symtab.h"
#include "kbd.h"

extern int optind, opterr;
extern char *optarg;
extern int inamap;	/* TRUE when in a map */
extern int linnum;	/* line number */

int nerrors = 0;	/* number of calls to yyerror */
int optreach = 0;	/* check for reachability */
int optR = 0;		/* print unreachables as themselves, not octal */
int optt = 0;		/* table summary */
int optv = 0;		/* verification only */
unsigned char oneone[256];	/* one-one mapping table */
int oneflag = 0;

char *prog;
FILE *lexfp;	/* file pointer for lexical analyzer */

main(argc, argv)

	int argc;
	char **argv;
{
	char *outfile;
	register int c;
	register int fd;

	extern struct node *root;
	extern int numnode;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxmesg");
	(void)setlabel("UX:kbdcomp");

	opterr = optt = optv = 0;
	prog = *argv;

	sym_init();
	lexfp = stdin;	/* default to compiling standard-in */
	outfile = NULL;
	while ((c = getopt(argc, argv, "Rrtvo:")) != EOF) {
		switch (c) {
			case 'R': optR = 1; /* fall through... */
			case 'r': optreach = 1; break;
#if 0	/* optt removed */
			case 't': optt = 1; break;
#endif
			case 'v': optv = 1; break;
			case 'o': outfile = optarg;
				  break;
			default:
			case '?':
				pfmt(stderr, MM_ACTION, ":68:Usage: %s [-vrR] [-o outfile] [infile]\n", prog);
				exit(1);
		}
	}
	if (outfile && optv) {
		pfmt(stderr, MM_ERROR, ":69:Options -%c and -%c are mutually exclusive\n", 'o', 'v');
		exit(1);
	}
	if (! outfile) {
		if (optv)
			outfile = "/dev/null";
		else {
			outfile = "kbd.out";
			pfmt(stderr, MM_INFO, ":70:Output file is \"%s\".\n", outfile);
		}
	}
	close(1);
	if (! optv) {
		if ((fd = open(outfile, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0) {
			pfmt(stderr, MM_ERROR, ":17:Cannot create %s: %s\n",
				outfile, strerror (errno));
			exit(1);
		}
		if (fd != 1) {
			pfmt(stderr, MM_ERROR, ":71:Internal error: unexpected file descriptor (%d).\n", fd);
			exit(1);
		}
	}
	if (optind < argc) {
		if (!(lexfp = fopen(argv[optind], "r"))) {
			pfmt(stderr, MM_ERROR, ":18:Cannot open %s: %s\n", argv[optind], strerror(errno));
			exit(1);
		}
	}
	yyparse();	/* compile it */
	/* s_dump(); dump symbol table */
	if (nerrors == 0)
		output();	/* output maps, etc. */
	else {
		if (inamap)
			pfmt(stderr, MM_WARNING,
				":72:Map not terminated (?)\n");
		pfmt(stderr, MM_WARNING, ":73:Errors in input; output empty.\n");
		exit(1);
	}
	exit(0);
}

yyerror(s)

	char *s;
{
	pfmt(stderr, MM_ERROR, ":74:%s on line %d.\n", s, linnum);
	++nerrors;
}
