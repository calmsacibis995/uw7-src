#ident  "@(#)add_group.c	1.3"
#ident  "$Header$"

#include	<sys/types.h>
#include 	<sys/stat.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<userdefs.h>
#include	<priv.h>
#include	<pfmt.h>
#include	<grp.h>

#define OGROUP    "/etc/ogroup"
#define GROUPTEMP "/etc/gtmp"

/*
 * Procedure:	add_group
 *
 * Restrictions:
 *		fopen:		none
 *		lckpwdf:	none
 *		ulckpwdf:	none
 *		fclose:		none
*/

extern	int	lckpwdf(),
		ulckpwdf();

extern struct group *nis_fgetgrent();

int
add_group(group, gid)
	char	*group;	/* name of group to add */
	gid_t	gid;		/* gid of group to add */
{
	FILE	*etcgrp;		/* /etc/group file */
	FILE	*tmpgrp;		/* /etc/gtmp file */
	struct  group *g;
	struct	stat  statbuf;

	/*
	 * lockout anyone trying to write the group file.  This
	 * is done by calling ``lckpwdf()'' which sets a lock
	 * on the file ``/etc/security/ia/.pwd.lock''.
	*/
	if (lckpwdf() != 0) {
		pfmt(stderr, MM_ERROR, ":1346:Group file busy.  Try again later\n");
		return EX_UPDATE;
	}
	if (stat(GROUP, &statbuf) < 0) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}
	if ((etcgrp = fopen(GROUP, "r")) == NULL) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}
	if ((tmpgrp = fopen(GROUPTEMP, "w")) == NULL) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}
	while (g = nis_fgetgrent(etcgrp)) {
		if (*g->gr_name == '+' || *g->gr_name == '-'){
			savenisgrent(g);
		} else {
			putgrent(g, tmpgrp);
		}
	}
	(void) fprintf(tmpgrp, "%s::%ld:\n", group, gid);
	putnisgrents(tmpgrp);

	fclose(tmpgrp);
	fclose(etcgrp);

	if (rename(GROUP, OGROUP) < 0){
		(void) ulckpwdf();
		return EX_UPDATE;
	}
	if (chmod(GROUPTEMP, statbuf.st_mode) < 0 ) {
		pfmt(stderr, MM_ERROR, ":1347:Unable to change mode of group file\n");
		(void) ulckpwdf();
		return EX_UPDATE;
	}
		
	if (chown(GROUPTEMP, 0, 3) != 0) {
		pfmt(stderr, MM_ERROR, ":1348:Unable to change ownership of group file\n");
		(void) ulckpwdf();
		return EX_UPDATE;
	}
	if (rename(GROUPTEMP, GROUP) < 0){
		(void) rename(OGROUP, GROUP);
		(void) ulckpwdf();
		return EX_UPDATE;
	}

	(void) fclose(etcgrp);

	(void) ulckpwdf();

	return EX_SUCCESS;
}
