/* @(#)globals.c	1.5
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
 *
 * Revision history:
 *
 * 25 June 97     tonylo
 * 	Tidied up init_globals() to enable default replogfile to be 
 *	specified. set_slurpd_file_locations() added.
 */

/*
 * globals.c - initialization code for global data
 */

#include <stdio.h>

#include "slurp.h"
#include "globals.h"
#include "ldaplog.h"

#define MSG_INITSTAT1 \
     1,162,"Cannot initialize status data\n"
#define MSG_INITQUEUE1 \
    1,163,"Cannot initialize queue\n"


Globals		 *sglob;

/*
 * Initialize the globals
 */
Globals *init_globals()
{
    Globals *g;

    g = ( Globals * ) malloc( sizeof( Globals ));
    if ( g == NULL ) {
	return NULL;
    }

    g->slapd_configfile = SLAPD_DEFAULT_CONFIGFILE;
    g->no_work_interval = DEFAULT_NO_WORK_INTERVAL;
    g->slurpd_shutdown = 0;
    g->num_replicas = 0;
    g->replicas = NULL;
    g->slurpd_rdir = DEFAULT_SLURPD_REPLICA_DIR;
    g->slapd_replogfile[0] = '\0';
    g->slurpd_replogfile[0] = '\0';
    g->slurpd_status_file[0] = '\0';
    g->one_shot_mode = 0;
    g->myname = NULL;
    g->srpos = 0L;

    if ( St_init( &(g->st)) < 0 ) {
	logError(get_ldap_message(MSG_INITSTAT1));
	exit( 1 );
    }
    pthread_mutex_init( &(g->rej_mutex), pthread_mutexattr_default );
    if ( Rq_init( &(g->rq)) < 0 ) {
	logError(get_ldap_message(MSG_INITQUEUE1));
	exit( 1 );
    }
#ifdef KERBEROS
    g->default_srvtab = SRVTAB;
#endif /* KERBEROS */
#if defined( THREAD_SUNOS4_LWP )
    g->tsl_list = NULL;
    mon_create( &g->tsl_mon ); 
#endif /* THREAD_SUNOS4_LWP */

    return g;
}

/*
 * void
 * set_slurpd_file_locations
 *
 * globals:
 *	sglob 
 *
 * Description:
 *	This function initialises the location of the file used by
 *	slurpd. This fn should be called after the command line 
 *	arguments have been retrieved.
 */

void
set_slurpd_file_locations()
{
	/* Set location/name of our private copy of the slapd replog file */
	sprintf( sglob->slurpd_replogfile, "%s/%s", sglob->slurpd_rdir,
		DEFAULT_SLURPD_REPLOGFILE );
	/* Set location/name of the slurpd status file */
	sprintf( sglob->slurpd_status_file, "%s/%s", sglob->slurpd_rdir,
		DEFAULT_SLURPD_STATUS_FILE );

	/* 
	 * First check a replog file has been specified at the command-line 
	 * or in the config file
	 */
    	if(sglob->slapd_replogfile[0] == '\0') {
		sprintf( sglob->slapd_replogfile, "%s/%s", sglob->slurpd_rdir,
			DEFAULT_SLAPD_REPLOGFILE);
	}
}
