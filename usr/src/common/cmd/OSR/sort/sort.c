#ident	"@(#)OSRcmds:sort/sort.c	1.1"
#pragma comment(exestr, "@(#) sort.c 26.3 95/09/11 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1995 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sort:sort.c	1.4" */

/*
 *	MODIFICATION HISTORY
 *	S000	20 Jun 1989	scol!abraham
 *	-UNIX Internationalisation
 *	-To avoid possible problems with sign extension
 *	-declared chars to be unsigned.
 *	-'decimal' is declared for LC_NUMERIC dependencies
 *	-Added support for strftime
 *	S001	28 Sept 1989	scol!nitin 	sco!kenl
 *	-Fixed the problem with sort to use _strcoll in cmpa()
 *	L002	01 Nov 1990	scol!dipakg	
 *	- allow for long filenames
 *	L003	05sep91		scol!hughd
 *	- cannot use the undocumented _strcoll (with eos '\n') any longer:
 *	  3.2.4 DS no longer has _strcoll(), nor does xtra/suppds/lib/libintl.a:
 *	  in 3.2.2 DS build, strcoll.o was being taken from the wrong libintl.a
 *	- instead use the undocumented _strncoll for now (hope its args are
 *	  still the same!), anthonys will clean up later
 *	L004	07 Aug 1992	scol!anthonys
 *	- Removed sort's use of _strncoll (see L003). To achieve this, '\n'
 *	  is no longer stored during sorting, but then re-added during output.
 *	- Added the -k command line option, for POSIX.2/XPG4 conformance.
 *	- Added support for correct internationalized collation when sorting
 *	  with key fields.
 *	- Numerical sorting now allows for "thousands separators".
 *	- Made the command line parsing much more rigorous (e.g., use getopt())
 *	- Converted to use message catalogues.
 *	- Sort examines $TMPDIR as a possible temporary directory.
 *	- Improved error reporting during read/write errors.
 *	- "Blank" characters are now defined by the locale instead of being
 *	  hardwired as the space and tab characters.
 *	- When sorting using field specifiers, characters can no longer
 *	  be selected from after the end of the field.
 *	- Removed some lint errors and warnings.
 *	- Removed all the #ifdef INTL stuff - can only build the
 *	  internationalized version now.
 *	- Too many changes, so no diff marks.
 *	L005	26 April 1994	scol!blf
 *	- Fix LTD-212-106: An argument of just `-' means stdin.
 *	- Fix L004: getopt(S) returns an int, not a char.
 *	L006	07 Jul 1994	scol!ianw
 *	- Removed the work around of calling __brk instead of brk and __sbrk
 *	  instead of sbrk, its not necessary now we are using the USL libc.
 *	L007	12 July 1995	scol!anthonys
 *	- Ignore leading blank characters in a numerical sort.
 *	- If a field start falls beyond the field end, a key field is
 *	  considered empty.
 *	- Silenced some compiler/lint warnings (unmarked).
 *	L008	01 Sept 1995	scol!ianw
 *	- Added an early call to strcoll to ensure the memory required by the
 *	  libc collation routines is allocated before sort calls sbrk.
 *	L009	07sep95		scol!hughd
 *	- stop "Invalid reference" notice under load: check for NULL malloc()
 *	  initially, using same error message as when its realloc() fails
 */

typedef char uchar;

#include <unistd.h>						/* L006 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>				/* L002, for PATHSIZE */
#include <values.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
/* #include <errormsg.h> */
#include <errno.h>
#include <locale.h>
#include "sort_msg.h"
#include <langinfo.h>
#include <nl_types.h>
#include <wchar.h>
#include <wctype.h>
#include "../include/osr.h"


#define SIZE	256		/* size of character conversion table */
#define	N	16
#define	C	20
#define NF	10
#define MTHRESH	 8 /* threshold for doing median of 3 qksort selection */
#define TREEZ	32 /* no less than N and best if power of 2 */

/*
 * Memory administration
 *
 * Using a lot of memory is great when sorting a lot of data.
 * Using a megabyte to sort the output of `who' loses big.
 * MAXMEM, MINMEM and DEFMEM define the absolute maximum,
 * minimum and default memory requirements.  Administrators
 * can override any or all of these via defines at compile time.
 * Users can override the amount allocated (within the limits
 * of MAXMEM and MINMEM) on the command line.
 *
 * For PDP-11s, memory is limited by the maximum unsigned number, 2^16-1.
 * Administrators can override this too.
 * Arguments to core getting routines must be unsigned.
 * Unsigned long not supported on 11s.
 *
 */

#ifndef	MAXMEM
#ifdef pdp11
#define	MAXMEM ((1L << 16)-1)
#define	MAXUNSIGNED ((1L << 16)-1)
#else
#define	MAXMEM	1048576	/* Megabyte maximum */
#endif
#endif

#ifndef	MINMEM
#define	MINMEM	  16384	/* 16K minimum */
#endif

#ifndef	DEFMEM
#define	DEFMEM	  32768	/* Same as old sort */
#endif


#define ASC 	0
#define NUM	1
#define MON	2

wctype_t blanktype;
#define	blank(c) iswctype( (wint_t)c, blanktype)

FILE	*os;
static char	*os_name;		/* File name of the current output steam */
static char	*dirtry[] = {"/usr/tmp", "/tmp", NULL};
static char	*tmpdir = NULL;
static char	file1[PATHSIZE + 1];		/* L002 */
static char	*file = file1;
static char	*filep;
#define NAMEOHD 12			/* sizeof("/stm00000aa") */
static int	nfiles;			/* nfiles - eargc = number of tmp files being used */
static int	*lspace;
static int	*maxbrk;
static unsigned tryfor;
static unsigned alloc;
static char bufin[BUFSIZ], bufout[BUFSIZ];	/* Use setbuf's to avoid malloc calls.
					** malloc seems to get heartburn
					** when brk returns storage.
					*/
static char	*no_nl;
static char	*no_nl_f;
static nl_catd catd;
char    *command_name   = "sort";

static int	xfrm_mode;		/* Set to number field that need strxfrm()ing. */
static int	maxrec;
static int 	mflg;
static int	nway;
static int	cflg;
static int	uflg;
static char	*outfil;
static int unsafeout;	/*kludge to assure -m -o works*/
static unsigned char	tabchar;
static int 	eargc;		/* Number of input files to read. If stdin is to
				   be read, it counts also. */
static char	**eargv;
static struct btree {
    uchar *rp;
    int  rn;
} tree[TREEZ], *treep[TREEZ];
static int	blkcnt[TREEZ];
static uchar	**blkcur[TREEZ];
static long	wasfirst = 0, notfirst = 0;
static int	bonus;

static unsigned char zero[256];

static unsigned char	fold[256];

static unsigned char nofold[256] = {
	0000,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0100,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0133,0134,0135,0136,0137,
	0140,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0152,0153,0154,0155,0156,0157,
	0160,0161,0162,0163,0164,0165,0166,0167,
	0170,0171,0172,0173,0174,0175,0176,0177,
	0200,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0212,0213,0214,0215,0216,0217,
	0220,0221,0222,0223,0224,0225,0226,0227,
	0230,0231,0232,0233,0234,0235,0236,0237,
	0240,0241,0242,0243,0244,0245,0246,0247,
	0250,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0300,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0312,0313,0314,0315,0316,0317,
	0320,0321,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0350,0351,0352,0353,0354,0355,0356,0357,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0372,0373,0374,0375,0376,0377
};

static unsigned char	nonprint[256];

static unsigned char	dict[256];

static struct	field {
	unsigned char *code;	/* Mappings of lower case to upper characters */
	unsigned char *ignore;  /* Array of characters to ignore during sort */
	int fcmp;		/* Type of sort, i.e., ascii, numeric or month */
	int rflg;		/* Sort this field in reverse order */
	int bflg[2];		/* Blanks are significant at beginning/end of field */
	int m[2];		/* Field numbers for start/end of this sort field */
	int n[2];		/* First significant character positions of start/end fields */
	int xfrm;		/* This field's sort key has already been strxfrm()ed */
}	*fields;

static struct field proto = {
	nofold,			/* Default is all characters are themselves */
	zero,			/* No characters are ignored by default */
	ASC,			/* Default sorting is ascii */
	1,			/* 1 = normal sort, -1 = reverse order sort */
	0,0,
	0,-1,
	0,0,
	0			/* Sorting is done directly on the input line */
};
static int	nfields;
static int 	error = 1;
static int	rd_errno = 0;		/* Errno of an unsuccessful read */
static int	wt_errno = 0;		/* Errno of an unsuccessful write */

static int	cmp(uchar *, uchar *);
static int	cmpa(uchar *, uchar *);
static void	sort(void);
static int	transform(char *, char *, char *);
static void	msort(uchar **, uchar **);
static void	insert(struct btree **, int );
static void	merge(int, int);
static void	cline(uchar *, uchar *);
static int	rline(register FILE *, register uchar *);
static int	wline(uchar *);
static void	checksort(void);
static char	*setfil(int);
static void	safeoutfil(void);
static void	disorder(char *, char *);
static void	newfile(void);
static void	oldfile(void);
static void	cant(char *);
static void	diag_errorl(int, char *);
static void	diag_psyserrorl(int, int, char *, char *);
static void	diag(char *, char *);
static void	term(int);
static unsigned char *skip(unsigned char *, struct field *, int);
static unsigned char *eol(unsigned char *p);
static void	copyproto(void);
static void	initree(void);
static int	cmpsave(register int);
static void	field(char *, int ,int);
static int	number(char **);
static void	qksort(uchar **, uchar **);
static void	month_init(void);
static int	month(unsigned char *s);
static void	rderror(char *);
static void	wterror(int);
static unsigned grow_core(unsigned, unsigned);
static void	usage(void);

static int	(*compare)() = cmpa;

#define MAXLEN  10

static uchar	*months[12][MAXLEN];
struct tm *localtime();
extern char *nl_langinfo();
struct	tm	ct = {
	0, 0, 1, 0, 86, 0, 0};

extern int  optind, optopt;
extern char *optarg;

main(argc, argv)
char **argv;
{
	register int a;
	struct field *p, *q;
	struct stat statbuf;
	int i, nf;
	int errflg = 0;

	char *ptr;
	long ulimit();

	char tmpstring[2];
	char	**dirs;

	/* close any file descriptors that may have been */
	/* left open -- we may need them all		*/
	for (i = 3; i < 3 + N; i++)
		(void) close(i);

	setlocale (LC_ALL, "");
	catd = catopen(MF_SORT, MC_FLAGS);

	blanktype = wctype("blank");

	fields = (struct field *)malloc(NF*sizeof(struct field));
	if (fields == NULL) {					/* L009 begin */
		errorl(MSGSTR(SORT_ERR_TOOMANYKEYS, "Couldn't malloc space for keys"));
		exit(1);
	}							/* L009 end */
	nf = NF;
	copyproto();
	initree();
	eargv = argv;
	tryfor = DEFMEM;
	tmpstring[1] = '\0';
	while (optind < argc) {
		if (argv[optind][0] == '+') {
			if(++nfields >= nf) {
				if((fields = (struct field *)realloc(fields, (nf + NF) * sizeof(struct field))) == NULL) {
					errorl(MSGSTR(SORT_ERR_TOOMANYKEYS, "Couldn't malloc space for keys"));
					exit(1);
				}
				nf += NF;
			}
			copyproto();
			field(argv[optind++]+1, 0, 0);
		}
		else if (argv[optind][0] == '-' && isdigit (argv[optind][1])) {
			field(argv[optind++]+1, nfields>0, 0);
		}					/* L005 begin... */
		else if (argv[optind][0] != '-' || argv[optind][1] == '\0') {
			eargv[eargc++] = argv[optind++];
		}
		else {
			i = getopt (argc, argv, ":bcdfik:mMno:T:t:ruy.z:");

			switch(i) {			/* ...end L005 */
			case 'b':
			case 'd':
			case 'f':
			case 'i':
			case 'M':
			case 'n':
			case 'r':
				tmpstring[0] = (char)i;		/* L005 */
				field(tmpstring, nfields>0, 0);
				break;

			case 'c':
				cflg = 1;
				break;

			case 'k':
				if(++nfields >= nf) {
					if((fields = (struct field *)realloc(fields, (nf + NF) * sizeof(struct field))) == NULL) {
						errorl(MSGSTR(SORT_ERR_TOOMANYKEYS, "Couldn't malloc space for keys"));
						exit(1);
					}
					nf += NF;
				}
				copyproto();
				field(optarg, 0, 1);
				break;

			case 'm':
				mflg = 1;
				break;

			case 'o':
				outfil = optarg;
				break;

			case 'T':
				if ((strlen(optarg) + NAMEOHD) > sizeof(file1)) {
					errorl(MSGSTR(SORT_ERR_TMP_NAME, "path name too long: %s"), optarg);
					exit(1);
				}
				else
					tmpdir = optarg;
				break;

			case 't':
				tabchar = (unsigned char)*optarg;
				break;

			case 'u':
				uflg = 1;
				break;

			case 'y':
				errno = 0;
				tryfor = strtoul(optarg, &ptr, 10);
				if (errno || ptr == optarg ){
					errorl(MSGSTR(SORT_ERR_ARG, "Invalid argument '%s' to '%c' option"), optarg, (char)i);
					usage();
					exit(1);
				}
				tryfor *= 1024;
				break;

			case 'z':
				errno = 0;
				maxrec = strtol(optarg, &ptr, 10);
				if (errno || ptr == optarg || maxrec < 0 ){
					errorl(MSGSTR(SORT_ERR_ARG, "Invalid argument '%s' to '%c' option"), optarg, (char)i);
					exit(1);
				}
				break;
			case ':':
				switch(optopt){
				case 'y':
					tryfor = MAXMEM;
					break;
				default:
					errorl(MSGSTR(SORT_ERR_MISSOPTARG, "Option -%c requires an operand"), optopt);
					errflg++;
				}
				break;
			case '?':
			default:				/* L005 */
				errorl(MSGSTR(SORT_ERR_UNKNOPT, "Unknown option -%c"), optopt);
				errflg++;
				
			}
		}
	}
	if (errflg){
		usage();
		exit(1);
	}

	q = &fields[0];
	for(a=1; a<=nfields; a++) {
		p = &fields[a];
		if(p->code != proto.code) continue;
		if(p->ignore != proto.ignore) continue;
		if(p->fcmp != proto.fcmp) continue;
		if(p->rflg != proto.rflg) continue;
		if(p->bflg[0] != proto.bflg[0]) continue;
		if(p->bflg[1] != proto.bflg[1]) continue;
		if(p->xfrm != proto.xfrm) continue;
		p->code = q->code;
		p->ignore = q->ignore;
		p->fcmp = q->fcmp;
		p->rflg = q->rflg;
		p->bflg[0] = p->bflg[1] = q->bflg[0];
		p->xfrm = q->xfrm;
	}

	xfrm_mode = 0;
	for(a = nfields>0; a<=nfields; a++) {
		if (fields[a].xfrm)
			++xfrm_mode;
	}

	if(eargc == 0)
		eargv[eargc++] = "-";
	if(cflg && eargc>1) {
		errorl(MSGSTR(SORT_ERR_CHECKFILES, "can check only 1 file"));
		exit(1);
	}

	safeoutfil();

	/*
	 * The following localizable strings may be needed while sorting.
	 * Fetch them now, to avoid any interactions between implicit mallocs in
	 * the message catalogue libraries and sort's use of sbrk()
	 */

	no_nl = MSGSTR(SORT_MSG_NO_NL, "warning: missing NEWLINE added at EOF");
	no_nl_f = MSGSTR(SORT_MSG_NO_NL_F, "warning: missing NEWLINE added at end of input file ");

	/*
	 * This call to strcoll initialises the libc collation routines so
	 * the memory required is allocated before sort calls sbrk.
	 */
	(void) strcoll("", "");					/* L008 */

	lspace = (int *) sbrk(0);				/* L006 */
	maxbrk = (int *) ulimit(3,0L);
	if (!mflg && !cflg)
		if ((alloc=grow_core(tryfor,(unsigned) 0)) == 0) {
			errorl(MSGSTR(SORT_ERR_PSALLOC, "allocation error before sort"));
			exit(1);
		}

	if (tmpdir != NULL){
		dirtry[0] = tmpdir;
	}
	else
		if ((tmpdir = getenv("TMPDIR")) != NULL)
			dirtry[0] = tmpdir;

	a = -1;
	for(dirs=dirtry; *dirs; dirs++) {
		if ( stat(*dirs, &statbuf) == -1){
			a = -2;
			continue;
		}
		(void) sprintf(filep=file1, "%s/stm%.5uaa", *dirs, getpid());
		while (*filep)
			filep++;
		filep -= 2;
		if ( (a=creat(file, 0600)) >=0)
			break;
	}
	switch(a) {
		case -2:
			psyserrorl(errno, MSGSTR(SORT_ERR_NOTEMP, "can't locate temp directory '%s'"), *--dirs);
			exit(1);
			break;
		case -1:
			psyserrorl(errno, MSGSTR(SORT_ERR_NOCREAT_T, "Cannot create temp file '%s'"), file);
			exit(1);
		default:
			break;
	}
	(void) close(a);
	(void) unlink(file);
	if ( (void(*)(int)) sigset(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) sigset(SIGHUP, term);
	if ( (void(*)(int)) sigset(SIGINT, SIG_IGN) != SIG_IGN)
		(void) sigset(SIGINT, term);
	(void) sigset(SIGPIPE, term);
	if ( (void(*)(int)) sigset(SIGTERM, SIG_IGN) != SIG_IGN)
		(void) sigset(SIGTERM, term);
	nfiles = eargc;
	if(!mflg && !cflg) {
		sort();
		if (ferror(stdin))
			rderror("stdin");
		(void) fclose(stdin);
	}

	if (maxrec == 0)  maxrec = 512;
	alloc = (N + 1) * maxrec + N * BUFSIZ;
	for (nway = N; nway >= 2; --nway) {
		if (alloc < (maxbrk - lspace) * sizeof(int *))
			break;
		alloc -= maxrec + BUFSIZ;
	}
	if (nway < 2 || brk((uchar *)lspace + alloc) != 0) {	/* L006 */
		diag_errorl(SORT_ERR_PMALLOC, "allocation error before merge");
		term(0);
	}

	if (cflg)   checksort();

	wasfirst = notfirst = 0;
	a = mflg || cflg ? 0 : eargc;
	if ((i = nfiles - a) > nway) {	/* Do leftovers early */
		if ((i %= (nway - 1)) == 0)
			i = nway - 1;
		if (i != 1)  {
			newfile();
			setbuf(os, bufout);
			merge(a, a+i);
			a += i;
		}
	}
	for(; a+nway<nfiles || unsafeout&&a<eargc; a=i) {
		i = a+nway;
		if(i>=nfiles)
			i = nfiles;
		newfile();
		setbuf(os, bufout);
		merge(a, i);
	}
	if(a != nfiles) {
		oldfile();
		setbuf(os, bufout);
		merge(a, nfiles);
	}
	error = 0;
	term(0);
	/*NOTREACHED*/
}

static void
sort(void)
{
	register uchar *cp;
	register uchar **lp;
	FILE *iop;
	uchar *keep, *ekeep, **mp, **lmp, **ep;
	int n;
	int input_available = 1;	/* True if we still have input to read */
	int i;
	uchar *f;
	int xfrm_incomplete;	/* Ran out of memory while finding strxfrm()ed sort key */
	int can_grow;		/* 1 if there is still more memory available from brk */

	/*
	 * Records are read in from the front of the buffer area.
	 * Pointers to the records are allocated from the back of the buffer.
	 * If a partially read record exhausts the buffer, it is saved and
	 * then copied to the start of the buffer for processing with the
	 * next coreload.
	 *
	 * If any of the d, f, or i flags occurred as options or modifiers
	 * in the command line, we must preprocess the input line since
	 * strcoll() won't work.
	 * We call the function transform() which removes ignored characters,
	 * does case mapping, and then strxfrm()s to produce a sort key
	 * for the line.
	 */

	can_grow = 1;
	keep = 0;
#ifdef lint
	ekeep = 0;
	xfrm_incomplete = 0;
#endif
	i = 0;
	ep = (uchar **) (((uchar *) lspace) + alloc);
	if ((f=setfil(i++)) == NULL) /* open first file */
		iop = stdin;
	else if ((iop=fopen(f,"r")) == NULL)
		cant(f);
	setbuf(iop,bufin);
	do {
		lp = ep - 1;
		cp = (uchar *) lspace;
		*lp-- = cp;
		if (keep != 0){		/* move incomplete line from previous coreload */
			for(; keep < ekeep; *cp++ = *keep++);
			if (xfrm_incomplete)	/* complete line already read,
						   just weren't able to xfrm it */
				if ( (n = transform(*(lp+1), cp, (uchar *)lp)) > 0){
					cp += n;
					*lp-- = cp;
				} else {
					diag_errorl(SORT_ERR_RTL, "fatal: record too large");
					term(0);
				}
		}
		xfrm_incomplete = 0;
		while ((uchar *)lp - cp > 1 && !xfrm_incomplete) {
			/*
			 * Read a line of input. We either
			 *	- Finished with current file - read error or EOF.
			 *	- Managed to read a whole line.
			 *	- Managed to read a whole line, although missing its NL.
			 *	- Ran out of space before reading the whole line
			 *	If we do obtain a full line, we then, if necessary,
			 *	try to extract a key. This involves transforming
			 *	key fields using strxfrm().
			 */
			if (fgets(cp,(uchar *) lp - cp, iop) == NULL){
				if (ferror(iop))
					rderror(f);

				if (keep != 0 )
					/* The kept record was at
					   the EOF.  Let the code
					   below handle it.       */
					;
				else if (i < eargc) {
					if ((f=setfil(i++)) == NULL)
						iop = stdin;
					else if ((iop=fopen(f,"r")) == NULL )
						cant(f);
					setbuf(iop,bufin);
					continue;
				}
				else {
					input_available = 0;
					break;
				}
			}

			n = strlen(cp);
			cp += n - 1;
			if ( *cp == '\n') {
				/*
				 * Must have got a full line without running out
				 * of space
				 */
				*cp++ = '\0';
				keep = 0;
			} else if ( ++cp < (uchar *) lp - 1 ) {
				/*
				 * We can be certain that we completely read
				 * the final record.
				 * The last record of the input file is missing
				 * a NEWLINE, but there was space for the line
				 */
				if(f == NULL)
					diag(no_nl, "");
				else
					diag(no_nl_f, f);
				keep = 0;
				cp++;
			} else {
				/*
				 * the buffer is full. However, we can't tell
				 * yet if there is more input available. It is
				 * also possible that we have just read the
				 * final record, and that it doesn't contain
				 * a newline.
				 */
				keep = *(lp+1);		/* First character to keep */
				ekeep = cp;		/* One past last character to keep */
			}

			if (!keep){
				if ( cp - *(lp+1) + 1 > maxrec )
					maxrec = cp - *(lp+1) + 1;
				if (xfrm_mode){
					if ( (n = transform(*(lp+1), cp, (uchar *)lp)) > 0){
						cp += n;
						*lp-- = cp;
					} else
						xfrm_incomplete = 1;
				} else
					*lp-- = cp;
			}

			while ( ((uchar *)lp - cp <= 2 || xfrm_incomplete ) && can_grow) {
				if (xfrm_incomplete)
					if ( (n = transform(*(lp+1), cp, (uchar *)lp)) > 0){
						cp += n;
						*lp-- = cp;
						xfrm_incomplete = 0;
						continue;
					}
				tryfor = alloc;
				tryfor = grow_core(tryfor,alloc);
				if (tryfor == 0)
					/* could not grow */
					can_grow = 0;
				else {
					/*
					 *  were able to grow so move pointers
					 */
					lmp = ep +
					   (tryfor/sizeof(uchar **) - 1);
					for ( mp = ep - 1; mp > lp;)
						*lmp-- = *mp--;
					ep += tryfor/sizeof(uchar **);
					lp += tryfor/sizeof(uchar **);
					alloc += tryfor;
				}
			}
		}
		if (xfrm_incomplete){
			keep = *(lp+1);
			ekeep = cp;
		}
		if (keep != 0 && *(lp+1) == (uchar *) lspace) {
			diag_errorl(SORT_ERR_RTL, "fatal: record too large");
			term(0);
		}
		lp += 2;
		if(input_available || nfiles != eargc)
			/*
			 * Either this coreload filled memory before exhausting input,
			 * or finally exhausting input after committing to use
			 * tmp files when a previous coreload filled memory.
			 * In either case, must sort to a tmp file.
			 */
			newfile();
		else
			/*
			 * Everything is in memory. Can sort in memory and write
			 * directly to the desired output file/stdio.
			 */
			oldfile();
		setbuf(os, bufout);
		msort(lp, ep);
		if (ferror(os))
			wterror(0);
		else if (fclose(os))
			wterror(0);
	} while (input_available);
}

/*
 * This routine takes a line of input, and extracts any key fields
 * that can't be collated using strcoll(). Each field is extracted,
 * ignore characters removed, character mappings made, and then strxfrm()ed.
 *
 * start_of_line    - line to be transformed
 * start_free_space - beginning of unused scratch area
 * end_free_space   - first unusable location at end of scratch area.
 *
 * Return value n 0 - insufficient space available
 *               >0 - total amount of space occupied be the
 *                    transformed strings. The global maxrec is also
 *		      updated to indicate the maximum memory usage that
 *		      occurred during the transformation.
 */

static int
transform(char *start_of_line, char *start_free_space, char *end_free_space)
{
	register unsigned char *pa;
	register unsigned char *ignore;
	int sa;
	int total_sa;
	unsigned char *code;
	int k;
	unsigned char *la;
	unsigned char *ipa;
	char *topa;
	struct field *fp;

	total_sa = 0;

	for(k = nfields>0; k<=nfields; k++) {
		fp = &fields[k];
		if (!fp->xfrm)
			continue;
		topa = start_free_space;
		pa = (unsigned char *)start_of_line;
		if(k) {
			la = skip(pa, fp, 1);
			pa = skip(pa, fp, 0);
		} else {
			la = eol(pa);
		}
		code = fp->code;
		ignore = fp->ignore;
		for ( ipa = pa ; ipa < la ; ipa++){
			if (ignore[*ipa])
				continue;
			if ( topa == end_free_space )
				return(0);
			else
				*topa++ = code[*ipa];
		}
		if ( topa == end_free_space )
			return(0);
		else
			*topa++ = '\0';
		sa = strxfrm(topa, start_free_space, end_free_space - topa);
		if (sa < end_free_space - topa){

			if ( (topa - start_of_line) + sa + 1 > maxrec)
				maxrec = (topa - start_of_line) + sa + 1;
			/*
			 * copy strxfrm'ed key back over the intermediate string.
			 */
			memmove(start_free_space, topa, sa +1);

			total_sa += sa + 1;
			start_free_space += sa + 1;
		} else
			return(0);
	}
	return(total_sa);
}

static void
msort(uchar ** a, uchar ** b)
{
	register struct btree **tp;
	register int i, j, n;
	uchar *save;

	i = (b - a);
	if (i < 1)
		return;
	else if (i == 1) {
		(void) wline(*a);
		return;
	}
	else if (i >= TREEZ)
		n = TREEZ; /* number of blocks of records */
	else n = i;

	/* break into n sorted subgroups of approximately equal size */
	tp = &(treep[0]);
	j = 0;
	do {
		(*tp++)->rn = j;
		b = a + (blkcnt[j] = i / n);
		qksort(a, b);
		blkcur[j] = a = b;
		i -= blkcnt[j++];
	} while (--n > 0);
	n = j;

	/* make a sorted binary tree using the first record in each group */
	for (i = 0; i < n;) {
		(*--tp)->rp = *(--blkcur[--j]);
		insert(tp, ++i);
	}
	wasfirst = notfirst = 0;
	bonus = cmpsave(n);


	j = uflg;
	tp = &(treep[0]);
	while (n > 0)  {
		if (wline((*tp)->rp))
			return;
		if (j) save = (*tp)->rp;

		/* Get another record and insert.  Bypass repeats if uflg */

		do {
			i = (*tp)->rn;
			if (j) while((blkcnt[i] > 1) &&
					(**(blkcur[i]-1) == '\0')) {
				--blkcnt[i];
				--blkcur[i];
			}
			if (--blkcnt[i] > 0) {
				(*tp)->rp = *(--blkcur[i]);
				insert(tp, n);
			} else {
				if (--n <= 0) break;
				bonus = cmpsave(n);
				tp++;
			}
		} while (j && (*compare)((*tp)->rp, save) == 0);
	}
}


/* Insert the element at tp[0] into its proper place in the array of size n */
/* Pretty much Algorith B from 6.2.1 of Knuth, Sorting and Searching */
/* Special case for data that appears to be in correct order */

static void
insert(struct btree** tp, int n)
{
    register struct btree **lop, **hip, **midp;
    register int c;
    struct btree *hold;

    midp = lop = tp;
    hip = lop++ + (n - 1);
    if ((wasfirst > notfirst) && (n > 2) &&
	((*compare)((*tp)->rp, (*lop)->rp) >= 0)) {
	wasfirst += bonus;
	return;
    }
    while ((c = hip - lop) >= 0) { /* leave midp at the one tp is in front of */
	midp = lop + c / 2;
	if ((c = (*compare)((*tp)->rp, (*midp)->rp)) == 0) break; /* match */
	if (c < 0) lop = ++midp;   /* c < 0 => tp > midp */
	else       hip = midp - 1; /* c > 0 => tp < midp */
    }
    c = midp - tp;
    if (--c > 0) { /* number of moves to get tp just before midp */
	hip = tp;
	lop = hip++;
	hold = *lop;
	do *lop++ = *hip++; while (--c > 0);
	*lop = hold;
	notfirst++;
    } else wasfirst += bonus;
}


static void
merge(int a, int b)
{
	FILE *tfile[N];
	uchar *buffer = (uchar *) lspace;
	register int nf;		/* number of merge files */
	register struct btree **tp;
	register int i, j;
	uchar	*f;
	uchar	*save, *iobuf;

	save = (uchar *) lspace + (nway * maxrec);
	iobuf = save + maxrec;
	tp = &(treep[0]);
	for (nf=0, i=a; i < b; i++)  {
		f = setfil(i);
		if (f == 0)
			tfile[nf] = stdin;
		else if ((tfile[nf] = fopen(f, "r")) == NULL)
			cant(f);
		(*tp)->rp = buffer + (nf * maxrec);
		(*tp)->rn = nf;
		setbuf(tfile[nf], iobuf);
		iobuf += BUFSIZ;
		if (rline(tfile[nf], (*tp)->rp)==0) {
			nf++;
			tp++;
		} else {
			if(ferror(tfile[nf]))
				rderror(f);
			(void) fclose(tfile[nf]);
		}
	}


	/* make a sorted btree from the first record of each file */
	for (--tp, i = 1; i++ < nf;) insert(--tp, i);

	bonus = cmpsave(nf);
	tp = &(treep[0]);
	j = uflg;
	while (nf > 0) {
		if (wline((*tp)->rp))
			break;
		if (j) cline(save, (*tp)->rp);

		/* Get another record and insert.  Bypass repeats if uflg */

		do {
			i = (*tp)->rn;
			if (rline(tfile[i], (*tp)->rp)) {
				if (ferror(tfile[i]))
					rderror(setfil(i+a));
				(void) fclose(tfile[i]);
				if (--nf <= 0) break;
				++tp;
				bonus = cmpsave(nf);
			} else insert(tp, nf);
		} while (j && (*compare)((*tp)->rp, save) == 0 );
	}


	for (i=a; i < b; i++) {
		if (i >= eargc)
			(void) unlink(setfil(i));
	}
	if (ferror(os))
		wterror(1);
	else if (fclose(os))
		wterror(1);
}

static void
cline(uchar *tp, uchar *fp)
{
	int i;

	if (xfrm_mode){
		while(*fp++)
			;
		*tp++ = '\0';
		for ( i = 0 ; i < xfrm_mode ; i++ ){
			while ((*tp++ = *fp++) != '\0')
				;
		}
	} else
		while ((*tp++ = *fp++) != '\0');
}

static int
rline(iop, s)
register FILE *iop;
register uchar *s;
{
	uchar *s_temp;
	register int n;

	s_temp = s;

	if (fgets(s_temp, maxrec,iop) == NULL )
		n = 0;
	else
		n = strlen(s_temp);

	if ( n == 0 )
		return(1);

	s_temp += n - 1;

	if ( *s_temp == '\n' )
		*s_temp++ = '\0';
	else if ( ++n < maxrec) {
		diag(no_nl, "");
		s_temp++;
	} else {
		diag_errorl(SORT_ERR_LTL, "fatal: line too large");
		term(0);
	}
	if (xfrm_mode){
		if ( transform(s, s_temp, s_temp + (maxrec - n)) == 0){
			diag_errorl(SORT_ERR_LTL, "fatal: line too large");
			term(0);
		}
	}
	return(0);
}

static int
wline(uchar *s)
{
	if (fputs( (char *)s,os) == EOF){
		if (!wt_errno)
			wt_errno = errno;
		return(errno);
	}
	if (fputc( '\n', os) == EOF){
		if (!wt_errno)
			wt_errno = errno;
		return(errno);
	}
	return(0);
}

static void
checksort()
{
	uchar *f;
	uchar *lines[2];
	register int i, j, r;
	register uchar **s;
	register FILE *iop;

	s = &(lines[0]);
	f = setfil(0);
	if (f == 0)
		iop = stdin;
	else if ((iop = fopen(f, "r")) == NULL)
		cant(f);
	setbuf(iop, bufin);

	i = 0;   j = 1;
	s[0] = (uchar *) lspace;
	s[1] = s[0] + maxrec;
	if ( rline(iop, s[0]) ) {
		if (ferror(iop)) {
			rderror(f);
		}
		(void) fclose(iop);
		exit(0);
	}
	while ( !rline(iop, s[j]) )  {
		r = (*compare)(s[i], s[j]);
		if (r < 0)
			disorder(MSGSTR(SORT_MSG_DISORDER, "disorder: "), s[j]);
		if (r == 0 && uflg)
			disorder(MSGSTR(SORT_MSG_N_UNIQUE, "non-unique: "), s[j]);
		r = i;  i = j; j = r;
	}
	if (ferror(iop))
		rderror(f);
	(void) fclose(iop);
	exit(0);
}


static void
disorder(char *s, char *t)
{
	diag(s, t);
	term(0);
}

static void
newfile()
{
	register char *f;

	f = setfil(nfiles);
	if((os=fopen(f, "w")) == NULL) {
		diag_psyserrorl(errno, SORT_ERR_NOCREAT, "Cannot create file '%s'", f);
		term(0);
	}
	os_name = f;
	nfiles++;
}

/*
 * If i <  no of input files, return - input filename for a real file
 *                                   - 0              for stdin
 * If i >= no of input files, return - name of a tmp file
 */

static char *
setfil(int i)
{
	if(i < eargc)
		if(eargv[i][0] == '-' && eargv[i][1] == '\0')
			return(0);
		else
			return(eargv[i]);
	i -= eargc;
	filep[0] = i/26 + 'a';
	filep[1] = i%26 + 'a';
	if ( i >= 676 ){	/* 676 = 26*26, the number permutations of two letters */
		diag_errorl(SORT_ERR_TOOMANYTEMPS, "Fatal error, too many temp files needed");
		term(0);
	}
	return(file);
}

static void
oldfile()
{
	if(outfil) {
		if((os=fopen(outfil, "w")) == NULL) {
			diag_psyserrorl(errno, SORT_ERR_NOF, "Cannot create output file %s", outfil);
			term(0);
		}
		os_name = outfil;
	} else
		os = stdout;
}

static void
safeoutfil()
{
	register int i;
	struct stat ostat, istat;

	if(!mflg||outfil==0)
		return;
	if(stat(outfil, &ostat)==-1)
		return;
	if ((i = eargc - N) < 0) i = 0;	/*-N is suff., not nec. */
	for (; i < eargc; i++) {
		if(stat(eargv[i], &istat)==-1)
			continue;
		if(ostat.st_dev==istat.st_dev&&
		   ostat.st_ino==istat.st_ino)
			unsafeout++;
	}
}

static void
cant(char *f)
{
	diag_psyserrorl(errno, SORT_ERR_CANT, "Can't open input file '%s'", f);
	term(0);
}

static void
diag_errorl(int msg_no, char *msg)
{
	/*
	 * Undo any use of data. This avoids any problems should
	 * the following code do an implicit malloc()
	 */
	brk((uchar *)lspace);		/* Assume this can't fail */ /* L006 */
	errorl(MSGSTR(msg_no, msg));
}

static void
diag_psyserrorl(int err, int msg_no, char *msg, char *arg)
{
	/*
	 * Undo any use of data. This avoids any problems should
	 * the following code do an implicit malloc()
	 */
	brk((uchar *)lspace);		/* Assume this can't fail */ /* L006 */
	psyserrorl(err, MSGSTR(msg_no, msg), arg);
}

static void
diag(char *s, char *t)
{
	register FILE *iop;

	iop = stderr;
	(void) fputs(command_name, iop);
	(void) fputs(": ", iop);
	(void) fputs(s, iop);
	(void) fputs(t, iop);
	(void) fputs("\n", iop);
}

static void
term(int sig)
{
	register int i;

	if(nfiles == eargc)
		nfiles++;
	for(i=eargc; i<=nfiles; i++) {	/*<= in case of interrupt*/
		/*
		 * 676 = 26*26, the number permutations of two
		 * lower case letters
		 */
		if (i == eargc + 676)
			continue;
		(void) unlink(setfil(i));	/*with nfiles not updated*/
	}
	exit(error);
}

static int
cmp(uchar *i, uchar *j)
{
	register unsigned char *pa, *pb;
	register int sa;
	int sb;
	int a, b;
	int k;
	unsigned char *la, *lb;
	unsigned char *ipa, *ipb, *jpa, *jpb;
	struct field *fp;
	char *xfrm_pa, *xfrm_pb;
	unsigned char tempa, tempb;

	char decimal = *nl_langinfo(RADIXCHAR);
	char thou_sep = *nl_langinfo(THOUSEP);

	if (xfrm_mode){
		xfrm_pa = memchr(i, '\0', UINT_MAX);
		xfrm_pb = memchr(j, '\0', UINT_MAX);
		xfrm_pa++;
		xfrm_pb++;
	}

	for(k = nfields>0; k<=nfields; k++) {
		fp = &fields[k];
		if (fp->xfrm){
			if ( sa = strcmp(xfrm_pa, xfrm_pb))
				return( sa < 0 ? fp->rflg :
					-fp->rflg);
			else {
				xfrm_pa += strlen(xfrm_pa) + 1;
				xfrm_pb += strlen(xfrm_pb) + 1;
				continue;
			}
		}
		pa = (unsigned char *)i;
		pb = (unsigned char *)j;
		if(k) {
			la = skip(pa, fp, 1);
			pa = skip(pa, fp, 0);
			lb = skip(pb, fp, 1);
			pb = skip(pb, fp, 0);
		} else {
			la = eol(pa);
			lb = eol(pb);
		}
		if (pa > la)				/* L007 */
			pa = la;			/* L007 */
		if (pb > lb)				/* L007 */
			pb = lb;			/* L007 */
		if(fp->fcmp==NUM) {
			sa = sb = fp->rflg;
			while (pa<la && blank(*pa))	/* L007 */
				pa++;			/* L007 */
			while (pb<lb && blank(*pb))	/* L007 */
				pb++;			/* L007 */
			if(*pa == '-') {
				pa++;
				sa = -sa;
			}
			if(*pb == '-') {
				pb++;
				sb = -sb;
			}
			for(ipa = pa; ipa<la&&(isdigit(*ipa) || *ipa == thou_sep) ; ipa++) ;
			for(ipb = pb; ipb<lb&&(isdigit(*ipb) || *ipb == thou_sep) ; ipb++) ;
			jpa = ipa;
			jpb = ipb;
			a = 0;
			if(sa==sb)
				while(ipa > pa && ipb > pb){
					if (*--ipa == thou_sep)
						--ipa;
					if (*--ipb == thou_sep)
						--ipb;
					if((b = *ipb - *ipa))
						a = b;
				}
			while(ipa > pa){
				if (*--ipa == thou_sep)
					--ipa;
				if (*ipa != '0')
					return(-sa);
			}
			while(ipb > pb){
				if (*--ipb == thou_sep)
					--ipb;
				if (*ipb != '0')
					return(sb);
			}

			if(a) return(a*sa);

			if(*(pa=jpa) == decimal)
				pa++;
			if(*(pb=jpb) ==  decimal)
				pb++;
			if(sa==sb)
				while(pa<la && isdigit(*pa)
				   && pb<lb && isdigit(*pb))
					if(a = *pb++ - *pa++)
						return(a*sa);
			while(pa<la && isdigit(*pa))
				if(*pa++ != '0')
					return(-sa);
			while(pb<lb && isdigit(*pb))
				if(*pb++ != '0')
					return(sb);
			continue;
		} else if(fp->fcmp==MON) {
			sa = fp->rflg*(month(pb)-month(pa));
			if(sa)
				return(sa);
			else
				continue;
		} else {
			/*
			 * we can simply sort the fields using strcoll(),
			 * provided we bound things correctly
			 */
			tempa = *la;
			tempb = *lb;
			*la = '\0';
			*lb = '\0';
			if ( (sa = strcoll((char *)pa, (char *)pb))){
				*la = tempa;
				*lb = tempb;
				return( sa < 0 ? fp->rflg :
					-fp->rflg);
			}
			else{
				*la = tempa;
				*lb = tempb;
				continue;
			}
		}
	}
	if(uflg)
		return(0);
	return(cmpa(i, j));
}

static int
cmpa(uchar *pa, uchar *pb)
{
	int res;
	res = strcoll(pa, pb);				/* L003 */
	return (res == 0 ? 0 :
		res < 0 ? fields[0].rflg :
		-fields[0].rflg);
}

static unsigned char *
skip(unsigned char *p, struct field *fp, int j)
{
	register int i;
	register unsigned char tbc;

	if( (i=fp->m[j]) < 0)
		return(eol(p));
	if (tbc = tabchar){
		while (--i >= 0) {
			while(*p != tbc)
				if(*p == '\0')
					return(p);
				else
					p++;
			if (i > 0 || j == 0)
				p++;
		}
		if(fp->bflg[j]) {
			if (j == 1 && fp->m[j] > 0)
				p++;
			while(blank(*p) && *p != tbc)
				p++;
		}
		i = fp->n[j];
		while((i-- > 0) && (*p != '\0') && *p != tbc )
			p++;
	}
	else{
		while (--i >= 0) {
			while(blank(*p))
				p++;
			while(!blank(*p))
				if(*p != '\0')
					p++;
				else
					return(p);
		}
		if(fp->bflg[j]) {
			if (j == 1 && fp->m[j] > 0)
				p++;
			while(blank(*p))
				p++;
		}
		i = fp->n[j];
		/*
		 * Find the i'th character in the field. Note that
		 * blanks at the start of a field count as any other
		 * character, whereas blanks found after the first
		 * non-blank character terminate the field.
		 */
		while(blank(*p) && (i-- > 0))
			p++;
		while((i-- > 0) && !blank(*p) && (*p != '\0'))
			p++;
	}
	return(p);
}

static unsigned char *
eol(unsigned char *p)
{
	while(*p)
		p++;
	return(p);
}

static void
copyproto()
{
	register int i;
	register int *p, *q;

	p = (int *)&proto;
	q = (int *)&fields[nfields];
	for(i=0; i<sizeof(proto)/sizeof(*p); i++)
		*q++ = *p++;
}

static void
initree()
{
	register struct btree **tpp, *tp;
	register int i;

	for (tp = &(tree[0]), tpp = &(treep[0]), i = TREEZ; --i >= 0;)
	    *tpp++ = tp++;
}

static int
cmpsave(n)
register int n;
{
	register int award;

	if (n < 2) return (0);
	for (n++, award = 0; (n >>= 1) > 0; award++);
	return (award);
}

static void
field(char *s, int k, int kflag)
{
	register struct field *p;
	register int d;
	register int i;

	p = &fields[nfields];
	d = 0;
	for(; *s!=0; s++) {
		switch(*s) {
		case '\0':
			return;

		case 'b':
			p->bflg[k]++;
			break;

		case 'd':
			for(i=0; i<256; i++)
				dict[i] = (!isalnum(i));
			dict['\t'] = dict[' '] = dict['\n'] = 0;
			p->ignore = dict;
			p->xfrm = 1;
			break;

		case 'f':
			for(i=0; i<SIZE; i++)
				fold[i] = (islower(i)? _toupper(i) : i);
			p->code = fold;
			p->xfrm = 1;
			break;

		case 'i':
			for(i=0; i<256; i++)
				nonprint[i] =  (isprint(i)  == 0);
			p->ignore = nonprint;
			p->xfrm = 1;
			break;

		case 'M':
			for(i=0; i<SIZE; i++)
				fold[i] = (islower(i)? _toupper(i) : i);
			month_init();
			p->fcmp = MON;
			p->bflg[0]++;
			break;

		case 'n':
			p->fcmp = NUM;
			p->bflg[0]++;
			break;

		case 'r':
			p->rflg = -1;
			continue;

		case ',':
			d = 0;
			k = 1;
			break;

		case '.':
			if(p->m[k] == -1)	/* -m.n with m missing */
				p->m[k] = 0;
			d = &fields[0].n[0]-&fields[0].m[0];
			if (*++s == '\0') {
				--s;
				p->m[k+d] = 0;
				continue;
			}
			/*FALLTHROUGH*/
		default:
			if (isdigit(*s))
				p->m[k+d] = number(&s);
			else {
				errorl(MSGSTR(SORT_ERR_KARG, "Invalid type '%c' to sort key"), *s);
				exit(1);
			}
		}
		compare = cmp;
	}
	if (kflag){
		--p->m[0];
		--p->n[0];
		if (p->n[1] > 0)
			--p->m[1];
	}
}

static int
number(char **ppa)
{
	register int n;
	register char *pa;

	pa = *ppa;
	n = 0;
	while(isdigit(*pa)) {
		n = n*10 + toint(*pa);
		*ppa = pa++;
	}
	return(n);
}

#define qsexc(p,q) t= *p;*p= *q;*q=t
#define qstexc(p,q,r) t= *p;*p= *r;*r= *q;*q=t

static void
qksort(uchar **a, uchar **l)
{
	register uchar **i, **j;
	register uchar **lp, **hp;
	uchar **k;
	int c, delta;
	uchar *t;
	unsigned n;


start:
	if((n=l-a) <= 1)
		return;

	n /= 2;
	if (n >= MTHRESH) {
		lp = a + n;
		i = lp - 1;
		j = lp + 1;
		delta = 0;
		c = (*compare)(*lp, *i);
		if (c < 0) --delta;
		else if (c > 0) ++delta;
		c = (*compare)(*lp, *j);
		if (c < 0) --delta;
		else if (c > 0) ++delta;
		if ((delta /= 2) && (c = (*compare)(*i, *j)))
		    if (c > 0) n -= delta;
		    else       n += delta;
	}
	hp = lp = a+n;
	i = a;
	j = l-1;


	for(;;) {
		if(i < lp) {
			if((c = (*compare)(*i, *lp)) == 0) {
				--lp;
				qsexc(i, lp);
				continue;
			}
			if(c < 0) {
				++i;
				continue;
			}
		}

loop:
		if(j > hp) {
			if((c = (*compare)(*hp, *j)) == 0) {
				++hp;
				qsexc(hp, j);
				goto loop;
			}
			if(c > 0) {
				if(i == lp) {
					++hp;
					qstexc(i, hp, j);
					i = ++lp;
					goto loop;
				}
				qsexc(i, j);
				--j;
				++i;
				continue;
			}
			--j;
			goto loop;
		}


		if(i == lp) {
			if(uflg)
				for(k=lp; k<hp;) **k++ = '\0';
			if(lp-a >= l-hp) {
				qksort(hp+1, l);
				l = lp;
			} else {
				qksort(a, lp);
				a = hp+1;
			}
			goto start;
		}


		--lp;
		qstexc(j, lp, i);
		j = --hp;
	}
}


static void
month_init()
{
	char	time_buf[MAXLEN];
	int	i;

	for(i=0; i<12; i++)
		{
		ct.tm_mon = i;
		strftime(time_buf, MAXLEN, "%b", &ct);
		memccpy(months[i], time_buf, '\0', MAXLEN);

		}
}

static int
month(unsigned char *s)
{
	register unsigned char *t, *u;
	register int i;
	register unsigned char *f = fold ;

	for(i=0; i<12; i++) {
		for(t=(unsigned char *)s, u=(unsigned char *)months[i]; f[*t++]==f[*u++]; )
			if(*u==0)
				return(i);
	}
	return(-1);
}

static void
rderror(char *s)
{
	if (s == 0)
		diag_psyserrorl(rd_errno, SORT_ERR_READ_SI, "read error on stdin", "");
	else
		diag_psyserrorl(rd_errno, SORT_ERR_READ_ON, "read error on %s", s);
	term(0);
}

static void
wterror(int action)
{
	if (action == 0){
		if (os == stdout)
			diag_psyserrorl(wt_errno, SORT_ERR_SORT_TO_STDOUT,
					"write error while sorting to stdout", "");
		else
			diag_psyserrorl(wt_errno, SORT_ERR_OSRT_TO_FILE,
					"write error while sorting to file %s", os_name);
	}
	else{
		if (os == stdout)
			diag_psyserrorl(wt_errno, SORT_ERR_MERGE_TO_STDOUT,
					"write error while merging to stdout", "");
		else
			diag_psyserrorl(wt_errno, SORT_ERR_MERGE_TO_FILE,
					"write error while merging to file %s", os_name);
	}
	term(0);
}

static unsigned
grow_core(unsigned size, unsigned cursize)
{
	unsigned newsize;
	/*The variable below and its associated code was written so this would work on */
	/*pdp11s.  It works on the vax & 3b20 also. */
	long longnewsize;

	longnewsize = (long) size + (long) cursize;
	if (longnewsize < MINMEM)
		longnewsize = MINMEM;
	else
	if (longnewsize > MAXMEM)
		longnewsize = MAXMEM;
	newsize = (unsigned) longnewsize;
	for (; ((uchar *)lspace+newsize) <= (uchar *)lspace; newsize >>= 1);
	if (longnewsize > (long) (maxbrk - lspace) * (long) sizeof(int *))
		newsize = (maxbrk - lspace) * sizeof(int *);
	if (newsize <= cursize)
		return(0);
	if (brk((uchar *) lspace + newsize) != 0)		/* L006 */
		return(0);
	return(newsize - cursize);
}

static void
usage()

{
	(void) fprintf(stderr, MSGSTR(SORT_MSG_USAGE, "Usage: sort [-m|c] [-o output][-bdfiMnru][-t tabchar][-k keydef][-T tempdir] ...[-y[kmem]] [-z recsz][file ...]\n"));
}
