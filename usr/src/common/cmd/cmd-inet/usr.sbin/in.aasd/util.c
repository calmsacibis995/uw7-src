#ident "@(#)util.c	1.3"
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
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#ifdef _USE_PATHS
#include <paths.h>
#else
#define _PATH_CONSOLE	"/dev/console"
#endif

extern int debug;

/*
 * report -- syslog message or write to stderr if we're not running
 * in the background.
 */

void
report(int pri, char *fmt, ...)
{
	char buf[1024], *p, *bp;
	const char *fp;
	va_list argp;
	extern int sys_nerr;
	extern char *sys_errlist[];

	va_start(argp, fmt);

	if (debug >= 3) {
		/*
		 * Replace %m with the error message.  This only looks for
		 * one %m.
		 */
		
		if (p = strstr(fmt, "%m")) {
			if (p > fmt) {
				memcpy(buf, fmt, p - fmt);
				bp = &buf[p - fmt];
			}
			else {
				bp = buf;
			}
			if (errno < sys_nerr) {
				strcpy(bp, sys_errlist[errno]);
			}
			else {
				sprintf(bp, "Error %d", errno);
			}
			strcat(bp, p + 2);
			fp = buf;
		}
		else {
			fp = fmt;
		}

		fprintf(stderr, "aasd: ");
		vfprintf(stderr, fp, argp);
		putc('\n', stderr);
	}
	else {
		vsyslog(pri, fmt, argp);
	}

	va_end(argp);
}

/*
 * malloc_error -- log an error message about a malloc error
 * A message is logged using report().  If we are not logging to stderr,
 * this message is also written to the console.  This is done because
 * syslog uses malloc, so the syslog itself may fail.
 */

void
malloc_error(char *where)
{
	static char buf[256];
	int cons;

	if (debug < 3) {
		if ((cons = open(_PATH_CONSOLE, O_WRONLY)) != -1) {
			sprintf(buf, "aasd: %s: Memory allocation failed.\n",
				where);
			write(cons, buf, strlen(buf));
			close(cons);
		}
	}
	report(LOG_ERR, "%s: Unable to allocate memory: %m", where);
}

char *
binary2string(u_char *buf, int len)
{
	static char str[1024];
	char *p;
	int i;

	str[0] = '\0';
	for (i = 0, p = str; i < len; i++, p += 2) {
		sprintf(p, "%02x", buf[i]);
	}

	return str;
}
