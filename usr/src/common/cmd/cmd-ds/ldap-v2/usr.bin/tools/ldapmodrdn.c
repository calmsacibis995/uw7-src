/* @(#)ldapmodrdn.c	1.6
 *
 * ldapmodrdn.c - generic program to modify an entry's RDN using LDAP
 *
 */

/* Revision history:
 *
 * 20/2/97 tonylo
 *	I18N
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <lber.h>
#include <ldap.h>

#include "ldapconfig.h"
#include "ldaplog.h"

static char	*binddn = NULL;
static char	*passwd = NULL;
static char	*ldaphost = NULL;
static int	ldapport = LDAP_PORT;
static int	not, verbose, contoper;
static LDAP	*ld;

void ldap_read_defaults(FILE *fp);
char *ldap_getdef(char *name);

/* Messages */
#define USAGE \
    1,219,"Usage: %s [-abcnrvFW] [-d debug-level] [-h ldaphost] [-p ldap port] \
[-D binddn] [-w passwd] [ -f file | < entryfile ]\n"
#define MSG_BOTHW \
    1,220,"Cannot use  both -w and -W options\n"
#define MSG_PASSWD1 \
    1,221,"Password:"
#define MSG_INVARGS1 \
    1,222,"%s: invalid number of arguments, only two allowed\n"
#define MSG_DOMODRDN1 \
    1,223,"Removing old RDN\n"
#define MSG_DOMODRDN2 \
    1,224,"Keeping old RDN\n"
#define MSG_DOMODRDN3 \
    1,225,"modrdn complete\n"

#define safe_realloc( ptr, size )	( ptr == NULL ? malloc( size ) : \
					 realloc( ptr, size ))


main( argc, argv )
    int		argc;
    char	**argv;
{
    char		*myname,*infile, *p, *entrydn, *rdn, buf[ 4096 ];
    FILE		*fp;
    int			rc, i, remove, havedn, authmethod;
    LDAPMod		**pmods;
    int			getpasswd, gotpasswd;
#ifdef KERBEROS
    int			kerberos;
#endif

    extern char	*optarg;
    extern int	optind;

    /* i18n stuff */
    (void)setlocale(LC_ALL, "");
    (void)setlabel("UX:ldapmodrdn");
    open_message_catalog("ldap.cat");

    infile = NULL;
    not = contoper = verbose = remove = 0;
    getpasswd = 0;
    gotpasswd = 0;
#ifdef KERBEROS
    kerberos = 0;
#endif

    myname = (myname = strrchr(argv[0], '/')) == NULL ? argv[0] : ++myname;

    while (( i = getopt( argc, argv, 
#ifdef KERBEROS
		"kKcnvrWh:p:D:w:d:f:" 
#else
		"cnvrWh:p:D:w:d:f:" 
#endif
		)) != EOF ) {
	switch( i ) {
#ifdef KERBEROS
	case 'k':	/* kerberos bind */
	    kerberos = 2;
	    break;
	case 'K':	/* kerberos bind, part one only */
	    kerberos = 1;
	    break;
#endif
	case 'c':	/* continuous operation mode */
	    ++contoper;
	    break;
	case 'h':	/* ldap host */
	    ldaphost = strdup( optarg );
	    break;
	case 'D':	/* bind DN */
	    binddn = strdup( optarg );
	    break;
	case 'w':	/* password */
	    passwd = strdup( optarg );
	    gotpasswd = 1;
	    break;
	case 'W':	/* bind password from stdin */
	    getpasswd = 1;
	    break;
	case 'd':
	    ldapdebug_level = atoi( optarg );	/* */
	    if(ldapdebug_level & LDAP_LOG_BER) { lberSetDebug(1); }
	    break;
	case 'f':	/* read from file */
	    infile = strdup( optarg );
	    break;
	case 'p':
	    ldapport = atoi( optarg );
	    break;
	case 'n':	/* print adds, don't actually do them */
	    ++not;
	    break;
	case 'v':	/* verbose mode */
	    verbose++;
	    break;
	case 'r':	/* remove old RDN */
	    remove++;
	    break;
	default:
	    logError(get_ldap_message(USAGE, argv[0] ));
	    exit( 1 );
	}
    }


    if (getpasswd) {
 	if (gotpasswd) {
	    logError(get_ldap_message(MSG_BOTHW));
 	    exit(1);
 	}
 	passwd = (char *)ldap_getpass(get_ldap_message(MSG_PASSWD1));
    }

    /*
     * If search host or bind DN not explicitly supplied, 
     * try environment.  
     */
    if (ldaphost == NULL) {
	ldaphost = getenv("LDAP_HOST");
    }

    if (binddn == NULL) {
	binddn = getenv("LDAP_BINDDN_CHANGE");
    }

    /*
     * If still waiting to find out host or binddn,
     * try a config file
     */
    if ((ldaphost == NULL) || (binddn == NULL)) {
        FILE 	*fp1;
	fp1 = fopen( COMMAND_DEFAULTFILE, "r" );

	if (fp1 != 0) {
	    /* Note - it's OK if file not there, we can just default */
	    ldap_read_defaults(fp1);
	    fclose(fp1);
            if (ldaphost == NULL) {
		ldaphost = ldap_getdef("LDAP_HOST");
	    }
	    if (binddn == NULL) {
		binddn = ldap_getdef("LDAP_BINDDN_CHANGE");
	    }
	}
	if (binddn == NULL) {
	    binddn = ""; 
	}
	/*
	 * Note - ldaphost might still be NULL, 
	 * but remainder of code handles NULL and null string identically. 
	 */
    }

    havedn = 0;
    if (argc - optind == 2) {
	if (( rdn = strdup( argv[argc - 1] )) == NULL ) {
	    perror( "strdup" );
	    exit( 1 );
	}
        if (( entrydn = strdup( argv[argc - 2] )) == NULL ) {
	    perror( "strdup" );
	    exit( 1 );
        }
	++havedn;
    } else if ( argc - optind != 0 ) {
	logError(get_ldap_message(MSG_INVARGS1,myname));
	logError(get_ldap_message(USAGE, argv[0] ));
	exit( 1 );
    }

    if ( infile != NULL ) {
	if (( fp = fopen( infile, "r" )) == NULL ) {
	    perror( infile );
	    exit( 1 );
	}
    } else {
	fp = stdin;
    }

    if (( ld = ldap_open( ldaphost, ldapport )) == NULL ) {
	perror( "ldap_open" );
	exit( 1 );
    }

    ld->ld_deref = LDAP_DEREF_NEVER;	/* this seems prudent */

#ifdef KERBEROS
    if ( !kerberos ) {
	authmethod = LDAP_AUTH_SIMPLE;
    } else if ( kerberos == 1 ) {
	authmethod = LDAP_AUTH_KRBV41;
    } else {
	authmethod = LDAP_AUTH_KRBV4;
    }
#else
    authmethod = LDAP_AUTH_SIMPLE;
#endif
    if ( ldap_bind_s( ld, binddn, passwd, authmethod ) != LDAP_SUCCESS ) {
	ldap_perror( ld, "ldap_bind" );
	exit( 1 );
    }

    rc = 0;
    if (havedn)
	rc = domodrdn(ld, entrydn, rdn, remove);
    else while ((rc == 0 || contoper) && fgets(buf, sizeof(buf), fp) != NULL) {
	if ( *buf != '\0' ) {	/* blank lines optional, skip */
	    buf[ strlen( buf ) - 1 ] = '\0';	/* remove nl */

	    if ( havedn ) {	/* have DN, get RDN */
		if (( rdn = strdup( buf )) == NULL ) {
                    perror( "strdup" );
                    exit( 1 );
		}
		rc = domodrdn(ld, entrydn, rdn, remove);
		havedn = 0;
	    } else if ( !havedn ) {	/* don't have DN yet */
	        if (( entrydn = strdup( buf )) == NULL ) {
		    perror( "strdup" );
		    exit( 1 );
	        }
		++havedn;
	    }
	}
    }

    ldap_unbind( ld );

    exit( rc );
}

domodrdn( ld, dn, rdn, remove )
    LDAP	*ld;
    char	*dn;
    char	*rdn;
    int		remove;	/* flag: remove old RDN */
{
    int	i;

    if ( verbose ) {
	printf( "modrdn %s:\n\t%s\n", dn, rdn );
	if (remove)
	    logError(get_ldap_message(MSG_DOMODRDN1));
	else
	    logError(get_ldap_message(MSG_DOMODRDN2));
    }

    if ( !not ) {
	i = ldap_modrdn2_s( ld, dn, rdn, remove );
	if ( i != LDAP_SUCCESS ) {
	    ldap_perror( ld, "ldap_modrdn2_s" );
	} else if ( verbose ) {
	    logError(get_ldap_message(MSG_DOMODRDN3));
	}
    } else {
	i = LDAP_SUCCESS;
    }

    return( i );
}
