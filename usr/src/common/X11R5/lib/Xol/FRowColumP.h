#ifndef	NOIDENT
#ident	"@(#)flat:FRowColumP.h	1.2"
#endif

#ifndef _OL_FROWCOLUMP_H
#define _OL_FROWCOLUMP_H

/*
 ************************************************************************	
 * Description:
 *	This is the flat row/column widget's private header file.
 ************************************************************************	
 */

#include <Xol/FlatP.h>
#include <Xol/FRowColumn.h>
/*
 ************************************************************************	
 * Define Expanded Sub-object Instance Structure
 ************************************************************************	
 */

typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} FlatRowColumnItemPart;

			/* Item's Full Instance record declaration	*/

typedef struct {
	FlatItemPart		flat;
	FlatRowColumnItemPart	row_column;
} FlatRowColumnItemRec, *FlatRowColumnItem;

/*
 ************************************************************************	
 * Define Widget Instance Structure
 ************************************************************************	
 */
			/* Define new fields for the instance part	*/

typedef struct _FlatRowColumnPart {
			/* application resources for describing a generic
			 * row/column layout of the items.		*/

	Dimension	h_pad;		/* horizontal margin padding	*/
	Dimension	h_space;	/* internal horizontal padding	*/
	Dimension	item_max_height;/* item's maximum allow. height	*/
	Dimension	item_max_width;	/* item's maximum allow. width	*/
	Dimension	item_min_height;/* item's minimum allow. height	*/
	Dimension	item_min_width;	/* item's minimum allow. width	*/
	Dimension	overlap;	/* overlap of each item		*/
	Dimension	v_pad;		/* vertical margin padding	*/
	Dimension	v_space;	/* internal vertical padding	*/
	int		gravity;	/* gravity for sub-object group	*/
	int		item_gravity;	/* gravity for each sub-object	*/
	int		measure;	/* layout related dimension	*/
	OlDefine	layout_height;	/* height boundary constraint	*/
	OlDefine	layout_type;	/* type of desired layout	*/
	OlDefine	layout_width;	/* width boundary constraint	*/
	OlDefine	same_height;	/* forcing of sub-object height	*/
	OlDefine	same_width;	/* forcing of sub-object width	*/

			/* resources for holding calculated geometry
			 * information that describes the row/column
			 * configuration of the items.			*/

	Cardinal	rows;		/* number of rows		*/
	Cardinal	cols;		/* number of columns		*/
	Position	x_offset;	/* x coordinate of first item	*/
	Position	y_offset;	/* y coordinate of first item	*/
	Dimension	bounding_width;	/* width tightly bounding items	*/
	Dimension	bounding_height;/* height tightly bounding items*/
	Dimension *	col_widths;	/* array of column widths	*/
	Dimension *	row_heights;	/* array of row heights		*/
} FlatRowColumnPart;

			/* Full instance record declaration:
			 * 1. declare Widget "Part" Fields and then
			 * 2. declare Full flat Item Record		*/

typedef struct _FlatRowColumnRec {
    CorePart			core;
    PrimitivePart		primitive;
    FlatPart			flat;
    FlatRowColumnPart		row_column;

    FlatRowColumnItemRec	default_item;
} FlatRowColumnRec;

/*
 ************************************************************************	
 * Define Widget Class Part and Class Rec
 ************************************************************************	
 */

				/* Define new fields for the class part	*/

typedef struct _FlatRowColumnClassPart {
	int	no_class_fields;
} FlatRowColumnClassPart;

				/* Full class record declaration 	*/

typedef struct _FlatRowColumnClassRec {
    CoreClassPart		core_class;
    PrimitiveClassPart		primitive_class;
    FlatClassPart		flat_class;
    FlatRowColumnClassPart	row_column_class;
} FlatRowColumnClassRec;

				/* External class record declaration	*/

extern FlatRowColumnClassRec		flatRowColumnClassRec;

#endif /* _OL_FROWCOLUMP_H */
