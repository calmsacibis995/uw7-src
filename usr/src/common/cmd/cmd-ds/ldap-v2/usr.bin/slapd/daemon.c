/* "@(#)daemon.c	1.10"
 *
 * Revision history
 *
 * 6 Jan 97	From 5-Jun-96    hodges
 *     Added locking of new_conn_mutex when traversing the c[] array.
 *
 * 5 Jun 97	tonylo
 *	pid is set in file
 *
 * 13 Jun 97	geoffh
 *	Added new signal handler (void) SIGNAL( SIGCHLD, SIG_IGN );
 *
 *	Changes made to handle reverse lookups. A condition has been added
 *	so that if a slapd ACL checks the domain of a client then the address
 *	of the client will automatically be looked up., rather than get the
 *	address of the client every time - or not at all.
 *	
 * 1 Aug 97   tonylo
 *	Logging scheme changes
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include "slap.h"
#include "portable.h"
#include "ldapconfig.h"

#ifdef NEED_FILIO		/* defined */
#include <sys/filio.h>
#else /* NEED_FILIO */
#include <sys/ioctl.h>
#endif /* NEED_FILIO */
#ifdef USE_SYSCONF              /* defined */
#include <unistd.h>
#endif /* USE_SYSCONF */
#include "ldaplog.h"

/* Messages */

#define MSG_SOCKET \
    1,59,"socket() call failed (errno %d), stopping daemon\n"
#define MSG_SETSOCK \
    1,60,"setsockopt() failed (errno %d)\n"
#define MSG_BIND \
    1,61,"bind() failed (errno %d), stopping daemon\n"
#define MSG_LISTEN \
    1,62,"listen() failed (errno %d), stopping daemon\n"
#define MSG_STARTSLAPD \
    1,63,"slapd started\n"
#define MSG_SLAPDDOWN \
    1,64,"slapd shutting down - waiting for %d threads to terminate\n"
#define MSG_SLAPDSTOPPED \
    1,65,"slapd stopped\n"
#define MSG_SLAPDSHUTSIG \
    1,66,"slapd got shutdown signal\n"


extern Operation	*op_add();
extern void		removePIDfile();
extern void		setPIDfile();

#ifndef SYSERRLIST_IN_STDIO
extern int		sys_nerr;
extern char		*sys_errlist[];
#endif
extern time_t		currenttime;
extern pthread_mutex_t	currenttime_mutex;
extern int		active_threads;
extern pthread_mutex_t	active_threads_mutex;
extern pthread_mutex_t	new_conn_mutex;
extern int		slapd_shutdown;
extern pthread_t	listener_tid;
extern int		num_conns;
extern pthread_mutex_t	ops_mutex;
extern int              getcallername;
int		dtblsize;
Connection	*c;

static void	set_shutdown();
static void	do_nothing();

void
daemon(
    int	port
)
{
	Operation		*o;
	BerElement		ber;
	unsigned long		len, tag, msgid;
	int			i;
	int			tcps, ns;
	struct sockaddr_in	addr;
	fd_set			readfds;
	fd_set			writefds;
	int			on = 1;

#ifdef USE_SYSCONF
        dtblsize = sysconf( _SC_OPEN_MAX );
#else /* USE_SYSCONF */
        dtblsize = getdtablesize();
#endif /* USE_SYSCONF */

	c = (Connection *) ch_calloc( 1, dtblsize * sizeof(Connection) );

	if ( c == NULL ) {
		exit ( LDAP_NO_MEMORY );
	}

	for ( i = 0; i < dtblsize; i++ ) {
		c[i].c_dn = NULL;
		c[i].c_addr = NULL;
		c[i].c_domain = NULL;
		c[i].c_ops = NULL;
		c[i].c_sb.sb_sd = -1;
		c[i].c_sb.sb_options = LBER_NO_READ_AHEAD;
		c[i].c_sb.sb_naddr = 0;
		c[i].c_sb.sb_ber.ber_buf = NULL;
		c[i].c_sb.sb_ber.ber_ptr = NULL;
		c[i].c_sb.sb_ber.ber_end = NULL;
		c[i].c_writewaiter = 0;
		c[i].c_connid = 0;
		pthread_mutex_init( &c[i].c_dnmutex,
		    pthread_mutexattr_default );
		pthread_mutex_init( &c[i].c_opsmutex,
		    pthread_mutexattr_default );
		pthread_mutex_init( &c[i].c_pdumutex,
		    pthread_mutexattr_default );
		pthread_cond_init( &c[i].c_wcv, pthread_condattr_default );
	}

	if ( (tcps = socket( AF_INET, SOCK_STREAM, 0 )) == -1 ) {
		logError(get_ldap_message(MSG_SOCKET, errno));
		removePIDfile();
		exit( 1 );
	}

	i = 1;
	if (setsockopt(tcps,SOL_SOCKET,SO_REUSEADDR,(char *)&i,sizeof(i))==-1){
		logInfo(get_ldap_message(MSG_SETSOCK,errno));
	}

	(void) memset( (void *) &addr, '\0', sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons( port );
	if ( bind( tcps, (struct sockaddr *) &addr, sizeof(addr) ) == -1 ) {
		logError(get_ldap_message(MSG_BIND, errno));
		removePIDfile();
		exit( 1 );
	}

	if ( listen( tcps, 5 ) == -1 ) {
		logError(get_ldap_message(MSG_LISTEN, errno));
		removePIDfile();
		exit( 1 );
	}

	(void) SIGNAL( SIGPIPE, SIG_IGN );
	(void) SIGNAL( SIGUSR1, (void(*)(int)) do_nothing );
	(void) SIGNAL( SIGUSR2, (void(*)(int)) set_shutdown );
	(void) SIGNAL( SIGTERM, (void(*)(int)) set_shutdown );
	(void) SIGNAL( SIGHUP, (void(*)(int)) set_shutdown );
	(void) SIGNAL( SIGCHLD, SIG_IGN );

	logDebug(LDAP_LOG_NETWORK, 
		"(daemon) Established initial network connection %d\n",tcps,0,0);

	logError(get_ldap_message(MSG_STARTSLAPD));

	/* I can now pretty much say that the slapd has started and 
	   write the PID to PIDFILENAME 
	*/
	setPIDfile();

	while ( !slapd_shutdown ) {
		struct sockaddr_in	from;
		struct hostent		*hp;
		struct timeval		zero;
		struct timeval		*tvp;
		size_t			len;
		pid_t			pid;
		int 			acceptfailflag=0;

		FD_ZERO( &writefds );
		FD_ZERO( &readfds );
		FD_SET( tcps, &readfds );

		pthread_mutex_lock( &active_threads_mutex );
		logDebug( LDAP_LOG_NETWORK,
		    "(daemon) listening for connections on socket no \"%d\"\n",tcps,0,0);

		pthread_mutex_lock( &new_conn_mutex );
		for ( i = 0; i < dtblsize; i++ ) {
			if ( c[i].c_sb.sb_sd != -1 ) {
				FD_SET( c[i].c_sb.sb_sd, &readfds );

				if ( c[i].c_writewaiter ) {
					FD_SET( c[i].c_sb.sb_sd, &writefds );
				}
			}
		}
		pthread_mutex_unlock( &new_conn_mutex );

		zero.tv_sec = 0;
		zero.tv_usec = 0;
		logDebug( LDAP_LOG_NETWORK,
		    "(daemon) Number of active threads before select() call=\"%d\"\n",
		    active_threads,0,0);

#ifdef PTHREAD_PREEMPTIVE	/* Defined */
		tvp = NULL;
#else
		tvp = active_threads ? &zero : NULL;
#endif
		pthread_mutex_unlock( &active_threads_mutex );

		switch ( select( dtblsize, &readfds, &writefds, 0, tvp ) ) {
		case -1:	/* failure - try again */
			logDebug( LDAP_LOG_NETWORK,
                            "(daemon) select() failed (errno %d,%s)\n", errno,logErrNoMsg,0);
			continue;

		case 0:		/* timeout - let threads run */
			logDebug( LDAP_LOG_NETWORK,
                            "(daemon) select() timeout - yielding\n",0,0,0);
			pthread_yield();
			continue;

		default:	/* something happened - deal with it */
			logDebug( LDAP_LOG_NETWORK,
                            "(daemon) select() picked up an operation\n",0,0,0);
			;	/* FALL */
		}
		pthread_mutex_lock( &currenttime_mutex );
		time( &currenttime );
		pthread_mutex_unlock( &currenttime_mutex );

		/* new connection */
		pthread_mutex_lock( &new_conn_mutex );
		if ( FD_ISSET( tcps, &readfds ) ) {
			len = sizeof(from);
			if ( (ns = accept( tcps, (struct sockaddr *) &from,
			    &len )) == -1 ) {
				/*
				This check has been put in so that if the 
				accept() failed then this fact is only 
				registered to the error or system logs once
				*/
				if( acceptfailflag != 1 ) {
					logDebug( LDAP_LOG_NETWORK,
					    "(daemon) accept() failed (errno %d)\n",
					    errno,0,0);
					acceptfailflag=1;
				}
				pthread_mutex_unlock( &new_conn_mutex );
				continue;
			}
			acceptfailflag=0;

			if ( ioctl( ns, FIONBIO, (caddr_t) &on ) == -1 ) {
				logDebug( LDAP_LOG_NETWORK,
				    "(daemon) FIONBIO ioctl on %d failed\n", ns,0,0);
			}
			c[ns].c_sb.sb_sd = ns;
			logDebug( LDAP_LOG_NETWORK,
			    "(daemon) new connection, descriptor=%d\n", ns,0,0);

			pthread_mutex_lock( &ops_mutex );
			c[ns].c_connid = num_conns++;
			pthread_mutex_unlock( &ops_mutex );
			len = sizeof(from);
			if ( getpeername( ns, (struct sockaddr *) &from, &len )
			    == 0 ) {
				char	*s;
				if (getcallername) {
					hp = gethostbyaddr( (char *)
				    		&(from.sin_addr.s_addr),
				    		sizeof(from.sin_addr.s_addr), 
						AF_INET );
				} else {
					hp = NULL;
				}

				logDebug(LDAP_LOG_BIND | LDAP_LOG_NETWORK,
				    "(daemon) got connection from %s\n",
				    (hp == NULL ? "unknown" : hp->h_name,
				    inet_ntoa( from.sin_addr )),0,0);

				if ( c[ns].c_addr != NULL ) {
					free( c[ns].c_addr );
				}
				c[ns].c_addr = strdup( inet_ntoa(
				    from.sin_addr ) );
				if ( c[ns].c_domain != NULL ) {
					free( c[ns].c_domain );
				}
				c[ns].c_domain = strdup( hp == NULL ? "" :
				    hp->h_name );
				/* normalize the domain */
				for ( s = c[ns].c_domain; *s; s++ ) {
					*s = TOLOWER( *s );
				}
			} else {
				logDebug(LDAP_LOG_BIND | LDAP_LOG_NETWORK,
				    "(daemon) got connection from unknown\n",0,0,0);
			}
			pthread_mutex_lock( &c[ns].c_dnmutex );
			if ( c[ns].c_dn != NULL ) {
				free( c[ns].c_dn );
				c[ns].c_dn = NULL;
			}
			pthread_mutex_unlock( &c[ns].c_dnmutex );
			c[ns].c_starttime = currenttime;
			c[ns].c_opsinitiated = 0;
			c[ns].c_opscompleted = 0;
		}
		pthread_mutex_unlock( &new_conn_mutex );

		for ( i = 0; i < dtblsize; i++ ) {
			int	r, w;

			r = FD_ISSET( i, &readfds );
			w = FD_ISSET( i, &writefds );
		}

		for ( i = 0; i < dtblsize; i++ ) {
			if ( i == tcps || (! FD_ISSET( i, &readfds ) &&
			    ! FD_ISSET( i, &writefds )) ) {
				continue;
			}

			if ( FD_ISSET( i, &writefds ) ) {
				logDebug( LDAP_LOG_NETWORK,
				    "(daemon) signaling write waiter on descriptor %d\n", i,0,0);

				pthread_mutex_lock( &active_threads_mutex );
				pthread_cond_signal( &c[i].c_wcv );
				c[i].c_writewaiter = 0;
				active_threads++;
				pthread_mutex_unlock( &active_threads_mutex );
			}

			if ( FD_ISSET( i, &readfds ) ) {
				logDebug( LDAP_LOG_NETWORK,
				    "(daemon) read activity on descriptor %d\n", i,0,0);

				connection_activity( &c[i] );
			}
		}

		pthread_yield();
	}

	close( tcps );
	pthread_mutex_lock( &active_threads_mutex );

	logError(get_ldap_message(MSG_SLAPDDOWN, active_threads));

	while ( active_threads > 0 ) {
		pthread_mutex_unlock( &active_threads_mutex );
		pthread_yield();
		pthread_mutex_lock( &active_threads_mutex );
	}
	pthread_mutex_unlock( &active_threads_mutex );

	/* let backends do whatever cleanup they need to do */
	logDebug( LDAP_LOG_NETWORK,
	    "(daemon) slapd shutting down - waiting for backends to close down\n",0,0,0);

	be_close();
	logError(get_ldap_message(MSG_SLAPDSTOPPED));
}

static void
set_shutdown()
{
	logError(get_ldap_message(MSG_SLAPDSHUTSIG));

	/* Remove pid and arguments file */
	removePIDfile();

	slapd_shutdown = 1;
	pthread_kill( listener_tid, SIGUSR1 );
	(void) SIGNAL( SIGUSR2, (void(*)(int)) set_shutdown );
	(void) SIGNAL( SIGTERM, (void(*)(int)) set_shutdown );
	(void) SIGNAL( SIGHUP, (void(*)(int)) set_shutdown );
}

static void
do_nothing()
{
	logInfo("slapd got SIGUSR1\n");
	(void) SIGNAL( SIGUSR1, (void(*)(int)) do_nothing );
}
