#pragma ident	"@(#)dtm:dnd_util.c	1.15.1.1"

/******************************file*header********************************

    Description:
	This file contains the source code for handling an external
	drag-an-drop transaction where dtm is either the source or
	destination.
*/
						/* #includes go here	*/

#include <libgen.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>

#include <Xm/Xm.h>
#include <Xm/DragDrop.h>
#include <Xm/Frame.h>

#include "Dtm.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

static void DmIconShellDropProc(Widget w, XtPointer client_data,
		XtPointer call_data);

					/* public procedures		*/

Boolean DtmConvertSelectionProc(Widget w, Atom *selection, Atom *target,
	Atom *type_rtn, XtPointer *val_rtn, unsigned long *length_rtn,
	int *format_rtn, unsigned long *ignored_max_len, XtPointer *client_data,
	XtRequestId *ignored_req_id);

void DtmDnDFinishProc(Widget w, XtPointer client_data, XtPointer call_data);

DmDnDInfoPtr DtmDnDNewTransaction(DmWinPtr wp, DmItemPtr *ilist,
	DtAttrs attrs, Window dst_win, Atom selection,
	unsigned char operation);

void DtmDnDFreeTransaction(DmDnDInfoPtr dip);

void DmDnDRegIconShell(Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *cont_to_dispatch);

/****************************procedure*header*****************************
 * DtmConvertSelectionProc - Used in the Xt selection mechanism to convert
 * the target into the actual data.  This is one of the routines called
 * when dtm is the DnD source.
 */
Boolean
DtmConvertSelectionProc(
	Widget w,
	Atom *selection,
	Atom *target,
	Atom *type_rtn,
	XtPointer *val_rtn,
	unsigned long *length_rtn,
	int *format_rtn,
	unsigned long *ignored_max_len,
	XtPointer *client_data,
	XtRequestId *ignored_req_id)
{
	Boolean ret_val = False;
	Display *dpy = XtDisplay(w);
	String stuff;
	DmDnDInfoPtr dip;

	char *ret_str = NULL; /* return string */

	/* dip should have been cached in DmDnDNewTransaction() */
	if ((dip = DtGetData(XtScreen(w), DM_CACHE_DND, (void *)selection,
				sizeof(Atom))) == NULL) {
		/*
		 * A selection conversion request that is not part
		 * of any outstanding transactions.
		 */
		return(False);
	}

	/* target TARGETS is no longer needed because dtm exports its
	 targets to drop sites
	 */
	if (*target == OL_XA_TARGETS(dpy)) {
		Atom *everything;

#define N_ATOMS	7

		everything = (Atom *)malloc(N_ATOMS * sizeof(Atom));
		everything[0] = OL_USL_NUM_ITEMS(dpy);
		everything[1] = OL_USL_ITEM(dpy);
		everything[2] = OL_XA_FILE_NAME(dpy);
		everything[3] = OL_XA_COMPOUND_TEXT(dpy);
		everything[4] = XA_STRING;
		everything[5] = OL_XA_HOST_NAME(dpy);
		everything[6] = OL_XA_DELETE(dpy);
#ifdef NOT_USE
		everything[5] = OL_USL_OBJECT_CLASS(dpy);
		everything[6] = OL_USL_HOTSPOT_TO_OBJECT_POSITION(dpy);
		everything[7] = OL_USL_OBJECT_PROPERTIES(dpy);
#endif

		*format_rtn = 32;
		*length_rtn = (unsigned long)N_ATOMS;
		*val_rtn    = (XtPointer)everything;
		*type_rtn   = XA_ATOM;
		ret_val = True;
#undef N_ATOMS
	}
	else if (*target == OL_USL_NUM_ITEMS(dpy)) {
		int cnt;
		DmItemPtr *lp;

		/* count the # of items */
		for (cnt=0, lp=dip->ilist; *lp; lp++, cnt++) ;

		*format_rtn = 32;
		*length_rtn = 1;
		*val_rtn    = (XtPointer)malloc(sizeof(long));
		*type_rtn   = *target;
		*(long *)(*val_rtn) = cnt;
		ret_val = True;
	}
	else if (*target == OL_USL_ITEM(dpy)) {
		if (*(dip->list_idx))
			dip->ip = *(++(dip->list_idx));
		else {
			/* reset to the beginning */
			dip->list_idx = dip->ilist;
			dip->ip = *(dip->ilist);
		}
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn    = NULL;
		*type_rtn   = *target;
		ret_val = True;
	}
	else if (*target == OL_XA_FILE_NAME(dpy) || *target == XA_STRING) {
		if (dip->ip) {
			ret_str = DmObjPath(ITEM_OBJ(dip->ip));
		}
	}
	else if (*target == OL_XA_COMPOUND_TEXT(dpy)) {
		if (dip->ip) {
			XmString str;
			char *ctext;

			str = (XtPointer)XmStringCreateLocalized(
				DmObjPath(ITEM_OBJ(dip->ip)));
			ctext = XmCvtXmStringToCT(str);
			*length_rtn = strlen(ctext);
			*val_rtn    = (XtPointer)malloc(*length_rtn + 1);
			strcpy((char *)*val_rtn, ctext);
			*type_rtn   = *target;
			*format_rtn = 8;
			ret_val     = True;
		}
	} else if (*target == OL_XA_HOST_NAME(dpy))
		ret_str = DESKTOP_NODE_NAME(Desktop);
	else if (*target == OL_XA_DELETE(dpy)) {
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn    = NULL;
		*type_rtn   = *target;

		ret_val = True;
	}
#ifdef FUTURE
	else if (*target == OL_USL_OBJECT_CLASS(dpy)) {
		if (dip->ip)
			ret_str = OBJ_CLASS_NAME(ITEM_OBJ(dip->ip));
	}
	else if (*target == OL_USL_OBJECT_PROPERTIES(dpy)) {
		/* to be implemented */
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn    = (XtPointer)malloc(*length_rtn);
		*type_rtn   = *target;

		ret_val = True;
	}
	else if (*target == OL_USL_HOTSPOT_TO_OBJECT_POSITION(dpy)) {
		*format_rtn = 32;
		*length_rtn = 2;
		*val_rtn    = (XtPointer)malloc(sizeof(long) * 2);
		*type_rtn   = *target;
		*(long *)(*val_rtn) = (long)((dip->wp->views[0].itp +
			dip->idx)->icon_width) / 2;
		*((long *)(*val_rtn) + 1) = (long)(dip->op->fcp->glyph->height)
						/ 2;
		ret_val = True;
	}
#endif
	if (ret_str) {
		*length_rtn = strlen(ret_str);
		*val_rtn    = (XtPointer)malloc(*length_rtn + 1);
		strcpy((char *)*val_rtn, ret_str);
		*type_rtn   = XA_STRING;
		*format_rtn = 8;
		ret_val = True;
	}

	return(ret_val);
} /* end of DtmConvertSelectionProc */

/****************************procedure*header*****************************
 * DtmDnDFinishProc - This is the DnD source's dragDropFinishCallback.
 * Called when the whole DnD transaction has finished and is only
 * called for external drops.
 */
void
DtmDnDFinishProc(Widget w, XtPointer client_data, XtPointer call_data)
{
	ExmFlatDragDropFinishCallData *cd =
		(ExmFlatDragDropFinishCallData *)call_data;
	DmDnDInfoPtr dip;

	if ((dip = DtGetData(XtScreen(w), DM_CACHE_DND,
		(void *)&(cd->selection), sizeof(Atom))) == NULL)
		/*
		 * A selection conversion request that is not part
		 * of any outstanding transactions.
		 */
		return;

	DtmDnDFreeTransaction(dip);

} /* end of DtmDnDFinishProc */

/****************************procedure*header*****************************
 * DtmDnDNewTransaction - Called by dtm when dtm initiates a drop
 * onto an external application to set up and cache DnD information
 * that will be needed later. dip will be freed in DtmDnDFinishProc().
 */
DmDnDInfoPtr
DtmDnDNewTransaction(
	DmWinPtr wp,
	DmItemPtr *ilist,
	DtAttrs attrs,
	Window dst_win,
	Atom selection,
	unsigned char operation)
{
	DmDnDInfoPtr dip;

	if ((dip = (DmDnDInfoPtr)malloc(sizeof(DmDnDInfoRec))) == NULL)
		return(NULL);

	/*
	 * Set and cache information that will be needed by the convertProc.
	 */
	dip->wp		= wp;
	dip->ilist	= ilist;
	dip->x		= UNSPECIFIED_POS;
	dip->y		= UNSPECIFIED_POS;
	dip->timestamp	= 0;
	dip->attrs	= attrs;
	dip->user_data	= NULL;
	dip->selection	= selection;
	dip->opcode     = operation;

	/* set default item to the first item in the list */
	dip->list_idx = ilist;
	dip->ip = *ilist;

	DtPutData(XtScreen(wp->views[0].box), DM_CACHE_DND,
		(void *)&(dip->selection), sizeof(Atom), dip);
	return(dip);

} /* end of DtmDnDNewTransaction */

/****************************procedure*header*****************************
 * DtmDnDFreeTransaction - Frees cached DnD information. 
 */
void
DtmDnDFreeTransaction(DmDnDInfoPtr dip)
{
	/* remove it from cache */
	DtDelData(XtScreen(dip->wp->views[0].box), DM_CACHE_DND,
		  (void *)&(dip->selection), sizeof(Atom));

	free(dip->ilist);
	free(dip);
} /* end of DtmDnDFreeTransaction */


/****************************procedure*header*****************************
 * DmDnDRegIconShell - Registers an icon shell as a drop site.
 */
void
DmDnDRegIconShell(Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *cont_to_dispatch)
{
	int n;
	Widget frame;
	Pixmap pixmap;
	Dimension width, height;
	DmFolderWindow fwp = (DmFolderWindow)client_data;

	n = 0;
	XtSetArg(Dm__arg[n], XtNbackgroundPixmap, &pixmap); n++;
	XtSetArg(Dm__arg[n], XtNwidth,            &width); n++;
	XtSetArg(Dm__arg[n], XtNheight,           &height); n++;
	XtGetValues(w, Dm__arg, n);

	n = 0;
	XtSetArg(Dm__arg[n], XmNbackgroundPixmap, pixmap); n++;
	XtSetArg(Dm__arg[n], XmNwidth,            width); n++;
	XtSetArg(Dm__arg[n], XmNheight,           height); n++;
	frame = XtCreateManagedWidget("frame", xmFrameWidgetClass,
		w, Dm__arg, n);

	n = 0;
	XtSetArg(Dm__arg[n], XmNimportTargets, DESKTOP_DND_TARGETS(Desktop));
		n++;
	XtSetArg(Dm__arg[n], XmNnumImportTargets, XtNumber(
		DESKTOP_DND_TARGETS(Desktop))); n++;
	XtSetArg(Dm__arg[n], XmNdropSiteOperations,
		(fwp == DESKTOP_WB_WIN(Desktop) ? XmDROP_MOVE :
		XmDROP_MOVE | XmDROP_COPY | XmDROP_LINK)); n++;
	XtSetArg(Dm__arg[n], XmNdropProc, DmIconShellDropProc); n++;
	XtSetArg(Dm__arg[n], XmNuserData, client_data); n++;
	XmDropSiteRegister(frame, Dm__arg, n);

	XtRemoveEventHandler(w, (EventMask)StructureNotifyMask, False,
		DmDnDRegIconShell, NULL);

} /* end of DmDnDRegIconShell */

/****************************procedure*header*****************************
 * DmIconShellDropProc - XmNdropProc for icon shell.
 */
static void
DmIconShellDropProc(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmFolderWindow fwp;

	XtSetArg(Dm__arg[0], XmNuserData, &fwp);
	XtGetValues(w, Dm__arg, 1);
printf("DmIconShellDropProc: fwp=%x\n",fwp);

} /* end of DmIconShellDropProc */

