#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/wsmproperty.c	1.97.1.1"
#endif

#include <stdio.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#ifdef DEBUG
#include <X11/Xmu/Editres.h>
#endif /* DEBUG */

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/Protocols.h>
#include <Xm/MessageB.h>

#include <Dtm.h>
#include "dm_strings.h"

#include "error.h"
#include <misc.h>
#include <node.h>
#include <list.h>
#include <exclusive.h>
#include <nonexclu.h>
#include <property.h>
#include <resource.h>
#include <xtarg.h>
#include <wsm.h>

/*
 * Convenient macros:
 */

#define MAX_BUF		256
#define DEFS		"/.Xdefaults"

#define PropertyOf(X) ((Property *)DNODE_data(X))

/*
 * Global data:
 */

List			global_resources = LISTOF(Resource);

/*
 * Local functions:
 */

static void		DestroyPropertyPopup_Phase2( void );
static Widget		CreatePropertyPopup( void );
static void		CheckApplyAll( int );
static void		PopdownCB( Widget, XtPointer, XtPointer );
static void		PropertyResetCB( Widget, XtPointer, XtPointer );
static void		PropertyFactoryCB( Widget, XtPointer, XtPointer );
static void		NewPageCB( Widget, XtPointer, XtPointer );
static void		ChangeSheets(Property *	, Property *, int, int	);
static DNODE *		GetSheet(Property * );
static void		_BringDownPopup();
static DNODE *		CreateSheet( Property *	);
void 			BusyPeriod ( Widget, Boolean );
void			WSMHelpCB( Widget, XtPointer, XtPointer );
static void		SetMinimumSize( void );

/*
 * Local data:
 */

static Property *	*_property;

static List		property;
static List		buffer		= LISTOF(char);
static List		resources	= LISTOF(Resource);

static DNODE *		sheets		= NULL;
static DNODE *		current_sheet;

static Widget		properties	= 0;
static Widget		mainRow	= 0;
static Widget		category	= 0;
static Widget		footer		= 0;
static Widget		rowOfPB		= 0;
static Widget		popupmenu	= 0;
static Widget		buttons[5];

static String		resource_filename;

#define APPLY_BUTTON		0

static String		ApplyLabel =	"Apply";
static String		ApplyAllLabel = "Apply All";
static String		CurrentApplyLabel;

static int		ApplyAllStatus = True;
static Property *	oldSheet;

/* name of property sheets and corresponding help files */

static char *helpFileInfo[][2] = {
	NULL, "DesktopMgr/dskpref.hlp",
	NULL, "DesktopMgr/moupref.hlp",
	NULL, "DesktopMgr/locpref.hlp",
	NULL, "DesktopMgr/winpref.hlp"
};

static char *(*HelpFileInfo)[2];

/* Mnemonic information */
static DmMnemonicInfoRec	mneInfo[5];
static Cardinal			numMne;

/**
 ** InitProperty()
 **/

void InitProperty (Display* dpy)
{
	list_ITERATOR		I;

	Property **		p;
	Property *		q;
	int i = 0;
	int j;
	
	delete_RESOURCE_MANAGER();
	resource_filename = GetPath(DEFS);

	windowProperty.pLabel = pGGT(TXT_pageLabel_window);
	desktopProperty.pLabel = pGGT(TXT_pageLabel_desktop);
	localeProperty.pLabel = pGGT(TXT_pageLabel_setLocale);
	settingsProperty.pLabel = pGGT(TXT_pageLabel_settings);
	
	_property = (Property **)malloc(sizeof(Property *) * 8);
	
	_property[i++] = &desktopProperty;
	_property[i++] = &windowProperty;
	
	_property[i++] = &settingsProperty;
	_property[i++] = &localeProperty;
	
	
	property.entry = (ADDR)_property;
	property.size = sizeof(Property *);
	property.count = i;
	property.max = 0;
	
	
	I = list_iterator(&property);
	while (p = (Property **)list_next(&I))
	{
		q = *p;
		if (q->import)
		{
			(*q->import) (q->closure);
		}
	}
	
	list_clear (&buffer);
	
	if (read_buffer(resource_filename, &buffer)) 
	{
		list_clear (&resources);

		buffer_to_resources (&buffer, &resources);
		merge_resources (&global_resources, &resources);
		free_resources (&resources);
	}
	else
	{
		Dm__VaPrintMsg(TXT_warningMsg_noDefaults,
				resource_filename ? resource_filename : ""); 
	}
	
	I = list_iterator(&property);
	while (p = (Property **)list_next(&I))
	{
		q = *p;
		if (q->export)
		{
			(*q->export) (q->closure);
		}
	}
	
	UpdateResources ();
	
	return;
} /* InitProperty */

/**
 ** UpdateResources()
 **/

void UpdateResources ( void )
{
	list_clear (&buffer);
	resources_to_buffer (&global_resources, &buffer);
	change_RESOURCE_MANAGER (&buffer);
	if (!write_buffer(resource_filename, &buffer)) 
	{
		Dm__VaPrintMsg(TXT_warningMsg_cannotWrite,
			      resource_filename ? resource_filename : "");
	}

	return;
} /* UpdateResources */

/**
 ** MergeResources()
 **/

void MergeResources ( String str )
{
	list_clear (&buffer);
	list_clear (&resources);
	list_append (&buffer, str, strlen(str));
	buffer_to_resources (&buffer, &resources);
	merge_resources (&global_resources, &resources);
	free_resources (&resources);
	UpdateResources ();

	return;
} /* MergeResources */

/**
 ** DeleteResources()
 **/

void DeleteResources ( String str )
{
	list_clear (&buffer);
	list_clear (&resources);
	list_append (&buffer, str, strlen(str));
	buffer_to_resources (&buffer, &resources);
	delete_resources (&global_resources, &resources);
	free_resources (&resources);
	UpdateResources ();

	return;
} /* DeleteResources */

/**
 ** PropertySheetByName()
 **/

void PropertySheetByName ( String name )
{
	Cardinal		n;
	
	for (n = 0; n < property.count; n++)
	{
		if (MATCH(_property[n]->name, name)) 
		{
			PropertyCB ( (Widget)0, (XtPointer)_property[n], (XtPointer)0 );
			break;
		}
        }
	return;
} /* PropertySheetByName */

/**
 ** PropertyCB()
 **/

void PropertyCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
	)
{
	Property *		p	= (Property *) client_data;

	DNODE *			sheet	= (DNODE *) GetSheet(p);

	Display *		display	= XtDisplayOfObject(handleRoot);


#if	defined(MEMDEBUG)
	_SetMemutilDebug(True);
#endif

	/*
	 * Make sure the user can see the property window, if it already
	 * exists. We do this only here (not, e.g., in ChangeSheets()",
	 * to give the user the chance to NOT bring the window to the
	 * front yet still switch pages.
	 */
	if (properties)
	{
	  	XtManageChild(properties);
		XtPopup(XtParent(properties), XtGrabNone);
		XRaiseWindow(display, XtWindow(properties));
		XmUpdateDisplay(properties);
	}
	ChangeSheets((Property *)0, (Property *)p, True, True);

	return;
} /* PropertyCB */

/**
 ** DestroyPropertyPopup()
 **/

void DestroyPropertyPopup (Widget w, XtPointer clientData, XtPointer callData)
{
	/*
	 * Phase 1: Just pop the window down--this will cause
	 * the XtNpopdownCallback to be issued.
	 */
	if (properties)
	{
		XtUnmanageChild(properties);
		PopdownCB(properties, NULL, NULL);
		
	}
	return;
} /* DestroyPropertyPopup */

/**
 ** DestroyPropertyPopup_Phase2()
 **/

static void DestroyPropertyPopup_Phase2 ( void )
{
	if (properties) 
	{
		XtDestroyWidget(properties);
		properties = 0;
		current_sheet = 0;
		oldSheet = 0;
	}
	return;
} /* DestroyPropertyPopup_Phase2 */

/**
 ** ResourceItem()
 **/

ExclusiveItem * ResourceItem (
	Exclusive *		exclusive,
	String			name
	)
{
	list_ITERATOR		I;

	ExclusiveItem *		p;


	if (name)
	{
		I = list_iterator(exclusive->items);
		while ((p = (ExclusiveItem *)list_next(&I)))
		{
			if (MATCH(name, (String)p->addr))
			{
				return (p);
			}
	        }
	}
	return (0);
} /* ResourceItem */

/**
 ** NonexclusiveResourceItem()
 **/

NonexclusiveItem * NonexclusiveResourceItem (
	Nonexclusive *		nonexclusive,
	String			name
	)
{
	list_ITERATOR		I;

	NonexclusiveItem *	p;


	if (name)
	{
		I = list_iterator(nonexclusive->items);
		while ((p = (NonexclusiveItem *)list_next(&I)))
		{
			if (MATCH(name, (String)p->addr))
			{
				return (p);
			}
		}
	}
	return (0);
} /* NonexclusiveResourceItem */

/**
 ** CreateCaption()
 **/

Widget CreateCaption (
	String			name,
	String			label,
	Widget			parent
	)
{
 	Widget			w;
	Widget			p	= parent;
	Screen *		screen	= XtScreenOfObject(parent);
	XFontStruct *		fs;
	XmString		string;

#define _1horzinch _XmConvertUnits(screen, XmHORIZONTAL, Xm100TH_MILLIMETERS, 254.0, XmPIXELS);
                   /* OlScreenMMToPixel(OL_HORIZONTAL,2.54,screen) */
	
#ifdef CAPTION_WIDGET
	w = XtVaCreateManagedWidget(
		name,
		xmCaptionWidgetClass,
		parent,
		XmNlabelString, XmStringCreateLocalized(label),
		NULL
		);
	XtVaSetValues(w,
		XmNmarginWidth, 20,
		NULL);
		
			/* Set the resources if this	*/
			/* is a decendent of the category	*/
			/* widget. Satisfies the resource	*/
			/* specification:			*/
			/* *category*Caption			*/
#else
	string = XmStringCreateLocalized(label);
	w = XtVaCreateWidget(
		name,
		xmFormWidgetClass,
		parent,
		XmNfractionBase,	10,
		NULL);
	w = XtVaCreateManagedWidget(
		"CaptionLabel",
		xmLabelWidgetClass,
		w,
		XmNlabelString, 	string,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_END,
		NULL);
	XmStringFree(string);
#endif
	return (w);
} /* CreateCaption */

void AddToCaption (
	Widget	item,
	Widget	attach)
{

#ifndef CAPTION_WIDGET
	XtVaSetValues(item,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		attach,
		NULL);
	XtManageChild(XtParent(item));
#endif
	return;
}

static XtCallbackRec cb[] = {
	{(XtCallbackProc)NULL, (XtPointer)NULL},
	{(XtCallbackProc)NULL, (XtPointer)NULL}
};

/**
 ** CreatePropertyPopup()
 **/

static Widget CreatePropertyPopup ( void )
{
	Widget			pup;
	Widget			w;
	Widget			lca;
	Widget			menupane;
	Widget form,
	       pulldown;

	list_ITERATOR		I;

	Property **		pp;
	Property *		q;

	Cardinal		i;
	Screen *		screen	= XtScreenOfObject(handleRoot);

	Display *		dpy;
	char *			temp;
	Arg			args[30];
	XmString		string[10];
	int			j;
	Dimension		width;
	Dimension		total;
	unsigned char * 	mnemonic[5];
	char *			label[10];
	KeySym          	ks;

	label[0] = pGGT(TXT_fixedString_apply);
	label[1] = pGGT(TXT_fixedString_reset);
	label[2] = pGGT(TXT_fixedString_factory);
	label[3] = pGGT(TXT_fixedString_cancel);
	label[4] = pGGT(TXT_fixedString_helpdot);
	
	string[0] = XmStringCreateLocalized(label[0]);
	string[1] = XmStringCreateLocalized(label[1]);
	string[2] = XmStringCreateLocalized(label[2]);
	string[3] = XmStringCreateLocalized(label[3]);
	string[4] = XmStringCreateLocalized(label[4]);
	
	mnemonic[0] = (unsigned char *)pGGT(TXT_mnemonic_apply);
	mnemonic[1] = (unsigned char *)pGGT(TXT_mnemonic_reset);
	mnemonic[2] = (unsigned char *)pGGT(TXT_mnemonic_factory);
	mnemonic[3] = (unsigned char *)pGGT(TXT_mnemonic_cancel);
	mnemonic[4] = (unsigned char *)pGGT(TXT_mnemonic_helpdot);
	
	cb[0].callback = (XtCallbackProc)WSMHelpCB;

	XtSetArg(args[0], XmNokLabelString,	 string[0]);
	XtSetArg(args[1], XmNapplyLabelString,	 string[1]);
	XtSetArg(args[2], XmNcancelLabelString, 	 string[3]);
	XtSetArg(args[3], XmNhelpLabelString,	 string[4]);
	XtSetArg(args[4], XmNminimizeButtons,	 True);
	XtSetArg(args[5], XmNdefaultButtonType,	 XmDIALOG_CANCEL_BUTTON);
	XtSetArg(args[6], XmNtitle, (XtArgVal) pGGT(TXT_fixedString_propsTitle));
	XtSetArg(args[7], XmNautoUnmanage, False);
	XtSetArg(args[8], XmNborderWidth, 2);
	XtSetArg(args[9], XmNnoResize, True);
	XtSetArg(args[10], XmNhelpCallback, cb);
	pup = (Widget) XmCreatePromptDialog(
		DESKTOP_SHELL(Desktop), 
		"properties", 
		args, 11
	);
#ifdef DEBUG
	XtAddEventHandler(
		XtParent(pup), (EventMask) 0, True,
		_XEditResCheckMessages, NULL
	);
#endif /* DEBUG */
	
	XtVaSetValues(
		XtParent(pup),
		XmNallowShellResize,	True,
		NULL
	);
	
	buttons[0] = (Widget ) XmSelectionBoxGetChild( pup, XmDIALOG_OK_BUTTON);
	buttons[1] = (Widget ) XmSelectionBoxGetChild( pup, XmDIALOG_APPLY_BUTTON);
	buttons[3] = (Widget ) XmSelectionBoxGetChild( pup, XmDIALOG_CANCEL_BUTTON);
	buttons[4] = (Widget ) XmSelectionBoxGetChild( pup, XmDIALOG_HELP_BUTTON);
	
	XtManageChild(buttons[1]);
	
	XtAddCallback(buttons[0], XmNactivateCallback, PropertyApplyCB, NULL);
	XtAddCallback(buttons[1], XmNactivateCallback, PropertyResetCB, NULL);
	XtAddCallback(buttons[3], XmNactivateCallback, DestroyPropertyPopup, NULL);
	/*
	XtAddCallback(buttons[4], XmNactivateCallback, WSMHelpCB, NULL);
	*/
	
	XtUnmanageChild((Widget) XmSelectionBoxGetChild( pup, XmDIALOG_TEXT));
	XtUnmanageChild((Widget) XmSelectionBoxGetChild( pup, XmDIALOG_SELECTION_LABEL));
	
	dpy = XtDisplay(pup);
	
	XmAddWMProtocolCallback(XtParent(pup), (Atom) XA_WM_DELETE_WINDOW(dpy),
			DestroyPropertyPopup, (XtPointer) WM_DELETE_WINDOW);
	
	/* Create form widget */
	mainRow = XtVaCreateWidget("form", 
	                               xmRowColumnWidgetClass, 
				       pup, 
				       XmNallowOverlap, False,
	                               NULL);

	buttons[2] = XtVaCreateManagedWidget("factory", xmPushButtonGadgetClass, pup, 
		XmNlabelString, string[2],
		NULL);

	total = 0;
	numMne = 0;
	for(j = 0; j < 5; j++) 
	{ 

	    if (
	    	mnemonic[j] != NULL &&
	    	(((ks = XStringToKeysym((char *)mnemonic[j])) != NoSymbol) ||
	    	(ks = (KeySym)(mnemonic[j][0]))) &&
	    	strchr(label[j], (char)(ks & 0xff)) != NULL
	    ) {
	    	XtVaSetValues(buttons[j], XmNmnemonic, ks, NULL);
		mneInfo[j].mne = (unsigned char *)mnemonic[j];
		mneInfo[j].mne_len = strlen((char *)mnemonic[j]);
		mneInfo[j].op = DM_B_MNE_ACTIVATE_BTN | DM_B_MNE_GET_FOCUS;
		mneInfo[j].w = buttons[j];
		if (j == 4) {
			mneInfo[j].op = DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS;
			mneInfo[j].cb = WSMHelpCB;
		}
		numMne += 1;
	    }
	    XmStringFree(string[j]); 
	    XtVaGetValues(buttons[j], XmNwidth, &width, NULL);
	    total += width;
	}
	
	XtVaGetValues(buttons[2], XmNwidth, &width, NULL);
	total += width;
	width = XDisplayWidth(XtDisplay(pup), 0);
	if (total > width*8/9) {
		XtVaSetValues(pup, XmNwidth, width*8/9, NULL);
	}

	XtAddCallback(buttons[2], XmNactivateCallback, PropertyFactoryCB, NULL);
	
	/* Create option menu for Category */ 
	pulldown = XmCreatePulldownMenu(mainRow, "pulldown", NULL, 0);
	XtSetArg(args[0], XmNsubMenuId, pulldown);
	XtSetArg(args[1], XmNentryBorder, 1);
	XtSetArg(args[2], XmNlabelString, XmStringCreateLocalized(pGGT(TXT_fixedString_Category)));
	
	category = XmCreateOptionMenu(mainRow, "menu", args, 3);
	
	/* Set value of apply button "Apply/Apply All" */

	CurrentApplyLabel = ApplyLabel;
	
	/*
	 * Create each Property Sheet composite (with no contents):
	 */
	
	I = list_iterator(&property);
	while (pp = (Property **)list_next(&I)) 
	{
		q = *pp;
		
		q->w = XtVaCreateWidget(q->name, xmFormWidgetClass, mainRow,
					       NULL);
		
		q->pb = XtVaCreateManagedWidget("catPB", xmPushButtonGadgetClass, pulldown,
		                            XmNlabelString, XmStringCreateLocalized(q->pLabel),
					    NULL);
		
		XtAddCallback (q->pb, XmNactivateCallback, NewPageCB, (Property *) q);
		XtSetValues (q->w, q->args, q->num_args);
		
	}
	
	
	/* Create label for footer */
	footer = XtVaCreateManagedWidget("statusArea", xmLabelWidgetClass, mainRow,
					NULL);
	
	XtManageChild(category);
	XtManageChild(mainRow);

	return (pup);
} /* CreatePropertyPopup */

/**
 ** CheckApplyAll()
 **/

static void CheckApplyAll ( int apply_all )
{
  static Bool firsttime = TRUE;
  XmString	string;
  /* Display *dpy = XtDisplay(LcaApplyMenu); */

  if (firsttime)
  {
      firsttime = FALSE;
      ApplyLabel = pGGT(TXT_fixedString_apply);
      ApplyAllLabel = pGGT(TXT_fixedString_applyall);
      CurrentApplyLabel = ApplyLabel;
  }
	
	if ( (apply_all) && (CurrentApplyLabel == ApplyLabel))
	{
		string = XmStringCreateLocalized(ApplyAllLabel);
		XtVaSetValues(buttons[0],
		              XmNlabelString, string,
			      NULL);
		XmUpdateDisplay(buttons[0]);
		XmStringFree(string);
		CurrentApplyLabel = ApplyAllLabel;
	}
	else if (!apply_all && CurrentApplyLabel == ApplyAllLabel)
	{
		string = XmStringCreateLocalized(ApplyLabel);
		XtVaSetValues(buttons[0],
		              XmNlabelString,string,
			      NULL);
		XmUpdateDisplay(buttons[0]);
		XmStringFree(string);
		CurrentApplyLabel = ApplyLabel;
	}
	
	return;
}

static Gizmo		g = NULL;

static void
DestroyCB(Widget w, XtPointer clientData, XtPointer callData)
{
	FreeGizmo(ModalGizmoClass, g);
}

static void
DestroyNoticeBox()
{
	if (g != NULL) {
		XtAddCallback(
			GetModalGizmoShell(g), XtNdestroyCallback,
			DestroyCB, NULL
		);
		XtDestroyWidget(GetModalGizmoShell(g));
		g = NULL;
	}
}

/**
 ** PopdownCB()
 **/

static void PopdownCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
	)
{
	dring_ITERATOR		I;

	DNODE *			q;
	
	I = dring_iterator(&sheets);
	while (q = dring_next(&I)) 
	{
		Property *		p = PropertyOf(q);

		if (p->popdown)
		{
			(*p->popdown) (p->w, p->closure);
	        }
		free_DNODE (dring_delete(&sheets, q));
	}
	DestroyNoticeBox();
	DestroyPropertyPopup_Phase2 ();

	return;
} /* PopdownCB */

static MenuItems	modalItems[] = {
	{True, TXT_OK, TXT_M_OK, I_PUSH_BUTTON},
	{NULL}
};

static MenuGizmo	modalMenu = {
	NULL, "modalMenu", NULL, modalItems, NULL, NULL, XmHORIZONTAL, 1, 0, 0
};

static ModalGizmo	modal = {
	NULL, "modal", NULL, &modalMenu, NULL, NULL, 0,
	XmDIALOG_FULL_APPLICATION_MODAL, XmDIALOG_WARNING
};

static void
CreateNoticeBox(Widget w, Notice *notice)
{
	XmString		str;
	static Widget		box;
	static XtCallbackRec	ok[] = {
		{NULL, NULL},
		{NULL, NULL}
	};

	if (g == NULL) {
		g = CreateGizmo(
			w, ModalGizmoClass, &modal, NULL, 0
		);
		box = GetModalGizmoDialogBox(g);
		XtUnmanageChild(
			XmMessageBoxGetChild(box, XmDIALOG_HELP_BUTTON)
		);
		XtUnmanageChild(
			XmMessageBoxGetChild(box, XmDIALOG_CANCEL_BUTTON)
		);
		XtVaSetValues(
			box,
			XmNminimizeButtons, True,
			NULL
		);
	}
	ok[0].callback = notice->callback;
	ok[0].closure = (XtPointer)PropertyApplyCB;
	str = XmStringCreateLocalized(pGGT(notice->title));
	XtVaSetValues(
		box,
		XmNokCallback,	ok,
		XmNdialogTitle, str,
		NULL
	);
	XmStringFree(str);
	SetModalGizmoMessage(g, pGGT(notice->message));
	notice->g = g;
}

/**
 ** PropertyApplyCB()
 **/

void PropertyApplyCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
	)
{
	dring_ITERATOR		I;

	DNODE *			q;

	Boolean			ok	= True;
	Boolean			refresh = False;

	Property *		p	= 0;
	Property *		bad_p;

	extern Widget		InitShell;


	BusyPeriod (handleRoot, True);

	I = dring_iterator(&sheets);
	while (ok && (q = dring_next(&I))) 
	{
		ApplyReturn *		r;
		Boolean			changed;

		p = PropertyOf(q);
		
		changed = p->changed;
		
		/* XtVaGetValues (p->w, XmNchanged, (XtArgVal)&changed, (String)0); */
		if( (!p->apply) || (!changed)) 
		{
			continue;
		}

		r = (*p->apply)(p->w, p->closure);
		switch (r->reason)
		{

		case APPLY_REFRESH:
			refresh = True;
			/*FALLTHROUGH*/

		case APPLY_OK:
			p->changed = False;
			FooterMessage (category, (p->footer = 0), False); 
			break;

		case APPLY_NOTICE:
			/*
			 * Don't use "w" here, because one of the sheets
			 * may have called us directly, using some other
			 * widget for "w". We want the Apply/ApplyAll
			 * button as the emanate widget.
			 */

			CreateNoticeBox(InitShell, r->u.notice);
			MapGizmo(ModalGizmoClass, r->u.notice->g);
			bad_p = (r->bad_sheet? r->bad_sheet : p);
			ok = False;
			break;

		case APPLY_ERROR:
			bad_p = (r->bad_sheet? r->bad_sheet : p);
			FooterMessage (
				category,
				(bad_p->footer = r->u.message),
				True
			);
			ok = False;
			break;
		} 
	}

	/*
	 * If we went through the loop at least once ("p" != 0),
	 * and if we emerged without an error ("ok" is True), then
	 * it is time to tell the world about the changes.
	 */
	if (p && ok) 
	{
		UpdateResources ();
		if (refresh)
		{
		    RefreshCB (w, (XtPointer)0, (XtPointer)0);
		}
		_BringDownPopup ();

	/*
	 * If we had an error, then switch to that sheet so that
	 * the user can see what needs to be done.
	 */
	} 
	else if (!ok)
	{
		ChangeSheets((Property *)0,(Property *)bad_p, True, True);
	}

	BusyPeriod (handleRoot, False);
	return;
} /* PropertyApplyCB */

/**
 ** PopupMenuCB()
 **/

void PopupMenuCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
	)
{
	/* 
	switch (ve->virtual_name) 
	{
	    case OL_MENU:
	    case OL_MENUKEY:
		    if (ve->xevent->type == ButtonPress) 
		    {
			    ve->consumed = True;
		    }
		break;
	}
	*/

	return;
} /* PopupMenuCB */

/**
 ** PropertyResetCB()
 **/

static void PropertyResetCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
	)
{
	Property *		p = PropertyOf(current_sheet);
	
	if (p->reset)
	{
		(*p->reset) (p->w, p->closure);
	}
	FooterMessage (category, (p->footer = 0),  False);

	return;
} /* PropertyResetCB */

/**
 ** PropertyFactoryCB()
 **/

static void PropertyFactoryCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
	)
{
	Property *		p = PropertyOf(current_sheet);


	if (p->factory)
	{
		(*p->factory) (p->w, p->closure);
	}
	FooterMessage (category, (p->footer = 0),  False);

	return;
} /* PropertyFactoryCB */

/**
 ** NewPageCB()
 **/

static void NewPageCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
	)
{
	Property *		new;
	
	new = (Property *)client_data;
	ChangeSheets(oldSheet, new, True, ApplyAllStatus );
	oldSheet = new;

	return;
} /* NewPageCB */

/**
 ** ChangeSheets()
 **/

static void ChangeSheets(Property * old, Property * new, int tell_category, int apply_all)
{
	Boolean			do_popup = False;
	Widget			wid;
	Dimension		border;
	
	BusyPeriod (handleRoot, True);

	/*
	 * If the property window doesn't exist yet, create it.
	 */
	if (properties == NULL)
	{
		properties = CreatePropertyPopup();
		XtAddCallback(properties, XmNhelpCallback, WSMHelpCB, NULL);
		do_popup = True;
		DmRegisterMnemonicWithGrab(properties, mneInfo, numMne);
	}

	/*
	 * Specifying NULL for the "old" sheet is a convenient way
	 * of saying it's the current sheet.
	 */
	if (old == NULL) 
	{
		if (current_sheet != NULL)
		{
			old = PropertyOf(current_sheet);
		}
	}

	/*
	 * If we're already on the new sheet, we're done.
	 */
	if (old && new && (old == new))
	{
		goto Return;  /* Oh, my God, a goto!!! */
	}

	/*
	 * Create the new sheet if it doesn't exist yet, tell the category
	 * widget to manage and display it (unless it was the one who told
	 * US about the new sheet), then restore its last footer.
	 *
	 * (The creation routine will have provided defaults if this
	 * is a new sheet).
	 */
	if (!(current_sheet = GetSheet(new)))
	{
		current_sheet = CreateSheet(new);
	}
	
	/* Unmanage old sheet, set attachments, manage new sheet */
	if(new == NULL)
	{
	}
	
	XtVaSetValues(category, XmNmenuHistory, new->pb, XmNallowShellResize, False, NULL);
	
	
	if(old != NULL)
	{
	    XtUnmanageChild(old->w);
	    old = new;
	}
	
	XtUnmanageChild(footer);
	XtManageChild(new->w);
        XtManageChild(footer);
	XmUpdateDisplay(new->w);
	XtVaSetValues(category, XmNallowShellResize, True, NULL);
	
	if (tell_category)
	{
		TouchPropertySheets();
		apply_all = ApplyAllStatus;
	}
	FooterMessage (category, new->footer, False );  /* New format */

	/*
	 * Now pop up the property window if it isn't up yet.
	 * Only if it is already displayed is it possible to have
	 * need to show the Apply All button (think about it).
	 */
	
	if (do_popup) {
		XtManageChild(properties);
	}
	else
	{
	  	XtManageChild(properties);
		CheckApplyAll ( (int) apply_all);
	}
	
	XtVaGetValues(properties, XmNborderWidth, &border, NULL);

	/*
	 * MORE: It might be nice to keep track of where the focus
	 * was last on each sheet, and restore the focus when moving
	 * to a new sheet. Two problems make this difficult at the
	 * present:
	 *
	 *	(1) We can't move focus to a widget inside the new
	 *	sheet until it is mapped, but that doesn't happen
	 *	until after we return from this callback.
	 *
	 *	(2) The user will often choose the new sheet from
	 *	the CATEGORY menu, so that is where the focus is;
	 *	asking for the current focus on the old sheet here,
	 *	for the purpose of saving it for later, will return
	 *	nothing.
	 */

Return:
	BusyPeriod (handleRoot, False);
	return;
} /* ChangeSheets */

/**
 ** GetSheet()
 **/

static DNODE * GetSheet ( Property * sheet )
{
	dring_ITERATOR		I;

	DNODE *			p;


	I = dring_iterator(&sheets);
	while ((p = dring_next(&I)))
	{
		if (sheet == PropertyOf(p))
		{
			return (p);
		}
	}
	return (0);
} /* GetSheet */

/**
 ** _BringDownPopup()
 **/

static void _BringDownPopup ()
{
	CheckApplyAll ( (int) False);
	ApplyAllStatus = False;
	XtUnmanageChild(properties);
	PopdownCB(properties, NULL, NULL);
	return;
} /* _BringDownPopup */

/**
 ** CreateSheet()
 **/

static DNODE * CreateSheet ( Property * p )
{
	DNODE *			sheet;
	char *			lab;
	
	sheet = alloc_DNODE((ADDR)p);
	dring_push (&sheets, sheet);
	(*p->create) (p->w, p->closure);
	
	/* XtVaGetValues(p->w, XmNpageLabel, (XtArgVal)&lab, (String)0); */
	
	/*
	p->help->appTitle = pGGT(TXT_fixedString_propsTitle);
	XtRemoveCallback(p->w, XmNhelpCallback, HelpCB, NULL);
	XtAddCallback(p->w, XmNhelpCallback, HelpCB, (XtPointer)p->help);
	*/

	return (sheet);
} /* CreateSheet */

/**
 ** BusyPeriod()
 **/

void BusyPeriod ( Widget w, Boolean busy )
{

	XDefineCursor (
		XtDisplayOfObject(w),
		XtWindowOfObject(w),
#ifdef KENBOCURSOR
		(busy? XCreateFontCursor(XtDisplay(w), XC_pirate)
		     : XCreateFontCursor(XtDisplay(w), XC_shuttle) )
#else
		(busy? XCreateFontCursor(XtDisplay(w), XC_watch)
		     : XCreateFontCursor(XtDisplay(w), XC_left_ptr) )
#endif
		);
	return;
} /* BusyPeriod */

void WSMHelpCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
{
	static DmHelpAppPtr hap = NULL;
	Property *p = PropertyOf(current_sheet);

	if (hap == NULL) 
	{
		hap = (DmHelpAppPtr)DmNewHelpAppID(XtScreen(properties),
				XtWindow(properties), GetText(TXT_DESKTOP_MGR),
				pGGT(TXT_fixedString_propsTitle), DESKTOP_NODE_NAME(Desktop),
				NULL, "prefldr.icon");

		HelpFileInfo = helpFileInfo;

		(*HelpFileInfo++)[0] = pGGT(TXT_pageLabel_desktop);
		(*HelpFileInfo++)[0] = pGGT(TXT_pageLabel_settings);
		(*HelpFileInfo++)[0] = pGGT(TXT_pageLabel_setLocale);
		(*HelpFileInfo++)[0] = pGGT(TXT_pageLabel_window);
	}

	for (HelpFileInfo = helpFileInfo;
		HelpFileInfo < (helpFileInfo + XtNumber(helpFileInfo));
		HelpFileInfo++) 
	{

		if (strcmp((*HelpFileInfo)[0], p->pLabel) == 0)
		{
			DmDisplayHelpSection(DmGetHelpApp(hap->app_id),
				NULL, (*HelpFileInfo)[1], NULL);
			return;
		}
	}
	/* should never get here */
} /* end of WSMHelpCB */

void TouchPropertySheets()
{
	list_ITERATOR		I;

	Property **		p;
	Property *		q;
	int j = 0;
	
	I = list_iterator(&property);
	while (p = (Property **)list_next(&I))
	{
		q = *p;
		if (q->changed)
		{
			++j;
		}
	}
	
	if(j > 0)
	{
	    ApplyAllStatus = True;
	}
	else
	{
	    ApplyAllStatus = False;
	}
	
	return;
}

AlignLabels(Widget *labels, int num)
{
	int		i;
	Arg		args[1];
	Dimension	width;
	Dimension	maxWidth = 0;

	XtSetArg(args[0], XmNwidth, &width);

	for (i=0; i<num; i++) {
		if (labels[i] != NULL) {
			XtGetValues(labels[i], args, 1);
			if (width > maxWidth) {
				maxWidth = width;
			}
		}
	}

	XtSetArg(args[0], XmNwidth, maxWidth+10);
	for (i=0; i<num; i++) {
		if (labels[i] != NULL) {
			XtSetValues(labels[i], args, 1);
		}
	}
}

/**
 ** WSMExitProperty()
 **/

void WSMExitProperty (Display* dpy)
{
	list_ITERATOR		I;

	Property **		p;
	Property *		q;
	
	I = list_iterator(&property);
	while (p = (Property **)list_next(&I))
	{
		q = *p;
		if (q->exit)
		{
			(*q->exit) (q->closure);
		}
	}
	UpdateResources ();
	delete_RESOURCE_MANAGER();
	
	return;
} /* WSMExitProperty */

char *
GetResourceFilename()
{
	return resource_filename;
}
