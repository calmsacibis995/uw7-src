/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)textedit:Margin.c	1.8"
#endif

/*
 * Margin.c
 *
 */

/*#define DEBUG/**/

#include <buffutil.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <textbuff.h>		/* must follow IntrinsicP.h */

#include <OpenLookP.h>
#include <Dynamic.h>
#include <TextEdit.h>
#include <TextEditP.h>
#include <Margin.h>
#include <Util.h>

#define ITEMHT(ctx)             (ctx-> textedit.lineHeight)
#define FONT(ctx)               ctx-> primitive.font
#define NORMALGC(ctx)           ctx-> textedit.gc

static int     LocateItem();


/*
 * _OlRegisterTextLineNumberMargin
 *
 */

extern void
_OlRegisterTextLineNumberMargin(ctx)
	TextEditWidget ctx;
{

XtAddCallback((Widget)ctx, XtNmargin, _OlDisplayTextEditLineNumberMargin, NULL);

if (XtIsRealized((Widget)ctx))
   {
   XClearArea(XtDisplay((Widget)ctx), XtWindow((Widget)ctx), 
       0, PAGE_T_MARGIN(ctx), PAGE_L_MARGIN(ctx), PAGEHT(ctx), False);

   _OlDisplayTextEditLineNumberMargin(ctx, NULL, NULL);
   }

} /* end of _OlRegisterTextLineNumberMargin */
/*
 * _OlUnregisterTextLineNumberMargin
 *
 */

extern void
_OlUnregisterTextLineNumberMargin(ctx)
	TextEditWidget ctx;
{

if (XtIsRealized((Widget)ctx))
   XClearArea(XtDisplay((Widget)ctx), XtWindow((Widget)ctx), 
       0, PAGE_T_MARGIN(ctx), PAGE_L_MARGIN(ctx), PAGEHT(ctx), False);

XtRemoveCallback((Widget)ctx, XtNmargin, _OlDisplayTextEditLineNumberMargin, NULL);

} /* end of _OLUnregisterTextLineNumberMargin */
/*
 * _OlDisplayTextEditLineNumberMargin
 *
 * Note: This is defined as an extern procedure to allow users to
 *       simply add is as a callback without using the provided
 *       Register/Unregister functions.
 */

extern void
_OlDisplayTextEditLineNumberMargin(w, client_data, call_data)
	Widget  w;
	caddr_t client_data;
	caddr_t call_data;
{
TextEditWidget         ctx          = (TextEditWidget)w;
Display *              dpy          = XtDisplay(ctx);
Window                 win          = XtWindow(ctx);
XFontStruct *          font         = (ctx-> primitive.font_list ?
                                       ctx-> primitive.font_list-> fontl[0] :
                                       FONT(ctx));
GC                     gc           = NORMALGC(ctx);
int                    fontht       = ITEMHT(ctx);
int                    ascent       = ASCENT(ctx);
int                    lmargin      = PAGE_L_MARGIN(ctx) - FONTWID(ctx);
DisplayTable *         DT           = ctx-> textedit.DT;
OlTextMarginCallData * cd           = (OlTextMarginCallData *) call_data;
XRectangle *           cd_rect      = cd-> rect;
XRectangle             rect;
int                    start;
int                    end;
int                    len;
int                    x;
int                    y;
int                    i;
char                   buffer[20];

if (cd_rect &&
   (cd_rect-> x > PAGE_L_MARGIN(ctx) || 
    cd_rect-> y + (Position)cd_rect-> height < PAGE_T_MARGIN(ctx) || 
    cd_rect-> y > PAGE_B_MARGIN(ctx)))
   return;

if (cd_rect == NULL)
   {
   start = 0;
   end   = LINES_VISIBLE(ctx) - 1;
   }
else
   {
   y = cd_rect-> y + cd_rect-> height;
   start = LocateItem(ctx, MAX(cd_rect-> y, PAGE_T_MARGIN(ctx)));
   end   = LocateItem(ctx, MIN(y, PAGE_B_MARGIN(ctx)));
   }

rect.x      = 0;
rect.width  = PAGE_L_MARGIN(ctx);
rect.y      = PAGE_T_MARGIN(ctx) + fontht * start;
rect.height = PAGE_T_MARGIN(ctx) + fontht * end;

XClearArea(dpy, win, rect.x, rect.y, rect.width, rect.height, False);

for (i = start, y = PAGE_T_MARGIN(ctx) + (i * fontht) + ascent; 
     i <= end; i++, y += fontht)
   {
   if (DT[i].p == NULL)
      break;
   else
      if (DT[i].wraploc.offset == 0)
         {
         sprintf(buffer, "%d", DT[i].wraploc.line + 1);
         len = strlen(buffer);
         x   = lmargin - XTextWidth(font, buffer, len);
         XDrawImageString(dpy, win, gc, x, y, buffer, len);
         }
   }

} /* end of _OlDisplayTextEditLineNumberMargin */
/*
 * LocateItem
 *
 */

static int
LocateItem(ctx, y)
	TextEditWidget	ctx;
	int		y;
{
int i;

if (y < PAGE_T_MARGIN(ctx))
   i = 0;
else
   if (y > PAGE_B_MARGIN(ctx))
      i = LINES_VISIBLE(ctx) - 1;
   else
      i = (y - PAGE_T_MARGIN(ctx)) / ITEMHT(ctx);

return (i);

} /* end of LocateItem */
