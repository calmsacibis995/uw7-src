#ident	"@(#)pcintf:bridge/log.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)log.c	6.9	LCC);	/* Modified: 14:18:00 10/17/91 */

/****************************************************************************

	Copyright (c) 1985 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

****************************************************************************/

#include "sysconfig.h"

#include <errno.h>
#include <fcntl.h>
#include <lmf.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#if defined(__STDC__)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

#include "log.h"
#include "common.h"

#ifdef	VERBOSE_LOG
#define	dbgInit		~(DBG_LOG | DBG_RLOG)
#else
#define	dbgInit		0
#endif	/* VERBOSE_LOG */

unsigned long
	dbgEnable = dbgInit;		/* Debug channel enables */

extern char
	*myname;			/* Name of program */

FILE
	*logFile = NULL;                       /* Log file stream */

char logName[64];			/* Full log file name */
static char savlogName[64];

#ifndef	JANUS
LOCAL void	popenSafe	PROTO((void));
#endif	/* not JANUS */

#if !defined(NOLOG)
void		logv		PROTO((char *, va_list));
#endif	/* !NOLOG */

/*
   In order to create a clean and controlled interface to the logging
   calls, it is necessary to have access to the internal printf interface
   which accepts a pointer to an argument vector, instead of an argument
   list itself.  This is C-library dependant.
*/
#undef	prfKnown

#ifdef	SYS5
#define	prfKnown			/* Sys5 has vfprintf in library */
#ifdef	IX370
#define	vfprintf(file,fmt,aList)        _doprnt(fmt,aList,file)
#endif	/* IX370 */
#endif	/* SYS5 */

#ifdef	DOPRNT41
#define	vfprintf(file, fmt, aList)	_doprnt(fmt, aList, file)
#define	prfKnown
#endif	/* DOPRNT41 */

#ifndef	prfKnown
#include	"log: varargs printf interface unknown"
#endif	/* !prfKnown */


#ifndef NOLOG
/*
   logOpen: Open a log file
*/

void
logOpen(logBase, logPid)
char
	*logBase;			/* Base name of log file to create */
int
	logPid;				/* Pid for name extension */
{
		
	/* If logBase is not a full path, it is prefixed with LOGDIR. */
	/* If logPid is non-zero, it is appended to logName. */
	if (*logBase == '/') {
		if (logPid)
			sprintf(logName, "%s.%d", logBase, logPid);
		else
			strncpy(logName, logBase, 64);
	} else {
		if (logPid)
			sprintf(logName, "%s/%s.%d", LOGDIR, logBase, logPid);
		else
			sprintf(logName, "%s/%s", LOGDIR, logBase);
	}

	strcpy(savlogName, logName);

	/* Close an already existing log file */
	if (logFile != NULL)
		fclose(logFile);

	logFile = fopen(logName, "a");
	if (logFile == NULL) {
		serious(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("LOG1",
			"logOpen:error opening %1\n"),
			"%s", logName));
	}
	
	/* KLO0168  - begin */
	if (chmod(logName,0600) < 0) {
		serious(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("LOG2",
			"logOpen:chmod error on %1, errno %2\n"),
			"%s%d", logName,errno));
	}
	/* KLO0168  - end */

	log("logOpen: savlogName = %s\n", savlogName);
}
#ifdef	JANUS				/* Generic Merge support. */
/* The JANUS version does not use the above "logOpen()" function in
** the normal case.  Thus the following function "register_logname" is
** used to set logName and savlogName, which are only used internally
** in this file.
*/
void
register_logname(filename)
char *filename;
{
	strncpy(logName, filename, sizeof(logName));
	logName[sizeof(logName)-1] = '\0';
	strcpy(savlogName, logName);
}
#endif	/* JANUS */

/* KLO0168 - begin */
void
logChown(uid,gid)
uid_t uid;
gid_t gid;
{
	if (logFile) {
		if (chown(logName,uid,gid) < 0) {
			serious(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("LOG3",
				"logChown:chown error %1 %2 %3,errno %4\n"),
				"%s%d%d%d" , logName,uid,gid,errno));
		}
	}
}
/* KLO0168 - end */

/*
   logDOpen: Establish a unix file descriptor as the log stream
*/

void
logDOpen(logDesc)
int
	logDesc;
{
	if (logFile != NULL)
		fclose(logFile);

	logFile = fdopen(logDesc, "a");
	if (logFile == NULL) {
		serious(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("LOG4",
			"logDOpen:can't fdopen %1\n"),
			"%d", logDesc));
	}
}


void
logClose()
{
	if (logFile == NULL)
		return;

	fclose(logFile);
	logFile = (FILE *)NULL;
}


/*
   log: Log message to standard log file
*/

#if defined(__STDC__)
int log(const char *fmt, ...)
#else
int log(va_alist)
	va_dcl
#endif
{
#if !defined(__STDC__)
	char		*fmt;		/* printf-style format string */
#endif
	va_list		args;
	register int	prfRet;		/* Return value from printf */
	static int	efbig_err = FALSE;
	int tmp;

	/* Only print logs if enabled and logFile is open */
	if (!logsOn())
		return 0;

/*
 *	If the log file is too big but we dont know the log name then
 *	return. This is for the dosout process which doesn't know the 
 *	log name whereas dossvr does, so we'll wait for dossvr to log
 *	and it can truncate the log file.
 *	We do this here because if we continue to write to the file
 *	when it is already to big software signals will get sent to us
 *	and they will interupt the pipe reads done by dosout and everybody
 *	will become unnecessarily confused.
 */
	if (efbig_err == TRUE && savlogName[0] == '\0')
		return 1;

#if defined(__STDC__)
	va_start(args, fmt);
#else
	va_start(args);
	fmt = va_arg(args, char *);
#endif
	prfRet = vfprintf(logFile, fmt, args);
	va_end(args);

	/* Flush output immediately */
	if ((fflush(logFile)) == EOF) { /* file to big, truncate it */
		if (errno == EFBIG) {
			efbig_err = TRUE;
			if (savlogName[0] != '\0') { /* make sure we know who */
				logClose();
				do
					tmp = open(savlogName, O_TRUNC);
				while (tmp == -1 && errno == EINTR);
				close(tmp);
				logFile = fopen(savlogName, "a");
				if (logFile == NULL) {
					serious(lmf_format_string((char *) NULL,
					  0, lmf_get_message("LOG5",
					  "error re-opening %1 errno = %2\n"),
					  "%s%d" , savlogName, errno));
				}
				efbig_err = FALSE;
			}
		}
	}
	return prfRet;
}


/*
   ulog: Log message unconditionally - mostly for debug() macro
*/

#if defined(__STDC__)
int ulog(const char *fmt, ...)
#else
int ulog(va_alist)
	va_dcl
#endif
{
#if !defined(__STDC__)
	char		*fmt;		/* printf-style format string */
#endif
	va_list		args;
	register int	prfRet;		/* Printf return value */

	if (logFile == NULL)
		return 0;		/* need a file to write to */

#if defined(__STDC__)
	va_start(args, fmt);
#else
	va_start(args);
	fmt = va_arg(args, char *);
#endif
	prfRet = vfprintf(logFile, fmt, args);
	va_end(args);

	/* Force output to log file */
	fflush(logFile);
	return prfRet;
}


/*
   tlog: Log message - used by Vlog() macro
	use first format when DBG_VLOG is on (verbose logs),
	else if DBG_LOG is on (normal logs) use second format.
*/

#if defined(__STDC__)
int tlog(const char *fmt1, const char *fmt2, ...)
#else
int tlog(va_alist)
	va_dcl
#endif
{
#if !defined(__STDC__)
	char		*fmt1;		/* printf verbose format string */
	char		*fmt2;		/* printf terse format string */
#endif
	va_list		args;
	register int	prfRet;		/* Printf return value */

	/* Only print when normal or verbose logs are enabled */
	/* and logFile is open */
	if ( !dbgCheck(DBG_LOG | DBG_VLOG) || logFile == NULL)
		return 0;

#if defined(__STDC__)
	va_start(args, fmt2);
#else
	va_start(args);
	fmt1 = va_arg(args, char *);
	fmt2 = va_arg(args, char *);
#endif
	prfRet = vfprintf(logFile, dbgCheck(DBG_VLOG) ? fmt1 : fmt2, args);
	va_end(args);

	/* Force output to log file */
	fflush(logFile);
	return prfRet;
}


/*
   logv: Log call with argument vector instead of argument list
*/

void
logv(fmt, args)
char *fmt;				/* printf-style format string */
va_list args;
{
	/* If remote logs disabled, or no log file open, return immediately */
	if (!(dbgEnable & DBG_RLOG) || logFile == NULL)
		return;

	/* Format and print message */
	vfprintf(logFile, fmt, args);

	/* Flush output immediately */
	fflush(logFile);
}
#endif /* ~NOLOG */


#ifndef	JANUS
/*
   serious: Log serious error message
*/

void
serious(string)
char *string;
{
	char	*save_ptr;		/* pointer to our saved string */
	FILE	*seriousFile;		/* Stream to error logger */

	popenSafe();

	/* 
	 * We have to save string
	 * because the next call
	 * to lmf_format_string
	 * will obliterate it
	 */
	save_ptr = strdup(string);

	if ((seriousFile = popen(ERR_LOGGER, "w")) == NULL) {
		log("serious: Cannot create logger `%s' (%d)\n",
			ERR_LOGGER, errno);
		seriousFile = fopen("/dev/console","w");
		fprintf(seriousFile, 
			lmf_format_string((char *) NULL, 0, 
			lmf_get_message("LOG6","Serious error in %1 %2:\n"),
			"%s%d", myname, getpid()));
		fprintf(seriousFile, save_ptr);
		fclose(seriousFile);
	} else {
		fprintf(seriousFile, 
			lmf_format_string((char *) NULL, 0, 
			lmf_get_message("LOG6","Serious error in %1 %2:\n"),
			"%s%d", myname, getpid()));
		fprintf(seriousFile, save_ptr);
		pclose(seriousFile);
	}

	free(save_ptr);
}


/*
   fatal: Log fatal error message and exit
*/

void
fatal(string)
char *string;
{
	char	*save_ptr;		/* pointer to our saved string */
	FILE	*fatalFile;		/* Stream to error logger */

	popenSafe();

	/* 
	 * We have to save string
	 * because the next call
	 * to lmf_format_string
	 * will obliterate it
	 */
	save_ptr = strdup(string);

	if ((fatalFile = popen(ERR_LOGGER, "w")) == NULL) {
		log("fatal: Cannot create logger `%s' (%d)\n",
			ERR_LOGGER, errno);
		fatalFile = fopen("/dev/console","w");
		fprintf(fatalFile, 
			lmf_format_string((char *) NULL, 0, 
			lmf_get_message("LOG7", "Fatal error in %1 %2:\n"),
			"%s%d", myname, getpid()));
		fprintf(fatalFile, save_ptr);
		fclose(fatalFile);
	} else {
		fprintf(fatalFile, 
			lmf_format_string((char *) NULL, 0, 
			lmf_get_message("LOG7", "Fatal error in %1 %2:\n"),
			"%s%d", myname, getpid()));
		fprintf(fatalFile, save_ptr);
		pclose(fatalFile);
	}

	exit(3);
}

/*
   popenSafe: Make things safe for a popen
*/

void
popenSafe()
{
int
	dummyDesc;			/* Dummy variable for descriptors */

	/*
	   "Making things safe for popen" means making sure that file
	   descriptors 0, 1 and 2 are open before popen is called.
	   If any of them are not, they are opened to /dev/null before
	   popen is called.  The problem is that popen does some file
	   descriptor manipulations that assume descriptors 0 and 1
	   are open before they begin.  If this assumption is not met,
	   popen clobbers it's own pipe file descriptors.
	*/
	do
		dummyDesc = open("/dev/null", O_RDONLY);
	while (dummyDesc == -1 && errno == EINTR);

	if (dummyDesc > 2)
		close(dummyDesc);
	else
		while (dummyDesc < 3)
			do
				dummyDesc = fcntl(dummyDesc, F_DUPFD, 0);
			while (dummyDesc == -1 && errno == EINTR);
}
#endif	/* not JANUS */


#ifndef NOLOG
/*
   newLogs: Set new log bits from channel file in /tmp - called on receipt
	of logging control signal
*/

long
newLogs(logName, namePid, childEnable, dbg)
char
	*logName;				/* Name of log file */
int
	namePid;				/* Process id for unique name */
long
	*childEnable;				/* Use these channel bits */
struct
	dbg_struct *dbg ;			/* Used for control from pc */
{
char
	chanName[32];				/* Name of debug channel file */
int
	chanDesc;				/* Descriptor of channel file */
long
	logChanges,				/* Which changes to make */
	setChans,				/* New absolute channel set */
	onChans,				/* Turn these channels on */
	offChans,				/* Turn these off */
	flipChans;				/* Invert these */

	/* Get name of channel file */
	sprintf(chanName, chanPat, getpid());

	if (dbg != NULL) {
		logChanges = dbg->change;
		setChans = dbg->set;
		onChans = dbg->on;
		offChans = dbg->off;
		flipChans = dbg->flip;
	} else {
		do
			chanDesc = open(chanName, O_RDONLY);
		while (chanDesc == -1 && errno == EINTR);
		/* If there's no channel file, use default changes */
		if (chanDesc < 0) {
			/* Default changes is to toggle standard logs */
			logChanges = CHG_INV;
			flipChans = DBG_LOG;
			setChans = onChans = offChans = 0L;
		} else {
		    while (read(chanDesc, &logChanges, sizeof logChanges) == -1
							&& errno == EINTR)
			;
		    while (read(chanDesc, &setChans, sizeof setChans) == -1
							&& errno == EINTR)
			;
		    while (read(chanDesc, &onChans, sizeof onChans) == -1
							&& errno == EINTR)
			;
		    while (read(chanDesc, &offChans, sizeof offChans) == -1
							&& errno == EINTR)
			;
		    while (read(chanDesc, &flipChans, sizeof flipChans) == -1
							&& errno == EINTR)
			;
		    close(chanDesc);
		    unlink(chanName);
		}
	}

	
	/* Are changes for child? */
	if ((logChanges & CHG_CHILD) && childEnable != NULL) {
		/* Record child changes, if childEnable is non-NULL */
		if (childEnable != NULL) {
			if (logChanges & CHG_SET)
				*childEnable = setChans;
			if (logChanges & CHG_ON)
				*childEnable |= onChans;
			if (logChanges & CHG_OFF)
				*childEnable &= ~offChans;
			if (logChanges & CHG_INV)
				*childEnable ^= flipChans;
		}

		return logChanges;
	}

	/* Change debug bits as requested by logChanges */
	if (logChanges & CHG_SET)
		dbgSet(setChans);
	if (logChanges & CHG_ON)
		dbgOn(onChans);
	if (logChanges & CHG_OFF)
		dbgOff(offChans);
	if (logChanges & CHG_INV)
		dbgToggle(flipChans);

	/*
	   Close the log file, if requested -
	   otherwise if not already open, open new log file.
	*/
	if (logChanges & CHG_CLOSE)
		logClose();
	else
		if (logFile == NULL && dbgEnable != 0)
			logOpen(logName, namePid);

	return logChanges;
}
#endif /* ~NOLOG */


/*
 * dup a string into malloc'd space
 * and return pointer to the new space
 */
char *
strdup(s)
CONST char *s;
{
	char *p;

	if ((p = malloc(strlen(s)+1)) != NULL) {
		strcpy(p,s);
	}
	return(p);
}
