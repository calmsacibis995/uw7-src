#ident	"@(#)ldif2id2children.c	1.8"
#ident	"$Header$"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "ldapconfig.h"
#include "ldaplog.h"
#include "../slap.h"
#include "../back-ldbm/back-ldbm.h"
#include "ldaplog.h"



extern struct dbcache	*ldbm_cache_open();
extern void		attr_index_config();
extern char		*str_getline();
extern char		*dn_parent();
extern char		*dn_normalize_case();
extern int		strcasecmp();
extern int		nbackends;
extern Backend		*backends;

int		lineno;
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

static int	make_index();

static char	*tailorfile;
static char	*inputfile;
 
/* Messages */
#define MSG_DBNUM1 \
     1,108,"Database number selected via -n is out of range\n\
Must be in the range 1 to %d (number of databases in the config file)\n"
#define MSG_DBNUM2 \
     1,109,"Database number selected via -n is not an ldbm database\n"
#define MSG_NOVISSYM \
     1,110,"Line %d has no visible symbols, treat as empty line\n"
#define MSG_NODNSTR \
     1,111,"Entry %d ending at line %d has no dn\n"
#define MSG_USAGE \
     1,112,"Usage: %s -i ldiffile [options]\n\
    -i   ldiffile        ldif format file specifying the directory data\n\
options:\n\
    -f   file            configuration file, default is slapd.conf\n\
    -n   dbnum           which database in the config file\n\
    -d   debuglevel      control level of debug output, default 0\n"
#define MSG_NOLDBM1 \
    1,113,"No ldbm database found in configuration file\n"
#define MSG_BADLINE1 \
    1,114,"Bad line %d in entry ending at line %d ignored\n"
#define MSG_BADLINE2 \
    1,115,"Bad line %d in entry ending at line %d ignored\n"
#define MSG_NOPARENT1 \
    1,116,"No parent \"%s\" of \"%s\" has been specified\n"


static void
usage( char *name )
{
	logError(get_ldap_message(MSG_USAGE,name));
	exit( 1 );
}

main( int argc, char **argv )
{
	int		i, cargc, indb, stop, status;
	char		*cargv[LDIF2X_MAXARGS];
	char		*defargv[LDIF2X_MAXARGS];
	char		*linep, *buf;
	char		line[BUFSIZ];
	int		lineno, elineno;
	int      	lmax, lcur;
	int		dbnum;
	ID		id;
	struct dbcache	*db, *db2;
	Backend		*be;
	struct berval	bv;
	struct berval	*vals[2];
	Avlnode		*avltypes = NULL;
	extern char	*optarg;


	/* i18n stuff */
	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:ldif2id2children");
        open_message_catalog("ldap.cat");



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

	if ( dbnum == -1 ) {
		for ( dbnum = 0; dbnum < nbackends; dbnum++ ) {
			if ( strcasecmp( backends[dbnum].be_type, "ldbm" )
			    == 0 ) {
				break;
			}
		}
		if ( dbnum == nbackends ) {
			logError(get_ldap_message(MSG_NOLDBM1));
			exit( 1 );
		}
	} else if ( dbnum < 0 || dbnum > (nbackends-1) ) {
		logError(get_ldap_message(MSG_DBNUM1, nbackends));
		exit( 1 );
	} else if ( strcasecmp( backends[dbnum].be_type, "ldbm" ) != 0 ) {
		logError(get_ldap_message(MSG_DBNUM2));
		exit( 1 );
	}
	be = &backends[dbnum];

	/*
	 * first, make the dn2id index
	 */

	if ( (db = ldbm_cache_open( be, "dn2id", LDBM_SUFFIX, LDBM_NEWDB ))
	    == NULL ) {
		perror( "dn2id file" );
		exit( 1 );
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
		Datum		key, data;

		if ( fgets( line, sizeof(line), stdin ) != NULL ) {
			int     len;

			lineno++;
			len = strlen( line );
			/*
			 * Lines that begin with a '#' are ignored
			 */
			if (line[0] == '#')  {
				continue;
			}

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
					     MSG_NOVISSYM,lineno));

					line[0] = '\n';
					line[1] = '\0';
					len = 1;
				}
			}

			while ( lcur + len + 1 > lmax ) {
				lmax += BUFSIZ;
				buf = (char *) ch_realloc( buf, lmax );
				if ( buf == NULL ) {
					exit ( LDAP_NO_MEMORY );
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

					     logError(get_ldap_message(
						MSG_BADLINE1,
						elineno, lineno));

						continue;
					}

					if ( strcmp( type, "dn" ) == 0 )
						break;
				}

				if ( linep == NULL ) {
					logError(get_ldap_message(
					    MSG_NODNSTR, id, lineno));
				} else {
					key.dptr = dn_normalize_case( val );
					key.dsize = strlen( val ) + 1;
					data.dptr = (char *) &id;
					data.dsize = sizeof(ID);
					if ( ldbm_store( db->dbc_db, key, data,
					    LDBM_REPLACE ) != 0 ) {
						perror( "dn2id ldbm_store" );
						exit( 1 );
					}
				}
			}
			*buf = '\0';
			lcur = 0;
			line[0] = '\0';
		}
	}

	/*
	 * next, make the id2children index
	 */

	if ( (db2 = ldbm_cache_open( be, "id2children", LDBM_SUFFIX,
	    LDBM_NEWDB )) == NULL ) {
		perror( "id2children file" );
		exit( 1 );
	}

	rewind( stdin );
	id = 0;
	stop = 0;
	buf = NULL;
	lineno = 0;
	lcur = lmax = 0;
	vals[0] = &bv;
	vals[1] = NULL;
	while ( ! stop ) {
		char	*type, *val, *s, *dn;
		int	vlen;
		ID	pid;
		char	buf2[20];
		Datum	key, data;

		if ( fgets( line, sizeof(line), stdin ) != NULL ) {
			int     len;

			lineno++;
			len = strlen( line );
			/*
			 * Lines that begin with a '#' are ignored
			 */
			if (line[0] == '#')  {
				continue;
			}

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
					    MSG_NOVISSYM,lineno));
					line[0] = '\n';
					line[1] = '\0';
					len = 1;
				}
			}

			while ( lcur + len + 1 > lmax ) {
				lmax += BUFSIZ;
				buf = (char *) ch_realloc( buf, lmax );
				if ( buf == NULL ) {
					exit ( LDAP_NO_MEMORY );
				}
			}
			strcpy( buf + lcur, line );
			lcur += len;
		} else {
			stop = 1;
		}
		if ( line[0] == '\n' || stop && buf && *buf ) {
			if ( * buf != '\n' ) {
				id++;
				s = buf;
				elineno = 0;
				while ( (linep = str_getline( &s )) != NULL ) {
					elineno++;
					if ( str_parse_line( linep, &type, &val,
					    &vlen ) != 0 ) {

					    logError(get_ldap_message(
						MSG_BADLINE2,
						elineno,
						lineno));
						continue;
					}

					if ( strcmp( type, "dn" ) == 0 )
						break;
				}

				if ( linep == NULL ) {
					logError(get_ldap_message(
					    MSG_NODNSTR,id,lineno));
				} else {
					if ( (dn = dn_parent( be, val ))
					    == NULL ) {
						pid = 0;
					} else {
						key.dptr =
						    dn_normalize_case( dn );
						key.dsize = strlen( dn ) + 1;

						data = ldbm_fetch( db->dbc_db,
						    key );
						if ( data.dptr == NULL ) {
							dn_normalize( val );
							if ( ! be_issuffix( be,
							    val ) ) {

		logError(get_ldap_message(MSG_NOPARENT1, dn, val));

							}
							*buf = '\0';
							lcur = 0;
							line[0] = '\0';
							continue;
						}
						(void) memcpy( (char *) &pid,
						    data.dptr, sizeof(ID) );
					}

					sprintf( buf2, "%c%d", EQ_PREFIX, pid );
					key.dptr = buf2;
					key.dsize = strlen( buf2 ) + 1;
					if ( idl_insert_key( be, db2, key, id )
					    != 0 ) {
						perror( "idl_insert_key" );
						exit( 1 );
					}
				}
			}
			*buf = '\0';
			lcur = 0;
			line[0] = '\0';
		}
	}
	(*be->be_close)( be );

	exit( 0 );
}
