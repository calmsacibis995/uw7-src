#ident	"@(#)r5extensions:lib/xtrap/XEStrMap.c	1.1"

/*****************************************************************************
Copyright 1987, 1988, 1989, 1990, 1991, 1992 by Digital Equipment Corp., 
Maynard, MA

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*****************************************************************************/
#include <stdio.h>
#include "Xlib.h"
#include "xtraplib.h"
#include "xtraplibp.h"

static char *eventName[] = {
    "", "",
    "KeyPress",         "KeyRelease",       "ButtonPress",
    "ButtonRelease",    "MotionNotify",     "EnterNotify",
    "LeaveNotify",      "FocusIn",          "FocusOut",
    "KeymapNotify",     "Expose",           "GraphicsExpose",
    "NoExpose",         "VisibilityNotify", "CreateNotify",
    "DestroyNotify",    "UnmapNotify",      "MapNotify",
    "MapRequest",       "ReparentNotify",   "ConfigureNotify",
    "ConfigureRequest", "GravityNotify",    "ResizeRequest",
    "CirculateNotify",  "CirculateRequest", "PropertyNotify",
    "SelectionClear",   "SelectionRequest", "SelectionNotify",
    "ColormapNotify",   "ClientMessage",    "MappingNotify",
    "LASTEvent",
};

static char *requestName[] = {
    "Invalid",
    "X_CreateWindow",           "X_ChangeWindowAttributes",
    "X_GetWindowAttributes",    "X_DestroyWindow",
    "X_DestroySubwindows",      "X_ChangeSaveSet",
    "X_ReparentWindow",         "X_MapWindow",
    "X_MapSubwindows",          "X_UnmapWindow",
    "X_UnmapSubwindows",        "X_ConfigureWindow",
    "X_CirculateWindow",        "X_GetGeometry",
    "X_QueryTree",              "X_InternAtom",
    "X_GetAtomName",            "X_ChangeProperty",
    "X_DeleteProperty",         "X_GetProperty",
    "X_ListProperties",         "X_SetSelectionOwner",
    "X_GetSelectionOwner",      "X_ConvertSelection",
    "X_SendEvent",              "X_GrabPointer",
    "X_UngrabPointer",          "X_GrabButton",
    "X_UngrabButton",           "X_ChangeActivePointerGrab",
    "X_GrabKeyboard",           "X_UngrabKeyboard",
    "X_GrabKey",                "X_UngrabKey",
    "X_AllowEvents",            "X_GrabServer",
    "X_UngrabServer",           "X_QueryPointer",
    "X_GetMotionEvents",        "X_TranslateCoords",
    "X_WarpPointer",            "X_SetInputFocus",
    "X_GetInputFocus",          "X_QueryKeymap",
    "X_OpenFont",               "X_CloseFont",
    "X_QueryFont",              "X_QueryTextExtents",
    "X_ListFonts",              "X_ListFontsWithInfo",
    "X_SetFontPath",            "X_GetFontPath",
    "X_CreatePixmap",           "X_FreePixmap",
    "X_CreateGC",               "X_ChangeGC",
    "X_CopyGC",                 "X_SetDashes",
    "X_SetClipRectangles",      "X_FreeGC",
    "X_ClearArea",              "X_CopyArea",
    "X_CopyPlane",              "X_PolyPoint",
    "X_PolyLine",               "X_PolySegment",
    "X_PolyRectangle",          "X_PolyArc", 
    "X_FillPoly",               "X_PolyFillRectangle",
    "X_PolyFillArc",            "X_PutImage",
    "X_GetImage",               "X_PolyText8",
    "X_PolyText16",             "X_ImageText8",
    "X_ImageText16",            "X_CreateColormap",
    "X_FreeColormap",           "X_CopyColormapAndFree",
    "X_InstallColormap",        "X_UninstallColormap",
    "X_ListInstalledColormaps", "X_AllocColor",
    "X_AllocNamedColor",        "X_AllocColorCells",
    "X_AllocColorPlanes",       "X_FreeColors",
    "X_StoreColors",            "X_StoreNamedColor",
    "X_QueryColors",            "X_LookupColor",
    "X_CreateCursor",           "X_CreateGlyphCursor",
    "X_FreeCursor",             "X_RecolorCursorSize",
    "X_QueryBestSize",          "X_QueryExtension",
    "X_ListExtensions",         "X_ChangeKeyboardMapping",
    "X_GetKeyboardMapping",     "X_ChangeKeyboardControl",
    "X_GetKeyboardControl",     "X_Bell",
    "X_ChangePointerControl",   "X_GetPointerControl",
    "X_SetScreenSaver",         "X_GetScreenSaver",
    "X_ChangeHosts",            "X_ListHosts",
    "X_SetAccessControl",       "X_SetCloseDownMode",
    "X_KillClient",             "X_RotateProperties",
    "X_ForceScreenSaver",       "X_SetPointerMapping",
    "X_GetPointerMapping",      "X_SetModifierMapping",
    "X_GetModifierMapping",     "Invalid", 
    "Invalid",                  "Invalid", 
    "Invalid",                  "Invalid",
    "Invalid",                  "Invalid",
    "X_NoOperation",
};

char **extensionName = NULL;
int numExtension;

static struct _pf_tbl {CARD32 id; char *str;} pf_tbl[] =
{
    {PF_Apollo,         "Apollo"},
    {PF_ATT,            "ATT"},
    {PF_Cray1,          "Cray1"},
    {PF_Cray2,          "Cray2"},
    {PF_DECUltrix,      "DECUltrix"},
    {PF_DECVMS,         "DECVMS"},
    {PF_DECELN,         "DECELN"},
    {PF_DECOSF1,        "DECOSF1"},
    {PF_DECVT1000,      "DECVT1000"},
    {PF_DECXTerm,       "DECXTerm"},
    {PF_HP9000s800,     "HP9000s800"},
    {PF_HP9000s300,     "HP9000s300"},
    {PF_IBMAT,          "IBMAT"},
    {PF_IBMRT,          "IBMRT"},
    {PF_IBMPS2,         "IBMPS2"},
    {PF_IBMRS,          "IBMRS"},
    {PF_MacII,          "MacII"},
    {PF_Pegasus,        "Pegasus"},
    {PF_SGI,            "SGI"},
    {PF_Sony,           "Sony"},
    {PF_Sun3,           "Sun3"},
    {PF_Sun386i,        "Sun386i"},
    {PF_SunSparc,       "SunSparc"},
    {PF_Other,          "Other"}        /* always the last one! */
};

static char unknown[] = "unknown";

#define ASize(array) (sizeof(array)/sizeof((array)[0]))

#ifdef FUNCTION_PROTOS
static INT16 _StringToID(register char *match, register char **strings,
    INT16 nstrings)
#else
static INT16 _StringToID(match,strings,nstrings)
    register char *match;
    register char **strings;
    INT16 nstrings;
#endif
{
    register INT16 id = nstrings;

    if (match && *match)
    { while ((--id >= 0L) && (strcmp(match,strings[id]) != 0L)); }
    else
    { id = -1L; }

    return(id);
}

#ifdef FUNCTION_PROTOS
INT16 XEEventStringToID(register char *string)
#else
INT16 XEEventStringToID(string)
    register char *string;
#endif
{
    return(_StringToID(string,eventName,ASize(eventName)));
}

#ifdef FUNCTION_PROTOS
INT16 XERequestStringToID(register char *string)
#else
INT16 XERequestStringToID(string)
    register char *string;
#endif
{
    return(_StringToID(string,requestName,ASize(requestName)));
}

#ifdef FUNCTION_PROTOS
CARD32 XEPlatformStringToID(register char *string)
#else
CARD32 XEPlatformStringToID(string)
    register char *string;
#endif
{
    struct _pf_tbl *ptr = &(pf_tbl[0]);
    while(ptr->id != PF_Other)
    {
        if (!strncmp(ptr->str, string, strlen(ptr->str)))
        {
            return(ptr->id);
        }
        ptr++;
    }
    return((!strncmp(ptr->str,string,strlen(ptr->str))) ? ptr->id : -1L);
}

#ifdef FUNCTION_PROTOS
char *XEEventIDToString(register CARD8 id)
#else
char *XEEventIDToString(id)
    register CARD8 id;
#endif
{
    return((id >= 0 && id < ASize(eventName)) ? eventName[id] : unknown);
}

#ifdef FUNCTION_PROTOS
char *XERequestIDToExtString(register CARD8 id)
#else
char *XERequestIDToExtString(id)
    register CARD8 id;
#endif
{
    int extid;

    extid = id - ASize(requestName);

    return((extid >=0 && extid < numExtension) ? 
        extensionName[extid] : unknown);
}


#ifdef FUNCTION_PROTOS
char *XERequestIDToString(register CARD8 id)
#else
char *XERequestIDToString(id)
    register CARD8 id;
#endif
{
    return((id >= 0 && id < ASize(requestName)) ? requestName[id] :
        XERequestIDToExtString(id));
}

#ifdef FUNCTION_PROTOS
char *XEPlatformIDToString(register CARD32 id)
#else
char *XEPlatformIDToString(id)
    register CARD32 id;
#endif
{
    struct _pf_tbl *ptr = &(pf_tbl[0]);
    while((ptr->id != PF_Other) || (id == ptr->id))
    {
        if (id == ptr->id)
        {
            return(ptr->str);
        }
        ptr++;
    }
    return(unknown);
}
