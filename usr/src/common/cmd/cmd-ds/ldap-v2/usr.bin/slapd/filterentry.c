/* @(#)filterentry.c	1.4
 *
 * filterentry.c - apply a filter to an entry
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "regex.h"
#include "slap.h"

/*
 * Modified for Patch 21 from the LDAP patch repository,
 * Ashok Choudhry's fix for access violation with free()
 */

extern Attribute	*attr_find();
extern char		*first_word();
extern char		*next_word();
extern char		*phonetic();
extern char		*re_comp();

extern pthread_mutex_t	regex_mutex;

static int	test_filter_list();
static int	test_substring_filter();
static int	test_ava_filter();
static int	test_approx_filter();
static int	test_presence_filter();

/*
 * test_filter - test a filter against a single entry.
 * returns	0	filter matched
 *		-1	filter did not match
 *		>0	an ldap error code
 */

int
test_filter(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    Entry	*e,
    Filter	*f
)
{
	int	rc;

	logDebug(LDAP_LOG_FILTER,"=> test_filter\n",0,0,0);

	switch ( f->f_choice ) {
	case LDAP_FILTER_EQUALITY:
		logDebug( LDAP_LOG_FILTER,
		    "    EQUALITY\n", 0, 0, 0 );

		rc = test_ava_filter( be, conn, op, e, &f->f_ava,
		    LDAP_FILTER_EQUALITY );
		break;

	case LDAP_FILTER_SUBSTRINGS:
		logDebug( LDAP_LOG_FILTER, "    SUBSTRINGS\n", 0, 0, 0 );
		rc = test_substring_filter( be, conn, op, e, f );
		break;

	case LDAP_FILTER_GE:
		logDebug( LDAP_LOG_FILTER, "    GE\n", 0, 0, 0 );
		rc = test_ava_filter( be, conn, op, e, &f->f_ava,
		    LDAP_FILTER_GE );
		break;

	case LDAP_FILTER_LE:
		logDebug( LDAP_LOG_FILTER, "    LE\n", 0, 0, 0 );
		rc = test_ava_filter( be, conn, op, e, &f->f_ava,
		    LDAP_FILTER_LE );
		break;

	case LDAP_FILTER_PRESENT:
		logDebug( LDAP_LOG_FILTER, "    PRESENT\n", 0, 0, 0 );
		rc = test_presence_filter( be, conn, op, e, f->f_type );
		break;

	case LDAP_FILTER_APPROX:
		logDebug( LDAP_LOG_FILTER, "    APPROX\n", 0, 0, 0 );
		rc = test_approx_filter( be, conn, op, e, &f->f_ava );
		break;

	case LDAP_FILTER_AND:
		logDebug( LDAP_LOG_FILTER, "    AND\n", 0, 0, 0 );
		rc = test_filter_list( be, conn, op, e, f->f_and,
		    LDAP_FILTER_AND );
		break;

	case LDAP_FILTER_OR:
		logDebug( LDAP_LOG_FILTER, "    OR\n", 0, 0, 0 );
		rc = test_filter_list( be, conn, op, e, f->f_or,
		    LDAP_FILTER_OR );
		break;

	case LDAP_FILTER_NOT:
		logDebug( LDAP_LOG_FILTER, "    NOT\n", 0, 0, 0 );
		rc = (! test_filter( be, conn, op, e, f->f_not ) );
		break;

	default:
		logDebug( LDAP_LOG_FILTER, "    unknown filter type %d\n",
		    f->f_choice, 0, 0 );
		rc = -1;
	}

	logDebug( LDAP_LOG_FILTER, "<= test_filter %d\n", rc, 0, 0 );
	return( rc );
}

static int
test_ava_filter(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    Entry	*e,
    Ava		*ava,
    int		type
)
{
	int		i, rc;
	Attribute	*a;

	if ( be != NULL && ! access_allowed( be, conn, op, e, ava->ava_type,
	    &ava->ava_value, op->o_dn, ACL_SEARCH ) ) {
		return( -2 );
	}

	if ( (a = attr_find( e->e_attrs, ava->ava_type )) == NULL ) {
		return( -1 );
	}

	if ( a->a_syntax == 0 ) {
		a->a_syntax = attr_syntax( ava->ava_type );
	}
	for ( i = 0; a->a_vals[i] != NULL; i++ ) {
		rc = value_cmp( a->a_vals[i], &ava->ava_value, a->a_syntax,
		    3 );

		switch ( type ) {
		case LDAP_FILTER_EQUALITY:
			if ( rc == 0 ) {
				return( 0 );
			}
			break;

		case LDAP_FILTER_GE:
			if ( rc > 0 ) {
				return( 0 );
			}
			break;

		case LDAP_FILTER_LE:
			if ( rc < 0 ) {
				return( 0 );
			}
			break;
		}
	}

	return( 1 );
}

static int
test_presence_filter(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    Entry	*e,
    char	*type
)
{
	if ( be != NULL && ! access_allowed( be, conn, op, e, type, NULL,
	    op->o_dn, ACL_SEARCH ) ) {
		return( -2 );
	}

	return( attr_find( e->e_attrs, type ) != NULL ? 0 : -1 );
}

static int
test_approx_filter(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    Entry	*e,
    Ava		*ava
)
{
	char		*w1, *w2, *c1, *c2;
	int		i, rc, match;
	Attribute	*a;

	if ( be != NULL && ! access_allowed( be, conn, op, e, ava->ava_type,
	    NULL, op->o_dn, ACL_SEARCH ) ) {
		return( -2 );
	}

	if ( (a = attr_find( e->e_attrs, ava->ava_type )) == NULL ) {
		return( -1 );
	}

	/* for each value in the attribute */
	for ( i = 0; a->a_vals[i] != NULL; i++ ) {
		/*
		 * try to match words in the filter value in order
		 * in the attribute value.
		 */

		w2 = a->a_vals[i]->bv_val;
		/* for each word in the filter value */
		for ( w1 = first_word( ava->ava_value.bv_val ); w1 != NULL;
		    w1 = next_word( w1 ) ) {
			if ( (c1 = phonetic( w1 )) == NULL ) {
				break;
			}

			/*
			 * for each word in the attribute value from
			 * where we left off...
			 */
			for ( w2 = first_word( w2 ); w2 != NULL;
			    w2 = next_word( w2 ) ) {
				c2 = phonetic( w2 );
				if ( strcmp( c1, c2 ) == 0 ) {
					free( c2 );
					break;
				}
				free( c2 );
			}
			free( c1 );

			/*
			 * if we stopped because we ran out of words
			 * before making a match, go on to the next
			 * value.  otherwise try to keep matching
			 * words in this value from where we left off.
			 */
			if ( w2 == NULL ) {
				break;
			} else {
				w2 = next_word( w2 );
			}
		}
		/*
		 * if we stopped because we ran out of words we
		 * have a match.
		 */
		if ( w1 == NULL ) {
			return( 0 );
		}
	}

	return( 1 );
}

static int
test_filter_list(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    Entry	*e,
    Filter	*flist,
    int		ftype
)
{
	int	rc, nomatch;
	Filter	*f;

	logDebug( LDAP_LOG_FILTER, "=> test_filter_list\n", 0, 0, 0 );

	nomatch = 1;
	for ( f = flist; f != NULL; f = f->f_next ) {
		if ( test_filter( be, conn, op, e, f ) != 0 ) {
			if ( ftype == LDAP_FILTER_AND ) {
				logDebug( LDAP_LOG_FILTER,
				    "<= test_filter_list\n", 0, 0, 0 );
				return( 1 );
			}
		} else {
			nomatch = 0;
		}
	}

	logDebug( LDAP_LOG_FILTER,
	    "<= test_filter_list %d\n", nomatch, 0, 0 );
	return( nomatch );
}

static void
strcpy_special( char *d, char *s )
{
	for ( ; *s; s++ ) {
		switch ( *s ) {
		case '.':
		case '\\':
		case '[':
		case ']':
		case '*':
		case '+':
		case '^':
		case '$':
			*d++ = '\\';
			/* FALL */
		default:
			*d++ = *s;
		}
	}
	*d = '\0';
}

static int
test_substring_filter(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    Entry	*e,
    Filter	*f
)
{
	Attribute	*a;
	int		i, rc;
	char		*p, *end, *realval, *tmp;
	char		pat[BUFSIZ];
	char		buf[BUFSIZ];
	struct berval	*val;

	logDebug( LDAP_LOG_FILTER,
	    "=> test_substring_filter\n", 0, 0, 0 );

	if ( be != NULL && ! access_allowed( be, conn, op, e, f->f_sub_type,
	    NULL, op->o_dn, ACL_SEARCH ) ) {
		return( -2 );
	}

	if ( (a = attr_find( e->e_attrs, f->f_sub_type )) == NULL ) {
		return( -1 );
	}

	if ( a->a_syntax & SYNTAX_BIN ) {
		logDebug( LDAP_LOG_FILTER,
		    "<= test_substring_filter (binary attribute)\n",
		    0, 0, 0 );
		return( -1 );
	}

	/*
	 * construct a regular expression corresponding to the
	 * filter and let regex do the work
	 */

	pat[0] = '\0';
	p = pat;
	end = pat + sizeof(pat) - 2;	/* leave room for null */
	if ( f->f_sub_initial != NULL ) {
		strcpy( p, "^" );
		p = strchr( p, '\0' );
		/* 2 * in case every char is special */
		if ( p + 2 * strlen( f->f_sub_initial ) > end ) {

			logDebug( LDAP_LOG_FILTER,
			   "(test_substring_filter) not enough pattern space\n",
			    0, 0, 0 );

			return( -1 );
		}
		strcpy_special( p, f->f_sub_initial );
		p = strchr( p, '\0' );
	}
	if ( f->f_sub_any != NULL ) {
		for ( i = 0; f->f_sub_any[i] != NULL; i++ ) {
			/* ".*" + value */
			if ( p + 2 * strlen( f->f_sub_any[i] ) + 2 > end ) {

				logDebug( LDAP_LOG_FILTER,
			   "(test_substring_filter) not enough pattern space\n",
				    0, 0, 0 );

				return( -1 );
			}
			strcpy( p, ".*" );
			p = strchr( p, '\0' );
			strcpy_special( p, f->f_sub_any[i] );
			p = strchr( p, '\0' );
		}
	}
	if ( f->f_sub_final != NULL ) {
		/* ".*" + value */
		if ( p + 2 * strlen( f->f_sub_final ) + 2 > end ) {

			logDebug( LDAP_LOG_FILTER,
	"(test_substring_filter) not enough pattern space\n",
			    0, 0, 0 );

			return( -1 );
		}
		strcpy( p, ".*" );
		p = strchr( p, '\0' );
		strcpy_special( p, f->f_sub_final );
		p = strchr( p, '\0' );
		strcpy( p, "$" );
	}

	/* compile the regex */
	pthread_mutex_lock( &regex_mutex );
	if ( (p = re_comp( pat )) != 0 ) {

		logDebug( LDAP_LOG_FILTER,
		     "(test_substring_filter) re_comp failed (%s)\n",
		      p, 0, 0 );

		pthread_mutex_unlock( &regex_mutex );
		return( -1 );
	}

	/* for each value in the attribute see if regex matches */
	for ( i = 0; a->a_vals[i] != NULL; i++ ) {
		val = a->a_vals[i];
		tmp = NULL;
		if ( val->bv_len < sizeof(buf) ) {
			strcpy( buf, val->bv_val );
			realval = buf;
		} else {
			tmp = (char *) ch_malloc( val->bv_len + 1 );
			if ( tmp == NULL ) {
				return ( -1 );
			}
			strcpy( tmp, val->bv_val );
			realval = tmp;
		}
		value_normalize( realval, a->a_syntax );

		rc = re_exec( realval );

		if ( tmp != NULL ) {
			free( tmp );
		}
		if ( rc == 1 ) {
			pthread_mutex_unlock( &regex_mutex );
			return( 0 );
		}
	}
	pthread_mutex_unlock( &regex_mutex );

	logDebug( LDAP_LOG_FILTER, "<= test_substring_filter\n",0,0,0);
	return( 1 );
}
