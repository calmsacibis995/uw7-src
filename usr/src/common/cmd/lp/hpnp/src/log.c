/*		copyright	"%c%"	*/

#ident	"@(#)log.c	1.3"

/*
 * (c) Copyright 1991 Hewlett-Packard Company.  All Rights Reserved.
 *
 * This source is for demonstration purposes only.
 *
 * Since this source has not been debugged for all situations, it will not be
 * supported by Hewlett-Packard in any manner.
 *
 * This material is provided "as is".  Hewlett-Packard makes no warranty of
 * any kind with regard to this material.  Hewlett-Packard shall not be liable
 * for errors contained herein for incidental or consequential damages in 
 * connection with the furnishing, performance, or use of this material.
 * Hewlett-Packard assumes no responsibility for the use or reliability of 
 * this software.
 */

#include	<stdio.h>
#include	<time.h>
#include	<signal.h>
#include	<varargs.h>

#include <locale.h>
#include "hpnp_msg.h"

extern nl_catd catd;

void	EndLog();
void	LogMessage();

char            *LogFileName = NULL;    /* default name of log file */
FILE		*LogFile = NULL;	/* logging file */
extern char 	*ProgName;      	/* argv[0] */
int             Logging = 0;
int             DelayedEndLog = 0;

/*
 * NAME
 *	OpenLog - open the logging file
 *
 * SYNOPSIS
 *	void OpenLog(log_name)
 *      char *log_name;
 *
 * DESCRIPTION
 *      Open the named log file if a log file has not already been opened.
 *      If the open succeeds, save the name so that it can be closed and
 *      opened again.
 *
 * RETURNS
 *	"OpenLog" returns 1 on suceess, 0 on failure.  If the open of
 *      the log file fails, the program exits.
 */
OpenLog(char *log_name)
{
	static char save_name[160];

	if (LogFile != NULL) {
		LogMessage(1, "%s: Log file is already open", ProgName);
		return(0);
	}

	if((LogFile = fopen(log_name, "a")) == NULL) {
		fprintf
		  (
		  stderr,
		  MSGSTR
		    (
		    HPNP_LOGC_FOLOG, "%s: Couldn't open logging file %s\n"
		    ),
		  ProgName, log_name
		  );
		exit(1);
	}
	if (strcmp(save_name, log_name))
		strcpy(save_name, log_name);
	LogFileName = save_name;
        LogMessage(1, "OpenLog: logging started");
	return(1);
}

/*
 * NAME
 *	FlushLog - flush the log file if it is open
 *
 * SYNOPSIS
 *	void Flush()
 *
 * DESCRIPTION
 *      Flush the log file if it is open.
 *
 * RETURNS
 *	"FlushLog" returns nothing.
 */
void
FlushLog()
{
	if (LogFile != NULL)
		fflush(LogFile);
}


/*
 * NAME
 *	LogMessage - optionally send message to logging file
 *
 * SYNOPSIS
 *      void LogMessage(flush, format, arg1, arg2, arg3, arg4, arg5)
 *      int     flush;
 *      char    *format;
 *      char	*arg1, *arg2, *arg3, *arg4, *arg5;
 *
 * DESCRIPTION
 *	This routine emits a message to the logging file if the log file is
 *      open.  The log message includes the time, name of the program, PID,
 *      and the log message.  The log file is flushed to disk if requested.
 *
 * RETURNS
 *	"LogMessage" returns nothing.
 */
void
LogMessage(va_alist)
va_dcl
{
	va_list args;
	time_t  current_time;   /* current system time */
	char    ascii_time[26]; /* ASCII local time */
	int     flush;
	char    *format;

	if (LogFile != NULL) {
		Logging = 1;
		va_start(args);

		flush = va_arg(args, int);
		format = va_arg(args, char *);

		/* Get the system time and convert to an ASCII local time 
		 * removing the year.
		 */
		current_time = time(NULL);
		strcpy(ascii_time, ctime(&current_time));
		ascii_time[19] = '\0';

		/* Output the logging message skipping day of the week, month,
		 * and day.
		 */
		fprintf(LogFile, "%s %s[%d] ", &ascii_time[11], ProgName, getpid());
		vfprintf(LogFile, format, args);
		fprintf(LogFile, "\n");
		if (flush)
			fflush(LogFile);
		va_end(args);
		Logging = 0;
	}

	if (DelayedEndLog)
		EndLog();
}


/*
 * NAME
 *      EndLog - end logging
 *
 * SYNOPSIS
 *	void EndLog()
 *
 * DESCRIPTION
 *	This routine closes the logging file after writing a message.
 *
 * RETURNS
 *	"LogMessage" returns nothing.
 */
void
EndLog()
{
	if (LogFile != NULL) {
		if (Logging) {
	    	DelayedEndLog = 1;
	    	return;
		}
		DelayedEndLog = 0;
        	LogMessage(1, "StopLog: logging stopped");
		fclose(LogFile);
		LogFile = NULL;
	}
}


/*
 * NAME
 *	StartLog - start logging to a file
 *
 * SYNOPSIS
 *	void StartLog(msg)
 *
 * DESCRIPTION
 *	This routine is called when SIGUSR1 is received by hpnptyd.
 *      Logging is started either with the old log file name or a name
 *      generated based on the process ID.
 *
 * RETURNS
 *	"StartLog" returns nothing.
 */
void
StartLog(int sig /*unused */)
{
	char log_name[40];

	if (LogFileName == NULL) {
#ifdef sun
		sprintf(log_name, "/var/tmp/hpnptyd.%d", getpid());
#else
		sprintf(log_name, "/usr/tmp/hpnptyd.%d", getpid());
#endif
		OpenLog(log_name);
	} else
		OpenLog(LogFileName);
#ifdef SIGUSR1
	signal(SIGUSR1, StartLog);
#endif
}

/*
 * NAME
 *	StopLog - stop logging to a file
 *
 * SYNOPSIS
 *	void StopLog()
 *
 * DESCRIPTION
 *	This routine is called when SIGUSR2 is received by hpnptyd.
 *      Logging is stopped.
 *
 * RETURNS
 *	"LogMessage" returns nothing.
 */
void
StopLog(int sig)
{
	EndLog();
#ifdef SIGUSR2
	signal(SIGUSR2, StopLog);
#endif
}
