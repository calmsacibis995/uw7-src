/* value.c - routines for dealing with values */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "portable.h"
#include "slap.h"

int
value_add_fast( 
    struct berval	***vals,
    struct berval	**addvals,
    int			nvals,
    int			naddvals,
    int			*maxvals
)
{
	int	need, i, j;

	if ( *maxvals == 0 ) {
		*maxvals = 1;
	}
	need = nvals + naddvals + 1;
	while ( *maxvals < need ) {
		*maxvals *= 2;
		*vals = (struct berval **) ch_realloc( (char *) *vals,
		    *maxvals * sizeof(struct berval *) );
		if ( *vals == NULL ) { return -1; }
	}

	for ( i = 0, j = 0; i < naddvals; i++, j++ ) {
		if ( addvals[i]->bv_len > 0 ) {
			(*vals)[nvals + j] = ber_bvdup( addvals[i] );
		}
	}
	(*vals)[nvals + j] = NULL;

	return( 0 );
}

int
value_add( 
    struct berval	***vals,
    struct berval	**addvals
)
{
	int	n, nn, i, j;

	for ( nn = 0; addvals != NULL && addvals[nn] != NULL; nn++ )
		;	/* NULL */

	if ( *vals == NULL ) {
		*vals = (struct berval **) ch_malloc( (nn + 1)
		    * sizeof(struct berval *) );
		if ( *vals == NULL ) {
			return 1;
		}
		n = 0;
	} else {
		for ( n = 0; (*vals)[n] != NULL; n++ )
			;	/* NULL */
		*vals = (struct berval **) ch_realloc( (char *) *vals,
		    (n + nn + 1) * sizeof(struct berval *) );
		if ( *vals == NULL ) { return -1; }
	}

	for ( i = 0, j = 0; i < nn; i++ ) {
		if ( addvals[i]->bv_len > 0 ) {
			(*vals)[n + j++] = ber_bvdup( addvals[i] );
		}
	}
	(*vals)[n + j] = NULL;

	return( 0 );
}

void
value_normalize(
    char	*s,
    int		syntax
)
{
	char	*d, *save;

	if ( ! (syntax & SYNTAX_CIS) ) {
		return;
	}

	if ( syntax & SYNTAX_DN ) {
		(void) dn_normalize_case( s );
		return;
	}

	save = s;
	for ( d = s; *s; s++ ) {
		if ( (syntax & SYNTAX_TEL) && (*s == ' ' || *s == '-') ) {
			continue;
		}
		*d++ = TOUPPER( *s );
	}
	*d = '\0';
}

#define MIN( a, b )	(a < b ? a : b )

int
value_cmp(
    struct berval	*v1,
    struct berval	*v2,
    int			syntax,
    int			normalize	/* 1 => arg 1; 2 => arg 2; 3 => both */
)
{
	int		rc;
	struct stat	st1, st2;

	if ( normalize & 1 ) {
		v1 = ber_bvdup( v1 );
		value_normalize( v1->bv_val, syntax );
	}
	if ( normalize & 2 ) {
		v2 = ber_bvdup( v2 );
		value_normalize( v2->bv_val, syntax );
	}

	switch ( syntax ) {
	case SYNTAX_CIS:
	case (SYNTAX_CIS | SYNTAX_TEL):
	case (SYNTAX_CIS | SYNTAX_DN):
		rc = strcasecmp( v1->bv_val, v2->bv_val );
		break;

	case SYNTAX_CES:
		rc = strcmp( v1->bv_val, v2->bv_val );
		break;

	case SYNTAX_BIN:
		rc = memcmp( v1->bv_val, v2->bv_val, MIN( v1->bv_len,
		    v2->bv_len ) );
		break;
	}

	if ( normalize & 1 ) {
		ber_bvfree( v1 );
	}
	if ( normalize & 2 ) {
		ber_bvfree( v2 );
	}

	return( rc );
}

int
value_ncmp(
    struct berval	*v1,
    struct berval	*v2,
    int			syntax,
    int			len,
    int			normalize
)
{
	int	rc;

	if ( normalize & 1 ) {
		v1 = ber_bvdup( v1 );
		value_normalize( v1->bv_val, syntax );
	}
	if ( normalize & 2 ) {
		v2 = ber_bvdup( v2 );
		value_normalize( v2->bv_val, syntax );
	}

	switch ( syntax ) {
	case SYNTAX_CIS:
	case (SYNTAX_CIS | SYNTAX_TEL):
		rc = strncasecmp( v1->bv_val, v2->bv_val, len );
		break;

	case SYNTAX_CES:
		rc = strncmp( v1->bv_val, v2->bv_val, len );
		break;

	case SYNTAX_BIN:
		rc = memcmp( v1->bv_val, v2->bv_val, len );
	}

	if ( normalize & 1 ) {
		ber_bvfree( v1 );
	}
	if ( normalize & 2 ) {
		ber_bvfree( v2 );
	}

	return( rc );
}

int
value_find(
    struct berval	**vals,
    struct berval	*v,
    int			syntax,
    int			normalize
)
{
	int	i;

	for ( i = 0; vals[i] != NULL; i++ ) {
		if ( value_cmp( vals[i], v, syntax, normalize ) == 0 ) {
			return( 0 );
		}
	}

	return( 1 );
}
