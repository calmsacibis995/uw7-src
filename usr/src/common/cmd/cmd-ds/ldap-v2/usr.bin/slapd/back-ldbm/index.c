/* @(#)index.c	1.4
 *
 * index.c - routines for dealing with attribute indexes
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"

/* Messages */
#define MSG_NOOPENINDEX \
    1,92,"Could not open database index file %s%s%s\n"


extern char		*first_word();
extern char		*next_word();
extern char		*phonetic();
extern IDList		*idl_fetch();
extern IDList		*idl_allids();
extern struct dbcache	*ldbm_cache_open();

int	index_add_values();

static int	add_value();
static int	index2prefix();

int
index_add_entry(
    Backend	*be,
    Entry	*e
)
{
	Attribute	*ap;
	char		*dnval;
	struct berval	bv;
	struct berval	*bvals[2];

	logDebug( LDAP_LOG_LDBMBE, "=> index_add( %ld, \"%s\" )\n",
	    e->e_id, e->e_dn, 0 );

	/*
	 * dn index entry - make it look like an attribute so it works
	 * with index_add_values() call
	 */

	bv.bv_val = strdup( e->e_dn );
	bv.bv_len = strlen( bv.bv_val );
	(void) dn_normalize_case( bv.bv_val );
	bvals[0] = &bv;
	bvals[1] = NULL;

	/* add the dn to the indexes */
	index_add_values( be, "dn", bvals, e->e_id );

	free( bv.bv_val );

	/* add each attribute to the indexes */
	for ( ap = e->e_attrs; ap != NULL; ap = ap->a_next ) {
		index_add_values( be, ap->a_type, ap->a_vals, e->e_id );
	}

	logDebug( LDAP_LOG_LDBMBE, "<= index_add( %ld, \"%s\" ) 0\n",
	    e->e_id, e->e_dn, 0 );
	return( 0 );
}

int
index_add_mods(
    Backend	*be,
    LDAPMod	*mods,
    ID		id
)
{
	int	rc;

	for ( ; mods != NULL; mods = mods->mod_next ) {
		switch ( mods->mod_op & ~LDAP_MOD_BVALUES ) {
		case LDAP_MOD_ADD:
		case LDAP_MOD_REPLACE:
			rc = index_add_values( be, mods->mod_type,
			    mods->mod_bvalues, id );
			break;

		case LDAP_MOD_DELETE:
			rc = 0;
			break;
		}

		if ( rc != 0 ) {
			return( rc );
		}
	}

	return( 0 );
}

IDList *
index_read(
    Backend	*be,
    char	*type,
    int		indextype,
    char	*val
)
{
	struct ldbminfo *li = (struct ldbminfo *) be->be_private;
	struct dbcache	*db;
	Datum   	key;
	IDList		*idl;
	int		indexmask, syntax;
	char		prefix;
	char		*realval, *tmpval;
	char		buf[BUFSIZ];

	prefix = index2prefix( indextype );
	logDebug(LDAP_LOG_LDBMBE,"=> index_read(\"%s\" \"%c\" \"%s\")\n",
	    type, prefix, val );

	attr_masks( be->be_private, type, &indexmask, &syntax );
	if ( ! (indextype & indexmask) ) {
		idl =  idl_allids( be );
		logDebug( LDAP_LOG_LDBMBE,
		    "<= index_read %d candidates (allids - not indexed)\n",
		    idl ? idl->b_nids : 0, 0, 0 );
		return( idl );
	}

	attr_normalize( type );
	if ( (db = ldbm_cache_open( be, type, LDBM_SUFFIX, LDBM_WRCREAT ))
	    == NULL ) {

		logError( get_ldap_message(MSG_NOOPENINDEX,
                    li->li_directory, type, LDBM_SUFFIX));

		return( NULL );
	}

	realval = val;
	tmpval = NULL;
	if ( prefix != '\0' ) {
		int	len;

		if ( (len = strlen( val )) < sizeof(buf) ) {
			buf[0] = prefix;
			strcpy( &buf[1], val );
			realval = buf;
		} else {
			/* value + prefix + null */
			tmpval = (char *) ch_malloc( len + 2 );
			if ( tmpval == NULL ) { return NULL; }

			tmpval[0] = prefix;
			strcpy( &tmpval[1], val );
			realval = tmpval;
		}
	}

	key.dptr = realval;
	key.dsize = strlen( realval ) + 1;

	idl = idl_fetch( be, db, key );

	if (tmpval != NULL) {
		free(tmpval);
	}
	ldbm_cache_close( be, db );

	logDebug( LDAP_LOG_LDBMBE, "<= index_read %d candidates\n",
	    idl ? idl->b_nids : 0, 0, 0 );
	return( idl );
}

static int
add_value(
    Backend		*be,
    struct dbcache	*db,
    char		*type,
    int			indextype,
    char		*val,
    ID			id
)
{
	int	rc;
	Datum   key;
	IDList	*idl;
	char	prefix;
	char	*realval, *tmpval, *s;
	char	buf[BUFSIZ];

	prefix = index2prefix( indextype );
	logDebug( LDAP_LOG_LDBMBE, "=> add_value( \"%c%s\" )\n",
	    prefix, val, 0 );

	realval = val;
	tmpval = NULL;
	idl = NULL;
	if ( prefix != '\0' ) {
		int	len;

		if ( (len = strlen( val )) < sizeof(buf) ) {
			buf[0] = prefix;
			strcpy( &buf[1], val );
			realval = buf;
		} else {
			/* value + prefix + null */
			tmpval = (char *) ch_malloc( len + 2 );
			if ( tmpval == NULL ) { return NULL; }

			tmpval[0] = prefix;
			strcpy( &tmpval[1], val );
			realval = tmpval;
		}
	}

	key.dptr = realval;
	key.dsize = strlen( realval ) + 1;

	rc = idl_insert_key( be, db, key, id );

	if ( tmpval != NULL ) {
		free( tmpval );
	}
	idl_free( idl );

	pthread_yield();

	return( rc );
}

int
index_add_values(
    Backend		*be,
    char		*type,
    struct berval	**vals,
    ID			id
)
{
	struct ldbminfo *li = (struct ldbminfo *) be->be_private;
	char		*val, *p, *code, *w;
	int		i, j, len;
	int		indexmask, syntax;
	char		buf[SUBLEN + 1];
	char		vbuf[BUFSIZ];
	char		*bigbuf;
	struct dbcache	*db;

	logDebug( LDAP_LOG_LDBMBE, "=> index_add_values(\"%s\", %ld)\n",
	    type,id,0 );

	attr_masks( be->be_private, type, &indexmask, &syntax );
	if ( indexmask == 0 ) {
		return( 0 );
	}

	if ( (db = ldbm_cache_open( be, type, LDBM_SUFFIX, LDBM_WRCREAT ))
	    == NULL ) {

		logError( get_ldap_message(MSG_NOOPENINDEX,
                    li->li_directory, type, LDBM_SUFFIX));

		return( -1 );
	}


	for ( i = 0; vals[i] != NULL; i++ ) {
		/*
		 * presence index entry
		 */
		if ( indexmask & INDEX_PRESENCE ) {
			add_value( be, db, type, INDEX_PRESENCE, "*", id );
		}

		logDebug( LDAP_LOG_LDBMBE,
	"(index_add_values) index_add_values syntax 0x%x syntax bin 0x%x\n",
		    syntax, SYNTAX_BIN, 0 );

		if ( syntax & SYNTAX_BIN ) {
			ldbm_cache_close( be, db );
			return( 0 );
		}

		bigbuf = NULL;
		len = vals[i]->bv_len;

		/* value + null */
		if ( len + 2 > sizeof(vbuf) ) {
			bigbuf = (char *) ch_malloc( len + 1 );
			if ( bigbuf == NULL ) { return -1; }

			val = bigbuf;
		} else {
			val = vbuf;
		}
		(void) memcpy( val, vals[i]->bv_val, len );
		val[len] = '\0';

		value_normalize( val, syntax );

		/*
		 * equality index entry
		 */
		if ( indexmask & INDEX_EQUALITY ) {
			add_value( be, db, type, INDEX_EQUALITY, val, id );
		}

		/*
		 * approximate index entry
		 */
		if ( indexmask & INDEX_APPROX ) {
			for ( w = first_word( val ); w != NULL;
			    w = next_word( w ) ) {
				if ( (code = phonetic( w )) != NULL ) {
					add_value( be, db, type, INDEX_APPROX,
					    code, id );
					free( code );
				}
			}
		}

		/*
		 * substrings index entry
		 */
		if ( indexmask & INDEX_SUB ) {
			/* leading and trailing */
			if ( len > SUBLEN - 2 ) {
				buf[0] = '^';
				for ( j = 0; j < SUBLEN - 1; j++ ) {
					buf[j + 1] = val[j];
				}
				buf[SUBLEN] = '\0';

				add_value( be, db, type, INDEX_SUB, buf, id );

				p = val + len - SUBLEN + 1;
				for ( j = 0; j < SUBLEN - 1; j++ ) {
					buf[j] = p[j];
				}
				buf[SUBLEN - 1] = '$';
				buf[SUBLEN] = '\0';

				add_value( be, db, type, INDEX_SUB, buf, id );
			}

			/* any */
			for ( p = val; p < (val + len - SUBLEN + 1); p++ ) {
				for ( j = 0; j < SUBLEN; j++ ) {
					buf[j] = p[j];
				}
				buf[SUBLEN] = '\0';

				add_value( be, db, type, INDEX_SUB, buf, id );
			}
		}

		if ( bigbuf != NULL ) {
			free( bigbuf );
		}
	}
	ldbm_cache_close( be, db );

	return( 0 );
}

static int
index2prefix( int indextype )
{
	int	prefix;

	switch ( indextype ) {
	case INDEX_EQUALITY:
		prefix = EQ_PREFIX;
		break;
	case INDEX_APPROX:
		prefix = APPROX_PREFIX;
		break;
	case INDEX_SUB:
		prefix = SUB_PREFIX;
		break;
	default:
		prefix = '\0';
		break;
	}

	return( prefix );
}
