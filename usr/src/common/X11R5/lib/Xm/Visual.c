#pragma ident	"@(#)m1.2libs:Xm/Visual.c	1.4"
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

#include <Xm/XmP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#include <X11/ShellP.h>
#include "MessagesI.h"
#include <stdio.h>
#include <ctype.h>

#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


/* Warning and Error messages */

#ifdef I18N_MSG
#define MESSAGE0	catgets(Xm_catd,MS_VIsual,MSG_VI_1,_XmMsgVisual_0000)
#define MESSAGE1	catgets(Xm_catd,MS_VIsual,MSG_VI_2,_XmMsgVisual_0001)
#define MESSAGE2	catgets(Xm_catd,MS_VIsual,MSG_VI_3,_XmMsgVisual_0002)
#else
#define MESSAGE0	_XmMsgVisual_0000
#define MESSAGE1	_XmMsgVisual_0001
#define MESSAGE2	_XmMsgVisual_0002
#endif


/*  Defines and functions for processing dynamic defaults  */

#define XmMAX_SHORT 65535

#define XmCOLOR_PERCENTILE (XmMAX_SHORT / 100)
#define BoundColor(value)\
	((value < 0) ? 0 : (((value > XmMAX_SHORT) ? XmMAX_SHORT : value)))

/* Contributions of each primary to overall luminosity, sum to 1.0 */

#define XmRED_LUMINOSITY 0.30
#define XmGREEN_LUMINOSITY 0.59
#define XmBLUE_LUMINOSITY 0.11

/* Percent effect of intensity, light, and luminosity & on brightness,
   sum to 100 */

#define XmINTENSITY_FACTOR  75
#define XmLIGHT_FACTOR       0
#define XmLUMINOSITY_FACTOR 25

/* LITE color model
   percent to interpolate RGB towards black for SEL, BS, TS */

#define XmCOLOR_LITE_SEL_FACTOR   15
#define XmCOLOR_LITE_BS_FACTOR   40
#define XmCOLOR_LITE_TS_FACTOR   20

/* DARK color model
   percent to interpolate RGB towards white for SEL, BS, TS */

#define XmCOLOR_DARK_SEL_FACTOR   15
#define XmCOLOR_DARK_BS_FACTOR   30
#define XmCOLOR_DARK_TS_FACTOR   50

/* STD color model
   percent to interpolate RGB towards black for SEL, BS
   percent to interpolate RGB towards white for TS
   HI values used for high brightness (within STD)
   LO values used for low brightness (within STD)
   Interpolate factors between HI & LO values based on brightness */

#define XmCOLOR_HI_SEL_FACTOR   15
#define XmCOLOR_HI_BS_FACTOR   40
#define XmCOLOR_HI_TS_FACTOR   60

#define XmCOLOR_LO_SEL_FACTOR   15
#define XmCOLOR_LO_BS_FACTOR    60
#define XmCOLOR_LO_TS_FACTOR   50

#define done(type, value)			\
    {						\
        if (toVal->addr != NULL) {		\
            if (toVal->size < sizeof(type)) {	\
                toVal->size = sizeof(type);	\
                return( False);			\
            }					\
	    *((type *) (toVal->addr)) = value;	\
        }					\
        else {					\
            static type buf;			\
	    buf = (value);			\
            toVal->addr = (XPointer) &buf;	\
        }					\
        toVal->size = sizeof(type);		\
        return (True);				\
    } 

#ifdef OSF_v1_2_4

#define DEPTH(widget)  \
    (XtIsWidget(widget))? \
       ((widget)->core.depth):((XtParent(widget))->core.depth)


#endif /* OSF_v1_2_4 */
/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Boolean CvtStringToBackgroundPixmap() ;
static void CvtStringToPixmap() ;
static void _XmCvtStringToGadgetPixmap() ;
static void _XmCvtStringToPrimForegroundPixmap() ;
static void _XmCvtStringToPrimHighlightPixmap() ;
static void _XmCvtStringToPrimTopShadowPixmap() ;
static void _XmCvtStringToPrimBottomShadowPixmap() ;
static void _XmCvtStringToManForegroundPixmap() ;
static void _XmCvtStringToManHighlightPixmap() ;
static void _XmCvtStringToManTopShadowPixmap() ;
static void _XmCvtStringToManBottomShadowPixmap() ;
static Boolean CvtStringToAnimationMask() ;
static Boolean CvtStringToAnimationPixmap() ;
static void _XmGetDynamicDefault() ;
static void SetMonochromeColors() ;
static int _XmBrightness() ;
static void CalculateColorsForLightBackground() ;
static void CalculateColorsForDarkBackground() ;
static void CalculateColorsForMediumBackground() ;
static void _XmCalculateColorsRGB() ;

#else

static Boolean CvtStringToBackgroundPixmap( 
                        Display *dpy,
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *fromVal,
                        XrmValue *toVal,
                        XtPointer *closure_ret) ;
static void CvtStringToPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static void _XmCvtStringToGadgetPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static void _XmCvtStringToPrimForegroundPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static void _XmCvtStringToPrimHighlightPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static void _XmCvtStringToPrimTopShadowPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static void _XmCvtStringToPrimBottomShadowPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static void _XmCvtStringToManForegroundPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static void _XmCvtStringToManHighlightPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static void _XmCvtStringToManTopShadowPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static void _XmCvtStringToManBottomShadowPixmap( 
                        XrmValue *args,
                        Cardinal *numArgs,
                        XrmValue *fromVal,
                        XrmValue *toVal) ;
static Boolean CvtStringToAnimationMask( 
                        Display *dpy,
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *fromVal,
                        XrmValue *toVal,
                        XtPointer *closure_ret) ;
static Boolean CvtStringToAnimationPixmap( 
                        Display *dpy,
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *fromVal,
                        XrmValue *toVal,
                        XtPointer *closure_ret) ;
static void _XmGetDynamicDefault( 
                        Widget widget,
                        int type,
                        int offset,
                        XrmValue *value) ;
static void SetMonochromeColors( 
                        XmColorData *colors) ;
static int _XmBrightness( 
                        XColor *color) ;
static void CalculateColorsForLightBackground( 
                        XColor *bg_color,
                        XColor *fg_color,
                        XColor *sel_color,
                        XColor *ts_color,
                        XColor *bs_color) ;
static void CalculateColorsForDarkBackground( 
                        XColor *bg_color,
                        XColor *fg_color,
                        XColor *sel_color,
                        XColor *ts_color,
                        XColor *bs_color) ;
static void CalculateColorsForMediumBackground( 
                        XColor *bg_color,
                        XColor *fg_color,
                        XColor *sel_color,
                        XColor *ts_color,
                        XColor *bs_color) ;
static void _XmCalculateColorsRGB( 
                        XColor *bg_color,
                        XColor *fg_color,
                        XColor *sel_color,
                        XColor *ts_color,
                        XColor *bs_color) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/
	
/*
 * GLOBAL VARIABLES
 *
 * These variables define the color cache.
 */
static int Set_Count=0, Set_Size=0;
static XmColorData *Color_Set=NULL;

/* Thresholds for brightness
   above LITE threshold, LITE color model is used
   below DARK threshold, DARK color model is be used
   use STD color model in between */

static int XmCOLOR_LITE_THRESHOLD;
static int XmCOLOR_DARK_THRESHOLD;
static int XmFOREGROUND_THRESHOLD;
static Boolean XmTHRESHOLDS_INITD = FALSE;


static Boolean app_defined = FALSE;
static String default_background_color_spec = NULL;

/*  Name set by background pixmap converter, used and cleared  */
/*  Primitive and Manager Initialize procedure.                */

static char * background_pixmap_name = NULL;

static XmColorProc ColorRGBCalcProc = _XmCalculateColorsRGB;


/*  Argument lists sent down to the pixmap converter functions  */

static XtConvertArgRec backgroundArgs[] =
{
   { XtBaseOffset, (XtPointer) 0, sizeof (int) },
};

static XtConvertArgRec primForegroundArgs[] =
{
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen), sizeof (Screen*) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _XmPrimitiveRec,
primitive.foreground), sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.background_pixel),
sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.depth), sizeof (int) }
};

static XtConvertArgRec primHighlightArgs[] =
{
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen), sizeof (Screen*) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _XmPrimitiveRec,
primitive.highlight_color), sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.background_pixel),
sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.depth), sizeof (int) }
};

static XtConvertArgRec primTopShadowArgs[] =
{
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen), sizeof (Screen*) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _XmPrimitiveRec,
primitive.top_shadow_color), sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.background_pixel),
sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.depth), sizeof (int) }
};

static XtConvertArgRec primBottomShadowArgs[] =
{
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen), sizeof (Screen*) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _XmPrimitiveRec,
primitive.bottom_shadow_color), sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.background_pixel),
sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.depth), sizeof (int) }
};

static XtConvertArgRec manForegroundArgs[] =
{
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen),
sizeof (Screen*) },
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _XmManagerRec,
manager.foreground), sizeof (Pixel) },
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec,
core.background_pixel), sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.depth), sizeof (int) }
};

static XtConvertArgRec manHighlightArgs[] =
{
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen),
sizeof (Screen*) },
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _XmManagerRec,
manager.highlight_color), sizeof (Pixel) },
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec,
core.background_pixel), sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.depth), sizeof (int) }
};

static XtConvertArgRec manTopShadowArgs[] =
{
   { XtWidgetBaseOffset, (XtPointer)XtOffsetOf( struct _WidgetRec, core.screen), sizeof (Screen*) },
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _XmManagerRec,
manager.top_shadow_color), sizeof (Pixel) },
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec,
core.background_pixel), sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.depth), sizeof (int) }
};

static XtConvertArgRec manBottomShadowArgs[] =
{
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen),
sizeof (Screen*) },
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _XmManagerRec,
manager.bottom_shadow_color), sizeof (Pixel) },
   { XtWidgetBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec,
core.background_pixel), sizeof (Pixel) },
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.depth), sizeof (int) }
};

static XtConvertArgRec gadgetPixmapArgs[] =
{
   { XtBaseOffset, (XtPointer) XtOffsetOf( struct _WidgetRec, core.parent),
sizeof (Widget ) }
};


/************************************************************************
 *
 *  _XmRegisterPixmapConverters
 *	Register the pixmap converters used exclusively by primitive
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmRegisterPixmapConverters()
#else
_XmRegisterPixmapConverters( void )
#endif /* _NO_PROTO */
{
   static Boolean inited = False;


   if (inited == False)
   {
	  inited = True;

      XtSetTypeConverter (XmRString, XmRXmBackgroundPixmap,
                      CvtStringToBackgroundPixmap, 
                      backgroundArgs, XtNumber(backgroundArgs),
                      XtCacheNone, NULL);

      XtAddConverter (XmRString, XmRPrimForegroundPixmap,
                      _XmCvtStringToPrimForegroundPixmap, 
                      primForegroundArgs, XtNumber(primForegroundArgs));

      XtAddConverter (XmRString, XmRPrimHighlightPixmap,
                      _XmCvtStringToPrimHighlightPixmap, 
                      primHighlightArgs, XtNumber(primHighlightArgs));

      XtAddConverter (XmRString, XmRPrimTopShadowPixmap,
                      _XmCvtStringToPrimTopShadowPixmap, 
                      primTopShadowArgs, XtNumber(primTopShadowArgs));

      XtAddConverter (XmRString, XmRPrimBottomShadowPixmap,
                      _XmCvtStringToPrimBottomShadowPixmap, 
                      primBottomShadowArgs, XtNumber(primBottomShadowArgs));

      XtAddConverter (XmRString, XmRManForegroundPixmap,
                      _XmCvtStringToManForegroundPixmap, 
                      manForegroundArgs, XtNumber(manForegroundArgs));

      XtAddConverter (XmRString, XmRManHighlightPixmap,
                      _XmCvtStringToManHighlightPixmap, 
                      manHighlightArgs, XtNumber(manHighlightArgs));

      XtAddConverter (XmRString, XmRManTopShadowPixmap,
                      _XmCvtStringToManTopShadowPixmap, 
                      manTopShadowArgs, XtNumber(manTopShadowArgs));

      XtAddConverter (XmRString, XmRManBottomShadowPixmap,
                      _XmCvtStringToManBottomShadowPixmap, 
                      manBottomShadowArgs, XtNumber(manBottomShadowArgs));

      XtAddConverter (XmRString, XmRGadgetPixmap,
                      _XmCvtStringToGadgetPixmap, 
                      gadgetPixmapArgs, XtNumber(gadgetPixmapArgs));

      XtSetTypeConverter (XmRString, XmRAnimationMask, 
		          CvtStringToAnimationMask,
                          backgroundArgs, XtNumber(backgroundArgs),
		          XtCacheNone, NULL);

      XtSetTypeConverter (XmRString, XmRAnimationPixmap, 
		          CvtStringToAnimationPixmap,
                          backgroundArgs, XtNumber(backgroundArgs),
		          XtCacheNone, NULL);
   }
}


/************************************************************************
 *
 *  _XmGetBGPixmapName
 *	Return the background pixmap name set by the string to background
 *	resource converter.  This is used by primitive and manager.
 *
 ************************************************************************/
char * 
#ifdef _NO_PROTO
_XmGetBGPixmapName()
#else
_XmGetBGPixmapName( void )
#endif /* _NO_PROTO */
{
   return (background_pixmap_name);
}




/************************************************************************
 *
 *  _XmClearBGPixmapName
 *	Clear the background pixmap name set by the string to background
 *	resource converter.  This is used by primitive and manager.
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmClearBGPixmapName()
#else
_XmClearBGPixmapName( void )
#endif /* _NO_PROTO */
{
   background_pixmap_name = NULL;
}



/************************************************************************
 *
 *  Primitive, gadget and manager resource converters
 *
 ************************************************************************/

/************************************************************************
 *
 *  CvtStringToBackgroundPixmap()
 *	Convert a string to a background pixmap.  Used by both
 *	primitive and manager.
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
CvtStringToBackgroundPixmap( dpy, args, num_args, fromVal, toVal, closure_ret )
        Display *dpy ;
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
        XtPointer *closure_ret ;
#else
CvtStringToBackgroundPixmap(
        Display *dpy,
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *fromVal,
        XrmValue *toVal,
        XtPointer *closure_ret )
#endif /* _NO_PROTO */
{
   Widget widget;

   widget = *((Widget *) args[0].addr);

   if (!XtIsShell(widget))
      background_pixmap_name = (char *) (fromVal->addr);

   /*  Always send back a NULL pixmap  */

   done(Pixmap, XmUNSPECIFIED_PIXMAP)
}


/************************************************************************
 *
 *  CvtStringToPixmap
 *	Convert a string to a pixmap.  This is the general conversion
 *	routine referenced by many of the specific string to pixmap
 *	converter functions.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CvtStringToPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
CvtStringToPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   static Pixmap pixmap;
   char   * image_name = (char *) (fromVal->addr);
   Screen * screen;
   Pixel    foreground;
   Pixel    background;
   int      depth;

   if (_XmStringsAreEqual(image_name, "unspecified_pixmap"))
   {
       pixmap = XmUNSPECIFIED_PIXMAP;
   }
   else
   {
       screen = *((Screen **) args[0].addr);
       foreground = *((Pixel *) args[1].addr);
       background = *((Pixel *) args[2].addr);
       depth = *((int *) args[3].addr);
       pixmap = XmGetPixmapByDepth (screen, image_name, foreground, 
				    background, depth);
   }
   (*toVal).size = sizeof (Pixmap);
   (*toVal).addr = (XPointer) &pixmap;
}

/************************************************************************
 *
 *  _XmCvtStringToGadgetPixmap
 *	Convert a string to a pixmap as used by a gadget.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
_XmCvtStringToGadgetPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
_XmCvtStringToGadgetPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   static Pixmap pixmap;
   char   * image_name = (char *) (fromVal->addr);
   XmManagerWidget mom;
   Screen * screen;
   Pixel    foreground;
   Pixel    background;

   if (_XmStringsAreEqual(image_name, "unspecified_pixmap"))
   {
       pixmap = XmUNSPECIFIED_PIXMAP;
   }
   else
   {
       mom = *((XmManagerWidget *) args[0].addr);
       screen = XtScreen(mom);
       foreground = mom->manager.foreground;
       background = mom->core.background_pixel;
       pixmap = XmGetPixmapByDepth (screen, image_name, 
				    foreground, background, mom->core.depth);
   }
   (*toVal).size = sizeof (Pixmap);
   (*toVal).addr = (XPointer) &pixmap;
}


/************************************************************************
 *
 *  The following set of converter functions call the general converter
 *  to do all of the work.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
_XmCvtStringToPrimForegroundPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
_XmCvtStringToPrimForegroundPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   CvtStringToPixmap (args, numArgs, fromVal, toVal);
}

static void 
#ifdef _NO_PROTO
_XmCvtStringToPrimHighlightPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
_XmCvtStringToPrimHighlightPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   CvtStringToPixmap (args, numArgs, fromVal, toVal);
}

static void 
#ifdef _NO_PROTO
_XmCvtStringToPrimTopShadowPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
_XmCvtStringToPrimTopShadowPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   CvtStringToPixmap (args, numArgs, fromVal, toVal);
}

static void 
#ifdef _NO_PROTO
_XmCvtStringToPrimBottomShadowPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
_XmCvtStringToPrimBottomShadowPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   CvtStringToPixmap (args, numArgs, fromVal, toVal);
}

static void 
#ifdef _NO_PROTO
_XmCvtStringToManForegroundPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
_XmCvtStringToManForegroundPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   CvtStringToPixmap (args, numArgs, fromVal, toVal);
}

static void 
#ifdef _NO_PROTO
_XmCvtStringToManHighlightPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
_XmCvtStringToManHighlightPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   CvtStringToPixmap (args, numArgs, fromVal, toVal);
}

static void 
#ifdef _NO_PROTO
_XmCvtStringToManTopShadowPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
_XmCvtStringToManTopShadowPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   CvtStringToPixmap (args, numArgs, fromVal, toVal);
}

static void 
#ifdef _NO_PROTO
_XmCvtStringToManBottomShadowPixmap( args, numArgs, fromVal, toVal )
        XrmValue *args ;
        Cardinal *numArgs ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
#else
_XmCvtStringToManBottomShadowPixmap(
        XrmValue *args,
        Cardinal *numArgs,
        XrmValue *fromVal,
        XrmValue *toVal )
#endif /* _NO_PROTO */
{
   CvtStringToPixmap (args, numArgs, fromVal, toVal);
}


/************************************************************************
 *
 *  CvtStringToAnimationMask
 *
 ************************************************************************/

static Boolean 
#ifdef _NO_PROTO
CvtStringToAnimationMask( dpy, args, num_args, fromVal, toVal, closure_ret )
        Display *dpy ;
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
        XtPointer *closure_ret ;
#else
CvtStringToAnimationMask(
        Display *dpy,
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *fromVal,
        XrmValue *toVal,
        XtPointer *closure_ret )
#endif /* _NO_PROTO */
{
    char	*image_name = (char *) (fromVal->addr);
    Widget	widget;
    Screen	*screen;
    static Pixmap mask = XmUNSPECIFIED_PIXMAP;

    if (!_XmStringsAreEqual(image_name, "unspecified_pixmap")) {
    	if (*num_args != 1) {
	    XtAppWarningMsg (XtDisplayToApplicationContext(dpy),
			     "wrongParameters", "cvtStringToBitmap",
			     "XtToolkitError",
		"String to AnimationMask converter needs Widget argument",
			     (String *) NULL, (Cardinal *)NULL);
	    return False;
    	}
    	widget = *((Widget *) args[0].addr);
       	screen = XtScreenOfObject(widget);

	mask = _XmGetPixmap (screen, image_name, 1, 1, 0);

	if (mask == XmUNSPECIFIED_PIXMAP) {
	    XtDisplayStringConversionWarning(dpy, image_name,
					     XmRAnimationMask);
	    return False;
        }
    }
    done(Pixmap, mask)
}

/************************************************************************
 *
 *  CvtStringToAnimationPixmap
 *
 ************************************************************************/

static Boolean 
#ifdef _NO_PROTO
CvtStringToAnimationPixmap( dpy, args, num_args, fromVal, toVal, closure_ret )
        Display *dpy ;
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *fromVal ;
        XrmValue *toVal ;
        XtPointer *closure_ret ;
#else
CvtStringToAnimationPixmap(
        Display *dpy,
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *fromVal,
        XrmValue *toVal,
        XtPointer *closure_ret )
#endif /* _NO_PROTO */
{
    char	*image_name = (char *) (fromVal->addr);
    Widget	widget;
    Screen	*screen;
    static Pixmap pixmap = XmUNSPECIFIED_PIXMAP;
    Pixel	foreground;
    Pixel	background;

    if (!_XmStringsAreEqual(image_name, "unspecified_pixmap")) {

	if (*num_args != 1) {
	    XtAppWarningMsg (XtDisplayToApplicationContext(dpy),
			     "wrongParameters", "cvtStringToBitmap",
			     "XtToolkitError",
		"String to AnimationPixmap converter needs Widget argument",
			     (String *) NULL, (Cardinal *)NULL);
	    return False;
	}
	widget = *((Widget *) args[0].addr);

	if (XmIsPrimitive(widget)) {
       	    screen = XtScreen(widget);
	    background = widget->core.background_pixel;
       	    foreground = ((XmPrimitiveWidget)widget)->primitive.foreground;
	}
	else if (XmIsManager(widget)) {
       	    screen = XtScreen(widget);
	    background = widget->core.background_pixel;
       	    foreground = ((XmManagerWidget)widget)->manager.foreground;
	}
	else if (XtIsWidget(widget)) {
       	    screen = XtScreen(widget);
	    background = widget->core.background_pixel;
	    XmGetColors (screen, DefaultColormapOfScreen(screen),
			 background, &foreground, NULL, NULL, NULL);
	}
	else if (XmIsGadget(widget)) {
	    XmManagerWidget parent = (XmManagerWidget) XtParent(widget);

       	    screen = XtScreen(parent);
	    background = parent->core.background_pixel;
       	    foreground = parent->manager.foreground;
	}
	else {
       	    screen = XtScreenOfObject(widget);
    	    foreground = BlackPixelOfScreen(screen);
	    background = WhitePixelOfScreen(screen);
	}

#ifndef OSF_v1_2_4
	pixmap = XmGetPixmap (screen, image_name, foreground, background);
#else /* OSF_v1_2_4 */
	pixmap = _XmGetPixmap (screen, image_name, DEPTH(widget),
			       foreground, background);
#endif /* OSF_v1_2_4 */

	if (pixmap == XmUNSPECIFIED_PIXMAP) {
	    XtDisplayStringConversionWarning(dpy, image_name,
					     XmRAnimationPixmap);
	    return False;
        }
    }
    done(Pixmap, pixmap)
}

/************************************************************************
 *
 *  Dynamic defaulting color and pixmap functions
 *
 ************************************************************************/

void 
#ifdef _NO_PROTO
_XmForegroundColorDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmForegroundColorDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   _XmGetDynamicDefault (widget, XmFOREGROUND, offset, value);
}

void 
#ifdef _NO_PROTO
_XmHighlightColorDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmHighlightColorDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   _XmGetDynamicDefault (widget, XmFOREGROUND, offset, value);
}

void 
#ifdef _NO_PROTO
_XmBackgroundColorDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmBackgroundColorDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   _XmGetDynamicDefault (widget, XmBACKGROUND, offset, value);
}

void 
#ifdef _NO_PROTO
_XmTopShadowColorDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmTopShadowColorDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   _XmGetDynamicDefault (widget, XmTOP_SHADOW, offset, value);
}

void 
#ifdef _NO_PROTO
_XmBottomShadowColorDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmBottomShadowColorDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   _XmGetDynamicDefault (widget, XmBOTTOM_SHADOW, offset, value);
}

void 
#ifdef _NO_PROTO
_XmSelectColorDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmSelectColorDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   _XmGetDynamicDefault (widget, XmSELECT, offset, value);
}

void 
#ifdef _NO_PROTO
_XmPrimitiveTopShadowPixmapDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmPrimitiveTopShadowPixmapDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   XmPrimitiveWidget pw = (XmPrimitiveWidget) widget;
   static Pixmap pixmap;

   pixmap = XmUNSPECIFIED_PIXMAP;

   value->addr = (char *) &pixmap;
   value->size = sizeof (Pixmap);

   if (pw->primitive.top_shadow_color == pw->core.background_pixel)
      pixmap = XmGetPixmapByDepth (XtScreen (pw), "50_foreground",
                            pw->primitive.top_shadow_color,
                            pw->primitive.foreground,
                            pw->core.depth);

   else if (DefaultDepthOfScreen (XtScreen (widget)) == 1)
      pixmap = XmGetPixmapByDepth (XtScreen (pw), "50_foreground",
                            pw->primitive.top_shadow_color,
                            pw->core.background_pixel,
                            pw->core.depth);
}

void 
#ifdef _NO_PROTO
_XmManagerTopShadowPixmapDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmManagerTopShadowPixmapDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   XmManagerWidget mw = (XmManagerWidget) widget;
   static Pixmap pixmap;

   pixmap = XmUNSPECIFIED_PIXMAP;

   value->addr = (char *) &pixmap;
   value->size = sizeof (Pixmap);

   if (mw->manager.top_shadow_color == mw->core.background_pixel)
      pixmap = XmGetPixmapByDepth (XtScreen (mw), "50_foreground",
                            mw->manager.top_shadow_color,
                            mw->manager.foreground,
                            mw->core.depth);

   else if (DefaultDepthOfScreen (XtScreen (widget)) == 1)
      pixmap = XmGetPixmapByDepth (XtScreen (mw), "50_foreground",
                            mw->manager.top_shadow_color,
                            mw->core.background_pixel,
                            mw->core.depth);
}

void 
#ifdef _NO_PROTO
_XmPrimitiveHighlightPixmapDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmPrimitiveHighlightPixmapDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   XmPrimitiveWidget pw = (XmPrimitiveWidget) widget;
   static Pixmap pixmap;

   pixmap = XmUNSPECIFIED_PIXMAP;

   value->addr = (char *) &pixmap;
   value->size = sizeof (Pixmap);

   if (pw->primitive.highlight_color == pw->core.background_pixel)
      pixmap = XmGetPixmapByDepth (XtScreen (pw), "50_foreground",
                            pw->primitive.highlight_color,
                            pw->primitive.foreground,
                            pw->core.depth);

/*   else if (DefaultDepthOfScreen (XtScreen (widget)) == 1)
      pixmap = XmGetPixmapByDepth (XtScreen (pw), "50_foreground",
                            pw->primitive.highlight_color,
                            pw->core.background_pixel,
			    pw->core.depth);
*/
}

void 
#ifdef _NO_PROTO
_XmManagerHighlightPixmapDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
_XmManagerHighlightPixmapDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
   XmManagerWidget mw = (XmManagerWidget) widget;
   static Pixmap pixmap;

   pixmap = XmUNSPECIFIED_PIXMAP;

   value->addr = (char *) &pixmap;
   value->size = sizeof (Pixmap);

   if (mw->manager.highlight_color == mw->core.background_pixel)
      pixmap = XmGetPixmapByDepth (XtScreen (mw), "50_foreground",
                            mw->manager.highlight_color,
                            mw->manager.foreground,
                            mw->core.depth);

/*   if (DefaultDepthOfScreen (XtScreen (widget)) == 1)
      pixmap = XmGetPixmapByDepth (XtScreen (mw), "50_foreground",
                            mw->manager.highlight_color,
                            mw->core.background_pixel,
                            mw->core.depth);
*/
}




/*********************************************************************
 *
 *  _XmGetDynamicDefault
 *	Given the widget and the requested type of default, generate the
 *	default and store it in the value structure to be returned.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
_XmGetDynamicDefault( widget, type, offset, value )
        Widget widget ;
        int type ;
        int offset ;
        XrmValue *value ;
#else
_XmGetDynamicDefault(
        Widget widget,
        int type,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
	Screen *screen;
	Colormap color_map;
	static Pixel new_value;
	XmColorData *color_data;

	value->size = sizeof(new_value);
	value->addr = (char *) &new_value;

	if (!XtIsWidget(widget))
		widget = widget->core.parent;

	screen = XtScreen(widget);
	color_map = widget->core.colormap;

	if (type == XmBACKGROUND)
	{
		color_data = _XmGetDefaultColors(screen, color_map);
	}
	else
	{
		color_data = _XmGetColors(screen, color_map,
			widget->core.background_pixel);
	}

	new_value = _XmAccessColorData(color_data, type);
}

void
#ifdef _NO_PROTO
_XmGetDefaultThresholdsForScreen( screen )
     Screen *screen ;
#else
_XmGetDefaultThresholdsForScreen( Screen *screen )
#endif /* _NO_PROTO */
{
  XrmName names[2];
  XrmClass classes[2];
  XrmRepresentation rep;
  XrmValue db_value, to_value;
  int int_value;
  int default_light_threshold_spec;
  int default_dark_threshold_spec;
  int default_foreground_threshold_spec;
  WidgetRec widget;

  XmTHRESHOLDS_INITD = True;

 /* 
  * We need a widget to pass into the XtConvertAndStore() function
  * to convert the string to an int.  Since a widget can't be
  * passed into this procedure because the public interfaces
  * that call this routine don't have a widget, we need this hack
  * to create a dummy widget.
  */
  memset((char*) &widget, 0, sizeof(widget) );
  widget.core.self = &widget;
  widget.core.widget_class = coreWidgetClass;
  widget.core.screen = screen;
  XtInitializeWidgetClass(coreWidgetClass);


  names[0] = XrmStringToQuark(XmNlightThreshold);
  names[1] = NULLQUARK;
      
  classes[0] = XrmStringToQuark(XmCLightThreshold);
  classes[1] = NULLQUARK;

  if (XrmQGetResource(XtScreenDatabase(screen), names, classes,
		      &rep, &db_value))  
    {
     /* convert the string to an int value */
      to_value.size = sizeof(int);
      to_value.addr = (XPointer) &int_value;
      if (XtConvertAndStore(&widget, XmRString, &db_value, XmRInt, &to_value))
      {
	default_light_threshold_spec = int_value;
	if ( (default_light_threshold_spec >= 0) && 
	     (default_light_threshold_spec <= 100) )
	  ;
	else default_light_threshold_spec = XmDEFAULT_LIGHT_THRESHOLD;
      }
      else default_light_threshold_spec = XmDEFAULT_LIGHT_THRESHOLD;
    }
  else default_light_threshold_spec = XmDEFAULT_LIGHT_THRESHOLD; 

  names[0] = XrmStringToQuark(XmNdarkThreshold);
  names[1] = NULLQUARK;
      
  classes[0] = XrmStringToQuark(XmCDarkThreshold);
  classes[1] = NULLQUARK;
      
  if (XrmQGetResource(XtScreenDatabase(screen), names, classes,
		      &rep, &db_value))  
    {
     /* convert the string to an int value */
      to_value.size = sizeof(int);
      to_value.addr = (XPointer) &int_value;
      if (XtConvertAndStore(&widget, XmRString, &db_value, XmRInt, &to_value))
      {
        XtConvertAndStore(&widget, XmRString, &db_value, XmRInt, &to_value);
	default_dark_threshold_spec = int_value;
	if ( (default_dark_threshold_spec >= 0) && 
	     (default_dark_threshold_spec <= 100) )
	  ;
	else default_dark_threshold_spec = XmDEFAULT_DARK_THRESHOLD;
       }
       else default_dark_threshold_spec = XmDEFAULT_DARK_THRESHOLD;
    }
  else default_dark_threshold_spec = XmDEFAULT_DARK_THRESHOLD;

  names[0] = XrmStringToQuark(XmNforegroundThreshold);
  names[1] = NULLQUARK;
      
  classes[0] = XrmStringToQuark(XmCForegroundThreshold);
  classes[1] = NULLQUARK;
      
  if (XrmQGetResource(XtScreenDatabase(screen), names, classes,
		      &rep, &db_value))  
    {
     /* convert the string to an int value */
      to_value.size = sizeof(int);
      to_value.addr = (XPointer) &int_value;
      if (XtConvertAndStore(&widget, XmRString, &db_value, XmRInt, &to_value))
      {
	default_foreground_threshold_spec = int_value;
	if ( (default_foreground_threshold_spec >= 0) && 
	     (default_foreground_threshold_spec <= 100) )
	  ;
	else default_foreground_threshold_spec = XmDEFAULT_FOREGROUND_THRESHOLD;
      }
      else default_foreground_threshold_spec = XmDEFAULT_FOREGROUND_THRESHOLD;
    }
  else default_foreground_threshold_spec = XmDEFAULT_FOREGROUND_THRESHOLD;

  XmCOLOR_LITE_THRESHOLD = default_light_threshold_spec * XmCOLOR_PERCENTILE;
  XmCOLOR_DARK_THRESHOLD = default_dark_threshold_spec * XmCOLOR_PERCENTILE;
  XmFOREGROUND_THRESHOLD = default_foreground_threshold_spec * XmCOLOR_PERCENTILE;
}

String   
#ifdef _NO_PROTO
_XmGetDefaultBackgroundColorSpec( screen )
     Screen *screen;
#else
_XmGetDefaultBackgroundColorSpec( Screen *screen )
#endif /* _NO_PROTO */
{
  XrmName names[2];
  XrmClass classes[2];
  XrmRepresentation rep;
  XrmValue db_value;

  names[0] = XrmStringToQuark(XmNbackground);
  names[1] = NULLQUARK;
      
  classes[0] = XrmStringToQuark(XmCBackground);
  classes[1] = NULLQUARK;
	 
  if (XrmQGetResource(XtScreenDatabase(screen), names, classes,
		      &rep, &db_value)) 
     {
	if (rep == XrmStringToQuark(XmRString))
	    default_background_color_spec = db_value.addr;
     }
  else default_background_color_spec = XmDEFAULT_BACKGROUND;

  return(default_background_color_spec);
}

void 
#ifdef _NO_PROTO
_XmSetDefaultBackgroundColorSpec( screen, new_color_spec )
        Screen *screen ;
        String new_color_spec ;
#else
_XmSetDefaultBackgroundColorSpec(
	Screen *screen,
        String new_color_spec )
#endif /* _NO_PROTO */
{
	if (app_defined)
	{
		XtFree(default_background_color_spec);
	}

	default_background_color_spec = (String)
		XtMalloc(strlen(new_color_spec) + 1);
	/* this needs to be set per screen */
	strcpy(default_background_color_spec, new_color_spec);

	app_defined = TRUE;
}

XmColorData * 
#ifdef _NO_PROTO
_XmGetDefaultColors( screen, color_map )
        Screen *screen ;
        Colormap color_map ;
#else
_XmGetDefaultColors(
        Screen *screen,
        Colormap color_map )
#endif /* _NO_PROTO */
{
#ifndef OSF_v1_2_4
	static XmColorData ** default_set = NULL;
#else /* OSF_v1_2_4 */
	static XmColorData * default_set = NULL;
#endif /* OSF_v1_2_4 */
	static int default_set_count = 0;
	static int default_set_size = 0;
	register int i;
	XColor color_def;
	static Pixel background;
        XrmValue fromVal;
        XrmValue toVal;
        XrmValue args[2];
        Cardinal num_args;
	String default_string = XtDefaultBackground;

	/*  Look through  a set of screen / background pairs to see  */
	/*  if the default is already in the table.                  */

	for (i = 0; i < default_set_count; i++)
	{
#ifndef OSF_v1_2_4
	if ((default_set[i]->screen == screen) &&
		(default_set[i]->color_map == color_map))
		return (default_set[i]);
#else /* OSF_v1_2_4 */
	if ((default_set[i].screen == screen) &&
		(default_set[i].color_map == color_map))
		return (default_set + i);
#endif /* OSF_v1_2_4 */
	}

	/*  See if more space is needed in the array  */
  
	if (default_set == NULL)
	{
		default_set_size = 10;
#ifndef OSF_v1_2_4
		default_set = (XmColorData **) XtRealloc((char *) default_set, 
			(sizeof(XmColorData *) * default_set_size));
#else /* OSF_v1_2_4 */
		default_set = (XmColorData *) XtRealloc((char *) default_set, 
			(sizeof(XmColorData) * default_set_size));
#endif /* OSF_v1_2_4 */
		
	}
	else if (default_set_count == default_set_size)
	{
		default_set_size += 10;
#ifndef OSF_v1_2_4
		default_set = (XmColorData **) XtRealloc((char *) default_set, 
			sizeof(XmColorData *) * default_set_size);
#else /* OSF_v1_2_4 */
		default_set = (XmColorData *) XtRealloc((char *) default_set, 
			sizeof(XmColorData) * default_set_size);
#endif /* OSF_v1_2_4 */
	}

	/* Find the background based on the depth of the screen */
	if (DefaultDepthOfScreen(screen) == 1)
        {
	  /*
	   * Fix for 4603 - Convert the string XtDefaultBackground into a Pixel
	   *                value using the XToolkit converter.  This converter
	   *                will set this value to WhitePixelOfScreen if reverse
	   *                video is not on, and to BlackPixelOfScreen if reverse
	   *                video is on.
	   */
	  args[0].addr = (XPointer) &screen;
	  args[0].size = sizeof(Screen*);
	  args[1].addr = (XPointer) &color_map;
	  args[1].size = sizeof(Colormap);
	  num_args = 2;
	  
	  fromVal.addr = default_string;
	  fromVal.size = strlen(default_string);
	  
	  toVal.addr = (XPointer) &background;
	  toVal.size = sizeof(Pixel);
	  
	  if(!XtCallConverter(DisplayOfScreen(screen),XtCvtStringToPixel, 
			      args, num_args, &fromVal, &toVal, NULL))
	    background = WhitePixelOfScreen(screen);
        }

	else
	{
		/*  Parse out a color for the default background  */

		if (XParseColor(DisplayOfScreen(screen), color_map,
			_XmGetDefaultBackgroundColorSpec(screen), &color_def))
		{
			if (XAllocColor(DisplayOfScreen(screen), color_map,
				&color_def))
			{
				background = color_def.pixel;
			}
			else
			{
				_XmWarning(NULL, MESSAGE1);
				background = WhitePixelOfScreen(screen);
			}
		}
		else
		{
			_XmWarning(NULL, MESSAGE2);
			background = WhitePixelOfScreen(screen);
		}
	}

	/*
	 * Get the color data generated and save it in the next open
	 * slot in the default set array.  default_set points to a subset
	 * of the data pointed to by color_set (defined in _XmGetColors).
	 */

	default_set[default_set_count] = 
#ifndef OSF_v1_2_4
		_XmGetColors(screen, color_map, background);
#else /* OSF_v1_2_4 */
		*_XmGetColors(screen, color_map, background);
#endif /* OSF_v1_2_4 */
	default_set_count++;

#ifndef OSF_v1_2_4
	return (default_set[default_set_count - 1]);
#else /* OSF_v1_2_4 */
 	return (default_set + default_set_count - 1);
#endif /* OSF_v1_2_4 */
}


#define RGBMatchP(one,two) \
	  ((one.red	== two.red)	\
	&& (one.green	== two.green)	\
	&& (one.blue	== two.blue))

Boolean 
#ifdef _NO_PROTO
_XmSearchColorCache( which, values, ret )
        unsigned int which ;
        XmColorData *values ;
        XmColorData **ret ;
#else
_XmSearchColorCache(
        unsigned int which,
        XmColorData *values,
        XmColorData **ret )
#endif /* _NO_PROTO */
{
	register int i;

	/* 
	 * Look through  a set of screen, color_map, background triplets 
	 * to see if these colors have already been generated.
	 */

	for (i = 0; i < Set_Count; i++)
	{
		if ( (!(which & XmLOOK_AT_SCREEN) ||
				((Color_Set + i)->screen == values->screen))
			&&
			(!(which & XmLOOK_AT_CMAP) ||
				((Color_Set + i)->color_map == values->color_map))
			&&
			(!(which & XmLOOK_AT_BACKGROUND) ||
				(((Color_Set + i)->allocated & XmBACKGROUND) &&
				((Color_Set + i)->background.pixel == 
					values->background.pixel)))
			&&
			(!(which & XmLOOK_AT_FOREGROUND) ||
				(((Color_Set + i)->allocated & XmFOREGROUND) &&
				((Color_Set + i)->foreground.pixel ==
					values->foreground.pixel)))
			&&
			(!(which & XmLOOK_AT_TOP_SHADOW) ||
				(((Color_Set + i)->allocated & XmTOP_SHADOW) &&
				((Color_Set + i)->top_shadow.pixel == 
					values->top_shadow.pixel)))
			&&
			(!(which & XmLOOK_AT_BOTTOM_SHADOW) ||
				(((Color_Set + i)->allocated & XmBOTTOM_SHADOW) &&
				((Color_Set+ i)->bottom_shadow.pixel == 
					values->bottom_shadow.pixel)))
			&&
			(!(which & XmLOOK_AT_SELECT) ||
				(((Color_Set + i)->allocated & XmSELECT) &&
				((Color_Set + i)->select.pixel ==
					values->select.pixel))))
			{
				*ret = (Color_Set + i);
				return (TRUE);
			}
	}

	*ret = NULL;
	return (FALSE);
}

XmColorData * 
#ifdef _NO_PROTO
_XmAddToColorCache( new_rec )
        XmColorData *new_rec ;
#else
_XmAddToColorCache(
        XmColorData *new_rec )
#endif /* _NO_PROTO */
{
	/*  See if more space is needed */

	if (Set_Count == Set_Size)
	{
		Set_Size += 10;
		Color_Set = 
		(XmColorData *)XtRealloc((char *) Color_Set, 
			sizeof(XmColorData) * Set_Size);
	}

	*(Color_Set + Set_Count) = *new_rec;
	Set_Count++;

	return(Color_Set + (Set_Count - 1));
}

Pixel
#ifdef _NO_PROTO
_XmBlackPixel(screen, colormap, blackcolor)
     Screen *screen;
     Colormap colormap;
     XColor blackcolor;
#else
_XmBlackPixel(
	      Screen *screen,
	      Colormap colormap,
	      XColor blackcolor )
#endif /* _NO_PROTO */
{
  Pixel p;
  
  blackcolor.red = 0;
  blackcolor.green = 0;
  blackcolor.blue = 0;

  if (colormap == DefaultColormapOfScreen(screen))
    p = blackcolor.pixel = BlackPixelOfScreen(screen);
  else if (XAllocColor(screen->display, colormap, &blackcolor))
    p = blackcolor.pixel;
  else
    p = blackcolor.pixel = BlackPixelOfScreen(screen); /* fallback pixel */
  
  return (p);}
Pixel
#ifdef _NO_PROTO
_XmWhitePixel(screen, colormap, whitecolor)
     Screen *screen;
     Colormap colormap;
     XColor whitecolor;
#else
_XmWhitePixel(
	      Screen *screen,
	      Colormap colormap,
	      XColor whitecolor )
#endif /* _NO_PROTO */
{
  Pixel p;

  whitecolor.red = XmMAX_SHORT;
  whitecolor.green = XmMAX_SHORT;
  whitecolor.blue = XmMAX_SHORT;
 
  if (colormap == DefaultColormapOfScreen(screen))
    p = whitecolor.pixel = WhitePixelOfScreen(screen);
  else if (XAllocColor(screen->display, colormap, &whitecolor))
    p = whitecolor.pixel;
  else
    p = whitecolor.pixel = WhitePixelOfScreen(screen); /* fallback pixel */
  return (p);
}
  
Pixel 
#ifdef _NO_PROTO
_XmAccessColorData( cd, which )
        XmColorData *cd ;
        unsigned char which ;
#else
_XmAccessColorData(
        XmColorData *cd,
#if NeedWidePrototypes
        unsigned int which )
#else
        unsigned char which )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	Pixel p;

	switch(which)
	{
	        case XmBACKGROUND:
	                if (!(cd->allocated & which) && 
			    (XAllocColor(cd->screen->display,
					 cd->color_map, &(cd->background)) == 0))
			  {
			        if (_XmBrightness(&(cd->background))
					    < XmFOREGROUND_THRESHOLD )
				    cd->background.pixel = _XmBlackPixel(cd->screen, 
									 cd->color_map,
									 cd->background);
				    else 
				    cd->background.pixel = _XmWhitePixel(cd->screen, 
									 cd->color_map,
									 cd->background);				    
				    XQueryColor(cd->screen->display, cd->color_map, 
						&(cd->background));
				  }
			p = cd->background.pixel;
			cd->allocated |= which;
		break;
         	       case XmFOREGROUND:
			if (!(cd->allocated & which) &&
			   (XAllocColor(cd->screen->display,
					cd->color_map, &(cd->foreground)) == 0 )) 
			  {
			    if (_XmBrightness(&(cd->background))
					    < XmFOREGROUND_THRESHOLD )
				    cd->foreground.pixel = _XmWhitePixel(cd->screen, 
									 cd->color_map,
									 cd->foreground);
				    else 
				    cd->foreground.pixel = _XmBlackPixel(cd->screen, 
									 cd->color_map,
									 cd->foreground);
				    XQueryColor(cd->screen->display, cd->color_map, 
						&(cd->foreground));
			}
			p =  cd->foreground.pixel;	
			cd->allocated |= which;
		break;
		case XmTOP_SHADOW:
			if (!(cd->allocated & which) &&
				(XAllocColor(cd->screen->display,
				cd->color_map, &(cd->top_shadow)) == 0))
			{
				if (_XmBrightness(&(cd->background))
						> XmCOLOR_LITE_THRESHOLD)
					cd->top_shadow.pixel = 
						_XmBlackPixel(cd->screen, cd->color_map,
							      cd->top_shadow);
				else
					cd->top_shadow.pixel =
						_XmWhitePixel(cd->screen, cd->color_map,
							      cd->top_shadow);
				XQueryColor(cd->screen->display, cd->color_map, 
					    &(cd->top_shadow));

			}
			p = cd->top_shadow.pixel;
			cd->allocated |= which;
		break;
		case XmBOTTOM_SHADOW:
			if (!(cd->allocated & which) &&
				(XAllocColor(cd->screen->display,
					cd->color_map, &(cd->bottom_shadow)) == 0))
			{
				if (_XmBrightness(&(cd->background))
						< XmCOLOR_DARK_THRESHOLD)
					cd->bottom_shadow.pixel =  
						_XmWhitePixel(cd->screen, cd->color_map,
							      cd->bottom_shadow);
				else
					cd->bottom_shadow.pixel = 
						_XmBlackPixel(cd->screen, cd->color_map,
							      cd->bottom_shadow);
				XQueryColor(cd->screen->display, cd->color_map, 
					    &(cd->bottom_shadow));
			}
			p = cd->bottom_shadow.pixel;
			cd->allocated |= which;
		break;
		case XmSELECT:
			if (!(cd->allocated & which) &&
				(XAllocColor(cd->screen->display,
				cd->color_map, &(cd->select)) == 0))
			{
				if (_XmBrightness(&(cd->background)) 
						< XmFOREGROUND_THRESHOLD)
					cd->select.pixel = _XmWhitePixel(cd->screen, 
									 cd->color_map, 
									 cd->select);
				else
					cd->select.pixel = _XmBlackPixel(cd->screen, 
									 cd->color_map, 
									 cd->select);
				    XQueryColor(cd->screen->display, cd->color_map, 
						&(cd->select));
			}
			p = cd->select.pixel;
			cd->allocated |= which;
		break;
		default:
			_XmWarning(NULL, MESSAGE0);
			p = _XmBlackPixel(cd->screen, cd->color_map, cd->background);
		break;
	}

	return(p);
}

static void 
#ifdef _NO_PROTO
SetMonochromeColors( colors )
        XmColorData *colors ;
#else
SetMonochromeColors(
        XmColorData *colors )
#endif /* _NO_PROTO */
{
	Screen *screen = colors->screen;
	Pixel background = colors->background.pixel;

	if (background == BlackPixelOfScreen(screen))
	{
		colors->foreground.pixel = WhitePixelOfScreen (screen);
		colors->foreground.red = colors->foreground.green = 
			colors->foreground.blue = XmMAX_SHORT;

#ifndef OSF_v1_2_4
		colors->bottom_shadow.pixel = WhitePixelOfScreen(screen);
#else /* OSF_v1_2_4 */
		colors->bottom_shadow.pixel = BlackPixelOfScreen(screen);
#endif /* OSF_v1_2_4 */
		colors->bottom_shadow.red = colors->bottom_shadow.green = 
#ifndef OSF_v1_2_4
			colors->bottom_shadow.blue = XmMAX_SHORT;
#else /* OSF_v1_2_4 */
			colors->bottom_shadow.blue = 0;
#endif /* OSF_v1_2_4 */

		colors->select.pixel = WhitePixelOfScreen(screen);
		colors->select.red = colors->select.green = 
			colors->select.blue = XmMAX_SHORT;

#ifndef OSF_v1_2_4
		colors->top_shadow.pixel = BlackPixelOfScreen(screen);
#else /* OSF_v1_2_4 */
		colors->top_shadow.pixel = WhitePixelOfScreen(screen);
#endif /* OSF_v1_2_4 */
		colors->top_shadow.red = colors->top_shadow.green = 
#ifndef OSF_v1_2_4
			colors->top_shadow.blue = 0;
#else /* OSF_v1_2_4 */
			colors->top_shadow.blue = XmMAX_SHORT;
#endif /* OSF_v1_2_4 */
	}
	else if (background == WhitePixelOfScreen(screen))
	{
		colors->foreground.pixel = BlackPixelOfScreen(screen);
		colors->foreground.red = colors->foreground.green = 
			colors->foreground.blue = 0;

		colors->top_shadow.pixel = WhitePixelOfScreen(screen);
		colors->top_shadow.red = colors->top_shadow.green = 
			colors->top_shadow.blue = XmMAX_SHORT;

		colors->bottom_shadow.pixel = BlackPixelOfScreen(screen);
		colors->bottom_shadow.red = colors->bottom_shadow.green = 
			colors->bottom_shadow.blue = 0;

		colors->select.pixel = BlackPixelOfScreen(screen);
		colors->select.red = colors->select.green = 
			colors->select.blue = 0;
	}

	colors->allocated |= (XmFOREGROUND | XmTOP_SHADOW 
		| XmBOTTOM_SHADOW | XmSELECT);
}

static int 
#ifdef _NO_PROTO
_XmBrightness( color )
        XColor *color ;
#else
_XmBrightness(
        XColor *color )
#endif /* _NO_PROTO */
{
	int brightness;
	int intensity;
	int light;
	int luminosity, maxprimary, minprimary;
	int red = color->red;
	int green = color->green;
	int blue = color->blue;

	intensity = (red + green + blue) / 3;

	/* 
	 * The casting nonsense below is to try to control the point at
	 * the truncation occurs.
	 */

	luminosity = (int) ((XmRED_LUMINOSITY * (float) red)
		+ (XmGREEN_LUMINOSITY * (float) green)
		+ (XmBLUE_LUMINOSITY * (float) blue));

	maxprimary = ( (red > green) ?
					( (red > blue) ? red : blue ) :
					( (green > blue) ? green : blue ) );

	minprimary = ( (red < green) ?
					( (red < blue) ? red : blue ) :
					( (green < blue) ? green : blue ) );

	light = (minprimary + maxprimary) / 2;

	brightness = ( (intensity * XmINTENSITY_FACTOR) +
				   (light * XmLIGHT_FACTOR) +
				   (luminosity * XmLUMINOSITY_FACTOR) ) / 100;
	return(brightness);
}

static void 
#ifdef _NO_PROTO
CalculateColorsForLightBackground( bg_color, fg_color, sel_color, ts_color, bs_color )
        XColor *bg_color ;
        XColor *fg_color ;
        XColor *sel_color ;
        XColor *ts_color ;
        XColor *bs_color ;
#else
CalculateColorsForLightBackground(
        XColor *bg_color,
        XColor *fg_color,
        XColor *sel_color,
        XColor *ts_color,
        XColor *bs_color )
#endif /* _NO_PROTO */
{
	int brightness = _XmBrightness(bg_color);
	int color_value;

	if (fg_color)
	{
/*
 * Fix for 4602 - Compare the brightness with the foreground threshold.
 *                If its larger, make the foreground color black.
 *                Otherwise, make it white.
 */
          if (brightness > XmFOREGROUND_THRESHOLD)
          {
                  fg_color->red = 0;
                  fg_color->green = 0;
                  fg_color->blue = 0;
          }
          else
          {
                  fg_color->red = XmMAX_SHORT;
                  fg_color->green = XmMAX_SHORT;
                  fg_color->blue = XmMAX_SHORT;
          }
/*
 * End Fix 4602
 */
	}

	if (sel_color)
	{
		color_value = bg_color->red;
		color_value -= (color_value * XmCOLOR_LITE_SEL_FACTOR) / 100;
		sel_color->red = color_value;

		color_value = bg_color->green;
		color_value -= (color_value * XmCOLOR_LITE_SEL_FACTOR) / 100;
		sel_color->green = color_value;

		color_value = bg_color->blue;
		color_value -= (color_value * XmCOLOR_LITE_SEL_FACTOR) / 100;
		sel_color->blue = color_value;
	}

	if (bs_color)
	{
		color_value = bg_color->red;
		color_value -= (color_value * XmCOLOR_LITE_BS_FACTOR) / 100;
		bs_color->red = color_value;

		color_value = bg_color->green;
		color_value -= (color_value * XmCOLOR_LITE_BS_FACTOR) / 100;
		bs_color->green = color_value;

		color_value = bg_color->blue;
		color_value -= (color_value * XmCOLOR_LITE_BS_FACTOR) / 100;
		bs_color->blue = color_value;
	}

	if (ts_color)
	{
		color_value = bg_color->red;
		color_value -= (color_value * XmCOLOR_LITE_TS_FACTOR) / 100;
		ts_color->red = color_value;

		color_value = bg_color->green;
		color_value -= (color_value * XmCOLOR_LITE_TS_FACTOR) / 100;
		ts_color->green = color_value;

		color_value = bg_color->blue;
		color_value -= (color_value * XmCOLOR_LITE_TS_FACTOR) / 100;
		ts_color->blue = color_value;
	}
}
	
static void 
#ifdef _NO_PROTO
CalculateColorsForDarkBackground( bg_color, fg_color, sel_color, ts_color, bs_color )
        XColor *bg_color ;
        XColor *fg_color ;
        XColor *sel_color ;
        XColor *ts_color ;
        XColor *bs_color ;
#else
CalculateColorsForDarkBackground(
        XColor *bg_color,
        XColor *fg_color,
        XColor *sel_color,
        XColor *ts_color,
        XColor *bs_color )
#endif /* _NO_PROTO */
{
	int brightness = _XmBrightness(bg_color);
	int color_value;

	if (fg_color)
	{
/*
 * Fix for 4602 - Compare the brightness with the foreground threshold.
 *                If its larger, make the foreground color black.
 *                Otherwise, make it white.
 */
          if (brightness > XmFOREGROUND_THRESHOLD)
          {
                  fg_color->red = 0;
                  fg_color->green = 0;
                  fg_color->blue = 0;
          }
          else
          {
                  fg_color->red = XmMAX_SHORT;
                  fg_color->green = XmMAX_SHORT;
                  fg_color->blue = XmMAX_SHORT;
          }
/*
 * End Fix 4602
 */
	}

	if (sel_color)
	{
		color_value = bg_color->red;
		color_value += XmCOLOR_DARK_SEL_FACTOR *
			(XmMAX_SHORT - color_value) / 100;
		sel_color->red = color_value;

		color_value = bg_color->green;
		color_value += XmCOLOR_DARK_SEL_FACTOR *
			(XmMAX_SHORT - color_value) / 100;
		sel_color->green = color_value;

		color_value = bg_color->blue;
		color_value += XmCOLOR_DARK_SEL_FACTOR *
			(XmMAX_SHORT - color_value) / 100;
		sel_color->blue = color_value;
	}

	if (bs_color)
	{
		color_value = bg_color->red;
		color_value += XmCOLOR_DARK_BS_FACTOR *
			(XmMAX_SHORT - color_value) / 100;
		bs_color->red = color_value;

		color_value = bg_color->green;
		color_value += XmCOLOR_DARK_BS_FACTOR *
			(XmMAX_SHORT - color_value) / 100;
		bs_color->green = color_value;

		color_value = bg_color->blue;
		color_value += XmCOLOR_DARK_BS_FACTOR *
			(XmMAX_SHORT - color_value) / 100;
		bs_color->blue = color_value;
	}

	if (ts_color)
	{
		color_value = bg_color->red;
		color_value += XmCOLOR_DARK_TS_FACTOR *
			(XmMAX_SHORT - color_value) / 100;
		ts_color->red = color_value;

		color_value = bg_color->green;
		color_value += XmCOLOR_DARK_TS_FACTOR *
			(XmMAX_SHORT - color_value) / 100;
		ts_color->green = color_value;

		color_value = bg_color->blue;
		color_value += XmCOLOR_DARK_TS_FACTOR *
			(XmMAX_SHORT - color_value) / 100;
		ts_color->blue = color_value;
	}
}

static void 
#ifdef _NO_PROTO
CalculateColorsForMediumBackground( bg_color, fg_color, sel_color, ts_color, bs_color )
        XColor *bg_color ;
        XColor *fg_color ;
        XColor *sel_color ;
        XColor *ts_color ;
        XColor *bs_color ;
#else
CalculateColorsForMediumBackground(
        XColor *bg_color,
        XColor *fg_color,
        XColor *sel_color,
        XColor *ts_color,
        XColor *bs_color )
#endif /* _NO_PROTO */
{
	int brightness = _XmBrightness(bg_color);
	int color_value, f;

	if (brightness > XmFOREGROUND_THRESHOLD)
	{
		fg_color->red = 0;
		fg_color->green = 0;
		fg_color->blue = 0;
	}
	else
	{
		fg_color->red = XmMAX_SHORT;
		fg_color->green = XmMAX_SHORT;
		fg_color->blue = XmMAX_SHORT;
	}

	if (sel_color)
	{
		f = XmCOLOR_LO_SEL_FACTOR + (brightness
			* ( XmCOLOR_HI_SEL_FACTOR - XmCOLOR_LO_SEL_FACTOR )
			/ XmMAX_SHORT );

		color_value = bg_color->red;
		color_value -= (color_value * f) / 100;
		sel_color->red = color_value;

		color_value = bg_color->green;
		color_value -= (color_value * f) / 100;
		sel_color->green = color_value;

		color_value = bg_color->blue;
		color_value -= (color_value * f) / 100;
		sel_color->blue = color_value;
	}

	if (bs_color)
	{
		f = XmCOLOR_LO_BS_FACTOR + (brightness 
			* ( XmCOLOR_HI_BS_FACTOR - XmCOLOR_LO_BS_FACTOR )
			/ XmMAX_SHORT);

		color_value = bg_color->red;
		color_value -= (color_value * f) / 100;
		bs_color->red = color_value;

		color_value = bg_color->green;
		color_value -= (color_value * f) / 100;
		bs_color->green = color_value;

		color_value = bg_color->blue;
		color_value -= (color_value * f) / 100;
		bs_color->blue = color_value;
	}

	if (ts_color)
	{
		f = XmCOLOR_LO_TS_FACTOR + (brightness
			* ( XmCOLOR_HI_TS_FACTOR - XmCOLOR_LO_TS_FACTOR )
			/ XmMAX_SHORT);

		color_value = bg_color->red;
		color_value += f * ( XmMAX_SHORT - color_value ) / 100;
		ts_color->red = color_value;

		color_value = bg_color->green;
		color_value += f * ( XmMAX_SHORT - color_value ) / 100;
		ts_color->green = color_value;

		color_value = bg_color->blue;
		color_value += f * ( XmMAX_SHORT - color_value ) / 100;
		ts_color->blue = color_value;
	}
}

static void 
#ifdef _NO_PROTO
_XmCalculateColorsRGB( bg_color, fg_color, sel_color, ts_color, bs_color )
        XColor *bg_color ;
        XColor *fg_color ;
        XColor *sel_color ;
        XColor *ts_color ;
        XColor *bs_color ;
#else
_XmCalculateColorsRGB(
        XColor *bg_color,
        XColor *fg_color,
        XColor *sel_color,
        XColor *ts_color,
        XColor *bs_color )
#endif /* _NO_PROTO */
{
	int brightness = _XmBrightness(bg_color);

	if (brightness < XmCOLOR_DARK_THRESHOLD)
		CalculateColorsForDarkBackground(bg_color, fg_color,
			sel_color, ts_color, bs_color);
	else if (brightness > XmCOLOR_LITE_THRESHOLD)
		CalculateColorsForLightBackground(bg_color, fg_color,
			sel_color, ts_color, bs_color);
	else
		CalculateColorsForMediumBackground(bg_color, fg_color,
			sel_color, ts_color, bs_color);
}


XmColorProc 
#ifdef _NO_PROTO
XmSetColorCalculation( proc )
        XmColorProc proc ;
#else
XmSetColorCalculation(
        XmColorProc proc )
#endif /* _NO_PROTO */
{
	XmColorProc a = ColorRGBCalcProc;

	if (proc != NULL)
		ColorRGBCalcProc = proc;
	else
		ColorRGBCalcProc = _XmCalculateColorsRGB;
	
	return(a);
}

XmColorProc 
#ifdef _NO_PROTO
XmGetColorCalculation()
#else
XmGetColorCalculation( void )
#endif /* _NO_PROTO */
{
	return(ColorRGBCalcProc);
}


/*********************************************************************
 *
 *  _XmGetColors
 *
 *********************************************************************/
XmColorData * 
#ifdef _NO_PROTO
_XmGetColors( screen, color_map, background )
        Screen *screen ;
        Colormap color_map ;
        Pixel background ;
#else
_XmGetColors(
        Screen *screen,
        Colormap color_map,
        Pixel background )
#endif /* _NO_PROTO */
{
	Display * display = DisplayOfScreen (screen);
	XmColorData *old_colors;
	XmColorData new_colors;


	new_colors.screen = screen;
	new_colors.color_map = color_map;
	new_colors.background.pixel = background;

	if (_XmSearchColorCache(
		(XmLOOK_AT_SCREEN | XmLOOK_AT_CMAP | XmLOOK_AT_BACKGROUND),
			&new_colors, &old_colors)) {
               /*
		* initialize the thresholds if the current color scheme
		* already matched what is in the cache and the thresholds
		* haven't already been initialized.
                */
                if (!XmTHRESHOLDS_INITD)
	            _XmGetDefaultThresholdsForScreen(screen);
		return(old_colors);
        }

	XQueryColor (display, color_map, &(new_colors.background));
	new_colors.allocated = XmBACKGROUND;

	/*
	 * Just in case somebody looks at these before they're ready,
	 * initialize them to a value that is always valid (for most
	 * implementations of X).
	 */
	new_colors.foreground.pixel = new_colors.top_shadow.pixel = 
		new_colors.top_shadow.pixel = new_colors.select.pixel = 0;

	/*  Generate the foreground, top_shadow, and bottom_shadow based  */
	/*  on the background                                             */

	if (DefaultDepthOfScreen(screen) == 1)
		SetMonochromeColors(&new_colors);
	else
	  {
	    _XmGetDefaultThresholdsForScreen(screen);
	    (*ColorRGBCalcProc)(&(new_colors.background),
				&(new_colors.foreground), &(new_colors.select),
				&(new_colors.top_shadow), &(new_colors.bottom_shadow));
	  }
	return (_XmAddToColorCache(&new_colors));
}

void 
#ifdef _NO_PROTO
XmGetColors( screen, color_map, background, foreground_ret, top_shadow_ret, bottom_shadow_ret, select_ret )
        Screen *screen ;
        Colormap color_map ;
        Pixel background ;
        Pixel *foreground_ret ;
        Pixel *top_shadow_ret ;
        Pixel *bottom_shadow_ret ;
        Pixel *select_ret ;
#else
XmGetColors(
        Screen *screen,
        Colormap color_map,
        Pixel background,
        Pixel *foreground_ret,
        Pixel *top_shadow_ret,
        Pixel *bottom_shadow_ret,
        Pixel *select_ret )
#endif /* _NO_PROTO */
{
	XmColorData *cd;

	cd = _XmGetColors(screen, color_map, background);

	if (foreground_ret)
		*foreground_ret = _XmAccessColorData(cd, XmFOREGROUND);
	if (top_shadow_ret)
		*top_shadow_ret = _XmAccessColorData(cd, XmTOP_SHADOW);
	if (bottom_shadow_ret)
		*bottom_shadow_ret = _XmAccessColorData(cd, XmBOTTOM_SHADOW);
	if (select_ret)
		*select_ret = _XmAccessColorData(cd, XmSELECT);
}

/*********************************************************************
 *
 *  XmChangeColor - change set of colors for existing widget, given 
 *                  background color
 *
 *********************************************************************/
void 
#ifdef _NO_PROTO
XmChangeColor( w, background )
     Widget w ;
     Pixel background ;
#else
XmChangeColor(
	      Widget w,
	      Pixel background )
#endif /* _NO_PROTO */

{
  Pixel foreground_ret;
  Pixel topshadow_ret;
  Pixel bottomshadow_ret;
  Pixel select_ret; 

  Arg args[5];

/*
 * Fix for 4601 - Check to see if the input widget is a Gadget.  If it is,
 *                check to see if it is a PushButtonGadget or a 
 *                ToggleButtonGadget.  If so, get a new select color from
 *                the background of the parent.
 */
  if (XmIsGadget(w))
  {
    if (XmIsPushButtonGadget(w) || XmIsToggleButtonGadget(w))
    {
      Widget parent = XtParent(w);
      XmGetColors(parent->core.screen, parent->core.colormap, 
                  parent->core.background_pixel, NULL, 
		  &topshadow_ret, &bottomshadow_ret, &select_ret);
      if (XmIsPushButtonGadget(w))
        XtSetArg(args[0], XmNarmColor, select_ret);
      else
        XtSetArg(args[0], XmNselectColor, select_ret);
      XtSetValues(w, args, 1);
#ifndef OSF_v1_2_4

      /* CR 1139 - Set the shadow colors for gadgets too. */
      XtSetArg (args[0], XmNtopShadowColor, (XtArgVal) topshadow_ret);
      XtSetArg (args[1], XmNbottomShadowColor, (XtArgVal) bottomshadow_ret);
      XtSetValues(parent, args, 2);
#endif /* OSF_v1_2_4 */
    }
    return;
  }
/*
 * End 4601 Fix
 */
      
  XmGetColors( w->core.screen, w->core.colormap, background, &foreground_ret,
	      &topshadow_ret, &bottomshadow_ret, NULL );
  if ( (XmIsManager(w)) ||  (XmIsPrimitive(w)) )
    { 
      XtSetArg (args[0], XmNbackground, (XtArgVal) background);
      XtSetArg (args[1], XmNforeground, (XtArgVal) foreground_ret);
      XtSetArg (args[2], XmNtopShadowColor, (XtArgVal) topshadow_ret);
      XtSetArg (args[3], XmNbottomShadowColor, (XtArgVal) bottomshadow_ret);
      XtSetArg (args[4], XmNhighlightColor, (XtArgVal) foreground_ret);
      
      XtSetValues (w, args, 5);
      
      if (XmIsPrimitive(w))
	{
	  if ( (XmIsScrollBar(w)) || (XmIsPushButton(w)) || (XmIsToggleButton(w)) )
	    { 
	      XmGetColors( w->core.screen, w->core.colormap, background, NULL,
			  NULL, NULL, &select_ret);
	      if (XmIsScrollBar(w))
		XtSetArg (args[0], XmNtroughColor, (XtArgVal) select_ret);
	      else if (XmIsPushButton(w))
		XtSetArg (args[0], XmNarmColor, (XtArgVal) select_ret);
	      else if (XmIsToggleButton(w))
		XtSetArg (args[0], XmNselectColor, (XtArgVal) select_ret);
	      XtSetValues (w, args, 1);
	    }
	}
    }
}
