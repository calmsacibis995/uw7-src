#ifndef NOIDENT
#ident	"@(#)olmisc:Dynamic.c	1.123"
#endif


#define noDEBUG1K	/* dump partial table (string,OlKeyDef)*/
#define noDEBUG1B	/* dump partial table (string,OlBtnDef)*/
#define noDEBUG2K	/* print (mod, keysym) when a key/btn was pressed */
#define noDEBUG2B	/* print (state, button) when a btn was pressed */
#define noDEBUG3	/* dump table whenever there is an update */
#define noDEBUG4	/* misc */

#if defined(__STDC__)
#include <stdlib.h>	/* get qsort(3C) */
#endif

#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/keysym.h>

#include <Xol/OpenLookP.h>
#include <Xol/AcceleratP.h>
#include <Xol/DynamicP.h>
#include <Xol/DynamicI.h>
#include <Xol/OlStrings.h>
#include <Xol/ConvertersI.h>
#include <Xol/Flat.h>
#ifdef I18N
#include <Xol/PrimitiveP.h>
#include <Xol/TextEdit.h>
#endif

#if !defined(XtSpecificationRelease) || XtSpecificationRelease == 4
	/*
 error	Is the per display language string still being corrupted? not in r5.
	 * The Intrinsics assume that the location pointed to by
	 * the XtPerDisplay 'language' field points to a valid
	 * memory location.  This location is corrupted when
	 * updates to the resource manager property containing the
	 * resource "xnlLanguage" are merged into the display resource
	 * database.  
	 */
#define NEED_XNLLANGUAGE_UPDATE

	/* Xt (sic) so that we don't have to install it in X11 */
#include <Xt/IntrinsicI.h>
#endif
/*
 *************************************************************************
 *
 * Forward Procedure Declarations
 *
 **************************forward*declarations***************************
 */

OlInputEvent
	LookupOlInputEvent OL_ARGS((Widget, XEvent *, KeySym *,
				    char **, int *));
void	OlCallDynamicCallbacks OL_NO_ARGS();
KeySym	_OlCanonicalKeysym OL_ARGS((Display *, KeySym, KeyCode *, Modifiers *));
void	OlClassSearchIEDB OL_ARGS((WidgetClass, OlVirtualEventTable));
void	OlClassSearchTextDB OL_ARGS((WidgetClass));
void	OlClassUnsearchIEDB OL_ARGS((WidgetClass, OlVirtualEventTable));
OlVirtualEventTable
	OlCreateInputEventDB OL_ARGS((Widget, OlKeyOrBtnInfo, int,
				       OlKeyOrBtnInfo, int));
void	OlDestroyInputEventDB OL_ARGS((OlVirtualEventTable));
void	OlGrabVirtualKey OL_ARGS((Widget, OlVirtualName, Boolean, int, int));
void	OlLookupInputEvent OL_ARGS((Widget, XEvent *,
				     OlVirtualEvent, XtPointer));
void	_OlInitDynamicHandler OL_ARGS((Widget));
void	OlRegisterDynamicCallback OL_ARGS((OlDynamicCallbackProc, XtPointer));
Boolean	_OlStringToOlBtnDef OL_ARGS((Display *, XrmValue *, Cardinal *,
				     XrmValue *, XrmValue *, XtPointer *));
Boolean _OlStringToOlKeyDef OL_ARGS((Display *, XrmValue *, Cardinal *,
				     XrmValue *, XrmValue *, XtPointer *));
OlVirtualName
	_OlStringToVirtualKeyName OL_ARGS((String));
void	OlUngrabVirtualKey OL_ARGS((Widget, OlVirtualName));
int	OlUnregisterDynamicCallback OL_ARGS((OlDynamicCallbackProc, XtPointer));
void	OlWidgetSearchIEDB OL_ARGS((Widget, OlVirtualEventTable));
void	OlWidgetSearchTextDB OL_ARGS((Widget));
void	OlWidgetUnsearchIEDB OL_ARGS((Widget, OlVirtualEventTable));


static int		BinarySearch OL_ARGS((KeySym, Modifiers, Token *,
					      int, OlKeyBinding *));
static void		BuildIEDBstack OL_ARGS((Widget, XtPointer));
static int		CatchBadAccessError OL_ARGS((Display *,
						     XErrorEvent *));
static int		CompareFunc OL_ARGS((OLconst void *, OLconst void *));
static int		CompareUtil OL_ARGS((KeySym, Modifiers,
					     KeySym, Modifiers));
static OlInputEvent	DoLookup OL_ARGS((Widget, XEvent *, KeySym *,
					  String *, Cardinal *, Boolean, XtPointer));
static void		DynamicHandler OL_ARGS((
				Widget, XtPointer, XEvent *, Boolean *));
static XtPointer	GetMoreMem OL_ARGS((XtPointer, int, int,
					    short, short *));
static void		GetNewXtdefaults OL_ARGS((Widget));
static char *		GetStringDb OL_ARGS((Display *));
static void		GrabKey OL_ARGS((Widget, KeySym,
					 Modifiers, Boolean, int, int));
static void		IEDBPopCB OL_ARGS((Widget, XtPointer, XtPointer));
static void		InitSortedKeyDB OL_ARGS((OlVirtualEventTable));
static Boolean		IsComposeBtnOrKey OL_ARGS((Boolean, String,
						   int *, int *));
static Bool		IsSameKeyEvent OL_ARGS((Display *, XEvent *, char *));
static Boolean		ParseKeysymList OL_ARGS((String * , Modifiers *,
						 String *));
static void		RegrabVirtualKeys OL_ARGS((XtPointer));
static BtnSym		StringToButton OL_ARGS((String));
static KeySym		StringToKey OL_ARGS((Display * , String , Modifiers *));
static void		StringToKeyDefOrBtnDef OL_ARGS((Display * , Boolean,
							String , XtPointer));
static void		UngrabKey OL_ARGS((Widget, KeySym, Modifiers));
static void		UpdateIEDB OL_ARGS((Widget, OlVirtualEventTable));
static void		UpdateLocal OL_ARGS((Widget));
static OlVirtualName	WhatOlBtn OL_ARGS((unsigned int, unsigned int));
static OlVirtualName	WhatOlKey OL_ARGS((Widget, XEvent *, Boolean));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

	/* Dynamic change in progress flag */
Boolean				_OlDynResProcessing = False;
extern int			_XDefaultError();

#ifdef I18N
int Ol_last_event_received = 0;	 /* last event passed through OlAction */

	/* last event passed to the input method */	
static int last_im_event_processed = 0;
#endif

	/* the following static vars are for GrabKey() and	*/
	/* CatchBadAccessError()				*/
static int			(*gb_old_error_handler)();
static Boolean			gb_flag;
static Widget			gb_w;
static KeyCode			gb_keycode;
static Modifiers		gb_modifiers;
static Boolean			gb_owner_events;
static int			gb_pointer_mode;
static int			gb_keyboard_mode;

static GrabbedVirtualKey *	grab_list	= 0;
static Cardinal			grab_list_size	= 0;
static Boolean			registered_yet	= False;
	/* this will be used by GrabKey() and UnGrabkey(), and set by	*/
	/* _OlCanonicalKeysym(). this is because for keys like +, !,	*/
	/* & etc. the shift mask will be removed from "modifiers" but	*/
	/* XtGrabKey and XtUnGrabKey are expecting this mask		*/
static Boolean			grab_want_shift_mask = False;

static LocalData		local = { 0 };		/* See UpdateLocal */

static int               	num_dynamic_callbacks;
static DynamicCallback * 	dynamic_callbacks;
static char              	buffer_return[BUFFER_SIZE];

static OlVirtualEventTable	OlCoreDB = NULL;
static OlVirtualEventTable	OlTextDB = NULL;
static Boolean			create_dft_db = False;

			/* set from InitSortedKeyDB and use by CompareFunc */
static OlKeyBinding * 		current_key_bindings_to_sort = NULL;

static OlVirtualEventTable *	db_stack = NULL;
static short			db_stack_slots_alloced = 0;
static short			db_stack_entries = 0;

static OlVirtualEventTable *	avail_dbs = NULL;
static short			avail_dbs_slots_alloced = 0;
static short			avail_dbs_entries = 0;

static OlWidgetSearchInfo 	wid_list = NULL;
static short			wid_list_slots_alloced = 0;
static short			wid_list_entries = 0;

static OlClassSearchInfo 	wc_list = NULL;
static short			wc_list_slots_alloced = 0;
static short			wc_list_entries = 0;



/*
 * LookupOlInputEvent
 *
 * The \fILookupOlInputEvent\fR function is used to decode the \fIevent\fR
 * for widget \fIw\fR to an \fIOlInputEvent\fR.  The event passed should
 * be a ButtonPress, ButtonRelease, or KeyPress event.  The function attempts
 * to decode this event based on the settings of the OPEN LOOK(tm) defined
 * dynamic mouse and keyboard settings.
 *
 * If the event is a KeyPress, the function may return the \fIkeysym\fR,
 * \fIbuffer\fR, and/or \fIlength\fR of the buffer returned from a call to
 * XLookupString(3X).  It returns these values if non-NULL values are 
 * provided by the caller.
 *
 * See also:
 *
 * OlReplayBtnEvent(3), OlDetermineMouseAction(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 *#include <Dynamic.h>
 * ...
 */
OlInputEvent
LookupOlInputEvent OLARGLIST((w, event, keysym, buffer, length))
	OLARG( Widget,		w)
	OLARG( XEvent *,	event)
	OLARG( KeySym *,	keysym)
	OLARG( char **,		buffer)
	OLGRA( int *,		length)
{
	return (DoLookup(
		w, event, keysym, (String *)buffer,
				(Cardinal *) length, True, OL_DEFAULT_IE));
} /* end of LookupOlInputEvent */

/*
 * OlCallDynamicCallbacks
 *
 * The \fIOlCallDynamicCallbacks\fR procedure is used to trigger
 * the calling of the functions registered on the dynamic callback list.
 * This procedure is called automatically whenever the RESOURCE_MANAGER
 * property of the Root Window is updated.  It may also
 * be called to force a synchronization of the dynamic settings.
 *
 * See also:
 *
 * OlRegisterDynamicCallback(3), OlUnregisterDynamicCallback(3),
 * OlGetApplicationResources(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 *#include <Dynamic.h>
 * ...
 */
void
OlCallDynamicCallbacks OL_NO_ARGS()
{
	register int	i;

	for (i = 0; i < num_dynamic_callbacks; i++)
   		(*dynamic_callbacks[i].CB)(dynamic_callbacks[i].data);
} /* end of OlCallDynamicCallbacks */

/**
 ** _OlCanonicalKeysym()
 **/
KeySym
_OlCanonicalKeysym OLARGLIST((display, keysym, p_keycode, p_modifiers))
	OLARG( Display *,	display)
	OLARG( KeySym,		keysym)
	OLARG( KeyCode *,	p_keycode)
	OLGRA( Modifiers *,	p_modifiers)
{
	KeySym *		syms;

	int			keycode;
	KeyCode			foo_min_keycode;

	Modifiers		modifiers;

	int			per;
	int			min_keycode;
	int			max_keycode;


	/*
	 * This routine reduces a keysym to a canonical (or consistent)
	 * form. Generally this means converting a shifted-keysym into
	 * its unshifted form and ORing the ShiftMask into the modifiers
	 * mask. Thus, for example:
	 *
	 *	<M>	->	Shift<m>
	 *	<F14>	->	Shift<F2>
	 *
	 * However, certain shifted-keysyms look better the way they
	 * are, and we leave them alone:
	 *
	 *	<!>, not Shift<1>
	 *	<">, not Shift<'>
	 *	etc.
	 *
	 * (I18N)
	 */


	/*
	 * Grumble...the Intrinsics don't give us "max_keycode",
	 * so we have to beg Xlib for it. Fortunately, this doesn't
	 * involve a server request.
	 *
	 * Arrgh...and to top it off, Xt wants a (KeyCode *) while
	 * Xlib wants an (int *), for the same value ("min_keycode").
	 */
	syms = XtGetKeysymTable(display, &foo_min_keycode, &per);
	XDisplayKeycodes (display, &min_keycode, &max_keycode);

	modifiers = (p_modifiers? *p_modifiers : 0);
	for (
		keycode = (KeyCode)min_keycode;
		keycode <= max_keycode;
		keycode++, syms += per
	) {
		if (per > 1 && keysym == syms[1]) {
			if (!isascii(keysym) || isalpha(keysym)) {
				modifiers |= ShiftMask;
				keysym = syms[0];

		/* s<space> will not work without "!isspace()" check.	*/
		/* this is because <space> is ASCII but not ALPHA...	*/
			} else if (isascii(keysym) && !isalpha(keysym) &&
							!isspace(keysym)) {
				modifiers &= ~ShiftMask;
				grab_want_shift_mask = True;
			}
			break;
		} else if (per == 1 || syms[1] == NoSymbol) {
			KeySym			lower;
			KeySym			upper;

			XtConvertCase (display, syms[0], &lower, &upper);
			if (keysym == lower)
				break;
			else if (keysym == upper) {
				if (!isascii(keysym) || isalpha(keysym)) {
					modifiers |= ShiftMask;
					grab_want_shift_mask = True;
					keysym = lower;
				} else if (isascii(keysym) && !isalpha(keysym))
				{
					modifiers &= ~ShiftMask;
				}
				break;
			}
		} else if (keysym == syms[0])
			break;
	}

	if (p_keycode)
		*p_keycode = (keycode <= (KeyCode)max_keycode? keycode : 0);
	if (p_modifiers)
		*p_modifiers = modifiers;

	return (keysym);
} /* end of _OlCanonicalKeysym */

/*
 * OlClassSearchIEDB - This routine adds the given db into wc_list.
 */
void
OlClassSearchIEDB OLARGLIST((wc, db))
	OLARG( WidgetClass,		wc)
	OLGRA( OlVirtualEventTable,	db)
{
	int	i;

	if (wc == NULL || db == NULL)
		return;

	if (wc_list != NULL)
	{
		for (i = 0; i < wc_list_entries; i++)
			if (wc_list[i].wc == wc && wc_list[i].db == db)
				return;
	}
	wc_list = (OlClassSearchInfo) GetMoreMem(
			(XtPointer) wc_list,
			(int) sizeof(OlClassSearchRec),
			MORESLOTS,
			wc_list_entries,
			&wc_list_slots_alloced);

	wc_list [wc_list_entries].wc = wc;
	wc_list [wc_list_entries++].db = db;
} /* end of OlClassSearchIEDB */

/*
 * a convenience routine to register the Text DB - class
 */
void
OlClassSearchTextDB OLARGLIST((wc))
	OLGRA( WidgetClass,	wc)
{
	OlClassSearchIEDB(wc, OlTextDB);
} /* end of OlClassSearchTextDB */

/*
 * OlClassUnsearchIEDB - if "wc" is NULL then all db in wc_list should
 *	be removed.
 */
void
OlClassUnsearchIEDB OLARGLIST((wc, db))
	OLARG( WidgetClass,		wc)
	OLGRA( OlVirtualEventTable,	db)
{
	int	i;

	if (db == (OlVirtualEventTable)NULL || wc_list == NULL)
		return;

		/* no-op if this is the case...		*/
	if ((XtPointer)db == OL_DEFAULT_IE || (XtPointer)db == OL_CORE_IE)
		return;

	if ((XtPointer)db == OL_TEXT_IE)
		db = OlTextDB;

	for (i = 0; i < wc_list_entries; i++)
		if ((wc == (WidgetClass)NULL || wc_list[i].wc == wc) &&
		    wc_list[i].db == db)
			break;

	if (i == wc_list_entries)	 /* not found	*/
		return;

	OlMemMove(OlClassSearchRec, &wc_list[i], &wc_list[i+1],
			wc_list_entries - i - 1);

	wc_list_entries--;

		/* do it recursively if wc == NULL */
	if (wc == (WidgetClass)NULL)
		OlClassUnsearchIEDB(wc, db);
} /* end of OlClassUnsearchIEDB */

/*
 * OlCreateInputEventDB -
 */
OlVirtualEventTable
OlCreateInputEventDB OLARGLIST((w, key_info, num_keys, btn_info, num_btns))
	OLARG( Widget,		w)
	OLARG( OlKeyOrBtnInfo,	key_info)
	OLARG( int,		num_keys)
	OLARG( OlKeyOrBtnInfo,	btn_info)
	OLGRA( int,		num_btns)
{
	int			i;
	OlVirtualEventTable	db;
	OlKeyBinding *		key_bindings = NULL;
	OlBtnBinding *		btn_bindings = NULL;

	if (num_keys == 0 && num_btns == 0)
		return (NULL);

#define BINDINGS	key_bindings[i]
#define NAME		BINDINGS.name
#define DFT		BINDINGS.default_value
#define OLCMD		BINDINGS.ol_event
#define USED		BINDINGS.def.used

	if (num_keys != 0)
	{
		if (create_dft_db == True)
			key_bindings = (OlKeyBinding *) key_info;
		else
		{
			key_bindings = (OlKeyBinding *) XtMalloc(
						sizeof(OlKeyBinding) *
						num_keys);

			for (i = 0; i < num_keys; i++)
			{
				NAME	= key_info[i].name;
				DFT	= key_info[i].default_value;
				OLCMD	= key_info[i].virtual_name;
				USED	= 0;
			}
#undef BINDINGS
		}
	}
	if (num_btns != 0)
	{
		if (create_dft_db == True)
			btn_bindings = (OlBtnBinding *) btn_info;
		else
		{
#define BINDINGS	btn_bindings[i]

			btn_bindings = (OlBtnBinding *) XtMalloc(
						sizeof(OlBtnBinding) *
						num_btns);
			for (i = 0; i < num_btns; i++)
			{
		  		NAME	= btn_info[i].name;
		  		DFT	= btn_info[i].default_value;
		  		OLCMD	= btn_info[i].virtual_name;
		  		USED	= 0;
			}
		}
	}
#undef BINDINGS
#undef NAME
#undef DFT
#undef OLCMD
#undef USED

	db = (OlVirtualEventTable) XtMalloc(sizeof(OlVirtualEventInfo));

	avail_dbs = (OlVirtualEventTable *) GetMoreMem(
			(XtPointer) avail_dbs,
			(int) sizeof(OlVirtualEventTable *),
			MORESLOTS,
			avail_dbs_entries,
			&avail_dbs_slots_alloced);
	avail_dbs [avail_dbs_entries++] = db;

	db->key_bindings     = key_bindings;
	db->btn_bindings     = btn_bindings;
	db->num_key_bindings = (char)num_keys;
	db->num_btn_bindings = (char)num_btns;
	db->sorted_key_db    = NULL;
	UpdateIEDB(w, db);

	return (db);
} /* end of OlCreateInputEventDB */

/*
 * OlDestroyInputEventDB - destroy a given db
 */
void
OlDestroyInputEventDB OLARGLIST((db))
	OLGRA( OlVirtualEventTable,	db)
{
	int	i;

	if (db == (OlVirtualEventTable)NULL || avail_dbs == NULL)
		return;

		/* can't destroy these three...		*/
	if ((XtPointer)db == OL_DEFAULT_IE || (XtPointer)db == OL_CORE_IE ||
	    (XtPointer)db == OL_TEXT_IE)
		return;


		/* de-reference "db" from wc_list and wid_list	*/
	OlClassUnsearchIEDB((WidgetClass)NULL, db);
	OlWidgetUnsearchIEDB((Widget)NULL, db);

	for (i = 0; i < avail_dbs_entries; i++)
		if (avail_dbs[i] == db)
		{
			if (db->key_bindings)
				XtFree((XtPointer)db->key_bindings);
			if (db->btn_bindings)
				XtFree((XtPointer)db->btn_bindings);
			XtFree((XtPointer)db);

			OlMemMove(OlVirtualEventTable *, &avail_dbs[i],
				&avail_dbs[i+1],
				avail_dbs_entries - i - 1
			);

			avail_dbs_entries--;
			break;
		}
} /* end of OlDestroyInputEventDB */

/*
 * OlGrabVirtualKey -
 */
void
OlGrabVirtualKey OLARGLIST((w, vkey, owner_events, pointer_mode, keyboard_mode))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	vkey)
	OLARG( Boolean,		owner_events)
	OLARG( int,		pointer_mode)
	OLGRA( int,		keyboard_mode)
{
	GrabbedVirtualKey *	p	= grab_list;
	GrabbedVirtualKey *	pend	= grab_list + grab_list_size;

	OlKeyBinding *		kb	= 0;

	Cardinal		i, j;


	if (!w)
		return;

	if (p)
		for (; p < pend; p++)
			if (p->vkey == vkey)
				return;	/* already grabbed */

	for (i = 0; i < avail_dbs_entries; i++)
	{
		for (j = 0; j < avail_dbs[i]->num_key_bindings; j++)
			if ((OlInputEvent)vkey ==
				avail_dbs[i]->key_bindings[j].ol_event)
			{
				kb = &(avail_dbs[i]->key_bindings[j]);
				break;
			}
		if (kb)
			break;
	}
	if (!kb)
		return;	/* bogus OlVirtualEvent */

	for (i = 0; i < kb->def.used; i++)
		GrabKey (w, kb->def.keysym[i], kb->def.modifier[i],
			 owner_events, pointer_mode, keyboard_mode);

	grab_list_size++;
	grab_list = Array(grab_list, GrabbedVirtualKey, grab_list_size);
	i = grab_list_size - 1;
	grab_list[i].w             = w;
	grab_list[i].vkey          = vkey;
	grab_list[i].kb            = kb;
	grab_list[i].as_grabbed    = kb->def;
	grab_list[i].grabbed       = True;
	grab_list[i].owner_events  = owner_events;
	grab_list[i].pointer_mode  = pointer_mode;
	grab_list[i].keyboard_mode = keyboard_mode;

	if (!registered_yet) {
		OlRegisterDynamicCallback (RegrabVirtualKeys, (XtPointer)0);
		registered_yet = True;
	}

} /* end of OlGrabVirtualKey */

/**
 ** _OlKeysymToSingleChar()
 **/
int
_OlKeysymToSingleChar OLARGLIST((keysym))
	OLGRA( KeySym,		keysym)
{
	register Cardinal	i;


	for (i = 0; i < XtNumber(singlechar_map); i++)
		if (keysym == singlechar_map[i].keysym)
			return (singlechar_map[i].single);
	return (0);
} /* _OlKeysymToSingleChar */

/*
 * OlLookupInputEvent -
 */
void
OlLookupInputEvent OLARGLIST((w, xevent, virtual_event_ret, db_flag))
	OLARG( Widget,		w)		   /* widget getting xevent */
	OLARG( XEvent *,	xevent)		   /* xevent to look at     */
	OLARG( OlVirtualEvent,	virtual_event_ret) /* returned virtual event */
	OLGRA( XtPointer,	db_flag)	   /* OL_CORE_IE, OL_TEXT_IE,
						      OL_DEFAULT_IE, or db ptr*/
{
	virtual_event_ret->consumed     = False;
	virtual_event_ret->xevent       = xevent;
	virtual_event_ret->dont_care    = (xevent->type == KeyPress) ?
						local.key_dont_care_bits :
						local.dont_care_bits;
	virtual_event_ret->keysym       = NoSymbol;
	virtual_event_ret->buffer       = NULL;
	virtual_event_ret->length       = 0;
	virtual_event_ret->virtual_name =
		(OlVirtualName) DoLookup (
					w,
					virtual_event_ret->xevent,
					&virtual_event_ret->keysym,
					&virtual_event_ret->buffer,
					&virtual_event_ret->length,
					False,
					db_flag
				);
	if (w != (Widget)NULL && xevent != (XEvent *)NULL && _OlIsFlat(w))
	{
#define X_N_Y(etype)  (Position)xevent->etype.x, (Position)xevent->etype.y
		switch(xevent->type) {
		case KeyPress:		/* FALLTHROUGH */
		case KeyRelease:	/* FALLTRHOUGH */
		case FocusIn:		/* FALLTHROUGH */
		case FocusOut:
			virtual_event_ret->item_index =
				OlFlatGetFocusItem(w);
			break;
		case ButtonPress:	/* FALLTHROUGH */
		case ButtonRelease:
			virtual_event_ret->item_index =
				OlFlatGetItemIndex(w, X_N_Y(xbutton));
			break;
		case MotionNotify:
			virtual_event_ret->item_index =
				OlFlatGetItemIndex(w, X_N_Y(xmotion));
			break;
		case EnterNotify:	/* FALLTHROUGH */
		case LeaveNotify:
			virtual_event_ret->item_index =
				OlFlatGetItemIndex(w, X_N_Y(xcrossing));
			break;
		default:
			virtual_event_ret->item_index = (Cardinal)OL_NO_ITEM;
			break;
		}
#undef X_N_Y
	}
	else
	{
		virtual_event_ret->item_index = (Cardinal)OL_NO_ITEM;
	}
} /* end of OlLookupInputEvent */

/*
 *************************************************************************
 * _OlInitDynamicHandler - this routine initializes the dynamic hander.
 * It is called at application startup time.
 *************************************************************************
 */
void
_OlInitDynamicHandler OLARGLIST((w))
	OLGRA( Widget,	w)
{
	static Boolean	first_time = True;

	if (first_time)
	{
		int	dummy = -1;

		first_time = False;

		XtSetTypeConverter(XtRString, XtROlKeyDef, _OlStringToOlKeyDef,
				(XtConvertArgList)NULL, (Cardinal)0,
				XtCacheNone, (XtDestructor)0);

		XtSetTypeConverter(XtRString, XtROlBtnDef, _OlStringToOlBtnDef,
				(XtConvertArgList)NULL, (Cardinal)0,
				XtCacheNone, (XtDestructor)0);

		w = XtWindowToWidget(XtDisplayOfObject(w),
			       RootWindowOfScreen(XtScreenOfObject(w)));

		XtRealizeWidget(w);

		XtAddEventHandler(w, PropertyChangeMask, False,
				     DynamicHandler, (XtPointer)NULL);

				/* Initialize the keys and buttons	*/
				/* Turn on the flag below to let routine*/
				/* know we don't need to alloc space	*/

		create_dft_db = True;

		OlCoreDB = OlCreateInputEventDB(w,
					(OlKeyOrBtnInfo) OlCoreKeyBindings,
					XtNumber(OlCoreKeyBindings),
					(OlKeyOrBtnInfo) OlCoreBtnBindings,
					XtNumber(OlCoreBtnBindings)
		);

		OlTextDB = OlCreateInputEventDB(w,
					(OlKeyOrBtnInfo) OlTextKeyBindings,
					XtNumber(OlTextKeyBindings),
					NULL, 0);

		create_dft_db = False;


				/* Initialize local copies of some
				 * global application resources.
				 */
		UpdateLocal(w);

		OlGetOlKeysForIm(NULL, &dummy, &dummy);
	}
} /* end of _OlInitDynamicHandler */

/*
 * OlRegisterDynamicCallback
 *
 * The \fIOlRegisterDynamicCallback\fR procedure is used to add
 * a function to the list of registered callbacks to be called
 * whenever the procedure \fIOlCallDynamicCallbacks\fR is invoked.
 * The OlCallDynamicCallback procedure is invoked whenever the
 * RESOURCE_MANAGER property of the Root Window is updated and after
 * the dynamic resources registered using the \fIOlGetApplicationResources\fR
 * have been refreshed.  The OlCallDynamicCallbacks procedure may
 * also be called directly by either the application or other routines in
 * the widget libraries.  The callbacks registered are guaranteed to
 * be called in FIFO order of registration and will be called as
 *
 * .so CWstart
 *  (*CB)(data);
 * .so CWend
 *
 * See also:
 *
 * OlUnregisterDynamicCallback(3), OlCallDynamicCallbacks(3)
 * OlGetApplicationResources(3)
 *
 * Synopsis:
 *
 *#include <Dynamic.h>
 * ...
 */
void
OlRegisterDynamicCallback OLARGLIST((CB, data))
	OLARG( OlDynamicCallbackProc,	CB)
	OLGRA( XtPointer,		data)
{
	register int	i = num_dynamic_callbacks++;

	dynamic_callbacks = (DynamicCallback *) XtRealloc(
				(char *)dynamic_callbacks,
			  	num_dynamic_callbacks *
					sizeof(DynamicCallback));

	dynamic_callbacks[i].CB = CB;
	dynamic_callbacks[i].data = data;

} /* end of OlRegisterDynamicCallback */

/**
 ** _OlSingleCharToKeysym()
 **/

KeySym
_OlSingleCharToKeysym OLARGLIST((chr))
	OLGRA( int,		chr)
{
	register Cardinal	i;


	for (i = 0; i < XtNumber(singlechar_map); i++)
		if (chr == singlechar_map[i].single)
			return (singlechar_map[i].keysym);
	return (NoSymbol);
} /* _OlSingleCharToKeysym */

/**
 ** _OlStringToOlBtnDef()
 **/

/* ARGSUSED */
Boolean
_OlStringToOlBtnDef OLARGLIST((display,args,num_args,from,to,converter_data))
	OLARG( Display *,	display)
	OLARG( XrmValue *,	args)			/* ignored	*/
	OLARG( Cardinal *,	num_args)
	OLARG( XrmValue *,	from)
	OLARG( XrmValue *,	to)
	OLGRA( XtPointer *,	converter_data)		/* ignored	*/
{
	OlBtnDef		bd;


	if (*num_args)
	  OlVaDisplayErrorMsg(display,
			      OleNbadConversion,
			      OleTtooManyParams,
			      OleCOlToolkitError,
			      OleMbadConversion_tooManyParams,
			      "String","OlBtnDef");
		/*NOTREACHED*/

	if (from->addr)
		StringToKeyDefOrBtnDef (display, False,
				(String)from->addr, (XtPointer)&bd);
	else
		bd.used = 0;

		/* enable NULL binding */
	if (!bd.used && from->addr[0] != 0) {
		static String		_args[2]  = { 0 , 0 };
		Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		OlVaDisplayWarningMsg(display,
				      OleNfileDynamic,
				      OleTmsg3,
				      OleCOlToolkitWarning,
				      OleMfileDynamic_msg3,
				      "_OlStringToOlBtnDef",
				      (String)from->addr);
		return (False);
	}

	ConversionDone (OlBtnDef, bd);
} /* end of _OlStringToOlBtnDef */

/* ARGSUSED */
Boolean
_OlStringToOlKeyDef OLARGLIST((display,args,num_args,from,to,converter_data))
	OLARG( Display *,		display)
	OLARG( XrmValue *,		args)		/* ignored	*/
	OLARG( Cardinal *,		num_args)
	OLARG( XrmValue *,		from)
	OLARG( XrmValue *,		to)
	OLGRA( XtPointer *,		converter_data)	/* ignored	*/
{
	OlKeyDef		kd;

	if (*num_args)
	  OlVaDisplayErrorMsg(display,
			      OleNbadConversion,
			      OleTtooManyParams,
			      OleCOlToolkitError,
			      OleMbadConversion_tooManyParams,
			      "String","OlKeyDef");
		/*NOTREACHED*/

	if (from->addr)
		StringToKeyDefOrBtnDef (display, True,
				(String)from->addr, (XtPointer)&kd);
	else
		kd.used = 0;

		/* enable NULL binding */
	if (!kd.used && from->addr[0] != 0) {
		static String		_args[2]  = { 0 , 0 };
		Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		OlVaDisplayWarningMsg(display,
				      OleNfileDynamic,
				      OleTmsg3,
				      OleCOlToolkitWarning,
				      OleMfileDynamic_msg3,
				      "_OlStringToOlKeyDef",
				      (String)from->addr);
		return (False);
	}

	ConversionDone (OlKeyDef, kd);
} /* end of _OlStringToOlKeyDef */

OlVirtualName
_OlStringToVirtualKeyName OLARGLIST((str))
	OLGRA( String,	str)
{
	Cardinal	i, j;

	for (i = 0; i < avail_dbs_entries; i++)
		for (j = 0; j < avail_dbs[i]->num_key_bindings; j++)
			if (STREQU(str, avail_dbs[i]->key_bindings[j].name))
				return (avail_dbs[i]->key_bindings[j].ol_event);
	return (OL_UNKNOWN_KEY_INPUT);
} /* end of _OlStringToVirtualKeyName */

void
OlUngrabVirtualKey OLARGLIST((w, vkey))
	OLARG( Widget,		w)
	OLGRA( OlVirtualName,	vkey)
{
	GrabbedVirtualKey *	p	= grab_list;
	GrabbedVirtualKey *	pend	= grab_list + grab_list_size;

	OlKeyDef *		kd;

	Cardinal		i;


	if (!w)
		return;

	if (!p)
		return; /* nothing grabbed */
	for (; p < pend; p++)
		if (p->vkey == vkey)
			break;
	if (p >= pend)
		return;	/* not grabbed */

	kd = &(p->as_grabbed);
	for (i = 0; i < kd->used; i++)
		UngrabKey (w, kd->keysym[i], kd->modifier[i]);

	OlMemMove (GrabbedVirtualKey, p, p+1, pend - p - 1);
	grab_list_size--;
	grab_list = Array(grab_list, GrabbedVirtualKey, grab_list_size);

	if (!grab_list) {
		OlUnregisterDynamicCallback (RegrabVirtualKeys, (XtPointer)0);
		registered_yet = False;
	}
} /* end of OlUngrabVirtualKey */

/*
 * OlUnregisterDynamicCallback
 *
 * The \fIOlUnregisterDynamicCallback\fR procedure is used to remove
 * a function from the list of registered callbacks to be called
 * whenever the procedure \fIOlCallDynamicCallbakcs\fR is invoked.
 *
 * See also:
 *
 * OlRegisterDynamicCallback(3), OlCallDynamicCallbacks(3)
 * OlGetApplicationResources(3)
 *
 * Synopsis:
 *
 *#include <Dynamic.h>
 * ...
 */
int
OlUnregisterDynamicCallback OLARGLIST((CB, data))
	OLARG( OlDynamicCallbackProc,	CB)
	OLGRA( XtPointer,		data)
{
	register int	i;
	int		retval;

	for (i = 0; i < num_dynamic_callbacks; i++)
		if (dynamic_callbacks[i].CB == CB &&
			dynamic_callbacks[i].data == data)
				break;

	if (i == num_dynamic_callbacks)
		retval = 0;
	else
	{
		num_dynamic_callbacks--;
		if (num_dynamic_callbacks == 0)
      		{
      			FREE(dynamic_callbacks);
      			dynamic_callbacks = (DynamicCallback *)NULL;
      		}
   		else
      		{
      			if (i < num_dynamic_callbacks)
         			OlMemMove(DynamicCallback,
					&dynamic_callbacks[i],
					&dynamic_callbacks[i + 1],
               				num_dynamic_callbacks - i);
      			dynamic_callbacks = (DynamicCallback *)
         			REALLOC(dynamic_callbacks,
					num_dynamic_callbacks *
					sizeof(DynamicCallback));
      		}
   		retval = 1;
   	}
	return (retval);
} /* end of OlUnregisterDynamicCallback */

/*
 * OlWidgetSearchIEDB - This routine adds the given w into wid_list.
 */
void
OlWidgetSearchIEDB OLARGLIST((w, db))
	OLARG( Widget,			w)
	OLGRA( OlVirtualEventTable,	db)
{
	int	i;

	if (w == NULL)
		return;

	if (wid_list != NULL)
	{
		for (i = 0; i < wid_list_entries; i++)
			if (wid_list[i].w == w && wid_list[i].db == db)
				return;
	}
	wid_list = (OlWidgetSearchInfo) GetMoreMem(
			(XtPointer) wid_list,
			(int) sizeof(OlWidgetSearchRec),
			MORESLOTS,
			wid_list_entries,
			&wid_list_slots_alloced);

	wid_list [wid_list_entries].w = w;
	wid_list [wid_list_entries++].db = db;

		/* de-reference from wid_list when "w" is destroyued */
	XtAddCallback(w, XtNdestroyCallback, IEDBPopCB, (XtPointer)db);
} /* end of OlWidgetSearchIEDB */

/*
 * OlWidgetSearchTextDB - a convenience routine to register the Text DB -
				instance
 */
void
OlWidgetSearchTextDB OLARGLIST((w))
	OLGRA( Widget,		w)
{
	OlWidgetSearchIEDB(w, OlTextDB);
} /* end of OlWidgetSearchTextDB */

/*
 * OlWidgetUnsearchIEDB - if w == NULL then all db in wid_list should
 *				be removed.
 */
void
OlWidgetUnsearchIEDB OLARGLIST((w, db))
	OLARG( Widget,			w)
	OLGRA( OlVirtualEventTable,	db)
{
	int	i;

	if (db == (OlVirtualEventTable)NULL || wid_list == NULL)
		return;

		/* no-op if this is the case...		*/
	if ((XtPointer)db == OL_DEFAULT_IE || (XtPointer)db == OL_CORE_IE)
		return;

	if ((XtPointer)db == OL_TEXT_IE)
		db = OlTextDB;

	for (i = 0; i < wid_list_entries; i++)
		if ((w == NULL || wid_list[i].w == w) && wid_list[i].db == db)
			break;

	if (i == wid_list_entries)	 /* not found	*/
		return;

	OlMemMove(OlClassSearchRec, &wid_list[i], &wid_list[i+1],
			wid_list_entries - i - 1);

	wid_list_entries--;

		/* do it recursively if "w" is NULL...	*/
	if (w == (Widget)NULL)
		OlWidgetUnsearchIEDB(w, db);
} /* end of OlWidgetUnsearchIEDB */

/*
 * BinarySearch - This routine returns an index which points to the given list
 *	if the search succeed otherwise it returns -1.
 */
static int
BinarySearch OLARGLIST((keysym, modifier, list, num_tokens, key_bindings))
	OLARG( KeySym,		keysym)		/* primary search key	*/
	OLARG( Modifiers,	modifier)	/* secondary search key	*/
	OLARG( Token *,		list)		/* the sorted table	*/
	OLARG( int,		num_tokens)	/* # of table tokens	*/
	OLGRA( OlKeyBinding *,	key_bindings)	/* the info table	*/
{
#define YY(i)		key_bindings[i].def
#define KEYSYM_T        YY(list[j].i).keysym[list[j].j]
#define MOD_T           YY(list[j].i).modifier[list[j].j]

	int	k, m, j;
	int	retval;

	k = 1;
	m = num_tokens;
	while (k <= m)
	{
		j = (k+m) / 2;
		if ((retval = CompareUtil (keysym, modifier,
					   KEYSYM_T, MOD_T)) == 0)
			return (j);
		if (retval < 0)
			m = j - 1;
		else
			k = j + 1;
	}

	return (-1);

#undef YY
#undef KEYSYM_T
#undef MOD_T
} /* end of BinarySearch */

/*
 * BuildIEDBstack -
 *
 * build a stack w.r.t db_flag, return num_entries in stack
 *	If the db_flag == OL_DEFAULT_IE, then
 *		the stack is according to the registering order
 *		(Last in First out) in the Class list, the Widget
 *		list, and OlCoreDB.
 *	else
 *		only one DB will be pushed into stack.
 */
static void
BuildIEDBstack OLARGLIST((w, db_flag))
	OLARG( Widget,		w)
	OLGRA( XtPointer,	db_flag)
{
	int		i, j;

	db_stack = (OlVirtualEventTable *) GetMoreMem (
				(XtPointer) db_stack,
			    	(int) sizeof (OlVirtualEventTable *),
			    	MORESLOTS,
			    	db_stack_entries,
				&db_stack_slots_alloced);
	db_stack_entries = 0;

	if (db_flag == OL_TEXT_IE)
	{
		db_stack[db_stack_entries++] = OlTextDB;
		return;
	}
	if (db_flag == OL_CORE_IE)
	{
		db_stack[db_stack_entries++] = OlCoreDB;
		return;
	}
	if (db_flag != OL_DEFAULT_IE)
	{
		db_stack[db_stack_entries++] = (OlVirtualEventTable) db_flag;
		return;
	}
/* OL_DEFAULT_IE */
/* search for wid_list first. have to search all entries because
	mutiple DBs may be registered by the same widget */

	for (i = wid_list_entries - 1; i >= 0; i--)
	{
		if (wid_list[i].w == w)
		{
				/* no need to push if it is there already */
			for (j = 0; j < db_stack_entries; j++)
				if (db_stack[j] == wid_list[i].db)
					continue;
			db_stack = (OlVirtualEventTable *) GetMoreMem (
				(XtPointer) db_stack,
			    	(int) sizeof (OlVirtualEventTable *),
			    	MORESLOTS,
			    	db_stack_entries,
				&db_stack_slots_alloced);

			db_stack[db_stack_entries++] = wid_list[i].db;
		}
	}
	
/* search for wc_list next. have to search all entries because
	mutiple DBs may be registered by the same widget class */

	for (i = wc_list_entries - 1; i >= 0; i--)
	{
		if (XtIsSubclass(w, wc_list[i].wc) == True)
		{
				/* no need to push if it is there already */
			for (j = 0; j < db_stack_entries; j++)
				if (db_stack[j] == wc_list[i].db)
					continue;

			db_stack = (OlVirtualEventTable *) GetMoreMem (
				(XtPointer) db_stack,
			    	(int) sizeof (OlVirtualEventTable *),
			    	MORESLOTS,
			    	db_stack_entries,
				&db_stack_slots_alloced);

			db_stack[db_stack_entries++] = wc_list[i].db;
		}
	}

/* push coreDB at end */

	db_stack = (OlVirtualEventTable *) GetMoreMem (
				(XtPointer) db_stack,
			    	(int) sizeof (OlVirtualEventTable *),
			    	MORESLOTS,
			    	db_stack_entries,
				&db_stack_slots_alloced);

	db_stack[db_stack_entries++] = OlCoreDB;

} /* end of BuildIEDBstack */

static int
CatchBadAccessError OLARGLIST((dpy, xevent))
	OLARG( Display *,	dpy)
	OLGRA( XErrorEvent *,	xevent)
{
		/* only called once, if still fails, we won't try again */
	if (gb_flag == False)
	{
		if (gb_old_error_handler == _XDefaultError)
			_XDefaultError(dpy, xevent);
		return (0);
	}

	gb_flag = False;

		/* always call the old one first, if an applic. is	*/
		/* traping the BadAccess error, then we don't need	*/
		/* to do anything...					*/
	if (gb_old_error_handler != _XDefaultError)
		(*gb_old_error_handler)(dpy, xevent);

	switch (xevent->error_code)
	{
		case BadAccess:
#ifdef DEBUG4
 fprintf (stderr, "got BadAccess, retrying after %d secs\n", local.key_remap_timeout);
#endif
			sleep (local.key_remap_timeout);
			XtGrabKey(gb_w, gb_keycode, gb_modifiers,
				  gb_owner_events, gb_pointer_mode,
				  gb_keyboard_mode);
			XSync(XtDisplayOfObject(gb_w), 0);
			break;
		default:
			if (gb_old_error_handler == _XDefaultError)
				_XDefaultError(dpy, xevent);
			break;
	}
	return (0);
} /* end of CatchBadAccessError */

/*
 * CompareFunc -
 *
 * This routine compares two given tokens by calling CompareUtil().
 *
 * note: token.i points to current_key_bindings[].
 *	 token.j points to current_key_bindings[token.i].modifiers[] and
 *			   current_key_bindings[token.i].keysym[].
 *
 * see qsort(3C) for more details
 */
static int
CompareFunc OLARGLIST((vv1, vv2))
	OLARG( OLconst void *,	vv1)
	OLGRA( OLconst void *,	vv2)
{
#define XX(i)		current_key_bindings_to_sort[i]
#define YY(i)		current_key_bindings_to_sort[i].def
#define KEYSYM1		YY(v1->i).keysym[v1->j]
#define KEYSYM2		YY(v2->i).keysym[v2->j]
#define MOD1		YY(v1->i).modifier[v1->j]
#define MOD2		YY(v2->i).modifier[v2->j]

	int     retval;
	OLconst Token *	v1;
	OLconst Token *	v2;

	v1 = (OLconst Token *)vv1;
	v2 = (OLconst Token *)vv2;
        retval = CompareUtil (KEYSYM1, MOD1, KEYSYM2, MOD2);

#ifdef DEBUG4
 if (retval == 0)	/* equal */
	fprintf (stderr, "%s and %s have the same bindings\n",
				XX(v1->i).name, XX(v2->i).name);
#endif

	return (retval);

#undef XX
#undef YY
#undef KEYSYM1
#undef KEYSYM2
#undef MOD1
#undef MOD2
} /* end of CompareFunc */

/*
 * CompareUtil -
 *
 * This routine uses the following semantics to do the comparsion.
 *
 *		 KEYSYM1 == KEYSYM2  KEYSYM1 < KEYSYM2  KEYSYM1 > KEYSYM2
 * MOD1 == MOD2  0		     -1			1
 * MOD1 <  MOD2  -1		     -1			1
 * MOD1 >  MOD2  1		     -1			1
 *
 */
static int
CompareUtil OLARGLIST((keysym1, mod1, keysym2, mod2))
	OLARG( KeySym,		keysym1)	/* set 1 */
	OLARG( Modifiers,	mod1)
	OLARG( KeySym,		keysym2)	/* set 2 */
	OLGRA( Modifiers,	mod2)
{
	if (keysym1 < keysym2)
		return (-1);

	if (keysym1 > keysym2)
		return (1);

	/*
	 * else..keysyms are equal
	 */

	if (mod1 < mod2)
		return (-1);

	if (mod1 > mod2)
		return (1);

	/*
	 * else..mod1 and mod2 are equal
	 */
	return(0);
} /* end of CompareUtil */

static OlInputEvent
DoLookup OLARGLIST((w, event, keysym, buffer, length, textedit_flag, db_flag))
	OLARG( Widget,		w)
	OLARG( XEvent *,	event)
	OLARG( KeySym *,	keysym)
	OLARG( String *,	buffer)
	OLARG( Cardinal *,	length)
	OLARG( Boolean,		textedit_flag)	/* True for backward compat.*/
	OLGRA( XtPointer,	db_flag)	/* OL_CORE_IE, OL_TEXT_IE,
						   OL_DEFAULT_IE, or db ptr */
{
	OlInputEvent	retval = OL_UNKNOWN_INPUT;

	XComposeStatus	status_return;
	KeySym		keysym_return;
	int		length_return;
	XEvent		newevent;

#ifdef I18N
	OlIc *			ic;
	OlImStatus		im_status;
	static int		buf_size = BUFFER_SIZE;
	static char *		buf = NULL;
	static char *		last_im_buffer;
	static KeySym		last_im_keysym;
	static int		last_im_length = 0;
#endif


	BuildIEDBstack(w, db_flag);

	switch (event->type)
	{
		case KeyPress:
#ifdef I18N
			if (buf == NULL){
					/* +1 for NULL terminator */
				buf = XtMalloc(BUFFER_SIZE+1);
				last_im_buffer = XtMalloc(BUFFER_SIZE+1);
			}
			/* 
			 * Check if the widget is using an input method.
			 *
			 */
			if (XtIsSubclass(w, primitiveWidgetClass)){

				PrimitiveWidget pw = (PrimitiveWidget) w;

				if (XtIsSubclass(w,textEditWidgetClass))
					textedit_flag = True;

					/* If this widget is attached to an
					 * input method via an ic...
					 */
				if ((ic = pw-> primitive.ic) != NULL){
						/*
					 	 * Pass the key event to the
						 * input method if it has not
						 * already been passed. (This
						 * routine may be called
						 * multiple times for a single
						 * key event. It is called *at
						 * least* once for each key
						 * event.)
						 */
					if (last_im_event_processed ==
						Ol_last_event_received)
					{
					   length_return = last_im_length;
					   keysym_return = last_im_keysym;
					   strcpy(buf, last_im_buffer);	
					   retval = WhatOlKey( w, event,
							      textedit_flag);

					}
					else
					{
					   last_im_event_processed =
							Ol_last_event_received;
					   length_return = OlLookupImString(
							(XKeyEvent *)event,
							ic,
							buf,
							buf_size,
							&keysym_return,
							&im_status
						);
					   if (im_status == XBufferOverflow)
					   {
					/* try again with larger buffer */
						buf = XtRealloc(
							buf,length_return+1);
						buf_size = length_return;
				       /* realloc buffer for copy */
						last_im_buffer = XtRealloc(
							last_im_buffer,
							length_return+1);
						length_return =OlLookupImString(
							(XKeyEvent *) event,
							ic,
							buf,
							buf_size,
							&keysym_return,
							&im_status); 
					    }
					    switch (im_status){
						case XLookupNone:
						  length_return = 0;
						  retval = OL_UNKNOWN_KEY_INPUT;
						  break;
						case XLookupKeySym:
				/* if the KeySym is returned, check for
				 * a special Open Look Key. This assumes
				 * that the KeySym is the same at that
				 * generated by XLookupString for event
				 */
						   retval = WhatOlKey(
								w, event,
								textedit_flag);
						   length_return =XLookupString(
							(XKeyEvent *) event,
							buf, buf_size,
							&keysym_return,
							&status_return
						   );
						   strcpy(last_im_buffer,buf);
						   break;
						case XLookupBoth:
				/* KeySym and String returned. See comment
				 * above about KeySym
				 */
						   retval = WhatOlKey(
								w, event,
								textedit_flag);
						   buf[length_return] = 0;	
						   strcpy(last_im_buffer,buf);
						   break;
						case XLookupChars:
				/* String returned, NULL terminate and
				 * save copy
				 */
						   buf[length_return] = 0;	
						   strcpy(last_im_buffer,buf);
						   retval =OL_UNKNOWN_KEY_INPUT;
						   break;
						case XBufferOverflow:
				/* give up */
							/* force this */
						   length_return = 0;
						   retval =OL_UNKNOWN_KEY_INPUT;
						   break;
						default:
						   break;
					    }
					    last_im_length = length_return;
					    last_im_keysym = keysym_return;
					}
				}else{
					/*
					 * Widget is not using an input method; 
					 * Check for a virtual OL key.
					 */
					retval = WhatOlKey(
							w, event,
							textedit_flag);
					
					/* get real keysym_return */
					length_return = XLookupString(
							(XKeyEvent *)event,
							buf, buf_size,
							&keysym_return,
							&status_return);
				}
			}else{
				/*
				 * Widget is not a subclass of primitive,
				 * no input method is being used.
				 */
				retval = WhatOlKey(w, event, textedit_flag);
					
				/* get real keysym_return */
				length_return = XLookupString(
							(XKeyEvent *)event,
							buf, buf_size,
							&keysym_return,
							&status_return);
			}
		if (keysym != NULL)
			*keysym = keysym_return;
		if (buffer != NULL)
			*buffer = buf;
		if (length != NULL)
			*length = length_return;
		if (IsDampableKey(textedit_flag, keysym_return))
			while (XCheckIfEvent(
					XtDisplayOfObject(w),
					&newevent, IsSameKeyEvent,
					(char *) event))
				;
#else
			retval = WhatOlKey(w, event, textedit_flag);

				/* get real keysym_return */
			length_return = XLookupString(
						(XKeyEvent *)event,
						buffer_return, BUFFER_SIZE,
						&keysym_return, &status_return);
			if (keysym != NULL)
				*keysym = keysym_return;
			if (buffer != NULL)
				*buffer = buffer_return;
			if (length != NULL)
				*length = length_return;
			if (IsDampableKey(textedit_flag, keysym_return))
				while (XCheckIfEvent(
						XtDisplayOfObject(w),
						&newevent, IsSameKeyEvent,
						(char *) event))
					;
#endif

      			break;
   		case ButtonPress:
   		case ButtonRelease:
				/* WhatOlBtn returns OL_UNKNOWN_INPUT because*/
				/* crossing/motion aren't the button events  */

			retval = WhatOlBtn(event->xbutton.state,
					    event->xbutton.button);
			if (retval == OL_UNKNOWN_INPUT)
      				retval = OL_UNKNOWN_BTN_INPUT;
      			break;
		case EnterNotify:
		case LeaveNotify:
			if (textedit_flag == True)
				break;
			retval = WhatOlBtn (event->xcrossing.state, 0);
			break;
		case MotionNotify:
			if (textedit_flag == True)
				break;
			retval = WhatOlBtn (event->xmotion.state, 0);
			break;
   		default:
      			break;
   	}
	return (retval);
} /* end of DoLookup */

/*
 * DynamicHandler - this routine is used to monitor changes of the
 * RESOURCE_MANAGER property that occur on the RootWindow.  When this
 * property changes, several application databases need to be
 * re-initialized.
 */
/* ARGSUSED */
static void
DynamicHandler OLARGLIST((w, client_data, event, cont_to_dispatch))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)	/* ignored	*/
	OLARG( XEvent *,	event)
	OLGRA( Boolean *,	cont_to_dispatch)
{
	if ((event-> type == PropertyNotify) &&
	    (event-> xproperty.atom == XA_RESOURCE_MANAGER) &&
	    (event-> xproperty.state == PropertyNewValue))
	{
		int	i;
		int	dummy = -1; /* need to reset sentinel */

			/* so we know we are doing the dynamic changes */
		_OlDynResProcessing = True;

				/* Re-initialize the Xt Database
				 */
		GetNewXtdefaults(w);

				/* Re-initialize the Virtual keys and buttons
				 */
		for (i = 0; i < avail_dbs_entries; i++)
			UpdateIEDB(w, avail_dbs[i]);

				/* Re-initialize the global application's
				 * resources.
				 */
		_OlInitAttributes(w);

				/* Now that we've re-initialized the
				 * application's resources, update
				 * local copies of them.
				 */
		UpdateLocal(w);

				/* Do OPENLOOK dynamic resource processing */
		_OlDynResProc();

				/* update list of keys for IM */
		OlGetOlKeysForIm(NULL, &dummy, &dummy);

				/* Call the Dynamic callbacks last */
		OlCallDynamicCallbacks();

			/* always turn it off when operaiton is completed */
		_OlDynResProcessing = False;
	}
} /* end of DynamicHandler */

/*
 * GetMoreMem - This routine will alloc more memory if necessary.
 */
static XtPointer
GetMoreMem OLARGLIST((list, size, more_slots, num_entries, num_slots_alloced))
	OLARG( XtPointer,	list)
	OLARG( int,		size)
	OLARG( int,		more_slots) /*additional slots when reallocing*/
	OLARG( short,		num_entries)
	OLGRA( short *,		num_slots_alloced)
{
	XtPointer	new_list;

		/* XtRealloc() will call XtMalloc() if list is NULL */
	if (*num_slots_alloced == num_entries)
	{
		*num_slots_alloced += more_slots;
		new_list = XtRealloc (list, size * (*num_slots_alloced));
	}
	else
		new_list = list;

	return (new_list);
} /* end of GetMoreMem */

static void
GetNewXtdefaults OLARGLIST((w))
	OLGRA( Widget,	w)
{
	Display * dpy = XtDisplay(w);
	char *	string_db = GetStringDb(dpy);
	XrmDatabase db;
#if defined(NEED_XNLLANGUAGE_UPDATE)
	static char * resource_name = "xnlLanguage";
	static char * resource_class = "XnlLanguage";
	char *resource_type;
	static XrmValue value;
	XtPerDisplay pd = _XtGetPerDisplay(dpy);
#endif

	if (string_db != (char *)NULL)
	{
		XrmDatabase rdb = XrmGetStringDatabase(string_db);

			/* free the string database	*/
		Xfree(string_db);

			/* Merge the new database into the existing
			 * Intrinsics database.  Note, the merge
			 * is destructive for 'rdb,' so we don't
			 * have to free it explicitly.
			 */
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
		db = XtScreenDatabase(XtScreenOfObject(w));
#else
		db = dpy->db;
#endif
		if (rdb != NULL) {
			XrmMergeDatabases(rdb, &db);
#if defined(NEED_XNLLANGUAGE_UPDATE)
				/*
				 * The merge corrupts the language
				 * pointer in the per display
				 * structure if the xnlLanguage resource
				 * was in both databases.  Update the pointer
				 * with the address from the merged
				 * database.
				 */
			if (XrmGetResource(db, resource_name, 
					resource_class, &resource_type,
					&value) == True)
			{
				pd->language =  value.addr;
			}else{
	  			OlVaDisplayWarningMsg(dpy,
			      		OleNfileDynamic,
			      		OleTmsg4,
			      		OleCOlToolkitWarning,
			      		OleMfileDynamic_msg4);
			}
#endif
		}
	}
} /* end of GetNewXtdefaults */

/*
 * GetStringDb
 *
 * This procedure reads the RESOURCE_MANAGER property of the Root Window
 * and returns it.
 * This code was swiped (almost verbatim) from Xlib (XConnectDisplay).
 *
 */
static char *
GetStringDb OLARGLIST((dpy))
	OLGRA( Display *,	dpy)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long leftover;
	char *	string_db;

	if (XGetWindowProperty(dpy, RootWindow(dpy, 0),
	    XA_RESOURCE_MANAGER, 0L, 100000000L, False, XA_STRING,
	    &actual_type, &actual_format, &nitems, &leftover,
	    (unsigned char **) &string_db) != Success)
	{
                        string_db = (char *) NULL;
	}
	else if ((actual_type != XA_STRING) || (actual_format != 8))
	{
		if (string_db != NULL) {
			Xfree ( string_db );
                        string_db = (char *) NULL;
                }
	}
	return(string_db);
} /* end of GetStringDb */

static void
GrabKey OLARGLIST((w, keysym, modifiers, owner_events, pointer_mode, kbd_mode))
	OLARG( Widget,			w)
	OLARG( KeySym,			keysym)
	OLARG( Modifiers,		modifiers)
	OLARG( Boolean,			owner_events)
	OLARG( int,			pointer_mode)
	OLGRA( int,			kbd_mode)
{
	KeyCode			keycode;

	grab_want_shift_mask = False;
	(void)_OlCanonicalKeysym (
		XtDisplayOfObject(w),
		keysym,
		&keycode,
		&modifiers
	);
	if (!keycode)
		return;

	if (grab_want_shift_mask)
		modifiers |= ShiftMask;

		/* we want to trap the BadAccess error only when doing	*/
		/* the dynamic changes	 				*/
	if (_OlDynResProcessing)
	{
		gb_old_error_handler = XSetErrorHandler(CatchBadAccessError);
		gb_flag = True;
		gb_w = w;
		gb_keycode = keycode;
		gb_modifiers = modifiers;
		gb_owner_events = owner_events;
		gb_pointer_mode = pointer_mode;
		gb_keyboard_mode = kbd_mode;
	}

	XtGrabKey(w, keycode, modifiers, owner_events,
		  pointer_mode, kbd_mode);

	if (_OlDynResProcessing)
	{
			/* do XSync() to force all events get processed */
			/* before resetting the old error handler back	*/
		XSync(XtDisplayOfObject(w), 0);
		(void) XSetErrorHandler(gb_old_error_handler);
		gb_flag = False;
		gb_old_error_handler = NULL;
	}
} /* end of GrabKey */

static void	
IEDBPopCB OLARGLIST((w, client_data, call_data))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)
	OLGRA( XtPointer,	call_data)
{
	OlWidgetUnsearchIEDB(w, (OlVirtualEventTable)client_data);
} /* end of IEDBPopCB */

/*
 * InitSortedKeyDB- Produce db->sorted_key_db by a given db
 */
static void
InitSortedKeyDB OLARGLIST((db))
	OLGRA( OlVirtualEventTable,	db)
{
#define SORTEDDB	db->sorted_key_db
#define NUMKEYS		db->sorted_key_db[0].i
#define DEFUSED		db->key_bindings[i].def.used
#define TABLE		SORTEDDB[NUMKEYS]

	int		num_keys = 0;
	int		i, j;

	if (db == NULL || db->num_key_bindings == 0)
		return;

	num_keys = db->num_key_bindings * MAXDEFS;

	if (SORTEDDB == NULL)
		SORTEDDB = (Token *) XtMalloc(sizeof(Token) * (num_keys + 1));

	NUMKEYS = 0;

	for (i = 0; i < db->num_key_bindings; i++)
	{
		for (j = 0; j < DEFUSED; j++)
		{
			NUMKEYS++;
			TABLE.i = (short) i;
			TABLE.j = (short) j;
		}
	}
	current_key_bindings_to_sort = db->key_bindings;
	qsort (&db->sorted_key_db[1], NUMKEYS, sizeof(Token), CompareFunc);

#undef SORTEDDB
#undef NUMKEYS
#undef DEFUSED
#undef TABLE
} /* end of InitSortedKeyDB */

static Boolean
IsComposeBtnOrKey OLARGLIST((is_keydef, name, which_db, which_binding))
	OLARG( Boolean,		is_keydef)
	OLARG( String,		name)
	OLARG( int *,		which_db)
	OLGRA( int *,		which_binding)
{
#define NUMKEYS		avail_dbs[i]->num_key_bindings
#define KEYNAME		avail_dbs[i]->key_bindings[j].name
#define KEYUSED		avail_dbs[i]->key_bindings[j].def.used
#define NUMBTNS		avail_dbs[i]->num_btn_bindings
#define BTNNAME		avail_dbs[i]->btn_bindings[j].name
#define BTNUSED		avail_dbs[i]->btn_bindings[j].def.used

	int		i, j;
	Boolean		ret_val = False,
			done_flag = False;

	for (i = 0; i < avail_dbs_entries; i++)
	{
		if (is_keydef == True)
		{
			for (j = 0; j < NUMKEYS; j++)
			{
				if (STREQU(name, KEYNAME))
				{
					done_flag = True;
					if (KEYUSED != 0)
						ret_val = True;
					break;
				}
			}
		}
		else	/* is_btn_def */
		{
			for (j = 0; j < NUMBTNS; j++)
			{
				if (STREQU(name, BTNNAME))
				{
					done_flag = True;
					if (BTNUSED != 0)
						ret_val = True;
					break;
				}
			}
		}
		if (done_flag == True)
			break;
	}
	if (done_flag == True)
	{
		if (ret_val == False)
		{
		  OlVaDisplayWarningMsg((Display *) NULL,
					OleNfileDynamic,
					OleTmsg2,
					OleCOlToolkitWarning,
					OleMfileDynamic_msg2,
					name);
		  
		}
		else
		{
			*which_db = i;
			*which_binding = j;
		}
	}
	return (ret_val);

#undef NUMKEYS
#undef KEYNAME
#undef KEYUSED
#undef NUMBTNS
#undef BTNNAME
#undef BTNUSED
} /* end of IsComposeBtnOrKey */

/*
 * IsSameKeyEvent
 *
 */
/* ARGSUSED */
static Bool
IsSameKeyEvent OLARGLIST((d, event, arg))
	OLARG( Display *,	d)		/* ignored	*/
	OLARG( XEvent *,	event)
	OLGRA( char *,		arg)
{
	XEvent * e = (XEvent *) arg;

	return(Bool)(event-> type         == e-> type &&
		     event-> xkey.window  == e-> xkey.window &&
		     event-> xkey.state   == e-> xkey.state &&
		     event-> xkey.keycode == e-> xkey.keycode);
} /* end of IsSameKeyEvent */

static Boolean
ParseKeysymList OLARGLIST((p_str, p_modifiers, p_detail))
	OLARG( String *,	p_str)
	OLARG( Modifiers *,	p_modifiers)
	OLGRA( String *,	p_detail)
{
	String			p		= *p_str;
	String			lbra;
	String			rbra;
	String			token;

	Cardinal		i;


	if (!p)
		return (False);

	/*
	 * Skip over leading ``whitespace''.
	 */
	p += strspn(p, DEF_SEPS);
	if (!*p)
		return (False);

	/*
	 * General syntax:
	 *
	 *	modifiers <keysym> [ sep modifiers <keysym> ... ]
	 *
	 * Each item in the list MUST have a <keysym>, thus the
	 * '<' (LBRA) and '>' (RBRA) are dominant, and we can scan for
	 * them before looking for the "sep". This allows for separator
	 * characters within the brackets, such as <,>, and allows one
	 * to use the same separator characters between modifiers as
	 * between items in the list. In fact, it allows one to forego
	 * using any separators whatsoever, as in:
	 *
	 *	Shift<a>Ctrl<b>		(identical to Shift<a>,Ctrl<b>)
	 */
	lbra = strchr(p, LBRA);
	if (!lbra)
		return (False);
	rbra = strchr(lbra, RBRA);
	if (!rbra)
		return (False);

	/*
	 * Allow for this: <>>
	 */
	if (rbra[1] == RBRA)
		rbra++;

	/*
	 * Skip over trailing ``whitespace'', to align with next
	 * item in the list.
	 */
	*p_str = rbra + strspn(rbra+1, DEF_SEPS);

	/*
	 * Set up returned detail:
	 */
	*p_detail = lbra + 1;
	*rbra = 0;

	/*
	 * Convert string of modifiers to bit-masks.
	 */
	*lbra = 0;
	*p_modifiers = 0;
	for (
		token = strtok(p, MOD_SEPS);
		token;
		token = strtok((String)0, MOD_SEPS)
	)
 		for (i = 0; i < XtNumber(mappings); i++)
			if (STREQU(mappings[i].s, token)) {
				*p_modifiers |= (Modifiers)mappings[i].m;
				break;
			}

	return (True);
} /* end of ParseKeysymList */

/* ARGSUSED */
static void
RegrabVirtualKeys OLARGLIST((ignore))
	OLGRA( XtPointer,	ignore)
{
	GrabbedVirtualKey *	p	= grab_list;
	GrabbedVirtualKey *	pend	= grab_list + grab_list_size;

	OlKeyDef *		kdAsIs;
	OlKeyDef *		kdToBe;

	Cardinal		i;
	Cardinal		j;

	Boolean			know_show	= False;

	OlDefine		show;


	if (!p)
		return; /* nothing grabbed */

#define UNGRAB(p,kd,i) \
	UngrabKey((p)->w,(kd)->keysym[i],(kd)->modifier[i])

#define GRAB(p,kd,i) \
	GrabKey ((p)->w,(kd)->keysym[i],(kd)->modifier[i], \
		 (p)->owner_events,(p)->pointer_mode,(p)->keyboard_mode)

#define SAMEKEY(I,J) \
	kdAsIs->keysym[I]   == kdToBe->keysym[J]			\
     && kdAsIs->modifier[I] == kdToBe->modifier[J]

	for (; p < pend; p++) {
		kdAsIs = &(p->as_grabbed);
		kdToBe = &(p->kb->def);
		/*
		 * If this key is grabbed but the "showAccelerators"
		 * resource says don't do accelerators, ungrab the
		 * key. Do save new bindings (if any), because the
		 * bindings could have changed at the same time the
		 * user said to stop using them!
		 */
		if (!know_show) {
			/*
			 * Need a widget ID for this, that's why we delay
			 * the check until the first time through loop.
			 */
			show = OlQueryAcceleratorDisplay(p->w);
			know_show = True;
		}
		if (show == OL_INACTIVE) {
			if (p->grabbed) {
				for (i = 0; i < kdAsIs->used; i++)
					UNGRAB (p, kdAsIs, i);
				p->grabbed = False;
			}
			p->as_grabbed = *kdToBe;
		}
	}
	if (show == OL_INACTIVE)
		return;

	p = grab_list;
	for (; p < pend; p++) {
		Boolean			same;

		kdAsIs = &(p->as_grabbed);
		kdToBe = &(p->kb->def);

		/*
		 * Remove grabs for bindings we no longer have,
		 * and, if we had once removed grabs because the
		 * user told us to and now we are to put them back,
		 * regrab them. Between grabbing the new bindings
		 * above and regrabbing bindings that haven't
		 * changed, we'll get all the grabs (re)done.
		 */
		for (i = 0; i < kdAsIs->used; i++) {
			same = False;
			for (j = 0; j < kdToBe->used; j++)
				if (SAMEKEY(i,j)) {
					same = True;
					break;
				}
			if (!same)
				UNGRAB (p, kdAsIs, i);
			else if (!p->grabbed)
				GRAB (p, kdAsIs, i);
		}
	}

	p = grab_list;
	for (; p < pend; p++) {
		Boolean			same;

		kdAsIs = &(p->as_grabbed);
		kdToBe = &(p->kb->def);

		/*
		 * Set new grabs for bindings we didn't have
		 * before.
		 */
		for (j = 0; j < kdToBe->used; j++) {
			same = False;
			for (i = 0; i < kdAsIs->used; i++)
				if (SAMEKEY(i,j)) {
					same = True;
					break;
				}
			if (!same)
				GRAB (p, kdToBe, j);
		}

		p->grabbed = True;

		p->as_grabbed = *kdToBe;
	}

#undef	GRAB
#undef	UNGRAB
#undef	SAMEKEY
} /* end of RegrabVirtualKeys */

/**
 ** StringToButton()
 **/
static BtnSym
StringToButton OLARGLIST((str))
	OLGRA( String,		str)
{
	Cardinal		i;

	BtnSym			ret	= 0;


	if (str)
		for (i = 0; i < XtNumber(mappings); i++)
			if (STREQU(mappings[i].s, str)) {
				ret = (BtnSym)mappings[i].m;
				break;
			}
	return (ret);
} /* end of StringToButton */

/**
 ** StringToKey()
 **/
static KeySym
StringToKey OLARGLIST((display, str, p_modifiers))
	OLARG( Display *,	display)
	OLARG( String,		str)
	OLGRA( Modifiers *,	p_modifiers)
{
	Modifiers		modifiers;

	KeySym			keysym;

	Cardinal		i;


	/*
	 * Allow the user to give an ASCII short-hand notation.
	 * (I18N)
	 */
	if (str[0] && !isalnum(str[0]) && !str[1]) {
		/*
		 * Single non-alphanumeric character--good bet
		 * it's short-hand.
		 */
		keysym = _OlSingleCharToKeysym(str[0]);
		if (keysym != NoSymbol)
			return (keysym);
	} else if (str[0] == '\\' && str[1] && !str[2]) {
		/*
		 * Back-quoted single character--good bet it's short-hand.
		 */
		for (i = 0; i < XtNumber(backslash_map); i++)
			if (str[1] == backslash_map[i].single) {
				keysym = backslash_map[i].keysym;
				return (keysym);
			}
	}

	/*
	 * Reduce KeySym to unmodified version.
	 * For example:
	 *
	 *	<A>	->	Shift<a>
	 *	<F16>	->	Shift<F4>
	 */

	modifiers = (p_modifiers? *p_modifiers : 0);

	if (isupper(str[0]) && !str[1]) {
		modifiers |= ShiftMask;
		str[0] = tolower(str[0]);
	}

	keysym  = XStringToKeysym(str);
	if (keysym != NoSymbol)
		keysym = _OlCanonicalKeysym(	display, keysym,
						(KeyCode *)0, &modifiers);

	if (p_modifiers)
		*p_modifiers = modifiers;

	return (keysym);
} /* end of StringToKey */

static void
StringToKeyDefOrBtnDef OLARGLIST((display, is_keydef, from, result))
	OLARG( Display *,	display)
	OLARG( Boolean,		is_keydef)
	OLARG( String,		from)
	OLGRA( XtPointer,	result)
{
	OlKeyDef *		kd	= (OlKeyDef *)result;
	OlBtnDef *		bd	= (OlBtnDef *)result;

	String			copy	= Strdup(from);
	String			detail;
	String			p;

	Modifiers		modifiers, keep;

	int			i, j, k;

	p = copy;
	for (
		i = 0;
		i < MAXDEFS && ParseKeysymList(&p, &modifiers, &detail);
		i++
	) {
		if (is_keydef) {
			keep = modifiers;
			kd->keysym[i]   = StringToKey(	display,
							detail, &modifiers);
			kd->modifier[i] = modifiers;

			if (kd->keysym[i] == NoSymbol)
			{
				if (IsComposeBtnOrKey(True, detail,
							&j, &k) == False)
					i--;
				else
				{
					*kd = avail_dbs[j]->key_bindings[k].def;
					for (i = 0; i < kd->used; i++)
						kd->modifier[i] |= keep;
					break;
				}
			}
		} else {
			bd->modifier[i] = modifiers;
			bd->button[i]   = StringToButton(detail);

			if (bd->button[i] == None)
			{
				if (IsComposeBtnOrKey(False, detail,
							&j, &k) == False)
					i--;
				else
				{
					*bd = avail_dbs[j]->btn_bindings[k].def;
					for (i = 0; i < bd->used; i++)
						bd->modifier[i] |= modifiers;
					break;
				}
			}
		}
	}
	if (is_keydef)
		kd->used = i;
	else
		bd->used = i;

	Free (copy);

#ifdef DEBUG1K
 if (is_keydef == True)
 {
	fprintf (stderr, "%22s ", from);
	for (i = 0; i < kd->used; i++)
		fprintf (stderr, "(0x%x, 0x%x)",
			 kd->modifier[i], kd->keysym[i]);
 	fprintf (stderr, "\n");
 }
#endif
#ifdef DEBUG1B
 if (is_keydef == False)
 {
	fprintf (stderr, "%22s ", from);
	for (i = 0; i < bd->used; i++)
		fprintf (stderr, "(0x%x, 0x%x)",
			 bd->modifier[i], bd->button[i]);
 	fprintf (stderr, "\n");
 }
#endif
} /* end of StringToKeyDefOrBtnDef */

/**
 ** UngrabKey()
 **/
static void
UngrabKey OLARGLIST((w, keysym, modifiers))
	OLARG( Widget,		w)
	OLARG( KeySym,		keysym)
	OLGRA( Modifiers,	modifiers)
{
	KeyCode			keycode;

	grab_want_shift_mask = False;
	(void)_OlCanonicalKeysym (
		XtDisplayOfObject(w),
		keysym,
		&keycode,
		&modifiers
	);
	if (!keycode)
		return;

	if (grab_want_shift_mask)
		modifiers |= ShiftMask;
	XtUngrabKey (w, keycode, modifiers);
} /* end of UngrabKey */

/*
 * UpdateIEDB - this routine is called to update a given DB
 */
static void
UpdateIEDB OLARGLIST((w, db))
	OLARG( Widget,			w)
	OLGRA( OlVirtualEventTable,	db)
{
	register int		i = 0;
	XtResource		r[100];

	if (db == NULL)
	{
#ifdef DEBUG4
 fprintf (stderr, "UpdateIEDB: NULL db\n");
#endif
		return;
	}

	if (_OlMax(db->num_key_bindings, db->num_btn_bindings) > XtNumber(r))
	{
	  OlVaDisplayErrorMsg(XtDisplay(w),
			      OleNfileDynamic,
			      OleTmsg1,
			      OleCOlToolkitError,
			      OleMfileDynamic_msg1);
	}

	for (i = 0; i < db->num_key_bindings; i++)
	{
		r[i].resource_name   = (String)db->key_bindings[i].name;
		r[i].resource_class  = (String)db->key_bindings[i].name;
		r[i].resource_type   = XtROlKeyDef;
		r[i].resource_size   = sizeof(OlKeyDef);
		r[i].resource_offset = (Cardinal) ((char *)
			&(db->key_bindings[i].def)-(char *)db->key_bindings);
		r[i].default_type    = XtRString;
		r[i].default_addr    = (XtPointer)
					db->key_bindings[i].default_value;
	}
	XtGetApplicationResources(w, (XtPointer)db->key_bindings,r,i,NULL,0);

	for (i = 0; i < db->num_btn_bindings; i++)
	{
		r[i].resource_name   = (String)db->btn_bindings[i].name;
		r[i].resource_class  = (String)db->btn_bindings[i].name;
		r[i].resource_type   = XtROlBtnDef;
		r[i].resource_size   = sizeof(OlBtnDef);
		r[i].resource_offset =  (Cardinal)((char *)
			&(db->btn_bindings[i].def)-(char *)db->btn_bindings);
		r[i].default_type    = XtRString;
		r[i].default_addr    = (XtPointer)
					db->btn_bindings[i].default_value;
	}
	XtGetApplicationResources(w, (XtPointer)db->btn_bindings,r,i,NULL,0);

#ifdef DEBUG3
 {
#define BINDINGS	db->key_bindings
#define TOTAL		db->num_key_bindings
#define NAME		BINDINGS[i].name
#define OLCMD		BINDINGS[i].ol_event
#define DEFUSED		BINDINGS[i].def.used
#define MODIFIER	BINDINGS[i].def.modifier[j]
#define KEYSYM		BINDINGS[i].def.keysym[j]

 int	i, j;

 fprintf (stderr, "\n");
 for (i = 0; i < TOTAL; i++)
 {
	fprintf (stderr, "%20s %d %d ", NAME, OLCMD, DEFUSED);
	for (j = 0; j < DEFUSED; j++)
		fprintf (stderr, "(0X%x 0X%x) ", MODIFIER, KEYSYM);
	fprintf (stderr, "\n");
 }
 fprintf (stderr, "\n");

#undef BINDINGS
#undef TOTAL
#undef KEYSYM

#define BINDINGS	db->btn_bindings
#define TOTAL		db->num_btn_bindings
#define BUTTON		BINDINGS[i].def.button[j]

 fprintf (stderr, "\n");
 for (i = 0; i < TOTAL; i++)
 {
	fprintf (stderr, "%20s %d %d ", NAME, OLCMD, DEFUSED);
	for (j = 0; j < DEFUSED; j++)
		fprintf (stderr, "(0X%x 0X%x) ", MODIFIER, BUTTON);
	fprintf (stderr, "\n");
 }
 fprintf (stderr, "\n");

#undef BINDINGS
#undef TOTAL
#undef NAME
#undef OLCMD
#undef DEFUSED
#undef MODIFIER
#undef BUTTON
 }
#endif

	InitSortedKeyDB (db);
} /* end of UpdateIEDB */

/*
 *************************************************************************
 * UpdateLocal - this routine caches two global
 * application resources.  They are are cached to save some of the
 * event handling routines from doing an expensive look up every time
 * they process an event.
 * This routine is called by _OlInitDynamicHandler and from
 * DynamicHandler.
 *************************************************************************
 */
static void
UpdateLocal OLARGLIST((w))
	OLGRA( Widget,	w)
{
	Arg	args[3];

	XtSetArg(args[0], XtNdontCare, &local.dont_care_bits);
	XtSetArg(args[1], XtNkeyRemapTimeOut, &local.key_remap_timeout);
	XtSetArg(args[2], XtNkeyDontCare, &local.key_dont_care_bits);

	OlGetApplicationValues(w, args, 3);
} /* end of UpdateLocal */

/*
 * WhatOlBtn - this routine returns an O/L button command by
 *	using "button" and "state".
 *	if "button" == 0, the query is from a motion, enter, or
 *	leave event. it will always do a perfect match.
 *
 *	note: this routine assumes the BuildIEDBstack has been called
 *		before getting here.
 */
static OlVirtualName
WhatOlBtn OLARGLIST((state, button))
	OLARG( unsigned int,	state)		/* state field in XEvent */
	OLGRA( unsigned int,	button)		/* button detail */
{
	int		i, j, k,
			button_type = 0;	/* button detail */

	Modifiers       dont_care = local.dont_care_bits;

#ifdef DEBUG2B
 if (button)
	printf ("(state, button): (0x%x, %d)", state, button);
#endif

	state &= ~dont_care;
	
	if (button != 0)	/* button event */
		button_type = button;
				/* motion, or crossing event, so	*/
	else			/* extract ButtonMask(s) from state	*/
	{
		for (i = 0; i < XtNumber(btn_mappings); i++)
			if ((state & btn_mappings[i].button_mask) != 0)
			{
				button_type = btn_mappings[i].button;
					/* rm it from state */
				state &= ~btn_mappings[i].button_mask;
				break;
			}
	}
	if (button_type == 0)	/* a crossing or motion event with no btn dn */
		return (OL_UNKNOWN_INPUT);

#define BTNBINDINGS	db_stack[i]->btn_bindings
#define DEFUSED		BTNBINDINGS[j].def.used
#define MODIFIER	BTNBINDINGS[j].def.modifier[k]
#define BUTTON		BTNBINDINGS[j].def.button[k]
#define OLCMD		BTNBINDINGS[j].ol_event

	for (i = 0; i < db_stack_entries; i++)
		for (j = 0; j < db_stack[i]->num_btn_bindings; j++)
			for (k = 0; k < DEFUSED; k++)
				if (MODIFIER == (state & 0xff) &&
				    BUTTON == button_type)
					{
#ifdef DEBUG2B
 if (button)
 	printf ("\tgot %s(%d)\n", BTNBINDINGS[j].name, OLCMD);
#endif
						return (OLCMD);
					}

#ifdef DEBUG2B
 if (button)
 	printf ("\tgot unknown\n");
#endif

	return (OL_UNKNOWN_INPUT);

#undef BTNBINDINGS
#undef DEFUSED
#undef MODIFIER
#undef BUTTON
#undef OLCMD
} /* end of WhatOlBtn */

static OlVirtualName
WhatOlKey OLARGLIST((w, xevent, textedit_flag))
	OLARG( Widget,		w)
	OLARG( XEvent *,	xevent)
	OLGRA( Boolean,		textedit_flag)	/* for backward compat. */
{
#define SORTEDDB	db_stack[i]->sorted_key_db
#define NUMKEYS		SORTEDDB[0].i
#define KEYBINDINGS	db_stack[i]->key_bindings
#define INDEX		SORTEDDB[index].i

	int		i, index, per;
	KeySym		keysym_return, lower, upper;
	KeySym *	syms;
	KeyCode		min_keycode;
	Modifiers	modifiers;
	OlVirtualName	retval = OL_UNKNOWN_KEY_INPUT;


	modifiers = xevent->xkey.state & ~local.key_dont_care_bits;

		/* see Accelerate.c:OlEventToKeyEvent() for detailts */
	syms = XtGetKeysymTable(XtDisplayOfObject(w), &min_keycode, &per);
	syms += (xevent->xkey.keycode - min_keycode) * per;

	if (per > 1 && syms[1] != NoSymbol)
	{
		lower = syms[0];
		upper = syms[1];
	}
	else
		XtConvertCase (XtDisplayOfObject(w), syms[0], &lower, &upper);

		/* s<space> will not work without "!isspace()" check.	*/
		/* this is because <space> is ASCII but not ALPHA...	*/
	if ((modifiers & ShiftMask) && isascii(upper) &&
		!isspace(upper) && !isalpha(upper))
	{
		keysym_return = upper;
		modifiers &= ~ShiftMask;
	}
	else if ((modifiers & ShiftMask) && IsKeypadKey(upper)) {
		keysym_return = upper;
		modifiers &= ~ShiftMask;
	}
	else
		keysym_return = lower;

#ifdef DEBUG2K
 printf ("(mod, state, keysym): (0x%x, 0x%x, 0x%x)",
		modifiers, xevent->xkey.state, keysym_return);
#endif
      	if (CanBeBound(textedit_flag, keysym_return, xevent->xkey.state))
      	{
		for (i = 0; i < db_stack_entries; i++)
			if ((index = BinarySearch(
					keysym_return,
					modifiers,
					SORTEDDB,
					(int) NUMKEYS,
					KEYBINDINGS)) != -1)
			{
				retval = KEYBINDINGS[INDEX].ol_event;
				break;
			}
      	}
#ifdef DEBUG2K
 if (retval == OL_UNKNOWN_KEY_INPUT)
	printf ("\tgot unknown key(%d)\n", retval);
 else
	printf ("\tgot %s(%d)\n", KEYBINDINGS[INDEX].name, retval);
#endif
		return (retval);

#undef SORTEDDB
#undef NUMKEYS
#undef KEYBINDINGS
#undef INDEX
} /* end of WhatOlKey */

void
OlGetOlKeysForIm OLARGLIST ((a_m_list, old_index, num_keys))
	OLARG(OlMAndAList **,	a_m_list)
	OLARG(int *,		old_index)
	OLGRA(int *,		num_keys)
{

	int		i, j, l;
	int		k = 0;
	int		num = 0;
	static int	olim_key_index = 0;
	OlMAndAList *	m_a_list = NULL;

	/* a value of -1 is a special value to indicate this function
	   that the key binding(s) may have changed as a result of
	   change in RESOURCE_MANAGER property. Note that we would come
	   in this function with -1 only from DynamicHandler().
	*/
	if (*old_index == -1)
		{
		olim_key_index++;
		return;
		}
	if (*old_index == olim_key_index)
		return;

	for (i = 0; i < db_stack_entries; i++)
		num += db_stack[i]->num_key_bindings * 2;

       m_a_list = (OlMAndAList *) XtMalloc(sizeof(OlMAndAList) * num);

	for (i = 0; i < db_stack_entries; i++)
	  {
	  for (l = 0; l < db_stack[i]->num_key_bindings; l++)
		{
	        switch(db_stack[i]->key_bindings[l].ol_event)
		    {
		    /* button equivalents */
		    case OL_SELECTKEY		:
   		    case OL_MENUKEY  		:
   		    case OL_MENUDEFAULTKEY	:
   		    case OL_HSBMENU		:
   		    case OL_VSBMENU		:
   		    case OL_ADJUSTKEY		:
   		    case OL_DUPLICATEKEY	:
    
		    /* traversal */
		    case OL_MOVEDOWN		:
		    case OL_MOVELEFT		:
		    case OL_MULTIDOWN		:
		    case OL_MULTILEFT		:
		    case OL_MULTIRIGHT		:
		    case OL_MULTIUP		:
		    case OL_NEXT_FIELD		:
		    case OL_PREV_FIELD		:
		    case OL_MOVERIGHT		:
		    case OL_MOVEUP		:
		    case OL_MENUBARKEY		:
    
		    /* misc */
		    case OL_CANCEL		:
		    case OL_DEFAULTACTION	:
		    case OL_DRAG		:
		    case OL_DROP		:
		    case OL_HELP		:
		    case OL_PROPERTY		:
		    case OL_STOP		:
    
		    /* scrolling */
		    case OL_PAGEDOWN		:
		    case OL_PAGELEFT		:
		    case OL_PAGERIGHT		:
		    case OL_PAGEUP		:
		    case OL_SCROLLBOTTOM	:
		    case OL_SCROLLDOWN		:
		    case OL_SCROLLLEFT		:
		    case OL_SCROLLLEFTEDGE	:
		    case OL_SCROLLRIGHT		:
		    case OL_SCROLLRIGHTEDGE	:
		    case OL_SCROLLTOP		:
		    case OL_SCROLLUP		:
    
#define STUFF	db_stack[i]->key_bindings[l].def
			for (j = 0; j < STUFF.used; j++)
			{
				m_a_list[k].keysym = STUFF.keysym[j];
				m_a_list[k].modifier = STUFF.modifier[j];
				*num_keys = *num_keys + 1;
				k++;
			}
#undef STUFF
			break;
		default:
			continue;
	
		}
	    }
	   }
	/* now update the info. so that we do not waste our time if things
	   have not changed
	*/ 
	*a_m_list = m_a_list;
	*old_index = olim_key_index;
}	/* End of OlGetOlKeysForIm */

void
OlGetMAndAList OLARGLIST((pe, m_a_list, num_keys, is_mnemonic))
	OLARG (OlVendorPartExtension,		pe)
	OLARG (OlMAndAList **,			m_a_list)
	OLARG (int *,				num_keys)
	OLGRA (Boolean,				is_mnemonic)
{

	int			i;
	int			nel;
	OlMAndAList	*	a_m_list = NULL;



	nel = pe->accelerator_list->nel;


	/* if there was no accelerators and no mnemonic, simply return */
	if (nel == 0)
		{
		*m_a_list = NULL;
		return;
		}

	a_m_list = (OlMAndAList *) XtMalloc(sizeof(OlMAndAList) * nel);

	for (i = 0; i < nel; i++)
		if (pe->accelerator_list->base[i].is_mnemonic == is_mnemonic)
			{
			a_m_list[*num_keys].keysym = pe->accelerator_list->
				              base[i].keysym;
			a_m_list[*num_keys].modifier = pe->accelerator_list->
				        	base[i].modifiers;
			*num_keys = *num_keys + 1;
			}

	if (*num_keys == 0)
		{
		XtFree((char *)a_m_list);
		a_m_list = NULL;
		}

	*m_a_list = a_m_list;

	return;
}	/* End of OlGetMAndAList */

extern OlVirtualName
_OlKeySymToVirtualKeyName OLARGLIST((keysym, modifiers, name))
	OLARG( KeySym,		keysym)
	OLARG( Modifiers,	modifiers)
	OLGRA( String *,	name)
{
	Cardinal		i, j, k;
	OlVirtualName		ret_val = OL_UNKNOWN_KEY_INPUT;

#define NUM_KEYS	avail_dbs[i]->num_key_bindings
#define VIR_NAME	avail_dbs[i]->key_bindings[j].ol_event
#define THE_NAME	avail_dbs[i]->key_bindings[j].name
#define KEY_DEFN	avail_dbs[i]->key_bindings[j].def
#define KEY_USED	KEY_DEFN.used
#define EQU_SPEC	(keysym == KEY_DEFN.keysym[k] &&	\
			 modifiers == KEY_DEFN.modifier[k])

	for (i = 0; i < avail_dbs_entries; i++)
		for (j = 0; j < NUM_KEYS; j++)
			for (k = 0; k < KEY_USED; k++)
				if (EQU_SPEC)
				{
					ret_val = VIR_NAME;
					if (name)
						*name = (String)THE_NAME;
					break;
				}

	return(ret_val);
	
#undef NUM_KEYS
#undef VIR_NAME
#undef THE_NAME
#undef KEY_DEFN
#undef KEY_USED
#undef EQU_SPEC

} /* end of _OlKeySymToVirtualKeyName */
