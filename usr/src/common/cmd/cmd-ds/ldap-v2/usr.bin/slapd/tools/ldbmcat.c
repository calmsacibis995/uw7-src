/* @(#)ldbmcat.c	1.3
 *
 * ldbmcat.c 
 *
 * Revison History:
 *
 *
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "ldbm.h"
#include "../slap.h"

/* Messages */
#define MSG_USAGE1 \
    5,30,"Usage: %s [-n] <filename>\n"


usage( name )
char	*name;
{
	fprintf( stderr, get_ldap_message(MSG_USAGE1, name ));
	exit( 1 );
}

main( argc, argv )
    int		argc;
    char	**argv;
{
	Datum		key, last, data;
	LDBM		dbp;
	int		rc, type;
	long		id;
	char		*file, *s;
	int		printid = 1;

	/* i18n stuff */
        (void)setlocale(LC_ALL, "");
        (void)setlabel("UX:ldbmcat");
        open_message_catalog("ldap.cat");


	if ( argc < 2 || argc > 3 || ( argc == 3 && strcmp( argv[1], "-n" )
	    != 0 )) {
		usage( argv[0] );
	}
	if ( argc == 3 && strcmp( argv[1], "-n" ) == 0 ) {
		printid = 0;
		file = argv[2];
	} else {
		file = argv[1];
	}

	if ( (dbp = ldbm_open( file, LDBM_READER, 0, 0 )) == NULL ) {
		perror( file );
		exit ( 1 );
	}

        last.dptr = NULL;
        for ( key = ldbm_firstkey( dbp ); key.dptr != NULL;
            key = ldbm_nextkey( dbp, last ) ) {
                if ( last.dptr != NULL )
                        ldbm_datum_free( dbp, last );
                data = ldbm_fetch( dbp, key );
		s = data.dptr;
		if ( !printid && isdigit( *s )) {
			if (( s = strchr( s, '\n' )) != NULL ) {
				++s;
			}
		}
		if ( s != NULL ) {
			puts( s );
		}
                ldbm_datum_free( dbp, data );
                last = key;
        }
        if ( last.dptr != NULL )
                ldbm_datum_free( dbp, last );
	ldbm_close( dbp );

	exit( 0 );
}
