/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:OlCursors.c	1.8"
#endif

/*
 * OlCursors.c
 *
 */

#include <stdio.h>

#include <X11/IntrinsicP.h>

#include <Xol/OpenLookP.h>
#include <OlCursors.h>
#include <X11/cursorfont.h>


/*
 * _GetBandWXColors
 *
 * The \fI_GetBandWXColors\fR procedure is used to retrieve the XColor
 * structures for the \fIblack\fR and \fIwhite\fR colors of \fIscreen\fR.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern void
_GetBandWXColors OLARGLIST((screen, black, white))
	OLARG( Screen *, 	screen)
	OLARG( XColor *,	black)
	OLGRA( XColor *,	white)
{
static char   initialized = 0;
static XColor rblack;
static XColor rwhite;

XColor        xblack;
XColor        xwhite;
Colormap      cmap = DefaultColormapOfScreen(screen);
Display *     dpy  = DisplayOfScreen(screen);

if (!initialized)
   {
   initialized = 1;
   XAllocNamedColor(dpy, cmap, "black", &rblack, &xblack);
   XAllocNamedColor(dpy, cmap, "white", &rwhite, &xwhite);
   }

*black = rblack;
*white = rwhite;

} /* end of _GetBandWXColors */
/*
 * _CreateCursorFromFiles
 *
 * The \fI_CreateCursorFromData\fR function is used to construct a cursor
 * for \fIscreen\fR using the data in \fIsourcefile\fR and \fImaskfile\fR.
 *
 * See also:
 * 
 * _CreateCursorFromData(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Cursor
_CreateCursorFromFiles OLARGLIST((screen, sourcefile, maskfile))
	OLARG( Screen *,	screen)
	OLARG( char *,	    	sourcefile)
	OLGRA( char *,	    	maskfile)
{
Pixmap        source;
Pixmap        mask;
XColor        white;
XColor        black;
Display *     dpy     = DisplayOfScreen(screen);
Window        root    = RootWindowOfScreen(screen);
Cursor        cursor  = 0;
unsigned int  w;
unsigned int  h;
int           xhot;
int           yhot;

_GetBandWXColors(screen, &black, &white);

if (XReadBitmapFile(dpy, root, sourcefile, &w, &h, &source, &xhot, &yhot)
    != BitmapSuccess)

	OlVaDisplayErrorMsg(	dpy,
				OleNinvalidDimension,
				OleTbadGeometry,
				OleCOlToolkitError,
				OleMinvalidDimension_badGeometry,
				sourcefile);

if (XReadBitmapFile(dpy, root, maskfile, &w, &h, &mask, &xhot, &yhot)
    != BitmapSuccess)

	OlVaDisplayErrorMsg(	dpy,
				OleNinvalidDimension,
				OleTbadGeometry,
				OleCOlToolkitError,
				OleMinvalidDimension_badGeometry,
				maskfile);

cursor = XCreatePixmapCursor(dpy, source, mask, &black, &white, xhot, yhot);

XFreePixmap(dpy, source);
XFreePixmap(dpy, mask);

return (cursor);

} /* end of _CreateCursorFromFiles */
/*
 * _CreateCursorFromData
 *
 * The \fI_CreateCursorFromData\fR function is used to construct a cursor
 * for \fIscreen\fR using the source data \fIsbits\fR and mask \fImbits\fR
 * whose dimensions are defined by \fIw\fR and \fIh\fR and whose hot-spot
 * position is defined by \fIxhot\fR and \fIyhot\fR.
 *
 * See also:
 * 
 * _CreateCursorFromFiles(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Cursor
_CreateCursorFromData OLARGLIST((screen, sbits, mbits, w, h, xhot, yhot))
	OLARG( Screen *,	screen)
	OLARG( unsigned char *,	sbits)
	OLARG( unsigned char *,	mbits)
	OLARG( int,		w)
	OLARG( int,		h)
	OLARG( int,		xhot)
	OLGRA( int,		yhot)
{
Pixmap        source;
Pixmap        mask;
XColor        white;
XColor        black;
Display *     dpy    = DisplayOfScreen(screen);
Window        root   = RootWindowOfScreen(screen);
Cursor        cursor = 0;

_GetBandWXColors(screen, &black, &white);

source = XCreateBitmapFromData(dpy, root, (OLconst char *)sbits, w, h);
mask   = XCreateBitmapFromData(dpy, root, (OLconst char *)mbits, w, h);

cursor = XCreatePixmapCursor(dpy, source, mask, &black, &white, xhot, yhot);

XFreePixmap(dpy, source);
XFreePixmap(dpy, mask);

return (cursor);

} /* end of _CreateCursorFromData */
/*
 * GetOlMoveCursor
 *
 * The \fIGetOlMoveCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMove\fR cursor.
 *
 * See also:
 *
 * GetOlDuplicateCursor(3), GetOlBusyCursor(3),
 * GetOlPanCursor(3), GetOlQuestionCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Cursor
GetOlMoveCursor OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
#include <movcurs.h>
#include <movmask.h>

static Display *MoveDisplay;
static Cursor MoveCursor;

if (MoveCursor == (Cursor)0 || MoveDisplay != DisplayOfScreen(screen))
   {
     MoveDisplay = DisplayOfScreen(screen);
     if (OlGetGui() == OL_OPENLOOK_GUI)
       MoveCursor = _CreateCursorFromData(screen, movcurs_bits,
					  movmask_bits, movcurs_width,
					  movcurs_height, movcurs_x_hot,
					  movcurs_y_hot);
     else
       MoveCursor = XCreateFontCursor(MoveDisplay, XC_fleur);
   }

return (MoveCursor);

} /* end of GetOlMoveCursor */
/*
 * GetOlDuplicateCursor
 *
 * The \fIGetOlDuplicateCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDuplicate\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlBusyCursor(3),
 * GetOlPanCursor(3), GetOlQuestionCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Cursor
GetOlDuplicateCursor OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
#include <dupcurs.h>
#include <dupmask.h>

static Display *DuplicateDisplay;
static Cursor DuplicateCursor;

if (DuplicateCursor == (Cursor)0 || DuplicateDisplay != DisplayOfScreen(screen))
   {
     DuplicateDisplay = DisplayOfScreen(screen);
   DuplicateCursor = _CreateCursorFromData(screen, dupcurs_bits, dupmask_bits, 
      dupcurs_width, dupcurs_height, dupcurs_x_hot, dupcurs_y_hot);
   }

return (DuplicateCursor);

} /* end of GetOlDuplicateCursor */
/*
 * GetOlBusyCursor
 *
 * The \fIGetOlBusyCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIBusy\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3),
 * GetOlPanCursor(3), GetOlQuestionCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Cursor
GetOlBusyCursor OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
#include <busycurs.h>
#include <busymask.h>

static Display *BusyDisplay;
static Cursor BusyCursor;

if (BusyCursor == (Cursor)0 || BusyDisplay != DisplayOfScreen(screen))
   {
     BusyDisplay = DisplayOfScreen(screen);
     if (OlGetGui() == OL_OPENLOOK_GUI) 
       BusyCursor = _CreateCursorFromData(screen, busycurs_bits,
					  busymask_bits, busycurs_width,
					  busycurs_height, busycurs_x_hot,
					  busycurs_y_hot);
     else
       BusyCursor = XCreateFontCursor(BusyDisplay, XC_watch);
   }

return (BusyCursor);

} /* end of GetOlBusyCursor */
/*
 * GetOlPanCursor
 *
 * The \fIGetOlPanCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIPan\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3), 
 * GetOlBusyCursor(3), GetOlQuestionCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Cursor
GetOlPanCursor OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
#include <pancurs.h>
#include <panmask.h>

static Display *PanDisplay;
static Cursor PanCursor;

if (PanCursor == (Cursor)0 || PanDisplay != DisplayOfScreen(screen))
   {
     PanDisplay = DisplayOfScreen(screen);
   PanCursor = _CreateCursorFromData(screen, pancurs_bits, panmask_bits, 
      pancurs_width, pancurs_height, pancurs_x_hot, pancurs_y_hot);
   }

return (PanCursor);

} /* end of GetOlPanCursor */
/*
 * GetOlQuestionCursor
 *
 * The \fIGetOlQuestionCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIQuestion\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3), 
 * GetOlBusyCursor(3), GetOlPanCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Cursor
GetOlQuestionCursor OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
#include <quescurs.h>
#include <quesmask.h>

static Display *QuestionDisplay;
static Cursor QuestionCursor;

if (QuestionCursor == (Cursor)0 || QuestionDisplay != DisplayOfScreen(screen))
   {
     QuestionDisplay = DisplayOfScreen(screen);
     if (OlGetGui() == OL_OPENLOOK_GUI)
       QuestionCursor = _CreateCursorFromData(screen, quescurs_bits,
					      quesmask_bits,
					      quescurs_width,
					      quescurs_height,
					      quescurs_x_hot,
					      quescurs_y_hot);
     else 
       QuestionCursor = XCreateFontCursor(QuestionDisplay,
					  XC_question_arrow); 
   }

return (QuestionCursor);

} /* end of GetOlQuestionCursor */
/*
 * GetOlTargetCursor
 *
 * The \fIGetOlTargetCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITarget\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3), 
 * GetOlBusyCursor(3), GetOlPanCursor(3), 
 * GetOlQuestionCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Cursor
GetOlTargetCursor OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
#include <targcurs.h>
#include <targmask.h>

static Display *TargetDisplay;
static Cursor TargetCursor;

if (TargetCursor == (Cursor)0 || TargetDisplay != DisplayOfScreen(screen))
   {
     TargetDisplay = DisplayOfScreen(screen);
     if (OlGetGui() == OL_OPENLOOK_GUI)
       TargetCursor = _CreateCursorFromData(screen, targcurs_bits,
					    targmask_bits, targcurs_width,
					    targcurs_height,
					    targcurs_x_hot,
					    targcurs_y_hot);
     else 
       TargetCursor = XCreateFontCursor(TargetDisplay, XC_crosshair);
   }

return (TargetCursor);

} /* end of GetOlTargetCursor */
/*
 * GetOlStandardCursor
 *
 * The \fIGetOlStandardCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIStandard\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3), 
 * GetOlBusyCursor(3), GetOlPanCursor(3), 
 * GetOlQuestionCursor(3), GetOlTargetCursor(3),
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Cursor
GetOlStandardCursor OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
#include <stdcurs.h>
#include <stdmask.h>

static Display *StandardDisplay;
static Cursor StandardCursor;

if (StandardCursor == (Cursor)0 || StandardDisplay != DisplayOfScreen(screen))
   {
     StandardDisplay = DisplayOfScreen(screen);
     if (OlGetGui() == OL_OPENLOOK_GUI)
       StandardCursor = _CreateCursorFromData(screen, stdcurs_bits,
					      stdmask_bits, stdcurs_width,
					      stdcurs_height,
					      stdcurs_x_hot,
					      stdcurs_y_hot);
     else 
       StandardCursor = XCreateFontCursor(StandardDisplay, XC_left_ptr);
   }

return (StandardCursor);

} /* end of GetOlStandardCursor */
/*
 * OlGet50PercentGrey
 *
 * The \fIOlGet50PercentGrey\fR function is used to retrieve the id
 * of a 50 percent grey Pixmap for \fIscreen\fR.
 *
 * Return value:
 *
 * The Pixmap id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Pixmap
OlGet50PercentGrey OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
static Pixmap grey = NULL;

#define grey_width 2
#define grey_height 2
static OLconst char grey_bits[] = { 0x01, 0x02};

if (grey == NULL)
   grey = XCreateBitmapFromData(DisplayOfScreen(screen),
                    RootWindowOfScreen(screen),
                    grey_bits, grey_width, grey_height);
#undef grey_width
#undef grey_height

return (grey);

} /* end of OlGet50PercentGrey */
/*
 * OlGet75PercentGrey
 *
 * The \fIOlGet75PercentGrey\fR function is used to retrieve the id
 * of a 75 percent grey Pixmap for \fIscreen\fR.
 *
 * Return value:
 *
 * The Pixmap id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

extern Pixmap
OlGet75PercentGrey OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
static Pixmap grey = NULL;

#define grey_width 4
#define grey_height 2
static OLconst char grey_bits[] = { 0x0d, 0x07};

if (grey == NULL)
   grey = XCreateBitmapFromData(DisplayOfScreen(screen),
                    RootWindowOfScreen(screen),
                    grey_bits, grey_width, grey_height);
#undef grey_width
#undef grey_height

return (grey);

} /* end of OlGet75PercentGrey */

/* probably should put the following info into headers...	*/
#define nocur_width 18
#define nocur_height 17
#define nocur_x_hot 8
#define nocur_y_hot 8

static unsigned char nocur_bits[] = {
   0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0xf0, 0x1f, 0x00, 0x38, 0x38, 0x00,
   0x0c, 0x70, 0x00, 0x0c, 0x68, 0x00, 0x06, 0xc4, 0x00, 0x06, 0xc2, 0x00,
   0x06, 0xc1, 0x00, 0x86, 0xc0, 0x00, 0x46, 0xc0, 0x00, 0x2c, 0x60, 0x00,
   0x1c, 0x60, 0x00, 0x38, 0x38, 0x00, 0xf0, 0x1f, 0x00, 0xc0, 0x07, 0x00,
   0x00, 0x00, 0x00};

#define nomask_width 18
#define nomask_height 17
#define nomask_x_hot 8
#define nomask_y_hot 8

static unsigned char nomask_bits[] = {
   0xc0, 0x07, 0x00, 0xf0, 0x3f, 0x00, 0xf8, 0x7f, 0x00, 0xfc, 0xff, 0x00,
   0xfe, 0xff, 0x00, 0xfe, 0xff, 0x00, 0xff, 0xff, 0x01, 0xff, 0xff, 0x01,
   0xff, 0xff, 0x01, 0xff, 0xff, 0x01, 0xff, 0xff, 0x01, 0xfe, 0xff, 0x00,
   0xfe, 0xff, 0x00, 0xfc, 0x7f, 0x00, 0xf8, 0x3f, 0x00, 0xf0, 0x1f, 0x00,
   0xc0, 0x07, 0x00};

/*
 * OlGetNoCursor -
 */
extern Cursor
OlGetNoCursor OLARGLIST((screen))
	OLGRA( Screen *,	screen)
{
	extern Cursor _CreateCursorFromData OL_ARGS((
				Screen *, unsigned char *, unsigned char *,
				int, int, int, int
	));
	static Cursor	csr = (Cursor)0;

	if (csr == (Cursor)0)
	{
		csr = _CreateCursorFromData(
				screen,
				nocur_bits, nomask_bits,
				nocur_width, nocur_height,
				nocur_x_hot, nocur_y_hot
			);
	}

	return(csr);
} /* end of OlGetNoCursor */
