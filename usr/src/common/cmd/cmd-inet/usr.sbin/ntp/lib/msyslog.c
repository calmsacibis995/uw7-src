#ident "@(#)msyslog.c	1.2"

/*
 * msyslog - either send a message to the terminal or print it on
 *	     the standard output.
 *
 * Converted to use varargs, much better ... jks
 */
#include <stdio.h>
#include <errno.h>

/* alternative, as Solaris 2.x defines __STDC__ as 0 in a largely standard
   conforming environment
   #if __STDC__ || (defined(SYS_SOLARIS) && defined(__STDC__))
*/
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef _POSIX_SOURCE
#include <unistd.h>
#endif

#include "ntp_types.h"
#include "ntp_string.h"
#include "ntp_stdlib.h"
#include "ntp_syslog.h"

#undef syslog

int syslogit = 1;
FILE *syslog_file = NULL;

u_long ntp_syslogmask = ~0;

#ifndef VMS
#ifndef SYS_WINNT
extern	int errno;
#else
HANDLE  hEventSource;
LPTSTR lpszStrings[1];
static WORD event_type[] = {
	EVENTLOG_ERROR_TYPE, EVENTLOG_ERROR_TYPE, EVENTLOG_ERROR_TYPE, EVENTLOG_ERROR_TYPE,
	EVENTLOG_WARNING_TYPE,
	EVENTLOG_INFORMATION_TYPE, EVENTLOG_INFORMATION_TYPE, EVENTLOG_INFORMATION_TYPE,
};
#endif /* SYS_WINNT */
#endif /* VMS */
extern	char *progname;

#if defined(__STDC__)
void msyslog(int level, char *fmt, ...)
#else
/*VARARGS*/
void msyslog(va_alist)
	va_dcl
#endif
{
#ifndef __STDC__
	int level;
	char *fmt;
#endif
	va_list ap;
	char buf[1025], nfmt[256], xerr[50];
	const char *err;
	register int c, l;
	register char *n, *f, *prog;
#if !defined(SYS_44BSD) && !defined(SYS_WINNT) && !defined(VMS)
	extern int sys_nerr;
	extern char *sys_errlist[];
#endif
	int olderrno;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);

	level = va_arg(ap, int);
	fmt = va_arg(ap, char *);
#endif

#ifndef SYS_WINNT
	olderrno = errno;
#endif /* SYS_WINNT */
	n = nfmt;
	f = fmt;
	while ((c = *f++) != '\0' && c != '\n' && n < &nfmt[252]) {
		if (c != '%') {
			*n++ = c;
			continue;
		}
		if ((c = *f++) != 'm') {
			*n++ = '%';
			*n++ = c;
			continue;
		}
#ifndef SYS_WINNT
#if !defined(VMS)
		if ((unsigned)olderrno > sys_nerr)
			sprintf((char *)(err = xerr), "error %d", olderrno);
		else
			err = sys_errlist[olderrno];
#else
		err = strerror(olderrno);
#endif /* VMS */
		if (n + (l = strlen(err)) < &nfmt[254]) {
			strcpy(n, err);
			n += strlen(err);
		}
#else
		{
			sprintf(xerr, "error code = %u", GetLastError());
			strcpy(n, xerr);
			n += strlen(xerr);
		}
#endif /* SYS_WINNT	*/
	}
#if !defined(VMS)
	if (!syslogit)
#endif /* VMS */
	  *n++ = '\n';
	*n = '\0';

	vsprintf(buf, nfmt, ap);
#if !defined(VMS)
	if (syslogit)
#ifndef SYS_WINNT
		syslog(level, "%s", buf);
#else
	{
		hEventSource = RegisterEventSource(NULL, TEXT("NTP"));
		lpszStrings[0] = buf;
        	if (hEventSource != NULL) {
        		ReportEvent(hEventSource, /* event log handle */
        		event_type[level],    /* event type */
            		0,                    /* event category */
            		0,                    /* event ID */
            		NULL,                 /* current user's SID */
            		1,                    /* one substitution string */
            		0,                    /* no bytes of raw data */
            		lpszStrings,          /* array of error strings */
            		NULL);                /* no raw data */
		}
        	(VOID) DeregisterEventSource(hEventSource);
	}
#endif /* SYS_WINNT */
	else {
#else
	{
#endif /* VMS */
		extern char * humanlogtime P((void));

	        FILE *out_file = syslog_file ? syslog_file
  	                                  : level <= LOG_ERR ? stderr : stdout;
  	        /* syslog() provides the timestamp, so if we're not using
  	           syslog, we must provide it. */
		prog = strrchr(progname, '/');
		if (prog == NULL)
		  prog = progname;
		else
		  prog++;
		(void) fprintf(out_file, "%s ", humanlogtime ());
                (void) fprintf(out_file, "%s[%d]: %s", prog, getpid(), buf);
		fflush (out_file);
	}
	va_end(ap);
}
