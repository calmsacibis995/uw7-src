/* @(#)connection.c	1.3
 *
 * connection.c - Handle incoming LDAP requests
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>

#include "portable.h"
#include "slap.h"
#include "ldaplog.h"

/* Messages */

#define MSG_THRCREATEFAILED \
     1,58,"Could not create thread to handle incoming LDAP request\n"


extern Operation	*op_add();
extern int		active_threads;
extern pthread_mutex_t	active_threads_mutex;
extern pthread_mutex_t	new_conn_mutex;
extern long		ops_initiated;
extern long		ops_completed;
extern pthread_mutex_t	ops_mutex;
extern pthread_t	listener_tid;
#ifndef SYSERRLIST_IN_STDIO
extern int		sys_nerr;
extern char		*sys_errlist[];
#endif

struct co_arg {
	Connection	*co_conn;
	Operation	*co_op;
};

/*
 * connection_activity - handle the request operation op on connection
 * conn.  This routine figures out what kind of operation it is and
 * calls the appropriate stub to handle it.
 */

static void
connection_operation( struct co_arg *arg )
{
	unsigned long	len;

	pthread_mutex_lock( &arg->co_conn->c_opsmutex );
	arg->co_conn->c_opsinitiated++;
	pthread_mutex_unlock( &arg->co_conn->c_opsmutex );

	pthread_mutex_lock( &ops_mutex );
	ops_initiated++;
	pthread_mutex_unlock( &ops_mutex );

	switch ( arg->co_op->o_tag ) {
	case LDAP_REQ_BIND:
		do_bind( arg->co_conn, arg->co_op );
		break;

#ifdef COMPAT30
	case LDAP_REQ_UNBIND_30:
#endif
	case LDAP_REQ_UNBIND:
		do_unbind( arg->co_conn, arg->co_op );
		break;

	case LDAP_REQ_ADD:
		do_add( arg->co_conn, arg->co_op );
		break;

#ifdef COMPAT30
	case LDAP_REQ_DELETE_30:
#endif
	case LDAP_REQ_DELETE:
		do_delete( arg->co_conn, arg->co_op );
		break;

	case LDAP_REQ_MODRDN:
		do_modrdn( arg->co_conn, arg->co_op );
		break;

	case LDAP_REQ_MODIFY:
		do_modify( arg->co_conn, arg->co_op );
		break;

	case LDAP_REQ_COMPARE:
		do_compare( arg->co_conn, arg->co_op );
		break;

	case LDAP_REQ_SEARCH:
		do_search( arg->co_conn, arg->co_op );
		break;

#ifdef COMPAT30
	case LDAP_REQ_ABANDON_30:
#endif
	case LDAP_REQ_ABANDON:
		do_abandon( arg->co_conn, arg->co_op );
		break;

	default:
		logDebug( (LDAP_LOG_CLNT | LDAP_LOG_NETWORK),
		    "(connection_operation) unknown request 0x%x\n",
		    arg->co_op->o_tag, 0, 0 );
		break;
	}

	pthread_mutex_lock( &arg->co_conn->c_opsmutex );
	arg->co_conn->c_opscompleted++;
	op_delete( &arg->co_conn->c_ops, arg->co_op );
	pthread_mutex_unlock( &arg->co_conn->c_opsmutex );

	free( (char *) arg );

	pthread_mutex_lock( &ops_mutex );
	ops_completed++;
	pthread_mutex_unlock( &ops_mutex );

	pthread_mutex_lock( &active_threads_mutex );
	active_threads--;
	pthread_mutex_unlock( &active_threads_mutex );
}

void
connection_activity(
    Connection *conn
)
{
	pthread_attr_t	attr;
	struct co_arg	*arg;
	unsigned long	tag, len;
	long		msgid;
	BerElement	*ber;
	char		*tmpdn;

	if ( conn->c_currentber == NULL && (conn->c_currentber = ber_alloc())
	    == NULL ) {
		logDebug((LDAP_LOG_BER | LDAP_LOG_CLNT | LDAP_LOG_NETWORK),
		     "(connection_activity) ber_alloc failed\n", 0, 0, 0 );
		return;
	}

	errno = 0;
	if ( (tag = ber_get_next( &conn->c_sb, &len, conn->c_currentber ))
	    != LDAP_TAG_MESSAGE ) {

		logDebug( (LDAP_LOG_BER | LDAP_LOG_CLNT | LDAP_LOG_NETWORK),
	"(connection_activity) ber_get_next on fd %d failed errno %d (%s)\n",
		    conn->c_sb.sb_sd, errno, errno > -1 && errno < sys_nerr ?
		    sys_errlist[errno] : "unknown" );

		logDebug( (LDAP_LOG_BER | LDAP_LOG_CLNT | LDAP_LOG_NETWORK),
		    "(connection_activity) got %d of %d so far\n",
		    conn->c_currentber->ber_rwptr - conn->c_currentber->ber_buf,
		    conn->c_currentber->ber_len, 0 );

		if ( errno != EWOULDBLOCK && errno != EAGAIN ) {
			/* log, close and send error */
			ber_free( conn->c_currentber, 1 );
			conn->c_currentber = NULL;

			close_connection( conn, conn->c_connid, -1 );
		}

		return;
	}
	ber = conn->c_currentber;
	conn->c_currentber = NULL;

	if ( (tag = ber_get_int( ber, &msgid )) != LDAP_TAG_MSGID ) {

		/* log, close and send error */
		logDebug( (LDAP_LOG_BER |LDAP_LOG_CLNT |LDAP_LOG_NETWORK),
		     "(connection_activity) ber_get_int returns 0x%x\n",
		     tag, 0, 0 );

		ber_free( ber, 1 );

		close_connection( conn, conn->c_connid, -1 );
		return;
	}

	if ( (tag = ber_peek_tag( ber, &len )) == LBER_ERROR ) {

		/* log, close and send error */
		logDebug( (LDAP_LOG_BER | LDAP_LOG_CLNT | LDAP_LOG_NETWORK),
		    "(connection_activity) ber_peek_tag returns 0x%x\n", tag,0,0);

		ber_free( ber, 1 );

		close_connection( conn, conn->c_connid, -1 );
		return;
	}

#ifdef COMPAT30
	if ( conn->c_version == 30 ) {
		(void) ber_skip_tag( ber, &len );
	}
#endif

	arg = (struct co_arg *) ch_malloc( sizeof(struct co_arg) );
	if ( arg == NULL ) {
		ber_free( ber, 1 );
		close_connection( conn, conn->c_connid, -1 );
		return;
	}
	arg->co_conn = conn;

	pthread_mutex_lock( &conn->c_dnmutex );
	if ( conn->c_dn != NULL ) {
		tmpdn = strdup( conn->c_dn );
	} else {
		tmpdn = NULL;
	}
	pthread_mutex_unlock( &conn->c_dnmutex );

	pthread_mutex_lock( &conn->c_opsmutex );
	arg->co_op = op_add( &conn->c_ops, ber, msgid, tag, tmpdn,
	    conn->c_opsinitiated, conn->c_connid );
	pthread_mutex_unlock( &conn->c_opsmutex );

	if ( tmpdn != NULL ) {
		free( tmpdn );
	}

	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
	if ( pthread_create( &arg->co_op->o_tid, attr,
	    (void *) connection_operation, (void *) arg ) != 0 ) {

		logError(get_ldap_message(MSG_THRCREATEFAILED));

	} else {
		pthread_mutex_lock( &active_threads_mutex );
		active_threads++;
		pthread_mutex_unlock( &active_threads_mutex );
	}
	pthread_attr_destroy( &attr );
}
