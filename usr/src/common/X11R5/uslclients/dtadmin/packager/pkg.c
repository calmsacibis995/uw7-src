#ifndef NOIDENT
#pragma ident	"@(#)pkg.c	15.1"
#endif

#include <stdio.h>
#include "packager.h"

void
FreePkgList(PkgPtr list, int *count)	/* free strings allocated to packages */
{
register  int	n = *count;

	while (n--) {
		if (list[n].pkg_name)	FREE(list[n].pkg_name);
		if (list[n].pkg_desc)	FREE(list[n].pkg_desc);
		if (list[n].pkg_fmt)	FREE(list[n].pkg_fmt);
		if (list[n].pkg_cat)	FREE(list[n].pkg_cat);
		if (list[n].pkg_set)	FREE(list[n].pkg_set);
		if (list[n].pkg_vers)	FREE(list[n].pkg_vers);
		if (list[n].pkg_arch)	FREE(list[n].pkg_arch);
		if (list[n].pkg_vend)	FREE(list[n].pkg_vend);
		if (list[n].pkg_date)	FREE(list[n].pkg_date);
		if (list[n].pkg_size)	FREE(list[n].pkg_size);
		if (list[n].pkg_help)	FREE(list[n].pkg_help);
		if (list[n].pkg_class)	FREE(list[n].pkg_class);
		list[n].pkg_name = NULL;
		list[n].pkg_desc = NULL;
		list[n].pkg_fmt = NULL;
		list[n].pkg_cat = NULL;
		list[n].pkg_set = NULL;
		list[n].pkg_vers = NULL;
		list[n].pkg_arch = NULL;
		list[n].pkg_vend = NULL;
		list[n].pkg_date = NULL;
		list[n].pkg_size = NULL;
		list[n].pkg_help = NULL;
		list[n].pkg_class = NULL;
		list[n].pkg_opflag = '\0';
		list[n].pkg_info = False;
	}
	*count = 0;
}

/*
 * Allocate more room for the list if needed and init the nth list item
 * to NULL's.
 */

PkgPtr
InitPkg (ListRec *lp, char *name)
{
	PkgPtr	p;
	int	i;

	/*
	 * Reuse any entry that already has the same name (if possible).
	 */
	for (p=lp->pkg; p<lp->pkg+lp->count; p++) {
		if (strcmp (p->pkg_name, name) == 0) {
			i = 1;
			FreePkgList (p, &i);
			p->pkg_reused = True;
			return p;
		}
	}
	/*
	 * Otherwise, alloc a new entry.
	 */
	if (lp->count == lp->max) {
		lp->max += QUANTUM;
		lp->pkg = (PkgPtr)REALLOC (
			lp->pkg, (1 + lp->max)*sizeof(PkgRec)
		);
	}
	p = lp->pkg + lp->count;
	memset(p, 0, sizeof(PkgRec));
	lp->count += 1;
	return p;
}

/*
 * Given a line that looks something like:
 *
 *       base	12	y	system	Base System
 *
 * construct a package record.
 */
PkgPtr
AddInstalledPackage (ListRec *lp, char *str)
{
	char *	ptr;
	char *	desc;
	PkgPtr	p;

	ptr = strtok (str, "\t");	/* base */
	p = InitPkg (lp, str);
	ptr = strtok (NULL, "\t");	/* 12 */
	ptr = strtok (NULL, "\t");	/* y */
	ptr = strtok (NULL, "\t");	/* system */
	desc = strtok (NULL, "\t");	/* Base System */
	p->pkg_cat = STRDUP (ptr);
	p->pkg_name = STRDUP (str);
	p->pkg_desc = STRDUP(desc);
	p->pkg_fmt = STRDUP("4.0");

	return p;
}

/*
 * Given a line that looks something like:
 *
 *   2  graphics     Graphical Applications Set
 *
 * construct a package record.
 */
PkgPtr
AddUninstalledPackage (ListRec *lp, char *str, Boolean set)
{
	char	*ptr;
	PkgPtr	p;

	ptr = strchr(str,' ');
	*ptr = '\0';
	p = InitPkg (lp, str);
	if (set == True) {
		p->pkg_cat = STRDUP(SET);
	}
	p->pkg_name = STRDUP(str);
	while (isspace(*++ptr))
		;
	p->pkg_desc = STRDUP(ptr);
	p->pkg_fmt = STRDUP("4.0");

	return p;
}

char *
PkgLabel(p)
PkgPtr	p;
{
	static char	plabel[80];

	plabel[0] = '\0';
	if (p->pkg_name != NULL) {
		strcat (plabel, p->pkg_name);
		strcat (plabel, ": ");
	}
	if (p->pkg_desc != NULL) {
		strcat (plabel, p->pkg_desc);
	}
	return plabel;
}

/*
 * Given a line that looks something like:
 *
 *   graphics    desktop        Desktop Manager
 *
 * construct a package record.
 */
PkgPtr
AddPackageFromServer (ListRec *lp, char *str, Boolean set)
{
	char	*ptr, *ptr2;
	PkgPtr	p;

	ptr = strchr(str,' ');
	while (isspace(*++ptr))
		;
	ptr2 = strchr(ptr,' ');
	*ptr2 = '\0';
	p = InitPkg (lp, ptr);
	if (set == True)
		p->pkg_cat = STRDUP(SET);
	p->pkg_name = STRDUP(ptr);
	while (isspace(*++ptr2))
		;
	p->pkg_desc = STRDUP(ptr2);
	p->pkg_fmt = STRDUP("4.0");

	return p;
}



