#ifndef NOIDENT
#pragma ident	"@(#)delete.c	15.1"
#endif

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "packager.h"

static void
BringDownPopups (PkgPtr p)
{
	if (pr->pkgBoxUp == True) {
		if (strcmp (pr->pkgSetName, p->pkg_name) == 0) {
		/* Bring down the package window */
			BringDownPopup (GetPopupGizmoShell (pr->pkgPopup));
			pr->pkgBoxUp = False;
		}
	}

	if (pr->iconBoxUp == True) {
		if (strcmp (pr->iconSetName, p->pkg_name) == 0) {
		/* Bring down the icon window */
			BringDownPopup (GetPopupGizmoShell (pr->iconPopup));
			pr->iconBoxUp = False;
		}
	}
}

/*
 * Confirm deletion success and mark icons accordingly.
 */

void
CheckDelete (ListRec *lp, char *names)
{
	PkgPtr	p;
	PkgPtr	q;
	PkgPtr	set_ptr = NULL;
	char *	ptr;
	char	list[BUFSIZ];
	Boolean	failed = FALSE;
	Boolean	some_left = FALSE;
	int	n;
	int	index;

	*list = '\0';
	for (index=0; index<lp->count; index++) {
		p = lp->pkg + index;
		if ((ptr=strstr(names, p->pkg_name))== NULL
		|| ptr[n=strlen(p->pkg_name)] != ' ' && ptr[n] != '\0')
			continue;
		/*
		 *	this package (or set) was in the list for deletion
		 */
		if (PkgInstalled(p)) {
			UpdateMediaListIcon (p, True);
			failed = TRUE;
			strcat(strcat(list," "), p->pkg_name);
		}
		else {
			p->pkg_opflag = 'D';
			p->pkg_reused = False;
			RemoveLinks(p);
			UpdateMediaListIcon (p, False);
			BringDownPopups (p);
		}
		/* Is this package actually a set? */
		if (p->pkg_cat == NULL || strcmp (p->pkg_cat,SET) != 0) {
			/* This is NOT a set */
			if (pr->pkgBoxUp == True) {
				some_left = True;
			}
			/* Look for a set containing this package */
			if (p->pkg_set && set_ptr == NULL) {
				for (q=lp->pkg; q<lp->pkg+lp->count; q++) {
					if (strcmp(q->pkg_name,p->pkg_set)==0) {
						/* Indicate set found */
						set_ptr = q;
						break;
					}
				}
			}
			if (failed == False) {
				if (p->pkg_help)
					RegisterHelp(p->pkg_help, FALSE);
				if (p->pkg_class)
					RegisterClass(p->pkg_class, FALSE);
			}
		}
		else {
			/* This is a set */
			set_ptr = p;
			/* Look for any packages in this set and check to
			 * see if they were deleted. */
			for (q=lp->pkg; q<lp->pkg+lp->count; q++) {
				if (q->pkg_set
				&&  strcmp(q->pkg_set,p->pkg_name)==0
				&&  q->pkg_opflag != 'D') {
					if (PkgInstalled(q))
						some_left = TRUE;
					else {
						p->pkg_reused = False;
						q->pkg_opflag = 'D';
						if (q->pkg_help)
							RegisterHelp
							    (q->pkg_help,FALSE);
						if (q->pkg_class)
							RegisterClass
							    (q->pkg_class,FALSE);
					}
				}
			}
		}
	}
	CreateSetIcons (pr->sysList, NULL);
	if (failed) {
		sprintf(buf, GGT(format_delbad), list);
	}
	else {
		sprintf (buf, GGT(format_delgood), names);
	}
	if (pr->delete == PKG_DELETE) {
		if (failed == True || some_left == True) {
			SetPopupMessage (pr->pkgPopup, buf);
		}
		else {
			set_ptr->pkg_opflag = 'D';
			set_ptr->pkg_reused = False;
			CancelPkgCB (NULL, NULL, NULL);
		}
	}
	else {
		FooterMsg2(buf);
	}
}
