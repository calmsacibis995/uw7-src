#ident	"@(#)cron:common/cmd/cron/cronfuncs.c	1.1.1.1"

/*
 * Routines used by cron and by one or more of the
 * other commands (at, crontab, etc.) in the cron family.
 */

#include <ctype.h>
#include <errno.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "cron.h"

/*
 * Return the decimal number corresponding to the
 * leading digits of the specified string,
 * 0 if the first character is not a digit.
 * Also advance the string pointer to point to
 * the first non-numeric character of the string.
 */

time_t
num(ptr)
	char **ptr;
{
	time_t n = 0;

	while (isdigit(**ptr)) {
		n = n*10 + (**ptr - '0');
		*ptr += 1;
	}
	return(n);
}

/*
 * Calculate the number of "full" days between m1/d1/y1 and m2/d2/y2.
 * NOTE: there should not be more than a year separation in the dates.
 * Also, m should be in 0 to 11, and d should be in 1 to 31.
 */

static int dom[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int
days_btwn(m1, d1, y1, m2, d2, y2)
	int m1, d1, y1, m2, d2, y2;
{
	int days, m;

	if (m1 == m2 && d1 == d2 && y1 == y2)
		return(0);
	if (m1 == m2 && d1 < d2)
		return(d2-d1-1);

	/* the remaining dates are on different months */
	days = (days_in_mon(m1, y1) - d1) + (d2-1);
	m = (m1 + 1) % 12;
	while (m != m2) {
		if (m == 0)
			y1++;
		days += days_in_mon(m, y1);
		m = (m + 1) % 12;
	}
	return(days);
}


/*
 * Returns the number of days in month m of year y.
 * NOTE: m should be in the range 0 to 11.
 */

int
days_in_mon(m, y)
	int m, y;
{
	return( dom[m] + (((m == 1) && ((y%4) == 0)) ? 1 : 0 ));
}

/*
 * Allocate memory and abort on error.
 */

char *
xmalloc(size)
	int	size;
{
	char	*p;

	if ((p = (char *)malloc(size)) == NULL) {
		pfmt(stdout, MM_ERROR,
			":31:Out of memory: %s\n", strerror(errno));
		fflush(stdout);
		exit(55);
	}
	return(p);
}

/*
 * Return the path to a shell that is suitable to use to execute
 * crontab/at jobs.
 *
 * If an argument is passed, it will be returned if suitable.
 */
char *select_shell(const char *s) {
char *t;
	if (s == NULL || (t = strrchr(s, '/')) == NULL || strcmp(t,"/sh") != 0)
		return SHELL;
	else
		return (char *) s;
}

/******************* memory debugging code follows *******************/

#ifdef MDEBUG

#include "cron.h"

#define NTHING	1000

static struct xxthing {
	char	*addr;	/* address from xmalloc, 0 if this slot never used */
	int	size;	/* size allocated; set to -1 when freed */
	int	type;	/* type of thing allocated */
} xxthing[NTHING];

static int xxthings = 0;
static int xxmemory = 0;

static char *typestring[] = {
	"unknown type 0",	/*  0 = not valid */
	"at cmd",		/*  1 = M_ATCMD */
	"at event struct",	/*  2 = M_ATEVENT */
	"cron cmd",		/*  3 = M_CRCMD */
	"cron event struct",	/*  4 = M_CREVENT */
	"cron time field",	/*  5 = M_CRFIELD */
	"elm struct",		/*  6 = M_ELM */
	"home dir",		/*  7 = M_HOME */
	"cron input",		/*  8 = M_IN */
	"runinfo jobname",	/*  9 = M_JOB */
	"runinfo outfile",	/* 10 = M_OUTFILE */
	"msg_wait recbuf",	/* 11 = M_RECBUF */
	"runinfo user name",	/* 12 = M_RNAME */
	"user name",		/* 13 = M_UNAME */
	"usr struct",		/* 14 = M_USR */
};

#define NTYPE (sizeof(typestring) / sizeof(typestring[0]))

static char *
xxtype(i)
	int	i;
{
	static char buf[25];

	if (i >= 0 && i < NTYPE)
		return(typestring[i]);

	sprintf(buf, "unknown type %d", i);
	return(buf);
}

char *
xxmalloc(size, type)
	int	size;
	int	type;
{
	char	*p;
	int	i;

	if (xxthings >= NTHING) {
		fprintf(stderr, "too many MALLOCs!\n");
		exit(56);
	}

	if (size <= 0 || type <= 0 || type >= NTYPE) {
		fprintf(stderr, "bad MALLOC(%d, %d)!\n", size, type);
		exit(57);
	}

	p = xmalloc(size);

	for (i = xxthings; i < NTHING && xxthing[i].addr != 0; i++)
		;

	if (i >= NTHING)
		for (i = 0; xxthing[i].size != -1; i++)
			;

	xxthing[i].addr = p;
	xxthing[i].size = size;
	xxthing[i].type = type;
	xxthings++;
	xxmemory += size;

	return(p);
}

void
xxfree(addr, type)
	char	*addr;
	int	type;
{
	int	i;

	for (i = 0; i < NTHING; i++)
		if (xxthing[i].addr == addr && xxthing[i].size > 0)
			break;

	if (i >= NTHING)
		for (i = 0; i < NTHING && xxthing[i].addr != addr; i++)
			;

	if (i >= NTHING) {
		fprintf(stderr, "xxfree(0x%X, %s) address not found??\n",
			addr, xxtype(type));
	} else if (xxthing[i].size == -1) {
		fprintf(stderr, "xxfree(0x%X, %s) already freed??\n",
			addr, xxtype(type));
	} else {
		if (xxthing[i].type != type)
			fprintf(stderr, "xxfree(0x%X, %s) was %s??\n",
				addr, xxtype(type), xxtype(xxthing[i].type));
		free(addr);
		xxthings--;
		xxmemory -= xxthing[i].size;
		xxthing[i].size = -1;
	}
}

char *
xxtempnam(dir, pfx, type)
	char	*dir;
	char	*pfx;
	int	type;
{
	char	*p;
	char	*q;

	if ((p = tempnam(dir, pfx)) == NULL)
		return(NULL);

	q = xxmalloc(strlen(p) + 1, type);
	strcpy(q, p);
	free(p);
	return(q);
}

void
xxmem()
{
	fprintf(stderr, "%d items allocated, %d bytes total\n",
		xxthings, xxmemory);
}

static char *
xxstr(s)
	char	*s;
{
	int	l;
	char	c;
	char	*p;
	static char buf[75];

	strcpy(buf, (l = strlen(s)) > 35 ? "\n     \"" : "\"");
	strncat(buf, s, sizeof(buf) - sizeof("\n     \"\""));

	if (l > sizeof(buf) - sizeof("\n     \"...\""))
		strcat(buf, "...");

	for (p = buf; (c = *++p) != '\0'; )
		if (!isascii(c) || iscntrl(c))
			*p = '?';
		else if (isspace(c))
			*p = ' ';

	strcat(p, "\"");
	return(buf);
}

void
xxdump(i)
	int	i;
{
	char	*addr;
	char	timebuf[80];

	if (i == -1) {
		xxmem();
		fprintf(stderr, "\n");

		for (i = 0; i < NTHING; i++)
			if (xxthing[i].addr != 0 && xxthing[i].size != -1)
				xxdump(i);

		fprintf(stderr, "\n");
		return;
	}

	addr = xxthing[i].addr;

	if (addr == 0) {
		fprintf(stderr, "%3d: unused slot\n", i);
		return;
	}

	if (xxthing[i].size == -1) {
		fprintf(stderr, "%3d: 0x%08X, freed, %s\n",
			i, addr, xxtype(xxthing[i].type));
		return;
	}

	fprintf(stderr, "%3d: 0x%08X, %d bytes, %s",
		i, addr, xxthing[i].size, xxtype(xxthing[i].type));

	switch (xxthing[i].type) {
	case M_ATCMD:
	case M_CRFIELD:
	case M_CRCMD:
	case M_HOME:
	case M_IN:
	case M_JOB:
	case M_OUTFILE:
	case M_RNAME:
	case M_UNAME:
		fprintf(stderr, " = %s\n", xxstr(addr));
		break;

	case M_ATEVENT:
	case M_CREVENT:
		/* we "know" event time is first thing in structure */
		cftime(timebuf, "%a %b %e %H:%M:%S %Y", (time_t *)addr);
		fprintf(stderr, " (%s)\n", timebuf);
		break;

	case M_USR:
		/* we "know" user name is first thing in structure */
		fprintf(stderr, " (for %s)\n", xxstr(*(char **)addr));
		break;

	default:
		fprintf(stderr, "\n");
		break;
	}
}

#endif
