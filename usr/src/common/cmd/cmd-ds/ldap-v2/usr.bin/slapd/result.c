/* @(#)result.c	1.4
 *
 * result.c - routines to send ldap results, errors, and referrals
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include "portable.h"
#include "slap.h"

#ifndef SYSERRLIST_IN_STDIO
extern int		sys_nerr;
extern char		*sys_errlist[];
#endif
extern int		active_threads;
extern pthread_mutex_t	active_threads_mutex;
extern pthread_mutex_t	new_conn_mutex;
extern pthread_t	listener_tid;
extern struct acl	*acl_get_applicable();
extern long		num_entries_sent;
extern long		num_bytes_sent;
extern pthread_mutex_t	num_sent_mutex;

void	close_connection();

static void
send_ldap_result2(
    Connection	*conn,
    Operation	*op,
    int		err,
    char	*matched,
    char	*text,
    int		nentries
)
{
	BerElement	*ber;
	int		rc, sd;
	unsigned long	tag, bytes;

	logDebug( LDAP_LOG_CLNT, "=> send_ldap_result\n",0,0,0);
	logDebug( LDAP_LOG_CLNT, 
	    "(send_ldap_result) %d:%s:%s\n", err, matched ?
	    matched : "", text ? text : "" );

	switch ( op->o_tag ) {
	case LBER_DEFAULT:
		tag = LBER_SEQUENCE;
		break;

	case LDAP_REQ_SEARCH:
		tag = LDAP_RES_SEARCH_RESULT;
		break;

	case LDAP_REQ_DELETE:
		tag = LDAP_RES_DELETE;
		break;

	default:
		tag = op->o_tag + 1;
		break;
	}

#ifdef COMPAT30
	if ( (ber = ber_alloc_t( conn->c_version == 30 ? 0 : LBER_USE_DER ))
	    == NULLBER ) {
#else
	if ( (ber = der_alloc()) == NULLBER ) {
#endif
		logDebug( (LDAP_LOG_CLNT | LDAP_LOG_BER),
		    "(send_ldap_result) ber_alloc failed\n", 0, 0, 0 );
		logDebug( LDAP_LOG_CLNT, "<= send_ldap_result\n",0,0,0);
		return;
	}

#ifdef CLDAP
	if ( op->o_cldap ) {
		rc = ber_printf( ber, "{is{t{ess}}}", op->o_msgid, "", tag,
		    err, matched ? matched : "", text ? text : "" );
	} else
#endif
#ifdef COMPAT30
	if ( conn->c_version == 30 ) {
		rc = ber_printf( ber, "{it{{ess}}}", op->o_msgid, tag, err,
		    matched ? matched : "", text ? text : "" );
	} else
#endif
		rc = ber_printf( ber, "{it{ess}}", op->o_msgid, tag, err,
		    matched ? matched : "", text ? text : "" );

	if ( rc == -1 ) {
		logDebug( (LDAP_LOG_CLNT | LDAP_LOG_BER),
		    "(send_ldap_result) ber_printf failed\n", 0, 0, 0 );
		logDebug( LDAP_LOG_CLNT, "<= send_ldap_result\n",0,0,0);
		return;
	}

	/* write only one pdu at a time - wait til it's our turn */
	pthread_mutex_lock( &conn->c_pdumutex );

	/* write the pdu */
	bytes = ber->ber_ptr - ber->ber_buf;
	pthread_mutex_lock( &new_conn_mutex );
	while ( conn->c_connid == op->o_connid && ber_flush( &conn->c_sb, ber,
	    1 ) != 0 ) {
		pthread_mutex_unlock( &new_conn_mutex );
		/*
		 * we got an error.  if it's ewouldblock, we need to
		 * wait on the socket being writable.  otherwise, figure
		 * it's a hard error and return.
		 */

		logDebug( (LDAP_LOG_NETWORK | LDAP_LOG_CLNT | LDAP_LOG_BER), 
		    "(send_ldap_result) ber_flush failed errno %d msg (%s)\n",
		    errno, errno > -1 && errno < sys_nerr ? sys_errlist[errno]
		    : "unknown", 0 );

		if ( errno != EWOULDBLOCK && errno != EAGAIN ) {
			close_connection( conn, op->o_connid, op->o_opid );

			pthread_mutex_unlock( &conn->c_pdumutex );
			logDebug( LDAP_LOG_CLNT, "<= send_ldap_result\n",0,0,0);
			return;
		}

		/* wait for socket to be write-ready */
		pthread_mutex_lock( &active_threads_mutex );
		active_threads--;
		conn->c_writewaiter = 1;
		pthread_kill( listener_tid, SIGUSR1 );
		pthread_cond_wait( &conn->c_wcv, &active_threads_mutex );
		pthread_mutex_unlock( &active_threads_mutex );

		pthread_yield();
		pthread_mutex_lock( &new_conn_mutex );
	}
	pthread_mutex_unlock( &new_conn_mutex );
	pthread_mutex_unlock( &conn->c_pdumutex );

	pthread_mutex_lock( &num_sent_mutex );
	num_bytes_sent += bytes;
	pthread_mutex_unlock( &num_sent_mutex );

	logDebug( LDAP_LOG_CLNT, "<= send_ldap_result\n",0,0,0);
	return;
}

void
send_ldap_result(
    Connection	*conn,
    Operation	*op,
    int		err,
    char	*matched,
    char	*text
)
{
#ifdef CLDAP
	if ( op->o_cldap ) {
		SAFEMEMCPY( (char *)conn->c_sb.sb_useaddr, &op->o_clientaddr,
		    sizeof( struct sockaddr ));
		logDebug( LDAP_LOG_NETWORK, "UDP response to %s port %d\n", 
		    inet_ntoa(((struct sockaddr_in *)
		    conn->c_sb.sb_useaddr)->sin_addr ),
		    ((struct sockaddr_in *) conn->c_sb.sb_useaddr)->sin_port,
		    0 );
	}
#endif
	send_ldap_result2( conn, op, err, matched, text, 0 );
}

void
send_ldap_search_result(
    Connection	*conn,
    Operation	*op,
    int		err,
    char	*matched,
    char	*text,
    int		nentries
)
{
	send_ldap_result2( conn, op, err, matched, text, nentries );
}

int
send_search_entry(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    Entry	*e,
    char	**attrs,
    int		attrsonly
)
{
	BerElement	*ber;
	Attribute	*a;
	int		i, rc, bytes, sd;
	struct acl	*acl;

	logDebug( LDAP_LOG_CLNT, "=> send_search_entry (%s)\n", e->e_dn, 0, 0 );

	if ( ! access_allowed( be, conn, op, e, "entry", NULL, op->o_dn,
	    ACL_READ ) ) {
		logDebug( (LDAP_LOG_ACL | LDAP_LOG_CLNT), 
		    "(send_search_entry) acl: access to entry not allowed\n",
		    0, 0, 0 );
		logDebug( LDAP_LOG_CLNT, "<= send_search_entry\n", 0, 0, 0 );
		return( 1 );
	}

#ifdef COMPAT30
	if ( (ber = ber_alloc_t( conn->c_version == 30 ? 0 : LBER_USE_DER ))
	    == NULLBER ) {
#else
	if ( (ber = der_alloc()) == NULLBER ) {
#endif
		logDebug( (LDAP_LOG_CLNT | LDAP_LOG_BER), 
		    "(send_search_entry) ber_alloc failed\n", 0, 0, 0 );
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, NULL,
		    "ber_alloc" );
		logDebug( LDAP_LOG_CLNT, "<= send_search_entry\n", 0, 0, 0 );
		return( 1 );
	}

#ifdef COMPAT30
	if ( conn->c_version == 30 ) {
		rc = ber_printf( ber, "{it{{s{", op->o_msgid,
		    LDAP_RES_SEARCH_ENTRY, e->e_dn );
	} else
#endif
		rc = ber_printf( ber, "{it{s{", op->o_msgid,
		    LDAP_RES_SEARCH_ENTRY, e->e_dn );

	if ( rc == -1 ) {
		logDebug( (LDAP_LOG_CLNT | LDAP_LOG_BER),
		    "(send_search_entry) ber_printf failed\n", 0, 0, 0 );
		ber_free( ber, 1 );
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, NULL,
		    "ber_printf dn" );
		logDebug( LDAP_LOG_CLNT, "<= send_search_entry\n", 0, 0, 0 );
		return( 1 );
	}

	for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
		if ( attrs != NULL && ! charray_inlist( attrs, a->a_type ) ) {
			continue;
		}

		acl = acl_get_applicable( be, op, e, a->a_type );

		if ( ! acl_access_allowed( acl, be, conn, e, NULL, op,
		    ACL_READ ) ) {
			continue;
		}

		if ( ber_printf( ber, "{s[", a->a_type ) == -1 ) {
			logDebug( (LDAP_LOG_CLNT | LDAP_LOG_BER), 
			    "(send_search_entry) ber_printf failed\n", 0, 0, 0 );
			ber_free( ber, 1 );
			send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR,
			    NULL, "ber_printf type" );
			logDebug( LDAP_LOG_CLNT, "<= send_search_entry\n", 0, 0, 0 );
			return( 1 );
		}

		if ( ! attrsonly ) {
			for ( i = 0; a->a_vals[i] != NULL; i++ ) {
				if ( a->a_syntax & SYNTAX_DN &&
				    ! acl_access_allowed( acl, be, conn, e,
				    a->a_vals[i], op, ACL_READ ) )
				{
					continue;
				}

				if ( ber_printf( ber, "o",
				    a->a_vals[i]->bv_val,
				    a->a_vals[i]->bv_len ) == -1 )
				{
					logDebug( (LDAP_LOG_CLNT | LDAP_LOG_BER),
					    "(send_search_entry) ber_printf failed\n",
					    0, 0, 0 );
					ber_free( ber, 1 );
					send_ldap_result( conn, op,
					    LDAP_OPERATIONS_ERROR, NULL,
					    "ber_printf value" );
					logDebug( LDAP_LOG_CLNT, 
					    "<= send_search_entry\n", 0, 0, 0 );
					return( 1 );
				}
			}
		}

		if ( ber_printf( ber, "]}" ) == -1 ) {
			logDebug( (LDAP_LOG_CLNT | LDAP_LOG_BER), 
			    "(send_search_entry) ber_printf failed\n", 0, 0, 0 );
			ber_free( ber, 1 );
			send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR,
			    NULL, "ber_printf type end" );
			logDebug( LDAP_LOG_CLNT, "<= send_search_entry\n", 0, 0, 0 );
			return( 1 );
		}
	}

#ifdef COMPAT30
	if ( conn->c_version == 30 ) {
		rc = ber_printf( ber, "}}}}" );
	} else
#endif
		rc = ber_printf( ber, "}}}" );

	if ( rc == -1 ) {
		logDebug( (LDAP_LOG_CLNT | LDAP_LOG_BER),
		    "(send_search_entry) ber_printf failed\n", 0, 0, 0 );
		ber_free( ber, 1 );
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, NULL,
		    "ber_printf entry end" );
		logDebug( LDAP_LOG_CLNT, "<= send_search_entry\n", 0, 0, 0 );
		return( 1 );
	}

	/* write only one pdu at a time - wait til it's our turn */
	pthread_mutex_lock( &conn->c_pdumutex );

	bytes = ber->ber_ptr - ber->ber_buf;
	pthread_mutex_lock( &new_conn_mutex );
	while ( conn->c_connid == op->o_connid && ber_flush( &conn->c_sb, ber,
	    1 ) != 0 ) {
		pthread_mutex_unlock( &new_conn_mutex );
		/*
		 * we got an error.  if it's ewouldblock, we need to
		 * wait on the socket being writable.  otherwise, figure
		 * it's a hard error and return.
		 */

		logDebug( (LDAP_LOG_CLNT | LDAP_LOG_BER), 
		    "(send_search_entry) ber_flush failed errno %d msg (%s)\n",
		    errno, errno > -1 && errno < sys_nerr ? sys_errlist[errno]
		    : "unknown", 0 );

		if ( errno != EWOULDBLOCK && errno != EAGAIN ) {
			close_connection( conn, op->o_connid, op->o_opid );

			pthread_mutex_unlock( &conn->c_pdumutex );
			logDebug( LDAP_LOG_CLNT, "<= send_search_entry\n", 0, 0, 0 );
			return( -1 );
		}

		/* wait for socket to be write-ready */
		pthread_mutex_lock( &active_threads_mutex );
		active_threads--;
		conn->c_writewaiter = 1;
		pthread_kill( listener_tid, SIGUSR1 );
		pthread_cond_wait( &conn->c_wcv, &active_threads_mutex );
		pthread_mutex_unlock( &active_threads_mutex );

		pthread_yield();
		pthread_mutex_lock( &new_conn_mutex );
	}
	pthread_mutex_unlock( &new_conn_mutex );
	pthread_mutex_unlock( &conn->c_pdumutex );

	pthread_mutex_lock( &num_sent_mutex );
	num_bytes_sent += bytes;
	num_entries_sent++;
	pthread_mutex_unlock( &num_sent_mutex );

	pthread_mutex_lock( &new_conn_mutex );
	if ( conn->c_connid == op->o_connid ) {
		rc = 0;
	} else {
		rc = -1;
	}
	pthread_mutex_unlock( &new_conn_mutex );

	logDebug( LDAP_LOG_CLNT, "<= send_search_entry\n", 0, 0, 0 );

	return( rc );
}

int
str2result(
    char	*s,
    int		*code,
    char	**matched,
    char	**info
)
{
	int	rc;
	char	*c;

	*code = LDAP_SUCCESS;
	*matched = NULL;
	*info = NULL;

	if ( strncasecmp( s, "RESULT", 6 ) != 0 ) {
		logDebug( LDAP_LOG_CLNT, "(str2result) (%s) expecting \"RESULT\"\n",
		    s, 0, 0 );

		return( -1 );
	}

	rc = 0;
	while ( (s = strchr( s, '\n' )) != NULL ) {
		*s++ = '\0';
		if ( *s == '\0' ) {
			break;
		}
		if ( (c = strchr( s, ':' )) != NULL ) {
			c++;
		}

		if ( strncasecmp( s, "code", 4 ) == 0 ) {
			if ( c != NULL ) {
				*code = atoi( c );
			}
		} else if ( strncasecmp( s, "matched", 7 ) == 0 ) {
			if ( c != NULL ) {
				*matched = c;
			}
		} else if ( strncasecmp( s, "info", 4 ) == 0 ) {
			if ( c != NULL ) {
				*info = c;
			}
		} else {
			logDebug( LDAP_LOG_CLNT, "(str2result)(%s) unknown\n",
			    s, 0, 0 );
			rc = -1;
		}
	}

	return( rc );
}

/*
 * close_connection - close a connection. takes the connection to close,
 * the connid associated with the operation generating the close (so we
 * don't accidentally close a connection that's not ours), and the opid
 * of the operation generating the close (for logging purposes).
 */
void
close_connection( Connection *conn, int opconnid, int opid )
{
	pthread_mutex_lock( &new_conn_mutex );
	if ( conn->c_sb.sb_sd != -1 && conn->c_connid == opconnid ) {
		close( conn->c_sb.sb_sd );
		conn->c_sb.sb_sd = -1;
#ifdef COMPAT
		conn->c_version = 0;
#endif
	}
	pthread_mutex_unlock( &new_conn_mutex );
}
