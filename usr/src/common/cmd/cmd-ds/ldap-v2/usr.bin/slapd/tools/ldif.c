/* @(#)ldif.c	1.3
 *
 * ldif.c
 *
 * Revision History:
 *
 * 
 */
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "lber.h"
#include "ldap.h"
#include "ldif.h"
#include "ldaplog.h"

/* Messages */

#define MSG_USAGE1 \
    5,28,"Usage: %s [-b] <attrtype>\n"


usage( name )
char	*name;
{
	fprintf( stderr, get_ldap_message(MSG_USAGE1, name));
	exit( 1 );
}

main( argc, argv )
    int		argc;
    char	**argv;
{
	char	buf[BUFSIZ];
	char	*type, *out;
	int	binary = 0;

	/* i18n stuff */
	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:ldbmtest");
	open_message_catalog("ldap.cat");

	if (argc < 2 || argc > 3 ) {
		usage( argv[0] );
	}
	if ( argc == 3 ) {
		if ( strcmp( argv[1], "-b" ) != 0 ) {
			usage( argv[0] );
		} else {
			binary = 1;
			type = argv[2];
		}
	} else {
		if ( strcmp( argv[1], "-b" ) == 0 ) {
			usage( argv[0] );
		}
		type = argv[1];
	}

	/* if the -b flag was used, read single binary value from stdin */
	if ( binary ) {
		char	*val;
		int	nread, max, cur;

		if (( val = (char *) malloc( BUFSIZ )) == NULL ) {
			perror( "malloc" );
			exit( 1 );
		}
		max = BUFSIZ;
		cur = 0;
		while ( (nread = read( 0, buf, BUFSIZ )) != 0 ) {
			if ( nread + cur > max ) {
				max += BUFSIZ;
				if (( val = (char *) realloc( val, max )) ==
				    NULL ) {
					perror( "realloc" );
					exit( 1 );
				}
			}
			memcpy( val + cur, buf, nread );
			cur += nread;
		}

		if (( out = ldif_type_and_value( type, val, cur )) == NULL ) {
		    	perror( "ldif_type_and_value" );
			exit( 1 );
		}

		fputs( out, stdout );
		free( out );
		free( val );
		exit( 0 );
	}

	/* not binary:  one value per line... */
	while ( gets( buf ) != NULL ) {
		if (( out = ldif_type_and_value( type, buf, strlen( buf ) ))
		    == NULL ) {
		    	perror( "ldif_type_and_value" );
			exit( 1 );
		}
		fputs( out, stdout );
		free( out );
	}

	exit( 0 );
}
