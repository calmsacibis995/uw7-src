/*		copyright	"%c%" 	*/

#ident	"@(#)join:join.c	1.5.3.4"

/*	join F1 F2 on stuff */

#include	<stdio.h>
#include	<locale.h>
#include	<sys/euc.h>
#include	<getwidth.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>
#include	<stdlib.h>
#include	<limits.h>
#include	<ctype.h>
#ifndef isblank
#define _XOPEN_SOURCE 1
#include	<wchar.h>
#define _no_isblank
#endif

#ifndef	LINE_MAX
#define	LINE_MAX	2048
#endif	LINE_MAX

#define F1 0
#define F2 1
#define Fany 2
#define	NFLD	(LINE_MAX/2)	/* max field per line */
#define comp() cmp(ppi[F1][j1],ppi[F2][j2])
#define putfield(string) if(*string == NULL) (void) fputs(null, stdout); \
			else (void) fputs(string, stdout)
#ifdef	_no_isblank
#define	isblank(c)	(iswctype((c), blank))
#endif

typedef unsigned char char_t;	/* machine independent type for char */

FILE *f[2];
char buf[2][LINE_MAX+1];	/*input lines */
char *ppi[2][NFLD];	/* pointers to fields in lines */
int	j1	= 1;	/* join of this field of file 1 */
int	j2	= 1;	/* join of this field of file 2 */
int	olist[2*NFLD];	/* output these fields */
int	olistf[2*NFLD];	/* from these files */
int	no;	/* number of entries in olist */
char*	null	= "";
int	aflg;
int	vflg;
int 	discard = 0;

#ifdef _no_isblank
wctype_t blank;
#endif

int	(*strcompare)() = strcmp;

char_t	*msep1	= NULL;
char_t	*msep2	= (char_t *)" ";
int	sepwidth = 1;
eucwidth_t	wp;

static const char badopen[] = ":3:Cannot open %s: %s\n";
static const char badflno[] =
	":92:Invalid file_number: %s\n";
static const char badoptarg[] =
	":93:Invalid argument to option -%c\n";
static const char needsarg[] =
	":94:The option -%c requires argument(s)\n";
static const char usage1[] =
	":95:Usage: join [-a n|-v n] [-e string]"
	" [-o list]\n\t\t\t     [-t c] [-1 m] [-2 m] file1 file2\n";
static const char usage2[] =
	":96:\t\t\tjoin [-a n] [-e string] [-j m] [-j1 m] "
	"[-j2 m]\n\t\t\t     [-o list ...] [-t c] file1 file2\n";
static const char excl[] =
	":97:-a and -v are mutually exclusive\n";

void usage();
void error();
void newsynopsis();
int  input();
void output();
int  cmp();

main(argc, argv)
char *argv[];
{
	int i;
	int n1, n2;
	long top2, bot2;
	int sargc = argc;
	char **sargv = argv;
	int new = 0;
	char *cp;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:join");

	cp = setlocale(LC_COLLATE, 0);
	if (strcmp(cp, "C") != 0 && strcmp(cp, "POSIX") != 0)
		strcompare = strcoll;

	getwidth(&wp);
	wp._eucw2++;
	wp._eucw3++;

#ifdef _no_isblank
	blank = wctype("blank");
#endif

	while (argc > 1 && argv[1][0] == '-') {
		if (argv[1][1] == '\0')
			break;
		switch (argv[1][1]) {
		case 'a':
			if (argv[1][2])
				switch(argv[1][2]) {
				case '1':
					aflg |= 1;
					break;
				case '2':
					aflg |= 2;
					break;
				default:
					aflg |= 3;
				}
			else {
				switch(argv[2][0]) {
				case '1':
					aflg |= 1;
					break;
				case '2':
					aflg |= 2;
					break;
				default:
					aflg |= 3;
				}
				argv++;
				argc--;
			}
			break;
		case 'e':
			if (argv[1][2])
				null = &argv[1][2];
			else {
				null = argv[2];
				argv++;
				argc--;
			}
			break;
		case 't':
			if (argv[1][2])
				msep1 = (char_t *)&argv[1][2];
			else {
				msep1 = (char_t *)argv[2];
				argv++;
				argc--;
			}
			if (wp._multibyte && NOTASCII(*msep1)) {
				if (ISSET2(*msep1))
					sepwidth = wp._eucw2;
				else
				if (ISSET3(*msep1))
					sepwidth = wp._eucw3;
				else
				if (*msep1 < 0240)
					sepwidth = 1;
				else
					sepwidth = wp._eucw1;
			}
			break;
		case 'o':
			for (no = 0; no < 2*NFLD; no++) {
				char *s;
				for (s = argv[2]; *s != '\0'; s++) 
					if (*s == ',' || isblank(*s))
						break;
				if (*s != '\0') {
					new++;
					break;
				}
				if (argv[2][0] == '1' && argv[2][1] == '.') {
					olistf[no] = F1;
					olist[no] = strtol(&argv[2][2],&cp,10);
				} else if (argv[2][0] == '2' && argv[2][1] == '.') {
					olist[no] = strtol(&argv[2][2],&cp,10);
					olistf[no] = F2;
				} else if (strcmp(argv[2],"0") == 0) {
					olistf[no] = Fany;
				} else if (no == 0) { /* first optarg */
				    (void)pfmt(stderr,MM_ERROR, badoptarg, 'o');
				    usage();
				} else
					break;
				argc--;
				argv++;
			}
			break;
		case 'j':
			if (argv[1][2] == '1')
				j1 = strtol(argv[2],&cp,10);
			else if (argv[1][2] == '2')
				j2 = strtol(argv[2],&cp,10);
			else
				j1 = j2 = strtol(argv[2],&cp,10);
			argc--;
			argv++;
			break;
		default:
			new++;
		}
		if (new) break;
		argc--;
		argv++;
	}
	if (new == 0) {
		for (i = 0; i < no; i++)
			olist[i]--;	/* 0 origin */
		if (argc != 3){
			(void)pfmt(stderr, MM_ERROR, ":2:Incorrect usage\n");
			usage();
		}
		j1--;
		j2--;	/* everyone else believes in 0 origin */
		if (strcmp(argv[1], "-") == 0)
			f[F1] = stdin;
		else if ((f[F1] = fopen(argv[1], "r")) == NULL)
			error(badopen, argv[1], strerror(errno));
		if (strcmp(argv[2], "-") == 0)
			f[F2] = stdin;
		else if ((f[F2] = fopen(argv[2], "r")) == NULL)
			error(badopen, argv[2], strerror(errno));
	} else
		newsynopsis(sargc, sargv);

#define get1() n1=input(F1)
#define get2() n2=input(F2)
	get1();
	bot2 = ftell(f[F2]);
	get2();
	while(n1>0 && n2>0 || (aflg!=0 || vflg!=0) && n1+n2>0) {
		if(n1>0 && n2>0 && comp()>0 || n1==0) {
			if(aflg&2 || vflg&2) output(0, n2);
			bot2 = ftell(f[F2]);
			get2();
		} else if(n1>0 && n2>0 && comp()<0 || n2==0) {
			if(aflg&1 || vflg&1) output(n1, 0);
			get1();
		} else /*(n1>0 && n2>0 && comp()==0)*/ {
			while(n2>0 && comp()==0) {
				if(!vflg) output(n1, n2);
				top2 = ftell(f[F2]);
				get2();
			}
			(void) fseek(f[F2], bot2, 0);
			get2();
			get1();
			for(;;) {
				if(n1>0 && n2>0 && comp()==0) {
					if (!vflg) output(n1, n2);
					get2();
				} else if(n1>0 && n2>0 && comp()<0 || n2==0) {
					(void) fseek(f[F2], bot2, 0);
					get2();
					get1();
				} else /*(n1>0 && n2>0 && comp()>0 || n1==0)*/{
					(void) fseek(f[F2], top2, 0);
					bot2 = top2;
					get2();
					break;
				}
			}
		}
	}
	if (discard) {
		(void)pfmt(stderr, MM_ERROR, ":68:Input line too long\n");
		exit(1);
	}
	return(0);
}

int
input(n)		/* get input line and split into fields */
{
	register int i, c;
	char_t *bp;
	char_t **pp;

	register int sepc;
	register int mltwidth = 1;
	int sepflag = 1;

	bp = (char_t *)buf[n];
	pp = (char_t **)ppi[n];
	if (fgets((char *)bp, LINE_MAX+1, f[n]) == NULL)
		return(0);
	i = 0;
	do {
	i++;
		if (msep1 == NULL)
			while (isblank(*bp))
				bp++;	/* skip blanks */
		else
			c = *bp;
		*pp++ = bp;	/* record beginning */
			/* fails badly if string doesn't have \n at end */
		while ((c = *bp) != '\n' && c != '\0') {
			if (!msep1 && isblank(c)) {
				break;
			} else if (msep1 && c == *msep1) {
				for (sepc = 0; sepc < sepwidth ; sepc++) {
					if (*bp != msep1[sepc]) {
						sepflag = 0;
						break;
					}
					++bp;
				}
				if (sepflag) {
					bp -= sepwidth;
					break;
				} else {
					sepflag = 1;
					bp -= sepc;
				}
			}
			if (wp._multibyte && NOTASCII(c)) {
				if (ISSET2(c))
					mltwidth = wp._eucw2;
				else
				if (ISSET3(c))
					mltwidth = wp._eucw3;
				else
				if (c < 0240)
					mltwidth = 1;
				else
					mltwidth = wp._eucw1;
			} else
				mltwidth = 1;
			bp += mltwidth;
		}
		*bp++ = '\0';
		bp += (sepwidth-1);
	} while (c != '\n' && c != '\0' && i < NFLD);
	if (c != '\n' && c != '\0')
		discard++;

	*pp = 0;
	return(i);
}

void
output(on1, on2)	/* print items from olist */
int on1, on2;
{
	int i;
	int sepc;

	if (no <= 0) {	/* default case */
		if (on1)
			putfield(ppi[F1][j1]);
		else
			putfield(ppi[F2][j2]);
		for (i = 0; i < on1; i++)
			if (i != j1) {
				for (sepc = 0; sepc < sepwidth ; sepc++)
				    (void) putchar(msep1?msep1[sepc]:*msep2);
				putfield(ppi[F1][i]);
			}
		for (i = 0; i < on2; i++)
			if (i != j2) {
				for (sepc = 0; sepc < sepwidth ; sepc++)
				    (void) putchar(msep1?msep1[sepc]:*msep2);
				putfield(ppi[F2][i]);
			}
		(void) putchar('\n');
	} else {
		for (i = 0; i < no; i++) {
			if(olistf[i]==F1 && on1<=olist[i] ||
			   olistf[i]==F2 && on2<=olist[i])
				(void) fputs(null, stdout);
			else if (olistf[i] == Fany) {
				if (on1)
					putfield(ppi[F1][j1]);
				else
					putfield(ppi[F2][j2]);
			} else
				putfield(ppi[olistf[i]][olist[i]]);
			if (i < no - 1)
				for (sepc = 0; sepc < sepwidth ; sepc++)
				    (void) putchar(msep1?msep1[sepc]:*msep2);
			else
				(void) putchar('\n');
		}
	}
}

/*VARARGS*/
void
error(s1, s2, s3, s4, s5)
char *s1;
{
	(void) pfmt(stderr, MM_ERROR, s1, s2, s3, s4, s5);
	exit(1);
}

int
cmp(s1, s2)
char *s1, *s2;
{
	if (s1 == NULL) {
		if (s2 == NULL)
			return(0);
		else
			return(-1);
	} else if (s2 == NULL)
		return(1);
	return((*strcompare)(s1, s2));
}

void
newsynopsis(argc, argv)
char *argv[];
{
	extern char *optarg;
	extern int   optind;
	int	     c, i;
	char	    *eargv;

	while ((c = getopt(argc, argv, "a:e:o:t:v:1:2:")) != EOF)
		switch (c) {
		case 'a':
			if (vflg) {
				(void)pfmt(stderr, MM_ERROR, excl);
				usage();
			}
			if (strcmp(optarg, "1") == 0)
				aflg |= 1;
			else if (strcmp(optarg, "2") == 0)
				aflg |= 2;
			else {
				(void)pfmt(stderr, MM_ERROR, badflno, optarg);
				usage();
			}
			break;
		case 'v':
			if (aflg) {
				(void)pfmt(stderr, MM_ERROR, excl);
				usage();
			}
			if (strcmp(optarg, "1") == 0)
				vflg |= 1;
			else if (strcmp(optarg, "2") == 0)
				vflg |= 2;
			else {
				(void)pfmt(stderr, MM_ERROR, badflno, optarg);
				usage();
			}
			break;
		case 'e':
			null = optarg;
			break;
		case 't':
			msep1 = (char_t *)optarg;
			if (wp._multibyte && NOTASCII(*msep1)) {
				if (ISSET2(*msep1))
					sepwidth = wp._eucw2;
				else
				if (ISSET3(*msep1))
					sepwidth = wp._eucw3;
				else
				if (*msep1 < 0240)
					sepwidth = 1;
				else
					sepwidth = wp._eucw1;
			}
			break;
		case 'o':
			eargv = optarg;
			for (no = 0; no < 2*NFLD; no++) {
			    if (eargv[0] == '\0')
				break;
			    if (((eargv[0] == '1' || eargv[0] =='2')
						&& eargv[1] == '.' )
						|| eargv[0] == '0') {
				if (eargv[0] == '1' && eargv[1] == '.') {
				    olistf[no] = F1;
				    olist[no] = strtol(&eargv[2],&eargv,10);
				} else
				if (eargv[0] == '2' && eargv[1] == '.') {
				    olistf[no] = F2;
				    olist[no] = strtol(&eargv[2],&eargv,10);
				} else if (strtol(&eargv[0],&eargv,10) == 0L) {
				   olistf[no] = Fany;
				} else {
				    (void)pfmt(stderr, MM_ERROR, badoptarg, c);
				    usage();
				}
				if (olistf[no] != Fany && olist[no] <= 0) {
				    (void)pfmt(stderr, MM_ERROR, badoptarg, c);
				    usage();
				}
				if (eargv[0] == ',') {
				    eargv++;
				    continue;
				} else
				if (isblank(eargv[0])) {
				    do {
					eargv++;
				    } while (isblank(eargv[0]));
				    if (eargv[0])
					continue;
				}
				else
				if (eargv[0] != '\0') {
				    (void)pfmt(stderr, MM_ERROR, badoptarg, c);
				    usage();
				}
			    } else {
				(void)pfmt(stderr, MM_ERROR, badoptarg, c);
				usage();
			    }
			}
			if (no == 0) {
				(void)pfmt(stderr, MM_ERROR, needsarg, c);
				usage();
			}
			break;
		case '1':
			j1 = strtol(optarg, &optarg, 10);
			if (j1 <= 0 || *optarg != 0) {
				(void)pfmt(stderr, MM_ERROR, badoptarg, c);
				usage();
			}
			break;
		case '2':
			j2 = strtol(optarg, &optarg, 10);
			if (j2 <= 0 || *optarg != 0) {
				(void)pfmt(stderr, MM_ERROR, badoptarg, c);
				usage();
			}
			break;
		case '?':
			usage();
		}

	for (i = 0; i < no; i++)
		olist[i]--;	/* 0 origin */
	if (argc - optind != 2) {
		(void)pfmt(stderr, MM_ERROR, ":2:Incorrect usage\n");
		usage();
	}
	j1--;
	j2--;	/* everyone else believes in 0 origin */
	if (strcmp(argv[optind], "-") == 0)
		f[F1] = stdin;
	else if ((f[F1] = fopen(argv[optind], "r")) == NULL)
		error(badopen, argv[optind], strerror(errno));
	if (strcmp(argv[optind+1], "-") == 0)
		f[F2] = stdin;
	else if ((f[F2] = fopen(argv[optind+1], "r")) == NULL)
		error(badopen, argv[optind+1], strerror(errno));
}

void
usage()
{
	(void)pfmt(stderr, MM_ACTION, usage1);
	(void)pfmt(stderr, MM_NOSTD, usage2);
	exit(1);
}
