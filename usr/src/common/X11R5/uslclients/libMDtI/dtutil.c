/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libMDtI:dtutil.c	1.11.1.2"

#include <wctype.h>	/* for towupper() */
#include <Xm/Xm.h>
#include "Xt/IntrinsicI.h"

#include "DtI.h"
Arg  Dm__arg[ARGLIST_SIZE];

/****************************procedure*header*****************************
 * DmModalCascadeActive - Returns True if a widget is on a grab list;
 * otherwise False.
 */
Boolean
DmModalCascadeActive(Widget w)
{
	XtGrabList *grabListPtr = 
		_XtGetGrabList(_XtGetPerDisplayInput(XtDisplay(w)));

	return(*grabListPtr != (XtGrabList)0);

} /* DmModalCascadeActive */

/****************************procedure*header*****************************
	DmGetTextFromXmString - Returns a copy of text from a XmString.
	The Caller is responsible for freeing the value returned, if non-NULL.
 */
char *
DmGetTextFromXmString(XmString str)
{
	return((char *)_XmStringGetTextConcat(str));

} /* end of GetTextFromXmString */


typedef struct {
	unsigned int		mne_prefix;
	unsigned int		num_mne_info;
	DmMnemonicInfo		mne_info;
	Boolean			with_grab;
} MneEHData;

static void		GatherAllWidgets(
				Widget, WidgetList, Cardinal, Cardinal *);
static unsigned int	GetAltMask(Display *);
static Widget		GetShellWidget(Widget);
static void		MneCB(Widget, XtPointer, XtPointer);
static void		MneEH(Widget, XtPointer, XEvent *, Boolean *);
static int		Strnicmp(register const char *, register const char *,
						register int);
static MneEHData *	SetupMne(Widget, unsigned int, Boolean,
						DmMnemonicInfo, Cardinal,
						WidgetList, Cardinal);

static int
Strnicmp(register const char * str1, register const char * str2,
							register int len)
{
	register int	c1;
	register int	c2;

	while ((--len >= 0) &&
		((c1 = towupper(*str1)) == (c2 = towupper(*str2++))))
		if (*str1++ == '\0')
			return(0);
	return(len < 0 ? 0 : (c1 - c2));
}

static void
MneCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MneEHData *	cd = (MneEHData *)client_data;
	register int	i;

	for (i = 0; i < cd->num_mne_info; i++)
		XtFree((char *)cd->mne_info[i].mne);

	XtFree((char *)cd->mne_info);
	XtFree((char *)cd);
}

static void
MneEH(Widget w, XtPointer client_data, XEvent * xe, Boolean * cont_to_dispatch)
{
	MneEHData *	cd = (MneEHData *)client_data;

	XComposeStatus	status;
	XKeyEvent *	key = (XKeyEvent *)xe;
	char		value[32];
	int		n;

	register int	i;

#define THIS_STATE	cd->mne_prefix

	if (key->state != THIS_STATE)
		return;

	key->state = 0;

	if ((n = XLookupString(key, value, sizeof(value), (KeySym *)NULL,
							&status)) > 0) {

#define THIS_MANY	cd->num_mne_info
#define THIS_MNE	cd->mne_info[i].mne
#define THIS_MNE_LEN	cd->mne_info[i].mne_len
#define THIS_OP		cd->mne_info[i].op
#define THIS_WIDGET	cd->mne_info[i].w
#define THIS_CB		cd->mne_info[i].cb
#define THIS_CB_DATA	cd->mne_info[i].cd

		value[n] = 0;

		for (i = 0; i < THIS_MANY; i++) {
			if (n == THIS_MNE_LEN &&
			    !Strnicmp(value, (char *)THIS_MNE, n)) {

				if (XtIsManaged(THIS_WIDGET) &&
				    XtIsSensitive(THIS_WIDGET)) {

					if (THIS_OP & DM_B_MNE_PASS_KEY)
						*cont_to_dispatch = True;
					else
						*cont_to_dispatch = False;

					if (THIS_OP & DM_B_MNE_GET_FOCUS)
						XmProcessTraversal(
							THIS_WIDGET,
							XmTRAVERSE_CURRENT);

					if (THIS_OP & DM_B_MNE_ACTIVATE_BTN)
						XtCallCallbacks(
							THIS_WIDGET,
							XmNactivateCallback,
								/* call_data*/
							(XtPointer)NULL);

					if (THIS_OP & DM_B_MNE_ACTIVATE_CB)
						(*THIS_CB)(THIS_WIDGET,
							THIS_CB_DATA,
								/* call_data*/
							(XtPointer) NULL);
				} else if (THIS_OP & DM_B_MNE_KEEP_LOOKING) {
					continue;
				}
				break;
			}
		}

	}

	key->state = THIS_STATE;

#undef THIS_STATE
#undef THIS_MANY
#undef THIS_MNE
#undef THIS_MNE_LEN
#undef THIS_OP
#undef THIS_WIDGET
#undef THIS_CB
#undef THIS_CB_DATA
}

static void
GatherAllWidgets(Widget w, WidgetList wids, Cardinal wids_size,
							Cardinal * how_many)
{
	Arg		args[2];
	Boolean		is_manager;
	Cardinal	num_kids;
	WidgetList	kids;
	register int	i;

	XtSetArg(args[0], XmNchildren, &kids);
	XtSetArg(args[1], XmNnumChildren, &num_kids);
	XtGetValues(w, args, 2);

	for (i = 0; i < num_kids; i++) {
		is_manager = False;
		if (XmIsPrimitive(kids[i]) ||
		    (is_manager = XmIsManager(kids[i]))) {

			if (*how_many < wids_size)
				wids[*how_many] = kids[i];

			(*how_many)++;

			if (is_manager)
				GatherAllWidgets(
					kids[i], wids, wids_size, how_many);
		}
	}
}

static unsigned int
GetAltMask(Display * dpy)
{
	static unsigned int	mne_prefix;
	register int		i;

	if (mne_prefix == 0) {	/* first time */

#define KS	XKeycodeToKeysym(dpy, map->modifiermap[k], 0)

		XModifierKeymap *	map;
		char *			nm;
		register int		j, k;

		map = XGetModifierMapping(dpy);

		mne_prefix = Mod1Mask;	/* fall back... */
		k = 0;
		for (i = 0; i < 8; i++) {
			for (j = 0; j < map->max_keypermod; j++) {
				if (map->modifiermap[k]) {

						/* Search for XK_ALT_L */
					if ((nm = XKeysymToString(KS)) &&
					    !strcmp("Alt_L", nm)) {
						mne_prefix = (1 << i);
						break;
					}
				}
				k++;
			}
			if (j != map->max_keypermod)	/* got it */
				break;
		}

#undef KS
	}

	return(mne_prefix);
}

static Widget
GetShellWidget(Widget w)
{
	Widget	shell = w;

	while (shell != NULL && !XtIsShell(shell))
		shell = XtParent(shell);

	return(shell);
}

static MneEHData *
SetupMne(Widget shell, unsigned int mne_prefix, Boolean with_grab,
			DmMnemonicInfo mne_info, Cardinal num_mne_info,
			WidgetList wids, Cardinal num_wids)
{
	MneEHData *	this_data;
	register int	i;

		/* Malloc and initialize the client_data for MneEH/MneCB */
	this_data = (MneEHData *)XtMalloc(sizeof(MneEHData));
	this_data->mne_info = (DmMnemonicInfo)XtMalloc(
				sizeof(DmMnemonicInfoRec) * num_mne_info);

		/* Determine AltMask! */
	this_data->mne_prefix	= mne_prefix;
	this_data->num_mne_info = num_mne_info;
	this_data->with_grab	= with_grab;

	for (i = 0; i < num_mne_info; i++) {
		this_data->mne_info[i] = mne_info[i];
		this_data->mne_info[i].mne = (unsigned char *)strdup(
						(char *)mne_info[i].mne);
	}

		/* Install the MneEH to the XtListHead */
	for (i = 0; i < num_wids; i++)
		XtInsertRawEventHandler(
			wids[i], KeyPressMask, False,
			MneEH, (XtPointer)this_data, XtListHead);

		/* Install MneCB */
	XtAddCallback(shell, XmNdestroyCallback, MneCB, (XtPointer)this_data);

	return((XtPointer)this_data);
}

/*
 * DmRegisterMnemonic - this procedure enables the mnemonic capability
 *	for Motif apps. This routine assumes that the given info are
 *	correct and the routine will not perform any error checking.
 *
 *	This routine returns a non-zero handle if the call was successful.
 *	With this handle, DmUpdateMnemonic() (if needed later on) can be
 *      implemented easily.
 *
 *		w		- Any widget/gadget within the shell.
 *				  This widget/gadget will be used to
 *				  locate the shell widget. You shall
 *				  pass the shell widget if it is possible!
 *		mne_info	- specify mnemoic information, see
 *				  DesktopP.h:DmMnemonicInfo for more info.
 *		num_mne_info	- specify number of mne_info.
 *
 *	Constrains - This API can't handle the following situations
 *
 *		a. You must call this routine after all children are
 *		   created, otherwise this routine won't be able to
 *		   catch all children within the shell, also see b.
 *
 *		b. If an appl needs to destroy/create object(s) on demand
 *		   within the `shell', and these objects appear (destroy
 *		   case) or do not appear (create case) in the initial
 *		   lists (mne_info).
 *
 *		c. This API won't handle the mnemonic visuals, they come
 *		   from Motif/libXm (usually, XmLabel subclasses).
 *
 *		d. DM_B_MNE_ACTIVATE_BTN can't build a reasonable call_data,
 *		   applications will have to check `call_data' if the CB code
 *		   uses `call_data'.
 *
 *	Hints -
 *
 *		a. To animate OLIT caption layout, usually, you can
 *		   create a XmLabel widget/gadget as a caption (say A)
 *		   and a 2nd widget/gadget as the caption child (say B).
 *		   To enable mnemonic in this case, you can borrow
 *		   mnemonic visual from XmLabel (A) but place B in
 *		   `mne_info'.
 *
 *		b. You may want to use DM_B_MNE_ACTIVATE_CB for Help
 *		   button for avoiding `Can't find per display info'
 *		   error. This is because libXm is doing too much
 *		   checking in this case.

 *		c. Try to use DmRegisterMnemonicWithGrab if you hit
 *		   the Constrain a and or b and if mne_info is still
 *		   same as before. If mne_info is changed, then
 *		   we will have to enable DmUpdateMnemonic!!!
 *
 *	You shall free up mne_info after the call if necessary.
 *
 * Logic - the routine will perform following work:
 *
 *		a. determine AltMask if it isn't initialized yet..
 *		b. locate the shell widget id.
 *		c. locate all XmManager and XmPrimitive widgets within
 *		   this shell, wids[].
 *		d. malloc client_data for MneEH() based on mne_info and
 *		   num_mne_info.
 *		e. add event handler to all widgets in wids[].
 *		f. add XmNdestroyCallback to the `shell' so that
 *		   client_data for MneEH() can be freed() when this
 *		   shell is destroyed (see d).
 *
 * Notes - it's difficult to say that whether DmRegisterMnemonic is
 *	better than DmRegisterMnemonicWithGrab from performance points.
 *	WithGrab version puts passive grabs on the toplevel window, so
 *	the startup time may be slower if num_mne_info is large (each
 *	causes 3 server round trips, I think). However, the advantage
 *	is that the caller won't need to worry about the constrain
 *	a and/or b if WithGrab version is used and if mne_info doesn't
 *	change.
 *
 *	DmUpdateMnemonic interface can be the following (if we want to
 *	enable it):
 *		extern XtPointer DmUpdateMnemonic(
 *					Widget		w,
 *					DmMnemonicInfo	new_mne_info,
 *					Cardinal	num_mne_info,
 *					XtPointer	handle
 *		);
 */
extern XtPointer
DmRegisterMnemonic(Widget w, DmMnemonicInfo mne_info, Cardinal num_mne_info)
{
#define WIDS_SIZE		100

	MneEHData *		this_data;
	Widget			shell;
	Widget			local_buf[WIDS_SIZE];
	WidgetList		wids;
	Cardinal		num_wids;
	unsigned int		mne_prefix;

		/* Determine AltMask */
	mne_prefix = GetAltMask(XtDisplay(w));

		/* Locate the shell */
	shell = GetShellWidget(w);

		/* Build wids[] */
	wids = local_buf;
	num_wids = 0;
	GatherAllWidgets(shell, wids, WIDS_SIZE, &num_wids);

		/* Try again if it's more than WIDS_SIZE!!
		 *
		 * We shall adjust WIDS_SIZE if the block below
		 * got exec'd often... */
	if (num_wids >= WIDS_SIZE) {
		Cardinal	new_size = num_wids;	/* save it */

		num_wids = 0;	/* have to reset it to zero... */
		wids = (WidgetList)XtMalloc(sizeof(Widget) * new_size);
		GatherAllWidgets(shell, wids, new_size, &num_wids);
	}

	this_data = SetupMne(shell, mne_prefix, False /* with_grab */,
				mne_info, num_mne_info, wids, num_wids);

	if (wids != local_buf)
		XtFree((char *)wids);

	return((XtPointer)this_data);
#undef WIDS_SIZE
}

/* See notes in DmRegisterMnemonic() */
extern XtPointer
DmRegisterMnemonicWithGrab(
		Widget w, DmMnemonicInfo mne_info, Cardinal num_mne_info)
{
	MneEHData *	this_data;
	KeyCode		kc;
	KeySym		ks;
	Widget		shell;
	register int	i, j;
	unsigned int	mne_prefix;

		/* Determine AltMask */
	mne_prefix = GetAltMask(XtDisplay(w));

		/* Locate the shell */
	shell = GetShellWidget(w);

		/* Passive grab all mnemonic keys with AltMask */
	for (i = 0, j = 0; i < num_mne_info; i++) {

#define MNE	(char *)mne_info[i].mne

		if ((ks = XStringToKeysym(MNE)) != NoSymbol &&
		    (kc = XKeysymToKeycode(XtDisplay(shell), ks)) != NoSymbol) {
			XtGrabKey(shell, kc, mne_prefix,
					True, GrabModeAsync, GrabModeAsync);
			j++;
		}
	}
#undef MNE

	if (!j)
		return;

	this_data = SetupMne(shell, mne_prefix, True /* with_grab */,
				mne_info, num_mne_info, &shell, 1);

	return((XtPointer)this_data);
}

/* 
 * DmXYToIconLabel - Returns the label of an item in a flat icon box widget
 * given its x and y positions.  Value returned must not be freed by caller.
 * Returns NULL on failure.
 */
extern _XmString
DmXYToIconLabel(Widget w, WidePosition x, WidePosition y)
{
	int nitems;
	DmItemPtr itp;
	DmItemPtr items = NULL;
	Cardinal item_index = ExmFlatGetItemIndex(w, x, y);
#if 0
	/* ignore icon sensitivity  */
	Cardinal item_index = ExmFlatGetIndex(w, x, y, True);
#endif

	if (item_index == ExmNO_ITEM) {
		fprintf(stderr,
		"DmXYToIconLabel: Couldn't get item index for %d %d\n", x, y);
		return(NULL);
	}
	XtSetArg(Dm__arg[0], XmNitems, &items);
	XtSetArg(Dm__arg[1], XmNnumItems, &nitems);
	XtGetValues(w, Dm__arg, 2);

	if (items == NULL) {
		fprintf(stderr,
		"DmXYToIconLabel: Couldn't get item index for %d %d\n", x, y);
		return(NULL);
	}
	itp = items + item_index;
	if (ITEM_LABEL(itp))
		return((_XmString)ITEM_LABEL(itp));
	else {
		_XmString label;
		XtSetArg(Dm__arg[0], XmNlabelString, &label);
		ExmFlatGetValues(w, item_index, Dm__arg, 1);
		return(label);
	}

} /* end of DmXYToIconLabel */

/*
 * DmIconLabelToXY - Returns the x and y positions of an item in a flat icon 
 * box widget given its label.  Returns -1 on failure.
 */
extern int
DmIconLabelToXY(Widget w, _XmString icon_name, WidePosition *x, WidePosition *y)
{
	DmItemPtr itp;
	DmItemPtr items = NULL;
	Cardinal nitems;
	Cardinal item_index;
	XmString xmstr1, xmstr2;
	int i;

	if (w == NULL || icon_name == NULL) {
		fprintf(stderr, "DmconNameToXY: Couldn't get x and y\n");
		return(-1);
	}
	XtSetArg(Dm__arg[0], XmNitems, &items);
	XtSetArg(Dm__arg[1], XmNnumItems, &nitems);
	XtGetValues(w, Dm__arg, 2);

	if (items == NULL || nitems == 0) {
		fprintf(stderr, "DmconNameToXY: Couldn't get x and y\n");
		return(-1);
	}
	xmstr1 = _XmStringCreateExternal(NULL, icon_name);
	for (i=0, itp = items; i < nitems; i++, itp++) {
		xmstr2 = _XmStringCreateExternal(NULL, (_XmString)ITEM_LABEL(itp));
		if (DmCompareXmStrings(xmstr1, xmstr2) == 0)
		{
			Dimension width, height;

			ExmFlatGetItemGeometry(w, i, x, y, &width, &height);
			XmStringFree(xmstr1);
			XmStringFree(xmstr2);
			return(0);
		}
		XmStringFree(xmstr2);
	}
	XmStringFree(xmstr1);
	return(-1);

} /* end of DmIconLabelToXY */

