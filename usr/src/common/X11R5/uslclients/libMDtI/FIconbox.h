#ifndef ExmFiconbox_H
#define ExmFiconbox_H

#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FIconbox.h	1.1"
#endif


/************************************************************************
 * Description:
 *	This is the flattened Iconbox widget's public header file.
 */

#include "FIconBox.h"

/************************************************************************
 **Describe available container resources in this widget
 *
 * XmNcolumnSpacing (XmCSpacing) - specify horizontal spacing between
 *					items contained within the widget.
 * XmNdimensionPolicy (XmCDimensionPolicy) - specify whether this widget
 *					should do geometry calulation.
 * XmNibDebug (XmCDebug) -
 * XmNlabelCount (XmCLabelCount) -
 * XmNlabelPosition (XmCLabelPositiion) - specify the relation between label
 *					and glyph, the valid values can be
 *					XmBOTTOM_LABEL (default),
 *					XmTOP_LABEL,
 *					XmLEFT_LABEL,
 *					XmRIGHT_LABEL,
 *					XmLEFT_LABELS,
 *					XmRIGHT_LABELS
 * XmNmaxItemHeight (XmCMaxItemHeight) -
 * XmNmaxItemWidth (XmCMaxItemWidth) -
 * XmNorientation (XmCOrientation) - Determine whether the layouts are
 *					displayed horizontally (XmHORIZONTAL)
 *					or vertically (XmVERTICAL)
 *					from item 0...n. The default is
 *					XmHORIZONTAL.
 * XmNrowSpacing (XmCSpacing) - specify vertical spacing between
 *					items contained within the widget.
 * XmNshowBusyLabel (XmCShowBusyLabel) -
 * XmNuseObjectData (XmCUseObjectData) - specify whether the glyph info
 *					are from XmNobjectData or from
 *					item resources of FIconbox.
 *
 **Describe available item resources in this widget
 * XmNglyphDepth (XmCGlyphDepth) -
 * XmNglyphHeight (XmCGlyphHeight) -
 * XmNglyphWidth (XmCGlyphWidth) -
 * XmNmask (XmCPixmap) -
 * XmNpixmap (XmCPixmap) -
 * XmNlabels (XmClabels) -
 */
/************************************************************************
 * Define class and instance pointers:
 *	- extern pointer to class data/procedures
 *	- typedef pointer to widget's class structure
 *	- typedef pointer to widget's instance structure
 */

extern WidgetClass				exmFlatIconboxWidgetClass;
typedef struct _ExmFlatIconboxClassRec *	ExmFlatIconboxWidgetClass;
typedef struct _ExmFlatIconboxRec *		ExmFlatIconboxWidget;

/************************************************************************
 * Define FlatIconbox resource strings
 */
	/* CONTAINER... */
#ifndef XmNcolumnSpacing
#define XmNcolumnSpacing	"columnSpacing"
#endif

#ifndef XmNdimensionPolicy
#define XmNdimensionPolicy	"dimensionPolicy"
#endif

#ifndef XmNibDebug
#define XmNibDebug		"ibDebug"
#endif

#ifndef XmNlabelCount
#define XmNlabelCount		"labelCount"
#endif

#ifndef XmNlabelPosition
#define XmNlabelPosition	"labelPosition"
#endif

#ifndef XmNmaxItemHeight
#define XmNmaxItemHeight	"maxItemHeight"
#endif

#ifndef XmNmaxItemWidth
#define XmNmaxItemWidth		"maxItemWidth"
#endif

#ifndef XmNrowSpacing
#define XmNrowSpacing		"rowSpacing"
#endif

#ifndef XmNshowBusyLabel
#define XmNshowBusyLabel	"showBusyLabel"
#endif

#ifndef XmNuseObjectData
#define XmNuseObjectData	"useObjectData"
#endif

#ifndef XmCDebug
#define XmCDebug		"Debug"
#endif

#ifndef XmCDimensionPolicy
#define XmCDimensionPolicy	"DimensionPolicy"
#endif

#ifndef XmCLabelCount
#define XmCLabelCount		"LabelCount"
#endif

#ifndef XmCLabelPosition
#define XmCLabelPosition	"LabelPosition"
#endif

#ifndef XmCMaxItemHeight
#define XmCMaxItemHeight	"MaxItemHeight"
#endif

#ifndef XmCMaxItemWidth
#define XmCMaxItemWidth		"MaxItemWidth"
#endif

#ifndef XmCShowBusyLabel
#define XmCShowBusyLabel	"ShowBusyLabel"
#endif

#ifndef XmCUseObjectData
#define XmCUseObjectData	"UseObjectData"
#endif

	/* Item resources */
#ifndef XmNglyphDepth
#define XmNglyphDepth		"glyphDepth"
#endif

#ifndef XmNglyphHeight
#define XmNglyphHeight		"glyphHeight"
#endif

#ifndef XmNglyphWidth
#define XmNglyphWidth		"glyphWidth"
#endif

#ifndef XmNlabels
#define XmNlabels		"labels"
#endif

#ifndef XmCGlyphDepth
#define XmCGlyphDepth		"GlyphDepth"
#endif

#ifndef XmCGlyphHeight
#define XmCGlyphHeight		"GlyphHeight"
#endif

#ifndef XmCGlyphWidth
#define XmCGlyphWidth		"GlyphWidth"
#endif

#ifndef XmCLabels
#define XmCLabels		"Labels"
#endif

/************************************************************************
 * Declare structure used with FIconbox widget
 */

	/* Possible values for XmNglyphPosition */
enum {
	XmBOTTOM_LABEL,		/* default */
	XmTOP_LABEL,
	XmLEFT_LABEL,
	XmRIGHT_LABEL,
	XmLEFT_LABELS,
	XmRIGHT_LABELS
};

#ifdef just_for_reference
	/* Possible values for XmNdimensionPolicy */
	/* Already defined in libXm */
enum {
	XmAUTOMATIC,			/* default */
	XmAPPLICATION_DEFINED,
};
#endif

	/* Note that this data structure is identical with (i.e., size wide)
	 * DmGlyphRec in dtm, except that:
	 *		the first field, client_data vs path, and
	 * 		the 4th field, busy vs insensitive.
	 * This means apps can use DmGetPixmap/DmMaskPixmap calls to
	 * initialize this glyph data.
	 *
	 * Apps should enable item_resource, XmNobjectData, if he/she
	 * want to display `glyph' as part of the item.
	 *
	 * Apps shall at least supply with pixmap/width/height/depth
	 * otherwise, the glyph part won't be displayed.
	 *
	 * Apps are responsibile for freeing pixmap/mask/busy when
	 * they are no longer needed (e.g., after the widget is destroyed).
	 */
typedef struct {
	XtPointer	client_data;	/* client data that widget won't look */
	Pixmap		pixmap;		/* glyph, dft = None */
	Pixmap		mask;		/* mask,  dft = None */
	Dimension	width;		/* width/height of pixmap/mask/busy */
	Dimension	height;
	short		depth;		/* depth of pixmap, no default */
	short		count;		/* usage count */
} ExmFIconboxGlyphData;

/************************************************************************
 * Declare external functions/procedures that application can use.
 */

#if defined(__cplusplus) || defined(c_plusplus)
#define ExmBeginFunctionPrototypeBlock  extern "C" {
#define ExmEndFunctionPrototypeBlock    }
#else /* defined(__cplusplus) || defined(c_plusplus) */
#define ExmBeginFunctionPrototypeBlock
#define ExmEndFunctionPrototypeBlock
#endif /* defined(__cplusplus) || defined(c_plusplus) */

ExmBeginFunctionPrototypeBlock

extern _XmString
ExmXmStringCreate(
	String		/* string -		*/
);

extern void
ExmXmStringFree(
	_XmString	/* string -		*/
);

ExmEndFunctionPrototypeBlock

#endif /* ExmFiconbox_H */
