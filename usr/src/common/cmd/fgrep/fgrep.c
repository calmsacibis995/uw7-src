/*	copyright	"%c%"	*/

#ident	"@(#)fgrep:fgrep.c	1.13.2.9"
/*
 * fgrep -- print all lines containing any of a set of keywords
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 */

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <limits.h> /* for LINE_MAX */
#include <wchar.h>

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif
#if BUFSIZ >= LINE_MAX
#undef BUFSIZ
#define BUFSIZ LINE_MAX/2
#endif

#include	<sys/euc.h>

eucwidth_t WW;		/* only need _multibyte flag */
#define Wgetwidth()	getwidth(&WW)

#ifndef _SYS_TYPES_H
typedef	unsigned char	uchar_t;
#endif

#define ISONEBYTE(ch)	((ch) < 0x80 || !WW._multibyte)

 /* same() reworked to ignore case in wide characters, too: ul95-03316 */
#define TOLOWER(ch) (isupper(ch) ? _tolower(ch) : (ch))
#define TOSAME(pat, src) (WW._multibyte \
	? towlower(pat) == towlower(src) : TOLOWER(pat) == TOLOWER(src))
#define same(pat, src) ((pat) == (src) || (iflag && TOSAME(pat, src)))

#define	BLKSIZE 512
#define	MAXSIZ 6000
#define QSIZE 400
struct words {
	wchar_t	inp;
	char	out;
	struct	words *nst;
	struct	words *link;
	struct	words *fail;
} w[MAXSIZ], *smax, *q;

FILE *fptr;
long	lnum;
int	bflag, cflag, lflag, fflag, nflag, vflag, xflag, eflag;
int	hflag, iflag;
int	Fflag;
int	sflag;
int	qflag;
int	retcode = 0;
int	nfile;
long	blkno;
int	nsucc;
long	tln;
extern	char *optarg;
extern	int optind;
extern 	char *gettxt();
void exit();

static struct list {
	uchar_t		*str;
	struct list	*next;
} *flst, *slst;

static void
regpat(str, isfile)
	uchar_t *str;
	int isfile;
{
	struct list *lp;

	if ((lp = (struct list *)malloc(sizeof(struct list))) == 0)
		overflo();
	lp->str = str;
	if (isfile)
	{
		lp->next = flst;
		flst = lp;
	}
	else
	{
		lp->next = slst;
		slst = lp;
	}
}

static char badopen[] = ":92:Cannot open %s: %s\n";

static const char usage[] = ":958:Usage:\n\
\t[-c|-l] [-bhinvx] patterns [file ...]\n\
\t[-c|-l] [-bhinvx] -e patterns ... [-f file ...] [file ...]\n\
\t[-c|-l] [-bhinvx] -f file ... [-e patterns ...] [file ...]\n";

#define STDINFID ":823"
#define STDINFNA "(standard input)"

main(argc, argv)
char **argv;
{
	register c;
	int errflg = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:fgrep");

	while(( c = getopt(argc, argv, "Fsqhybcie:f:lnvx")) != EOF)
		switch(c) {

		case 'F':
			Fflag++;
			continue;
		case 's':
			sflag++;
			continue;
		case 'q':
			qflag++;
			continue;
		case 'h':
			hflag++;
			continue;
		case 'b':
			bflag++;
			continue;

		case 'i':
		case 'y':
			iflag++;
			continue;

		case 'c':
			cflag++;
			continue;

		case 'e':
			eflag++;
			regpat((uchar_t *)optarg, 0);
			continue;

		case 'f':
			fflag++;
			regpat((uchar_t *)optarg, 1);
			continue;

		case 'l':
			lflag++;
			continue;

		case 'n':
			nflag++;
			continue;

		case 'v':
			vflag++;
			continue;

		case 'x':
			xflag++;
			continue;

		case '?':
			errflg++;
	}

	argc -= optind;
	if ((sflag | qflag) && !Fflag) {
  		pfmt(stderr, MM_ERROR,
			"uxlibc:1:Illegal option -- %c\n", sflag ? 's':'q');
			errflg++;
	}
	if ((cflag && lflag)
		|| errflg || ((argc <= 0) && !fflag && !eflag)) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, usage);
		exit(2);
	}
	if ( !eflag  && !fflag ) {
		regpat((uchar_t *)argv[optind], 0);
		optind++;
		argc--;
	}
	Wgetwidth();
	cgotofn();
	cfail();
	nfile = argc;
	argv = &argv[optind];
	if (argc<=0) {

		execute((char *)NULL);
	}
	else
		while ( --argc >= 0 ) {
			execute(*argv);
			argv++;
		}
	exit(retcode != 0 ? retcode : nsucc == 0);
}

execute(file)
char *file;
{
	register unsigned char *p;
	register struct words *c;
	register ccount;
	unsigned char buf[LINE_MAX];
	int failed;
	unsigned char *nlp;
	wchar_t lc, ch;
	int cw;
	static const char *pref_s, *pref_d, *pref_ld;
	char *filename  = file;

	if (file && strcmp(file, "-") != 0) {
		if ((fptr = fopen(file, "r")) == NULL) {
			if (!sflag)
				pfmt(stderr, MM_ERROR,
					badopen, file, strerror(errno));
			retcode = 2;
			return;
		}
	}
	else {
		fptr = stdin;
		filename = gettxt(STDINFID, STDINFNA);
	}
	ccount = 0;
	failed = 0;
	lnum = 1;
	tln = 0;
	blkno = 0;
	p = buf;
	nlp = p;
	c = w;
	for (;;) {
		if (ccount <= 0)
		{
			ccount = 0;
			if (p == &buf[LINE_MAX])
				p = buf;
		more1:;
			if ((cw = fread(p, sizeof(char),
				p > &buf[BUFSIZ] ? &buf[LINE_MAX] - p
				: BUFSIZ, fptr)) <= 0)
			{
				break;
			}
			if (ccount != 0)
				p = buf;
			ccount += cw;
			blkno += cw;
		}
		if (--ccount, !ISONEBYTE(lc = *p++))
		{
			ccount++;
			p--;
			if ((cw = mbtowc(&ch, (char *)p, (size_t)ccount)) >= 0)
			{
				ccount -= cw;
				p += cw;
				lc = ch;
			}
			else if (ccount < MB_LEN_MAX)
			{
				memmove(buf, p, ccount);
				p = &buf[ccount];
				goto more1;
			}
			else /* bad sequence */
			{
				ccount--;
				p++;
			}
		}
		nstate:
			if (same(c->inp,lc)) {
				c = c->nst;
			}
			else if (c->link != 0) {
				c = c->link;
				goto nstate;
			}
			else {
				c = c->fail;
				failed = 1;
				if (c==0) {
					c = w;
					istate:
					if (same(c->inp,lc)) {
						c = c->nst;
					}
					else if (c->link != 0) {
						c = c->link;
						goto istate;
					}
				}
				else goto nstate;
			}
		if (c->out) {
			while (lc != '\n')
			{
				if (ccount <= 0)
				{
					ccount = 0;
					if (p == &buf[LINE_MAX])
						p = buf;
				more2:;
					if ((cw = fread(p, sizeof(char),
						p > &buf[BUFSIZ]
						? &buf[LINE_MAX] - p
						: BUFSIZ, fptr)) <= 0)
					{
						break;
					}
					if (ccount != 0)
						p = buf;
					ccount += cw;
					blkno += cw;
				}
				if (--ccount, !ISONEBYTE(lc = *p++))
				{
					ccount++;
					p--;
					if ((cw = mbtowc(&ch, (char *)p,
						(size_t)ccount)) >= 0)
					{
						ccount -= cw;
						p += cw;
						lc = ch;
					}
					else if (ccount < MB_LEN_MAX)
					{
						memmove(buf, p, ccount);
						p = &buf[ccount];
						goto more2;
					}
					else /* bad sequence */
					{
						ccount--;
						p++;
					}
				}
			}
			if ( (vflag && (failed == 0 || xflag == 0)) || (vflag == 0 && xflag && failed) )
				goto nomatch;
	succeed:	nsucc = 1;
			if (qflag)
				exit(0);
			if (cflag) tln++;
			else if (lflag) {
				printf("%s\n", filename);
				if (fptr != stdin)
					fclose(fptr);
				return;
			}
			else {
				if (nfile > 1 && !hflag)
					printf(pref_s ? pref_s :
						(pref_s = gettxt(":188", "%s:")),
						filename);
				if (bflag)
					printf(pref_d ? pref_d :
						(pref_d = gettxt(":192", "%d:")),
						(blkno-(long)(ccount-1))/BLKSIZE);
				if (nflag)
					printf(pref_ld ? pref_ld :
						(pref_ld = gettxt(":189", "%ld:")),
						lnum);
				if (p <= nlp) {
					while (nlp < &buf[LINE_MAX]) putchar(*nlp++);
					nlp = buf;
				}
				while (nlp < p) putchar(*nlp++);
			}
	nomatch:	lnum++;
			nlp = p;
			c = w;
			failed = 0;
			continue;
		}
		if (lc == '\n')
			if (vflag) goto succeed;
			else {
				lnum++;
				nlp = p;
				c = w;
				failed = 0;
			}
	}
out:;
	if (fptr != stdin)
		fclose(fptr);
	if (cflag) {
		if ((nfile > 1) && !hflag)
			printf(pref_s ? pref_s : (pref_s = gettxt(":188", "%s:")),
				filename);
		printf("%ld\n", tln);
	}
}

wint_t
getargc()
{
	static struct list dead;
	static FILE *fp;
	static uchar_t *sp;
	static wint_t wc = '\n';
	wchar_t ch;
	int n;

	while (flst != 0 || fp != 0) /* for each -f file */
	{
		wint_t nwc;

		if (fp == 0)	/* first time for this file */
		{
			if ((fp = fopen((char *)flst->str, "r")) == 0)
			{
				pfmt(stderr, MM_ERROR, badopen, flst->str,
					strerror(errno));
				exit(2);
			}
			flst = flst->next;
			if (wc != '\n')	/* need to add separator */
				return wc = '\n';
		}
		if ((nwc = fgetwc(fp)) != EOF)
			return wc = nwc;
		fclose(fp);
		fp = 0;
	}
	while (slst != 0 || sp != 0) /* for each -e string */
	{
		if (sp == 0)	/* first time for this string */
		{
			sp = slst->str;
			slst = slst->next;
			if (wc != '\n')	/* need to add separator */
				return wc = '\n';
		}
		if ((wc = *sp++) != '\0')
		{
			if (!ISONEBYTE(wc) &&
				(n = mbtowc(&ch, &sp[-1], (size_t)MB_LEN_MAX))
				> 0)
			{
				sp += n - 1;
				wc = ch;
			}
			return wc;
		}
		sp = 0;
	}
	if (wc != '\n')	/* add final terminator */
		return wc = '\n';
	return EOF;
}

cgotofn() {
	register wint_t c;
	register struct words *s;

	s = smax = w;
nword:	for(;;) {
		c = getargc();
		if (c==EOF)
			return;
		if (c == '\n') {
			if (xflag) {
				for(;;) {
					if (s->inp == c) {
						s = s->nst;
						break;
					}
					if (s->inp == 0) goto nenter;
					if (s->link == 0) {
						if (smax >= &w[MAXSIZ -1]) overflo();
						s->link = ++smax;
						s = smax;
						goto nenter;
					}
					s = s->link;
				}
			}
			s->out = 1;
			s = w;
		} else {
		loop:	if (s->inp == c) {
				s = s->nst;
				continue;
			}
			if (s->inp == 0) goto enter;
			if (s->link == 0) {
				if (smax >= &w[MAXSIZ - 1]) overflo();
				s->link = ++smax;
				s = smax;
				goto enter;
			}
			s = s->link;
			goto loop;
		}
	}

	enter:
	do {
		s->inp = c;
		if (smax >= &w[MAXSIZ - 1]) overflo();
		s->nst = ++smax;
		s = smax;
	} while ((c = getargc()) != '\n' && c!=EOF);
	if (xflag) {
	nenter:	s->inp = '\n';
		if (smax >= &w[MAXSIZ -1]) overflo();
		s->nst = ++smax;
	}
	smax->out = 1;
	s = w;
	if (c != EOF)
		goto nword;
}

overflo() {
	pfmt(stderr, MM_ERROR, ":193:Wordlist too large\n");
	exit(2);
}
cfail() {
	struct words *queue[QSIZE];
	struct words **front, **rear;
	struct words *state, *st;
	register unsigned long c;
	register struct words *s;
	s = w;
	front = rear = queue;
init:	if ((s->inp) != 0) {
		*rear++ = s->nst;
		if (rear >= &queue[QSIZE - 1]) overflo();
	}
	if ((s = s->link) != 0) {
		goto init;
	}

	while (rear!=front) {
		s = *front;
		if (front == &queue[QSIZE-1])
			front = queue;
		else front++;
	cloop:	if ((c = s->inp) != 0) {
			*rear = (q = s->nst);
			if (front < rear)
				if (rear >= &queue[QSIZE-1])
					if (front == queue) overflo();
					else rear = queue;
				else rear++;
			else
				if (++rear == front) overflo();
			state = st = s->fail;
		floop:	if (state == 0) state = w;
			if (state->inp == c) {
			qloop:	q->fail = state->nst;
				if ((state->nst)->out == 1) q->out = 1;
				if ((q = q->link) != 0) goto qloop;
			}
			else if ((state = state->link) != 0)
				goto floop;
			else if (st != 0) {
				state = st = st->fail;
				goto floop;
			}
		}
		if ((s = s->link) != 0)
			goto cloop;
	}
}
