/*	copyright	"%c%"	*/

#ident	"@(#)od:od.c	1.11.4.10"
/*
 * od -- octal (also hex, decimal, and character) dump
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <sys/euc.h>
#include <getwidth.h>
#include <limits.h>
#include <widec.h>

#define BSIZE	512	/* standard buffer size */
#define YES	1
#define NO	0
#define DBUFSIZE 16
#define CONVSIZE 32
#define	PAD_STR	" **"

#ifdef	lint
static int	ZERO;	/* To get rid of lint warning */
#else
#define	ZERO	0
#endif

#define	ROUNDWORD(num)						\
		(((num) + sizeof(word[0]) - 1) / sizeof(word[0]))

#define	PRINTLINE(num)						\
		do {						\
			if ((same >= 0) && !showall) {		\
				for (f=0; f<DBUFSIZE; f++)	\
					if (lastword[f] != word[f])  { \
						same = -1;	\
						break;		\
					}			\
				if (same == 0) {		\
					(void)printf("*\n");	\
					same = 1;		\
				}				\
			}					\
			if ((same == -1) || showall) {		\
				line(addr, word, ROUNDWORD((size_t)(num)), \
					&remainbytes);		\
				same = 0;			\
				for (f=0; f<DBUFSIZE; f++)	\
					lastword[f] = word[f];	\
				for (f=0; f<DBUFSIZE; f++)	\
					word[f] = 0;		\
			}					\
			addr += (num);				\
		} while (ZERO)

#define	ESCSEQ(c)					\
		switch((c)) {				\
		case '\0':				\
			(void)printf(" \\0");		\
			break;				\
		case '\a':				\
			(void)printf(" \\a");		\
			break;				\
		case '\b':				\
			(void)printf(" \\b");		\
			break;				\
		case '\f':				\
			(void)printf(" \\f");		\
			break;				\
		case '\n':				\
			(void)printf(" \\n");		\
			break;				\
		case '\r':				\
			(void)printf(" \\r");		\
			break;				\
		case '\t':				\
			(void)printf(" \\t");		\
			break;				\
		case '\v':				\
			(void)printf(" \\v");		\
			break;				\
		default:				\
			putn((unsigned long)(c), 8, 3);	\
		}

#define	OESCSEQ(c)					\
		switch((c)) {				\
		case '\0':				\
			(void)printf(" \\0");		\
			break;				\
		case '\b':				\
			(void)printf(" \\b");		\
			break;				\
		case '\f':				\
			(void)printf(" \\f");		\
			break;				\
		case '\n':				\
			(void)printf(" \\n");		\
			break;				\
		case '\r':				\
			(void)printf(" \\r");		\
			break;				\
		case '\t':				\
			(void)printf(" \\t");		\
			break;				\
		default:				\
			putn((unsigned long)(c), 8, 3);	\
		}

struct dfmt {
	char	df_option;	/* command line option */
	int	df_field;	/* # of chars in a field */
	int	df_size;	/* # of bytes displayed in a field */
	int	df_radix;	/* conversion radix */
} *conv_vec[CONVSIZE];

static int	wcput(char *, int), owcput(char *, int),
		putx(char *, struct dfmt *, int *);
static void	aput(int), cput(int), ocput(int), 
		putn(unsigned long, int, int),
		line(unsigned long, char *, size_t, int *),
		pre(int, int), copy_str(char *, char *, struct dfmt *);
static long	offset(char *);


/* The names of the structures are chosen for the options action
 *	e.g. u_s_oct - unsigned octal
 *	     u_l_hex - unsigned long hex
*/

struct dfmt	nchar	= {'a',  3, sizeof (char),	 8};
struct dfmt	cchar	= {'c',  3, sizeof (char),	 8};
struct dfmt	occhar	= {'C',  3, sizeof (char),	 8};
struct dfmt	u_b_oct	= {'b',  3, sizeof (char),	 8};
struct dfmt	u_b_dec	= {'y',  3, sizeof (char),	10};
struct dfmt	s_b_dec	= {'t',  4, sizeof (char),	10};
struct dfmt	u_b_hex	= {'e',  2, sizeof (char),	16};
struct dfmt	u_s_oct	= {'o',  6, sizeof (short),	 8};
struct dfmt	u_s_dec	= {'d',  5, sizeof (short),	10};
struct dfmt	s_s_dec	= {'s',  6, sizeof (short),	10};
struct dfmt	os_s_dec= {'s',  6, sizeof (short),	10};
struct dfmt	u_s_hex	= {'x',  4, sizeof (short),	16};
struct dfmt	u_l_oct	= {'O', 11, sizeof (long),	 8};
struct dfmt	u_l_dec	= {'D', 10, sizeof (long),	10};
struct dfmt	s_l_dec	= {'S', 11, sizeof (long),	10};
struct dfmt	u_l_hex	= {'X',  8, sizeof (long),	16};
struct dfmt	flt	= {'f', 14, sizeof (float),	10};
struct dfmt	dble	= {'F', 22, sizeof (double),	10};
struct dfmt	odble	= {'G', 22, sizeof (double),	10};

/* Table of format entries indexed by type specifier and
 *	number of bytes indicator
 */

#define	SDEC	0	/* signed decimal */
#define	UDEC	1	/* unsigned decimal */
#define	UOCT	2	/* unsigned octal */
#define UHEX	3	/* unsigned hexadecimal */
#define	FLOAT	4	/* float */
#define CHAR	0	/* char */
#define	SHORT	1	/* short */
#define	INT	2	/* int */
#define LONG	3	/* long */
struct dfmt	*templates[4][4] = {
	     /*	CHAR	  SHORT	    INT	      LONG */
/* SDEC */	&s_b_dec, &s_s_dec, &s_l_dec, &s_l_dec,
/* UDEC */	&u_b_dec, &u_s_dec, &u_l_dec, &u_l_dec,
/* UOCT */	&u_b_oct, &u_s_oct, &u_l_oct, &u_l_oct,
/* UHEX */	&u_b_hex, &u_s_hex, &u_l_hex, &u_l_hex
};

/* Translation data */

char *ascii_values[128] =
{
"nul", "soh", "stx", "etx", "eot", "enq", "ack", "bel",
" bs", " ht", " nl", " vt", " ff", " cr", " so", " si",
"dle", "dc1", "dc2", "dc3", "dc4", "nak", "syn", "etb",
"can", " em", "sub", "esc", " fs", " gs", " rs", " us",
" sp", "  !", "  \"", "  #", "  $", "  %", "  &", "  '",
"  (", "  )", "  *", "  +", "  ,", "  -", "  .", "  /",
"  0", "  1", "  2", "  3", "  4", "  5", "  6", "  7",
"  8", "  9", "  :", "  ;", "  <", "  =", "  >", "  ?",
"  @", "  A", "  B", "  C", "  D", "  E", "  F", "  G",
"  H", "  I", "  J", "  K", "  L", "  M", "  N", "  O",
"  P", "  Q", "  R", "  S", "  T", "  U", "  V", "  W",
"  X", "  Y", "  Z", "  [", "  \\", "  ]", "  ^", "  _",
"  `", "  a", "  b", "  c", "  d", "  e", "  f", "  g",
"  h", "  i", "  j", "  k", "  l", "  m", "  n", "  o",
"  p", "  q", "  r", "  s", "  t", "  u", "  v", "  w",
"  x", "  y", "  z", "  {", "  |", "  }", "  ~", "del"
};

int	base =	010;
int	max;
unsigned long	addr;
static char		word[DBUFSIZE + MB_LEN_MAX + 1];
static char		lastword[DBUFSIZE];
static int		count;
static int		Nflag;
static long		skip;
static eucwidth_t	wp;

static char posix_var[] = "POSIX2";
static int posix;

static char readerr[] =
	":59:Read error on %s: %s\n";
static char invoptarg[] =
	":93:Invalid argument to option -%c\n";
static char usage_str[] =
	":123:Usage:\n"
	"\tod [-v] [-A address_base] [-j skip]\n"
	"\t\t[-N count] [-t type_string] ... [file ...]\n";
static char too_large[] =
	":124:'-%c %s' is too large.\n";
static char too_many_types[] =
	":125:Too many type_strings.\n";

void
usage()
{
	(void)pfmt(stderr, MM_ACTION, usage_str);
	exit(1);
}

main(argc, argv)
int argc;
char **argv;
{
	register char *p;
	register n = 0, f, same = -1;
	struct dfmt *d;
	struct dfmt **cv = conv_vec;
	int showall = NO;
	int c;
	int eofflag;
	int remainbytes = 0;
	int reopened = NO;
	extern int optind;
	extern char *optarg;
	int type;
	int prec;
	int status = 0;
	char *cp;
	char *skipstr;
	char new_opts_used = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:od");

	if (getenv(posix_var) != 0) {
		posix = 1;
	} else {
		posix = 0;
	}

	getwidth(&wp);

#ifdef STANDALONE
	if (argv[0][0] == '\0')
		argc = getargv("od", &argv, 0);
#endif

	while ((c = getopt(argc, argv, "bcdDhfFoOsSxXA:j:N:t:v")) != EOF) {
		switch (c) {
		/* compatibility cases: */
		case 'b':
			*(cv++) = &u_b_oct;
			break;
		case 'c':
			if (posix)
				*(cv++) = &cchar;
			else
				*(cv++) = &occhar;
			break;
		case 'd':
			*(cv++) = &u_s_dec;
			break;
		case 'D':
			*(cv++) = &u_l_dec;
			break;
		case 'f':
			*(cv++) = &flt;
			break;
		case 'F':
			*(cv++) = &odble;
			break;
		case 'o':
			*(cv++) = &u_s_oct;
			break;
		case 'O':
			*(cv++) = &u_l_oct;
			break;
		case 's':
			*(cv++) = &os_s_dec;
			break;
		case 'S':
			*(cv++) = &s_l_dec;
			break;
		case 'h':
		case 'x':
			*(cv++) = &u_s_hex;
			break;
		case 'X':
			*(cv++) = &u_l_hex;
			break;
		/* end of compatibility cases */
		case 'A':
			if (strcmp(optarg, "d") == 0)
				base = 10;
			else if (strcmp(optarg, "o") == 0)
				base = 8;
			else if (strcmp(optarg, "x") == 0)
				base = 16;
			else if (strcmp(optarg, "n") == 0)
				base = 0;
			else {
				(void)pfmt(stderr, MM_ERROR, invoptarg, c);
				usage();
			}
			new_opts_used = 1;
			break;
		case 'j':
			p = optarg + strlen(optarg) - 1;
			if (strcmp(p, "b") == 0) {
				if (strncmp(optarg, "0x", (size_t)2)
						== 0) {
					skip = strtol(optarg, &cp, 16);
				} else {
					*p = '\0';
					skip = strtol(optarg, &cp, 0)
						* 512;
				}
			} else if (strcmp(p, "k") == 0) {
				*p = '\0';
				skip = strtol(optarg, &cp, 0) * 1024;
			} else if (strcmp(p, "m") == 0) {
				*p = '\0';
				skip = strtol(optarg, &cp, 0)
					* (1024*1024);
			} else {
				skip = strtol(optarg, &cp, 0);
			}
			if (*cp != '\0') {
				(void)pfmt(stderr, MM_ERROR, invoptarg, c);
				usage();
			}
			addr = skip;
			skipstr = optarg;
			new_opts_used = 1;
			break;
		case 'N':
			Nflag++;
			count = strtol(optarg, &cp, 0);
			if (*cp != '\0') {
				(void)pfmt(stderr, MM_ERROR, invoptarg, c);
				usage();
			}
			new_opts_used = 1;
			/*
	 		 * SPEC1170 requires that when a utility reads a file
         		 * and terminates without an error before reaching EOF,
	 		 * then the file offset is positioned just past the
			 * last byte processed.
	 		 */
			setbuf(stdin, (char *)NULL);
			break;
		case 't':
			cp = optarg;
			while(*cp != '\0') {
				switch(*cp++) {
				case 'a':
				    d = &nchar; break;
				case 'c':
				    d = &cchar; break;
				case 'd':
				    type = SDEC;  goto get_size;
				case 'o':
				    type = UOCT;  goto get_size;
				case 'u':
				    type = UDEC;  goto get_size;
				case 'x':
				    type = UHEX;  goto get_size;
				case 'f':
				    type = FLOAT;
				get_size:
				    if (isdigit(*cp)) {
					prec = strtol(cp, &cp, 10);
					if (type == FLOAT) {
					    if ((prec != sizeof(float))
						&& (prec !=
						sizeof(double))
						&& (prec !=
						sizeof(long double)))
					    {
						(void)pfmt(stderr, MM_ERROR,
							invoptarg, c);
						usage();
					    }
					} else {
					    if (prec == sizeof(char))
					    {
						prec = CHAR;
					    }
					    else
					    if (prec == sizeof(short))
					    {
						prec = SHORT;
					    }
					    else
					    if (prec == sizeof(int))
					    {
						prec = INT;
					    }
					    else
					    if (prec == sizeof(long))
					    {
						prec = LONG;
					    }
					    else
					    {
						(void)pfmt(stderr, MM_ERROR,
							invoptarg, c);
						usage();
					    }
					}
				    } else {
					if (type == FLOAT) {
					    prec = sizeof(double);
					    if (*cp && strchr("FDL", *cp)) {
						if (*cp == 'F')
							prec = sizeof(float);
						cp++;
					    }
					} else {
					    prec = INT;
					    if (*cp && strchr("CSIL", *cp)) {
						if (*cp == 'C')
							prec = CHAR;
						else if (*cp == 'S')
							prec = SHORT;
						else if (*cp == 'I')
							prec = INT;
						else
							prec = LONG;
						cp++;
					    }
					}
				    }
				    if (type == FLOAT)
					switch (prec) {
					case sizeof(float):
						d = &flt; break;
					default:
						d = &dble; break;
					}
				    else
					d = templates[type][prec];
				    break;
				default :
				    (void)pfmt(stderr, MM_ERROR, invoptarg, c);
				    usage ();
				}
				if (cv >= &conv_vec[CONVSIZE]) {
					(void)pfmt(stderr, MM_ERROR,
						too_many_types);
					status = 2;
					exit(status);
				} else {
					*(cv++) = d;
				}
			}
			new_opts_used = 1;
			break;
		case 'v':
			showall = YES;
			break;
		default:
			usage();
		}
	}
	if (cv == conv_vec) {
		*(cv++) = &u_s_oct;
	}

	/* Calucate max number chars in line */
	max = 0;
	for (cv = conv_vec; d = *cv; cv++) {
		f = (d->df_field +1) * (DBUFSIZE / d->df_size);
		if (f > max)
			max = f;
	}

	argc -= optind;
	argv += optind;
	p = word;

	/*
	 * if one of the 'filenames' has a chance of being an offset
	 * check it out.
	 */
	if (!new_opts_used && (argc == 1 && argv[0][0] == '+' || argc == 2)) {
		char *maybe_offset;
		int maybe_skip;

		if (argc == 2) {
			maybe_offset = argv[1];
		} else {
			maybe_offset = argv[0];
		}
		if ((maybe_skip = offset(maybe_offset)) >= 0) {
			argc--;
			skip = maybe_skip;
			addr = skip;
			skipstr = maybe_offset;
		}
	}

	do {
		if(argc) {
			reopened = YES;
 			if (freopen(*argv++, "r", stdin) == NULL) {
				cp = *(argv - 1);
 				(void)pfmt(stderr, MM_ERROR,
					":3:Cannot open %s: %s\n",
 					cp, strerror(errno));
				status = 2;
				continue;
			}
		}
		if(skip) {
			if (!argc && (reopened == YES)) {
				(void)pfmt(stderr, MM_ERROR, too_large,
					'j', skipstr);
				status = 2;
				exit(status);
			/*
			 * Do not try to seek to the file end if the file is
			 * actually stdin.
			 */
			} else if (reopened == YES &&
					fseek(stdin, 0L, SEEK_END) == 0) {
				f = ftell(stdin);
				if (f < skip) {
					if (!argc) {
						(void)pfmt(stderr, MM_ERROR,
						    too_large, 'j', skipstr);
						status = 2;
						exit(status);
					}
					skip -= f;
					continue;
				} else {
					(void)fseek(stdin, skip, SEEK_SET);
					skip = 0;
				}
			} else {
				while (skip && (getchar() != EOF)) {
					skip--;
				}
				if (skip) {
					if (!argc) {
						(void)pfmt(stderr, MM_ERROR,
						    too_large, 'j', skipstr);
						status = 2;
						exit(status);
					} else {
						continue;
					}
				}
			}
		}

		for (eofflag = NO; (eofflag != YES) && (!Nflag || count); ) {
			if (!wp._multibyte) {
				for ( ; n < DBUFSIZE; n++) {
					if ((Nflag && n >= count) ||
					    (f = getchar()) == EOF) {
						*p = '\0';
						eofflag = YES;
						break;
					} else {
						*p++ = (char)f;
					}
				}
			} else {
				for ( ; n < (DBUFSIZE + MB_LEN_MAX); n++) {
					if ((Nflag && n >= count) ||
					    (f = getchar()) == EOF) {
						*p = '\0';
						eofflag = YES;
						break;
					} else {
						*p++ = (char)f;
					}
				}
				if (n > DBUFSIZE)
					eofflag = NO;
				for ( ; n > DBUFSIZE; n--) {
					f = *((unsigned char *)--p);
					if (ungetc(f, stdin) == EOF) {
					    cp = *(argv - 1);
					    (void)pfmt(stderr, MM_ERROR,
						readerr, cp, strerror(errno));
					    status = 2;
					    break;
					}
				}
			}
			if (Nflag) {
				if ((n >= DBUFSIZE) || !argc) {
					n = n < count ? n : count;
					count -= n;
				}
			}
			if (n > 0) {
				if ((n >= DBUFSIZE) || !argc
						|| (Nflag && !count)) {
					PRINTLINE((size_t)n);
					p = word;
					n = 0;
				}
			}
		}
	} while (argc-- && (!Nflag || count));
	if (base) {
		putn(addr, base, 7);
	}
	(void)putchar('\n');
	exit(status);
}

static void
line(a, w, n, remp)
unsigned long a;
char w[];
size_t n;
int *remp;
{
	register i, f;
	register struct dfmt *c;
	register struct dfmt **cv = conv_vec;

	f = 1;
	while (c = *cv++) {
		if (base) {
			if(f) {
				putn(a, base, 7);
				f = 0;
			} else
				(void)printf("       ");
		}

		i = 0;
		if (wp._multibyte &&
		   (c->df_option == 'c' || c->df_option == 'C' )) {
			while ((size_t)i < n) {
				if (*remp > 0) {
					pre(c->df_field, c->df_size);
					(void)printf(PAD_STR);
					(*remp)--;
					i++;
				} else {
					i += putx(w+i, c, remp);
				}
			}
		} else {
			while ((size_t)i < n) {
				i += putx(w+i, c, remp);
			}
		}
		(void)putchar('\n');
	}
}

static int
putx(n, c, remp)
char n[];
struct dfmt *c;
int *remp;
{
	register ret = 0;
	
	switch(c->df_option) {
	case 'b':
	case 'y':
	case 'e':
		{
			unsigned char *sn = (unsigned char *)n;
			pre(c->df_field, c->df_size);
			putn((unsigned long)*sn, c->df_radix,
							c->df_field);
			ret = c->df_size;
			break;
		}
	case 't':
		{
			unsigned char *sn = (unsigned char *)n;
			if (*sn > 127) {
				/* Modify copy of string - not actual */
				char nsn[DBUFSIZE+MB_LEN_MAX+1];

				sn = (unsigned char *)nsn;
				copy_str(nsn,n,c);
				*sn = (~(*sn) + 1) & 0377;
			} else 
				pre(c->df_field-1, c->df_size);
			putn((unsigned long)*sn, c->df_radix, c->df_field-1);
			ret = c->df_size;
			break;
		}
	case 'o':
	case 'd':
	case 'x':
		{
			unsigned short *sn = (unsigned short *)n;
			pre(c->df_field, c->df_size);
			putn((unsigned long)*sn, c->df_radix, c->df_field);
			ret = c->df_size;
			break;
		}
	case 's':
		{
			unsigned short *sn = (unsigned short *)n;
			if (c == &os_s_dec && !posix) {	/* backward compat */
				(void)putchar(' ');
			}
			if (*sn > 32767) {
				/* Modify copy of string - not actual */
				char nsn[DBUFSIZE+MB_LEN_MAX+1];

				sn = (unsigned short *)nsn;
				copy_str(nsn,n,c);
				*sn = (~(*sn) + 1) & 01777777;
			} else 
				pre(c->df_field-1, c->df_size);
			putn((unsigned long)*sn, c->df_radix, c->df_field-1);
			ret = c->df_size;
			break;
		}
	case 'a':
		pre(c->df_field, c->df_size);
		aput(*n);
		ret = c->df_size;
		break;
	case 'c':
		pre(c->df_field, c->df_size);
		if (!wp._multibyte) {
			cput(*((unsigned char *)n));
		} else {
			*remp = wcput(n, c->df_field);
		}
		ret = c->df_size;
		break;
	case 'C':
		pre(c->df_field, c->df_size);
		if (!wp._multibyte) {
			ocput(*((unsigned char *)n));
		} else {
			*remp = owcput(n, c->df_field);
		}
		ret = c->df_size;
		break;
	case 'D':
	case 'O':
	case 'X':
		pre(c->df_field, c->df_size);
		{
			unsigned long *ln = (unsigned long *)n;
			putn((unsigned long)*ln, c->df_radix, c->df_field);
			ret = c->df_size;
			break;
		}
	case 'f':
		pre(c->df_field, c->df_size);
		{
			float *fn = (float *)n;
			(void)printf("% 14.7e",*fn);
			ret = c->df_size;
			break;
		}
	case 'F':
		pre(c->df_field, c->df_size);
		{
			double *dn = (double *)n;
			(void)printf("% 22.14e",*dn);
			ret = c->df_size;
			break;
		}
	case 'G':	/* old version of 'F' */
		pre(c->df_field, c->df_size);
		{
			double *dn = (double *)n;
			(void)printf("%21.14e",*dn);
			ret = c->df_size;
			break;
		}
	case 'S':
		{
			unsigned long *ln = (unsigned long *)n;
			if (*ln > 2147483647){
				/* Modify copy of string - not actual */
				char nln[DBUFSIZE+MB_LEN_MAX+1];

				ln = (unsigned long *)nln;
				copy_str(nln,n,c);
				*ln = (~(*ln)+1) & 037777777777;
			} else 
				pre(c->df_field-1, c->df_size);
			putn((unsigned long)*ln, c->df_radix, c->df_field-1);
			ret = c->df_size;
			break;
		}
	}
	return(ret);
}

static void
copy_str(cp_str, orig_str, val)
char cp_str[];
char orig_str[];
struct dfmt *val;
{
	register i;

	for (i=0 ; i <= DBUFSIZE+MB_LEN_MAX ; i++)
		cp_str[i] = orig_str[i];
	pre(val->df_field, val->df_size);
	(void)putchar('-'); 
}

static void
aput(c)
int c;
{
	c &= 0177;
	(void)fputs(ascii_values[c], stdout);
}

static void
cput(c)
int c;
{
	c &= 0377;
	if(isprint(c)) {
		(void)printf("  ");
		(void)putchar(c);
		return;
		}
	ESCSEQ(c);
}

static int
wcput(cp, field)
	char	*cp;
	int	field;
{
	register int	eucw;
	wchar_t		wc;
	int		j;

	if ((eucw = mbtowc(&wc, cp, MB_LEN_MAX)) == -1) {
		putn(*((unsigned char *)cp), 8, 3);
		eucw = 0;
	} else if(iswprint(wc)) {
		for (j = field - scrwidth(wc); j > 0; j--) {
			(void)putchar(' ');
		}
		(void)putwchar(wc);
		eucw--;
	} else {
		ESCSEQ(*((unsigned char *)cp));
		eucw = 0;
	}
	return (eucw);
}

static void
ocput(c)
int c;
{
	c &= 0377;
	if(isprint(c)) {
		(void)printf("  ");
		(void)putchar(c);
		return;
		}
	OESCSEQ(c);
}

static int
owcput(cp, field)
	char	*cp;
	int	field;
{
	register int	eucw;
	wchar_t		wc;
	int		j;

	if ((eucw = mbtowc(&wc, cp, MB_LEN_MAX)) == -1) {
		putn(*((unsigned char *)cp), 8, 3);
		eucw = 0;
	} else if(iswprint(wc)) {
		for (j = field - scrwidth(wc); j > 0; j--) {
			(void)putchar(' ');
		}
		(void)putwchar(wc);
		eucw--;
	} else {
		OESCSEQ(*((unsigned char *)cp));
		eucw = 0;
	}
	return (eucw);
}

static void
putn(n, b, c)
unsigned long n;
int b, c;
{
	register d;

	if(!c)
		return;
	putn(n/b, b, c-1);
	d = n%b;
	if (d > 9)
		(void)putchar(d-10+'a');
	else
		(void)putchar(d+'0');
}

static void
pre(f,n)
int f, n;
{
	int i,m;

	m = (max/(DBUFSIZE/n)) - f;
	for(i=0; i<m; i++)
		(void)putchar(' ');
}

/*
 * if the string offset doesn't match the ERE:
 *     ^+?([0-7]+|[0-9]+\.|[0-9a-fA-F]+x)b?$
 * then a negative value is returned, otherwise the number of bytes
 * to skip is returned.
 *
 * Note that this offset function will behave differently under certain
 * circumstances, from the previous version. However, those cases are
 * due to errors in the previous function's design and implementation,
 * and, more importantly, are only in very unlikely cases.
 */

static long
offset(char *string) {
	char *offstr, *num_end, *end_ptr, *free_ptr;
	int block = 1, quit;
	long result;

	offstr = strdup(string);	/* so we can speculatively change it */
	free_ptr = offstr;

	if (offstr[0] == '+') {
		offstr++;
	}

	num_end = offstr + strlen(offstr) - 1;

	base = 0;
	quit = 0;
	while (!quit &&
		(*num_end == '.' || *num_end == 'b' || *num_end == 'x')) {
		if (*num_end == '.' && base == 0) {
			base = 10;
			quit = 1;
		} else if (*num_end == 'x' && base == 0) {
			base = 16;
			quit = 1;
		} else if (*num_end == 'b' && base != 16) {
			block = BSIZE;
		}
		num_end--;
	}
	*(num_end + 1) = '\0';

	if (base == 0) {
		base = 8;
	}

	/*
	 * Check to see if there's anything left to interpret. If there
	 * isn't then we should obviously return the 'no number' result.
	 */

	if (num_end < offstr) {
		free(free_ptr);
		return -1;
	}

	result = strtol(offstr, &end_ptr, base);
	if (end_ptr != num_end + 1) {
		/*
		 * Since strtol doesn't allow the default base to be set,
		 * setting base = 8 may break strtol when converting numbers
		 * with leading '0x' or equivalent. So try again with
		 * base = 0.
		 */
		if (base == 8) {
			result = strtol(offstr, &end_ptr, 0);
			if (end_ptr != num_end + 1) {
				free(free_ptr);
				return -1;
			}
			base = 16;
		} else {
			base = 8;
			free(free_ptr);
			return -1;
		}
	}

	result *= block;
	free(free_ptr);
	return result;
}
