#ifndef	NOIDENT
#ident	"@(#)scrollinglist:ScrollingL.h	1.20"
#endif

#ifndef _ScrollingL_h
#define _ScrollingL_h

/*
 * OPEN LOOK(TM) Scrolling List Widget
 */

#include <Xol/Form.h>		/* include superclass' header */

/*
 * New Resources:
 *
 * Name			Type		Default	   Meaning
 * ----			----		-------	   -------
 * XtNapplAddItem	function	n/a	get and call to add item
 * XtNapplDeleteItem	function	n/a	get and call to delete item
 * XtNapplEditClose	function	n/a	get and call to begin edit
 * XtNapplEditOpen	function	n/a	get and call to end edit
 * XtNapplTouchItem	function	n/a	get and call to change item
 * XtNapplUpdateView	function	n/a	get and call to (un)lock view
 * XtNapplViewItem	function	n/a	get and call to view item
 * XtNselectable	Boolean		True	List is selectable
 * XtNuserDeleteItems	Callback	NULL	user event: delete (cut)
 * XtNuserMakeCurrent	Callback	NULL	user event: select
 * XtNviewHeight	Cardinal	0	number of items in view
 */

/* Constants */

#define OL_B_LIST_ATTR_APPL	0x0000ffff
#define OL_B_LIST_ATTR_CURRENT	0x00020000
#define OL_B_LIST_ATTR_SELECTED	0x00040000
#define OL_B_LIST_ATTR_FOCUS	0x00080000

/* for 1.0 compatibility */
#define OL_LIST_ATTR_APPL	OL_B_LIST_ATTR_APPL
#define OL_LIST_ATTR_CURRENT	OL_B_LIST_ATTR_CURRENT
#define OL_LIST_ATTR_SELECTED	OL_B_LIST_ATTR_SELECTED


/* Structures */

typedef struct _OlListItem {			/* OPEN LOOK list item */
    OlDefine		label_type;
    XtPointer		label;
    XImage *		glyph;
    OlBitMask		attr;
    XtPointer		user_data;
    unsigned char	mnemonic;
} OlListItem;

typedef struct _OlListToken * OlListToken;	/* opaque item token */

typedef struct _OlListDelete {			/* XtNuserDelete call_data */
    OlListToken	*	tokens;
    Cardinal		num_tokens;
} OlListDelete;


/* Class record pointer */
extern WidgetClass scrollingListWidgetClass;

/* C Widget type definition */
typedef struct _ListClassRec	*ScrollingListWidgetClass;
typedef struct _ListRec		*ScrollingListWidget;

/*
 * function prototype section
 */

OLBeginFunctionPrototypeBlock

extern OlListItem *
OlListItemPointer OL_ARGS((OlListToken));

OLEndFunctionPrototypeBlock

#endif /* _ScrollingL_h */
