/* @(#)ldaplog.c	1.3
 *
 * Routines handling the display of messages from slapd
 *
 * Revision history:
 *
 * 1 Aug 1997	tonylo
 *	Created
 *
 */

#include <stdio.h>
#include <nl_types.h>
#include <stdarg.h>
#include <syslog.h>

#include "ldaplog.h"

/* messages */
#define MSG_INFOFILELOC \
    4,1,"See file \"%s\" for more details\n"
#define MSG_DBLEVELS \
    4,2,"Debug levels:\n"

#define DEFAULT_ERROR_LOG_FILE "/var/adm/log/ldap.log"

static nl_catd	msgcatalog;               /* id of msg catalog */
static char	catalog_msg[BUFSIZ];
static FILE*	logfile=NULL;                  /* The ldap.log file */
static char	logfilename[BUFSIZ];           /* Name of the ldap.log file */

/* Some new vars for error logging and display */
int             proc_detached=0;
int             ldapsyslog_level=0;
int             ldapdebug_level=0;

/*
 * void logProcessDetached( int )
 *	Indicate whether the logged process is detached or not
 */
void
logProcessDetached( int flag )
{
	proc_detached = flag ? 1 : 0;
}

/* 
 * void open_message_catalog(char* catalog_name)
 *	Opens message catalog for get_ldap_message, vget_ldap_message
 */
void
open_message_catalog(char* catalog_name)
{
    /* open message catalog */
    msgcatalog=catopen(catalog_name, 0);

/*
    if ( msgcatalog == (nl_catd)-1 ) {
	fprintf(stderr, "Could not open message catalog: %s\n", 
	    catalog_name);
	exit(1);
    }
*/
}

/* 
 * char* get_ldap_message(int set,int msgid, char *msg, ...)
 * 	Return a pointer to buffer string with a localised msg
 * 
 * Args:
 * 	set	- message catalog set
 * 	msgid 	- id number of message
 * 	msg	- the message itself (printf format)
 * 	....	- args to be applied to the string msg
 */
char*
get_ldap_message(int set,int msgid, char *msg, ...)
{
    va_list ap;
    char* format;

    va_start(ap, );
    format = catgets(msgcatalog,set,msgid,msg);
    vsprintf(catalog_msg, format, ap);
    va_end(ap);
    return(catalog_msg);
}

/*
 * char* vget_ldap_message(int set,int msgid, char *msg, va_list ap)
 *	Return a pointer to buffer string with a localised msg
 *
 * Remarks:
 *	Does the same job as get_ldap_message() except it takes the arguments
 *	to the message as a va_list structure.
 */
char*
vget_ldap_message(int set,int msgid, char *msg, va_list ap)
{
    char* format;
    format = catgets(msgcatalog,set,msgid,msg);
    vsprintf(catalog_msg, format, ap);
    return(catalog_msg);
}

/*
 * void open_info_log( int portno, char* mode )
 *	This function opens the info file to be used by slapd
 */
void
open_info_log(int port, char* mode)
{
    if ( logfile == NULL ) {

	sprintf(logfilename,"%s.%d",DEFAULT_ERROR_LOG_FILE,port);

	/* open ldap.log file */
	if((logfile = fopen(logfilename, mode)) == NULL) {
	    perror(logfilename);
	    exit(1);
	}
    }
}

void
close_info_log()
{
    if ( logfile != NULL ) {
	fclose(logfile);
	logfile=NULL;
    }
}

/*
 * close_logs()
 * 	Close message catalogs 
 */
void
close_logs()
{
    catclose(msgcatalog);
}

/*
 * void logInfo(char *msg)
 *	Send message to log file
 */
void
logInfo(char *msg)
{
	if ( logfile != NULL ) { 
		fprintf(logfile, msg); 
	}	
}

/*
 * void logError( char *msg )
 *	Send a notification error. If a command has daemonized itself
 *	send a message to the system log, other send the message to stderr.
 */
void
logError( char *msg )
{
	if( proc_detached )
       	 	syslog(LOG_ERR, msg);
	else
		fprintf(stderr, msg);
}

/*
 * void logLocation()
 *	Send a message notifying the user of the location of the ldap info log
 */
void
logLocation()
{
    if( proc_detached )
	syslog(LOG_INFO, get_ldap_message(MSG_INFOFILELOC,logfilename ));
    else
	fprintf(stderr, get_ldap_message(MSG_INFOFILELOC,logfilename));
}

void
logPrintLevels()
{
        printf(get_ldap_message(MSG_DBLEVELS));
        printf( "\tLDAP_LOG_FILTER    %d\n", LDAP_LOG_FILTER );
        printf( "\tLDAP_LOG_ACL  %d\n", LDAP_LOG_ACL );
        printf( "\tLDAP_LOG_NETWORK     %d\n", LDAP_LOG_NETWORK );
        printf( "\tLDAP_LOG_SHELLBE    %d\n", LDAP_LOG_SHELLBE );
        printf( "\tLDAP_LOG_LDBMBE      %d\n", LDAP_LOG_LDBMBE );
        printf( "\tLDAP_LOG_BER   %d\n", LDAP_LOG_BER );
        printf( "\tLDAP_LOG_CLNT   %d\n", LDAP_LOG_CLNT );
        printf( "\tLDAP_LOG_STATUS      %d\n", LDAP_LOG_STATUS );
        printf( "\tLDAP_LOG_MISC    %d\n", LDAP_LOG_MISC );
        printf( "\tLDAP_LOG_BIND   %d\n", LDAP_LOG_BIND );
        printf( "\tLDAP_CFG_FILE   %d\n", LDAP_CFG_FILE );
        printf( "\tLDAP_SLURPD    %d\n", LDAP_SLURPD );
}

