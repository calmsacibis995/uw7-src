#ident  "@(#)groups.c	1.3"
#ident  "$Header$"

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <grp.h>
#include <stdlib.h>
#include <unistd.h>
#include <userdefs.h>
#include <mac.h>
#include <sys/stat.h>
#include "users.h"

struct group *getgrnam();
struct group *getgrgid();
struct group *fgetgrent();
extern void putgrent();
extern long strtol();
extern	int	strcmp(),
		rename(),
		unlink(),
		ck_and_set_flevel();

/*
 * Procedure:	edit_group
 *
 * Restrictions:		
 *		stat(2):	None
 *		fopen:		None
 *		access(2):	None
 *		fgetgrent:	None
 *		fclose:		None
 *		unlink(2):	None
 *		rename(2):	None
 *		chmod(2):	None
 *		chown(2):	None
*/
int
edit_group(login, new_login, gids, overwrite)
	char	*login,
		*new_login;
	gid_t	gids[];		/* group id to add login to */
	int	overwrite;	/* overwrite != 0 means replace existing ones */
{
	char	**memptr, *t_name,
		*t_suffix = ".tmp";
	FILE *e_fptr, *t_fptr;
	struct group *g_ptr;	/* group structure from fgetgrent */
	struct stat stbuf;	
	register i, modified = 0;

	
	if (stat(GROUP, &stbuf) < 0)
		return EX_UPDATE;

	if ((e_fptr = fopen(GROUP, "r")) == NULL) {
		return EX_UPDATE;
	}

	t_name = (char *) malloc(strlen(GROUP) + strlen(t_suffix) + 1);
	
	(void) sprintf(t_name, "%s%s", GROUP, t_suffix);

	/* See if temp file exists before continuing */
	if (access(t_name, F_OK) == 0)
		return EX_UPDATE;

	if ((t_fptr = fopen(t_name, "w+")) == NULL) {
		return EX_UPDATE;
	}

	/* Make TMP file look like we want GROUP file to look */
	while((g_ptr = fgetgrent(e_fptr)) != NULL) {
		
		/* save NIS entries */
		if (*g_ptr->gr_name == '+' || *g_ptr->gr_name == '-'){
			savenisgrent(g_ptr);
			continue;
		}
		/* first delete the login from the group, if it's there */
		if (overwrite || !gids)
			for (memptr = g_ptr->gr_mem; *memptr; memptr++)
				if (!strcmp(*memptr, login)) {
					/* Delete this one */
					char **from = memptr + 1;

					do {
						*(from - 1) = *from;
					} while(*from++);

					modified++;
					break;
				}
		
		/* now check to see if group is one to add to */
		if (gids) 
			for (i = 0; gids[ i ] != -1; i++) 
				if (g_ptr->gr_gid == gids[i]) {
					/* Find end */
					for (memptr = g_ptr->gr_mem; *memptr; memptr++)
						;

					*memptr++ = new_login? new_login: login;
					*memptr = NULL;

					modified++;
				}

		putgrent(g_ptr, t_fptr);
	}

	/* add in the NIS entries */
	putnisgrents(t_fptr);

	(void) fclose(e_fptr);
	(void) fclose(t_fptr);
	
	if (modified) {
		if (ck_and_set_flevel(stbuf.st_level, t_name)) {
			(void) unlink(t_name);
			return EX_UPDATE;
		}
	}
	/*
	 * Now, update GROUP file, if it was modified
	*/
	if (modified && rename(t_name, GROUP) < 0) {
		(void) unlink(t_name);
		return EX_UPDATE;
	}

	(void) chmod(GROUP, stbuf.st_mode);
	(void) chown(GROUP, stbuf.st_uid, stbuf.st_gid);

	(void) unlink(t_name);

	return EX_SUCCESS;
}
