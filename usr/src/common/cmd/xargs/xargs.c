/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xargs:xargs.c	2.9.3.5"
#ident "$Header$"

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>
#include <limits.h>
#include <nl_types.h>
#include <langinfo.h>
#include <regex.h>
#include <errno.h>

#ifndef MAX_INPUT
#define MAX_INPUT _POSIX_MAX_INPUT
#endif

#define FALSE 0
#define TRUE 1
#define MAXSBUF	MAX_INPUT
#define MAXIBUF	(MAX_INPUT * 2)
#define MAXINSERTS 5

char	**arglist;
char	*next;
char	*lastarg = "";
char	**ARGV;
char	*LEOF = "_"; 
char	*INSPAT = "{}";
char	ins_buf[MAXIBUF];
char	*p_ibuf;

struct inserts {
	char	**p_ARGV;	/* where to put newarg ptr in arg list */
	char	*p_skel;	/* ptr to arg template */
} saveargv[MAXINSERTS];

int	PROMPT = -1;
int	BUFLIM;
int	N_ARGS = 0;
int	N_args = 0;
int	N_lines = 0;
int	DASHX = FALSE;
int	MORE = TRUE;
int	PER_LINE = FALSE;
int	ERR = FALSE;
int	OK = TRUE;
int	LEGAL = FALSE;
int	TRACE = FALSE;
int	INSERT = FALSE;
int	linesize = 0;
int	ibufsize = 0;

int	echoargs(), getchr(), lcall(), xindex();
void	addibuf();
char	*addarg(), *checklen(), *getarg(), *insert();
static long envsize();

main(argc, argv)
int	argc;
char	**argv;
{
	char	*cmdname, *initbuf, **initlist, *flagval;
	int	initsize;
	register int	j, n_inserts;
	register struct inserts *psave;
	
	static char Sccsid[] = "@(#)xargs:xargs.c	2.9";
	
	/* initialization */
	
	(void)setlocale(LC_ALL,"");
	(void)setcat("uxcore");
	(void)setlabel("UX:xargs");

	argc--;
	argv++;
	n_inserts = 0;
	psave = saveargv;
	
	/* look for flag arguments */
	
	while (argc > 0 && (*argv)[0] == '-') {
		flagval = *argv + 1;
		if (strcmp(flagval, "-") == 0) {
			argc--; argv++;
			break;
		}
		switch (*flagval++) {

		case 'x':
			DASHX = LEGAL = TRUE;
			break;

		case 'l':
			if (*flagval == '\0')
				flagval = "1";
			/*FALLTHROUGH*/
		case 'L':
			PER_LINE = LEGAL = TRUE;
			N_ARGS = 0;
			INSERT = FALSE;
			if (*flagval == '\0') {
				if (--argc == 0)
					goto noarg;
				flagval = *++argv;
			}
			if ((PER_LINE = atoi(flagval)) <= 0) {
				pfmt(stderr, MM_ERROR,
				    ":832:#lines must be positive int: %s\n",
					*argv);
				OK = FALSE;
			}
			break;

		case 'i':
			if (*flagval == '\0')
				flagval = "{}";
			/*FALLTHROUGH*/
		case 'I':
			INSERT = PER_LINE = LEGAL = TRUE;
			N_ARGS = 0;
			if (*flagval == '\0') {
				if (--argc == 0)
					goto noarg;
				flagval = *++argv;
			}
			INSPAT = flagval;
			break;

		case 't':
			TRACE = TRUE;
			break;

		case 'E':
			if (*flagval == '\0') {
				if (--argc == 0)
					goto noarg;
				flagval = *++argv;
			}
			/*FALLTHROUGH*/
		case 'e':
			LEOF = flagval;
			break;

		case 's':
			if (*flagval == '\0') {
				if (--argc == 0)
					goto noarg;
				flagval = *++argv;
			}
			BUFLIM = atoi(flagval);
			if (BUFLIM <= 0) {
				pfmt(stderr, MM_ERROR,
				    ":972:size must be positive: %s\n",
					*argv);
				OK = FALSE;
			}
			break;

		case 'n':
			if (*flagval == '\0') {
				if (--argc == 0)
					goto noarg;
				flagval = *++argv;
			}
			if ((N_ARGS = atoi(flagval)) <= 0 ) {
				pfmt(stderr, MM_ERROR,
				    ":834:#args must be positive int: %s\n",
					*argv);
				OK = FALSE;
			} else {
				LEGAL = DASHX || N_ARGS == 1;
				INSERT = PER_LINE = FALSE;
			}
			break;

		case 'p':
			if ((PROMPT = open("/dev/tty", O_RDONLY)) == -1) {
				pfmt(stderr, MM_ERROR, ":843:cannot read from tty for -p\n");

				OK = FALSE;
			} else {
				TRACE = TRUE;
			}
			break;

		default:
			pfmt(stderr, MM_ERROR,
				":835:unknown option: %s\n", *argv);
			OK = FALSE;
			break;

		noarg:;
			pfmt(stderr, MM_ERROR,
				":973:missing option-argument for %s\n", *argv);
			OK = FALSE;
			break;
		}

		argv++;
		argc--;
	}

	if (OK) {
		long max;

#ifdef ARG_MAX
		max = ARG_MAX;
#else
		if ((max = sysconf(_SC_ARG_MAX)) <= 0) {
			pfmt(stderr, MM_ERROR,
				":974:invalid ARG_MAX from sysconf()\n");
			OK = FALSE;
		}
#endif
		max -= envsize() + 2048; /* POSIX.2 says to leave 2K */
		if (BUFLIM == 0 || BUFLIM > max)
			BUFLIM = max;
	}
	
	if (!OK)
		exit(1);

	/* allocate space */

	next = (char *)malloc(BUFLIM + MAX_INPUT);
	ARGV = arglist = (char **)malloc((BUFLIM + 1) / 2 * sizeof(char *));
	if (next == 0 || arglist == 0) {
		pfmt(stderr, MM_ERROR, ":31:Out of memory: %s\n",
			strerror(errno));
		exit(1);
	}
	
	/* pick up command name */
	
	if (argc == 0) {
		cmdname = "/usr/bin/echo";
		*ARGV++ = addarg(cmdname);
	} else {
		cmdname = *argv;
	}
	
	/* pick up args on command line */
	
	while (OK && argc--) {
		if (INSERT && ! ERR) {
			if (xindex(*argv, INSPAT) != -1) {
				if (++n_inserts > MAXINSERTS) {
					pfmt(stderr, MM_ERROR,
					    ":836:too many args with %s\n",
						INSPAT);
					OK = FALSE;
					ERR = TRUE;
				}
				psave->p_ARGV = ARGV;
				(psave++)->p_skel = *argv;
			}
		}
		*ARGV++ = addarg( *argv++ );
	}
	
	/* pick up args from standard input */
	
	initbuf = next;
	initlist = ARGV;
	initsize = linesize;
	
	while (OK && MORE) {
		next = initbuf;
		ARGV = initlist;
		linesize = initsize;
		if (*lastarg)
			*ARGV++ = addarg( lastarg );
	
		while ((*ARGV++ = getarg()) && OK)
			;
	
		/* insert arg if requested */
	
		if (!ERR && INSERT) {
			p_ibuf = ins_buf;
			ARGV--;
			j = ibufsize = 0;
			for (psave = saveargv; ++j <= n_inserts; ++psave) {
				addibuf(psave);
				if (ERR)
					break;
			}
		}
		*ARGV = 0;
	
		/* exec command */
	
		if (!ERR) {
			if (!MORE &&
			    (PER_LINE && N_lines == 0 || N_ARGS && N_args == 0))
				exit(0);
			OK = TRUE;
			j = TRACE ? echoargs() : TRUE;
			if (j) {
				int r = lcall(cmdname, arglist);

				if (r != -1 && r != 127 && r != 126)
					continue;
				pfmt(stderr, MM_ERROR,
				    ":837:%s not executed or returned -1\n",
					cmdname);
				exit(r > 0 ? r : 1);
			}
		}
	}

	if (OK)
		exit(0);
	else
		exit(1);
}


char *
checklen(arg)
char	*arg;
{
	register int	oklen;
	
	oklen = TRUE;
	linesize += strlen(arg) + 1;
	if (linesize > BUFLIM) {
		lastarg = arg;
		oklen = OK = FALSE;

		if (LEGAL) {
			ERR = TRUE;
			pfmt(stderr, MM_ERROR, ":838:arg list too long\n");

			OK = FALSE;
		} else if (N_args > 1) {
			N_args = 1;
		} else {
			pfmt(stderr, MM_ERROR, ":839:a single arg was greater than the max arglist size\n");
			OK = FALSE;
			ERR = TRUE;
		}
	}
	return(oklen ? arg : 0);
}

char *
addarg(arg)
char	*arg;
{
	strcpy(next, arg);
	arg = next;
	next += strlen(arg)+1;
	return(checklen(arg));
}


char *
getarg()
{
	register char	c, c1, *arg;
	char		*retarg;

	while ((c = getchr()) == ' ' || c == '\n' || c == '\t')
		;

	if (c == '\0') {
		MORE = FALSE;
		return(0);
	}

	arg = next;
	for (;; c = getchr()) {
		switch ( c ) {
	
		case '\t':
		case ' ' :
			if (INSERT) {
				*next++ = c;
				break;
			}

		case '\0':
		case '\n':
			*next++ = '\0';
			if (strcmp(arg, LEOF) == 0 || c == '\0') {
				MORE = FALSE;
				if (c != '\n')
					while (c = getchr())
						if (c == '\n')
							break;
				return(0);
			} else {
				++N_args;
				if (retarg = checklen(arg)) {
					if ((PER_LINE && c == '\n' &&
					    ++N_lines >= PER_LINE) ||
					    (N_ARGS && N_args >= N_ARGS)) {
						N_lines = N_args = 0;
						lastarg = "";
						OK = FALSE;
					}
				}
				return(retarg);
			}
	
		case '\\':
			*next++ = getchr();
			break;
	
		case '"':
		case '\'':
			while ((c1 = getchr()) != c) {
				if (c1 == '\0' || c1 == '\n') {
					*next++ = '\0';
					pfmt(stderr, MM_ERROR,
					    ":840:missing quote?: %s\n",
					    	arg);
					OK = FALSE;
					ERR = TRUE;
					return(0);
				}
				*next++ = c1;
			}
			break;
	
		default:
			*next++ = c;
			break;
		}
	}
}




int
echoargs()
{
	static regex_t yesre;
	static int first = 1;
	char *p, ans[MAX_INPUT];
	char **argv;
	int err;
	char ch;

	for (argv = arglist; *argv != 0; argv++) {
		write(2, *argv, strlen(*argv));
		write(2, " ", 1);
	}
	if (PROMPT == -1) {
		write(2, "\n", 1);
		return TRUE;
	}
	p = gettxt(":975", "?...");
	write(2, p, strlen(p));
	if (first) {
		first = 0;
		err = regcomp(&yesre, nl_langinfo(YESEXPR),
			REG_EXTENDED | REG_NOSUB);
		if (err != 0) {
	badre:;
			regerror(err, &yesre, ans, MAX_INPUT);
			pfmt(stderr, MM_ERROR,
				"uxcore.abi:1234:RE failure: %s\n", ans);
			exit(2);
		}
	}
	p = ans;
	for (;;) {
		if (read(PROMPT, &ch, 1) == 0)
			exit(0);
		if (ch == '\n')
			break;
		if (p < &ans[MAX_INPUT - 1])
			*p++ = ch;
	}
	if (p != ans) {
		*p = '\0';
		err = regexec(&yesre, ans, (size_t)0, (regmatch_t *)0, 0);
		if (err == 0)
			return TRUE;
		if (err != REG_NOMATCH)
			goto badre;
	}
	return FALSE;
}


char *
insert(pattern, subst)
char	*pattern, *subst;
{
	static char	buffer[MAXSBUF+1];
	int		len, ipatlen;
	register char	*pat;
	register char	*bufend;
	register char	*pbuf;
	
	if (subst)
		len = strlen(subst);
	else {
		len = 0;
		subst="";
	}
	ipatlen = strlen(INSPAT) - 1;
	pat = pattern - 1;
	pbuf = buffer;
	bufend = &buffer[MAXSBUF];
	
	while (*++pat) {
		if (xindex(pat, INSPAT) == 0) {
			if (pbuf + len >= bufend) {
				break;
			} else {
				strcpy(pbuf, subst);
				pat += ipatlen;
				pbuf += len;
			}
		} else {
			*pbuf++ = *pat;
			if (pbuf >= bufend)
				break;
		}
	}
	
	if (!*pat) {
		*pbuf = '\0';
		return(buffer);
	} else {
		pfmt(stderr, MM_ERROR,
		    ":841:max arg size with insertion via %s's exceeded\n",
			INSPAT);
		OK = FALSE;
		ERR = TRUE;
		return(0);
	}
}


void
addibuf(p)
struct inserts	*p;
{
	register char	*newarg, *skel, *sub;
	int		l;
	
	skel = p->p_skel;
	sub = *ARGV;
	linesize -= strlen(skel) + 1;
	newarg = insert(skel, sub);
	if (checklen(newarg)) {
		if ((ibufsize += (l = strlen(newarg) + 1)) > MAXIBUF) {
			pfmt(stderr, MM_ERROR, ":842:insert-buffer overflow\n");
			OK = FALSE;
			ERR = TRUE;
		}
		strcpy(p_ibuf, newarg);
		*(p->p_ARGV) = p_ibuf;
		p_ibuf += l;
	}
}


int
getchr()
{
	char	c;

	if (read(0, &c, 1) == 1)
		return(c);
	return(0);
}


int
lcall(sub, subargs)
char	*sub, **subargs;
{

	int retcode;
	register pid_t iwait, child;

	switch (child = fork()) {
	default:
		while ((iwait = wait(&retcode)) != child && iwait != (pid_t)-1)
			;
		if (iwait == (pid_t)-1 || WIFEXITED(retcode) == 0 ||
		    (WEXITSTATUS(retcode) & 0377) == 0377)
			return(-1);
		return(WEXITSTATUS(retcode));
	case 0:
		execvp(sub,subargs);
		if (errno == ENOENT)
			exit(127);
		else
			exit(126);
	case -1:
		return(-1);
	}
}


/*
 * If `s2' is a substring of `s1' return the offset of the first
 * occurrence of `s2' in `s1', else return -1.
 */
int
xindex(as1, as2)
char	*as1, *as2;
{
	register char	*s1, *s2, c;
	int		offset;

	s1 = as1;
	s2 = as2;
	c = *s2;

	while (*s1) {
		if (*s1++ == c) {
			offset = s1 - as1 - 1;
			s2++;
			while ((c = *s2++) == *s1++ && c)
				;
			if (c == 0)
				return(offset);
			s1 = offset + as1 + 1;
			s2 = as2;
			c = *s2;
		}
	}
	return(-1);
}

static long
envsize()
{
	extern char **environ;
	char **p;
	long sz;

	sz = 0;
	for (p = environ; *p != 0; p++)
		sz += sizeof(char *) + 1 + strlen(*p);
	return sz;
}
