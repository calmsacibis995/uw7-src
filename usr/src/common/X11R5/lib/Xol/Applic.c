#ifndef	NOIDENT
#ident	"@(#)olmisc:Applic.c	1.48"
#endif

/*
 *************************************************************************
 * Description:
 *	This file contains the routines and static information that is
 * Global to the application.
 ******************************file*header********************************
 */
						/* #includes go here	*/
#include <stdio.h>
#ifdef I18N
#include <locale.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#endif
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookI.h>
#include <Xol/Notice.h>
#include <Xol/Olg.h>
#ifdef I18N
#include "sys/stat.h"
#endif

#ifdef	__hpux
# define LC_MESSAGES LC_CTYPE
# define setlocale OlSetLocale
#endif

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static XrmDatabase	_OlGetLocaleFileDatabase OL_ARGS((Widget));
static void _OlGetLocaleAnnouncer OL_ARGS((Widget, int, XrmValue *));
static Bool _OlHelpPred OL_ARGS((String));
static void _OlHelpPath OL_ARGS((Widget, int, XrmValue *));

					/* public procedures		*/

extern void	OlGetApplicationValues OL_ARGS((Widget, ArgList, Cardinal));
extern void	OlSetApplicationValues OL_ARGS((Widget, ArgList, Cardinal));
extern void	_OlBeepDisplay OL_ARGS((Widget, Cardinal));
extern Cardinal	_OlGetMultiObjectCount OL_ARGS((Widget));
extern Cardinal	_OlGetMouseDampingFactor OL_ARGS((Widget));
extern char *	_OlGetScaleMap OL_NO_ARGS();
extern Boolean 	_OlGetStatusArea OL_NO_ARGS();
extern char *	_OlGetFontGroupDef OL_NO_ARGS();
extern Atom		_OlGetFrontEndAtom OL_NO_ARGS();

extern char *	_OlGetHelpDirectory OL_ARGS((Widget));
  
extern Boolean	_OlGrabPointer OL_ARGS((Widget, Bool, unsigned int,
					int, int, Window, Cursor, Time));
extern Boolean	_OlGrabServer OL_ARGS((Widget));
extern void	_OlInitAttributes OL_ARGS((Widget));
extern Boolean	_OlSelectDoesPreview OL_ARGS((Widget));
extern void	_OlUngrabPointer OL_ARGS((Widget));
extern void	_OlUngrabServer OL_ARGS((Widget));


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#ifdef I18N
OlImFunctions       *OlImFuncs;
#endif

static Widget	widget_with_grab = (Widget)NULL;

	/* default value for XtNdontCare, for Button events */
#define DONT_CARE_BITS  (LockMask | Mod2Mask)           /* Mod2Mask=NumLock*/

	/* default value for XtNkeyDontCare, for KeyPress events */
#define KEY_DONT_CARE_BITS  LockMask

_OlAppAttributes	ol_app_attributes = { 0 };
static String OL_FRONTEND_MSG = "OL_FRONTEND_MSG";

static XtResource
resources[] = {

	/*
	 * { ResourceName, ResourceClass,
	 *   ResourceType, ResourceSize, ResourceOffset
	 *   DefaultType, DefaultValue }
	 */

#define offset(F) XtOffsetOf(_OlAppAttributes,F)

	{ XtNbeep, XtCBeep,
	  XtROlDefine, sizeof(OlDefine), offset(beep),
	  XtRImmediate, (XtPointer) OL_BEEP_ALWAYS },

	{ XtNbeepVolume, XtCBeepVolume,
	  XtRInt, sizeof(int), offset(beep_volume),
	  XtRImmediate, (XtPointer) 0 },

	{ XtNmultiClickTimeout, XtCMultiClickTimeout,
	  XtRCardinal, sizeof(Cardinal), offset(multi_click_timeout),
	  XtRImmediate, (XtPointer) 500 },

	{ XtNmultiObjectCount, XtCMultiObjectCount,
	  XtRCardinal, sizeof(Cardinal), offset(multi_object_count),
	  XtRImmediate, (XtPointer) 3 },

	{ XtNmouseDampingFactor, "MouseDampingFactor",
	  XtRCardinal, sizeof(Cardinal), offset(mouse_damping_factor),
	  XtRImmediate, (XtPointer) 8 },

	{ XtNselectDoesPreview, XtCSelectDoesPreview,
	  XtRBoolean, sizeof(Boolean), offset(select_does_preview),
	  XtRImmediate, (XtPointer) False },

	{ XtNgrabPointer, XtCGrabPointer,
	  XtRBoolean, sizeof(Boolean), offset(grab_pointer),
	  XtRImmediate, (XtPointer) True },

	{ XtNgrabServer, XtCGrabServer,
	  XtRBoolean, sizeof(Boolean), offset(grab_server),
	  XtRImmediate, (XtPointer) False },

	{ XtNdontCare, XtCDontCare,
	  XtROlBitMask, sizeof(OlBitMask), offset(dont_care),
	  XtRImmediate, (XtPointer) DONT_CARE_BITS },

	{ XtNkeyDontCare, XtCDontCare,
	  XtROlBitMask, sizeof(OlBitMask), offset(key_dont_care),
	  XtRImmediate, (XtPointer) KEY_DONT_CARE_BITS },

	{ XtNthreeD, XtCThreeD,
	  XtRBoolean, sizeof(Boolean), offset(three_d),
	  XtRImmediate, (XtPointer) True },

	{ XtNscaleMap, XtCScaleMap,
	  XtRString, sizeof(String), offset(scale_map_file),
	  XtRString, (XtPointer) NULL },

	{ XtNhelpDirectory, XtCHelpDirectory,
	  XtRString, sizeof(String), offset(help_directory),
	  XtRCallProc, (XtPointer)_OlHelpPath },

	{ XtNbackground, XtCBackground,
	  XtRPixel, sizeof(Pixel), offset(generic_background),
	  XtRString, (XtPointer) XtDefaultBackground },

	{ "textBackground", XtCTextBackground,
	  XtRPixel, sizeof(Pixel), offset(text_background),
	  XtRString, (XtPointer) XtDefaultBackground },

	{ "textFontColor", XtCTextFontColor,
	  XtRPixel, sizeof(Pixel), offset(text_font_color),
	  XtRString, (XtPointer) XtDefaultForeground },

	{ "controlBackground", "ControlBackground",
	  XtRPixel, sizeof(Pixel), offset(control_background),
	  XtRString, (XtPointer) XtDefaultBackground },

	{ "controlForeground", "ControlBackground",
	  XtRPixel, sizeof(Pixel), offset(control_foreground),
	  XtRString, (XtPointer) XtDefaultBackground },

	{ "controlFontColor", "ControlFontColor",
	  XtRPixel, sizeof(Pixel), offset(control_font_color),
	  XtRString, (XtPointer) XtDefaultForeground },

	{ XtNinputFocusColor, XtCInputFocusColor,
	  XtRPixel, sizeof(Pixel), offset(input_focus_color),
	  XtRString, (XtPointer) XtDefaultForeground },

	{ XtNmnemonicPrefix, XtCMnemonicPrefix,
	  XtRModifiers, sizeof(Modifiers), offset(mnemonic_modifiers),
	  XtRString, (XtPointer)NAlt },

	{ XtNshowMnemonics, XtCShowAccelerators,
	  XtROlDefine, sizeof(OlDefine), offset(show_mnemonics),
	  XtRString, (XtPointer)Nunderline },

	{ XtNshowAccelerators, XtCShowAccelerators,
	  XtROlDefine, sizeof(OlDefine), offset(show_accelerators),
	  XtRString, (XtPointer)Ndisplay },

	{ XtNshiftName, XtCShiftName,
	  XtRString, sizeof(String), offset(shift_name),
	  XtRString, (XtPointer)NShift },

	{ XtNlockName, XtCLockName,
	  XtRString, sizeof(String), offset(lock_name),
	  XtRString, (XtPointer)NLock },

	{ XtNcontrolName, XtCControlName,
	  XtRString, sizeof(String), offset(control_name),
	  XtRString, (XtPointer)NCtrl },

	{ XtNmod1Name, XtCMod1Name,
	  XtRString, sizeof(String), offset(mod1_name),
	  XtRString, (XtPointer)NAlt },

	{ XtNmod2Name, XtCMod2Name,
	  XtRString, sizeof(String), offset(mod2_name),
	  XtRString, (XtPointer)NMod2 },

	{ XtNmod3Name, XtCMod3Name,
	  XtRString, sizeof(String), offset(mod3_name),
	  XtRString, (XtPointer)NMod3 },

	{ XtNmod4Name, XtCMod4Name,
	  XtRString, sizeof(String), offset(mod4_name),
	  XtRString, (XtPointer)NMod4 },

	{ XtNmod5Name, XtCMod5Name,
	  XtRString, sizeof(String), offset(mod5_name),
	  XtRString, (XtPointer)NMod5 },

	{ XtNhelpModel, XtCHelpModel,
	  XtROlDefine, sizeof(OlDefine), offset(help_model),
	  XtRImmediate, (XtPointer) OL_POINTER },

	{ XtNmouseStatus, XtCMouseStatus,
	  XtRBoolean, sizeof(Boolean), offset(mouse_status),
	  XtRImmediate, (XtPointer) True },

	{ XtNdragRightDistance, XtCDragRightDistance,
	  XtRDimension, sizeof(Dimension), offset(drag_right_distance),
	  XtRImmediate, (XtPointer) 20 },

	{ XtNmenuMarkRegion, XtCMenuMarkRegion,
	  XtRDimension, sizeof(Dimension), offset(menu_mark_region),
	  XtRImmediate, (XtPointer) 10 },

	{ XtNkeyRemapTimeOut, XtCKeyRemapTimeOut,
	  XtRCardinal, sizeof(Cardinal), offset(key_remap_timeout),
	  XtRImmediate, (XtPointer) 2 },

	{ XtNxnlLanguage, XtCXnlLanguage, 
	  XtRString, sizeof(String), offset(xnllanguage),
	  XtRCallProc, (XtPointer) _OlGetLocaleAnnouncer},
	{ XtNinputLang, XtCInputLang,
	  XtRString, sizeof(String), offset(input_lang),
	  XtRString, (XtPointer) NULL },
	{ XtNdisplayLang, XtCDisplayLang,
	  XtRString, sizeof(String), offset(display_lang),
	  XtRString, (XtPointer) NULL },
	{ XtNtimeFormat, XtCTimeFormat,
	  XtRString, sizeof(String), offset(tdformat),
	  XtRString, (XtPointer) NULL },
	{ XtNnumeric, XtCNumeric,
	  XtRString, sizeof(String), offset(numeric),
	  XtRString, (XtPointer) NULL },
	{ XtNinputMethod, XtCInputMethod,
	  XtRString, sizeof(String), offset(input_method),
	  XtRString, (XtPointer) NULL },
	{ XtNfontGroupDef, XtCFontGroupDef,
	  XtRString, sizeof(String), offset(font_group_def),
	  XtRString, (XtPointer) NULL },
	{ XtNimStatus, XtCImStatus,
	  XtRBoolean, sizeof(Boolean), offset(im_status),
	  XtRImmediate, (XtPointer) False },
	{ XtNfrontEndString, XtCFrontEndString,
	  XtRString, sizeof(String), offset(frontend_im_string),
	  XtRString, (XtPointer) NULL },

	{ XtNuseShortOLWinAttr, XtCUseShortOLWinAttr,
	  XtRBoolean, sizeof(Boolean), offset(short_olwinattr),
	  XtRImmediate, (XtPointer) False },

	{ XtNcolorTupleList, XtCColorTupleList,
	  XtROlColorTupleList, sizeof(OlColorTupleList *), offset(color_tuple_list),
	  XtRImmediate, (XtPointer) 0 },

	{ XtNuseColorTupleList, XtCUseColorTupleList,
	  XtRBoolean, sizeof(Boolean), offset(use_color_tuple_list),
	  XtRImmediate, (XtPointer) XtUnspecifiedBoolean },

#undef offset

};

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/* There are no private procedures */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlGetAppAttributesRef - this routine returns a reference to the
 * structure of application attributes.
 *
 * USE CAREFULLY! USE READ-ONLY!
 *
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
extern _OlAppAttributes *
_OlGetAppAttributesRef OLARGLIST((widget))
	OLGRA( Widget,	widget )
{
	/*
	 * Currently the widget argument is ignored.
	 */
	return (&ol_app_attributes);
}

/*
 *************************************************************************
 * OlGetApplicationValues - this routine returns the values associated
 * with the application.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
extern void
OlGetApplicationValues OLARGLIST((w, args, num_args))
	OLARG( Widget,		w)	/* reserved for future use	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal,	num_args)
{
	XtGetSubvalues((XtPointer)&ol_app_attributes, resources,
			XtNumber(resources), args, num_args);
} /* END OF OlGetApplicationValues() */

/*
 *************************************************************************
 * OlSetApplicationValues - this routine sets the values associated
 * with the application.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
extern void
OlSetApplicationValues OLARGLIST((w, args, num_args))
	OLARG( Widget,		w)	/* reserved for future use	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal,	num_args)
{
	XtSetSubvalues((XtPointer)&ol_app_attributes, resources,
			XtNumber(resources), args, num_args);
} /* END OF OlGetApplicationValues() */

/*
 *************************************************************************
 * _OlBeepDisplay - this routine beeps the display.  It takes a widget
 * id as an argument.  The widget's class is checked with an internal
 * list of widget classes that are permitted to beep the display.
 * If the widget is on the list, the display is beeped.  If not, the
 * procedure returns immediately.
 ****************************procedure*header*****************************
 */
void
_OlBeepDisplay(widget, count)
	Widget		widget;
	Cardinal	count;
{
	if (widget != (Widget)NULL && count != (Cardinal)0)
	{
		switch(ol_app_attributes.beep)
		{
		case OL_BEEP_NOTICES:
			if (XtIsSubclass(widget, noticeShellWidgetClass)
						== False)
			{
				count = 0;
			}
			break;
		case OL_BEEP_NEVER:
			count = 0;
			break;
		default:			/* OL_BEEP_ALWAYS	*/
			break;
		}

		for(; count != (Cardinal)0; --count)
		{
			XBell(XtDisplayOfObject(widget),
				ol_app_attributes.beep_volume);
		}
	}
} /* END OF _OlBeepDisplay() */

/*
 *************************************************************************
 * _OlGetMouseDampingFactor - this routine returns the damping factor used
 * to determine a single mouse click.
 ****************************procedure*header*****************************
 */
 /* ARGSUSED */
Cardinal
_OlGetMouseDampingFactor(widget)
	Widget	widget;
{
	return((Cardinal)ol_app_attributes.mouse_damping_factor);
} /* END OF _OlGetMouseDampingFactor() */

/*
 *************************************************************************
 * _OlGetMultiObjectCount - this routine returns the damping factor used
 * to determine a single mouse click.
 ****************************procedure*header*****************************
 */
 /* ARGSUSED */
Cardinal
_OlGetMultiObjectCount(widget)
	Widget	widget;
{
	return((Cardinal)ol_app_attributes.multi_object_count);
} /* END OF _OlGetMultiObjectCount() */

/*
 *************************************************************************
 * _OlGetScaleMap - this routine returns the name of the file
 * containing the scale to resolution map.
 ****************************procedure*header*****************************
 */
char *
_OlGetScaleMap ()
{
    return (ol_app_attributes.scale_map_file);
} /* END OF _OlGetScaleMap() */

/*
 *************************************************************************
 * _OlGetStatusArea - this routine returns the Boolean value indicating
 * whether or not a StatusArea is needed by an application for
 * Internationalization.
 ****************************procedure*header*****************************
 */
Boolean
_OlGetStatusArea ()
{
    return (ol_app_attributes.im_status);
} /* END OF _OlGetStatusArea() */

/*
 *************************************************************************
 * _OlGetFrontEndAtom- this routine returns the Atom used by 
 * a front end input method to identify ClientMessage events
	* that contain converted keyboard input. (Internationalization)
 ****************************procedure*header*****************************
 */
Atom		
_OlGetFrontEndAtom ()
{
		return(ol_app_attributes.frontend_im_atom);
}	/* END of _OlGetFrontEndAtom */	
/*
 *************************************************************************
 * _OlGetFontGroupDef - this routine returns the value of the global
 *  resource FontGroupDef.
 ****************************procedure*header*****************************
 */
char *
_OlGetFontGroupDef ()
{
    return (ol_app_attributes.font_group_def);
} /* END OF _OlGetFontGroupDef() */

/*
 *************************************************************************
 * _OlGetHelpDirectory - this routine returns the value of the global
 * resource HelpDirectory.
 ****************************procedure*header*****************************
 */
char *
_OlGetHelpDirectory OLARGLIST((widget))
  OLGRA( Widget, widget)	/* reserved for future use */
{
    return (ol_app_attributes.help_directory);
} /* END OF _OlGetHelpDirectory() */

/*
 *************************************************************************
 * _OlHelpPred - a simple predicate used by _OlHelpPath in its call to
 * XtResolvePathname so that it will find a directory, not a file.
 ****************************procedure*header*****************************
 */
static Bool
_OlHelpPred OLARGLIST((filename))
  OLGRA (String, filename)
{
#if defined(SVR4_0) || defined(SVR4)
  struct stat			status;
  
  return ((access(filename, R_OK|X_OK) == 0) &&
	  (stat(filename, &status) == 0) &&
	  ((status.st_mode & S_IFDIR) == S_IFDIR));
#else
  return (access(filename, R_OK|X_OK) == 0);
#endif
}

/*
 *************************************************************************
 * _OlHelpPath - this routine returns the default value of the 
 *  help directory as found by XtResolvePathname.
 ****************************procedure*header*****************************
 */
static void
_OlHelpPath OLARGLIST((widget, closure, value))
  OLARG (Widget, widget)
  OLARG (int, closure)
  OLGRA (XrmValue *, value)
{
    value->addr = XtResolvePathname(XtDisplayOfObject(widget), "help",
				    NULL, NULL, (String)NULL,
				    (Substitution)NULL, (Cardinal)0,
				    (XtFilePredicate) _OlHelpPred);
} /* END OF _OlHelpPath() */

/*
 *************************************************************************
 * _OlGrabPointer - this routine acts as a locking switch that
 * keeps track of a widget who has explicitly grabbed the pointer.
 * It returns a Boolean value to indicate whether or not a widget is
 * allowed to grab the pointer and an XtGrabPointer is done. 
 * If this routine returns True, the requesting widget will be saved.
 * If this routine returns False, than the requesting should not grab
 * the pointer, since some other widget already has a grab on it.
 ****************************procedure*header*****************************
 */
Boolean
_OlGrabPointer(widget, owner_events, event_mask, pointer_mode, keyboard_mode,
		confine_to, cursor, time)
	Widget		widget;
	Bool		owner_events;
	unsigned int	event_mask;
	int		pointer_mode;
	int		keyboard_mode;
	Window		confine_to;
	Cursor		cursor;
	Time		time;
{
	Boolean		return_val = False;

				/* If NULL is passed in, remove any
				 * prior grabs; else, try to register a
				 * new grab				*/

	if (widget == (Widget)NULL)
	{
		_OlUngrabPointer(widget);
	}
	else if (ol_app_attributes.grab_pointer == True)
	{
		if (widget_with_grab == (Widget)NULL ||
		    widget_with_grab == widget)
		{
			if (XtIsRealized(widget) == True)
			{
#if Xt_works_right
				if (GrabSuccess == XtGrabPointer(
						(_OlIsGadget(widget) ?
						XtParent(widget) : widget),
#else
		/* Use Xlib directly since Xt is broken	*/

				if (GrabSuccess == XGrabPointer(
						XtDisplayOfObject(widget),
						XtWindowOfObject(widget),
#endif
						owner_events, event_mask,
						pointer_mode, keyboard_mode,
						confine_to, cursor, time))
				{
					return_val = True;
					widget_with_grab = widget;
				}
			}
			else
			{
				OlVaDisplayWarningMsg(XtDisplayOfObject(widget),
						      OleNnotRealized,
						      OleTpointerGrab,
						      OleCOlToolkitWarning,
						      OleMnotRealized_pointerGrab,
						      "_OlGrabPointer",
						      XrmQuarkToString(widget->core.xrm_name),
						      XtClass(widget)->core_class.class_name);
			}
		}
	}

	return(return_val);
} /* END OF _OlGrabPointer() */

/*
 *************************************************************************
 * _OlGrabServer - this grabs the server if the application attributes
 * permit it.  The function returns true if the grab is successful; false
 * is returned otherwise.
 ****************************procedure*header*****************************
 */
Boolean
_OlGrabServer(widget)
	Widget	widget;
{
	if (ol_app_attributes.grab_server == True && widget != (Widget)NULL)
	{
		XGrabServer(XtDisplayOfObject(widget));
		return ((Boolean)True);
	}
	return((Boolean)False);
} /* END OF _OlGrabServer() */


/*
 *************************************************************************
 * _OlInitAttributes - this routine gets the OPEN LOOK attributes for this
 * application.  This routine is called at application start-up and when
 * the user dynamically changes an application parameter.
 ****************************procedure*header*****************************
 */
extern void
_OlInitAttributes(w)
	Widget	w;
{
	static Boolean first_time = True;
	char	*str;

	if (first_time == True)
	{
			/*
			 * first_time is set to False at end of function
			 */
		_OlAddOlDefineType("never",           OL_BEEP_NEVER);
		_OlAddOlDefineType("always",          OL_BEEP_ALWAYS);
		_OlAddOlDefineType("notices",         OL_BEEP_NOTICES);

		_OlAddOlDefineType(Nunderline,        OL_UNDERLINE);
		_OlAddOlDefineType(Nhighlight,        OL_HIGHLIGHT);
		_OlAddOlDefineType(Nnone,             OL_NONE);
		_OlAddOlDefineType(Ninactive,         OL_INACTIVE);
		_OlAddOlDefineType(Ndisplay,          OL_DISPLAY);

		_OlAddOlDefineType("pointer",         OL_POINTER);
		_OlAddOlDefineType("inputfocus",      OL_INPUTFOCUS);
	}
	else
	{
		if (ol_app_attributes.scale_map_file)
		{
		XtFree (ol_app_attributes.scale_map_file);
		ol_app_attributes.scale_map_file = (char *) NULL;
		}
		if (ol_app_attributes.help_directory)
		{
		XtFree (ol_app_attributes.help_directory);
		ol_app_attributes.help_directory = (char *) NULL;
		}
		if (ol_app_attributes.input_method)
		{
		XtFree (ol_app_attributes.input_method);
		ol_app_attributes.input_method = (char *) NULL;
		}
		if (ol_app_attributes.font_group_def)
		{
		XtFree (ol_app_attributes.font_group_def);
		ol_app_attributes.font_group_def = (char *) NULL;
		}
		if (ol_app_attributes.input_lang)
		{
		XtFree (ol_app_attributes.input_lang);
		ol_app_attributes.input_lang = (char *) NULL;
		}
		if (ol_app_attributes.display_lang)
		{
		XtFree (ol_app_attributes.display_lang);
		ol_app_attributes.display_lang = (char *) NULL;
		}
		if (ol_app_attributes.xnllanguage)
		{
		XtFree (ol_app_attributes.xnllanguage);
		ol_app_attributes.xnllanguage = (char *) NULL;
		}
		if (ol_app_attributes.tdformat)
		{
		XtFree (ol_app_attributes.tdformat);
		ol_app_attributes.tdformat = (char *) NULL;
		}
		if (ol_app_attributes.numeric)
		{
		XtFree (ol_app_attributes.numeric);
		ol_app_attributes.tdformat = (char *) NULL;
		}
		if (ol_app_attributes.frontend_im_string)
		{
		XtFree (ol_app_attributes.frontend_im_string);
		ol_app_attributes.frontend_im_string = (char *) NULL;
		}
	}

				/* Initialize the Application Resources	*/

	XtGetApplicationResources(w, (XtPointer)&ol_app_attributes,
		resources, XtNumber(resources), (ArgList)NULL, (Cardinal)0);

	/* String resources are not certain to stay around, so make a local
	 * copy of these resource values.
	 */
	if (ol_app_attributes.scale_map_file)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.scale_map_file)+1);
	    strcpy (str, ol_app_attributes.scale_map_file);
	    ol_app_attributes.scale_map_file = str;
	}
	if (ol_app_attributes.help_directory)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.help_directory)+1);
	    strcpy (str, ol_app_attributes.help_directory);
	    ol_app_attributes.help_directory = str;
	}
	if (ol_app_attributes.input_method)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.input_method)+1);
	    strcpy (str, ol_app_attributes.input_method);
	    ol_app_attributes.input_method = str;
	}
	if (ol_app_attributes.font_group_def)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.font_group_def)+1);
	    strcpy (str, ol_app_attributes.font_group_def);
	    ol_app_attributes.font_group_def = str;
	}
	if (ol_app_attributes.display_lang)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.display_lang)+1);
	    strcpy (str, ol_app_attributes.display_lang);
	    ol_app_attributes.display_lang = str;
	}
	if (ol_app_attributes.input_lang)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.input_lang)+1);
	    strcpy (str, ol_app_attributes.input_lang);
	    ol_app_attributes.input_lang = str;
	}
	if (ol_app_attributes.xnllanguage)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.xnllanguage)+1);
	    strcpy (str, ol_app_attributes.xnllanguage);
	    ol_app_attributes.xnllanguage = str;
	}
	if (ol_app_attributes.tdformat)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.tdformat)+1);
	    strcpy (str, ol_app_attributes.tdformat);
	    ol_app_attributes.tdformat = str;
	}
	if (ol_app_attributes.numeric)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.numeric)+1);
	    strcpy (str, ol_app_attributes.numeric);
	    ol_app_attributes.numeric = str;
	}
	if (ol_app_attributes.frontend_im_string)
	{
	    str = (char *)XtMalloc(strlen(ol_app_attributes.frontend_im_string)+1);
	    strcpy (str, ol_app_attributes.frontend_im_string);
	    ol_app_attributes.frontend_im_string = str;
	}

	if (first_time == True){
		first_time = False;		/* must be outside I18N, see top of function */
#ifdef I18N

#ifndef sun
			/* Initialize resources for Internationalization */
		(void)OlLocaleInitialize(w);
#endif

#endif
	}
	OlgSetStyle3D (ol_app_attributes.three_d);

} /* END OF _OlInitAttributes() */

/*
 *************************************************************************
 * _OlSelectDoesPreview - if the SELECT Button is in the power-user mode
 * this routine returns True, otherwise, false is returned.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
Boolean
_OlSelectDoesPreview OLARGLIST((w))
	OLGRA( Widget,	w)
{
	if (OlGetGui() == OL_MOTIF_GUI)
		return(False);
	else
		return((Boolean) ol_app_attributes.select_does_preview);
} /* END OF _OlSelectDoesPreview() */

/*
 *************************************************************************
 * _OlUngrabPointer - this routine is called by widgets that want to
 * ungrab the mouse pointer, but are not sure if some other widget
 * really has the grab and is letting events pass through to the owner.
 * If some other widget really has the grab, the routine simply returns.
 * If the widget requesting the ungrab has the grab or if no other widget
 * has a grab, the routine calls XtUngrabPointer().
 *	If a NULL widget id is passed in and there has not been a 
 * previous _OlGrabPointer() call, the routine routines.  But, if
 * the passed-in widget is NULL and there was a previous _OlGrabPointer()
 * call, the routine ungrabs the pointer and resets the "widget_with_grab"
 * variable.
 ****************************procedure*header*****************************
 */
void
_OlUngrabPointer OLARGLIST((widget))
	OLGRA( Widget, widget)
{
	extern Widget widget_with_grab;

	if (widget != (Widget) NULL) {
		if (widget_with_grab == (Widget) NULL ||
		    widget_with_grab == widget) {
			widget_with_grab = (Widget) NULL;
		}
		else {
			widget = NULL;
		}
	}
	else if (widget_with_grab != (Widget) NULL) {
		widget_with_grab = (Widget) NULL;
	}
	else {
		widget = NULL;
	}

	if (widget != (Widget)NULL) {
#if Xt_works_right
		XtUngrabPointer((_OlIsGadget(widget) ?
			XtParent(widget) : widget), CurrentTime);
#else
	/* Use XLib directly since Xt is broken
	 */
		XUngrabPointer(XtDisplayOfObject(widget), CurrentTime);
#endif
	}
} /* END OF _OlUngrabPointer() */

/*
 *************************************************************************
 * _OlUngrabServer - this ungrabs the server.
 ****************************procedure*header*****************************
 */
void
_OlUngrabServer OLARGLIST((widget))
	OLGRA( Widget, widget)
{
	if (widget != (Widget)NULL)
	{
		XUngrabServer(XtDisplayOfObject(widget));
	}
} /* END OF _OlUngrabServer() */

/**
 ** OlQueryMnemonicDisplay()
 ** OlQueryAcceleratorDisplay()
 **/

OlDefine
OlQueryMnemonicDisplay OLARGLIST(( w ))
	OLGRA( Widget, w)
{
	return (ol_app_attributes.show_mnemonics);
}

OlDefine
OlQueryAcceleratorDisplay OLARGLIST(( w ))
	OLGRA( Widget, w)
{
	return (ol_app_attributes.show_accelerators);
}



/*
	LocaleInitialize() function is called at the start up and it
	announces the application locale to operating system. The locale
	definition file for the specified locale is read, and appropriate
	locale specific resources are initialized (the values come
	from specified resource or defaults from locale definition file)
*/ 
	
#ifdef I18N
extern void
OlLocaleInitialize OLARGLIST((w))
	OLGRA(Widget, w)
{

	char * name;
	XrmName			xrm_name[2];
	XrmClass		xrm_class[2];
	XrmRepresentation 	type;
	XrmValue		value;
	XrmDatabase  db = NULL;
	

	/* check if any of the input method or font specific resources
	   are NULL. If so, then read their default value from the locale
	   definition file, update resource database and keep a copy in
	   ol_locale_attributes structure.
	   Note that we do not have to check for locale specific resources
	   such as, inputlang, numeric etc., if they are not specified,
	   setlocale() is going to use value of "xnlLanguage" later on.
	*/

	if ((ol_app_attributes.input_method  == NULL)   || 
	    (*ol_app_attributes.input_method == '\0')   ||
	    (ol_app_attributes.font_group_def == NULL)  ||
	    (*ol_app_attributes.font_group_def == '\0'))
		{
		if ((db = _OlGetLocaleFileDatabase(w)) == NULL)
		  OlVaDisplayWarningMsg(XtDisplay(w),
					OleNfileApplic,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileApplic_msg1);
		/* imstatus goes with the inputmethod resource */
		else
		   {
		   if ((ol_app_attributes.input_method == NULL) ||
		       (*ol_app_attributes.input_method == '\0'))
		 	   {
			   xrm_name[0]  = XrmStringToName(XtNinputMethod);
			   xrm_name[1] = 0;
			   xrm_class[0] = XrmStringToClass(XtNinputMethod);
			   xrm_class[1] = 0;
			   if (XrmQGetResource(db, xrm_name, xrm_class, 
					       &type, &value))
			     {
			     char *p;
			     /* must copy the value returned in "value" */
					if (((String)value.addr != NULL) && *(value.addr))
						{
						ol_app_attributes.input_method = (char *) 
							XtMalloc(value.size+1);
						strcpy(ol_app_attributes.input_method, 
				       	(String)value.addr);
					  	p =  ol_app_attributes.input_method;
					  	while (*p && !isspace(*p)) p++;
					  	*p = '\0';
			        	}
			     else ol_app_attributes.input_method = NULL;

			     /* and now get "imstatus" */
			     xrm_name[0]  = XrmStringToName(XtNimStatus);
			     xrm_name[1] = 0;
                             xrm_class[0] = XrmStringToClass(XtNimStatus);
			     xrm_class[1] = 0;
                             if (XrmQGetResource(db, xrm_name, xrm_class,                                                       &type, &value))
				if ((value.addr != NULL) && *(value.addr))

				   /* this is not the ideal way, but will do
				      for now.
				   */
				   if ((strncmp((String)value.addr, "True", 4) == 0) ||
				       (strncmp((String)value.addr, "true", 4) == 0) ||
				       (strncmp((String)value.addr, "TRUE", 4) == 0))
					   ol_app_attributes.im_status = True; 
			           else   ol_app_attributes.im_status = False; 
			         else   ol_app_attributes.im_status = False; 
			    }
			    else   ol_app_attributes.im_status = False; 
		          }

		   if ((ol_app_attributes.font_group_def == NULL) ||
			(*ol_app_attributes.font_group_def == '\0'))
			   {
			   xrm_name[0]  = XrmStringToName(XtNfontGroupDef);
			   xrm_name[1] = 0;
                           xrm_class[0] = XrmStringToClass(XtNfontGroupDef);
			   xrm_class[1] = 0;
                           if (XrmQGetResource(db, xrm_name, xrm_class,                                                       &type, &value))
                             {     
                             /* must copy the value returned in "value" */
			     if (((String)value.addr != NULL) && *(value.addr))
			        {
                                ol_app_attributes.font_group_def = (char *)
						     XtMalloc(value.size + 1);
                                strcpy(ol_app_attributes.font_group_def,
                                       (String)value.addr);
			        }
				else {
			             ol_app_attributes.font_group_def = NULL;
				     }
			     }
			   }
		   }
		}
				/*
				 *	Set up Atom for front end input method
				 */
		if (ol_app_attributes.input_method != NULL &&
			ol_app_attributes.frontend_im_string == NULL)
			{
						/* check for the string (Atom) used by frontend 
						 * input methods to identify ClientMessages
						 * containing converted keyboard input
						 */
			db = (db ? db : _OlGetLocaleFileDatabase(w));
			if (!db)
		  		OlVaDisplayWarningMsg(XtDisplay(w),
					OleNfileApplic,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileApplic_msg1);
			else
				{
		   	xrm_name[0]  = XrmStringToName(XtNfrontEndString);
		   	xrm_name[1] = 0;
		   	xrm_class[0] = XrmStringToClass(XtNfrontEndString);
		   	xrm_class[1] = 0;
					/*
			 		 * get locale database from 'locale_def_file'
			 		 */
		   	if (XrmQGetResource(db, xrm_name, xrm_class, &type, &value))
			   		/* must copy the value returned in "value" */
		    		if (((String)value.addr != NULL) && *(value.addr))
		        		{
		        		ol_app_attributes.frontend_im_string = 
							XtMalloc(value.size+1);
		        		strcpy(ol_app_attributes.frontend_im_string, 
			       		(String)value.addr);
		        		}
				}
			if (ol_app_attributes.frontend_im_string == NULL)
				{
					/*
			  		 * Use the default OL frontend string
			  		 */
				ol_app_attributes.frontend_im_string = 
					XtMalloc(strlen(OL_FRONTEND_MSG)+1);
				strcpy(ol_app_attributes.frontend_im_string, 
					(String) OL_FRONTEND_MSG);	
				}
			}
							/*
					 		 *	Create the Atom for front end input methods
					 		 */
			if (ol_app_attributes.frontend_im_string!= NULL)
				ol_app_attributes.frontend_im_atom = 
					XInternAtom(XtDisplay(w), 
					ol_app_attributes.frontend_im_string,
					False);
			/*
			 * Destroy the locale database, if it was created
			 */
		if (db)
		   XrmDestroyDatabase(db);
	
	/* now announce the locale for this application */
	name = setlocale(LC_ALL, ol_app_attributes.xnllanguage);
	if (strcmp(name, ol_app_attributes.xnllanguage))
	  OlVaDisplayWarningMsg(XtDisplay(w),
				OleNsetLocale,
				OleTcategory,
				OleCOlToolkitWarning,
				OleMsetLocale_category,
				"LC_ALL");

	if (ol_app_attributes.input_lang != NULL)
		{
		name = setlocale(LC_CTYPE, ol_app_attributes.input_lang);
		if (strcmp(name, ol_app_attributes.input_lang))
		  OlVaDisplayWarningMsg(XtDisplay(w),
					OleNsetLocale,
					OleTcategory,
					OleCOlToolkitWarning,
					OleMsetLocale_category,
					"LC_ALL");
		}
	if (ol_app_attributes.display_lang != NULL)
		{
		name = setlocale(LC_MESSAGES, ol_app_attributes.display_lang);
		if (strcmp(name, ol_app_attributes.display_lang))
		  OlVaDisplayWarningMsg(XtDisplay(w),
					OleNsetLocale,
					OleTcategory,
					OleCOlToolkitWarning,
					OleMsetLocale_category,
					"LC_MESSAGES");
		}
	if (ol_app_attributes.numeric != NULL)
		{
		name = setlocale(LC_NUMERIC, ol_app_attributes.numeric);
		if (strcmp(name, ol_app_attributes.numeric))
		  OlVaDisplayWarningMsg(XtDisplay(w),
					OleNsetLocale,
					OleTcategory,
					OleCOlToolkitWarning,
					OleMsetLocale_category,
					"LC_NUMERIC");
		}
	if (ol_app_attributes.tdformat != NULL)
		{
		name = setlocale(LC_TIME, ol_app_attributes.tdformat);
		if (strcmp(name, ol_app_attributes.tdformat))
		  OlVaDisplayWarningMsg(XtDisplay(w),
					OleNsetLocale,
					OleTcategory,
					OleCOlToolkitWarning,
					OleMsetLocale_category,
					"LC_TIME");
		}

	/* do not attempt to free "name" */

	if (ol_app_attributes.input_method == NULL)
		OlImFuncs = (OlImFunctions *) NULL;
	else
		OlImFuncs = _OlSetupInputMethod(XtDisplayOfObject(w),
			     		ol_app_attributes.input_method,
			     		ol_app_attributes.input_lang,
					NULL, NULL);	

}  /* end of OlLocaleInitialize() */

static XrmDatabase
_OlGetLocaleFileDatabase OLARGLIST((w))
OLGRA (Widget,		w)
{

	char *filename;
	XrmDatabase db;

	if ((filename = XtResolvePathname(XtDisplay(w), 
						NULL, 				/*type */
						OL_LOCALE_DEF,		/*name*/
						NULL,					/*suffix*/
						NULL,					/*path*/
						NULL,					/*substitutions*/
						0, 					/*num substitutions*/
						NULL))				/*predicate*/
					== NULL)
		return(NULL);
	else
		{
		db = XrmGetFileDatabase(filename);
		XtFree(filename);
		return(db);
		}
} /* end of _OlGetLocaleFileDatabase() */

/*
 *	_OlGetLocaleAnnouncer:  get the default value for the locale
 *		announcer resource xnlLanguage.
 */
static void
_OlGetLocaleAnnouncer OLARGLIST((w, offset, value))
	OLARG(Widget, 		w)
	OLARG(int,			offset)
	OLGRA(XrmValue *,	value)
{
	static String Default = "C";

	/*
	 *	Check the LANG environment variable for an announced locale
	 */
	value->addr = (String) getenv("LANG");
	if (value->addr == NULL || !strcmp(value->addr,""))
	   {
	      /*
	       *	No announced LANG; set default to C
	       */
	      value->addr = (XtPointer) Default;	
	   }

} /* End of _OlGetLocaleAnnouncer */
#endif

/**
 ** OlGetColorTupleList()
 ** OlSetColorTupleList()
 **/

void
OlGetColorTupleList OLARGLIST((w, list, use))
	OLARG( Widget,			w )
	OLARG( OlColorTupleList **,	list )
	OLGRA( Boolean *,		use )
{
	*list = ol_app_attributes.color_tuple_list;
	*use  = ol_app_attributes.use_color_tuple_list;
	return;
}

void
OlSetColorTupleList OLARGLIST((w, list, use))
	OLARG( Widget,			w )
	OLARG( OlColorTupleList *,	list )
	OLGRA( Boolean,			use )
{
	ol_app_attributes.color_tuple_list = list;
	ol_app_attributes.use_color_tuple_list = use;
	return;
}
