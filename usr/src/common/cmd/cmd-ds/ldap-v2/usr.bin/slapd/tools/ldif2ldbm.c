#ident	"@(#)ldif2ldbm.c	1.7"
#ident	"$Header$"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include "ldapconfig.h"
#include "ldaplog.h"
#include "../slap.h"
#include "../back-ldbm/back-ldbm.h"
#include "ldaplog.h"


#define DEFAULT_CMDDIR		"/usr/bin"
#define INDEXCMD		"ldif2index"
#define ID2ENTRYCMD		"ldif2id2entry"
#define ID2CHILDRENCMD		"ldif2id2children"

/* This will get the first ldbm database defn in the config file */
#define DEFAULT_DBNUM -1


extern void		attr_index_config();
extern char		*str_getline();
extern int		strcasecmp();
extern int		nbackends;
extern Backend		*backends;

int		global_schemacheck;
int		num_entries_sent;
int		num_bytes_sent;
int		active_threads;
char		*default_referral;
struct objclass	*global_oc;
time_t		currenttime;
pthread_t	listener_tid;
pthread_mutex_t	num_sent_mutex;
pthread_mutex_t	entry2str_mutex;
pthread_mutex_t	active_threads_mutex;
pthread_mutex_t	new_conn_mutex;
pthread_mutex_t	currenttime_mutex;
pthread_mutex_t	replog_mutex;
pthread_mutex_t	ops_mutex;
pthread_mutex_t	regex_mutex;

static void	fork_child();
static void	wait4kids();

static char	*indexcmd;
static char	*tailorfile;
static char	*inputfile;
static int      maxkids = 1;
static int      nkids;
 

/* Messages */
#define MSG_USAGE \
    1,130,"Usage: %s -i ldiffile [options]\n\
    -i   ldiffile        ldif format file specifying the directory data\n\
options:\n\
    -f   file            configuration file, default is slapd.conf\n\
    -n   dbnum           which database in the config file\n\
    -j   #jobs           number of parallel processes for building the database\n\
    -c   cmddir          location of subsidiary commands such as ldif2index\n\
    -d   debuglevel      control level of debug output, default 0\n"

#define MSG_DBNUM1 \
    1,131,"Database number selected via -n is out of range\n\
Must be in the range 1 to %d (number of databases in the config file)\n"

#define MSG_DBNUM2 \
    1,132,"Database number selected via -n is not an ldbm database\n"
#define MSG_NOLDBM \
    1,133,"No ldbm database found in the configuration file\n"
#define MSG_VIS1 \
    1,134,"Line %d has no visible symbols, treated as an empty line\n"
#define MSG_BDLINE1 \
    1,135,"Bad line %d in entry ending at line %d ignored\n"
#define MSG_STOPSIG1 \
    1,136,"Stopping: child stopped with signal %d\n"
#define MSG_STOPSIG2 \
    1,137,"Stopping: child terminated with signal %d\n"
#define MSG_STOPSIG3 \
    1,138,"Stopping: child exited with status %d\n"
#define MSG_COULDNOTFORK \
    5,29,"Could not fork to run %s\n"
#define MSG_NOBACKENDDB \
    5,31,"No ldbm backend database in the configuration file \"%s\"\n"



static void
usage( char *name )
{
	logError(get_ldap_message(MSG_USAGE,name));
	exit( 1 );
}

main( int argc, char **argv )
{
	int		i, stop, status;
	char		*linep, *buf, *cmddir;
	char		*args[10];
	char		buf2[20], buf3[20];
	char		line[BUFSIZ];
	char		cmd[MAXPATHLEN];
	int		lineno, elineno;
	int      	lmax, lcur;
	int		dbnum;
	ID		id;
	Backend		*be = NULL;
	struct berval	bv;
	struct berval	*vals[2];
	Avlnode		*avltypes = NULL;
	extern char	*optarg;

	/* i18n stuff */
	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:ldif2ldbm");
        open_message_catalog("ldap.cat");

	cmddir		= DEFAULT_CMDDIR;
	tailorfile	= DEFAULT_CONFIGFILE;
	dbnum		= DEFAULT_DBNUM;

	while ( (i = getopt( argc, argv, "d:c:f:i:j:n:" )) != EOF ) {
		switch ( i ) {
		case 'd':	/* turn on debugging */
			ldapdebug_level = atoi( optarg );
			break;
		case 'c':	/* alternate cmddir (index cmd location) */
			cmddir = strdup( optarg );
			break;

		case 'f':	/* specify a tailor file */
			tailorfile = strdup( optarg );
			break;

		case 'i':	/* input file */
			inputfile = strdup( optarg );
			break;

		case 'j':	/* number of parallel index procs */
			maxkids = atoi( optarg );
			break;

		case 'n':	/* which config file db to index */
			dbnum = atoi( optarg ) - 1;
			break;

		default:
			usage( argv[0] );
			break;
		}
	}
	if ( inputfile == NULL ) {
		usage( argv[0] );
	} else {
		if ( freopen( inputfile, "r", stdin ) == NULL ) {
			perror( inputfile );
			exit( 1 );
		}
	}

	/*
	 * initialize stuff and figure out which backend we're dealing with
	 */

	init();
	read_config( tailorfile, &be, NULL );

	/*
	 * Just in case there was no backend database definition in the 
	 * configuration file
	 */
	if ( backends == NULL )
	{
		logError( get_ldap_message( MSG_NOBACKENDDB, tailorfile ));
		exit( 1 );
	}

	if ( dbnum == DEFAULT_DBNUM ) {
		for ( dbnum = 0; dbnum < nbackends; dbnum++ ) {
			if ( strcasecmp( backends[dbnum].be_type, "ldbm" )
			    == 0 ) 
			{
				break;
			}
		}
		if ( dbnum == nbackends ) {
			logError(get_ldap_message(MSG_NOLDBM));
			exit( 1 );
		}
	} else if ( dbnum < 0 || dbnum > (nbackends-1) ) {
		logError(get_ldap_message(MSG_DBNUM1,nbackends));
		exit( 1 );
	} else if ( strcasecmp( backends[dbnum].be_type, "ldbm" ) != 0 ) {
		logError(get_ldap_message(MSG_DBNUM2));
		exit( 1 );
	}
	be = &backends[dbnum];

	/*
	 * generate the id2entry index
	 */

	i = 0;
	sprintf( cmd, "%s/%s", cmddir, ID2ENTRYCMD );
	args[i++] = cmd;
	args[i++] = "-i";
	args[i++] = inputfile;
	args[i++] = "-f";
	args[i++] = tailorfile;
	args[i++] = "-n";
	sprintf( buf2, "%d", dbnum+1 );
	args[i++] = buf2;
	sprintf( buf3, "%d", ldapdebug_level );
	args[i++] = "-d";
	args[i++] = buf3;
	args[i++] = NULL;
	fork_child( cmd, args );

	/*
	 * generate the dn2id and id2children indexes
	 */

	i = 0;
	sprintf( cmd, "%s/%s", cmddir, ID2CHILDRENCMD );
	args[i++] = cmd;
	args[i++] = "-i";
	args[i++] = inputfile;
	args[i++] = "-f";
	args[i++] = tailorfile;
	args[i++] = "-n";
	sprintf( buf2, "%d", dbnum+1 );
	args[i++] = buf2;
	sprintf( buf3, "%d", ldapdebug_level );
	args[i++] = "-d";
	args[i++] = buf3;
	args[i++] = NULL;
	fork_child( cmd, args );

	/*
	 * generate the attribute indexes
	 */

	i = 0;
	sprintf( cmd, "%s/%s", cmddir, INDEXCMD );
	args[i++] = cmd;
	args[i++] = "-i";
	args[i++] = inputfile;
	args[i++] = "-f";
	args[i++] = tailorfile;
	args[i++] = "-n";
	sprintf( buf2, "%d", dbnum+1 );
	args[i++] = buf2;
	sprintf( buf3, "%d", ldapdebug_level );
	args[i++] = "-d";
	args[i++] = buf3;
	args[i++] = NULL;		/* will hold the attribute name */
	args[i++] = NULL;

	id = 0;
	stop = 0;
	buf = NULL;
	lineno = 0;
	lcur = lmax = 0;
	vals[0] = &bv;
	vals[1] = NULL;
	while ( ! stop ) {
		char		*type, *val, *s;
		int		vlen, indexmask, syntaxmask;
		Datum		key, data;

		if ( fgets( line, sizeof(line), stdin ) != NULL ) {
			int     len;

			lineno++;
			len = strlen( line );

			/*
			 * Lines that begin with a '#' are ignored
			 */
			if (line[0] == '#')  { continue; }

			/*
			 * Lines that contain only white space
			 * are treated as if they are empty.
			 */
			if ( line[0] != '\n' && isspace(line[0])) {
				int cnt;

				for (cnt = 1; cnt < len; cnt++) {
					if (!(isspace(line[cnt])))
						break;
				}
				if (cnt == len) {
				    logError(get_ldap_message(MSG_VIS1,
					lineno));
					line[0] = '\n';
					line[1] = '\0';
					len = 1;
				}
			}

			while ( lcur + len + 1 > lmax ) {
				lmax += BUFSIZ;
				buf = (char *) ch_realloc( buf, lmax );
				if ( buf == NULL ) {
					return ( LDAP_NO_MEMORY );
				}
			}
			strcpy( buf + lcur, line );
			lcur += len;
		} else {
			stop = 1;
		}
		if ( line[0] == '\n' || stop && buf && *buf ) {
			id++;
			s = buf;
			elineno = 0;
			while ( (linep = str_getline( &s )) != NULL ) {
				elineno++;

				/*
				 * Parse ldif expression 
				 */
				if ( str_parse_line( linep, &type, &val, &vlen )
				    != 0 ) {

				    logError(get_ldap_message(MSG_BDLINE1,
					elineno, lineno));

					continue;
				}

				if ( !isascii( *type ) || isdigit( *type ) ) {
					continue;
				}

				type = strdup( type );

				/*
				 * Insert the dn into the AVL 
				 * tree. If a matching type is found call 
				 * avl_dup_error() which returns -1.
				 *
				 */

				if ( avl_insert( &avltypes, type, strcasecmp,
				    avl_dup_error ) != 0 ) {

					free( type );

				} else {

					attr_masks( be->be_private, type,
					    &indexmask, &syntaxmask );

					if ( indexmask ) {
						args[i - 2] = type;
						fork_child( cmd, args );
					}
				}
			}
			*buf = '\0';
			lcur = 0;
		}
	}
	(*be->be_close)( be );

	wait4kids( -1 );

	exit( 0 );
}

static void
fork_child( char *prog, char *args[] )
{
	int	status, pid;

	wait4kids( maxkids );

	switch ( pid = fork() ) {
	case 0:		/* child */
		execvp( prog, args );
		fprintf( stderr, "%s: ", prog );
		perror( "execv" );
		exit( -1 );
		break;

	case -1:	/* trouble */
		logError( get_ldap_message( MSG_COULDNOTFORK ));
		perror( "fork" );
		break;

	default:	/* parent */
		nkids++;
		break;
	}
}

static void
wait4kids( int nkidval )
{
	int		status;
	unsigned char	*p;

	while ( nkids >= nkidval ) {
		wait( &status );
		p = (unsigned char *) &status;
		if ( p[sizeof(int) - 1] == 0177 ) {

			logError(get_ldap_message(MSG_STOPSIG1,
			    p[sizeof(int) - 2] ));

		} else if ( p[sizeof(int) - 1] != 0 ) {

			logError(get_ldap_message(MSG_STOPSIG2,
			    p[sizeof(int) - 1] ));

			exit( p[sizeof(int) - 1] );
		} else if ( p[sizeof(int) - 2] != 0 ) {

			logError(get_ldap_message(MSG_STOPSIG3,
			    p[sizeof(int) - 2] ));

			exit( p[sizeof(int) - 2] );
		} else {
			nkids--;
		}
	}
}
