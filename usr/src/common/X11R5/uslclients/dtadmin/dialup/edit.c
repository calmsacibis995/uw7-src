#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/edit.c	1.30.1.11"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <FButtons.h>
#include <FList.h>
#include <PopupWindo.h>
#include <Notice.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"


extern char *		ApplicationName;

extern void		FreeHostData();
extern Boolean		IsSystemFile();
extern void		BringDownPopup();
extern void		SetFields();
extern void		ResetFields();
extern void		AddLineToBuffer();
extern void		DeleteLineFromBuffer();
extern Widget		AddMenu();
extern void		UnselectSelect();
extern void		PropPopupCB();
extern void		CreateBlankEntry();
extern void		CBCreate();
extern void		CBProperty();
extern void		CBConfirm();
extern void		CBDelete();
extern void 	InstallCB();
void		AddEntryCB();
void		VerifyDeleteCB();
void		CancelCB();
void		DeleteEntryCB();

Arg arg[50];
static friend =1;
static port = 2;

static Items sys_editItems[] = {
	{AddEntryCB, NULL, (XA)TRUE}, /* New */
	{InstallCB, NULL, (XA)TRUE,NULL, NULL, (XA)&friend},
	{VerifyDeleteCB, NULL, (XA)TRUE},/* Delete */
	{PropPopupCB, NULL, (XA)TRUE},   /* Properties */
};

static Menus sys_editMenu = {
	"edit",
	sys_editItems,
	XtNumber (sys_editItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE,
 	NULL	
};

static Items dev_editItems[] = {
	{CBCreate, NULL, (XA)TRUE},
	{InstallCB, NULL, (XA) TRUE, NULL, NULL, (XA)&port},
	{CBConfirm, NULL, (XA)TRUE},
	{CBProperty, NULL, (XA)TRUE},
};

static Menus dev_editMenu = {
	"edit",
	dev_editItems,
	XtNumber (dev_editItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE,
	NULL
};

static Items cancelItems[] = {
	{DeleteEntryCB, NULL, (XA)TRUE},
	{CancelCB, NULL, (XA)TRUE},
};

static Menus cancelMenu = {
	"cancel",
	cancelItems,
	XtNumber (cancelItems),
	False,
	OL_FIXEDROWS,
	OL_NONE,
	NULL
};

Widget
AddEditMenu(wid)
Widget wid;
{
	Boolean sys = IsSystemFile(wid);
	
	if (sys) {
		SET_LABEL(sys_editItems,NEW_INDEX,new);
		SET_LABEL(sys_editItems,FOLDER_INDEX,install);
		SET_LABEL(sys_editItems,DELETE_INDEX,delete);
		SET_LABEL(sys_editItems,PROP_INDEX,properties);

        sys_editItems[NEW_INDEX].sensitive = sf->update;
			/* install button (copy to folder is insensitive if there
				are no items or no currently selected items */
		if ((sf->numFlatItems) && (sf->currentItem >= 0)) {
        	sys_editItems[FOLDER_INDEX].sensitive = sf->update;
        	sys_editItems[PROP_INDEX].sensitive = True;
        	sys_editItems[DELETE_INDEX].sensitive = sf->update;
		} else {
			sys_editItems[FOLDER_INDEX].sensitive = False;
			sys_editItems[PROP_INDEX].sensitive = False;
        	sys_editItems[DELETE_INDEX].sensitive = False;
		}

		return  AddMenu (wid, &sys_editMenu, False);
	} else
		SET_LABEL(dev_editItems,NEW_INDEX,new);
		SET_LABEL(dev_editItems,FOLDER_INDEX,install);
		SET_LABEL(dev_editItems,DELETE_INDEX,delete);
		SET_LABEL(dev_editItems,PROP_INDEX,properties);

        dev_editItems[NEW_INDEX].sensitive = sf->update;
		if ((df->cp->num_objs > 0) && (df->select_op!= NULL))  {
        	dev_editItems[FOLDER_INDEX].sensitive = sf->update;
        	dev_editItems[PROP_INDEX].sensitive =  True;
        	dev_editItems[DELETE_INDEX].sensitive = sf->update;
		} else	{
			dev_editItems[FOLDER_INDEX].sensitive = False;
        	dev_editItems[PROP_INDEX].sensitive = False;
        	dev_editItems[DELETE_INDEX].sensitive = False;
	}

		return   AddMenu (wid, &dev_editMenu, False);
} /* AddEditMenu */

void 
SetDevSensitivity()
{
#ifdef DEBUG
fprintf(stderr,"in SetDevSensitivity\n");
fprintf(stderr,"dp->select_op=%0x df->cp->num_objs=%d\n",df->select_op, df->cp->num_objs);
#endif
		if ((df->cp->num_objs > 0) && (df->select_op != NULL))  {
			OlVaFlatSetValues(dev_editMenu.widget,
					FOLDER_INDEX, XtNsensitive, 
					(Boolean) sf->update, 0 );
			OlVaFlatSetValues(dev_editMenu.widget,
					DELETE_INDEX, XtNsensitive, 
					(Boolean) sf->update, 0 );
			OlVaFlatSetValues(dev_editMenu.widget,
					PROP_INDEX, XtNsensitive, 
					(Boolean) True, 0 );
		} else {
			OlVaFlatSetValues(dev_editMenu.widget,
					FOLDER_INDEX, XtNsensitive, 
					False, 0 );
			OlVaFlatSetValues(dev_editMenu.widget,
					DELETE_INDEX, XtNsensitive, 
					False, 0 );
			OlVaFlatSetValues(dev_editMenu.widget,
					PROP_INDEX, XtNsensitive, 
					False, 0 );
		}
	SetDialSensitivity();
}

void 
SetSysCopyToFolder()
{
		if (sf->numFlatItems > 0) {
			OlVaFlatSetValues((Widget)sys_editMenu.widget, 
					FOLDER_INDEX, XtNsensitive, 
					(Boolean) sf->update, 0 );
			OlVaFlatSetValues((Widget)sys_editMenu.widget, 
					DELETE_INDEX, XtNsensitive, 
					(Boolean) sf->update, 0 );
			OlVaFlatSetValues((Widget)sys_editMenu.widget, 
					PROP_INDEX, XtNsensitive, 
					(Boolean) True, 0 );
		} else {
			OlVaFlatSetValues((Widget)sys_editMenu.widget,
					FOLDER_INDEX, XtNsensitive, 
					(Boolean) False, 0 );
			OlVaFlatSetValues((Widget)sys_editMenu.widget,
					DELETE_INDEX, XtNsensitive, 
					(Boolean) False, 0 );
			OlVaFlatSetValues((Widget)sys_editMenu.widget,
					PROP_INDEX, XtNsensitive, 
					(Boolean) False, 0 );
		}
}

void
AddEntryCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	/* Create a new entry */
    char buf[BUFSIZ];
	CreateBlankEntry ();
#ifdef DEBUG
fprintf(stderr,"calling ResetFields from AddEntryCB\n");
#endif
	ResetFields(new->pField);
	/* Reset the cusor to the first text field */
	AcceptFocus(sf->w_name);
	/* clear up the footer msg on the base window */
	CLEARMSG();
	ClearLeftFooter(sf->footer);
	sprintf(buf, "%s: %s", ApplicationName, GGT(title_addSys));
	SetPropertyAddLabel(0, label_add, mnemonic_add);
	SetValue(sf->propPopup, XtNfocusWidget, (Widget)sf->w_name);
	SetValue(sf->propPopup, XtNtitle, buf);
	callRegisterHelp(sf->propPopup, title_property, help_property);
	XtPopup(sf->propPopup, XtGrabNone);
} /* AddEntryCB */

void
ApplyNewEntry()
{
	register i;
	HostData *dp;
	LinePtr	lp;
	int len;
	char 	text[BUFSIZ*4];
	char 	expectSeq[BUFSIZ];
	char 	buf[BUFSIZ];

#ifdef TRACE
	fprintf(stderr,"ApplyNewEntry\n");
#endif
	sf->flatItems[sf->numFlatItems] = *new;
	new = (FlatList *) NULL;
	dp = sf->flatItems[sf->numFlatItems].pField;
	SetFields(dp);
	sf->currentItem += 1;
	/* widgets to sf->flatItems->pField */
#ifdef DEBUG
	fprintf(stderr,"sf->currentItem=%d sf->numFlatItems=%d\n",sf->currentItem,sf->numFlatItems);
#endif
	if (sf->numFlatItems) {
		/* Move all the entries back by one upto the current item */
		for (i=sf->numFlatItems; (i>=sf->currentItem && i > 0); i--) {
			sf->flatItems[i].pField = sf->flatItems[i-1].pField;
		}
		sf->flatItems[sf->currentItem-1].pField = dp;
	}
	sf->numFlatItems += 1;
	XtVaSetValues ((Widget)
		sf->scrollingList,
		XtNitems,		sf->flatItems,
		XtNnumItems,		sf->numFlatItems,
		XtNviewHeight,          VIEWHEIGHT,
		XtNitemsTouched,	True,
		(String)0
	);

#ifdef DEBUG
fprintf(stderr,"sf->currentItem=%d\n",sf->currentItem);
fprintf(stderr,"sf->numFlatItems=%d\n",sf->numFlatItems);
#endif
	XtVaSetValues ((Widget)
		sf->scrollingList,
		XtNviewItemIndex,	sf->currentItem-1,
		(String)0
	);

        if (sf->numFlatItems > 0) {
                /* Select the new item */
                OlVaFlatSetValues ((Widget)
                        sf->scrollingList,
                        sf->currentItem-1,
                        XtNset, True,
                        0
                );
        }

	lp = (LinePtr) XtMalloc (sizeof(LineRec));
	expectSeq[0] = 0;
	len = 0;
	for (i=0; i < dp->loginp->numExpectSend; i++) {
		strcpy(buf+len, dp->loginp->expectFlatItems[i].pExpectSend->f_prompt);
		len += strlen(dp->loginp->expectFlatItems[i].pExpectSend->f_prompt);
		buf[len++] = ' ';
		if ((strcmp(dp->loginp->expectFlatItems[i].pExpectSend->f_response , "") == 0)) {
		strcpy(buf+len,"\"\" ");
		len += 3;	
		} else  {
		strcpy(buf+len, dp->loginp->expectFlatItems[i].pExpectSend->f_response);
		len += strlen(dp->loginp->expectFlatItems[i].pExpectSend->f_response);
		buf[len++] = ' ';
		}

	}
	

	buf[len++] = '\0';
#ifdef DEBUG
	fprintf(stderr,"buf=%s\n",buf);
#endif
	
	sprintf (text,
		"%s %s %s %s %s %s\n",
		dp->f_name,
		dp->f_time,
		dp->f_type,
		dp->f_class,
		dp->f_phone,
		buf);
	lp->text = strdup(text);
	lp->next = NULL;
	if (sf->numFlatItems > 1) {
		AddLineToBuffer(sf->flatItems[sf->currentItem - 1].pField->lp, lp);
	} else {
		AddLineToBuffer(NULL, lp);
	}
	dp->lp = lp;
	if (sf->currentItem) 
		sf->currentItem = sf->currentItem -1;
#ifdef DEBUG
	fprintf(stderr, "end ApplyNewEntry\n");
#endif
} /* ApplyNewEntry */

static void
CancelCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BringDownPopup (sf->cancelNotice);
} /* CancelCB */

void
DeleteEntryCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	HostData *dp;
	register i;

	dp = sf->flatItems[sf->currentItem].pField;
#ifdef DEBUG
	fprintf(stderr,"DeleteEntryCB: sf->numFlatItems=%d\n",sf->numFlatItems);
#endif

	FreeHostData (dp);
#ifdef DEBUG
	fprintf(stderr,"sf->numFlatItems=%d sf->currentItem=%d\n",sf->numFlatItems,sf->currentItem);
#endif
	sf->numFlatItems -= 1;
#ifdef DEBUG
	fprintf(stderr,"DeleteEntryCB: sf->numFlatItems=%d\n",sf->numFlatItems);
#endif
	if (sf->numFlatItems) {
		for (i=sf->currentItem; i<sf->numFlatItems; i++) {
			sf->flatItems[i].pField = sf->flatItems[i+1].pField;
		}
		/* the very last entry */
		if (sf->currentItem == sf->numFlatItems)
			sf->currentItem -= 1;
	} else { /* sf->numFlatItems == 0 */
		sf->currentItem = -1;
		XtPopdown(sf->propPopup);
	}
#ifdef DEBUG
	fprintf(stderr,"now sf->flatItems=%d sf->currentItem=%d\n", sf->numFlatItems, sf->currentItem);
#endif
	XtVaSetValues ((Widget)
		sf->scrollingList,
		XtNitems,		sf->flatItems,
		XtNnumItems,		sf->numFlatItems,
		XtNviewHeight,          VIEWHEIGHT,
		XtNitemsTouched,	True,
		(String)0
	);
	if (sf->numFlatItems) {
		UnselectSelect ();
	}
	SaveSystemsFile();
	SetSysCopyToFolder();
	if(sf->cancelNotice)
		BringDownPopup (sf->cancelNotice);
} /* DeleteEntryCB */

static void
VerifyDeleteCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	static Widget	textArea,
			controlArea;
	char		warning[BUFSIZ];
	char		buf[BUFSIZ];
	
	if (sf->numFlatItems == 0) { /* nothing to delete */
		return;
	}
	if (sf->cancelNotice == (Widget) NULL) {

		sprintf(buf, "%s: %s", ApplicationName, GGT(title_deleteSys));
		XtSetArg (arg[0], XtNtitle, buf);
		sf->cancelNotice = XtCreatePopupShell(
			"CancelNotice",
			noticeShellWidgetClass,
			sf->toplevel,
			arg,
			1
		);

		XtVaGetValues(
			sf->cancelNotice,
			XtNtextArea, &textArea,
			XtNcontrolArea, &controlArea,
			(String)0
		);

		SET_LABEL(cancelItems,0,delete);
		SET_LABEL(cancelItems,1,cancel);
		AddMenu (controlArea, &cancelMenu, False);
	}
	sprintf (warning, GGT(string_deleteConfirm),
		((HostData*)(sf->flatItems[sf->currentItem].pField))->f_name);
	XtVaSetValues(
		textArea,
		XtNstring, warning,
		XtNborderWidth, 0,
		(String)0
	);
	XtPopup(sf->cancelNotice, XtGrabExclusive);
} /* VerifyDeleteCB */
