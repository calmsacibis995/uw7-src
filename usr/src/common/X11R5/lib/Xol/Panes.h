#ifndef	NOIDENT
#ident	"@(#)panes:Panes.h	1.7"
#endif

#ifndef _PANES_H
#define _PANES_H

#include "Xol/Manager.h"

extern WidgetClass			panesWidgetClass;

typedef struct _PanesClassRec *		PanesWidgetClass;
typedef struct _PanesRec *		PanesWidget;
typedef struct _PanesConstraintRec *	PanesConstraints;

typedef struct OlQueryGeometryCallData {
	Widget			w;
	XtWidgetGeometry *	suggested;
	XtWidgetGeometry *	preferred;
	XtGeometryResult	result;
}			OlQueryGeometryCallData;

#endif
