#ifndef ExmFiconboxP_H
#define ExmFiconboxP_H

#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FIconboxP.h	1.1"
#endif

/************************************************************************
 * Description:
 *	This is the flattened IconBox widget's private header file.
 */

#include "FIconBoxP.h"
#include "FIconbox.h"

/************************************************************************
 * Define structures and names/types needed to support flat widgets
 */
#define IBPART(w)		( ((ExmFlatIconboxWidget)(w))->icon_box )
#define IBITEM(i)		( ((ExmFlatIconboxItem)(i))->icon_box )

#ifndef XmRDimensionPolicy
#define XmRDimensionPolicy	"DimensionPolicy"
#endif

#ifndef XmRLabelPosition
#define XmRLabelPosition	"GlyphPosition"
#endif

/************************************************************************
 * Define Expanded Sub-object Instance Structure
 */
typedef struct {
	_XmString *	labels;		/* XmNlabels */
	Pixmap		pixmap;		/* XmNpixmap */
	Pixmap		mask;		/* XmNmask */
	int		depth;		/* XmNdepth */
	Dimension	width;		/* XmNglyphWidth */
	Dimension	height;		/* XmNglyphHeight */
} ExmFlatIconboxItemPart;

			/* Item's Full Instance record declaration	*/
typedef struct {
	ExmFlatItemPart		flat;
	ExmFlatGraphItemPart	graph;
	ExmFlatIconBoxItemPart	iconBox;
	ExmFlatIconboxItemPart	icon_box;
} ExmFlatIconboxItemRec, *ExmFlatIconboxItem;

/************************************************************************
 * Define Widget Instance Structure
 */
			/* Define new fields for the instance part	*/
typedef struct {
	Dimension *		max_col_wd;	/* private */
	Pixmap			label_insens;	/* private */
	Dimension		num_labels;	/* XmNlabelCount */
	Dimension		row_spacing;	/* XmNrowSpacing */
	Dimension		col_spacing;	/* XmNcolumnSpacing */
	Dimension		max_item_hi;	/* XmNmaxItemHeight */
	Dimension		max_item_wd;	/* XmNmaxItemWidth */
	unsigned char		orientation;	/* XmNorientation */
	unsigned char		label_pos;	/* XmNlabelPosition */
	unsigned char		dim_policy;	/* XmNdimensionPolicy */
	Boolean			use_obj_data;	/* XmNuseObjectData */
	Boolean			ib_debug;	/* XmNibDebug */
	Boolean			show_busy_label;/* XmNshowBusyLabel */
	Boolean			calc_x_uom;	/* private */
	Boolean			calc_y_uom;	/* private */
	Boolean			calc_max_hi;	/* private */
	Boolean			calc_max_wd;	/* private */
} ExmFlatIconboxPart;

			/* Full instance record declaration:
			 * 1. declare Widget "Part" Fields and then
			 * 2. declare Full Exmflat Item Record
			 */
typedef struct _ExmFlatIconboxRec {
	CorePart		core;
	XmPrimitivePart		primitive;
	ExmFlatPart		flat;
	ExmFlatGraphPart	graph;
	ExmFlatIconBoxPart	iconBox;
	ExmFlatIconboxPart	icon_box;

	ExmFlatIconboxItemRec	default_item;
} ExmFlatIconboxRec;

/************************************************************************
 * Define Widget Class Part and Class Rec
 */
				/* Define new fields for the class part	*/
typedef struct {
	char no_class_fields;	/* Makes compiler happy */
} ExmFlatIconboxClassPart;

				/* Full class record declaration 	*/

typedef struct _ExmFlatIconboxClassRec {
	CoreClassPart		core_class;
	XmPrimitiveClassPart	primitive_class;
	ExmFlatClassPart	flat_class;
	ExmFlatGraphClassPart	graph_class;
	ExmFlatIconBoxClassPart	iconBox_class;
	ExmFlatIconboxClassPart	icon_box_class;
} ExmFlatIconboxClassRec;

				/* External class record declaration	*/

extern ExmFlatIconboxClassRec		exmFlatIconboxClassRec;

#endif /* ExmFiconboxP_H */
