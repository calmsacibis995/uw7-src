/* @(#)ldapdelete.c	1.6
 * 
 * simple program to delete an entry using LDAP
 *
 * Revision history:
 *
 * 20 Feb 1997	tonylo
 *	I18N
 *
 * 1 Aug 1997	tonylo
 *	Doing the liblog changes. Also got rid of the not option and the 
 *	verbose option. Deleted entries will be displayed to the screen by
 *	default. I got rid of the not option because it did not do anything
 *	but bind to the server.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <lber.h>
#include <ldap.h>

#include "ldapconfig.h"
#include "ldaplog.h"

/* Messages */

#define MSG_USAGE1 \
    1,190,"Usage: %s [-c] [-W] [-d debug-level] [-f file] [-h ldaphost] [-p ldapport] \
[-D binddn] [-w passwd] [dn]...\n"
#define MSG_DODEL2 \
    1,191, "Removed entry %s\n"
#define MSG_PASSWD1 \
    1,192,"Can't use both -w and -W options\n"
#define MSG_PASSWD2 \
    1,193,"Password:"
#define MSG_LDAPBIND \
    1,194,"Cannot authenticate to host \"%s\"\n"


/* prototypes */
int dodelete( LDAP *ld, char *dn );

/* contained in libldap */
extern void ldap_read_defaults(FILE *fp);
extern char *ldap_getdef(char *name);

static char	*binddn = NULL;
static char	*passwd = NULL;
static char	*ldaphost = NULL;
static int	ldapport = LDAP_PORT;
static int	contoper;
static LDAP	*ld;

main( int argc, char **argv )
{
    char		*p, buf[ 4096 ];
    FILE		*fp;
    int			i, rc, linenum;
    int			getpasswd, gotpasswd;

    extern char	*optarg;
    extern int	optind;

    /* i18n stuff */
    (void)setlocale(LC_ALL, "");
    (void)setlabel("UX:ldapdelete");
    open_message_catalog("ldap.cat");

    contoper = 0;
    getpasswd = 0;
    gotpasswd = 0;

    fp = NULL;

    while (( i = getopt( argc, argv, "?Wch:p:D:w:d:f:" )) != EOF ) {
	switch( i ) {
	case 'c':	/* continuous operation mode */
	    ++contoper;
	    break;
	case 'h':	/* ldap host */
	    if ( ldaphost == NULL ) ldaphost = strdup( optarg );
	    break;
	case 'D':	/* bind DN */
	    if ( binddn == NULL ) binddn = strdup( optarg );
	    break;
	case 'w':	/* password */
	    if ( passwd == NULL ) passwd = strdup( optarg );
	    gotpasswd = 1;
	    break;
	case 'W':	/* bind password from stdin */
	    getpasswd = 1;
	    break;
	case 'f':	/* read DNs from a file */
	    if (( fp = fopen( optarg, "r" )) == NULL ) {
		perror( optarg );
		exit( 1 );
	    }
	    break;
	case 'd':
	    ldapdebug_level = atoi( optarg );	/* */
	    if(ldapdebug_level & LDAP_LOG_BER) { lberSetDebug(1); }
	    break;
	case 'p':
	    ldapport = atoi( optarg );
	    break;
	case '?':
	    logError(get_ldap_message(MSG_USAGE1, argv[ 0 ]));
	    exit(0);
	default:
	    logError(get_ldap_message(MSG_USAGE1, argv[ 0 ]));
	    exit( 1 );
	}
    }

    /*
     * If search host or bind DN not explicitly supplied, 
     * try environment.  
     */
    if (ldaphost == NULL) ldaphost = getenv("LDAP_HOST");
    if (binddn == NULL)   binddn   = getenv("LDAP_BINDDN_CHANGE");

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
	if (binddn == NULL) binddn = "";

	/*
	 * Note - ldaphost might still be NULL, 
	 * but remainder of code handles NULL and null string identically. 
	 */
    }

    if ( fp == NULL ) {
	if ( optind >= argc ) {
	    fp = stdin;
	}
    }

    if (( ld = ldap_open( ldaphost, ldapport )) == NULL ) {
	perror( "ldap_open" );
	exit( 1 );
    }

    /* if -W option used */
    if (getpasswd) {

	/* if -w option specified */
 	if (gotpasswd) {
	    logError(get_ldap_message(MSG_PASSWD1));
 	    exit(1);
 	}
	/* get -W password from the command line */
 	passwd = (char *)ldap_getpass(get_ldap_message(MSG_PASSWD2));
    }

    ld->ld_deref = LDAP_DEREF_NEVER;	/* prudent, but probably unnecessary */

    if ( ldap_bind_s( ld, binddn, passwd, LDAP_AUTH_SIMPLE) != LDAP_SUCCESS ) {
	logError(get_ldap_message(MSG_LDAPBIND,
	    strcmp(ldaphost,"")==0 ? "localhost":ldaphost));
	ldap_perror( ld, "ldap_bind" );
	exit( 1 );
    }

    if ( fp == NULL ) {
	for ( ; optind < argc; ++optind ) {
	    rc = dodelete( ld, argv[ optind ] );
	}
    } else {
	rc = 0;
	while ((rc == 0 || contoper) && fgets(buf, sizeof(buf), fp) != NULL) {
	    buf[ strlen( buf ) - 1 ] = '\0';	/* remove trailing newline */
	    if ( *buf != '\0' ) {
		rc = dodelete( ld, buf );
	    }
	}
    }

    ldap_unbind( ld );
    exit( rc );
}

int
dodelete( LDAP *ld, char *dn )
{
    int	rc;

    if (( rc = ldap_delete_s( ld, dn )) != LDAP_SUCCESS ) {
	ldap_perror( ld, "ldap_delete" );
    } else {
	printf(get_ldap_message(MSG_DODEL2,dn));
    }
    return( rc );
}
