#ifndef	NOIDENT
#ident	"@(#)slider:Slider.h	1.4"
#endif

#ifndef _Slider_h
#define _Slider_h

#include <Xol/Primitive.h>		/* include superclasses' header */

/***********************************************************************
 *
 * Slider Widget (subclass of CompositeClass)
 *
 ***********************************************************************/


/* Class record constants */

extern WidgetClass sliderWidgetClass;


typedef struct _SliderClassRec *SliderWidgetClass;
typedef struct _SliderRec      *SliderWidget;

typedef struct OlSliderVerify {
	int	new_location;
	Boolean	more_cb_pending;
} OlSliderVerify;

#endif /* _Slider_h */
