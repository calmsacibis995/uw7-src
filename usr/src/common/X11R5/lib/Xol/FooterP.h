#ifndef	NOIDENT
#ident	"@(#)footer:FooterP.h	1.3"
#endif

#ifndef _FOOTERP_H
#define _FOOTERP_H

#include "Xol/PrimitiveP.h"
#include "Xol/Footer.h"
#include "Xol/Olg.h"

/*
 * Footer dimensions, in points:
 *
 * FOOTER_MARGIN	Margin between left/right edges of window and
 *			left/right footer. Note that this does not include
 *			the space contributed by a spec-compliant Open
 *			Look window manager. When run with a different
 *			window manager, this space is missing--tough luck.
 *
 * FOOTER_INTERSPACE	One-half the space between the furthest-right
 *			extent of the left footer and the beginning of the
 *			right footer.
 *
 * FOOTER_HEIGHT	Height of footer area.
 *
 * FOOTER_MIN_GAP	The minimum gap to leave at the top and bottom of
 *			the footer. This is used only when the size of
 *			the font exceeds FOOTER_HEIGHT.
 *
 * FOOTER_BASELINE	The distance from the message's baseline to the
 *			bottom of the widget. If the font's descent is
 *			larger than this (plus the FOOTER_MIN_GAP), the
 *			font's descent is used.
 *
 * Default proportions for left and right parts, as arbitrary weights:
 *
 * DEFAULT_LEFT_WEIGHT
 * DEFAULT_RIGHT_WEIGHT
 */
#define FOOTER_MARGIN		10	/* Spec. says 12, but 2 by wmgr */
#define FOOTER_INTERSPACE 	4
#define FOOTER_HEIGHT		22
#define FOOTER_MIN_GAP		1
#define FOOTER_BASELINE		7
#define DEFAULT_LEFT_WEIGHT	3
#define DEFAULT_RIGHT_WEIGHT	1

/*
 * Class structure:
 */

typedef struct _FooterClassPart {
	XtPointer		extension;
}			FooterClassPart;

typedef struct _FooterClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	FooterClassPart		footer_class;
}			FooterClassRec;

extern FooterClassRec	footerClassRec;

/*
 * Instance structure:
 */

typedef struct _Foot {
	String			foot;
	Dimension		weight;
}			Foot;

typedef struct _FooterPart {
	/*
	 * Public:
	 */
	Foot			left;
	Foot			right;

	/*
	 * Private:
	 */
	GC			font_gc;
	OlgAttrs *		attrs;
}			FooterPart;

typedef struct _FooterRec {
	CorePart		core;
	PrimitivePart		primitive;
	FooterPart		footer;
}			FooterRec;

#define FOOTER_C(WC) ((FooterWidgetClass)(WC))->footer_class
#define FOOTER_P(W) ((FooterWidget)(W))->footer

#endif
