#ident  "@(#)mod_group.c	1.3"
#ident  "$Header$"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <grp.h>
#include <stdlib.h>
#include <unistd.h>
#include <userdefs.h>
#include <errno.h>
#include <mac.h>
#include <priv.h>
#include <pfmt.h>
#include "users.h"
#include "messages.h"
#include <rpc/rpc.h>
#include <rpc/rpc_com.h>
#include <rpc/rpcb_prot.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netconfig.h>
#include <string.h>
#include <stddef.h>

#define nisname(n) (*n == '+' || *n == '-')
static char *domain;


extern struct group *nis_fgetgrent();
void putgrent();

extern	int	strcmp(), unlink(), rename(),
		lckpwdf(), ulckpwdf();

/*
 * Procedure:	mod_group
 *
 * Restrictions:
 *		fopen:		none
 *		lckpwdf:	none
 *		ulckpwdf:	none
 *		access:		none
 *		fgetgrent:	none
 *		fclose:		none
 *		unlink(2):	none
 *		lvlfile(2):	none
 *		rename(2):	none
 *		chmod(2):	none
 *		chown(2):	none
*/

/* Modify group to new gid and/or new name */
int
mod_group(group, gid, newgroup)
	char	*group;
	gid_t	gid;
	char	*newgroup;
{
	register modified = 0;
	char *tname, *t_suffix = ".tmp";
	FILE *e_fptr, *t_fptr;
	struct group *g_ptr;
	struct stat statbuf;
	register char *bp;
	char *val = NULL;
	int vallen, err;
	struct group nisgrp;
	struct group *nisp=&nisgrp;
	int i=0;

	memset(nisp, 0, sizeof(struct group));

	if (stat(GROUP, &statbuf) < 0)  /* get file owner and mode */
		return EX_UPDATE;

	/*
	 * lockout anyone trying to write the group file.  This
	 * is done by calling ``lckpwdf()'' which sets a lock
	 * on the file ``/etc/security/ia/.pwd.lock''.
	*/
	if (lckpwdf() != 0) {
		errmsg (M_GROUP_BUSY);
		return EX_UPDATE;
	}
	if ((e_fptr = fopen(GROUP, "r")) == NULL) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}
	
	tname = (char *) malloc(strlen(GROUP) + strlen(t_suffix) + 1);

	(void) sprintf(tname, "%s%s", GROUP, t_suffix);

	/* See if temp file exists before continuing */
	if (access(tname, F_OK) == 0) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}

	if ((t_fptr = fopen( tname, "w+")) == NULL) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}

	errno = 0;

	if(nisname(group)) {
		if (!yp_get_default_domain(&domain)){
			if (!yp_match(domain, "group.byname", newgroup, strlen(newgroup), &val, &vallen)) {
				val[vallen] = '\0';
				nisp->gr_name = group;
			
				if (bp = strchr(val, '\n'))
			                        *bp = '\0';
			
				if ((bp = strchr(val, ':')) == NULL)
					return(-1);
				*bp++ = '\0';
				nisp->gr_passwd = bp;
			
				if ((bp = strchr(bp, ':')) == NULL)
					return(-1);
				*bp++ = '\0';
				nisp->gr_gid = atoi(bp);
			}
			
		}
	}
	while ((g_ptr = nis_fgetgrent(e_fptr)) != NULL) {
		/*
		 * check to see if group is one to modify
		*/
		if (!strcmp(g_ptr->gr_name, group)) {
			if (newgroup != NULL)
				g_ptr->gr_name = newgroup;
			if(nisname(group) && !nisname(newgroup)) {
				if (nisp->gr_gid) {
					g_ptr->gr_gid = nisp->gr_gid;
				} else {
					g_ptr->gr_gid = gid;
				}
			} else {
				if (gid != -1) {
					g_ptr->gr_gid = gid;
				}
			}
			modified++;
		}
		putgrent(g_ptr, t_fptr);
	}

	(void) fclose(e_fptr);
	(void) fclose(t_fptr);

	if (errno == EINVAL) {
		/* GROUP file contains bad entries */
		(void) unlink(tname);
		(void) ulckpwdf();
		return EX_UPDATE;
	}
	if (modified) {
		/* Set MAC level */
		if (statbuf.st_level) {
			if (lvlfile(tname, MAC_SET, &statbuf.st_level ) < 0 ) {
				(void) unlink(tname);
				(void) ulckpwdf();
				return EX_UPDATE;
			}
		}
		/*
		 * After that, go ahead and change the mode, owner, 
		 * and group of the file.  When all that has been done
		 * successfully, rename the file.
		*/
		if (chmod(tname, statbuf.st_mode) < 0 ||
		    chown(tname, statbuf.st_uid, statbuf.st_gid) < 0 ||
		    rename(tname, GROUP) < 0) {
			(void) unlink(tname);
			(void) ulckpwdf();
			return EX_UPDATE;
		}
	}
	(void) unlink(tname);
	(void) ulckpwdf();

	return EX_SUCCESS;
}
