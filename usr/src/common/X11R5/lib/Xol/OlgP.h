#ifndef NOIDENT
#ident	"@(#)olg:OlgP.h	1.6"
#endif

#ifndef _OLG_P_H_
#define _OLG_P_H_

#include <Xol/Olg.h>

typedef struct _OlgDevice {
    Screen	*scr;		/* display structure ptr */

    GC		whiteGC;	/* white pixel of screen */
    GC		blackGC;	/* black pixel of screen */
    GC		grayGC;		/* gray pixel (solid) */
    GC		busyGC;		/* black with 15% stipple */
    GC		lightGrayGC;	/* white with 50% stipple */
    GC		dimGrayGC;	/* black with 50% stipple */
    GC		scratchGC;	/* a GC for others to play with */

    unsigned	horizontalStroke;	/* width of horizontal stroke */
    unsigned	verticalStroke;		/* width of vertical stroke */

    char	*descPath;		/* base name of description files */

    Pixmap	busyStipple;
    Pixmap	inactiveStipple;

    /* Push Pin pixmaps */
    Pixmap	pushpin2D;
    Dimension	widthPushpin2D;
    Dimension	heightPushpin2D;

    Pixmap	pushpin3D;
    Dimension	widthPushpin3D;
    Dimension	heightPushpin3D;

    /* check box pixmaps */
    Pixmap	checks;
    Dimension	widthChecks;
    Dimension	heightChecks;
    Dimension	widthCheckBox;
    Dimension	heightCheckBox;

    /* miscellaneous arrows */
    _OlgDesc	arrowUp;
    _OlgDesc	arrowDown;
    _OlgDesc	arrowLeft;
    _OlgDesc	arrowRight;
    _OlgDesc	arrowText;

    /* Corner for oblong buttons */
    _OlgDesc	oblongB2UL;	/* 2-D corners */
    _OlgDesc	oblongB2UR;
    _OlgDesc	oblongB2LL;
    _OlgDesc	oblongB2LR;
    _OlgDesc	oblongB3UL;	/* 3-D corners */
    _OlgDesc	oblongB3UR;
    _OlgDesc	oblongB3LL;
    _OlgDesc	oblongB3LR;
    _OlgDesc	oblongDefUL;	/* default ring (2- and 3-D) */
    _OlgDesc	oblongDefUR;
    _OlgDesc	oblongDefLL;
    _OlgDesc	oblongDefLR;

    /* Small-radius rounded Corners for more rectangular things like
     * the abbreviated menu button
     */
    _OlgDesc	rect2UL;	/* 2-D */
    _OlgDesc	rect2UR;
    _OlgDesc	rect2LL;
    _OlgDesc	rect2LR;
    _OlgDesc	rect3UL;	/* 3-D */
    _OlgDesc	rect3UR;
    _OlgDesc	rect3LL;
    _OlgDesc	rect3LR;

    _OlgDesc	sliderVUL;	/* vertical slider */
    _OlgDesc	sliderVUR;
    _OlgDesc	sliderVLL;
    _OlgDesc	sliderVLR;
    _OlgDesc	sliderHUL;	/* horizontal slider */
    _OlgDesc	sliderHUR;
    _OlgDesc	sliderHLL;
    _OlgDesc	sliderHLR;

    /* metrics used to determine the sizes of various elements */

    char	lblOrigY;	/* amount from top of oblong default ring
				 * to offset label.
				 */
    char	lblCornerY;	/* amount from bottom of oblong default ring
				 * to offset label.
				 */

    char	rect2OrigX;	/* offsets of interior of small ring (2-D) */
    char	rect2OrigY;
    char	rect2CornerX;
    char	rect2CornerY;

    char	rect3OrigX;	/* offsets of interior of small ring (3-D) */
    char	rect3OrigY;
    char	rect3CornerX;
    char	rect3CornerY;

    unsigned char	downMenuMWidth;	/* down menu mark dimensions */
    unsigned char	downMenuMHeight;
    unsigned char	rightMenuMWidth;	/* right menu mark */
    unsigned char	rightMenuMHeight;
    unsigned char	menuMPad;	/* space between menu mark and lbl */

    struct _OlgDevice	*next;	/* ptr to next device struct */
    char	scale;		/* rendering size */

    /* MooLIT extension */
    int		shadow_thickness;	/*  for Motif Mode only */
    _OlgDesc	arrowUpCenter;		/* shadowed up arrow */
    _OlgDesc	arrowUpDark;
    _OlgDesc	arrowUpBright;
    _OlgDesc	arrowDownCenter;	/*  shadowed down arrow */
    _OlgDesc	arrowDownDark;
    _OlgDesc	arrowDownBright;
    _OlgDesc	arrowRightCenter;	/*  shadowed right arrow */
    _OlgDesc	arrowRightDark;
    _OlgDesc	arrowRightBright;
    _OlgDesc	arrowRightVert;
    _OlgDesc	arrowLeftCenter;	/*  shadowed left arrow */
    _OlgDesc	arrowLeftDark;
    _OlgDesc	arrowLeftBright;
    _OlgDesc	arrowLeftVert;
} _OlgDevice;

OLBeginFunctionPrototypeBlock

extern void (*_olmOlgSizeScrollbarElevator) OL_ARGS((
	Widget, OlgAttrs *, OlDefine, Dimension *, Dimension *));
extern void _OloOlgSizeScrollbarElevator OL_ARGS((
	Widget, OlgAttrs *, OlDefine, Dimension *, Dimension *));
extern void _OlmOlgSizeScrollbarElevator OL_ARGS((
	Widget, OlgAttrs *, OlDefine, Dimension *, Dimension *));

extern void (*_olmOlgSizeScrollbarAnchor) OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension *));
extern void _OlmOlgSizeScrollbarAnchor OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension *));
extern void _OloOlgSizeScrollbarAnchor OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension *));

extern void (*_olmOlgUpdateScrollbar) OL_ARGS((Widget, OlgAttrs *, OlBitMask));
extern void _OlmOlgUpdateScrollbar OL_ARGS((Widget, OlgAttrs *, OlBitMask));
extern void _OloOlgUpdateScrollbar OL_ARGS((Widget, OlgAttrs *, OlBitMask));

extern void (*_olmOlgDrawScrollbar) OL_ARGS((Widget, OlgAttrs *));
extern void _OlmOlgDrawScrollbar OL_ARGS((Widget, OlgAttrs *));
extern void _OloOlgDrawScrollbar OL_ARGS((Widget, OlgAttrs *));

extern void (*_olmOlgDrawSlider) OL_ARGS((Widget, OlgAttrs *));
extern void _OlmOlgDrawSlider OL_ARGS((Widget, OlgAttrs *));
extern void _OloOlgDrawSlider OL_ARGS((Widget, OlgAttrs *));

extern void (*_olmOlgUpdateSlider) OL_ARGS((Widget,OlgAttrs *, OlBitMask));
extern void _OlmOlgUpdateSlider OL_ARGS((Widget, OlgAttrs *, OlBitMask));
extern void _OloOlgUpdateSlider OL_ARGS((Widget, OlgAttrs *, OlBitMask));

extern void (*_olmOlgSizeSliderElevator) OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension * ));
extern void _OlmOlgSizeSliderElevator OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension * ));
extern void _OloOlgSizeSliderElevator OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension * ));

extern void (*_olmOlgSizeSlider) OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension * ));
extern void _OlmOlgSizeSlider OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension * ));
extern void _OloOlgSizeSlider OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension * ));

extern void (*_olmOlgDrawAbbrevMenuB) OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, OlBitMask));
extern void _OlmOlgDrawAbbrevMenuB OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, OlBitMask));
extern void _OloOlgDrawAbbrevMenuB OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, OlBitMask));

extern void (*_olmOlgDrawCheckBox) OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, OlBitMask));
extern void _OlmOlgDrawCheckBox OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, OlBitMask));
extern void _OloOlgDrawCheckBox OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, OlBitMask));

extern void (*_olmOlgDrawOblongButton) OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, Dimension,
	Dimension, XtPointer, OlgLabelProc, OlBitMask));
extern void _OlmOlgDrawOblongButton OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, Dimension,
	Dimension, XtPointer, OlgLabelProc, OlBitMask));
extern void _OloOlgDrawOblongButton OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, Dimension,
	Dimension, XtPointer, OlgLabelProc, OlBitMask));

extern Dimension (*_olmOlgDrawMenuMark) OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, Dimension,
	Dimension, OlBitMask));
extern Dimension _OlmOlgDrawMenuMark OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, Dimension,
	Dimension, OlBitMask));
extern Dimension _OloOlgDrawMenuMark OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, Dimension,
	Dimension, OlBitMask));

extern void		_OlgCBsizeBox OL_ARGS((_OlgDevice *));
extern void		_OlgDrawRectButton OL_ARGS((Screen *, Drawable,
					OlgAttrs *, Position, Position,
					Dimension, Dimension, XtPointer,
					OlgLabelProc, OlBitMask,
					Dimension, Dimension));
extern void		_OlgOBdownMMDimensions OL_ARGS((_OlgDevice *));
extern void		_OlgOBrightMMDimensions OL_ARGS((_OlgDevice *));
extern _OlgDevice *	_OlgGetDeviceData OL_ARGS((Screen *, Dimension));
extern void		_OlgGetBitmaps OL_ARGS((Screen *, OlgAttrs *,OlDefine));
extern void		_OlgDrawBorderShadow OL_ARGS((Screen *, Window,
					OlgAttrs *, OlDefine, Dimension,
					Position, Position, Dimension,
					Dimension, GC, GC));

OLEndFunctionPrototypeBlock

#endif /* _OLG_P_H_ */
