#ident	"@(#)sort:sort.c	1.22.3.5"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <values.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <sys/euc.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <ulimit.h>

#define SIZE	256		/* size of character conversion table */
#define	N	16
#define	C	20
#define NF	10
#define MTHRESH	 8 /* threshhold for doing median of 3 qksort selection */
#define TREEZ	32 /* no less than N and best if power of 2 */
#define MONTH_ABBREV_LENGTH	12  /* maximum bytes in month abbreviation */

#ifndef LINE_MAX
#define LINE_MAX	2048
#endif

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
 */

#ifndef	MAXMEM
#define	MAXMEM	1048576	/* Megabyte maximum */
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

#define	blank(c) ((c)==' ' || (c)=='\t')

FILE	*os;
char	*dirtry[] = {"/var/tmp", "/tmp", NULL};
char	**dirs;
char	file1[PATH_MAX];
char	*file = file1;
char	*filep;
#define NAMEOHD 12 /* sizeof("/stm00000aa") */
int	nfiles;
int	*lspace;
int	*maxbrk;
unsigned tryfor;
long longtryfor;
unsigned alloc;
char bufin[BUFSIZ], bufout[BUFSIZ];	/* Use setbuf's to avoid malloc calls.
					** malloc seems to get heartburn
					** when brk returns storage.
					*/
int	maxrec;
int	cmp(), cmpa();
int	(*compare)() = cmpa;
int	(*strcompare)() = strcmp;
unsigned char	*eol();
void	term();
int 	mflg;
int	nway;
int	cflg;
int	uflg;
char	*outfil;
int unsafeout;	/*kludge to assure -m -o works*/
unsigned char	*tabchar = (unsigned char*)"";
int 	eargc;
char	**eargv;
struct btree {
    char *rp;
    int  rn;
} tree[TREEZ], *treep[TREEZ];
int	blkcnt[TREEZ];
char	**blkcur[TREEZ];
long	wasfirst = 0, notfirst = 0;
int	bonus;

struct	field {
	int fcmp;
	int rflg;
	int dflg;
	int fflg;
	int iflg;
	int bflg[2];
	int m[2];
	int n[2];
}	*fields;
struct field proto = {
	ASC,
	1,
	0,
	0,
	0,
	0,0,
	0,-1,
	0,0
};
int	nfields = 0;
int 	error = 2;
char	*setfil();

unsigned grow_core();
int	cmpsave(), rline(), month(), number(), field();
void	copyproto(), initree(), kfield(), safeoutfil(),
	sort(), rderror(), checksort(), newfile(), merge(),
	oldfile(), cant(), msort(), wterror(), wline(), qksort(),
	insert(), cline(), month_init();

/* number of spaces in each field in months[] == MONTH_ABBREV_LENGTH */
char	*months[12] = {
	"            ", "            ", "            ",
	"            ", "            ", "            ",
	"            ", "            ", "            ",
	"            ", "            ", "            "};

struct	tm	ct = {
	0, 0, 0, 0, 0, 0, 0, 0, 0};

static const char USE[] = ":8:Incorrect usage\n";
static const char USAGE[] =":1192:\n\tsort [-m] [-o output] [-bdfiMnru] [-t x] [-y kmem] [-z recsz]\\\n\t\t[-k keydef] ... [file ...]\n";
static const char USAGE1[] =":1193:\tsort -c [-bdfiMnru] [-t x] [-y kmem] [-z recsz]\\\n\t\t[-k keydef] ... [file]\n";
static const char USAGE2[] =":1194:\tsort [-m] [-o output] [-bdfiMnru] [-t x] [-y kmem] [-z recsz]\\\n\t\t[+pos1 [-pos2]] ... [file ...]\n";
static const char USAGE3[] =":1195:\tsort -c [-bdfiMnru] [-t x] [-y kmem] [-z recsz]\\\n\t\t[+pos1 [-pos2]] ... [file]\n";
static const char nonl[] = ":566:Missing NEWLINE added at end of file\n";
static const char badcreate[] = ":148:Cannot create %s: %s\n";
static const char linetoolong[] = ":469:Line too long\n";
static const char misnl[] =
		":574:Missing NEWLINE added at end of input file %s\n";
static const char badchmod[] = ":840:Cannot change mode for file %s: %s\n";
static const char badkeydef[] =
		":1196:Invalid keydef argument\n";
static const char misoptarg[] =
		":1197:The option -%c requires argument\n";
static const char notnumeric[] =
		":1198:Key field does not begin with digit.\n";
static const char invmod[] =
		":1199:Invalid modifier - %c\n";

static char posix_var[] = "POSIX2";
static int posix;

eucwidth_t	wp;
int	tbcw;

#define MERGE	0
#define SORT	1

char *thsp;		/* thousands separator string */
int   thsplen;		/* thousands separator string length */
char *decp;		/* decimal point string */
int   decplen;		/* decimal point string length */

char lbufa[LINE_MAX];
char lbufb[LINE_MAX];

int
my_strcoll(char *a, char *b)
{
	int res;
	if((res = strcoll(a,b)) == 0 && !uflg)
		res = strcmp(a,b);
	return(res);
}

main(argc, argv)
char **argv;
{
	register a;
	char *arg;
	struct field *p, *q;
	int i, nf, arginc;
	unsigned char *s;
	struct lconv *lc;
	char *ls;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:sort");

	getwidth(&wp);

	if (getenv(posix_var) != 0) {
		posix = 1;
	} else {
		posix = 0;
	}

	lc = localeconv();
	thsp = lc->thousands_sep;
	thsplen = strlen(thsp);
	decp = lc->decimal_point;
	decplen = strlen(decp);

	ls = setlocale(LC_COLLATE, 0);
	if (strcmp(ls, "C") != 0 && strcmp(ls, "POSIX") != 0)
		strcompare = my_strcoll;

	/* close any file descriptors that may have been */
	/* left open -- we may need them all		*/
	for (i = 3; i < 3 + N; i++)
		(void) close(i);

	/* use environment if specified */
	if (s = (unsigned char *)getenv("TMPDIR"))
		dirtry[0] = (char *)s;

	fields = (struct field *)malloc(NF*sizeof(struct field));
	nf = NF;
	copyproto();
	initree();
	eargv = argv;
	tryfor = DEFMEM;
	while (--argc > 0) {
		if(**++argv == '-') {
			arg = *argv;
			switch(*++arg) {
			case '\0':
				if(arg[-1] == '-')
					eargv[eargc++] = "-";
				break;

			case 'o':
				if (*(arg+1) != '\0')
					outfil = arg + 1;
				else
				if(--argc > 0)
					outfil = *++argv;
				else {
					pfmt(stderr, MM_ERROR,
						":567:Cannot identify output file\n");
					exit(2);
				}
				break;

			case 'T':
				if (--argc > 0) {
					if ((strlen(*++argv) + NAMEOHD) > sizeof(file1)) {
						pfmt(stderr, MM_ERROR,
							":568:Path name too long: %s\n",
							 *argv);
						exit(2);
					}
					else dirtry[0] = *argv;
				}
				break;

			default:
				if ((arginc = field(arg, nfields>0)) == 0)
					break;
				arg += arginc;

			case 't':
				if (*(arg+1) != '\0')
					s = (unsigned char *)arg + 1;
				else
				if (--argc > 0)
					s = (unsigned char *)*++argv;
				else {
					pfmt(stderr, MM_ERROR, USE);
					exit(2);
				}
				tbcw = mblen((char *)s, MB_CUR_MAX);
				tabchar = s;
				break;

			case 'k':
				if (*(arg+1) != '\0')
					s = (unsigned char *)arg + 1;
				else
				if (--argc > 0)
					s = (unsigned char *)*++argv;
				else {
					pfmt(stderr, MM_ERROR,
						    misoptarg, *arg);
					exit(2);
				}
				if (!isdigit(*s)) {
					pfmt(stderr, MM_ERROR,
							notnumeric);
					exit(2);
				}
				if (++nfields >= nf) {
					if ((fields =
					    (struct field *)realloc(fields,
					    (nf+NF)*sizeof(struct field)))
						== NULL) {
						pfmt(stderr, MM_ERROR,
						    ":569:Too many keys\n");
						exit(2);
					}
					nf += NF;
				}
				copyproto();
				kfield((char *)s);
				break;
			}
		} else if (**argv == '+') {
			if(++nfields >= nf) {
				if((fields =
					(struct field *)realloc(fields,
					(nf + NF) * sizeof(struct field)))
						== NULL) {
					pfmt(stderr, MM_ERROR,
						":569:Too many keys\n");
					exit(2);
				}
				nf += NF;
			}
			copyproto();
			field(++*argv, 0);
		} else
			eargv[eargc++] = *argv;
	}
	q = &fields[0];
	for(a=1; a<=nfields; a++) {
		p = &fields[a];
		if(p->fflg != proto.fflg) continue;
		if(p->dflg != proto.dflg) continue;
		if(p->iflg != proto.iflg) continue;
		if(p->fcmp != proto.fcmp) continue;
		if(p->rflg != proto.rflg) continue;
		if(p->bflg[0] != proto.bflg[0]) continue;
		if(p->bflg[1] != proto.bflg[1]) continue;
		p->fflg = q->fflg;
		p->dflg = q->dflg;
		p->iflg = q->iflg;
		p->fcmp = q->fcmp;
		p->rflg = q->rflg;
		p->bflg[0] = p->bflg[1] = q->bflg[0];
	}
	if(eargc == 0)
		eargv[eargc++] = "-";
	if(cflg && eargc>1) {
		pfmt(stderr, MM_ERROR, ":570:Can check only 1 file\n");
		exit(2);
	}

	safeoutfil();

	lspace = (int *) sbrk(0);
	maxbrk = (int *) ulimit(UL_GMEMLIM,0L);
	if (!mflg && !cflg)
		if ((alloc=grow_core(tryfor,(unsigned) 0)) == 0) {
			pfmt(stderr, MM_ERROR,
				":571:Out of memory before sort: %s\n",
				strerror(errno));
			exit(2);
		}

	a = -1;
	for(dirs=dirtry; *dirs; dirs++) {
		(void) sprintf(filep=file1, "%s/stm%.5uaa", *dirs,
						(unsigned) getpid());
		while (*filep)
			filep++;
		filep -= 2;
		if ( (a=creat(file, 0600)) >=0)
			break;
	}
	if(a < 0) {
		pfmt(stderr, MM_ERROR, ":572:Cannot locate temporary file: %s\n",
			strerror(errno));
		exit(2);
	}
	(void) close(a);
	(void) unlink(file);
	if (sigset(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) sigset(SIGHUP, term);
	if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
		(void) sigset(SIGINT, term);
	(void) sigset(SIGPIPE, term);
	if (sigset(SIGTERM, SIG_IGN) != SIG_IGN)
		(void) sigset(SIGTERM, term);
	nfiles = eargc;
	if(!mflg && !cflg) {
		sort();
		if (ferror(stdin))
			rderror(NULL);
		(void) fclose(stdin);
	}

	if (maxrec == 0)  maxrec = LINE_MAX;
	alloc = (N + 1) * maxrec + N * BUFSIZ;
	for (nway = N; nway >= 2; --nway) {
		if (alloc < (maxbrk - lspace) * sizeof(int *))
			break;
		alloc -= maxrec + BUFSIZ;
	}
	if (nway < 2 || brk((char *)lspace + alloc) != 0) {
		pfmt(stderr, MM_ERROR, ":573:Out of memory before merge: %s\n",
			strerror(errno));
		term();
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
	term();
	/*NOTREACHED*/
}

void
sort()
{
	register char *cp;
	register char **lp;
	FILE *iop;
	char *keep, *ekeep, **mp, **lmp, **ep;
	int n;
	int done, i, first;
	char *f;

	/*
	** Records are read in from the front of the buffer area.
	** Pointers to the records are allocated from the back of the buffer.
	** If a partially read record exhausts the buffer, it is saved and
	** then copied to the start of the buffer for processing with the
	** next coreload.
	*/
	first = 1;
	done = 0;
	keep = 0;
	i = 0;
	ep = (char **) (((char *) lspace) + alloc);
	if ((f=setfil(i++)) == NULL) /* open first file */
		iop = stdin;
	else if ((iop=fopen(f,"r")) == NULL)
		cant(f);
	setbuf(iop,bufin);
	do {
		lp = ep - 1;
		cp = (char *) lspace;
		*lp-- = cp;
		if (keep != 0) /* move record from previous coreload */
			for(; keep < ekeep; *cp++ = *keep++);

		while ((char *)lp - cp > 1) {
			if (fgets(cp,(char *) lp - cp, iop) == NULL)
				n = 0;
			else
				n = strlen(cp);
			if (n == 0) {
				if (ferror(iop))
					rderror(f);

				if (keep != 0 )
					/* The kept record was at
					   the EOF.  Let the code
					   below handle it.       */;
				else
				if (i < eargc) {
					(void) fclose(iop);
					if ((f=setfil(i++)) == NULL)
						iop = stdin;
					else if ((iop=fopen(f,"r")) == NULL )
						cant(f);
					setbuf(iop,bufin);
					continue;
				}
				else {
					done++;
					break;
				}
			}
			cp += n - 1;
			if ( *cp == '\n') {
				*cp = '\0';
				cp += 2;
				if ( cp - *(lp+1) > maxrec )
					maxrec = cp - *(lp+1);
				*lp-- = cp;
				keep = 0;
			}
			else
			if ( cp + 2 < (char *) lp ) {
				/* the last record of the input */
				/* file is missing a NEWLINE    */
				if(f == NULL)
					pfmt(stderr, MM_WARNING, nonl);
				else
					pfmt(stderr, MM_WARNING, misnl, f);
				*++cp = '\0';
				*lp-- = ++cp;
				keep = 0;
			}
			else {  /* the buffer is full */
				keep = *(lp+1);
				ekeep = ++cp;
			}

			if ((char *)lp - cp <= 2 && first == 1) {

				/* full buffer */
				tryfor = alloc;
				tryfor = grow_core(tryfor,alloc);
				if (tryfor == 0)
					/* could not grow */
					first = 0;
				else { /* move pointers */
					lmp = ep + 
					   (tryfor/sizeof(char **) - 1);
					for ( mp = ep - 1; mp > lp;)
						*lmp-- = *mp--;
					ep += tryfor/sizeof(char **);
					lp += tryfor/sizeof(char **);
					alloc += tryfor;
				}
			}
		}
		if (keep != 0 && *(lp+1) == (char *) lspace) {
			pfmt(stderr, MM_ERROR, ":575:Record too large\n");
			term();
		}
		first = 0;
		lp += 2;
		if(done == 0 || nfiles != eargc)
			newfile();
		else
			oldfile();
		setbuf(os, bufout);
		msort(lp, ep);
		if (ferror(os))
			wterror(SORT);
		(void) fclose(os);
	} while(done == 0);
}

void
msort(a, b)
char **a, **b;
{
	register struct btree **tp;
	register int i, j, n;
	char *save;

	i = (b - a);
	if (i < 1)
		return;
	else if (i == 1) {
		wline(*a);
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
		wline((*tp)->rp);
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

void
insert(tp, n)
struct btree **tp;
int n;
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

void
merge(a, b)
{
	FILE *tfile[N];
	char *buffer = (char *) lspace;
	register int nf;		/* number of merge files */
	register struct btree **tp;
	register int i, j;
	char	*f;
	char	*save, *iobuf;

	save = (char *) lspace + (nway * maxrec);
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
		wline((*tp)->rp);
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
		wterror(MERGE);
	(void) fclose(os);
}

void
cline(tp, fp)
register char *tp, *fp;
{
	while ((*tp++ = *fp++) != '\0');
}

int
rline(iop, s)
register FILE *iop;
register char *s;
{
	register int n;

	if (fgets(s,maxrec,iop) == NULL )
		n = 0;
	else
		n = strlen(s);
	if ( n == 0 )
		return(1);
	s += n - 1;
	if ( *s == '\n' ) {
		*s = '\0';
		return(0);
	} else
	if ( n < maxrec - 1) {
		pfmt(stderr, MM_WARNING, nonl);
		*++s = '\0';
		return(0);
	}
	else {
		pfmt(stderr, MM_ERROR, linetoolong);
		term();
		/*NOTREACHED*/
	}
}

void
wline(s)
char *s;
{
	(void) fputs(s,os);
	(void) putc('\n',os);
	if(ferror(os))
		wterror(SORT);

}

void
checksort()
{
	char *f;
	char *lines[2];
	register int i, j, r;
	register char **s;
	register FILE *iop;

	s = &(lines[0]);
	f = setfil(0);
	if (f == 0)
		iop = stdin;
	else if ((iop = fopen(f, "r")) == NULL)
		cant(f);
	setbuf(iop, bufin);

	i = 0;   j = 1;
	s[0] = (char *) lspace;
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
		if (r < 0) {
			error = 1;
			if (!posix) {
				pfmt(stderr, MM_INFO,
					":576:disorder: %s\n", s[j]);
			}
			term();
			/*NOTREACHED*/
		}
		if (r == 0 && uflg) {
			error = 1;
			if (!posix) {
				pfmt(stderr, MM_INFO,
					":577:non-unique: %s\n", s[j]);
			}
			term();
			/*NOTREACHED*/
		}
		r = i;  i = j; j = r;
	}
	if (ferror(iop))
		rderror(f);
	(void) fclose(iop);
	exit(0);
}

void
newfile()
{
	register char *f;

	f = setfil(nfiles);
	if((os=fopen(f, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badcreate, f, strerror(errno));
		term();
	}
	if (chmod(f, S_IRUSR|S_IWUSR) == -1) {
		pfmt(stderr, MM_ERROR, badchmod, f, strerror(errno));
		term();
	}
	nfiles++;
}

char *
setfil(i)
register int i;
{
	if(i < eargc)
		if(eargv[i][0] == '-' && eargv[i][1] == '\0')
			return(0);
		else
			return(eargv[i]);
	i -= eargc;
	filep[0] = i/26 + 'a';
	filep[1] = i%26 + 'a';
	return(file);
}

void
oldfile()
{
	if(outfil) {
		if((os=fopen(outfil, "w")) == NULL) {
			pfmt(stderr, MM_ERROR, badcreate, outfil, strerror(errno));
			term();
		}
	} else
		os = stdout;
}

void
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

void
cant(f)
char *f;
{
	pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n", f, strerror(errno));
	term();
}

void
term()
{
	register i;

	if(nfiles == eargc)
		nfiles++;
	for(i=eargc; i<=nfiles; i++) {	/*<= in case of interrupt*/
		(void) unlink(setfil(i));	/*with nfiles not updated*/
	}
	exit(error);
}

cmp(i, j)
char *i, *j;
{
	register unsigned char *pa, *pb;
	register int sa;
	int sb;
	unsigned char *skip();
	int a, b;
	int k;
	int length;
	unsigned char *la, *lb;
	unsigned char *ipa, *ipb, *jpa, *jpb;
	unsigned char *tpa, *tpb;
	struct field *fp;
	for(k = nfields>0; k<=nfields; k++) {
		fp = &fields[k];
		tpa = (unsigned char *)lbufa;
		tpb = (unsigned char *)lbufb;
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
			if (fp->fcmp == NUM) {
				pa = skip(pa, fp, 0);
				pb = skip(pb, fp, 0);
			}
		}
		if(fp->fcmp==NUM) {
			/* skip leading blanks */
			while (blank(*pa))  pa++;
			while (blank(*pb))  pb++;

			sa = sb = fp->rflg;
			if(*pa == '-') {
				pa++;
				sa = -sa;
			}
			if(*pb == '-') {
				pb++;
				sb = -sb;
			}
			for(ipa = pa; ipa<la; ) {
				if (isdigit(*ipa))
					ipa++;
				else
				if (*thsp && strncmp(thsp,(char *)ipa,
						    thsplen) == 0)
					ipa += thsplen;
				else
					break;
			}
			for(ipb = pb; ipb<lb; ) {
				if (isdigit(*ipb))
					ipb++;
				else
				if (*thsp && strncmp(thsp,(char *)ipb,
						    thsplen) == 0)
					ipb += thsplen;
				else
					break;
			}
			jpa = ipa;
			jpb = ipb;
			a = 0;
			if(sa==sb)
				while(ipa > pa && ipb > pb) {
					if (!isdigit(ipa[-1])) {
						ipa -= thsplen;
						continue;
					}
					if (!isdigit(ipb[-1])) {
						ipb -= thsplen;
						continue;
					}
					if(b = *--ipb - *--ipa)
						a = b;
				}
			while(ipa > pa)
				if(*--ipa != '0')
					return(-sa);
			while(ipb > pb)
				if(*--ipb != '0')
					return(sb);
			if(a) return(a*sa);
			if(strncmp((char *)(pa=jpa), decp,
						    decplen) == 0)
				pa += decplen;
			if(strncmp((char *)(pb=jpb), decp,
						    decplen) == 0)
				pb += decplen;
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
		} else if(fp->fcmp==MON)  {
			/* skip leading blanks */
			while (blank(*pa))  pa++;
			while (blank(*pb))  pb++;

			sa = fp->rflg*(month(pb)-month(pa));
			if(sa)
				return(sa);
			else
				continue;
		}

		for ( ; pa < la; ) {
		    length = mblen((char *)pa, MB_CUR_MAX);
		    if (length == 1) {
/* the following code could be simplified -- see the multibye code below */
		    	if (fp->dflg) {
				if (!(isalnum(*pa)||blank(*pa)||*pa=='\0')) {
					pa++;
					continue;
				}
				if (fp->fflg)
					*tpa++ = islower(*pa)?_toupper(*pa):*pa;
				else
					*tpa++ = *pa;
		    	} else if (fp->iflg) {
				if (!isprint(*pa)) {
					pa++;
					continue;
				}
				if (fp->fflg)
					*tpa++ = islower(*pa)?_toupper(*pa):*pa;
				else
					*tpa++ = *pa;
		    	} else {
				if (fp->fflg)
					*tpa++ = islower(*pa)?_toupper(*pa):*pa;
				else
					*tpa++ = *pa;
		    	}
		    } else {  /* multibyte character */
			if (fp->dflg) {
				/* last 2 checks on next line unnecessary! */
				if (!(iswalnum(*pa)||blank(*pa)||*pa=='\0')) {
		    			pa += length;
					continue;
				}
			} else if (fp->iflg) {
				if (!iswprint(*pa)) {
		    			pa += length;
					continue;
				}
			} 
			if (fp->fflg) {
				*tpa = towupper(*pa);
			} else {
				*tpa = *pa;
			}
/* next line assumes uppercase width == lowercase width */
			tpa += length;
		    }
		    pa += length;
		}
		*tpa = 0;
		for ( ; pb < lb; ) {
		    length = mblen((char *)pb, MB_CUR_MAX);
		    if (length == 1) {
/* the following code could be simplified -- see the multibye code below */
		    	if (fp->dflg) {
				if (!(isalnum(*pb)||blank(*pb)||*pb=='\0')) {
					pb++;
					continue;
				}
				if (fp->fflg)
					*tpb++ = islower(*pb)?_toupper(*pb):*pb;
				else
					*tpb++ = *pb;
		    	} else if (fp->iflg) {
				if (!isprint(*pb)) {
					pb++;
					continue;
				}
				if (fp->fflg)
					*tpb++ = islower(*pb)?_toupper(*pb):*pb;
				else
					*tpb++ = *pb;
		    	} else {
				if (fp->fflg)
					*tpb++ = islower(*pb)?_toupper(*pb):*pb;
				else
					*tpb++ = *pb;
		    	}
		    } else {  /* multibyte character */
			if (fp->dflg) {
				/* last 2 checks on next line unnecessary! */
				if (!(iswalnum(*pb)||blank(*pb)||*pb=='\0')) {
					pb += length;
					continue;
				}
			} else if (fp->iflg) {
				if (!iswprint(*pb)) {
					pb += length;
					continue;
				}
			} 
			if (fp->fflg) {
				*tpb = towupper(*pb);
			} else {
				*tpb = *pb;
			}
/* next line assumes uppercase width == lowercase width */
			tpb += length;
		    }
		    pb += length;
		}
		*tpb = 0;

		if ((sa = (*strcompare)(lbufb, lbufa)) == 0)
			continue;
		return(sa*fp->rflg);
	}
	if(uflg)
		return(0);
	return(cmpa(i, j));
}

cmpa(pa, pb)
register char *pa, *pb;
{
	int sa;

	sa = (*strcompare)(pb, pa);
	if(sa == 0) return(0);
	
	return (sa > 0 ? fields[0].rflg : -fields[0].rflg);
}

unsigned char *
skip(p, fp, j)
struct field *fp;
register unsigned char *p;
{
	register i;
	register unsigned char *tbc;
	int contflag;
	int ew;
	int k;

	if( (i=fp->m[j]) < 0)
		return(eol(p));
	if (*tabchar) {
		while (--i >= 0) {
			do {
				contflag = 0;
				tbc = tabchar;
				ew = mblen((char *)p, MB_CUR_MAX);
				if (*p != *tbc)
					if(*p != '\0') {
						p += ew;
						contflag = 1;
					} else goto ret;
				else {
					for (k=1; k < tbcw; k++)
						if (*++p != *++tbc) {
							contflag = 1;
							p += (ew - k);
							break;
						}
				}
			} while (contflag);
			if (i > 0 || j == 0)
				p++;
			if (j == 1)
				p -= (tbcw -1);
		}
	} else while (--i >= 0) {
			while(blank(*p))
				p++;
			while(!blank(*p))
				if(*p != '\0')
					p++;
				else goto ret;
		}
	if(fp->bflg[j]) {
		if (j == 1 && fp->m[j] > 0)
			p++;
		while(blank(*p))
			p++;
	}
	i = fp->n[j];
	while((i > 0) && (*p != '\0')) {
		ew = mblen((char *)p, MB_CUR_MAX);
		p += ew;
		i--;
	}
ret:
	return(p);
}

unsigned char *
eol(p)
register unsigned char *p;
{
	while(*p != '\0') p++;
	return(p);
}

void
copyproto()
{
	register i;
	register int *p, *q;

	p = (int *)&proto;
	q = (int *)&fields[nfields];
	for(i=0; i<sizeof(proto)/sizeof(*p); i++)
		*q++ = *p++;
}

void
initree()
{
	register struct btree **tpp, *tp;
	register int i;

	for (tp = &(tree[0]), tpp = &(treep[0]), i = TREEZ; --i >= 0;)
	    *tpp++ = tp++;
}

int
cmpsave(n)
register int n;
{
	register int award;

	if (n < 2) return (0);
	for (n++, award = 0; (n >>= 1) > 0; award++);
	return (award);
}

int
field(s, k)
char *s;
{
	register struct field *p;
	register d;
	char *base = s;

	p = &fields[nfields];
	d = 0;
	for(; *s!=0; s++) {
		switch(*s) {
		case '\0':
			return 0;

		case 'b':
			p->bflg[k]++;
			break;

		case 'd':
			p->dflg = 1;
			p->iflg = 0;
			break;

		case 'f':
			p->fflg = 1;
			break;

		case 'i':
			p->iflg = 1;
			p->dflg = 0;
			break;

		case 'c':
			cflg = 1;
			continue;

		case 'm':
			mflg = 1;
			continue;

		case 'M':
			month_init();
			p->fcmp = MON;
			break;

		case 'n':
			p->fcmp = NUM;
			break;

		case 'r':
			p->rflg = -1;
			continue;

		case 'u':
			uflg = 1;
			continue;

		case 'y':
			if ( *++s )
				if (isdigit(*s))

					tryfor = number(&s) * 1024;

				else {
					pfmt(stderr, MM_ERROR, USE);
					pfmt(stderr, MM_ACTION, USAGE);
					pfmt(stderr, MM_NOSTD, USAGE1);
					pfmt(stderr, MM_NOSTD, USAGE2);
					pfmt(stderr, MM_NOSTD, USAGE3);
					exit(2);
				}
			else {
				--s;
				tryfor = MAXMEM;
			}
			continue;

		case 'z':
			if ( *++s && isdigit(*s))
				maxrec = number(&s);
			else {
				pfmt(stderr, MM_ERROR, USE);
				pfmt(stderr, MM_ACTION, USAGE);
				pfmt(stderr, MM_NOSTD, USAGE1);
				pfmt(stderr, MM_NOSTD, USAGE2);
				pfmt(stderr, MM_NOSTD, USAGE3);
				exit(2);
			}
			continue;

		case '.':
			if(p->m[k] == -1)	/* -m.n with m missing */
				p->m[k] = 0;
			d = &fields[0].n[0]-&fields[0].m[0];
			if (*++s == '\0') {
				p->m[k+d] = 0;
			}
			--s;
			continue;

		case 't':
			return (s - base);

		default:
			if (isdigit(*s))
				p->m[k+d] = number(&s);
			else {
				pfmt(stderr, MM_ERROR, USE);
				pfmt(stderr, MM_ACTION, USAGE);
				pfmt(stderr, MM_NOSTD, USAGE1);
				pfmt(stderr, MM_NOSTD, USAGE2);
				pfmt(stderr, MM_NOSTD, USAGE3);
				exit(2);
			}
		}
		compare = cmp;
	}
	return 0;
}

void
kfield(s)
char *s;
{
	register struct field *p;
	register d;
	register k = 0;

	compare = cmp;
	p = &fields[nfields];
	d = 0;
	for(; *s!=0; s++) {
		switch(*s) {
		case '\0':
			goto adjust;

		case 'b':
			p->bflg[k]++;
			break;

		case 'd':
			p->dflg = 1;
			p->iflg = 0;
			break;

		case 'f':
			p->fflg = 1;
			break;

		case 'i':
			p->iflg = 1;
			p->dflg = 0;
			break;

		case 'M':
			month_init();
			p->fcmp = MON;
			break;

		case 'n':
			p->fcmp = NUM;
			break;

		case 'r':
			p->rflg = -1;
			break;

		case ',':
			if(k!=0) {
				pfmt(stderr, MM_ERROR, badkeydef);
				exit(2);
			}
			k = 1;
			d = 0;
			continue;

		case '.':
			if(p->m[k] == -1) { /* -m.n with m missing */
				pfmt(stderr, MM_ERROR, badkeydef);
				exit(2);
			}
			if (d) {
				pfmt(stderr, MM_ERROR, badkeydef);
				exit(2);
			}
			d = &fields[0].n[0]-&fields[0].m[0];
			if (*++s == '\0') {
				--s;
				if (k)		/* end_field */
					p->m[k+d] = 0;
				else
					p->m[k+d] = 1;
				continue;
			}

		default:
			if (isdigit(*s)) {
				p->m[k+d] = number(&s);
				if ((k==0 || d==0) && p->m[k+d] == 0) {
					pfmt(stderr, MM_ERROR, badkeydef);
					exit(2);
				}
			} else {
				pfmt(stderr, MM_ERROR, invmod, *s);
				exit(2);
			}
		}
	}
    adjust:
	/* adjust to the obsolescent syntax */
	p->m[0]--;
	p->n[0]--;
	if (p->n[1])
		p->m[1]--;
}

int
number(ppa)
register char **ppa;
{
	int n;
	n = (int) strtoul(*ppa, ppa, 10);
	*ppa = *ppa - 1;
	return (n);
}

#define qsexc(p,q) t= *p;*p= *q;*q=t
#define qstexc(p,q,r) t= *p;*p= *r;*r= *q;*q=t

void
qksort(a, l)
char **a, **l;
{
	register char **i, **j;
	register char **lp, **hp;
	char **k;
	int c, delta;
	char *t;
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

void
month_init()
{
	char	time_buf[20];
	wchar_t pwc1, pwc2;
	int	i, j, k;

	for(i=0; i<12; i++) {
		ct.tm_mon = i;
		(void) strftime(time_buf, 20, "%b", &ct);
		for (j=0; j<MONTH_ABBREV_LENGTH;) {
			/* get width of next multibyte character */
			k = mblen((char *)&time_buf[j], MONTH_ABBREV_LENGTH);

			/* make sure next char non-null and fits in the array */
			if ((k == 0) || ((k+j) > MONTH_ABBREV_LENGTH)) {
				if (j < MONTH_ABBREV_LENGTH)
					months[i][j] = NULL;
				break;
			}
			/*
			 * the next 3 instructions convert the multibyte 
			 * abbreviation to upper case
			 */
			(void) mbtowc(&pwc1, (char *)&time_buf[j], 
						MONTH_ABBREV_LENGTH);
			pwc2 = towupper(pwc1);
			(void) wctomb((char *)&months[i][j], pwc2);
		
			/* next assumes uppercase width == lowercase width */
			j+=k;  
		}
	}
}

int
month(s)
unsigned char *s;
{
	register unsigned char *t, *u;
	register i;
	register unsigned char c;
	wchar_t	input[MONTH_ABBREV_LENGTH], INPUT[MONTH_ABBREV_LENGTH];
	wchar_t ref[MONTH_ABBREV_LENGTH];
	int	inputlength, reflength;

	/* convert string to wide, find its length, and make upper case */
	inputlength = mbstowcs(input, (char *)s, MONTH_ABBREV_LENGTH);
	for(i=0; i<inputlength; i++) {
		INPUT[i] = towupper(input[i]);
	}

	/* compare to upper case abbreviations till a match is found */
	for(i=0; i<12; i++) {
		reflength = mbstowcs(ref, months[i], MONTH_ABBREV_LENGTH);
		if (inputlength < reflength) {
			/* field not long enough to match abbreviation */
			continue;
		}
		if (wcsncmp(INPUT, ref, reflength) == 0) {
			/* got a match! */
			return(i);
		}
	}
	return(-1);
}

void
rderror(s)
char *s;
{
	if (s)
		pfmt(stderr, MM_ERROR, ":341:Read error in %s: %s\n", s,
			strerror(errno));
	else
		pfmt(stderr, MM_ERROR, ":578:Read error on stdin: %s\n",
			strerror(errno));
	term();
}

void
wterror(s)
int s;
{
	switch(s){
	case SORT:
		pfmt(stderr, MM_ERROR, ":579:Write error while sorting: %s\n",
			strerror(errno));
		break;
	case MERGE:
		pfmt(stderr, MM_ERROR, ":580:Write error while merging: %s\n",
			strerror(errno));
		break;
	}
	term();
}

unsigned
grow_core(size,cursize)
	unsigned size, cursize;
{
	unsigned newsize;
	/*The variable below and its associated code was written so this would work on */
	/*pdp11s.  It works on the vax & 3b20 also. */
	u_long longnewsize;

	longnewsize = (u_long) size + (u_long) cursize;
	if (longnewsize < MINMEM)
		longnewsize = MINMEM;
	else
	if (longnewsize > MAXMEM)
		longnewsize = MAXMEM;
	newsize = (unsigned) longnewsize;
	for (; ((char *)lspace+newsize) <= (char *)lspace; newsize >>= 1);
	if (longnewsize > (u_long) (maxbrk - lspace) * (u_long) sizeof(int *))
		newsize = (maxbrk - lspace) * sizeof(int *);
	if (newsize <= cursize)
		return(0);
	if ( brk((char *) lspace + newsize) != 0)
		return(0);
	return(newsize - cursize);
}

