#pragma ident	"@(#)m1.2libs:Xm/DragOverS.c	1.7"
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

/************************************************************************
 *
 *  This module dynamically blends and manages the dragover visual using
 *  the XmDragOverShell widget and the following API:
 *
 *	_XmDragOverHide()
 *	_XmDragOverShow()
 *	_XmDragOverMove()
 *	_XmDragOverChange()
 *	_XmDragOverFinish()
 *	_XmDragOverGetActiveCursor()
 *	_XmDragOverSetInitialPosition()
 *
 *  The XmDragOverShellPart structure has the following members:
 *
 *	Position hotX, hotY -- the coordinates of the dragover visual's
 *		hotspot.  These are settable resources and are maintained
 *		through the _XmDragOverMove() interface.
 *
 *	Position initialX, initialY -- the initial coordinates of the
 *		dragover visual's hotspot.  These are set in the
 *		XmDragOverShell's initialization and also through the
 *		_XmDragOverSetInitialPosition() interface.  These are
 *		used as the zap-back position in the zap-back dropFinish
 *		effect.
 *
 *	unsigned char mode -- one of {XmPIXMAP, XmCURSOR, XmWINDOW}.
 *		XmPIXMAP indicates the current drag protocol style is
 *		pre-register and indicates that either a pixmap or a
 *		cursor can be used for the dragover visual.  XmCURSOR
 *		indicates dynamic protocol style is in effect and that
 *		a cursor must be used for the dragover visual.  XmWINDOW
 *		is used during dropFinish processing so that the dragover
 *		visual can persist even without the server grabbed.
 *
 *	unsigned char activeMode -- one of {XmPIXMAP, XmCURSOR, XmWINDOW}.
 *		Determined by the value of mode and the capabilities of
 *		the hardware.  Indicates how the dragover visual is being
 *		rendered.  XmCURSOR indicates that the hardware cursor is
 *		being used.  XmPIXMAP indicates that a pixmap is being
 *		dragged.  XmWINDOW indicates that the XmDragOverShell's
 *		window is popped up and is being dragged.
 *
 *	Cursor activeCursor -- if activeMode == XmCURSOR, contains the
 *		cursor being used to render the dragover visual.
 *		Otherwise, contains a null cursor.
 *
 *	unsigned char cursorState -- set within _XmDragOverChange(),
 *		and one of {XmVALID_DROP_SITE, XmINVALID_DROP_SITE,
 *		XmNO_DROP_SITE}, indicates the status of the current
 *		dropsite.
 *		
 *	Pixel cursorForeground, cursorBackground -- indicates the current
 *		foreground and background colors of the dragover visual.
 *		Can depend on the cursorState and the screen's default
 *		colormap.
 *
 *	XmDragIconObject stateIcon, opIcon -- indicates, respectively, the
 *		current state and operation icons being blended to the
 *		source icon to form the dragover visual.  If a state or
 *		operation icon is not being blended, the corresponding
 *		icon here is NULL.
 *
 *	XmDragOverBlendRec cursorBlend, rootBlend -- cursorBlend contains
 *		the blended icon data for XmCURSOR activeMode and rootBlend
 *		contains the blended icon data for XmPIXMAP and XmWINDOW
 *		activeMode.  The blended icon data consists of
 *
 *			XmDragIconObject sourceIcon --	the source icon.
 *			Position sourceX, sourceY -- the source location
 *				within the blended icon (dragover visual).
 *			XmDragIconObject mixedIcon -- the blended icon.
 *			GC gc -- the gc used to create the blended icon.
 *				the rootBlend gc is also used to render
 *				the blended icon to the display.
 *
 *	Boolean isVisible -- indicates whether the dragover visual is
 *		visible.  Used to avoid unnecessary computation.
 *
 *	XmBackingRec backing -- contains the backing store needed during
 *		pixmap dragging (XmPIXMAP activeMode) and the dropFinish
 *		effects.  Consists of an (x,y) position in root coordinates
 *		and a pixmap.
 *
 *	Pixmap tmpPix, tmpBit -- scratch pixmap and bitmap used during
 *		pixmap dragging (XmPIXMAP activeMode) to reduce screen
 *		flashing.
 *
 *	Cursor ncCursor -- the cursor id of the last noncached cursor
 *		used.  Needed so the cursor can be freed when it is no
 *		longer needed.
 *
 *
 *
 *  NOTE:  This module directly accesses three members of the
 *         XmDragContext structure:
 *
 *	dc->drag.origDragOver
 *	dc->drag.operation
 *	dc->drag.lastChangeTime
 *
 ***********************************************************************/

#include <Xm/XmP.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <Xm/VendorSEP.h>
#include "DragCI.h"
#include "DragICCI.h"
#include "MessagesI.h"
#include <Xm/DragCP.h>
#include <Xm/DragIconP.h>
#include <Xm/DragOverSP.h>
#include <Xm/XmosP.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <X11/Xos.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MESSAGE1	catgets(Xm_catd,MS_DragOver,MSG_DO_1,_XmMsgDragOverS_0000)
#define MESSAGE2	catgets(Xm_catd,MS_DragOver,MSG_DO_1,_XmMsgDragOverS_0001)
#define MESSAGE3	catgets(Xm_catd,MS_DragOver,MSG_DO_1,_XmMsgDragOverS_0002)
#define MESSAGE4	catgets(Xm_catd,MS_DragOver,MSG_DO_1,_XmMsgDragOverS_0003)
#else
#define MESSAGE1	_XmMsgDragOverS_0000
#define MESSAGE2	_XmMsgDragOverS_0001
#define MESSAGE3	_XmMsgDragOverS_0002
#define MESSAGE4	_XmMsgDragOverS_0003
#endif


#define PIXMAP_MAX_WIDTH	128
#define PIXMAP_MAX_HEIGHT	128

#define BackingPixmap(dos) (dos->drag.backing.pixmap)
#define BackingX(dos) (dos->drag.backing.x)
#define BackingY(dos) (dos->drag.backing.y)

#undef Offset
#define Offset(x) (XtOffsetOf(XmDragOverShellRec, x))

#define ZAP_TIME 50000L
#define MELT_TIME 50000L

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void DoZapEffect() ;
static void DoMeltEffect() ;
static void GetIconPosition() ;
static void BlendIcon() ;
static void MixedIconSize() ;
static void DestroyMixedIcon() ;
static void MixIcons() ;
static Boolean FitsInCursor() ;
static Boolean GetDragIconColors() ;
static Cursor GetDragIconCursor() ;
static void Initialize() ;
static Boolean SetValues() ;
static void DrawIcon() ;
static void Redisplay() ;
static void Destroy() ;
static void ChangeActiveMode() ;
static void ChangeDragWindow() ;
static void Realize();

#else

static void DoZapEffect( 
                        XtPointer clientData,
                        XtIntervalId *id) ;
static void DoMeltEffect( 
                        XtPointer clientData,
                        XtIntervalId *id) ;
static void GetIconPosition( 
                        XmDragOverShellWidget dos,
                        XmDragIconObject icon,
                        XmDragIconObject sourceIcon,
                        Position *iconX,
                        Position *iconY) ;
static void BlendIcon( 
                        Display *display,
                        XmDragIconObject icon,
                        XmDragIconObject mixedIcon,
#if NeedWidePrototypes
                        int iconX,
                        int iconY,
#else
                        Position iconX,
                        Position iconY,
#endif /* NeedWidePrototypes */
                        GC maskGC,
                        GC pixmapGC) ;
static void MixedIconSize( 
                        XmDragOverShellWidget dos,
                        XmDragIconObject sourceIcon,
                        XmDragIconObject stateIcon,
                        XmDragIconObject opIcon,
                        Dimension *width,
                        Dimension *height) ;
static void DestroyMixedIcon( 
                        XmDragOverShellWidget dos,
                        XmDragIconObject mixedIcon) ;
static void MixIcons( 
                        XmDragOverShellWidget dos,
                        XmDragIconObject sourceIcon,
                        XmDragIconObject stateIcon,
                        XmDragIconObject opIcon,
                        XmDragOverBlendRec *blendPtr,
#if NeedWidePrototypes
                        int clip) ;
#else
                        Boolean clip) ;
#endif /* NeedWidePrototypes */
static Boolean FitsInCursor( 
                        XmDragOverShellWidget dos,
                        XmDragIconObject sourceIcon,
                        XmDragIconObject stateIcon,
                        XmDragIconObject opIcon) ;
static Boolean GetDragIconColors( 
                        XmDragOverShellWidget dos) ;
static Cursor GetDragIconCursor( 
                        XmDragOverShellWidget dos,
                        XmDragIconObject sourceIcon,
                        XmDragIconObject stateIcon,
                        XmDragIconObject opIcon,
#if NeedWidePrototypes
			int clip,
                        int dirty) ;
#else
                        Boolean clip,
                        Boolean dirty) ;
#endif /* NeedWidePrototypes */
static void Initialize( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *numArgs) ;
static Boolean SetValues( 
                        Widget current,
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void DrawIcon( 
                        XmDragOverShellWidget dos,
                        XmDragIconObject icon,
                        Window window,
#if NeedWidePrototypes
                        int x,
                        int y) ;
#else
                        Position x,
                        Position y) ;
#endif /* NeedWidePrototypes */
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void Destroy( 
                        Widget w) ;
static void ChangeActiveMode( 
                        XmDragOverShellWidget dos,
#if NeedWidePrototypes
                        unsigned int newActiveMode) ;
#else
                        unsigned char newActiveMode) ;
#endif /* NeedWidePrototypes */
static void ChangeDragWindow(
			XmDragOverShellWidget dos) ;
static void Realize(
        Widget wid,
        XtValueMask *vmask,
        XSetWindowAttributes *attr );

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static XtResource resources[]=
{
    {
	XmNoverrideRedirect, XmCOverrideRedirect,
	XmRBoolean, sizeof(Boolean), Offset(shell.override_redirect),
	XtRImmediate, (XtPointer)True,
    },
    {
	XmNhotX, XmCHot, XmRPosition,
        sizeof(Position), Offset(drag.hotX),
        XmRImmediate, (XtPointer)0,
    },
    {
	XmNhotY, XmCHot, XmRPosition,
        sizeof(Position), Offset(drag.hotY),
        XmRImmediate, (XtPointer)0,
    },
    {
	XmNdragOverMode, XmCDragOverMode, XtRUnsignedChar,
        sizeof(unsigned char), Offset(drag.mode),
        XmRImmediate, (XtPointer)XmCURSOR,
    },
};

/***************************************************************************
 *
 * DragOverShell class record
 *
 ***************************************************************************/

externaldef(xmdragovershellclassrec)
XmDragOverShellClassRec xmDragOverShellClassRec = {
    {					/* core class record */
	(WidgetClass) &vendorShellClassRec,	/* superclass */
	"XmDragOverShell",	 	/* class_name */
	sizeof(XmDragOverShellRec),	/* widget_size */
	(XtProc)NULL,			/* class_initialize proc */
	(XtWidgetClassProc)NULL,	/* class_part_initialize proc */
	FALSE, 				/* class_inited flag */
	Initialize,	 		/* instance initialize proc */
	(XtArgsProc)NULL, 		/* init_hook proc */
	Realize,		        /* realize widget proc */
	NULL,				/* action table for class */
	0,				/* num_actions */
	resources,			/* resource list of class */
	XtNumber(resources),		/* num_resources in list */
	NULLQUARK, 			/* xrm_class ? */
	FALSE, 				/* don't compress_motion */
	TRUE, 				/* do compress_exposure */
	FALSE, 				/* do compress enter-leave */
	FALSE, 				/* do have visible_interest */
	Destroy,			/* destroy widget proc */
	XtInheritResize, 		/* resize widget proc */
	Redisplay,			/* expose proc */
	SetValues, 			/* set_values proc */
	(XtArgsFunc)NULL, 		/* set_values_hook proc */
	XtInheritSetValuesAlmost, 	/* set_values_almost proc */
	(XtArgsProc)NULL, 		/* get_values_hook */
	(XtAcceptFocusProc)NULL, 	/* accept_focus proc */
	XtVersion, 			/* current version */
	NULL, 				/* callback offset    */
	NULL,		 		/* default translation table */
	XtInheritQueryGeometry, 	/* query geometry widget proc */
	(XtStringProc)NULL, 		/* display accelerator    */
	NULL,				/* extension record      */
    },
     { 					/* composite class record */
	XtInheritGeometryManager,	/* geometry_manager */
	XtInheritChangeManaged,		/* change_managed		*/
	XtInheritInsertChild,		/* insert_child			*/
	XtInheritDeleteChild,		/* from the shell */
	NULL, 				/* extension record      */
    },
    { 					/* shell class record */
	NULL, 				/* extension record      */
    },
    { 					/* wm shell class record */
	NULL, 				/* extension record      */
    },
    { 					/* vendor shell class record */
	NULL, 				/* extension record      */
    },
    { 					/* dragOver shell class record */
	NULL,				/* extension record      */
    },
};
externaldef(xmDragOvershellwidgetclass) WidgetClass xmDragOverShellWidgetClass = 
	(WidgetClass) (&xmDragOverShellClassRec);

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif /* min */

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif /* max */

typedef struct _MixedIconCache
{
	Cardinal		depth;
	Dimension		width;
	Dimension		height;
	Pixel			cursorForeground;
	Pixel			cursorBackground;
	Position		sourceX;
	Position		sourceY;
	Position		stateX;
	Position		stateY;
	Position		opX;
	Position		opY;
	Pixmap			sourcePixmap;
	Pixmap			statePixmap;
	Pixmap			opPixmap;
	Pixmap			sourceMask;
	Pixmap			stateMask;
	Pixmap			opMask;
	XmDragIconObject	mixedIcon;
	struct _MixedIconCache *next;
} MixedIconCache;
	 
static MixedIconCache * mixed_cache = NULL;

static Boolean
#ifdef _NO_PROTO
CacheMixedIcon(dos, depth, width, height, sourceIcon, stateIcon, opIcon,
	       sourceX, sourceY, stateX, stateY, opX, opY, mixedIcon)
	XmDragOverShellWidget	dos;
	Cardinal		depth;
	Dimension		width;
	Dimension		height;
	XmDragIconObject	sourceIcon;
	XmDragIconObject	stateIcon;
	XmDragIconObject	opIcon;
	Position		sourceX;
	Position		sourceY;
	Position		stateX;
	Position		stateY;
	Position		opX;
	Position		opY;
	XmDragIconObject	mixedIcon;
#else
CacheMixedIcon(
	XmDragOverShellWidget	dos,
	Cardinal		depth,
	Dimension		width,
	Dimension		height,
	XmDragIconObject	sourceIcon,
	XmDragIconObject	stateIcon,
	XmDragIconObject	opIcon,
	Position		sourceX,
	Position		sourceY,
	Position		stateX,
	Position		stateY,
	Position		opX,
	Position		opY,
	XmDragIconObject	mixedIcon)
#endif /* _NO_PROTO */
{
    register MixedIconCache * cache_ptr;

    if (mixedIcon == NULL) return False;

    cache_ptr = XtNew (MixedIconCache);
    cache_ptr->next = mixed_cache;
    mixed_cache = cache_ptr;

    cache_ptr->depth = depth;
    cache_ptr->width = width;
    cache_ptr->height = height;
    cache_ptr->cursorForeground = dos->drag.cursorForeground;
    cache_ptr->cursorBackground = dos->drag.cursorBackground;
    cache_ptr->sourcePixmap = sourceIcon->drag.pixmap;
    cache_ptr->sourceMask = sourceIcon->drag.mask;
    cache_ptr->sourceX = sourceX;
    cache_ptr->sourceY = sourceY;

    if (stateIcon) {
    	cache_ptr->statePixmap = stateIcon->drag.pixmap;
    	cache_ptr->stateMask = stateIcon->drag.mask;
    	cache_ptr->stateX = stateX;
    	cache_ptr->stateY = stateY;
    } else {
    	cache_ptr->statePixmap = NULL;
    }

    if (opIcon) {
       cache_ptr->opPixmap = opIcon->drag.pixmap;
       cache_ptr->opMask = opIcon->drag.mask;
       cache_ptr->opX = opX;
       cache_ptr->opY = opY;
    } else {
       cache_ptr->opPixmap = NULL;
    }

    cache_ptr->mixedIcon = mixedIcon;

    return True;
}


static XmDragIconObject
#ifdef _NO_PROTO
GetMixedIcon(dos, depth, width, height, sourceIcon, stateIcon, opIcon, sourceX,
	     sourceY, stateX, stateY, opX, opY)
	XmDragOverShellWidget	dos;
	Cardinal		depth;
	Dimension		width;
	Dimension		height;
	XmDragIconObject	sourceIcon;
	XmDragIconObject	stateIcon;
	XmDragIconObject	opIcon;
	Position		sourceX;
	Position		sourceY;
	Position		stateX;
	Position		stateY;
	Position		opX;
	Position		opY;
#else
GetMixedIcon(
	XmDragOverShellWidget	dos,
	Cardinal		depth,
	Dimension		width,
	Dimension		height,
	XmDragIconObject	sourceIcon,
	XmDragIconObject	stateIcon,
	XmDragIconObject	opIcon,
	Position		sourceX,
	Position		sourceY,
	Position		stateX,
	Position		stateY,
	Position		opX,
	Position		opY)
#endif /* _NO_PROTO */
{
    register MixedIconCache * cache_ptr;

    for (cache_ptr = mixed_cache; cache_ptr; cache_ptr = cache_ptr->next)
    {       
	if (cache_ptr->depth == depth &&
	    cache_ptr->width == width &&
	    cache_ptr->height == height &&
	    cache_ptr->cursorForeground == dos->drag.cursorForeground &&
	    cache_ptr->cursorBackground == dos->drag.cursorBackground &&
	    cache_ptr->sourcePixmap == sourceIcon->drag.pixmap &&
	    cache_ptr->sourceMask == sourceIcon->drag.mask &&
	    cache_ptr->sourceX == sourceX &&
	    cache_ptr->sourceY == sourceY &&
            ((cache_ptr->statePixmap == NULL && !stateIcon) ||
             (stateIcon && cache_ptr->statePixmap == stateIcon->drag.pixmap &&
			   cache_ptr->stateMask == stateIcon->drag.mask &&
	    		   cache_ptr->stateX == stateX &&
	    		   cache_ptr->stateY == stateY)) &&
            ((cache_ptr->opPixmap == NULL && !opIcon) ||
             (opIcon && cache_ptr->opPixmap == opIcon->drag.pixmap &&
			   cache_ptr->opMask == opIcon->drag.mask &&
	    		   cache_ptr->opX == opX &&
	    		   cache_ptr->opY == opY)))
	{
           return (cache_ptr->mixedIcon);
        }
     }
     return ((XmDragIconObject)NULL);
}


/************************************************************************
 *
 *  DoZapEffect()
 *
 ***********************************************************************/

static void
#ifdef _NO_PROTO
DoZapEffect( clientData, id )
    XtPointer		clientData;
    XtIntervalId	*id;
#else
DoZapEffect(
    XtPointer		clientData,
    XtIntervalId	*id)
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget)clientData;
    static XSegment 		segments[4];
    int 			j;
    int 			i = 0;
    int 			rise, run;
    int 			centerX, centerY;
    GC				draw_gc = dos->drag.rootBlend.gc;
    XGCValues			v;
    unsigned long		vmask;

    for (j = 0; j < 4; j++)
    {
	segments[j].x1 = dos->drag.initialX;
	segments[j].y1 = dos->drag.initialY;
    }
    segments[0].x2 = dos->core.x;
    segments[0].y2 = dos->core.y;
    segments[1].x2 = dos->core.x;
    segments[1].y2 = dos->core.y + dos->core.height;
    segments[2].x2 = dos->core.x + dos->core.width;
    segments[2].y2 = dos->core.y + dos->core.height;
    segments[3].x2 = dos->core.x + dos->core.width;
    segments[3].y2 = dos->core.y;

    centerX = dos->core.x + dos->core.width/2;
    centerY = dos->core.y + dos->core.height/2;
    rise = (dos->drag.initialY - centerY) / 5;
    run = (dos->drag.initialX - centerX) / 5;

    /*
     *  Draw the lines and add the timeout.
     */

    v.foreground = 1;
    v.function = GXxor;
    v.clip_mask = None;
    vmask = GCForeground|GCFunction|GCClipMask;
    XChangeGC (XtDisplay((Widget)dos), draw_gc, vmask, &v);
    XDrawSegments (XtDisplay((Widget)dos), 
		   RootWindowOfScreen(XtScreen(dos)),
		   draw_gc, segments, 4);
    XFlush(XtDisplay((Widget)dos));

      
    /*
     * Do an abbreviated zap effect if the rise and run are both small.
     */

    if (((rise <= 3) && (rise >= -3)) && ((run <= 3) && (run >= -3))) {
	i = 5;
    }

    for (; ; i++ )
    {
	/* wait */
	_XmMicroSleep (ZAP_TIME);

	/*
	 *  Erase the previously drawn lines and restore the root.
	 */

	XDrawSegments (XtDisplay((Widget)dos), 
		       RootWindowOfScreen(XtScreen(dos)), 
		       draw_gc, segments, 4);

    	v.foreground = dos->drag.cursorForeground;
    	v.function = GXcopy;
    	vmask = GCForeground|GCFunction;
    	XChangeGC (XtDisplay((Widget)dos), draw_gc, vmask, &v);
	XCopyArea (XtDisplay((Widget)dos),
		   BackingPixmap(dos),
		   RootWindowOfScreen(XtScreen(dos)),
		   draw_gc,
		   0, 0, dos->core.width, dos->core.height,
		   segments[0].x2, segments[0].y2);

	/* Here is where we always leave the loop */
	if (i == 5)
	    break;
	/*
	 *  Compute the new position.
	 *  Save the root.
	 *  Draw the pixmap (mixedIcon exists) and new lines.
	 */

	segments[0].x2 += run;
	segments[0].y2 += rise;
	segments[1].x2 += run;
	segments[1].y2 += rise;
	segments[2].x2 += run;
	segments[2].y2 += rise;
	segments[3].x2 += run;
	segments[3].y2 += rise;

	XCopyArea (XtDisplay((Widget)dos), 
		   RootWindowOfScreen(XtScreen(dos)),
		   BackingPixmap(dos),
		   draw_gc,
		   segments[0].x2, segments[0].y2, 
		   dos->core.width, dos->core.height, 0, 0);

	DrawIcon (dos,
		  (dos->drag.rootBlend.mixedIcon ?
		   dos->drag.rootBlend.mixedIcon :
		   dos->drag.cursorBlend.mixedIcon),
		  RootWindowOfScreen(XtScreen(dos)), 
		  segments[0].x2, segments[0].y2);

    	v.foreground = 1;
    	v.function = GXxor;
    	vmask = GCForeground|GCFunction;
    	XChangeGC (XtDisplay((Widget)dos), draw_gc, vmask, &v);
	XDrawSegments (XtDisplay((Widget)dos),  
		       RootWindowOfScreen(XtScreen(dos)), 
		       draw_gc, segments, 4);
	XFlush (XtDisplay((Widget)dos));
    }

    XFlush (XtDisplay((Widget)dos));
}

/************************************************************************
 *
 *  DoMeltEffect()
 *
 *   This function is responsible for restoring the display after a
 *   drop has occurred in a receptive area.  It uses a timeout to create
 *   a venetian blind effect as it removes the drag pixmap, and restores
 *   the original root window contents.  
 ***********************************************************************/

static void
#ifdef _NO_PROTO
DoMeltEffect( clientData, id )
    XtPointer		clientData;
    XtIntervalId	*id;
#else
DoMeltEffect(
    XtPointer		clientData,
    XtIntervalId	*id)
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget)clientData;
    XmDragIconObject    	sourceIcon;
    XmDragOverBlend		blend;
    int 			iterations;
    int 			i = 0;
    int 			xClipOffset;
    int 			yClipOffset;
    XRectangle 			rects[4];
    GC 				draw_gc = dos->drag.rootBlend.gc;

    /*
     *  Blend a new mixedIcon using only the source icon.
     *  Place the new mixedIcon to preserve the source icon location.
     *  The current mixedIcon data is valid.
     */

    if (dos->drag.rootBlend.sourceIcon) {
	sourceIcon = dos->drag.rootBlend.sourceIcon;
	blend = &dos->drag.rootBlend;
    } else {
	sourceIcon = dos->drag.cursorBlend.sourceIcon;
	blend = &dos->drag.cursorBlend;
    }

    /*
     *  Determine how much to shrink on each pass.
     */

    if ((xClipOffset = (sourceIcon->drag.width / 16)) <= 0) {
	xClipOffset = 1;
    }
    if ((yClipOffset = (sourceIcon->drag.height / 16)) <= 0) {
	yClipOffset = 1;
    }
      
    iterations = min(sourceIcon->drag.width/(2*xClipOffset),
    		      sourceIcon->drag.height/(2*yClipOffset));
    /* 
     *  Generate the clipping rectangles.
     *  We converge on the center of the cursor.
     */

    rects[0].x = dos->core.x;
    rects[0].y = dos->core.y;
    rects[0].width = dos->core.width;
    rects[0].height = blend->sourceY + yClipOffset;
      
    rects[1].x = rects[0].x + blend->sourceX + sourceIcon->drag.width -
		 xClipOffset;
    rects[1].y = rects[0].y + yClipOffset + blend->sourceY;
    rects[1].width = dos->core.width - (rects[1].x - rects[0].x);
    rects[1].height = dos->core.height - ((yClipOffset * 2) + blend->sourceY);
      
    rects[2].x = rects[0].x;
    rects[2].y = rects[0].y + blend->sourceY + sourceIcon->drag.height -
	         yClipOffset;
    rects[2].width = rects[0].width;
    rects[2].height = dos->core.height - (rects[2].y - rects[0].y);

    rects[3].x = rects[0].x;
    rects[3].y = rects[0].y + yClipOffset + blend->sourceY;
    rects[3].width = xClipOffset + blend->sourceX;
    rects[3].height = rects[1].height;

    for (i = 0; i < iterations; i++)
    {
	XSetClipRectangles (XtDisplay((Widget)dos), 
			    draw_gc, 0, 0, rects, 4, Unsorted);
	XCopyArea (XtDisplay((Widget)dos),
		   BackingPixmap(dos),
		   RootWindowOfScreen(XtScreen(dos)),
		   draw_gc,
		   0, 0, dos->core.width, dos->core.height,
		   dos->core.x, dos->core.y);
	XFlush (XtDisplay((Widget)dos));

	rects[0].height += yClipOffset;
	rects[1].x -= xClipOffset;
	rects[1].width += xClipOffset;
	rects[2].y -= yClipOffset;
	rects[2].height += yClipOffset;
	rects[3].width += xClipOffset;

	/* wait */
	_XmMicroSleep (MELT_TIME);
    }
    
    XSetClipMask (XtDisplay((Widget)dos), draw_gc, None);
    XCopyArea (XtDisplay((Widget)dos),
	       BackingPixmap(dos),
	       RootWindowOfScreen(XtScreen(dos)),
	       draw_gc,
	       0, 0, dos->core.width, dos->core.height,
	       dos->core.x, dos->core.y);

    XFlush (XtDisplay((Widget)dos));
}

/************************************************************************
 *
 *  GetIconPosition ()
 *
 *  Get the state or operation icon position, relative to the source.
 ***********************************************************************/

static void
#ifdef _NO_PROTO
GetIconPosition( dos, icon, sourceIcon, iconX, iconY)
    XmDragOverShellWidget	dos;
    XmDragIconObject		icon;
    XmDragIconObject		sourceIcon;
    Position			*iconX;
    Position			*iconY;
#else
GetIconPosition(
    XmDragOverShellWidget	dos,
    XmDragIconObject		icon,
    XmDragIconObject		sourceIcon,
    Position			*iconX,
    Position			*iconY)
#endif /* _NO_PROTO */
{
    switch ((int) icon->drag.attachment) {

	    default:
		_XmWarning ((Widget) icon, MESSAGE2); /* cast ok here */
            case XmATTACH_NORTH_WEST:
                *iconX = icon->drag.offset_x;
                *iconY = icon->drag.offset_y;
	        break;

            case XmATTACH_NORTH:
                *iconX = ((Position) sourceIcon->drag.width/2)
			 + icon->drag.offset_x;
                *iconY = icon->drag.offset_y;
	        break;

            case XmATTACH_NORTH_EAST:
                *iconX = ((Position) sourceIcon->drag.width)
			 + icon->drag.offset_x;
                *iconY = icon->drag.offset_y;
	        break;

            case XmATTACH_EAST:
                *iconX = ((Position) sourceIcon->drag.width)
			 + icon->drag.offset_x;
                *iconY = ((Position) sourceIcon->drag.height/2)
			 + icon->drag.offset_y;
	        break;

            case XmATTACH_SOUTH_EAST:
                *iconX = ((Position) sourceIcon->drag.width)
			 + icon->drag.offset_x;
                *iconY = ((Position) sourceIcon->drag.height)
			 + icon->drag.offset_y;
	        break;

            case XmATTACH_SOUTH:
                *iconX = ((Position) sourceIcon->drag.width/2)
			 + icon->drag.offset_x;
                *iconY = ((Position) sourceIcon->drag.height)
			 + icon->drag.offset_y;
	        break;

            case XmATTACH_SOUTH_WEST:
                *iconX = icon->drag.offset_x;
                *iconY = ((Position) sourceIcon->drag.height)
			 + icon->drag.offset_y;
	        break;

            case XmATTACH_WEST:
                *iconX = icon->drag.offset_x;
                *iconY = ((Position) sourceIcon->drag.height/2)
			 + icon->drag.offset_y;
	        break;

            case XmATTACH_CENTER:
                *iconX = ((Position) sourceIcon->drag.width/2)
			 + icon->drag.offset_x;
                *iconY = ((Position) sourceIcon->drag.height/2)
			 + icon->drag.offset_y;
	        break;

            case XmATTACH_HOT:
	        {
		    XmDragContext	dc = (XmDragContext)XtParent (dos);
		    Window		root, child;
		    int			rootX, rootY, winX, winY;
		    unsigned int	modMask;
		    XmDragOverShellWidget ref;

		    /*
		     *  This code is only applicable for the stateIcon.
		     *  If the opIcon is XmATTACH_HOT, its hotspot should
		     *  be placed at the stateIcon's hotspot.
		     *
		     *  If this is the first time we are blending the
		     *  stateIcon, we will place its hotspot at the 
		     *  same source icon position as the pointer's 
		     *  position within the reference widget.
		     *
		     *  Otherwise, we want to keep the icon fixed relative
		     *  to the source.  To do this, we need the mixedIcon
		     *  data to be current.  This means the cursorCache
		     *  must not be used with stateIcon XmATTACH_HOT.
		     */

		    ref = (dc->drag.origDragOver != NULL) ?
		          dc->drag.origDragOver : dos;

		    if (ref->drag.rootBlend.mixedIcon) {
                        *iconX = ref->drag.rootBlend.mixedIcon->drag.hot_x
                                 - ref->drag.rootBlend.sourceX
				 - icon->drag.hot_x;
                        *iconY = ref->drag.rootBlend.mixedIcon->drag.hot_y
                                 - ref->drag.rootBlend.sourceY
				 - icon->drag.hot_y;
                    }
		    else if (ref->drag.cursorBlend.mixedIcon) {
                        *iconX = ref->drag.cursorBlend.mixedIcon->drag.hot_x
                                 - ref->drag.cursorBlend.sourceX
				 - icon->drag.hot_x;
                        *iconY = ref->drag.cursorBlend.mixedIcon->drag.hot_y
                                 - ref->drag.cursorBlend.sourceY
				 - icon->drag.hot_y;
                    }
                    else {
			Widget		sourceWidget;
			Dimension	borderW = 0;
			Dimension	highlightT = 0;
			Dimension	shadowT = 0;
			Cardinal	ac;
			Arg		al[3];

			/*
			 *  First time:  get position from pointer,
			 *  adjusting for sourceWidget's border, highlight,
			 *  and shadow.
			 */

			sourceWidget = dc->drag.sourceWidget;

			ac = 0;
			XtSetArg (al[ac], XmNborderWidth, &borderW); ac++;
			XtSetArg (al[ac], XmNhighlightThickness,
				  &highlightT); ac++;
			XtSetArg (al[ac], XmNshadowThickness,
				  &shadowT); ac++;
			XtGetValues (sourceWidget, al, ac);

		        XQueryPointer (XtDisplay (dos),
			               XtWindow (sourceWidget),
			               &root, &child, &rootX, &rootY,
			               &winX, &winY, &modMask);

                        *iconX = winX - icon->drag.hot_x -
                                 borderW - highlightT - shadowT;
                        *iconY = winY - icon->drag.hot_y -
				 borderW - highlightT - shadowT;
		    }
	        }
	        break;
    }
}

/************************************************************************
 *  BlendIcon ()
 *
 *  Blend the icon mask and pixmap into mixedIcon.
 ***********************************************************************/

static void 
#ifdef _NO_PROTO
BlendIcon( display, icon, mixedIcon, iconX, iconY, maskGC, pixmapGC )
    Display		*display;
    XmDragIconObject	icon;
    XmDragIconObject	mixedIcon;
    Position		iconX;
    Position		iconY;
    GC			maskGC;		/* may be NULL */
    GC			pixmapGC;
#else
BlendIcon(
    Display		*display,
    XmDragIconObject	icon,
    XmDragIconObject	mixedIcon,
#if NeedWidePrototypes
    int			iconX,
    int			iconY,
#else
    Position		iconX,
    Position		iconY,
#endif /* NeedWidePrototypes */
    GC			maskGC,
    GC			pixmapGC)
#endif /* _NO_PROTO */
{
    Position		sourceX = 0;
    Position		sourceY = 0;
    Position		destX = iconX;
    Position		destY = iconY;
    Dimension		destWidth = icon->drag.width;
    Dimension		destHeight = icon->drag.height;
    XGCValues		v;
    unsigned long	vmask;

    if (icon->drag.pixmap != XmUNSPECIFIED_PIXMAP) {

        /*
         *  Clip to the destination drawable.
         */

        if (destX < 0) {
	    sourceX -= destX;	/* > 0 */
            destX = 0;
	    if (destWidth <= (Dimension) sourceX) {
	        return;
	    }
            destWidth -= sourceX;
        }
        if (destX + destWidth > mixedIcon->drag.width) {
            if ((Dimension) destX >= mixedIcon->drag.width) {
	        return;
            }
            destWidth = mixedIcon->drag.width - destX;
	}

        if (destY < 0) {
	    sourceY -= destY;	/* > 0 */
            destY = 0;
	    if (destHeight <= (Dimension) sourceY) {
	        return;
	    }
            destHeight -= sourceY;
        }
        if (destY + destHeight > mixedIcon->drag.height) {
            if ((Dimension) destY >= mixedIcon->drag.height) {
	        return;
            }
            destHeight = mixedIcon->drag.height - destY;
	}

        v.clip_mask = None;
        vmask = GCClipMask;

	if (icon->drag.mask != XmUNSPECIFIED_PIXMAP) {

	    v.function = GXor;
	    vmask |= GCFunction;
	    XChangeGC (display, maskGC, vmask, &v);
	    XCopyArea (display, icon->drag.mask,
		       mixedIcon->drag.mask, maskGC,
                       sourceX, sourceY,
		       mixedIcon->drag.width,
		       mixedIcon->drag.height,
                       destX, destY);
	    v.clip_mask = icon->drag.mask;
	    v.clip_x_origin = iconX;
	    v.clip_y_origin = iconY;
	    vmask = GCClipMask|GCClipXOrigin|GCClipYOrigin;
	}
        else {
            if (mixedIcon->drag.mask != XmUNSPECIFIED_PIXMAP){

	       v.function = GXset;
	       vmask |= GCFunction;
	       XChangeGC (display, maskGC, vmask, &v);
	       XFillRectangle (display, mixedIcon->drag.mask, maskGC,
		               destX, destY, destWidth, destHeight);
            } 
	}

        if (icon->drag.region != NULL && mixedIcon->drag.region != NULL) {
            if (icon->drag.x_offset || icon->drag.y_offset)
               XOffsetRegion(icon->drag.region, -icon->drag.x_offset,
						-icon->drag.y_offset);

            XOffsetRegion(icon->drag.region, destX, destY);
            icon->drag.x_offset = destX;
            icon->drag.y_offset = destY;

            XUnionRegion(mixedIcon->drag.region, icon->drag.region,
		         mixedIcon->drag.region);

         }

	/*
	 *  Copy the pixmap.
	 */

	v.function = GXcopy;
	vmask |= GCFunction;
	XChangeGC (display, pixmapGC, vmask, &v);

	if (icon->drag.depth == 1) {
	    XCopyPlane (display, icon->drag.pixmap,
		        mixedIcon->drag.pixmap, pixmapGC,
                        sourceX, sourceY,
		        mixedIcon->drag.width,
                        mixedIcon->drag.height,
                        destX, destY,
		        1L);
	}
	else if (icon->drag.depth == mixedIcon->drag.depth) {
	    XCopyArea (display, icon->drag.pixmap,
		       mixedIcon->drag.pixmap, pixmapGC,
                       sourceX, sourceY,
		       mixedIcon->drag.width,
                       mixedIcon->drag.height,
                       destX, destY);
	}
	else {
	    _XmWarning ((Widget) icon, MESSAGE1); /* cast ok here */
	}
    }
}

/************************************************************************
 *
 *  MixedIconSize ()
 *
 *  Determine the dimensions of the mixedIcon.
 ***********************************************************************/

static void
#ifdef _NO_PROTO
MixedIconSize( dos, sourceIcon, stateIcon, opIcon, width, height )
    XmDragOverShellWidget	dos;
    XmDragIconObject		sourceIcon;
    XmDragIconObject		stateIcon;
    XmDragIconObject		opIcon;
    Dimension			*width;
    Dimension			*height;
#else
MixedIconSize(
    XmDragOverShellWidget	dos,
    XmDragIconObject		sourceIcon,
    XmDragIconObject		stateIcon,
    XmDragIconObject		opIcon,
    Dimension			*width,
    Dimension			*height)
#endif /* _NO_PROTO */
{
    Position		sourceX = 0, sourceY = 0;
    Position		minX = 0, minY = 0;
    Position		stateX, stateY;
    Position		opX, opY;
    Position		maxX, maxY;

    if (stateIcon) {
	GetIconPosition (dos, stateIcon, sourceIcon, &stateX, &stateY);
        minX = min(stateX, minX);
        minY = min(stateY, minY);
    }

    if (opIcon) {
	if (opIcon->drag.attachment == XmATTACH_HOT) {
	    opX = stateX + stateIcon->drag.hot_x - opIcon->drag.hot_x;
	    opY = stateY + stateIcon->drag.hot_y - opIcon->drag.hot_y;
	}
	else {
	    GetIconPosition (dos, opIcon, sourceIcon, &opX, &opY);
	}
        minX = min(opX, minX);
        minY = min(opY, minY);
    }

    sourceX -= minX;	/* >= 0 */
    sourceY -= minY;	/* >= 0 */

    maxX = sourceX + sourceIcon->drag.width;
    maxY = sourceY + sourceIcon->drag.height;

    if (stateIcon) {
	stateX -= minX;
	stateY -= minY;
        maxX = max(stateX + ((Position) stateIcon->drag.width), maxX);
        maxY = max(stateY + ((Position) stateIcon->drag.height), maxY);
    }

    if (opIcon) {
	opX -= minX;
	opY -= minY;
        maxX = max(opX + ((Position) opIcon->drag.width), maxX);
        maxY = max(opY + ((Position) opIcon->drag.height), maxY);
    }

    *width = maxX;
    *height = maxY;
}

/************************************************************************
 *
 *  DestroyMixedIcon ()
 *
 *  The MixedIcon's pixmap and mask, if present, were scratch pixmaps,
 *  and not from the Xm pixmap cache.  Therefore, they need to be freed
 *  separately and reset to XmUNSPECIFIED_PIXMAP.
 ***********************************************************************/

static void
#ifdef _NO_PROTO
DestroyMixedIcon( dos, mixedIcon )
    XmDragOverShellWidget	dos;
    XmDragIconObject		mixedIcon;
#else
DestroyMixedIcon(
    XmDragOverShellWidget	dos,
    XmDragIconObject		mixedIcon)
#endif /* _NO_PROTO */
{
    XmScreen		xmScreen = (XmScreen) XmGetXmScreen(XtScreen(dos));
    register MixedIconCache * cache_ptr;
    register MixedIconCache * prev_cache_ptr = NULL;

    if (mixedIcon->drag.pixmap != XmUNSPECIFIED_PIXMAP) {
        _XmFreeScratchPixmap (xmScreen, mixedIcon->drag.pixmap);
        mixedIcon->drag.pixmap = XmUNSPECIFIED_PIXMAP;
    }

    if (mixedIcon->drag.mask != XmUNSPECIFIED_PIXMAP) {
	_XmFreeScratchPixmap (xmScreen, mixedIcon->drag.mask);
        mixedIcon->drag.mask = XmUNSPECIFIED_PIXMAP;
    }
	
    cache_ptr = mixed_cache;
    while (cache_ptr)
    {
	if (cache_ptr->mixedIcon == mixedIcon) {
	    register MixedIconCache * free_cache_ptr = cache_ptr;

	    if (cache_ptr == mixed_cache)
		mixed_cache = cache_ptr->next;
		/* leave prev_cache_ptr NULL - head of list */
	    else
		prev_cache_ptr->next = cache_ptr->next;
	    
	    cache_ptr = cache_ptr->next;

	    XtFree((char *) free_cache_ptr);
	}
	else {
	    prev_cache_ptr = cache_ptr;
	    cache_ptr = cache_ptr->next;
	}
    }

    XtDestroyWidget ((Widget) mixedIcon);
}

/************************************************************************
 *
 *  MixIcons ()
 *
 ***********************************************************************/

static void
#ifdef _NO_PROTO
MixIcons( dos, sourceIcon, stateIcon, opIcon, blendPtr, clip )
    XmDragOverShellWidget	dos;
    XmDragIconObject		sourceIcon;
    XmDragIconObject		stateIcon;
    XmDragIconObject		opIcon;
    XmDragOverBlendRec		*blendPtr;	/* root or cursor blends */
    Boolean			clip;
#else
MixIcons(
    XmDragOverShellWidget	dos,
    XmDragIconObject		sourceIcon,
    XmDragIconObject		stateIcon,
    XmDragIconObject		opIcon,
    XmDragOverBlendRec		*blendPtr,
#if NeedWidePrototypes
    int				clip)
#else
    Boolean			clip)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Display		*display = XtDisplay(dos);
    XmScreen		xmScreen = (XmScreen) XmGetXmScreen(XtScreen(dos));
    XmDragIconObject	mixedIcon = blendPtr->mixedIcon;
    XmDragOverBlendRec	*cursorBlend = &dos->drag.cursorBlend;
    Arg			al[8];
    Cardinal		ac;
    Pixmap		pixmap, mask;
    Position		sourceX = 0, sourceY = 0;
    Position		stateX = 0, stateY = 0;
    Position		opX = 0, opY = 0;
    Position		minX = 0, minY = 0;
    Position		maxX, maxY;
    Position		hotX, hotY;
    Dimension		width, height;
    Cardinal		depth;
    Boolean		need_mask = True;
    Boolean		do_cache = True;
    XGCValues		v;
    unsigned long	vmask;

    /* 
     *  Determine the dimensions of the blended icon and the positions
     *  of each component within it.
     */

    if (stateIcon) {
	GetIconPosition (dos, stateIcon, sourceIcon, &stateX, &stateY);
        minX = min(stateX, minX);
        minY = min(stateY, minY);
    }

    /*
     *  If the opIcon's attachment is XmATTACH_HOT, attach its hotspot
     *  to the stateIcon's hotspot -- the blended icon's hotspot will be
     *  there, and the blended icon will be positioned so that the blended
     *  hotspot is placed at the cursor position.
     */

    if (opIcon) {
	if (opIcon->drag.attachment == XmATTACH_HOT) {
	    opX = stateX + stateIcon->drag.hot_x - opIcon->drag.hot_x;
	    opY = stateY + stateIcon->drag.hot_y - opIcon->drag.hot_y;
	}
	else {
	    GetIconPosition (dos, opIcon, sourceIcon, &opX, &opY);
	}
        minX = min(opX, minX);
        minY = min(opY, minY);
    }

    sourceX -= minX;	/* >= 0 */
    sourceY -= minY;	/* >= 0 */

    maxX = sourceX + sourceIcon->drag.width;
    maxY = sourceY + sourceIcon->drag.height;

    if (stateIcon) {
	stateX -= minX;
	stateY -= minY;
        maxX = max(stateX + ((Position) stateIcon->drag.width), maxX);
        maxY = max(stateY + ((Position) stateIcon->drag.height), maxY);
        hotX = stateX + stateIcon->drag.hot_x;
	hotY = stateY + stateIcon->drag.hot_y;
    }
    else {
	hotX = sourceX + sourceIcon->drag.hot_x;
	hotY = sourceY + sourceIcon->drag.hot_y;
    }

    if (opIcon) {
	opX -= minX;
	opY -= minY;
        maxX = max(opX + ((Position) opIcon->drag.width), maxX);
        maxY = max(opY + ((Position) opIcon->drag.height), maxY);
    }

    width = maxX;
    height = maxY;

    depth = ((blendPtr == cursorBlend) ? 1 : dos->core.depth);

    /*
     *  If we are clipping the blended icon to fit within a cursor,
     *  we clip it around the hotspot.
     */

    if (clip) {
        Dimension	maxWidth, maxHeight;

	_XmGetMaxCursorSize((Widget) dos, &maxWidth, &maxHeight);

	if (width > maxWidth) {
	    if (hotX <= ((Position) maxWidth/2)) {
    		minX = 0;
	    }
	    else if (hotX >= width - maxWidth/2) {
		minX = width - maxWidth;
	    }
	    else {
		minX = (width - maxWidth)/2;
	    }
	    hotX -= minX;
	    sourceX -= minX;
	    stateX -= minX;
	    opX -= minX;
	    width = maxWidth;
	}
	if (height > maxHeight) {
	    if (hotY <= ((Position) maxHeight/2)) {
    		minY = 0;
	    }
	    else if (hotY >= height - maxHeight/2) {
		minY = height - maxHeight;
	    }
	    else {
		minY = (height - maxHeight)/2;
	    }
	    hotY -= minY;
	    sourceY -= minY;
	    stateY -= minY;
	    opY -= minY;
	    height = maxHeight;
	}
    }

     mixedIcon = GetMixedIcon(dos, depth, width, height, sourceIcon, stateIcon,
			    opIcon, sourceX, sourceY, stateX, stateY, opX, opY);
    /* 
     *  Create the blended XmDragIcon object or use the current one
     *  if it is the correct size and depth.
     */

    if (mixedIcon != NULL) {
       blendPtr->mixedIcon = mixedIcon;
       do_cache = False;
    }

    /*
     *  The blended icon needs a mask unless none of its component icons
     *  has a mask and its component icons completely cover it.
     */

    if ((sourceIcon->drag.mask == XmUNSPECIFIED_PIXMAP) &&
	(!stateIcon || stateIcon->drag.mask == XmUNSPECIFIED_PIXMAP) &&
	(!opIcon || opIcon->drag.mask == XmUNSPECIFIED_PIXMAP)) {

	XRectangle	rect;
	Region		source = XCreateRegion();
	Region		dest = XCreateRegion();
	Region		tmp;

	rect.x = (short) sourceX;
	rect.y = (short) sourceY;
	rect.width = (unsigned short) sourceIcon->drag.width;
	rect.height = (unsigned short) sourceIcon->drag.height;
	XUnionRectWithRegion (&rect, source, dest);

	if (stateIcon) {
	    tmp = source;
	    source = dest;
	    dest = tmp;

	    rect.x = (short) stateX;
	    rect.y = (short) stateY;
	    rect.width = (unsigned short) stateIcon->drag.width;
	    rect.height = (unsigned short) stateIcon->drag.height;
	    XUnionRectWithRegion (&rect, source, dest);
	}

	if (opIcon) {
	    tmp = source;
	    source = dest;
	    dest = tmp;

	    rect.x = (short) opX;
	    rect.y = (short) opY;
	    rect.width = (unsigned short) opIcon->drag.width;
	    rect.height = (unsigned short) opIcon->drag.height;
	    XUnionRectWithRegion (&rect, source, dest);
	}

	if (RectangleIn == XRectInRegion (dest, 0, 0, width, height)) {
	    need_mask = False;
	}

	XDestroyRegion (source);
	XDestroyRegion (dest);
    }	

    if (mixedIcon == NULL) {
	pixmap = _XmAllocScratchPixmap (xmScreen, depth, width, height);

        mask = XmUNSPECIFIED_PIXMAP;
	
	ac = 0;
	XtSetArg(al[ac], XmNpixmap, pixmap); ac++;
	XtSetArg(al[ac], XmNmask, mask); ac++;
	XtSetArg(al[ac], XmNdepth, depth); ac++;
	XtSetArg(al[ac], XmNwidth, width); ac++;
	XtSetArg(al[ac], XmNheight, height); ac++;
	XtSetArg(al[ac], XmNhotX, hotX); ac++;
	XtSetArg(al[ac], XmNhotY, hotY); ac++;
	mixedIcon = blendPtr->mixedIcon = (XmDragIconObject) 
	        XmCreateDragIcon ((Widget) xmScreen, "mixedIcon", al, ac);

	if (need_mask) {
	   mask = mixedIcon->drag.mask = _XmAllocScratchPixmap (xmScreen, 1,
							        width, height);
	}	
    }
    else {
	pixmap = mixedIcon->drag.pixmap;
	mixedIcon->drag.hot_x = hotX;
	mixedIcon->drag.hot_y = hotY;
	if (need_mask && mixedIcon->drag.mask == XmUNSPECIFIED_PIXMAP) {
	    mixedIcon->drag.mask =
	        _XmAllocScratchPixmap (xmScreen, 1, width, height);
	}
	mask = mixedIcon->drag.mask;
    }

    if (sourceIcon->drag.region != NULL) {
       if (mixedIcon->drag.region != NULL)
          XDestroyRegion(mixedIcon->drag.region);

       mixedIcon->drag.region = XCreateRegion();
    }

    /*
     *  Get the cursorBlend GC if needed (the root GC already exists).
     *  Set the pixmap to its background color.
     *  Clear any mask.
     */

    if (blendPtr->gc == NULL) {
	v.background = 0;
	v.foreground = 1;
	v.function = GXset;
	v.graphics_exposures = False;
	v.subwindow_mode = IncludeInferiors;
	vmask = GCBackground|GCForeground|GCFunction|
		GCGraphicsExposures|GCSubwindowMode;
	blendPtr->gc = XCreateGC (display, pixmap, vmask, &v);
    }
    else {
	v.clip_mask = None;
	v.function = GXset;
	vmask = GCClipMask|GCFunction;
        XChangeGC (display, blendPtr->gc, vmask, &v);
    }

    XFillRectangle (display, pixmap, blendPtr->gc,
		    0, 0, mixedIcon->drag.width, mixedIcon->drag.height);

    if (mask != XmUNSPECIFIED_PIXMAP) {

	if (cursorBlend->gc == NULL) {
	    v.background = 0;
	    v.foreground = 1;
	    v.function = GXclear;
	    v.graphics_exposures = False;
	    v.subwindow_mode = IncludeInferiors;
	    vmask = GCBackground|GCForeground|GCFunction|
		    GCGraphicsExposures|GCSubwindowMode;
	    cursorBlend->gc = XCreateGC (display, mask, vmask, &v);
	}
	else {
	    v.function = GXclear;
	    vmask = GCFunction;
	    if (cursorBlend->gc != blendPtr->gc) {
	       v.clip_mask = None;
	       vmask |= GCClipMask;
            }
            XChangeGC (display, cursorBlend->gc, vmask, &v);
        }

        /* Bandaid patch,  because the pixmap may be larger than
           requested,  and something copies out of the valid area,
           we'll clear the maximum area */
	XFillRectangle (display, mixedIcon->drag.mask, cursorBlend->gc,
		        0, 0, PIXMAP_MAX_WIDTH, PIXMAP_MAX_HEIGHT);
    }

    
    /*
     *  Blend the icons into mixedIcon.
     */

    BlendIcon (display, sourceIcon, mixedIcon,
	       sourceX, sourceY, cursorBlend->gc, blendPtr->gc);
    blendPtr->sourceX = sourceX;
    blendPtr->sourceY = sourceY;

    if (stateIcon) {
        BlendIcon (display, stateIcon, mixedIcon,
		   stateX, stateY, cursorBlend->gc, blendPtr->gc);
    }

    if (opIcon) {
        BlendIcon (display, opIcon, mixedIcon,
		   opX, opY, cursorBlend->gc, blendPtr->gc);
    }

    if (mixedIcon->drag.region != NULL) {
       XRectangle rect;

       if (mixedIcon->drag.restore_region != NULL)
          XDestroyRegion(mixedIcon->drag.restore_region);

       mixedIcon->drag.restore_region = XCreateRegion();

       rect.x = 0;
       rect.y = 0;
       rect.width = mixedIcon->drag.width;
       rect.height = mixedIcon->drag.height;

       XUnionRectWithRegion(&rect, mixedIcon->drag.restore_region,
                            mixedIcon->drag.restore_region);
       XSubtractRegion(mixedIcon->drag.restore_region, mixedIcon->drag.region,
                       mixedIcon->drag.restore_region);
    }

    if (do_cache) {
      CacheMixedIcon(dos, depth, width, height, sourceIcon, stateIcon, opIcon,
		   sourceX, sourceY, stateX, stateY, opX, opY, mixedIcon);
    } /* do_cache */
}

/************************************************************************
 *
 *  FitsInCursor ()
 *
 ***********************************************************************/

static Boolean
#ifdef _NO_PROTO
FitsInCursor( dos, sourceIcon, stateIcon, opIcon )
    XmDragOverShellWidget	dos;
    XmDragIconObject		sourceIcon;
    XmDragIconObject		stateIcon;
    XmDragIconObject		opIcon;
#else
FitsInCursor(
    XmDragOverShellWidget	dos,
    XmDragIconObject		sourceIcon,
    XmDragIconObject		stateIcon,
    XmDragIconObject		opIcon)
#endif /* _NO_PROTO */
{
    Dimension		maxWidth, maxHeight;
    Dimension		width, height;

    if (((sourceIcon->drag.depth != 1) || 
	 (sourceIcon->drag.pixmap == XmUNSPECIFIED_PIXMAP))) {
	return False;
    }
    
    MixedIconSize (dos, sourceIcon, stateIcon, opIcon, &width, &height);
    _XmGetMaxCursorSize((Widget)dos, &maxWidth, &maxHeight);

    if (width > maxWidth || height > maxHeight) {
	return False;
    }
    return True;
}

/************************************************************************
 *
 *  GetDragIconColors ()
 *
 *  Gets the cursor colors.  Creates or recolors the root's gc.
 ***********************************************************************/

static Boolean
#ifdef _NO_PROTO
GetDragIconColors( dos )
    XmDragOverShellWidget	dos;
#else
GetDragIconColors(
    XmDragOverShellWidget	dos )
#endif /* _NO_PROTO */
{
    XmDragContext	dc = (XmDragContext)XtParent(dos);
    Screen		*screen = XtScreen(dos);
    Display		*display = XtDisplay(dos);
    Boolean		doChange = False;
    Pixel		fg;
    Pixel		bg;
    XGCValues	        v;
    unsigned long	vmask;
    XColor 		colors[2];
    Colormap		colormap;

    colormap = dc->core.colormap;
    bg = dc->drag.cursorBackground;
    switch ((int) dos->drag.cursorState) {

	case XmVALID_DROP_SITE:
            fg = dc->drag.validCursorForeground;
	    break;

	case XmINVALID_DROP_SITE:
            fg = dc->drag.invalidCursorForeground;
	    break;

	default:
	    _XmWarning ((Widget) dos, MESSAGE3);
	case XmNO_DROP_SITE:
            fg = dc->drag.noneCursorForeground;
	    break;
    }

    /*
     *  Find the best RGB fit for fg and bg on the current screen.
     *  If XAllocColor() fails or if the fitted fg and bg are the same on the
     *  current screen, use black and white respectively.
     */

    colors[0].pixel = fg;
    colors[1].pixel = bg;
    XQueryColors(display, colormap, colors, 2);

    fg = BlackPixelOfScreen(screen);
    bg = WhitePixelOfScreen(screen);
    if (XAllocColor(display, DefaultColormapOfScreen(screen), &colors[0]) &&
        XAllocColor(display, DefaultColormapOfScreen(screen), &colors[1])) {
	fg = colors[0].pixel;
	bg = colors[1].pixel;

    	if (fg == bg) {
    	    fg = BlackPixelOfScreen(screen);
    	    bg = WhitePixelOfScreen(screen);
    	}
    }

    /*
     *  Create or recolor the root's gc.
     *  The cursorForeground and cursorBackground are first set
     *  when the root's gc is created.
     */

    if (dos->drag.rootBlend.gc == NULL) {
	doChange = True;
	v.background = dos->drag.cursorBackground = bg;
	v.foreground = dos->drag.cursorForeground = fg;
	v.graphics_exposures = False;
	v.subwindow_mode = IncludeInferiors;
	vmask = GCBackground|GCForeground|
		GCGraphicsExposures|GCSubwindowMode;
	dos->drag.rootBlend.gc =
	    XCreateGC (display, RootWindowOfScreen(screen), vmask, &v);
    }
    else if (dos->drag.cursorBackground != bg ||
	     dos->drag.cursorForeground != fg) {
	doChange = True;
	v.background = dos->drag.cursorBackground = bg;
	v.foreground = dos->drag.cursorForeground = fg;
	vmask = GCBackground|GCForeground;
	XChangeGC (display, dos->drag.rootBlend.gc, vmask, &v);
    }
    return (doChange);
}

/************************************************************************
 *
 *  GetDragIconCursor ()
 *
 *  Tries to create a pixmap cursor with correct colors.
 *  Creates and/or colors the root's gc.
 ***********************************************************************/

static Cursor
#ifdef _NO_PROTO
GetDragIconCursor( dos, sourceIcon, stateIcon, opIcon, clip, dirty )
    XmDragOverShellWidget	dos;
    XmDragIconObject		sourceIcon;	/* nonNULL */
    XmDragIconObject		stateIcon;
    XmDragIconObject		opIcon;
    Boolean			clip;
    Boolean			dirty;
#else
GetDragIconCursor(
    XmDragOverShellWidget	dos,
    XmDragIconObject		sourceIcon,
    XmDragIconObject		stateIcon,
    XmDragIconObject		opIcon,
#if NeedWidePrototypes
    int				clip,
    int				dirty)
#else
    Boolean			clip,
    Boolean			dirty)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Screen			*screen = XtScreen(dos);
    Display			*display = XtDisplay(dos);
    XmDragCursorCache		*cursorCachePtr= NULL;
    register XmDragCursorCache	cursorCache = NULL;
    XColor 			colors[2];
    Boolean			useCache = True;
    Cursor			cursor;

    /*
     *  If the cursor doesn't fit and we cannot clip, return None.
     *  Don't look in the cursorCache -- the cursor would only be there
     *    if clipping were allowed.
     */
    
    if (!clip && !FitsInCursor (dos, sourceIcon, stateIcon, opIcon)) {
	return None;
    }

    /*
     *  Don't use the cursorCache with stateIcon attachment XmATTACH_HOT
     *  (opIcon attachment XmATTACH_HOT is OK).
     */

    colors[0].pixel = dos->drag.cursorForeground;
    colors[1].pixel = dos->drag.cursorBackground;
    XQueryColors(display, DefaultColormapOfScreen(screen), colors, 2);

    if (stateIcon && stateIcon->drag.attachment == XmATTACH_HOT) {
	useCache = False;
    }
    else {
	cursorCachePtr =
	    _XmGetDragCursorCachePtr((XmScreen)XmGetXmScreen(screen));

	cursorCache = *cursorCachePtr;
	while (cursorCache) {
            if ((cursorCache->stateIcon == stateIcon) &&
                (cursorCache->opIcon == opIcon) &&
	        (cursorCache->sourceIcon == sourceIcon)) {

		/*
		 *  Found a cursorCache match.
		 *  If any of the icons were dirty, replace this cursor.
		 *  Otherwise, use it.
		 */

		if (dirty) {
		    break;
		}

	        /* recolor the cursor */
	        XRecolorCursor (display, cursorCache->cursor,
			        &colors[0], /* foreground_color */ 
			        &colors[1]  /* background_color */);
	        return cursorCache->cursor;
	    }
	    else {
	        cursorCache = cursorCache->next;
	    }
	}
    }

    /*
     *  We didn't find a valid match in the cursorCache.
     *  Blend the icons and create a pixmap cursor.
     */

    MixIcons (dos, sourceIcon, stateIcon, opIcon,
	      &dos->drag.cursorBlend, clip);

    cursor =
	XCreatePixmapCursor (display,
			     dos->drag.cursorBlend.mixedIcon->drag.pixmap,
	((dos->drag.cursorBlend.mixedIcon->drag.mask == XmUNSPECIFIED_PIXMAP) ?
		None : dos->drag.cursorBlend.mixedIcon->drag.mask),
			     &colors[0], /* foreground_color */
			     &colors[1], /* background_color */
			     dos->drag.cursorBlend.mixedIcon->drag.hot_x,
			     dos->drag.cursorBlend.mixedIcon->drag.hot_y);

    /*
     *  Cache the cursor if using the cursorCache.  If the cached cursor
     *    was dirty, replace it.  Otherwise, create a new cursorCache entry
     *    at the head of the cache.
     *  Otherwise, save it and free any previously saved cursor.
     */

    if (useCache) {
	if (cursorCache) {
	    XFreeCursor (display, cursorCache->cursor);
	}
	else {
	    cursorCache = XtNew(XmDragCursorRec);
	    cursorCache->sourceIcon = sourceIcon;
	    cursorCache->stateIcon = stateIcon;
	    cursorCache->opIcon = opIcon;
	    cursorCache->next = *cursorCachePtr;
	    *cursorCachePtr = cursorCache;
	}
	cursorCache->cursor = cursor;
    }
    else {
        if (dos->drag.ncCursor != None) {
	    XFreeCursor (display, dos->drag.ncCursor);
        }
        dos->drag.ncCursor = cursor;
    }

    return cursor;
}

/************************************************************************
 *
 *  Initialize ()
 *
 ***********************************************************************/

static void
#ifdef _NO_PROTO
Initialize( req, new_w, args, numArgs )
    Widget	req;
    Widget	new_w;
    ArgList	args;
    Cardinal	*numArgs;
#else
Initialize(
    Widget	req,
    Widget	new_w,
    ArgList	args,
    Cardinal	*numArgs)
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget)new_w;

    /* Assure that *geometry will never affect this widget,  CR 5778 */
    dos->shell.geometry = NULL;

    dos->drag.opIcon = dos->drag.stateIcon = NULL;
    dos->drag.cursorBlend.gc =
	dos->drag.rootBlend.gc = NULL;
    dos->drag.cursorBlend.sourceIcon =
	dos->drag.cursorBlend.mixedIcon =
	    dos->drag.rootBlend.sourceIcon =
		dos->drag.rootBlend.mixedIcon = NULL;
    dos->drag.backing.pixmap = dos->drag.tmpPix =
	dos->drag.tmpBit = XmUNSPECIFIED_PIXMAP;
    
    dos->drag.initialX = dos->drag.hotX;
    dos->drag.initialY = dos->drag.hotY;
    dos->drag.ncCursor = None;
    dos->drag.isVisible = False;
    dos->drag.activeCursor = None;

    /*
     *  Width/height are valid only in XmPIXMAP and XmWINDOW active modes.
     */

    dos->core.width = 0;
    dos->core.height = 0;

    dos->drag.activeMode = XmCURSOR;

    /* Get the DragOverShell out of the business of adding and removing
     * grabs for shell modality.
     */
    XtRemoveAllCallbacks( new_w, XmNpopupCallback) ;
    XtRemoveAllCallbacks( new_w, XmNpopdownCallback) ;

    _XmDragOverChange (new_w, XmNO_DROP_SITE);
}

/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/

static Boolean
#ifdef _NO_PROTO
SetValues( current, req, new_w, args, num_args )
    Widget	current;
    Widget	req;
    Widget	new_w;
    ArgList	args;
    Cardinal	*num_args;
#else
SetValues(
    Widget	current,
    Widget	req,
    Widget	new_w,
    ArgList	args,
    Cardinal	*num_args)
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget) new_w;
    XmDragOverShellWidget	oldDos = (XmDragOverShellWidget) current;
    XmDragContext		dc = (XmDragContext)XtParent(dos);

    /*
     *  A mode change handles a change in hotspot automatically.
     *  If we are in XmPIXMAP mode and we haven't yet done a root blend,
     *  try XmCURSOR active mode.
     */

    if (oldDos->drag.mode != dos->drag.mode && dc->drag.blendModel != XmBLEND_NONE) {
	if ((dos->drag.mode == XmPIXMAP) &&
	    (dos->drag.rootBlend.sourceIcon == NULL)) {
	    ChangeActiveMode(dos, XmCURSOR);
	}
	else {
	    ChangeActiveMode(dos, dos->drag.mode);
	}
    }
    else if ((dos->drag.hotX != oldDos->drag.hotX) ||
             (dos->drag.hotY != oldDos->drag.hotY)) {
	_XmDragOverMove (new_w, dos->drag.hotX, dos->drag.hotY);
    }
    return False;
}

/************************************************************************
 *
 *  DrawIcon ()
 *
 ************************************************************************/

static void
#ifdef _NO_PROTO
DrawIcon( dos, icon, window, x, y )
    XmDragOverShellWidget	dos;
    XmDragIconObject		icon;
    Window			window;
    Position			x;
    Position			y;
#else
DrawIcon(
    XmDragOverShellWidget	dos,
    XmDragIconObject		icon,
    Window			window,
#if NeedWidePrototypes
    int				x,
    int				y)
#else
    Position			x,
    Position			y)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    GC		draw_gc = dos->drag.rootBlend.gc;
    Boolean	clipped = False;
    XGCValues	        v;
    unsigned long	vmask;
    Display * 	display = XtDisplay((Widget)dos);

    v.function = GXcopy;
    vmask = GCFunction;

    if (icon->drag.region == NULL &&
        icon->drag.mask != XmUNSPECIFIED_PIXMAP) {
	v.clip_mask = icon->drag.mask;
	v.clip_x_origin = x;
	v.clip_y_origin = y;
	vmask |= GCClipMask|GCClipXOrigin|GCClipYOrigin;
	XChangeGC (display, draw_gc, vmask, &v);
	clipped = True;
    }
    else {
        if (icon->drag.region != NULL) {
	   XSetRegion(display, draw_gc, icon->drag.region);
	   v.clip_x_origin = x;
	   v.clip_y_origin = y;
	   vmask |= GCClipXOrigin|GCClipYOrigin;
	   XChangeGC (display, draw_gc, vmask, &v);
	   clipped = True;
        }
        else {
	   v.clip_mask = None;
	   vmask |= GCClipMask;
	   XChangeGC (display, draw_gc, vmask, &v);
        }
    }

    /*
     *  If the icon is from the cursorBlend, treat the icon as a bitmap
     *  and use XCopyPlane.  Otherwise, treat it as a pixmap and use
     *  XCopyArea.  The distinction is important -- the icon data are
     *  treated differently by XCopyPlane and XCopyArea.
     */

    if (icon == dos->drag.cursorBlend.mixedIcon) {
	XCopyPlane(display,
		   icon->drag.pixmap, window, draw_gc,
		   0, 0,
		   dos->core.width, dos->core.height,
		   x, y, 1L);
    }
    else if (icon->drag.depth == dos->core.depth) {
	XCopyArea(display,
		  icon->drag.pixmap, window, draw_gc,
		  0, 0,
		  dos->core.width, dos->core.height,
		  x, y);
    }
    else {
	_XmWarning ((Widget) icon, MESSAGE1); /* cast ok here */
    }

    if (clipped) {
        XSetClipMask (display, draw_gc, None);
    }
}

/************************************************************************
 *
 *  Redisplay ()
 *
 *  Called in XmWINDOW mode only.
 ***********************************************************************/

static void
#ifdef _NO_PROTO
Redisplay( wid, event, region )
    Widget wid ;
    XEvent *event ;	
    Region region ;
#else
Redisplay(
    Widget wid,
    XEvent *event,
    Region region)
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget)wid;

    DrawIcon (dos, 
	      (dos->drag.rootBlend.mixedIcon ?
	       dos->drag.rootBlend.mixedIcon :
               dos->drag.cursorBlend.mixedIcon),
	      XtWindow(dos), 0, 0);
}

/************************************************************************
 *
 *  _XmDragOverHide ()
 *
 ***********************************************************************/

void
#ifdef _NO_PROTO
_XmDragOverHide( w, clipOriginX, clipOriginY, clipRegion )
    Widget	w;
    Position	clipOriginX;
    Position	clipOriginY;
    XmRegion	clipRegion;
#else
_XmDragOverHide(
    Widget	w,
#if NeedWidePrototypes
    int		clipOriginX,
    int		clipOriginY,
#else
    Position	clipOriginX,
    Position	clipOriginY,
#endif /* NeedWidePrototypes */
    XmRegion	clipRegion )
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget) w;
    XmDragContext	dc = (XmDragContext)XtParent(dos);
    Boolean		clipped = False;

    if (dos->drag.isVisible &&
	dc->drag.blendModel != XmBLEND_NONE &&
	dos->drag.activeMode != XmCURSOR) {

	if (dos->drag.activeMode == XmWINDOW) {
	    XtPopdown(w);
	}

	if (dos->drag.activeMode != XmWINDOW && clipRegion != None) {
	    clipped = True;
	    _XmRegionSetGCRegion (XtDisplay(w),
                                  dos->drag.rootBlend.gc,
				  clipOriginX, clipOriginY, clipRegion);
	}
	else {
	    XSetClipMask (XtDisplay(w),
                          dos->drag.rootBlend.gc, None);
	}

	if (BackingPixmap(dos) != XmUNSPECIFIED_PIXMAP) {
	    XCopyArea (XtDisplay(w),
	               BackingPixmap(dos),
	               RootWindowOfScreen(XtScreen(w)),
		       dos->drag.rootBlend.gc,
	               0, 0, dos->core.width, dos->core.height,
	               BackingX(dos), BackingY(dos));
	}

        if (clipped) {
            XSetClipMask (XtDisplay(w), 
		          dos->drag.rootBlend.gc, None);
        }
	dos->drag.isVisible = False;
    }
}

/************************************************************************
 *
 *  _XmDragOverShow ()
 *
 ***********************************************************************/

void
#ifdef _NO_PROTO
_XmDragOverShow( w, clipOriginX, clipOriginY, clipRegion )
    Widget	w;
    Position	clipOriginX;
    Position	clipOriginY;
    XmRegion	clipRegion;
#else
_XmDragOverShow(
    Widget w,
#if NeedWidePrototypes
    int clipOriginX,
    int clipOriginY,
#else
    Position clipOriginX,
    Position clipOriginY,
#endif /* NeedWidePrototypes */
    XmRegion			clipRegion )
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget) w;
    Display		*display = XtDisplay(w);
    XmDragContext	dc = (XmDragContext)XtParent(dos);
    Boolean		clipped = False;

    if (!dos->drag.isVisible &&
	dc->drag.blendModel != XmBLEND_NONE &&
	dos->drag.activeMode != XmCURSOR) {

	if (dos->drag.activeMode != XmWINDOW && clipRegion != None) {
	    clipped = True;
	    _XmRegionSetGCRegion (display, dos->drag.rootBlend.gc,
	                          clipOriginX - BackingX(dos),
	                          clipOriginY - BackingY(dos),
				  clipRegion);
	}
	else {
	    XSetClipMask (display, dos->drag.rootBlend.gc, None);
	}

	XCopyArea (display, RootWindowOfScreen(XtScreen(w)),
	           BackingPixmap(dos),
	           dos->drag.rootBlend.gc,
	           BackingX(dos), BackingY(dos),
	           dos->core.width, dos->core.height,
	           0, 0);

        if (clipped) {
            XSetClipMask (display, dos->drag.rootBlend.gc, None);
        }

	if (dos->drag.activeMode == XmPIXMAP) {
	    DrawIcon (dos,
	              (dos->drag.rootBlend.mixedIcon ?
	               dos->drag.rootBlend.mixedIcon :
                       dos->drag.cursorBlend.mixedIcon),
		      RootWindowOfScreen(XtScreen(w)),
		      dos->core.x, dos->core.y);
	}
	else {
    	    XtPopup(w, XtGrabNone);
	    /*
	     * don't call thru class record since VendorS bug may be
	     * causing override
	     */
	    Redisplay(w, NULL, NULL);
	}
	dos->drag.isVisible = True;
    }
}

/************************************************************************
 *
 *  Destroy ()
 *
 *  Destroy method for the XmDragOverShellClass.
 *  Hide the XmDragOverShell before destroying it.
 ***********************************************************************/

static void
#ifdef _NO_PROTO
Destroy( w )
    Widget	w;
#else
Destroy(
    Widget	w)
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget)w;
    Display			*display = XtDisplay((Widget)dos);
    XmScreen			xmScreen =
				    (XmScreen) XmGetXmScreen(XtScreen(dos));

    _XmDragOverHide (w, 0, 0, None);

    if (dos->drag.rootBlend.mixedIcon) {
	DestroyMixedIcon (dos, dos->drag.rootBlend.mixedIcon);
    }
    if (dos->drag.rootBlend.gc) {
        XFreeGC (display, dos->drag.rootBlend.gc);
    }

    if (dos->drag.cursorBlend.mixedIcon) {
	DestroyMixedIcon (dos, dos->drag.cursorBlend.mixedIcon);
    }
    if (dos->drag.cursorBlend.gc) {
        XFreeGC (display, dos->drag.cursorBlend.gc);
    }

    if (BackingPixmap(dos) != XmUNSPECIFIED_PIXMAP) {
	_XmFreeScratchPixmap (xmScreen, BackingPixmap(dos));
    }
    if (dos->drag.tmpPix != XmUNSPECIFIED_PIXMAP) {
	_XmFreeScratchPixmap (xmScreen, dos->drag.tmpPix);
    }
    if (dos->drag.tmpBit != XmUNSPECIFIED_PIXMAP) {
	_XmFreeScratchPixmap (xmScreen, dos->drag.tmpBit);
    }

    if (dos->drag.ncCursor != None) {
	XFreeCursor (display, dos->drag.ncCursor);
    }
}

/************************************************************************
 *
 *  ChangeActiveMode ()
 *
 ***********************************************************************/

static void 
#ifdef _NO_PROTO
ChangeActiveMode(dos, newActiveMode)
    XmDragOverShellWidget	dos;
    unsigned char		newActiveMode;
#else
ChangeActiveMode(
    XmDragOverShellWidget	dos,
#if NeedWidePrototypes
    unsigned int		newActiveMode)
#else
    unsigned char		newActiveMode)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Display		*display = XtDisplay((Widget)dos);
    XmDragContext	dc = (XmDragContext)XtParent(dos);
    GC			draw_gc = dos->drag.rootBlend.gc;

    /*
     *  Remove the effects of the current active mode.
     */

    if (dos->drag.activeMode == XmCURSOR) {
	if (newActiveMode != XmCURSOR) {
	    dos->drag.activeCursor = _XmGetNullCursor((Widget)dos);
	    XChangeActivePointerGrab (display,
				      (unsigned int) _XmDRAG_EVENT_MASK(dc),
				      dos->drag.activeCursor,
				      dc->drag.lastChangeTime);
	}
    }
    else {
	/*
	 *  BackingPixmap was created and core.width/height were set
	 *  the first time we entered XmPIXMAP or XmWINDOW active mode.
	 */

        if (dos->drag.activeMode == XmWINDOW) {
	    XtPopdown((Widget)dos);
	}
	XSetClipMask (display, draw_gc, None);
	if (BackingPixmap(dos) != XmUNSPECIFIED_PIXMAP) {
            XCopyArea (display, BackingPixmap(dos),
	               RootWindowOfScreen(XtScreen(dos)), draw_gc,
	               0, 0,
		       dos->core.width, dos->core.height,
	               BackingX(dos), BackingY(dos));
	}
    }
    dos->drag.isVisible = False;

    /*
     *  Add the effects of the new active mode.
     */
    
    if ((dos->drag.activeMode = newActiveMode) == XmCURSOR) {
	_XmDragOverChange ((Widget)dos, dos->drag.cursorState);
    }
    else {
	XmScreen		xmScreen = (XmScreen)
				    XmGetXmScreen(XtScreen(dos));
	XmDragIconObject	sourceIcon;
	XmDragOverBlend		blend;

	/*
	 *  (Re)generate a mixedIcon, in case we have a state
	 *  change, or we have used the cursor cache up to this
	 *  point.  Place the new mixedIcon so as to preserve the
	 *  hotspot location.
	 */

        if (dos->drag.rootBlend.sourceIcon) {
	    sourceIcon = dos->drag.rootBlend.sourceIcon;
	    blend = &dos->drag.rootBlend;
        }
        else {
	    sourceIcon = dos->drag.cursorBlend.sourceIcon;
	    blend = &dos->drag.cursorBlend;
        }
        MixIcons (dos, sourceIcon, dos->drag.stateIcon, dos->drag.opIcon,
		  blend, False);

        /*
	 *  Compute the new location and handle an icon size change.
         */

        BackingX(dos) = dos->core.x =
	    dos->drag.hotX - blend->mixedIcon->drag.hot_x;
        BackingY(dos) = dos->core.y =
	    dos->drag.hotY - blend->mixedIcon->drag.hot_y;

	if (dos->core.width != blend->mixedIcon->drag.width ||
	    dos->core.height != blend->mixedIcon->drag.height) {

	    dos->core.width = blend->mixedIcon->drag.width;
	    dos->core.height = blend->mixedIcon->drag.height;
            if (BackingPixmap(dos) != XmUNSPECIFIED_PIXMAP) {
		_XmFreeScratchPixmap (xmScreen, BackingPixmap(dos));
		BackingPixmap(dos) = XmUNSPECIFIED_PIXMAP;
	    }
	    if (dos->drag.tmpPix != XmUNSPECIFIED_PIXMAP) {
		_XmFreeScratchPixmap (xmScreen, dos->drag.tmpPix);
                dos->drag.tmpPix = XmUNSPECIFIED_PIXMAP;
	    }
	    if (dos->drag.tmpBit != XmUNSPECIFIED_PIXMAP) {
		_XmFreeScratchPixmap (xmScreen, dos->drag.tmpBit);
                dos->drag.tmpBit = XmUNSPECIFIED_PIXMAP;
	    }
	}

        /*
         *  Save the obscured root in backing.
         */

        if (BackingPixmap(dos) == XmUNSPECIFIED_PIXMAP) {
	    BackingPixmap(dos) =
		_XmAllocScratchPixmap (xmScreen, dos->core.depth,
			               dos->core.width, dos->core.height);
        }

        XSetClipMask (display, draw_gc, None);
        XCopyArea (display, RootWindowOfScreen(XtScreen(dos)),
		   BackingPixmap(dos), draw_gc,
		   BackingX(dos), BackingY(dos),
		   dos->core.width, dos->core.height, 0, 0);

	if (dos->drag.activeMode == XmPIXMAP) {
	    DrawIcon (dos, blend->mixedIcon,
		      RootWindowOfScreen(XtScreen(dos)),
		      dos->core.x, dos->core.y);
	}
	else {
	    XSetWindowAttributes  xswa;

    	    XtPopup((Widget)dos, XtGrabNone);
	    xswa.cursor = _XmGetNullCursor((Widget)dos);
	    XChangeWindowAttributes (display, XtWindow(dos),
	                             CWCursor, &xswa);
	    /*
	     * don't call thru class record since VendorS bug may be
	     * causing override
	     */
	    Redisplay((Widget)dos, NULL, NULL);
	}
	dos->drag.isVisible = True;
    }
}

/************************************************************************
 *
 *  ChangeDragWindow ()
 *
 ***********************************************************************/

static void 
#ifdef _NO_PROTO
ChangeDragWindow(dos)
    XmDragOverShellWidget	dos;
#else
ChangeDragWindow(
    XmDragOverShellWidget	dos)
#endif /* _NO_PROTO */
{
    Display		*display = XtDisplay((Widget)dos);
    Window		win = XtWindow((Widget)dos);
    GC			draw_gc = dos->drag.rootBlend.gc;
    XmScreen		xmScreen = (XmScreen) XmGetXmScreen(XtScreen(dos));
    XmDragIconObject	sourceIcon;
    XmDragOverBlend	blend;
    XmDragIconObject    mixedIcon;
    XmDragOverBlendRec	*cursorBlend = &dos->drag.cursorBlend;
    XGCValues           v;
    unsigned long       vmask;


    /*
     *  Blend a new mixedIcon using only the source icon.
     *  Place the new mixedIcon to preserve the source icon location.
     *  The current mixedIcon data is valid.
     */

    if (dos->drag.rootBlend.sourceIcon) {
	sourceIcon = dos->drag.rootBlend.sourceIcon;
	blend = &dos->drag.rootBlend;
    } else {
	sourceIcon = dos->drag.cursorBlend.sourceIcon;
	blend = &dos->drag.cursorBlend;
    }
    mixedIcon = blend->mixedIcon;

    XSetFunction (display, blend->gc, GXset);
    XFillRectangle (display, mixedIcon->drag.pixmap, blend->gc,
                    0, 0, mixedIcon->drag.width, mixedIcon->drag.height);

    if (mixedIcon->drag.mask != XmUNSPECIFIED_PIXMAP) {
       if (cursorBlend->gc == NULL) {
          v.background = 0;
          v.foreground = 1;
	  v.function = GXclear;
          v.graphics_exposures = False;
          v.subwindow_mode = IncludeInferiors;
	  vmask = GCBackground|GCForeground|GCFunction|
                  GCGraphicsExposures|GCSubwindowMode;
          cursorBlend->gc = XCreateGC (display, mixedIcon->drag.mask,
				       vmask, &v);
        } else {
	    v.clip_mask = None;
	    v.function = GXclear;
	    vmask = GCClipMask|GCFunction;
            XChangeGC (display, cursorBlend->gc, vmask, &v);
        }
        XFillRectangle (display, mixedIcon->drag.mask, cursorBlend->gc,
                        0, 0, mixedIcon->drag.width, mixedIcon->drag.height);
    }

    BlendIcon (display, sourceIcon, mixedIcon, blend->sourceX,
	       blend->sourceY, cursorBlend->gc, blend->gc);

    /*
     *  Remove the current drag window.
     */

    XUnmapWindow(display, win);
    XSetClipMask (display, draw_gc, None);
    if (BackingPixmap(dos) != XmUNSPECIFIED_PIXMAP) {
        XCopyArea (display, BackingPixmap(dos),
	           RootWindowOfScreen(XtScreen(dos)), draw_gc,
	           0, 0,
		   dos->core.width, dos->core.height,
	           BackingX(dos), BackingY(dos));
    }

    /*
     *  Handle an icon size change.
     */

    if (dos->core.width != blend->mixedIcon->drag.width ||
	dos->core.height != blend->mixedIcon->drag.height) {

/*
*/
        if (BackingPixmap(dos) != XmUNSPECIFIED_PIXMAP) {
	    _XmFreeScratchPixmap (xmScreen, BackingPixmap(dos));
	    BackingPixmap(dos) = XmUNSPECIFIED_PIXMAP;
	}
	if (dos->drag.tmpPix != XmUNSPECIFIED_PIXMAP) {
	    _XmFreeScratchPixmap (xmScreen, dos->drag.tmpPix);
            dos->drag.tmpPix = XmUNSPECIFIED_PIXMAP;
	}
	if (dos->drag.tmpBit != XmUNSPECIFIED_PIXMAP) {
	    _XmFreeScratchPixmap (xmScreen, dos->drag.tmpBit);
            dos->drag.tmpBit = XmUNSPECIFIED_PIXMAP;
	}
    }

    /*
     *  Save the obscured root in backing.
     */

    if (BackingPixmap(dos) == XmUNSPECIFIED_PIXMAP) {
	BackingPixmap(dos) =
	    _XmAllocScratchPixmap (xmScreen, dos->core.depth,
			           dos->core.width, dos->core.height);
    }
    BackingX(dos) = dos->core.x;
    BackingY(dos) = dos->core.y;

    XSetClipMask (display, draw_gc, None);
    XCopyArea (display, RootWindowOfScreen(XtScreen(dos)),
	       BackingPixmap(dos), draw_gc,
	       BackingX(dos), BackingY(dos),
	       dos->core.width, dos->core.height, 0, 0);

    /*
     *  Move, resize, and remap the drag window.
     *  This is an override_redirect window.
     */

    XMoveResizeWindow(display, win, dos->core.x, dos->core.y,
	              dos->core.width, dos->core.height);
    XMapWindow(display, win);
    /*
     * don't call thru class record since VendorS bug may be
     * causing override
     */
    Redisplay((Widget)dos, NULL, NULL);
}

/************************************************************************
 *
 *  _XmDragOverMove ()
 *
 *  This method is less efficient than the obvious method of copying the
 *  backing to the root, saving the new icon destination in the backing,
 *  and copying the icon to the root.  However, it results in a smoother
 *  appearance.
 ***********************************************************************/

void
#ifdef _NO_PROTO
_XmDragOverMove( w, x, y )
    Widget	w;
    Position	x;	/* hot_x */
    Position	y;	/* hot_y */
#else
_XmDragOverMove(
    Widget	w,
#if NeedWidePrototypes
    int		x,
    int		y)
#else
    Position	x,
    Position	y)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget) w;
    XmDragContext	dc = (XmDragContext)XtParent(dos);
    Display		*display = XtDisplay(w);
    XmScreen		xmScreen = (XmScreen) XmGetXmScreen(XtScreen(w));
    Window		root = RootWindowOfScreen(XtScreen(w));
    Pixmap		old_backing = BackingPixmap(dos);
    Pixmap		new_backing;
    GC			draw_gc = dos->drag.rootBlend.gc;
    XmDragIconObject	mixedIcon;
    XGCValues           v;
    unsigned long       vmask;

    dos->drag.hotX = x;
    dos->drag.hotY = y;

    if (!dos->drag.isVisible ||
	dc->drag.blendModel == XmBLEND_NONE ||
	dos->drag.activeMode == XmCURSOR) {
        return;
    }

    if (dos->drag.rootBlend.mixedIcon) {
        mixedIcon = dos->drag.rootBlend.mixedIcon;
    }
    else {	/* exists */
        mixedIcon = dos->drag.cursorBlend.mixedIcon;
    }

    dos->core.x = x -= mixedIcon->drag.hot_x;
    dos->core.y = y -= mixedIcon->drag.hot_y;

    if (dos->drag.activeMode == XmWINDOW) {
	/* this is an override_redirect window */
        XMoveWindow(display, XtWindow (w), x, y);
    }
    
    /*
     *  From here on, the active mode is XmPIXMAP.
     */

    if (dos->drag.tmpPix == XmUNSPECIFIED_PIXMAP) {
        dos->drag.tmpPix =
	    _XmAllocScratchPixmap (xmScreen, dos->core.depth,
	                           dos->core.width, dos->core.height);
    }
    new_backing = dos->drag.tmpPix;

    /*
     *  Save the area where the new icon is to go.
     */
    
    v.clip_mask = None;
    v.function = GXcopy;
    vmask = GCClipMask|GCFunction;
    XChangeGC (display, draw_gc, vmask, &v);
    XCopyArea (display, root, new_backing, draw_gc,
	       x, y, dos->core.width, dos->core.height, 0, 0);
    
    if (x + ((Position) dos->core.width) > BackingX(dos) &&
	x < BackingX(dos) + ((Position) dos->core.width) &&
	y + ((Position) dos->core.height) > BackingY(dos) && 
	y < BackingY(dos) + ((Position) dos->core.height)) {

	XRectangle 	rect, rect1, rect2;
	XPoint 		pt;

	/*
	 *  Have overlap:
	 *
	 *  Calculate the intersection between the 2 areas and 
	 *  copy the non-overlapping old area in the window.
	 *
         *  If the icon has a mask, create a mask through which we will
	 *  copy the old backing to the root.
	 *  Otherwise, use one or two rectangles.
	 */
    
        if (mixedIcon->drag.region == NULL &&
	    mixedIcon->drag.mask != XmUNSPECIFIED_PIXMAP) {

	    Pixmap	root_mask;
	    GC		mask_gc = dos->drag.cursorBlend.gc;

	    if (dos->drag.tmpBit == XmUNSPECIFIED_PIXMAP) {
		dos->drag.tmpBit =
		    _XmAllocScratchPixmap (xmScreen, 1, dos->core.width,
					   dos->core.height);
	    }
	    root_mask = dos->drag.tmpBit;

    	    v.clip_mask = None;
    	    v.function = GXset;
    	    vmask = GCClipMask|GCFunction;
    	    XChangeGC (display, mask_gc, vmask, &v);
	    XFillRectangle (display, root_mask, mask_gc,
		            0, 0, dos->core.width, dos->core.height);

	    XSetFunction (display, mask_gc, GXandInverted);
            XCopyArea (display, mixedIcon->drag.mask, root_mask, mask_gc,
	               0, 0, mixedIcon->drag.width, mixedIcon->drag.height,
	               x - BackingX(dos), y - BackingY(dos));
    
	    /*
	     *  Copy the icon into the new area and refresh the root.
	     */
    
	    DrawIcon (dos, mixedIcon, root, x, y);

    	    v.clip_mask = root_mask;
    	    v.clip_x_origin = BackingX(dos);
    	    v.clip_y_origin = BackingY(dos);
    	    vmask = GCClipMask|GCClipXOrigin|GCClipYOrigin;
    	    XChangeGC (display, draw_gc, vmask, &v);
            XCopyArea (display, old_backing, root, draw_gc,
	               0, 0, dos->core.width, dos->core.height,
	               BackingX(dos), BackingY(dos));
	    XSetClipMask (display, draw_gc, None);
	}
	else {
    
	    /*
	     *  Copy the icon into the new area.
	     */
    
	    DrawIcon (dos, mixedIcon, root, x, y);

	    /*
	     *  Use rectangles to refresh exposed root.
	     *  The first rectangle (horizontal movement).
	     */
	
	    if (x > BackingX(dos)) {
	        rect1.x = 0;
	        rect1.width = x - BackingX(dos);
	    }
	    else {
	        rect1.width = BackingX(dos) - x;
	        rect1.x = dos->core.width - rect1.width;
	    }
	
	    rect1.y = 0;
	    rect1.height = dos->core.height;
	    pt.x = BackingX(dos) + rect1.x;
	    pt.y = BackingY(dos);
	
	    if (rect1.width > 0) {
                XCopyArea (display, old_backing, root, draw_gc,
	                   rect1.x, rect1.y, rect1.width, rect1.height,
		           pt.x, pt.y);
	    }
	
	    /*
	     *  The second rectangle (vertical movement).
	     */
	
	    if (y > BackingY(dos)) {
	        rect2.y = 0;
	        rect2.height = y - BackingY(dos);
	    }
	    else {
	        rect2.height = BackingY(dos) - y;
	        rect2.y = dos->core.height - rect2.height;
	    }
	
	    rect2.x = 0;
	    rect2.width = dos->core.width;
	    pt.x = BackingX(dos);
	    pt.y = BackingY(dos) + rect2.y;
	
	    if (rect2.height > 0) {
                XCopyArea (display, old_backing, root, draw_gc,
	                   rect2.x, rect2.y, rect2.width, rect2.height,
		           pt.x, pt.y);
	    }
	}

	/*
	 *  Copy the overlapping area between old_backing and
	 *  new_backing into new_backing to be used for the
	 *  next cursor move.
	 */
	
	if (x > BackingX(dos)) {
	    rect.x = x - BackingX(dos);
	    pt.x = 0;
	    rect.width = dos->core.width - rect.x;
	}
	else {
	    rect.x = 0;
	    pt.x = BackingX(dos) - x;
	    rect.width = dos->core.width - pt.x;
	}
	
	if (y > BackingY(dos)) {
	    rect.y = y - BackingY(dos);
	    pt.y = 0;
	    rect.height = dos->core.height - rect.y;
	}
	else {
	    rect.y = 0;
	    pt.y = BackingY(dos) - y;
	    rect.height = dos->core.height - pt.y;
	}
	
	XCopyArea (display, old_backing, new_backing, draw_gc,
		   rect.x, rect.y, rect.width, rect.height, pt.x, pt.y);

        if (mixedIcon->drag.restore_region) {
            XSetRegion(display, draw_gc, mixedIcon->drag.restore_region);
            XSetClipOrigin(display, draw_gc, x, y);
            XCopyArea (display, new_backing, root,
                       draw_gc, 0, 0, dos->core.width,
                       dos->core.height, x, y);
            XSetClipMask(display, draw_gc, None);
        }
    }
    else {

	/*
	 *  No overlap:  refresh the root from old_backing.
	 *  new_backing is valid.
	 */

        XCopyArea (display, old_backing, root, draw_gc,
	           0, 0, dos->core.width, dos->core.height,
	           BackingX(dos), BackingY(dos));
    
	DrawIcon (dos, mixedIcon, root, x, y);
    }
    
    /*  Update the variables needed for the next loop  */
    
    BackingX(dos) = x;
    BackingY(dos) = y;
    BackingPixmap(dos) = new_backing;
    dos->drag.tmpPix = old_backing;
}

/************************************************************************
 *
 *  _XmDragOverChange ()
 *
 *  Make dragover changes to track changes in component icons or colors.
 ***********************************************************************/

void
#ifdef _NO_PROTO
_XmDragOverChange( w, dropSiteStatus )
    Widget		w;
    unsigned char	dropSiteStatus;
#else
_XmDragOverChange(
    Widget		w,
#if NeedWidePrototypes
    unsigned int	dropSiteStatus)
#else
    unsigned char	dropSiteStatus)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget) w;
    XmDragContext	dc = (XmDragContext)XtParent(dos);
    XmDragIconObject	sourceIcon = NULL;
    XmDragIconObject	opIcon = NULL;
    XmDragIconObject	stateIcon = NULL;
    Boolean		doChange, dirty = False;
    Boolean		usedSrcPixIcon = True;
    
    dos->drag.cursorState = dropSiteStatus;

    if (dos->drag.mode == XmWINDOW) {
	return;
    }

    if (dc->drag.blendModel == XmBLEND_NONE) {
	return;
    }

    /*
     *  Get the sourceIcon.
     *  If we are in XmPIXMAP mode use the XmNsourcePixmapIcon if:
     *    1. it exists,
     *    2. it has the same screen as dos, and
     *    3. it has depth 1 or the same depth as dos.
     *  Otherwise, use the XmNsourceCursorIcon if:
     *    1. it exists,
     *    2. it has the same screen as dos, and
     *    3. is a bitmap.
     *  Otherwise, use the XmNdefaultSourceCursorIcon.
     */

    if (dos->drag.mode == XmPIXMAP) {
        sourceIcon = dc->drag.sourcePixmapIcon;
    } 

    if (sourceIcon == NULL ||
        XtScreenOfObject(XtParent(sourceIcon)) != XtScreen(w) ||
	((sourceIcon->drag.depth != dos->core.depth) &&
	 (sourceIcon->drag.depth != 1))) {

	usedSrcPixIcon = False;
        sourceIcon = dc->drag.sourceCursorIcon;

	if (sourceIcon == NULL ||
            XtScreenOfObject(XtParent(sourceIcon)) != XtScreen(w) ||
	    sourceIcon->drag.depth != 1) {
	    sourceIcon = _XmScreenGetSourceIcon (w); /* nonNULL */
	}
    }

    /*
     *  Get the state and operation icons, according to the blending model.
     */

    switch ((int) dc->drag.blendModel) {

	default:
	    _XmWarning( (Widget) dc, MESSAGE4);
	case XmBLEND_ALL:
	    /*
	     *  Get the operation icon bitmap.
	     */

            opIcon = dc->drag.operationCursorIcon;

	    if (opIcon == NULL || opIcon->drag.depth != 1 ||
			XtScreenOfObject(XtParent(opIcon)) != XtScreen(w)) {
	    	opIcon = _XmScreenGetOperationIcon (w,
					            dc->drag.operation);
	        if (opIcon && opIcon->drag.depth != 1) {
	            opIcon = NULL;
	        }
	    }

	    /* fall through */

	case XmBLEND_STATE_SOURCE:
	    /*
	     *  Get the state icon bitmap.
	     */

            stateIcon = dc->drag.stateCursorIcon;

	    if (stateIcon == NULL || stateIcon->drag.depth != 1 ||
			XtScreenOfObject(XtParent(stateIcon)) != XtScreen(w)) {
	    	stateIcon = _XmScreenGetStateIcon (w,
						   dropSiteStatus);
	        if (stateIcon && stateIcon->drag.depth != 1) {
	            stateIcon = NULL;
	        }
	    }
	    break;

	case XmBLEND_JUST_SOURCE:
	    break;
    }

    /*
     *  Determine the cursor colors and create or recolor the root's gc.
     *  Record that a change is necessary if the cursor colors or any
     *  of the component icons have changed.
     */

    dirty = (_XmDragIconIsDirty (sourceIcon) ||
             (opIcon && _XmDragIconIsDirty (opIcon)) ||
	     (stateIcon && _XmDragIconIsDirty (stateIcon)));

    doChange = GetDragIconColors (dos) ||
	       dos->drag.opIcon != opIcon ||
               dos->drag.stateIcon != stateIcon ||
               dos->drag.rootBlend.sourceIcon != sourceIcon ||
	       dirty;

    /*
     *  If we are not using the XmNsourcePixmapIcon, then try to create
     *  a cursor from the specified icons.  If we are successful, we will 
     *  use the cursor in both cursor and pixmap mode.
     *  Remember:  XmNsourcePixmapIcon is only used in XmPIXMAP mode.
     */

    dos->drag.opIcon = opIcon;
    dos->drag.stateIcon = stateIcon;
    dos->drag.cursorBlend.sourceIcon = sourceIcon;

    if (!usedSrcPixIcon &&
        (dos->drag.activeCursor =
	     GetDragIconCursor (dos, sourceIcon, stateIcon, opIcon,
				False /* no clip */, dirty))
	     != None) {
	/*
	 *  We have created a new cursor:  clean the icons.
	 */

	_XmDragIconClean (sourceIcon, stateIcon, opIcon);

	if (dos->drag.activeMode != XmCURSOR) {
	    _XmDragOverHide (w, 0, 0, None);
	    dos->drag.activeMode = XmCURSOR;
	}
	XChangeActivePointerGrab (XtDisplay(w), 
				  (unsigned int) _XmDRAG_EVENT_MASK(dc),
				  dos->drag.activeCursor,
				  dc->drag.lastChangeTime);

	/*
	 *  We will use XmCURSOR active mode:  destroy any previously
	 *  used rootBlend icon.
	 */

	dos->drag.rootBlend.sourceIcon = NULL;
	if (dos->drag.rootBlend.mixedIcon) {
	    DestroyMixedIcon (dos, dos->drag.rootBlend.mixedIcon);
	    dos->drag.rootBlend.mixedIcon = NULL;
	}
	return;
    }

    /*
     *  Am using XmNsourcePixmapIcon or the cursor was too big.
     *  Save the sourceIcon for non-XmCURSOR active mode.
     */

    dos->drag.rootBlend.sourceIcon = sourceIcon;

    if (dos->drag.mode == XmCURSOR) {

    	/*
	 *  XmCURSOR mode:  the cursor was too large for the hardware; clip
	 *  it to the maximum hardware cursor size.
	 */

	dos->drag.activeCursor = 
	    GetDragIconCursor (dos, sourceIcon, stateIcon, opIcon,
				True /* clip */, dirty);

	_XmDragIconClean (sourceIcon, stateIcon, opIcon);
	if (dos->drag.activeMode != XmCURSOR) {
	    _XmDragOverHide (w, 0, 0, None);
	    dos->drag.activeMode = XmCURSOR;
	}
	XChangeActivePointerGrab (XtDisplay(w), 
				  (unsigned int) _XmDRAG_EVENT_MASK(dc),
				  dos->drag.activeCursor,
				  dc->drag.lastChangeTime);
    }

    /*
     *  Else unable to use XmCURSOR activeMode in XmPIXMAP mode.
     *  Change activeMode to XmPIXMAP.
     */

    else if (doChange || dos->drag.activeMode != XmPIXMAP){
	_XmDragIconClean (sourceIcon, stateIcon, opIcon);
        ChangeActiveMode (dos, XmPIXMAP);
    }
}

/************************************************************************
 *
 *  _XmDragOverFinish ()
 *
 ***********************************************************************/

void
#ifdef _NO_PROTO
_XmDragOverFinish( w, completionStatus )
    Widget		w;
    unsigned char	completionStatus;
#else
_XmDragOverFinish(
    Widget		w,
#if NeedWidePrototypes
    unsigned int	completionStatus)
#else
    unsigned char	completionStatus)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget) w;
    XmDragContext	dc = (XmDragContext)XtParent(dos);
/*
    GC			draw_gc = dos->drag.rootBlend.gc;
*/

    if (dc->drag.blendModel != XmBLEND_NONE) {

/* If there could be some way to only do this code when we know that
 * there is animation being done under the drag over effects.  Maybe
 * add a XmDROP_ANIMATE completionStatus type?  XmDROP_ANIMATE would
 * be the same as XmDROP_SUCCESS, but it would also indicate that
 * animation is being done.  This code causes unecessary flashing
 * when animation is not being done.  It also fixes a bug the make
 * the melt effects look correct when the area under the icons has
 * changed.

	XFlush (XtDisplay((Widget)dos));
        XSetClipMask (XtDisplay((Widget)dos), draw_gc, None);
	XtPopdown(w);
        XCopyArea (XtDisplay((Widget)dos), RootWindowOfScreen(XtScreen(dos)),
	           BackingPixmap(dos), draw_gc,
	           BackingX(dos), BackingY(dos),
	           dos->core.width, dos->core.height, 0, 0);
    	XtPopup(w, XtGrabNone);
*/
	XGrabServer(XtDisplay(w));

	/* 
	 *  Create and draw a source-only mixedIcon.
	 *  Do not recolor it, even though the state is no longer used.
	 *  Place the source-only mixedIcon so the source doesn't move.
	 *
	 *  The current active mode is XmWINDOW, so the blend data is valid.
	 *  However, the backing may not be, since the server was ungrabbed.
	 *  We keep the active mode as XmWINDOW and delay XtPopDown until
	 *  the finish effects are finished to force the server to generate
	 *  an expose event at the initial mixedIcon location.
	 */

	ChangeDragWindow (dos);

	if (completionStatus == XmDROP_FAILURE) {
	    /* generate zap back effects */
	    DoZapEffect((XtPointer)dos, (XtIntervalId *)NULL);
	}
	else {
	    /* generate melt effects */
	    DoMeltEffect((XtPointer)dos, (XtIntervalId *)NULL);
	}

	XtPopdown(w);
	dos->drag.isVisible = False;
	XUngrabServer(XtDisplay(w));
    }
}

/************************************************************************
 *
 *  _XmDragOverGetActiveCursor ()
 *
 ***********************************************************************/

Cursor
#ifdef _NO_PROTO
_XmDragOverGetActiveCursor( w )
    Widget	w;
#else
_XmDragOverGetActiveCursor(
    Widget	w)
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget) w;
    return dos->drag.activeCursor;
}

/************************************************************************
 *
 *  _XmDragOverSetInitialPosition ()
 *
 ***********************************************************************/

void
#ifdef _NO_PROTO
_XmDragOverSetInitialPosition( w, initialX, initialY )
    Widget	w;
    Position	initialX;
    Position	initialY;
#else
_XmDragOverSetInitialPosition(
    Widget	w,
#if NeedWidePrototypes
    int		initialX,
    int		initialY)
#else
    Position	initialX,
    Position	initialY)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmDragOverShellWidget	dos = (XmDragOverShellWidget) w;
    dos->drag.initialX = initialX;
    dos->drag.initialY = initialY;
}


static void 
#ifdef _NO_PROTO
Realize( wid, vmask, attr )
        Widget wid ;
        XtValueMask *vmask ;
        XSetWindowAttributes *attr ;
#else
Realize(
        Widget wid,
        XtValueMask *vmask,
        XSetWindowAttributes *attr )
#endif /* _NO_PROTO */
{
   WidgetClass super_wc = wid->core.widget_class->core_class.superclass ;
   XSetWindowAttributes attrs;

   (*super_wc->core_class.realize)(wid, vmask, attr);
   attrs.do_not_propagate_mask = ButtonPressMask;
   XChangeWindowAttributes(XtDisplay(wid), XtWindow(wid), 
                           CWDontPropagate, &attrs);
}
