/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)textedit:TextUtil.c	1.14"
#endif

/*
 * TextUtil.c
 *
 */

#include <stdio.h>

#include <X11/memutil.h>
#include <X11/IntrinsicP.h>
#include <X11/Xatom.h>

#include <OpenLook.h>
#include <Error.h>
#include <TextUtil.h>
#include <Util.h>
#include <textbuff.h>	/* for _StringLength */


typedef struct _PropertyEventInfo
   {
   Display * display;
   Window    window;
   Atom      atom;
   } PropertyEventInfo;

static Bool _IsPropertyNotify();


/*
 * _IsGraphicsExpose
 *
 * The \fI_IsGraphicsExpose\fR function is used as a predicate function
 * for check event processing.  It is normally used to determine if
 * GraphicsExpose events caused by scrolling (XCopyArea) window contents
 * has occurred.  Note a XCopyArea is guaranteed to generate at least
 * one NoExpose event and this event will follow all necessary GraphicsExpose
 * events caused by the copy.
 *
 * Synopsis:
 *
 *#include <TextUtil.h>
 * ...
 */

extern Bool
_IsGraphicsExpose(d, event, arg)
Display *     d;
XEvent *      event;
char *        arg;
{

return (Bool)(event-> xany.window == (Window)arg
                                  &&
             (event-> type == GraphicsExpose || event-> type == NoExpose));

} /* _IsGraphicsExpose() */
/*
 * _CreateCursor
 *
 * The \fI_CreateCursor\fR function is a utility used to create a cursor
 * from the given \fIsource\fR and \fImask\fR files colored as
 * specified in \fIforeground\fR and \fIbackground\fR for the \fIdisplay\fR
 * and \fIscreen\fR.

 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <TextUtil.h>
 * ...
 */

extern Cursor
_CreateCursor(display, screen, foreground, background, source, mask)
Display * display;
Screen *  screen;
char *    foreground;
char *    background;
char *    source;
char *    mask;
{
XColor       fg;
XColor       bg;
XColor       xfg;
XColor       xbg;
Pixmap       sourcepm;
Pixmap       maskpm;
unsigned int wid;
unsigned int ht;
int          XHot;
int          YHot;
Colormap     cmap = DefaultColormapOfScreen(screen);
Window       root = RootWindowOfScreen(screen);

XAllocNamedColor(display, cmap, foreground, &fg, &xfg);
XAllocNamedColor(display, cmap, background, &bg, &xbg);

if (XReadBitmapFile(display, root, source, &wid, &ht, &sourcepm, &XHot, &YHot)
    != BitmapSuccess)
  OlVaDisplayWarningMsg(display,
			OleNfileOlCursors,
			OleTmsg1,
			OleCOlToolkitWarning,
			OleMfileOlCursors_msg1,
			source);

if (XReadBitmapFile(display, root, mask, &wid, &ht, &maskpm, &XHot, &YHot)
    != BitmapSuccess)
  OlVaDisplayWarningMsg(display,
			OleNfileOlCursors,
			OleTmsg1,
			OleCOlToolkitWarning,
			OleMfileOlCursors_msg1,
			mask);

return (XCreatePixmapCursor(display, sourcepm, maskpm, &fg, &bg, XHot, YHot));

} /* end of _CreateCursor */
/*
 * _CreateCursorFromBitmaps
 *
 * The \fI_CreateCursorFromBitmaps\fR procedure is used to create the
 * the TextEdit drag-and-drop cursor.  It uses the bitmaps created
 * by the DragText procedure (in TextEdit.c); clearing the text area
 * in the source bitmap then drawing the text into the source.
 *
 * See also:
 *
 * _CreateCursorFromBitmaps(3)
 *
 * Synopsis:
 *
 *#include <TextUtil.h>
 * ...
 */

extern Cursor
_CreateCursorFromBitmaps (dpy, screen, source, mask, gc, foreground, background, s, l, x, y, XHot, YHot, more)
Display * dpy;
Screen *  screen;
Pixmap    source;
Pixmap    mask;
GC        gc;
char *    foreground;
char *    background;
char *    s;
int       l;
Position  x;
Position  y;
int       XHot;
int       YHot;
Pixmap    more;
{
XColor    fg;
XColor    bg;
XColor    xfg;
XColor    xbg;
Colormap  cmap = DefaultColormapOfScreen(screen);

XAllocNamedColor(dpy, cmap, foreground, &fg, &xfg);
XAllocNamedColor(dpy, cmap, background, &bg, &xbg);

XSetFunction(dpy, gc, GXcopy);

/*
 * note: the rectangle values can be set in the create function
 *       and used in the fill rectangle below
 *
 */

XFillRectangle(dpy, source, gc, 13, 10, 33, 12);
XSetFunction(dpy, gc, GXcopyInverted);

/*
 * note: x and y should be calculated in the routines here!
 *
 */

XDrawString(dpy, source, gc, x, y, s, l);

if (more)
   XCopyPlane(dpy, more, source, gc, 0, 0, 5, 9, 35, 10, 1L);


return (XCreatePixmapCursor(dpy, source, mask, &fg, &bg, XHot, YHot));

} /* end of _CreateCursorFromBitmaps */
/*
 * _XtLastTimestampProcessed
 *
 */

extern Time
_XtLastTimestampProcessed(widget)
Widget widget;
{
PropertyEventInfo lti;
XEvent            event;

lti.display = XtDisplay(widget);
lti.window  = XtWindow(widget);
/* FLH REMOVE when OL_XA_TIMESTAMP is defined */
lti.atom    = XInternAtom(XtDisplay(widget), "TIMESTAMP", False);

XChangeProperty(lti.display, lti.window, lti.atom, 
                XA_STRING, 8, PropModeAppend, (OLconst unsigned char *)"", 0);

XIfEvent(lti.display, &event, _IsPropertyNotify, (char *)&lti);

return (event.xproperty.time);

} /* end of _XtLastTimestampProcessed */
/*
 * _IsPropertyNotify
 *
 * The \fI_IsPropertyNotify\fR function is used as a predicate function
 * for check event processing.  It is normally used to determine when
 * PropertyNotify events caused when attempting to get a timestamp
 * has occurred.
 *
 * Synopsis:
 *
 *#include <TextUtil.h>
 * ...
 */

static Bool
_IsPropertyNotify(d, event, arg)
Display *     d;
XEvent *      event;
char *        arg;
{
PropertyEventInfo * propinfo = (PropertyEventInfo *)arg;

#ifdef DEBUG
fprintf(stderr,"type = %d (%d)\ndpy  = %x (%x)\nwin  = %x (%x)atom = %d (%d)\n",
event-> type, PropertyNotify, 
event-> xproperty.display , propinfo-> display ,
event-> xproperty.window  , propinfo-> window  ,
event-> xproperty.atom    , propinfo-> atom);
#endif

return (Bool)(event-> type == PropertyNotify                  &&
              event-> xproperty.display == propinfo-> display &&
              event-> xproperty.window  == propinfo-> window  &&
              event-> xproperty.atom    == propinfo-> atom);

} /* _IsPropertyNotify */

/*
 * _mbCopyOfwcString
 *
 * The \fI_mbCopyOfwcString\fR function is used to convert a string
 * wchar_t BufferElements to a string of multibyte characters.
 * The return value is the number of bytes filled by the converted
 * multibyte string.
 * 
 * The memory for the copy is allocated by this function.  It
 * is the responsibility of the caller to free the memory.
 *
 */

#ifdef I18N
extern int
_mbCopyOfwcString OLARGLIST((mb_string, wc_string))
OLARG(char **,         mb_string)
OLGRA(BufferElement *, wc_string)
{
int mb_size;
int buffer_size;

if (wc_string){
   buffer_size = sizeof(BufferElement) * _StringLength(wc_string) + 1;
   *mb_string = (char *) MALLOC(buffer_size);
   mb_size = wcstombs(*mb_string, wc_string, buffer_size);
   (*mb_string)[mb_size] = '\0';
}
#ifdef DEBUG
printf("WCHAR-->MB\n");
printf("\tbuffer_size is %d CHARS\n", _StringLength(wc_string));
if (*mb_string)
printf("\tstring:%s\n",*mb_string);
#endif

return (mb_size);
} /* end of _mbCopyOfwcString */
#endif

/*
 * _mbCopyOfwcSegment
 *
 * The \fI_mbCopyOfwcSegment\fR function is used to convert a segment
 * of \fIlength\fR wchar_t BufferElements to a string of multibyte
 * characters.
 *
 * The memory for the copy is allocated by this function.  It
 * is the responsibility of the caller to free the memory.
 *
 */

#ifdef I18N
extern int
_mbCopyOfwcSegment OLARGLIST((mb_string, wc_string, wc_length))
OLARG(char **,         mb_string)
OLARG(BufferElement *, wc_string)
OLGRA(int,             wc_length)
{
char *    p;
char *    mbstart;
int       i;
int       count = 0;
int       mb_length = 0;

if (wc_string){
   if (wc_length > 0) {
      *mb_string = mbstart = (char *) MALLOC(sizeof(BufferElement) * wc_length);
      for (p = mbstart, count = 0; count++ < wc_length; p = &mbstart[mb_length])
      {
         mb_length +=  wctomb(p, *wc_string);
         wc_string++;
      }
   }
}

return (mb_length);
} /* end of _mbCopyOfwcSegment */
#endif
