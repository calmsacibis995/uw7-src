/*
 * #ident @(#)getreplicainfo.c	1.3
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

#include <stdio.h>
#include <string.h>
#include <lber.h>
#include <ldap.h>

#define MAXARGS	100

#define ERR_COULD_NOT_OPEN_FILE 1
#define ERR_INVALID_ARGS 2
#define ERR_BADLY_FORMED_RECORD 3
#define ERR_MALLOC_FAILED 4
#define ERR_REALLOC_FAILED 5

/* Forward declarations */
#ifdef NEEDPROTOS
static int	parse_replica_line( char **, int);
static void	parse_line( char *, int *, char ** );
static char	*getline( FILE * );
static char	*strtok_quote( char *, char * );
#else /* NEEDPROTOS */
static int	parse_replica_line();
static void	parse_line();
static char	*getline();
static char	*strtok_quote();
#endif /* NEEDPROTOS */

/* current config file line # */
static int	lineno;

int
main( int argc, char** argv) {
	if( argc == 2 ) {
		read_config( argv[1] );
	} else {
		exit(ERR_INVALID_ARGS);
	}
	return 0;
}

char *
ch_malloc( unsigned long size )
{
        char    *new;
        if ( (new = (char *) malloc( size )) == NULL ) {
                fprintf( stderr, "Memory allocation (malloc) of %d bytes failed\n", size );
                exit( ERR_MALLOC_FAILED );
        }
        return( new );
}

char *
ch_realloc( char *block, unsigned long size)
{
        char    *new;
        if ( block == NULL ) { return( ch_malloc( size ) ); }
        if ( (new = (char *) realloc( block, size )) == NULL ) {
		fprintf(stderr,
		    "Memory reallocation (realloc) of %d bytes failed\n",
		    size );
		exit( ERR_REALLOC_FAILED );
	}
	return( new );
}



int
read_config( char *fname)
{
    FILE	*fp;
    char	buf[BUFSIZ];
    char	*line, *p;
    int		cargc;
    char	*cargv[MAXARGS];

/*    printf("opening config file \"%s\"\n", fname); */

    if ( (fp = fopen( fname, "r" )) == NULL ) {
	perror( fname );
	exit( ERR_COULD_NOT_OPEN_FILE );
    }

    lineno = 0;
    while ( (line = getline( fp )) != NULL ) {
	if ( line[0] == '#' || line[0] == '\0' ) { continue; }
/*	printf("%s\n", line); */
	parse_line( line, &cargc, cargv );

	if ( cargc < 1 ) {
	    printf("line %d: bad config line (ignored)\n", lineno );
	    continue;
	}
	if ( strcasecmp( cargv[0], "replica" ) == 0 ) {
	    parse_replica_line( cargv, cargc );
	}
    }
    fclose( fp );
/*    printf("config file successfully read and parsed\n");*/
    return 0;
}




/*
 * Parse one line of input.
 */
static void
parse_line( char *line, int *argcp, char **argv)
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
strtok_quote( char *line, char *sep)
{
    int		inquote;
    char	*tmp;
    static char	*next;

    if ( line != NULL ) { next = line; }
    while ( *next && strchr( sep, *next ) ) { next++; }
    if ( *next == '\0' ) { next = NULL; return( NULL ); }
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

#define CATLINE( buf )	{ \
    int	len; \
    len = strlen( buf ); \
    while ( lcur + len + 1 > lmax ) { \
	lmax += BUFSIZ; \
	line = (char *) ch_realloc( line, lmax ); \
    } \
    strcpy( line + lcur, buf ); \
    lcur += len; \
}



/*
 * Get a line of input.
 */
static char *
getline( FILE *fp)
{
    char	*p;
    static char	buf[BUFSIZ];
    static char	*line;
    static int	lmax, lcur;

    lcur = 0;
    CATLINE( buf );
    while ( fgets( buf, sizeof(buf), fp ) != NULL ) {
	if ( (p = strchr( buf, '\n' )) != NULL ) {
	    *p = '\0';
	}
	lineno++;
	if ( ! isspace( buf[0] ) ) {
	    return( line );
	}

	CATLINE( buf );
    }
    buf[0] = '\0';

    return( line[0] ? line : NULL );
}



/* 
 * Parse a "replica" line from the config file.  replica lines should be
 * in the following format:
 * replica    host=<hostname:portnumber> binddn=<binddn>
 *            bindmethod="simple" credentials=<creds>
 *
 * where:
 * <hostname:portnumber> describes the host name and port number where the
 * replica is running,
 *
 * <binddn> is the DN to bind to the replica slapd as,
 *
 * bindmethod is either "simple" or "kerberos", and
 *
 * The "replica" config file line may be split across multiple lines.  If
 * a line begins with whitespace, it is considered a continuation of the
 * previous line.
 */
#define HOSTSTR                 "host"
#define BINDDNSTR               "binddn"
#define BINDMETHSTR             "bindmethod"
#define SIMPLESTR               "simple"
#define CREDSTR                 "credentials"

/* We support simple (plaintext password) and kerberos authentication */
#define AUTH_SIMPLE     1
#define AUTH_KERBEROS   2

#define	GOT_HOST	1
#define	GOT_DN		2
#define	GOT_METHOD	4
#define	GOT_ALL		( GOT_HOST | GOT_DN | GOT_METHOD )

static int
parse_replica_line( char **cargv, int cargc)
{
    int		gots = 0;
    int		i;
    int		port=0;
    char	*hp, *val;
    char        *hostname;           /* canonical hostname of replica */
    char        *bind_method;         /* AUTH_SIMPLE or AUTH_KERBEROS */
    char        *bind_dn;            /* DN to bind as when replicating */
    char        *password;           /* Password for AUTH_SIMPLE */


	for ( i = 1; i < cargc; i++ )
	{
		if ( !strncasecmp( cargv[ i ], HOSTSTR, strlen( HOSTSTR ))) 
		{
			val = cargv[ i ] + strlen( HOSTSTR ) + 1;
			if (( hp = strchr( val, ':' )) != NULL ) 
			{
				*hp = '\0';
				hp++;
				port = atoi( hp );
			}
		    hostname = strdup( val );
		    gots |= GOT_HOST;
		}
		else if ( !strncasecmp( cargv[ i ], 
			BINDDNSTR, strlen( BINDDNSTR ))) 
		{ 
			val = cargv[ i ] + strlen( BINDDNSTR ) + 1;
			bind_dn = strdup( val );
			gots |= GOT_DN;
		}
		else if ( !strncasecmp( cargv[ i ], BINDMETHSTR, 
			strlen( BINDMETHSTR ))) 
		{
			val = cargv[ i ] + strlen( BINDMETHSTR ) + 1;
			if ( !strcasecmp( val, SIMPLESTR )) {
				bind_method = strdup( val );
				gots |= GOT_METHOD;
			} else {
				bind_method = 0;
			}
		}
		else if ( !strncasecmp( cargv[ i ], CREDSTR, 
			strlen( CREDSTR ))) 
		{
			val = cargv[ i ] + strlen( CREDSTR ) + 1;
			password = strdup( val );
		}
		else
		{
/*			printf("parse_replica_line: unknown keyword \"%s\"\n", cargv[i]);*/
		}
	}
	if ( gots != GOT_ALL )
	{
/*		printf( "Malformed \"replica\" line in slapd config file, line %d\n", lineno ); */
		exit(ERR_BADLY_FORMED_RECORD);
	}
	else
	{
		if ( port == 0 )
		{
			printf("host %s\n", hostname);
		}
		else
		{
			printf("host %s:%d\n", hostname,port);
		}
		printf("bindmethod %s\n", bind_method);
		printf("bind_dn %s\n", bind_dn);
		printf("password %s\n", password);
	}
    return 0;
}

