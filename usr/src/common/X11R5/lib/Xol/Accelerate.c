/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)mouseless:Accelerate.c	1.40"
#endif

#include "string.h"
#include "ctype.h"

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include "Xol/OpenLookI.h"
#include "Xol/AcceleratP.h"
#include "Xol/DynamicP.h"
#include "Xol/VendorI.h"
#include "Xol/Menu.h"
#ifndef OLD_MENU
#include "Xol/PopupMenu.h"
#endif
#include "Xol/ConvertersI.h"
#include "Xol/EventObjP.h"
#include "Xol/Flat.h"

/*
 * Macros:
 */

/*
 * Types of actions that can be performed on an
 * accelerator or mnemonic (i.e. on a KeyEvent):
 */
#define KE_ADD			1
#define KE_FETCH		2
#define KE_REMOVE		3
#define KE_REMOVE_ALL		4   /* ingore key and data in search */
#define KE_REMOVE_USING_DATA	5   /* use only data in searching	*/

#if	!defined(STREQU)
# define STREQU(A,B) (strcmp((A),(B)) == 0)
#endif

#if	!defined(Malloc)
# define Malloc(N) XtMalloc(N)
#endif

#if	!defined(Realloc)
# define Realloc(P,N) XtRealloc(P,N)
#endif

#if	!defined(New)
# define New(M) XtNew(M)
#endif

#if	!defined(Free)
# define Free(M) XtFree(M)
#endif

#if	!defined(Strlen)
# define Strlen(S) ((S) && *(S)? strlen((S)) : 0)
#endif

#if	!defined(Strdup)
# define Strdup(S) strcpy(Malloc((unsigned)Strlen(S) + 1), S)
#endif

#if	!defined(Array)
# define Array(P,T,N) \
	((N)?								\
		  ((P)?							\
			  (T *)Realloc((P), sizeof(T) * (N))		\
			: (T *)Malloc(sizeof(T) * (N))			\
		  )							\
		: ((P)? (Free(P),(T *)0) : (T *)0)			\
	)
#endif

/*
 * Local routines:
 */

static OlDefine		AddAorM OL_ARGS((
	Widget			w,
	XtPointer		data,
	String			a_or_m,
	Boolean			is_mnemonic
));
static void		RemoveAorM OL_ARGS((
	Widget			w,
	XtPointer		data,
	Boolean			ignore_data,
	String			a_or_m,
	Boolean			is_mnemonic
));
static Widget		FetchAorM OL_ARGS((
	Widget			w,
	XtPointer *		p_data,
	OlVirtualEvent		ve,
	Boolean			is_mnemonic
));
static Widget		ActOnAorM OL_ARGS((
	Widget			w,
	XtPointer		data,
	KeyEvent *		item,
	Boolean			is_mnemonic,
	int			action,
	XtPointer *		returned_data,	/* only valid for KE_ADD */
	Boolean *		is_virtual_key	/* only valid for KE_ADD */
));
static KeyEvent *	StringToKeyEvent OL_ARGS((
	Widget			w,
	String			str,
	Boolean			decompose_virtual_name
));
static KeyEvent *	OlEventToKeyEvent OL_ARGS((
	Widget			w,
	OlVirtualEvent		ve
));
static KeyEvent *	FindKeyEvent OL_ARGS((
	KeyEvent *		base,
	register KeyEvent *	item,
	Cardinal		nel,
	KeyEvent **		p_insert
));
static OlVendorPartExtension	FetchVendorExtension OL_ARGS((
	Widget			w,
	Boolean			is_mnemonic,
	Boolean *		p_use_mnemonic_prefix,
	Boolean *		p_accelerators_do_grabs
));
static Boolean		UseMnemonicPrefix OL_ARGS((
	Widget			w
));
static void		GrabAccelerator OL_ARGS((
	Widget			w,
	KeyEvent *		ke,
	Widget *		shells,
	Cardinal		nshells,
	Boolean			grab
));
void			_OlNewAcceleratorResourceValues OL_ARGS((
	XtPointer		client_data
));
static void		RegisterChangeShowAccelerator OL_ARGS((
	Widget			w,
	Boolean			is_mnemonic
));
static void		UnregisterChangeShowAccelerator OL_ARGS((
	Widget			w
));

/*
 * Local data:
 */

typedef struct MaskName {
	Cardinal		p_name_offset;
	Modifiers		mask;
}			MaskName;

static MaskName		mask_names[] = {

#define offset(F) XtOffsetOf(_OlAppAttributes,F)
	{ offset(shift_name),   (Modifiers)ShiftMask   },
	{ offset(lock_name),    (Modifiers)LockMask    },
	{ offset(control_name), (Modifiers)ControlMask },
	{ offset(mod1_name),    (Modifiers)Mod1Mask    },
	{ offset(mod2_name),    (Modifiers)Mod2Mask    },
	{ offset(mod3_name),    (Modifiers)Mod3Mask    },
	{ offset(mod4_name),    (Modifiers)Mod4Mask    },
	{ offset(mod5_name),    (Modifiers)Mod5Mask    },
#undef offset

};

#define KEYSYM_POSITION 1
static char		fake_accelerator[4]	= { LBRA, 'X', RBRA, 0 };

/**
 ** _OlDestroyKeyboardHooks()
 **/

void
_OlDestroyKeyboardHooks OLARGLIST((w))
	OLGRA (Widget,	w)
{
	/*
	 * Remove all mnemonics and accelerators.
	 */
	RemoveAorM (w, (XtPointer)0, True, (String)0, True);
	RemoveAorM (w, (XtPointer)0, True, (String)0, False);

	/*
	 * Disassociate this widget from all others.
	 */
	OlUnassociateWidget (w);

	/*
	 * Remove from traversal list.
	 */
	_OlDeleteDescendant (w);

	/*
	 * If this is the default widget, make default widget NULL.
	 */
	_OlSetDefault(w, False);

	/*
	 * If this is the initial focus widget, set it to NULL.
	 */
	{
		OlFocusData *	fd = _OlGetFocusData(w, NULL);

		if (fd != NULL && fd->initial_focus_widget == w)
		{
			fd->initial_focus_widget = NULL;
		}
	}

	return;
}

/**
 ** _OlMakeAcceleratorText()
 **/

String
_OlMakeAcceleratorText OLARGLIST((w, str))
	OLARG (Widget,		w)
	OLGRA (String,		str)
{
	KeyEvent *		ke	= StringToKeyEvent(w, str, False);

	Cardinal		len	= 0;
	Cardinal		i;

	String			detail;
	String			ret;

	static char		ssingle[2] = { 0 , 0 };
	char			single;

	_OlAppAttributes *	base	= _OlGetAppAttributesRef(w);

#define NAME(I)	*(String *)((char *)base + mask_names[(I)].p_name_offset)


	if (!ke || ke->keysym == NoSymbol)
		return (0);

	if ((single = _OlKeysymToSingleChar(ke->keysym)))
		*(detail = ssingle) = single;
	else if (!(detail = XKeysymToString(ke->keysym)))
		return (0);

	/*
	 * Count the number of bytes needed to form the accelerator text,
	 * then allocate that much space (plus one for our friend, the
	 * Terminating Null).
	 */

	for (i = 0; i < XtNumber(mask_names); i++)
		if (mask_names[i].mask & ke->modifiers)
			len += Strlen(NAME(i)) + 1;

	len += Strlen(detail);

	ret = Malloc(len + 1);
	*ret = 0;

	/*
	 * Now run through similar code, but copy the pieces this time.
	 */

	for (i = 0; i < XtNumber(mask_names); i++)
		if (mask_names[i].mask & ke->modifiers) {
			(void) strcat (ret, NAME(i));
			(void) strcat (ret, PLUS);
		}

	(void) strcat (ret, detail);

	return (ret);

#undef NAME

}

/**
 ** _OlAddMnemonic()
 **/

OlDefine
_OlAddMnemonic OLARGLIST((w, data, mnemonic))
	OLARG (Widget,		w)
	OLARG (XtPointer,	data)
	OLGRA (char,		mnemonic)
{
	if (mnemonic) {
		fake_accelerator[KEYSYM_POSITION] = mnemonic;
		return (AddAorM(w, data, fake_accelerator, True));
	} else
		return (OL_BAD_KEY);
}

/**
 ** _OlRemoveMnemonic()
 **/

void
_OlRemoveMnemonic OLARGLIST((w, data, ignore_data, mnemonic))
	OLARG (Widget,		w)
	OLARG (XtPointer,	data)
	OLARG (Boolean,		ignore_data)
	OLGRA (char,		mnemonic)
{
	if (mnemonic) {
		fake_accelerator[KEYSYM_POSITION] = mnemonic;
		RemoveAorM (w, data, ignore_data, fake_accelerator, True);
	} else
		RemoveAorM (w, data, ignore_data, (String)0, True);
	return;
}

/**
 ** _OlFetchMnemonicOwner()
 **/

Widget
_OlFetchMnemonicOwner OLARGLIST((w, p_data, virtual_event))
	OLARG (Widget,		w)
	OLARG (XtPointer *,	p_data)
	OLGRA (OlVirtualEvent,	virtual_event)
{
	if (virtual_event && OlQueryMnemonicDisplay(w) != OL_INACTIVE)
		return (FetchAorM(w, p_data, virtual_event, True));
	else
		return (0);
}

/**
 ** _OlAddAccelerator()
 **/

OlDefine
_OlAddAccelerator OLARGLIST((w, data, accelerator))
	OLARG (Widget,		w)
	OLARG (XtPointer,	data)
	OLGRA (String,		accelerator)
{
	if (accelerator)
		return (AddAorM(w, data, accelerator, False));
	else
		return (OL_BAD_KEY);
}

/**
 ** _OlRemoveAccelerator()
 **/

void
_OlRemoveAccelerator OLARGLIST((w, data, ignore_data, accelerator))
	OLARG (Widget,		w)
	OLARG (XtPointer,	data)
	OLARG (Boolean,		ignore_data)
	OLGRA (String,		accelerator)
{
	RemoveAorM (w, data, ignore_data, accelerator, False);
	return;
}

/**
 ** _OlFetchAcceleratorOwner()
 **/

Widget
_OlFetchAcceleratorOwner OLARGLIST((w, p_data, virtual_event))
	OLARG (Widget,			w)
	OLARG (XtPointer *,		p_data)
	OLGRA (OlVirtualEvent,		virtual_event)
{
	if (virtual_event && OlQueryAcceleratorDisplay(w) != OL_INACTIVE)
		return (FetchAorM(w, p_data, virtual_event, False));
	else
		return (0);
}

/**
 ** OlFetchMneOrAccOwner - finds the owner of the mnemonic
 **	key or accelerator key based on "ve". This function
 **	returns the "owner" id if the call to this function is
 **	successful otherwise NULL is returned. Two fields,
 **	virtual_name and item_index, will be changed in "ve".
 **	Where necessary, applications should save these
 **	values before invoking this function.
 **
 **	virtual_name is set to OL_MNEMONICKEY or OL_ACCELERATORKEY
 **	item_index shows which sub-item (for example, flat widgets)
 **		owns the mnemonic or accelerator key.
 **/
extern Widget
OlFetchMneOrAccOwner OLARGLIST((w, ve))
	OLARG( Widget,		w)	/* owner of ve		*/
	OLGRA( OlVirtualEvent,	ve)	/* virtual event	*/
{
	Widget		acc_w = (Widget)NULL,
			mne_w = (Widget)NULL,
			ret_w;
	XtPointer	data;

	if (w == (Widget)NULL || ve == (OlVirtualEvent)NULL ||
	    ve->xevent->type != KeyPress)
		return((Widget)NULL);

	if ((mne_w = _OlFetchMnemonicOwner(w, &data, ve)) == (Widget)NULL &&
	    (acc_w = _OlFetchAcceleratorOwner(w, &data, ve)) == (Widget)NULL)
		return((Widget)NULL);

		/* This is the main reason why I don't want to
		 * reveal _OlFetch*.. Assumption here is that
		 * flat widgets (possibilly scrollling list)
		 * use this "data" when adding it and IT STARTS
		 * from "1" so the future change of implemention
		 * of "flat" may affect this, watch it...
		 */
	if (data)
		ve->item_index = (Cardinal)data - 1;

	if (mne_w)	/* is a mnemonic owner */
	{
		ret_w = mne_w;
		ve->virtual_name = OL_MNEMONICKEY;
	}
	else		/* is an accelerator owner */
	{
		ret_w = acc_w;
		ve->virtual_name = OL_ACCELERATORKEY;
	}

	return(ret_w);
} /* end of OlFetchMneOrAccOwner */

/**
 ** AddAorM()
 **/

static OlDefine
AddAorM OLARGLIST((w, data, a_or_m, is_mnemonic))
	OLARG (Widget,		w)
	OLARG (XtPointer,	data)
	OLARG (String,		a_or_m)
	OLGRA (Boolean,		is_mnemonic)
{
	KeyEvent *		ke	= StringToKeyEvent(w, a_or_m, True);

	Widget			owner;


	if (!ke)
		return (OL_BAD_KEY);
	else {
		XtPointer	r_data = NULL;
		Boolean		is_virtual_key = False;

		owner = ActOnAorM(w, data, ke, is_mnemonic, KE_ADD,
				  &r_data, &is_virtual_key);

			/* the 2nd check is to avoid unnecessary warnings
			 * if an application calls with the same info
			 * later on but we have to check "data" because
			 * some widgets (e.g., flat, scrollinglist) can
			 * have many mne/acc (i.e., for each sub-object).
			 * Also check whether it's conflicted with virtual
			 * key bindings...
			 */
		if (owner && (owner != w || r_data != data || is_virtual_key))
			return (OL_DUPLICATE_KEY);
		if (!owner)
			RegisterChangeShowAccelerator (w, is_mnemonic);
		return (OL_SUCCESS);
	}
}

/**
 ** RemoveAorM()
 **/

static void
RemoveAorM OLARGLIST((w, data, ignore_data, a_or_m, is_mnemonic))
	OLARG (Widget,		w)
	OLARG (XtPointer,	data)
	OLARG (Boolean,		ignore_data)
	OLARG (String,		a_or_m)
	OLGRA (Boolean,		is_mnemonic)
{
	KeyEvent *		ke;

	if (a_or_m) {
		if ((ke = StringToKeyEvent(w, a_or_m, True)))
			(void)ActOnAorM (
				w, data, ke, is_mnemonic,
				KE_REMOVE, (XtPointer *)NULL, (Boolean *)NULL);
	} else if (ignore_data) {
		(void)ActOnAorM (
			w,
			data,
			(KeyEvent *)0,
			is_mnemonic,
			KE_REMOVE_ALL,
			(XtPointer *)NULL,
			(Boolean *)NULL
		);
		UnregisterChangeShowAccelerator (w);
	} else
		(void)ActOnAorM (
			w,
			data,
			(KeyEvent *)0,
			is_mnemonic,
			KE_REMOVE_USING_DATA,
			(XtPointer *)NULL,
			(Boolean *)NULL
		);

	return;
}

/**
 ** FetchAorM()
 **/

static Widget
FetchAorM OLARGLIST((w, p_data, ve, is_mnemonic))
	OLARG (Widget,		w)
	OLARG (XtPointer *,	p_data)
	OLARG (OlVirtualEvent,	ve)
	OLGRA (Boolean,		is_mnemonic)
{
	KeyEvent *		ke	= OlEventToKeyEvent(w, ve);

	if (ke)
		return (ActOnAorM(
				w, (XtPointer)p_data, ke,
				is_mnemonic, KE_FETCH,
				(XtPointer *)NULL, (Boolean *)NULL));
	else
		return (0);
}

/**
 ** ActOnAorM()
 **/

static Widget
ActOnAorM OLARGLIST((w, data, item, is_mnemonic, action, returned_data, is_virtual_key))
	OLARG (Widget,		w)
	OLARG (XtPointer,	data)
	OLARG (KeyEvent *,	item)
	OLARG (Boolean,		is_mnemonic)
	OLARG (int,		action)
	OLARG (XtPointer *,	returned_data)	/* valid for KE_ADD only */
	OLGRA (Boolean *,	is_virtual_key)	/* valid for KE_ADD only */
{
	_OlAppAttributes *	resources = _OlGetAppAttributesRef(w);

	OlVendorPartExtension	pe;

	KeyEvent *		base;
	KeyEvent *		insert = NULL;
	KeyEvent *		old;
	KeyEvent *		new;
	KeyEvent *		p	= 0;

	Cardinal		n;
	Cardinal		nel;
	Cardinal		insert_index;

	Widget			ret;

	Boolean			use_prefix;
	Boolean			do_grabs;


	pe = FetchVendorExtension(w, is_mnemonic, &use_prefix, &do_grabs);
	if (!pe)
		return (0);
	if (pe->accelerator_list) {
		base = pe->accelerator_list->base;
		nel  = pe->accelerator_list->nel;
	} else {
		base = 0;
		nel  = 0;
	}

	if (item) {
		/*
		 * Mnemonic events get adjusted slightly:
		 *
		 *	- Mnemonics for objects inside (e.g.) menus
		 *	  don't need (but can optionally have) a prefix
		 *	  such as Alt.
		 *
		 *	- Mnemonics are case-insensitive.
		 *
		 * Since we get to this code for both registration and
		 * fetching of mnemonics, we are adjusting the events
		 * consistently. Note, though, that we don't add the
		 * mnemonic prefix for the FETCH case, otherwise what
		 * would be the point?
		 */
		if (is_mnemonic) {
			item->modifiers &= ~ShiftMask;
			if (use_prefix && action != KE_FETCH)
			    item->modifiers |= resources->mnemonic_modifiers;
			else if (!use_prefix && action == KE_FETCH)
			    item->modifiers &= ~resources->mnemonic_modifiers;
		}

		p = FindKeyEvent(base, item, nel, &insert);
	}

	switch (action) {

	case KE_ADD:
		if (p)
		{
			*returned_data = p->data;
			return (p->w);
		}

		if (_OlKeySymToVirtualKeyName(item->keysym,
			item->modifiers, (String *)NULL)!=OL_UNKNOWN_KEY_INPUT)
		{
			*is_virtual_key = True;
			return(w);
		}

		nel++;

		/*
		 * Careful: "base" may change due to reallocation,
		 * so we have to realign "insert"--but only if it
		 * is not null!
		 */
		if (insert)
			insert_index = insert - base;
		base = Array(base, KeyEvent, nel);
		if (insert)
			insert = base + insert_index;

		/*
		 * Note: "nel" is now equal to the new length of the list.
		 * "insert - base" is equal to the number of items before
		 * where the new item goes. Thus the difference is one
		 * more than the number of items after the new item. The
		 * "OlMemMove" below will move the latter items up, to
		 * make room for the new item at "insert".
		 */
		if (insert)
			OlMemMove (KeyEvent, insert+1, insert,
					nel - insert_index - 1);
		else
			insert = base;

		*insert = *item;
		insert->is_mnemonic = is_mnemonic;
		insert->data        = data;
		insert->w           = w;

		/*
		 * If this is an accelerator and we have DoGrab turned
		 * on, then set grabs for this key on all top-level
		 * widgets.
		 */
		if (
			!is_mnemonic
		     && do_grabs
		     && pe->shell_list
		     && pe->shell_list->shells
		) {
			GrabAccelerator (
				w,
				item,
				pe->shell_list->shells,
				pe->shell_list->nshells,
				True
			);
			insert->grabbed = True;
		} else
			insert->grabbed = False;

		ret = 0;
		break;

	case KE_REMOVE:
		if (!p)
			return (0);

		/*
		 * If this was an accelerator and we had DoGrab turned
		 * on, then turn off grabs for this key on all top-level
		 * widgets.
		 */
		if (
			!p->is_mnemonic
		     && p->grabbed
		     && pe->shell_list
		     && pe->shell_list->shells
		)
			GrabAccelerator (
				w,
				p,
				pe->shell_list->shells,
				pe->shell_list->nshells,
				False
			);

		nel--;

		/*
		 * Note: "nel" is now the new size of the list (sans
		 * item to be deleted).
		 * "p - base" equals the number of items before the
		 * item to be deleted. Thus the difference is
		 * the number of items after the one to be deleted.
		 * The following will move the latter items down, to
		 * cover up (delete) the defunct item.
		 */
		if (nel)
			OlMemMove (KeyEvent, p, p+1, nel - (p - base));
		base = Array(base, KeyEvent, nel);

		ret = 0;
		break;

	case KE_REMOVE_USING_DATA:		/* FALLTHROUGH */
	case KE_REMOVE_ALL:
		for (old = new = base, n = 0; n < nel; old++, n++) {
			if (
				old->w == w
			     && old->is_mnemonic == is_mnemonic
			     && (action == KE_REMOVE_ALL || old->data == data)
			) {
				if (
					!old->is_mnemonic
				     && old->grabbed
				     && pe->shell_list
				     && pe->shell_list->shells
				)
					GrabAccelerator (
						w,
						old,
						pe->shell_list->shells,
						pe->shell_list->nshells,
						False
					);
			} else
				*new++ = *old;
		}
		nel = new - base;
		base = Array(base, KeyEvent, nel);
		break;

	case KE_FETCH:
		if (p && p->is_mnemonic == is_mnemonic) {
			if (data)
				*(XtPointer *)data = p->data;
			return (p->w);
		} else
			return (0);

	}

	/*
	 * Getting here means we added or deleted an item, or
	 * have freed the storage space.
	 * Failures, or the fetch case, have already returned.
	 */
	if (base) {
		if (!pe->accelerator_list)
			pe->accelerator_list = New(OlAcceleratorList);
		pe->accelerator_list->base = base;
		pe->accelerator_list->nel  = nel;
		pe->a_m_index++;
	} else {
		if (pe->accelerator_list)
			Free ((char *)pe->accelerator_list);
		pe->accelerator_list = 0;
		pe->a_m_index++;
	}

	return (ret);
}

/**
 ** StringToKeyEvent()
 **/

static KeyEvent *
StringToKeyEvent OLARGLIST((w, str, decompose_virtual_name))
	OLARG (Widget,		w)
	OLARG (String,		str)
	OLGRA (Boolean,		decompose_virtual_name)
{
	static KeyEvent		ret	= { 0 };

	XrmValue		from;
	XrmValue		to;

	OlKeyDef *		kd;

	/*
	 * First see if a ``virtual'' key event is specified.
	 * If not, then we must have a specific key event.
	 * Don't try to map it into a virtual key event, because
	 * the latter are changeable--if the user gave a specific key
	 * event, we assume it is immutable.
	 *
	 * See also the comment in "FindKeyEvent()".
	 */

	if (decompose_virtual_name)
		ret.name = _OlStringToVirtualKeyName(str);
	else
		ret.name = OL_UNKNOWN_KEY_INPUT;

	switch (ret.name) {

	case OL_UNKNOWN_INPUT: /* just in case */
		ret.name = OL_UNKNOWN_KEY_INPUT;
		/*FALLTHROUGH*/

	case OL_UNKNOWN_KEY_INPUT:
		from.addr = (XtPointer)str;
		from.size = Strlen(str) + 1;
		to.addr   = 0;
		if (!XtCallConverter(
			XtDisplayOfObject(w),
			_OlStringToOlKeyDef,
			(XrmValuePtr)0,
			(Cardinal)0,
			&from,
			&to,
			(XtCacheRef)0
		))
			return (0);

		kd = (OlKeyDef *)to.addr;
		if (kd->used != 1) {
			OlVaDisplayWarningMsg(XtDisplay(w),
					      OleNbadConversion,
					      OleTillegalSyntax,
					      OleCOlToolkitWarning,
					      OleMbadConversion_illegalSyntax,
					      "String","KeyEvent",
					      str);
			return (0);
		}

		ret.modifiers = kd->modifier[0];
		ret.keysym    = kd->keysym[0];
		break;

	default:
		ret.modifiers = 0;
		ret.keysym    = NoSymbol;
		break;

	}

	if (ret.keysym == NoSymbol && ret.name == OL_UNKNOWN_KEY_INPUT)
		return (0);
	else
		return (&ret);
}

/**
 ** OlEventToKeyEvent()
 **/

static KeyEvent *
OlEventToKeyEvent OLARGLIST((w, ve))
	OLARG (Widget,		w)
	OLGRA (OlVirtualEvent,	ve)
{
	static KeyEvent		ret	= { 0 };

	KeySym			lower;
	KeySym			upper;

	KeySym *		syms;

	KeyCode			min_keycode;	/* not int, when Xt */

	int			per;



	switch (ve->virtual_name) {

	case OL_UNKNOWN_INPUT: /* just in case */
	case OL_UNKNOWN_KEY_INPUT:
		ret.name = OL_UNKNOWN_KEY_INPUT;
		break;

	default:
		ret.name = ve->virtual_name;
		break;

	}

	/*
	 * Save the detail (even if the event is a ``virtual'' one),
	 * because we may have to compare this to a specific (non-virtual)
	 * accelerator.
	 *
	 * We ignore the "keysym" given in the virtual event structure,
	 * because it may have some of the modifiers already ``folded
	 * in''. We instead convert the naked KeyCode to a KeySym.
	 * Furthermore, because the KeyCode itself may have some implied
	 * modifiers (some servers give a different KeyCode for <A>
	 * than for <a>, where <A> == Shift<a>), we also convert to
	 * the lowest denominator, canonical form.
	 *
	 * Note: If we do have the situation where the KeyCode is <A>,
	 * the modifiers will already include ShiftMask.
	 *
	 * Further note: For some shifted keys we fold the ShiftMask
	 * back into the keysym, for example:
	 *
	 *	Shift<1>   ->	<!>
	 *	Shift<'>   ->	<">
	 *
	 * but we leave these alone:
	 *
	 *	Shift<m>, not <M>
	 *	Shift<F2>, not <F14>
	 *
	 * See _OlCanonicalKeysym() in Dynamic.c.
	 *
	 * (I18N)
	 */

	ret.modifiers = ve->xevent->xkey.state;

	syms = XtGetKeysymTable(XtDisplayOfObject(w), &min_keycode, &per);
	syms += (ve->xevent->xkey.keycode - min_keycode) * per;

	if (per > 1 && syms[1] != NoSymbol) {
		lower = syms[0];
		upper = syms[1];
	} else
		XtConvertCase (XtDisplayOfObject(w), syms[0], &lower, &upper);
	if (
		(ret.modifiers & ShiftMask)
	     && isascii(upper)		/* e.g. not <F14> */
	     && !isspace(upper)		/* so s<space> will work */
	     && !isalpha(upper)		/* e.g. not <M>   */
	) {
		ret.keysym = upper;
		ret.modifiers &= ~ShiftMask;
	} else
		ret.keysym = lower;

	return (&ret);
}

/**
 ** FindKeyEvent()
 **/

static KeyEvent *
FindKeyEvent OLARGLIST((base, item, nel, p_insert))
	OLARG (KeyEvent *,		base)
	OLARG (register KeyEvent *,	item)
	OLARG (Cardinal,		nel)
	OLGRA (KeyEvent **,		p_insert)
{
	register KeyEvent *	low		= base;
	register KeyEvent *	high		= low + (nel - 1);
	register KeyEvent *	p;

	KeyEvent *		tentative	= 0;


	/*
	 * The easy case.
	 */
	if (!base) {
		if (p_insert)
			*p_insert = 0;
		return (0);
	}


	/*
	 * For the key event described in "item" we have the following
	 * possibilities:
	 *
	 *  (a) keysym == NoSymbol && name != OL_UNKNOWN_KEY_INPUT
	 *  (b) keysym != NoSymbol && name == OL_UNKNOWN_KEY_INPUT
	 *  (c) keysym != NoSymbol && name != OL_UNKNOWN_KEY_INPUT
	 *
	 * For the key events in the list starting at "base", only
	 * (a) and (b) are possible. Furthermore, it is possible to have
	 * two (or more) key events in the list that are ``similar'',
	 * such that a specific key event (keysym != NoSymbol) is
	 * equal to one of the current values for a ``virtual'' key
	 * event (name != OL_UNKNOWN_KEY_EVENT).
	 *
	 * WE TREAT THESE SIMILAR EVENTS AS DIFFERENT. This allows a
	 * user to have a (specific) accelerator that is the same as
	 * one of the aliases for a virtual key event.
	 *
	 * When searching for a fully defined event (possibility (c)),
	 * we ensure that we always favor matching the specific event
	 * over matching the virtual event, by not stopping on the first
	 * match if it is a virtual event.
	 */

#define K_EQ(a,b) (a->keysym == b->keysym)
#define M_EQ(a,b) (a->modifiers == b->modifiers)
#define K_LT(a,b) (a->keysym < b->keysym)
#define M_LT(a,b) (a->modifiers < b->modifiers)

#define VIRT_EQ(a,b) (a->name == b->name)
#define VIRT_LT(a,b) (a->name < b->name)
#define SPEC_EQ(a,b) (K_EQ(a,b) && M_EQ(a,b))
#define SPEC_LT(a,b) (K_LT(a,b) || K_EQ(a,b) && M_LT(a,b))

#define LT(a,b) (VIRT_LT(a,b) || VIRT_EQ(a,b) && SPEC_LT(a,b))

	while (high >= low) {
		p = low + ((high - low) / 2);

		/*
		 * If we find that the specific-event information matches,
		 * AND it's real, that's good.
		 */
		if (SPEC_EQ(item, p) && p->keysym != NoSymbol)
			return (p);

		/*
		 * If we find that the virtual-event information matches,
		 * AND it's real, then it MIGHT be good. Then again, it
		 * might be that we haven't reached an element of the list
		 * that has the same specific-event information.
		 */
		if (VIRT_EQ(item, p) && p->name != OL_UNKNOWN_KEY_INPUT)
			tentative = p;

		/*
		 * At this point, "p" always points within the ordered
		 * list of items. When the loop ends with no match,
		 * the item we failed to find can be inserted either
		 * immediately before or immediately after "p".
		 */
		if (LT(item, p))
			high = p - 1;
		else
			low = p + 1;
	}

	/*
	 * The search failed unless we had a tentative match.
	 */
	if (tentative)
		return (tentative);
	if (p_insert) {
		/*
		 * As suggested above, the new item should be inserted
		 * either after or before the item at "p". Adjust "p",
		 * if needed, so that it points to where the item should
		 * be inserted, once the existing items starting with "p"
		 * are shifted up to make room.
		 */
		if (!LT(item, p))
			p++;
		*p_insert = p;
	}
	return (0);
}

/**
 ** FetchVendorExtension()
 **/

static OlVendorPartExtension
FetchVendorExtension OLARGLIST((w, is_mnemonic, p_use_mnemonic_prefix,
					p_accelerators_do_grab))
	OLARG (Widget,		w)
	OLARG (Boolean,		is_mnemonic)
	OLARG (Boolean *,	p_use_mnemonic_prefix)
	OLGRA (Boolean *,	p_accelerators_do_grab)
{
	Widget			vsw	= _OlFindVendorShell(w, is_mnemonic);

	OlVendorPartExtension	pe	= 0;


	if (vsw) {
		pe = _OlGetVendorPartExtension(vsw);
		if (p_use_mnemonic_prefix)
			*p_use_mnemonic_prefix = UseMnemonicPrefix(vsw);
		if (p_accelerators_do_grab) {
			static Arg	arg = {	XtNacceleratorsDoGrab };

			arg.value = (XtArgVal)p_accelerators_do_grab;
			XtGetValues (vsw, &arg, 1);
		}
	}

	return (pe);
}

/**
 ** UseMnemonicPrefix()
 **/

static Boolean
UseMnemonicPrefix OLARGLIST((w))
	OLGRA (Widget,	w)
{
	return (!XtIsSubclass(w, popupMenuShellWidgetClass));
}

/**
 ** GrabAccelerator()
 **/

static void
GrabAccelerator OLARGLIST((w, ke, shells, nshells, grab))
	OLARG (Widget,		w)
	OLARG (KeyEvent *,	ke)
	OLARG (Widget *,	shells)
	OLARG (Cardinal,	nshells)
	OLGRA (Boolean,		grab)
{
	Cardinal		j;


#define GRAB_KIND False,GrabModeAsync,GrabModeAsync

#define OLGRAB(W,O,G) \
	(G?  OlGrabVirtualKey((W),(O),GRAB_KIND) \
	   : OlUngrabVirtualKey((W),(O)))

#define XTGRAB(W,K,M,G) \
	(G?  XtGrabKey((W),(K),(M),GRAB_KIND) \
	   : XtUngrabKey((W),(K),(M)))

	if (ke->name != OL_UNKNOWN_KEY_INPUT) {
		for (j = 0; j < nshells; j++)
			OLGRAB (shells[j], ke->name, grab);
	} else {
		KeyCode			keycode;
		Modifiers		modifiers	= ke->modifiers;

		(void)_OlCanonicalKeysym (
			XtDisplayOfObject(w),
			ke->keysym,
			&keycode,
			&modifiers
		);
		if (!keycode)
			return;
		for (j = 0; j < nshells; j++)
			XTGRAB (shells[j], keycode, modifiers, grab);
	}

	return;
}

/**
 ** _OlNewAcceleratorResourceValues()
 **/

void
_OlNewAcceleratorResourceValues OLARGLIST((client_data))
	OLGRA (XtPointer, client_data)
{
	Widget			vsw	= (Widget)client_data;

	_OlAppAttributes *	resources = _OlGetAppAttributesRef(vsw);

	OlVendorPartExtension	pe	= _OlGetVendorPartExtension(vsw);

	register KeyEvent *	p;

	register Cardinal	nel;

	String			vir_name = NULL;


	if (!pe || !pe->accelerator_list)
		return;

	if (UseMnemonicPrefix(vsw))
	{
		p   = pe->accelerator_list->base;
		nel = pe->accelerator_list->nel;

		while (nel--) {
			if (p->is_mnemonic)
				p->modifiers = resources->mnemonic_modifiers;
			p++;
		}
	}

		/* check whether new key bindings are conflicted
		 * with accelerators and/or mnemonics...
		 */
	p   = pe->accelerator_list->base;
	nel = pe->accelerator_list->nel;

	while (nel--) {

		if (_OlKeySymToVirtualKeyName(p->keysym,
			 p->modifiers, &vir_name) != OL_UNKNOWN_KEY_INPUT)
		{
			Arg		args[1];
			unsigned char	f_ch[2];
			unsigned char	ch = '\0';
			String		str = NULL, xtn, val;

			f_ch[0] = '\0';
			f_ch[1] = '\0';
			xtn = p->is_mnemonic ? XtNmnemonic : XtNaccelerator;

			XtSetArg(args[0], xtn,
				 p->is_mnemonic?(XtArgVal)&ch : (XtArgVal)&str);

			if (XtIsSubclass(p->w, flatWidgetClass))
				OlFlatGetValues(
					p->w, (Cardinal)p->data-1, args, 1);
			else
				XtGetValues(p->w, args, 1);

			if (!p->is_mnemonic)
				val = str;
			else
			{
				f_ch[0] = ch;
				val = (String)f_ch;
			}

			OlVaDisplayWarningMsg(
				XtDisplayOfObject(p->w),
				OleNillegalAccOrMne,
				OleTduplicateKey,
				OleCOlToolkitWarning,
				OleMillegalAccOrMne_duplicateKey,
				XtName(p->w),
				OlWidgetToClassName(p->w),
				xtn,
				val,
				vir_name
			);
		}
		p++;
	}

	return;
}

/**
 ** RegisterChangeShowAccelerator()
 ** UnregisterChangeShowAccelerator()
 **/

static _OlArrayDef(WidgetArray, mnemonic_array);
static _OlArrayDef(WidgetArray, accelerator_array);

static Boolean		local_show_mnemonics;
static Boolean		local_show_accelerators;

static void		ChangeShowAccelerator OL_ARGS((
	XtPointer		client_data
));
static void		ExposeWidgets OL_ARGS((
	WidgetArray *		array
));

static void
RegisterChangeShowAccelerator OLARGLIST((w, is_mnemonic))
	OLARG (Widget,		w)
	OLGRA (Boolean,		is_mnemonic)
{
	static Boolean		registered_yet	= False;


	/*
	 * Register a callback to handle dynamic changes in the
	 * method of showing mnemonics and accelerators.
	 */
	if (!registered_yet) {
		_OlAppAttributes *	resources = _OlGetAppAttributesRef(w);

		OlRegisterDynamicCallback (ChangeShowAccelerator, (XtPointer)0);
		local_show_mnemonics    = resources->show_mnemonics;
		local_show_accelerators = resources->show_accelerators;
		registered_yet = True;
	}

	if (is_mnemonic)
		_OlArrayUniqueAppend (&mnemonic_array, w);
	else
		_OlArrayUniqueAppend (&accelerator_array, w);

	return;
} /* RegisterChangeShowAccelerator */

static void
UnregisterChangeShowAccelerator OLARGLIST((w))
	OLGRA (Widget,		w)
{
	int			i;


	if ((i = _OlArrayFind(&mnemonic_array, w)) != _OL_NULL_ARRAY_INDEX)
		_OlArrayDelete (&mnemonic_array, i);
	if ((i = _OlArrayFind(&accelerator_array, w)) != _OL_NULL_ARRAY_INDEX)
		_OlArrayDelete (&accelerator_array, i);

	return;
} /* UnregisterChangeShowAccelerator */

static void
ChangeShowAccelerator OLARGLIST((client_data))
	OLGRA (XtPointer,	client_data)
{
	Widget			w;

	_OlAppAttributes *	resources;


	/*
	 * Fetch the current XtNshowAccelerator and XtNshowMnemonic
	 * resources (not from the database, as that's already been
	 * done--fetch from an internal structure). We need a widget
	 * ID for this, so use any one from the arrays. Of course,
	 * if both arrays are empty, we've nothing to do.
	 */
	if (
		_OL_ARRAY_IS_EMPTY(&mnemonic_array)
	     && _OL_ARRAY_IS_EMPTY(&accelerator_array)
	)
		return;
	if (!_OL_ARRAY_IS_EMPTY(&mnemonic_array))
		w = (Widget)_OlArrayElement(&mnemonic_array, 0);
	else
		w = (Widget)_OlArrayElement(&accelerator_array, 0);
	resources = _OlGetAppAttributesRef(w);

	/*
	 * Cause Exposure events to be generated for each of the
	 * widgets that has a mnemonic/accelerator.
	 */
	if (local_show_mnemonics != resources->show_mnemonics) {
		ExposeWidgets (&mnemonic_array);
		local_show_mnemonics = resources->show_mnemonics;
	}
	if (local_show_accelerators != resources->show_accelerators) {
		ExposeWidgets (&accelerator_array);
		local_show_accelerators = resources->show_accelerators;
	}

	return;
} /* ChangeShowAccelerator */

static void
ExposeWidgets OLARGLIST((array))
	OLGRA (WidgetArray *,	array)
{
	Cardinal		i;

	Widget			w;


	for (i = 0; i < _OlArraySize(array); i++) {
		w = (Widget)_OlArrayElement(array, i);
		if (XtIsRealized(w))
			XClearArea (
				XtDisplayOfObject(w),
				XtWindowOfObject(w),
				_OlXTrans(w, 0),
				_OlYTrans(w, 0),
				w->core.width,
				w->core.height,
				True
			);
	}
	return;
} /* ExposeWidgets */
