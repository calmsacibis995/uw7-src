#pragma ident	"@(#)m1.2libs:Xm/TextOut.c	1.8"
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
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/* Private definitions. */

#include <stdio.h>
#include <limits.h>
#include <X11/Xatom.h>
#include "XmI.h"
#include <Xm/TextP.h>
#include <Xm/TextStrSoP.h>
#include <X11/Shell.h>
#include <X11/Vendor.h>
#include <Xm/AtomMgr.h>
#include <Xm/DrawP.h>
#include "MessagesI.h"
#include <Xm/ScrolledWP.h>
#include <Xm/ScrollBar.h>
#include <Xm/XmosP.h>
#include <Xm/Display.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MSG1	catgets(Xm_catd,MS_Text,MSG_T_5,_XmMsgTextOut_0000)
#define MSG2	catgets(Xm_catd,MS_TField,MSG_TF_2,_XmMsgTextF_0001)
#define MSG3	catgets(Xm_catd,MS_Text,MSG_T_7,_XmMsgTextF_0002)
#define MSG4	catgets(Xm_catd,MS_Text,MSG_T_8,_XmMsgTextF_0003)
#else
#define MSG1	_XmMsgTextOut_0000
#define MSG2	_XmMsgTextF_0001
#define MSG3	_XmMsgTextF_0002
#define MSG4	_XmMsgTextF_0003
#endif


typedef struct {
   XmTextWidget tw;
} TextGCDataRec, *TextGCData;

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static TextGCData GetTextGCData() ;
static void _XmTextDrawShadow() ;
static void CheckHasRect() ;
static void XmSetFullGC() ;
static void XmResetSaveGC() ;
static void GetRect() ;
static void XmSetMarginGC() ;
static void XmSetNormGC() ;
static void InvertImageGC() ;
static void XmSetInvGC() ;
static int _FontStructFindWidth() ;
static int FindWidth() ;
static XmTextPosition XYToPos() ;
static Boolean PosToXY() ;
static XtGeometryResult TryResize() ;
static int CountLines() ;
static void TextFindNewWidth() ;
static void TextFindNewHeight() ;
static void CheckForNewSize() ;
static XtPointer OutputBaseProc() ;
static Boolean MeasureLine() ;
static void Draw() ;
static OnOrOff CurrentCursorState() ;
static void PaintCursor() ;
static void ChangeHOffset() ;
static void DrawInsertionPoint() ;
static void MakePositionVisible() ;
static void BlinkInsertionPoint() ;
static Boolean MoveLines() ;
static void OutputInvalidate() ;
static void RefigureDependentInfo() ;
static void SizeFromRowsCols() ;
static void LoadFontMetrics() ;
static void LoadGCs() ;
static void MakeIBeamOffArea() ;
static void MakeIBeamStencil() ;
static void MakeAddModeCursor() ;
static void MakeCursors() ;
static void OutputGetValues() ;
static Boolean CKCols() ;
static Boolean CKRows() ;
static Boolean OutputSetValues() ;
static void NotifyResized() ;
static void HandleTimer() ;
static void HandleFocusEvents() ;
static void HandleGraphicsExposure() ;
static void OutputRealize() ;
static void OutputDestroy() ;
static void RedrawRegion() ;
static void OutputExpose() ;
static void GetPreferredSize() ;
static void HandleVBarButtonRelease() ;
static void HandleVBar() ;
static void HandleHBar() ;
static Pixmap GetClipMask() ;

#else

static TextGCData GetTextGCData( 
                        Widget w) ;
static void _XmTextDrawShadow( 
                        XmTextWidget tw) ;
static void CheckHasRect( 
                        XmTextWidget tw) ;
static void XmSetFullGC( 
                        XmTextWidget tw,
                        GC gc) ;
static void XmResetSaveGC( 
                        XmTextWidget tw,
                        GC gc) ;
static void GetRect( 
                        XmTextWidget tw,
                        XRectangle *rect) ;
static void XmSetMarginGC( 
                        XmTextWidget tw,
                        GC gc) ;
static void XmSetNormGC( 
                        XmTextWidget tw,
                        GC gc,
#if NeedWidePrototypes
                        int change_stipple,
                        int stipple) ;
#else
                        Boolean change_stipple,
                        Boolean stipple) ;
#endif /* NeedWidePrototypes */
static void InvertImageGC( 
                        XmTextWidget tw) ;
static void XmSetInvGC( 
                        XmTextWidget tw,
                        GC gc) ;
static int _FontStructFindWidth( 
                        XmTextWidget tw,
                        int x,
                        XmTextBlock block,
                        int from,
                        int to) ;
static int FindWidth( 
                        XmTextWidget tw,
                        int x,
                        XmTextBlock block,
                        int from,
                        int to) ;
static XmTextPosition XYToPos( 
                        XmTextWidget widget,
#if NeedWidePrototypes
                        int x,
                        int y) ;
#else
                        Position x,
                        Position y) ;
#endif /* NeedWidePrototypes */
static Boolean PosToXY( 
                        XmTextWidget widget,
                        XmTextPosition position,
                        Position *x,
                        Position *y) ;
static XtGeometryResult TryResize( 
                        XmTextWidget widget,
#if NeedWidePrototypes
                        int width,
                        int height) ;
#else
                        Dimension width,
                        Dimension height) ;
#endif /* NeedWidePrototypes */
static int CountLines( 
                        XmTextWidget widget,
                        XmTextPosition start,
                        XmTextPosition end) ;
static void TextFindNewWidth( 
                        XmTextWidget widget,
                        Dimension *widthRtn) ;
static void TextFindNewHeight( 
                        XmTextWidget widget,
                        XmTextPosition position,
                        Dimension *heightRtn) ;
static void CheckForNewSize( 
                        XmTextWidget widget,
                        XmTextPosition position) ;
static XtPointer OutputBaseProc( 
                        Widget widget,
                        XtPointer client_data) ;
static Boolean MeasureLine( 
                        XmTextWidget widget,
                        LineNum line,
                        XmTextPosition position,
                        XmTextPosition *nextpos,
                        LineTableExtra *extra) ;
static void Draw( 
                        XmTextWidget widget,
                        LineNum line,
                        XmTextPosition start,
                        XmTextPosition end,
                        XmHighlightMode highlight) ;
static OnOrOff CurrentCursorState( 
                        XmTextWidget widget) ;
static void PaintCursor( 
                        XmTextWidget widget) ;
static void ChangeHOffset( 
                        XmTextWidget widget,
                        int newhoffset,
#if NeedWidePrototypes
                        int redisplay_hbar) ;
#else
                        Boolean redisplay_hbar) ;
#endif /* NeedWidePrototypes */
static void DrawInsertionPoint( 
                        XmTextWidget widget,
                        XmTextPosition position,
                        OnOrOff onoroff) ;
static void MakePositionVisible( 
                        XmTextWidget widget,
                        XmTextPosition position) ;
static void BlinkInsertionPoint( 
                        XmTextWidget widget) ;
static Boolean MoveLines( 
                        XmTextWidget widget,
                        LineNum fromline,
                        LineNum toline,
                        LineNum destline) ;
static void OutputInvalidate( 
                        XmTextWidget widget,
                        XmTextPosition position,
                        XmTextPosition topos,
                        long delta) ;
static void RefigureDependentInfo( 
                        XmTextWidget widget) ;
static void SizeFromRowsCols( 
                        XmTextWidget widget,
                        Dimension *width,
                        Dimension *height) ;
static void LoadFontMetrics( 
                        XmTextWidget widget) ;
static void LoadGCs( 
                        XmTextWidget widget,
                        Pixel background,
                        Pixel foreground) ;
static void MakeIBeamOffArea( 
                        XmTextWidget tw,
#if NeedWidePrototypes
                        int width,
                        int height) ;
#else
                        Dimension width,
                        Dimension height) ;
#endif /* NeedWidePrototypes */
static void MakeIBeamStencil( 
                        XmTextWidget tw,
                        int line_width) ;
static void MakeAddModeCursor( 
                        XmTextWidget tw,
                        int line_width) ;
static void MakeCursors( 
                        XmTextWidget widget) ;
static void OutputGetValues( 
                        Widget wid,
                        ArgList args,
                        Cardinal num_args) ;
static Boolean CKCols( 
                        ArgList args,
                        Cardinal num_args) ;
static Boolean CKRows( 
                        ArgList args,
                        Cardinal num_args) ;
static Boolean OutputSetValues( 
                        Widget oldw,
                        Widget reqw,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void NotifyResized( 
                        Widget w,
#if NeedWidePrototypes
                        int o_create) ;
#else
                        Boolean o_create) ;
#endif /* NeedWidePrototypes */
static void HandleTimer( 
                        XtPointer closure,
                        XtIntervalId *id) ;
static void HandleFocusEvents( 
                        Widget w,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static void HandleGraphicsExposure( 
                        Widget w,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static void OutputRealize( 
                        Widget w,
                        XtValueMask *valueMask,
                        XSetWindowAttributes *attributes) ;
static void OutputDestroy( 
                        Widget w) ;
static void RedrawRegion( 
                        XmTextWidget widget,
                        int x,
                        int y,
                        int width,
                        int height) ;
static void OutputExpose( 
                        Widget w,
                        XEvent *event,
                        Region region) ;
static void GetPreferredSize( 
                        Widget widget,
                        Dimension *width,
                        Dimension *height) ;
static void HandleVBarButtonRelease( 
                        Widget w,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static void HandleVBar( 
                        Widget w,
                        XtPointer param,
                        XtPointer cback) ;
static void HandleHBar( 
                        Widget w,
                        XtPointer param,
                        XtPointer cback) ;
static Pixmap GetClipMask( 
                        XmTextWidget tw,
                        char *pixmap_name) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

#define EraseInsertionPoint(widget)\
{\
    (*widget->text.output->DrawInsertionPoint)(widget,\
                                         widget->text.cursor_position, off);\
}

#define TextDrawInsertionPoint(widget)\
{\
    (*widget->text.output->DrawInsertionPoint)(widget,\
                                         widget->text.cursor_position, on);\
}


static XContext _XmTextGCContext = 0;
static XmTextWidget posToXYCachedWidget = NULL;
static XmTextPosition posToXYCachedPosition;
static Position posToXYCachedX;
static Position posToXYCachedY;

static XtCallbackRec hcallback[] = {
    {HandleHBar, NULL},
    {NULL,       NULL},
};
static XtCallbackRec vcallback[] = {
    {HandleVBar, NULL},
    {NULL,       NULL},
};


static XtResource output_resources[] =
{
    {
      XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
      XtOffsetOf( struct _OutputDataRec, fontlist),
      XmRImmediate, NULL
    },

    {
      XmNwordWrap, XmCWordWrap, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _OutputDataRec, wordwrap),
      XmRImmediate, (XtPointer) False
    },

    {
      XmNblinkRate, XmCBlinkRate, XmRInt, sizeof(int),
      XtOffsetOf( struct _OutputDataRec, blinkrate),
      XmRImmediate, (XtPointer) 500
    },

    {
      XmNcolumns, XmCColumns, XmRShort, sizeof(short),
      XtOffsetOf( struct _OutputDataRec, columns),
      XmRImmediate, (XtPointer) 20
    },

    {
      XmNrows, XmCRows, XmRShort, sizeof(short),
      XtOffsetOf( struct _OutputDataRec, rows),
      XmRImmediate, (XtPointer) 1
    },

    {
      XmNresizeWidth, XmCResizeWidth, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _OutputDataRec, resizewidth),
      XmRImmediate, (XtPointer) False
    },

    {
      XmNresizeHeight, XmCResizeHeight, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _OutputDataRec, resizeheight),
      XmRImmediate, (XtPointer) False
    },

    {
      XmNscrollVertical, XmCScroll, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _OutputDataRec, scrollvertical),
      XmRImmediate,(XtPointer) True
    },

    {
      XmNscrollHorizontal, XmCScroll, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _OutputDataRec, scrollhorizontal),
      XmRImmediate, (XtPointer) True
    },

    {
      XmNscrollLeftSide, XmCScrollSide, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _OutputDataRec, scrollleftside),
      XmRImmediate,(XtPointer) False
    },

    {
      XmNscrollTopSide, XmCScrollSide, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _OutputDataRec, scrolltopside),
      XmRImmediate, (XtPointer) False
    },

    {
      XmNcursorPositionVisible, XmCCursorPositionVisible, XmRBoolean,
      sizeof(Boolean),
      XtOffsetOf( struct _OutputDataRec, cursor_position_visible),
      XmRImmediate, (XtPointer) True
    },

};

void
#ifdef _NO_PROTO
_XmTextFreeContextData(w, clientData, callData)
    Widget w;
    XtPointer clientData;
    XtPointer callData;
#else
_XmTextFreeContextData(
        Widget w,
        XtPointer clientData,
        XtPointer callData )
#endif /* _NO_PROTO */
{
    XmTextContextData ctx_data = (XmTextContextData) clientData;
    Display *display = DisplayOfScreen(ctx_data->screen);
    XtPointer data_ptr;

    if (XFindContext(display, (Window) ctx_data->screen,
                     ctx_data->context, (char **) &data_ptr)) {

       if (ctx_data->type == _XM_IS_PIXMAP_CTX) {
          XFreePixmap(display, (Pixmap) data_ptr);
       } else if (ctx_data->type != '\0') {
          if (data_ptr)
             XtFree((char *) data_ptr);
       }

       XDeleteContext (display, (Window) ctx_data->screen, ctx_data->context);
    }

    XtFree ((char *) ctx_data);
}


static TextGCData 
#ifdef _NO_PROTO
GetTextGCData( w )
        Widget w ;
#else
GetTextGCData(
        Widget w )
#endif /* _NO_PROTO */
{
   TextGCData gc_data;
   Display *display = XtDisplay(w);
   Screen *screen = XtScreen(w);

   if (_XmTextGCContext == 0)
      _XmTextGCContext = XUniqueContext();

   if (XFindContext(display, (Window)screen, _XmTextGCContext, 
	(char **) &gc_data)) {
       XmTextContextData ctx_data;
       Widget xm_display = (Widget) XmGetXmDisplay(display);

       ctx_data = (XmTextContextData) XtMalloc(sizeof(XmTextContextDataRec));

       ctx_data->screen = screen;
       ctx_data->context = _XmTextGCContext;
       ctx_data->type = _XM_IS_GC_DATA_CTX;

       gc_data = (TextGCData) XtCalloc(1, sizeof(TextGCDataRec));

       XtAddCallback(xm_display, XmNdestroyCallback,
                     (XtCallbackProc) _XmTextFreeContextData,
		     (XtPointer) ctx_data);

       XSaveContext(display, (Window)screen, _XmTextGCContext, 
		    (char *)gc_data);
       gc_data->tw = (XmTextWidget) w;
   }

   if (gc_data->tw == NULL) gc_data->tw = (XmTextWidget) w;

   return gc_data;

}

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
static void 
#ifdef _NO_PROTO
_XmTextDrawShadow( tw )
        XmTextWidget tw ;
#else
_XmTextDrawShadow(
        XmTextWidget tw )
#endif /* _NO_PROTO */
{
   if (XtIsRealized(tw)) {
      if (tw->primitive.shadow_thickness > 0)
         _XmDrawShadows (XtDisplay (tw), XtWindow (tw),
                        tw->primitive.bottom_shadow_GC,
                        tw->primitive.top_shadow_GC,
                        tw->primitive.highlight_thickness,
                        tw->primitive.highlight_thickness,
                        tw->core.width -
			     2 * tw->primitive.highlight_thickness,
                        tw->core.height -
			     2 * tw->primitive.highlight_thickness,
                        tw->primitive.shadow_thickness,
			XmSHADOW_OUT);

      if (tw -> primitive.highlighted)
      {   if(    ((XmTextWidgetClass) XtClass( tw))
                                        ->primitive_class.border_highlight    )
          {   (*((XmTextWidgetClass) XtClass( tw))
                            ->primitive_class.border_highlight)( (Widget) tw) ;
              }
          }
      else if (_XmDifferentBackground( (Widget) tw, XtParent (tw)))
      {   if(    ((XmTextWidgetClass) XtClass( tw))
                                      ->primitive_class.border_unhighlight    )
          {   (*((XmTextWidgetClass) XtClass( tw))
                          ->primitive_class.border_unhighlight)( (Widget) tw) ;
              }
          } 
   }
}

/* ARGSUSED */
void
#ifdef _NO_PROTO
_XmTextResetClipOrigin(tw, position, clip_mask_reset)
	XmTextWidget tw;
	XmTextPosition position;
	Boolean clip_mask_reset;
#else /* _NO_PROTO */
_XmTextResetClipOrigin(
	XmTextWidget tw,
	XmTextPosition position,
#if NeedWidePrototypes
	int clip_mask_reset)
#else
	Boolean clip_mask_reset)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   OutputData data = tw->text.output->data;

   unsigned long valuemask = (GCTileStipXOrigin | GCTileStipYOrigin |
                             GCClipXOrigin | GCClipYOrigin);
   XGCValues values;
   int x, y, clip_mask_x, clip_mask_y;
   Position x_pos, y_pos;

   if (!XtIsRealized((Widget)tw)) return;

   if (!PosToXY(tw, tw->text.cursor_position, &x_pos, &y_pos)) return;

   x = (int) x_pos; y = (int) y_pos;

   x -=(data->cursorwidth >> 1) + 1;
   clip_mask_y = y = (y + data->font_descent) - data->cursorheight;

                  
   if (x < tw->primitive.highlight_thickness +
       tw->primitive.shadow_thickness + (int)(tw->text.margin_width)){
          clip_mask_x = tw->primitive.highlight_thickness +
          tw->primitive.shadow_thickness + (int)(tw->text.margin_width);
   } else
     clip_mask_x = x;

   if (clip_mask_reset) {
      values.ts_x_origin = x;
      values.ts_y_origin = y;
      values.clip_x_origin = clip_mask_x;
      values.clip_y_origin = clip_mask_y;
      XChangeGC(XtDisplay((Widget)tw), data->imagegc, valuemask, &values);
   }
   else
      XSetTSOrigin(XtDisplay((Widget)tw), data->imagegc, x, y);
}

/*
 * Make sure the cached GC has the clipping rectangle
 * set to the current widget.
 */
static void 
#ifdef _NO_PROTO
CheckHasRect( tw )
        XmTextWidget tw ;
#else
CheckHasRect(
        XmTextWidget tw )
#endif /* _NO_PROTO */
{
    OutputData data = tw->text.output->data;
    TextGCData gc_data = GetTextGCData((Widget)tw);

    if (data->has_rect && gc_data->tw != tw) {
       if (gc_data->tw)
         gc_data->tw->text.output->data->has_rect = False;
       data->has_rect = False;
    }
    gc_data->tw = tw;
}

static void 
#ifdef _NO_PROTO
XmSetFullGC( tw, gc )
        XmTextWidget tw ;
        GC gc ;
#else
XmSetFullGC(
        XmTextWidget tw,
        GC gc )
#endif /* _NO_PROTO */
{
    XRectangle ClipRect;

    ClipRect.x = tw->primitive.highlight_thickness +
                 tw->primitive.shadow_thickness;
    ClipRect.y = tw->primitive.highlight_thickness +
                 tw->primitive.shadow_thickness;
    ClipRect.width = tw->core.width - (2 *(tw->primitive.highlight_thickness +
                                             tw->primitive.shadow_thickness));
    ClipRect.height = tw->core.height - (2 *(tw->primitive.highlight_thickness +
                                             tw->primitive.shadow_thickness));

    XSetClipRectangles(XtDisplay(tw), gc, 0, 0, 
                        &ClipRect, 1, Unsorted);
}

static void 
#ifdef _NO_PROTO
XmResetSaveGC( tw, gc )
        XmTextWidget tw ;
        GC gc ;
#else
XmResetSaveGC(
        XmTextWidget tw,
        GC gc )
#endif /* _NO_PROTO */
{
    XRectangle ClipRect;

    ClipRect.x = 0;
    ClipRect.y = 0;
    ClipRect.width = tw->core.width;
    ClipRect.height = tw->core.height;

    XSetClipRectangles(XtDisplay(tw), gc, 0, 0, &ClipRect, 1, Unsorted);
}


static void 
#ifdef _NO_PROTO
GetRect( tw, rect )
        XmTextWidget tw ;
        XRectangle *rect ;
#else
GetRect(
        XmTextWidget tw,
        XRectangle *rect )
#endif /* _NO_PROTO */
{
  Dimension margin_width = tw->text.margin_width +
                           tw->primitive.shadow_thickness +
                           tw->primitive.highlight_thickness;
  Dimension margin_height = tw->text.margin_height +
                           tw->primitive.shadow_thickness +
                           tw->primitive.highlight_thickness;

  if (margin_width < tw->core.width)
     rect->x = margin_width;
  else
     rect->x = tw->core.width;

  if (margin_height < tw->core.height)
     rect->y = margin_height;
  else
     rect->y = tw->core.height;

  if ((2 * margin_width) < tw->core.width)
     rect->width = (int) tw->core.width - (2 * margin_width);
  else
     rect->width = 0;

  if ((2 * margin_height) < tw->core.height)
     rect->height = (int) tw->core.height - (2 * margin_height);
  else
     rect->height = 0;
}

static void 
#ifdef _NO_PROTO
XmSetMarginGC( tw, gc )
        XmTextWidget tw ;
        GC gc ;
#else
XmSetMarginGC(
        XmTextWidget tw,
        GC gc )
#endif /* _NO_PROTO */
{
  XRectangle ClipRect;

  GetRect(tw, &ClipRect);
  XSetClipRectangles(XtDisplay(tw), gc, 0, 0, &ClipRect, 1,
                     Unsorted);
}


/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
/*
 * Set new clipping rectangle for text widget.  This is
 * done on each focus in event since the text widgets
 * share the same GC.
 */
void 
#ifdef _NO_PROTO
_XmTextAdjustGC( tw )
        XmTextWidget tw;
#else
_XmTextAdjustGC(
        XmTextWidget tw)
#endif /* _NO_PROTO */
{
  OutputData data = tw->text.output->data;
  unsigned long valuemask = (GCForeground | GCBackground);
  XGCValues values;

  if (!XtIsRealized((Widget)tw)) return;

  XmSetMarginGC(tw, data->gc);
  XmSetFullGC(tw, data->imagegc);
  XmResetSaveGC(tw, data->save_gc);

  _XmTextResetClipOrigin(tw, tw->text.cursor_position, False);

 /* Restore cached save gc to state correct for this instantiation */
  if (data->save_gc){
     valuemask = (GCFunction | GCBackground | GCForeground);
     values.function = GXcopy;
     values.foreground = tw->primitive.foreground ;
     values.background = tw->core.background_pixel;
     XChangeGC(XtDisplay(tw), data->save_gc, valuemask, &values);
  }

 /* Restore cached text gc to state correct for this instantiation */

  if (data->gc){
     if (!data->use_fontset && (data->font != NULL)){
        valuemask |= GCFont;
        values.font = data->font->fid;
     }
     valuemask |= GCGraphicsExposures;
     values.graphics_exposures = (Bool) True;
     values.foreground = tw->primitive.foreground ^ tw->core.background_pixel;
     values.background = 0;
     XChangeGC(XtDisplay(tw), data->gc, valuemask, &values);
  }

  _XmTextToggleCursorGC((Widget)tw);
  
  data->has_rect = True;
}


static void 
#ifdef _NO_PROTO
XmSetNormGC( tw, gc, change_stipple, stipple )
        XmTextWidget tw ;
        GC gc ;
#if NeedWidePrototypes
	int change_stipple;
	int stipple;
#else
	Boolean change_stipple;
	Boolean stipple;
#endif /* NeedWidePrototypes */
#else
XmSetNormGC(
        XmTextWidget tw,
        GC gc,
#if NeedWidePrototypes
	int change_stipple,
	int stipple)
#else
	Boolean change_stipple,
	Boolean stipple)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    unsigned long valueMask = (GCForeground | GCBackground);
    XGCValues values;
    OutputData data = tw->text.output->data;

    values.foreground = tw->primitive.foreground;
    values.background = tw->core.background_pixel;
    if (change_stipple) {
       valueMask |= GCTile | GCFillStyle;
       values.tile = data->stipple_tile;
       if (stipple) values.fill_style = FillTiled;
       else values.fill_style = FillSolid;
    }

    XChangeGC(XtDisplay(tw), gc, valueMask, &values);
}



static void 
#ifdef _NO_PROTO
InvertImageGC( tw)
        XmTextWidget tw ;
#else
InvertImageGC(
        XmTextWidget tw)
#endif /* _NO_PROTO */
{
    unsigned long valueMask = (GCForeground | GCBackground);
    XGCValues values;
    Pixel temp;
    OutputData data = tw->text.output->data;
    Display *dpy = XtDisplay((Widget)tw);

    if (tw->text.input->data->overstrike) {
      data->have_inverted_image_gc = !data->have_inverted_image_gc;
      return; /* bg & fg are the same */
    }

    XGetGCValues(dpy, data->imagegc, valueMask, &values);

    temp = values.foreground;
    values.foreground = values.background;
    values.background = temp;

    XChangeGC(dpy, data->imagegc, valueMask, &values);

    if (data->have_inverted_image_gc)
       data->have_inverted_image_gc = False;
    else
       data->have_inverted_image_gc = True;
}

static void
#ifdef _NO_PROTO
XmSetInvGC( tw, gc )
        XmTextWidget tw ;
        GC gc ;
#else
XmSetInvGC(
        XmTextWidget tw,
        GC gc)
#endif /* _NO_PROTO */
{
    unsigned long valueMask = (GCForeground | GCBackground);
    XGCValues values;

    values.foreground = tw->core.background_pixel;
    values.background = tw->primitive.foreground;

    XChangeGC(XtDisplay(tw), gc, valueMask, &values);
}


static int 
#ifdef _NO_PROTO
_FontStructFindWidth( tw, x, block, from, to )
        XmTextWidget tw ;
        int x ;
        XmTextBlock block ;
        int from ;
        int to ;
#else
_FontStructFindWidth(
        XmTextWidget tw,
        int x,                  /* Starting position (needed for tabs) */
        XmTextBlock block,
        int from,               /* How many bytes in to start measuring */
        int to )                /* How many bytes in to stop measuring */
#endif /* _NO_PROTO */
{
    OutputData data = tw->text.output->data;
    XFontStruct *font = data->font;
    char *ptr;
    unsigned char c;
    int i, csize; 
    int result = 0;

    if (tw->text.char_size != 1) {
      int dummy;
      XCharStruct overall;

      for (i = from, ptr = block->ptr + from; i < to; i +=csize, ptr += csize){
          csize = mblen(ptr, tw->text.char_size);
          if (csize <= 0) break;
          c = (unsigned char) *ptr;
	  if (csize == 1){
             if (c == '\t'){
	        result += (data->tabwidth -
		           ((x + result - data->leftmargin) % data->tabwidth));
             } else {
              if (font->per_char && (c >= font->min_char_or_byte2 && 
                                     c <= font->max_char_or_byte2))
                 result += font->per_char[c - font->min_char_or_byte2].width;
              else
                 result += font->min_bounds.width;
             }
	  } else {
             XTextExtents(data->font, ptr, csize, &dummy, &dummy, &dummy,
                          &overall);
	     result += overall.width;
          }
       }
    } else {
       for (i=from, ptr = block->ptr + from; i<to ; i++, ptr++) {
          c = (unsigned char) *ptr;
          if (c == '\t')
              result += (data->tabwidth -
                         ((x + result - data->leftmargin) % data->tabwidth));
          /* %%% Do something for non-printing? */
          else {
              if (font->per_char && (c >= font->min_char_or_byte2 && 
                                     c <= font->max_char_or_byte2))
                 result += font->per_char[c - font->min_char_or_byte2].width;
              else
                 result += font->min_bounds.width;
          }
      }
   }
   return result;
}

static int 
#ifdef _NO_PROTO
FindWidth( tw, x, block, from, to)
        XmTextWidget tw ;
        int x ;
        XmTextBlock block ;
        int from ;
        int to ;
#else
FindWidth(
        XmTextWidget tw,
        int x,                  /* Starting position (needed for tabs) */
        XmTextBlock block,
        int from,               /* How many bytes in to start measuring */
        int to)                 /* How many bytes in to stop measuring */
#endif /* _NO_PROTO */
{
    OutputData data = tw->text.output->data;
    char *ptr;
    unsigned char c;
    int result = 0;
    int tmp;
    int csize = 1;
    int i;

    if (!data->use_fontset)
        return _FontStructFindWidth(tw, x, block, from, to);

    if (to > block->length)
       to = block->length;
    if (from > to){
       tmp = to;
       to = from;
       from = tmp;
    }

    if (to == from || to == 0) return 0;

    if (tw->text.char_size != 1) {
       for (i = from, ptr = block->ptr + from; i < to; i +=csize, ptr += csize){
	  csize = mblen(ptr, tw->text.char_size);
	  if (csize <= 0) break;
          c = (unsigned char) *ptr;
	  if (csize == 1 && c == '\t')
	     result += (data->tabwidth -
		        ((x + result - data->leftmargin) % data->tabwidth));
	  else
	     result += XmbTextEscapement((XFontSet)data->font, ptr, csize);
       }
	  
    } else { /* no need to pay for mblen if we know all chars are 1 byte */
       for (i = from, ptr = block->ptr + from; i < to; i++, ptr++) {
          c = (unsigned char) *ptr;
          if (c == '\t')
	     result += (data->tabwidth -
			   ((x + result - data->leftmargin) % data->tabwidth));
	  else
	     result += XmbTextEscapement((XFontSet)data->font, ptr, 1);
       }
    } 
    return result;
}

/* Semi-public routines. */

static XmTextPosition 
#ifdef _NO_PROTO
XYToPos( widget, x, y )
        XmTextWidget widget ;
        Position x ;
        Position y ;
#else
XYToPos(
        XmTextWidget widget,
#if NeedWidePrototypes
        int x,
        int y )
#else
        Position x,
        Position y )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    LineTableExtra extra;
    int i, width, lastwidth, length;
    int num_chars = 0;
    int num_bytes = 0;
    LineNum line = 0;
    XmTextPosition start, end, laststart;
    XmTextBlockRec block;
    int delta = 0;

    start = end = laststart = 0;

    x += data->hoffset;
    y -= data->topmargin;
   /* take care of negative y case */
    if (data->lineheight) {
       if (y < 0) {
          delta = ((int)(y + 1)/ (int) data->lineheight) - 1;
          y = 0;
       }
       line = y / data->lineheight;
    }
    if (line > _XmTextNumLines(widget)) line = _XmTextNumLines(widget);
    _XmTextLineInfo(widget, line, &start, &extra);
    if (start == PASTENDPOS)
        return (*widget->text.source->Scan)(widget->text.source, 0,
					    XmSELECT_ALL, XmsdRight, 1, FALSE);
    _XmTextLineInfo(widget, line+1, &end, &extra);
    end = (*widget->text.source->Scan)(widget->text.source, end,
				          XmSELECT_POSITION, XmsdLeft, 1, TRUE);
    width = lastwidth = data->leftmargin;
    if (start >= end && !delta) return start;

   /* if original y was negative, we need to find new laststart */
    if (delta && start > 0) {
       end = (*widget->text.source->Scan)(widget->text.source, start,
                                       XmSELECT_POSITION, XmsdLeft, 1, TRUE);;
       start = _XmTextFindScroll(widget, start, delta);
    }

    do {
        laststart = start;
        start = (*widget->text.source->ReadSource)(widget->text.source, start,
						   end, &block);
        length = block.length;
	if ((int)widget->text.char_size > 1) {
	   for (i = num_chars = 0;
                i < length && width < x && num_bytes >= 0;
                i += num_bytes, num_chars++) {
                  lastwidth = width;
                  num_bytes = mblen(&block.ptr[i], (int)widget->text.char_size);
                  width += FindWidth(widget, width, &block, i, i + num_bytes);
                }
           i = num_chars;
        } else {
           for (i=0 ; i<length && width < x; i++) {
               lastwidth = width;
               width += FindWidth(widget, width, &block, i, i+1);
           }
	}
    } while (width < x && start < end && laststart != end);

    if (abs(lastwidth - x) < abs(width - x)) i--;
    return (*widget->text.source->Scan)(widget->text.source, laststart,
				        XmSELECT_POSITION, (i < 0) ?
					XmsdLeft : XmsdRight, abs(i), TRUE);
}

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmTextShouldWordWrap( widget )
        XmTextWidget widget ;
#else
_XmTextShouldWordWrap(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    return (ShouldWordWrap(data, widget));
}


/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmTextScrollable( widget )
        XmTextWidget widget ;
#else
_XmTextScrollable(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    return (data->scrollvertical && 
            XtClass(widget->core.parent) == xmScrolledWindowWidgetClass);
}

static Boolean 
#ifdef _NO_PROTO
PosToXY( widget, position, x, y )
        XmTextWidget widget ;
        XmTextPosition position ;
        Position *x ;
        Position *y ;
#else
PosToXY(
        XmTextWidget widget,
        XmTextPosition position,
        Position *x,
        Position *y )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    LineNum line;
    XmTextPosition linestart;
    LineTableExtra extra;
    XmTextBlockRec block;

    if (widget == posToXYCachedWidget && position == posToXYCachedPosition) {
        *x = posToXYCachedX;
        *y = posToXYCachedY;
        return TRUE;
    }
    line = _XmTextPosToLine(widget, position);
    if (line == NOLINE || line >= data->number_lines) return FALSE;
    *y = data->topmargin + line * data->lineheight + data->font_ascent;
    *x = data->leftmargin;
    _XmTextLineInfo(widget, line, &linestart, &extra);
    while (linestart < position) {
       linestart = (*widget->text.source->ReadSource)(widget->text.source,
						      linestart, position, 
						      &block);
       *x += FindWidth(widget, *x, &block, 0, block.length);
    }
    *x -= data->hoffset;
    posToXYCachedWidget = widget;
    posToXYCachedPosition = position;
    posToXYCachedX = *x;
    posToXYCachedY = *y;
    return TRUE;
}

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
XmTextPosition 
#ifdef _NO_PROTO
_XmTextFindLineEnd( widget, position, extra )
     XmTextWidget widget ;
     XmTextPosition position ;
     LineTableExtra *extra ;
#else
_XmTextFindLineEnd(XmTextWidget widget,
		   XmTextPosition position,
		   LineTableExtra *extra )
#endif /* _NO_PROTO */
{
  OutputData data = widget->text.output->data;
  XmTextPosition lastChar, lineEnd, nextLeft, nextBreak, lastBreak, oldpos;
  XmTextPosition startpos;
  XmTextBlockRec block;
  int x, lastX, goalwidth, length, i;
  int num_bytes = 0;
  
  lastChar = (*widget->text.source->Scan)(widget->text.source, position,
					  XmSELECT_LINE, XmsdRight, 1, FALSE);
  lastBreak = startpos = position;
  x = lastX = data->leftmargin;
  goalwidth = widget->text.inner_widget->core.width - data->rightmargin;
  while (position < lastChar) {
    nextLeft = (*widget->text.source->Scan)(widget->text.source, position,
                                            XmSELECT_WHITESPACE, XmsdRight,
                                            1, FALSE);
    nextBreak = (*widget->text.source->Scan)(widget->text.source, nextLeft,
					     XmSELECT_WHITESPACE, XmsdRight,
					     1, TRUE);
    while (position < nextLeft) {
      position = (*widget->text.source->ReadSource)(widget->text.source,
						    position, nextLeft,
						    &block);
      length = block.length;
      x += FindWidth(widget, x, &block, 0, block.length);
      if (x > goalwidth) {
	if (lastBreak > startpos) {
	  if (lastX <= goalwidth) {/* word wrap is being performed */
	    return lastBreak;
	  }
	  x = lastX;
	  oldpos = position = lastBreak;
	  while (x > goalwidth && position > startpos) {
	    oldpos = position;
	    position = 
	      (*widget->text.source->Scan) (widget->text.source, position,
					    XmSELECT_POSITION, XmsdLeft,
					    1, TRUE);
	    (void) (*widget->text.source->ReadSource) (widget->text.source,
						       position, oldpos, 
						       &block);
	    num_bytes = mblen(block.ptr, (int)widget->text.char_size);
	    /* Pitiful error handling of -1, but what else can you do? */
	    if (num_bytes < 0) 
	      break;
	    x -= FindWidth(widget, x, &block, 0, num_bytes);
	  }
	  if (extra) {
	    *extra = (LineTableExtra)
	      XtMalloc((unsigned) sizeof(LineTableExtraRec));
	    (*extra)->wrappedbychar = TRUE;
	    (*extra)->width = 0;
	  }
	  return oldpos; /* Allows one whitespace char to appear */
	  /* partially off the edge. */
	}
	if (extra) {
	  *extra = (LineTableExtra)
	    XtMalloc((unsigned) sizeof(LineTableExtraRec));
	  (*extra)->wrappedbychar = TRUE;
	  (*extra)->width = 0;
	}
	if ((int)widget->text.char_size == 1) {
	  for (i=length - 1 ; i>=0 && x > goalwidth ; i--) {
	    x -= FindWidth(widget, x, &block, i, i + 1);
	    position = 
	      (*widget->text.source->Scan)(widget->text.source,
					   position, 
					   XmSELECT_POSITION,
					   XmsdLeft, 1, TRUE);
	  }
	  return position;
	} else {
	  char tmp_cache[200];
	  wchar_t * tmp_wc;
	  Cardinal tmp_wc_size;
	  char tmp_char[MB_LEN_MAX];
	  int num_chars = 0;
	  XmTextBlockRec mini_block;
	  
	  /* If 16-bit data, convert the char* to wchar_t*... this
	   * allows us to scan backwards through the text one
	   * character at a time.  Without wchar_t, we would have
	   * to continually scan from the start of the string to
	   * find the byte offset of character n-1. 
	   */
	  mini_block.ptr = tmp_char;
	  num_chars = _XmTextCountCharacters(block.ptr, block.length);
	  tmp_wc_size = (num_chars + 1) * sizeof(wchar_t);
	  tmp_wc = (wchar_t *) XmStackAlloc(tmp_wc_size, tmp_cache);
	  num_chars = mbstowcs(tmp_wc, block.ptr, num_chars);
	  if (num_chars > 0) {
	    for (i = num_chars - 1; i >= 0 && x > goalwidth; i--) {
	      mini_block.length = wctomb(mini_block.ptr, tmp_wc[i]);
	      if (mini_block.length < 0) mini_block.length = 0;
	      x -= FindWidth(widget, x, &mini_block,
			     0, mini_block.length);
	      position = 
		(*widget->text.source->Scan)(widget->text.source,
					     position, 
					     XmSELECT_POSITION,
					     XmsdLeft, 1, TRUE);
	    }
	  }
	  XmStackFree((char*)tmp_wc, tmp_cache);
	} /* end multi-byte handling */
	return position;
      }
    }
    while (position < nextBreak) {
      position = 
	(*widget->text.source->ReadSource)(widget->text.source,
					   position, nextBreak, &block);
      length = block.length;
      x += FindWidth(widget, x, &block, 0, block.length);
    }
    lastBreak = nextBreak;
    lastX = x;
  }
  lineEnd = (*widget->text.source->Scan)(widget->text.source, lastChar,
					 XmSELECT_LINE, XmsdRight, 1, TRUE);
  if (lineEnd != lastChar) return lineEnd;
  else return PASTENDPOS;
}

static XtGeometryResult 
#ifdef _NO_PROTO
TryResize( widget, width, height )
        XmTextWidget widget ;
        Dimension width ;
        Dimension height ;
#else
TryResize(
        XmTextWidget widget,
#if NeedWidePrototypes
        int width,
        int height )
#else
        Dimension width,
        Dimension height )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XtGeometryResult result;
    Dimension origwidth = widget->text.inner_widget->core.width;
    Dimension origheight = widget->text.inner_widget->core.height;
    XtWidgetGeometry request, reply;

    if (origwidth != width) {
       request.request_mode = CWWidth;
       request.width = width;
    } else
       request.request_mode = (XtGeometryMask)0;

    if (origheight != height) {
       request.request_mode |= CWHeight;
       request.height = height;
    }

    /* requesting current size */
    if (request.request_mode == (XtGeometryMask)0) return XtGeometryNo;

    result = XtMakeGeometryRequest(widget->text.inner_widget, &request, &reply);

    if (result == XtGeometryAlmost) {
       if (request.request_mode & CWWidth)
          request.width = reply.width;
       if (request.request_mode & CWHeight)
          request.height = reply.height;

       result = XtMakeGeometryRequest(widget->text.inner_widget, &request,
                                         &reply);
       if (result == XtGeometryYes) {
          result = XtGeometryNo;
          if (((request.request_mode & CWWidth) && reply.width != origwidth) ||
              ((request.request_mode & CWHeight) && reply.height != origheight))
             result = XtGeometryYes;
       }
       return result;
    }


    if (result == XtGeometryYes) {
       /* Some brain damaged geometry managers return XtGeometryYes and
          don't change the widget's size. */
       if (((request.request_mode & CWWidth) &&
            widget->text.inner_widget->core.width != width) ||
           ((request.request_mode & CWHeight) &&
            widget->text.inner_widget->core.height != height) ||
           ((request.request_mode == (CWWidth & CWHeight)) &&
            (widget->text.inner_widget->core.width == origwidth &&
             widget->text.inner_widget->core.height == origheight)))
          result = XtGeometryNo;
    }
    return result;
}

void 
#ifdef _NO_PROTO
_XmRedisplayHBar( widget )
        XmTextWidget widget ;
#else
_XmRedisplayHBar(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    int value, sliderSize, maximum,new_sliderSize;
    Arg args[3];

    static Arg arglist[] = {
        {XmNmaximum, (XtArgVal)NULL},
        {XmNvalue, (XtArgVal)NULL},
        {XmNsliderSize, (XtArgVal)NULL},
    };

    if (!(data->scrollhorizontal &&
         (XtClass(widget->core.parent) == xmScrolledWindowWidgetClass)) ||
	 data->suspend_hoffset || widget->text.disable_depth != 0 ||
 	widget->core.being_destroyed)
       return;

    ChangeHOffset(widget, data->hoffset, False);/* Makes sure that hoffset is */
                                                /* still reasonable. */

    new_sliderSize = widget->text.inner_widget->core.width
             - (data->leftmargin + data->rightmargin);

    if (new_sliderSize < 1) new_sliderSize = 1;
    if (new_sliderSize > data->scrollwidth) new_sliderSize = data->scrollwidth;

    XtSetArg(args[0], XmNmaximum, &maximum);
    XtSetArg(args[1], XmNvalue, &value);
    XtSetArg(args[2], XmNsliderSize, &sliderSize);
    XtGetValues(data->hbar, args, XtNumber(args));

    if ((maximum != data->scrollwidth || 
     value != data->hoffset || 
     sliderSize != new_sliderSize) &&
         !(sliderSize == maximum && new_sliderSize == data->scrollwidth)) {
       arglist[0].value = (XtArgVal) data->scrollwidth;
       arglist[1].value = (XtArgVal) data->hoffset;
       arglist[2].value = (XtArgVal) new_sliderSize;

       data->ignorehbar = TRUE;
       XtSetValues(data->hbar, arglist, XtNumber(arglist));
       data->ignorehbar = FALSE;
    }
}

static int 
#ifdef _NO_PROTO
CountLines( widget, start, end )
        XmTextWidget widget ;
        XmTextPosition start ;
        XmTextPosition end ;
#else
CountLines(
        XmTextWidget widget,
        XmTextPosition start,
        XmTextPosition end )
#endif /* _NO_PROTO */
{
  register XmTextLineTable line_table;
  register unsigned int t_index;
  register unsigned int max_index = 0;
  int numlines = 0;

  line_table = widget->text.line_table;
  t_index = widget->text.table_index;

  max_index = widget->text.total_lines - 1;

 /* look forward to find the current record */
  if (line_table[t_index].start_pos < (unsigned int) start) {
     while (t_index <= max_index &&
            line_table[t_index].start_pos < (unsigned int) start) t_index++;
  } else {
  /* look backward to find the current record */
     while (t_index &&
            line_table[t_index].start_pos > (unsigned int) start) t_index--;
  }

  while(line_table[t_index].start_pos < end) {
     t_index++;
     numlines++;
  }

  return (numlines);
}

void 
#ifdef _NO_PROTO
_XmChangeVSB( widget )
        XmTextWidget widget ;
#else
_XmChangeVSB(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    static Arg args[3];
    int local_total;
    int new_size;

    if (widget->text.disable_depth != 0) return;
    if (widget->core.being_destroyed) return;

    if (!widget->text.top_character)
      widget->text.top_line = 0;
    else 
      widget->text.top_line = _XmTextGetTableIndex(widget,
						   widget->text.top_character);

   if (widget->text.top_line > widget->text.total_lines)
      widget->text.top_line = widget->text.total_lines;

   if (widget->text.top_line + widget->text.number_lines >
                                       widget->text.total_lines)
      local_total = widget->text.top_line + widget->text.number_lines;
   else
      local_total = widget->text.total_lines;

   if (data->vbar){
      XtSetArg(args[0], XmNmaximum, local_total);
      XtSetArg(args[1], XmNvalue, widget->text.top_line);

      if (local_total >= widget->text.number_lines)
	new_size = widget->text.number_lines;
      else
	new_size = local_total;
      if (new_size + widget->text.top_line > local_total)
        new_size = local_total - widget->text.top_line;

      XtSetArg(args[2], XmNsliderSize, new_size);
      data->ignorevbar = TRUE;
      XtSetValues(data->vbar, args, XtNumber(args));
      data->ignorevbar = FALSE;
   }
}


static void 
#ifdef _NO_PROTO
TextFindNewWidth( widget, widthRtn )
        XmTextWidget widget ;
        Dimension *widthRtn ;
#else
TextFindNewWidth(
        XmTextWidget widget,
        Dimension *widthRtn)
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    XmTextPosition start;
    Dimension newwidth;
    
    newwidth = 0;

    if (data->resizeheight && widget->text.total_lines > data->number_lines) {
       int i;
       XmTextPosition linestart, position;
       Dimension text_width;
       XmTextBlockRec block;

       i = _XmTextGetTableIndex(widget, widget->text.top_character);
       for (linestart = widget->text.top_character;
	    i + 1 < widget->text.total_lines; i++) {
           text_width = data->leftmargin;
	   position = widget->text.line_table[i + 1].start_pos - 1;
	   while (linestart < position) {
             linestart = (*widget->text.source->ReadSource)
			     (widget->text.source, linestart, position, &block);
             text_width += FindWidth(widget, text_width, 
					&block, 0, block.length);
           }
	   text_width += data->rightmargin;
           if (text_width > newwidth) newwidth = text_width;
       }
       text_width = data->leftmargin;
       position = widget->text.last_position;
       while (linestart < position) {
         linestart = (*widget->text.source->ReadSource)
		          (widget->text.source, linestart, position, &block);
         text_width += FindWidth(widget, text_width,
					&block, 0, block.length);
       }
       text_width += data->rightmargin;
       if (text_width > newwidth) newwidth = text_width;
    } else {
       LineNum l;
       LineTableExtra extra;

       for (l = 0 ; l < data->number_lines ; l++) {
	   _XmTextLineInfo(widget, l, &start, &extra);
	   if (extra && newwidth < extra->width) newwidth = extra->width;
       }
    }

    *widthRtn = newwidth;
}


static void 
#ifdef _NO_PROTO
TextFindNewHeight( widget, position, heightRtn )
        XmTextWidget widget ;
        XmTextPosition position ;
        Dimension *heightRtn ;
#else
TextFindNewHeight(
        XmTextWidget widget,
        XmTextPosition position,
        Dimension *heightRtn)
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    XmTextPosition first_position, start;
    LineTableExtra extra;

    *heightRtn = widget->text.total_lines * data->lineheight +
                 data->topmargin + data->bottommargin;

    _XmTextLineInfo(widget, (LineNum) 0, &start, &extra);

    if (start > 0) {
	first_position = (*widget->text.source->Scan)
				    (widget->text.source, start,
				     XmSELECT_ALL, XmsdLeft, 1, TRUE);
	if (start > first_position) {
	    _XmTextSetTopCharacter((Widget)widget, start);
	    return;
	}
    }
/*
    newheight = data->number_lines * data->lineheight +
                data->topmargin + data->bottommargin;

    _XmTextLineInfo(widget, (LineNum) 0, &start, &extra);

    if (start > 0) {
	first_position = (*widget->text.source->Scan)
				    (widget->text.source, start,
				     XmSELECT_ALL, XmsdLeft, 1, TRUE);
	if (start > first_position) {
	    _XmTextSetTopCharacter((Widget)widget, start);
	    return;
	}
    }

    if (position != PASTENDPOS) {
	newheight += data->lineheight;
    } else {
	numlines = _XmTextNumLines(widget);
	_XmTextLineInfo(widget, numlines - 1, &start, &extra);
*/
       /*
	*  "i" is used for getting the "start" position at each
	*  line beginning at the bottom line.  Since "start" is 
	*  already gotten for the bottom line, "i" will start at 2.i
	*/
/*
	for (i = 2; start == PASTENDPOS && newheight > data->minheight && 
			   i <= numlines; i++) {
	    newheight -= data->lineheight;
	    _XmTextLineInfo(widget, numlines - i, &start, &extra);
	}
    }

    if (newheight < data->minheight) newheight = data->minheight;

    *heightRtn = newheight;
*/
}


static void 
#ifdef _NO_PROTO
CheckForNewSize( widget, position )
        XmTextWidget widget ;
        XmTextPosition position ;
#else
CheckForNewSize(
        XmTextWidget widget,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    Dimension newwidth, newheight;

    if (data->scrollvertical &&
        XtClass(widget->core.parent) == xmScrolledWindowWidgetClass &&
	!widget->text.vsbar_scrolling)
        _XmChangeVSB(widget);


    if (widget->text.in_resize || widget->text.in_expose) {
       if (data->scrollhorizontal &&
            XtClass(widget->core.parent) == xmScrolledWindowWidgetClass) {
	 TextFindNewWidth(widget, &newwidth);
         newwidth -= (data->rightmargin + data->leftmargin);
         if (newwidth != data->scrollwidth &&
	     !data->suspend_hoffset) {
            if (newwidth) data->scrollwidth = newwidth;
            else data->scrollwidth = 1;
            _XmRedisplayHBar(widget);
         }
       }
    } else {
       if (data->resizewidth  || (data->scrollhorizontal &&
	   XtClass(widget->core.parent) == xmScrolledWindowWidgetClass))
       {
	   TextFindNewWidth(widget, &newwidth);
	   if (data->scrollhorizontal &&
	       XtClass(widget->core.parent) == xmScrolledWindowWidgetClass)
	   {
	       newwidth -= (data->rightmargin + data->leftmargin);
	       if (newwidth != data->scrollwidth &&
		   !data->suspend_hoffset) {
                  if (newwidth) data->scrollwidth = newwidth;
                  else data->scrollwidth = 1;
		  _XmRedisplayHBar(widget);
	       }
	       newwidth = widget->text.inner_widget->core.width;
	   } else if (newwidth < data->minwidth) newwidth = data->minwidth;
       } else newwidth = widget->text.inner_widget->core.width;

       newheight = widget->text.inner_widget->core.height;

       if (data->resizeheight) {
	  TextFindNewHeight(widget, position, &newheight);
       }

       if ((newwidth != widget->text.inner_widget->core.width ||
	    newheight != widget->text.inner_widget->core.height)) {
	  if (widget->text.in_setvalues) {
	     widget->core.width = newwidth;
	     widget->core.height = newheight;
	  } else {
	     if (TryResize(widget, newwidth, newheight) == XtGeometryYes)
	        NotifyResized( (Widget) widget, FALSE);
	     else
	        widget->text.needs_refigure_lines = False;
	  }
       }
   }
}

/* ARGSUSED */
static XtPointer
#ifdef _NO_PROTO
OutputBaseProc( widget, client_data)
      Widget widget;
      XtPointer client_data;
#else
OutputBaseProc(
      Widget widget,
      XtPointer client_data)
#endif /* _NO_PROTO */
{
      XmTextWidget tw = (XmTextWidget) widget;
      return (XtPointer) tw->text.output;
}


/* ARGSUSED */
void
#ifdef _NO_PROTO
_XmTextOutputGetSecResData( secResDataRtn )
      XmSecondaryResourceData *secResDataRtn;
#else
_XmTextOutputGetSecResData(
      XmSecondaryResourceData *secResDataRtn )
#endif /* _NO_PROTO */
{
     XmSecondaryResourceData secResData = XtNew(XmSecondaryResourceDataRec);

     _XmTransformSubResources(output_resources, XtNumber(output_resources),
                              &(secResData->resources), 
			      &(secResData->num_resources));

     secResData->name = NULL;
     secResData->res_class = NULL;
     secResData->client_data = NULL;
     secResData->base_proc = OutputBaseProc;
     *secResDataRtn = secResData;
}

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
int
#ifdef _NO_PROTO
_XmTextGetNumberLines(widget)
	XmTextWidget widget;
#else
_XmTextGetNumberLines(
	XmTextWidget widget)
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    return (data->number_lines);
}

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
/* This routine is used to control foreground vs. background when moving
 * cursor position.  It ensures that when cursor position is changed
 * between "inside the selection" and "outside the selection", that the
 * correct foreground and background are used when "painting" the cursor
 * through the IBeam stencil.
 */
void
#ifdef _NO_PROTO
_XmTextMovingCursorPosition(tw, position)
	XmTextWidget	tw;
	XmTextPosition	position;
#else
_XmTextMovingCursorPosition(
	XmTextWidget	tw,
	XmTextPosition	position)
#endif /* _NO_PROTO */
{
    OutputData data = tw->text.output->data;
    _XmHighlightRec 	*hl_list = tw->text.highlight.list;
    int			i;

    for (i = tw->text.highlight.number - 1 ; i >= 0 ; i--)
       if (position >= hl_list[i].position)
          break;

    if (position == hl_list[i].position) {
       if (data->have_inverted_image_gc)
          InvertImageGC(tw);
    } else if (hl_list[i].mode != XmHIGHLIGHT_SELECTED) {
       if (data->have_inverted_image_gc)
          InvertImageGC(tw);
    } else if (!data->have_inverted_image_gc) {
       InvertImageGC(tw);
    }
}

static Boolean 
#ifdef _NO_PROTO
MeasureLine( widget, line, position, nextpos, extra )
        XmTextWidget widget ;
        LineNum line ;
        XmTextPosition position ;
        XmTextPosition *nextpos ;
        LineTableExtra *extra ;
#else
MeasureLine(
        XmTextWidget widget,
        LineNum line,
        XmTextPosition position,
        XmTextPosition *nextpos,
        LineTableExtra *extra )
#endif /* _NO_PROTO */
{
  OutputData data = widget->text.output->data;
  XmTextPosition temp, last_position;
  XmTextBlockRec block;
  Dimension width;
  
  posToXYCachedWidget = NULL;
  if (extra) *extra = NULL;
  if (line >= data->number_lines) {
    if (data->resizewidth || data->resizeheight ||
	((data->scrollvertical || data->scrollhorizontal) &&
	 XtClass(widget->core.parent) == xmScrolledWindowWidgetClass)) {
      CheckForNewSize(widget, position);
    }
    return(False);
  }
  if (nextpos) {
    if (position == PASTENDPOS) {
      *nextpos = last_position = PASTENDPOS;
    } else {
      if (ShouldWordWrap(data, widget)) {
	*nextpos = _XmTextFindLineEnd(widget, position, extra);
      } else {
	last_position = (*widget->text.source->Scan)(widget->text.source,
						     position, XmSELECT_LINE,
						     XmsdRight, 1, FALSE);
	*nextpos = (*widget->text.source->Scan)(widget->text.source,
						last_position, XmSELECT_LINE,
						XmsdRight, 1, TRUE);
	if (*nextpos == last_position)
	  *nextpos = PASTENDPOS;
	if (extra && (data->resizewidth || (data->scrollhorizontal &&
					    XtClass(widget->core.parent) ==
					    xmScrolledWindowWidgetClass))) {
	  (*extra) = (LineTableExtra)
	    XtMalloc((unsigned) sizeof(LineTableExtraRec));
	  (*extra)->wrappedbychar = FALSE;
	  width = data->leftmargin;
	  temp = position;
	  while (temp < last_position) {
	    temp = (*widget->text.source->ReadSource)
	      (widget->text.source, temp, last_position, &block);
	    width += FindWidth(widget, (Position) width, &block,
			       0, block.length);
	  }
	  (*extra)->width = width + data->rightmargin;
	}
      }
      if (*nextpos == position)
	*nextpos = (*widget->text.source->Scan)(widget->text.source,
						position, XmSELECT_POSITION,
						XmsdRight, 1, TRUE);
    }
  }
  return (True);
}


static void 
#ifdef _NO_PROTO
Draw( widget, line, start, end, highlight )
        XmTextWidget widget ;
        LineNum line ;
        XmTextPosition start ;
        XmTextPosition end ;
        XmHighlightMode highlight ;
#else
Draw(XmTextWidget widget,
     LineNum line,
     XmTextPosition start,
     XmTextPosition end,
     XmHighlightMode highlight )
#endif /* _NO_PROTO */
{
  OutputData data = widget->text.output->data;
  XmTextPosition linestart, nextlinestart;
  LineTableExtra extra;
  XmTextBlockRec block;
  int x, y, length, newx, i;
  int num_bytes = 0;
  int text_border;
  int rightedge = (((int)widget->text.inner_widget->core.width) -
		   data->rightmargin) + data->hoffset;
  Boolean stipple = False;
  
  int width, height;
  int rec_width = 0;
  int rec_height = 0;
  Boolean cleartoend, cleartobottom;
  
  if (!XtIsRealized((Widget) widget)) return;
  _XmTextLineInfo(widget, line+1, &nextlinestart, &extra);
  _XmTextLineInfo(widget, line, &linestart, &extra);
  
  CheckHasRect(widget);

  if (!data->has_rect) _XmTextAdjustGC(widget);
  
  if (!XtSensitive(widget)) stipple = True;
  
  if (linestart == PASTENDPOS) {
    start = end = nextlinestart = PASTENDPOS;
    cleartoend = cleartobottom = TRUE;
  } else if (nextlinestart == PASTENDPOS) {
    nextlinestart = (*widget->text.source->Scan)(widget->text.source, 0,
						 XmSELECT_ALL, XmsdRight, 
						 1, FALSE);
    cleartoend = cleartobottom = (end >= nextlinestart);
    if (start >= nextlinestart) highlight = XmHIGHLIGHT_NORMAL;
  } else {
    cleartobottom = FALSE;
    cleartoend = (end >= nextlinestart);
    if (cleartoend && (!extra || !extra->wrappedbychar))
      end = (*widget->text.source->Scan)(widget->text.source,
					 nextlinestart, XmSELECT_POSITION, 
					 XmsdLeft, 1, TRUE);
  }
  y = data->topmargin + line * data->lineheight + data->font_ascent;
  x = data->leftmargin;
  while (linestart < start && x <= rightedge) {
    linestart = (*widget->text.source->ReadSource)(widget->text.source,
						   linestart, start, &block);
    x += FindWidth(widget, x, &block, 0, block.length);
  }
  
  newx = x;
  
  while (start < end && x <= rightedge) {
    start = (*widget->text.source->ReadSource)(widget->text.source, start,
					       end, &block);
    if ((int)widget->text.char_size == 1) 
      num_bytes = 1;
    else {
      num_bytes = mblen(block.ptr, (int)widget->text.char_size);
      if (num_bytes < 1) num_bytes = 1;
    }
    while (block.length > 0) {
      while (num_bytes == 1 && block.ptr[0] == '\t') {
	newx = x;
	while (block.length > 0 && num_bytes == 1 &&
	       newx - data->hoffset < data->leftmargin) {
	  width = FindWidth(widget, newx, &block, 0, 1);
	  newx += width;
	  
	  if (newx - data->hoffset < data->leftmargin) {
	    block.length--;
	    block.ptr++;
	    x = newx;
	    if ((int)widget->text.char_size != 1){ 
	      /* check if we've got mbyte char */
	      num_bytes = mblen(block.ptr, (int)widget->text.char_size);
	      if (num_bytes < 1) num_bytes = 1;
	    }
	  }
	}
	if (block.length <= 0 || num_bytes != 1 || 
	    block.ptr[0] != '\t') break;
	
	width = FindWidth(widget, x, &block, 0, 1);
	
	if (highlight == XmHIGHLIGHT_SELECTED)
	  XmSetNormGC(widget, data->gc, False, False);
	else
	  XmSetInvGC(widget, data->gc);
	XmSetFullGC(widget, data->gc);
	
	if (((x - data->hoffset) + width) >
	    widget->text.inner_widget->core.width - data->rightmargin)
	  rec_width = (widget->text.inner_widget->core.width -
		       data->rightmargin) - (x - data->hoffset);
	else
	  rec_width = width;
	
	if (y + data->font_descent >
	    widget->text.inner_widget->core.height - data->bottommargin)
	  rec_height = (widget->text.inner_widget->core.height -
			data->bottommargin) - y;
	else
	  rec_height = data->font_ascent + data->font_descent;
	
	XFillRectangle(XtDisplay(widget),
		       XtWindow(widget->text.inner_widget), data->gc, 
		       x - data->hoffset, y - data->font_ascent,
		       rec_width, rec_height);
	
	XmSetMarginGC(widget, data->gc);
	if (highlight == XmHIGHLIGHT_SECONDARY_SELECTED) {
	  
	  if (highlight == XmHIGHLIGHT_SELECTED)
	    XmSetInvGC(widget, data->gc);
	  else
	    XmSetNormGC(widget, data->gc, False, False);
	  
	  XDrawLine(XtDisplay(widget),
		    XtWindow(widget->text.inner_widget), data->gc, 
		    x - data->hoffset, y,
		    ((x - data->hoffset) + width) - 1, y);
	}
	x += width;
	
	
	block.length--;
	block.ptr++;
	if ((int)widget->text.char_size != 1) {
	  num_bytes = abs(mblen(block.ptr, (int)widget->text.char_size));
	  /* crummy error handling, but ... */
	}
	if (block.length <= 0) break;
      }
      if ((int)widget->text.char_size == 1){
	for ( length = 0; length < block.length; length++ ) {
	  if (block.ptr[length] == '\t') break;
	}
      } else {
	for (length = 0, 
	     num_bytes = mblen(block.ptr, (int)widget->text.char_size); 
	     length < block.length; 
	     num_bytes = mblen(&block.ptr[length], 
			       (int)widget->text.char_size)) {
	  if ((num_bytes == 1) && block.ptr[length] == '\t') break;
	  if (num_bytes == 0) break;
	  if (num_bytes < 0) num_bytes = 1;
	  length += num_bytes;
	}
      }
      if (length <= 0) break;
      newx = x;
      while (length > 0 && newx - data->hoffset < data->leftmargin) {
	newx += FindWidth(widget, newx, &block, 0, 1);
	if ((int)widget->text.char_size == 1) {
	  if (newx - data->hoffset < data->leftmargin) {
	    length--;
	    block.length--;
	    block.ptr++;
	    x = newx;
	  }
	} else {
	  if (newx - data->hoffset < data->leftmargin) {
	    num_bytes = abs(mblen(block.ptr, (int)widget->text.char_size));
	    length -= num_bytes;
	    block.length -= num_bytes;
	    block.ptr += num_bytes;
	    x = newx;
	  }
	}
      }
      if (length == 0) continue;
      newx = x + FindWidth(widget, x, &block, 0, length);
      if (newx > rightedge) {
	newx = x;
	if ((int)widget->text.char_size == 1){
	  for (i=0 ; i < length && newx <= rightedge ; i++) {
	    newx += FindWidth(widget, newx, &block, i, i+1);
	  }
	} else {
	  num_bytes = abs(mblen(block.ptr, (int)widget->text.char_size)); 
	  for (i=0;
	       i < length && newx <= rightedge && num_bytes > 0;
	       i += num_bytes) {
	    newx += FindWidth(widget, newx, &block, i, i + num_bytes);
	    num_bytes = abs(mblen(&block.ptr[i], (int)widget->text.char_size));
	  } /* end for */
	}
	length = i;
	start = end; /* Force a break out of the outer loop. */
	block.length = length; /* ... and out of the inner loop. */
      }   
      if (highlight == XmHIGHLIGHT_SELECTED) {
	/* Draw the inverse background, then draw the text over it */
	XmSetNormGC(widget, data->gc, False, False);
	XmSetFullGC(widget, data->gc);
	
	if (((x - data->hoffset) + (newx - x)) >
	    widget->text.inner_widget->core.width - data->rightmargin)
	  rec_width = widget->text.inner_widget->core.width -
	    (x - data->hoffset) - data->rightmargin;
	else
	  rec_width = newx - x;
	
	if (y + data->font_descent >
	    widget->text.inner_widget->core.height - data->bottommargin)
	  rec_height = (widget->text.inner_widget->core.height -
			data->bottommargin) - (y - data->font_ascent);
	else
	  rec_height = data->font_ascent + data->font_descent;
	
	XFillRectangle(XtDisplay(widget),
		       XtWindow(widget->text.inner_widget),
		       data->gc, x - data->hoffset,
		       y - data->font_ascent, rec_width, rec_height);
	
	XmSetInvGC(widget, data->gc);
	XmSetMarginGC(widget, data->gc);
	if (data->use_fontset) {
	  XmbDrawString(XtDisplay(widget),
			XtWindow(widget->text.inner_widget), 
			(XFontSet) data->font, data->gc, 
			x - data->hoffset, y, block.ptr, length);
	} else {
	  XDrawString(XtDisplay(widget),
		      XtWindow(widget->text.inner_widget), 
		      data->gc, x - data->hoffset, y, 
		      block.ptr, length);
	}
      } else {
	XmSetInvGC(widget, data->gc);
	if (newx > x){
	  if (y + data->font_descent >
	      widget->text.inner_widget->core.height - data->bottommargin)
	    rec_height = (widget->text.inner_widget->core.height -
			  data->bottommargin) - (y - data->font_ascent);
	  else
	    rec_height = data->font_ascent + data->font_descent;
	  
	  XFillRectangle(XtDisplay(widget),
			 XtWindow(widget->text.inner_widget),
			 data->gc, x - data->hoffset,
			 y - data->font_ascent, newx - x, 
			 rec_height);
	}
	XmSetNormGC(widget, data->gc, True, stipple);
	if (data->use_fontset) {
	  XmbDrawString(XtDisplay(widget),
			XtWindow(widget->text.inner_widget),
			(XFontSet) data->font, data->gc, 
			x - data->hoffset, y, block.ptr, length);
	} else {
	  XDrawString(XtDisplay(widget),
		      XtWindow(widget->text.inner_widget), data->gc,
		      x - data->hoffset, y, block.ptr, length);
	}
	if (stipple) XmSetNormGC(widget, data->gc, True, !stipple);
      }
      if (highlight == XmHIGHLIGHT_SECONDARY_SELECTED)
	XDrawLine(XtDisplay(widget), XtWindow(widget->text.inner_widget),
		  data->gc, x - data->hoffset, y, 
		  (newx - data->hoffset) - 1, y);
      x = newx;
      block.length -= length;
      block.ptr += length;
      if ((int)widget->text.char_size != 1) {
	num_bytes = mblen(block.ptr, (int)widget->text.char_size);
	if (num_bytes < 1) num_bytes = 1;
      }
    }    
  }
  
  /* clear left margin */
  text_border = widget->primitive.shadow_thickness +
    widget->primitive.highlight_thickness;
  if (data->leftmargin - text_border > 0 && y + data->font_descent > 0)
    XClearArea(XtDisplay(widget), XtWindow(widget->text.inner_widget),
	       text_border, text_border, data->leftmargin - text_border,
	       y + data->font_descent - text_border, FALSE);
  
  if (cleartoend) {
    x -= data->hoffset;
    if (x > ((int)widget->text.inner_widget->core.width)- data->rightmargin)
      x = ((int)widget->text.inner_widget->core.width)- data->rightmargin;
    if (x < data->leftmargin)
      x = data->leftmargin;
    width = ((int)widget->text.inner_widget->core.width) - x -
      data->rightmargin;
    if (width > 0 && data->lineheight > 0) {
      if (highlight == XmHIGHLIGHT_SELECTED)
	XmSetNormGC(widget, data->gc, False, False);
      else
	XmSetInvGC(widget, data->gc);
      XmSetFullGC(widget, data->gc);
      if (y + data->font_descent >
	  widget->text.inner_widget->core.height - data->bottommargin)
	rec_height = (widget->text.inner_widget->core.height -
		      data->bottommargin) - (y - data->font_ascent);
      else
	rec_height = data->font_ascent + data->font_descent;
      
      XFillRectangle(XtDisplay(widget), 
		     XtWindow(widget->text.inner_widget), data->gc, x, 
		     y - data->font_ascent, width, rec_height);
      XmSetMarginGC(widget, data->gc);
    }
  }
  if (cleartobottom) {
    x = data->leftmargin;
    width = widget->text.inner_widget->core.width -
      (data->rightmargin + data->leftmargin);
    height = widget->text.inner_widget->core.height -
      ((y + data->font_descent) + data->bottommargin);
    if (width > 0 && height > 0)
      XClearArea(XtDisplay(widget), XtWindow(widget->text.inner_widget),
		 x, y + data->font_descent, width, height, FALSE);
  }
  /* Before exiting, force PaintCursor to refresh its save area */
  data->refresh_ibeam_off = True;
}

static OnOrOff 
#ifdef _NO_PROTO
CurrentCursorState( widget )
        XmTextWidget widget ;
#else
CurrentCursorState(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    if (data->cursor_on < 0) return off;
    if (data->blinkstate == on || !XtSensitive(widget))
        return on;
    return off;
}


/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmTextDrawDestination( widget )
        XmTextWidget widget ;
#else
_XmTextDrawDestination( XmTextWidget widget )
#endif /* _NO_PROTO */
{
   /* DEPRECATED */
}

/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmTextClearDestination( widget, ignore_sens )
        XmTextWidget widget ;
        Boolean ignore_sens ;
#else
_XmTextClearDestination( XmTextWidget widget,
#if NeedWidePrototypes
        int ignore_sens )
#else
        Boolean ignore_sens )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   /* DEPRECATED */
}

/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmTextDestinationVisible( w, turn_on )
        Widget w ;
        Boolean turn_on ;
#else
_XmTextDestinationVisible( Widget w,
#if NeedWidePrototypes
        int turn_on )
#else
        Boolean turn_on )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   /* DEPRECATED */
}

/*
 * All the info about the cursor has been figured; draw or erase it.
 */
static void 
#ifdef _NO_PROTO
PaintCursor( widget )
        XmTextWidget widget ;
#else
PaintCursor(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    Position x, y;

    if (!data->cursor_position_visible) return;

    CheckHasRect(widget);

    if (!data->has_rect) _XmTextAdjustGC(widget);

    if (!widget->text.input->data->overstrike)
      x = data->insertx - (data->cursorwidth >> 1) - 1;
    else {
      int pxlen;
      XmTextBlockRec block;
      XmTextPosition cursor_pos = XmTextGetCursorPosition((Widget)widget);
      x = data->insertx;
      (void) (*widget->text.source->ReadSource) (widget->text.source,
						 cursor_pos, cursor_pos+1,
						 &block);
      pxlen = FindWidth(widget, x, &block, 0, block.length);
      if (pxlen > data->cursorwidth)
	x += (pxlen - data->cursorwidth) >> 1;
    }
    y = data->inserty + data->font_descent - data->cursorheight;

/* If time to paint the I Beam... first capture the IBeamOffArea, then draw
 * the I Beam. */
    if ((widget->text.top_character <= widget->text.cursor_position) &&
	(widget->text.cursor_position <= widget->text.bottom_position)){
       if (data->refresh_ibeam_off == True) { /* get area under IBeam first */
         /* Fill is needed to realign clip rectangle with gc */
	  XFillRectangle(XtDisplay((Widget)widget), XtWindow((Widget)widget),
			 data->save_gc, 0, 0, 0, 0);
          XCopyArea(XtDisplay((Widget)widget), XtWindow((Widget)widget),
		    data->ibeam_off, data->save_gc, x, y, data->cursorwidth,
		    data->cursorheight, 0, 0);
          data->refresh_ibeam_off = False;
       }

       if ((data->cursor_on >= 0) && (data->blinkstate == on)) {
          XFillRectangle(XtDisplay((Widget)widget), XtWindow((Widget)widget),
				   data->imagegc, x, y, data->cursorwidth, 
				   data->cursorheight);
       } else {
          XCopyArea(XtDisplay((Widget)widget), data->ibeam_off, 
		    XtWindow((Widget)widget), data->save_gc, 0, 0,
		    data->cursorwidth, data->cursorheight, x, y);
       }
   }
}



static void 
#ifdef _NO_PROTO
ChangeHOffset( widget, newhoffset, redisplay_hbar )
        XmTextWidget widget ;
        int newhoffset ;
        Boolean redisplay_hbar ;
#else
ChangeHOffset(
        XmTextWidget widget,
        int newhoffset,
#if NeedWidePrototypes
        int redisplay_hbar )
#else
        Boolean redisplay_hbar )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    int delta;
    int width = widget->text.inner_widget->core.width;
    int height = widget->text.inner_widget->core.height;
    int innerwidth = width - (data->leftmargin + data->rightmargin);
    int innerheight = height - (data->topmargin + data->bottommargin);
    if (ShouldWordWrap(data, widget) || data->suspend_hoffset) return;
    if ((data->scrollhorizontal &&
            XtClass(widget->core.parent) == xmScrolledWindowWidgetClass) &&
	data->scrollwidth - innerwidth < newhoffset)
        newhoffset = data->scrollwidth - innerwidth;
    if (newhoffset < 0) newhoffset = 0;
    if (newhoffset == data->hoffset) return;
    delta = newhoffset - data->hoffset;
    data->hoffset = newhoffset;
    posToXYCachedWidget = NULL;
    if (XtIsRealized(widget)) {
       CheckHasRect(widget);
       if (!data->has_rect) _XmTextAdjustGC(widget);
       XmSetNormGC(widget, data->gc, False, False);
       if (delta < 0) {
	   if (width > 0 && innerheight > 0) {
	       XCopyArea(XtDisplay(widget), XtWindow(widget->text.inner_widget),
			 XtWindow(widget->text.inner_widget), data->gc,
			 data->leftmargin, data->topmargin, width, innerheight,
			 data->leftmargin - delta, data->topmargin);
	       /* clear left margin + delta change */
	       if ((data->leftmargin - (widget->primitive.shadow_thickness +
		   widget->primitive.highlight_thickness) - delta) < innerwidth)
		  XClearArea(XtDisplay(widget), XtWindow(widget),
			     widget->primitive.shadow_thickness +
			     widget->primitive.highlight_thickness,
			     data->topmargin, data->leftmargin -
			     (widget->primitive.shadow_thickness +
			     widget->primitive.highlight_thickness) - delta,
			     innerheight, FALSE);
	       /* clear right margin */
	       XClearArea(XtDisplay(widget), XtWindow(widget),
			  data->leftmargin + innerwidth, data->topmargin,
			  data->rightmargin -
			  (widget->primitive.shadow_thickness +
			  widget->primitive.highlight_thickness),
			  innerheight,
			  FALSE);
	       data->exposehscroll++;
	   }
	   _XmTextResetClipOrigin(widget, widget->text.cursor_position, False);
	   RedrawRegion(widget, data->leftmargin, 0, -delta, height);
       } else {
	   if (innerwidth - delta > 0 && innerheight > 0) {
	       XCopyArea(XtDisplay(widget), XtWindow(widget->text.inner_widget),
			 XtWindow(widget->text.inner_widget), data->gc,
			 data->leftmargin + delta, data->topmargin,
			 innerwidth - delta, innerheight,
			 data->leftmargin, data->topmargin);
	       /* clear right margin + delta change */
	       XClearArea(XtDisplay(widget), XtWindow(widget),
			 data->leftmargin + innerwidth - delta, data->topmargin,
			 delta + data->rightmargin -
			 (widget->primitive.shadow_thickness +
			 widget->primitive.highlight_thickness),
			 innerheight, FALSE);
	      /* clear left margin */
               if (data->leftmargin - (int)(widget->primitive.shadow_thickness +
				widget->primitive.highlight_thickness) > 0)
		   XClearArea(XtDisplay(widget), XtWindow(widget),
			     widget->primitive.shadow_thickness +
			     widget->primitive.highlight_thickness,
			     data->topmargin, data->leftmargin -
			     (widget->primitive.shadow_thickness +
			     widget->primitive.highlight_thickness),
			     innerheight, FALSE);
	       data->exposehscroll++;
	   } else {
	    /* clear all text */
	     XClearArea(XtDisplay(widget), XtWindow(widget),
		       widget->primitive.shadow_thickness +
		       widget->primitive.highlight_thickness,
		       data->topmargin,
		       width - 2 *(widget->primitive.shadow_thickness +
		       widget->primitive.highlight_thickness),
		       innerheight,
		       FALSE);
	     data->exposehscroll++;
	   }
	   _XmTextResetClipOrigin(widget, widget->text.cursor_position, False);
	   RedrawRegion(widget, width - data->rightmargin - delta, 0,
			delta, height);
       }
    }

    if (redisplay_hbar) _XmRedisplayHBar(widget);
}

static void 
#ifdef _NO_PROTO
DrawInsertionPoint( widget, position, onoroff )
        XmTextWidget widget ;
        XmTextPosition position ;
        OnOrOff onoroff ;
#else
DrawInsertionPoint(
        XmTextWidget widget,
        XmTextPosition position,
        OnOrOff onoroff )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;

    if (onoroff == on) {
       data->cursor_on +=1;
       if (data->blinkrate == 0 || !data->hasfocus)
	  data->blinkstate = on;
    } else {
       if ((data->blinkstate == on) && data->cursor_on == 0)
	  if (data->blinkstate == CurrentCursorState(widget) &&
	      XtIsRealized((Widget)widget)){
	     data->blinkstate = off;  /* Toggle blinkstate to off */
	     PaintCursor(widget);
          }
	  data->cursor_on -= 1;
    }

    if (data->cursor_on < 0 || !XtIsRealized((Widget)widget))
	return;
    if (PosToXY(widget, position, &data->insertx, &data->inserty))
        PaintCursor(widget);
}

static void 
#ifdef _NO_PROTO
MakePositionVisible( widget, position )
        XmTextWidget widget ;
        XmTextPosition position ;
#else
MakePositionVisible(
        XmTextWidget widget,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    Position x, y;

    CheckHasRect(widget);

    if (!data->has_rect && XtIsRealized((Widget)widget))
       _XmTextAdjustGC(widget);
    if (!ShouldWordWrap(data, widget) && PosToXY(widget, position, &x, &y)) {
        x += data->hoffset;
        if (x - data->hoffset < data->leftmargin) {
            ChangeHOffset(widget, x - data->leftmargin, True);
        } else if (x - data->hoffset >
                   ((Position) (widget->text.inner_widget->core.width -
                                data->rightmargin))) {
            ChangeHOffset(widget, (int) (x) -
                                 (int) (widget->text.inner_widget->core.width) +
                                 (int) (data->rightmargin), True);
        }
    }
}

static void 
#ifdef _NO_PROTO
BlinkInsertionPoint( widget )
        XmTextWidget widget ;
#else
BlinkInsertionPoint(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;

    if ((data->cursor_on >=0) && 
	data->blinkstate == CurrentCursorState(widget) &&
	XtIsRealized((Widget)widget)) {

      /* Toggle blink state */
       if (data->blinkstate == on) data->blinkstate = off;
       else data->blinkstate = on;

       PaintCursor(widget);

   }
}

static Boolean 
#ifdef _NO_PROTO
MoveLines( widget, fromline, toline, destline )
        XmTextWidget widget ;
        LineNum fromline ;
        LineNum toline ;
        LineNum destline ;
#else
MoveLines(
        XmTextWidget widget,
        LineNum fromline,
        LineNum toline,
        LineNum destline )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    if (!XtIsRealized((Widget) widget)) return FALSE;
    CheckHasRect(widget);
    if (!data->has_rect) _XmTextAdjustGC(widget);
    XmSetNormGC(widget, data->gc, False, False);
    XmSetFullGC(widget, data->gc);
    XCopyArea(XtDisplay(widget), XtWindow(widget->text.inner_widget),
              XtWindow(widget->text.inner_widget), data->gc,
              widget->primitive.shadow_thickness +
	      widget->primitive.highlight_thickness,
              (Position) data->lineheight * fromline + data->topmargin,
              widget->text.inner_widget->core.width -
              2 * (widget->primitive.shadow_thickness +
	      widget->primitive.highlight_thickness),
              data->lineheight * (toline - fromline + 1),
              widget->primitive.shadow_thickness +
	      widget->primitive.highlight_thickness,
              (Position) data->lineheight * destline + data->topmargin);
    XmSetMarginGC(widget, data->gc);
    data->exposevscroll++;
    return TRUE;
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
OutputInvalidate( widget, position, topos, delta )
        XmTextWidget widget ;
        XmTextPosition position ;
        XmTextPosition topos ;
        long delta ;
#else
OutputInvalidate(
        XmTextWidget widget,
        XmTextPosition position,
        XmTextPosition topos,
        long delta )
#endif /* _NO_PROTO */
{
    posToXYCachedWidget = NULL;
}

static void 
#ifdef _NO_PROTO
RefigureDependentInfo( widget )
        XmTextWidget widget ;
#else
RefigureDependentInfo(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;

    data->rows = data->number_lines;
    data->columns = (short)((widget->core.width -
                    (data->leftmargin + data->rightmargin))
        	    / (data->averagecharwidth));

    if (data->columns <= 0)
      data->columns = 1;
}

static void 
#ifdef _NO_PROTO
SizeFromRowsCols( widget, width, height )
        XmTextWidget widget ;
        Dimension *width ;
        Dimension *height ;
#else
SizeFromRowsCols(
        XmTextWidget widget,
        Dimension *width,
        Dimension *height )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
/*
 * Fix for CR 5634 - If the widget is in SINGLE_LINE_EDIT mode, force the
 *                   number of rows to be 1 and only 1.  Otherwise, use the
 *                   rows_set variable.
 */
    short rows;

    if (widget->text.edit_mode == XmSINGLE_LINE_EDIT)
      rows = 1;
    else
      rows = data->rows_set;

    *width = (Dimension) ((data->columns_set * data->averagecharwidth) +
                           data->leftmargin + data->rightmargin);
    *height = (Dimension) ((rows * data->lineheight) +
                            data->topmargin + data->bottommargin);
/*
 * End Fix for CR 5634
 */
}

static void 
#ifdef _NO_PROTO
LoadFontMetrics( widget )
        XmTextWidget widget ;
#else
LoadFontMetrics(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    XmFontContext context;
    XmFontListEntry next_entry;
    XmFontType type_return = XmFONT_IS_FONT;
    XtPointer tmp_font;
    Boolean have_font_struct = False;
    Boolean have_font_set = False;
    XFontSetExtents *fs_extents;
    XFontStruct *font;
    unsigned long width = 0;
    char* font_tag = NULL;

    if (!XmFontListInitFontContext(&context, data->fontlist))
       _XmWarning( (Widget) widget, MSG3);

    do {
       next_entry = XmFontListNextEntry(context);
       if (next_entry) {
          tmp_font = XmFontListEntryGetFont(next_entry, &type_return);
          if (type_return == XmFONT_IS_FONTSET) {
             font_tag = XmFontListEntryGetTag(next_entry);
             if (!have_font_set){ /* this saves the first fontset found, just in
                                   * case we don't find a default tag set.
                                   */
                data->use_fontset = True;
                data->font = (XFontStruct *)tmp_font;
                have_font_struct = True; /* we have a font set, so no need to
                                          * consider future font structs */
                have_font_set = True;    /* we have a font set. */

                if (!strcmp(XmFONTLIST_DEFAULT_TAG, font_tag))
                   break; /* Break out!  We've found the one we want. */

             } else if (!strcmp(XmFONTLIST_DEFAULT_TAG, font_tag)){
                data->font = (XFontStruct *)tmp_font;
                have_font_set = True;    /* we have a font set. */
                break; /* Break out!  We've found the one we want. */
             }
          } else if (!have_font_struct){/* return_type must be XmFONT_IS_FONT */
             data->use_fontset = False;
            /* save the first one in case no font set is found */
             data->font = (XFontStruct*)tmp_font;
             data->use_fontset = False;
             have_font_struct = True;
          }
       }
    } while(next_entry != NULL);

    if (!have_font_struct && !have_font_set)
          _XmWarning ((Widget)widget, MSG4);

    XmFontListFreeFontContext(context);

    if(data->use_fontset){
       fs_extents = XExtentsOfFontSet((XFontSet)data->font);
#ifdef NON_OSF_FIX
       width = (unsigned long)fs_extents->max_logical_extent.width;
#else /* NON_OSF_FIX */
       width = (unsigned long)fs_extents->max_ink_extent.width;
#endif /* NON_OSF_FIX */
       /* max_ink_extent.y is number of pixels from origin to top of
        * rectangle (i.e. y is negative) */
#ifdef NON_OSF_FIX
       data->font_ascent = -fs_extents->max_logical_extent.y;
       data->font_descent = fs_extents->max_logical_extent.height +
                               fs_extents->max_logical_extent.y;
#else /* NON_OSF_FIX */
       data->font_ascent = -fs_extents->max_ink_extent.y;
       data->font_descent = fs_extents->max_ink_extent.height +
                               fs_extents->max_ink_extent.y;
#endif /* NON_OSF_FIX */
    } else {
       font = data->font;
       data->font_ascent = font->max_bounds.ascent;
       data->font_descent = font->max_bounds.descent;
       if ((!XGetFontProperty(font, XA_QUAD_WIDTH, &width)) || width == 0) {
           if (font->per_char && font->min_char_or_byte2 <= '0' &&
                                 font->max_char_or_byte2 >= '0')
               width = font->per_char['0' - font->min_char_or_byte2].width;
           else
              width = font->max_bounds.width;
       }
    }
    data->lineheight = data->font_descent + data->font_ascent;
    if (width <= 0) width = 1;
    data->averagecharwidth = (int) width; /* This assumes there will be no
				             truncation */
    data->tabwidth = (int)(8 * width); /* This assumes there will be no
					  truncation */
}

static void 
#ifdef _NO_PROTO
LoadGCs( widget, background, foreground )
        XmTextWidget widget ;
        Pixel background ;
        Pixel foreground ;
#else
LoadGCs(
        XmTextWidget widget,
        Pixel background,
        Pixel foreground )
#endif /* _NO_PROTO */
{
   OutputData data = widget->text.output->data;
   Display *display = XtDisplay((Widget)widget);
   Screen *screen = XtScreen((Widget)widget);
   XGCValues values;
   static XContext context = 0;
   Pixmap tw_cache_pixmap;
   unsigned long value_mask = (GCFunction | GCForeground | GCBackground | 
                              GCClipMask| GCArcMode | GCDashOffset);
   unsigned long dynamic_mask;

   if (context == 0) context = XUniqueContext();

   if (XFindContext(display, (Window)screen, context,
		    (char **) &tw_cache_pixmap)) {
      XmTextContextData ctx_data;
      Widget xm_display = (Widget) XmGetXmDisplay(display);

      ctx_data = (XmTextContextData) XtMalloc(sizeof(XmTextContextDataRec));

      ctx_data->screen = screen;
      ctx_data->context = context;
      ctx_data->type = _XM_IS_PIXMAP_CTX;

      /* Get the Pixmap identifier that the X Toolkit uses to cache our */
      /* GC's.  We never actually use this Pixmap; just so long as it's */
      /* a unique identifier. */

      tw_cache_pixmap = XCreatePixmap(display, RootWindowOfScreen(screen),
                                       1, 1, 1);

      XtAddCallback(xm_display, XmNdestroyCallback,
                    (XtCallbackProc) _XmTextFreeContextData,
		    (XtPointer) ctx_data);

      XSaveContext(display, (Window)screen, context, (char *) tw_cache_pixmap);
   }
   values.clip_mask = tw_cache_pixmap; /* use in caching Text widget gc's */
   values.arc_mode = ArcChord; /* Used in differentiating from TextField
                                     widget GC caching */

   CheckHasRect(widget);

   /* 
    *Get GC for saving area under cursor.
    */
   values.function = GXcopy;
   values.foreground = widget->primitive.foreground;
   values.background = widget->core.background_pixel;
   values.dash_offset = MOTIF_PRIVATE_GC;
   if (data->save_gc != NULL)
       XtReleaseGC((Widget) widget, data->save_gc);
   dynamic_mask = GCClipMask;
   data->save_gc = XtAllocateGC((Widget) widget, widget->core.depth, value_mask,
			      &values, dynamic_mask, 0);

   /*
    * Get GC for drawing text.
    */

   if (!data->use_fontset){
      value_mask |= GCFont | GCGraphicsExposures;
      values.font = data->font->fid;
   } else {
      value_mask |= GCGraphicsExposures;
   }

   values.graphics_exposures = (Bool) TRUE;
   values.foreground = foreground ^ background;
   values.background = 0;
   if (data->gc != NULL)
       XtReleaseGC((Widget) widget, data->gc);
   dynamic_mask |=  GCForeground | GCBackground | GCFillStyle | GCTile;
   data->gc = XtAllocateGC((Widget) widget, widget->core.depth, value_mask,
			   &values, dynamic_mask, 0);

   /* Create a temporary GC - change it later in MakeIBeamStencil */
   value_mask |= GCTile;
   values.tile = data->stipple_tile;
   if (data->imagegc != NULL)
       XtReleaseGC((Widget) widget, data->imagegc);
   dynamic_mask = (GCForeground | GCBackground | GCStipple | GCFillStyle |
		   GCTileStipXOrigin | GCTileStipYOrigin | GCFunction |
		   GCClipMask | GCClipXOrigin | GCClipYOrigin);
   data->imagegc = XtAllocateGC((Widget) widget, widget->core.depth,value_mask,
				&values, dynamic_mask, 0);
}

static void
#ifdef _NO_PROTO
MakeIBeamOffArea( tw, width, height )
        XmTextWidget tw ;
        Dimension width ;
        Dimension height ;
#else
MakeIBeamOffArea(
        XmTextWidget tw,
#if NeedWidePrototypes
        int width,
        int height)
#else
        Dimension width,
        Dimension height)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   OutputData data = tw->text.output->data;
   Display *dpy = XtDisplay(tw);
   Screen  *screen = XtScreen((Widget)tw);
   GC fillGC;

  /* Create a pixmap for storing the screen data where the I-Beam will
   * be painted */
   data->ibeam_off = XCreatePixmap(dpy, RootWindowOfScreen(screen), width,
                                      height, tw->core.depth);

  /* Create a GC for drawing 0's into the pixmap */
   fillGC = XCreateGC(dpy, data->ibeam_off, 0, (XGCValues *) NULL);

  /* Initialize the pixmap to 0's */
   XFillRectangle(dpy, data->ibeam_off, fillGC, 0, 0, width, height);

  /* Free the GC */
   XFreeGC(XtDisplay(tw), fillGC);
   data->refresh_ibeam_off = True;
}

static void
#ifdef _NO_PROTO
MakeIBeamStencil( tw, line_width )
        XmTextWidget tw ;
        int line_width ;
#else
MakeIBeamStencil(
        XmTextWidget tw,
        int line_width )
#endif /* _NO_PROTO */
{
   Screen *screen = XtScreen((Widget)tw);
   char pixmap_name[17];
   Pixmap clip_mask;
   XGCValues values;
   unsigned long valuemask;
   OutputData data = tw->text.output->data;

   sprintf(pixmap_name, "_XmText_%d_%d", data->cursorheight, line_width);
   data->cursor = XmGetPixmapByDepth(screen, pixmap_name, 1, 0, 1);

   if (data->cursor == XmUNSPECIFIED_PIXMAP) {
      Display *dpy = XtDisplay(tw);
      GC fillGC;
      XSegment segments[3];
      XRectangle ClipRect;

     /* Create a pixmap for the I-Beam stencil */
      data->cursor = XCreatePixmap(dpy, XtWindow(tw), data->cursorwidth,
			      data->cursorheight, 1);

     /* Create a GC for "cutting out" the I-Beam shape from the pixmap in
      * order to create the stencil.
      */
      fillGC = XCreateGC(dpy, data->cursor, 0, (XGCValues *)NULL);

     /* Fill in the stencil with a solid in preparation
      * to "cut out" the I-Beam.
      */
      XFillRectangle(dpy, data->cursor, fillGC, 0, 0, data->cursorwidth,
		     data->cursorheight);

     /* Change the GC for use in "cutting out" the I-Beam shape */
      values.foreground = 1;
      values.line_width = line_width;
      XChangeGC(dpy, fillGC, GCForeground | GCLineWidth, &values);

     /* Draw the segments of the I-Beam */
     /* 1st segment is the top horizontal line of the 'I' */
      segments[0].x1 = 0;
      segments[0].y1 = line_width - 1;
      segments[0].x2 = data->cursorwidth;
      segments[0].y2 = line_width - 1;

     /* 2nd segment is the bottom horizontal line of the 'I' */
      segments[1].x1 = 0;
      segments[1].y1 = data->cursorheight - 1;
      segments[1].x2 = data->cursorwidth;
      segments[1].y2 = data->cursorheight - 1;

     /* 3rd segment is the vertical line of the 'I' */
      segments[2].x1 = data->cursorwidth >> 1;
      segments[2].y1 = line_width;
      segments[2].x2 = data->cursorwidth >> 1;
      segments[2].y2 = data->cursorheight - 1;

     /* Set the clipping rectangle of the image GC from drawing */
      ClipRect.width = data->cursorwidth;
      ClipRect.height = data->cursorheight;
      ClipRect.x = 0;
      ClipRect.y = 0;

      XSetClipRectangles(XtDisplay(tw), fillGC, 0, 0, &ClipRect, 1, Unsorted);

     /* Draw the segments onto the cursor */
      XDrawSegments(dpy, data->cursor, fillGC, segments, 3);

     /* Install the cursor for pixmap caching */
      (void) _XmInstallPixmap(data->cursor, screen, pixmap_name, 1, 0);

     /* Free the fill GC */
      XFreeGC(XtDisplay(tw), fillGC);
   }

  /* Get/create the imagegc used to paint the I-Beam */

    sprintf(pixmap_name, "_XmText_CM_%d", data->cursorheight);
    clip_mask = XmGetPixmapByDepth(XtScreen(tw), pixmap_name, 1, 0, 1);
    if (clip_mask == XmUNSPECIFIED_PIXMAP)
       clip_mask = GetClipMask(tw, pixmap_name);

    valuemask = (GCForeground | GCBackground | GCClipMask | GCStipple |
	         GCFillStyle);
   if (tw->text.input->data->overstrike) {
     values.background = values.foreground = 
       tw->core.background_pixel ^ tw->primitive.foreground;
    } else {
      values.foreground = tw->primitive.foreground;
      values.background = tw->core.background_pixel;
    }
    values.clip_mask = clip_mask;
    values.stipple = data->cursor;
    values.fill_style = FillStippled;
    XChangeGC(XtDisplay(tw), data->imagegc, valuemask, &values);

}



 /* The IBeam Stencil must have already been created before this routine
  * is called.
  */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MakeAddModeCursor( tw, line_width )
        XmTextWidget tw ;
        int line_width ;
#else
MakeAddModeCursor(
        XmTextWidget tw,
        int line_width )
#endif /* _NO_PROTO */
{
   Screen *screen = XtScreen((Widget)tw);
   char pixmap_name[25];
   OutputData data = tw->text.output->data;

   sprintf(pixmap_name, "_XmText_AddMode_%d_%d",
	   data->cursorheight,line_width);

   data->add_mode_cursor = XmGetPixmapByDepth(screen, pixmap_name, 1, 0, 1);

   if (data->add_mode_cursor == XmUNSPECIFIED_PIXMAP) {
      GC fillGC;
      XtGCMask  valueMask;
      XGCValues values;
      Display *dpy = XtDisplay((Widget)tw);
      Pixmap stipple;
      XImage *image;

      _XmGetImage(screen, "50_foreground", &image);

      stipple = XCreatePixmap(dpy, XtWindow((Widget)tw), image->width, 
			      image->height, 1);

      data->add_mode_cursor =  XCreatePixmap(dpy, XtWindow((Widget)tw),
			       		     data->cursorwidth,
			       		     data->cursorheight, 1);

      fillGC = XCreateGC(dpy, data->add_mode_cursor, 0, (XGCValues *)NULL);

      XPutImage(dpy, stipple, fillGC, image, 0, 0, 0, 0, image->width,
		image->height);

      XCopyArea(dpy, data->cursor, data->add_mode_cursor, fillGC, 0, 0,
		data->cursorwidth, data->cursorheight, 0, 0);

      valueMask = (GCTile | GCFillStyle | GCForeground |
		   GCBackground | GCFunction);
      values.function = GXand;
      values.tile = stipple;
      values.fill_style = FillTiled;
      values.foreground = tw->primitive.foreground;
      values.background = tw->core.background_pixel;

      XChangeGC(XtDisplay((Widget)tw), fillGC, valueMask, &values);

      XFillRectangle(dpy, data->add_mode_cursor, fillGC,
                     0, 0, data->cursorwidth, data->cursorheight);

      /* Install the pixmap for pixmap caching */
       _XmInstallPixmap(data->add_mode_cursor, screen, pixmap_name, 1, 0);

      XFreePixmap(dpy, stipple);
      XFreeGC(dpy, fillGC);
   }
}

static void 
#ifdef _NO_PROTO
MakeCursors( widget )
        XmTextWidget widget ;
#else
MakeCursors(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
   OutputData data = widget->text.output->data;
   Screen *screen = XtScreen(widget);
   int line_width = 1;

   if (!XtIsRealized((Widget) widget)) return;

   data->cursorwidth = 5;
   data->cursorheight = data->font_ascent + data->font_descent;

  /* setup parameters to make a thicker I-Beam */
   if (data->cursorheight > 19) {
      data->cursorwidth++;
      line_width = 2;
   }

  /* Remove old ibeam_off pixmap */
   if (data->ibeam_off != XmUNSPECIFIED_PIXMAP)
      XFreePixmap(XtDisplay((Widget)widget), data->ibeam_off);

  /* Remove old insert stencil */
   if (data->cursor != XmUNSPECIFIED_PIXMAP)
       (void) XmDestroyPixmap(screen, data->cursor);

  /* Remove old add mode cursor */
   if (data->add_mode_cursor != XmUNSPECIFIED_PIXMAP)
       (void) XmDestroyPixmap(screen, data->add_mode_cursor);

/* Create area in which to save text located underneath I beam */
   MakeIBeamOffArea(widget, MAX(data->cursorwidth, data->cursorheight >> 1),
		    data->cursorheight);

  /* Create a new i-beam stencil */
   MakeIBeamStencil(widget, line_width);

  /* Create a new add_mode cursor */
   MakeAddModeCursor(widget, line_width);

   _XmTextResetClipOrigin(widget, XmTextGetCursorPosition((Widget)widget),
			  False);

   if (widget->text.input->data->overstrike)
     data->cursorwidth = data->cursorheight >> 1;
}

static void 
#ifdef _NO_PROTO
OutputGetValues( wid, args, num_args )
        Widget wid ;
        ArgList args ;
        Cardinal num_args ;
#else
OutputGetValues(
        Widget wid,
        ArgList args,
        Cardinal num_args )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) wid ;

    RefigureDependentInfo(widget);
    XtGetSubvalues((XtPointer) widget->text.output->data, output_resources,
		   XtNumber(output_resources), args, num_args);
}


static Boolean
#ifdef _NO_PROTO
CKCols(args, num_args)
	ArgList args;
	Cardinal num_args;
#else
CKCols(
	ArgList args,
	Cardinal num_args )
#endif /* _NO_PROTO */
{
    register ArgList arg;
    for (arg = args ; num_args != 0; num_args--, arg++) {
       if (strcmp(arg->name, XmNcolumns) == 0) return(TRUE);
    }
    return(FALSE);
}


static Boolean
#ifdef _NO_PROTO
CKRows(args, num_args)
	ArgList args;
	Cardinal num_args;
#else
CKRows(
	ArgList args,
	Cardinal num_args )
#endif /* _NO_PROTO */
{
    register ArgList arg;
    for (arg = args ; num_args != 0; num_args--, arg++) {
       if (strcmp(arg->name, XmNrows) == 0) return(TRUE);
    }
    return(FALSE);
}


/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
OutputSetValues( oldw, reqw, new_w, args, num_args )
        Widget oldw ;
        Widget reqw ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
OutputSetValues(
        Widget oldw,
        Widget reqw,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmTextWidget oldtw = (XmTextWidget) oldw ;
    XmTextWidget newtw = (XmTextWidget) new_w ;
    OutputData data = newtw->text.output->data;
    OutputDataRec newdatarec;
    OutputData newdata = &newdatarec;
    Boolean needgcs = False;
    Boolean newsize = False;
    Boolean o_redisplay = False;
    Dimension new_width = newtw->core.width;/* save in case something changes */
    Dimension new_height = newtw->core.height;/* these values during SetValues*/
    XPoint xmim_point;
    Arg sb_args[3];
    Arg im_args[6];
    int n = 0;
    *newdata = *data;
    XtSetSubvalues((XtPointer) newdata, output_resources,
		   XtNumber(output_resources), args, *num_args);

#define CK(fld) (newdata->fld != data->fld)
#define CP(fld) (data->fld = newdata->fld)

    if (newtw->core.background_pixel != oldtw->core.background_pixel) {
       XtSetArg(im_args[n], XmNbackground, newtw->core.background_pixel); n++;
       needgcs = True;
    }

    if (newtw->primitive.foreground != oldtw->primitive.foreground) {
       XtSetArg(im_args[n], XmNforeground, newtw->primitive.foreground); n++;
       needgcs = True;
    }

    if (CK(fontlist)) {
       XmFontListFree(data->fontlist);
       if (newdata->fontlist == NULL)
          newdata->fontlist = _XmGetDefaultFontList(new_w, XmTEXT_FONTLIST);
       newdata->fontlist = (XmFontList)XmFontListCopy(newdata->fontlist);
       CP(fontlist);
       LoadFontMetrics(newtw);

       if (data->hbar) {
          XtSetArg(sb_args[0], XmNincrement, data->averagecharwidth);
          XtSetValues(data->hbar, sb_args, 1);
       }
       o_redisplay = True;
       XtSetArg(im_args[n], XmNfontList, data->fontlist); n++;
       needgcs = True;
       newsize = True;
    }

  /* Don't word wrap, have multiple row or have vertical scrollbars
     if editMode is single_line */
    if (newtw->text.edit_mode != oldtw->text.edit_mode)
      if (newtw->text.edit_mode == XmSINGLE_LINE_EDIT) {
          newdata->rows = 1;
          o_redisplay = True;
          if (data->vbar) XtUnmanageChild(data->vbar);
      } else {
          if (data->vbar) XtManageChild(data->vbar);
      }

/*  what is called margin, in this code, is composed of margin, shadow, and
    highlight.   Previously, only margin was accomodated.   This addition
    may not be very clever, but it blends in with the rest of the way this
    code works.
*/

    if (newtw->text.margin_width != oldtw->text.margin_width ||
	newtw->text.margin_height != oldtw->text.margin_height ||
	newtw->primitive.shadow_thickness !=
				 oldtw->primitive.shadow_thickness ||
	newtw->primitive.highlight_thickness !=
				 oldtw->primitive.highlight_thickness)
    {
       data->leftmargin = data->rightmargin = newtw->text.margin_width +
					  newtw->primitive.shadow_thickness +
					  newtw->primitive.highlight_thickness;
       data->topmargin = data->bottommargin = newtw->text.margin_height +
					  newtw->primitive.shadow_thickness +
					  newtw->primitive.highlight_thickness;
       o_redisplay = True;
       newsize = True;
    }

    if (CK(wordwrap)) {
     /* If we are turning on wrapping, we don't want any horiz. offset */
       if (!data->wordwrap) ChangeHOffset(newtw, 0, True);

       if (data->hbar) {
          if (newdata->wordwrap) {
             XtSetArg(sb_args[0], XmNvalue, 0);
             XtSetArg(sb_args[1], XmNsliderSize, 1);
             XtSetArg(sb_args[2], XmNmaximum, 1);
             XtSetValues(data->hbar, sb_args, 3);
             data->hoffset = 0;
          } else {
             _XmRedisplayHBar(newtw);
          }
       }

       _XmTextRealignLineTable(newtw, NULL, 0, 0, 0, PASTENDPOS);

     /* If we've just turned off wrapping, get new top_character by scanning */
     /* left from the current top character until we find a new line. */
       if (!data->wordwrap) {
	  if (data->resizeheight)
	     newtw->text.top_character = newtw->text.new_top = 0;
          else {
	     newtw->text.top_character = (*newtw->text.source->Scan)
		 (newtw->text.source, newtw->text.top_character,
		 XmSELECT_LINE, XmsdLeft, 1, FALSE);
	     newtw->text.new_top = newtw->text.top_character;
	  }
       }

       if (newtw->text.top_character)
	  newtw->text.top_line = CountLines(newtw, 0,
                                             newtw->text.top_character);


       o_redisplay = True;
    }

    CP(wordwrap);

    if (data->hasfocus && XtSensitive(newtw) && CK(blinkrate)) {
        if (newdata->blinkrate == 0) {
            data->blinkstate = on;
            if (data->timerid) {
                XtRemoveTimeOut(data->timerid);
                data->timerid = (XtIntervalId)0;
            }
        } else if (data->timerid == (XtIntervalId)0) {
            data->timerid =
                XtAppAddTimeOut(XtWidgetToApplicationContext(new_w),
                                (unsigned long) newdata->blinkrate,
                                HandleTimer, (XtPointer) newtw);
        }
    }
    CP(blinkrate);

    CP(resizewidth);
    CP(resizeheight);

    CP(cursor_position_visible);

    if (needgcs) {
        EraseInsertionPoint(newtw);
        LoadGCs(newtw, newtw->core.background_pixel,
	        newtw->primitive.foreground);
        if (XtIsRealized(new_w)) {
           MakeCursors(newtw);
           _XmTextAdjustGC(newtw);
        }
        TextDrawInsertionPoint(newtw);
        o_redisplay = True;
    }

    if (newdata->rows <= 0) {
        _XmWarning(new_w, MSG1);
        newdata->rows = data->rows;
    }

    if (newdata->columns <= 0) {
        _XmWarning(new_w, MSG2);
        newdata->columns = data->columns;
    }

   /* Process arglist to verify the a value is being set */
    if (CKCols(args, *num_args))
       data->columns_set = newdata->columns_set = newdata->columns;

   /* Process arglist to verify the a value is being set */
    if (CKRows(args, *num_args))
       data->rows_set = newdata->rows_set = newdata->rows;

    if (!(new_width != oldtw->core.width &&
	  new_height != oldtw->core.height)) {
       if (CK(columns) || CK(rows) || newsize) {
           Dimension width, height;
           CP(columns);
           CP(rows);
           SizeFromRowsCols(newtw, &width, &height);
           if (new_width == oldtw->core.width)
              newtw->core.width = width;
           if (new_height == oldtw->core.height)
              newtw->core.height = height;
           o_redisplay = True;
       }
    } else {
      if (new_width != newtw->core.width) 
	 newtw->core.width = new_width;
      if (new_height != newtw->core.height) 
	 newtw->core.height = new_height;
    }

    PosToXY(newtw, newtw->text.cursor_position, &xmim_point.x, &xmim_point.y);
    XtSetArg(im_args[n], XmNbackgroundPixmap,
	     newtw->core.background_pixmap);n++;
    XtSetArg(im_args[n], XmNspotLocation, &xmim_point); n++;
    XtSetArg(im_args[n], XmNlineSpace, data->lineheight); n++;
    XmImSetValues(new_w, im_args, n);

    return (o_redisplay);
}

static void 
#ifdef _NO_PROTO
NotifyResized( w, o_create )
        Widget w ;
        Boolean o_create ;
#else
NotifyResized(
        Widget w,
#if NeedWidePrototypes
        int o_create )
#else
        Boolean o_create )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) w;
    OutputData data = widget->text.output->data;
    Boolean resizewidth = data->resizewidth;
    Boolean resizeheight = data->resizeheight;
    XmTextPosition linestart = 0;
    XmTextPosition position;
    XPoint xmim_point;
    int text_width = 0;
    int new_width;
    static XmTextBlockRec block;
    Arg args[1];

    data->resizewidth = data->resizeheight = FALSE;
    data->number_lines = widget->text.inner_widget->core.height -
                     data->topmargin - data->bottommargin;
    if (data->number_lines < (int) data->lineheight || !data->lineheight)
       data->number_lines = 1;
    else
       data->number_lines /= data->lineheight;

    if (widget->text.top_character)
       widget->text.top_line = CountLines(widget, 0,
                                          widget->text.top_character);

    if (data->vbar)
    {
        if (data->number_lines > 1)
           XtSetArg(args[0], XmNpageIncrement, data->number_lines - 1);
        else
           XtSetArg(args[0], XmNpageIncrement, 1);
        XtSetValues(data->vbar, args, 1);
    }

    if (data->hbar)
    {
        XtSetArg(args[0], XmNpageIncrement,
			 widget->text.inner_widget->core.width);
        XtSetValues(data->hbar, args, 1);
    }

    RefigureDependentInfo(widget);
    if (resizewidth)
      data->columns_set = data->columns;
    if (resizeheight)
      data->rows_set = data->rows;

    if (XtIsRealized(w)) {
        XClearWindow(XtDisplay(widget), XtWindow(widget->text.inner_widget));
	data->refresh_ibeam_off = True;
       _XmTextAdjustGC(widget);
    }

    if (!o_create)              /* FALSE only if called from OutputCreate */
        _XmTextInvalidate(widget, (XmTextPosition) 0, (XmTextPosition) 0,
                          NODELTA);

    /* the new size grew enough to include new text */
    new_width = widget->core.width - (data->leftmargin + data->rightmargin);

    if (widget->text.edit_mode == XmSINGLE_LINE_EDIT) {
       position = (*widget->text.source->Scan)(widget->text.source, linestart,
					       XmSELECT_LINE, XmsdRight,
					       1, FALSE);
       while (linestart < position) {
          linestart = (*widget->text.source->ReadSource) (widget->text.source,
			      		          linestart, position, &block);
          text_width += FindWidth(widget, 0, &block, 0, block.length);
       }
       if (widget->text.auto_show_cursor_position) {
          if (text_width - new_width < data->hoffset)
             if (text_width - new_width >= 0)
                ChangeHOffset(widget, text_width - new_width, True);
             else
	        ChangeHOffset(widget, 0, True);
          else if (text_width - new_width > data->hoffset)
             ChangeHOffset(widget, text_width - new_width, True);
       }
    } else _XmRedisplayHBar(widget);

    data->resizewidth = resizewidth;
    data->resizeheight = resizeheight;

    if (XtIsRealized(w))
       _XmTextDrawShadow(widget);

    /* Text is now rediplayed at the correct location, so force the widget to
     * refresh the putback area.
     */

    data->refresh_ibeam_off = True;

    /* Somehow we need to let the input method know that the window has
     * changed size (for case of over-the-spot).  Try telling it that
     * the cursor position has changed and hopefully it will re-evaluate
     * the position/visibility/... of the pre-edit window.
     */

    PosToXY(widget, widget->text.cursor_position, &xmim_point.x, &xmim_point.y);
    XtSetArg(args[0], XmNspotLocation, &xmim_point);
    XmImSetValues(w, args, 1);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleTimer( closure, id )
        XtPointer closure ;
        XtIntervalId *id ;
#else
HandleTimer(
        XtPointer closure,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) closure;
    OutputData data = widget->text.output->data;
    if (data->blinkrate != 0)
        data->timerid = XtAppAddTimeOut(
                                 XtWidgetToApplicationContext((Widget) widget),
                                        (unsigned long)data->blinkrate,
                                        HandleTimer, (XtPointer) closure);
    if (data->hasfocus && XtSensitive(widget)) BlinkInsertionPoint(widget);
}

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
void 
#ifdef _NO_PROTO
_XmTextChangeBlinkBehavior( widget, newvalue )
        XmTextWidget widget ;
        Boolean newvalue ;
#else
_XmTextChangeBlinkBehavior(
        XmTextWidget widget,
#if NeedWidePrototypes
        int newvalue )
#else
        Boolean newvalue )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;

    if (newvalue) {
        if (data->blinkrate != 0 && data->timerid == (XtIntervalId)0)
            data->timerid =
                XtAppAddTimeOut(XtWidgetToApplicationContext((Widget) widget),
                                        (unsigned long)data->blinkrate,
                                        HandleTimer, (XtPointer) widget);
        data->blinkstate = on;
    } else {
        if (data->timerid)
            XtRemoveTimeOut(data->timerid);
        data->timerid = (XtIntervalId)0;
    }
}

/* ARGSUSED */

static void
#ifdef _NO_PROTO
HandleFocusEvents( w, closure, event, cont )
        Widget w ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
HandleFocusEvents(
        Widget w,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) w;
    OutputData data = widget->text.output->data;
    Boolean newhasfocus = data->hasfocus;
    XmAnyCallbackStruct cb;
    XPoint xmim_point;
    Arg  args[6];
    int          n = 0;

    PosToXY(widget, widget->text.cursor_position, &xmim_point.x, &xmim_point.y);

    switch (event->type) {
      case FocusIn:
        if (event->xfocus.send_event && !(newhasfocus)) {
            cb.reason = XmCR_FOCUS;
            cb.event = event;
            XtCallCallbackList (w, widget->text.focus_callback, (XtPointer) &cb);
            newhasfocus = TRUE;

            n = 0;
            XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
            XmImSetFocusValues(w, args, n);
        }
        break;
      case FocusOut:
        if (event->xfocus.send_event && newhasfocus) {
            newhasfocus = FALSE;
            XmImUnsetFocus(w);
        }
        break;
      case EnterNotify:
        if ((_XmGetFocusPolicy(w) != XmEXPLICIT) && !(newhasfocus) &&
	    event->xcrossing.focus &&
            (event->xcrossing.detail != NotifyInferior)) {
            cb.reason = XmCR_FOCUS;
            cb.event = event;
            XtCallCallbackList (w, widget->text.focus_callback, (XtPointer) &cb);
            newhasfocus = TRUE;
	    n = 0;
            XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
            XmImSetFocusValues(w, args, n);
        }
        break;
      case LeaveNotify:
        if ((_XmGetFocusPolicy(w) != XmEXPLICIT) && newhasfocus &&
            event->xcrossing.focus &&
            (event->xcrossing.detail != NotifyInferior)) {
            newhasfocus = FALSE;
            XmImUnsetFocus(w);
        }
        break;
    }
    if (newhasfocus != data->hasfocus) {
       data->hasfocus = newhasfocus;
       CheckHasRect(widget);
       if (!data->has_rect) _XmTextAdjustGC(widget);
       if (newhasfocus && XtSensitive(widget)) {
          _XmTextToggleCursorGC(w);
	  EraseInsertionPoint(widget);
          data->blinkstate = off;
          _XmTextChangeBlinkBehavior(widget, True);
	  TextDrawInsertionPoint(widget);
       } else {
	  _XmTextChangeBlinkBehavior(widget, False);
	  EraseInsertionPoint(widget);
	  _XmTextToggleCursorGC(w);
	  data->blinkstate = on;
	  TextDrawInsertionPoint(widget);
       }
    }
}



/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleGraphicsExposure( w, closure, event, cont )
        Widget w ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
HandleGraphicsExposure(
        Widget w,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) w;
    OutputData data = widget->text.output->data;
    if (event->xany.type == GraphicsExpose) {
        XGraphicsExposeEvent *xe = (XGraphicsExposeEvent *) event;
        if (data->exposehscroll != 0) {
            xe->x = 0;
            xe->width = widget->core.width;
        }
        if (data->exposevscroll != 0) {
            xe->y = 0;
            xe->height = widget->core.height;
        }
        RedrawRegion(widget, xe->x, xe->y, xe->width, xe->height);
        if (xe->count == 0) {
            if (data->exposehscroll) data->exposehscroll--;
            if (data->exposevscroll) data->exposevscroll--;
        }
    }
    if (event->xany.type == NoExpose) {
        if (data->exposehscroll) data->exposehscroll--;
        if (data->exposevscroll) data->exposevscroll--;
    }
}


static void 
#ifdef _NO_PROTO
OutputRealize( w, valueMask, attributes )
        Widget w ;
        XtValueMask *valueMask ;
        XSetWindowAttributes *attributes ;
#else
OutputRealize(
        Widget w,
        XtValueMask *valueMask,
        XSetWindowAttributes *attributes )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) w ;

    XtCreateWindow(w, (unsigned int) InputOutput, (Visual *) CopyFromParent,
		   *valueMask, attributes);
    MakeCursors(widget);
    _XmTextAdjustGC(widget);
}


static void 
#ifdef _NO_PROTO
OutputDestroy( w )
        Widget w ;
#else
OutputDestroy(
        Widget w )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) w ;
    char pixmap_name[17];
    Pixmap clip_mask;
    OutputData data = widget->text.output->data;
    Cardinal depth;
    TextGCData gc_data = GetTextGCData(w);

    if (gc_data->tw == widget)
      gc_data->tw = NULL;

    if (data->timerid)
        XtRemoveTimeOut(data->timerid);

    XtRemoveEventHandler((Widget) widget->text.inner_widget,
                   (EventMask)FocusChangeMask|EnterWindowMask|LeaveWindowMask,
                   FALSE, HandleFocusEvents, NULL);

    XtRemoveEventHandler((Widget) widget->text.inner_widget,
                      (EventMask) 0, TRUE, HandleGraphicsExposure,
                      NULL);

    XmDestroyPixmap(XtScreen(widget), data->stipple_tile);

    depth = widget->core.depth;
    widget->core.depth = 1;
    XtReleaseGC(w, data->imagegc);
    widget->core.depth = depth;

    XtReleaseGC(w, data->gc);
    XtReleaseGC(w, data->save_gc);

    XmFontListFree(data->fontlist);

    if (data->add_mode_cursor != XmUNSPECIFIED_PIXMAP)
       (void) XmDestroyPixmap(XtScreen(widget), data->add_mode_cursor);

    if (data->cursor != XmUNSPECIFIED_PIXMAP)
       (void) XmDestroyPixmap(XtScreen(widget), data->cursor);

    if (data->ibeam_off != XmUNSPECIFIED_PIXMAP)
       XFreePixmap(XtDisplay((Widget)widget), data->ibeam_off);

    sprintf(pixmap_name, "_XmText_CM_%d", data->cursorheight);
    clip_mask = XmGetPixmapByDepth(XtScreen(widget), pixmap_name, 1, 0, 1);
    if (clip_mask != XmUNSPECIFIED_PIXMAP)
       XmDestroyPixmap(XtScreen(widget), clip_mask);

    XtFree((char *)data);
    XtFree((char *)widget->text.output);
    posToXYCachedWidget = NULL;
}

static void 
#ifdef _NO_PROTO
RedrawRegion( widget, x, y, width, height )
        XmTextWidget widget ;
        int x ;
        int y ;
        int width ;
        int height ;
#else
RedrawRegion(
        XmTextWidget widget,
        int x,
        int y,
        int width,
        int height )
#endif /* _NO_PROTO */
{
    OutputData data = widget->text.output->data;
    int i;
    XmTextPosition first, last;
    for (i = y ; i < y + height + data->lineheight ; i += data->lineheight) {
       first = XYToPos(widget, x, i);
       last = XYToPos(widget, x + width, i);
       first = (*widget->text.source->Scan)(widget->text.source, first,
					    XmSELECT_POSITION,
                             		    XmsdLeft, 1, TRUE);
       last = (*widget->text.source->Scan)(widget->text.source, last,
					   XmSELECT_POSITION,
                              		   XmsdRight, 1, TRUE);
       _XmTextMarkRedraw(widget, first, last);
    }
} 

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
OutputExpose( w, event, region )
        Widget w ;
        XEvent *event ;
        Region region ;
#else
OutputExpose(
        Widget w,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) w ;
    XExposeEvent *xe = (XExposeEvent *) event;
    OutputData data = widget->text.output->data;
    Boolean erased_cursor = False;
    int old_number_lines = data->number_lines;
    Arg args[1];
    Arg im_args[1];
    XPoint xmim_point;
    int n = 0;
    Boolean font_may_have_changed = False;

    if (widget->text.in_setvalues) {
    /* Get here via SetValues.  Force x,y of IM and clip origin for
     * I-beam in case font changed.
     */
       widget->text.in_setvalues = False;
       font_may_have_changed = True;
    }

    if (event->xany.type != Expose)
        return;
    
    CheckHasRect(widget);

    if (!data->has_rect) _XmTextAdjustGC(widget);

    if (XtSensitive(widget) && data->hasfocus)
       _XmTextChangeBlinkBehavior(widget, False);
    EraseInsertionPoint(widget);

    data->number_lines = widget->text.inner_widget->core.height -
                     data->topmargin - data->bottommargin;
    if (data->number_lines < (int) data->lineheight || !data->lineheight)
       data->number_lines = 1;
    else
       data->number_lines /= data->lineheight;

    if (data->vbar && old_number_lines != data->number_lines)
    {
        if (data->number_lines > 1)
           XtSetArg(args[0], XmNpageIncrement, data->number_lines - 1);
        else
           XtSetArg(args[0], XmNpageIncrement, 1);
        XtSetValues(data->vbar, args, 1);
    }

    if (!data->handlingexposures) {
        _XmTextDisableRedisplay(widget, FALSE);
        data->handlingexposures = TRUE;
    }
    if (data->exposehscroll != 0) {
        xe->x = 0;
        xe->width = widget->core.width;
    }
    if (data->exposevscroll != 0) {
        xe->y = 0;
        xe->height = widget->core.height;
    }
    if (xe->x == 0 && xe->y == 0 && xe->width == widget->core.width &&
          xe->height == widget->core.height)
        _XmTextMarkRedraw(widget, (XmTextPosition)0, 9999999);
    else {
        if (!erased_cursor)
           RedrawRegion(widget, xe->x, xe->y, xe->width, xe->height);
    }

    _XmTextInvalidate(widget, (XmTextPosition) widget->text.top_character,
		      (XmTextPosition) widget->text.top_character, NODELTA);

    _XmTextEnableRedisplay(widget);

    data->handlingexposures = FALSE;

    _XmTextDrawShadow(widget);

    /* If the expose happened because of SetValues, the font may have changed.
     * At this point, RefigureLines has run and the widget is relayed out.
     * So it is safe to calculate the x,y position of the cursor to pass
     * to the IM.  And we can reset the clip origin so that the I-Beam will
     * be drawn correctly.
     */
     if (font_may_have_changed) {
	EraseInsertionPoint(widget);
	_XmTextResetClipOrigin(widget, widget->text.cursor_position, False);
	TextDrawInsertionPoint(widget);
	PosToXY(widget, widget->text.cursor_position, &xmim_point.x,
		&xmim_point.y);
	XtSetArg(im_args[n], XmNspotLocation, &xmim_point); n++;
	XmImSetValues(w, im_args, n);
    }

    if ((data->cursor_on < 0) || (data->blinkstate == off))
       data->refresh_ibeam_off = True;

    if (XtSensitive(widget) && data->hasfocus)
       _XmTextChangeBlinkBehavior(widget, True);
    TextDrawInsertionPoint(widget);
}

static void 
#ifdef _NO_PROTO
GetPreferredSize( widget, width, height )
        Widget widget ;
        Dimension *width ;
        Dimension *height ;
#else
GetPreferredSize(
        Widget widget,
        Dimension *width,
        Dimension *height )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) widget;
    OutputData data = tw->text.output->data;

    SizeFromRowsCols((XmTextWidget) widget, width, height);

    if (data->resizewidth) {
       TextFindNewWidth(tw, width);
       if (*width < data->minwidth) *width = data->minwidth;
    }

    if (data->resizeheight)
       TextFindNewHeight(tw, PASTENDPOS, height);

    if (*width == 0) *width = 1;
    if (*height == 0) *height = 1;
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleVBarButtonRelease( w, closure, event, cont )
        Widget w ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
HandleVBarButtonRelease(
        Widget w,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) closure;
    OutputData data = tw->text.output->data;

    if (event->xbutton.button != Button1) return;

    data->suspend_hoffset = False;

    EraseInsertionPoint(tw);
    XmTextScroll((Widget) tw, 0);
    _XmTextResetClipOrigin(tw, XmTextGetCursorPosition((Widget)tw), False);
    TextDrawInsertionPoint(tw);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleVBar( w, param, cback )
        Widget w ;
        XtPointer param ;
        XtPointer cback ;
#else
HandleVBar(
        Widget w,
        XtPointer param,
        XtPointer cback )
#endif /* _NO_PROTO */
{
    XmScrollBarCallbackStruct *info = (XmScrollBarCallbackStruct *) cback;
    Widget parent = XtParent(w);
    Widget widget;
    XmTextWidget tw;
    XPoint xmim_point;
    Arg args[1];
    OutputData data;
    int lines;

    XtSetArg(args[0], XmNworkWindow, &widget);
    XtGetValues(parent, args, 1);
    tw = (XmTextWidget) widget;
    data = tw->text.output->data;
    if (data->ignorevbar) return;
    data->suspend_hoffset = True;
    tw->text.vsbar_scrolling = True;
    lines = info->value - tw->text.top_line;
    tw->text.top_line = info->value;
    EraseInsertionPoint(tw);
    XmTextScroll(widget, lines);
    _XmTextResetClipOrigin(tw, XmTextGetCursorPosition(w), False);
    TextDrawInsertionPoint(tw);
    tw->text.vsbar_scrolling = False;

    PosToXY(tw,  tw->text.cursor_position, &xmim_point.x, &xmim_point.y);
    XtSetArg(args[0], XmNspotLocation, &xmim_point);
    XmImSetValues(w, args, 1);

    data->suspend_hoffset = False;
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleHBar( w, param, cback )
        Widget w ;
        XtPointer param ;
        XtPointer cback ;
#else
HandleHBar(
        Widget w,
        XtPointer param,
        XtPointer cback )
#endif /* _NO_PROTO */
{
    XmScrollBarCallbackStruct *info = (XmScrollBarCallbackStruct *) cback;
    Widget parent = XtParent(w);
    Widget widget;
    XmTextWidget tw;
    Arg args[1];
    OutputData data;
    int newhoffset;
    XPoint xmim_point;

    XtSetArg(args[0], XmNworkWindow, &widget);
    XtGetValues(parent, args, 1);
    tw = (XmTextWidget) widget;
    data = tw->text.output->data;
    newhoffset = data->hoffset;
    if (data->ignorehbar) return;
    newhoffset = info->value;
    EraseInsertionPoint(tw);
    ChangeHOffset(tw, newhoffset, False);
    TextDrawInsertionPoint(tw);

    PosToXY(tw, tw->text.cursor_position, &xmim_point.x, &xmim_point.y);
    XtSetArg(args[0], XmNspotLocation, &xmim_point);
    XmImSetValues(w, args, 1);
}


/* Public routines. */

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
void 
#ifdef _NO_PROTO
_XmTextOutputCreate( wid, args, num_args )
        Widget wid ;
        ArgList args ;
        Cardinal num_args ;
#else
_XmTextOutputCreate(
        Widget wid,
        ArgList args,
        Cardinal num_args )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) wid ;
    Output output;
    OutputData data;
    Dimension width, height;

    widget->text.output = output = (Output)
        XtMalloc((unsigned) sizeof(OutputRec));
    output->data = data = (OutputData)
        XtMalloc((unsigned) sizeof(OutputDataRec));

    XtGetSubresources((Widget) widget->core.parent, (XtPointer)data,
                      widget->core.name, "XmText", output_resources,
                      XtNumber(output_resources), args, num_args);

    output->XYToPos = XYToPos;
    output->PosToXY = PosToXY;
    output->MeasureLine = MeasureLine;
    output->Draw = Draw;
    output->DrawInsertionPoint = DrawInsertionPoint;
    output->MakePositionVisible = MakePositionVisible;
    output->MoveLines = MoveLines;
    output->Invalidate = OutputInvalidate;
    output->GetPreferredSize = GetPreferredSize;
    output->GetValues = OutputGetValues;
    output->SetValues = OutputSetValues;
    output->realize = OutputRealize;
    output->destroy = OutputDestroy;
    output->resize = NotifyResized;
    output->expose = OutputExpose;

    data->insertx = data->inserty = -99;
    data->suspend_hoffset = False;
    data->hoffset = 0;
    data->scrollwidth = 1;
    data->exposehscroll = data->exposevscroll = FALSE;
    data->stipple_tile = XmGetPixmapByDepth(XtScreen(widget), "50_foreground",
				            widget->primitive.foreground,
				            widget->core.background_pixel,
					    widget->core.depth);
    data->add_mode_cursor = XmUNSPECIFIED_PIXMAP;
    data->ibeam_off = XmUNSPECIFIED_PIXMAP;
    data->cursor = XmUNSPECIFIED_PIXMAP;
    data->timerid = (XtIntervalId)0;
    data->font = NULL;
    /* copy over the font list */
    if (data->fontlist == NULL)
        data->fontlist = _XmGetDefaultFontList(wid, XmTEXT_FONTLIST);
    data->fontlist = (XmFontList)XmFontListCopy(data->fontlist);
    LoadFontMetrics(widget);

    data->cursorwidth = 5;
    data->cursorheight = data->font_ascent + data->font_descent;
    widget->text.inner_widget = wid;
    data->leftmargin = data->rightmargin = widget->text.margin_width +
					  widget->primitive.shadow_thickness +
					  widget->primitive.highlight_thickness;
    data->topmargin = data->bottommargin = widget->text.margin_height +
					  widget->primitive.shadow_thickness +
					  widget->primitive.highlight_thickness;

  /* Don't word wrap, have multiple row or have vertical scrollbars
     if editMode is single_line */
     if (widget->text.edit_mode == XmSINGLE_LINE_EDIT)
        data->rows = 1;

  /* Don't grow in width if word wrap is on */
    if (widget->text.edit_mode != XmSINGLE_LINE_EDIT &&
	data->wordwrap)
       data->resizewidth = FALSE;

    if (data->rows <= 0) {
        _XmWarning(wid, MSG1);
        data->rows = 1;
    }

    if (data->columns <= 0) {
        _XmWarning(wid, MSG2);
        data->columns = 20;
    }

   /* Initialize columns_set and rows_set for Query Geometry.  Also
    * used in SizeFromRowsCols().
    */
    data->columns_set = data->columns;
    data->rows_set = data->rows;

    SizeFromRowsCols(widget, &width, &height);

    if (widget->core.width == 0)
        widget->core.width = width;
    if (widget->core.height == 0)
        widget->core.height = height;

   /* initialize number_lines before RefigureDependentInfo() */
    data->number_lines = widget->text.inner_widget->core.height -
                     data->topmargin - data->bottommargin;
    if (data->number_lines < (int) data->lineheight || !data->lineheight)
       data->number_lines = 1;
    else
       data->number_lines /= data->lineheight;

    if (widget->core.height != height || widget->core.width != width)
       RefigureDependentInfo(widget);

   /* reset columns_set and rows_set after RefigureDependentInfo() */
    data->columns_set = data->columns;
    data->rows_set = data->rows;
    data->prevW = widget->core.width;
    data->prevH = widget->core.height;
    data->minwidth = widget->core.width;
    data->minheight = widget->core.height;

    data->imagegc = NULL;
    data->gc = NULL;
    data->save_gc = NULL;
    data->has_rect = False;

    LoadGCs(widget, widget->core.background_pixel,
                         widget->primitive.foreground);

  /* Only create the scrollbars if text is a child of scrolled window */

   if ((XtClass(widget->core.parent) == xmScrolledWindowWidgetClass) &&
      (((XmScrolledWindowWidget)widget->core.parent)->swindow.VisualPolicy ==
				XmVARIABLE))
      
   {
       int n;
       static Arg arglist[30];
       static Arg swarglist[] = {
            {XmNhorizontalScrollBar, (XtArgVal) NULL},
            {XmNverticalScrollBar, (XtArgVal) NULL},
            {XmNworkWindow, (XtArgVal) NULL},
            {XmNscrollBarPlacement, (XtArgVal) NULL},
	};
       unsigned char unit_type;

      if (data->scrollhorizontal) {
          data->resizewidth = FALSE;
          data->ignorehbar = FALSE;

          n = 0;
          XtSetArg(arglist[n], XmNorientation, (XtArgVal) XmHORIZONTAL); n++;
          XtSetArg(arglist[n], XmNvalueChangedCallback,
                        (XtArgVal) hcallback); n++;
          XtSetArg(arglist[n], XmNincrementCallback, (XtArgVal) hcallback); n++;
          XtSetArg(arglist[n], XmNdecrementCallback, (XtArgVal) hcallback); n++;
          XtSetArg(arglist[n], XmNpageIncrementCallback,
                        (XtArgVal) hcallback); n++;
          XtSetArg(arglist[n], XmNpageDecrementCallback,
                        (XtArgVal) hcallback); n++;
          XtSetArg(arglist[n], XmNtoTopCallback, (XtArgVal) hcallback); n++;
          XtSetArg(arglist[n], XmNtoBottomCallback, (XtArgVal) hcallback); n++;
          XtSetArg(arglist[n], XmNdragCallback, (XtArgVal) hcallback); n++;
          XtSetArg(arglist[n], XmNshadowThickness,
                        (XtArgVal) widget->primitive.shadow_thickness); n++;
          XtSetArg(arglist[n], XmNminimum, 0); n++;
          XtSetArg(arglist[n], XmNmaximum, 1); n++;
          XtSetArg(arglist[n], XmNvalue, 0); n++;
          XtSetArg(arglist[n], XmNsliderSize, 1); n++;
          XtSetArg(arglist[n], XmNpageIncrement, 
                   widget->text.inner_widget->core.width); n++;
          XtSetArg(arglist[n], XmNincrement, data->averagecharwidth); n++;
          XtSetArg(arglist[n], XmNtraversalOn, False); n++;
          XtSetArg(arglist[n], XmNhighlightThickness, 0); n++;
          data->hbar = XmCreateScrollBar(XtParent(widget), "HorScrollBar",
					 arglist, n);

	  /* Don't re-convert shadow_thickness obtained from widget */
          XtSetArg(arglist[0], XmNunitType, &unit_type);
          XtGetValues(data->hbar, arglist, 1);
          if (unit_type != XmPIXELS)
          {
            XtSetArg(arglist[0], XmNunitType, XmPIXELS);
	    XtSetArg(arglist[1], XmNshadowThickness,
			  (XtArgVal) widget->primitive.shadow_thickness); n++;
            XtSetValues(data->hbar, arglist, 2);
            XtSetArg(arglist[0], XmNunitType, unit_type);
            XtSetValues(data->hbar, arglist, 1);
	  }

          XtManageChild(data->hbar);
      } else data->hbar = NULL;

      if (data->scrollvertical) {
          data->resizeheight = FALSE;
          data->ignorevbar = FALSE;

          n = 0;
          XtSetArg(arglist[n], XmNorientation, (XtArgVal) XmVERTICAL); n++;
          XtSetArg(arglist[n], XmNvalueChangedCallback,
                        (XtArgVal) vcallback); n++;
          XtSetArg(arglist[n], XmNincrementCallback, (XtArgVal) vcallback); n++;
          XtSetArg(arglist[n], XmNdecrementCallback, (XtArgVal) vcallback); n++;
          XtSetArg(arglist[n], XmNpageIncrementCallback,
                        (XtArgVal) vcallback); n++;
          XtSetArg(arglist[n], XmNpageDecrementCallback,
                        (XtArgVal) vcallback); n++;
          XtSetArg(arglist[n], XmNtoTopCallback, (XtArgVal) vcallback); n++;
          XtSetArg(arglist[n], XmNtoBottomCallback, (XtArgVal) vcallback); n++;
          XtSetArg(arglist[n], XmNdragCallback, (XtArgVal) vcallback); n++;
          XtSetArg(arglist[n], XmNshadowThickness,
                        (XtArgVal) widget->primitive.shadow_thickness); n++;
          XtSetArg(arglist[n], XmNminimum, 0); n++;
          XtSetArg(arglist[n], XmNmaximum, 5); n++;
          XtSetArg(arglist[n], XmNvalue, 0); n++;
          XtSetArg(arglist[n], XmNsliderSize, 5); n++;
          XtSetArg(arglist[n], XmNtraversalOn, False); n++;
          XtSetArg(arglist[n], XmNhighlightThickness, 0); n++;
          if (data->number_lines > 1) {
            XtSetArg(arglist[n], XmNpageIncrement, data->number_lines - 1); n++;
          } else {
            XtSetArg(arglist[n], XmNpageIncrement, 1); n++;
          }
          data->vbar = XmCreateScrollBar(XtParent(widget), "VertScrollBar", arglist, n);

	  /* Don't re-convert shadow_thickness obtained from widget */
          XtSetArg(arglist[0], XmNunitType, &unit_type);
          XtGetValues(data->vbar, arglist, 1);
          if (unit_type != XmPIXELS)
          {
            XtSetArg(arglist[0], XmNunitType, XmPIXELS);
	    XtSetArg(arglist[1], XmNshadowThickness,
			  (XtArgVal) widget->primitive.shadow_thickness); n++;
            XtSetValues(data->vbar, arglist, 2);
            XtSetArg(arglist[0], XmNunitType, unit_type);
            XtSetValues(data->vbar, arglist, 1);
	  }

          if (widget->text.edit_mode != XmSINGLE_LINE_EDIT)
             XtManageChild(data->vbar);
          XtAddEventHandler(data->vbar, (EventMask)ButtonReleaseMask,
                            FALSE, HandleVBarButtonRelease, (XtPointer)widget);
      } else data->vbar = NULL;

      swarglist[0].value = (XtArgVal)data->hbar;
      swarglist[1].value = (XtArgVal)data->vbar;
      swarglist[2].value = (XtArgVal)widget;

    /* Tell scrolled window parent where to put the scrollbars */

        if (data->scrollleftside) {
           if (data->scrolltopside)
              swarglist[3].value = (XtArgVal) XmTOP_LEFT;
           else
              swarglist[3].value = (XtArgVal) XmBOTTOM_LEFT;
        } else {
           if (data->scrolltopside)
              swarglist[3].value = (XtArgVal) XmTOP_RIGHT;
           else
              swarglist[3].value = (XtArgVal) XmBOTTOM_RIGHT;
        }
        XtSetValues(widget->core.parent, swarglist, XtNumber(swarglist));

    } else {
        data->vbar = NULL;
        data->hbar = NULL;
        if ((XtClass(widget->core.parent) == xmScrolledWindowWidgetClass) &&
           (((XmScrolledWindowWidget)widget->core.parent)->swindow.VisualPolicy
					 == XmCONSTANT)) {
           data->scrollhorizontal = FALSE;
           data->scrollvertical = FALSE;
           data->resizewidth = TRUE;
           data->resizeheight = TRUE;
        }
    }

    data->hasfocus = FALSE;
    data->blinkstate = on;
    data->cursor_on = 0;
    data->refresh_ibeam_off = True;
    data->have_inverted_image_gc = False;
    data->handlingexposures = FALSE;
    XtAddEventHandler((Widget) widget->text.inner_widget,
                     (EventMask)FocusChangeMask|EnterWindowMask|LeaveWindowMask,
                      FALSE, HandleFocusEvents, (XtPointer)NULL);
    XtAddEventHandler((Widget) widget->text.inner_widget,
                      (EventMask) 0, TRUE, HandleGraphicsExposure,
                      (XtPointer)NULL);
    CheckHasRect(widget);
}

/* ARGSUSED */
static Pixmap
#ifdef _NO_PROTO
GetClipMask(tw, pixmap_name)
	XmTextWidget tw;
	char *pixmap_name;
#else
GetClipMask(
	XmTextWidget tw,
	char *pixmap_name)
#endif /* _NO_PROTO */
{
   Display *dpy = XtDisplay(tw);
   Screen *screen = XtScreen((Widget)tw);
   XGCValues values;
   GC fillGC;
   Pixmap clip_mask;
   OutputData data = tw->text.output->data;

   clip_mask = XCreatePixmap(dpy, RootWindowOfScreen(screen),
                             data->cursorwidth, data->cursorheight, 1);

   values.foreground = 1;
   fillGC = XCreateGC(dpy, clip_mask, GCForeground, &values);

   XFillRectangle(dpy, clip_mask, fillGC, 0, 0, data->cursorwidth,
                  data->cursorheight);

   /* Install the pixmap for pixmap caching */
    _XmInstallPixmap(clip_mask, screen, pixmap_name, 1, 0);

   XFreeGC(XtDisplay(tw), fillGC);

   return(clip_mask);
}

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmTextGetBaselines( widget, baselines, line_count )
        Widget widget ;
	Dimension ** baselines;
	int *line_count;
#else
_XmTextGetBaselines(
        Widget widget,
	Dimension ** baselines,
	int *line_count )
#endif /* _NO_PROTO */
{
   XmTextWidget w = (XmTextWidget) widget;
   OutputData data = w->text.output->data;
   Dimension *base_array;
   int i;

   *line_count = data->number_lines;

   base_array = (Dimension *)XtMalloc((sizeof(Dimension) * (*line_count)));

   for (i = 0; i < *line_count; i++) {
       base_array[i] = data->topmargin + i * data->lineheight +
		       data->font_ascent;
   }

   *baselines = base_array;

   return (TRUE);
}


/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmTextGetDisplayRect( w, display_rect )
        Widget w;
	XRectangle * display_rect;
#else
_XmTextGetDisplayRect(
        Widget w,
	XRectangle * display_rect )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;
   OutputData data = tw->text.output->data;

   (*display_rect).x = data->leftmargin;
   (*display_rect).y = data->topmargin;
   (*display_rect).width = tw->core.width -
			   (data->leftmargin + data->rightmargin);
   (*display_rect).height = data->number_lines * data->lineheight;

   return(TRUE);
}


/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmTextMarginsProc( w, margins_rec )
        Widget w ;
	XmBaselineMargins *margins_rec;
#else
_XmTextMarginsProc(
        Widget w,
	XmBaselineMargins *margins_rec )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    OutputData data = tw->text.output->data;

    if (margins_rec->get_or_set == XmBASELINE_SET) {
       data->topmargin = margins_rec->margin_top +
			     tw->primitive.shadow_thickness +
			     tw->primitive.highlight_thickness;
       data->bottommargin = margins_rec->margin_bottom +
			     tw->primitive.shadow_thickness +
			     tw->primitive.highlight_thickness;
       posToXYCachedWidget = NULL;
    } else {
       margins_rec->margin_top = data->topmargin -
			     (tw->primitive.shadow_thickness +
			      tw->primitive.highlight_thickness);
       margins_rec->margin_bottom = data->bottommargin -
			     (tw->primitive.shadow_thickness +
			      tw->primitive.highlight_thickness);
       margins_rec->text_height =  data->font_ascent + data->font_descent;
       margins_rec->shadow = tw->primitive.shadow_thickness;
       margins_rec->highlight = tw->primitive.highlight_thickness;
    }
}

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
void 
#ifdef _NO_PROTO
_XmTextChangeHOffset( widget, length )
        XmTextWidget widget ;
        int length ;
#else
_XmTextChangeHOffset(
        XmTextWidget widget,
        int length )
#endif /* _NO_PROTO */
{
   OutputData data = widget->text.output->data;
   Dimension margin_width = widget->text.margin_width +
                           widget->primitive.shadow_thickness +
                           widget->primitive.highlight_thickness;
   int new_offset = data->hoffset;
   XmTextPosition nextpos;
   XmTextPosition last_position;
   XmTextPosition temp;
   int inner_width, width, i;
   int text_width = 0;
   int new_text_width;
   XmTextBlockRec block;

   new_offset += length;

   for (i = 0 ; i < widget->text.number_lines ; i++) {
      last_position = (*widget->text.source->Scan) (widget->text.source,
                                                    widget->text.line[i].start,
                                                    XmSELECT_LINE,
                                                    XmsdRight, 1, FALSE);
      nextpos = (*widget->text.source->Scan)(widget->text.source,
                                              last_position, XmSELECT_LINE,
                                              XmsdRight, 1, TRUE);
      if (nextpos == last_position)
          nextpos = PASTENDPOS;
      width = 0;
      temp = widget->text.line[i].start;
      while (temp < last_position) {
            temp = (*widget->text.source->ReadSource)
		        (widget->text.source, temp, last_position, &block);
            width += FindWidth(widget, (Position) width, &block,
                                0, block.length);
      }
      new_text_width = width;
      if (new_text_width > text_width) text_width = new_text_width;
   }

   inner_width = widget->core.width - (2 * margin_width);
   if (new_offset >= text_width - inner_width)
       new_offset = text_width - inner_width;

   ChangeHOffset(widget, new_offset, True);
}

/*****************************************************************************
 * To make TextOut a true "Object" this function should be a class function. *
 *****************************************************************************/
void 
#ifdef _NO_PROTO
_XmTextToggleCursorGC( widget )
	Widget widget;
#else
_XmTextToggleCursorGC(
	Widget widget )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) widget;
    OutputData data = tw->text.output->data;
    InputData i_data = tw->text.input->data;
    XGCValues values;
    unsigned long valuemask = GCFillStyle|GCFunction|GCForeground|GCBackground;

    if (!XtIsRealized((Widget)tw)) return;

    if (i_data->overstrike) {
      if (XtIsSensitive(widget) && !tw->text.add_mode &&
	  (data->hasfocus || _XmTextHasDestination(widget)))
	values.fill_style = FillSolid;
      else
	values.fill_style = FillTiled;
      values.foreground = values.background =
	tw->primitive.foreground ^ tw->core.background_pixel;
      values.function = GXxor;
    } else {
      if (XtIsSensitive(widget) && !tw->text.add_mode &&
	  (data->hasfocus || _XmTextHasDestination(widget)))
	values.stipple = data->cursor;
      else
	values.stipple = data->add_mode_cursor;
      if (tw->text.input->data->overstrike) {
	values.background = values.foreground = 
	  tw->core.background_pixel ^ tw->primitive.foreground;
      } else if (data->have_inverted_image_gc){
	values.background = tw->primitive.foreground;
	values.foreground = tw->core.background_pixel;
      } else {
	values.foreground = tw->primitive.foreground;
	values.background = tw->core.background_pixel;
      }
      values.fill_style = FillStippled;
      values.function = GXcopy;
      valuemask |= GCStipple;
    }
    XChangeGC(XtDisplay(widget), data->imagegc, valuemask, &values);
}
