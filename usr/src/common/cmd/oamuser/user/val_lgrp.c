#ident  "@(#)val_lgrp.c	1.3"
#ident  "$Header$"

#include	<sys/types.h>
#include	<stdio.h>
#include	<sys/param.h>
#include	<grp.h>
#include	<users.h>
#include	<userdefs.h>
#include	"messages.h"

extern int valid_group(), get_ngm();
extern void errmsg(), exit();
extern char *strtok();

static gid_t grplist[ NGROUPS_UMAX + 1 ];
static ngroups_max = 0;

/* Validate a list of groups */
int	**
valid_lgroup( list, gid )
char *list;
gid_t gid;
{
	register n_invalid = 0, i = 0, j, overlap = 0;
	char *ptr;
	struct group *g_ptr;

	if( !list )
		return( (int **) NULL );

	while( ptr = strtok( ((i || overlap || n_invalid)? NULL: list), ",") ) {

		switch( valid_group( ptr, &g_ptr ) ) {
		case INVALID:
			errmsg( M_INVALID, ptr, "group id" );
			n_invalid++;
			break;
		case TOOBIG:
			errmsg( M_TOOBIG, "gid", ptr );
			n_invalid++;
			break;
		case UNIQUE:
			errmsg( M_GRP_NOTUSED, ptr );
			n_invalid++;
			break;
		case NOTUNIQUE:
			if( g_ptr->gr_gid == gid ) {
				if (i == 0) overlap++;
				continue;
			}

			if( !i )
				/* ignore respecified primary */
				grplist[ i++ ] = g_ptr->gr_gid;
			else {
				/* Keep out duplicates */
				for( j = 0; j < i; j++ ) 
					if( g_ptr->gr_gid == grplist[j] )
						break;

				if( j == i )
					/* Not a duplicate */
					grplist[i++] = g_ptr->gr_gid;
			}
			break;
				
		}

		if( !ngroups_max )
			ngroups_max = get_ngm();


		if( i >= ngroups_max ) {
			/* trying to add too many groups
			* abort giving errors
			*/
			errmsg( M_NGROUPS_MAX, ngroups_max );
			return NULL;
		}
	}

	/* Terminate the list */
	grplist[ i ] = -1;

	if( n_invalid )
		exit( EX_BADARG );

	return( (int **)grplist );
}
