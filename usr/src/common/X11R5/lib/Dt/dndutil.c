/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)Dt:dndutil.c	1.17.1.3"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <DnD/OlDnDVCX.h>
#include <DnD/OlDnDUtil.h>
#include <Dt/DesktopI.h>

	/* For libDt.a, we will use the OlDnD symbols directly, otherwise,
	 * we will get these symbols via dlopen() */
#if defined(ARCHIVE) || (!defined(SVR4) && !defined(sun))

#define GetOlDnDSyms()

#define AllocTransientAtom	OlDnDAllocTransientAtom
#define DeliverTriggerMessage	OlDnDDeliverTriggerMessage
#define DisownSelection		OlDnDDisownSelection
#define DragNDropError		OlDnDErrorDuringSelectionTransaction
#define DragNDropDone		OlDnDDragNDropDone
#define FreeTransientAtom	OlDnDFreeTransientAtom
#define OwnSelection		OlDnDOwnSelection
#define SendTriggerMessage	OlDnDSendTriggerMessage

#else /* defined(ARCHIVE) || (!defined(SVR4) && !defined(sun)) */

#include <dlfcn.h>

#define AllocTransientAtom	(*oldnd_func_table.alloc_transient_atom)
#define DeliverTriggerMessage	(*oldnd_func_table.deliver_trigger_msg)
#define DisownSelection		(*oldnd_func_table.disown_selection)
#define DragNDropError		(*oldnd_func_table.drag_n_drop_error)
#define DragNDropDone		(*oldnd_func_table.drag_n_drop_done)
#define FreeTransientAtom	(*oldnd_func_table.free_transient_atom)
#define OwnSelection		(*oldnd_func_table.own_selection)
#define SendTriggerMessage	(*oldnd_func_table.send_trigger_msg)

typedef Atom	(*type_alloc_transient_atom)(Widget);
typedef Boolean (*type_deliver_trigger_msg)(Widget, Window, Position, Position,
					Atom, OlDnDTriggerOperation, Time);
typedef void	(*type_disown_selection)(Widget, Atom, Time);
typedef void	(*type_drag_n_drop_error)(Widget, Atom, Time,
					OlDnDProtocolActionCbP, XtPointer);
typedef void	(*type_drag_n_drop_done)(Widget, Atom, Time,
					OlDnDProtocolActionCbP, XtPointer);
typedef void	(*type_free_transient_atom)(Widget, Atom);
typedef Boolean (*type_own_selection)(Widget, Atom, Time,
					XtConvertSelectionProc,
					XtLoseSelectionProc,
					XtSelectionDoneProc,
					OlDnDTransactionStateCallback,
					XtPointer);
typedef Boolean (*type_send_trigger_msg)(Widget, Window, Window, Atom,
					OlDnDTriggerOperation, Time);

typedef struct {
	type_alloc_transient_atom	alloc_transient_atom; /* be first one */
	type_deliver_trigger_msg	deliver_trigger_msg;
	type_disown_selection		disown_selection;
	type_drag_n_drop_error		drag_n_drop_error;
	type_drag_n_drop_done		drag_n_drop_done;
	type_free_transient_atom	free_transient_atom;
	type_own_selection		own_selection;
	type_send_trigger_msg		send_trigger_msg;
} OlDnDFuncRec;

static OlDnDFuncRec	oldnd_func_table = { { NULL } };

static void
GetOlDnDSyms(void)
{
		/* Important: the order has to be in sync with OlDnDFuncRec */
	static const char * const oldnd_func_names[] = {
		"OlDnDAllocTransientAtom",
		"OlDnDDeliverTriggerMessage",
		"OlDnDDisownSelection",
		"OlDnDErrorDuringSelectionTransaction",
		"OlDnDDragNDropDone",
		"OlDnDFreeTransientAtom",
		"OlDnDOwnSelection",
		"OlDnDSendTriggerMessage",
	};

	void *		foo;

	if (oldnd_func_table.alloc_transient_atom)
		return;

		/* Assume that the app will be linked with libOlit/libDnD,
		 * which should be the case any way, otherwise we will
		 * have to dlopen(libDnD.so) */
#ifdef RTLD_GLOBAL
	if ((foo = dlopen(NULL, RTLD_LAZY)) == (void *)NULL)
#else
	if ((foo = dlopen("libDnD.so", RTLD_LAZY)) == (void *)NULL)
#endif
	{
		fprintf(stderr, "%s\n", dlerror());
		exit(1);	/* dlopen problem */
	}
	else
	{
		typedef void	(*Func)(void);

		register int	i;
		Func *		func;

		func = (Func *)&oldnd_func_table.alloc_transient_atom;
		for (i = 0; i < XtNumber(oldnd_func_names); i++)
		{
#define TAG	oldnd_func_names[i]

			if ((*func = (Func)dlsym(foo, TAG)) == NULL)
			{
				fprintf(stderr, "%s\n", dlerror());
				dlclose(foo);
				oldnd_func_table.alloc_transient_atom = NULL;
				exit(2);	/* dlsym problem */
			}
			func++;
#undef TAG
		}
	}
}

#endif /* defined(ARCHIVE) || (!defined(SVR4) && !defined(sun)) */

#define TIMEOUT		5000

static void
FreeFileList(dip)
Dt__DnDInfoPtr dip;
{
	if (!(dip->attrs & DT_B_STATIC_LIST) && dip->files) {
		register char **p = dip->files;

		while (*p)
			free(*p++);

		free(dip->files);
		dip->files = NULL;
	}
}

static void
CallAppProc(w, selection, dip)
Widget		w;
Atom		*selection;
Dt__DnDInfoPtr	dip;
{

	if (dip->error != False)
		/* Free the partial list */
		FreeFileList(dip);

	if (dip->proc)
		(*(dip->proc))(w, dip->client_data, (XtPointer)dip);

	if (dip->send_done)
	{
		GetOlDnDSyms();
		DragNDropDone(w, *selection, dip->timestamp, NULL, NULL);
	}

	if (dip->error && !dip->send_done && dip->send_error)
	{
		GetOlDnDSyms();
		DragNDropError(w, *selection, dip->timestamp, NULL, NULL);
	}

	FreeFileList(dip);

	if (dip->targets)
		free(dip->targets);

	if (dip->cd_list)
		free(dip->cd_list);
	free(dip);
}

static void
GetOneName(w, client_data, selection, type, value, length, format)
Widget		w;
XtPointer	client_data;
Atom *		selection;
Atom *		type;
XtPointer	value;
unsigned long *	length;
int *		format;
{
	Dt__DnDInfoPtr	dip = (Dt__DnDInfoPtr)client_data;

	if ((*type == XT_CONVERT_FAIL) || (*length == 0)) {
		dip->error = True;
		XtFree(value);
	}
	else if (*type == OL_XA_FILE_NAME(XtDisplay(w)) || *type == XA_STRING)
	{
		dip->files     = (char **)malloc(sizeof(char *) * 2);
		dip->nitems    = 1;
		dip->nreceived = 1;
		dip->files[0]  = value;
		dip->files[1]  = NULL;
	}
	else {
		/* got something unexpected */
		dip->error = True;
		XtFree(value);
	}

	CallAppProc(w, selection, dip);
}

static void
GetFileNames(w, client_data, selection, type, value, length, format)
Widget		w;
XtPointer	client_data;
Atom *		selection;
Atom *		type;
XtPointer	value;
unsigned long *	length;
int *		format;
{
	Atom COMPOUND_TEXT = XInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
	Dt__DnDInfoPtr	dip = (Dt__DnDInfoPtr)client_data;

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
			CallAppProc(w, selection, dip);
			return;
		}
	}
	else if (value == NULL)
		/* can't convert */
		goto err;
	else if (*type == OL_USL_ITEM(XtDisplay(w))) {
		dip->nreceived++;
	} else if (*type == OL_XA_FILE_NAME(XtDisplay(w)) ||
		*type == XA_STRING) {
		dip->files[dip->nreceived] = value;
	} else if (*type == COMPOUND_TEXT)
	{
		String eucstring;

		if (DtCTToEuc(XtDisplay(w), value, &eucstring))
			dip->files[dip->nreceived] = eucstring;
		else
			goto err;
	}
	else {
		/* got something unexpected */
		goto err;
	}

	if (dip->nreceived == dip->nitems)
		CallAppProc(w, selection, dip);
}

static void
GetNumItems(w, client_data, selection, type, value, length, format)
Widget		w;
XtPointer	client_data;
Atom *		selection;
Atom *		type;
XtPointer	value;
unsigned long *	length;
int *		format;
{
	Dt__DnDInfoPtr	dip = (Dt__DnDInfoPtr)client_data;
	Atom		*t;
	XtPointer	*c;
	int		i;
	Atom		file;
	Atom		next;
	int		natoms;
	
	if (*type == XT_CONVERT_FAIL) {
		dip->error = True;
		CallAppProc(w, selection, dip);
		return;
	}

 	if ((value == NULL) || (*length == 0)) {
		/* since cannot convert NUM_ITEMS, just try FILENAME */
		dip->files = NULL;
		XtGetSelectionValue(w, *selection,
				OL_XA_FILE_NAME(XtDisplay(w)),
				GetOneName, (XtPointer)dip, dip->timestamp);
		XtFree(value);
		return;
	}

	i = dip->nitems = *((int *)value);
	free(value);

	/* compose list of targets */
	natoms = dip->nitems * 2;
	dip->targets = t = (Atom *)malloc(sizeof(Atom) * natoms);
	dip->cd_list = c = (XtPointer *)malloc(sizeof(XtPointer) * natoms);

	file = OL_XA_FILE_NAME(XtDisplay(w));
	next = OL_USL_ITEM(XtDisplay(w));
		
	for (; i--;) {
		*t++ = file;
		*t++ = next;
		*c++ = client_data;
		*c++ = client_data;
	}

	dip->nreceived = 0;

	/* Add one for a NULL terminated list */
	dip->files = (char **)calloc(sizeof(char *), dip->nitems + 1);

	/*
	 * Set the timeout factor proportional to the # of items.
	 */
	XtSetSelectionTimeout(TIMEOUT * dip->nitems);
	XtGetSelectionValues(w, *selection, dip->targets, natoms, GetFileNames,
			 dip->cd_list, dip->timestamp);
}

DtDnDInfoPtr
DtGetFileNames( Widget		w,
		Atom		selection,
		Time		timestamp,
		Boolean		send_done,
		void		(*proc)(),
		XtPointer	client_data)
{
	Dt__DnDInfoPtr dip;

	if (dip = (Dt__DnDInfoPtr)malloc(sizeof(Dt__DnDInfo))) {
		dip->files		= NULL;
		dip->timestamp		= timestamp;
		dip->send_done		= send_done;
		dip->send_error		= False;
		dip->proc		= proc;
		dip->error		= False;
		dip->client_data	= client_data;
		dip->attrs		= 0;
		dip->targets		= NULL;
		dip->cd_list		= NULL;

		XtGetSelectionValue(w, selection,
				    OL_USL_NUM_ITEMS(XtDisplay(w)),
				    GetNumItems, (XtPointer)dip, timestamp);
	}
	else
		dip = NULL;

	/*
	 * We want to set send_error to True only here in case the
	 * above XtGetSelectionValue eventually calls CallAppProc().
	 * In that case, if dip->error is True we do not want to
	 * terminate the DnD since the application may still be
	 * trying another Target conversion. One example of this
	 * case is dtedit in DropNotify() function in editor.c.
	 * We use _OlTextEditTriggerNotify()  of the OpenLook
	 * Text Editor Widget.
	 */

	if (dip)
		dip->send_error = True;

	return((DtDnDInfoPtr)dip);
}

/***********************************************************/
/**** Routines used by owners of drag&drop transactions ****/
/***********************************************************/

#define N_ATOMS		6

static void
FreeTransaction(sip)
Dt__DnDSendPtr sip;
{

	/* remove it from cache */
	DtDelData(XtScreen(sip->widget), DT_CACHE_DND,
		  (void *)&(sip->selection), sizeof(sip->selection));

	GetOlDnDSyms();
	DisownSelection(sip->widget, sip->selection, CurrentTime);
	FreeTransientAtom(sip->widget, sip->selection);

	FreeFileList(sip);
	free(sip);
} /* end of FreeTransaction */

/*
 * ConvertSelectionProc -
 */
static Boolean
ConvertSelectionProc( Widget		w,
			Atom *		selection,
			Atom *		target,
			Atom *		type_rtn,
			XtPointer *	val_rtn,
			unsigned long *	length_rtn,
			int *		format_rtn)
{
	Boolean			ret_val = False;
	Display *		dpy = XtDisplay(w);
	Dt__DnDSendPtr		sip;
	char			*ret_str = NULL; /* return string */
	struct utsname		unames; /* must define here */
	Atom COMPOUND_TEXT = XInternAtom(dpy, "COMPOUND_TEXT", False);

	if ((sip = DtGetData(XtScreen(w), DT_CACHE_DND, (void *)selection,
				sizeof(*selection))) == NULL)
		/*
		 * A selection conversion request that is not part
		 * of any outstanding transactions.
		 */
		return(False);

	if (*target == OL_XA_TARGETS(dpy)) {
		Atom *		everything;

		everything = (Atom *)malloc(N_ATOMS * sizeof(Atom));
		everything[0] = OL_XA_FILE_NAME(dpy);
		everything[1] = OL_XA_HOST_NAME(dpy);
		everything[2] = OL_USL_NUM_ITEMS(dpy);
		everything[3] = OL_USL_ITEM(dpy);
		everything[4] = XA_STRING;
		everything[5] = COMPOUND_TEXT;
		*format_rtn = 32;
		*length_rtn = (unsigned long)N_ATOMS;
		*val_rtn    = (XtPointer)everything;
		*type_rtn   = XA_ATOM;
		ret_val = True;
	}
	else if (*target == OL_XA_FILE_NAME(dpy) || *target == XA_STRING) {
		if (sip->fp)
			ret_str = *(sip->fp);
	}
	else if (*target == OL_XA_HOST_NAME(dpy)) {
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
	else if (*target == COMPOUND_TEXT) {
	        *length_rtn = DtEucToCT(dpy, *sip->fp, (String *)val_rtn);
		*format_rtn = 8;
		*type_rtn   = *target;
		ret_val     = True;
	}

	else if (*target == OL_XA_DELETE(dpy)) {
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn    = NULL;
		*type_rtn   = *target;

		if (sip->del_proc)
			return((*(sip->del_proc))(sip->widget, sip->client_data,
				(XtPointer)*(sip->fp)));
		ret_val = False;
	}

	if (ret_str) {
		*format_rtn = 8;
		*length_rtn = strlen(ret_str);
		*val_rtn    = (XtPointer)malloc(*length_rtn + 1);
		*type_rtn   = XA_STRING;
		strcpy((char *)*val_rtn, ret_str);
		ret_val = True;
	}

	return(ret_val);
} /* end of ConvertSelectionProc */

/*
 * TransactionStateProc -
 *
 *	This should be the only place to disown/free "transient"
 *	because this is the responsibility of "resource". This also
 *	implies all relevant info/resources should also be freed here
 *	(included the icon etc.). Without the assumption above (i.e.,
 *	freeing the transient), the comments about "transient" in the
 *	DroppedOnSomething() is not hold.
 *
 */
static void
TransactionStateProc(Widget			w,
		     Atom			selection,
		     OlDnDTransactionState	state,
		     Time			timestamp,
		     XtPointer			closure)
{
	Dt__DnDSendPtr sip;

	if ((sip = DtGetData(XtScreen(w), DT_CACHE_DND, (void *)&selection,
				sizeof(selection))) == NULL)
		/*
		 * A selection conversion request that is not part
		 * of any outstanding transactions.
		 */
		return;

	switch(state) {
	case OlDnDTransactionDone:
	case OlDnDTransactionRequestorError:
	case OlDnDTransactionRequestorWindowDeath:
		if (sip->state_proc) {
			sip->state = (int)state;
			(*(sip->state_proc))(sip->widget, sip->client_data,
						 (XtPointer)sip);
		}
		FreeTransaction(sip);
		break;
	}
} /* end of DmTransactionStateProc */

DtDnDSendPtr
DtNewDnDTransaction(Widget				w,
		    char				**files,
		    DtAttrs				attrs,
		    Position				root_x,
		    Position				root_y,
		    Time				drop_timestamp,
		    Window				dst_win,
		    int					hint,
		    XtCallbackProc			del_proc,
		    XtCallbackProc			state_proc,
		    XtPointer				client_data)
{
	Dt__DnDSendPtr sip;

	if ((sip = (Dt__DnDSendPtr)malloc(sizeof(Dt__DnDSend))) == NULL)
		return(NULL);

	GetOlDnDSyms();
	if ((sip->selection = AllocTransientAtom(w)) == (Atom)None) {
		/* couldn't allocate a free transient atom */
		free(sip);
		return(NULL);
	}

	/* own the transient id */
	if (OwnSelection(w, sip->selection,
			      drop_timestamp,
			      ConvertSelectionProc,
			      (XtLoseSelectionProc)NULL,
			      (XtSelectionDoneProc)NULL,
			      TransactionStateProc,
			      (XtPointer)sip) == False) {
		/* failed to own selection */
		FreeTransientAtom(w, sip->selection);
		free(sip);
		return(NULL); /* for now */
	}

	sip->hint = hint;

	/*
	 * Must initialize and add sip to the cache first!
	 * You see, DeliverTriggerMessage() will shortcircuit transactions
	 * that go to windows in the same process space.
	 */

	/* initialize structure */
	sip->widget		= w;
	sip->files		= files;
	sip->drop_timestamp	= drop_timestamp;
	sip->attrs		= attrs;
	sip->del_proc		= (Boolean(*)())del_proc;
	sip->state_proc		= state_proc;
	sip->client_data	= client_data;

	/* set default item to the first item in the list */
	sip->fp = files;

	DtPutData(XtScreen(w), DT_CACHE_DND, (void *)&(sip->selection),
		  sizeof(sip->selection), sip);

	if (attrs & DT_B_SEND_EVENT) {
		if (SendTriggerMessage(w,
			RootWindowOfScreen(XtScreen(w)),
			dst_win,
			sip->selection,
			(OlDnDTriggerOperation)(sip->hint),
			drop_timestamp) == False) {
				/* failed to deliver trigger message */
				FreeTransaction(sip);
				return(NULL); /* for now */
		}
	}
	else {
		if (DeliverTriggerMessage(w,
			RootWindowOfScreen(XtScreen(w)),
			root_x,
			root_y,
			sip->selection,
			(OlDnDTriggerOperation)(sip->hint),
			drop_timestamp) == False) {
			/* failed to deliver trigger message */
			FreeTransaction(sip);
			return(NULL); /* for now */
		}
	}

	return((DtDnDSendPtr)sip);
} /* end of DtNewDnDTransaction */
