/* @(#)filter.c	1.4
 *
 * filter.c - routines for parsing and dealing with filters
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "ldaplog.h"

static int	get_filter_list();
static int	get_substring_filter();

extern int	get_ava();
extern char	*ch_malloc();
extern char	*ch_realloc();

int
get_filter( Connection *conn, BerElement *ber, Filter **filt, char **fstr )
{
	unsigned long	tag, len;
	int		err;
	Filter		*f;
	char		*ftmp;

	logDebug( LDAP_LOG_FILTER, "=>get_filter\n", 0, 0, 0 );

	/*
	 * A filter looks like this coming in:
	 *	Filter ::= CHOICE {
	 *		and		[0]	SET OF Filter,
	 *		or		[1]	SET OF Filter,
	 *		not		[2]	Filter,
	 *		equalityMatch	[3]	AttributeValueAssertion,
	 *		substrings	[4]	SubstringFilter,
	 *		greaterOrEqual	[5]	AttributeValueAssertion,
	 *		lessOrEqual	[6]	AttributeValueAssertion,
	 *		present		[7]	AttributeType,,
	 *		approxMatch	[8]	AttributeValueAssertion
	 *	}
	 *
	 *	SubstringFilter ::= SEQUENCE {
	 *		type               AttributeType,
	 *		SEQUENCE OF CHOICE {
	 *			initial          [0] IA5String,
	 *			any              [1] IA5String,
	 *			final            [2] IA5String
	 *		}
	 *	}
	 */

	f = (Filter *) ch_malloc( sizeof(Filter) );
	if ( f == NULL ) {
		return( LDAP_NO_MEMORY );
	}
	*filt = f;
	f->f_next = NULL;

	err = 0;
	*fstr = NULL;
	f->f_choice = ber_peek_tag( ber, &len );
#ifdef COMPAT30
	if ( conn->c_version == 30 ) {
		switch ( f->f_choice ) {
		case LDAP_FILTER_EQUALITY:
		case LDAP_FILTER_GE:
		case LDAP_FILTER_LE:
		case LDAP_FILTER_PRESENT:
		case LDAP_FILTER_PRESENT_30:
		case LDAP_FILTER_APPROX:
			(void) ber_skip_tag( ber, &len );
			if ( f->f_choice == LDAP_FILTER_PRESENT_30 ) {
				f->f_choice = LDAP_FILTER_PRESENT;
			}
			break;
		default:
			break;
		}
	}
#endif
	switch ( f->f_choice ) {
	case LDAP_FILTER_EQUALITY:
		logDebug( LDAP_LOG_FILTER, "EQUALITY\n", 0, 0, 0 );
		if ( (err = get_ava( ber, &f->f_ava )) == 0 ) {
			*fstr = ch_malloc(4 + strlen( f->f_avtype ) +
			    f->f_avvalue.bv_len);
			if ( *fstr == NULL ) {
				return( LDAP_NO_MEMORY );
			}
			sprintf( *fstr, "(%s=%s)", f->f_avtype,
			    f->f_avvalue.bv_val );
		}
		break;

	case LDAP_FILTER_SUBSTRINGS:
		logDebug( LDAP_LOG_FILTER, "SUBSTRINGS\n", 0, 0, 0 );
		err = get_substring_filter( conn, ber, f, fstr );
		break;

	case LDAP_FILTER_GE:
		logDebug( LDAP_LOG_FILTER, "GE\n", 0, 0, 0 );
		if ( (err = get_ava( ber, &f->f_ava )) == 0 ) {
			*fstr = ch_malloc(5 + strlen( f->f_avtype ) +
			    f->f_avvalue.bv_len);
			if ( *fstr == NULL ) {
				return( LDAP_NO_MEMORY );
			}
			sprintf( *fstr, "(%s>=%s)", f->f_avtype,
			    f->f_avvalue.bv_val );
		}
		break;

	case LDAP_FILTER_LE:
		logDebug( LDAP_LOG_FILTER, "LE\n", 0, 0, 0 );
		if ( (err = get_ava( ber, &f->f_ava )) == 0 ) {
			*fstr = ch_malloc(5 + strlen( f->f_avtype ) +
			    f->f_avvalue.bv_len);
			if ( *fstr == NULL ) {
				return( LDAP_NO_MEMORY );
			}
			sprintf( *fstr, "(%s<=%s)", f->f_avtype,
			    f->f_avvalue.bv_val );
		}
		break;

	case LDAP_FILTER_PRESENT:
		logDebug( LDAP_LOG_FILTER, "PRESENT\n", 0, 0, 0 );
		if ( ber_scanf( ber, "a", &f->f_type ) == LBER_ERROR ) {
			err = LDAP_PROTOCOL_ERROR;
		} else {
			err = LDAP_SUCCESS;
			attr_normalize( f->f_type );
			*fstr = ch_malloc( 5 + strlen( f->f_type ) );
			if ( *fstr == NULL ) {
				return( LDAP_NO_MEMORY );
			}
			sprintf( *fstr, "(%s=*)", f->f_type );
		}
		break;

	case LDAP_FILTER_APPROX:
		logDebug( LDAP_LOG_FILTER, "APPROX\n", 0, 0, 0 );
		if ( (err = get_ava( ber, &f->f_ava )) == 0 ) {
			*fstr = ch_malloc(5 + strlen( f->f_avtype ) +
			    f->f_avvalue.bv_len);
			if ( *fstr == NULL ) {
				return( LDAP_NO_MEMORY );
			}
			sprintf( *fstr, "(%s~=%s)", f->f_avtype,
			    f->f_avvalue.bv_val );
		}
		break;

	case LDAP_FILTER_AND:
		logDebug( LDAP_LOG_FILTER, "AND\n", 0, 0, 0 );
		if ( (err = get_filter_list( conn, ber, &f->f_and, &ftmp ))
		    == 0 ) {
			*fstr = ch_malloc( 4 + strlen( ftmp ) );
			if ( *fstr == NULL ) {
				free( ftmp );
				return( LDAP_NO_MEMORY );
			}
			sprintf( *fstr, "(&%s)", ftmp );
			free( ftmp );
		}
		break;

	case LDAP_FILTER_OR:
		logDebug( LDAP_LOG_FILTER, "OR\n", 0, 0, 0 );
		if ( (err = get_filter_list( conn, ber, &f->f_or, &ftmp ))
		    == 0 ) {
			*fstr = ch_malloc( 4 + strlen( ftmp ) );
			if ( *fstr == NULL ) {
				free( ftmp );
				return( LDAP_NO_MEMORY );
			}
			sprintf( *fstr, "(|%s)", ftmp );
			free( ftmp );
		}
		break;

	case LDAP_FILTER_NOT:
		logDebug( LDAP_LOG_FILTER, "NOT\n", 0, 0, 0 );
		(void) ber_skip_tag( ber, &len );
		if ( (err = get_filter( conn, ber, &f->f_not, &ftmp )) == 0 ) {
			*fstr = ch_malloc( 4 + strlen( ftmp ) );
			if ( *fstr == NULL ) {
				free( ftmp );
				return( LDAP_NO_MEMORY );
			}
			sprintf( *fstr, "(!%s)", ftmp );
			free( ftmp );
		}
		break;

	default:
		logDebug( LDAP_LOG_FILTER, "unknown filter type %d\n",
		    f->f_choice,
		    0, 0 );
		err = LDAP_PROTOCOL_ERROR;
		break;
	}

	if ( err != 0 ) {
		free( (char *) f );
		if ( *fstr != NULL ) {
			free( *fstr );
		}
	}

	logDebug( LDAP_LOG_FILTER, "<=get_filter (err=%d)\n", err, 0, 0 );
	return( err );
}

static int
get_filter_list( Connection *conn, BerElement *ber, Filter **f, char **fstr )
{
	Filter		**new;
	int		err;
	unsigned long	tag, len;
	char		*last, *ftmp;

	logDebug( LDAP_LOG_FILTER, "=>get_filter_list\n", 0, 0, 0 );

#ifdef COMPAT30
	if ( conn->c_version == 30 ) {
		(void) ber_skip_tag( ber, &len );
	}
#endif
	*fstr = NULL;
	new = f;
	for ( tag = ber_first_element( ber, &len, &last ); tag != LBER_DEFAULT;
	    tag = ber_next_element( ber, &len, last ) ) {
		if ( (err = get_filter( conn, ber, new, &ftmp )) != 0 ) {
			logDebug( LDAP_LOG_FILTER,
			    "<=get_filter_list\n",0,0,0 );
			return( err );
		}
		if ( *fstr == NULL ) {
			*fstr = ftmp;
		} else {
			*fstr = ch_realloc( *fstr, strlen( *fstr ) +
			    strlen( ftmp ) + 1 );
			if ( *fstr == NULL ) { return( LDAP_NO_MEMORY ); }
			strcat( *fstr, ftmp );
			free( ftmp );
		}
		new = &(*new)->f_next;
	}
	*new = NULL;

	logDebug( LDAP_LOG_FILTER, "<=get_filter_list\n", 0, 0, 0 );
	return( 0 );
}

static int
get_substring_filter(
    Connection	*conn,
    BerElement	*ber,
    Filter	*f,
    char	**fstr
)
{
	unsigned long	tag, len, rc;
	char		*val, *last;
	int		syntax;

	logDebug( LDAP_LOG_FILTER, "=> get_substring_filter\n", 0, 0, 0 );

#ifdef COMPAT30
	if ( conn->c_version == 30 ) {
		(void) ber_skip_tag( ber, &len );
	}
#endif
	if ( ber_scanf( ber, "{a", &f->f_sub_type ) == LBER_ERROR ) {
		logDebug( LDAP_LOG_FILTER, "<= get_substring_filter\n", 0, 0, 0 );
		return( LDAP_PROTOCOL_ERROR );
	}
	attr_normalize( f->f_sub_type );
	syntax = attr_syntax( f->f_sub_type );
	f->f_sub_initial = NULL;
	f->f_sub_any = NULL;
	f->f_sub_final = NULL;

	*fstr = ch_malloc( strlen( f->f_sub_type ) + 3 );
	if ( *fstr == NULL ) {
		return( LDAP_NO_MEMORY );
	}
	sprintf( *fstr, "(%s=", f->f_sub_type );
	for ( tag = ber_first_element( ber, &len, &last ); tag != LBER_DEFAULT;
	    tag = ber_next_element( ber, &len, last ) ) {
#ifdef COMPAT30
		if ( conn->c_version == 30 ) {
			rc = ber_scanf( ber, "{a}", &val );
		} else
#endif
			rc = ber_scanf( ber, "a", &val );
		if ( rc == LBER_ERROR ) {
			logDebug( LDAP_LOG_FILTER, "<= get_substring_filter\n",
			    0, 0, 0 );
			return( LDAP_PROTOCOL_ERROR );
		}
		if ( val == NULL || *val == '\0' ) {
			if ( val != NULL ) {
				free( val );
			}
			logDebug( LDAP_LOG_FILTER, "<= get_substring_filter\n",
			    0, 0, 0 );
			return( LDAP_INVALID_SYNTAX );
		}
		value_normalize( val, syntax );

		switch ( tag ) {
#ifdef COMPAT30
		case LDAP_SUBSTRING_INITIAL_30:
#endif
		case LDAP_SUBSTRING_INITIAL:
			logDebug( LDAP_LOG_FILTER, "  INITIAL\n", 0, 0, 0 );
			if ( f->f_sub_initial != NULL ) {

				logDebug( LDAP_LOG_FILTER, 
				    "<= get_substring_filter\n", 0, 0, 0 );

				return( LDAP_PROTOCOL_ERROR );
			}
			f->f_sub_initial = val;
			*fstr = ch_realloc( *fstr, strlen( *fstr ) +
			    strlen( val ) + 1 );
			if ( *fstr == NULL ) { return( LDAP_NO_MEMORY ); }
			strcat( *fstr, val );
			break;

#ifdef COMPAT30
		case LDAP_SUBSTRING_ANY_30:
#endif
		case LDAP_SUBSTRING_ANY:
			logDebug( LDAP_LOG_FILTER, "  ANY\n", 0, 0, 0 );
			charray_add( &f->f_sub_any, val );
			*fstr = ch_realloc( *fstr, strlen( *fstr ) +
			    strlen( val ) + 2 );
			if ( *fstr == NULL ) { return( LDAP_NO_MEMORY ); }
			strcat( *fstr, "*" );
			strcat( *fstr, val );
			break;

#ifdef COMPAT30
		case LDAP_SUBSTRING_FINAL_30:
#endif
		case LDAP_SUBSTRING_FINAL:
			logDebug( LDAP_LOG_FILTER, "  FINAL\n", 0, 0, 0 );
			if ( f->f_sub_final != NULL ) {

				logDebug( LDAP_LOG_FILTER, 
				    "<= get_substring_filter\n", 0, 0, 0 );

				return( LDAP_PROTOCOL_ERROR );
			}
			f->f_sub_final = val;
			*fstr = ch_realloc( *fstr, strlen( *fstr ) +
			    strlen( val ) + 2 );
			if ( *fstr == NULL ) { return( LDAP_NO_MEMORY ); }
			strcat( *fstr, "*" );
			strcat( *fstr, val );
			break;

		default:
			logDebug( LDAP_LOG_FILTER,
				"unknown type\n", tag, 0, 0 );

			logDebug( LDAP_LOG_FILTER, 
			    "<= get_substring_filter\n", 0, 0, 0 );

			return( LDAP_PROTOCOL_ERROR );
		}
	}
	*fstr = ch_realloc( *fstr, strlen( *fstr ) + 3 );
	if ( *fstr == NULL ) { return( LDAP_NO_MEMORY ); }

	if ( f->f_sub_final == NULL ) {
		strcat( *fstr, "*" );
	}
	strcat( *fstr, ")" );

	logDebug( LDAP_LOG_FILTER, "<= get_substring_filter\n", 0, 0, 0 );
	return( 0 );
}

void
filter_free( Filter *f )
{
	Filter	*p, *next;

	if ( f == NULL ) {
		return;
	}

	switch ( f->f_choice ) {
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
	case LDAP_FILTER_APPROX:
		ava_free( &f->f_ava, 0 );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		if ( f->f_sub_type != NULL ) {
			free( f->f_sub_type );
		}
		if ( f->f_sub_initial != NULL ) {
			free( f->f_sub_initial );
		}
		charray_free( f->f_sub_any );
		if ( f->f_sub_final != NULL ) {
			free( f->f_sub_final );
		}
		break;

	case LDAP_FILTER_PRESENT:
		if ( f->f_type != NULL ) {
			free( f->f_type );
		}
		break;

	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
	case LDAP_FILTER_NOT:
		for ( p = f->f_list; p != NULL; p = next ) {
			next = p->f_next;
			filter_free( p );
		}
		break;

	default:
		logDebug( LDAP_LOG_FILTER, "unknown filter type %d\n",
		    f->f_choice, 0, 0 );
		break;
	}
	free( f );
}

#ifdef LDAP_DEBUG

void
filter_print( Filter *f )
{
	int	i;
	Filter	*p;

	if ( f == NULL ) {
		printf( "NULL" );
	}

	switch ( f->f_choice ) {
	case LDAP_FILTER_EQUALITY:
		printf( "(%s=%s)", f->f_ava.ava_type,
		    f->f_ava.ava_value.bv_val );
		break;

	case LDAP_FILTER_GE:
		printf( "(%s>=%s)", f->f_ava.ava_type,
		    f->f_ava.ava_value.bv_val );
		break;

	case LDAP_FILTER_LE:
		printf( "(%s<=%s)", f->f_ava.ava_type,
		    f->f_ava.ava_value.bv_val );
		break;

	case LDAP_FILTER_APPROX:
		printf( "(%s~=%s)", f->f_ava.ava_type,
		    f->f_ava.ava_value.bv_val );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		printf( "(%s=", f->f_sub_type );
		if ( f->f_sub_initial != NULL ) {
			printf( "%s", f->f_sub_initial );
		}
		if ( f->f_sub_any != NULL ) {
			for ( i = 0; f->f_sub_any[i] != NULL; i++ ) {
				printf( "*%s", f->f_sub_any[i] );
			}
		}
		charray_free( f->f_sub_any );
		if ( f->f_sub_final != NULL ) {
			printf( "*%s", f->f_sub_final );
		}
		break;

	case LDAP_FILTER_PRESENT:
		printf( "%s=*", f->f_type );
		break;

	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
	case LDAP_FILTER_NOT:
		printf( "(%c", f->f_choice == LDAP_FILTER_AND ? '&' :
		    f->f_choice == LDAP_FILTER_OR ? '|' : '!' );
		for ( p = f->f_list; p != NULL; p = p->f_next ) {
			filter_print( p );
		}
		printf( ")" );
		break;

	default:
		printf( "unknown type %d", f->f_choice );
		break;
	}
}

#endif /* ldap_debug */
