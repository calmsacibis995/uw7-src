#ifndef NOIDENT
#ident	"@(#)primitive:Primitive.h	1.3"
#endif


#ifndef _OlPrimitive_h
#define _OlPrimitive_h


#include <X11/Core.h>


/* Class record constants */

extern WidgetClass	primitiveWidgetClass;

typedef struct _PrimitiveClassRec	*PrimitiveWidgetClass;
typedef struct _PrimitiveRec		*PrimitiveWidget;

#endif	/* _OlPrimitive_h */
