#ident	"@(#)pr:pr.c	1.19.3.11"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	PR command (print files in pages and columns, with headings)
 *	2+head+2+page[56]+5
 */

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/euc.h>
#include <getwidth.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <widec.h>
#include <wchar.h>

/*
 * This is done to prevent redefinition of our own definition of ERR.
 */
#undef ERR

#define ESC	L'\033'
#define LENGTH	66
#define LINEW	72
#define NUMW	5
#define MARGIN	10
#define DEFTAB	8
#define NFILES	10

#define STDINNAME()	nulls
#define PROMPT()	(void) putc('\7', stderr) /* BEL */
#define NOFILE	nulls
#define TABS(N,C)	if ((N = intopt(argv, &C)) <= 0) N = DEFTAB
#define ETABS	(Inpos % Etabn)
#define ITABS	(Itabn > 0 && Nspace >= (nc = Itabn - Outpos % Itabn))
#define NSEPC	L'\t'
#define PR_HEAD()	(void)pfmt(stdout, MM_NOSTD, ":347:%s  %s Page %d\n\n\n", date, head, Page)
#define done()	if (Ttyout) (void) chmod(Ttyout, Mode)
#define	SCRWIDTH(c)	(iscodeset0(c)?(isprint(c)?1:0):	\
			 (iscodeset1(c)?wp._scrw1:		\
			 (iscodeset2(c)?wp._scrw2:		\
			 (iscodeset3(c)?wp._scrw3:1))))

#define FORMAT	"%b %e %H:%M %Y"	/* ---date time format--- 
					   b -- abbreviated month name 
					   e -- day of month
					   H -- Hour (24 hour version)
					   M -- Minute
					   Y -- Year in the form ccyy */
#define FORMATID ":348"

typedef wchar_t CHAR;
typedef int ANY;
typedef unsigned UNS;
typedef struct { FILE *f_f; char *f_name; int f_nextc; } FILS;
typedef struct {int fold; int skip; int eof; } foldinf;

ANY *getspace();

FILS *Files;
FILE *fopen(), *mustopen();

mode_t Mode;
int Multi = 0, Nfiles = 0, Error = 0;
void onintr();

int  findopt(), print(), atoix(), intopt(),
	get(), put(), Fread(), readbuf();
void die(), errprint(), putpage(), nexbuf(), putspace(), err_report(),
	foldpage(), unget(), foldbuf(), balance(), cerror(), usage();

char nulls[] = "";
char *Ttyout, obuf[BUFSIZ];

static char	time_buf[50];	/* array to hold the time and date */

static char posix_var[] = "POSIX2";
static int posix;

static int stdin_done = 0;

eucwidth_t	wp;

#define GETC(f) ((!wp._multibyte)?(wchar_t)getc(f):getwc(f))
#define UNGETC(c,f) ((!wp._multibyte)?ungetc((int)c,f):ungetwc(c,f))
#define PUTCHAR(c) ((!wp._multibyte)?putchar((int)c):putwchar(c))

void
fixtty()
{
	struct stat sbuf;

	setbuf(stdout, obuf);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, onintr);
	if (Ttyout= ttyname(fileno(stdout))) {		/* is stdout a tty? */
		(void) stat(Ttyout, &sbuf);
		Mode = sbuf.st_mode;		/* save permissions */
		(void) chmod(Ttyout, (S_IREAD|S_IWRITE));
	}
	return;
}

char *
GETDATE() /* return date file was last modified */
{
	static char *now = NULL;
	static struct stat sbuf, nbuf;

	if (Nfiles > 1 || Files->f_name == nulls) {
		if (now == NULL)
			{
			(void) time(&nbuf.st_mtime);
			(void) cftime(time_buf, gettxt(FORMATID, FORMAT),
				&nbuf.st_mtime);
			now = time_buf;
			}
		return (now);
	} else {
		(void) stat(Files->f_name, &sbuf);
		(void) cftime(time_buf, gettxt(FORMATID, FORMAT), &sbuf.st_mtime);
		return (time_buf);
	}
}


main(argc, argv) char *argv[];
{
	FILS fstr[NFILES];
	int nfdone = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:pr");

	if (getenv(posix_var) != 0) {
		posix = 1;
	} else {
		posix = 0;
	}

	(void)iswalpha(128);

	getwidth(&wp);
	wp._eucw2++;
	wp._eucw3++;

	Files = fstr;
	for (argc = findopt(argc, argv); argc > 0; --argc, ++argv)
		if (Multi == 'm') {
			if (Nfiles >= NFILES - 1) die(":349:Too many files\n");
			if (mustopen(*argv, &Files[Nfiles++]) == NULL)
				++nfdone; /* suppress printing */
		} else if (strcmp(*argv, "-") == 0)	{
			if (stdin_done) {
				err_report(*argv, -2);
			} else 
				(void) print(NOFILE);
			++nfdone;
			++stdin_done;
		} else {
			if (print(*argv))
				(void) fclose(Files->f_f);
			++nfdone;
		}
	if (!nfdone) /* no files named, use stdin */
		(void) print(NOFILE); /* on GCOS, use current file, if any */
	errprint(); /* print accumulated error reports */
	exit(Error);	/*NOTREACHED*/
}


long Lnumb = 0;
FILE *Ttyin = stdin;
int Dblspace = 1, Fpage = 1, Formfeed = 0,
	Length = LENGTH, Linew = 0, Offset = 0, Ncols = 1, Pause = 0,
	Colw, Plength, Margin = MARGIN, Numw, Report = 1,
	Etabn = 0, Itabn = 0, fold = 0,
	foldcol = 0, alleof=0;
CHAR Sepc = 0, Nsepc = NSEPC, Etabc = L'\t', Itabc = L'\t';
char *Head = NULL;
CHAR *Buffer = NULL, *Bufend, *Bufptr;
UNS Buflen;
typedef struct { CHAR *c_ptr, *c_ptr0; long c_lno; int c_skip; } *COLP;
COLP Colpts;
foldinf *Fcol;
/* findopt() returns eargc modified to be    */
/* the number of explicitly supplied         */
/* filenames, including '-', the explicit    */
/* request to use stdin.  eargc == 0 implies */
/* that no filenames were supplied and       */
/* stdin should be used.		     */
int
findopt(argc, argv) char *argv[];
{
	char **eargv = argv;
	int eargc = 0, c;
	int mflg = 0, aflg = 0, i;
	int optflg;
	int optend = 0;
	fixtty();
	while (--argc > 0) {
		optflg = 0;
		c = **++argv;
		if (!optend)
		    switch (c) {
		    case '-':
			if ((c = *++*argv) == '\0') break;
			else if (c == '-' && *(*argv+1) == '\0')	{
				optend++;
				argc--;
				argv++;
				break;
			}
		    case '+':
			do {
				if (isdigit(c)) {
					--*argv;
					Ncols = atoix(argv, 1);
					}
				else
					switch (c) {
					case '+': if ((*argv)[1] == '\0' ||
						    (Fpage = atoix(argv, 1)) < 1)
							Fpage = 1;
						  continue;
					case 'd': Dblspace = 2;
						  continue;
					case 'e': TABS(Etabn, Etabc);
						  continue;
					case 'F': if (posix) {
							++Formfeed;
						  } else {
							fold++;
						  }
						  continue;
					case 'h': if ((*argv)[1] != '\0') {
						    Head = ++*argv;
						    while ((*argv)[1])
							++*argv;
					          } else
						  if (--argc > 0) {
						    Head = argv[1];
						    optflg++;
						  }
						  else {
						    (void)pfmt(stderr, MM_ERROR,
						       ":1:Incorrect usage\n");
						    usage();
						  }
						  continue;
					case 'i': TABS(Itabn, Itabc);
						  continue;
					case 'l': if ((*argv)[1] != '\0')
						    Length = atoix(argv, 1);
						  else
						  if (--argc > 0) {
						    Length =
						      atoix(argv+1, 0);
						    optflg++;
						  }
						  else {
						    (void)pfmt(stderr, MM_ERROR,
						       ":1:Incorrect usage\n");
						    usage();
						  }
						  continue;
					case 'a': aflg++;
						  Multi = c;
						  continue;
					case 'm': mflg++; 
						  Multi = c; 
						  continue;
					case 'o': if ((*argv)[1] != '\0')
						    Offset = atoix(argv, 1);
						  else
						  if (--argc > 0) {
						    Offset =
						      atoix(argv+1, 0);
						    optflg++;
						  }
						  else {
						    (void)pfmt(stderr, MM_ERROR,
						       ":1:Incorrect usage\n");
						    usage();
						  }
						  continue;
					case 'p': ++Pause;
						  continue;
					case 'r': Report = 0;
						  continue;
					case 's': if ((c=mbtowc(&Sepc,
						    (*argv)+1,MB_LEN_MAX))
								!= '\0')
						    *argv += c;
						  else
						    Sepc = L'\t';
						  continue;
					case 't': Margin = 0;
						  continue;
					case 'w': if ((*argv)[1] != '\0')
						    Linew = atoix(argv, 1);
						  else
						  if (--argc > 0) {
						    Linew =
						      atoix(argv+1, 0);
						    optflg++;
						  }
						  else {
						    (void)pfmt(stderr, MM_ERROR,
						       ":1:Incorrect usage\n");
						    usage();
						  }
						  continue;
					case 'n': ++Lnumb;
						  if ((Numw =
						    intopt(argv,&Nsepc))<=0)
							Numw = NUMW;
						  continue;
					case 'f': ++Formfeed;
						  continue;
					default : 
						  (void)pfmt(stderr, MM_ERROR,
					  "uxlibc:1:Illegal option -- %c\n", c);
						  usage();
					}
				/* Advance over options    */
				/* until null is found.    */
				/* Allows clumping options */
				/* as in "pr -pm f1 f2".   */
			    } while ((c = *++*argv) != '\0');

			    if (optflg) ++argv;
			    continue;
		    }
		*eargv++ = *argv;
		++eargc;	/* count the filenames */
	}

	if (mflg && (Ncols > 1)) {
		(void)pfmt(stderr, MM_ERROR,
			":350:Only one of either -m or -column allowed\n");
		usage();
	}
	if (aflg && (Ncols < 2)) {
		(void)pfmt(stderr, MM_ERROR,
			":351:-a valid only with -column\n");
		usage();
	}

	if (Ncols == 1 && fold)
		Multi = 'm';

	if (Length == 0) Length = LENGTH;
	if (Length <= Margin) Margin = 0;
	Plength = Length - Margin/2;
	if (Multi == 'm') Ncols = eargc;
	switch (Ncols) {
	case 0:
		Ncols = 1;
	case 1:
		break;
	default:
		if (Multi != 'm') {  /* do not set Etabn and Itabn for -m */
			if (Etabn == 0) /* respect explicit tab specification */
				Etabn = DEFTAB;
			if (Itabn == 0)
				Itabn = DEFTAB;
		}
	}
	if ((Fcol = (foldinf *)malloc(sizeof(foldinf) * Ncols)) == NULL) {
		(void)pfmt(stderr,MM_ERROR, ":31:Out of memory: %s\n",
				strerror(errno));
		exit(1);
	}
	for ( i=0; i<Ncols; i++)
		Fcol[i].fold = Fcol[i].skip = 0;
	if (Linew == 0) Linew = Ncols != 1 && Sepc == 0 ? LINEW : 512;
	if (Lnumb) {
		int numw;

		if (Nsepc == L'\t') {
			if(Itabn == 0)
					numw = Numw + DEFTAB - (Numw % DEFTAB);
			else
					numw = Numw + Itabn - (Numw % Itabn);
		}else {
			if (!wp._multibyte)
				numw = Numw + (isprint(Nsepc) != 0);
			else
				numw = Numw + SCRWIDTH(Nsepc);
		}
		Linew -= (Multi == 'm') ? numw : numw * Ncols;
	}
	if ((Colw = (Linew - Ncols + 1)/Ncols) < 1)
		die(":352:Width too small\n");
	if (Ncols != 1 && Multi == 0) {
		Buflen = ((UNS)(Plength/Dblspace + 1))*(Linew+1);
		Buffer = (CHAR *)getspace(Buflen*sizeof(CHAR));
		Bufptr = Bufend = &Buffer[Buflen];
		Colpts = (COLP)getspace((UNS)((Ncols+1)*sizeof(*Colpts)));
		Colpts[0].c_lno = 0;
	}
						/* is stdin not a tty? */
	if (Ttyout && (Pause || Formfeed) && !ttyname(fileno(stdin)))
		Ttyin = fopen("/dev/tty", "r");
	return (eargc);
}

int
intopt(argv, optp) char *argv[]; wchar_t *optp;
{
	int c;

	if ((c = (*argv)[1]) != '\0' && !isdigit(c)) {
		++*argv;
		c = mbtowc(optp, *argv, MB_LEN_MAX);
		*argv += c - 1;
	}
	if ((*argv)[1] != '\0')
		return(atoix(argv, 1));
	else
		return(-1);
}

int Page, Nspace, Inpos;
CHAR C = L'\0';

int
print(name) char *name;
{
	static int notfirst = 0;
	char *date = NULL, *head = NULL;
	int c;

	if (Multi != 'm' && mustopen(name, &Files[0]) == NULL) return (0);
	if (Multi == 'm' && Nfiles == 0 && mustopen(name, &Files[0]) == NULL)
		die(":353:Cannot open stdin\n");
	if (Buffer) (void) UNGETC(Files->f_nextc, Files->f_f);
	if (Lnumb) Lnumb = 1;
	for (Page = 0; ; putpage()) {
		if (C==(CHAR)EOF && !(fold && Buffer)) break;
		if (Buffer) nexbuf();
		Inpos = 0;
		if (get(0) == (CHAR)EOF) break;
		(void) fflush(stdout);
		if (++Page >= Fpage) {
			if (Ttyout && (Pause || Formfeed && !notfirst++)) {
				PROMPT(); /* prompt with bell and pause */
				while ((c = getc(Ttyin)) != EOF && c != '\n') ;
			}
			if (Margin == 0) continue;
			if (date == NULL) date = GETDATE();
			if (head == NULL) head = Head != NULL ? Head :
				Nfiles < 2 ? Files->f_name : nulls;
			(void) printf("\n\n");
			Nspace = Offset;
			putspace();
			PR_HEAD();
		}
	}
	C = L'\0';
	return (1);
}

int Outpos, Lcolpos, Pcolpos, Line;

void
putpage()
{
	register int colno;

	if (fold) {
		foldpage();
		return;
	}
	for (Line = Margin/2; ; (void)get(0)) {
		for (Nspace = Offset, colno = 0, Outpos = 0; C != L'\f'; ) {
			if (Lnumb && C != (CHAR)EOF
			&& ((colno == 0 && Multi == 'm') || Multi != 'm')) {
				if (Page >= Fpage) {
					putspace();
					(void) printf("%*ld%C", Numw, Buffer ?
					    Colpts[colno].c_lno++ : Lnumb, Nsepc);
					Outpos += Numw;
					if (iswprint(Nsepc))
					    Outpos += SCRWIDTH(Nsepc);
					else if (Nsepc == L'\t')
					    Outpos = Outpos + DEFTAB
						- (Outpos % DEFTAB);
				}
				++Lnumb;
			}
			for (Lcolpos = 0, Pcolpos = 0;
				C != L'\n' && C != L'\f' && C != (CHAR)EOF;
							(void)get(colno))
					(void) put(C);
			if (C == (CHAR)EOF || ++colno == Ncols ||
				C == L'\n' && get(colno) == (CHAR)EOF) break;
			if (Sepc) {
				putspace();
				(void) PUTCHAR(Sepc);
				if (iswprint(Sepc))
					Outpos += SCRWIDTH(Sepc);
				else if (Sepc == L'\t')
					Outpos = Outpos + DEFTAB
						    - (Outpos % DEFTAB);
			}
			else if ((Nspace += Colw - Lcolpos + 1) < 1) Nspace = 1;
		}
		if (C == (CHAR)EOF) {
			if (Margin != 0) break;
			if (colno != 0) (void) put(L'\n');
			return;
		}
		if (C == L'\f') break;
		(void) put(L'\n');
		if (Dblspace == 2 && Line < Plength) (void) put(L'\n');
		if (Line >= Plength) break;
	}
	if (Formfeed) (void) put(L'\f');
	else while (Line < Length) (void) put(L'\n');
}

void
foldpage()
{
	register int colno;
	CHAR keep;
	int i;
	static int  sl;

	for (Line = Margin/2; ; (void)get(0)) {
		for (Nspace = Offset, colno = 0, Outpos = 0; C != L'\f'; ) {
			if (Lnumb && Multi == 'm' && foldcol) {
				if (!Fcol[colno].skip) {
					unget(colno);
					putspace();
					if (!colno) {
					    for(i=0;i<=Numw;i++) (void) printf(" ");
					    (void) printf("%C",Nsepc);
					    Outpos += Numw;
					    if (iswprint(Nsepc))
						Outpos += SCRWIDTH(Nsepc);
					    else if (Nsepc == L'\t')
						Outpos = Outpos + DEFTAB
						    - (Outpos % DEFTAB);
					}
					for(i=0;i<=Colw;i++) (void) printf(" ");
					Outpos += Colw;
					(void) put(Sepc);
					if (++colno == Ncols) break;
					(void) get(colno);
					continue;
				}
				else if (!colno) Lnumb = sl;
			}

			if (Lnumb && (C != (CHAR)EOF)
			&& ((colno == 0 && Multi == 'm') || Multi != 'm')) {
				if (Page >= Fpage) {
					putspace();
					if ((foldcol &&
					Fcol[colno].skip && Multi!='a') ||
					(Fcol[0].fold && Multi == 'a') ||
					(Buffer && Colpts[colno].c_skip)) {
						for (i=0; i<Numw;i++) (void) printf(" ");
						(void) printf("%C",Nsepc);
						if (Buffer) {
							Colpts[colno].c_lno++;
							Colpts[colno].c_skip = 0;
						}
					}
					else
					    (void) printf("%*ld%C", Numw,
						Buffer ?
						    Colpts[colno].c_lno++
						    : Lnumb, Nsepc);
					Outpos += Numw;
					if (iswprint(Nsepc))
					    Outpos += SCRWIDTH(Nsepc);
					else if (Nsepc == L'\t')
					    Outpos = Outpos + DEFTAB
						    - (Outpos % DEFTAB);
				}
				sl = Lnumb++;
			}
			for (Lcolpos = 0, Pcolpos = 0;
				C != L'\n' && C != L'\f' && C != (CHAR)EOF;
							(void)get(colno))
					if (put(C)) {
					    unget(colno);
					    Fcol[(Multi=='a')?0:colno].fold = 1;
					    break;
					}
					else if (Multi == 'a') {
					    Fcol[0].fold = 0;
					}
			if (Buffer) {
				alleof = 1;
				for (i=0; i<Ncols; i++)
					if (!Fcol[i].eof) alleof = 0;
				if (alleof || ++colno == Ncols)
					break;
			} else if (C == (CHAR)EOF || ++colno == Ncols)
				break;
			keep = C;
			(void) get(colno);
			if (keep == L'\n' && C == (CHAR)EOF)
				break;
			if (Sepc) (void) put(Sepc);
			else if ((Nspace += Colw - Lcolpos + 1) < 1) Nspace = 1;
		}
		foldcol = 0;
		if (Lnumb && Multi != 'a') {
			for (i=0; i<Ncols; i++) {
				if (Fcol[i].skip = Fcol[i].fold)
					foldcol++;
				Fcol[i].fold = 0;
			}
		}
		if (C == (CHAR)EOF) {
			if (Margin != 0) break;
			if (colno != 0) (void) put(L'\n');
			return;
		}
		if (C == L'\f') break;
		(void) put(L'\n');
		(void) fflush(stdout);
		if (Dblspace == 2 && Line < Plength) (void) put(L'\n');
		if (Line >= Plength) break;
	}
	if (Formfeed) (void) put(L'\f');
	else while (Line < Length) (void) put(L'\n');
}

void
nexbuf()
{
	register CHAR *s = Buffer;
	register COLP p = Colpts;
	int j, bline = 0;
	CHAR c;
	int bufcolw;

	bufcolw = Colw;

	if (fold) {
		foldbuf();
		return;
	}
	for ( ; ; ) {
		p->c_ptr0 = p->c_ptr = s;
		if (p == &Colpts[Ncols]) return;
		(p++)->c_lno = Lnumb + bline;
		for (j = (Length - Margin)/Dblspace; --j >= 0; ++bline)
			for (Inpos = 0; ; ) {
				if ((c = GETC(Files->f_f)) == (CHAR)EOF) {
					for (*s = (CHAR)EOF;
					    p <= &Colpts[Ncols]; ++p)
						p->c_ptr0 = p->c_ptr = s;
					balance(bline);
					return;
				}
				if (!wp._multibyte)
					Inpos += (isprint(c) != 0);
				else if (iswprint(c))
					Inpos += SCRWIDTH(c);
				if (Inpos <= bufcolw || c == L'\n') {
					*s = c;
					if (++s >= Bufend)
						die(":354:Page-buffer overflow\n");
				}
				if (c == L'\n') break;
				switch (c) {
				case L'\b': if (Inpos == 0) --s;
				case ESC:  if (Inpos > 0) --Inpos;
				}
			}
	}
}

void
foldbuf()
{
	int num, i, colno=0;
	int size = Buflen;
	CHAR *s, *d;
	register COLP p=Colpts;

	for (i =0; i< Ncols; i++)
		Fcol[i].eof = 0;
	d = Buffer;
	if (Bufptr != Bufend) {
		s = Bufptr;
		while (s < Bufend) *d++ = *s++;
		size -= (Bufend - Bufptr);
	}
	Bufptr = Buffer;
	p->c_ptr0 = p->c_ptr = Buffer;
	if (p->c_lno == 0) {
		p->c_lno = Lnumb;
		p->c_skip = 0; 
	}
	else {
		p->c_lno = Colpts[Ncols-1].c_lno;
		if (p->c_skip = Colpts[Ncols].c_skip)
			p->c_lno--;
	}
	if ((num = Fread(d, 1, (size_t)size, Files->f_f)) != size) {
		for (*(d+num) = (CHAR)EOF; (++p)<= &Colpts[Ncols]; )
			p->c_ptr0 = p->c_ptr = (d+num);
		balance(0);
		return;
	}
	i = (Length - Margin) / Dblspace;
	do {
		(void) readbuf(&Bufptr, i, p++);
	} while (++colno < Ncols);
}

int
Fread(ptr, size, nitems, stream)
CHAR *ptr;
size_t size;
size_t nitems;
FILE *stream;
{
	register CHAR *p = ptr, *pe = ptr + size * nitems;

	while (p < pe && (*p = GETC(stream)) != (CHAR)EOF) p++;
	return (p - ptr);
}

void
balance(bline) /* line balancing for last page */
{
	CHAR *s = Buffer;
	register COLP p = Colpts;
	int colno = 0, j, c, l;
	int lines;

	if (!fold) {
		c = bline % Ncols;
		l = (bline + Ncols - 1)/Ncols;
		bline = 0;
		do {
			for (j = 0; j < l; ++j)
				while (*s++ != L'\n') ;
			(++p)->c_lno = Lnumb + (bline += l);
			p->c_ptr0 = p->c_ptr = s;
			if (++colno == c) --l;
		} while (colno < Ncols - 1);
	} else {
		lines = readbuf(&s, 0, 0);
		l = (lines + Ncols - 1)/Ncols;
		if (l > ((Length - Margin) / Dblspace)) {
			l = (Length - Margin) / Dblspace;
			c = Ncols;
		} else
			c = lines % Ncols;
		s = Buffer;
		do {
			(void) readbuf(&s, l, p++);
			if (++colno == c) --l;
		} while (colno < Ncols);
		Bufptr = s;
	}
}

int
readbuf(s, lincol, p)
CHAR **s;
int lincol;
COLP p;
{
	int lines = 0;
	int chars = 0, width;
	int nls = 0, move;
	int skip = 0;
	int decr = 0;

	width = (Ncols == 1) ? Linew : Colw;
	while (**s != (CHAR)EOF) {
		switch (**s) {
			case L'\n': lines++; nls++; chars=0; skip = 0;
				  break;
			case L'\b':
			case ESC: if (chars) chars--;
				   break;
			case L'\t':
				move = Itabn - ((chars + Itabn) % Itabn);
				move = (move < width-chars) ? move : width-chars;
				chars += move;
			default:
				if (!wp._multibyte)
					chars += (isprint(**s) != 0);
				else
					chars += SCRWIDTH(**s);
		}
		if (chars > width) {
			lines++; skip++; decr++; chars = 0; 
		}
		if (lincol && lines == lincol) {
			(p+1)->c_lno = p->c_lno + nls;
			(++p)->c_skip = skip;
			if (**s == L'\n') (*s)++;
			p->c_ptr0 = p->c_ptr = (CHAR *)*s;
			return(lines);
		}
		if (decr) decr = 0;
		else (*s)++;
	}
	return(lines);
}

int
get(colno)
{
	static CHAR peekc = 0;
	register COLP p;
	register FILS *q;
	CHAR c;

	if (peekc)
		{ peekc = 0; c = Etabc; }
	else if (Buffer) {
		p = &Colpts[colno];
		if (p->c_ptr >= (p+1)->c_ptr0) c = (CHAR)EOF;
		else if ((c = *p->c_ptr) != (CHAR)EOF) ++p->c_ptr;
		if (fold && c == (CHAR)EOF) Fcol[colno].eof = 1;
	} else if ((c = 
		(q = &Files[Multi == 'a' ? 0 : colno])->f_nextc)
							== (CHAR)EOF) {
		for (q = &Files[Nfiles];
			--q >= Files && q->f_nextc == (CHAR)EOF; ) ;
		if (q >= Files) c = L'\n';
	} else
		q->f_nextc = GETC(q->f_f);
	if (Etabn != 0 && c == Etabc) {
		++Inpos;
		peekc = ETABS;
		c = L' ';
	} else if (!wp._multibyte && isprint(c))
			++Inpos;
	else if (iswprint(c))
			Inpos += SCRWIDTH(c);
	else
		switch (c) {
		case L'\b':
		case ESC:
			if (Inpos > 0) --Inpos;
			break;
		case L'\f':
			if (Ncols == 1) break;
			c = L'\n';
		case L'\n':
		case L'\r':
			Inpos = 0;
			break;
		default  :
			Inpos += SCRWIDTH(c);
		}
	if (c == (CHAR)EOF) {
		C = (CHAR)EOF;
		return((CHAR)EOF);
	}
	else
		return (C = c);
}

int space_cnt = 0;

int 
put(c)
CHAR c;
{
	int move=0;
	int width=Colw;
	int sp=Lcolpos;

	if (fold && Ncols == 1) width = Linew;
	switch (c) {
	case L' ':
		if ( Itabn > 0 && ++space_cnt == 1 
		  && (Outpos % Itabn) == (Itabn - 1)) {
		/* a single space at a tabstop should not be */
		/* converted to a tab -- treat as regular char */
			move = 1;
			break;
		}
		if((!fold && Ncols < 2) || (Lcolpos < width)) {
			++Nspace;
			++Lcolpos;
		}
		goto rettab;
	case L'\t':
		if(Itabn == 0)
			break;
		if(Lcolpos < width) {
			move = Itabn - ((Lcolpos + Itabn) % Itabn);
			move = (move < width-Lcolpos) ? move : width-Lcolpos;
			Nspace += move;
			Lcolpos += move;
		}
rettab:
		if (fold && sp == Lcolpos)
		if (Lcolpos >= width)
				return(1);
		return(0);	
	case L'\b':
		if (Lcolpos == 0) return(0);
		if (Nspace > 0) { --Nspace; --Lcolpos; return(0); }
		if (Lcolpos > Pcolpos) { --Lcolpos; return(0); }
	case ESC:
		move = -1;
		break;
	case L'\n':
		++Line;
	case L'\r':
	case L'\f':
		Pcolpos = 0; Lcolpos = 0; Nspace = 0; Outpos = 0;
	default:
		if (!wp._multibyte)
			move = (isprint(c) != 0);
		else
			move = iswprint(c) ? SCRWIDTH(c) : 0;
	}
	if (Page < Fpage) return(0);
	if (Lcolpos > 0 || move > 0) Lcolpos += move;
	putspace();
	if ((!fold && Ncols < 2) || (Lcolpos <= width)) {
		(void) PUTCHAR(c);
		Outpos += move;
		Pcolpos = Lcolpos;
	}
	else if (Lcolpos - move < width) {
		Nspace += (width - (Lcolpos - move));
		putspace();
	}
	if (fold && Lcolpos > width)
		return(1);

	return(0);
}

void
putspace()
{
	int nc = 0;

	for ( ; Nspace > 0; Outpos += nc, Nspace -= nc)
		if (ITABS && !fold)
			(void) PUTCHAR(Itabc);
		else {
			nc = 1;
			(void) PUTCHAR(L' ');
		}
	space_cnt = 0;
}

void
unget(colno)
int colno;
{
	if (Buffer) {
		if (*(Colpts[colno].c_ptr-1) != L'\t')
			--(Colpts[colno].c_ptr);
		if (Colpts[colno].c_lno) Colpts[colno].c_lno--;
	}
	else {
		if ((Multi == 'm' && colno == 0) || Multi != 'm')
			if (Lnumb && !foldcol) Lnumb--;
		colno = (Multi == 'a') ? 0 : colno;
		(void) UNGETC(Files[colno].f_nextc, Files[colno].f_f);
		Files[colno].f_nextc = C;
	}
}

int
atoix(p, incr) register char **p; int incr;
{
	unsigned long n;
	char *parg;

	if (incr)
		++*p;
	parg = *p;
	errno = 0;
	n = strtoul(*p, p, 10);
	if (errno) {
		(void)pfmt(stderr, MM_ERROR, ":37:%s: %s\n", strerror(errno), parg);
		usage();
	}
	else if (**p != '\0') {
		(void)pfmt(stderr, MM_ERROR, ":37:%s: %s\n",
			gettxt("uxsyserr:25", "Invalid argument"), parg);
		usage();
	}
	if (incr)
		--*p;
	return ((int)n);
}

/* Defer message about failure to open file to prevent messing up
   alignment of page with tear perforations or form markers.
   Treat empty file as special case and report as diagnostic.
*/
typedef struct err { struct err *e_nextp; char *e_name; int e_errno; } ERR;
ERR *Err = NULL, *Lasterr = (ERR *)&Err;

FILE *
mustopen(s, f) char *s; register FILS *f;
{
	int lasterrno;

	if (*s == '\0' || (strcmp(s, "-") == 0)) {
		f->f_name = STDINNAME();
		f->f_f = stdin;
		if (stdin_done)	{
			err_report(s, -2);
			return((FILE *)NULL);
		}
		stdin_done++;
	} else if ((f->f_f = fopen(f->f_name = s, "r")) == NULL) {
		lasterrno = errno;
		s = strcpy((char *)getspace((UNS)(strlen(f->f_name) + 1)), 
			f->f_name);
	}
	if (f->f_f != NULL) {
		if ((f->f_nextc = GETC(f->f_f)) != (CHAR)EOF
		    || Multi == 'm')
			return (f->f_f);
		lasterrno = -1;
		s = strcpy((char *)getspace((UNS)(strlen(f->f_name) + 1)), 
			f->f_name);
		(void) fclose(f->f_f);
	}
	err_report(s, lasterrno);
	return ((FILE *)NULL);
}

void
err_report(name, error)
char *name;
int error;
{
	Error++;
	if (Report)
		if (Ttyout) {	/* accumulate error reports */
			Lasterr = Lasterr->e_nextp =
				(ERR *)getspace((UNS)sizeof(ERR));
			Lasterr->e_nextp = NULL;
			Lasterr->e_name = name;
			Lasterr->e_errno = error;
		} else { /* ok to print error report now */
			cerror(name, error);
		}
}

ANY *
getspace(n) UNS n;
{
	ANY *t;

	if ((t = (ANY *)malloc(n)) == NULL)
		die(":31:Out of memory: %s\n", strerror(errno));
	return (t);
}

/* VARARGS1 */
void
die(s, a1, a2, a3) char *s, *a1, *a2, *a3;
{
	++Error;
	errprint();
	(void)pfmt(stderr, MM_ERROR, s, a1, a2, a3);
	exit(1);
}

void
onintr()
{
	++Error;
	errprint();
	_exit(1);
}

void
errprint() /* print accumulated error reports */
{
	(void) fflush(stdout);
	for ( ; Err != NULL; Err = Err->e_nextp)
		cerror(Err->e_name, Err->e_errno);
	done();
}
void
usage()
{
char *usage2 =
	"[+page] [-columns] [-adfFmprt] [-e[c][k]]";
char *usage3 =
	"[-h header] [-i[c][k]] [-l length] [-n[c][k]]";
char *usage4 =
	"[-o offset] [-s[separator]] [-w width] [file ...]";
char *usage2id = ":827";
char *usage3id = ":828";
char *usage4id = ":829";

	(void)pfmt(stderr, MM_ACTION,
		":830:Usage: pr %s\\\n\t\t\t %s\\\n\t\t\t %s\n",
		gettxt(usage2id, usage2), gettxt(usage3id, usage3),
		gettxt(usage4id, usage4));
	exit(1);
}

void
cerror(name, error)
char *name;
int error;
{
	if (error == -1)
		(void)pfmt(stderr, MM_ERROR, ":360:Empty file -- %s\n", name);
	else if (error == -2)
		(void)pfmt(stderr, MM_ERROR, ":1013:Cannot reuse stdin\n");
	else
		(void)pfmt(stderr, MM_ERROR, ":92:Cannot open %s: %s\n", name,
			strerror(error));
}
