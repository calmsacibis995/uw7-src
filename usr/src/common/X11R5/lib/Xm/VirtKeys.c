#pragma ident	"@(#)m1.2libs:Xm/VirtKeys.c	1.6"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1990 MOTOROLA, INC. */
#include      	<stdio.h>
#include	<ctype.h>
#include 	<string.h>
#include	<X11/keysym.h>
#include 	<Xm/VirtKeysP.h>
#include 	<Xm/DisplayP.h>
#include 	<Xm/TransltnsP.h>
#include        <Xm/AtomMgr.h>
#include	<Xm/XmosP.h>
#ifndef X_NOT_STDC_ENV
#include        <stdlib.h>
#endif


#define	done(type, value) \
	{							\
	    if (toVal->addr != NULL) {				\
		if (toVal->size < sizeof(type)) {		\
		    toVal->size = sizeof(type);			\
		    return False;				\
		}						\
		*(type*)(toVal->addr) = (value);		\
	    }							\
	    else {						\
		static type static_val;				\
		static_val = (value);				\
		toVal->addr = (XPointer)&static_val;		\
	    }							\
	    toVal->size = sizeof(type);				\
	    return True;					\
	}

#define defaultFallbackBindings _XmVirtKeys_fallbackBindingString

#define BUFFERSIZE 2048
#define MAXLINE 256

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Boolean CvtStringToVirtualBinding() ;
static XmKeyBindingRec * FillBindingsFromDB() ;
static Boolean GetBindingsProperty() ;
static void FindVirtKey() ;
static Modifiers EffectiveStdModMask() ;
static void LoadVendorBindings() ;

#else

static Boolean CvtStringToVirtualBinding( 
                        Display *dpy,
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValuePtr fromVal,
                        XrmValuePtr toVal,
                        XtPointer *closure_ret) ;
static XmKeyBindingRec * FillBindingsFromDB( 
                        Display *dpy,
                        XrmDatabase rdb) ;
static Boolean GetBindingsProperty( 
                        Display *display,
			String property,
                        String *binding) ;
static void FindVirtKey( 
                        Display *dpy,
#if NeedWidePrototypes
                        int keycode,
#else
                        KeyCode keycode,
#endif /* NeedWidePrototypes */
                        Modifiers modifiers,
                        Modifiers *modifiers_return,
                        KeySym *keysym_return) ;
static Modifiers EffectiveStdModMask(
                        Display *dpy,
                        KeySym *kc_map,
                        int ks_per_kc) ;
static void LoadVendorBindings(
			Display *display,
			char *path,
			FILE *fp,
			String *binding) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

static XmKeyBindingRec nullBinding = { 0L, 0 };

static XmVirtualKeysymRec virtualKeysyms[] =
{
    {(String)XmVosfBackSpace	, (KeySym)0x1004FF08	},
    {(String)XmVosfInsert	, (KeySym)0x1004FF63	},
    {(String)XmVosfDelete	, (KeySym)0x1004FFFF	},
    {(String)XmVosfCopy		, (KeySym)0x1004FF02	},
    {(String)XmVosfCut		, (KeySym)0x1004FF03	},
    {(String)XmVosfPaste	, (KeySym)0x1004FF04	},
    {(String)XmVosfAddMode	, (KeySym)0x1004FF31	},
    {(String)XmVosfPrimaryPaste	, (KeySym)0x1004FF32	},
    {(String)XmVosfQuickPaste	, (KeySym)0x1004FF33	},
    {(String)XmVosfPageLeft	, (KeySym)0x1004FF40	},
    {(String)XmVosfPageUp	, (KeySym)0x1004FF41	},
    {(String)XmVosfPageDown	, (KeySym)0x1004FF42	},
    {(String)XmVosfPageRight	, (KeySym)0x1004FF43	},
    {(String)XmVosfEndLine	, (KeySym)0x1004FF57	},
    {(String)XmVosfBeginLine	, (KeySym)0x1004FF58	},
    {(String)XmVosfActivate	, (KeySym)0x1004FF44	},
    {(String)XmVosfMenuBar	, (KeySym)0x1004FF45	},
    {(String)XmVosfClear	, (KeySym)0x1004FF0B	},
    {(String)XmVosfCancel	, (KeySym)0x1004FF69	},
    {(String)XmVosfHelp		, (KeySym)0x1004FF6A	},
    {(String)XmVosfMenu		, (KeySym)0x1004FF67	},
    {(String)XmVosfSelect	, (KeySym)0x1004FF60	},
    {(String)XmVosfUndo		, (KeySym)0x1004FF65	},
    {(String)XmVosfLeft		, (KeySym)0x1004FF51	},
    {(String)XmVosfUp		, (KeySym)0x1004FF52	},
    {(String)XmVosfRight	, (KeySym)0x1004FF53	},
    {(String)XmVosfDown		, (KeySym)0x1004FF54	},
};

static XmDefaultBindingStringRec fallbackBindingStrings[] =
{
    {"Acorn Computers Ltd"			, _XmVirtKeys_acornFallbackBindingString},
    {"Apollo Computer Inc."			, _XmVirtKeys_apolloFallbackBindingString},
    {"DECWINDOWS DigitalEquipmentCorp."		, _XmVirtKeys_decFallbackBindingString},
    {"Data General Corporation  Rev 04"		, _XmVirtKeys_dgFallbackBindingString},
    {"Double Click Imaging, Inc. KeyX"		, _XmVirtKeys_dblclkFallbackBindingString},
    {"Hewlett-Packard Company"			, _XmVirtKeys_hpFallbackBindingString},
    {"International Business Machines"		, _XmVirtKeys_ibmFallbackBindingString},
    {"Intergraph Corporation"			, _XmVirtKeys_ingrFallbackBindingString},
    {"Megatek Corporation"			, _XmVirtKeys_megatekFallbackBindingString},
    {"Motorola Inc. (Microcomputer Division) "	, _XmVirtKeys_motorolaFallbackBindingString},
    {"Silicon Graphics Inc."			, _XmVirtKeys_sgiFallbackBindingString},
    {"Silicon Graphics"				, _XmVirtKeys_sgiFallbackBindingString},
    {"Siemens Munich by SP-4's Hacker Crew"	, _XmVirtKeys_siemensWx200FallbackBindingString},
    {"Siemens Munich (SP-4's hacker-clan)"	, _XmVirtKeys_siemens9733FallbackBindingString},
    {"X11/NeWS - Sun Microsystems Inc."		, _XmVirtKeys_sunFallbackBindingString},
    {"Sun Microsystems, Inc."			, _XmVirtKeys_sunFallbackBindingString},
    {"Tektronix, Inc."				, _XmVirtKeys_tekFallbackBindingString},
};


/*ARGSUSED*/
static Boolean 
#ifdef _NO_PROTO
CvtStringToVirtualBinding( dpy, args, num_args, fromVal, toVal, closure_ret )
        Display *dpy ;
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValuePtr fromVal ;
        XrmValuePtr toVal ;
        XtPointer *closure_ret ;
#else
CvtStringToVirtualBinding(
        Display *dpy,
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValuePtr fromVal,
        XrmValuePtr toVal,
        XtPointer *closure_ret )
#endif /* _NO_PROTO */
{
    char 		*str = (char *)fromVal->addr ;
    XmKeyBindingRec	keyBindingRec;
    int			eventType;

    if (_XmMapKeyEvent( str, &eventType, &keyBindingRec.keysym,
                                                     &keyBindingRec.modifiers))
      {
	  done(XmKeyBindingRec,  keyBindingRec);
      }
    XtDisplayStringConversionWarning(dpy, str, XmRVirtualBinding);
    return False;
}


static XmKeyBindingRec * 
#ifdef _NO_PROTO
FillBindingsFromDB( dpy, rdb )
        Display *dpy ;
        XrmDatabase rdb ;
#else
FillBindingsFromDB(
        Display *dpy,
        XrmDatabase rdb )
#endif /* _NO_PROTO */
{
    XmKeyBindingRec	*keyBindings, *virtBinding;
    XmVirtualKeysym	virtKey;
    XrmName 		xrm_name[2];
    XrmClass 		xrm_class[2];
    XrmRepresentation 	rep_type;
    XrmValue 		value;
    Cardinal		i;

    xrm_class[0] = XrmStringToQuark(XmRVirtualBinding);
    xrm_class[1] = 0;

    keyBindings = (XmKeyBindingRec *)
      XtMalloc(sizeof(XmKeyBindingRec) * XtNumber(virtualKeysyms));

    for (virtKey = virtualKeysyms, virtBinding = keyBindings, i = 0; 
	 i < XtNumber(virtualKeysyms);
	 virtKey++, virtBinding++, i++)
      {
	  xrm_name[0] = XrmStringToQuark(virtKey->name);
	  xrm_name[1] = 0;
	  if (XrmQGetResource(rdb, xrm_name, xrm_class,
			      &rep_type, &value ))
	    {
		if (rep_type == XrmStringToQuark(XmRVirtualBinding))

		  *virtBinding = *(XmKeyBindingRec *)value.addr;

		else if (rep_type == XrmStringToQuark(XmRString)) 
		  {
		      XrmValue toVal;
		      toVal.addr = (XPointer)virtBinding;
		      toVal.size = sizeof(XmKeyBindingRec);
		      if (!XtCallConverter(dpy, 
					  CvtStringToVirtualBinding, 
					  NULL,
					  0,
					  &value, 
					  &toVal,
					  (XtCacheRef*)NULL))
			    *virtBinding = nullBinding;
		  }
		else 
		  *virtBinding = nullBinding;
	    }
	  else
	    *virtBinding = nullBinding;
      }
    return keyBindings;
}


static Boolean 
#ifdef _NO_PROTO
GetBindingsProperty( display, property, binding )
        Display *display;
	String	property;
        String  *binding;
#else
GetBindingsProperty(
	Display *display,
	String	property,
	String	*binding )
#endif /* _NO_PROTO */
{
    char		*prop = NULL;
    Atom		actual_type;
    int			actual_format;
    unsigned long	num_items;
    unsigned long	bytes_after;


    if ( binding == NULL ) return( False );

    XGetWindowProperty (display, 
			RootWindow(display, 0),
			XmInternAtom(display, property, FALSE),
			0, (long)1000000,
			FALSE, XA_STRING,
			&actual_type, &actual_format,
			&num_items, &bytes_after,
			(unsigned char **) &prop);

    if ((actual_type != XA_STRING) ||
	(actual_format != 8) || 
	(num_items == 0))
    {
        if (prop != NULL) XFree(prop);
        return( False );
    }
    else
    {
        *binding = prop;
        return( True );
    }
}

	   
/*
 * This routine is called by the XmDisplay Initialize method to set
 * up the virtual bindings table, XtKeyProc, and event handler.
 */
void 
#ifdef _NO_PROTO
_XmVirtKeysInitialize( widget )
        Widget widget ;
#else
_XmVirtKeysInitialize(
        Widget widget )
#endif /* _NO_PROTO */
{
    XmDisplay xmDisplay = (XmDisplay) widget;
    Display *dpy = XtDisplay(xmDisplay);
    Cardinal i;
    XrmDatabase keyDB;
    String bindingsString;
    String fallbackString = NULL;
    Boolean needXFree = False;

    if ( !XmIsDisplay (widget) ) return;

    bindingsString = xmDisplay->display.bindingsString;
    xmDisplay->display.lastKeyEvent = XtNew(XKeyEvent);
    memset((void *) (xmDisplay->display.lastKeyEvent), 0, sizeof(XKeyEvent));

    if (bindingsString == NULL) {

	/* XmNdefaultVirtualBindings not set, try _MOTIF_BINDINGS */

	if (GetBindingsProperty( XtDisplay(xmDisplay),
				 "_MOTIF_BINDINGS",
				 &bindingsString) == True) {
	    needXFree = True;
	}
	else if (GetBindingsProperty( XtDisplay(xmDisplay),
				      "_MOTIF_DEFAULT_BINDINGS",
				      &bindingsString) == True) {
	    needXFree = True;
	}
	else {
	    /* property not set, find a useful fallback */

	    _XmVirtKeysLoadFallbackBindings( XtDisplay(xmDisplay),
					     &fallbackString );
	    bindingsString = fallbackString;
	}
    }

    keyDB = XrmGetStringDatabase( bindingsString );
    xmDisplay->display.bindings = FillBindingsFromDB (XtDisplay(xmDisplay),
							keyDB);
    XrmDestroyDatabase(keyDB);
    if (needXFree) XFree (bindingsString);
    if (fallbackString) XtFree (fallbackString);

    XtSetKeyTranslator(dpy, (XtKeyProc)XmTranslateKey);

 /*
  * The Xt R5 translation manager cache conflicts with our XtKeyProc.
  * A virtual key with modifiers bound will confuse the cache.
  *
  * The keycode_tag[] has one bit for each possible keycode. If a virtual
  * key has modifiers bound, we flag its keycode. In _XmVirtKeysHandler
  * we check the event keycode, and reset the cache to get the correct
  * behavior.
  */
    memset (xmDisplay->display.keycode_tag, 0, XmKEYCODE_TAG_SIZE);
    for (i = 0; i < XtNumber(virtualKeysyms); i++) {
	KeyCode kc = XKeysymToKeycode (dpy,
				       xmDisplay->display.bindings[i].keysym);
	if (kc != 0 && xmDisplay->display.bindings[i].modifiers != 0)
	    xmDisplay->display.keycode_tag[kc/8] |= 1 << (kc % 8);
    }
}


/*
 * This routine is called by the XmDisplay Destroy method to free
 * up the virtual bindings table.
 */
void 
#ifdef _NO_PROTO
_XmVirtKeysDestroy( widget )
        Widget widget ;
#else
_XmVirtKeysDestroy(
        Widget widget )
#endif /* _NO_PROTO */
{
    XmDisplay xmDisplay = (XmDisplay) widget;

    XtFree ((char *)xmDisplay->display.lastKeyEvent);
    XtFree ((char *)xmDisplay->display.bindings);
}

#ifdef XT_HAS_TRANS_FIX

static void 
#ifdef _NO_PROTO
FindVirtKey( dpy, keycode, modifiers, modifiers_return, keysym_return )
        Display *dpy ;
        KeyCode keycode ;
        Modifiers modifiers ;
        Modifiers *modifiers_return ;
        KeySym *keysym_return ;
#else
FindVirtKey(
        Display *dpy,
#if NeedWidePrototypes
        int keycode,
#else
        KeyCode keycode,
#endif /* NeedWidePrototypes */
        Modifiers modifiers,
        Modifiers *modifiers_return,
        KeySym *keysym_return )
#endif /* _NO_PROTO */
{
    Cardinal		i;
    XmDisplay           xmDisplay = (XmDisplay) XmGetXmDisplay( dpy) ;
    XmKeyBinding	keyBindings = xmDisplay->display.bindings;
    KeyCode min_kcode ;
    int ks_per_kc ;
    KeySym *ks_table = XtGetKeysymTable( dpy, &min_kcode, &ks_per_kc) ;
    KeySym *kc_map = &ks_table[(keycode - min_kcode) * ks_per_kc] ;
    Modifiers EffectiveSMMask = EffectiveStdModMask( dpy, kc_map, ks_per_kc) ;
    Modifiers effective_Xt_modifiers ;

    /* Get the modifiers from the actual event:
     */
    Modifiers eventMods = (Modifiers)(xmDisplay->display.lastKeyEvent->state);
    Modifiers VirtualStdMods = 0 ;
    Modifiers StdModMask ;

    for (i = 0; i < XtNumber(virtualKeysyms); i++)
      {
        unsigned j = ks_per_kc ;
        KeySym vks = keyBindings[i].keysym ;

        while(    j--    )
          {
            /* Want to walk through keymap (containing all possible
             * keysyms generated by this keycode) to compare against
             * virtual key keysyms.  Any keycode that can possibly
             * generate a virtual keysym must be sure to return all
             * modifiers that are in the virtual key binding, since
             * this means that those modifiers are now part of the
             * "standard modifiers" for this keycode.  (A "standard
             * modifier" is a modifier that can affect which keysym
             * is generated from a particular keycode.)
             */
            if(    (j == 1)  &&  (kc_map[j] == NoSymbol)    )
              {   
                KeySym uc ; 
                KeySym lc ;

                XtConvertCase( dpy, kc_map[0], &lc, &uc) ;
                if(    (vks == lc)  ||  (vks == uc)    )
                  {   
                    VirtualStdMods |= keyBindings[i].modifiers ;
                  } 
                break ;
              } 
            else
              {   
                if(    vks == kc_map[j]    )
                  {
                    /* The keysym generated by this keycode can possibly
                     * be influenced by the virtual key modifier(s), so must
                     * add the modifier(s) associated with this virtual
                     * key to the returned list of "standard modifiers".
                     * The Intrinsics requires that the set of modifiers
                     * returned by the keyproc is constant for a given
                     * keycode.
                     */
                    VirtualStdMods |= keyBindings[i].modifiers ;
                    break ;
                  }
              } 
          }
      }
    /* Don't want to return standard modifiers that do not
     * impact the keysym selected for a particular keycode,
     * since this blocks matching of translation productions
     * which use ":" style translations with the returned
     * standard modifier in the production.  The ":" style
     * of production is essential for proper matching of
     * Motif translations (PC numeric pad, for instance).
     *
     * Recent fixes to the Intrinsics should have included this
     * change to the set of standard modifiers returned from
     * the default key translation routine, but could not be
     * done for reasons of backwards compatibility (which is
     * not an issue for Xm, since we do not export this facility).
     * So, we do the extra masking here after the return from
     * the call to XtTranslateKey.
     */
    *modifiers_return &= EffectiveSMMask ;
    effective_Xt_modifiers = *modifiers_return ;

    /* Modifiers present in the virtual binding table for the
     * keysyms associated with this keycode, which are or might
     * have been used to change the keysym generated by this
     * keycode (to a virtual keysym), must be included in the
     * returned set of standard modifiers.  Remember that "standard
     * modifiers" for a keycode are those modifiers that can affect
     * which keysym is generated by that keycode.
     */
    *modifiers_return |= VirtualStdMods ;

    /* Effective standard modifiers that are set in the event
     * will be 0 in the following bit mask, which will be used
     * to collapse conflicting modifiers in the virtual
     * key binding table, as described below.
     */
    StdModMask = ~(modifiers & eventMods & EffectiveSMMask) ;

    for (i = 0; i < XtNumber(virtualKeysyms); i++)
      {
        XmKeyBinding currBinding = (XmKeyBinding) &keyBindings[i];
        KeySym vks = currBinding->keysym ;
        Modifiers overloaded_mods = currBinding->modifiers
                                                     & effective_Xt_modifiers ;
        KeySym effective_keysym ;
        KeySym uc, lc ;

        /* When the virtual key binding rhs keysym has case, then Shift
         * and Lock are always candidates for being overloaded modifiers.
         * (More brain-dead code for the sake of the VTS, which validates
         * semantics that are sometimes neither documented nor rational.)
         */
        XtConvertCase( dpy, vks, &lc, &uc) ;
        if(    lc != uc    )
          {
            overloaded_mods |= (ShiftMask | LockMask) & effective_Xt_modifiers;
          }
        /* Note that when a virtual key binding table uses a modifier
         * that is treated by the Intrinsics as a standard modifier,
         * we have a semantic collision.  What to do?  Must disambiguate
         * in favor of one or the other.  The VTS assumes that any
         * specified virtual binding table modifier dominates, so even
         * though this is not the appropriate answer in terms of
         * consistency with the Xt keyboard model, we let the virtual
         * key table modifier dominate and reprocess the Xt translation
         * without the virtually preempted modifier to get the keysym
         * for use in matching the rhs of the virtual keysym table.
         */
        if(    overloaded_mods    )
          {
            Modifiers dummy ;
            XtTranslateKey( dpy, keycode, (modifiers & ~overloaded_mods),
                                                   &dummy, &effective_keysym) ;
          }
        else
          {
            effective_keysym = *keysym_return ;
          }
	/* The null binding should not be interpreted as a match
	 * keysym is zero (e.g. pre-edit terminator)
	 */
        /* Those modifiers that are effective standard modifiers and
         * that are set in the event will be ignored in the following
         * conditional (i.e., the state designated in the virtual key
         * binding will not be considered), since these modifiers have
         * already had their affect in the determination of the value
         * of *keysym_return.  This allows matching that is consistent
         * with industry-standard interpretation for keys such as
         * those of the PC-style numeric pad.  This apparent loss of
         * binding semantics is an unavoidable consequence of specifying
         * a modifier in the virtual binding table that is already being
         * used to select one of several keysyms associated with a
         * particular keycode (usually as printed on the keycap).
         * The disambiguation of the collapsing of key bindings
         * is based on "first match" in the virtual key binding table.
         */
        if(    vks && (vks == effective_keysym)
            && ((currBinding->modifiers & StdModMask)
                 == (modifiers & eventMods & VirtualStdMods & StdModMask))  )
          {
            *keysym_return = virtualKeysyms[i].keysym;
            break ;
          }
      }
}

static Modifiers
#ifdef _NO_PROTO
EffectiveStdModMask( dpy, kc_map, ks_per_kc )
        Display *dpy ;
        KeySym *kc_map ;
        int ks_per_kc ;
#else
EffectiveStdModMask(
        Display *dpy,
        KeySym *kc_map,
        int ks_per_kc)
#endif /* _NO_PROTO */
{
  /* This routine determines which set of modifiers can possibly
   * impact the keysym that is generated by the keycode associated
   * with the keymap passed in.  The basis of the algorithm used
   * here is described in section 12.7 "Keyboard Encoding" of the
   * R5 "Xlib - C Language X Interface" specification.
   */
  KeySym uc ;
  KeySym lc ;

  /* Since the group modifier can be any of Mod1-Mod5 or Control, we will
   * return all of these bits if the group modifier is found to be effective.
   * Lock will always be returned (for backwards compatibility with
   * productions assuming that Lock is always a "don't care" modifier
   * for non-alphabetic keys).  Shift will be returned unless it has
   * no effect on the selection of keysyms within either group.
   */
  Modifiers esm_mask = Mod5Mask | Mod4Mask | Mod3Mask | Mod2Mask | Mod1Mask
                                         | ControlMask | LockMask | ShiftMask ;

#ifdef NON_OSF_FIX	/* OSF contact # 18883 */
  /* The standard rules for obtaining a KeySym from a keycode use only
   * the first 4 elements from the associated KeySym list. So
   * normalize ...
   */
  if (ks_per_kc > 4)
    ks_per_kc = 4;
#endif
 
  switch(    ks_per_kc    )
    {
      case 4:
        if(    kc_map[3] != NoSymbol    )
          {
            /* The keysym in position 4 is selected when the group
             * modifier is set and the Shift (or Lock) modifier is
             * set, so both Shift/Lock and the group modifiers are
             " all "effective" standard modifiers.
             */
            break ;
          } 
      case 3:
        if(    kc_map[2] == NoSymbol    )
          {
            /* Both Group 2 keysyms are NoSymbol, so the group
             * modifier has no effect; only Shift and Lock remain
             * as possible effective modifiers.
             */
            esm_mask = ShiftMask | LockMask ;
          }
        else
          {
            XtConvertCase( dpy, kc_map[2], &lc, &uc) ;
            if(    lc != uc    )
              {   
                /* The Group 2 keysym is case-sensitive, so group
                 * modifiers and Shift/Lock modifiers are effective.
                 */
                break ;
              }
          } 
        /* At this fall-through, the group modifier bits have been
         * decided, while the case is still out on Shift/Lock.
         */
      case 2:
        if(    kc_map[1] != NoSymbol    )
          {
            /* Shift/Lock modifier selects keysym from Group 1,
             * so leave those bits in the mask.  The group modifier
             * was determined above, so leave those bits in the mask.
             */
            break ;
          } 
      case 1:
        if(    kc_map[0] != NoSymbol    )
          {
            XtConvertCase( dpy, kc_map[0], &lc, &uc) ;
            if(    lc != uc    )
              {
                /* The Group 1 keysym is case-sensitive, so Shift/Lock
                 * modifiers are effective.
                 */
                break ;
              }
          } 
        /* If we did not break out of the switch before this, then
         * the Shift modifier is not effective; mask it out.
         */
        esm_mask &= ~ShiftMask ;
      default:
        break ;
    } 
  return esm_mask ;
}

#else

static void 
#ifdef _NO_PROTO
FindVirtKey( dpy, keycode, modifiers, modifiers_return, keysym_return )
        Display *dpy ;
        KeyCode keycode ;
        Modifiers modifiers ;
        Modifiers *modifiers_return ;
        KeySym *keysym_return ;
#else
FindVirtKey(
        Display *dpy,
#if NeedWidePrototypes
        int keycode,
#else
        KeyCode keycode,
#endif /* NeedWidePrototypes */
        Modifiers modifiers,
        Modifiers *modifiers_return,
        KeySym *keysym_return )
#endif /* _NO_PROTO */
{
    XmDisplay           xmDisplay = (XmDisplay) XmGetXmDisplay( dpy) ;
    XmKeyBinding	keyBindings = xmDisplay->display.bindings;
    Cardinal		i;
    XmKeyBinding	currBinding;
    Modifiers		eventMods;

    /*
     * get the modifiers from the actual event
     */
    eventMods = (Modifiers)(xmDisplay->display.lastKeyEvent->state);

    for (i = 0; i < XtNumber(virtualKeysyms); i++)
    {
        /*
	 * the null binding should not be interpreted as a match
	 * keysym is zero (e.g. pre-edit terminator)
	 */
	 currBinding = (XmKeyBinding) &keyBindings[i];
	 if ((currBinding->modifiers == (modifiers & eventMods)) &&
	     (currBinding->keysym &&
	     (currBinding->keysym == *keysym_return)))  
	 {
	     *keysym_return = virtualKeysyms[i].keysym;
	     break;
	 }
    }
    *modifiers_return |= ControlMask | Mod1Mask;
}

#endif /* XT_HAS_TRANS_FIX */

/************************************************************************
 *
 *  _XmVirtKeysHandler
 *
 *  This handler provides all kind of magic. It is added to all widgets.
 *     
 ************************************************************************/
/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmVirtKeysHandler( widget, client_data, event, dontSwallow )
        Widget widget ;
        XtPointer client_data ;
        XEvent *event ;
        Boolean *dontSwallow ;
#else
_XmVirtKeysHandler(
        Widget widget,
        XtPointer client_data,
        XEvent *event,
        Boolean *dontSwallow )
#endif /* _NO_PROTO */
{
    XmDisplay xmDisplay = (XmDisplay)XmGetXmDisplay (XtDisplay (widget) );
    KeyCode keycode;

    if (widget->core.being_destroyed)
      {
	  *dontSwallow = False;
	  return;
      }
    switch( event->type ) {
      case KeyPress:
	*(xmDisplay->display.lastKeyEvent) = *((XKeyEvent *)event);

	/*
	 * if keycode is tagged as a modified virtual key, reset
	 * the Xt translation manager cache.
	 */
	keycode = ((XKeyEvent *)event)->keycode;
	if ((xmDisplay->display.keycode_tag[keycode/8] & (1 << (keycode % 8)))
									!= 0) {
	    XtSetKeyTranslator (XtDisplay(widget), (XtKeyProc)XmTranslateKey);
	}
	break;
    }
}

void
#ifdef _NO_PROTO
XmTranslateKey( dpy, keycode, modifiers, modifiers_return, keysym_return )
        Display *dpy ;
        KeyCode keycode ;
        Modifiers modifiers ;
        Modifiers *modifiers_return ;
        KeySym *keysym_return ;
#else
XmTranslateKey(
        Display *dpy,
#if NeedWidePrototypes
        unsigned int keycode,
#else
        KeyCode keycode,
#endif /* NeedWidePrototypes */
        Modifiers modifiers,
        Modifiers *modifiers_return,
        KeySym *keysym_return )
#endif /* _NO_PROTO */
{
    XtTranslateKey(dpy, keycode, modifiers, modifiers_return, keysym_return);

    FindVirtKey(dpy, keycode, modifiers, modifiers_return, keysym_return);
}

void 
#ifdef _NO_PROTO
_XmVirtualToActualKeysym( dpy, virtKeysym, actualKeysymRtn, modifiersRtn )
        Display *dpy ;
        KeySym virtKeysym ;
        KeySym *actualKeysymRtn ;
        Modifiers *modifiersRtn ;
#else
_XmVirtualToActualKeysym(
        Display *dpy,
        KeySym virtKeysym,
        KeySym *actualKeysymRtn,
        Modifiers *modifiersRtn )
#endif /* _NO_PROTO */
{
    Cardinal		i;
    XmKeyBinding	currBinding;
    XmKeyBinding        keyBindings;
    XmDisplay		xmDisplay = (XmDisplay)XmGetXmDisplay (dpy);
    
    keyBindings = xmDisplay->display.bindings;

    for (i = 0; i < XtNumber(virtualKeysyms); i++)
	 {
	     if (virtualKeysyms[i].keysym == virtKeysym)
	       {
		   currBinding = (XmKeyBinding) &keyBindings[i];
		   
		   *actualKeysymRtn = currBinding->keysym;
		   *modifiersRtn = currBinding->modifiers;
		   return;
	       }
	 }
    *actualKeysymRtn = NoSymbol;
    *modifiersRtn = 0;
}


Boolean 
#ifdef _NO_PROTO
_XmVirtKeysLoadFileBindings( fileName, binding )
    char      *fileName;
    String    *binding;
#else
_XmVirtKeysLoadFileBindings(
    char      *fileName,
    String    *binding )
#endif /* _NO_PROTO */
{
    FILE *fileP;
    int offset = 0;
    int count;

    if ((fileP = fopen (fileName, "r")) != NULL) {
	*binding = NULL;
	do {
	    *binding = XtRealloc (*binding, offset + BUFFERSIZE);
	    count = (int)fread (*binding + offset, 1, BUFFERSIZE, fileP);
	    offset += count;
	} while (count == BUFFERSIZE);
	(*binding)[offset] = '\0';

	/* trim unused buffer space */
	*binding = XtRealloc (*binding, offset + 1);

	return (True);
    }
    else {
	return (False);
    }
}


static void 
#ifdef _NO_PROTO
LoadVendorBindings(display, path, fp, binding )
    Display   *display;
    char      *path;
    FILE      *fp;
    String    *binding;
#else
LoadVendorBindings(
    Display   *display,
    char      *path,
    FILE      *fp,
    String    *binding )
#endif /* _NO_PROTO */
{
    char buffer[MAXLINE];
    char *bindFile;
    char *vendor;
    char *vendorV;
    char *ptr;
    char *start;

    vendor = ServerVendor(display);
    vendorV = XtMalloc (strlen(vendor) + 20); /* assume rel.# is < 19 digits */
    sprintf (vendorV, "%s %d", vendor, VendorRelease(display));

    while (fgets (buffer, MAXLINE, fp) != NULL) {
	ptr = buffer;
	while (*ptr != '"' && *ptr != '!' && *ptr != '\0') ptr++;
	if (*ptr != '"') continue;
	start = ++ptr;
	while (*ptr != '"' && *ptr != '\0') ptr++;
	if (*ptr != '"') continue;
	*ptr = '\0';
	if ((strcmp (start, vendor) == 0) || (strcmp (start, vendorV) == 0)) {
	    ptr++;
	    while (isspace((unsigned char)*ptr) && *ptr != '\0') ptr++;
	    if (*ptr == '\0') continue;
	    start = ptr;
	    while (!isspace((unsigned char)*ptr) && *ptr != '\n' && *ptr != '\0') ptr++;
	    *ptr = '\0';
	    bindFile = _XmOSBuildFileName (path, start);
	    if (_XmVirtKeysLoadFileBindings (bindFile, binding)) {
		XtFree (bindFile);
		break;
	    }
	    XtFree (bindFile);
	}
    }
    XtFree (vendorV);
    return;
}


int 
#ifdef _NO_PROTO
_XmVirtKeysLoadFallbackBindings( display, binding )
    Display	*display;
    String	*binding;
#else
_XmVirtKeysLoadFallbackBindings(
    Display	*display,
    String	*binding )
#endif /* _NO_PROTO */
{
    XmDefaultBindingString currDefault;
    int i;
    FILE *fp;
    char *homeDir;
    char *fileName;
    char *bindDir;
    static char xmbinddir_fallback[] = XMBINDDIR_FALLBACK;

    *binding = NULL;

    /* load .motifbind - necessary, if mwm and xmbind are not used */

    homeDir = _XmOSGetHomeDirName();
    fileName = _XmOSBuildFileName (homeDir, MOTIFBIND);
    _XmVirtKeysLoadFileBindings (fileName, binding);
    XtFree (fileName);

    /* look for a match in the user's xmbind.alias */

    if (*binding == NULL) {
	fileName = _XmOSBuildFileName (homeDir, XMBINDFILE);
	if ((fp = fopen (fileName, "r")) != NULL) {
	    LoadVendorBindings (display, homeDir, fp, binding);
	    fclose (fp);
	}
	XtFree (fileName);
    }

    if (*binding != NULL) {

	/* set the user property for future Xm applications */

	XChangeProperty (display, RootWindow(display, 0),
		XInternAtom (display, "_MOTIF_BINDINGS", False),
		XA_STRING, 8, PropModeReplace,
		(unsigned char *)*binding, strlen(*binding));
	return (0);
    }

    /* look for a match in the system xmbind.alias */

    if (*binding == NULL) {
	if ((bindDir = getenv(XMBINDDIR)) == NULL)
	    bindDir = xmbinddir_fallback;
	fileName = _XmOSBuildFileName (bindDir, XMBINDFILE);
	if ((fp = fopen (fileName, "r")) != NULL) {
	    LoadVendorBindings (display, bindDir, fp, binding);
	    fclose (fp);
	}
	XtFree (fileName);
    }

    /* check hardcoded fallbacks (for 1.1 bc) */

    if (*binding == NULL) for (i = 0, currDefault = fallbackBindingStrings;
         i < XtNumber(fallbackBindingStrings);
         i++, currDefault++) {
	if (strcmp(currDefault->vendorName, ServerVendor(display)) == 0) {
	    *binding = XtMalloc (strlen (currDefault->defaults) + 1);
	    strcpy (*binding, currDefault->defaults);
	    break;
	}
    }

    /* use generic fallback bindings */

    if (*binding == NULL) {
	*binding = XtMalloc (strlen (defaultFallbackBindings) + 1);
	strcpy (*binding, defaultFallbackBindings);
    }

    /* set the fallback property for future Xm applications */

    XChangeProperty (display, RootWindow(display, 0),
		XInternAtom (display, "_MOTIF_DEFAULT_BINDINGS", False),
		XA_STRING, 8, PropModeReplace,
		(unsigned char *)*binding, strlen(*binding));

    return (0);
}

