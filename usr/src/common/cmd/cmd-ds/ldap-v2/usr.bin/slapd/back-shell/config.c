/* @(#)config.c	1.3
 *
 * config.c - shell backend configuration file routine
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "slap.h"
#include "shell.h"
#include "ldaplog.h"

/* Messages */
#define MSG_ERR1 \
    1,99,"%s: line %d: Uncaught error in shell database structure\n"
#define MSG_NOEXECSTRING \
    1,100,"%s: line %d: missing <executable> in \"%s <executable>\" expression\n"
#define MSG_UNKDIR \
    1,101,"%s: line %d: unknown directive \"%s\" in shell database definition (ignored)\n"


/* report_missing_executable
 *
 * Macro to save on the number of similar messages 
 *
 */
#define report_missing_executable( file, lineno, expr ) \
     logError(get_ldap_message(MSG_NOEXECSTRING, file, lineno, expr))


extern char	**charray_dup();

shell_back_config(
    Backend	*be,
    char	*fname,
    int		lineno,
    int		argc,
    char	**argv
)
{
	struct shellinfo	*si = (struct shellinfo *) be->be_private;

	if ( si == NULL ) {

		logError(get_ldap_message(MSG_ERR1,fname, lineno));
		exit( 1 );
	}

	/* command + args to exec for binds */
	if ( strcasecmp( argv[0], "bind" ) == 0 ) {
		if ( argc < 2 ) {
			report_missing_executable( fname, lineno, "bind" );
			exit( 1 );
		}
		si->si_bind = charray_dup( &argv[1] );

	/* command + args to exec for unbinds */
	} else if ( strcasecmp( argv[0], "unbind" ) == 0 ) {
		if ( argc < 2 ) {
			report_missing_executable( fname, lineno, "unbind" );
			exit( 1 );
		}
		si->si_unbind = charray_dup( &argv[1] );

	/* command + args to exec for searches */
	} else if ( strcasecmp( argv[0], "search" ) == 0 ) {
		if ( argc < 2 ) {
			report_missing_executable( fname, lineno, "search" );
			exit( 1 );
		}
		si->si_search = charray_dup( &argv[1] );

	/* command + args to exec for compares */
	} else if ( strcasecmp( argv[0], "compare" ) == 0 ) {
		if ( argc < 2 ) {

			report_missing_executable( fname, lineno, "compare" );

			exit( 1 );
		}
		si->si_compare = charray_dup( &argv[1] );

	/* command + args to exec for modifies */
	} else if ( strcasecmp( argv[0], "modify" ) == 0 ) {
		if ( argc < 2 ) {

			report_missing_executable( fname, lineno, "modify" );

			exit( 1 );
		}
		si->si_modify = charray_dup( &argv[1] );

	/* command + args to exec for modrdn */
	} else if ( strcasecmp( argv[0], "modrdn" ) == 0 ) {
		if ( argc < 2 ) {

			report_missing_executable( fname, lineno, "modrdn" );

			exit( 1 );
		}
		si->si_modrdn = charray_dup( &argv[1] );

	/* command + args to exec for add */
	} else if ( strcasecmp( argv[0], "add" ) == 0 ) {
		if ( argc < 2 ) {

			report_missing_executable( fname, lineno, "add" );

			exit( 1 );
		}
		si->si_add = charray_dup( &argv[1] );

	/* command + args to exec for delete */
	} else if ( strcasecmp( argv[0], "delete" ) == 0 ) {
		if ( argc < 2 ) {

			report_missing_executable( fname, lineno, "delete" );

			exit( 1 );
		}
		si->si_delete = charray_dup( &argv[1] );

	/* command + args to exec for abandon */
	} else if ( strcasecmp( argv[0], "abandon" ) == 0 ) {
		if ( argc < 2 ) {

			report_missing_executable( fname, lineno, "abandon" );

			exit( 1 );
		}
		si->si_abandon = charray_dup( &argv[1] );

	/* anything else */
	} else {
		logError(get_ldap_message(MSG_UNKDIR,fname, lineno, argv[0]));

	}
}
