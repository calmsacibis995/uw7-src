/* @(#)fm.c	1.6
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
 * fm.c - file management routines.
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "slurp.h"
#include "globals.h"
#include "ldaplog.h"


#define MSG_REPLOG1 \
    1,154,"Fatal error while copying replication log\n"
#define MSG_REPLOG2 \
    1,155,"Error: cannot acquire lock on \"%s\" for trimming\n"
#define MSG_NOLOCK1 \
    1,156,"Failed to populate replication file: can't lock file \"%s\": %s\n"
#define MSG_POPQ1 \
    1,157,"Failed to populate replication file: can't seek to offset %ld in file \"%s\"\n"
#define MSG_POPQ2 \
    1,158,"Replication file error: malformed replog entry (begins with \"%s\")\n"
#define MSG_MSG1 \
    1,159,"Processing in one-shot mode:\n"
#define MSG_MSG2 \
    1,160,"%d total replication records in file,\n"
#define MSG_MSG3 \
    1,161,"%d replication records to process\n"



extern void do_admin();
static void set_shutdown();
void do_nothing();

/*
 * Externs
 */
extern int file_nonempty( char * );
extern int acquire_lock(char *, FILE **, FILE ** );
extern int relinquish_lock(char *, FILE *, FILE * );

/*
 * Forward references
 */
static char *get_record( FILE * );
static void populate_queue( char *f );
static void set_shutdown();
void do_nothing();

#ifndef SYSERRLIST_IN_STDIO
extern char *sys_errlist[];
#endif /* SYSERRLIST_IN_STDIO */

extern void removeSlurpdPIDfile();


/*
 * Main file manager routine.  Watches for new data to be appended to the
 * slapd replication log.  When new data is appended, fm does the following:
 *  - appends the data to slurpd's private copy of the replication log.
 *  - truncates the slapd replog
 *  - adds items to the internal queue of replication work to do
 *  - signals the replication threads to let them know new work has arrived.
 */
void
fm(
    void *arg
)
{
    int rc;

    /* Set up our signal handlers:
     * SIG{TERM,INT,HUP} causes a shutdown
     * SIGUSR1 - does nothing, used to wake up sleeping threads.
     * SIGUSR2 - causes slurpd to read its administrative interface file.
     *           (not yet implemented).
     */
    (void) SIGNAL( SIGUSR1, (void(*)(int)) do_nothing );
    (void) SIGNAL( SIGUSR2, (void(*)(int)) do_admin );
    (void) SIGNAL( SIGTERM, (void(*)(int)) set_shutdown );
    (void) SIGNAL( SIGINT, (void(*)(int)) set_shutdown );
    (void) SIGNAL( SIGHUP, (void(*)(int)) set_shutdown );

    if ( sglob->one_shot_mode ) {
	if ( file_nonempty( sglob->slapd_replogfile )) {
	    populate_queue( sglob->slapd_replogfile );
	}

	logError(get_ldap_message(MSG_MSG1));
	logError(get_ldap_message(MSG_MSG2,
	     sglob->rq->rq_getcount( sglob->rq, RQ_COUNT_ALL )));
	logError(get_ldap_message(MSG_MSG3,
	     sglob->rq->rq_getcount( sglob->rq, RQ_COUNT_NZRC )));
	return;
    }
    /*
     * There may be some leftover replication records in our own
     * copy of the replication log.  If any exist, add them to the
     * queue.
     */
    if ( file_nonempty( sglob->slurpd_replogfile )) {
	populate_queue( sglob->slurpd_replogfile );
    }


    while ( !sglob->slurpd_shutdown ) {
	if ( file_nonempty( sglob->slapd_replogfile )) {
	    /* New work found - copy to slurpd replog file */
	    if (( rc = copy_replog( sglob->slapd_replogfile,
		    sglob->slurpd_replogfile )) == 0 )  {
		populate_queue( sglob->slurpd_replogfile );
	    } else {
		if ( rc < 0 ) {
			logError(get_ldap_message(MSG_REPLOG1));
			sglob->slurpd_shutdown = 1;
		}
	    }
	} else {
	    tsleep( sglob->no_work_interval );
	}

	/* Garbage-collect queue */
	sglob->rq->rq_gc( sglob->rq );

	/* Trim replication log file, if needed */
	if ( sglob->rq->rq_needtrim( sglob->rq )) {
	    FILE *fp, *lfp;
	    if (( rc = acquire_lock( sglob->slurpd_replogfile, &fp,
		    &lfp )) < 0 ) {

		logInfo(get_ldap_message(MSG_REPLOG2,
		    sglob->slurpd_replogfile));

	    } else {
		sglob->rq->rq_write( sglob->rq, fp );
		(void) relinquish_lock( sglob->slurpd_replogfile, fp, lfp );
	    }
	}
    }
}




/*
 * Set a global flag which signals that we're shutting down.
 */
static void
set_shutdown()
{
    int	i;

    removeSlurpdPIDfile();

    sglob->slurpd_shutdown = 1;				/* set flag */
    pthread_kill( sglob->fm_tid, SIGUSR1 );		/* wake up file mgr */
    sglob->rq->rq_lock( sglob->rq );			/* lock queue */
    pthread_cond_broadcast( &(sglob->rq->rq_more) );	/* wake repl threads */
    for ( i = 0; i < sglob->num_replicas; i++ ) {
	(sglob->replicas[ i ])->ri_wake( sglob->replicas[ i ]);
    }
    sglob->rq->rq_unlock( sglob->rq );			/* unlock queue */
    (void) SIGNAL( SIGTERM, (void(*)(int)) set_shutdown );	/* reinstall handlers */
    (void) SIGNAL( SIGINT, (void(*)(int)) set_shutdown );
    (void) SIGNAL( SIGHUP, (void(*)(int)) set_shutdown );
}




/*
 * A do-nothing signal handler.
 */
void
do_nothing()
{
    (void) SIGNAL( SIGUSR1, (void(*)(int)) do_nothing );
}




/*
 * Open the slurpd replication log, seek to our last known position, and
 * process any pending replication entries.
 */
static void
populate_queue(
    char *f
)
{
    FILE	*fp, *lfp;
    Rq		*rq = sglob->rq;
    char	*p;

    if ( acquire_lock( f, &fp, &lfp ) < 0 ) {

	logInfo(get_ldap_message(MSG_NOLOCK1,f, sys_errlist[ errno ]));
	return;
    }

    /*
     * Read replication records from fp and append them the
     * the queue.
     */
    if ( fseek( fp, sglob->srpos, 0 ) < 0 ) {
	logInfo(get_ldap_message(MSG_POPQ1,sglob->srpos, f));
	return;
    }

    while (( p = get_record( fp )) != NULL ) {
	if ( sglob->rq->rq_add( sglob->rq, p ) < 0 ) {
	    char *t;
	    if (( t = strchr( p, '\n' )) != NULL ) {
		*t = '\0';
	    }
	    logInfo(get_ldap_message(MSG_POPQ2,p));
	}
	free( p );
	pthread_yield();
    }
    sglob->srpos = ftell( fp );
    (void) relinquish_lock( f, fp, lfp );
}
    



/*
 * Get the next "record" from the file pointed to by fp.  A "record"
 * is delimited by two consecutive newlines.  Returns NULL on EOF.
 */
static char *
get_record(
    FILE *fp
)
{
    int		len;
    static char	line[BUFSIZ];
    char	*buf = NULL;
    static int	lcur, lmax;

    lcur = lmax = 0;

    while (( fgets( line, sizeof(line), fp ) != NULL ) &&
	    (( len = strlen( line )) > 1 )) {
	while ( lcur + len + 1 > lmax ) {
	    lmax += BUFSIZ;
	    buf = (char *) ch_realloc( buf, lmax );
	    if ( buf == NULL ) { return NULL; }
	}
	strcpy( buf + lcur, line );
	lcur += len;
    }
    return( buf );
}

