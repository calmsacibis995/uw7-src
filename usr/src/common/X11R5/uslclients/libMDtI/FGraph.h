#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FGraph.h	1.5"
#endif

#ifndef ExmFGRAPH_H
#define ExmFGRAPH_H

/************************************************************************
 * Description:
 *	This is the flattened Graph widget's public header file.
 */

#include "Flat.h"	/* include superclass header */

/************************************************************************
 * Define class and instance pointers:
 *	- extern pointer to class data/procedures
 *	- typedef pointer to widget's class structure
 *	- typedef pointer to widget's instance structure
 */

extern WidgetClass			exmFlatGraphWidgetClass;
typedef struct _ExmFlatGraphClassRec *	ExmFlatGraphWidgetClass;
typedef struct _ExmFlatGraphRec *	ExmFlatGraphWidget;

#define XmNgridColumns		(_XmConst char *)"gridColumns"
#define XmCGridColumns		(_XmConst char *)"GridColumns"
#define XmNgridRows		(_XmConst char *)"gridRows"
#define XmCGridRows		(_XmConst char *)"GridRows"
#define XmNgridWidth		(_XmConst char *)"gridWidth"
#define XmCGridWidth		(_XmConst char *)"GridWidth"
#define XmNgridHeight		(_XmConst char *)"gridHeight"
#define XmCGridHeight		(_XmConst char *)"GridHeight"
#define XmNnormalizePosition	(_XmConst char *)"normalizePosition"
#define XmCNormalizePosition	(_XmConst char *)"NormalizePosition"

extern void	ExmFlatRaiseItem(Widget, Cardinal, Boolean);

#endif /* ExmFGRAPH_H */
