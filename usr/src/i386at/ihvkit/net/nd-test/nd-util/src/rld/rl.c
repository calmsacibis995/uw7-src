#pragma comment(exestr, "@(#) rl.c 12.1 95/10/02 SCOINC")

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 * program to do io to a shell through a pseudo tty.
 * written to fill the need to allow remote login testing.
 */
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <wait.h>
#include <time.h>

#define uchar unsigned char

FILE *input = stdin;

char buf[BUFSIZ*2];

/* file matching stuff */
FILE *matchfd;
char mbuf1[BUFSIZ];
char mbuf2[BUFSIZ];

long	timestart = 0;
long	opt_timecount = 0;
int	opt_verbose = 0;    /* 0 none, 1 - commands, 2 - commands plus data */
int	opt_noexecute = 0;
int	loopcnt = 0;

int shellfd = -1;		/* fd to /dev/ptyp? for shell */

#define C_WAIT	0
#define C_SEND	1
#define	C_LOOP	2
#define C_ENDLP	3
#define C_LABEL	4
#define C_EXIT	5
#define C_MSG	6
#define	C_TIME	7
#define	C_GOTO	8

typedef struct cmddef
{
    uchar	c_type;
    int		c_arg1;
    int		c_arg2;
    char	*c_str1;
    char	*c_str2;
    char	*c_lbl1;
    char	*c_lbl2;
    char	*c_match;
} cmd_t;

#define MAXPGM	128
int	pctr = 0;
int	cmds = 0;
int	cline = 0;
cmd_t	pgm[MAXPGM];

#define	P_NONE		((uchar)0x00)
#define	P_ARG1		((uchar)0x01)
#define	P_ARG2		((uchar)0x02)
#define P_STR1		((uchar)0x04)
#define P_STR2		((uchar)0x08)
#define P_LBL1		((uchar)0x10)
#define P_LBL2		((uchar)0x20)
#define P_MATCH		((uchar)0x40)

typedef struct parsedef
{
    char	*p_cmd;
    uchar	p_args;
} parse_t;

/*
 * string substitution storage
 */
typedef struct strdef
{
    char	*s_string;	/* string to substitute */
    char	*s_value;	/* new value to substitute in */
} str_t;

#define MAXSTRS		100
str_t	strtab[MAXSTRS];
int	strings = 0;

/*
 * index into this array is the command type
 */
parse_t parsetab[] =
{
    { "wait:",		P_ARG1 | P_ARG2 | P_STR1 | P_STR2 |
				 P_LBL1 | P_LBL2 | P_MATCH },
    { "send:",		P_STR1 },
    { "loop:",		P_ARG1 },
    { "endloop",	P_NONE },
    { "label:",		P_LBL1 },
    { "exit:",		P_ARG1 },
    { "msg:",		P_STR1 },
    { "time:",		P_LBL1 },
    { "goto:",		P_LBL1 },
    { NULL,		P_NONE },
};

int	main(int argc, const char * const *argv, const char * const *envp);
void	usage(void);
void	rlhelp(void);
int	fgetln(FILE *fd, char *buf, int len);
void	parseline(char *line, int len);
int	findcmd(char **cur);
int	getarg(char *line, char **cur);
char	*getstr(char *line, char **cur);
char	*getlbl(char *line, char **cur);
void	skipws(char **cur);
void	serr(const char *line, const char *msg);
void	escape(char *dst, char *src);
int	otoi(char *cp);
void	unescape(char *dst, char *src);
void	itoo(char *cp, int val);
void	cmdout(cmd_t *cmdp);
void	startsh(void);
void	execute(void);
void	gotolbl(char *lbl);
void	acatch(int sig);
int	do_wait(cmd_t *cmdp);
int	search(char *buf, int len, char *searchbuf, int searchlen);
void	matchopen(char *fname);
void	matchcmp(char *buf, int len);
void	matchclose(void);
int	parsevar(const char *var);
void	procvars(char *dst, const char *src);

int
main(int argc, const char * const *argv, const char * const *envp)
{
    char ibuf[BUFSIZ];

    int i;

    argc--;
    argv++;
    while (argc > 0 && **argv == '-')
    {
	if (strcmp(*argv, "-help") == 0)
	    rlhelp();
	else if (strcmp(*argv, "-t") == 0)
	{
	    argc--;
	    argv++;
	    if (argc == 0)
		usage();
	    opt_timecount = atol(*argv);
	    if (opt_timecount <= 0)
		usage();
	}
	else if (strcmp(*argv, "-v") == 0)
	    opt_verbose = 1;
	else if (strcmp(*argv, "-V") == 0)
	    opt_verbose = 2;
	else if (strcmp(*argv, "-n") == 0)
	    opt_noexecute = 1;
	else
	    usage();
	argc--;
	argv++;
    }

    while ((argc > 0) && parsevar(*argv))
    {
	argc--;
	argv++;
    }

    if (argc > 1)
	usage();

    if (argc && ((input = fopen(*argv, "r")) == NULL))
    {
	sprintf(buf, "Unable to open %s", *argv);
	perror(buf);
	exit(1);
    }

    while ((i = fgetln(input, ibuf, sizeof(ibuf))) >= 0)
    {
	procvars(buf, ibuf);
	parseline(buf, i);
    }

    if (opt_noexecute)
	exit(0);

    timestart = time(NULL);
    startsh();
    execute();
}

void
usage()
{
    printf("usage: rl [-t n] [-vV] [-n] variable=value... [script]\n");
    printf("       rl -help will print a definition of the script language\n");
    printf("    the script may also be passed in through stdin\n");
    printf("This program forks a shell, using the next availble pseudo tty\n");
    printf("    and does I/O to it as prescribed by the script\n");
    printf("rl normally executes silently, -v echoes each script command\n");
    printf("    and -V echoes both script commands and the tty data\n");
    printf("-t passes a time to execute to the script\n");
    printf("-n is noexecute, just syntax check the script.\n");
    printf("variable=value allows string substitution from the command line\n");
    printf("    FOO=hello would substitute all occurances of FOO with hello\n");
    exit(1);
}

void
rlhelp()
{
    printf("Script command format:\n\n");
    printf("Strings must be enclosed in quotes, whitespace may be used as needed\n");
    printf("# this is a comment in the script\n");
    printf("wait:noactive,timeout,str1,lbl1,str1,lbl2[,matchfile]\n");
    printf("\twait waits for output from the shell or it's subprocesses\n");
    printf("    noactive is how long (in seconds) before the command times out if no output\n");
    printf("\tis seen and either string has not been matched in the output stream.\n");
    printf("    timeout is how long overall before the wait will timeout\n");
    printf("    str1 is a termcap format string that describes a string to be looked for.\n");
    printf("    matchfile, if present specifies a file that contains a copy\n");
    printf("    of the data you expect to receive during the wait command.\n");
    printf("    data up to the length of the file is compared.\n");
    printf("\tIf this string is found a branch to label1 is taken.\n");
    printf("    str2 is an optional string that if found indicates failure.\n");
    printf("    Any failure or timeout branches to label2.\n");
    printf("send:str\n");
    printf("    str is a termcap formatted string that is sent to the pseudo tty\n");
    printf("loop:n\n");
    printf("    loop n times\n");
    printf("endloop\n");
    printf("    end of loop construct, no nesting allowed\n");
    printf("label:str\n");
    printf("    a label is defined here. Labels are 8 characters in length maximum\n");
    printf("    Any printable character can be in a label name except : or ,.\n");
    printf("goto:str\n");
    printf("    branch to the specified label\n");
    printf("exit:n\n");
    printf("    exit with a return code\n");
    printf("msg:str\n");
    printf("    output a message to rl\'s stdout\n");
    printf("time:label\n");
    printf("    checks if the elapsed time passed via -t has passed yet\n");
    printf("    branches to label if the time has expired.\n");
    exit(0);
}

/*
 * fix up fgets to work like read
 * returns -1 on EOF though
 */

int
fgetln(FILE *fd, char *buf, int len)
{
    char *cp;
    int l;

    cp = fgets(buf, len, fd);
    if (cp == 0)
	return(-1);
    l = strlen(buf) - 1;
    *(buf + l) = 0;
    return(l);
}

void
parseline(char *line, int len)
{
    register cmd_t *cmdp;
    char *cur;
    int i;
    int cmd;
    char args;

    cline++;
    if (cmds == MAXPGM)
    {
	    fprintf(stderr, "rl: program too long > %d commands\n", MAXPGM);
	    exit(1);
    }
    if (*line == '#')
	    return;
    cur = line;
    skipws(&cur);
    i = findcmd(&cur);
    if (i < 0)
    {
	serr(line, "Invalid command");
	exit(1);
    }
    cmdp = &pgm[cmds];
    cmdp->c_type = i;
    args = parsetab[i].p_args;
    if (args & P_ARG1)
	    cmdp->c_arg1 = getarg(line, &cur);
    if (args & P_ARG2)
	    cmdp->c_arg2 = getarg(line, &cur);
    if (args & P_STR1)
	    cmdp->c_str1 = getstr(line, &cur);
    if (args & P_STR2)
	    cmdp->c_str2 = getstr(line, &cur);
    if (args & P_LBL1)
	    cmdp->c_lbl1 = getlbl(line, &cur);
    if (args & P_LBL2)
	    cmdp->c_lbl2 = getlbl(line, &cur);
    if (args & P_MATCH)
	    cmdp->c_match = getlbl(line, &cur);
    cmds++;
}

/*
 * returns the cmd type
 */
int
findcmd(char **cur)
{
    register char *cp;
    register parse_t *pp;
    int i;
    int j;

    cp = *cur;
    i = strlen(cp);
    for (pp = parsetab; pp->p_cmd; pp++)
    {
	    j = strlen(pp->p_cmd);
	    if (i < j)
		    continue;
	    if (strncmp(cp, pp->p_cmd, j)==0)
	    {
		    *cur += j;
		    return(pp - parsetab);
	    }
    }
    return(-1);
}

int
getarg(char *line, register char **cur)
{
    register char	*cp;
    char		*start;
    int			val;

    skipws(cur);
    if (**cur == 0)
	serr(line, "Missing parameter");
    cp = *cur;
    start = cp;
    if (isdigit(*cp) == 0)
	serr(line, "Bad numeric parameter");
    while (isdigit(*cp))
	    cp++;
    *cur = cp;
    val = atoi(start);
    return(val);
}

char *
getstr(char *line, char **cur)
{
    register char	*cp;
    char		*start;
    char		*str;
    char		c;

    skipws(cur);
    if (**cur != '"')
	serr(line, "Bad string parameter");
    start = cp = ++(*cur);
    while (*cp && (*cp != '"'))
	cp++;
    if (*cp != '"')
	serr(line, "Bad string parameter");
    c = *cp;
    *cp = '\0';
    str = malloc(strlen(start) + 1);
    if (pgm[cmds].c_type == C_MSG)
	strcpy(str, start);
    else
	escape(str, start);
    *cp = c;
    *cur = cp + 1;
    return(str);
}

char *
getlbl(char *line, char **cur)
{
    register char	*cp;
    char		*start;
    char		*str;
    int			c;

    skipws(cur);
    cp = *cur;
    start = cp;
    while (*cp && *cp != ',')
	    cp++;
    c = *cp;
    *cp = 0;
    str = malloc(strlen(start) + 1);
    strcpy(str, start);
    *cp = c;
    if (*cp)
	    *cur = cp + 1;
    else
	    *cur = cp;
    return(str);
}

/*
 * skip spaces leading up to an argument
 */

void
skipws(register char **cur)
{
    register char	*cp;

    cp = *cur;
    while ((*cp == ' ') || (*cp == '\t') || (*cp == ','))
	    cp++;
    *cur = cp;
}

void
serr(const char *line, const char *msg)
{
    fprintf(stderr, "rl: %s on script line %d\n", msg, cline);
    fprintf(stderr, "\t%s\n", line);
    exit(1);
}

static char bk[] = "n\nr\rt\tb\bf\f\\\\^^E\033";
static char uk[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";

void
escape(char *dst, register char *src)
{
    char		*cp;
    register int	c;

    for (; *src; src++)
    {
	c = *src;
	if (c == '\\')
	{
	    c = *(++src);
	    if (!isdigit(c))
	    {
		cp = strchr(bk, c);
		if (cp)
		    c = cp[1];
	    }
	    else
	    {
		c = otoi(src);
		while (isdigit(*++src));
		    src--;
	    }
	}
	else if (c == '^')
	{
	    c = *++src;
	    if (c == '?')
		c = 0x7f;
	    else
	    {
		cp = strchr(uk, c);
		if (cp)
		    c = cp - uk;
		else
		    c = *src;
	    }
	}
	*dst++ = c & 0x7f;
    }
    *dst = '\0';
}

int
otoi(register char *cp)
{
	register int i;

	i = 0;
	while (isdigit(*cp))
	    i = (i << 3) | (*cp++ - '0');
	return(i);
}

void
unescape(char *dst, char *src)
{
    register char c;
    register char *cp;

    for (; *src; src++)
    {
	c = *src;
	if ((c >= 0x20) && (c <= 0x7f))
	    *dst++ = c;
	else
	{
	    for (cp = bk; *cp; cp += 2)
		if (cp[1] == c)
		    break;
	    if (*cp)
	    {
		*dst++ = '\\';
		*dst++ = *cp;
		continue;
	    }

	    if ((c >= 0x01) && (c <= (sizeof(uk) - 1)))
	    {
		*dst++ = '^';
		*dst++ = uk[c];
		continue;
	    }
	    *dst++ = '\\';
	    itoo(dst, c);
	    dst += strlen(dst);
	}
    }
    *dst = '\0';
}

/*
 * Integer to octal
 */
void
itoo(char *cp, int val)
{
    int i;

    val &= 0777;
    i = val / 0100;
    *cp++ = i + '0';
    val &= 077;
    i = val / 010;
    *cp++ = i + '0';
    val &= 07;
    *cp++ = val + '0';
    *cp++ = 0;
}

/*
 * output a command as it is executed
 */
void
cmdout(register cmd_t *cmdp)
{
	char str1[BUFSIZ];
	char str2[BUFSIZ];

	if (parsetab[cmdp->c_type].p_args & P_STR1)
	    unescape(str1, cmdp->c_str1);

	if (parsetab[cmdp->c_type].p_args & P_ARG2)
	    unescape(str2, cmdp->c_str2);

	switch (cmdp->c_type)
	{
	    case C_WAIT:
		    printf("wait:%d:%d:\"%s\",\"%s\",%s,%s", 
			    cmdp->c_arg1, cmdp->c_arg2,
			    str1, str2,
			    cmdp->c_lbl1, cmdp->c_lbl2);
		    if (*cmdp->c_match)
			    printf(",%s", cmdp->c_match);
		    printf("\n");
		    break;

	    case C_SEND:
		    printf("send:%s\n", str1);
		    break;
	    case C_LOOP:
		    printf("loop:%d\n", cmdp->c_arg1);
		    break;
	    case C_ENDLP:
		    printf("endloop\n");
		    break;
	    case C_LABEL:
		    printf("label:%s\n", cmdp->c_lbl1);
		    break;
	    case C_EXIT:
		    printf("exit:%d\n", cmdp->c_arg1);
		    break;
	    case C_MSG:
		    printf("msg:\"%s\"\n", cmdp->c_str1);
		    break;
	    case C_TIME:
		    printf("time:%s\n", cmdp->c_lbl1);
		    break;
	    case C_GOTO:
		    printf("goto:%s\n", cmdp->c_lbl1);
		    break;
	}
}

/*
 * grab a pseudo tty and start a shell on it
 * don't bother to do any signal processing
 * let sighup on the pseudo tty handle it.
 */

void
startsh()
{
	int tty;
	int i;
	char ptty[40];

	for (tty = 0; tty < 128; tty++)
	{
		sprintf(ptty, "/dev/ptyp%d", tty);
		shellfd = open(ptty, 2);
		if (shellfd >= 0)
			break;
	}
	if (shellfd < 0)
	{
		fprintf(stderr, "rl: Unable to allocate a pseudo tty\n");
		exit(1);
	}
	if (opt_verbose)
		printf("rl: Shell started on %s\n", ptty);
	if (fork() == 0)
	{
		/* make child of process 1 */
		if (fork() == 0)
		{
			for (i = 0; i < 20; i++)
				close(i);
			setpgrp();
			ptty[5] = 't';
			if (open(ptty, 2) < 0)
				exit(0);
			dup(0);
			dup(0);
			execl("/bin/sh", "-sh", NULL);
		}
		exit(0);
	}
	wait(NULL);
}

void
execute()
{
    register cmd_t *cmdp;
    cmd_t *tp;
    int bumppc;

    while (pctr < cmds)
    {
	cmdp = &pgm[pctr];
	bumppc = 1;
	if (opt_verbose)
		cmdout(cmdp);
	switch (cmdp->c_type)
	{
	    case C_WAIT:
		bumppc = do_wait(cmdp);
		break;
	    case C_SEND:
		write(shellfd, cmdp->c_str1, strlen(cmdp->c_str1));
		break;
	    case C_LOOP:
		/* use arg2 as loop counter */
		loopcnt = 1;
		cmdp->c_arg2 = cmdp->c_arg1;
		break;
	    case C_ENDLP:
		for (tp = cmdp - 1; tp >= pgm; tp--)
		    if (tp->c_type == C_LOOP)
			break;
		if (tp < pgm)
		{
		    fprintf(stderr, "rl: ENDLOOP without LOOP\n");
		    exit(1);
		}
		tp->c_arg2--;
		if (tp->c_arg2 > 0)
		{
		    bumppc = 0;
		    tp++;
		    pctr = tp - pgm;
		    loopcnt++;
		}
		break;
	    case C_LABEL:
		break;
	    case C_EXIT:
		exit(cmdp->c_arg1);
		break;
	    case C_MSG:
		printf(cmdp->c_str1, loopcnt);
		printf("\n");
		break;
	    case C_TIME:
		if (opt_timecount && ((time(NULL) - timestart) > opt_timecount))
		    gotolbl(cmdp->c_lbl1);
		break;
	    case C_GOTO:
		gotolbl(cmdp->c_lbl1);
		break;
	}
	if (bumppc)
	    pctr++;
    }
    fprintf(stderr, "rl: Program did not end with an exit statement\n");
    exit(1);
}

void
gotolbl(char *lbl)
{
    register cmd_t *cmdp;
    register cmd_t *tp;

    cmdp = &pgm[pctr];
    for (tp = pgm; tp < &pgm[cmds]; tp++)
    {
	    if (tp->c_type != C_LABEL)
		    continue;
	    if (strcmp(lbl, tp->c_lbl1) == 0)
		    break;
    }
    if (tp == &pgm[cmds])
    {
	    fprintf(stderr, "rl: label %s not found\n", cmdp->c_lbl1);
	    exit(1);
    }
    pctr = tp - pgm;
    if (opt_verbose)
	    cmdout(&pgm[pctr]);
}

void
acatch(int sig)
{
}

/*
 * process wait command
 */

int
do_wait(cmd_t *cmdp)
{
    int		status[2];
    int		l1, l2, len;
    long	start;		/* time we started in wait */
    long	last;		/* last time we got some data */
    int		timer;		/* flag if we need to check for timeouts */
    long	t;			/* current time */
    int		len1;
    int		len2;
    int		bumppc;
    int		slen;
    char	*errmesg;

    /* wait for a pattern match */

    l1 = 0;
    bumppc = 0;
    timer = 0;
    if ((cmdp->c_arg1) || (cmdp->c_arg2))
	timer = 1;

    len1 = strlen(cmdp->c_str1);
    len2 = strlen(cmdp->c_str2);
    slen = len1;
    if (len2 > len1)
	    slen = len2;
    if (*cmdp->c_match)
	    matchopen(cmdp->c_match);

    start = last = time(NULL);
    for (;;)
    {
	if (timer)
	{
	    signal(SIGALRM, acatch);
	    alarm(10);
	}

	l2 = read(shellfd, &buf[l1], BUFSIZ);

	if (timer)
	    alarm(0);

	t = time(NULL);
	if (opt_verbose == 2)
	{
	    fflush(stdout);
	    write(1, ctime(&t), 24);
	    write(1, ":recv<", 6);
	    if (l2 > 0)
		{
		write(1, &buf[l1], l2);
		write(1, ">\n", 2);
		}
	    else if (l2 < 0)
		{
		write(1, "> ", 2);
		errmesg = strerror(errno);
		write(1, errmesg, strlen(errmesg));
		write(1, "\n", 1);
		}
	    else
		write(1, ">\n", 2);
	}

	/*
	 * Check for active timeout
	 */

	if (l2 > 0)
	{
	    last = t;
	}
	else if (cmdp->c_arg1 && ((t - last) >= cmdp->c_arg1))
	{
	    if (opt_verbose == 2)
		write(1, "Timeout on last\n", 16);
	    bumppc = 1;
	    break;
	}

	if (cmdp->c_arg2 && ((t - start) >= cmdp->c_arg2))
	{
	    if (opt_verbose == 2)
		write(1, "Timeout on start\n", 17);
	    bumppc = 1;
	    break;
	}

	if ((l2 < 0) && (errno == EINTR))
	    continue;

	/* child proc died */
	if (l2 == 0)
	{
	    fprintf(stderr, "rl: Shell process quit unexpectedly\n");
	    exit(1);
	}

	if (*cmdp->c_match)
	    matchcmp(&buf[l1], l2);

	len = l1 + l2;
	if (cmdp->c_str1[0] && search(buf, len, cmdp->c_str1, len1))
	{
		gotolbl(cmdp->c_lbl1);
		break;
	}
	if (cmdp->c_str2[0] && search(buf, len, cmdp->c_str2, len2))
	{
		gotolbl(cmdp->c_lbl2);
		break;
	}
	l1 = l1 + l2;
	if (l1 > slen)
	{
		l1 = l1 - slen;
		l2 = slen;
		memcpy(buf, &buf[l1], l2);
		l1 = l2;
	}
    }

    if (timer)
	alarm(0);
    if (*cmdp->c_match)
	    matchclose();

    return bumppc;
}

int
search(register char *buf, int len, register char *searchbuf, int searchlen)
{
	register int i;
	register int count;

	if (len < searchlen)
		return(0);
	count = len - searchlen + 1;
	for (i = 0; i < count; (i++, buf++))
	{
		if (*buf != *searchbuf)
			continue;
		if (memcmp(buf, searchbuf, searchlen) == 0)
			return(1);
	}
	return(0);
}

void
matchopen(char *fname)
{
	matchfd = fopen(fname, "r");
	if (matchfd == 0)
	{
		sprintf(buf, "rl: Unable to open match file %s", fname);
		perror(buf);
		exit(1);
	}
}

void
matchcmp(char *buf, int len)
{
	register char *cp;
	register char *cp1;
	int i;
	int nlen;

	if (matchfd == 0)
		return;
	cp = buf;
	cp1 = mbuf1;
	i = len;
	nlen = 0;
	while (i-- > 0)
	{
		/* remove carriage returns to match UNIX file storage methods */
		if (*cp == '\r')
		{
			cp++;
			continue;
		}
		*cp1++ = *cp++;
		nlen++;
	}
	/* no data left over */
	if (nlen == 0)
		return;
	i = fread(mbuf2, 1, nlen, matchfd);
	if (i != nlen)
	{
		if (i == 0)
		{
			matchfd = 0;
			return;
		}
		nlen = i;
	}
	if (memcmp(mbuf1, mbuf2, nlen))
	{
		fprintf(stderr, "rl: match failed, data mismatch\n");
		exit(1);
	}
}

void
matchclose()
{
	fclose(matchfd);
	matchfd = 0;
}

int
parsevar(const char *var)
{
	register char *cp;
	register str_t *sp;

	if ((cp = strchr(var, '=')) == 0)
		return(0);
	if (strings == MAXSTRS)
	{
	    fprintf(stderr, "rl: Too many string substitutes > %d\n", MAXSTRS);
	    exit(1);
	}
	sp = &strtab[strings++];
	*cp++ = '\0';
	sp->s_string = strdup(var);
	sp->s_value = strdup(cp);
	return(1);
}

/*
 * do string substitution
 */
void
procvars(char *dst, const char *src)
{
    register str_t *sp;

    while (*src)
    {
	for (sp = strtab; sp < &strtab[strings]; sp++)
		if (!memcmp(src, sp->s_string, strlen(sp->s_string)))
			break;
	if (sp == &strtab[strings])
	{
		*dst++ = *src++;
		continue;
	}
	/* got a match */
	src += strlen(sp->s_string);
	strcpy(dst, sp->s_value);
	dst += strlen(dst);
    }
    *dst = 0;
}

