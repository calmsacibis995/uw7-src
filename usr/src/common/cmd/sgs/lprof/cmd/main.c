#ident	"@(#)lprof:cmd/main.c	1.16.4.7"
/*
* main.c - top level for lprof
*
* lprof [-CPSTVflpsx] [-F func] [-I dir] [-c cntfile] [-o prog] [-r srcfile]
* lprof [-T] -d destfile [-m cntfile] [cntfile ...]
*/
#include <locale.h>
#include <pfmt.h>
#include <stdio.h>
#include <string.h>
#include <sgs.h>
#include <unistd.h>
#include "lprof.h"

struct arginfo args;

static const char usagestr[] = ":1744:Usage:\n\
\t[-CPSTVflpsx] [-F func] [-I dir] [-c cntfile] [-o prog] [-r srcfile]\n\
\t[-T] -d destfile [-m cntfile] [cntfile ...]\n";

static void
argitem(struct strlist *slp, const char *str) /* append str to list slp */
{
	if (slp->nused >= slp->nlist) /* need to grow */
	{
		slp->nlist += 8; /* heuristic amount to increase by */
		slp->list = grow(slp->list, slp->nlist * sizeof(const char *));
	}
	slp->list[slp->nused++] = str;
}

int
main(int argc, char **argv)
{
	char label[256];
	int ch;

	setlocale(LC_ALL, "");
	setcat("uxcds");
	sprintf(label, "UX:%slprof", SGS);
	setlabel(label);
	if (elf_version(EV_CURRENT) == EV_NONE)
		fatal(":1366:elf library out of date\n");
	/*
	* Gather options.  (No more violating command syntax rules.)
	*/
	while ((ch = getopt(argc, argv, "CI:F:PSTVc:d:flm:o:pr:sx")) != EOF)
	{
		switch (ch)
		{
		case 'C': /* demangle function names in summary listing */
			args.option |= OPT_CPPNAMES;
			break;
		case 'I':
			args.option |= OPT_INCDIR;
			argitem(&args.dirs, optarg);
			break;
		case 'F': /* restrict listing to function */
			args.option |= OPT_FUNCTION;
			argitem(&args.fcns, optarg);
			break;
		case 'P': /* "human readable" direct cntfile dump */
			args.option |= OPT_CNTDUMP;
			break;
		case 'S': /* separate source file report */
			args.option |= OPT_SEPARATE;
			break;
		case 'T': /* override time stamp mismatch */
			args.option |= OPT_TIMESTAMP;
			break;
		case 'V': /* print command/sgs version string */
			args.option |= OPT_VERSION;
			break;
		case 'c': /* single source cnt file */
			args.option |= OPT_CNTFILE;
			args.cntf = optarg;
			break;
		case 'd': /* destination cnt file for merge */
			args.option |= OPT_DSTFILE;
			args.cntf = optarg;
			break;
		case 'f': /* emit form feed between source files */
			args.option |= OPT_FORMFEED;
			break;
		case 'l': /* default listing with execution counts */
			args.option |= OPT_BLKCNTS;
			break;
		case 'm': /* cntfile to merge */
			args.option |= OPT_CNTMERGE;
			argitem(&args.mrgs, optarg);
			break;
		case 'o': /* program pathname */
			args.option |= OPT_PROGFILE;
			args.prog = optarg;
			break;
		case 'p': /* default listing style */
			args.option |= OPT_LISTING;
			break;
		case 'r': /* restrict listing to source file */
			args.option |= OPT_SRCFILE;
			argitem(&args.srcs, optarg);
			break;
		case 's': /* produce summary output */
			args.option |= OPT_SUMMARY;
			break;
		case 'x': /* coverage listing (no counts) */
			args.option |= OPT_BLKCOVER;
			break;
		default:
		usage:;
			pfmt(stderr, MM_ACTION, usagestr);
			exit(2);
		}
	}
	/*
	* Gather remaining arguments.  This is an alternate means
	* to name cnt files for merging.
	*/
	if (optind < argc)
	{
		args.option |= OPT_CNTMERGE;
		do {
			argitem(&args.mrgs, argv[optind]);
		} while (++optind < argc);
	}
	/*
	* Check for clashing options and set defaults.
	* Handle merging immediately.
	*/
	if (args.option & (OPT_DSTFILE|OPT_CNTMERGE))
	{
		/*
		* The -d, -T, and -m options are only permitted
		* with each other, and at least the -d must be
		* present along with at least two cnt files.
		*/
		if (args.option & ~(OPT_DSTFILE|OPT_TIMESTAMP|OPT_CNTMERGE))
		{
			error(":1745:Cannot mix merging options " /*CAT*/
				"(-Tdm) with others\n");
			goto usage;
		}
		if ((args.option & OPT_DSTFILE) == 0)
		{
			error(":1746:Merging requires -d option\n");
			goto usage;
		}
		if (args.mrgs.nused < 2)
		{
			error(":1747:Insufficient cnt files for merge\n");
			goto usage;
		}
		mergefiles();
		/*NOTREACHED*/
	}
	/*
	* Not merging, check these options.
	*/
	if ((args.option & (OPT_BLKCNTS|OPT_BLKCOVER))
		== (OPT_BLKCNTS|OPT_BLKCOVER))
	{
		error(":1748:Cannot specify both -l and -x\n");
		goto usage;
	}
	if (args.option & OPT_CNTDUMP
		&& args.option & (OPT_SUMMARY|OPT_BLKCOVER))
	{
		error(":1749:Cannot specify -P with -s or -x\n");
		goto usage;
	}
	if (args.option & OPT_VERSION)
	{
		pfmt(stderr, MM_INFO, ":8: %s %s\n", CPPT_PKG, CPPT_REL);
		exit(0);
	}
	/*
	* Default output is -lp.
	* Default cnt file is "<prog>.cnt",
	* where <prog> defaults to "a.out".
	*/
	if ((args.option & (OPT_SUMMARY|OPT_BLKCOVER)) == 0)
		args.option |= OPT_BLKCNTS|OPT_LISTING;
	if (args.cntf == 0)
	{
		if (args.prog == 0)
			args.cntf = "a.out.cnt";
		else
		{
			const char *s;
			char *p;

			if ((s = strrchr(args.prog, '/')) == 0)
				s = args.prog;
			else
				s++;
			sprintf(p = alloc(strlen(s) + 4 + 1), "%s.cnt", s);
			args.cntf = p;
		}
	}
	/*
	* Verify the source files named for OPT_SRCFILE now.
	*/
	if (args.option & OPT_SRCFILE)
		chksrcs();
	/*
	* Run through the cnt file gathering all the information.
	* Reduce the information (based on restrictions of files
	* or functions, or based on output style).  Then generate
	* the reports, summary first.
	*/
	gather();
	reduce();
	if (args.option & OPT_SUMMARY)
	{
		summary();
		if (args.option & (OPT_LISTING|OPT_BLKCOVER))
		{
			putchar('\n');
			if (args.option & OPT_FORMFEED)
				putchar('\f');
			listing();
		}
	}
	else if (args.option & (OPT_LISTING|OPT_BLKCOVER))
		listing();
	exit(0);
}
