#ifndef	NOIDENT
#ident	"@(#)flat:FNonexclus.h	1.3"
#endif

#ifndef _OL_FNONEXCLUS_H
#define _OL_FNONEXCLUS_H

/************************************************************************
    Description:
 	This is the flat non-exclusives container's public header file.
*/

#include <Xol/FButtons.h>		/* superclasses' header */

/************************************************************************
    Define class and instance pointers:
	- extern pointer to class data/procedures
	- typedef pointer to widget's class structure
	- typedef pointer to widget's instance structure
*/

extern WidgetClass				flatNonexclusivesWidgetClass;
typedef struct _FlatNonexclusivesClassRec *	FlatNonexclusivesWidgetClass;
typedef struct _FlatNonexclusivesRec *		FlatNonexclusivesWidget;

#endif /* _OL_FNONEXCLUS_H */
