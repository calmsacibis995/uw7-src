#ifndef	NOIDENT
#ident	"@(#)oldattlib:pID.c	1.1"
#endif
/*
 pID.c (C source file)
	Acc: 575322333 Fri Mar 25 14:45:33 1988
	Mod: 575321571 Fri Mar 25 14:32:51 1988
	Sta: 575321571 Fri Mar 25 14:32:51 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	char * match_ID (type, x)	find ID that matches type
	unsigned long	type;
	ID		x[];

	char ** mask_ID (mask, x)	find all IDs contained in mask
	unsigned long	mask;
	ID		x[];

	void fprint_match (stream, type, x)	print ID that matches type
	FILE * stream;
	unsigned long	type;
	ID		x[];

	void fprint_mask (stream, mask, x)	print all IDs contained in mask
	FILE * stream;
	unsigned long	mask;
	ID		x[];

	author:
		Ross Hilbert
		AT&T 10/20/87
************************************************************************/

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "Xprint.h"
#include "pID.h"
/*
	global IDs
*/
ID _events[] =
{
	"KeyPress",			(unsigned long) KeyPress,
	"KeyRelease",			(unsigned long) KeyRelease,
	"ButtonPress",			(unsigned long) ButtonPress,
	"ButtonRelease",		(unsigned long) ButtonRelease,
	"MotionNotify",			(unsigned long) MotionNotify,
	"EnterNotify",			(unsigned long) EnterNotify,
	"LeaveNotify",			(unsigned long) LeaveNotify,
	"FocusIn",			(unsigned long) FocusIn,
	"FocusOut",			(unsigned long) FocusOut,
	"KeymapNotify",			(unsigned long) KeymapNotify,
	"Expose",			(unsigned long) Expose,
	"GraphicsExpose",		(unsigned long) GraphicsExpose,
	"NoExpose",			(unsigned long) NoExpose,
	"VisibilityNotify",		(unsigned long) VisibilityNotify,
	"CreateNotify",			(unsigned long) CreateNotify,
	"DestroyNotify",		(unsigned long) DestroyNotify,
	"UnmapNotify",			(unsigned long) UnmapNotify,
	"MapNotify",			(unsigned long) MapNotify,
	"MapRequest",			(unsigned long) MapRequest,
	"ReparentNotify",		(unsigned long) ReparentNotify,
	"ConfigureNotify",		(unsigned long) ConfigureNotify,
	"ConfigureRequest",		(unsigned long) ConfigureRequest,
	"GravityNotify",		(unsigned long) GravityNotify,
	"ResizeRequest",		(unsigned long) ResizeRequest,
	"CirculateNotify",		(unsigned long) CirculateNotify,
	"CirculateRequest",		(unsigned long) CirculateRequest,
	"PropertyNotify",		(unsigned long) PropertyNotify,
	"SelectionClear",		(unsigned long) SelectionClear,
	"SelectionRequest",		(unsigned long) SelectionRequest,
	"SelectionNotify",		(unsigned long) SelectionNotify,
	"ColormapNotify",		(unsigned long) ColormapNotify,
	"ClientMessage",		(unsigned long) ClientMessage,
	"MappingNotify",		(unsigned long) MappingNotify,
	(char *) 0,			(unsigned long) 0,
};
ID _eventmasks[] =
{
	"NoEventMask",			(unsigned long) NoEventMask,
	"KeyPressMask",			(unsigned long) KeyPressMask,
	"KeyReleaseMask",		(unsigned long) KeyReleaseMask,
	"ButtonPressMask",		(unsigned long) ButtonPressMask,
	"ButtonReleaseMask",		(unsigned long) ButtonReleaseMask,
	"EnterWindowMask",		(unsigned long) EnterWindowMask,
	"LeaveWindowMask",		(unsigned long) LeaveWindowMask,
	"PointerMotionMask",		(unsigned long) PointerMotionMask,
	"PointerMotionHintMask",	(unsigned long) PointerMotionHintMask,
	"Button1MotionMask",		(unsigned long) Button1MotionMask,
	"Button2MotionMask",		(unsigned long) Button2MotionMask,
	"Button3MotionMask",		(unsigned long) Button3MotionMask,
	"Button4MotionMask",		(unsigned long) Button4MotionMask,
	"Button5MotionMask",		(unsigned long) Button5MotionMask,
	"ButtonMotionMask",		(unsigned long) ButtonMotionMask,
	"KeymapStateMask",		(unsigned long) KeymapStateMask,
	"ExposureMask",			(unsigned long) ExposureMask,
	"VisibilityChangeMask",		(unsigned long) VisibilityChangeMask,
	"StructureNotifyMask",		(unsigned long) StructureNotifyMask,
	"ResizeRedirectMask",		(unsigned long) ResizeRedirectMask,
	"SubstructureNotifyMask",	(unsigned long) SubstructureNotifyMask,
	"SubstructureRedirectMask",	(unsigned long) SubstructureRedirectMask,
	"FocusChangeMask",		(unsigned long) FocusChangeMask,
	"PropertyChangeMask",		(unsigned long) PropertyChangeMask,
	"ColormapChangeMask",		(unsigned long) ColormapChangeMask,
	"OwnerGrabButtonMask",		(unsigned long) OwnerGrabButtonMask,
	(char *) 0,			(unsigned long) 0,
};
ID _mousekeystate[] =
{
	"ShiftMask",			(unsigned long) ShiftMask,
	"LockMask",			(unsigned long) LockMask,
	"ControlMask",			(unsigned long) ControlMask,
	"Mod1Mask",			(unsigned long) Mod1Mask,
	"Mod2Mask",			(unsigned long) Mod2Mask,
	"Mod3Mask",			(unsigned long) Mod3Mask,
	"Mod4Mask",			(unsigned long) Mod4Mask,
	"Mod5Mask",			(unsigned long) Mod5Mask,
	"Button1Mask",			(unsigned long) Button1Mask,
	"Button2Mask",			(unsigned long) Button2Mask,
	"Button3Mask",			(unsigned long) Button3Mask,
	"Button4Mask",			(unsigned long) Button4Mask,
	"Button5Mask",			(unsigned long) Button5Mask,
	(char *) 0,			(unsigned long) 0,
};
ID _mousebutton[] =
{
	"Button1",			(unsigned long) Button1,
	"Button2",			(unsigned long) Button2,
	"Button3",			(unsigned long) Button3,
	"Button4",			(unsigned long) Button4,
	"Button5",			(unsigned long) Button5,
	(char *) 0,			(unsigned long) 0,
};
ID _notifymode[] =
{
	"NotifyNormal",			(unsigned long) NotifyNormal,
	"NotifyGrab",			(unsigned long) NotifyGrab,
	"NotifyUngrab",			(unsigned long) NotifyUngrab,
	"NotifyWhileGrabbed",		(unsigned long) NotifyWhileGrabbed,
	(char *) 0,			(unsigned long) 0,
};
ID _notifydetail[] =
{
	"NotifyAncestor",		(unsigned long) NotifyAncestor,
	"NotifyVirtual",		(unsigned long) NotifyVirtual,
	"NotifyInferior",		(unsigned long) NotifyInferior,
	"NotifyNonlinear",		(unsigned long) NotifyNonlinear,
	"NotifyNonlinearVirtual",	(unsigned long) NotifyNonlinearVirtual,
	"NotifyPointer",		(unsigned long) NotifyPointer,
	"NotifyPointerRoot",		(unsigned long) NotifyPointerRoot,
	"NotifyDetailNone",		(unsigned long) NotifyDetailNone,
	(char *) 0,			(unsigned long) 0,
};
ID _visibilitystate[] =
{
	"VisibilityUnobscured",		(unsigned long) VisibilityUnobscured,
	"VisibilityPartiallyObscured",	(unsigned long) VisibilityPartiallyObscured,
	"VisibilityFullyObscured",	(unsigned long) VisibilityFullyObscured,
	(char *) 0,			(unsigned long) 0,
};
ID _configuredetail[] =
{
	"Above",			(unsigned long) Above,
	"Below",			(unsigned long) Below,
	"TopIf",			(unsigned long) TopIf,
	"BottomIf",			(unsigned long) BottomIf,
	"Opposite",			(unsigned long) Opposite,
	(char *) 0,			(unsigned long) 0,
};
ID _configuremask[] =
{
	"CWX",				(unsigned long) CWX,
	"CWY",				(unsigned long) CWY,
	"CWWidth",			(unsigned long) CWWidth,
	"CWHeight",			(unsigned long) CWHeight,
	"CWBorderWidth",		(unsigned long) CWBorderWidth,
	"CWSibling",			(unsigned long) CWSibling,
	"CWStackMode",			(unsigned long) CWStackMode,
	(char *) 0,			(unsigned long) 0,
};
ID _circulateplace[] =
{
	"PlaceOnTop",			(unsigned long) PlaceOnTop,
	"PlaceOnBottom",		(unsigned long) PlaceOnBottom,
	(char *) 0,			(unsigned long) 0,
};
ID _propertystate[] =
{
	"PropertyNewValue",		(unsigned long) PropertyNewValue,
	"PropertyDelete",		(unsigned long) PropertyDelete,
	(char *) 0,			(unsigned long) 0,
};
ID _colormapstate[] =
{
	"ColormapInstalled",		(unsigned long) ColormapInstalled,
	"ColormapUninstalled",		(unsigned long) ColormapUninstalled,
	(char *) 0,			(unsigned long) 0,
};
ID _mappingrequest[] =
{
	"MappingModifier",		(unsigned long) MappingModifier,
	"MappingKeyboard",		(unsigned long) MappingKeyboard,
	"MappingPointer",		(unsigned long) MappingPointer,
	(char *) 0,			(unsigned long) 0,
};
ID _notifyhint[] =
{
	"NotifyNormal",			(unsigned long) NotifyNormal,
	"NotifyHint",			(unsigned long) NotifyHint,
	(char *) 0,			(unsigned long) 0,
};
ID _boolean[] =
{
	"True",				(unsigned long) True,
	"False",			(unsigned long) False,
	(char *) 0,			(unsigned long) 0,
};

ID _class[] =
{
	"InputOutput",			(unsigned long) InputOutput,
	"InputOnly",			(unsigned long) InputOnly,
	(char *) 0,			(unsigned long) 0,
};
ID _bit_gravity[] =
{
	"ForgetGravity",		(unsigned long) ForgetGravity,
	"NorthWestGravity",		(unsigned long) NorthWestGravity,
	"NorthGravity",			(unsigned long) NorthGravity,
	"NorthEastGravity",		(unsigned long) NorthEastGravity,
	"WestGravity",			(unsigned long) WestGravity,
	"CenterGravity",		(unsigned long) CenterGravity,
	"EastGravity",			(unsigned long) EastGravity,
	"SouthWestGravity",		(unsigned long) SouthWestGravity,
	"SouthGravity",			(unsigned long) SouthGravity,
	"SouthEastGravity",		(unsigned long) SouthEastGravity,
	"StaticGravity",		(unsigned long) StaticGravity,
	(char *) 0,			(unsigned long) 0,
};
ID _win_gravity[] =
{
	"UnmapGravity",			(unsigned long) UnmapGravity,
	"NorthWestGravity",		(unsigned long) NorthWestGravity,
	"NorthGravity",			(unsigned long) NorthGravity,
	"NorthEastGravity",		(unsigned long) NorthEastGravity,
	"WestGravity",			(unsigned long) WestGravity,
	"CenterGravity",		(unsigned long) CenterGravity,
	"EastGravity",			(unsigned long) EastGravity,
	"SouthWestGravity",		(unsigned long) SouthWestGravity,
	"SouthGravity",			(unsigned long) SouthGravity,
	"SouthEastGravity",		(unsigned long) SouthEastGravity,
	"StaticGravity",		(unsigned long) StaticGravity,
	(char *) 0,			(unsigned long) 0,
};
ID _backing_store[] =
{
	"NotUseful",			(unsigned long) NotUseful,
	"WhenMapped",			(unsigned long) WhenMapped,
	"Always",			(unsigned long) Always,
	(char *) 0,			(unsigned long) 0,
};
ID _map_state[] =
{
	"IsUnmapped",			(unsigned long) IsUnmapped,
	"IsUnviewable",			(unsigned long) IsUnviewable,
	"IsViewable",			(unsigned long) IsViewable,
	(char *) 0,			(unsigned long) 0,
};
ID _GXfunction[] =
{
	"GXclear",			(unsigned long) GXclear,
	"GXand",			(unsigned long) GXand,
	"GXandReverse",			(unsigned long) GXandReverse,
	"GXcopy",			(unsigned long) GXcopy,
	"GXandInverted",		(unsigned long) GXandInverted,
	"GXnoop",			(unsigned long) GXnoop,
	"GXxor",			(unsigned long) GXxor,
	"GXor",				(unsigned long) GXor,
	"GXnor",			(unsigned long) GXnor,
	"GXequiv",			(unsigned long) GXequiv,
	"GXinvert",			(unsigned long) GXinvert,
	"GXorReverse",			(unsigned long) GXorReverse,
	"GXcopyInverted",		(unsigned long) GXcopyInverted,
	"GXorInverted",			(unsigned long) GXorInverted,
	"GXnand",			(unsigned long) GXnand,
	"GXset",			(unsigned long) GXset,
	(char *) 0,			(unsigned long) 0,
};
ID _line_style[] =
{
	"LineSolid",			(unsigned long) LineSolid,
	"LineOnOffDash",		(unsigned long) LineOnOffDash,
	"LineDoubleDash",		(unsigned long) LineDoubleDash,
	(char *) 0,			(unsigned long) 0,
};
ID _cap_style[] =
{
	"CapNotLast",			(unsigned long) CapNotLast,
	"CapButt",			(unsigned long) CapButt,
	"CapRound",			(unsigned long) CapRound,
	"CapProjecting",		(unsigned long) CapProjecting,
	(char *) 0,			(unsigned long) 0,
};
ID _join_style[] =
{
	"JoinMiter",			(unsigned long) JoinMiter,
	"JoinRound",			(unsigned long) JoinRound,
	"JoinBevel",			(unsigned long) JoinBevel,
	(char *) 0,			(unsigned long) 0,
};
ID _fill_style[] =
{
	"FillSolid",			(unsigned long) FillSolid,
	"FillTiled",			(unsigned long) FillTiled,
	"FillStippled",			(unsigned long) FillStippled,
	"FillOpaqueStippled",		(unsigned long) FillOpaqueStippled,
	(char *) 0,			(unsigned long) 0,
};
ID _fill_rule[] =
{
	"EvenOddRule",			(unsigned long) EvenOddRule,
	"WindingRule",			(unsigned long) WindingRule,
	(char *) 0,			(unsigned long) 0,
};
ID _arc_mode[] =
{
	"ArcChord",			(unsigned long) ArcChord,
	"ArcPieSlice",			(unsigned long) ArcPieSlice,
	(char *) 0,			(unsigned long) 0,
};
ID _subwindow_mode[] =
{
	"ClipByChildren",		(unsigned long) ClipByChildren,
	"IncludeInferiors",		(unsigned long) IncludeInferiors,
	(char *) 0,			(unsigned long) 0,
};
ID _visualclass[] =
{
	"StaticGray",			(unsigned long) StaticGray,
	"GrayScale",			(unsigned long) GrayScale,
	"StaticColor",			(unsigned long) StaticColor,
	"PseudoColor",			(unsigned long) PseudoColor,
	"TrueColor",			(unsigned long) TrueColor,
	"DirectColor",			(unsigned long) DirectColor,
	(char *) 0,			(unsigned long) 0,
};
ID _fontdirection[] =
{
	"FontLeftToRight",		(unsigned long) FontLeftToRight,
	"FontRightToLeft",		(unsigned long) FontRightToLeft,
	(char *) 0,			(unsigned long) 0,
};
ID _bytebitorder[] =
{
	"LSBFirst",			(unsigned long) LSBFirst,
	"MSBFirst",			(unsigned long) MSBFirst,
	(char *) 0,			(unsigned long) 0,
};
ID _knownproperties[] =
{
	"XA_PRIMARY",			(unsigned long) XA_PRIMARY,
	"XA_SECONDARY",			(unsigned long) XA_SECONDARY,
	"XA_ARC",			(unsigned long) XA_ARC,
	"XA_ATOM",			(unsigned long) XA_ATOM,
	"XA_BITMAP",			(unsigned long) XA_BITMAP,
	"XA_CARDINAL",			(unsigned long) XA_CARDINAL,
	"XA_COLORMAP",			(unsigned long) XA_COLORMAP,
	"XA_CURSOR",			(unsigned long) XA_CURSOR,
	"XA_CUT_BUFFER0",		(unsigned long) XA_CUT_BUFFER0,
	"XA_CUT_BUFFER1",		(unsigned long) XA_CUT_BUFFER1,
	"XA_CUT_BUFFER2",		(unsigned long) XA_CUT_BUFFER2,
	"XA_CUT_BUFFER3",		(unsigned long) XA_CUT_BUFFER3,
	"XA_CUT_BUFFER4",		(unsigned long) XA_CUT_BUFFER4,
	"XA_CUT_BUFFER5",		(unsigned long) XA_CUT_BUFFER5,
	"XA_CUT_BUFFER6",		(unsigned long) XA_CUT_BUFFER6,
	"XA_CUT_BUFFER7",		(unsigned long) XA_CUT_BUFFER7,
	"XA_DRAWABLE",			(unsigned long) XA_DRAWABLE,
	"XA_FONT",			(unsigned long) XA_FONT,
	"XA_INTEGER",			(unsigned long) XA_INTEGER,
	"XA_PIXMAP",			(unsigned long) XA_PIXMAP,
	"XA_POINT",			(unsigned long) XA_POINT,
	"XA_RECTANGLE",			(unsigned long) XA_RECTANGLE,
	"XA_RESOURCE_MANAGER",		(unsigned long) XA_RESOURCE_MANAGER,
	"XA_RGB_COLOR_MAP",		(unsigned long) XA_RGB_COLOR_MAP,
	"XA_RGB_BEST_MAP",		(unsigned long) XA_RGB_BEST_MAP,
	"XA_RGB_BLUE_MAP",		(unsigned long) XA_RGB_BLUE_MAP,
	"XA_RGB_DEFAULT_MAP",		(unsigned long) XA_RGB_DEFAULT_MAP,
	"XA_RGB_GRAY_MAP",		(unsigned long) XA_RGB_GRAY_MAP,
	"XA_RGB_GREEN_MAP",		(unsigned long) XA_RGB_GREEN_MAP,
	"XA_RGB_RED_MAP",		(unsigned long) XA_RGB_RED_MAP,
	"XA_STRING",			(unsigned long) XA_STRING,
	"XA_VISUALID",			(unsigned long) XA_VISUALID,
	"XA_WINDOW",			(unsigned long) XA_WINDOW,
	"XA_WM_COMMAND",		(unsigned long) XA_WM_COMMAND,
	"XA_WM_HINTS",			(unsigned long) XA_WM_HINTS,
	"XA_WM_CLIENT_MACHINE",		(unsigned long) XA_WM_CLIENT_MACHINE,
	"XA_WM_ICON_NAME",		(unsigned long) XA_WM_ICON_NAME,
	"XA_WM_ICON_SIZE",		(unsigned long) XA_WM_ICON_SIZE,
	"XA_WM_NAME",			(unsigned long) XA_WM_NAME,
	"XA_WM_NORMAL_HINTS",		(unsigned long) XA_WM_NORMAL_HINTS,
	"XA_WM_SIZE_HINTS",		(unsigned long) XA_WM_SIZE_HINTS,
	"XA_WM_ZOOM_HINTS",		(unsigned long) XA_WM_ZOOM_HINTS,
	"XA_MIN_SPACE",			(unsigned long) XA_MIN_SPACE,
	"XA_NORM_SPACE",		(unsigned long) XA_NORM_SPACE,
	"XA_MAX_SPACE",			(unsigned long) XA_MAX_SPACE,
	"XA_END_SPACE",			(unsigned long) XA_END_SPACE,
	"XA_SUPERSCRIPT_X",		(unsigned long) XA_SUPERSCRIPT_X,
	"XA_SUPERSCRIPT_Y",		(unsigned long) XA_SUPERSCRIPT_Y,
	"XA_SUBSCRIPT_X",		(unsigned long) XA_SUBSCRIPT_X,
	"XA_SUBSCRIPT_Y",		(unsigned long) XA_SUBSCRIPT_Y,
	"XA_UNDERLINE_POSITION",	(unsigned long) XA_UNDERLINE_POSITION,
	"XA_UNDERLINE_THICKNESS",	(unsigned long) XA_UNDERLINE_THICKNESS,
	"XA_STRIKEOUT_ASCENT",		(unsigned long) XA_STRIKEOUT_ASCENT,
	"XA_STRIKEOUT_DESCENT",		(unsigned long) XA_STRIKEOUT_DESCENT,
	"XA_ITALIC_ANGLE",		(unsigned long) XA_ITALIC_ANGLE,
	"XA_X_HEIGHT",			(unsigned long) XA_X_HEIGHT,
	"XA_QUAD_WIDTH",		(unsigned long) XA_QUAD_WIDTH,
	"XA_WEIGHT",			(unsigned long) XA_WEIGHT,
	"XA_POINT_SIZE",		(unsigned long) XA_POINT_SIZE,
	"XA_RESOLUTION",		(unsigned long) XA_RESOLUTION,
	"XA_COPYRIGHT",			(unsigned long) XA_COPYRIGHT,
	"XA_NOTICE",			(unsigned long) XA_NOTICE,
	"XA_FONT_NAME",			(unsigned long) XA_FONT_NAME,
	"XA_FAMILY_NAME",		(unsigned long) XA_FAMILY_NAME,
	"XA_FULL_NAME",			(unsigned long) XA_FULL_NAME,
	"XA_CAP_HEIGHT",		(unsigned long) XA_CAP_HEIGHT,
	"XA_WM_CLASS",			(unsigned long) XA_WM_CLASS,
	"XA_WM_TRANSIENT_FOR",		(unsigned long) XA_WM_TRANSIENT_FOR,
	(char *) 0,			(unsigned long) 0,
};

/*
	find name in ID array x that matches type
*/
char * match_ID (type, x)
unsigned long	type;
ID		x[];
{
	int i;

	for (i = 0; x[i].name && x[i].type != type; ++i)
	;
	return x[i].name;
}

#define MAX_NAMES	64			/* max entries (bits) in mask */
static char *		namebuf[MAX_NAMES];	/* buffer for returning names */
/*
	find all names in ID array x included in mask
*/
char ** mask_ID (mask, x)
unsigned long	mask;
ID		x[];
{
	int i, j;

	for (i = j = 0; x[i].name; ++i)
		if (x[i].type & mask)
			namebuf[j++] = x[i].name;
	namebuf[j] = (char *) 0;
	return j ? namebuf : (char **) 0;
}

void fprint_match (stream, type, x)
FILE * stream;
unsigned long	type;
ID		x[];
{
	char * name = match_ID (type, x);

	if (name)
		fprintf (stream, "%s", name);
	else
		fprintf (stream, "%ld", type);
}

void fprint_mask (stream, mask, x)
FILE * stream;
unsigned long	mask;
ID	x[];
{
	char ** names = mask_ID (mask, x);

	if (names)
	{
		fprintf (stream, "%s", *names);
		while (*++names)
			fprintf (stream, " | %s", *names);
	}
	else
		fprintf (stream, "0x%lx", mask);
}
