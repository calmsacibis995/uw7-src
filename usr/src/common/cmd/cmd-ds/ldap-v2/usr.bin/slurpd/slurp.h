/* @(#)slurp.h	1.5
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
 *	Added DEFAULT_SLAPD_REPLOGFILE so that if a replogfile is not
 *	specified then a file with this name will be used instead
 *
 * 26 June 97     tonylo
 *	SLURPD_PIDDIR added 
 *
 */

/* slurp.h - Standalone Ldap Update Replication Daemon (slurpd) */

#ifndef _SLURPD_H_
#define _SLURPD_H_

#define LDAP_SYSLOG

#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include "lber.h"
#include "ldap.h"
#include "ldaplog.h"
#include "lthread.h"
#include "portable.h"
#include "ldapconfig.h"
#include "ldif.h"


/* Default directory for slurpd's private copy of replication logs */
#define	DEFAULT_SLURPD_REPLICA_DIR	"/usr/tmp"

/* Default name for slurpd's private copy of the replication log */
#define	DEFAULT_SLURPD_REPLOGFILE	"slurpd.replog"

/* Name of file which stores saved slurpd state info, for restarting */
#define	DEFAULT_SLURPD_STATUS_FILE	"slurpd.status"

/* This the public copy of the replication log */
#define DEFAULT_SLAPD_REPLOGFILE	"replogfile"

/* slurpd dump file - contents of rq struct are written here (debugging) */
#define	SLURPD_DUMPFILE			"/tmp/slurpd.dump"

/* This is where the slurpd pid file goes */
#define SLURPD_PIDDIR			"/var/ldap/pids/slurpd"

/* default srvtab file.  Can be overridden */
#define	SRVTAB				"/etc/srvtab"

/* Amount of time to sleep if no more work to do */
#define	DEFAULT_NO_WORK_INTERVAL	3

/* The time we wait between checks to see if the replog file needs trimming */
#define	TRIMCHECK_INTERVAL		( 60 * 5 )

/* Only try to trim slurpd replica files larger than this size */
#define	MIN_TRIM_FILESIZE		( 10L * 1024L )

/* Maximum line length we can read from replication log */
#define	REPLBUFLEN			256

/* We support simple (plaintext password) and kerberos authentication */
#define	AUTH_SIMPLE	1
#define	AUTH_KERBEROS	2

/* Rejection records are prefaced with this string */
#define	ERROR_STR	"ERROR"

/* Strings found in replication entries */
#define	T_CHANGETYPESTR		"changetype"
#define	T_CHANGETYPE		1
#define	T_TIMESTR		"time"
#define	T_TIME			2
#define	T_DNSTR			"dn"
#define	T_DN			3

#define	T_ADDCTSTR		"add"
#define	T_ADDCT			4
#define	T_MODIFYCTSTR		"modify"
#define	T_MODIFYCT		5
#define	T_DELETECTSTR		"delete"
#define	T_DELETECT		6
#define	T_MODRDNCTSTR		"modrdn"
#define	T_MODRDNCT		7

#define	T_MODOPADDSTR		"add"
#define	T_MODOPADD		8
#define	T_MODOPREPLACESTR	"replace"
#define	T_MODOPREPLACE		9
#define	T_MODOPDELETESTR	"delete"
#define	T_MODOPDELETE		10
#define	T_MODSEPSTR		"-"
#define	T_MODSEP		11

#define	T_NEWRDNSTR		"newrdn"
#define	T_DRDNFLAGSTR		"deleteoldrdn"

#define	T_ERR			-1

/* Config file keywords */
#define	HOSTSTR			"host"
#define	BINDDNSTR		"binddn"
#define	BINDMETHSTR		"bindmethod"
#define	KERBEROSSTR		"kerberos"
#define	SIMPLESTR		"simple"
#define	CREDSTR			"credentials"
#define BINDPSTR		"bindprincipal"
#define	SRVTABSTR		"srvtab"

#define	REPLICA_SLEEP_TIME	( 10 )

/* Enumeration of various types of bind failures */
#define BIND_OK 			0
#define BIND_ERR_BADLDP			1
#define	BIND_ERR_OPEN			2
#define	BIND_ERR_BAD_ATYPE		3
#define	BIND_ERR_SIMPLE_FAILED		4
#define	BIND_ERR_KERBEROS_FAILED	5
#define	BIND_ERR_BADRI			6

/* Return codes for do_ldap() */
#define	DO_LDAP_OK			0
#define	DO_LDAP_ERR_RETRYABLE		1
#define	DO_LDAP_ERR_FATAL		2

/*
 * Types of counts one can request from the Rq rq_getcount()
 * member function
 */
/* all elements */
#define	RQ_COUNT_ALL			1
/* all elements with nonzero refcnt */
#define	RQ_COUNT_NZRC			2

/* Amount of time, in seconds, for a thread to sleep when it encounters
 * a retryable error in do_ldap().
 */
#define	RETRY_SLEEP_TIME		60



/*
 * ****************************************************************************
 * Data types for replication queue and queue elements.
 * ****************************************************************************
 */


/*
 * Replica host information.  An Ri struct will contain an array of these,
 * with one entry for each replica.  The end of the array is signaled
 * by a NULL value in the rh_hostname field.
 */
typedef struct rh {
    char 	*rh_hostname;		/* replica hostname  */
    int		rh_port;		/* replica port */
} Rh;


/*
 * Per-replica information.
 *
 * Notes:
 *  - Private data should not be manipulated expect by Ri member functions.
 */
typedef struct ri {

    /* Private data */
    char	*ri_hostname;		/* canonical hostname of replica */
    int		ri_port;		/* port where slave slapd running */
    LDAP	*ri_ldp;		/* LDAP struct for this replica */
    int		ri_bind_method;		/* AUTH_SIMPLE or AUTH_KERBEROS */
    char	*ri_bind_dn;		/* DN to bind as when replicating */
    char	*ri_password;		/* Password for AUTH_SIMPLE */
    char	*ri_principal;		/* principal for kerberos bind */
    char	*ri_srvtab;		/* srvtab file for kerberos bind */
    struct re	*ri_curr;		/* current repl entry being processed */
    struct stel	*ri_stel;		/* pointer to Stel for this replica */
    unsigned long
		ri_seq;			/* seq number of last repl */
    pthread_t	ri_tid;			/* ID of thread for this replica */

    /* Member functions */
    int		(*ri_process)();	/* process the next repl entry */
    void	(*ri_wake)();		/* wake up a sleeping thread */
} Ri;
    



/*
 * Information about one particular modification to make.  This data should
 * be considered private to routines in re.c, and to routines in ri.c.
 */
typedef struct mi {
    
    /* Private data */
    char	*mi_type;		/* attr or type */
    char	*mi_val;		/* value */
    int		mi_len;			/* length of mi_val */

} Mi;




/* 
 * Information about one particular replication entry.  Only routines in
 * re.c  and rq.c should touch the private data.  Other routines should
 * only use member functions.
 */
typedef struct re {

    /* Private data */
    pthread_mutex_t
		re_mutex;		/* mutex for this Re */
    int		re_refcnt;		/* ref count, 0 = done */
    char	*re_timestamp;		/* timestamp of this re */
    int		re_seq;			/* sequence number */
    Rh    	*re_replicas;		/* array of replica info */
    char	*re_dn;			/* dn of entry being modified */
    int		re_changetype;		/* type of modification */
    Mi		*re_mods;		/* array of modifications to make */
    struct re	*re_next;		/* pointer to next element */

    /* Public functions */
    int 	(*re_free)();		/* free an re struct */
    struct re	*(*re_getnext)();	/* return next Re in linked list */
    int		(*re_parse)();		/* parse a replication log entry */
    int		(*re_write)();		/* write a replication log entry */
    void	(*re_dump)();		/* debugging  - print contents */
    int		(*re_lock)();		/* lock this re */
    int		(*re_unlock)();		/* unlock this re */
    int		(*re_decrefcnt)();	/* decrement the refcnt */
    int		(*re_getrefcnt)();	/* get the refcnt */
} Re;




/* 
 * Definition for the queue of replica information.  Private data is
 * private to rq.c.  Other routines should only touch public data or
 * use member functions.  Note that although we have a member function
 * for locking the queue's mutex, we need to expose the rq_mutex
 * variable so routines in ri.c can use it as a mutex for the
 * rq_more condition variable.
 */
typedef struct rq {

    /* Private data */
    Re		*rq_head;		/* pointer to head */
    Re		*rq_tail;		/* pointer to tail */
    int		rq_nre;			/* total number of Re's in queue */
    int		rq_ndel;		/* number of deleted Re's in queue */
    time_t	rq_lasttrim;		/* Last time we trimmed file */
    
    /* Public data */
    pthread_mutex_t
		rq_mutex;		/* mutex for whole queue */
    pthread_cond_t
		rq_more;		/* condition var - more work added */

    /* Member functions */
    Re		*(*rq_gethead)();	/* get the element at head */
    Re		*(*rq_getnext)();	/* get the next element */
    int		(*rq_delhead)();	/* delete the element at head */
    int		(*rq_add)();		/* add at tail */
    void	(*rq_gc)();		/* garbage-collect queue */
    int		(*rq_lock)();		/* lock the queue */
    int		(*rq_unlock)();		/* unlock the queue */
    int		(*rq_needtrim)();	/* see if queue needs trimming */
    int		(*rq_write)();		/* write Rq contents to a file */
    int		(*rq_getcount)();	/* return queue counts */
    void	(*rq_dump)();		/* debugging  - print contents */
} Rq;



/*
 * An Stel (status element) contains information about one replica.
 * Stel structs are associated with the St (status) struct, defined 
 * below.
 */
typedef struct stel {
    char	*hostname;		/* host name of replica */
    int		port;			/* port number of replica */
    char	last[ 64 ];		/* timestamp of last successful repl */
    int		seq;			/* Sequence number of last repl */
} Stel;


/*
 * An St struct in an in-core structure which contains the current
 * slurpd state.  Most importantly, it contains an array of Stel
 * structs which contain the timestamp and sequence number of the last
 * successful replication for each replica.  The st_write() member
 * function is called periodically to flush status information to
 * disk.  At startup time, slurpd checks for the status file, and
 * if present, uses the timestamps to avoid "replaying" replications
 * which have already been sent to a given replica.
 */
typedef struct st {

    /* Private data */
    pthread_mutex_t
		st_mutex;		/* mutex to serialize access */
    Stel	**st_data;		/* array of pointers to Stel structs */
    int		st_nreplicas;		/* number of repl hosts */
    int		st_err_logged;		/* 1 if fopen err logged */
    FILE	*st_fp;			/* st file kept open */
    FILE	*st_lfp;		/* lockfile fp */

    /* Public member functions */
    int		(*st_update)();		/* update the entry for a host */
    Stel	*(*st_add)();		/* add a new repl host */
    int		(*st_write)();		/* write status to disk */
    int		(*st_read)();		/* read status info from disk */
    int		(*st_lock)();		/* read status info from disk */
    int		(*st_unlock)();		/* read status info from disk */
} St;

#if defined( THREAD_SUNOS4_LWP )
typedef struct tl {
    thread_t	tl_tid; 	/* thread being managed */
    time_t	tl_wake;	/* time thread should be resumed */
    struct tl	*tl_next;	/* next node in list */
} tl_t;

typedef struct tsl {
    tl_t	*tsl_list;
    mon_t	tsl_mon;
} tsl_t;
#endif /* THREAD_SUNOS4_LWP */

    

/* 
 * Public functions used to instantiate and initialize queue objects.
 */
#ifdef NEEDPROTOS
extern int Ri_init( Ri **ri );
extern int Rq_init( Rq **rq );
extern int Re_init( Re **re );
#else /* NEEDPROTOS */
extern int Ri_init();
extern int Rq_init();
extern int Re_init();
#endif /* NEEDPROTOS */

#endif /* _SLURPD_H_ */

