#ifndef	NOIDENT
#ident	"@(#)arrow:Arrow.h	1.6"
#endif

/*
 *  Arrow.h - Public Definitions for Arrow widget (used by Scroll Widget)
 *
 */

#ifndef _XtArrow_h
#define _XtArrow_h

#include <Xol/Primitive.h>		/* include superclasses' header */

/***************************************************************************
 *
 * Arrow Widget 
 *
 **************************************************************************/

/* Parameters:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 background	     Background		pixel		Black
 border		     BorderColor	pixel		Black
 borderWidth	     BorderWidth	int		0
 btnDown	     Callback		Pointer		NULL
 btnMotion	     Callback		Pointer		NULL
 btnUp		     Callback		Pointer		NULL
 destroyCallback     Callback		Pointer		NULL
 height		     Height		int		6
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 sensitive	     Sensitive		Boolean		True
 width		     Width		int		6
 x		     Position		int		0
 y		     Position		int		0

*/

typedef struct {
  XEvent *event;		/* the event causing the ArrowAction */
  String *params;		/* the TranslationTable params */
  Cardinal num_params;		/* count of params */
} ArrowCallDataRec, *ArrowCallData;

/* Class Record Constant */

extern WidgetClass arrowWidgetClass;

typedef struct _ArrowClassRec *ArrowWidgetClass;
typedef struct _ArrowRec      *ArrowWidget;

#endif /* _XtArrow_h */
