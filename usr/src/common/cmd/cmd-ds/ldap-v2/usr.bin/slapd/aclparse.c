/* "@(#)aclparse.c	1.5"
 * routines to parse and check acl's 
 *
 * Revision history:
 *
 * 13 Jun 97	geoffh
 *	Changes made to handle reverse lookups. A condition has been added
 *	so that if a slapd ACL checks the domain of a client then the address
 *	of the client will automatically be looked up., rather than get the 
 *	address of the client every time - or not at all.
 *
 * 1  Aug 97    tonylo
 *      Fixes to ACL parsing routines and addition of logging information
 */


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdarg.h>

#include "regex.h"
#include "slap.h"
#include "portable.h"
#include "ldaplog.h"

#define MSG_NOMEM1 \
    5,32,"Out of memory parsing ACLs\n"
#define ERROR_MSG_1 \
    1,1,"%s: line %d: There is more than one 'to' clause defined in the ACL entry\n"
#define ERROR_MSG_2 \
    1,2,"%s: line %d: Incomplete ACL entry\n"
#define ERROR_MSG_3 \
    1,3,"%s: line %d: 'to <what>' clause must be specified before 'by' clause\n"
#define ERROR_MSG_4 \
    1,4,"%s: line %d: Invalid token \"%s\" in ACL entry\n"
#define ERROR_MSG_5 \
    1,5,"%s: line %d: Invalid '<what>' expression\n"
#define ERROR_MSG_6 \
    1,6,"%s: line %d: Error in regular expression in clause 'dn=\"%s\"'\n"
#define ERROR_MSG_7 \
    1,7,"%s: line %d: Bad ldap filter \"%s\"\n"
#define ERROR_MSG_8 \
    1,8,"%s: line %d: Error in regular expression in  clause 'dn=\"%s\"'\n"
#define ERROR_MSG_9 \
    1,9,"%s: line %d: Error in regular expression in  clause 'domain=\"%s\"'\n"
#define ERROR_MSG_10 \
    1,10,"%s: line %d: Error in regular expression in  clause 'addr=\"%s\"'\n"
#define ERROR_MSG_11 \
    1,11,"%s: line %d: Expected an \"=\" in <who> expression\n"
#define ERROR_MSG_12 \
    1,12,"%s: line %d: No 'by' clause specified in ACL expression\n"

#define ERROR_MSG_13 \
    5,33,"%s: line %d: Expected <what> clause before 'by' token\n"
#define ERROR_MSG_14 \
    5,34,"%s: line %d: Expected an <access> clause\n"
#define ERROR_MSG_15 \
    5,35,"%s: line %d: Invalid <access> clause \"%s\"\n"

#define ACL_CLAUSE1 \
    1,13,"<access clause> ::= access to <what> [ by <who> <access> ]+ \n"
#define ACL_CLAUSE2 \
    1,14,"<what> ::= * | [dn=<regex>] [filter=<ldapfilter>] [attrs=<attrlist>]\n"
#define ACL_CLAUSE3 \
    1,15,"<attrlist> ::= <attr> | <attr> , <attrlist>\n"
#define ACL_CLAUSE4 \
    1,16,"<who> ::= * | self | dn=<regex> | addr=<regex> |\n\tdomain=<regex> | dnattr=<dnattrname>\n"
#define ACL_CLAUSE5 \
    1,17,"<access> ::= [self]{none | compare | search | read | write }\n"
#define STANDARD_ACL_ERROR \
    1,18,"Syntax error in access control list\n"
#define MSG_NOTOCLAUSE \
    1,19,"%s: line %d: No 'to' clause in ACL definition\n"


extern Filter		*str2filter();
extern char		*re_comp();
extern struct acl	*global_acl;
extern int              getcallername;
extern char		**str2charray();
extern char		*dn_upcase();

static void		split();
static void		acl_append();
static void		access_append();
#ifdef LDAP_DEBUG
static void		print_acl();
static void		print_access();
static void           parse_acl_byclause();
static void           parse_acl_whatclause();
#endif

/*
 * report_acl_error
 *
 */
static void
report_acl_error(int set, int msgid, char *msg, ...)
{
    va_list ap;
    char* format;

    va_start(ap, );
    logError(get_ldap_message(STANDARD_ACL_ERROR));
    logInfo(vget_ldap_message(set,msgid,msg,ap));
    logInfo(get_ldap_message(ACL_CLAUSE1));
    logInfo(get_ldap_message(ACL_CLAUSE2));
    logInfo(get_ldap_message(ACL_CLAUSE3));
    logInfo(get_ldap_message(ACL_CLAUSE4));
    logInfo(get_ldap_message(ACL_CLAUSE5));

    va_end(ap);
    logLocation();
    exit(1);
}

/*
 * parse_acl2
 *
 * Description
 * ~~~~~~~~~~~
 * An user defined ACL expression will be stored in the array 'argv'.
 * This function will look at parse the data in the array and populate the 
 * data structure 'acl_list'. This structure will then be appended to either
 * the global ACL or the database backend ACL.
 * 
 */

void
parse_acl( Backend *be, char *fname, int lineno, int argc, char **argv)
{
	int index;
	char            *expr, *leftstr, *rightstr;
	struct acl      *acl_list;       /* will store acl entry */
	struct access   *access_list;    /* stores the 'who' part of the acl */
	char* msg;

	acl_list=NULL;
	access_list=NULL;

	for( index=1; index < argc; index++)
	{
		/* Get the 'what' part of the ACL */
		if ( strcasecmp( argv[index], "to" ) == 0 )
		{
			/* If acl_list is not NULL then a 'to' token has already
		 	* been specified so exit.
                 	*/
			if ( acl_list != NULL )
			    report_acl_error(ERROR_MSG_1, fname, lineno );

			acl_list=(struct acl *)ch_calloc(1, sizeof(struct acl));
			if ( acl_list == NULL ) {
				logError(get_ldap_message(MSG_NOMEM1));
				exit( LDAP_NO_MEMORY );
			}

			index++;
			parse_acl_whatclause(fname, 
 			    lineno, acl_list, &index, argc, argv);

		}
		else if ( strcasecmp( argv[index], "by" ) == 0 )
		{
			if ( acl_list == NULL )
			    report_acl_error(ERROR_MSG_3, fname, lineno );

			access_list = (struct access *) ch_calloc( 1,
			    sizeof(struct access) );
			if ( access_list == NULL ) {
				logError(get_ldap_message(MSG_NOMEM1));
				exit( LDAP_NO_MEMORY );
			}

			index++;
			parse_acl_byclause(fname,
			    lineno, acl_list, access_list, &index, argc, argv);
		}
		else
		{
			report_acl_error(ERROR_MSG_4, fname, lineno, argv[index]);
		}
	} /* for loop */

	if ( acl_list != NULL ) {

		/* No access specified - exit */
		if ( acl_list->acl_access == NULL ) {
			report_acl_error(ERROR_MSG_12, fname, lineno);
		}

		if ( be != NULL ) {
			acl_append( &be->be_acl, acl_list );
		} else {
			acl_append( &global_acl, acl_list );
		}
	} else {
		report_acl_error(MSG_NOTOCLAUSE, fname, lineno);
	}
}

/*
 * parse_acl_byclause
 *
 * Description
 * ~~~~~~~~~~~
 * This function parses the 'by' clause of an ACL entry
 */
static void
parse_acl_byclause(
	char          *fname, 
	int           lineno,
	struct acl    *acl_list, 
	struct access *access_list, 
	int           *index, 
	const int     argc,
	const char    **argv)
{
	char *leftstr, *rightstr, *expr;

	/* Check for any more args */
	if(*index == argc) report_acl_error(ERROR_MSG_2, fname, lineno);

	/* Start looking for <who>:
	 * 	* | self | dn=<regex> | 
	 * 	addr=<regex> | domain=<regex> | dnattr=<dn attribute>
	 */

	if ( strcasecmp( argv[*index], "*" ) == 0 )
	{
	    logDebug(LDAP_LOG_ACL,"ACL <by>: found \"*\" token\n",0,0,0);
	    access_list->a_dnpat = strdup( ".*" );
	}
	else if ( strcasecmp( argv[*index], "self" ) == 0 )
	{
	    logDebug(LDAP_LOG_ACL,"ACL <by>: found \"self\" token\n",
		0,0,0);
	    access_list->a_dnpat = strdup( "self" );
	}
	else
	{
	    split( argv[*index], '=', &leftstr, &rightstr);
	    if( *rightstr == NULL || *rightstr == '\0' )
		report_acl_error(ERROR_MSG_11, fname, lineno);

	    logDebug(LDAP_LOG_ACL,"ACL <by>: left=\"%s\" right=\"%s\"\n",
		leftstr,rightstr,0);

	    /* Check for 'dn' | 'dnattr' | 'domain' | 'addr' */
	    if ( strcasecmp( leftstr, "dn" ) == 0 )
	    {
	        logDebug(LDAP_LOG_ACL,"ACL <by>: found \"dn\" token\n",
		    0,0,0);
		if ( (expr = re_comp( rightstr )) != NULL )
			report_acl_error(ERROR_MSG_8, fname, lineno, rightstr);
		access_list->a_dnpat = dn_upcase( strdup( rightstr ) );
	    }
	    else if ( strcasecmp( leftstr, "dnattr" ) == 0 )
	    {
	        logDebug(LDAP_LOG_ACL,"ACL <by>: found \"dnattr\" token\n",
		    0,0,0);
		access_list->a_dnattr = strdup( rightstr );
	    }
	    else if ( strcasecmp( leftstr, "domain" ) == 0 )
	    {
		char	*s;

	        logDebug(LDAP_LOG_ACL,"ACL <by>: found \"domain\" token\n",
		    0,0,0);
		if ( (expr = re_comp( rightstr )) != NULL )
			report_acl_error(ERROR_MSG_9, fname, lineno, rightstr);

		access_list->a_domainpat = strdup( rightstr );

		/* normalize the domain */
		for ( s = access_list->a_domainpat; *s; s++ ) *s = TOLOWER( *s );

		/* 
		 * The flag getcallername, will make the
		 * daemon lookup the address of the 
		 * LDAP client
		 */
		getcallername = 1;
	    }
	    else if ( strcasecmp( leftstr, "addr" ) == 0 )
	    {
	        logDebug(LDAP_LOG_ACL,"ACL <by>: found \"addr\" token\n",
		    0,0,0);
		if ( (expr = re_comp( rightstr )) != NULL )
			report_acl_error(ERROR_MSG_10, fname, lineno, rightstr);
		access_list->a_addrpat = strdup( rightstr );
	    }
	    else
	    {
		report_acl_error(ERROR_MSG_4, fname, lineno,leftstr);
	    }
	}

	if ( ++(*index) == argc )
		report_acl_error(ERROR_MSG_14, fname, lineno);

	logDebug(LDAP_LOG_ACL, "ACL <by>: Attempting <access> == %s\n",
	    argv[*index],0,0);
	/* get <access> */
	/*
	split( argv[*index], '=', &leftstr, &rightstr );
	*/
	if ( (access_list->a_access = str2access( argv[*index] )) == -1 ) {
	    report_acl_error(ERROR_MSG_15, fname, lineno, argv[*index]);
	} else {
		/* Add access entry to acl_list */
		access_append( &acl_list->acl_access, access_list );
	}
}


/*
 * parse_acl_whatclause
 *
 * Description
 * ~~~~~~~~~~~
 * This function parses the 'what' clause of an ACL entry
 */
static void
parse_acl_whatclause(
	char *fname, 
	int lineno,
	struct acl *acl_list, 
	int *index, 
	const int argc,
	const char **argv
	)
{
	/* save the original index value */
	int startindex = *index;

	/* Check for any more args */
	if(*index == argc) report_acl_error(ERROR_MSG_2, fname, lineno);
	
	/* Loop until no args left or until a 'by' token */
	for(;*index < argc; (*index)++)
	{
	    if ( strcasecmp( argv[*index], "by" ) == 0 )
	    {
	        logDebug(LDAP_LOG_ACL,"ACL <what>: found \"by\" token\n",
		    0,0,0);

		/* Check if no <what> clause items have been read */
		if( startindex == *index)
			report_acl_error(ERROR_MSG_13, fname, lineno);
		--(*index);
		break;
	    }
	    else if ( strcasecmp( argv[*index], "*" ) == 0 )
	    {
		acl_list->acl_dnpat = strdup( ".*" );
	    }
	    else
	    {
		char *leftstr, *rightstr;
		split( argv[*index], '=', &leftstr, &rightstr);

		if( *rightstr == NULL ||  *rightstr == '\0' )
			report_acl_error(ERROR_MSG_5, fname, lineno);

	        logDebug(LDAP_LOG_ACL,"ACL <what>: left=\"%s\" right=\"%s\"\n", leftstr,rightstr,0);

		/* Now looking for:
		 * dn=<regex> | filter=<ldapfilter> | attr=<attrlist>
		 */
		if( strcasecmp( leftstr, "dn" ) == 0 )
		{
		    char* expr;
	            logDebug(LDAP_LOG_ACL,"ACL <what>: found \"dn\" token\n",0,0,0);
		    if( (expr = re_comp( rightstr )) != NULL )
			report_acl_error(ERROR_MSG_6,fname,lineno,rightstr);
		    acl_list->acl_dnpat = dn_upcase( strdup(rightstr) );
		}
		else if ( strcasecmp( leftstr, "filter" ) == 0 ) 
		{
	            logDebug(LDAP_LOG_ACL,"ACL <what>: found \"filter\" token\n",0,0,0);
		    if ((acl_list->acl_filter = str2filter(rightstr)) == NULL)
		    {
			report_acl_error(ERROR_MSG_7,fname,lineno,rightstr);
		    }
		}
		else if (( strcasecmp( leftstr, "attr" ) == 0 ) ||
		         ( strcasecmp( leftstr, "attrs" ) == 0 ))
		{
		    char **alist;
	            logDebug(LDAP_LOG_ACL,"ACL <what>: found \"attr\" token\n",0,0,0);
		    alist = str2charray( rightstr, "," );
		    charray_merge( &acl_list->acl_attrs, alist);
		    free(alist);
		}
		else
		    report_acl_error(ERROR_MSG_4, fname, lineno, argv[*index]);
	    }
	}
}





char *
access2str( int access )
{
	static char	buf[12];

	if ( access & ACL_SELF ) {
		strcpy( buf, "self" );
	} else {
		buf[0] = '\0';
	}

	if ( access & ACL_NONE ) {
		strcat( buf, "none" );
	} else if ( access & ACL_COMPARE ) {
		strcat( buf, "compare" );
	} else if ( access & ACL_SEARCH ) {
		strcat( buf, "search" );
	} else if ( access & ACL_READ ) {
		strcat( buf, "read" );
	} else if ( access & ACL_WRITE ) {
		strcat( buf, "write" );
	} else {
		strcat( buf, "unknown" );
	}

	return( buf );
}

int
str2access( const char *str )
{
	int	access;

	access = 0;
	if ( strncasecmp( str, "self", 4 ) == 0 ) {
		access |= ACL_SELF;
		str += 4;
	}

	if ( strcasecmp( str, "none" ) == 0 ) {
		access |= ACL_NONE;
	} else if ( strcasecmp( str, "compare" ) == 0 ) {
		access |= ACL_COMPARE;
	} else if ( strcasecmp( str, "search" ) == 0 ) {
		access |= ACL_SEARCH;
	} else if ( strcasecmp( str, "read" ) == 0 ) {
		access |= ACL_READ;
	} else if ( strcasecmp( str, "write" ) == 0 ) {
		access |= ACL_WRITE;
	} else {
		access = -1;
	}

	return( access );
}

static void
split(
    char	*line,
    int		splitchar,
    char	**left,
    char	**right
)
{
	*left = line;
	if ( (*right = strchr( line, splitchar )) != NULL ) {
		*((*right)++) = '\0';
	}
}

static void
access_append( struct access **l, struct access *a )
{
	for ( ; *l != NULL; l = &(*l)->a_next )
		;	/* NULL */

	*l = a;
}

static void
acl_append( struct acl **l, struct acl *a )
{
	for ( ; *l != NULL; l = &(*l)->acl_next )
		;	/* NULL */

	*l = a;
}

#ifdef LDAP_DEBUG

static void
print_access( struct access *b )
{
	printf( "\tby" );
	if ( b->a_dnpat != NULL ) {
		printf( " dn=%s", b->a_dnpat );
	} else if ( b->a_addrpat != NULL ) {
		printf( " addr=%s", b->a_addrpat );
	} else if ( b->a_domainpat != NULL ) {
		printf( " domain=%s", b->a_domainpat );
	} else if ( b->a_dnattr != NULL ) {
		printf( " dnattr=%s", b->a_dnattr );
	}
	printf( " %s\n", access2str( b->a_access ) );
}

static void
print_acl( struct acl *a )
{
	int		i;
	struct access	*b;

	if ( a == NULL ) {
		printf( "NULL\n" );
	}
	printf( "access to" );
	if ( a->acl_filter != NULL ) {
		printf( " filter=" );
		filter_print( a->acl_filter );
	}
	if ( a->acl_dnpat != NULL ) {
		printf( " dn=" );
		printf( a->acl_dnpat );
	}
	if ( a->acl_attrs != NULL ) {
		int	first = 1;

		printf( " attrs=" );
		for ( i = 0; a->acl_attrs[i] != NULL; i++ ) {
			if ( ! first ) {
				printf( "," );
			}
			printf( a->acl_attrs[i] );
			first = 0;
		}
	}
	printf( "\n" );
	for ( b = a->acl_access; b != NULL; b = b->a_next ) {
		print_access( b );
	}
}

#endif
