#ident	"@(#)debugger:libmotif/common/DND.C	1.2"

#include "UI.h"
#include "Component.h"
#include "Machine.h"

#include "DND.h"
#include <Xm/List.h>
#include <Xm/ListP.h>
#include <Xm/DragDrop.h>

#include <stdio.h>

#if DEBUG > 1
#define DEBUG_DND(x)	fprintf x
#else
#define DEBUG_DND(x)
#endif

// Currently only support DND in the following cases:
//	1) from Table (List widget) to Table (List widget)
//	2) from Selection_list (List widget) to Text_display (Text widget)

struct Envelop_data
{
	XtPointer			save;
	Atom				atom;
	XtConvertSelectionIncrProc	cproc;
	Window				win;
	int				item_pos;
};

static Boolean
drag_convert(Widget w, Atom *selection, Atom *target, Atom *typeRtn,
	XtPointer *valueRtn, unsigned long *lengthRtn, int *formatRtn,
	unsigned long *max_lengthRtn, XtPointer client_data,
	XtRequestId *request_id)
{
	Envelop_data	*edata;
	int		ret;

	// extract envelop data & restore saved client data
	XtVaGetValues(w, XmNclientData, &edata, 0);
	XtVaSetValues(w, XmNclientData, edata->save, 0);
	if (*target == edata->atom)
	{
		char		buf[MAX_INT_DIGITS+1+MAX_INT_DIGITS+1];
		XmString	buf_str;
		int		len;
		char		*ctext;
		char		*passtext;

		// transmit source window id and item position to target
		sprintf(buf, "%d %d", edata->win, edata->item_pos);
		DEBUG_DND((stderr, "drag_convert: src_win %d item_pos %d", 
			edata->win, edata->item_pos));
		buf_str = XmStringCreateLocalized(buf);
		ctext = XmCvtXmStringToCT(buf_str);
		XmStringFree(buf_str);
		len = strlen(ctext) + 1;
		passtext = new char[len];
		memcpy(passtext, ctext, len);
		*typeRtn = edata->atom;
		*valueRtn = (XtPointer)passtext;
		*lengthRtn = len;
		*formatRtn = 8;
		ret = True;
	}
	else
		// not dropping on a designated drop site, so
		// invoke the default converter
		ret = (*edata->cproc)(w, selection, target, typeRtn, valueRtn, 
			lengthRtn, formatRtn, max_lengthRtn, client_data, 
			request_id);
	delete edata;
	return ret;
}

static void
list_drag_proc(Widget w, XEvent *e)
{
	Widget		context;
	Envelop_data	*edata;
	Atom		*exports = 0;
	Cardinal	nexports = 0;
	unsigned char	dragops = 0;
	Atom		*new_exports;
	DND		*dnd;
	Component	*comp;

	if ((context = XmGetDragContext(w, e->xbutton.time)) == 0)
		return;
	XtVaGetValues(w,	// list widget
		XmNuserData, &comp,
		0);
	dnd = (DND *)comp->get_client_data();
	if (!dnd)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	// transfer the component widget's window id
	// NOTE: this might be different from the src_widget's window id
	edata = new Envelop_data;
	edata->cproc = 0;
	edata->save = 0;
	edata->atom = dnd->get_atom();
	edata->win = XtWindow(comp->get_widget());
	XtVaGetValues(context,
		XmNexportTargets, &exports,
		XmNnumExportTargets, &nexports,
		XmNdragOperations, &dragops,
		XmNconvertProc, &edata->cproc, // default converter
		XmNclientData, &edata->save,
		0);
	XmListDragConvertStruct *conv = (XmListDragConvertStruct *)edata->save;
	edata->item_pos = XmListItemPos(w, conv->strings[0]);
	DEBUG_DND((stderr, "list_drag_proc: dnd %x, clientData %x, item %x, item_pos %d\n", 
		dnd, conv, conv->strings[0], edata->item_pos));
	new_exports = new Atom[nexports + 1];
	for (int i = 0; i < nexports; ++i)
		new_exports[i] = exports[i];
	new_exports[nexports] = dnd->get_atom();
	XtVaSetValues(context,
		XmNexportTargets, new_exports,
		XmNnumExportTargets, nexports+1,
		XmNdragOperations, dragops|XmDROP_COPY,
		XmNconvertProc, drag_convert,
		XmNclientData, edata,
		0);
	delete new_exports;
}

#define DAMPING 5
#define DELTA(x1,x2) (x1 < x2 ? x2 - x1 : x1 - x2)

static int
btn1_events(Display *, XEvent *e, XPointer arg)
{
	if (e->type == MotionNotify)
	{
		XEvent	*press = (XEvent *)arg;

		if (DELTA(press->xbutton.x_root, e->xmotion.x_root) > DAMPING ||
		    DELTA(press->xbutton.y_root, e->xmotion.y_root) > DAMPING)
			return 1;
	}
	else if (e->type == ButtonRelease)
		return 1;
	return 0;
}

static void
list_btn1_handler(Widget w, XEvent *e, char **params, Cardinal *num_params)
{
	XEvent new_e;
	XPeekIfEvent(XtDisplay(w), &new_e, btn1_events, (XPointer)e);
	if (new_e.type == MotionNotify)
	{
		// call ListProcessDrag
		XtCallActionProc(w, params[1], e, 0, 0);
		list_drag_proc(w, e);
	}
	else
		// call ListBeginSelect
		XtCallActionProc(w, params[0], e, 0, 0);
}

void
DND::setup_drag_source(Widget lw, DND_callback_proc drop_cb)
{
	// we make LOTs of assumptions about the fact that 'lw' is
	// a list widget
	static char btn1_xlations[] =
		"~c ~s ~m ~a <Btn1Down>: list_btn1_handler(ListBeginSelect,ListProcessDrag)";
	static char btn2_xlations[] =
		"<Btn2Down>: ListProcessDrag() list_drag_proc()";
	static XtActionsRec btn1_actions[] =
		{ {"list_btn1_handler", (XtActionProc)list_btn1_handler} };
	static XtActionsRec btn2_actions[] =
		{ {"list_drag_proc", (XtActionProc)list_drag_proc} };
	static Boolean btn1_action_added = FALSE;
	static Boolean btn2_action_added = FALSE;
	static XtTranslations parsed_btn1_xlations;
	static XtTranslations parsed_btn2_xlations;

	if (!btn2_action_added)
	{
		XtAppAddActions(base_context, btn2_actions, XtNumber(btn2_actions));
		btn2_action_added = TRUE;
	}
	if (!parsed_btn2_xlations)
		parsed_btn2_xlations = XtParseTranslationTable(btn2_xlations);
	XtOverrideTranslations(lw, parsed_btn2_xlations);

	extern Boolean btn1_transfer; // this is actually multi valued
	if (btn1_transfer)
	{
		if (!btn1_action_added)
		{
			XtAppAddActions(base_context, btn1_actions, XtNumber(btn1_actions));
			btn1_action_added = TRUE;
		}
		if (!parsed_btn1_xlations)
			parsed_btn1_xlations = XtParseTranslationTable(btn1_xlations);
		XtOverrideTranslations(lw, parsed_btn1_xlations);
	}
	_cb = drop_cb;
	DEBUG_DND((stderr,"setup_drag_source: lw %x, atom %d\n", 
		lw, _atom));
}

static void
list_drop_transfer(Widget w, XtPointer client, Atom *seltype, Atom *type,
	XtPointer value, unsigned long *length, int format)
{
	XmString	data_str;
	char		*data = 0;
	Window		src_win;
	int		item_pos;
	Widget		src_widget;

	// assume the source widget is a list
	DEBUG_DND((stderr, "list_drop_transfer: w %x, client %x, atom %d\n", 
		w, client, *type));
	data_str = XmCvtCTToXmString((char *)value);
	if (!XmStringGetLtoR(data_str, XmFONTLIST_DEFAULT_TAG, &data) ||
	    !data)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	sscanf(data, "%d %d", &src_win, &item_pos);
	src_widget = XtWindowToWidget(XtDisplay((Widget)client), src_win);
	DEBUG_DND((stderr, "list_drop_transfer: src_win %x, item_pos %d, src_widget %x\n", 
		src_win, item_pos, src_widget));
	if (src_widget == 0)
	{
		// not in this app
		XtVaSetValues(w,
			XmNtransferStatus, XmTRANSFER_FAILURE,
			XmNnumDropTransfers, 0,
			0);
		if (value)
			XtFree((String)value);
		return;
	}
	Component	*src_comp;
	Component	*targ_comp;
	XtVaGetValues(src_widget, XmNuserData, &src_comp, 0);
	XtVaGetValues((Widget)client, XmNuserData, &targ_comp, 0);
	DND_calldata	cdata;
	cdata.dropped_on = targ_comp;
	cdata.item_pos = item_pos;
	DND_callback_proc 	dcb = 
		((DND *)src_comp->get_client_data())->get_drop_cb();
	(src_comp->*dcb)(&cdata);
	if (value)
		XtFree((String)value);
}

static Boolean
drop_check(Widget w, Atom a)
{
	Atom		*exports = 0;
	Cardinal	nexports = 0;

	XtVaGetValues(w,
		XmNexportTargets, &exports,
		XmNnumExportTargets, &nexports,
		0);
	for (int i = 0; i < nexports; ++i)
	{
		if (exports[i] == a)
		{
			DEBUG_DND((stderr, "drop_check: got atom %d\n", a));
			return True;
		}
	}
	DEBUG_DND((stderr, "drop_check: failed\n"));
	return False;
}

static void
drop_proc(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmDropProcCallback DropData = (XmDropProcCallback)call_data;
	XmDropTransferEntryRec	transferEntries[1];
	Arg			args[4];
	int			n = 0;
	Widget			dc = DropData->dragContext;
	Component		*comp;
	DND			*dnd;
	DND_drop_proc		proc;

	XtVaGetValues(w, XmNuserData, &comp, 0);
	dnd = (DND *)comp->get_client_data();
	if (!dnd)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	DEBUG_DND((stderr, "drop_proc: w %x, comp %x, dnd %x\n",
		w, comp, dnd));
	if (drop_check(dc, dnd->get_atom()))
	{
		if (DropData->dropAction == XmDROP &&
	 	   (DropData->operation & (XmDROP_COPY|XmDROP_MOVE)))
		{
			transferEntries[0].target = dnd->get_atom();
			transferEntries[0].client_data = (XtPointer)w;
			XtSetArg(args[n], XmNdropTransfers, transferEntries); ++n;
			XtSetArg(args[n], XmNnumDropTransfers, 1); ++n;
			XtSetArg(args[n], XmNtransferProc, list_drop_transfer); ++n;
		}
		else 
		{
			XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); ++n;
			XtSetArg(args[n], XmNnumDropTransfers, 0); ++n;
		}
	}
	else if ((proc = dnd->get_old_drop_proc()) != 0)
	{
		(*proc)(w, client_data, call_data);
		return;
	}
	else
	{
		XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); ++n;
		XtSetArg(args[n], XmNnumDropTransfers, 0); ++n;
	}
	XmDropTransferStart(dc, args, n);
}

void
DND::setup_drop_site(Widget w)
{
	Arg	args[4];
	int	n = 0;
	Atom	*old_imports = 0;
	int	old_nimports = 0;

	XtSetArg(args[n], XmNimportTargets, &old_imports); ++n;
	XtSetArg(args[n], XmNnumImportTargets, &old_nimports); ++n;
	XtSetArg(args[n], XmNdropProc, &old_proc); ++n;
	XmDropSiteRetrieve(w, args, n);
	n = 0;
	if (old_nimports == 0)
	{
		// new drop site
		Atom	imports[1];

		imports[0] = _atom;
		XtSetArg(args[n], XmNimportTargets, imports); ++n;
		XtSetArg(args[n], XmNnumImportTargets, 1); ++n;
		XtSetArg(args[n], XmNdropSiteOperations, XmDROP_COPY|XmDROP_MOVE); ++n;
		XtSetArg(args[n], XmNdropProc, drop_proc); ++n;
		XmDropSiteRegister(w, args, n);
	}
	else
	{
		// add to existing import targets
		Atom	*new_imports = new Atom[old_nimports+1];
		for( int i = 0 ; i < old_nimports; ++i)
			new_imports[i] = old_imports[i];
		new_imports[old_nimports] = _atom;
		XtSetArg(args[n], XmNimportTargets, new_imports); ++n;
		XtSetArg(args[n], XmNnumImportTargets, old_nimports+1); ++n;
		XtSetArg(args[n], XmNdropProc, drop_proc); ++n;
		XmDropSiteUpdate(w, args, n);
		delete new_imports;
	}
	DEBUG_DND((stderr, "setup_drop_site: old_nimports %d, old_proc %x, w %x, atom %d\n", 
		old_nimports, old_proc, w, _atom));
}

