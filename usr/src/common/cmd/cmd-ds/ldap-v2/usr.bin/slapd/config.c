/* "@(#)config.c	1.7"
 *
 * configuration file handling routines
 *
 * Revision history:
 *
 * 13 Jun 97	geoffh
 *	Changes made to handle reverse lookups. A condition has been added
 *	so that if a slapd ACL checks the domain of a client then the address
 *	of the client will automatically be looked up., rather than get the 
 *	address of the client every time - or not at all.
 *
 *	This means that a new config option has been added: 
 *
 *	getcallername [on | off]
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "ldapconfig.h"
#include "ldaplog.h"

#define MAXARGS	100

#define MSG_FILE_ERRNO 10

/* Messages */
#define MSG_OPEN \
    1,24,"Can not open configuration file \"%s\" - absolute path?\n"
#define MSG_BADCFGLINE \
    1,25,"%s: line %d: Bad config line (ignored)\n"
#define MSG_BADDBLINE \
    1,26,"%s: line %d: Missing <type> in \"database <type>\"\n"
#define MSG_BADSZLIMIT \
    1,27,"%s: line %d: Missing <limit> in \"sizelimit <limit>\"\n"
#define MSG_BADTMLIMIT \
    1,28,"%s: line %d: Missing <limit> in \"timelimit <limit>\"\n"
#define MSG_BADSUFFIX \
    1,29,"%s: line %d: Missing <DN> in \"suffix <DN>\"\n"
#define MSG_ECRUFT \
    1,30,"%s: line %d: Extra text after \"suffix %s\" definition (ignored)\n"
#define MSG_BESUFFIX \
    1,31,"%s: line %d: \"suffix\" must appear after \"database <type>\" definition\n"
#define MSG_BADROOTDN \
    1,32,"%s: line %d: Missing <DN> in \"rootdn <DN>\"\n"
#define MSG_BADROOTDN2 \
    1,33,"%s: line %d: \"rootdn\" must appear after \"database <type>\" definition (ignored)\n"
#define MSG_NOROOTPW \
    1,34, "%s: line %d: Missing <password> in \"rootpw <password>\"\n"
#define MSG_ROOTPWNO \
    1,35,"%s: line %d: \"rootpw\" must appear after \"database <type>\" definition (ignored)\n"
#define MSG_READONLYERR \
    1,36,"%s: line %d: Missing <on|off> in \"readonly <on|off>\"\n"
#define MSG_INVREADONLY \
    1,37,"%s: line %d: \"readonly\" must appear after \"database <type>\" definition (ignored)\n"
#define MSG_ERRREFERRAL \
    1,38,"%s: line %d: Missing <URL> in \"referral <URL>\"\n"
#define MSG_ERRSCHEMACHECK \
    1,39,"%s: line %d: Missing <on|off> in \"schemacheck <on|off>\"\n"
#define MSG_ERRGETCALLERNAME \
    1,40,"%s: line %d: Missing <on|off> in \"getcallername <on|off>\"\n"
#define MSG_SEARCHALG \
    1,41,"%s: line %d: Missing <soundex|metaphone> in \"phonetic <soundex|metaphone>\"\n"
#define MSG_MONITORDN \
    1,42,"%s: line %d: Missing <DN> in \"monitor_dn <DN>\"\n"
#define MSG_CONFIGDN \
    1,43,"%s: line %d: Missing <DN> in \"config_dn <DN>\"\n"
#define MSG_DEFACCESS \
    1,44,"%s: line %d: Missing <access-level> in \"defaultaccess <access-level>\"\n"
#define MSG_DEFACCESS2 \
   1,45,"%s: line %d: Unknown access level \"%s\" expecting [self]{none|compare|read|write}\n"
#define MSG_LOGLEVEL \
    1,46, ":141: %s: line %d: Missing <level> in \"loglevel <level>\"\n"
#define MSG_REPLICAERR1 \
    1,47,"%s: line %d: Missing <host> in \"replica <host[:port]>\"\n"
#define MSG_REPLICAERR2 \
    1,48,"%s: line %d: \"replica\" must appear after \"database <type>\" definition (ignored)\n"
#define MSG_REPLICAERR3 \
    1,49,"%s: line %d: Missing <host> in \"replica <host[:port]>\" (ignored)\n"
#define MSG_UPDATEDNERR \
   1,50,"%s: line %d: Missing <DN> in \"updatedn <DN>\"\n"
#define MSG_UPDATEDNERR2 \
    1,51,"%s: line %d: \"updatedn\" must appear after \"database <type>\" definition (ignored)\n"
#define MSG_REPLOGFILE \
     1,52,"%s: line %d: Missing <filename> in \"replogfile <filename>\"\n"
#define MSG_LASTMOD \
    1,53,"%s: line %d: Missing <on|off> in \"lastmod <on|off>\"\n"
#define MSG_INCLUDE \
    1,54,"%s: line %d: Missing <filename> in \"include <filename>\"\n"
#define MSG_UNKNOWNDIR \
    1,55,"%s: line %d: Unknown directive \"%s\" outside database definition (ignored)\n"
#define MSG_UNKNOWNDIR2 \
    1,56,"%s: line %d: Unknown directive \"%s\" inside database definition (ignored)\n"
#define MSG_TOOMANY \
    1,57, "Too many tokens in configuration file \"%s\" (max %d)\n"

extern Backend	*new_backend();
extern char	*default_referral;
extern int	ldap_syslog;
extern int	global_schemacheck;
extern phonetic_alg_t global_phonetic_style;
char	*slapd_config_dn = NULL;
char	*slapd_monitor_dn = NULL;
int     getcallername = 0;

/*
 * defaults for various global variables
 */
int		defsize = SLAPD_DEFAULT_SIZELIMIT;
int		deftime = SLAPD_DEFAULT_TIMELIMIT;
struct acl	*global_acl = NULL;
int		global_default_access = ACL_READ;
char		*replogfile;
int		global_lastmod;
char		*ldap_srvtab = "";

static char	*fp_getline();
static void	fp_getline_init();
static void	fp_parse_line();

static char	*strtok_quote();

static char	g_fname[BUFSIZ];

void
read_config( char *fname, Backend **bep, FILE *pfp )
{
	FILE	*fp;
	char	*line, *savefname, *dn;
	int	cargc, savelineno;
	char	*cargv[MAXARGS];
	int	lineno, i;
	Backend	*be;

	if ( (fp = pfp) == NULL && (fp = fopen( fname, "r" )) == NULL ) {
		logError(get_ldap_message(MSG_OPEN,fname));
		perror( fname );
		exit( MSG_FILE_ERRNO );
	}

	/* copy to global */
	strcpy(g_fname, fname);

	logDebug(LDAP_CFG_FILE, "(read_config) reading config file %s\n",fname,0,0 );

	be = *bep;
	fp_getline_init( &lineno );
	while ( (line = fp_getline( fp, &lineno )) != NULL ) {
		/* skip comments and blank lines */
		if ( line[0] == '#' || line[0] == '\0' ) continue;

		fp_parse_line( line, &cargc, cargv );

		if ( cargc < 1 ) {
			logInfo(
			    get_ldap_message(MSG_BADCFGLINE,fname,lineno));
			continue;
		}

		/* start of a new database definition */
		if ( strcasecmp( cargv[0], "database" ) == 0 ) {
			if ( cargc < 2 ) {
				logError(get_ldap_message(
				    MSG_BADDBLINE,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}
			*bep = new_backend( cargv[1] );
			be = *bep;

		/* set size limit */
		} else if ( strcasecmp( cargv[0], "sizelimit" ) == 0 ) {
			if ( cargc < 2 ) {
				logError(get_ldap_message(
				    MSG_BADSZLIMIT,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}
			if ( be == NULL ) {
				defsize = atoi( cargv[1] );
			} else {
				be->be_sizelimit = atoi( cargv[1] );
			}

		/* set time limit */
		} else if ( strcasecmp( cargv[0], "timelimit" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_BADTMLIMIT,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}
			if ( be == NULL ) {
				deftime = atoi( cargv[1] );
			} else {
				be->be_timelimit = atoi( cargv[1] );
			}

		/* set database suffix */
		} else if ( strcasecmp( cargv[0], "suffix" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_BADSUFFIX,fname,lineno));
				exit( MSG_FILE_ERRNO );

			} else if ( cargc > 2 ) {

				logInfo(get_ldap_message(
				    MSG_ECRUFT,fname,lineno,cargv[1]));
			}
			if ( be != NULL ) {
				dn = strdup( cargv[1] );
				(void) dn_normalize( dn );
				charray_add( &be->be_suffix, dn );
			} else {
				logInfo(get_ldap_message(
				    MSG_BESUFFIX,fname,lineno));
			}

		/* set magic "root" dn for this database */
		} else if ( strcasecmp( cargv[0], "rootdn" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_BADROOTDN,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}
			if ( be == NULL ) {

				logInfo(get_ldap_message(
				    MSG_BADROOTDN2,fname,lineno));
			} else {
				dn = strdup( cargv[1] );
				(void) dn_normalize( dn );
				be->be_rootdn = dn;
			}

		/* set super-secret magic database password */
		} else if ( strcasecmp( cargv[0], "rootpw" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_NOROOTPW,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}
			if ( be == NULL ) {
				logInfo(get_ldap_message(
				    MSG_ROOTPWNO,fname,lineno));

			} else {
				be->be_rootpw = strdup( cargv[1] );
			}

		/* make this database read-only */
		} else if ( strcasecmp( cargv[0], "readonly" ) == 0 ) {
			if ( cargc < 2 ) {


				logError(get_ldap_message(
				    MSG_READONLYERR,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}
			if ( be == NULL ) {

				logInfo(get_ldap_message(
				    MSG_INVREADONLY,fname,lineno));

			} else {
				if ( strcasecmp( cargv[1], "on" ) == 0 ) {
					be->be_readonly = 1;
				} else {
					be->be_readonly = 0;
				}
			}

		/* where to send clients when we don't hold it */
		} else if ( strcasecmp( cargv[0], "referral" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_ERRREFERRAL,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}
			default_referral = (char *) malloc( strlen( cargv[1] )
			    + sizeof("Referral:\n") + 1 );
			strcpy( default_referral, "Referral:\n" );
			strcat( default_referral, cargv[1] );

		/* specify an objectclass */
		} else if ( strcasecmp( cargv[0], "objectclass" ) == 0 ) {
			parse_oc( be, fname, lineno, cargc, cargv );

		/* specify an attribute */
		} else if ( strcasecmp( cargv[0], "attribute" ) == 0 ) {
			attr_syntax_config( fname, lineno, cargc - 1,
			    &cargv[1] );

		/* turn on/off schema checking */
		} else if ( strcasecmp( cargv[0], "schemacheck" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_ERRSCHEMACHECK,fname,lineno));
				exit( MSG_FILE_ERRNO );

			}
			if ( strcasecmp( cargv[1], "on" ) == 0 ) {
				global_schemacheck = 1;
			} else {
				global_schemacheck = 0;
			}
		/* turn on/off getting of caller name */
		} else if ( strcasecmp( cargv[0], "getcallername" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_ERRGETCALLERNAME,fname,lineno));
				exit( MSG_FILE_ERRNO );

			}
			if ( strcasecmp( cargv[1], "on" ) == 0 ) {
				getcallername = 1;
			} else {
				getcallername = 0;
			}
		/* specify phonetic algorithm to use */
		} else if ( strcasecmp( cargv[0], "phonetic" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_SEARCHALG,fname,lineno));
				exit( MSG_FILE_ERRNO );

			}
			if ( strcasecmp( cargv[1], "metaphone" ) == 0 ) {
				global_phonetic_style = METAPHONE;
			} else if ( strcasecmp( cargv[1], "soundex" ) == 0 ) {
				global_phonetic_style = SOUNDEX;
			} else {
				logError(get_ldap_message(
				    MSG_SEARCHALG,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}

		/* specify slapd_monitor_dn for monitoring queries */
		} else if ( strcasecmp( cargv[0], "monitor_dn" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_MONITORDN,fname,lineno));
				exit( MSG_FILE_ERRNO );

			}
			slapd_monitor_dn = strdup( cargv[1] );
			(void) dn_normalize( slapd_monitor_dn);

		/* specify slapd_config_dn for config  queries */
		} else if ( strcasecmp( cargv[0], "config_dn" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_CONFIGDN,fname,lineno));
				exit( MSG_FILE_ERRNO );

			}
			slapd_config_dn = strdup( cargv[1] );
			(void) dn_normalize( slapd_config_dn);

		/* specify access control info */
		} else if ( strcasecmp( cargv[0], "access" ) == 0 ) {
			parse_acl( be, fname, lineno, cargc, cargv );

		/* specify default access control info */
		} else if ( strcasecmp( cargv[0], "defaultaccess" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_DEFACCESS,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}
			if (( be == NULL ) && 
			    ((global_default_access=str2access(cargv[1]))==-1))
			{
				logError(get_ldap_message(
				    MSG_DEFACCESS2,fname,lineno));
				exit( MSG_FILE_ERRNO );
			} 
			else if((be->be_dfltaccess=str2access(cargv[1]))==-1) 
			{
				logError(get_ldap_message(
				    MSG_DEFACCESS2,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}

		/* debug level to log things to syslog */
		} else if ( strcasecmp( cargv[0], "loglevel" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_LOGLEVEL,fname,lineno));
				exit( MSG_FILE_ERRNO );

			}
			ldapsyslog_level = atoi( cargv[1] );

		/* list of replicas of the data in this backend (master only) */
		} else if ( strcasecmp( cargv[0], "replica" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_REPLICAERR1,fname,lineno));
				exit( MSG_FILE_ERRNO );

			}
			if ( be == NULL ) {

				logInfo(get_ldap_message(
					MSG_REPLICAERR2,fname,lineno));
				
			} else {
				for ( i = 1; i < cargc; i++ ) {
					if ( strncasecmp( cargv[i], "host=", 5 )
					    == 0 ) {
						charray_add( &be->be_replica,
						    strdup( cargv[i] + 5 ) );
						break;
					}
				}
				if ( i == cargc ) {
					logInfo(get_ldap_message(
						MSG_REPLICAERR3,fname,lineno));
				}
			}

		/* dn of master entity allowed to write to replica */
		} else if ( strcasecmp( cargv[0], "updatedn" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_UPDATEDNERR,fname,lineno));
				exit( MSG_FILE_ERRNO );
				
			}
			if ( be == NULL ) {

				logError(get_ldap_message(
				    MSG_UPDATEDNERR2,fname,lineno));
				exit( MSG_FILE_ERRNO );

			} else {
				be->be_updatedn = strdup( cargv[1] );
				(void) dn_normalize( be->be_updatedn );
			}

		/* replication log file to which changes are appended */
		} else if ( strcasecmp( cargv[0], "replogfile" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_REPLOGFILE,fname,lineno));
				exit( MSG_FILE_ERRNO );

			}
			if ( be ) {
				be->be_replogfile = strdup( cargv[1] );
			} else {
				replogfile = strdup( cargv[1] );
			}

		/* maintain lastmodified{by,time} attributes */
		} else if ( strcasecmp( cargv[0], "lastmod" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_LASTMOD,fname,lineno));
				exit( MSG_FILE_ERRNO );
			}
			if ( strcasecmp( cargv[1], "on" ) == 0 ) {
				if ( be )
					be->be_lastmod = ON;
				else
					global_lastmod = ON;
			} else {
				if ( be )
					be->be_lastmod = OFF;
				else
					global_lastmod = OFF;
			}

		/* include another config file */
		} else if ( strcasecmp( cargv[0], "include" ) == 0 ) {
			if ( cargc < 2 ) {

				logError(get_ldap_message(
				    MSG_INCLUDE,fname,lineno));
				exit( MSG_FILE_ERRNO );

			}
			savefname = strdup( cargv[1] );
			savelineno = lineno;
			read_config( savefname, bep, NULL );
			be = *bep;
			free( savefname );
			lineno = savelineno - 1;

		} else {
			if ( be == NULL ) {
				logInfo(get_ldap_message(
					MSG_UNKNOWNDIR,fname,lineno));

			} else if ( be->be_config == NULL ) {

				logInfo(get_ldap_message(
					MSG_UNKNOWNDIR,fname,lineno));

			} else {
				(*be->be_config)( be, fname, lineno, cargc,
				    cargv );
			}
		}
	}

	/* If no suffix entry was specified then add an empty suffix entry.
	 * Otherwise there is no base dn to search from
	 */

	if (( be != NULL ) && ( be->be_suffix == NULL )) {
		dn = strdup( "" );
		(void) dn_normalize( dn );
		charray_add( &be->be_suffix, dn );
	}
	fclose( fp );
}

static void
fp_parse_line(
    char	*line,
    int		*argcp,
    char	**argv
)
{
	char *	token;

	*argcp = 0;
	for ( token = strtok_quote( line, " \t" ); token != NULL;
	    token = strtok_quote( NULL, " \t" ) ) {
		if ( *argcp == MAXARGS ) {

			logError(get_ldap_message(
			    MSG_TOOMANY,g_fname,MAXARGS));
			exit( MSG_FILE_ERRNO );
		}
		argv[(*argcp)++] = token;
	}
	argv[*argcp] = NULL;
}

static char *
strtok_quote( char *line, char *sep )
{
	int		inquote;
	char		*tmp;
	static char	*next;

	if ( line != NULL ) {
		next = line;
	}
	while ( *next && strchr( sep, *next ) ) {
		next++;
	}

	if ( *next == '\0' ) {
		next = NULL;
		return( NULL );
	}
	tmp = next;

	for ( inquote = 0; *next; ) {
		switch ( *next ) {
		case '"':
			if ( inquote ) {
				inquote = 0;
			} else {
				inquote = 1;
			}
			strcpy( next, next + 1 );
			break;

		case '\\':
			strcpy( next, next + 1 );
			break;

		default:
			if ( ! inquote ) {
				if ( strchr( sep, *next ) != NULL ) {
					*next++ = '\0';
					return( tmp );
				}
			}
			next++;
			break;
		}
	}

	return( tmp );
}

static char	buf[BUFSIZ];
static char	*line;
static int	lmax, lcur;

#define CATLINE( buf )	{ \
	int	len; \
	len = strlen( buf ); \
	while ( lcur + len + 1 > lmax ) { \
		lmax += BUFSIZ; \
		line = (char *) ch_realloc( line, lmax ); \
		if ( line == NULL ) { return NULL; } \
	} \
	strcpy( line + lcur, buf ); \
	lcur += len; \
}

static char *
fp_getline( FILE *fp, int *lineno )
{
	char		*p;

	lcur = 0;
	CATLINE( buf );
	(*lineno)++;

	/* hack attack - keeps us from having to keep a stack of bufs... */
	if ( strncasecmp( line, "include", 7 ) == 0 ) {
		buf[0] = '\0';
		return( line );
	}

	while ( fgets( buf, sizeof(buf), fp ) != NULL ) {
		if ( (p = strchr( buf, '\n' )) != NULL ) {
			*p = '\0';
		}
		if ( ! isspace( buf[0] ) ) {
			return( line );
		}

		CATLINE( buf );
		(*lineno)++;
	}
	buf[0] = '\0';

	return( line[0] ? line : NULL );
}

static void
fp_getline_init( int *lineno )
{
	*lineno = -1;
	buf[0] = '\0';
}
