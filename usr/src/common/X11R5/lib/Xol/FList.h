#ifndef	NOIDENT
#ident	"@(#)flat:FList.h	1.7"
#endif

#ifndef _OL_FLIST_H
#define _OL_FLIST_H

/***********************************************************************
    Description:
	This is the flat list container's public header file.
*/

#include <Xol/FRowColumn.h>	/* superclasses' header */

/***********************************************************************
    Declare extern variables, structures, and defines.
*/

typedef struct {
    OlFlatCallData *	item_data;
    Cardinal *		enters;
    Cardinal		num_enters;
    Cardinal *		leaves;
    Cardinal		num_leaves;
} OlFListItemVisibilityCD;

typedef struct {
    OlFlatCallData *	item_data;	/* current is item_data->num_items */
    Cardinal		preferred;	/* preferred item list size... */
    Boolean		ok;		/* T, replace numItems with preferred */
					/* F, a warning... */
} OlFListItemsLimitExceededCD;

#define OL_LIST_DEFAULT_FORMAT "%s"

#define XtNviewItemIndex "viewItemIndex"

/***********************************************************************
    Declare public routines
*/

void	OlFlatListViewItem OL_ARGS((
				    Widget,	/* FlatList widget */
				    Cardinal	/* Item index to view */
				    ));

/************************************************************************
    Define class and instance pointers:
	- extern pointer to class data/procedures
	- typedef pointer to widget's class structure
	- typedef pointer to widget's instance structure
*/

extern WidgetClass			flatListWidgetClass;
typedef struct _FlatListClassRec *	FlatListWidgetClass;
typedef struct _FlatListRec *		FlatListWidget;

#endif /* _OL_FLIST_H */
