/* @(#)args.c	1.4
 *
 * Copyright (c) 1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 * args.c - process command-line arguments, and set appropriate globals.
 *
 * Revision history:
 *
 * 5th March 1997       tonylo
 *      I18N
 *
 */


#include <stdio.h>
#include <string.h>

#include <lber.h>
#include <ldap.h>

#include "slurp.h"
#include "globals.h"
#include "ldaplog.h"

/* Messages */

#define MSG_USAGE \
    1,139,"usage: %s\t[-d debug-level] [-s syslog-level]\n\t\t[-f slapd-config-file] [-r replication-log-file]\n\t\t[-t tmp-dir] [-o] [-k srvtab-file]\n\t\t[-t tmp-dir] [-o]\n"

#define MSG_ONESHOT \
    1,140,"If -o flag is given, -r flag must also be given\n"


static int
usage( const char *name )
{
	logError(get_ldap_message(MSG_USAGE,name));
}

/*
 * Interpret argv, and fill in any appropriate globals.
 */
int
doargs(
    int		argc,
    char	**argv,
    Globals	*g
)
{
    int		i;
    extern char	*optarg;
    int		rflag = 0;

    if ( (g->myname = strrchr( argv[0], '/' )) == NULL ) {
	g->myname = strdup( argv[0] );
    } else {
	g->myname = strdup( g->myname + 1 );
    }

    while ( (i = getopt( argc, argv, "hd:f:r:t:k:s:o" )) != EOF ) {
	switch ( i ) {
	case 'd':	/* turn on debugging */
	    if ( optarg[0] == '?' ) {
		logPrintLevels(); /* from liblog */
		exit(0);
	    } else {
		ldapdebug_level = atoi( optarg );
	    }
	    break;
	case 'f':	/* slapd config file */
	    g->slapd_configfile = strdup( optarg );
	    break;
	case 'r':	/* slapd replog file */
	    strcpy( g->slapd_replogfile, optarg );
	    rflag++;
	    break;
	case 't':	/* dir to use for our copies of replogs */
	    g->slurpd_rdir = strdup( optarg );
	    break;
	case 'k':	/* name of kerberos srvtab file */
#ifdef KERBEROS
	    g->default_srvtab = strdup( optarg );
#endif /* KERBEROS */
	    break;
	case 'h':
	    usage( g->myname );
	    return( -1 );
	case 'o':
	    g->one_shot_mode = 1;
	    break;
	case 's':       /* set syslog level */
		ldapsyslog_level = atoi( optarg );
		break;

	default:
	    usage( g->myname );
	    return( -1 );
	}
    }

    if ( g->one_shot_mode && !rflag ) {
	logError(get_ldap_message(MSG_ONESHOT));
	usage( g->myname );
	return( -1 );
    }

    /* Set location/name of our private copy of the slapd replog file */
    sprintf( g->slurpd_replogfile, "%s/%s", g->slurpd_rdir,
	    DEFAULT_SLURPD_REPLOGFILE );

    /* Set location/name of the slurpd status file */
    sprintf( g->slurpd_status_file, "%s/%s", g->slurpd_rdir,
	    DEFAULT_SLURPD_STATUS_FILE );

#ifdef LOG_LOCAL4
    openlog( g->myname, OPENLOG_OPTIONS, LOG_LOCAL4 );
#else
    openlog( g->myname, OPENLOG_OPTIONS );
#endif

    return 0;

}


