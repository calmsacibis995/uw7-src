/* @(#)schemaparse.c	1.4
 * 
 * schemaparse.c - routines to parse config file objectclass definitions
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "ldaplog.h"

/* Messages */
#define MSG_PARSEOC1 \
    1,75,"%s: line %d: expecting \"requires\" or \"allows\" got \"%s\"\n"
#define MSG_OCUSE1 \
    1,76,"<object class clause> ::= <objectclass name>\n"
#define MSG_OCUSE2 \
    1,77,"                          [ requires <attribute list> ]\n"
#define MSG_OCUSE3 \
    1,78,"                          [ allows <attribute list> ]\n"


extern char		**str2charray();
extern void		charray_merge();

struct objclass		*global_oc;
int			global_schemacheck;

static void		oc_usage();

void
parse_oc(
    Backend	*be,
    char	*fname,
    int		lineno,
    int		argc,
    char	**argv
)
{
	int		i;
	char		last;
	struct objclass	*oc;
	struct objclass	**ocp;

	oc = (struct objclass *) ch_calloc( 1, sizeof(struct objclass) );
	if ( oc == NULL ) { return; }

	oc->oc_name = strdup( argv[1] );
	for ( i = 2; i < argc; i++ ) {
		/* required attributes */
		if ( strcasecmp( argv[i], "requires" ) == 0 ) {
			do {
				i++;
				if ( i < argc ) {
					last = argv[i][strlen( argv[i] ) - 1];
					charray_merge( &oc->oc_required,
						str2charray( argv[i], "," ) );
				}
			} while ( i < argc && last == ',' );

		/* optional attributes */
		} else if ( strcasecmp( argv[i], "allows" ) == 0 ) {
			do {
				i++;
				if ( i < argc ) {
					last = argv[i][strlen( argv[i] ) - 1];
					charray_merge( &oc->oc_allowed,
						str2charray( argv[i], "," ) );
				}
			} while ( i < argc && last == ',' );

		} else {
			logError(get_ldap_message(MSG_PARSEOC1,
                            fname, lineno, argv[i] ));
			oc_usage();
		}
	}

	ocp = &global_oc;
	while ( *ocp != NULL ) {
		ocp = &(*ocp)->oc_next;
	}
	*ocp = oc;
}

static void
oc_usage()
{
	logError(get_ldap_message(MSG_OCUSE1));
	logError(get_ldap_message(MSG_OCUSE2));
	logError(get_ldap_message(MSG_OCUSE3));
	exit( 1 );
}

