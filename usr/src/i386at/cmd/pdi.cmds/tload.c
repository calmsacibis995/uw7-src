#ident	"@(#)pdi.cmds:tload.c	1.1"

/*
 *	tload is a function that will load all of the PDI target drivers
 *	installed on a system.  It identifies these driver by the presense
 *	of a file named the same as the target driver in /etc/scsi/target.d
 */

#include	<sys/types.h>
#include	<sys/mod.h>
#include	<dirent.h>
#include	<errno.h>

#define DIRNAME	"/etc/scsi/target.d"
#define LIST_SIZE	50

static int forced_to_load[LIST_SIZE], number_forced;

void
tload(char *root)
{
	DIR		*parent;
	struct	dirent	*direntp;
	char	basename[256];

	number_forced = 0;

	if (root == NULL) {
		(void)sprintf(basename, "%s", DIRNAME);
	} else {
		(void)sprintf(basename, "%s%s", root, DIRNAME);
	}

	if ((parent = opendir(basename)) == NULL)
		return;

	while ((direntp = readdir( parent )) != NULL) {
		if (direntp->d_name[0] == '.')
			continue;
		if ((forced_to_load[number_forced] = modload(direntp->d_name)) > 0)
			number_forced++;
	}

	(void)closedir( parent );
	return;
}

void
tuload(void)
{
	int index;

	for (index = 0; index < number_forced; index++) {
		(void)moduload(forced_to_load[index]);
	}
	number_forced = 0;
}
