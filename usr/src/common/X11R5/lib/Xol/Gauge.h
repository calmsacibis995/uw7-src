#ifndef	NOIDENT
#ident	"@(#)slider:Gauge.h	1.1"
#endif

#ifndef _Gauge_h
#define _Gauge_h

#include <Xol/Primitive.h>		/* include superclasses' header */

/***********************************************************************
 *
 * Gauge Widget (subclass of PrimitiveClass)
 *
 ***********************************************************************/


/* Class record constants */

extern WidgetClass gaugeWidgetClass;


typedef struct _SliderClassRec *GaugeWidgetClass;
typedef struct _SliderRec      *GaugeWidget;

/* extern functions */
extern void OlSetGaugeValue();
/*
 * Widget sw;
 * int    val;
 */

#endif /* _Gauge_h */
