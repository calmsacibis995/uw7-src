
#ifndef _OL_FGRAPHP_H
#define _OL_FGRAPHP_H

#ifndef	NOIDENT
#ident	"@(#)flat:FGraphP.h	1.4"
#endif

/*
 ************************************************************************
 * Description:
 *	This is the flattened Graph widget's private header file.
 ************************************************************************
 */

#include <Xol/FlatP.h>
#include <Xol/FGraph.h>

/*
 ************************************************************************
 * Define Widget Class Part and Class Rec
 ************************************************************************
 */

				/* Define new fields for the class part	*/

typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} FlatGraphClassPart;

				/* Full class record declaration 	*/

typedef struct _FlatGraphClassRec {
    CoreClassPart	core_class;
    PrimitiveClassPart	primitive_class;
    FlatClassPart	flat_class;
    FlatGraphClassPart	graph_class;
} FlatGraphClassRec;

				/* External class record declaration	*/

extern FlatGraphClassRec		flatGraphClassRec;

/*
 ************************************************************************
 * Define Widget Instance Structure
 ************************************************************************
 */

				/* Define Expanded sub-object instance	*/

typedef struct {
	char	no_fields;
} FlatGraphItemPart;

			/* Item's Full Instance record declaration	*/

typedef struct {
	FlatItemPart		flat;
	FlatGraphItemPart	graph;
} FlatGraphItemRec, *FlatGraphItem;

typedef struct {
	Cardinal	item_index;
	Position	x;
	Position	y;
	Dimension	width;
	Dimension	height;
	unsigned char	flags;
#define OL_B_FG_SENSITIVE	(1<<0)	/* item is sensitive		*/
#define OL_B_FG_MANAGED		(1<<1)	/* item is managed		*/
#define OL_B_FG_MAPPED		(1<<2)	/* item is mapped when managed	*/
} OlFlatGraphInfo, *OlFlatGraphInfoList;

			/* Define new fields for the instance part	*/

typedef struct {
    Widget		vsbar;		/* vertical scrollbar of swin   */
    Widget		hsbar;		/* horizontaol scrollbar of swin*/
    Dimension		view_width;	/* view width			*/
    Dimension		view_height;	/* view height			*/
    OlFlatGraphInfoList	info;		/* item geometry & state info.
					 * This list represents stacking
					 * order!!!			*/
} FlatGraphPart;

			/* Full instance record declaration:
			 * 1. declare Widget "Part" Fields and then
			 * 2. declare Full flat Item Record		*/

typedef struct _FlatGraphRec {
    CorePart		core;
    PrimitivePart	primitive;
    FlatPart		flat;
    FlatGraphPart	graph;

    FlatGraphItemRec	default_item;
} FlatGraphRec;

extern void	OlFlatRaiseExpandedItem OL_ARGS((Widget, FlatItem, Boolean));

#endif /* _OL_FGRAPHP_H */
