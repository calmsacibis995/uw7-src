#ident "@(#)util.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>

#ifdef _USE_PATHS
#include <paths.h>
#else
#define _PATH_DEVTTY "/dev/tty"
#endif

void
error(int code, char *fmt, ...)
{
	va_list argp;
	extern char *myname;

	fprintf(stderr, "%s: ", myname);
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	if (fmt[strlen(fmt) - 1] != '\n') {
		putc('\n', stderr);
	}
	exit(code);
}

static struct termios otios;

static void
catch(int sig)
{
	tcsetattr(0, TCSANOW, &otios);
	exit(1);
}

char *
read_password(void)
{
	struct termios tios;
	FILE *ttyfp;
	void (*handler)(int);
	int tty, len;
	char *p, *ret;
	static char buf[256];

	if (tty = isatty(0)) {
		if (!(ttyfp = fopen(_PATH_DEVTTY, "w"))) {
			error(99, "%s: %s", _PATH_DEVTTY, strerror(errno));
		}
		fprintf(ttyfp, "Enter password: ");
		fflush(ttyfp);
		tcgetattr(0, &otios);
		tios = otios;
		tios.c_lflag &= ~ECHO;
		handler = sigset(SIGINT, catch);
		tcsetattr(0, TCSANOW, &tios);
	}

	if (!fgets(buf, sizeof(buf), stdin)) {
		ret = NULL;
	}
	else {
		len = strlen(buf);
		if (buf[len - 1] == '\n') {
			buf[len - 1] = '\0';
		}
		ret = buf;
	}

	if (tty) {
		tcsetattr(0, TCSANOW, &otios);
		(void) sigset(SIGINT, handler);
		putc('\n', ttyfp);
		fclose(ttyfp);
	}

	return ret;
}
