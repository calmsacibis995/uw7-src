/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)printf:printf.c	1.2.6.2"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <string.h>
#include <pfmt.h>
#include <ctype.h>

extern  char *strcadd();

#ifdef  __STDC__
static int  format (char *str, char *end, int, char**);
static void printb (char *fmt, char *);
static void printl (char *fmt, char *, int);
#else
static int format ();
static void printb ();
static void printl ();
#endif


#define CONV_CHARS	"diouxXcbs"		/* conversion character */

#define FLAG_CHARS	"$-+ #0123456789."	/* conversion flags */

#define CONS_CHARS	"\'\""			/* character constant leader */

static const char Incorrect[] =
	":1:Incorrect usage\n";

static const char Usage[] =
	":63:Usage: printf format [[[arg1] arg2] ... argn]\n";

static const char overflow[] =
	":64:\"%s\" arithmetic overflow\n" ;

static const char badchar[] =
	":65:\"%s\" not completely converted\n" ;

static const char badstr[] =
	":66:\"%s\" expected numeric value\n" ;

static const char nomem[] =
	":67:Out of memory: %s\n";

/* type of numeric value */
#define UNSIGNED 	1
#define SIGNED 		0

static int errcode = 0; /* exit status */

main(argc, argv)
int	argc;
char	**argv;
{
	char	*fmt, *end;
	int 	n;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxsysadm");
	(void)setlabel("UX:printf");

	if (argc > 1 && strcmp(argv[1],"--") == 0) {
		argc--;argv++;
	}

	if (argc == 1) {
		pfmt(stderr, MM_ERROR, Incorrect);
		pfmt(stderr, MM_ACTION, Usage) ;
		exit(1);
	}

	if ((fmt = malloc(strlen(argv[1]) + 1)) == (char *)NULL) {
		pfmt(stderr, MM_ERROR, nomem, strerror(errno));
		exit(1);
	}

	end= strcadd(fmt, argv[1]);
	argc -= 2;
	argv += 2;
	do {
		n = format(fmt, end, argc, argv) ;
		argc -= n;
		argv += n;
	} while (n != 0 && argc > 0);
	exit(errcode ? 2:0) ;
	/*NOTREACHED*/
}

static int
#ifdef __STDC__
format(char *str, char *end, int argc, char **argv)
#else
format(str, end, argc, argv)
register char *str;
char *end;
int argc;
char **argv;
#endif
{
	register char 	*p;
	register char 	*s = str;
	static   char 	*tmpf = NULL;/* temporary format buffer */

	register int 	conv;	/* conversion character */
	register char  	*sval;	/* string pointer */
	int 		nargs = 0;

	int		looping = 1;	/* re-using format or not flag */
	int		posn, oldposn;	/* position specifiers */ 
	char		*remains;	/* remains of a converted number */
	char		*posnstr;	/* pos'n spec string */

	/*  allocate temporary format buffer */
	if (tmpf == NULL) 
		if ((tmpf = malloc((end - str) + 1)) == NULL) {
			pfmt(stderr, MM_ERROR, nomem, strerror(errno));
			exit(1);
		}

	posn = 0;

	while (s < end) {

		if (*s != '%') {
			(void) putchar((int)*s++) ;
			continue;
		}
		/* %% string */
		if (*(s+1) == '%') {
			(void) putchar((int)*s) ;
			s+=2;
			continue;
		}

		oldposn = posn;
		posn = 0;

		/*
		 * skip flags, grabbing and removing the pos'n
		 * specifier, if it exists
		 */
		for (p=tmpf; (*p++ = *s++) , s < end ; ) {
			if (*s == '$') {
				*p = '\0';
				for (posnstr=p; isdigit(*(--posnstr)););
				posnstr++;
				posn = strtol(posnstr, &remains, 10);
				if (*remains == '\0' && remains != posnstr) {
					/*
					 * don't re-use the format once we have
					 * found a position specifier
					 */
					looping = 0;

					/* remove pos'n spec */
					p = posnstr;
					s++;
				} else {
					break;
				}
			}
			if (strchr(FLAG_CHARS, (int) *s) == NULL)
				break;
		}
		/* conversion character */
		conv = *s++;

		/* not a valid %... specifier */
		if (conv == '\0' || strchr(CONV_CHARS, conv) == NULL) {
			char *t;

			for(t = tmpf; t < p; t++)
				(void) putchar ((int) *t);
			if (s <= end)
				(void) putchar(conv);
			posn = oldposn;
			continue;
		}

		if (conv == 'b')
			*p++ = 's' ;
		else
			*p++ = conv ;
		*p = '\0';

		/* get argument */
		if (posn == 0) {
			posn = oldposn + 1;
		}
		if (posn > argc) {
			sval = "";
		} else {
			sval = argv[posn - 1];
		}
		nargs++;

		switch(conv) {
		case 'c':			/* char */
			if (*sval)
				(void) printf(tmpf, *sval);
			break ;
		case 's':			/* string %s */
			(void) printf(tmpf, sval) ;
			break ;
		case 'b':			/* string %b */
			printb(tmpf, sval) ;
			break ;
		case 'd': case 'i':		/* signed long */
			printl(tmpf, sval, SIGNED) ;
			break ;
		case 'x': case 'X': case 'o': case 'u':	/* unsigned long*/
			printl(tmpf, sval, UNSIGNED) ;
			break ;
		}
	}
	return(looping ? nargs : 0);
}

#define isodigit(c)	(c >= '0' && c <= '7')

static void
#ifdef __STCC__
printb(char *fmt, char *str)
#else
printb(fmt, str)
	char *fmt;
	char *str ;
#endif 
{
	register char 	*s, *p, *end;
	register int	quit=0;
	register int	n;
	int len;
	char odigit[4];
	char *f;
	int ljust=0, seendot=0;
	int width=0, prec=0;
	int done = 0;
	int pad=' ';

	for (s = p  = str  ; *p != '\0' && quit == 0 ; p++) {
		if (*p == '\\' ) {
			switch(*++p) {
			case '\\':	 	/* <backslash> */
				*s++ = '\\';
				continue;
			case 'a': 		/* <alert> */
				*s++ = '\007';	/* ='\a'*/
				continue;
			case 'b': 		/* <backspace> */
				*s++ = '\b';
				continue;
			case 'f': 		/* <form-feed> */
				*s++ = '\f';
				continue;
			case 'n': 		/* <newline> */
				*s++ = '\n';
				continue;
			case 'r': 		/* <carriage return> */
				*s++ = '\r';
				continue;
			case 't': 		/* <tab> */
				*s++ = '\t';
				continue;
			case 'v': 		/* <vertical tab> */
				*s++ = '\v';
				continue;
			case 'c':
				quit++;
				continue;
			case '0': 		/* \0ddd octal constant*/
				n = 0;
				while (*++p && isodigit(*p) && n < 3) {
					odigit[n++] = *p;
				}
				odigit[n] = '\0';
				*s++ = strtol(odigit, NULL, 8);
				--p;
				continue;
			default:
				--p;
			}
		}
		*s++ = *p;
	}

	*s = '\0';
	end = s;
	len = end - str;

	/* 
	 * fmt can only be string of form
	 * %[flags][width][precision][...]s
	 *  where
	 * 	flags = ['-+ #0]*
	 *		where '# + are ignored
	 *		      - left justify instead of right
	 *		      0 pad with '0's instead of spaces
	 *	width = a decimal integer
	 *	precision = .[number]
	 *	...	is ignored
	 */

	
	 for(f = fmt ; !done && *f ; f++) 
		switch (*f) {
			case '-':	ljust=1;
					break;
			case '0':	pad='0';
					break;
			case '.':	seendot=1;
					break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':	{
						unsigned int n = *f - '0';

						while (++f, isdigit(*f)){
							n *= 10;
							n += *f - '0';
						}

						if (seendot)
							prec = n;
						else
							width = n;
						f--;
				 	}
					break;
			case 's':	/* print string */
					done=1;
					break;
			default:
					break;
		}

	/* Truncate the string if max precision is less than length */
	if (seendot && prec < len) {
		end -= (len-prec);
		len -= (len-prec);
	}
					
	/* print any inital pad characters */

	if (!ljust && width > len)
		while (width-- > len)
			(void) putchar (pad);

	/* print the string itself */
	for (s=str; s < end; s++)
		(void) putchar((int) *s);


	/* print any tailing pad characters */

	if (ljust && width > len)
		while (width-- > len)
			(void) putchar (pad);


	if (quit)
		exit(errcode ? 2:0);
	return;
}

static void
#ifdef __STCC__
printl(char *fmt, char *str, int type)
#else
printl(fmt, str, type)
	char *fmt;
	char *str ;
	int  type ;
#endif 
    {
	wchar_t 	wc = 0;
	unsigned long 	lval ;
	char 		*badnum;

	/* print value of the character */
	if (str[0] == '\0')
		lval = 0;
	else if (strchr(CONS_CHARS, (int) str[0]) != NULL) {
		 if (mbtowc(&wc, &str[1], MB_CUR_MAX) <= 0) {
			pfmt(stderr, MM_WARNING, badchar, str);
			errcode++;
		 }
		 lval = wc ;
	} else {
		if (type == UNSIGNED)
			lval = strtoul(str, &badnum, 0);
		else
			lval = strtol(str, &badnum, 0);
		
		if (*badnum != '\0') {
			if (lval)
				pfmt(stderr, MM_WARNING, badchar, str);
			else
				pfmt(stderr, MM_WARNING, badstr, str);
			errcode++;
		} else if (errno == ERANGE) {
			pfmt(stderr, MM_WARNING, overflow, str);
			errno = 0;
			errcode++;
		}
	}
	(void) printf(fmt, lval);
}
