#ifndef _LDAP_DEBUG_H
#define _LDAP_DEBUG_H

/*
 * This file mostly contains items that were in ldap.h but which
 * are really private to the LDAP implementation.
 */
#ifdef __cplusplus
extern "C" {
#endif


/* debugging stuff */
#ifdef LDAP_DEBUG
#ifdef LDAP_SYSLOG
extern int	ldap_syslog;
extern int	ldap_syslog_level;
#endif
#define LDAP_DEBUG_TRACE	0x001
#define LDAP_DEBUG_PACKETS	0x002
#define LDAP_DEBUG_ARGS		0x004
#define LDAP_DEBUG_CONNS	0x008
#define LDAP_DEBUG_BER		0x010
#define LDAP_DEBUG_FILTER	0x020
#define LDAP_DEBUG_CONFIG	0x040
#define LDAP_DEBUG_ACL		0x080
#define LDAP_DEBUG_STATS	0x100
#define LDAP_DEBUG_STATS2	0x200
#define LDAP_DEBUG_SHELL	0x400
#define LDAP_DEBUG_PARSE	0x800
#define LDAP_DEBUG_ANY		0xffff
#ifdef LDAP_SYSLOG
#define Debug( level, fmt, arg1, arg2, arg3 )	\
	{ \
		if ( ldap_debug & level ) \
			fprintf( stderr, fmt, arg1, arg2, arg3 ); \
		if ( ldap_syslog & level ) \
			syslog( ldap_syslog_level, fmt, arg1, arg2, arg3 ); \
	}
#else /* LDAP_SYSLOG */

#define Debug( level, fmt, arg1, arg2, arg3 ) \
		if ( ldap_debug & level ) \
			fprintf( stderr, fmt, arg1, arg2, arg3 );
#endif /* LDAP_SYSLOG */
#else /* LDAP_DEBUG */
#define Debug( level, fmt, arg1, arg2, arg3 )
#endif /* LDAP_DEBUG */

#endif /* LDAP_DEBUG_H */
