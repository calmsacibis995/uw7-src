#ident	"@(#)groups:groups.c	1.4.4.3"
#ident "$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
*/

/*
 * groups - show group memberships
 */

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>

extern struct group *getgrent();
extern struct group *getgrgid();
extern struct passwd *getpwnam();

extern char *malloc();

long ngroups_max;

main(argc, argv)
	int argc;
	char *argv[];
{
	register int xval = 0;
	register struct passwd *pw;

	ngroups_max = sysconf(_SC_NGROUPS_MAX);

	if (ngroups_max < 0) {
		fprintf(stderr, "groups: could not get configuration info\n");
		exit(1);
	}

	if (ngroups_max == 0)
		exit(0);	

	if (argc == 1) {

		if ((pw = getpwuid(getuid())) == NULL) {
			fprintf(stderr, "groups: No passwd entry\n");
			xval = 1;
		} else
			showgroups(pw);

	} else while (*++argv) {

		if ((pw = getpwnam(*argv)) == NULL) {
			fprintf(stderr, "groups: %s : No such user\n", *argv);
			xval = 1;
		} else {
			if (argc > 2)
				printf("%s : ", *argv);
			showgroups(pw);
		}
	}

	exit(xval);

}

showgroups(pw)
	register struct passwd *pw;
{
	register struct group *gr;
	register char **cp;
	int ngroups = 1;
	char primary_grp[BUFSIZ];

	if (ngroups_max == 0)
		return;

	setgrent();
	for (;;) {
		if ((gr = getgrent()) == NULL) {
			printf("%d", pw->pw_gid);
			break;;
		}
		if (pw->pw_gid == gr->gr_gid) {
			printf("%s", gr->gr_name);
			(void)strcpy(primary_grp,gr->gr_name);
			break;;
		}	
	}

	setgrent();
	while ((gr = getgrent()) != NULL) {
		for (cp = gr->gr_mem; ngroups <= ngroups_max && cp && *cp; cp++)
			if ((strcmp(*cp, pw->pw_name) == 0) &&
			    (strcmp(gr->gr_name, primary_grp) != 0)) {
				printf(" %s", gr->gr_name);
				++ngroups;
				break;
			}
	}

	printf("\n");
}
