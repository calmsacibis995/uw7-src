/* @(#)ldap_op.c	1.5
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
 * ldap_op.c - routines to perform LDAP operations
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>

#ifdef KERBEROS
#include <krb.h>
#endif /* KERBEROS */

#include <lber.h>
#include <ldap.h>

#include "portable.h"
#include "slurp.h"
#include "ldaplog.h"

/* Messages */
#define MSG_DOBIND001 \
    5,4,"ldap_unbind failed, error = %s\n"
#define MSG_DOBIND002 \
    5,5,"Could not bind to replica %s, %d, error = %s\n"
#define MSG_DOBIND003 \
    5,6,"Could not bind to %s:%d, error = %d\n"
#define MSG_DOBIND004 \
    5,7,"Unknown authentication type \"%d\" for %s:%d\n"
#define MSG_SLURPOP1 \
    5,8,"Failed to add entry to replica (ldap_add_s), dn=\"%s\", error=%s\n"
#define MSG_SLURPOP2 \
    5,9,"Failed to modify entry in replica (ldap_modify_s), dn=\"%s\", error=%s\n"
#define MSG_SLURPOP3 \
    5,10,"Failed to delete entry in replica (ldap_delete_s), dn=\"%s\", error=%s\n"
#define MSG_SLURPOP4 \
    5,11,"Failed to modrdn in replica (ldap_modrdn_s),dn=\"%s\", error=%s\n"
#define MSG_SLURPOP5 \
    5,12,"slurpd attempted an unknown LDAP operation, op \"%d\", dn=\"%s\"\n"
#define MSG_OPLDAPADD \
    5,13,"No modifications to do"
#define MSG_OPLDAPMOD1 \
    5,14,"No arguments given"
#define MSG_OPLDAPMODRDN \
    5,15,"No arguments given"
#define MSG_OPLDAPMODRDN01 \
    5,16,"Incorrect argument to delete old RDN"
#define MSG_OPLDAPMODRDN02 \
    5,17,"Bad value in replication log entry"
#define MSG_OPLDAPMODRDN03 \
    5,18,"Missing argument: requires \"new RDN\" and \"delete old rdn\""
#define MSG_DOUNBIND \
    5,19,"slurpd failed to unbind from replica %s, %p, error = %s\n"



/* Forward references */
static int get_changetype( char * );
static struct berval **make_singlevalued_berval( char	*, int );
static int op_ldap_add( Ri *, Re *, char ** );
static int op_ldap_modify( Ri *, Re *, char ** );
static int op_ldap_delete( Ri *, Re *, char ** );
static int op_ldap_modrdn( Ri *, Re *, char ** );
static LDAPMod *alloc_ldapmod();
static void free_ldapmod( LDAPMod * );
static void free_ldmarr( LDAPMod ** );
static int getmodtype( char * );
static int do_bind( Ri *, int * );
static int do_unbind( Ri * );


/* External references */
#ifndef SYSERRLIST_IN_STDIO
extern char *sys_errlist[];
#endif /* SYSERRLIST_IN_STDIO */

extern char *ch_malloc( unsigned long );

static char *kattrs[] = {"kerberosName", NULL };
static struct timeval kst = {30L, 0L};



/*
 * Determine the type of ldap operation being performed and call the
 * appropriate routine.
 * - If successful, returns ERR_DO_LDAP_OK
 * - If a retryable error occurs, ERR_DO_LDAP_RETRYABLE is returned.
 *   The caller should wait a while and retry the operation.
 * - If a fatal error occurs, ERR_DO_LDAP_FATAL is returned.  The caller
 *   should reject the operation and continue with the next replication
 *   entry.
 */
int
do_ldap(
    Ri		*ri,
    Re		*re,
    char	**errmsg
)
{
    int	rc = 0;
    int	lderr = LDAP_SUCCESS;
    int	retry = 2;
    char *msg;

    *errmsg = NULL;

    while ( retry > 0 ) {
	if ( ri->ri_ldp == NULL ) {
	    rc = do_bind( ri, &lderr );
	    if ( rc != BIND_OK ) {
		return DO_LDAP_ERR_RETRYABLE;
	    }
	}

	switch ( re->re_changetype ) {
	case T_ADDCT:
	    lderr = op_ldap_add( ri, re, errmsg );
	    if ( lderr != LDAP_SUCCESS ) {
		logInfo(get_ldap_message(MSG_SLURPOP1,
		    re->re_dn,*errmsg ? *errmsg : ldap_err2string( lderr )));

	    }
	    break;
	case T_MODIFYCT:
	    lderr = op_ldap_modify( ri, re, errmsg );
	    if ( lderr != LDAP_SUCCESS ) {
		logInfo(get_ldap_message(MSG_SLURPOP2,
		    re->re_dn,*errmsg ? *errmsg : ldap_err2string( lderr )));

	    }
	    break;
	case T_DELETECT:
	    lderr = op_ldap_delete( ri, re, errmsg );
	    if ( lderr != LDAP_SUCCESS ) {
		 logInfo(get_ldap_message(MSG_SLURPOP3,
		    re->re_dn,*errmsg ? *errmsg : ldap_err2string( lderr )));

	    }
	    break;
	case T_MODRDNCT:
	    lderr = op_ldap_modrdn( ri, re, errmsg );
	    if ( lderr != LDAP_SUCCESS ) {
		logInfo(get_ldap_message(MSG_SLURPOP4,
		    re->re_dn,*errmsg ? *errmsg : ldap_err2string( lderr )));

	    }
	    break;
	default:
	logInfo(get_ldap_message(MSG_SLURPOP5,
	    re->re_changetype, re->re_dn));

	    return DO_LDAP_ERR_FATAL;
	}

	/*
	 * Analyze return code.  If ok, just return.  If LDAP_SERVER_DOWN,
	 * we may have been idle long enough that the remote slapd timed
	 * us out.  Rebind and try again.
	 */
	if ( lderr == LDAP_SUCCESS ) {
	    return DO_LDAP_OK;
	} else if ( lderr == LDAP_SERVER_DOWN ) {
	    /* The LDAP server may have timed us out - rebind and try again */
	    (void) do_unbind( ri );
	    retry--;
	} else {
	    return DO_LDAP_ERR_FATAL;
	}
    }
    return DO_LDAP_ERR_FATAL;
}




/*
 * Perform an ldap add operation.
 */
static int
op_ldap_add(
    Ri		*ri,
    Re		*re,
    char	**errmsg
)
{
    Mi		*mi;
    int		nattrs, rc = 0, i;
    LDAPMod	*ldm, **ldmarr;
    int		lderr = 0;

    nattrs = i = 0;
    ldmarr = NULL;

    /*
     * Construct a null-terminated array of LDAPMod structs.
     */
    mi = re->re_mods;
    while ( mi[ i ].mi_type != NULL ) {
	ldm = alloc_ldapmod();
	ldmarr = ( LDAPMod ** ) ch_realloc( ldmarr,
		( nattrs + 2 ) * sizeof( LDAPMod * ));
	if ( ldmarr == NULL ) { return LDAP_NO_MEMORY; }

	ldmarr[ nattrs ] = ldm;
	ldm->mod_op = LDAP_MOD_BVALUES;
	ldm->mod_type = mi[ i ].mi_type;
	ldm->mod_bvalues =
		make_singlevalued_berval( mi[ i ].mi_val, mi[ i ].mi_len );
	i++;
	nattrs++;
    }

    if ( ldmarr != NULL ) {
	ldmarr[ nattrs ] = NULL;

	/* Perform the operation */
	rc = ldap_add_s( ri->ri_ldp, re->re_dn, ldmarr );
	lderr = ri->ri_ldp->ld_errno;
    } else {
	*errmsg = get_ldap_message(MSG_OPLDAPADD);

    }
    free_ldmarr( ldmarr );
    return( lderr ); 
}




/*
 * Perform an ldap modify operation.
 */
#define	AWAITING_OP -1
static int
op_ldap_modify(
    Ri		*ri,
    Re		*re,
    char	**errmsg
)
{
    Mi		*mi;
    int		state;	/* This code is a simple-minded state machine */
    int		nvals;	/* Number of values we're modifying */
    int		nops;	/* Number of LDAPMod structs in ldmarr */
    LDAPMod	*ldm, *nldm, **ldmarr;
    int		i, len;
    char	*type, *value;
    int		rc = 0;

    state = AWAITING_OP;
    nvals = 0;
    nops = 0;
    ldmarr = NULL;

    if ( re->re_mods == NULL ) {
	*errmsg = get_ldap_message(MSG_OPLDAPMOD1);
	return -1;
    }

    /*
     * Construct a null-terminated array of LDAPMod structs.
     */
    for ( mi = re->re_mods, i = 0; mi[ i ].mi_type != NULL; i++ ) {
	type = mi[ i ].mi_type;
	value = mi[ i ].mi_val;
	len = mi[ i ].mi_len;
	switch ( getmodtype( type )) {
	case T_MODSEP:
	    state = T_MODSEP; /* Got a separator line "-\n" */
	    continue;
	case T_MODOPADD:
	    state = T_MODOPADD;
	    ldmarr = ( LDAPMod ** )
		    ch_realloc(ldmarr, (( nops + 2 ) * ( sizeof( LDAPMod * ))));
	    if ( ldmarr == NULL ) { return -1; }

	    ldmarr[ nops ] = ldm = alloc_ldapmod();
	    ldm->mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
	    ldm->mod_type = value;
	    nvals = 0;
	    nops++;
	    break;
	case T_MODOPREPLACE:
	    state = T_MODOPREPLACE;
	    ldmarr = ( LDAPMod ** )
		    ch_realloc(ldmarr, (( nops + 2 ) * ( sizeof( LDAPMod * ))));
	    if ( ldmarr == NULL ) { return -1; }

	    ldmarr[ nops ] = ldm = alloc_ldapmod();
	    ldm->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
	    ldm->mod_type = value;
	    nvals = 0;
	    nops++;
	    break;
	case T_MODOPDELETE:
	    state = T_MODOPDELETE;
	    ldmarr = ( LDAPMod ** )
		    ch_realloc(ldmarr, (( nops + 2 ) * ( sizeof( LDAPMod * ))));
	    if ( ldmarr == NULL ) { return -1; }

	    ldmarr[ nops ] = ldm = alloc_ldapmod();
	    ldm->mod_op = LDAP_MOD_DELETE | LDAP_MOD_BVALUES;
	    ldm->mod_type = value;
	    nvals = 0;
	    nops++;
	    break;
	default:
	    if ( state == AWAITING_OP ) {
		continue;
	    }

	    /*
	     * We should have an attribute: value pair here.
	     * Construct the mod_bvalues part of the ldapmod struct.
	     */
	    if ( strcasecmp( type, ldm->mod_type )) {
		continue;
	    }
	    ldm->mod_bvalues = ( struct berval ** )
		    ch_realloc( ldm->mod_bvalues,
		    ( nvals + 2 ) * sizeof( struct berval * ));
	    if ( ldm->mod_bvalues == NULL ) { return -1; }

	    ldm->mod_bvalues[ nvals + 1 ] = NULL;
	    ldm->mod_bvalues[ nvals ] = ( struct berval * )
		    ch_malloc( sizeof( struct berval ));
	    if ( ldm->mod_bvalues[ nvals ] == NULL ) {
		return( -1 );
	    }
	    ldm->mod_bvalues[ nvals ]->bv_val = value;
	    ldm->mod_bvalues[ nvals ]->bv_len = len;
	    nvals++;
	}
    }
    ldmarr[ nops ] = NULL;

    if ( nops > 0 ) {
	/* Actually perform the LDAP operation */
	rc = ldap_modify_s( ri->ri_ldp, re->re_dn, ldmarr );
    }
    free_ldmarr( ldmarr );
    return( rc );
}




/*
 * Perform an ldap delete operation.
 */
static int
op_ldap_delete(
    Ri		*ri,
    Re		*re,
    char	**errmsg
)
{
    int		rc;

    rc = ldap_delete_s( ri->ri_ldp, re->re_dn );

    return( rc );
}




/*
 * Perform an ldap modrdn operation.
 */
#define	GOT_NEWRDN		1
#define	GOT_DRDNFLAGSTR		2
#define	GOT_ALLNEWRDNFLAGS	( GOT_NEWRDN | GOT_DRDNFLAGSTR )
static int
op_ldap_modrdn(
    Ri		*ri,
    Re		*re,
    char	**errmsg
)
{
    int		rc = 0;
    Mi		*mi;
    int		i;
    int		state = 0;
    int		drdnflag = -1;
    char	*newrdn;

    if ( re->re_mods == NULL ) {
	*errmsg = get_ldap_message(MSG_OPLDAPMODRDN);
	return -1;
    }

    /*
     * Get the arguments: should see newrdn: and deleteoldrdn: args.
     */
    for ( mi = re->re_mods, i = 0; mi[ i ].mi_type != NULL; i++ ) {
	if ( !strcmp( mi[ i ].mi_type, T_NEWRDNSTR )) {
	    newrdn = mi[ i ].mi_val;
	    state |= GOT_NEWRDN;
	} else if ( !strcmp( mi[ i ].mi_type, T_DRDNFLAGSTR )) {
	    state |= GOT_DRDNFLAGSTR;
	    if ( !strcmp( mi[ i ].mi_val, "0" )) {
		drdnflag = 0;
	    } else if ( !strcmp( mi[ i ].mi_val, "1" )) {
		drdnflag = 1;
	    } else {
		*errmsg = get_ldap_message(MSG_OPLDAPMODRDN01);
		return -1;
	    }
	} else {
	    *errmsg = get_ldap_message(MSG_OPLDAPMODRDN02);
	    return -1;
	}
    }

    /*
     * Punt if we don't have all the args.
     */
    if ( state != GOT_ALLNEWRDNFLAGS ) {
	*errmsg = get_ldap_message(MSG_OPLDAPMODRDN03);
	return -1;
    }

    /* Do the modrdn */
    rc = ldap_modrdn2_s( ri->ri_ldp, re->re_dn, mi->mi_val, drdnflag );

    return( ri->ri_ldp->ld_errno );
}



/*
 * Allocate and initialize an ldapmod struct.
 */
static LDAPMod *
alloc_ldapmod()
{
    LDAPMod	*ldm;

    ldm = ( struct ldapmod * ) ch_malloc( sizeof ( struct ldapmod ));
    if ( ldm == NULL ) { return NULL; }
    ldm->mod_type = NULL;
    ldm->mod_bvalues = ( struct berval ** ) NULL;
    return( ldm );
}



/*
 * Free an ldapmod struct associated mod_bvalues.  NOTE - it is assumed
 * that mod_bvalues and mod_type contain pointers to the same block of memory
 * pointed to by the repl struct.  Therefore, it's not freed here.
 */
static void
free_ldapmod(
LDAPMod *ldm )
{
    int		i;

    if ( ldm == NULL ) {
	return;
    }
    if ( ldm->mod_bvalues != NULL ) {
	for ( i = 0; ldm->mod_bvalues[ i ] != NULL; i++ ) {
	    free( ldm->mod_bvalues[ i ] );
	}
	free( ldm->mod_bvalues );
    }
    free( ldm );
    return;
}


/*
 * Free an an array of LDAPMod pointers and the LDAPMod structs they point
 * to.
 */
static void
free_ldmarr(
LDAPMod **ldmarr )
{
    int	i;

    for ( i = 0; ldmarr[ i ] != NULL; i++ ) {
	free_ldapmod( ldmarr[ i ] );
    }
    free( ldmarr );
}


/*
 * Create a berval with a single value. 
 */
static struct berval **
make_singlevalued_berval( 
char	*value,
int	len )
{
    struct berval **p;

    p = ( struct berval ** ) ch_malloc( 2 * sizeof( struct berval * ));
    if ( p == NULL ) { return NULL; }
    p[ 0 ] = ( struct berval * ) ch_malloc( sizeof( struct berval ));
    if ( p[0] == NULL ) { return NULL; }
    p[ 1 ] = NULL;
    p[ 0 ]->bv_val = value;
    p[ 0 ]->bv_len = len;
    return( p );
}


/*
 * Given a modification type (string), return an enumerated type.
 * Avoids ugly copy in op_ldap_modify - lets us use a switch statement
 * there.
 */
static int
getmodtype( 
char *type )
{
    if ( !strcmp( type, T_MODSEPSTR )) {
	return( T_MODSEP );
    }
    if ( !strcmp( type, T_MODOPADDSTR )) {
	return( T_MODOPADD );
    }
    if ( !strcmp( type, T_MODOPREPLACESTR )) {
	return( T_MODOPREPLACE );
    }
    if ( !strcmp( type, T_MODOPDELETESTR )) {
	return( T_MODOPDELETE );
    }
    return( T_ERR );
}


/*
 * Perform an LDAP unbind operation.  If replica is NULL, or the
 * repl_ldp is NULL, just return LDAP_SUCCESS.  Otherwise, unbind,
 * set the ldp to NULL, and return the result of the unbind call.
 */
static int
do_unbind(
    Ri	*ri
)
{
    int		rc = LDAP_SUCCESS;

    if (( ri != NULL ) && ( ri->ri_ldp != NULL )) {
	rc = ldap_unbind( ri->ri_ldp );
	if ( rc != LDAP_SUCCESS ) {
	    logInfo(get_ldap_message(MSG_DOUNBIND,
		ldap_err2string( rc ), ri->ri_hostname, ri->ri_port ));
	}
	ri->ri_ldp = NULL;
    }
    return rc;
}



/*
 * Perform an LDAP bind operation to the replication site given
 * by replica.  If replica->repl_ldp is non-NULL, then we unbind
 * from the replica before rebinding.  It should be safe to call
 * this to re-connect if the replica's connection goes away
 * for some reason.
 *
 * Returns 0 on success, -1 if an LDAP error occurred, and a return
 * code > 0 if some other error occurred, e.g. invalid bind method.
 * If an LDAP error occurs, the LDAP error is returned in lderr.
 */
static int
do_bind( 
    Ri	*ri,
    int	*lderr
)
{
    int		rc;
    int		ldrc;
    char	msgbuf[ 1024];
#ifdef KERBEROS
    int retval = 0;
    int kni, got_tgt;
    char **krbnames;
    char *skrbnames[ 2 ];
    char realm[ REALM_SZ ];
    char name[ ANAME_SZ ];
    char instance[ INST_SZ ];
#endif /* KERBEROS */

    *lderr = 0;

    if ( ri == NULL ) {
	return( BIND_ERR_BADRI );
    }

    if ( ri->ri_ldp != NULL ) {
	ldrc = ldap_unbind( ri->ri_ldp );
	if ( ldrc != LDAP_SUCCESS ) {

	    logInfo(get_ldap_message(MSG_DOBIND001,
		ldap_err2string(ldrc)));
	}
	ri->ri_ldp = NULL;
    }

    logDebug(LDAP_SLURPD,
	"(do_bind) Open connection to %s:%d\n",
	ri->ri_hostname, ri->ri_port, 0 );

    ri->ri_ldp = ldap_open( ri->ri_hostname, ri->ri_port );
    if ( ri->ri_ldp == NULL ) {

	logInfo(get_ldap_message(MSG_DOBIND002,
	    ri->ri_hostname, ri->ri_port, sys_errlist[ errno ] ));

	return( BIND_ERR_OPEN );
    }

    /*
     * Set ldap library options to (1) not follow referrals, and 
     * (2) restart the select() system call.
     */
#ifdef LDAP_REFERRALS
    ri->ri_ldp->ld_options &= ~LDAP_FLAG_REFERRALS;
#endif /* LDAP_REFERRALS */
    ri->ri_ldp->ld_options |= LDAP_FLAG_RESTART;

    switch ( ri->ri_bind_method ) {
    case AUTH_SIMPLE:
	/*
	 * Bind with a plaintext password.
	 */
	logDebug(LDAP_SLURPD,
	    "(do_bind) bind to %s:%d as %s (simple)\n",
	    ri->ri_hostname, ri->ri_port, ri->ri_bind_dn );

	ldrc = ldap_simple_bind_s( ri->ri_ldp, ri->ri_bind_dn,
		ri->ri_password );
	if ( ldrc != LDAP_SUCCESS ) {

	    logInfo(get_ldap_message(MSG_DOBIND003,
		    ri->ri_hostname, ri->ri_port, ldap_err2string( ldrc )));

	    *lderr = ldrc;
	    return( BIND_ERR_SIMPLE_FAILED );
	} else {
	    return( BIND_OK );
	}
	break;
    default:

	logInfo(get_ldap_message(MSG_DOBIND004,
		ri->ri_bind_method, ri->ri_hostname, ri->ri_port ));

	return( BIND_ERR_BAD_ATYPE );
    }
}
