#ifndef NOIDENT
#ident	"@(#)olg:Olg.h	1.29"
#endif

#ifndef _OLG_H_
#define _OLG_H_


typedef struct {
    unsigned char	width;	/* width of image */
    unsigned char	height;	/* height of image */
    unsigned char	*pData;	/* pointer to coordinate data */
} _OlgDesc, OlgDesc;

	/* an opque structure...	*/
typedef struct _OlgDevice	OlgDevice;

typedef union {
    Pixel	pixel;
    Pixmap	pixmap;
} OlgBG;

typedef struct _OlgAttrs {
    int		refCnt;		/* reference count */
    struct _OlgAttrs	*next;	/* pointer to next struct in list */

    Pixel	fg;		/* foreground pixel */
    OlgBG	bg;		/* background pixel or Pixmap */
    GC		bg0;		/* bg1, only lighter */
    GC		bg1;		/* User specified background */
    GC		bg2;		/* bg1, only a little darker */
    GC		bg3;		/* bg1, only much darker */
    OlgDevice	*pDev;		/* pointer to device specific data */
    unsigned char	flags;
} OlgAttrs;

typedef struct	{
    char	*label;
    GC		normalGC;
    GC		inverseGC;
    XFontStruct	*font;
    char	*accelerator;
    char	mnemonic;
    unsigned char	justification;
    unsigned char	flags;
    OlFontList		*font_list;
    Pixel	stippleColor;
} OlgTextLbl;

typedef struct {
    union {
	Pixmap	pixmap;
	XImage	*image;
    } label;
    GC		normalGC;
    Pixel	stippleColor;
    unsigned char	justification;
    unsigned char	type;
    unsigned char	flags;
} OlgPixmapLbl;    

extern	unsigned	_olgIs3d;

/* attributes defines */
#define OLG_BGPIXMAP	1
#define OLG_ALLOCBG0	2
#define OLG_ALLOCBG1	4
#define OLG_ALLOCBG2	8
#define OLG_ALLOCBG3	16

/* Scrollbar defines */
#define SB_POSITION	1
#define SB_BEGIN_ANCHOR	2
#define SB_END_ANCHOR	4
#define SB_PREV_ARROW	8
#define SB_NEXT_ARROW	16
#define SB_DRAG		32

/* Slider defines */
#define SL_POSITION	1
#define SL_DRAG		2
#define SL_BEGIN_ANCHOR	4
#define SL_END_ANCHOR	8
#define SL_BORDER	16
#define SL_NOBORDER	32

/* Oblong button defines */
#define OB_SELECTED	1
#define OB_DEFAULT	2
#define OB_BUSY		4
#define OB_INSENSITIVE	8
#define OB_MENUITEM	16
#define OB_MENUMARK	32
#define OB_MENU_R	OB_MENUMARK
#define OB_MENU_D	(OB_MENUMARK | 64)
#define OB_FILLONSELECT	128

/* Rectangular button defines */
#define RB_SELECTED	1
#define RB_DEFAULT	2
#define RB_DIM		4
#define RB_INSENSITIVE	8
#define RB_NOFRAME	16

/* Label defines */
#define TL_LEFT_JUSTIFY		0
#define TL_CENTER_JUSTIFY	1
#define TL_RIGHT_JUSTIFY 	2
#define TL_SELECTED		1
#define TL_POPUP		2
#define TL_INSENSITIVE		4

/* Image label defines */
#define PL_IMAGE	0
#define PL_PIXMAP	1
#define PL_BITMAP	2
#define PL_TILED	1
#define PL_INSENSITIVE	2

/* Abbreviated menu button defines */
#define AM_NORMAL	1
#define AM_SELECTED	2
#define AM_WINDOW	4

/* Menu mark defines */
#define MM_DOWN		1
#define MM_CENTER	2
#define MM_INVERT	4
#define MM_JUST_SIZE	8
#define MM_POPUP	16

/* Pushpin defines */
#define PP_OUT		0
#define PP_IN		1
#define PP_DEFAULT	2

/* Checkbox defines */
#define	CB_CHECKED	1
#define CB_DIM		4	/* consistent with rect buttons */
#define CB_DIAMOND	32	/* so rectbtns can use it, too	*/
#define CB_FILLONSELECT	64	/* new moolit resource		*/

/* Rounded box defines */
#define RB_UP		1
#define RB_OMIT_LCAP	2
#define RB_OMIT_BCAP	4
#define RB_MONO		8

/* Filled Rounded box defines */
#define FB_UP		1
#define FB_BUSY		2
#define FB_OMIT_RCAP	4
#define FB_OMIT_TCAP	8
#define FB_OMIT_LCAP	16
#define FB_OMIT_BCAP	32

/* Shadowed arrow definds */
#define AR_PRESSED	1
#define AR_UP		2
#define AR_DOWN		4
#define AR_RIGHT	8
#define AR_LEFT		16

/* Bitmap type defines */
#define OLG_PUSHPIN_2D	0
#define OLG_PUSHPIN_3D	1
#define OLG_CHECKS	2

/* Miscellaneous Macros */
#define OlgGetBg1GC(p)	((p) ? (p)->bg1 : (GC) 0)
#define OlgGetBg2GC(p)	((p) ? (p)->bg2 : (GC) 0)
#define OlgGetBg3GC(p)	((p) ? (p)->bg3 : (GC) 0)
#define OlgGetFgGC(p)	((p) ? (p)->bg2 : (GC) 0)
#define OlgGetBrightGC(p)	((p) ? (p)->bg0 : (GC) 0)
#define OlgGetDarkGC(p)	((p) ? (p)->pDev->blackGC : (GC) 0)
#define OlgGetScratchGC(p)	((p) ? (p)->pDev->scratchGC : (GC) 0)
#define OlgGetHorizontalStroke(p)	((p) ? (p)->pDev->horizontalStroke:0)
#define OlgGetVerticalStroke(p)	((p) ? (p)->pDev->verticalStroke : 0)
#define OlgGetInactiveStipple(p)	((p) ? (p)->pDev->inactiveStipple : (Pixmap) 0)
#define OlgGetScreen(p)	((p) ? (p)->pDev->scr : (Screen *) 0)

#define OlgIs3d()	_olgIs3d

typedef void (*OlgLabelProc) OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension, XtPointer));

typedef void (*OlgSizeButtonProc) OL_ARGS((
		Screen *, OlgAttrs *, XtPointer, Dimension *, Dimension *));


OLBeginFunctionPrototypeBlock

extern OlgAttrs *OlgCreateAttrs OL_ARGS((
		Screen *, Pixel, OlgBG *, Boolean, Dimension));

extern void OlgDestroyAttrs OL_ARGS((OlgAttrs *));

extern void OlgSetStyle3D OL_ARGS((Boolean));

extern void OlgSizeScrollbarElevator OL_ARGS((
		Widget, OlgAttrs *, OlDefine, Dimension *, Dimension *));

extern void OlgSizeScrollbarAnchor OL_ARGS((
		Widget, OlgAttrs *, Dimension *, Dimension *));

extern void OlgUpdateScrollbar OL_ARGS((Widget, OlgAttrs *, OlBitMask));

extern void OlgDrawScrollbar OL_ARGS((Widget, OlgAttrs *));

extern void OlgDrawAnchor OL_ARGS((
		Screen *, Drawable, OlgAttrs *, Position, Position,
		Dimension, Dimension, Boolean));

extern void OlgDrawAbbrevMenuB OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, OlBitMask));

extern void OlgDrawSlider OL_ARGS((Widget, OlgAttrs *));

extern void OlgUpdateSlider OL_ARGS((Widget, OlgAttrs *, OlBitMask));

extern void OlgDrawPushPin OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension, OlDefine));

extern void OlgDrawChiseledBox OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension));

extern void OlgDrawBox OL_ARGS((
		Screen *, Drawable, OlgAttrs *, Position, Position,
		Dimension, Dimension, Boolean));

extern void OlgDrawRBox OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension,
		OlgDesc *, OlgDesc *, OlgDesc *, OlgDesc *, OlBitMask));

extern void OlgDrawFilledRBox OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension,
		OlgDesc *, OlgDesc *, OlgDesc *, OlgDesc *, OlBitMask));

extern void OlgSizeSlider OL_ARGS((
		Widget, OlgAttrs *, Dimension *, Dimension *));

extern void OlgSizeSliderAnchor OL_ARGS((
		Widget, OlgAttrs *, Dimension *, Dimension *));

extern void OlgSizeSliderElevator OL_ARGS((
		Widget, OlgAttrs *, Dimension *, Dimension *));

extern void OlgSizeOblongButton OL_ARGS((
		Screen *, OlgAttrs *, XtPointer, OlgSizeButtonProc,
		OlBitMask, Dimension *, Dimension *));

extern void OlgSizeRectButton OL_ARGS((
		Screen *, OlgAttrs *, XtPointer, OlgSizeButtonProc,
		OlBitMask, Dimension *, Dimension *));

extern void OlgSizeAbbrevMenuB OL_ARGS((
		Screen *, OlgAttrs *, Dimension *, Dimension *));

extern void OlgDrawTextLabel OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension, OlgTextLbl *));

extern void OlgSizeTextLabel OL_ARGS((
		Screen *, OlgAttrs *, OlgTextLbl *, Dimension *, Dimension *));

extern void OlgDrawPixmapLabel OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension, OlgPixmapLbl *));

extern void OlgSizePixmapLabel OL_ARGS((
		Screen *, OlgAttrs *, OlgPixmapLbl *,
		Dimension *, Dimension *));

extern void OlgDrawBorderShadow OL_ARGS((
		Screen *, Window, OlgAttrs *, OlDefine, Dimension,
		Position, Position, Dimension, Dimension));

extern void OlgDrawCheckBox OL_ARGS((
		Screen *, Drawable, OlgAttrs *, Position, Position, OlBitMask));

extern void OlgDrawArrow OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension, OlBitMask));

extern void OlgDrawRectButton OL_ARGS((
		Screen *, Drawable, OlgAttrs *, Position, Position,
		Dimension, Dimension, XtPointer, OlgLabelProc, OlBitMask));

extern void OlgDrawOblongButton OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension,
		XtPointer, OlgLabelProc, OlBitMask));

extern Dimension OlgDrawMenuMark OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension, OlBitMask));

extern void OlgDrawWindowMark OL_ARGS((
		Screen *, Drawable, OlgAttrs *,
		Position, Position, Dimension, Dimension, OlBitMask));

extern void OlgGetColors OL_ARGS((
		Screen *, Pixel, XColor *, XColor *, XColor *));

OLEndFunctionPrototypeBlock

#endif /* _OLG_H_ */
