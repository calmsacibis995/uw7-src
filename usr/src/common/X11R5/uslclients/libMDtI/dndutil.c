/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)libMDtI:dndutil.c	1.15"

/******************************file*header********************************

    Description:
	This file contains the source code for MOTIF DnD convenience
	routines used by dtm and other desktop applications.
*/
						/* #includes go here	*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include <Dt/DesktopI.h>

#include <Xm/DragDrop.h>

#include "FIconBox.h"
#include "DnDUtilP.h"
#include "DtStubI.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static void DestroyCB(Widget, XtPointer, XtPointer);
static void FreeSrcInfo(_DmDnDSrcInfoPtr sip);
static void FreeDstInfo(DmDnDDstInfoPtr dip);
static void FreeFileList(_DmDnDDstInfoPtr dip);
static void CallAppProc(Widget w, Atom *selection, _DmDnDDstInfoPtr dip);
static void GetNumItems(Widget w, XtPointer value, unsigned long *length,
	_DmDnDDstInfoPtr dip);
static void GetFileNames(Atom *selection, Atom *type, XtPointer value,
	unsigned long *length, _DmDnDDstInfoPtr dip);

					/* public procedures		*/
void DmDnDGetOneName(Widget w, XtPointer client_data, Atom *selection,
	Atom *type, XtPointer value, unsigned long *length, int *format);
void DmDnDGetNumItems(Widget w, XtPointer client_data, Atom *selection,
	Atom *type, XtPointer value, unsigned long *length, int *format);
DmDnDDstInfoPtr DmDnDGetFileNames(Widget w, DtAttrs attrs,
	void (*proc)(), XtPointer client_data, XtPointer call_data);
Boolean DmDnDConvertSelectionProc(Widget w, Atom *selection, Atom *target,
	Atom *type_rtn, XtPointer *val_rtn, unsigned long *length_rtn,
	int *format_rtn);
void DmDnDFinishProc(Widget w, XtPointer client_data, XtPointer call_data);
DmDnDSrcInfoPtr DmDnDNewTransaction(Widget w, char **files, DtAttrs attrs,
	Atom selection, unsigned char operation, XtCallbackProc del_proc,
	XtCallbackProc done_proc, XtPointer client_data);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define TIMEOUT		5000
#define N_ATOMS		7

#define CHKPT() fprintf(stderr,"checkpoint at line %d in %s\n",__LINE__,__FILE__)

#define MEMCHECK(SIZE)  { char *__p__; CHKPT();__p__ = malloc(SIZE); free(__p__); }

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
 * DestroyCB - Called when transfer object is destroyed at the end of
 * a transfer to free dip.
 */
static void
DestroyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	_DmDnDDstInfoPtr dip = (_DmDnDDstInfoPtr)client_data;

	FreeFileList(dip);

	if (dip->targets)
		free(dip->targets);

	if (dip->cd_list)
		free(dip->cd_list);
	free(dip);

} /* end of DestroyCB */

/****************************procedure*header*****************************
 * FreeFileList - Frees list of files if it is not a static list.
 */
static void
FreeFileList(_DmDnDDstInfoPtr dip)
{
	if (!(dip->attrs & DT_B_STATIC_LIST) && dip->files) {
		register char **p = dip->files;

		while (*p)
			free(*p++);

		free(dip->files);
		dip->files = NULL;
	}
} /* end of FreeFileList */

/****************************procedure*header*****************************
	FreeSrcInfo -
 */
static void
FreeSrcInfo(_DmDnDSrcInfoPtr sip)
{
	/* remove it from cache */
	DtsDelData(XtScreen(sip->widget), DT_CACHE_DND,
		  (void *)&(sip->selection), sizeof(sip->selection));

	FreeFileList((_DmDnDDstInfoPtr)sip);
	free(sip);

} /* end of FreeSrcInfo */

/****************************procedure*header*****************************
	FreeDstInfo - Frees DnD information used by a DnD destination.
 */
static void
FreeDstInfo(DmDnDDstInfoPtr dip)
{
	FreeFileList((_DmDnDDstInfoPtr)dip);
	free((_DmDnDDstInfoPtr)dip);

} /* end of FreeDstInfo */

/****************************procedure*header*****************************
 * CallAppProc - Calls application-specified procedure.  Any client data
 * defined is used in as client_data to the procedure.  DnD information
 * used by the convenience routines (dip) is freed here.
 */
static void
CallAppProc(Widget w, Atom *selection, _DmDnDDstInfoPtr dip)
{
	if (dip->error != False)
		/* Free the partial list */
		FreeFileList(dip);

	if (dip->proc)
		(*(dip->proc))(w, dip->client_data, (XtPointer)dip);

} /* end of CallAppProc */

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * DmDnDGetOneName - Used as a transferProc to get one name.
 */
void
DmDnDGetOneName(Widget w,
	XtPointer client_data,
	Atom *selection,
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	/* get dip from client_data or from cache? */
	_DmDnDDstInfoPtr dip = (_DmDnDDstInfoPtr)client_data;

	/* Can't use w passed in because it is a windowless object.
	 * We have to get display later.  The flat icon box widget id
	 * is made available via dip for this reason.
	 */

	if ((*type == XT_CONVERT_FAIL) || (*length == 0)) {
		dip->error = True;
		XtFree(value);
	}
	else if (*type == OL_XA_FILE_NAME(XtDisplay(dip->widget)) ||
		 *type == XA_STRING) {
		dip->files     = (char **)malloc(sizeof(char *) * 2);
		dip->nitems    = 1;
		dip->nreceived = 1;
		dip->files[0]  = value;
		dip->files[1]  = NULL;
	}
	else if (*type == OL_XA_COMPOUND_TEXT(XtDisplay(dip->widget)))
	{
		char * new_value_NULL_terminated = malloc(*length+1);
		XmString str;
		
		/* Value passed in is not NULL terminated so create */
		/* a new string and copy and NULL terminate string. */
		strncpy(new_value_NULL_terminated, value, *length);
		new_value_NULL_terminated[*length] = NULL;
		str = XmCvtCTToXmString(new_value_NULL_terminated);
		dip->files     = (char **)malloc(sizeof(char *) * 2);
		dip->nitems    = 1;
		dip->nreceived = 1;
		dip->files[0]  = (char *)DmGetTextFromXmString(str);
		dip->files[1]  = NULL;
		XmStringFree(str);
		XtFree(value);
		XtFree(new_value_NULL_terminated);
	}
	else {
		/* got something unexpected */
		dip->error = True;
		XtFree(value);
	}
	CallAppProc(dip->widget, selection, dip);

} /* end of DmDnDGetOneName */

/****************************procedure*header*****************************
 * GetFileNames - Used as a secondary transferProc when the requested target
 * is OL_USL_NUM_ITEMS.  DmDnDGetNumItems() is the primary transferProc.
 * This is called (2 * number of items) times, if the transfer is completed
 * successfully.  It stores file names received in dip->files to be passed on
 * to the DnD receiver at the end of the transfer.  The procedure specified
 * by the DnD receiver is invoked when the transfer is over and is notified
 * if an error has occurred or the transfer was successful.
 *
 * (Note that to the DnD receiver, that procedure is its transferProc. It
 * has no knowledge of DmDnDGetNumItems() or this routine.)
 */
static void
GetFileNames(
	Atom *selection,
	Atom *type,
	XtPointer value,
	unsigned long *length,
	_DmDnDDstInfoPtr dip)
{
	dip->nreplies++;
	if (*type == XT_CONVERT_FAIL) {
		/* time out */
		/* But still need to wait to get all the replies from the
		 * multiple selection requests, before returning.
		 */
		if (dip->nreplies == (2 * dip->nitems)) {
err:
			XtFree(value);
			dip->error = True;
			CallAppProc(dip->widget, selection, dip);
			return;
		}
	}
	else if (value == NULL) {
		/* can't convert */
		goto err;
	} else if (*type == OL_USL_ITEM(XtDisplay(dip->widget)))
	{
		dip->nreceived++;
	} else if (*type == XA_STRING ||
		*type == OL_XA_FILE_NAME(XtDisplay(dip->widget)))
	{
		dip->files[dip->nreceived] = value;
	} else if (*type == OL_XA_COMPOUND_TEXT(XtDisplay(dip->widget)))
	{
		XmString str = XmCvtCTToXmString(value);
		dip->files[dip->nreceived] =(char *)DmGetTextFromXmString(str);
		XmStringFree(str);
	} else {
		/* got something unexpected */
		goto err;
	}
	if (dip->nreceived == dip->nitems) {
		/* all is well */
		CallAppProc(dip->widget, selection, dip);
	}

} /* end of GetFileNames */

/****************************procedure*header*****************************
 * DmDnDGetNumItems - Used as a transferProc for the OL_USL_NUM_ITEMS
 * target.  This function is called (2 * number of items) + 1 times.
 *
 * The first time it is called, target should be OL_USL_NUM_ITEMS.
 * GetNumItems() takes over in this case to set up transfer entries for
 * the requested items and submit additional transfer requests for the
 * file names via XmDropTransferAdd().
 *
 * On subsequent calls, GetFileNames() is invoked to handle the remaining
 * information obtained from the DnD source.  The information in this case
 * should be file names.
 *
 * This is done this way because MOTIF 1.2 does not provide a way to change
 * the transferProc with XmDropTransferAdd().  
 */
void
DmDnDGetNumItems(
	Widget w,
	XtPointer client_data,
	Atom *selection,
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	Atom *t;
	XtPointer *c;
	int i;
	Atom file;
	Atom next;
	int natoms;
	_DmDnDDstInfoPtr dip = (_DmDnDDstInfoPtr)client_data;

	if (*type == XT_CONVERT_FAIL) {
		dip->error = True;
		CallAppProc(dip->widget, selection, dip);
		return;
	} else if (*type == OL_USL_NUM_ITEMS(XtDisplay(dip->widget))) {
		GetNumItems(w, value, length, dip);
	} else {
		GetFileNames(selection, type, value, length, dip);
	}

} /* end of DmDnDGetNumItems */

/****************************procedure*header*****************************
 * GetNumItems -
 */
static void
GetNumItems(
	Widget w,
	XtPointer value,
	unsigned long *length,
	_DmDnDDstInfoPtr dip)
{
	Atom *t;
	XtPointer *c;
	int i;
	Atom file;
	Atom next;
	int natoms;
        XmDropTransferEntry transfers, tr;

 	if ((value == NULL) || (*length == 0)) {
		/* Couldn't get NUM_ITEMS, let try for FILE_NAME */
		dip->files = NULL;
		i = dip->nitems = 1;
		XtFree(value);
	} else {
		i = dip->nitems = *((int *)value);
		free(value);
	}
	/* compose list of targets - multiple selections */
	natoms = dip->nitems * 2;

	/* target list */
	dip->targets = t = (Atom *)malloc(sizeof(Atom) * natoms);

	/* client data list */
	dip->cd_list = c = (XtPointer *)malloc(sizeof(XtPointer) * natoms);

	/* transfer entries */
	transfers = tr =
	  (XmDropTransferEntry)malloc(sizeof(XmDropTransferEntryRec) * natoms);

	file = OL_XA_FILE_NAME(XtDisplay(dip->widget));
	next = OL_USL_ITEM(XtDisplay(dip->widget));

	for (; i--;) {
		*t++ = file;
		*t++ = next;
		*c++ = (XtPointer)dip;
		*c++ = (XtPointer)dip;
		tr->target = file;
		tr->client_data = (XtPointer)dip;
		tr++;
		tr->target = next;
		tr->client_data = (XtPointer)dip;
		tr++;
	}
	dip->nreceived = 0;

	/* Add one for a NULL terminated list */
	dip->files = (char **)calloc(sizeof(char *), dip->nitems + 1);

	XmDropTransferAdd(w, transfers, natoms);
	XtFree(transfers);

} /* end of GetNumItems */

/****************************procedure*header*****************************
 * DmDnDGetFileNames - Called from application's triggerMsgProc to initiate
 * a transfer request.  dip is freed for the application when the DnD 
 * transaction is finished. 
 */
DmDnDDstInfoPtr
DmDnDGetFileNames(
	Widget w,
	DtAttrs attrs,
	void (*transferProc)(),
	XtPointer client_data,
	XtPointer call_data)
{
	Widget transfer_object;
	int i, j;
	Arg args[10];
	_DmDnDDstInfoPtr dip;
	XmDropTransferEntryRec transferEntries[2];
	Cardinal numTransfers = 1;
	Boolean found = False;
	ExmFIconBoxTriggerMsgCD *cd = (ExmFIconBoxTriggerMsgCD *)call_data;

	if (dip = (_DmDnDDstInfoPtr)malloc(sizeof(_DmDnDDstInfo))) {
		dip->widget      = w; /* needed in transferProc later */
		dip->proc        = transferProc; /* called from transferProc*/
		dip->error       = False; /* set to True if error later */
		dip->attrs       = attrs;
		dip->client_data = client_data; /* dst win */
		dip->targets     = NULL; /* will be set in GetNumItems() */
		dip->cd_list     = NULL; /* will be set in GetNumItems() */
		dip->operation   = cd->operation;
		dip->items       = (XtPointer)(cd->item_data.items);
		dip->item_index  = cd->item_data.item_index;
	} else {
		return(NULL);
	}
	for (i = 0; i < cd->num_import_targets; i++) {
		for (j = 0; j < cd->num_export_targets; j++) {
			if (cd->import_targets[i] == cd->export_targets[j]) {
				found = True;
				transferEntries[0].target =
					cd->import_targets[i];
				break;
			}
		}
		if (found)
			break;
	}
	if (!found) {
		/* This is unlikely to happen because Xm should have ensure
		 * that there's at least one match in the target lists.
		 */
		free(dip);
		XtSetArg(args[0], XmNtransferStatus,XmTRANSFER_FAILURE);
		XtSetArg(args[1], XmNnumDropTransfers, 0);
		XmDropTransferStart(cd->drag_context, args, 2);
		return(NULL);
	}
	transferEntries[0].client_data = dip;

	XtSetArg(args[0], XmNdropTransfers, transferEntries);
	XtSetArg(args[1], XmNnumDropTransfers, numTransfers);
	if (transferEntries[0].target == OL_USL_NUM_ITEMS(XtDisplay(w))) {
		XtSetArg(args[2], XmNtransferProc, DmDnDGetNumItems);
	} else {
		XtSetArg(args[2], XmNtransferProc, DmDnDGetOneName);
	}
	transfer_object = XmDropTransferStart(cd->drag_context, args, 3);

	/* register a callback to free dip when transfer_object is destroyed */
	XtAddCallback(transfer_object, XmNdestroyCallback, DestroyCB,
		(XtPointer)dip);
	return((DmDnDDstInfoPtr)dip);

} /* end of DmDnDGetFileNames */

/****************************procedure*header*****************************
 * DmConvertSelectionProc - Performs conversion of requested information.
 */
Boolean
DmDnDConvertSelectionProc(
	Widget w,
	Atom *selection,
	Atom *target,
	Atom *type_rtn,
	XtPointer *val_rtn,
	unsigned long *length_rtn,
	int *format_rtn)
{
	Boolean ret_val = False;
	Display *dpy = XtDisplay(w);
	_DmDnDSrcInfoPtr sip;
	char *ret_str = NULL; /* return string */
	struct utsname unames; /* must define here */

	if ((sip = DtsGetData(XtScreen(w), DT_CACHE_DND, (void *)selection,
				sizeof(*selection))) == NULL)
		/*
		 * A selection conversion request that is not part
		 * of any outstanding transactions.
		 */
		return(False);

	if (*target == OL_XA_TARGETS(dpy)) {
		Atom *everything;

		everything = (Atom *)malloc(N_ATOMS * sizeof(Atom));
		everything[0] = OL_USL_NUM_ITEMS(dpy);
		everything[1] = OL_USL_ITEM(dpy);
		everything[2] = OL_XA_FILE_NAME(dpy);
		everything[3] = OL_XA_COMPOUND_TEXT(dpy);
		everything[4] = XA_STRING;
		everything[5] = OL_XA_HOST_NAME(dpy);
		everything[6] = OL_XA_DELETE(dpy);
		*format_rtn = 32;
		*length_rtn = (unsigned long)N_ATOMS;
		*val_rtn    = (XtPointer)everything;
		*type_rtn   = XA_ATOM;
		ret_val = True;
	}
	else if (*target == OL_XA_FILE_NAME(dpy) || *target == XA_STRING) {
		if (sip->fp) {
			ret_str = *(sip->fp);
		}
	}
	else if (*target == OL_XA_COMPOUND_TEXT(dpy)) {
		if (sip->fp) {
			XmString str;
			char *ctext;

			str = (XtPointer)XmStringCreateLocalized(*(sip->fp));
			ctext = XmCvtXmStringToCT(str);
			*length_rtn = strlen(ctext);
			*val_rtn    = (XtPointer)malloc(*length_rtn + 1);
			strcpy((char *)*val_rtn, ctext);
			*type_rtn   = *target;
			*format_rtn = 8;
			ret_val     = True;
		}
	} else if (*target == OL_XA_HOST_NAME(dpy)) {
		(void)uname(&unames);
		ret_str = unames.nodename;
	}
	else if (*target == OL_USL_NUM_ITEMS(dpy)) {
		register char **fp;
		register int cnt;

		/* count the # of items */
		for (cnt=0, fp=sip->files; *fp; fp++, cnt++) ;

		*format_rtn = 32;
		*length_rtn = 1;
		*val_rtn    = (XtPointer)malloc(sizeof(long));
		*type_rtn   = *target;
		*(long *)(*val_rtn) = cnt;
		ret_val = True;
	}
	else if (*target == OL_USL_ITEM(dpy)) {
		if (*(sip->fp))
			(sip->fp)++;
		else {
			/* reset to the beginning */
			sip->fp = sip->files;
		}
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn    = NULL;
		*type_rtn   = *target;
		ret_val = True;
	}
	else if (*target == OL_XA_DELETE(dpy)) {
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn    = NULL;
		*type_rtn   = *target;

		if (sip->del_proc)
			return((*(sip->del_proc))(sip->widget,
				sip->client_data, (XtPointer)*(sip->fp)));
		ret_val = False;
	}

	if (ret_str) {
		*length_rtn = strlen(ret_str);
		*val_rtn    = (XtPointer)malloc(*length_rtn + 1);
		strcpy((char *)*val_rtn, ret_str);
		*type_rtn   = XA_STRING;
		*format_rtn = 8;
		ret_val = True;
	}

	return(ret_val);
} /* end of DmDnDConvertSelectionProc */

/****************************procedure*header*****************************
 * DmDnDFinishProc - This is the initiator's dragDropFinishCallback, called
 * when the whole DnD transaction has finished. sip is removed from the
 * cache and freed here.  Application-specified doneProc is invoked.  
 */
void
DmDnDFinishProc(Widget w, XtPointer client_data, XtPointer call_data)
{
	ExmFlatDragDropFinishCallData *cd =
		(ExmFlatDragDropFinishCallData *)call_data;
	_DmDnDSrcInfoPtr sip;

	if ((sip = DtsGetData(XtScreen(w), DT_CACHE_DND, (void *)cd->selection,
				sizeof(Atom))) == NULL)
		/*
		 * A selection conversion request that is not part
		 * of any outstanding transactions.
		 */
		return;

	if (sip->done_proc) {
		(*(sip->done_proc))(sip->widget, sip->client_data,
			(XtPointer)sip);
	}
	FreeSrcInfo(sip);

} /* end of DmDnDFinishProc */

/****************************procedure*header*****************************
 * DmDnDNewTransaction - Called by a DnD source in a external drop to set
 * up information that is cached and retrieve later to be used in
 * convertProc.
 */
DmDnDSrcInfoPtr
DmDnDNewTransaction(
	Widget w,
	char **files,
	DtAttrs attrs,
	Atom selection,
	unsigned char operation,
	XtCallbackProc deleteProc,
	XtCallbackProc finishProc,
	XtPointer client_data)
{
	_DmDnDSrcInfoPtr sip;

	if ((sip = (_DmDnDSrcInfoPtr)malloc(sizeof(_DmDnDSrcInfo))) == NULL) {
		return(NULL);
	}

	/* initialize structure */
	sip->widget		= w;
	sip->files		= files;
	sip->attrs		= attrs;
	sip->selection		= selection;
	sip->opcode		= operation;
	sip->del_proc		= (Boolean(*)())deleteProc;
	sip->done_proc		= finishProc;
	sip->client_data	= client_data;

	/* set default item to the first item in the list */
	sip->fp = files;

	DtsPutData(XtScreen(w), DT_CACHE_DND, (void *)&(sip->selection),
		  sizeof(sip->selection), sip);

	return((DmDnDSrcInfoPtr)sip);

} /* end of DmDnDNewTransaction */

