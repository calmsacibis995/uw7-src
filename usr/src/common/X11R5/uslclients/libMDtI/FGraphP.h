#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FGraphP.h	1.3"
#endif

#ifndef ExmFGRAPHP_H
#define ExmFGRAPHP_H

/************************************************************************
 * Description:
 *	This is the flattened Graph widget's private header file.
 */

#include "FlatP.h"		/* include superclass header */
#include "FGraph.h"		/* include public header */

/************************************************************************
 * Define structures and names/types needed to support flat widgets
 */
#define GPART(w)	( ((ExmFlatGraphWidget)(w))->graph )
#define GITEM(i)	( ((ExmFlatGraphItem)(i))->graph )

typedef struct {
	Cardinal	item_index;
	WidePosition	x;
	WidePosition	y;
	Dimension	width;
	Dimension	height;
	unsigned char	flags;
#define ExmB_FG_SENSITIVE	( 1 << 0 )	/* item is sensitive	*/
#define ExmB_FG_MANAGED		( 1 << 1 )	/* item is managed	*/
#define ExmB_FG_MAPPED		( 1 << 2 )	/* mapped when managed	*/
} ExmFlatGraphInfo, *ExmFlatGraphInfoList;

/************************************************************************
 * Define Expanded Sub-object Instance Structure
 */
typedef struct {
    char	no_fields;
} ExmFlatGraphItemPart;

			/* Item's Full Instance record declaration	*/
typedef struct {
    ExmFlatItemPart		flat;
    ExmFlatGraphItemPart	graph;
} ExmFlatGraphItemRec, *ExmFlatGraphItem;

/************************************************************************
 * Define Widget Instance Structure
 */
			/* Define new fields for the instance part	*/
typedef struct _ExmFlatGraphPart {
    ExmFlatGraphInfoList info;		/* item geometry & state info: This
					 * list represents stacking order!!!
					 */
    Boolean		 normalize_pos;	/* XmNnormalizePosition		*/
} ExmFlatGraphPart;

			/* Full instance record declaration:
			 * 1. declare Widget "Part" Fields and then
			 * 2. declare Full flat Item Record
			 */
typedef struct _ExmFlatGraphRec {
    CorePart		core;
    XmPrimitivePart	primitive;
    ExmFlatPart		flat;
    ExmFlatGraphPart	graph;

    ExmFlatGraphItemRec	default_item;
} ExmFlatGraphRec;

/************************************************************************
 * Define Widget Class Part and Class Rec
 */
				/* Define new fields for the class part	*/
typedef struct {
    Boolean check_uoms;
} ExmFlatGraphClassPart;

				/* Full class record declaration 	*/
typedef struct _ExmFlatGraphClassRec {
    CoreClassPart		core_class;
    XmPrimitiveClassPart	primitive_class;
    ExmFlatClassPart		flat_class;
    ExmFlatGraphClassPart	graph_class;
} ExmFlatGraphClassRec;

				/* External class record declaration	*/

extern ExmFlatGraphClassRec		exmFlatGraphClassRec;

extern void	ExmFlatRaiseExpandedItem (Widget, ExmFlatItem, Boolean);

#endif /* ExmFGRAPHP_H */
