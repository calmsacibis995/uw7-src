/* @(#)config.c	1.4
 *
 * ldbm backend configuration file routine 
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"
#include "ldaplog.h"

/* Messages */

#define MSG_ERR1 \
    1,79,"%s: line %d: Uncaught error in ldbm database structure\n"
#define MSG_MISSINGDIR \
    1,80,"%s: line %d: Missing <dir> in \"directory <dir>\"\n"
#define MSG_MISSINGMODE \
    1,81,"%s: line %d: Missing <mode> in \"mode <mode>\"\n"
#define MSG_MISSINGATTR \
    1,82,"%s: line %d: Missing <attr> in \"index <attr> [pres,eq,approx,sub]\"\n"
#define MSG_ATTRJUNK \
    1,83,"%s: line %d: Extra junk after \"index <attr> [pres,eq,approx,sub]\" line (ignored)\n"
#define MSG_BADCACHESIZE \
    1,84,"%s: line %d: Missing <size> in \"cachesize <size>\"\n"
#define MSG_BADDBCACHESZ \
    1,85,"%s: line %d: Missing <size> in \"dbcachesize <size>\"\n"
#define MSG_BADMINMAX1 \
    1,86,"%s: line %d: Missing <size> in \"min_maxids <size>\"\n"
#define MSG_UNKNOWNDIRECTIVE \
    1,87,"%s: line %d: Unknown directive \"%s\" in ldbm database definition (ignored)\n"


/*
 * ldbm_back_config()
 * 	Read a line from ldbm backend configuration file
 */
ldbm_back_config(
    Backend	*be,
    char	*fname,
    int		lineno,
    int		argc,
    char	**argv
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;

	if ( li == NULL ) {
		logError(get_ldap_message(MSG_ERR1,fname, lineno));
		exit( 1 );
	}

	/* directory where database files live */
	if ( strcasecmp( argv[0], "directory" ) == 0 ) {
		if ( argc < 2 ) {

			logError(get_ldap_message(MSG_MISSINGDIR,
			     fname, lineno));

			exit( 1 );
		}
		li->li_directory = strdup( argv[1] );

	/* mode with which to create new database files */
	} else if ( strcasecmp( argv[0], "mode" ) == 0 ) {
		if ( argc < 2 ) {

			logError(get_ldap_message(MSG_MISSINGMODE,
			    fname, lineno ));
			exit( 1 );
		}
		li->li_mode = strtol( argv[1], NULL, 0 );

	/* attribute to index */
	} else if ( strcasecmp( argv[0], "index" ) == 0 ) {
		if ( argc < 2 ) {

			logError(get_ldap_message(MSG_MISSINGATTR,
			    fname, lineno ));

			exit( 1 );
		} else if ( argc > 3 ) {

			logError(get_ldap_message(MSG_ATTRJUNK,
			    fname, lineno ));
		}
		attr_index_config( li, fname, lineno, argc - 1, &argv[1], 0 );

	/* size of the cache in entries */
	} else if ( strcasecmp( argv[0], "cachesize" ) == 0 ) {
		if ( argc < 2 ) {

			logError(get_ldap_message(MSG_BADCACHESIZE,
			    fname, lineno ));
			exit( 1 );
		}
		li->li_cache.c_maxsize = atoi( argv[1] );

	/* size of each dbcache in bytes */
	} else if ( strcasecmp( argv[0], "dbcachesize" ) == 0 ) {
		if ( argc < 2 ) {

			logError(get_ldap_message(MSG_BADDBCACHESZ,
			    fname, lineno));

			exit( 1 );
		}
		li->li_dbcachesize = atoi( argv[1] );

	/* tuning variable min maxids */
	} else if ( strcasecmp( argv[0], "min_maxids" ) == 0 ) {
		if ( argc < 2 ) {

			logError(get_ldap_message(MSG_BADMINMAX1,
			    fname, lineno ));
			exit( 1 );
		}
		li->li_min_maxids = atoi( argv[1] );

	/* anything else */
	} else {

		logError(get_ldap_message(MSG_UNKNOWNDIRECTIVE,
		    fname, lineno, argv[0] ));
	}
}
