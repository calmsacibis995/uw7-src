#ifndef NOIDENT
#pragma ident	"@(#)lists.c	15.1"
#endif

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "packager.h"
#include <Xt/Shell.h>
#include <Xol/ScrolledWi.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/STextGizmo.h>

static void	PkgContentsCB (BLAH_BLAH_BLAH);
static void	PkgInfoCB (BLAH_BLAH_BLAH);
static void	DeletePkgCB (BLAH_BLAH_BLAH);
extern void	FindFuture(PkgPtr, int);

MenuItems	pkg_menu_item[] = {  
	{TRUE, label_show,	mnemonic_show,	0, PkgContentsCB},
	{TRUE, label_info,	mnemonic_info,	0, PkgInfoCB},
	{TRUE, label_delete,	mnemonic_delete,0, DeletePkgCB,  NULL},
	{TRUE, label_cancel,	mnemonic_cancel,0, CancelPkgCB,  NULL},
	{TRUE, label_help,	mnemonic_help,	0, helpCB, (char*)&HelpPkgwin},
	{NULL}
};
MenuGizmo	pkg_menu = {0, "pkgMenu", NULL, pkg_menu_item };
PopupGizmo	pkg_popup = {0,"popup",string_newpkgTitle,(Gizmo)&pkg_menu };

/*
 * Get the correct pixmap for this package.
 * If this is the media list then the system list needs to be
 * checked for this package.  If the package is already in the
 * system list then this package needs a special icon to show that
 * it is already installed.
 */
static void
GetPixmap (ListRec *lp, PkgPtr p, DmObjectPtr optr)
{
	PkgPtr		q;
	Boolean		uninstalled = True;
	ListRec *	sl = pr->sysList;

	if (lp == pr->mediaList) {
		for (q=sl->pkg; q<sl->pkg + sl->count; q++) {
			if (strcmp(p->pkg_name, q->pkg_name) == 0) {
				if (q->pkg_opflag != 'D') {
					uninstalled = False;
				}
				break;
			}
		}
	}
	if (uninstalled == False || lp != pr->mediaList) {
		optr->fcp = (
			STRCMP_CHK_ZERO(p->pkg_cat, SET) ?
			&set_fcrec :
			&pkg_fcrec
		);
	}
	else {
		optr->fcp = (
			STRCMP_CHK_ZERO(p->pkg_cat, SET) ?
			&unset_fcrec :
			&unpkg_fcrec
		);

	}
}

/*
 * Add to the icons that will be displayed.
 */

static void
AddItem (
	ListRec *lp, PkgPtr p, DmContainerRec *cntrec_ptr,
	int *count, Dimension *x, Dimension *y, Boolean no_sub
)
{
	DmObjectPtr	optr;
	ViewTypes	filter;

	if (p->pkg_opflag == 'D')
		return;
	
	/* Packages in sets should be shown in a separate popup icon window.*/
	if (no_sub && p->pkg_set != NULL) 
		return;		  

	filter = (lp == pr->sysList) ? lp->viewType : ALL_VIEW ;

	switch (filter) {
		/*
		 *	filter out inappropriate icons to a view - System
		 * 	view should be as (or pe) packages only.
		 * 	With Eiger, the "UnixWare" set replaces both
		 * 	the "as" and "pe" sets.
		 */
		case SYS_VIEW: {
     			if (p->pkg_fmt[0] != '4')
				return;
			if (STRCMP_CHK_ZERO(p->pkg_cat,SET)) {
			        if (STRCMP_CHK_ZERO(p->pkg_name, UNIXWARE) ||
			            STRCMP_CHK_ZERO(p->pkg_name, AS) ||
			            STRCMP_CHK_ZERO(p->pkg_name, PE))
				    break;
			}
			else {
			        if (STRCMP_CHK_ZERO(p->pkg_set, UNIXWARE) ||
			            STRCMP_CHK_ZERO(p->pkg_set, AS) ||
			            STRCMP_CHK_ZERO(p->pkg_set, PE))
				    break;
			}
			return;
		}
		case APP_VIEW: {
			if (p->pkg_fmt[0] != '4')
				break;

			if (STRCMP_CHK_ZERO(p->pkg_cat,SET)) {
			        if (STRCMP_CHK_ZERO(p->pkg_name, UNIXWARE) ||
			            STRCMP_CHK_ZERO(p->pkg_name, AS) ||
			            STRCMP_CHK_ZERO(p->pkg_name, PE))
				return;
			}
			break;
		}
		case ALL_VIEW:	
		default: {
			break;
		}
	}
	optr = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
	optr->container = cntrec_ptr;
	GetPixmap (lp, p, optr);
	optr->name = p->pkg_name;
	optr->x = *x;
	optr->y = *y;
	optr->objectdata = (XtPointer)p;
	if ((int)(*x += INC_X) > 9*WIDTH/10 - MARGIN) {
		*x = INIT_X;
		*y += INC_Y;
	}
	if ((*count)++ == 0) {
		cntrec_ptr->op = optr;
	}
	else {
		DmObjectPtr endp = cntrec_ptr->op;
		while (endp->next)
			endp = endp->next;
		endp->next = optr;
	}
	cntrec_ptr->num_objs = *count;
}

/*
 * Set the title appearing above the sw with the system list of packagers
 */
void
SetSysListTitle (PackageRecord *pr, char *str)
{
	StaticTextGizmo *	g;

	sprintf (buf, GGT (format_apps_installed), str, our_node);
	SetStaticTextGizmoText (pr->sysList->title, buf);
}

/*
 * Set the title appearing above the sw with the media list of packagers
 */
void
SetMediaListTitle (PackageRecord *pr)
{
	ChoiceGizmo *	g;
	char *		where;

	if (pr->curalias == spooled) {
	        if (pr->spooldir && *pr->spooldir) 
		        where = pr->spooldir;
	        else
		        where = GGT(label_spooldir);
	}
	else if (pr->curalias == network) {
                if (pr->servername && *pr->servername)
      		        where = pr->servername;
	        else
		        where = GGT(label_network);
	}
        else {
		where = pr->mediaList->curlabel;
	}

	sprintf (buf, GGT (format_apps_in), where);
	SetStaticTextGizmoText (pr->mediaList->title, buf);

	/* Set the preview label on the media list */
	g = QueryGizmo (
			BaseWindowGizmoClass, pr->base,
			GetGizmoGizmo, "firstChoice"
	);
	XtVaSetValues (
		       g->previewWidget,
		       XtNstring,      pr->mediaList->curlabel,
		       (String)0
        );
	SetViewMenuLabels (0);

}

void
SetSetSensitivity (ListRec *lp)
{
	int		i;
	Widget		w_menu;
	Boolean		selected = False;
	PkgPtr		p;
	DmObjectPtr	optr;

	for (i=0; i<lp->setCount; i++) {
		optr = (DmObjectPtr)lp->setitp[i].object_ptr;
		p = (PkgPtr)optr->objectdata;
		if (lp->setitp[i].managed && lp->setitp[i].select) {
			selected = True;
			break;
		}
	}
	XtSetArg (arg[0], XtNsensitive, selected!=0);
	if (lp == pr->sysList) {
		w_menu = (Widget)QueryGizmo (
			BaseWindowGizmoClass, pr->base, GetGizmoWidget,
			"removeMenu"
		);
		OlFlatSetValues (w_menu, 0, arg, 1);
		OlFlatSetValues (w_menu, 1, arg, 1);
		if (pr->installInProgress == False && owner) {
			OlFlatSetValues (w_menu, 2, arg, 1);
		}
	}
	else {
	        if (owner && pr->installInProgress == False) {
			w_menu = (Widget)QueryGizmo (
				BaseWindowGizmoClass, pr->base, GetGizmoWidget,
				"addMenu"
			);
			OlFlatSetValues (w_menu, 1, arg, 1);
		}
	}
}

void
SetPkgSensitivity ()
{
	Widget	w_menu;
	int	i;
	PkgPtr	p;

	if (pr->pkgBoxUp == False) {
		return;
	}
	p = GetSelectedPkg ();

	w_menu = (Widget)QueryGizmo (
		PopupGizmoClass, pr->pkgPopup, GetGizmoWidget,
		"pkgMenu"
	);
	XtSetArg(arg[0], XtNsensitive, p!=(PkgPtr)0);
	for (i=0; i<2; i++) {
		OlFlatSetValues(w_menu, i, arg, 1);
	}
	if (owner)
		OlFlatSetValues(w_menu, i, arg, 1);
}

/*
 * SelectCB from set icon box.
 */
static void
SelectSetCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		op;
	PkgPtr			pp;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	ListRec *		lp = (ListRec *)client_data;
	int			item = d->item_index;

	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, item, arg, 1);
	pp = (PkgPtr)op->objectdata;
	if (lp == pr->sysList) {
		FooterMsg2 (PkgLabel(pp));
	}
	else {
		FooterMsg1 (PkgLabel(pp));
	}
	SetSetSensitivity (lp);
}

/*
 * SelectCB from pkg icon box
 */
static void
SelectPkgCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		op;
	PkgPtr			pp;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	ListRec *		lp = (ListRec *)client_data;

	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, d->item_index, arg, 1);
	pp = (PkgPtr)op->objectdata;
	SetPopupMessage(pr->pkgPopup, PkgLabel(pp));
	SetPkgSensitivity ();
}

/*
 * ExecuteCB from set icon box
 */
static void
ExecuteSetCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		op;
	PkgPtr			pp;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	ListRec *		lp = (ListRec *)client_data;

	if (lp == pr->mediaList) {
		return;
	}
	SelectSetCB(w, lp, call_data);
	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, d->item_index, arg, 1);
	pp = (PkgPtr)op->objectdata;
	if (STRCMP_CHK_ZERO(pp->pkg_cat,SET))
		CreatePkgIcons(pp, lp);
	else {
		SetPopupMessage(pr->iconPopup, NULL);
		PopupIconBox (pp);
	}
}

/*
 * ExecuteCB from pkg icon box
 */
static void
ExecutePkgCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		op;
	PkgPtr			pp;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	ListRec *		lp = (ListRec *)client_data;

	SelectPkgCB(w, lp, call_data);
	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, d->item_index, arg, 1);
	pp = (PkgPtr)op->objectdata;
	SetPopupMessage(pr->iconPopup, NULL);
	PopupIconBox (pp);
}

static void
AdjustPkgCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		optr;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	int			item = d->item_index;

	if (pr->pkgitp[item].managed && pr->pkgitp[item].select) {
		optr = (DmObjectPtr)pr->pkgitp[item].object_ptr;
		SetPopupMessage (pr->pkgPopup, PkgLabel(optr->objectdata));
	}
	else {
		SetPopupMessage (pr->pkgPopup, NULL);
	}
	SetPkgSensitivity ();
}

static void
AdjustSetCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		optr;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	ListRec *		lp = (ListRec *)client_data;
	int			item = d->item_index;

	if (lp->setitp[item].managed && lp->setitp[item].select) {
		optr = (DmObjectPtr)lp->setitp[item].object_ptr;
		FooterMsg (lp, PkgLabel(optr->objectdata));
	}
	else {
		FooterMsg (lp, NULL);
	}
	SetSetSensitivity (lp);
}

int
cmpicon(PkgPtr x, PkgPtr y)
{
        return strcoll(x->pkg_name, y->pkg_name);
}

/*
 * Populate the base icon window.
 */
void
CreateSetIcons (ListRec *lp, char *title)
{
	Widget		parent = lp->sw;
	int		n;
	char *		type = title ? title : pr->labelAll;
	ChoiceGizmo *	g;
	int		i = 0;

	if (lp->setBox) {
		XtUnmanageChild(lp->setBox);
		XtDestroyWidget(lp->setBox);
		lp->setBox = (Widget)NULL;
	}
	lp->setCount = 0;
	lp->setx = INIT_X;
	lp->sety = INIT_Y;

 	qsort((void *)lp->pkg, lp->count, sizeof(PkgRec), (int (*)())cmpicon);

	for (n = 0; n < lp->count; n++) {
		AddItem (
			lp, &lp->pkg[n], &lp->set_cntrec,
			&(lp->setCount), &lp->setx, &lp->sety, TRUE
		);
	}

	XtSetArg(arg[i], XtNpostSelectProc,	(XtArgVal)SelectSetCB); i++;
	XtSetArg(arg[i], XtNmovableIcons,	(XtArgVal)FALSE); i++;
	XtSetArg(arg[i], XtNminWidth,		(XtArgVal)1); i++;
	XtSetArg(arg[i], XtNminHeight,		(XtArgVal)1); i++;
	XtSetArg(arg[i], XtNdrawProc,		(XtArgVal)DmDrawIcon); i++;
	XtSetArg(arg[i], XtNdblSelectProc,	(XtArgVal)ExecuteSetCB); i++;
	XtSetArg(arg[i], XtNclientData,		(XtArgVal)lp); i++;
	XtSetArg(arg[i], XtNpostAdjustProc,	(XtArgVal)AdjustSetCB); i++;

	lp->setBox = DmCreateIconContainer (
		(Widget)parent, DM_B_CALC_SIZE,
		arg, i, lp->set_cntrec.op, lp->setCount, &lp->setitp,
		lp->setCount, NULL, NULL, def_font, 1
	);

	if (lp == pr->sysList) {
	        SetSysListTitle (pr, type);
	}
	else {
		SetMediaListTitle (pr);
	}

	/* Select first icons by default */
	XtSetArg(arg[0], XtNset, True);
	OlFlatSetValues(lp->setBox, 0, arg, 1);

	SetSetSensitivity (lp);
}

void
CreatePkgIcons (PkgPtr pp, ListRec *lp)
{
	int		n;
	int		i = 0;
	char *		set_name = pp->pkg_name;
	Widget		w_shell;
	Widget		w_up;
	Widget		w_menu;
	MenuGizmo *     m_menu;		

	if (pr->pkgBox) {
		XtUnmanageChild(pr->pkgBox);
		XtDestroyWidget(pr->pkgBox);
		pr->pkgBox = (Widget)NULL;
	}
	if (!pr->pkgsw) {
		pr->pkgPopup = CopyGizmo (PopupGizmoClass, &pkg_popup);
		w_shell = CreateGizmo (
			pr->base->shell, PopupGizmoClass,
			pr->pkgPopup, NULL, 0
		);
		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(w_shell, arg, 1);
		XtSetArg(arg[0], XtNheight,	(XtArgVal)(4*HEIGHT/5));
		XtSetArg(arg[1], XtNwidth,	(XtArgVal)WIDTH);
		XtSetArg(arg[2], XtNhStepSize,  (XtArgVal)(INC_X/2));
		XtSetArg(arg[3], XtNvStepSize,  (XtArgVal)(INC_Y/2));
		pr->pkgsw = XtCreateManagedWidget (
			"package_window", scrolledWindowWidgetClass,
			w_up, arg, 4
		);
		if (!owner) {
			w_menu = QueryGizmo (
				PopupGizmoClass, pr->pkgPopup,
				GetGizmoWidget, "pkgMenu"
			);
			XtSetArg(arg[0], XtNsensitive, FALSE);
			OlFlatSetValues(w_menu, 2, arg, 1);
		}
	}
	pr->pkgCount = 0;
	pr->pkgx = INIT_X;
	pr->pkgy = INIT_Y;

 	qsort((void *)lp->pkg, lp->count, sizeof(PkgRec), (int (*)())cmpicon);

	for (n=0; n<lp->count; n++) {
		if (lp->pkg[n].pkg_set &&
		    STRCMP_CHK_ZERO(lp->pkg[n].pkg_set, set_name)) {
			AddItem (
				lp, &lp->pkg[n], &pkg_cntrec, &pr->pkgCount,
				&pr->pkgx, &pr->pkgy, FALSE);
		}
	}

	XtSetArg(arg[i], XtNpostSelectProc,	(XtArgVal)SelectPkgCB); i++;
	XtSetArg(arg[i], XtNmovableIcons,	(XtArgVal)FALSE); i++;
	XtSetArg(arg[i], XtNminWidth,		(XtArgVal)1); i++;
	XtSetArg(arg[i], XtNminHeight,		(XtArgVal)1); i++;
	XtSetArg(arg[i], XtNdrawProc,		(XtArgVal)DmDrawIcon); i++;
	XtSetArg(arg[i], XtNdblSelectProc,	(XtArgVal)ExecutePkgCB); i++;
	XtSetArg(arg[i], XtNclientData,		(XtArgVal)lp); i++;
	XtSetArg(arg[i], XtNpostAdjustProc,	(XtArgVal)AdjustPkgCB); i++;

	pr->pkgBox = DmCreateIconContainer (
		pr->pkgsw, DM_B_CALC_SIZE,
		arg, i, pkg_cntrec.op, pr->pkgCount, &pr->pkgitp,
		pr->pkgCount, NULL, NULL, def_font, 1
	);

	/* Select first icons by default */
	XtSetArg(arg[0], XtNset, True);
	OlFlatSetValues(pr->pkgBox, 0, arg, 1);

	SetPopupMessage(pr->pkgPopup, NULL);
	sprintf(buf, GGT(string_newpkgTitle), pp->pkg_desc);
	XtSetArg(arg[0], XtNtitle, buf);
	XtSetValues(pr->pkgPopup->shell, arg, 1);
	MapGizmo (PopupGizmoClass, pr->pkgPopup);
	pr->pkgBoxUp = True;
	pr->pkgSetName = pp->pkg_name;
	SetPkgSensitivity ();

        m_menu = QueryGizmo (PopupGizmoClass, pr->pkgPopup,
			     GetGizmoGizmo, "pkgMenu"
	);
	OlSetInputFocus(m_menu->parent, RevertToNone, CurrentTime);
}

static void
SetCatalogMessage (PopupGizmo *p, char *str)
{
	static StaticTextGizmo *	stg = NULL;

	if (stg == NULL) {
		stg = QueryGizmo (
			PopupGizmoClass, p, GetGizmoGizmo, "msg_text"
		);
		XtVaSetValues (
			stg->widget, XtNalignment, OL_CENTER, (String)0
		);
	}
	SetStaticTextGizmoText (stg, str);
}

void
GetCustomList (ListRec *lp)
{

	char	str[BUFSIZ];
	char *	where;
	char *	devline = DtamGetDev(pr->curalias, FIRST);
	char *	cdev = DtamDevAttr(devline, CDEVICE);

	FPRINTF ((stderr, "GetCustomList\n"));
	lp->validList = False;
	where = (pr->curalias == spooled? pr->spooldir: lp->curlabel);
	if (lp->max == 0) {
		lp->max = QUANTUM;
		lp->pkg = (PkgPtr)MALLOC((lp->max+1)*sizeof(PkgRec));
	}
	if (lp->count) {
		FreePkgList(lp->pkg, &lp->count);
		lp->count = 0;
	}
	/*
	 *	Get the name of the "product" and the "set" description
	 *	from the initial header and perms file of the TAR medium
	 *	This will read from the same fp as if we did a p3open.
	 */
	if (lp->cmdfp[1] = fopen(cdev,"r")) {
		*buf = '\0';
		fcntl(fileno(lp->cmdfp[1]), F_SETFL, O_NONBLOCK);
		lp->timeout = XtAddTimeOut (
			2500, (XtTimerCallbackProc)ReadCustom, pr
		);
		sprintf(str, GGT(format_wait), where);
	}
	else {
		sprintf(str, GGT(format_cantRead), lp->curlabel);
	}
	if (cdev) {
		FREE(cdev);
	}
	if (devline) {
		FREE(devline);
	}
	SetCatalogMessage (pr->catalog, str);
	MapGizmo (PopupGizmoClass, pr->catalog);
	XSync (theDisplay, FALSE);
}

void
GetUninstalledList (ListRec *lp, char *where, char *label)
{
	char	str[BUFSIZ];

	FPRINTF ((stderr, "GetUninstalledList\n"));
	lp->validList = False;
	/* 
	 * The sets_plus command will return all sets and all packages
	 * that are not members of any set.
	 */ 
	sprintf(str, "%s%s/sets_plus -p -d %s 2>&1", LANG_C, 
			GetXWINHome("adm"), where);
	where = (pr->curalias == spooled? pr->spooldir: label);
	if (lp->max == 0) {
		lp->max = QUANTUM;
		lp->pkg = (PkgPtr)MALLOC((lp->max+1)*sizeof(PkgRec));
	}
	if (lp->count) {
		FreePkgList(lp->pkg, &lp->count);
		lp->count = 0;
	}
	if (P3OPEN(str, lp->cmdfp, FALSE) != -1) {
		*(lp->input) = '\0';
		lp->timeout = XtAddTimeOut (
			2500, (XtTimerCallbackProc)ReadUninstalledList, NULL
		);
		sprintf(str, GGT(format_wait), where);
	}
	else
		sprintf(str, GGT(format_cantRead), lp->curlabel);
	SetCatalogMessage (pr->catalog, str);
	MapGizmo (PopupGizmoClass, pr->catalog);
	XSync (theDisplay, FALSE);
}

void
CancelPkgCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(pr->pkgPopup->shell);
	pr->pkgBoxUp = False;
}

static void
AllowDisallow (Boolean sensitive)
{
	Widget	menu;

	pr->installInProgress = !sensitive;
	XtSetArg(arg[0], XtNsensitive, sensitive);
	menu = (Widget)QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget,
		"removeMenu"
	);
	OlFlatSetValues (menu, 2, arg, 1);

	menu = (Widget)QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget,
		"addMenu"
	);
	OlFlatSetValues (menu, 1, arg, 1);

	if (sensitive == True) {
		/* Set buttons back to the way they were */
		SetSetSensitivity (pr->mediaList);
		SetSetSensitivity (pr->sysList);
	}
}

/*
 * Turn off the Install and Remove buttons thus preventing 
 * these functions from being called.
 */
void
DontAllowAddOrDelete ()
{
	AllowDisallow (False);
}

/*
 * Turn on the Install and Remove buttons thus allowing 
 * these functions from being called.
 */
void
AllowAddAndDelete ()
{
	AllowDisallow (True);
}

/*
 * This timeout routine contains the termination processing for pkgrm()
 */

void
WaitRemovePkg (char *names, XtIntervalId intid)
{
	char		str[BUFSIZ];
	ListRec *	lp = pr->sysList;
	int		n;

	if (lp->cmdfp[1] == NULL || (n=read(fileno(lp->cmdfp[1]), str, BUFSIZ)) == 0) {
		setuid(our_uid);
		SetBusy(FALSE);
		lp->timeout = (XtIntervalId)NULL;	/* no more calls! */
		if (lp->cmdfp[1]) {
			_Dtam_p3close(lp->cmdfp, 0);
			lp->cmdfp[0] = lp->cmdfp[1] = (FILE *)0;
		}
		CheckDelete (lp, names);
		SetSetSensitivity (lp);
		FREE(names);
		AllowAddAndDelete ();
	}
	else {
		lp->timeout = XtAddTimeOut (
			1000, (XtTimerCallbackProc)WaitRemovePkg, names
		);
	}
}

/*
 * Handle deletes that involve packages in sets.
 */

static void
DeletePkgCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	int		n;
	char *		ptr;
	char *		names = NULL;
	Boolean		chosen;
	Boolean		cust_type;
	Boolean		delete_all = TRUE;
	DmObjectPtr	optr;
	ListRec *	lp = pr->sysList;
	PkgPtr		p_sel = GetSelectedPkg ();
	int 		del_num = 0;

	if (!p_sel) {
		SetPopupMessage(pr->pkgPopup, GGT(string_noSelect));
		return;
	}
	pr->delete = PKG_DELETE;
	cust_type = (p_sel->pkg_fmt[0] == 'C');
	for (n = 0; n < pr->pkgCount; n++) {
		XtSetArg(arg[0], XtNset, &chosen);
		XtSetArg(arg[1], XtNobjectData, &optr);
		OlFlatGetValues (pr->pkgBox, n, arg, 2);
		p_sel = (PkgPtr)optr->objectdata;
		if (chosen) {
			if (names == NULL) {
				names = (char *)MALLOC(2+strlen(p_sel->pkg_name));
				*names = '\0';
			}
			else
				names = (char *)REALLOC(names,2+strlen(names)+
						strlen(p_sel->pkg_name));
			sprintf(names+strlen(names) ," %s", p_sel->pkg_name);
			FindFuture(p_sel, del_num);
			del_num++;
		}
		else if (p_sel->pkg_opflag != 'D')
			delete_all = FALSE;
	}
	if (delete_all)  {
		names = (char *)REALLOC(names, cust_type? 5:
						strlen(p_sel->pkg_set)+1);
		strcpy(names, (cust_type? " ALL": p_sel->pkg_set));
	}
	if (cust_type)
		CallCustomDel(pr, p_sel->pkg_set, names, our_uid);
	else {
		sprintf(buf, "%s\"%s%s\" -e %s/dtexec -N %s %s", XTERM,
					GGT(string_remTitle), names, 
					GetXWINHome("adm"), REMPKG, names);
		if (P3OPEN(buf, lp->cmdfp, FALSE) != -1) {
			SetPopupMessage(pr->pkgPopup,
					GGT(string_invokePkgOp));
			lp->timeout = XtAddTimeOut(1000,
					(XtTimerCallbackProc)WaitRemovePkg,
					STRDUP(names));
		}
		else {
			SetPopupMessage(pr->pkgPopup,
					GGT(string_badPkgOp));
			lp->timeout = 0;
		}
	}
	FREE(names);
}

/*
 * Invoked from the "Properties" button in pkg window.
 */

static void
PkgInfoCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	PkgPtr	p_select = GetSelectedPkg ();

	SetPopupMessage (pr->pkgPopup, NULL);
	PopupPropSheet (p_select);
}

/*
 * Invoked by "Show Contents" button for packages
 */
static void
PkgContentsCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	ListRec *	lp = pr->sysList;
	PkgPtr		p_select = GetSelectedPkg (lp);

	SetPopupMessage(pr->iconPopup, NULL);
	PopupIconBox (p_select);
}

/*
 *  Added for Network Install
 */
void
GetListFromServer (ListRec *lp, char *where)
{
        char    str[BUFSIZ];

	lp->validList = False;
	/* 
	 * The sets_plus command will return all sets and all packages
	 * that are not members of any set.
	 */ 
	sprintf(str, "%s/sets_plus -s %s 2>&1", GetXWINHome("adm"), where);
	if (lp->max == 0) {
	        lp->max = QUANTUM;
		lp->pkg = (PkgPtr)MALLOC((lp->max+1)*sizeof(PkgRec));
	 }
	if (lp->count) {
	        FreePkgList(lp->pkg, &lp->count);
		lp->count = 0;
	}
	if (P3OPEN(str, lp->cmdfp, FALSE) != -1) {
	        *(lp->input) = '\0';
		lp->timeout = XtAddTimeOut (
			2500, (XtTimerCallbackProc)ReadUninstalledList, NULL
		);
		sprintf(str, GGT(format_wait), where);
	}
	else
	        sprintf(str, GGT(format_cantRead), lp->curlabel);
	SetCatalogMessage (pr->catalog, str);
	MapGizmo (PopupGizmoClass, pr->catalog);
	XSync (theDisplay, FALSE);
}
