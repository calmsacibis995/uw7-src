/* "@(#)error.c	1.5"
 *
 * Revision history:
 *
 * 23 Feb 1997	tonylo
 *	I18N
 * 1 Aug 1997	tonylo
 *
 *
 *
 */

#include <stdio.h>
#include <string.h>
#ifdef MACOS
#include <stdlib.h>
#else /* MACOS */
#if defined( DOS ) || defined( _WIN32 )
#include <malloc.h>
#include "msdos.h"
#else /* DOS */
#include <sys/types.h>
#include <sys/socket.h>
#endif /* DOS */
#endif /* MACOS */
#include <unistd.h>
#include "lber.h"
#include "ldap.h"
#include "ldaplog.h"

/* Messages */
#define MSG_PERR0R1 \
    1,226,"%s: %s\n"
#define MSG_PERROR2 \
    1,227,"%s: matched: %s\n"
#define MSG_PERROR3 \
    1,228,"%s: additional info: %s\n"
#define MSG_LDAPPERROR \
    1,229,"%s: Unrecognised LDAP errno %x\n"


struct ldaperror {
	int	e_code;
	int     msg_id;
	char	*e_reason;
};

static struct ldaperror ldap_errlist[] = {
	LDAP_SUCCESS, 			2, "Success",
	LDAP_OPERATIONS_ERROR, 		3, "Operations error",
	LDAP_PROTOCOL_ERROR, 		4, "Protocol error",
	LDAP_TIMELIMIT_EXCEEDED,	5, "Timelimit exceeded",
	LDAP_SIZELIMIT_EXCEEDED, 	6, "Sizelimit exceeded",
	LDAP_COMPARE_FALSE, 		7, "Compare false",
	LDAP_COMPARE_TRUE, 		8, "Compare true",
	LDAP_STRONG_AUTH_NOT_SUPPORTED, 9, "Strong authentication not supported",
	LDAP_STRONG_AUTH_REQUIRED, 	10,"Strong authentication required",
	LDAP_PARTIAL_RESULTS, 		11,"Partial results and referral received",
	LDAP_NO_SUCH_ATTRIBUTE, 	12,"No such attribute",
	LDAP_UNDEFINED_TYPE, 		13,"Undefined attribute type",
	LDAP_INAPPROPRIATE_MATCHING, 	14,"Inappropriate matching",
	LDAP_CONSTRAINT_VIOLATION, 	15,"Constraint violation",
	LDAP_TYPE_OR_VALUE_EXISTS, 	16,"Type or value exists",
	LDAP_INVALID_SYNTAX, 		17,"Invalid syntax",
	LDAP_NO_SUCH_OBJECT, 		18,"No such object",
	LDAP_ALIAS_PROBLEM, 		19,"Alias problem",
	LDAP_INVALID_DN_SYNTAX,		20,"Invalid DN syntax",
	LDAP_IS_LEAF, 			21,"Object is a leaf",
	LDAP_ALIAS_DEREF_PROBLEM, 	22,"Alias dereferencing problem",
	LDAP_INAPPROPRIATE_AUTH, 	23,"Inappropriate authentication",
	LDAP_INVALID_CREDENTIALS, 	24,"Invalid credentials",
	LDAP_INSUFFICIENT_ACCESS, 	25,"Insufficient access",
	LDAP_BUSY, 			26,"DSA is busy",
	LDAP_UNAVAILABLE, 		27,"DSA is unavailable",
	LDAP_UNWILLING_TO_PERFORM, 	28,"DSA is unwilling to perform",
	LDAP_LOOP_DETECT, 		29,"Loop detected",
	LDAP_NAMING_VIOLATION, 		30,"Naming violation",
	LDAP_OBJECT_CLASS_VIOLATION, 	31,"Object class violation",
	LDAP_NOT_ALLOWED_ON_NONLEAF, 	32,"Operation not allowed on nonleaf",
	LDAP_NOT_ALLOWED_ON_RDN, 	33,"Operation not allowed on RDN",
	LDAP_ALREADY_EXISTS, 		34,"Already exists",
	LDAP_NO_OBJECT_CLASS_MODS, 	35,"Cannot modify object class",
	LDAP_RESULTS_TOO_LARGE,		36,"Results too large",
	LDAP_OTHER, 			37,"Unknown error",
	LDAP_SERVER_DOWN,		38,"Can't contact LDAP server",
	LDAP_LOCAL_ERROR,		39,"Local error",
	LDAP_ENCODING_ERROR,		40,"Encoding error",
	LDAP_DECODING_ERROR,		41,"Decoding error",
	LDAP_TIMEOUT,			42,"Timed out",
	LDAP_AUTH_UNKNOWN,		43,"Unknown authentication method",
	LDAP_FILTER_ERROR,		44,"Bad search filter",
	LDAP_USER_CANCELLED,		45,"User cancelled operation",
	LDAP_PARAM_ERROR,		46,"Bad parameter to an ldap routine",
	LDAP_NO_MEMORY,			47,"Out of memory",
	-1, 50, 0
};

char *
ldap_err2string( int err )
{
	int	i;

	for ( i = 0; ldap_errlist[i].e_code != -1; i++ ) {
		if ( err == ldap_errlist[i].e_code ) {
			return( get_ldap_message(2,ldap_errlist[i].msg_id,ldap_errlist[i].e_reason) );

		}
	}

	return( "Unknown error" );
}

#ifndef NO_USERINTERFACE
void
ldap_perror( LDAP *ld, char *s )
{
	int	i;
	char	buf[BUFSIZ];

	logDebug(LDAP_LOG_CLNT, "=> ldap_perror\n",0,0,0);

	if ( ld == NULL ) {
		perror( s );
		logDebug(LDAP_LOG_CLNT,
		    "<= ldap_perror (LDAP struct NULL)\n",0,0,0);
		return;
	}

	for ( i = 0; ldap_errlist[i].e_code != -1; i++ ) {
		if ( ld->ld_errno == ldap_errlist[i].e_code ) {

			logDebug(LDAP_LOG_CLNT,
		"(ldap_perror) Matched an existing error code\n",0,0,0);

			strcpy(buf, get_ldap_message(2,
			    ldap_errlist[i].msg_id,ldap_errlist[i].e_reason));

			logError(get_ldap_message(MSG_PERR0R1,s,buf));

			if ( ld->ld_matched != NULL && *ld->ld_matched != '\0' ) 
				logError(get_ldap_message(MSG_PERROR2,
				    s, ld->ld_matched));

			if ( ld->ld_error != NULL && *ld->ld_error != '\0' ) {

				logError(get_ldap_message(MSG_PERROR3,
				    s, ld->ld_error));

			}
			logDebug(LDAP_LOG_CLNT,"<= ldap_perror\n",0,0,0);
			return;
		}
	}

	logError(get_ldap_message(MSG_LDAPPERROR,s,ld->ld_errno));
	logDebug(LDAP_LOG_CLNT,"<= ldap_perror (Unknown error)\n",0,0,0);
}

#else

void
ldap_perror( LDAP *ld, char *s )
{
}

#endif /* NO_USERINTERFACE */


int
ldap_result2error( LDAP *ld, LDAPMessage *r, int freeit )
{
	LDAPMessage	*lm;
	BerElement	ber;
	long		along;
	int		rc;

	if ( r == NULLMSG )
		return( LDAP_PARAM_ERROR );

	for ( lm = r; lm->lm_chain != NULL; lm = lm->lm_chain )
		;	/* NULL */

	if ( ld->ld_error ) {
		free( ld->ld_error );
		ld->ld_error = NULL;
	}
	if ( ld->ld_matched ) {
		free( ld->ld_matched );
		ld->ld_matched = NULL;
	}

	ber = *(lm->lm_ber);
	if ( ld->ld_version == LDAP_VERSION2 ) {
		rc = ber_scanf( &ber, "{iaa}", &along, &ld->ld_matched,
		    &ld->ld_error );
	} else {
		rc = ber_scanf( &ber, "{ia}", &along, &ld->ld_error );
	}
	if ( rc == LBER_ERROR ) {
		ld->ld_errno = LDAP_DECODING_ERROR;
	} else {
		ld->ld_errno = (int) along;
	}

	if ( freeit )
		ldap_msgfree( r );

	return( ld->ld_errno );
}
