#ifndef NOIDENT
#pragma ident	"@(#)add.c	15.1"
#endif

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "packager.h"

int	folder_end;
int	folder_start;
Boolean set_install;

/*
 * Add a package to the passed in list.  Lookup the packages
 * description and add it to the entry.  Also mark the package
 * as a set if appropriate.
 */

PkgPtr
AddPkgDesc (ListRec *lp, char *name)
{
	PkgPtr	p;
	FILE *	fp;
	char	line[BUFSIZ];
	char *	cp;
	char *  as_ptr = NULL;

	/* If this directory doesn't contain a "pkginfo" */
	/* file then it isn't a package. */
	sprintf(buf, "%s/%s/pkginfo", PKGDIR, name);
	if (access (buf, R_OK) != 0 || (fp = fopen (buf, "r")) == NULL) {
		return NULL;
	}
	p = InitPkg (lp, name);

	while (fgets (line, BUFSIZ, fp) != NULL) {
		if (strncmp (line, "NAME=", 5) == 0) {
			for (cp=line+5; *cp==' '; cp++);
			p->pkg_desc = STRDUP (cp);
			if (strncmp (cp, "\"BUILT IN", 9) == 0) {
				/*
				 * To prevent this package from ever being
				 * displayed, pkg_set is set to "builtin".
				 * This is because the set "builtin"
				 * doesn't exist and can never be opened
				 * by the user.
				 */
				p->pkg_set = STRDUP("builtin");
			}
		}
		if (strncmp(line, "CATEGORY=", 9) == 0)  {
		        if ((as_ptr = strstr(line, "as")) == NULL ||
			       (strlen(as_ptr) != 2)) {
			       p->pkg_cat = STRDUP(line+9);
			       p->pkg_as = NULL;
			}
			else {
			       as_ptr = 0; as_ptr++;
			       p->pkg_as = STRDUP(as_ptr);
			}
		}
		if (strncmp(line, "VERSION=", 8) == 0) 
                        p->pkg_vers = STRDUP(line+8);
		if (strncmp(line, "ARCH=", 5) == 0) 
                        p->pkg_arch = STRDUP(line+5);
		if (strncmp(line, "VENDOR=", 7) == 0) 
                        p->pkg_vend = STRDUP(line+7);
		if (strncmp(line, "INSTDATE=", 9) == 0) 
                        p->pkg_date = STRDUP(line+9);
	}
	fclose (fp);

	p->pkg_name = STRDUP (name);
	p->pkg_fmt  = STRDUP ("4.0");
	/* If a "setinfo" file exists in the package's */
	/* directory, mark this package as a set. */
	sprintf (buf, "%s/%s/setinfo", PKGDIR, name);
	if (access (buf, R_OK) == 0) {
		p->pkg_cat  = STRDUP(SET);
	}

	return p;
}

/*
 * Update the primary scrolling window that contains the sets and packages
 */
static void
UpdateSet (PkgPtr newp)
{
	ListRec *	lp = pr->sysList;
	DmObjectPtr	op;
	icon_obj *	iobj;
	int		count = lp->setCount;

	if (newp->pkg_reused == True) {
		/* This set is already in the list so just return. */
		return;
	}
	op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
	if (op == NULL) {
		return;
	}
	op->name = STRDUP (newp->pkg_name);
	op->objectdata = newp;
	op->fcp = &set_fcrec;
	op->x = lp->setx;
	op->y = lp->sety;
	if ((int)(lp->setx += 4*INC_X/3) > WIDTH - MARGIN) {
		lp->setx = INIT_X;
		lp->sety += INC_Y;
	}
	lp->setCount += 1;
	Dm__AddObjToIcontainer (
		lp->setBox,
		&lp->setitp,
		(Cardinal *)&count,
		&lp->set_cntrec,
		op,
		op->x,
		op->y,
		DM_B_CALC_SIZE,
		0, def_font, (Dimension)WIDTH, INC_X, INC_Y
	);
}

/*
 * Update the packages scrolling window that contains the installed
 * packages.
 */
static void
UpdatePkg (PkgPtr newp)
{
	PkgPtr		p;
	DmObjectPtr	op;
	icon_obj *	iobj;

	/* Only add to list if it wasn't previously in the list. */
	if (newp->pkg_reused == True) {
		return;
	}
	/* Only update the package icons if the window is up */
	if (pr->pkgPopup != NULL && XtIsRealized (pr->pkgPopup->shell)) {
		if ((p = GetSelectedSet (pr->sysList)) != (PkgPtr)0) {
			op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
			if (op == NULL) {
				return;
			}
			op->name = STRDUP (newp->pkg_name);
			op->objectdata = newp;
			op->fcp = &pkg_fcrec;
			op->x = pr->pkgx;
			op->y = pr->pkgy;
			if ((int)(pr->pkgx += 4*INC_X/3) > WIDTH - MARGIN) {
				pr->pkgx = INIT_X;
				pr->pkgy += INC_Y;
			}
			pr->pkgCount += 1;
			Dm__AddObjToIcontainer (
				pr->pkgBox,
				&pr->pkgitp,
				(Cardinal *)&pr->pkgCount,
				&pkg_cntrec,
				op,
				op->x,
				op->y,
				DM_B_CALC_SIZE,
				0, def_font, (Dimension)WIDTH, INC_X, INC_Y
			);
		}
	}
}

/*
 * If the package is already installed, put up a special
 * icon so the user knows that the package is already on their
 * system.  Otherwise, put up the normal icon.
 */
void
UpdateMediaListIcon (PkgPtr p, Boolean installed)
{
	DmObjectPtr	op;
	Boolean		changed = False;
	int		i;
	char *		label;
	ListRec *	lp = pr->mediaList;

	/* Look for this package in the set icon container*/
	for (i=0; i<lp->setCount; i++) {
		label = (char *)lp->setitp[i].label;
		if (strcmp (p->pkg_name, label) == 0) {
			op = (DmObjectPtr)lp->setitp[i].object_ptr;
			break;
		}
	}
	if (i == lp->setCount) {
		/* Didn't find the name in the list.  The user */
		/* must have switched to another device. */
		return;
	}
	if (installed == False) {
		if (op->fcp == &set_fcrec || op->fcp == &pkg_fcrec) {
			op->fcp = (
				STRCMP_CHK_ZERO(p->pkg_cat, SET) ?
				&unset_fcrec :
				&unpkg_fcrec
			);
			changed = True;
		}
	}
	else {
		if (op->fcp == &unset_fcrec || op->fcp == &unpkg_fcrec) {
			op->fcp = (
				STRCMP_CHK_ZERO(p->pkg_cat, SET) ?
				&set_fcrec :
				&pkg_fcrec
			);
			changed = True;
		}
	}
	if (changed == True) {
		XtVaSetValues (
			lp->setBox, XtNitemsTouched, True, (String)0
		);
	}
}

void
CreateNewIcons (ListRec *lp, XtIntervalId tid)
{
        CreateSetIcons(lp, NULL);
}

/*
 * Confirm installation success.
 */
void
CheckAdd (char *names)
{
	PkgPtr		p;
	char *		ptr;
	char		list[BUFSIZ];
	Boolean		failed = False;
	PkgPtr		newPkg = NULL;	/* Indicates a new package added */
	int		n;
	int		index;
	ListRec *	lp = pr->mediaList;

	*list = '\0';
	for (index=0; index<lp->count; index++) {
	        p = lp->pkg + index;
		if ((ptr=strstr(names, p->pkg_name))== NULL
		|| ptr[n=strlen(p->pkg_name)] != ' ' && ptr[n] != '\0') {
			p->pkg_opflag = '\0';
			continue;
		}
		if (!PkgInstalled(p)) {
			/* Display different icon */
			UpdateMediaListIcon (p, False);
			failed = TRUE;
			p->pkg_opflag = 'F';
			strcat(strcat(list," "), p->pkg_name);
		}
		else {
			FILE *	fp;
			PkgPtr	q;

			/* Display different icon */
			UpdateMediaListIcon (p, True);
			if ((newPkg=AddPkgDesc(pr->sysList,p->pkg_name))==NULL)
			        continue;
			p->pkg_opflag = 'T';
			newPkg->pkg_opflag = 'T';
			sprintf(buf, "%s/%s/setinfo", PKGDIR, newPkg->pkg_name);
			if ((fp = fopen(buf, "r")) == NULL) {
				SetPkgDefs(newPkg);
				RegisterAll(newPkg);
				/* Find out what set this pkg belongs to */
				sprintf (
					buf, "/bin/grep -l %s %s/*/setinfo",
					newPkg->pkg_name, PKGDIR
				);
				if (P3OPEN (buf, lp->cmdfp, FALSE) != -1) {
					if (fcntl(fileno(lp->cmdfp[1]),F_SETFL, 0)==-1) {
						FPRINTF ( (stderr, "Can't do fcntl\n"));
					}
					if (fgets (buf, BUFSIZ, lp->cmdfp[1]) != NULL) {
						ptr = buf+strlen (PKGDIR)+1;
						*(strchr (ptr, '/')) = '\0';
						newPkg->pkg_set = STRDUP(ptr);
					}
					_Dtam_p3close(lp->cmdfp, 0);
				}
			}
			else {
				UpdateSet (newPkg);
				set_install = True;
				while (fgets(buf, BUFSIZ, fp)) {
					if (*buf && *buf != '#' && *buf != '\n') {
						q = AddInstalledPackage (
							pr->sysList, buf
						);
						UpdatePkg (q);
						q->pkg_set = STRDUP (
							newPkg->pkg_name
						);
						SetPkgDefs(q);
						RegisterAll(q);
						q->pkg_opflag = 'T';
					}
				}
				fclose(fp);
			}
		}
	}
	lp->timeout = XtAddTimeOut(0, (XtTimerCallbackProc) CreateNewIcons,
				   pr->sysList);
	folder_end = set_install ? pr->sysList->count : pr->mediaList->count;
	folder_start = 0;
	PopupInstalledFolders(set_install ? pr->sysList : pr->mediaList);
        if (failed)
		sprintf(buf, GGT(format_badFmt), list);
	else {
		sprintf (buf,GGT(format_goodFmt), names);
	}
	FooterMsg1(buf);

}

