/* @(#)str2filter.c	1.3
 * 
 * str2filter.c - parse an rfc 1588 string filter
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"

static char	*find_matching_paren();
static Filter	*str2list();
static Filter	*str2simple();
static int	str2subvals();

Filter *
str2filter( char *str )
{
	Filter	*f;
	char	*end;

	logDebug( LDAP_LOG_FILTER, "=> str2filter (%s)\n",str,0,0);

	if ( str == NULL || *str == '\0' ) {
		return( NULL );
	}

	switch ( *str ) {
	case '(':
		if ( (end = find_matching_paren( str )) == NULL ) {
			filter_free( f );

			logDebug( LDAP_LOG_FILTER,
			    "<= str2filter\n",0,0,0);

			return( NULL );
		}
		*end = '\0';

		str++;
		switch ( *str ) {
		case '&':
			logDebug( LDAP_LOG_FILTER, "(str2filter) AND\n",
			    0, 0, 0 );

			str++;
			f = str2list( str, LDAP_FILTER_AND );
			break;

		case '|':
			logDebug( LDAP_LOG_FILTER, "(str2filter) OR\n",
			    0, 0, 0 );

			str++;
			f = str2list( str, LDAP_FILTER_OR );
			break;

		case '!':
			logDebug( LDAP_LOG_FILTER, "(str2filter) NOT\n",
			    0, 0, 0 );

			str++;
			f = str2list( str, LDAP_FILTER_NOT );
			break;

		default:
			logDebug( LDAP_LOG_FILTER, "(str2filter) simple\n",
			    0, 0, 0 );

			f = str2simple( str );
			break;
		}
		*end = ')';
		break;

	default:	/* assume it's a simple type=value filter */
		logDebug( LDAP_LOG_FILTER, "(str2filter) default\n",
		    0, 0, 0 );

		f = str2simple( str );
		break;
	}

	logDebug( LDAP_LOG_FILTER,"<= str2filter\n",0,0,0);
	return( f );
}

/*
 * Put a list of filters like this "(filter1)(filter2)..."
 */

static Filter *
str2list( char *str, unsigned long ftype )
{
	Filter	*f;
	Filter	**fp;
	char	*next;
	char	save;

	f = (Filter *) ch_calloc( 1, sizeof(Filter) );
	if ( f == NULL ) { return NULL; }

	f->f_choice = ftype;
	fp = &f->f_list;

	while ( *str ) {
		while ( *str && isspace( *str ) )
			str++;
		if ( *str == '\0' )
			break;

		if ( (next = find_matching_paren( str )) == NULL ) {
			filter_free( f );
			return( NULL );
		}
		save = *++next;
		*next = '\0';

		/* now we have "(filter)" with str pointing to it */
		if ( (*fp = str2filter( str )) == NULL ) {
			filter_free( f );
			*next = save;
			return( NULL );
		}
		*next = save;

		str = next;
		fp = &(*fp)->f_next;
	}
	*fp = NULL;

	return( f );
}

static Filter *
str2simple( char *str )
{
	Filter		*f;
	char		*s;
	char		*value, savechar;

	if ( (s = strchr( str, '=' )) == NULL ) {
		return( NULL );
	}
	value = s + 1;
	*s-- = '\0';
	savechar = *s;

	f = (Filter *) ch_calloc( 1, sizeof(Filter) );
	if ( f == NULL ) { return NULL; }

	switch ( *s ) {
	case '<':
		f->f_choice = LDAP_FILTER_LE;
		*s = '\0';
		break;
	case '>':
		f->f_choice = LDAP_FILTER_GE;
		*s = '\0';
		break;
	case '~':
		f->f_choice = LDAP_FILTER_APPROX;
		*s = '\0';
		break;
	default:
		if ( strchr( value, '*' ) == NULL ) {
			f->f_choice = LDAP_FILTER_EQUALITY;
		} else if ( strcmp( value, "*" ) == 0 ) {
			f->f_choice = LDAP_FILTER_PRESENT;
		} else {
			f->f_choice = LDAP_FILTER_SUBSTRINGS;
			f->f_sub_type = strdup( str );
			if ( str2subvals( value, f ) != 0 ) {
				filter_free( f );
				*(value-1) = '=';
				return( NULL );
			}
			*(value-1) = '=';
			return( f );
		}
		break;
	}

	if ( f->f_choice == LDAP_FILTER_PRESENT ) {
		f->f_type = strdup( str );
	} else {
		f->f_avtype = strdup( str );
		f->f_avvalue.bv_val = strdup( value );
		f->f_avvalue.bv_len = strlen( value );
	}

	*s = savechar;
	*(value-1) = '=';
	return( f );
}

static int
str2subvals( char *val, Filter *f )
{
	char	*nextstar;
	int	gotstar;

	gotstar = 0;
	while ( val != NULL && *val ) {
		if ( (nextstar = strchr( val, '*' )) != NULL )
			*nextstar++ = '\0';

		if ( gotstar == 0 ) {
			f->f_sub_initial = strdup( val );
		} else if ( nextstar == NULL ) {
			f->f_sub_final = strdup( val );
		} else {
			charray_add( &f->f_sub_any, strdup( val ) );
		}

		gotstar = 1;
		if ( nextstar != NULL )
			*(nextstar-1) = '*';
		val = nextstar;
	}

	return( 0 );
}

/*
 * find_matching_paren - return a pointer to the right paren in s matching
 * the left paren to which *s currently points
 */

static char *
find_matching_paren( char *s )
{
	int	balance, escape;

	balance = 0;
	escape = 0;
	for ( ; *s; s++ ) {
		if ( escape == 0 ) {
			if ( *s == '(' )
				balance++;
			else if ( *s == ')' )
				balance--;
		}
		if ( balance == 0 ) {
			return( s );
		}
		if ( *s == '\\' && ! escape )
			escape = 1;
		else
			escape = 0;
	}

	return( NULL );
}
