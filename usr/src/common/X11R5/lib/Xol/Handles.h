#ifndef	NOIDENT
#ident	"@(#)handles:Handles.h	1.3"
#endif

#ifndef _HANDLES_H
#define _HANDLES_H

#include "Xol/EventObj.h"

extern WidgetClass			handlesWidgetClass;

typedef struct _HandlesClassRec *	HandlesWidgetClass;
typedef struct _HandlesRec *		HandlesWidget;

typedef enum OlHandlesPaneResizeReason {
	OlHandlesPaneResizing,
	OlHandlesPaneResized
}			OlHandlesPaneResizeReason;

typedef struct OlHandlesPaneResizeData {
	OlHandlesPaneResizeReason	reason;
	Widget				pane;
	XtWidgetGeometry *		geometry;
}			OlHandlesPaneResizeData;

#endif
