#pragma ident	"@(#)m1.2libs:DtWidget/ComboBox.h	1.3"
/*
 * DtWidget/ComboBox.h
 */
/*
 *  (c) Copyright 1993, 1994 Hewlett-Packard Company
 *  (c) Copyright 1993, 1994 International Business Machines Corp.
 *  (c) Copyright 1993, 1994 Novell, Inc.
 *  (c) Copyright 1993, 1994 Sun Microsystems, Inc.
 */
/***********************************************************
Copyright 1993 Interleaf, Inc.

Permission to use, copy, modify, and distribute this software
and its documentation for any purpose without fee is granted,
provided that the above copyright notice appear in all copies
and that both copyright notice and this permission notice appear
in supporting documentation, and that the name of Interleaf not
be used in advertising or publicly pertaining to distribution of
the software without specific written prior permission.

Interleaf makes no representation about the suitability of this
software for any purpose. It is provided "AS IS" without any
express or implied warranty.
******************************************************************/

#ifndef _Dt_ComboBox_h
#define _Dt_ComboBox_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Constants
 */

/* Resources */

#ifndef DtNactivateCallback
#define DtNactivateCallback	XmNactivateCallback
#endif
#ifndef DtNalignment
#define DtNalignment		XmNalignment
#endif
#ifndef DtNcolumns
#define DtNcolumns		XmNcolumns
#endif
#ifndef DtNfocusCallback
#define DtNfocusCallback	XmNfocusCallback
#endif
#ifndef DtNhorizontalSpacing
#define DtNhorizontalSpacing	XmNhorizontalSpacing
#endif
#ifndef DtNitemCount
#define DtNitemCount		XmNitemCount
#endif
#ifndef DtNitems
#define DtNitems		XmNitems
#endif
#ifndef DtNlabelString
#define DtNlabelString		XmNlabelString
#endif
#ifndef DtNlistMarginHeight
#define DtNlistMarginHeight	XmNlistMarginHeight
#endif
#ifndef DtNlistMarginWidth
#define DtNlistMarginWidth	XmNlistMarginWidth
#endif
#ifndef DtNlistSpacing
#define DtNlistSpacing		XmNlistSpacing
#endif
#ifndef DtNlosingFocusCallback
#define DtNlosingFocusCallback	XmNlosingFocusCallback
#endif
#ifndef DtNmarginHeight
#define DtNmarginHeight		XmNmarginHeight
#endif
#ifndef DtNmarginWidth
#define DtNmarginWidth		XmNmarginWidth
#endif
#ifndef DtNmaxLength
#define DtNmaxLength		XmNmaxLength
#endif
#ifndef DtNorientation
#define DtNorientation		XmNorientation
#endif
#ifndef DtNrecomputeSize
#define DtNrecomputeSize	XmNrecomputeSize
#endif
#ifndef DtNtopItemPosition
#define DtNtopItemPosition	XmNtopItemPosition
#endif
#ifndef DtNverticalSpacing
#define DtNverticalSpacing	XmNverticalSpacing
#endif
#ifndef DtNvisibleItemCount
#define DtNvisibleItemCount	XmNvisibleItemCount
#endif

#ifdef  XmNarrowSize
#define DtNarrowSize		XmNarrowSize
#else   /* XmNarrowSize */
#ifndef DtNarrowSize
#define DtNarrowSize		"arrowSize"
#endif
#define XmNarrowSize		DtNarrowSize
#endif  /* XmNarrowSize */
#ifdef  XmNarrowSpacing
#define DtNarrowSpacing		XmNarrowSpacing
#else   /* XmNarrowSpacing */
#ifndef DtNarrowSpacing
#define DtNarrowSpacing		"arrowSpacing"
#endif
#define XmNarrowSpacing		DtNarrowSpacing
#endif  /* XmNarrowSpacing */
#ifdef  XmNarrowType
#define DtNarrowType		XmNarrowType
#else
#ifndef DtNarrowType
#define DtNarrowType		"arrowType"
#endif
#define XmNarrowType		DtNarrowType
#endif
#ifdef  XmNcomboBoxType
#define DtNcomboBoxType		XmNcomboBoxType
#else   /* XmNcomboBoxType */
#ifndef DtNcomboBoxType
#define DtNcomboBoxType		"comboBoxType"
#endif
#define XmNcomboBoxType		DtNcomboBoxType
#endif  /* XmNcomboBoxType */
#ifdef  XmNlist
#define DtNlist			XmNlist
#else   /* XmNlist */
#ifndef DtNlist
#define DtNlist			"list"
#endif
#define XmNlist			DtNlist
#endif  /* XmNlist */
#ifdef  XmNlistFontList
#define DtNlistFontList		XmNlistFontList
#else
#ifndef DtNlistFontList
#define DtNlistFontList		"listFontList"
#endif
#define XmNlistFontList		DtNlistFontList
#endif  /* XmNlistFontList */
#ifdef  XmNmenuPostCallback
#define DtNmenuPostCallback	XmNmenuPostCallback
#else
#ifndef DtNmenuPostCallback
#define DtNmenuPostCallback	"menuPostCallback"
#endif
#define XmNmenuPostCallback	DtNmenuPostCallback
#endif  /* XmNmenuPostCallback */
#ifdef  XmNpoppedUp
#define DtNpoppedUp		XmNpoppedUp
#else
#ifndef DtNpoppedUp
#define DtNpoppedUp		"poppedUp"
#endif
#define XmNpoppedUp		DtNpoppedUp
#endif  /* XmNpoppedUp */
#ifdef  XmNselectedItem
#define DtNselectedItem		XmNselectedItem
#else
#ifndef DtNselectedItem
#define DtNselectedItem		"selectedItem"
#endif
#define XmNselectedItem		DtNselectedItem
#endif  /* XmNselectedItem */
#ifdef  XmNselectedPosition
#define DtNselectedPosition	XmNselectedPosition
#else
#ifndef DtNselectedPosition
#define DtNselectedPosition	"selectedPosition"
#endif
#define XmNselectedPosition	DtNselectedPosition
#endif  /* XmNselectedPosition */
#ifdef  XmNselectionCallback
#define DtNselectionCallback	XmNselectionCallback
#else
#ifndef DtNselectionCallback
#define DtNselectionCallback	"selectionCallback"
#endif
#define XmNselectionCallback	DtNselectionCallback
#endif  /* XmNselectionCallback */
#ifdef  XmNtextField
#define DtNtextField		XmNtextField
#else
#ifndef DtNtextField
#define DtNtextField		"textField"
#endif
#define XmNtextField		DtNtextField
#endif  /* XmNtextField */
#ifdef  XmNupdateLabel
#define DtNupdateLabel		XmNupdateLabel
#else
#ifndef DtNupdateLabel
#define DtNupdateLabel		"updateLabel"
#endif
#define XmNupdateLabel		DtNupdateLabel
#endif  /* XmNupdateLabel */

#ifndef DtCAlignment
#define DtCAlignment		XmCAlignment
#endif
#ifndef DtCCallback
#define DtCCallback		XmCCallback
#endif
#ifndef DtCColumns
#define DtCColumns		XmCColumns
#endif
#ifndef DtCItemCount
#define DtCItemCount		XmCItemCount
#endif
#ifndef DtCItems
#define DtCItems		XmCItems
#endif
#ifndef DtCListMarginHeight
#define DtCListMarginHeight	XmCListMarginHeight
#endif
#ifndef DtCListMarginWidth
#define DtCListMarginWidth	XmCListMarginWidth
#endif
#ifndef DtCListSpacing
#define DtCListSpacing		XmCListSpacing
#endif
#ifndef DtCMarginHeight
#define DtCMarginHeight		XmCMarginHeight
#endif
#ifndef DtCMarginWidth
#define DtCMarginWidth		XmCMarginWidth
#endif
#ifndef DtCMaxLength
#define DtCMaxLength		XmCMaxLength
#endif
#ifndef DtCOrientation
#define DtCOrientation		XmCOrientation
#endif
#ifndef DtCRecomputeSize
#define DtCRecomputeSize	XmCRecomputeSize
#endif
#ifndef DtCTopItemPosition
#define DtCTopItemPosition	XmCTopItemPosition
#endif
#ifndef DtCVisibleItemCount
#define DtCVisibleItemCount	XmCVisibleItemCount
#endif
#ifndef DtCXmString
#define DtCXmString		XmCXmString
#endif

#ifdef  XmCArrowSize
#define DtCArrowSize		XmCArrowSize
#else
#ifndef DtCArrowSize
#define DtCArrowSize		"ArrowSize"
#endif
#define XmCArrowSize		DtCArrowSize
#endif
#ifdef  XmCArrowSpacing
#define DtCArrowSpacing		XmCArrowSpacing
#else
#ifndef DtCArrowSpacing
#define DtCArrowSpacing		"ArrowSpacing"
#endif
#define XmCArrowSpacing		DtCArrowSpacing
#endif
#ifdef  XmCArrowType
#define DtCArrowType		XmCArrowType
#else
#ifndef DtCArrowType
#define DtCArrowType		"ArrowType"
#endif
#define XmCArrowType		DtCArrowType
#endif
#ifdef  XmCComboBoxType
#define DtCComboBoxType		XmCComboBoxType
#else
#ifndef DtCComboBoxType
#define DtCComboBoxType		"ComboBoxType"
#endif
#define XmCComboBoxType		DtCComboBoxType
#endif
#ifdef  XmCHorizontalSpacing
#define DtCHorizontalSpacing	XmCHorizontalSpacing
#else
#ifndef DtCHorizontalSpacing
#define DtCHorizontalSpacing	"HorizontalSpacing"
#endif
#define XmCHorizontalSpacing	DtCHorizontalSpacing
#endif
#ifdef  XmCList
#define DtCList			XmCList
#else
#ifndef DtCList
#define DtCList			"List"
#endif
#define XmCList			DtCList
#endif
#ifdef  XmCListFontList
#define DtCListFontList		XmCListFontList
#else
#ifndef DtCListFontList
#define DtCListFontList		"ListFontList"
#endif
#define XmCListFontList		DtCListFontList
#endif
#ifdef  XmCPoppedUp
#define DtCPoppedUp		XmCPoppedUp
#else
#ifndef DtCPoppedUp
#define DtCPoppedUp		"PoppedUp"
#endif
#define XmCPoppedUp		DtCPoppedUp
#endif
#ifdef  XmCSelectedItem
#define DtCSelectedItem		XmCSelectedItem
#else
#ifndef DtCSelectedItem
#define DtCSelectedItem		"SelectedItem"
#endif
#define XmCSelectedItem		DtCSelectedItem
#endif
#ifdef  XmCSelectedPosition
#define DtCSelectedPosition	XmCSelectedPosition
#else
#ifndef DtCSelectedPosition
#define DtCSelectedPosition	"SelectedPosition"
#endif
#define XmCSelectedPosition	DtCSelectedPosition
#endif
#ifdef  XmCTextField
#define DtCTextField		XmCTextField
#else
#ifndef DtCTextField
#define DtCTextField		"TextField"
#endif
#define XmCTextField		DtCTextField
#endif
#ifdef  XmCUpdateLabel
#define DtCUpdateLabel		XmCUpdateLabel
#else
#ifndef DtCUpdateLabel
#define DtCUpdateLabel		"UpdateLabel"
#endif
#define XmCUpdateLabel		DtCUpdateLabel
#endif
#ifdef  XmCVerticalSpacing
#define DtCVerticalSpacing	XmCVerticalSpacing
#else
#ifndef DtCVerticalSpacing
#define DtCVerticalSpacing	"VerticalSpacing"
#endif
#define XmCVerticalSpacing	DtCVerticalSpacing
#endif

/* Representation types */

#ifdef  XmRArrowType
#define DtRArrowType		XmRArrowType
#else
#ifndef DtRArrowType
#define DtRArrowType		"ArrowType"
#endif
#define XmRArrowType		DtRArrowType
#endif
#ifdef  XmRComboBoxType
#define DtRComboBoxType		XmRComboBoxType
#else
#ifndef DtRComboBoxType
#define DtRComboBoxType		"ComboBoxType"
#endif
#define XmRComboBoxType		DtRComboBoxType
#endif

/* DtNorientation values */

#ifdef  XmLEFT
#define DtLEFT		XmLEFT
#else
#ifndef DtLEFT
#define DtLEFT		0
#endif
#define XmLEFT		DtLEFT
#endif
#ifdef  XmRIGHT
#define DtRIGHT		XmRIGHT
#else
#ifndef DtRIGHT
#define DtRIGHT		1
#endif
#define XmRIGHT		DtRIGHT
#endif

/* DtNarrowType values */

#ifdef  XmMOTIF
#define DtMOTIF		XmMOTIF
#else
#ifndef DtMOTIF
#define DtMOTIF		0
#endif
#define XmMOTIF		DtMOTIF
#endif
#ifdef  XmWINDOWS
#define DtWINDOWS	XmWINDOWS
#else
#ifndef DtWINDOWS
#define DtWINDOWS	1
#endif
#define XmWINDOWS	DtWINDOWS
#endif

/* DtNcomboBoxType values */

#ifdef  XmDROP_DOWN_LIST
#define DtDROP_DOWN_LIST	XmDROP_DOWN_LIST
#else
#ifndef DtDROP_DOWN_LIST
#define DtDROP_DOWN_LIST	0
#endif
#define XmDROP_DOWN_LIST	DtDROP_DOWN_LIST
#endif
#ifdef  XmDROP_DOWN_COMBO_BOX
#define DtDROP_DOWN_COMBO_BOX	XmDROP_DOWN_COMBO_BOX
#else
#ifndef DtDROP_DOWN_COMBO_BOX
#define DtDROP_DOWN_COMBO_BOX	1
#endif
#define XmDROP_DOWN_COMBO_BOX	DtDROP_DOWN_COMBO_BOX
#endif

/* DtNalignment values */

#ifndef DtALIGNMENT_BEGINNING
#define DtALIGNMENT_BEGINNING	XmALIGNMENT_BEGINNING
#endif
#ifndef DtALIGNMENT_CENTER
#define DtALIGNMENT_CENTER	XmALIGNMENT_CENTER
#endif
#ifndef DtALIGNMENT_END
#define DtALIGNMENT_END		XmALIGNMENT_END
#endif

/* Callback reasons */

#ifdef  XmCR_SELECT
#define DtCR_SELECT		XmCR_SELECT
#else
#ifndef DtCR_SELECT
#define DtCR_SELECT	    128 /* Large #, so no collisions with XM */
#endif
#define XmCR_SELECT		DtCR_SELECT
#endif
#ifdef  XmCR_MENU_POST
#define DtCR_MENU_POST		XmCR_MENU_POST
#else
#ifndef DtCR_MENU_POST
#define DtCR_MENU_POST      129 /* Large #, so no collisions with XM */
#endif
#define XmCR_MENU_POST		DtCR_MENU_POST
#endif


/*
 * Types
 */

typedef struct {
	int		reason;
	XEvent		*event;
	XmString	item_or_text;
	int		item_position;
} DtComboBoxCallbackStruct;


/* Widget class and instance */

typedef struct _DtComboBoxClassRec *DtComboBoxWidgetClass;
typedef struct _DtComboBoxRec      *DtComboBoxWidget;


/*
 * Data
 */

/* Widget class record */

externalref WidgetClass dtComboBoxWidgetClass;


/*
 * Functions
 */

extern Widget DtCreateComboBox(
		Widget		parent,
		char		*name,
		ArgList		arglist,
		Cardinal	argcount);

extern void DtComboBoxAddItem(
		Widget 		combo,
		XmString	item,
		int		pos,
		Boolean		unique);

extern void DtComboBoxDeletePos(
		Widget		combo,
		int		pos);

extern void DtComboBoxSetItem(
		Widget		combo,
		XmString	item);

extern void DtComboBoxSelectItem(
		Widget		combo,
		XmString	item);

#ifdef __cplusplus
}
#endif

#endif	/* _Dt_ComboBox_h */
