/* @(#)main.c	1.5
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
 *	Call to set_slurpd_file_locations() to enable default replogfile
 *	location to be specified.
 * 
 */

/* 
 * main.c - main routine for slurpd.
 */

#include <stdio.h>

#include "slurp.h"
#include "globals.h"
#include "ldaplog.h"

#define MSG_TERMNORM \
    1,164,"slurpd: terminating normally\n"
#define MSG_STATUSFILE \
    1,165,"Malformed slurpd status file \"%s\"\nStopping slurpd\n"
#define MSG_OOM1 \
    1,166,"Out of memory initializing globals\n"
#define MSG_CFGERRS \
    1,167,"Errors encountered whilst processing config file \"%s\"\nStopping slurpd\n"
#define MSG_NOTHREAD \
    1,168,"Could not create thread for slurpd, exitting program\n"


extern int		doargs( int, char **, Globals * );
extern void		fm();
extern int		start_replica_thread( Ri * );
extern Globals		*init_globals();
extern void		set_slurpd_file_locations();
extern int		sanity();
#if defined( THREAD_SUNOS4_LWP )
extern void		start_lwp_scheduler();
#endif /* THREAD_SUNOS4_LWP */

char* PIDFILENAME;
extern void removeSlurpdPIDfile();
extern void createSlurpdPIDfile(const char*);
extern void setSlurpdPIDfile();

/*
 * void
 * slurpdExit(int errcode)
 *
 * This fn exits the program and ensures that the pid file is deleted.
 *
 */
void
slurpdExit(int errcode)
{
	removeSlurpdPIDfile();
	exit(errcode);
}



/*
 * main
 *
 */
main(
    int		argc,
    char	**argv
)
{
    pthread_attr_t	attr;
    int			status;
    int			i;

    (void)setlocale(LC_ALL, "");
    (void)setlabel("UX:slurpd");
    open_message_catalog("ldap.cat");

    /* 
     * Create and initialize globals.  init_globals() also initializes
     * the main replication queue.
     */
    if (( sglob = init_globals()) == NULL ) {
	logError(get_ldap_message(MSG_OOM1));
	exit( 1 );
    }

    /*
     * Process command-line args and fill in globals.
     */
    if ( doargs( argc, argv, sglob ) < 0 ) {
	exit( 1 );
    }

	/* Create the pid file - no pid entry yet!*/
	createSlurpdPIDfile(sglob->slapd_configfile);
	
    /*
     * Read slapd config file and initialize Re (per-replica) structs.
     */

    if ( slurpd_read_config( sglob->slapd_configfile ) < 0 ) {

	logError(get_ldap_message(MSG_CFGERRS,sglob->slapd_configfile));
	slurpdExit( 1 );
    }

    set_slurpd_file_locations();

    /*
     * Get any saved state information off the disk.
     */
    if ( sglob->st->st_read( sglob->st )) {


	logError(get_ldap_message(
	    MSG_STATUSFILE,sglob->slurpd_status_file));
	slurpdExit( 1 );
    }

    /*
     * All readonly data should now be initialized. 
     * Check for any fatal error conditions before we get started
     */
     if ( sanity() < 0 ) {
	slurpdExit( 1 );
    }

    /*
     * Detach from the controlling terminal, if debug level = 0,
     * and if not in one-shot mode.
     */
    if (( ldapdebug_level == 0 )  && !sglob->one_shot_mode ) {
	detach();
    }

	setSlurpdPIDfile();

#ifdef _THREAD

#if defined( THREAD_SUNOS4_LWP )
    /*
     * Need to start a scheduler thread under SunOS 4
     */
    start_lwp_scheduler();
#endif /* THREAD_SUNOS4_LWP */


    /*
     * Start threads - one thread for each replica
     */
    for ( i = 0; sglob->replicas[ i ] != NULL; i++ ) {
	start_replica_thread( sglob->replicas[ i ]);
    }

    /*
     * Start the main file manager thread (in fm.c).
     */
    pthread_attr_init( &attr );
    if ( pthread_create( &(sglob->fm_tid), attr, (void *) fm, (void *) NULL )
	    != 0 ) {

	logError(get_ldap_message(MSG_NOTHREAD));
	slurpdExit( 1 );

    }
    pthread_attr_destroy( &attr );

    /*
     * Wait for the fm thread to finish.
     */
    pthread_join( sglob->fm_tid, (void *) &status );
    /*
     * Wait for the replica threads to finish.
     */
    for ( i = 0; sglob->replicas[ i ] != NULL; i++ ) {
	pthread_join( sglob->replicas[ i ]->ri_tid, (void *) &status );
    }

	logError(get_ldap_message(MSG_TERMNORM));

    sglob->slurpd_shutdown = 1;
    pthread_exit( 0 );

#else /* !_THREAD */
    /*
     * Non-threaded case.
     */
    slurpdExit( 0 );

#endif /* !_THREAD */
    
}
