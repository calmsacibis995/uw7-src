/*		copyright	"%c%" 	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/control.c	1.25"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/Notice.h>
#include <Gizmos.h>
#include <stdio.h>
#include "uucp.h"
#include "error.h"

Arg arg[50];

#define FILE    0
#define EDIT    1

extern char *	ApplicationName;

extern void SetPropertyLabel();
extern void	CBPopupPropertyWindow();
extern void	HandleButtonAction();
extern void	AddMenu();
extern void	BringDownPopup();
extern void	AlignIcons();


void    CBCreate();
void	CBProperty();
void	CBConfirm();
void	CBDelete();
void	CBCancel();

static Items noticeItems[] = {
    {CBDelete, NULL, (XA)TRUE},
    {CBCancel, NULL, (XA)TRUE},
};

static Menus deleteMenu = {
	"delete",
	noticeItems,
	XtNumber(noticeItems),
	False,
	OL_FIXEDROWS,
	OL_NONE,
	NULL
};

void
CBProperty(w, client_data, call_data)
Widget          w;
XtPointer       client_data;
XtPointer       call_data;
{

	char buf[BUFSIZ];
	/*
	 * set initial values to select by copying the obj select and
	 * forcing a SET
	 */
	if (df->select_op == (DmObjectPtr) NULL)
		return;
	df->request_type = B_MOD;
	SetSelectionData();
	HandleButtonAction(df->propPopup, RESET,NULL);
	sprintf(buf, "%s: %s", ApplicationName, GGT(title_dproperties));
	XtVaSetValues(df->propPopup, XtNtitle, buf, 0);
	SetDevPropertyLabel(0, label_ok, mnemonic_ok);
    ClearFooter(sf->footer); /* clear mesaage area */
    ClearFooter(df->footer); /* clear mesaage area */

	CBPopupPropertyWindow(w, (caddr_t)0, (caddr_t)0);
}

void
CBCreate(w, client_data, call_data)
Widget          w;
XtPointer       client_data;
XtPointer       call_data;
{
	char buf[BUFSIZ];

	df->request_type = B_ADD;
	HandleButtonAction(df->propPopup, RESETFACTORY, NULL);
	sprintf(buf, "%s: %s", ApplicationName, GGT(title_addDev));
	XtVaSetValues(df->propPopup, XtNtitle, buf, 0);
	SetDevPropertyLabel(0, label_add, mnemonic_add);
	CBPopupPropertyWindow(w, 0, 0);
}

/*
 *	delete the selected item from the icon container
 *	o skip unmanaged items in the icon container
 *	o find the item match with the selected object
 *	o unmanage the item
 *	o unselect the item and change its associated data
 *	  structure in the container
 */
void
CBDelete(w, client_data, call_data)
Widget          w;
XtPointer       client_data;
XtPointer       call_data;
{
	extern void DelObjectFromContainer();
	DmItemPtr		ip;
	int			nitems;
	int			i;
	char *oldport;
	char *port;
	DeviceData *tap;

	XtVaGetValues(df->iconbox,
		XtNnumItems, &nitems,
		0
		);
#ifdef DEBUG
	fprintf(stderr, "In Deletion: nitems = %d\n", nitems);
#endif
	for (i=0, ip = df->itp; i < nitems; i++, ip++) {
		if(ITEM_MANAGED(ip) == False)
			continue;
		if((DmObjectPtr)OBJECT_PTR(ip) == df->select_op) { /* found it */
			OlVaFlatSetValues(df->iconbox, (int)(ip - df->itp),
					XtNmanaged, False,
					XtNselect, False,
					0
				);
			ip->select = False;
			tap = df->select_op->objectdata;
			/* set port to the value of the port that
				is being deleted */
			port = strdup(tap->portNumber);
				/* oldport is the previous port number */
			oldport = strdup(tap->holdPortNumber);

			DelObjectFromContainer(df->select_op);
			df->select_op = (DmObjectPtr) NULL;
			AlignIcons();
			break;
		}
	}
	SaveDeviceFile (port, oldport, DELETE);
	free(port);
	free(oldport);
	if(df->cancelNotice)
		BringDownPopup (df->cancelNotice);

} /* CBDelete */

void
CBConfirm(w_parent, client_data, call_data)
Widget		w_parent;
XtPointer	client_data;
XtPointer	call_data;
{
        static Widget	textArea,
			controlArea;
        char		warning[BUFSIZ];
        char		buf[BUFSIZ];

	if (df->select_op == (DmObjectPtr) NULL)
		return;
	if (df->cancelNotice == (Widget) NULL) {
		sprintf(buf, "%s: %s", ApplicationName, GGT(title_deleteDev));
		XtSetArg (arg[0], XtNtitle, buf);
		df->cancelNotice = XtCreatePopupShell(
			"notice-shell",
			noticeShellWidgetClass,
			df->toplevel,
			arg,
			1
		);

		XtVaGetValues(
			df->cancelNotice,
			XtNtextArea, &textArea,
			XtNcontrolArea, &controlArea,
			(String)0
		);

		SET_LABEL (noticeItems,0,delete);
		SET_LABEL (noticeItems,1,cancel);

		AddMenu (controlArea, &deleteMenu, False);
	}
	sprintf (warning, GGT(string_deleteConfirm),
		((DeviceData*)(df->select_op->objectdata))->holdPortNumber);
	XtVaSetValues(
		textArea,
		XtNstring, warning,
		XtNborderWidth, 0,
		(String)0
	);

	XtPopup(df->cancelNotice, XtGrabExclusive);
}

void
CBCancel(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	if(df->cancelNotice)
		BringDownPopup (df->cancelNotice);
} /* CBCancel */
