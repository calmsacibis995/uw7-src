#ifndef NOIDENT
#pragma ident	"@(#)register.c	15.1"
#endif

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include "packager.h"
#include <Gizmo/BaseWGizmo.h>

extern void		DTMInstall();
extern Atom		class_atom, help_atom;

static char *
ValidLocale(char *name, char *type)
{
	static char	pathnm[PATH_MAX];
	char *		ptr;

	if (*name == '/') {
		strcpy(pathnm, name);
		if (access(name, R_OK) != 0) {
			return NULL;
		}
		else {
			return pathnm;
		}
	}
	if (ptr = strchr(name,':')) {	/* locale is specified */
		strncpy(pathnm, name, ptr-name);
		pathnm[ptr-name] = '\0';
	}
	else {
		strcpy(pathnm, name);
	}
	ptr = XtResolvePathname(theDisplay,type,pathnm,NULL,NULL,NULL,0 ,NULL);
	return ptr? pathnm: NULL;
}

/*
 * Add or delete class files from the user's .dtfclass file.
 */

void
RegisterClass(char *class_lines, Boolean flag)
{
	char *		ptr;
	char *		str;
	char *		fname;
	DtRequest *	request;

	str = STRDUP(class_lines);
	for (ptr=strtok(str, " "); ptr; ptr=strtok(NULL," ")) {
		if ((fname = ValidLocale(ptr, "classdb")) || !flag) {
		/*
		 *	Kai points out that I should post multiple requests
		 *	on different queues or the replies will overwrite.
		 *	As we do not really expect multiple CLASS files in
		 *	most packages (only our desktop package has them)
		 *	this can be deferred for the time being.)
		 */
		        if (fname == NULL)
			        fname = ptr;

			request = (DtRequest *)CALLOC(1,sizeof(DtRequest));
			request->create_fclass.rqtype = flag?
						DT_CREATE_FILE_CLASS:
						DT_DELETE_FILE_CLASS;
			request->create_fclass.file_name = fname; 
			DtEnqueueRequest(theScreen,_DT_QUEUE(theDisplay),
		 	   class_atom=OlDnDAllocTransientAtom(pr->base->shell),
			   XtWindow(pr->base->shell), request);
		}
		else {
			sprintf(buf, GGT(string_badClass), str);
			FooterMsg2(buf);
			FREE(str);
			return;
		}
	}
	FREE(str);
}

/*
 * Add or delete help files from the HelpDesk.
 */

void
RegisterHelp(char *help_lines, Boolean flag)
{
	char		*str, *ptr, *ptr2, *fname;
	DtRequest	*request;

	ptr = str = STRDUP(help_lines);
	while (ptr) {
		char	*next = strchr(ptr,'\n');
		ptr2 = strtok(ptr,"\t");
		ptr = (next && next[1] != '\0')? next+1: NULL;
		if ((fname = ValidLocale(ptr2, "help")) || !flag) {
			if (fname == NULL)
			        fname = ptr2;
			request = (DtRequest *)CALLOC(1,sizeof(DtRequest));
			if (flag) {
				request->add_to_helpdesk.rqtype =
							DT_ADD_TO_HELPDESK;
				request->add_to_helpdesk.help_file = fname; 
			}
			else {
				request->del_from_helpdesk.rqtype =
							DT_DEL_FROM_HELPDESK;
				request->del_from_helpdesk.help_file = fname; 
			}
		}
		else {
			sprintf(buf, GGT(string_badHelp), ptr2);
			FooterMsg2(buf);
			FREE(str);
			return;
		}
		ptr2 = strtok(NULL,"\t");
		if (flag)
			request->add_to_helpdesk.icon_file = ptr2;
		ptr2 = strtok(NULL,"\t\n");
		if (flag)
			request->add_to_helpdesk.icon_label = ptr2;
		ptr2 = strtok(NULL,"\t\n");
		if (flag)
			request->add_to_helpdesk.app_name = ptr2;
		else
			request->del_from_helpdesk.app_name = ptr2;
		DtEnqueueRequest (
			theScreen, _HELP_QUEUE(theDisplay),
			help_atom = OlDnDAllocTransientAtom(pr->base->shell),
			XtWindow(pr->base->shell),
			request
		);
	}
	FREE(str);
}

void
RegisterPkg(PackageRecord *pr, char *pkgname, Boolean flag)
{
	PkgPtr		p;
	ListRec *	lp = pr->sysList;

	for (p = lp->pkg; p < lp->pkg+lp->count; p++)
		if (strcmp(p->pkg_name, pkgname)==0) {
			if (p->pkg_help)
				RegisterHelp(p->pkg_help, flag);
			if (p->pkg_class)
				RegisterClass(p->pkg_class, flag);
			return;
		}
}

/*
 * Handle the help and class file registration.
 */

void
RegisterAll(PkgPtr p)
{

	if (p->pkg_help)
		RegisterHelp(p->pkg_help, TRUE);
	if (p->pkg_class)
		RegisterClass(p->pkg_class, TRUE);
}
