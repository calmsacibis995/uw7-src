/* @(#)ldapsearch.c	1.8
 *
 * simple program to search for entries using LDAP
 *
 * Revision history:
 *
 * 20 Feb 1997  tonylo
 *      I18N
 *
 * 1 Aug 1997   tonylo
 *      Doing the liblog changes. Also got rid of the -n option.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <lber.h>
#include <ldap.h>
#include <ldif.h>
#include <nl_types.h>

#include "ldapconfig.h"
#include "ldaplog.h"

#define DEFSEP		"="

/* ldapsearch prototypes */
void	usage();
void	init_settings();
void	display_settings();
void	default_settings();
void	get_ldapsearch_args();
int	do_ldapsearch();
int	async_unsorted_search();
int	sync_sorted_search();
void	print_name();
void	print_attr_to_file();
void	print_attr();
int	print_entry2();
int	write_ldif_value();

/* libldap calls */
extern void ldap_read_defaults(FILE *fp);
extern char *ldap_getdef(char *name);

/* Messages */
#define MSG_USAGE1 \
    6,1,"%s: [-n] [-u] [-v] [-t] [-T tempfile] [-A] [-B] [-L] [-R] [-d debuglevel] [-F sep] [-S attribute] [-f file] [-D binddn] [-w bindpasswd] [-W] [-h ldaphost] [-p ldapport] [-b searchbase] [-s scope] [-a deref] [-l timelimit] [-z sizelimit] filter [attrs ... ]\n"
#define MSG_SCOPE \
    6,2,"scope (-s) should be 'base', 'one', or 'sub'\n"
#define MSG_DEREF \
   6,3,"Alias dereferencing (-a) should be 'never', 'search', 'find', or 'always'\n"
#define MSF_FILT1 \
   6,4,"No search filter specified\n"
#define MSG_PASSWD2 \
    6,5,"Cannot use both -w and -W options\n"
#define MSG_PASSWD1 \
   6,6,"Password:"
#define MSG_OPEN1 \
    6,7,"Opening connection to host \"%s\" on port \"%d\"\n"
#define MSG_AUTHEN1 \
    6,8,"Authenticating to host \"%s\"\n"
#define MSG_DOSEARCH1 \
    6,9,"Filter is: (%s)\n"
#define MSG_DOSEARCH2 \
    6,10,"Returning ALL attributes\n"
#define MSG_DOSEARCH3 \
    6,11, "Returning attributes: "
#define MSG_MATCHES \
   6,12,"%d matches\n"
#define MSG_SERVERUP1 \
  6,13,"Check that a slapd process is running and listening to port \"%s\"\n"
#define MSG_SERVERUP2 \
  6,14,"Check that the LDAP server \"%s\" is up and listening to port \"%s\"\n"


typedef struct settings {
    int attrsonly;		/* Display the attributes of an entry only */
    int deref;			/* Do not dereference referrals */
    int scope;			/* Specify scope of search */
    int	getpasswd;		/* Get password from the passwd prompt */
    int gotpasswd;		/* Get password from the command line */
    int	ldap_options;		
    int timelimit;		/* time limit for search */
    int sizelimit;		/* number of entries to retrieve */
    int verbose;		/* display additional search information */
    int includeufn;		/* display UFN version of retrieved DNs */
    int allow_binary;		/* display binary values */
    int vals2tmp;		/* Send the attr of each entry to a file */
    char *temporaryfile;	/* Specify base name of temporary files */
    int ldif;			/* Display retrieved info is ldif format */
    char *binddn;		/* DN to bind as */
    char *passwd;		/* Password to authenticate with */
    char *base;			/* Search base to use */
    char *ldaphost;		/* Host to search */
    int  ldapport;		/* Tcp port to query */
    char *sep;			/* Record separator */
    char *sortattr;		/* Attribute to sort by */
    char *infile;
    char **attrs;
    char *filtpattern;

} SETTINGS;


int
main( int argc, char **argv )
{
    char		line[ BUFSIZ ];
    FILE		*fp;
    int			rc, i, first;
    LDAP		*ld;

    SETTINGS		s;

    /* i18n stuff */
    (void)setlocale(LC_ALL, "");
    (void)setlabel("UX:ldapsearch");
    open_message_catalog("ldap.cat");

    init_settings(&s);
    get_ldapsearch_args(argc, argv, &s);
    default_settings(&s);

    if (s.getpasswd) {
	if (s.gotpasswd) {
	    fprintf(stderr,get_ldap_message(MSG_PASSWD2));
	    exit(1);
	}
	s.passwd = (char *)ldap_getpass(get_ldap_message(MSG_PASSWD1));
    }

    if ( s.infile != NULL ) {
	if ( s.infile[0] == '-' && s.infile[1] == '\0' ) {
	    fp = stdin;
	} else if (( fp = fopen( s.infile, "r" )) == NULL ) {
	    perror( s.infile );
	    exit( 1 );
	}
    }

    if ( s.verbose ) {
	printf(get_ldap_message(MSG_OPEN1,s.ldaphost,s.ldapport));
    }

    /* ldap_open - open an ldap connection */
    if (( ld = ldap_open( s.ldaphost, s.ldapport )) == NULL ) {
	char	*portInHost;
	char	*hostName=NULL;

	portInHost = strchr( s.ldaphost, ':' );

	if ( portInHost != NULL ) {
		char *buf=strdup(s.ldaphost);
		portInHost = strchr( buf, ':' );
		*portInHost = '\0';
		portInHost++;
		hostName=buf;
	} else {
		
		if ( s.ldaphost != NULL ) {
			hostName = strdup(s.ldaphost);
		}
		portInHost=(char*)calloc(20,sizeof(char));
		sprintf(portInHost,"%d",s.ldapport);
	}
	perror( s.ldaphost );

	if ( strcmp(s.ldaphost,"") == 0 ) {
		fprintf(stderr, get_ldap_message( MSG_SERVERUP1, portInHost));
	} else {
		fprintf(stderr, get_ldap_message( MSG_SERVERUP2,
			hostName, portInHost));
	}
	exit( 1 );
    }

    ld->ld_deref = s.deref;
    ld->ld_timelimit = s.timelimit;
    ld->ld_sizelimit = s.sizelimit;
    ld->ld_options = s.ldap_options;

    if ( s.verbose ) {
	printf(get_ldap_message(MSG_AUTHEN1,
	    s.ldaphost== NULL ? "localhost" : s.ldaphost));
    }

    /* ldap_bind_s  - authenticate to server */
    if (ldap_bind_s(ld,s.binddn,s.passwd,LDAP_AUTH_SIMPLE) != LDAP_SUCCESS) {
	ldap_perror( ld, "ldap_bind" );
	exit( 1 );
    }

    if ( s.infile == NULL ) {
	rc = do_ldapsearch(ld, &s, "" );
    } else {
	rc = 0;
	first = 1;
	while ( rc == 0 && fgets( line, sizeof( line ), fp ) != NULL ) {
	    line[ strlen( line ) - 1 ] = '\0';
	    if ( !first ) putchar( '\n' ); else first = 0;
	    rc = do_ldapsearch( ld, &s, line );
	}
	if ( fp != stdin ) fclose( fp );
    }

    ldap_unbind( ld );
    exit( rc );
}

/* 
 * void usage( char *cmd )
 *	Display command usage then exit with an error code
 */
void
usage( char *cmd )
{
	fprintf(stderr,get_ldap_message(MSG_USAGE1,cmd));
	exit(1);
}

/* 
 * void init_settings( SETTINGS *settings )
 *	Initialise settings 
 */
void
init_settings( SETTINGS *settings )
{
    settings->attrsonly=	0;
    settings->deref=		LDAP_DEREF_NEVER;
    settings->scope=		LDAP_SCOPE_SUBTREE;
    settings->getpasswd=	0;
    settings->gotpasswd=	0;
    settings->ldap_options=	LDAP_FLAG_REFERRALS;
    settings->timelimit=	0;
    settings->sizelimit=	0;
    settings->verbose=		0;
    settings->includeufn=	0;
    settings->allow_binary=	0;
    settings->vals2tmp=		0;
    settings->temporaryfile=	NULL;
    settings->ldif=		0;
    settings->binddn=		NULL;
    settings->passwd=		NULL;
    settings->base=		NULL;
    settings->ldaphost=NULL;
    settings->ldapport=		LDAP_PORT;
    settings->sep=		NULL;
    settings->sortattr=		NULL;
    settings->infile=		NULL;
}

void
display_settings(SETTINGS *s)
{
    int i;
    printf("attrsonly:     %d\n",s->attrsonly);
    printf("deref:         %d\n",s->deref);
    printf("scope:         %d\n",s->scope);
    printf("getpasswd:     %d\n",s->getpasswd);
    printf("gotpasswd:     %d\n",s->gotpasswd);
    printf("ldap_options:  %d\n",s->ldap_options);
    printf("timelimit:     %d\n",s->timelimit);
    printf("sizelimit:     %d\n",s->sizelimit);
    printf("verbose:       %d\n",s->verbose);
    printf("includeufn:    %d\n",s->includeufn);
    printf("allow_binary:  %d\n",s->allow_binary);
    printf("vals2tmp:      %d\n",s->vals2tmp);
    printf("ldif:          %d\n",s->ldif);
    printf("ldapport:      %d\n",s->ldapport);
    printf("temporaryfile: %s\n",s->temporaryfile);
    printf("binddn:        %s\n",s->binddn);
    printf("passwd:        %s\n",s->passwd);
    printf("base:          %s\n",s->base);
    printf("ldaphost:      %s\n",s->ldaphost);
    printf("sep:           %s\n",s->sep);
    printf("sortattr:      %s\n",s->sortattr);
    printf("infile:        %s\n",s->infile);
    printf("filtpattern:   %s\n",s->filtpattern);

    printf("attrs:         ");
    if ( s->attrs != NULL )
    {
	for ( i = 0; s->attrs[ i ] != NULL; ++i,printf(" ") )
	    printf( "%s",  s->attrs[ i ] );
    }
    printf("\n");
}

/*
 * void default_settings( SETTINGS *settings )
 *	Set those ldapsearch settings which have not been set by command line
 * 	args to their default values 
 */
void
default_settings( SETTINGS *s )
{
    FILE *fp;

    /* Check if values are to be written to temporary files */
    if (( s->vals2tmp ) && ( s->temporaryfile == NULL ))
	    s->temporaryfile = strdup( "/tmp/ldapsearch" );

    if ( s->sep == NULL ) s->sep = strdup(DEFSEP);

    /*
     * If searchbase, search host or bind DN not explicitly supplied, 
     * try environment.  
     */
    if (s->base == NULL) { s->base = getenv("LDAP_BASE_DN"); }
    if (s->ldaphost == NULL) { s->ldaphost = getenv("LDAP_HOST"); }
    if (s->binddn == NULL) { s->binddn = getenv("LDAP_BINDDN_SEARCH"); }

    /*
     * If still waiting to find out search base or host,
     * try a config file
     */
    if ((s->base == NULL) || (s->ldaphost == NULL) || (s->binddn == NULL)) {
	fp = fopen( COMMAND_DEFAULTFILE, "r" );

	if (fp != 0) {
	    /* Note - it's OK if file not there, we can just default */
	    ldap_read_defaults(fp);
	    fclose(fp);
	    if (s->base == NULL)     s->base = ldap_getdef("LDAP_BASE_DN");
            if (s->ldaphost == NULL) s->ldaphost = ldap_getdef("LDAP_HOST");
	    if (s->binddn == NULL)   s->binddn = 
		ldap_getdef("LDAP_BINDDN_SEARCH");
	}
	if (s->binddn == NULL) s->binddn = "";
	if (s->base == NULL) s->base = "";
    }

}


void
get_ldapsearch_args(int argc, char **argv, SETTINGS *settings)
{
    int i;
    extern char		*optarg;
    extern int		optind;

    while ((i=getopt(argc,argv,"uvtRABLWT:D:s:f:h:b:d:p:F:a:w:l:z:S:"))!=EOF){
	switch( i ) {
	case 'v':	/* verbose mode */
	    ++settings->verbose;
	    break;
	case 'd':
	    ldapdebug_level = atoi( optarg );	/* */
	    if(ldapdebug_level & LDAP_LOG_BER) { lberSetDebug(1); }
	    break;
	case 'u':	/* include UFN */
	    ++settings->includeufn;
	    break;
	case 't':	/* write attribute values to /tmp files */
	    ++settings->vals2tmp;
	    break;
	case 'R':	/* don't automatically chase referrals */
	    settings->ldap_options &= ~LDAP_FLAG_REFERRALS;
	    break;
	case 'A':	/* retrieve attribute names only -- no values */
	    ++settings->attrsonly;
	    break;
	case 'L':	/* print entries in LDIF format */
	    ++settings->ldif;
	    /* fall through -- always allow binary when outputting LDIF */
	case 'B':	/* allow binary values to be printed */
	    ++settings->allow_binary;
	    break;
	case 'T':       /* specify base name of temp files */
	    if (settings->temporaryfile == NULL ) 
		settings->temporaryfile=strdup( optarg );
	    break;
	case 's':	/* search scope */
	    if ( strncasecmp( optarg, "base", 4 ) == 0 ) {
		settings->scope = LDAP_SCOPE_BASE;
	    } else if ( strncasecmp( optarg, "one", 3 ) == 0 ) {
		settings->scope = LDAP_SCOPE_ONELEVEL;
	    } else if ( strncasecmp( optarg, "sub", 3 ) == 0 ) {
		settings->scope = LDAP_SCOPE_SUBTREE;
	    } else {
		fprintf(stderr, get_ldap_message(MSG_SCOPE));
		usage( argv[ 0 ] );
	    }
	    break;

	case 'a':	/* set alias deref option */
	    if ( strncasecmp( optarg, "never", 5 ) == 0 ) {
		settings->deref = LDAP_DEREF_NEVER;
	    } else if ( strncasecmp( optarg, "search", 5 ) == 0 ) {
		settings->deref = LDAP_DEREF_SEARCHING;
	    } else if ( strncasecmp( optarg, "find", 4 ) == 0 ) {
		settings->deref = LDAP_DEREF_FINDING;
	    } else if ( strncasecmp( optarg, "always", 6 ) == 0 ) {
		settings->deref = LDAP_DEREF_ALWAYS;
	    } else {
		fprintf(stderr, get_ldap_message(MSG_DEREF));
		usage( argv[ 0 ] );
	    }
	    break;
	    
	case 'F':	/* field separator */
	    if (settings->sep == NULL ) settings->sep = strdup( optarg );
	    break;
	case 'f':	/* input file */
	    if ( settings->infile == NULL ) settings->infile = strdup( optarg );
	    break;
	case 'h':	/* ldap host */
	    if (settings->ldaphost == NULL ) settings->ldaphost = strdup( optarg );
	    break;
	case 'b':	/* searchbase */
	    if (settings->base == NULL ) settings->base = strdup( optarg );
	    break;
	case 'D':	/* bind DN */
	    if ( settings->binddn == NULL ) settings->binddn = strdup( optarg );
	    break;
	case 'p':	/* ldap port */
	    settings->ldapport = atoi( optarg );
	    break;
	case 'w':	/* bind password */
	    if (settings->passwd == NULL ) settings->passwd = strdup( optarg );
	    settings->gotpasswd = 1;
	    break;
	case 'W':	/* bind password from stdin */
	    settings->getpasswd = 1;
	    break;
	case 'l':	/* time limit */
	    settings->timelimit = atoi( optarg );
	    break;
	case 'z':	/* size limit */
	    settings->sizelimit = atoi( optarg );
	    break;
	case 'S':	/* sort attribute */
	    if (settings->sortattr == NULL) settings->sortattr=strdup(optarg);
	    break;
	default:
	    usage( argv[0] );
	}
    }

    /* Check for filter expression */
    if ( argc - optind < 1 ) {
	fprintf(stderr, get_ldap_message(MSF_FILT1));
	usage( argv[ 0 ] );
    }

    settings->filtpattern = strdup( argv[ optind ] );
    if ( argv[optind+1] == NULL ) settings->attrs = NULL; 
	else settings->attrs = &argv[optind+1];

}

/*
 * int do_ldapsearch( LDAP *ld, SETTINGS *s, char *value )
 *	Perform appropriate search
 */
int
do_ldapsearch( LDAP *ld, SETTINGS *s, char *value )
{
    char	filter[ BUFSIZ ];
    int		rc, matches;

    sprintf( filter, s->filtpattern, value );

    if ( s->verbose ) {
	printf(get_ldap_message(MSG_DOSEARCH1, filter ));
	if ( s->attrs == NULL ) {
		printf(get_ldap_message(MSG_DOSEARCH2));
	} else {
		int i;
		printf(get_ldap_message(MSG_DOSEARCH3));
		for ( i = 0; s->attrs[ i ] != NULL; ++i )
			printf( "%s",  s->attrs[ i ] );
		putchar( '\n' );
	}
    }

    matches = 0;
    if ( s->sortattr != NULL ) {

	rc = sync_sorted_search(ld, s, filter, &matches);

    } else {

	rc = async_unsorted_search( ld, s, filter, &matches);
    } /* endif */


    if ( s->verbose ) {
        printf(get_ldap_message(MSG_MATCHES, matches ));
    }

    return( rc );
}

/*
 * int async_unsorted_search(LDAP *ld, SETTINGS *s, char *filter, int *matches)
 *
 */
int
async_unsorted_search(LDAP *ld, SETTINGS *s, char *filter, int *matches)
{
    int			first, rc;
    LDAPMessage		*res, *e;

    *matches=0;

    /* ldap_search - perform the asunchronous search */
    if (ldap_search(ld,s->base,s->scope,filter,s->attrs,s->attrsonly)==-1) {

	ldap_perror( ld, "ldap_search" );
	return( ld->ld_errno );
    }

    /* ldap_result - get results from ldap_search call */
    while ( (rc = ldap_result(ld, LDAP_RES_ANY, 0, NULL, &res ))
	== LDAP_RES_SEARCH_ENTRY ) 
    {
	
	*matches=*matches+1;

	/* ldap_first_entry - get the entry from the result */
	if( (e = ldap_first_entry( ld, res )) == NULL ) {
	    ldap_perror(ld,"ldap_first_entry");
	    return(1);
	}
	if ( !first ) putchar( '\n' ); else first = 0;
	print_entry2( ld, e, s );
	ldap_msgfree( res );
    }

    rc = ldap_result2error( ld, res, 0 );
    if ( rc != LDAP_SUCCESS ) {

	/* Just report the error */
	ldap_perror( ld, "ldap_search" );
    }


    ldap_msgfree( res );
    return( rc );
}

/* 
 * int sync_sorted_search(...)
 *	Perform a synchronous search, displaying the attributes in a sorted
 *	ordered
 *
 * returns:
 *	number of matches
 */

int
sync_sorted_search(LDAP *ld, SETTINGS *s, char *filter, int *matches)
{
    LDAPMessage		*res, *e;
    int 		first = 1;
    extern int		strcasecmp();

    *matches=0;
    if( ldap_search_s( ld,
	s->base,s->scope,filter,s->attrs,s->attrsonly,&res)==-1) 
    {
	ldap_perror( ld, "ldap_search_s" );
	return( ld->ld_errno );
    }

    ldap_sort_entries(ld,
	&res,(*s->sortattr=='\0') ? NULL:s->sortattr,strcasecmp );

    for ( e = ldap_first_entry( ld, res ); e != NULLMSG;
	e = ldap_next_entry( ld, e ) ) 
    {
	*matches=*matches+1;
	if ( !first ) putchar( '\n' ); else first = 0;
	print_entry2( ld, e, s );
    }
    ldap_msgfree( res );
    return( 0 );
}

/*
 * void print_name( char *attr, char *value, SETTINGS *s)
 *	
 */

void
print_name( char *attr, char *value, SETTINGS *s)
{
    if ( s->ldif ) {
	write_ldif_value( attr, value, strlen( value ));
    } else {
	printf( "%s\n", value );
    }
}

/*
 * print_attr_to_file
 *	Print attributes to a temporary file
 */
void
print_attr_to_file(char *attrname, 
     struct berval *bval, const SETTINGS* settings)
{
    char tmpfname[BUFSIZ];
    FILE *tmpfp=NULL;

    sprintf( tmpfname, "%s-%s-XXXXXX", settings->temporaryfile, attrname );
    if ( mktemp( tmpfname ) == NULL ) {
	perror( tmpfname );
    } else if (( tmpfp = fopen( tmpfname, "w")) == NULL ) {
	perror( tmpfname );
    } else if ( fwrite( bval->bv_val,
	    bval->bv_len, 1, tmpfp ) == 0 ) {
	perror( tmpfname );
    } else if ( settings->ldif ) {
	write_ldif_value( attrname, tmpfname, strlen( tmpfname ));
    } else {
	printf( "%s%s%s\n", attrname, settings->sep, tmpfname );
    }

    if ( tmpfp != NULL ) {
	fclose( tmpfp );
    }
}


/*
 * void print_attr( char *attr, char *value)
 *	Display attributes depending on settings
 */
void
print_attr( char *attrname, struct berval *bval, const SETTINGS* settings)
{

    if (settings->vals2tmp) {
	print_attr_to_file(attrname,bval,settings);
    } else {
	/* Check if entry is binary */
	char	*value=bval->bv_val;
	int	notascii = 0;
	int	j;
	
	if ( !settings->allow_binary ) {
	    for ( j = 0; j < bval->bv_len; ++j ) {
		if ( !isascii( bval->bv_val[ j ] )) {
		    notascii = 1;
		    break;
	        }
	    }
	}

	if (settings->ldif ) {
		write_ldif_value( attrname, bval->bv_val,bval->bv_len );
	} else {
		printf("%s%s%s\n",attrname,settings->sep,
		    notascii ? "NOT ASCII" : bval->bv_val );
	}
    }
}


int
print_entry2( LDAP *ld, LDAPMessage *entry, SETTINGS* s )
{
	char		*dn;
    	struct berval	**bvals;
	BerElement	*ber;
	char 		*attrname;
	int		i;

	/* Get dn (and ufn if required) */
	dn = ldap_get_dn( ld, entry );

	print_name("dn", dn, s);
	if (( s->includeufn ) && (!s->attrsonly)) {
		char *ufn;
		ufn = ldap_dn2ufn( dn );
		print_name("ufn",ufn,s);
		free( ufn );
	}
	free(dn);

	/* Get attributes */
	for (
	    attrname = ldap_first_attribute( ld, entry, &ber ); 
	    attrname != NULL;
	    attrname = ldap_next_attribute( ld, entry, ber ) 
	    ) 
	{
	    if ( s->attrsonly ) {
		if (s->ldif ) {
		    write_ldif_value( attrname, "", 0);
		} else {
		    printf("%s\n",attrname);
		}
	    } else {
		bvals = ldap_get_values_len( ld, entry, attrname );
		for ( i = 0; bvals[i] != NULL; i++ )
		{
		    print_attr( attrname, bvals[i], s);
		}
		ldap_value_free_len(bvals);
	    }
	}
	ber_free(ber,0);
}


int
write_ldif_value( char *type, char *value, unsigned long vallen )
{
    char	*ldif;

    if (( ldif = ldif_type_and_value( type, value, (int)vallen )) == NULL ) {
	return( -1 );
    }

    fputs( ldif, stdout );
    free( ldif );

    return( 0 );
}
