#pragma ident	"@(#)m1.2libs:Xm/Screen.c	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
ifndef OSF_v1_2_4
 * Motif Release 1.2.3
else
 * Motif Release 1.2.4
endif
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1987, 1988, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY */

#include <Xm/DragIconP.h>
#include <Xm/ScreenP.h>
#include <Xm/DisplayP.h>
#include <Xm/AtomMgr.h>
#include "RepTypeI.h"
#include "MessagesI.h"
#include <X11/Xatom.h>
#include <stdio.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#define DEFAULT_QUAD_WIDTH 10

#define RESOURCE_DEFAULT  (-1)


#ifdef I18N_MSG
#define MESSAGE1	catgets(Xm_catd,MS_Screen,MSG_SCR_1,_XmMsgScreen_0000)
#define MESSAGE2	catgets(Xm_catd,MS_Screen,MSG_SCR_2,_XmMsgScreen_0001)
#else
#define MESSAGE1	_XmMsgScreen_0000
#define MESSAGE2        _XmMsgScreen_0001
#endif


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ScreenClassPartInitialize() ;
static void ScreenClassInitialize() ;
static void GetUnitFromFont() ;
static void ScreenInitialize() ;
static Boolean ScreenSetValues() ;
static void ScreenDestroy() ;
static void ScreenInsertChild() ;
static void ScreenDeleteChild() ;

#else

static void ScreenClassPartInitialize( 
                        WidgetClass wc) ;
static void ScreenClassInitialize( void ) ;
static void GetUnitFromFont( 
                        Display *display,
                        XFontStruct *fst,
                        int *ph_unit,
                        int *pv_unit) ;
static void ScreenInitialize( 
                        Widget requested_widget,
                        Widget new_widget,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean ScreenSetValues( 
                        Widget current,
                        Widget requested,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void ScreenDestroy( 
                        Widget widget) ;
static void ScreenInsertChild( 
                        Widget wid) ;
static void ScreenDeleteChild( 
                        Widget wid) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

#define Offset(x) (XtOffsetOf(XmScreenRec, x))

static XtResource resources[] = {
    {
	XmNdarkThreshold, XmCDarkThreshold, XmRInt,
	sizeof(int), Offset(screen.darkThreshold), XmRImmediate, (XtPointer)NULL, 
    },
    {
	XmNlightThreshold, XmCLightThreshold, XmRInt,
	sizeof(int), Offset(screen.lightThreshold), XmRImmediate, (XtPointer)NULL, 
    },
    {
	XmNforegroundThreshold, XmCForegroundThreshold, XmRInt,
	sizeof(int), Offset(screen.foregroundThreshold), XmRImmediate, (XtPointer)NULL, 
    },
    {
	XmNdefaultNoneCursorIcon, XmCDefaultNoneCursorIcon, XmRWidget,
	sizeof(Widget), Offset(screen.defaultNoneCursorIcon), XmRImmediate, (XtPointer)NULL, 
    },
    {
	XmNdefaultValidCursorIcon, XmCDefaultValidCursorIcon,
	XmRWidget, sizeof(Widget), Offset(screen.defaultValidCursorIcon), XmRImmediate, (XtPointer)NULL, 
    },
    {
	XmNdefaultInvalidCursorIcon, XmCDefaultInvalidCursorIcon, XmRWidget,
	sizeof(Widget), Offset(screen.defaultInvalidCursorIcon),
	XmRImmediate, (XtPointer)NULL, 
    },   
    {
	XmNdefaultMoveCursorIcon, XmCDefaultMoveCursorIcon, XmRWidget,
	sizeof(Widget), Offset(screen.defaultMoveCursorIcon), XmRImmediate, (XtPointer)NULL, 
    },
    {
	XmNdefaultLinkCursorIcon, XmCDefaultLinkCursorIcon,
	XmRWidget, sizeof(Widget), Offset(screen.defaultLinkCursorIcon), XmRImmediate, (XtPointer)NULL, 
    },
    {
	XmNdefaultCopyCursorIcon, XmCDefaultCopyCursorIcon, XmRWidget,
	sizeof(Widget), Offset(screen.defaultCopyCursorIcon),
	XmRImmediate, (XtPointer)NULL, 
    },   
    {
	XmNdefaultSourceCursorIcon, XmCDefaultSourceCursorIcon, XmRWidget,
	sizeof(Widget), Offset(screen.defaultSourceCursorIcon),
	XmRImmediate, (XtPointer)NULL, 
    },   
    {
	XmNmenuCursor, XmCCursor, XmRCursor,
	sizeof(Cursor), Offset(screen.menuCursor),
	XmRString, "arrow",
    },
    {
	XmNunpostBehavior, XmCUnpostBehavior, XmRUnpostBehavior,
	sizeof(unsigned char), Offset(screen.unpostBehavior),
	XmRImmediate, (XtPointer)XmUNPOST_AND_REPLAY,
    },
    {
	XmNfont, XmCFont, XmRFontStruct,
	sizeof(XFontStruct *), Offset(screen.font_struct),
	XmRString, "Fixed",
    },
    {
	XmNhorizontalFontUnit, XmCHorizontalFontUnit, XmRInt,
	sizeof(int), Offset(screen.h_unit),
	XmRImmediate, (XtPointer) RESOURCE_DEFAULT,
    },
    {
	XmNverticalFontUnit, XmCVerticalFontUnit, XmRInt,
	sizeof(int), Offset(screen.v_unit),
	XmRImmediate, (XtPointer) RESOURCE_DEFAULT,
    },
    {
      XmNmoveOpaque, XmCMoveOpaque, XmRBoolean,
      sizeof(unsigned char), Offset(screen.moveOpaque),
      XmRImmediate, (XtPointer) False,
    },
};

static char fakeIcon;	/* placeholder for icon deletions */

/***************************************************************************
 *
 * Class Record
 *
 ***************************************************************************/

static XmBaseClassExtRec baseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    NULL,				/* InitializePrehook	*/
    NULL,				/* SetValuesPrehook	*/
    NULL,				/* InitializePosthook	*/
    NULL,				/* SetValuesPosthook	*/
    NULL,				/* secondaryObjectClass	*/
    NULL,				/* secondaryCreate	*/
    NULL,          		/* getSecRes data	*/
    { 0 },     			/* fastSubclass flags	*/
    NULL,				/* getValuesPrehook	*/
    NULL,				/* getValuesPosthook	*/
    NULL,               /* classPartInitPrehook */
    NULL,               /* classPartInitPosthook*/
    NULL,               /* ext_resources        */
    NULL,               /* compiled_ext_resources*/
    0,                  /* num_ext_resources    */
    FALSE,              /* use_sub_resources    */
    NULL,               /* widgetNavigable      */
    NULL                /* focusChange          */
};


externaldef(xmscreenclassrec)
XmScreenClassRec xmScreenClassRec = {
    {	
	(WidgetClass) &coreClassRec,	/* superclass		*/   
	"XmScreen",			/* class_name 		*/   
	sizeof(XmScreenRec), 		/* size 		*/   
	ScreenClassInitialize,		/* Class Initializer 	*/   
	ScreenClassPartInitialize,		/* class_part_init 	*/ 
	FALSE, 				/* Class init'ed ? 	*/   
	ScreenInitialize,		/* initialize         	*/   
	NULL, 				/* initialize_notify    */ 
	NULL,	 			/* realize            	*/   
	NULL,	 			/* actions            	*/   
	0,				/* num_actions        	*/   
	resources,			/* resources          	*/   
	XtNumber(resources),		/* resource_count     	*/   
	NULLQUARK, 			/* xrm_class          	*/   
	FALSE, 				/* compress_motion    	*/   
	FALSE, 				/* compress_exposure  	*/   
	FALSE, 				/* compress_enterleave	*/   
	FALSE, 				/* visible_interest   	*/   
	ScreenDestroy,			/* destroy            	*/   
	NULL,		 		/* resize             	*/   
	NULL, 				/* expose             	*/   
	ScreenSetValues, 		/* set_values         	*/   
	NULL, 				/* set_values_hook      */ 
	NULL,			 	/* set_values_almost    */ 
	NULL,				/* get_values_hook      */ 
	NULL, 				/* accept_focus       	*/   
	XtVersion, 			/* intrinsics version 	*/   
	NULL, 				/* callback offsets   	*/   
	NULL,				/* tm_table           	*/   
	NULL, 				/* query_geometry       */ 
	NULL, 				/* screen_accelerator  */ 
	(XtPointer)&baseClassExtRec,	/* extension            */ 
    },	
    {					/* desktop		*/
	NULL,				/* child_class		*/
	ScreenInsertChild,		/* insert_child		*/
	ScreenDeleteChild,		/* delete_child		*/
	NULL,				/* extension		*/
    },
    {	
	NULL,
    },
};

externaldef(xmscreenclass) WidgetClass 
      xmScreenClass = (WidgetClass) (&xmScreenClassRec);



static void 
#ifdef _NO_PROTO
ScreenClassPartInitialize(wc)
	WidgetClass wc;
#else
ScreenClassPartInitialize( 
	WidgetClass wc )
#endif /* _NO_PROTO */
{
	_XmFastSubclassInit(wc, XmSCREEN_BIT);
}    

externaldef(xmscreenquark) XrmQuark	_XmInvalidCursorIconQuark ;
externaldef(xmscreenquark) XrmQuark	_XmValidCursorIconQuark ;
externaldef(xmscreenquark) XrmQuark	_XmNoneCursorIconQuark ;
externaldef(xmscreenquark) XrmQuark	_XmDefaultDragIconQuark ;
externaldef(xmscreenquark) XrmQuark	_XmMoveCursorIconQuark ;
externaldef(xmscreenquark) XrmQuark	_XmCopyCursorIconQuark ;
externaldef(xmscreenquark) XrmQuark	_XmLinkCursorIconQuark ;

static void 
#ifdef _NO_PROTO
ScreenClassInitialize()
#else
ScreenClassInitialize( void )
#endif /* _NO_PROTO */
{
	baseClassExtRec.record_type = XmQmotif;

    _XmInvalidCursorIconQuark = XrmStringToQuark("defaultInvalidCursorIcon");
    _XmValidCursorIconQuark = XrmStringToQuark("defaultValidCursorIcon");
    _XmNoneCursorIconQuark = XrmStringToQuark("defaultNoneCursorIcon");
    _XmDefaultDragIconQuark = XrmStringToQuark("defaultSourceCursorIcon"); 
    _XmMoveCursorIconQuark = XrmStringToQuark("defaultMoveCursorIcon"); 
    _XmCopyCursorIconQuark = XrmStringToQuark("defaultCopyCursorIcon"); 
    _XmLinkCursorIconQuark = XrmStringToQuark("defaultLinkCursorIcon"); 
}    


/************************************************************************
 *
 *  GetUnitFromFont
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetUnitFromFont(display, fst, ph_unit, pv_unit)
        Display * display ;
        XFontStruct * fst ;
        int * ph_unit ;
        int * pv_unit ;
#else
GetUnitFromFont(
	Display * display,
	XFontStruct * fst,
	int * ph_unit,
	int * pv_unit)
#endif /* _NO_PROTO */
{
    unsigned long pixel_s, avg_w, point_s, resolution_y;
    Atom xa_average_width, xa_pixel_size, xa_resolution_y;
    unsigned long font_unit_return;

    if (fst) {
      xa_average_width = XmInternAtom(display, "AVERAGE_WIDTH",TRUE);
      xa_pixel_size = XmInternAtom(display, "PIXEL_SIZE",TRUE);
      xa_resolution_y = XmInternAtom(display, "RESOLUTION_Y",TRUE);

              /* Horizontal units */
      if (ph_unit) {
	  if (xa_average_width && XGetFontProperty(fst, xa_average_width,
						   (unsigned long *) &avg_w)) {
	      *ph_unit = ((float) (avg_w / 10) + 0.5) ;
	  } else if (XGetFontProperty (fst, XA_QUAD_WIDTH, &font_unit_return)){
	      *ph_unit = font_unit_return ;
	  } else {
	      *ph_unit =  ((fst->min_bounds.width + fst-> max_bounds.width)
			  / 2.3) + 0.5;
	  }
      }

              /* Vertical units */
      if (pv_unit) {
	  if (XGetFontProperty(fst, xa_pixel_size, 
			       (unsigned long *) &pixel_s)) {
	      *pv_unit = (((float) pixel_s) / 1.8) + 0.5;
	  } else if ((XGetFontProperty(fst, XA_POINT_SIZE,
				       (unsigned long *) &point_s)) &&
		     (XGetFontProperty(fst, xa_resolution_y, 
				       (unsigned long *) &resolution_y))) {
	      float ps, ry, tmp;

	      ps = point_s;
	      ry = resolution_y;

	      tmp = (ps * ry) / 1400.0;

	      *pv_unit = (int) (tmp + 0.5) ;
	  } else {
	      *pv_unit =
		  ((float) (fst->max_bounds.ascent + fst->max_bounds.descent)
		   / 2.2) + 0.5;
	  }
      }

    } else {  /* no X fontstruct */
      if (ph_unit) *ph_unit = DEFAULT_QUAD_WIDTH ;
      if (pv_unit) *pv_unit = DEFAULT_QUAD_WIDTH ;
    }

}


/************************************************************************
 *
 *  ScreenInitialize
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ScreenInitialize( requested_widget, new_widget, args, num_args )
        Widget requested_widget ;
        Widget new_widget ;
        ArgList args ;
        Cardinal *num_args ;
#else
ScreenInitialize(
        Widget requested_widget,
        Widget new_widget,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmScreen		xmScreen = (XmScreen)new_widget;
    Display * display = XtDisplay(new_widget);

    xmScreen->screen.screenInfo = NULL;

    XQueryBestCursor(display,
		     RootWindowOfScreen(XtScreen(xmScreen)),
		     1024, 1024,
		     &xmScreen->screen.maxCursorWidth, 
		     &xmScreen->screen.maxCursorHeight);

    xmScreen->screen.nullCursor = None;
    xmScreen->screen.cursorCache = NULL;
    xmScreen->screen.scratchPixmaps = NULL;
    xmScreen->screen.xmStateCursorIcon = NULL;
    xmScreen->screen.xmMoveCursorIcon = NULL;
    xmScreen->screen.xmCopyCursorIcon = NULL;
    xmScreen->screen.xmLinkCursorIcon = NULL;
    xmScreen->screen.xmSourceCursorIcon = NULL;
    xmScreen->screen.mwmPresent = XmIsMotifWMRunning( new_widget) ;
    xmScreen->screen.numReparented = 0;
    xmScreen->desktop.num_children = 0;
    xmScreen->desktop.children = NULL;
    xmScreen->desktop.num_slots = 0;

    if(!XmRepTypeValidValue(XmRID_UNPOST_BEHAVIOR,
                          xmScreen->screen.unpostBehavior,
                          new_widget)) {
      xmScreen->screen.unpostBehavior = XmUNPOST_AND_REPLAY;
    }

    /* font unit stuff, priority to actual unit values, if they haven't
     been set yet, compute from the font, otherwise, stay unchanged */

    if (xmScreen->screen.h_unit == RESOURCE_DEFAULT) 
	GetUnitFromFont(display, xmScreen->screen.font_struct, 
			&xmScreen->screen.h_unit, NULL);

    if (xmScreen->screen.v_unit == RESOURCE_DEFAULT)
	GetUnitFromFont(display, xmScreen->screen.font_struct, 
			NULL, &xmScreen->screen.v_unit);

    xmScreen->screen.imageGC = (GC) NULL;
    xmScreen->screen.imageGCDepth = 0;
    xmScreen->screen.imageForeground = 0;
    xmScreen->screen.imageBackground = 0;
    xmScreen->screen.screenInfo = (XtPointer) XtNew(XmScreenInfo);

    ((XmScreenInfo *)(xmScreen->screen.screenInfo))->menu_state = 
		(XtPointer)NULL;
    ((XmScreenInfo *)(xmScreen->screen.screenInfo))->destroyCallbackAdded = 
		False;
}

/************************************************************************
 *
 *  ScreenSetValues
 *
 ************************************************************************/
/* ARGSUSED */
static Boolean
#ifdef _NO_PROTO
ScreenSetValues( current, requested, new_w, args, num_args )
        Widget current ;
        Widget requested ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
ScreenSetValues(
        Widget current,
        Widget requested,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmScreen		newScr = (XmScreen)new_w;
    XmScreen		oldScr = (XmScreen)current;
    Display * display = XtDisplay( new_w);

    if(!XmRepTypeValidValue(XmRID_UNPOST_BEHAVIOR,
                          newScr->screen.unpostBehavior, new_w)) {
      newScr->screen.unpostBehavior = XmUNPOST_AND_REPLAY;
    }

    /*
     *  If we are setting a default cursor icon, verify that
     *  it has the same screen as the new XmScreen.
     */

    /* defaultNoneCursorIcon */

    if (newScr->screen.defaultNoneCursorIcon != 
	    oldScr->screen.defaultNoneCursorIcon &&
	newScr->screen.defaultNoneCursorIcon != NULL &&
	XtScreenOfObject (XtParent (newScr->screen.defaultNoneCursorIcon)) !=
	     XtScreen (newScr)) {

	_XmWarning( (Widget) new_w, MESSAGE1);
	newScr->screen.defaultNoneCursorIcon =
	    oldScr->screen.defaultNoneCursorIcon;
    }

    /* defaultValidCursorIcon */

    if (newScr->screen.defaultValidCursorIcon != 
	    oldScr->screen.defaultValidCursorIcon &&
	newScr->screen.defaultValidCursorIcon != NULL &&
	XtScreenOfObject (XtParent (newScr->screen.defaultValidCursorIcon)) !=
	     XtScreen (newScr)) {

	_XmWarning( (Widget) new_w, MESSAGE1);
	newScr->screen.defaultValidCursorIcon =
	    oldScr->screen.defaultValidCursorIcon;
    }

    /* defaultInvalidCursorIcon */

    if (newScr->screen.defaultInvalidCursorIcon != 
	    oldScr->screen.defaultInvalidCursorIcon &&
	newScr->screen.defaultInvalidCursorIcon != NULL &&
	XtScreenOfObject (XtParent (newScr->screen.defaultInvalidCursorIcon)) !=
	     XtScreen (newScr)) {

	_XmWarning( (Widget) new_w, MESSAGE1);
	newScr->screen.defaultInvalidCursorIcon =
	    oldScr->screen.defaultInvalidCursorIcon;
    }

    /* defaultMoveCursorIcon */

    if (newScr->screen.defaultMoveCursorIcon != 
	    oldScr->screen.defaultMoveCursorIcon &&
	newScr->screen.defaultMoveCursorIcon != NULL &&
	XtScreenOfObject (XtParent (newScr->screen.defaultMoveCursorIcon)) !=
	     XtScreen (newScr)) {

	_XmWarning( (Widget) new_w, MESSAGE1);
	newScr->screen.defaultMoveCursorIcon =
	    oldScr->screen.defaultMoveCursorIcon;
    }

    /* defaultCopyCursorIcon */

    if (newScr->screen.defaultCopyCursorIcon != 
	    oldScr->screen.defaultCopyCursorIcon &&
	newScr->screen.defaultCopyCursorIcon != NULL &&
	XtScreenOfObject (XtParent (newScr->screen.defaultCopyCursorIcon)) !=
	     XtScreen (newScr)) {

	_XmWarning( (Widget) new_w, MESSAGE1);
	newScr->screen.defaultCopyCursorIcon =
	    oldScr->screen.defaultCopyCursorIcon;
    }

    /* defaultLinkCursorIcon */

    if (newScr->screen.defaultLinkCursorIcon != 
	    oldScr->screen.defaultLinkCursorIcon &&
	newScr->screen.defaultLinkCursorIcon != NULL &&
	XtScreenOfObject (XtParent (newScr->screen.defaultLinkCursorIcon)) !=
	     XtScreen (newScr)) {

	_XmWarning( (Widget) new_w, MESSAGE1);
	newScr->screen.defaultLinkCursorIcon =
	    oldScr->screen.defaultLinkCursorIcon;
    }

    /* defaultSourceCursorIcon */

    if (newScr->screen.defaultSourceCursorIcon != 
	    oldScr->screen.defaultSourceCursorIcon &&
	newScr->screen.defaultSourceCursorIcon != NULL &&
	XtScreenOfObject (XtParent (newScr->screen.defaultSourceCursorIcon)) !=
	     XtScreen (newScr)) {

	_XmWarning( (Widget) new_w, MESSAGE1);
	newScr->screen.defaultSourceCursorIcon =
	    oldScr->screen.defaultSourceCursorIcon;
    }

    /* font unit stuff, priority to actual unit values, if the
       font has changed but not the unit values, report the change,
       otherwise, use the unit value - i.e do nothing */

    if (newScr->screen.font_struct->fid != 
	oldScr->screen.font_struct->fid) {
	
	if (newScr->screen.h_unit == oldScr->screen.h_unit) {
	    GetUnitFromFont(display, newScr->screen.font_struct, 
			    &newScr->screen.h_unit, NULL);
	}

	if (newScr->screen.v_unit == oldScr->screen.v_unit) {
	    GetUnitFromFont(display, newScr->screen.font_struct, 
			    NULL, &newScr->screen.v_unit);
	}
    }    
	
    return FALSE ;
}

/************************************************************************
 *
 *  _XmScreenRemoveFromCursorCache
 *
 *  Mark any cursor cache reference to the specified icon as deallocated.
 ************************************************************************/

void 
#ifdef _NO_PROTO
_XmScreenRemoveFromCursorCache( icon )
    XmDragIconObject	icon ;
#else
_XmScreenRemoveFromCursorCache(
    XmDragIconObject	icon )
#endif /* _NO_PROTO */
{
    XmScreen		xmScreen =
			    (XmScreen) XmGetXmScreen(XtScreen(icon));
    XmDragCursorCache  cache = xmScreen->screen.cursorCache;
    XmDragCursorCache  previous = xmScreen->screen.cursorCache;
    XmDragCursorCache  next;
    Boolean done;

    while (cache) {
        done = False;
	if (cache->sourceIcon == icon) {
	    cache->sourceIcon = (XmDragIconObject) &fakeIcon;
	    done = True;
	}
	if (cache->stateIcon == icon) {
	    cache->stateIcon = (XmDragIconObject) &fakeIcon;
	    done = True;
	}
	if (cache->opIcon == icon) {
	    cache->opIcon = (XmDragIconObject) &fakeIcon;
	    done = True;
	}
	if ( done && cache->cursor )
	  {
	    XFreeCursor (XtDisplay(icon), cache->cursor);
#ifndef OSF_v1_2_4
	    cache->cursor = NULL;
#else /* OSF_v1_2_4 */
	    cache->cursor = None;
#endif /* OSF_v1_2_4 */
	  }
	next = cache->next;
	if ((cache->sourceIcon == (XmDragIconObject) &fakeIcon ||
	     cache->stateIcon  == (XmDragIconObject) &fakeIcon ||
	     cache->opIcon     == (XmDragIconObject) &fakeIcon) &&
#ifndef OSF_v1_2_4
	     cache->cursor     == NULL )
#else /* OSF_v1_2_4 */
	     cache->cursor     == None )
#endif /* OSF_v1_2_4 */
	  {
	    if ( xmScreen->screen.cursorCache == cache )
		xmScreen->screen.cursorCache = cache->next;
	    else
		previous->next = cache->next;
	    XtFree ((char*)cache);
	  }
	else
	    previous = cache;
	cache = next;
    }
}

/************************************************************************
 *
 *  ScreenDestroy
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ScreenDestroy( widget )
        Widget widget ;
#else
ScreenDestroy(
        Widget widget )
#endif /* _NO_PROTO */
{
    XmScreen		xmScreen = (XmScreen)widget;
    XmDragCursorCache 	prevCache, cache;
    XmScratchPixmap     prevPix, pix;

#ifdef OSF_v1_2_4
    /* Begin fixing OSF CR 5626 */
    /* destroy any default icons created by Xm */

    if (xmScreen->screen.xmStateCursorIcon != NULL) {
	int i;
	XmDragIconObject * icon_list = (XmDragIconObject *)
		xmScreen->screen.xmStateCursorIcon;
	for (i = 0; i < 3; i++)  {
		if (icon_list[i])
			_XmDestroyDefaultDragIcon (icon_list[i]);
		else
			break;
	}
	XtFree((char *)icon_list);
	xmScreen->screen.xmStateCursorIcon = NULL;
    }
    if (xmScreen->screen.xmMoveCursorIcon != NULL) {
	_XmDestroyDefaultDragIcon (xmScreen->screen.xmMoveCursorIcon);
    }
    if (xmScreen->screen.xmCopyCursorIcon != NULL) {
	_XmDestroyDefaultDragIcon (xmScreen->screen.xmCopyCursorIcon);
    }
    if (xmScreen->screen.xmLinkCursorIcon != NULL) {
	_XmDestroyDefaultDragIcon (xmScreen->screen.xmLinkCursorIcon);
    }
    if (xmScreen->screen.xmSourceCursorIcon != NULL) {
	_XmDestroyDefaultDragIcon (xmScreen->screen.xmSourceCursorIcon);
    }

#endif /* OSF_v1_2_4 */
    XtFree((char *) xmScreen->desktop.children);

    /* free the cursorCache and the pixmapCache */
    cache = xmScreen->screen.cursorCache;
    while(cache) {
	prevCache = cache;
	if (cache->cursor)
	  XFreeCursor(XtDisplay(xmScreen), cache->cursor);
	cache = cache->next;
	XtFree((char *)prevCache);
    }

    pix = xmScreen->screen.scratchPixmaps;
    while(pix) {
	prevPix = pix;
	if (pix->pixmap)
	    XFreePixmap(XtDisplay(xmScreen), pix->pixmap);
	pix = pix->next;
	XtFree((char *)prevPix);
    }
#ifndef OSF_v1_2_4

    /* destroy any default icons created by Xm */

    if (xmScreen->screen.xmStateCursorIcon != NULL) {
	int i;
	XmDragIconObject * icon_list = (XmDragIconObject *)
		xmScreen->screen.xmStateCursorIcon;
	for (i = 0; i < 3; i++)  {
		if (icon_list[i])
			_XmDestroyDefaultDragIcon (icon_list[i]);
		else
			break;
	}
	XtFree((char *)icon_list);
	xmScreen->screen.xmStateCursorIcon = NULL;
    }
    if (xmScreen->screen.xmMoveCursorIcon != NULL) {
	_XmDestroyDefaultDragIcon (xmScreen->screen.xmMoveCursorIcon);
    }
    if (xmScreen->screen.xmCopyCursorIcon != NULL) {
	_XmDestroyDefaultDragIcon (xmScreen->screen.xmCopyCursorIcon);
    }
    if (xmScreen->screen.xmLinkCursorIcon != NULL) {
	_XmDestroyDefaultDragIcon (xmScreen->screen.xmLinkCursorIcon);
    }
    if (xmScreen->screen.xmSourceCursorIcon != NULL) {
	_XmDestroyDefaultDragIcon (xmScreen->screen.xmSourceCursorIcon);
    }
#else /* OSF_v1_2_4 */
    /* End fixing OSF CR 5626 */
#endif /* OSF_v1_2_4 */

    if (xmScreen->screen.imageGC != NULL)
	XFreeGC (XtDisplay(xmScreen), xmScreen->screen.imageGC);
    XtFree((char *) xmScreen->screen.screenInfo);
}

static void 
#ifdef _NO_PROTO
ScreenInsertChild( wid )
        Widget wid ;
#else
ScreenInsertChild(
        Widget wid )
#endif /* _NO_PROTO */
{
    register Cardinal	     	position;
    register Cardinal        	i;
    register XmScreen 		cw;
    register WidgetList      	children;
    XmDesktopObject		w = (XmDesktopObject)wid;
    cw = (XmScreen) w->desktop.parent;
    children = cw->desktop.children;
    
    position = cw->desktop.num_children;
    
    if (cw->desktop.num_children == cw->desktop.num_slots) {
	/* Allocate more space */
	cw->desktop.num_slots +=  (cw->desktop.num_slots / 2) + 2;
	cw->desktop.children = children = 
	  (WidgetList) XtRealloc((char *) children,
				 (unsigned) (cw->desktop.num_slots) * sizeof(Widget));
    }
    /* Ripple children up one space from "position" */
    for (i = cw->desktop.num_children; i > position; i--) {
	children[i] = children[i-1];
    }
    children[position] = (Widget)w;
    cw->desktop.num_children++;
}

static void 
#ifdef _NO_PROTO
ScreenDeleteChild( wid )
        Widget wid ;
#else
ScreenDeleteChild(
        Widget wid )
#endif /* _NO_PROTO */
{
    register Cardinal	     	position;
    register Cardinal	     	i;
    register XmScreen 		cw;
    XmDesktopObject		w = (XmDesktopObject)wid;
    
    cw = (XmScreen) w->desktop.parent;
    
    for (position = 0; position < cw->desktop.num_children; position++) {
        if (cw->desktop.children[position] == (Widget)w) {
	    break;
	}
    }
    if (position == cw->desktop.num_children) return;
    
    /* Ripple children down one space from "position" */
    cw->desktop.num_children--;
    for (i = position; i < cw->desktop.num_children; i++) {
        cw->desktop.children[i] = cw->desktop.children[i+1];
    }
}

/************************************************************************
 *
 *  _XmScreenGetOperationIcon ()
 *
 *  Returns one of the three XmScreen operation cursor types. These aren't
 *  created ahead of time in order to let the client specify its own.
 *  If they haven't by now (a drag is in process) then we create our
 *  own. The name of the OperatonIcon can cause the built-in cursor data
 *  to get loaded in (if not specified in the resource file).
 ************************************************************************/

XmDragIconObject 
#ifdef _NO_PROTO
_XmScreenGetOperationIcon( w, operation )
        Widget w ;
        unsigned char operation ;
#else
_XmScreenGetOperationIcon(
        Widget w,
#if NeedWidePrototypes
        unsigned int operation )
#else
        unsigned char operation )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmScreen		xmScreen = (XmScreen) XmGetXmScreen(XtScreenOfObject(w));
    XrmQuark		nameQuark = NULLQUARK;
    XmDragIconObject	*ptr = NULL;
    XmDragIconObject	*pt2 = NULL;

    switch ((int) operation) {
	case XmDROP_MOVE:
	    ptr = &xmScreen->screen.defaultMoveCursorIcon;
	    pt2 = &xmScreen->screen.xmMoveCursorIcon;
	    nameQuark = _XmMoveCursorIconQuark;
	    break;

	case XmDROP_COPY:
	    ptr = &xmScreen->screen.defaultCopyCursorIcon;
	    pt2 = &xmScreen->screen.xmCopyCursorIcon;
	    nameQuark = _XmCopyCursorIconQuark;
	    break;

	case XmDROP_LINK:
	    ptr = &xmScreen->screen.defaultLinkCursorIcon;
	    pt2 = &xmScreen->screen.xmLinkCursorIcon;
	    nameQuark = _XmLinkCursorIconQuark;
	    break;

	default:
	    return (NULL);
    }
    if (*ptr == NULL) {
	if (*pt2 == NULL) {
	    *pt2 = (XmDragIconObject)
	        XmCreateDragIcon ((Widget) xmScreen,
			          XrmQuarkToString(nameQuark), NULL, 0);
	}
	*ptr = *pt2;
    }
    return *ptr;
}

/************************************************************************
 *
 *  _XmScreenGetStateIcon ()
 *
 *  Returns one of the three XmScreen state cursor types. These aren't
 *  created ahead of time in order to let the client specify its own.
 *  If they haven't by now (a drag is in process) then we create our
 *  own. The name of the StateIcon can cause the built-in cursor data
 *  to get loaded in (if not specified in the resource file).
 *  The default state cursors are the same XmDragIcon object.
 ************************************************************************/

XmDragIconObject 
#ifdef _NO_PROTO
_XmScreenGetStateIcon( w, state )
        Widget w ;
        unsigned char state ;
#else
_XmScreenGetStateIcon(
        Widget w,
#if NeedWidePrototypes
        unsigned int state )
#else
        unsigned char state )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmScreen		xmScreen = (XmScreen) XmGetXmScreen(XtScreenOfObject(w));
    XrmQuark		nameQuark = NULLQUARK;
    XmDragIconObject	icon = NULL;
    XmDragIconObject *	icon_list;
    int			i;

    switch(state) {
	default:
	case XmNO_DROP_SITE:
	    icon = xmScreen->screen.defaultNoneCursorIcon;
	    nameQuark = _XmNoneCursorIconQuark;
	    break;

	case XmVALID_DROP_SITE:
	    icon = xmScreen->screen.defaultValidCursorIcon;
	    nameQuark = _XmValidCursorIconQuark;
	    break;

	case XmINVALID_DROP_SITE:
	    icon = xmScreen->screen.defaultInvalidCursorIcon;
	    nameQuark = _XmInvalidCursorIconQuark;
	    break;
    }
    if (icon == NULL) {

	/*
	 *  We need to create the default state icons when they are NULL.
	 */

	icon = (XmDragIconObject) XmCreateDragIcon ((Widget) xmScreen,
			          XrmQuarkToString(nameQuark),
				  NULL, 0);

	/*  Must keep track of the icons that are created so that they can
	    be destroyed.  Re-use the stateCursorIcon field as a list. (Gross
	    but I can't change the instance part */
	if (xmScreen->screen.xmStateCursorIcon == NULL)  {
		icon_list = (XmDragIconObject *)
			XtCalloc(3, sizeof(XmDragIconObject));
		xmScreen->screen.xmStateCursorIcon = (XmDragIconObject)icon_list;
	}
	else
		icon_list = (XmDragIconObject *) xmScreen->screen.xmStateCursorIcon;
	
	for (i = 0; i < 3; i++)
		if (!icon_list[i])
			icon_list[i] = icon;

        switch(state) {
	    default:
	    case XmNO_DROP_SITE:
	        xmScreen->screen.defaultNoneCursorIcon = icon;
	        break;
	    case XmVALID_DROP_SITE:
	        xmScreen->screen.defaultValidCursorIcon = icon;
	        break;
	    case XmINVALID_DROP_SITE:
	        xmScreen->screen.defaultInvalidCursorIcon = icon;
	        break;
	}

    }

    return (icon);
}

/************************************************************************
 *
 *  _XmScreenGetSourceIcon ()
 *
 *  Returns the XmScreen source cursor types.  This isn't created ahead of
 *  time in order to let the client specify its own.  If it hasn't by now
 *  (a drag is in process) then we create our own.
 ************************************************************************/

XmDragIconObject 
#ifdef _NO_PROTO
_XmScreenGetSourceIcon( w )
        Widget w ;
#else
_XmScreenGetSourceIcon(
        Widget w )
#endif /* _NO_PROTO */
{
    XmScreen	xmScreen = (XmScreen) XmGetXmScreen(XtScreenOfObject(w));

    if (xmScreen->screen.defaultSourceCursorIcon == NULL) {

	if (xmScreen->screen.xmSourceCursorIcon == NULL) {
	    xmScreen->screen.xmSourceCursorIcon = (XmDragIconObject)
	        XmCreateDragIcon ((Widget) xmScreen,
			          XrmQuarkToString(_XmDefaultDragIconQuark),
			          NULL, 0);
	}
	xmScreen->screen.defaultSourceCursorIcon = 
	    xmScreen->screen.xmSourceCursorIcon;
    }
    return xmScreen->screen.defaultSourceCursorIcon;
}

/*********************************************************************
 *
 *  XmGetXmScreen
 *
 *********************************************************************/

/* ARGSUSED */
Widget 
#ifdef _NO_PROTO
XmGetXmScreen( screen )
        Screen *screen ;
#else
XmGetXmScreen(
        Screen *screen )
#endif /* _NO_PROTO */
{ 
    XmDisplay	xmDisplay;
	WidgetList	children;
	int	num_children;
	Arg args[5];
	int i;
	Screen *scr;
	char name[25];


    if ((xmDisplay = (XmDisplay) XmGetXmDisplay(DisplayOfScreen(screen))) == NULL)
	{
		_XmWarning(NULL, MESSAGE2);
		return(NULL);
	}

	children = xmDisplay->composite.children;
	num_children = xmDisplay->composite.num_children;

	for (i=0; i < num_children; i++)
	{
		Widget child = children[i];
		if ((XmIsScreen(child)) &&
			(screen == XtScreen(child)))
			return(child);
	}

	/* Not found; have to do an implied creation */
	for (scr = ScreenOfDisplay(XtDisplay(xmDisplay), i);
		i < ScreenCount(XtDisplay(xmDisplay));
		i++, scr = ScreenOfDisplay(XtDisplay(xmDisplay), i))
	{
		if (scr == screen)
			break;
	}

	sprintf(name, "screen%d", i);

	i = 0;
	XtSetArg(args[i], XmNscreen, screen); i++;
	return(XtCreateWidget(name, xmScreenClass, (Widget)xmDisplay,
		args, i));
}

/************************************************************************
 *
 *  _XmAllocScratchPixmap
 *
 ************************************************************************/

Pixmap 
#ifdef _NO_PROTO
_XmAllocScratchPixmap( xmScreen, depth, width, height )
        XmScreen xmScreen ;
        Cardinal depth ;
        Dimension width ;
        Dimension height ;
#else
_XmAllocScratchPixmap(
        XmScreen xmScreen,
#if NeedWidePrototypes
        unsigned int depth,
        int width,
        int height )
#else
        Cardinal depth,
        Dimension width,
        Dimension height )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmScratchPixmap	*scratchPtr = &xmScreen->screen.scratchPixmaps;
    XmScratchPixmap	scratchPixmap = NULL;
    XmScratchPixmap	ptr = *scratchPtr;

    /*
     * This could be tuned such that we only accept partial matches
     * which are within some number of pixels from the desired area.
     * Which is more expensive, a bad match or another call to
     * XCreatePixmap?
     *
     * The match must be exact.  This code is primarily used in
     * Drag and Drop,  and the pixmaps returned will be used
     * to create cursors at times.  The X cursor code only knows
     * the size of the real pixmap,  and has no other parameters,
     * therefore the pixmap had better be the right size.
     */
    while (ptr)
    {

	if (( !(ptr->inUse)) &&
	    (ptr->depth == depth))
	{
	    if ((ptr->width == width) &&
		(ptr->height == height))
	    {
		/* Exact match.  We're done */
		ptr->inUse = True;
		return(ptr->pixmap);
	    }
        }
	ptr = ptr->next;
      }
    if (scratchPixmap)
    {
	/* We have an acceptable fit */
	scratchPixmap->inUse = True;
    }
    else
    {
	/*
	 * We only get here if we don't get an exact match, nor a
	 * next best fit.
	 */
	scratchPixmap = XtNew(XmScratchPixmapRec);
	scratchPixmap->inUse = True;
	scratchPixmap->width = width;
	scratchPixmap->height = height;
	scratchPixmap->depth = depth;
	scratchPixmap->pixmap = 
		XCreatePixmap (XtDisplay(xmScreen),
		               RootWindowOfScreen(XtScreen(xmScreen)),
			       width, height,
			       depth);
	scratchPixmap->next = *scratchPtr;
	*scratchPtr = scratchPixmap;
    }
    return(scratchPixmap->pixmap);
}

/************************************************************************
 *
 *  _XmFreeScratchPixmap
 *
 ************************************************************************/

void 
#ifdef _NO_PROTO
_XmFreeScratchPixmap( xmScreen, pixmap )
        XmScreen xmScreen ;
        Pixmap pixmap ;
#else
_XmFreeScratchPixmap(
        XmScreen xmScreen,
        Pixmap pixmap )
#endif /* _NO_PROTO */
{
    XmScratchPixmap	scratchPixmap = xmScreen->screen.scratchPixmaps;

    while (scratchPixmap) {
	if (scratchPixmap->pixmap == pixmap) {
	    scratchPixmap->inUse = False;
	    return;
	}
        scratchPixmap = scratchPixmap->next;
    }
}


/************************************************************************
 *
 *  _XmGetDragCursorCachePtr ()
 *
 ************************************************************************/

XmDragCursorCache * 
#ifdef _NO_PROTO
_XmGetDragCursorCachePtr( xmScreen )
        XmScreen xmScreen ;
#else
_XmGetDragCursorCachePtr(
        XmScreen xmScreen )
#endif /* _NO_PROTO */
{
    return &xmScreen->screen.cursorCache;
}

/************************************************************************
 *
 *  _XmGetMaxCursorSize ()
 *
 ************************************************************************/

void 
#ifdef _NO_PROTO
_XmGetMaxCursorSize( w, width, height )
        Widget w ;
        Dimension *width ;
        Dimension *height ;
#else
_XmGetMaxCursorSize(
        Widget w,
        Dimension *width,
        Dimension *height )
#endif /* _NO_PROTO */
{
    XmScreen	xmScreen = (XmScreen) XmGetXmScreen(XtScreenOfObject(w));

    *width = (Dimension)xmScreen->screen.maxCursorWidth;
    *height = (Dimension)xmScreen->screen.maxCursorHeight;
    return;
}

static char nullBits[] = 
{ 
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

/************************************************************************
 *
 *  _XmGetNullCursor ()
 *
 ************************************************************************/

Cursor 
#ifdef _NO_PROTO
_XmGetNullCursor( w )
        Widget w ;
#else
_XmGetNullCursor(
        Widget w )
#endif /* _NO_PROTO */
{
    XmScreen		xmScreen = (XmScreen) XmGetXmScreen(XtScreenOfObject(w));
    Pixmap		pixmap;
    Cursor		cursor;
    XColor		foreground;
    XColor		background;

    if (xmScreen->screen.nullCursor == None) {
	foreground.pixel =  
	  background.pixel = 0;
	pixmap =
	  XCreatePixmapFromBitmapData(XtDisplayOfObject(w), 
				      RootWindowOfScreen(XtScreenOfObject(w)),
				      nullBits,
				      4, 4,  
				      0, 0,
				      1);
	cursor =
	  XCreatePixmapCursor(XtDisplayOfObject(w),
			      pixmap,
			      pixmap,
			      &foreground, &background,
			      0, 0);
	XFreePixmap(XtDisplayOfObject(w), pixmap);
	xmScreen->screen.nullCursor = cursor;
    }
    return xmScreen->screen.nullCursor;
}

/*
 * The following set of functions support the menu cursor functionality.
 * They have moved from MenuUtil to here.
 */


/* Obsolete global per-display menu cursor manipulation functions */
/* Programs have to use XtSet/GetValues on the XmScreen object instead */
void
#ifdef _NO_PROTO
XmSetMenuCursor( display, cursorId )
        Display *display ;
        Cursor cursorId ;
#else
XmSetMenuCursor(
        Display *display,
        Cursor cursorId )
#endif /* _NO_PROTO */
{
    XmScreen          xmScreen;
    Screen            *scr;
    int i ;

    /* This function has no screen parameter, so we have to set the
       menucursor for _all_ the xmscreen available on this display. why?
       because when RowColumn will be getting a menucursor for a particular
       screen, it will have to get what the application has set using
       this function, not the default for that particular screen (which is
       what will happen if we were only setting the default display here) */

    for (i=0, scr = ScreenOfDisplay(display, i); i < ScreenCount(display);
       i++, scr = ScreenOfDisplay(display, i)) {

      xmScreen = (XmScreen) XmGetXmScreen(scr);
      xmScreen->screen.menuCursor = cursorId ;
    }
}


Cursor
#ifdef _NO_PROTO
XmGetMenuCursor( display )
        Display *display ;
#else
XmGetMenuCursor(
        Display *display )
#endif /* _NO_PROTO */
{
   XmScreen           xmScreen;

   /* get the default screen menuCursor since there is no
      other information available to this function */
   xmScreen = (XmScreen) XmGetXmScreen(DefaultScreenOfDisplay(display));
   return(xmScreen->screen.menuCursor);
}

/* a convenience for RowColumn */
Cursor
#ifdef _NO_PROTO
_XmGetMenuCursorByScreen(screen)
        Screen * screen ;
#else
_XmGetMenuCursorByScreen(
        Screen * screen  )
#endif /* _NO_PROTO */
{
   XmScreen           xmScreen;

   xmScreen = (XmScreen) XmGetXmScreen(screen);
   return(xmScreen->screen.menuCursor);
}

Boolean
#ifdef _NO_PROTO
_XmGetMoveOpaqueByScreen(screen)
        Screen * screen ;
#else
_XmGetMoveOpaqueByScreen(
        Screen * screen  )
#endif /* _NO_PROTO */
{
   XmScreen           xmScreen;

   xmScreen = (XmScreen) XmGetXmScreen(screen);
   return(xmScreen->screen.moveOpaque);
}

/* a convenience for RowColumn */
unsigned char
#ifdef _NO_PROTO
_XmGetUnpostBehavior( wid )
        Widget wid;
#else
_XmGetUnpostBehavior(
        Widget wid )
#endif /* _NO_PROTO */
{
   XmScreen	xmScreen = (XmScreen) XmGetXmScreen(XtScreenOfObject(wid));

   return(xmScreen->screen.unpostBehavior);
}


/**********************************************************************
 **********************************************************************

      Font unit handling functions

 **********************************************************************
 **********************************************************************/

/**********************************************************************
 *
 *  XmSetFontUnits
 *    Set the font_unit value for all screens.  These values can
 *    then be used later to process the font unit conversions.
 *
 **********************************************************************/
void
#ifdef _NO_PROTO
XmSetFontUnits( display, h_value, v_value )
        Display *display ;
        int h_value ;
        int v_value ;
#else
XmSetFontUnits(
        Display *display,
        int h_value,
        int v_value )
#endif /* _NO_PROTO */
{
    XmScreen          xmScreen;
    Screen            *scr;
    int i ;

    /* This function has no screen parameter, so we have to set the
       fontunit for _all_ the xmscreen available on this display. why?
       because when someone will be getting fontunits for a particular
       screen, it will have to get what the application has set using
       this function, not the default for that particular screen (which is
       what will happen if we were only setting the default display here) */

    for (i=0, scr = ScreenOfDisplay(display, i); i < ScreenCount(display);
       i++, scr = ScreenOfDisplay(display, i)) {

      xmScreen = (XmScreen) XmGetXmScreen(scr);
      xmScreen->screen.h_unit =  h_value ;
      xmScreen->screen.v_unit =  v_value ;
    }

}

/* DEPRECATED */
void
#ifdef _NO_PROTO
XmSetFontUnit( display, value )
        Display *display ;
        int value ;
#else
XmSetFontUnit(
        Display *display,
        int value )
#endif /* _NO_PROTO */
{
    XmSetFontUnits(display, value, value);
}



/**********************************************************************
 *
 *  _XmGetFontUnit
 *
 **********************************************************************/
int
#ifdef _NO_PROTO
_XmGetFontUnit(screen, dimension )
        Screen *screen ;
        int dimension ;
#else
_XmGetFontUnit(
        Screen *screen,
        int dimension )
#endif /* _NO_PROTO */
{
    XmScreen          xmScreen;

    xmScreen = (XmScreen) XmGetXmScreen(screen);
    if (dimension == XmHORIZONTAL)
      return(xmScreen->screen.h_unit);
    else
      return(xmScreen->screen.v_unit);
}
