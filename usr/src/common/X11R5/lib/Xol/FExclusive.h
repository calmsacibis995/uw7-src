#ifndef	NOIDENT
#ident	"@(#)flat:FExclusive.h	1.5"
#endif

#ifndef _OL_FEXCLUSIVE_H
#define _OL_FEXCLUSIVE_H

/************************************************************************
    Description:
	This is the flat exclusives container's public header file.
*/

#include <Xol/FButtons.h>		/* superclasses' header */

/************************************************************************
    Define class and instance pointers:
	- extern pointer to class data/procedures
	- typedef pointer to widget's class structure
	- typedef pointer to widget's instance structure
*/

extern WidgetClass				flatExclusivesWidgetClass;
typedef struct _FlatExclusivesClassRec *	FlatExclusivesWidgetClass;
typedef struct _FlatExclusivesRec *		FlatExclusivesWidget;

#endif /* _OL_FEXCLUSIVE_H */
