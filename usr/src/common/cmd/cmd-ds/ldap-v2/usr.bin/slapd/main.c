/* "@(#)main.c	1.7"
 *
 * Revision history:
 *
 * 20 Feb 97	 tonylo
 * 	I18N 
 *
 * 5 Jun 97	tonylo
 *	PID file is created
 *
 * 13 Jun 97	geoffh
 *	Changes made to handle reverse lookups. A condition has been added
 *	so that if a slapd ACL checks the domain of a client then the address
 *	of the client will automatically be looked up., rather than get the 
 *	address of the client every time - or not at all.
 *
 * 1 Aug 97     tonylo
 * 	Added ldaplog.h calls
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "portable.h"
#include "slap.h"
#include "ldapconfig.h"

#include "ldaplog.h"

extern void	daemon();
extern void	createPIDfile();
extern int      getcallername;

extern char Versionstr[];

/*
 * read-only global variables or variables only written by the listener
 * thread (after they are initialized) - no need to protect them with a mutex.
 */
int		udp;
int		slapd_shutdown;
char		*default_referral;
char		*configfile;
time_t		starttime;
pthread_t	listener_tid;
char            *PIDFILENAME;
/*
 * global variables that need mutex protection
 */
time_t		currenttime;
pthread_mutex_t	currenttime_mutex;
int		active_threads;
pthread_mutex_t	active_threads_mutex;
pthread_mutex_t	new_conn_mutex;
long		ops_initiated;
long		ops_completed;
int		num_conns;
pthread_mutex_t	ops_mutex;
long		num_entries_sent;
long		num_bytes_sent;
pthread_mutex_t	num_sent_mutex;
/*
 * these mutexes must be used when calling the entry2str()
 * routine since it returns a pointer to static data.
 */
pthread_mutex_t	entry2str_mutex;
pthread_mutex_t	replog_mutex;
#ifndef sunos5
pthread_mutex_t	regex_mutex;
#endif

/* messages */

#define MSG_USAGE \
    1,68,"usage: %s [-d debuglevel] [-f configfile] [-p port] [-s sysloglevel]\n"
#define MSG_DBLEVELS \
    1,69,"Debug levels:\n"
#define MSG_NOTHREADS \
    1,70,"listener pthread_create failed: Could not start daemon\n"
#define MSG_COULDNOTBIND \
    1,71,"Cannot bind to port %d, slapd not started (Another process maybe using this port)\n"
#define MSG_SOCKET \
    1,72,"socket() call failed (errno %d), exitting slapd\n"
#define MSG_TAGERR \
    1,73,"Fatal networking error occured decoding BER element\n"
#define MSG_TAGERR2 \
    1,74,"Fatal networking error occured decoding BER tag\n"

/* prototypes */
static void	usage( char* name );
static void	write_args_file(int, char**, int);
static int	attach_to_port(int portno);


main( argc, argv )
    int		argc;
    char	**argv;
{
	int		i;
	int		inetd = 0;
	int		port;
	char		*myname;
	Backend		*be = NULL;
	FILE		*fp = NULL;
	extern char	*optarg;

	/* i18n stuff */
	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:slapd");

	open_message_catalog("ldap.cat");


	configfile = SLAPD_DEFAULT_CONFIGFILE;
	port = LDAP_PORT;

	while ( (i = getopt( argc, argv, "d:f:ip:s:u" )) != EOF ) {
		switch ( i ) {
		case 'd':	/* turn on debugging */
			if ( optarg[0] == '?' ) {
				logPrintLevels(); /* From liblog */
				exit( 0 );
			} else {
				ldapdebug_level = atoi( optarg );

				/* Check to see if lber debugging needs
				 * to be set
				 */
				if(ldapdebug_level & LDAP_LOG_BER) {
				    lberSetDebug(1);
				}
			}
			break;
		case 'f':	/* read config file */
			configfile = strdup( optarg );
			break;

		case 'i':	/* run from inetd */
			inetd = 1;
			break;

		case 'p':	/* port on which to listen */
			port = atoi( optarg );
			break;

		case 's':	/* set syslog level */
			ldapsyslog_level = atoi( optarg );
			break;

		case 'u':	/* do udp */
			udp = 1;
			break;

		default:
			usage( argv[0] );
			exit( 1 );
		}
	}
	/* This sets the identifier of messages going to the system log */
	if ( (myname = strrchr( argv[0], '/' )) == NULL )
		myname = strdup( argv[0] );
	else
		myname = strdup( myname + 1 );
	openlog( myname, OPENLOG_OPTIONS, LOG_DAEMON);

	/* Test whether the port is available before pid file is written */
	if( attach_to_port(port)) {
		logDebug(LDAP_LOG_NETWORK,"(main) Port %d should be available\n",
		    port,0,0);
	}

	/* open info log */
	open_info_log(port, "w");

	write_args_file(argc, argv, port);
	createPIDfile();

	/* open config */
	fp = fopen( configfile, "r" );

	/* initialise some thread vars */
	init();
	open_info_log(port, "w+");

	/* read the slapd configuration file */
	read_config( configfile, &be, fp );

	if ( ! inetd ) {
		pthread_attr_t	attr;
		int		status;

		/* close ttys and stuff */
		close_info_log();
		detach();

		/* Reopen message log */
		open_info_log(port, "a+");

		time( &starttime );
		pthread_attr_init( &attr );
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

		if ( pthread_create( &listener_tid, attr, (void *)daemon, (void *)port ) != 0 ) {
			logError( get_ldap_message(MSG_NOTHREADS) );
			exit( 1 );
		}
		pthread_attr_destroy( &attr );
		pthread_join( listener_tid, (void *) &status );
		pthread_exit( 0 );
	} else {
		Connection		c;
		Operation		*o;
		BerElement		ber;
		unsigned long		len, tag;
		long			msgid;
		size_t			flen;
		struct sockaddr_in	from;
		struct hostent		*hp;

		c.c_dn = NULL;
		c.c_ops = NULL;
		c.c_sb.sb_sd = 0;
		c.c_sb.sb_options = 0;
		c.c_sb.sb_naddr = udp ? 1 : 0;
		c.c_sb.sb_ber.ber_buf = NULL;
		c.c_sb.sb_ber.ber_ptr = NULL;
		c.c_sb.sb_ber.ber_end = NULL;
		pthread_mutex_init( &c.c_dnmutex, pthread_mutexattr_default );
		pthread_mutex_init( &c.c_opsmutex, pthread_mutexattr_default );
		pthread_mutex_init( &c.c_pdumutex, pthread_mutexattr_default );
#ifdef notdefcldap
		c.c_sb.sb_addrs = (void **) saddrlist;
		c.c_sb.sb_fromaddr = &faddr;
		c.c_sb.sb_useaddr = saddrlist[ 0 ] = &saddr;
#endif
		flen = sizeof(from);
		if ( getpeername( 0, (struct sockaddr *) &from, &flen ) == 0 ) {
			if (getcallername) {
				hp = gethostbyaddr((char *)&(from.sin_addr.s_addr),
			    		sizeof(from.sin_addr.s_addr), AF_INET );
			} else {
				hp = NULL;
			}

			logDebug( LDAP_LOG_NETWORK, "(main) connection from %s (%s)\n",
			    hp == NULL ? "unknown" : hp->h_name,
			    inet_ntoa( from.sin_addr ), 0 );

			c.c_addr = inet_ntoa( from.sin_addr );
			c.c_domain = strdup( hp == NULL ? "" : hp->h_name );
		} else {
			logDebug( LDAP_LOG_NETWORK, "(main) connection from unknown\n",
			    0, 0, 0 );
		}

		ber_init( &ber, 0 );
		while ( (tag = ber_get_next( &c.c_sb, &len, &ber ))
		    == LDAP_TAG_MESSAGE ) {
			pthread_mutex_lock( &currenttime_mutex );
			time( &currenttime );
			pthread_mutex_unlock( &currenttime_mutex );

			if ( (tag = ber_get_int( &ber, &msgid ))
			    != LDAP_TAG_MSGID ) {
				/* log and send error */

				logError(get_ldap_message(MSG_TAGERR2));

				return;
			}

			if ( (tag = ber_peek_tag( &ber, &len ))
			    == LBER_ERROR ) {
				/* log, close and send error */

				logError(get_ldap_message(MSG_TAGERR));

				ber_free( &ber, 1 );
				close( c.c_sb.sb_sd );
				c.c_sb.sb_sd = -1;
				return;
			}

			connection_activity( &c );

			ber_free( &ber, 1 );
		}
	}
}


static void
usage( char* name )
{
	logError(get_ldap_message(MSG_USAGE,name));
}

static void
write_args_file(int g_argc, char **g_argv, int port)
{
	FILE	*fp;
	char	argfilename[50]="";
	int	i;

        mkdir("/var/ldap/args",755);
        sprintf(argfilename, "/var/ldap/args/slapd.args.%d",port);
        if ( (fp = fopen( argfilename, "w" )) != NULL ) {
                for ( i = 0; i < g_argc; i++ ) {
                        fprintf( fp, "%s ", g_argv[i] );
                }
                fprintf( fp, "\n" );
                fclose( fp );
        }

}


int 
attach_to_port(int portno)
{
   int			tcps;
   int			i;
   struct sockaddr_in	addr;

   if ( (tcps = socket( AF_INET, SOCK_STREAM, 0 )) == -1 ) {
	logError(get_ldap_message(MSG_SOCKET, errno));
	exit(1);
   }

   i=1;
   if ( setsockopt( tcps, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i) ) == -1 ) {
	logDebug(LDAP_LOG_NETWORK,"setsockopt() failed (errno %d)\n",
	    errno, 0, 0);
   }
   memset( &addr, '\0', sizeof(addr) );

   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons( portno );

   if ( bind( tcps, (struct sockaddr *) &addr, sizeof(addr) ) == -1 ) {
	logError(get_ldap_message(MSG_COULDNOTBIND, portno));
	exit(1);
   }
   close(tcps);
   return(1);
}
