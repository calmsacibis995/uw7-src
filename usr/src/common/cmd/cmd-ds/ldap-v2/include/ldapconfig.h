/*
 * Copyright (c) 1994 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef _CONFIG_H
#define _CONFIG_H

/*
 * config.h for LDAP -- edit this file to customize LDAP client behavior.
 * NO platform-specific definitions should be placed in this file.
 * Note that this is NOT used by the LDAP or LBER libraries.
 */


/*********************************************************************
 *                                                                   *
 * You probably do not need to edit anything below this point        *
 *                                                                   *
 *********************************************************************/

/*
 * SHARED DEFINITIONS - other things you can change
 */
	/* default config file locations */
#define FILTERFILE		"/etc/ldapfilter.conf"
#define TEMPLATEFILE		"/etc/ldaptemplates.conf"
#define SEARCHFILE		"/etc/ldapsearchprefs.conf"
#define FRIENDLYFILE		"/etc/ldapfriendly"
#define DEFAULT_CONFIGFILE	"/etc/slapd.conf"
#define COMMAND_DEFAULTFILE	"/etc/ldap/ldap_defaults"


/*
 * SLAPD DEFINITIONS
 */
	/* location of the default slapd config file */
#define SLAPD_DEFAULT_CONFIGFILE	DEFAULT_CONFIGFILE
	/* default sizelimit on number of entries from a search */
#define SLAPD_DEFAULT_SIZELIMIT		500
	/* default timelimit to spend on a search */
#define SLAPD_DEFAULT_TIMELIMIT		3600
	/* location of the slapd pid file */
#define SLAPD_PIDFILE			"/etc/slapd.pid"
	/* location of the slapd args file */
#define SLAPD_ARGSFILE			"/etc/slapd.args"
#define SLAPD_PIDDIR			"/var/ldap/pids"

/*
 * LDIF2X TOOL DEFINITIONS
 */
	/* maximum size of arguments */
#define LDIF2X_MAXARGS		100

/* LDAPTAB functions */
char *ldaptabIDtoFile( const char *id );

#endif /* _CONFIG_H */
