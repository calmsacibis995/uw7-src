/* @(#)attr.c	1.4
 *
 * attr.c - routines for dealing with attributes 
 *
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>

#include "portable.h"
#include "slap.h"
#include "proto-slap.h"
#include "ldaplog.h"

#define MSG_ATTR \
   5,1,"%s: line %d: missing name in \"attribute <name>+ <syntax>\" (ignored)\n"
#define MSG_ATTRSYN1 \
   5,2,"%s: line %d: unknown syntax \"%s\" in attribute line (ignored)\n"
#define MSG_ATTRSYN2 \
   5,3,"possible syntaxes are \"cis\", \"ces\", \"tel\", \"dn\", or \"bin\"\n"


void
attr_free( Attribute *a )
{
	free( a->a_type );
	ber_bvecfree( a->a_vals );
	free( a );
}

/*
 * attr_normalize - normalize an attribute name (make it all lowercase)
 */

char *
attr_normalize( char *s )
{
	char	*save;

	for ( save = s; *s; s++ ) {
		*s = TOLOWER( *s );
	}

	return( save );
}

/*
 * attr_merge_fast - merge the given type and value with the list of
 * attributes in attrs. called from str2entry(), where we can make some
 * assumptions to make things faster.
 * returns	0	everything went ok
 *		-1	trouble
 */

int
attr_merge_fast(
    Entry		*e,
    char		*type,
    struct berval	**vals,
    int			nvals,
    int			naddvals,
    int			*maxvals,
    Attribute		***a
)
{
	int		i;

	if ( *a == NULL ) {
		for ( *a = &e->e_attrs; **a != NULL; *a = &(**a)->a_next ) {
			if ( strcasecmp( (**a)->a_type, type ) == 0 ) {
				break;
			}
		}
	}

	if ( **a == NULL ) {
		**a = (Attribute *) ch_malloc( sizeof(Attribute) );
		if ( **a == NULL ) { return -1; }

		(**a)->a_type = attr_normalize( strdup( type ) );
		(**a)->a_vals = NULL;
		(**a)->a_syntax = attr_syntax( type );
		(**a)->a_next = NULL;
	}

	return( value_add_fast( &(**a)->a_vals, vals, nvals, naddvals,
	    maxvals ) );
}

/*
 * attr_merge - merge the given type and value with the list of
 * attributes in attrs.
 * returns	0	everything went ok
 *		-1	trouble
 */

int
attr_merge(
    Entry		*e,
    char		*type,
    struct berval	**vals
)
{
	int		i;
	Attribute	**a;

	for ( a = &e->e_attrs; *a != NULL; a = &(*a)->a_next ) {
		if ( strcasecmp( (*a)->a_type, type ) == 0 ) {
			break;
		}
	}

	if ( *a == NULL ) {
		*a = (Attribute *) ch_malloc( sizeof(Attribute) );
		if ( *a == NULL ) { return -1; }

		(*a)->a_type = attr_normalize( strdup( type ) );
		(*a)->a_vals = NULL;
		(*a)->a_syntax = attr_syntax( type );
		(*a)->a_next = NULL;
	}

	return( value_add( &(*a)->a_vals, vals ) );
}

/*
 * attr_find - find and return attribute type in list a
 */

Attribute *
attr_find(
    Attribute	*a,
    char	*type
)
{
	for ( ; a != NULL; a = a->a_next ) {
		if ( strcasecmp( a->a_type, type ) == 0 ) {
			return( a );
		}
	}

	return( NULL );
}

/*
 * attr_delete - delete the attribute type in list pointed to by attrs
 * return	0	deleted ok
 * 		1	not found in list a
 * 		-1	something bad happened
 */

int
attr_delete(
    Attribute	**attrs,
    char	*type
)
{
	Attribute	**a;
	Attribute	*save;

	for ( a = attrs; *a != NULL; a = &(*a)->a_next ) {
		if ( strcasecmp( (*a)->a_type, type ) == 0 ) {
			break;
		}
	}

	if ( *a == NULL ) {
		return( 1 );
	}

	save = *a;
	*a = (*a)->a_next;
	attr_free( save );

	return( 0 );
}

#define DEFAULT_SYNTAX	SYNTAX_CIS

struct asyntaxinfo {
	char	**asi_names;
	int	asi_syntax;
};

static Avlnode	*attr_syntaxes = NULL;

static int
attr_syntax_cmp(
    struct asyntaxinfo        *a1,
    struct asyntaxinfo        *a2
)
{
      return( strcasecmp( a1->asi_names[0], a2->asi_names[0] ) );
}

static int
attr_syntax_name_cmp(
    char		*type,
    struct asyntaxinfo	*a
)
{
	return( strcasecmp( type, a->asi_names[0] ) );
}

static int
attr_syntax_names_cmp(
    char		*type,
    struct asyntaxinfo	*a
)
{
	int	i;

	for ( i = 0; a->asi_names[i] != NULL; i++ ) {
		if ( strcasecmp( type, a->asi_names[i] ) == 0 ) {
			return( 0 );
		}
	}
	return( 1 );
}

static int
attr_syntax_dup(
    struct asyntaxinfo        *a1,
    struct asyntaxinfo        *a2
)
{
	if ( a1->asi_syntax != a2->asi_syntax ) {
		return( -1 );
	}

	return( 1 );
}

/*
 * attr_syntax - return the syntax of attribute type
 */

int
attr_syntax( char *type )
{
	struct asyntaxinfo	*asi = NULL;

	if ( (asi = (struct asyntaxinfo *) avl_find( attr_syntaxes, type,
            attr_syntax_name_cmp )) != NULL || (asi = (struct asyntaxinfo *)
	    avl_find_lin( attr_syntaxes, type, attr_syntax_names_cmp ))
	    != NULL )
	{
		return( asi->asi_syntax );
	}

	return( DEFAULT_SYNTAX );
}

/*
 * attr_syntax_config - process an attribute syntax config line
 */

void
attr_syntax_config(
    char	*fname,
    int		lineno,
    int		argc,
    char	**argv
)
{
	char			*save;
	struct asyntaxinfo	*a;
	int			i, lasti;

	if ( argc < 2 ) {
		logInfo(get_ldap_message(MSG_ATTR,fname,lineno));
		return;
	}

	a = (struct asyntaxinfo *) ch_calloc( 1, sizeof(struct asyntaxinfo) );
	if ( a == NULL ) { return; }

	lasti = argc - 1;
	if ( strcasecmp( argv[lasti], "caseignorestring" ) == 0 ||
	    strcasecmp( argv[lasti], "cis" ) == 0 ) {
		a->asi_syntax = SYNTAX_CIS;
	} else if ( strcasecmp( argv[lasti], "telephone" ) == 0 ||
	    strcasecmp( argv[lasti], "tel" ) == 0 ) {
		a->asi_syntax = (SYNTAX_CIS | SYNTAX_TEL);
	} else if ( strcasecmp( argv[lasti], "dn" ) == 0 ) {
		a->asi_syntax = (SYNTAX_CIS | SYNTAX_DN);
	} else if ( strcasecmp( argv[lasti], "caseexactstring" ) == 0 ||
	    strcasecmp( argv[lasti], "ces" ) == 0 ) {
		a->asi_syntax = SYNTAX_CES;
	} else if ( strcasecmp( argv[lasti], "binary" ) == 0 ||
	    strcasecmp( argv[lasti], "bin" ) == 0 ) {
		a->asi_syntax = SYNTAX_BIN;
	} else {
		logInfo(get_ldap_message(MSG_ATTRSYN1,fname,lineno));
		logInfo(get_ldap_message(MSG_ATTRSYN2,fname,lineno));

		free( (char *) a );
		return;
	}
	save = argv[lasti];
	argv[lasti] = NULL;
	a->asi_names = charray_dup( argv );
	argv[lasti] = save;

	switch ( avl_insert( &attr_syntaxes, a, attr_syntax_cmp,
	    attr_syntax_dup ) ) {
	case -1:	/* duplicate - different syntaxes */

	case 1:		/* duplicate - same syntaxes */
		charray_free( a->asi_names );
		free( (char *) a );
		break;

	default:	/* inserted */
		break;
	}
}
