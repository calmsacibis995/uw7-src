#ifndef NOIDENT
#pragma ident	"@(#)custom.c	15.1"
#endif

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include "packager.h"
#include <Gizmo/BaseWGizmo.h>

static void
Finished(char **desc, Boolean *init, PackageRecord *pr)
{
	ListRec *	lp = pr->mediaList;

	*desc = NULL;
	*init = TRUE;
	fclose (lp->cmdfp[1]);
	lp->cmdfp[1] = NULL;
	lp->timeout = (XtIntervalId)0;
	BringDownPopup(pr->catalog->shell);
	CreateSetIcons (lp, NULL);
}

/*
 *	Read the medium until the desciption is found; create a package icon
 *	for the set (product) on the medium.
 *
 *	ReadCustom scans through the initial segment of the TAR file for
 *	a custom medium, that is, through the perms file for the product,
 *	and sets the (single) pr->mediaList package structure.  The relevant
 *	fields of the perms file are prd=<name> for the name of the product
 *	-- which becomes the icon name, equivalent to the "short" name of a
 *	pkgadd package -- and set=<desc> for the longer name or description.
 *	In the event of multiple custom packages on a server, ReadCustom
 *	or something like it should be invoked in a loop stepping
 *	pr->mediaList->count.
 */
void
ReadCustom(PackageRecord *pr, XtIntervalId tid)
{
	int		n;
	char *		ptr;
	char *		ptr2;
	static Boolean	init = TRUE;
	static char *	desc = NULL;
	ListRec *	lp = pr->mediaList;
	PkgPtr		p;

	if (lp->cmdfp[1] == NULL) {	/* canceled -- restore sanity */
		*buf = '\0';
		Finished (&desc, &init, pr);
		return;
	}
	n = read(fileno(lp->cmdfp[1]), buf, BUFSIZ);
	switch (n) {
	case -1:/*
		 *	no current input; keep going
		 */
		break;
	case 0:	/*
		 *	end of file; we should NOT get this far!
		 */
		Finished (&desc, &init, pr);
		return;
	default:/*
		 *	examine each (whole) line
		 */
		if (n < BUFSIZ)
			buf[n] = '\0';
		if (init) {
			ptr = strstr(buf,"/prd=");
			/*
			 *	note: this must exist or the disk would not
			 *	be recognized as Custom format.
			 */
			ptr2 = ptr+5;	/* start of product name */
			if (ptr = strchr(ptr2,'/'))
				*ptr = '\0';
			p = InitPkg(pr->mediaList, ptr2);
			p->pkg_cat = STRDUP(SET);
			p->pkg_fmt = STRDUP(CUSTM);
			p->pkg_name = STRDUP(ptr2);
			init = FALSE;
		}
		else if (desc) {
		/*
		 *	rest of the description was truncated; finish it off
		 */
			ptr = strchr(buf,'\n');
			*ptr = '\0';
			desc = (char *)REALLOC(desc,strlen(desc)+strlen(buf)+1);
			strcat(desc, buf);
			lp->pkg[lp->count-1].pkg_desc = desc;
			Finished (&desc, &init, pr);
			return;
		}
		else {
		/*
		 *	description follows "#set="
		 */
			if (ptr=strstr(buf,"#set=")) {
				desc = ptr+5;
				if (*desc == '"')
					desc++;
				if (ptr2=strpbrk(desc,"\"\n")) {
					*ptr2 = '\0';
					lp->pkg[lp->count-1].pkg_desc =
								STRDUP(desc);
					Finished (&desc, &init, pr);
					return;
				}
				else
					desc = STRDUP(desc);
					/*
					 *	and continue for one more read
					 */
			}
			/* keep on reading */
		}
	}
	pr->mediaList->timeout = XtAddTimeOut (
		150, (XtTimerCallbackProc)ReadCustom, pr
	);
	return;
}


/*
 *	Invoke custom for deletion of all or part of one set/product.
 *
 *	note: CallCustomDel should deal with queueing multiple requests.
 *	Multiple independent invocations of custom will "work" -- but
 *	leave the post-custom cleanup (CheckDelete) confused (this can
 *	also happen if a custom and a pkgrm are spun off by deleteCB.)
 */
void
CallCustomDel(PackageRecord *pr, char *prdname, char *pkglist, uid_t our_uid)
{
	char *		str;
	ListRec *	lp = pr->sysList;

	str = (char *)MALLOC(128+2*strlen(prdname)+2*strlen(pkglist));
	if (str == NULL) {
		FooterMsg1(GGT(string_badCustom));
		return;
	}
	sprintf(str, "%s\"%s %s (%s)\" -e %s/dtexec -ZN %s -s %s -r \"%s\"",
			XTERM,GGT(string_remTitle),prdname,pkglist+1,
			GetXWINHome("adm"),CUSTOM,prdname,pkglist+1);
	setuid(0);
	if (P3OPEN(str, lp->cmdfp, FALSE) != -1) {
		FooterMsg1(GGT(string_invokeCustom));
		lp->timeout = XtAddTimeOut (
			1000, (XtTimerCallbackProc)WaitRemovePkg,
			strcmp(pkglist," ALL")==0? prdname: STRDUP(pkglist)
		);
	}
	else {
		setuid(our_uid);
		FooterMsg1(GGT(string_badCustom));
	}
	FREE(str);
}

/*
 * Open the file /etc/perms/<pkg_name> and parse the file for packages within
 * the pkg_name package.  For each line that is of the form:
 *
 *         #!<thing><white space>
 *         
 *         where <thing> is not "ALL"
 * 
 * create a package that is a subset of pkg_name.  Mark this package as part
 * of a package by setting the pkg_set member to the parent package's name.
 */

void
ExpandCustomSet (ListRec *lp, PkgPtr p)
{
	PkgPtr	q;
	FILE	*fp;
	Boolean	show_it = FALSE;
	char	*ptr, *ptr2;

	sprintf(buf, "%s/%s", PERMS, p->pkg_name);
	if (fp=fopen(buf, "r")) {
		while (fgets(buf, BUFSIZ, fp)) {
			ptr2 = buf+strlen(buf)-1;
			*ptr2 = '\0';
			/*
			 * Look for line containing #set="<name>" and store
			 * <name> as the description of the package.
			 */
			if (strncmp(buf, "#set=", 5)==0) {
				ptr = buf+5;
				if (*ptr == '"')
					ptr++;
				if (*--ptr2 == '"')
					*ptr2 = '\0';
				p->pkg_desc = STRDUP(ptr);
			}
			/*
			 * Look for line containing "#!" that isn't
			 * followed by "ALL" and white space.
			 */
			else if (strncmp(buf,"#!",2)==0) {
				if (strncmp(buf+2,"ALL",3) == 0
				&&  isspace(buf[5]))
					continue;
				/*
			 	 * It's a package in the custom set
			 	 */
				for (ptr=buf+2; !isspace(*ptr); ++ptr)
					;
				*ptr = '\0';
				q = InitPkg (lp, buf+2);
				q->pkg_fmt = STRDUP(CUSTM);
				q->pkg_cat = STRDUP(APPL);
				q->pkg_set = STRDUP(p->pkg_name);
				q->pkg_name = STRDUP(buf+2);
				while (isspace(*++ptr))
					;
				for (ptr2 = ptr; isdigit(*ptr2); ptr2++)
					;
				if (ptr2 > ptr) {
					*ptr2++ = '\0';
					PkgSize(ptr, p, q);
				}
				while (isspace(*ptr2))
					ptr2++;
				if (*ptr2)
					q->pkg_desc = STRDUP(ptr2);
				if (!PkgInstalled(q)) {
					q->pkg_opflag = 'D';
					q->pkg_reused = False;
				}
			}
		}
		fclose(fp);
		/*
		 * Look thru the list of packages and look for all packages
		 * that belong to the set p.  If all of these are marked as
		 * deleted then mark this set as deleted.
		 */
		for (q=lp->pkg+lp->count-1;strcmp(q->pkg_name,p->pkg_name);q--) {
			if (q->pkg_set && strcmp(q->pkg_set,p->pkg_name)==0) {
				if (q->pkg_opflag != 'D') {
					show_it = TRUE;
					break;
				}
			}
		}
		if (!show_it) {
			p->pkg_opflag = 'D';	/* suppress it */
			p->pkg_reused = False;
		}
	}
}
