/*
 * File server.c
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) server.c 11.1 95/05/01 SCOINC")

#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include "dlpiut.h"

static void	srvdone(int code);
static void	serverinit(void);
static void	errinit(void);
static int	errchkovfl(int len);
static void	errout(void);

#define SAFEBUF		1024	/* Safety in case of buffer overflow */
static char	errinfo[MAXMSG+SAFEBUF] = "";
static int	errovfl = 0;

fd_t	fds[MAXFD];

int
server()
{
	int l;
	int i;
	uchar *cp;

	serverinit();

	/* main server loop */
	for (;;) {
		l = mrecv(sfd, ibuf, MAXMSG);

		cfd = mopen(clientname);
		if (cfd < 0)
			srvdone(0);

		obuf[0] = 0;
		errinit();
		if (parse()) {
			if (verify())
				execute();
		}

		/* output any err messages */
		errout();
		/* return our message, could be a one byte null message */
		msend(cfd, obuf, strlen(obuf) + 1);

		mclose(cfd);
		if (srvterm)
			srvdone(0);
	}
}

static void
serverinit()
{
	int		fd;

	signal(SIGINT, srvdone);
	sfd = mcreat(servername);
	if (sfd < 0)
		srvdone(0);

	memset((void *)fds, 0, sizeof(fds));
}

static void
srvdone(int code)
{
	if (sfd >= 0)
		mdelete(sfd);
	if (cfd >= 0)
		mclose(cfd);

	exit(code);
}

/*
 * our error log function
 *
 * ability to accumulate a bunch of messages
 * to be sent back to the user
 */

static void
errinit()
{
	errinfo[0] = '\0';
	errovfl = 0;
}

void
errlogprf(const char *Fmt, ...)
{
	int		i;
	va_list	Args;

	if (!Fmt || !*Fmt)
		return;

	if (errchkovfl(i = strlen(errinfo)))
		return;

	/*
	 * If there is something already in this log
	 * line, format this message on a new line.
	 */
	if (i && (errinfo[i-1] != '\n'))
		errinfo[i++] = '\n';

	va_start(Args, Fmt);
	vsprintf(&errinfo[i], Fmt, Args);
	va_end(Args);
}

static int
errchkovfl(int len)
{
	if (errovfl)
		return 1;

	if ((len + 128) > MAXMSG) {
		if (errovfl == 0) {
			strcpy(&errinfo[len], "\nErrlog buffer overflow");
			errovfl = 1;
		}
		return(1);
	}
	return(0);
}

static void
errout()
{
	if (!errinfo[0])
		return;

	sprintf(&obuf[strlen(obuf)], "LOG='%s'\n", errinfo);
}

void
error(const char *Fmt, ...)
{
	char	buf[256];
	va_list	Args;

	va_start(Args, Fmt);
	vsprintf(buf, Fmt, Args);
	va_end(Args);

	sprintf(&obuf[strlen(obuf)], "ERROR='%s'\n", buf);
}

/*
 * format our non-matching multicast table into the obuf area
 */

void
tableout(uchar *ptr, int len)
{
	int		i;
	uchar	*cp;
	char	*ocp;

	ocp = ibuf;
	strcpy(ocp, "actual table:");
	ocp += strlen(ocp);
	cp = ptr;
	for (i = 0; i < len; i += ADDR_LEN) {
		sprintf(ocp, "%02x%02x%02x%02x%02x%02x",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
		cp += ADDR_LEN;
		ocp += strlen(ocp);
		if ((i + ADDR_LEN) < len)
			*ocp++ = ',';
	}
	*ocp++ = '\n';
	strcpy(ocp, "desired table:");
	ocp += strlen(ocp);
	cp = &cmd.c_table[0][0];
	for (i = 0; i < cmd.c_tabsiz; i++) {
		sprintf(ocp, "%02x%02x%02x%02x%02x%02x",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
		cp += ADDR_LEN;
		ocp += strlen(ocp);
		if ((i + 1) < cmd.c_tabsiz)
			*ocp++ = ',';
	}
	*ocp++ = 0;
	errlogprf(ibuf);
}

void
varout(char *var, char *msg)
{
	sprintf(&obuf[strlen(obuf)], "%s='%s'\n", var, msg);
}

