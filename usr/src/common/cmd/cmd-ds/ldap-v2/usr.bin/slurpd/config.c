/* @(#)config.c	1.5
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
 *
 * config.c - configuration file handling routines
 *
 * Revision history:
 *
 * 25 June 97     tonylo
 *	Error messages were fixed to display the right prefix. Fixes 
 *	made so that slurpd_read_config() checks whether a replogfile
 *	was specified at the command-line. Fix also allows a default
 *	replogfile to be used.
 *
 * 26 June 97     tonylo
 *	Added ability of slurpd to read config files with 'include'
 *	files in them. Also added check_dup_filename() which checks for 
 *	duplicate included files.
 *
 *	getline() was altered to remove a bug. For simplicity multiline
 *	entries are no longer handled.
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <lber.h>
#include <ldap.h>

#include "slurp.h"
#include "globals.h"
#include "ldaplog.h"

#define MSG_ADDREPMEM \
    1,144,"Out of memory, could not create replica data structure\n"
#define MSG_BADCFGLINE \
    1,145,"%s: line %d: Bad configuration file line (ignored)\n"
#define MSG_BADREPLOGFILE \
    1,146,"%s:line %d: Missing <filename> in \"replogfile <filename>\" expression\n"
#define MSG_BADREPLOGFILE1 \
    1,147,"%s:line %d: Junk at the end of \"replogfile %s\" (ignored)"
#define MSG_INCLUDE1 \
    1,148,"%s: line %d: Missing <filename> in \"include <filename>\"\n"
#define MSG_INCLUDE2 \
    1,149,"Cannot include file \"%s\" in \"%s\", this file has already been read\n"
#define MSG_ADDREPERR1 \
    1,150,"Failed to add replica\n"
#define MSG_ADDREPERR2 \
    1,151,"Fatal error occured adding replica in config file\n"
#define MSG_KERB1 \
    1,152,"A bind method of \"kerberos\" was specified in the slapd configuration\nfile, kerberos is not supported in this version of LDAP\n"
#define MSG_MALREP \
    1,153,"Malformed \"replica\" line in slapd config file, line %d\n"



#define MAXARGS	100

/* Forward declarations */
#ifdef NEEDPROTOS
static void	add_replica( char **, int );
static int	parse_replica_line( char **, int, Ri *);
static void	parse_line( char *, int *, char ** );
static char	*getline( FILE * );
static char	*strtok_quote( char *, char * );
#else /* NEEDPROTOS */
static void	add_replica();
static int	parse_replica_line();
static void	parse_line();
static char	*getline();
static char	*strtok_quote();
#endif /* NEEDPROTOS */

static int check_dup_filename(char* container, char* included);


/* current config file line # */
static int	lineno;


struct filelist {
	struct filelist* next;
	char* name;
};

static struct filelist *incfiles=NULL;

/*
 * Read the slapd config file, looking only for config options we're
 * interested in.  Since we haven't detached from the controlling
 * terminal yet, we just perror() and fprintf here.
 */
int
slurpd_read_config(
    char	*fname
)
{
    FILE	*fp;
    char	buf[BUFSIZ];
    char	*line, *p;
    int		cargc;
    char	*cargv[MAXARGS];
    char	*savefname;
    int		savelineno;

    logDebug( LDAP_CFG_FILE,
	"(slurpd_read_config) opening config file \"%s\"\n",
	fname, 0, 0 );

    if ( (fp = fopen( fname, "r" )) == NULL ) {
	perror( fname );
	exit( 1 );
    }

    lineno = 0;
    while ( (line = getline( fp )) != NULL ) {
	/* skip comments and blank lines */
	if ( line[0] == '#' || line[0] == '\0' ) {
	    continue;
	}

	parse_line( line, &cargc, cargv );

	if ( cargc < 1 ) {


	    logInfo(get_ldap_message(MSG_BADCFGLINE,fname,lineno));
	    continue;
	}

	/* replication log file to which changes are appended */
	if ( strcasecmp( cargv[0], "replogfile" ) == 0 ) {
	    /* 
	     * if slapd_replogfile has a value, the -r option was given,
	     * so use that value.  If slapd_replogfile has length == 0,
	     * then we should use the value in the config file we're reading.
	     */
	    if ( sglob->slapd_replogfile[ 0 ] == '\0' ) {
		if ( cargc < 2 ) {

		    logError(get_ldap_message(MSG_BADREPLOGFILE,
			fname, lineno));
		    exit( 1 );

		} else if ( cargc > 2 && *cargv[2] != '#' ) {

		    logInfo(get_ldap_message(MSG_BADREPLOGFILE1,
			fname,lineno, cargv[1] ));
		}
		sprintf( sglob->slapd_replogfile, cargv[1] );
	    }
	} else if ( strcasecmp( cargv[0], "include" ) == 0 ) {
		if ( cargc < 2 ) {

		    logError(get_ldap_message(
			MSG_INCLUDE1,fname,lineno));

		    exit( 1 );
		}
		savefname = strdup( cargv[1] );
		savelineno = lineno;
		if( check_dup_filename(fname, savefname) != 0 ) {

			logError(get_ldap_message(MSG_INCLUDE2,
			    savefname, fname));
			exit(1);
		}
		slurpd_read_config( savefname );
		free( savefname );
		lineno = savelineno - 1;
	} else if ( strcasecmp( cargv[0], "replica" ) == 0 ) {
	    add_replica( cargv, cargc );
	}
    }

    logDebug( LDAP_CFG_FILE, 
	"(slurpd_read_config) closing config file \"%s\"\n",fname,0,0);

    fclose( fp );

    logDebug( LDAP_CFG_FILE,
	"(slurpd_read_config) cfg file successfully read and parsed\n",
	0,0,0);

    return 0;
}

/*
 * check_dup_filename
 *
 * globals:
 *	incfiles
 *
 * returns:
 *	0	- ok
 *	1	- duplicate has occured
 *
 * description:
 *	checks for cases where included files in have already 
 *	been read.
 */
static
int check_dup_filename(char* container, char* included)
{
	struct filelist* ptr=incfiles;
	if(strcmp(container, included)==0) { return 1; }
	if( incfiles != NULL ) {
		while(ptr->next) {
			if(strcmp(ptr->name, included)==0) {
				return 1;
			}
			ptr=ptr->next;
		}
		if(strcmp(ptr->name, included)==0) { return 1; }
		ptr->next=(struct filelist*) malloc(sizeof(struct filelist));
		ptr->next->name=strdup(included);
		ptr->next->next=NULL;
	} else {
		incfiles=(struct filelist*) malloc(sizeof(struct filelist));
		incfiles->name=strdup(container);
		incfiles->next=(struct filelist*) malloc(sizeof(struct filelist));
		incfiles->next->next=NULL;
		incfiles->next->name=strdup(included);
	}
	return 0;
}



/*
 * Parse one line of input.
 */
static void
parse_line(
    char	*line,
    int		*argcp,
    char	**argv
)
{
    char *	token;

    *argcp = 0;
    for ( token = strtok_quote( line, " \t" ); token != NULL;
	token = strtok_quote( NULL, " \t" ) ) {
	argv[(*argcp)++] = token;
    }
    argv[*argcp] = NULL;
}




static char *
strtok_quote(
    char *line,
    char *sep
)
{
    int		inquote;
    char	*tmp;
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

char mybuf[BUFSIZ];

static char*
getline( FILE* fp )
{
	char* p;
	while( fgets( mybuf, sizeof(mybuf), fp ) != NULL ) {
		if ( (p = strchr( mybuf, '\n' )) != NULL ) { *p = '\0'; }
	        lineno++;
		if ( ! isspace( mybuf[0] ) ) {
			return( mybuf );
		}
	}
	return NULL;
}



/*
 * Add a node to the array of replicas.
 */
static void
add_replica(
    char	**cargv,
    int		cargc
)
{
    int	nr;

    nr = ++sglob->num_replicas;
    sglob->replicas = (Ri **) ch_realloc( sglob->replicas,
	    ( nr + 1 )  * sizeof( Re * ));

    if ( sglob->replicas == NULL ) {
	logError(get_ldap_message(MSG_ADDREPMEM));
	exit( 1 );
    }
    sglob->replicas[ nr ] = NULL; 

    if ( Ri_init( &(sglob->replicas[ nr - 1 ])) < 0 ) {
	logError(get_ldap_message(MSG_ADDREPMEM));
	exit( 1 );
    }
    if ( parse_replica_line( cargv, cargc,
	    sglob->replicas[ nr - 1] ) < 0 ) {
	/* Something bad happened - back out */

	    logInfo(get_ldap_message(MSG_ADDREPERR1));

	sglob->replicas[ nr - 1] = NULL;
	sglob->num_replicas--;
    } else {
	logDebug((LDAP_SLURPD | LDAP_CFG_FILE),
	    "(add_replica) ** successfully added replica \"%s:%d\"\n",
	    sglob->replicas[ nr - 1 ]->ri_hostname == NULL ? 
	    "(null)" : sglob->replicas[ nr - 1 ]->ri_hostname,
	    sglob->replicas[ nr - 1 ]->ri_port,
	    0 );

	sglob->replicas[ nr - 1]->ri_stel = 
	    sglob->st->st_add( sglob->st, sglob->replicas[ nr - 1 ] );

	if ( sglob->replicas[ nr - 1]->ri_stel == NULL ) {
	    logError(get_ldap_message(MSG_ADDREPERR2));
	    exit( 1 );
	}
    }
}



/* 
 * Parse a "replica" line from the config file.  replica lines should be
 * in the following format:
 * replica    host=<hostname:portnumber> binddn=<binddn>
 *            bindmethod="simple|kerberos" credentials=<creds>
 *
 * where:
 * <hostname:portnumber> describes the host name and port number where the
 * replica is running,
 *
 * <binddn> is the DN to bind to the replica slapd as,
 *
 * bindmethod is either "simple" or "kerberos", and
 *
 * <creds> are the credentials (e.g. password) for binddn.  <creds> are
 * only used for bindmethod=simple.  For bindmethod=kerberos, the
 * credentials= option should be omitted.  Credentials for kerberos
 * authentication are in the system srvtab file.
 *
 * The "replica" config file line may be split across multiple lines.  If
 * a line begins with whitespace, it is considered a continuation of the
 * previous line.
 */
#define	GOT_HOST	1
#define	GOT_DN		2
#define	GOT_METHOD	4
#define	GOT_ALL		( GOT_HOST | GOT_DN | GOT_METHOD )
static int
parse_replica_line( 
    char	**cargv,
    int		cargc,
    Ri		*ri
)
{
    int		gots = 0;
    int		i;
    char	*hp, *val;

    for ( i = 1; i < cargc; i++ ) {
	if ( !strncasecmp( cargv[ i ], HOSTSTR, strlen( HOSTSTR ))) {
	    val = cargv[ i ] + strlen( HOSTSTR ) + 1;
	    if (( hp = strchr( val, ':' )) != NULL ) {
		*hp = '\0';
		hp++;
		ri->ri_port = atoi( hp );
	    }
	    if ( ri->ri_port <= 0 ) {
		ri->ri_port = LDAP_PORT;
	    }
	    ri->ri_hostname = strdup( val );
	    gots |= GOT_HOST;
	} else if ( !strncasecmp( cargv[ i ],
		BINDDNSTR, strlen( BINDDNSTR ))) { 
	    val = cargv[ i ] + strlen( BINDDNSTR ) + 1;
	    ri->ri_bind_dn = strdup( val );
	    gots |= GOT_DN;
	} else if ( !strncasecmp( cargv[ i ], BINDMETHSTR,
		strlen( BINDMETHSTR ))) {
	    val = cargv[ i ] + strlen( BINDMETHSTR ) + 1;
	    if ( !strcasecmp( val, KERBEROSSTR )) {
#ifdef KERBEROS
		ri->ri_bind_method = AUTH_KERBEROS;
		if ( ri->ri_srvtab == NULL ) {
		    ri->ri_srvtab = strdup( sglob->default_srvtab );
		}
		gots |= GOT_METHOD;
#else /* KERBEROS */

	    logError(get_ldap_message(MSG_KERB1));
	    exit( 1 );

#endif /* KERBEROS */
	    } else if ( !strcasecmp( val, SIMPLESTR )) {
		ri->ri_bind_method = AUTH_SIMPLE;
		gots |= GOT_METHOD;
	    } else {
		ri->ri_bind_method = -1;
	    }
	} else if ( !strncasecmp( cargv[ i ], CREDSTR, strlen( CREDSTR ))) {
	    val = cargv[ i ] + strlen( CREDSTR ) + 1;
	    ri->ri_password = strdup( val );
	} else if ( !strncasecmp( cargv[ i ], BINDPSTR, strlen( BINDPSTR ))) {
	    val = cargv[ i ] + strlen( BINDPSTR ) + 1;
	    ri->ri_principal = strdup( val );
	} else if ( !strncasecmp( cargv[ i ], SRVTABSTR, strlen( SRVTABSTR ))) {
	    val = cargv[ i ] + strlen( SRVTABSTR ) + 1;
	    if ( ri->ri_srvtab != NULL ) {
		free( ri->ri_srvtab );
	    }
	    ri->ri_srvtab = strdup( val );
	}
    }
    if ( gots != GOT_ALL ) {
	logError(get_ldap_message(MSG_MALREP,lineno));
	return -1;
    }
    return 0;
}

