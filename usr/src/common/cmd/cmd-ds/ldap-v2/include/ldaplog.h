/* @(#)ldaplog.h	1.2
 *
 * Routines handling the display of error and logging messages.
 *
 * Revision history:
 *
 * 1 Aug 1997	tonylo
 *	Created
 *
 */

#ifndef _LDAP_LDAPLOG_H
#define _LDAP_LDAPLOG_H

#include <syslog.h>
#include <stdarg.h>
#include <locale.h>
#include <unistd.h> 

extern int             ldapsyslog_level;
extern int             ldapdebug_level;

extern void	open_message_catalog(char* catalog_name);
extern char*	get_ldap_message(int set,int msgid, char *msg, ...);
extern char*	vget_ldap_message(int set,int msgid, char *msg, va_list ap);
extern void 	open_info_log(int port, char* mode);
extern void 	close_logs();
extern void	logInfo(char *msg);
extern void	logError( char *msg );
extern void	logLocation();
extern void	logPrintLevels();
extern void	logProcessDetached( int );

#define LDAP_LOG_FILTER    0x001   /* LDAP filter processing            */
#define LDAP_LOG_ACL       0x002   /* ACL processing                    */
#define LDAP_LOG_NETWORK   0x004   /* Networking                        */
#define LDAP_LOG_SHELLBE   0x008   /* Shell backend communication       */
#define LDAP_LOG_LDBMBE    0x010   /* ldbm backend communication        */
#define LDAP_LOG_BER       0x020   /* BER processing                    */
#define LDAP_LOG_CLNT      0x040   /* Client request processing         */
#define LDAP_LOG_STATUS    0x080   /* status info                       */
#define LDAP_LOG_MISC      0x100   /* Miscellaneous                     */
#define LDAP_LOG_BIND      0x200   /* who is connecting to the daemon
                                      and doing what                    */
#define LDAP_CFG_FILE      0x400   /* config file processing            */
#define LDAP_SLURPD        0x800   /* slurpd messages */

#define LDAP_LOG_BACKEND  ( LDAP_LOG_SHELLBE | LDAP_LOG_LDBMBE )


#define logDebug(level, fmt, arg1,arg2,arg3) \
{ \
	if ( ldapdebug_level & level ) { \
		fprintf(stderr, fmt, arg1,arg2,arg3); \
	} else if ( ldapsyslog_level & level) { \
		syslog(LOG_DEBUG, fmt, arg1,arg2,arg3); \
	} \
}

/* Get error text string from errno (if there is one) */
#define logErrNoMsg (errno > -1 && errno < sys_nerr ? sys_errlist[errno] : "unknown")

#endif /* _LDAP_LDAPLOG_H */
