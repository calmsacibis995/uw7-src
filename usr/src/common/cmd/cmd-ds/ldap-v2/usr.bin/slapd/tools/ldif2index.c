#ident	"@(#)ldif2index.c	1.8"
#ident	"$Header$"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "ldaplog.h"
#include "ldapconfig.h"
#include "../slap.h"
#include "ldaplog.h"


extern void	attr_index_config();
extern char	*str_getline();
extern char	*attr_normalize();
extern int	nbackends;
extern Backend	*backends;

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

/* Messages */
#define MSG_USAGE \
    1,124,"usage: %s -i ldiffile [options]\n\
    -i   ldiffile        ldif format file specifying the directory data\n\
  options:\n\
    -f   conffile        configuration file, default is slapd.conf\n\
    -n   database number which database in the config file\n\
    -d   debuglevel      control level of debug output, default none\n"

#define MSG_DBNUMERR1 \
    1,125,"Database number selected via -n is out of range\n\
Must be in the range 1 to %d (number of databases in the config file)\n"

#define MSG_DBNUMERR2 \
    1,126,"Database number selected via -n is not an ldbm database\n"
#define MSG_NODBCONF \
    1,127,"No ldbm database found in configuration file\n"
#define MSG_EMPTLINE \
    1,128,"Line %d has no visible symbols, treating as empty line\n"
#define MSG_BDLINE1 \
    1,129,"Bad line %d in entry ending at line %d ignored\n"




static void
usage( char *name )
{
	logError(get_ldap_message(MSG_USAGE,name));
	exit( 1 );
}

main( int argc, char **argv )
{
	int		i, cargc, indb, stop;
	char		*cargv[LDIF2X_MAXARGS];
	char		*defargv[LDIF2X_MAXARGS];
	char		*tailorfile, *inputfile;
	char		*linep, *buf, *attr;
	char		line[BUFSIZ];
	int		lineno, elineno;
	int      	lmax, lcur, indexmask, syntaxmask;
	int		dbnum;
	unsigned long	id;
	Backend		*be=NULL;
	struct berval	bv;
	struct berval	*vals[2];
	extern char	*optarg;


	/* i18n stuff */
	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:ldif2index");
        open_message_catalog("ldap.cat");

	inputfile = NULL;
	tailorfile = DEFAULT_CONFIGFILE;
	dbnum = -1;
	while ( (i = getopt( argc, argv, "d:f:i:n:" )) != EOF ) {
		switch ( i ) {
		case 'd':	/* turn on debugging */
			ldapdebug_level = atoi( optarg );
			break;

		case 'f':	/* specify a tailor file */
			tailorfile = strdup( optarg );
			break;

		case 'i':	/* input file */
			inputfile = strdup( optarg );
			break;

		case 'n':	/* which config file db to index */
			dbnum = atoi( optarg ) - 1;
			break;

		default:
			usage( argv[0] );
			break;
		}
	}
	attr = attr_normalize( argv[argc - 1] );
	if ( inputfile == NULL ) {
		usage( argv[0] );
	} else {
		if ( freopen( inputfile, "r", stdin ) == NULL ) {
			perror( inputfile );
			exit( 1 );
		}
	}

	init();
	read_config( tailorfile, &be, NULL );

	if ( dbnum == -1 ) {
		for ( dbnum = 0; dbnum < nbackends; dbnum++ ) {
			if ( strcasecmp( backends[dbnum].be_type, "ldbm" )
			    == 0 ) {
				break;
			}
		}
		if ( dbnum == nbackends ) {
			logError(get_ldap_message(MSG_NODBCONF));
			exit( 1 );
		}
	} else if ( dbnum < 0 || dbnum > (nbackends-1) ) {
		logError(get_ldap_message(MSG_DBNUMERR1,nbackends));
		exit( 1 );
	} else if ( strcasecmp( backends[dbnum].be_type, "ldbm" ) != 0 ) {
		logError(get_ldap_message(MSG_DBNUMERR2));
		exit( 1 );
	}
	be = &backends[dbnum];

	attr_masks( be->be_private, attr, &indexmask, &syntaxmask );
	if ( indexmask == 0 ) {
		exit( 0 );
	}

	id = 0;
	stop = 0;
	lineno = 0;
	buf = NULL;
	lcur = lmax = 0;
	vals[0] = &bv;
	vals[1] = NULL;
	while ( ! stop ) {
		char		*type, *val, *s;
		int		vlen;

		if ( fgets( line, sizeof(line), stdin ) != NULL ) {
			int     len;

			lineno++;
			len = strlen( line );

			/*
			 * Lines that begin with a '#' are ignored
			 */
			if (line[0] == '#') 
				continue;

			/*
			 * Lines that contain only white space
			 * are treated as if they are empty.
			 */
			if (line[0] != '\n' && isspace(line[0])) {
				int cnt;

				for (cnt = 1; cnt < len; cnt++) {
					if (!(isspace(line[cnt])))
						break;
				}
				if (cnt == len) {
				    logError(get_ldap_message(
					MSG_EMPTLINE,lineno));

					line[0] = '\n';
					line[1] = '\0';
					len = 1;
				}
			}

			while ( lcur + len + 1 > lmax ) {
				lmax += BUFSIZ;
				buf = (char *) ch_realloc( buf, lmax );
				if ( buf == NULL ) {
					exit( LDAP_NO_MEMORY );
				}
			}
			strcpy( buf + lcur, line );
			lcur += len;
		} else {
			stop = 1;
		}
		if ( line[0] == '\n' || stop && buf && *buf ) {
			if ( *buf != '\n' ) {
				id++;
				s = buf;
				elineno = 0;
				while ( (linep = str_getline( &s )) != NULL ) {
					elineno++;

					if ( str_parse_line( linep, &type, &val,
					    &vlen ) != 0 ) {

				logError(get_ldap_message(MSG_BDLINE1,
				    elineno, lineno));

						continue;
					}

					if ( strcasecmp( type, attr ) == 0 ) {
						bv.bv_val = val;
						bv.bv_len = vlen;
						index_add_values( be, attr,
						    vals, id );
					}
				}
			}
			*buf = '\0';
			lcur = 0;
		}
	}
	(*be->be_close)( be );

	exit( 0 );
}
