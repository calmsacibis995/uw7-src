#pragma ident	"@(#)m1.2libs:DtWidget/SpinBox.h	1.3"
/*
 * DtWidget/SpinBox.h
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

#ifndef _Dt_SpinBox_h
#define _Dt_SpinBox_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Constants
 */

/* Resources */

#ifdef  XmNarrowLayout
#define DtNarrowLayout		XmNarrowLayout
#else
#ifndef DtNarrowLayout
#define DtNarrowLayout		"arrowLayout"
#endif
#define XmNarrowLayout		DtNarrowLayout
#endif
#ifdef  XmNarrowSensitivity
#define DtNarrowSensitivity	XmNarrowSensitivity
#else
#ifndef DtNarrowSensitivity
#define DtNarrowSensitivity	"arrowSensitivity"
#endif
#define XmNarrowSensitivity	DtNarrowSensitivity
#endif
#ifdef  XmNarrowSize
#define DtNarrowSize		XmNarrowSize
#else
#ifndef DtNarrowSize
#define DtNarrowSize		"arrowSize"
#endif
#define XmNarrowSize		DtNarrowSize
#endif
#ifdef  XmNspinBoxChildType
#define DtNspinBoxChildType	XmNspinBoxChildType
#else
#ifndef DtNspinBoxChildType
#define DtNspinBoxChildType	"spinBoxChildType"
#endif
#define XmNspinBoxChildType	DtNspinBoxChildType
#endif
#ifdef  XmNposition
#define DtNposition		XmNposition
#else
#ifndef DtNposition
#define DtNposition		"position"
#endif
#define XmNposition		DtNposition
#endif
#ifdef  XmNtextField
#define DtNtextField		XmNtextField
#else
#ifndef DtNtextField
#define DtNtextField		"textField"
#endif
#define XmNtextField		DtNtextField
#endif
#ifdef  XmNwrap
#define DtNwrap			XmNwrap
#else
#ifndef DtNwrap
#define DtNwrap			"wrap"
#endif
#define XmNwrap			DtNwrap
#endif
#ifdef  XmNincrementValue
#define DtNincrementValue	XmNincrementValue
#else
#ifndef DtNincrementValue
#define DtNincrementValue	"incrementValue"
#endif
#define XmNincrementValue	DtNincrementValue
#endif
#ifdef  XmNmaximumValue
#define DtNmaximumValue		XmNmaximumValue
#else
#ifndef DtNmaximumValue
#define DtNmaximumValue		"maximumValue"
#endif
#define XmNmaximumValue		DtNmaximumValue
#endif
#ifdef  XmNminimumValue
#define DtNminimumValue		XmNminimumValue
#else
#ifndef DtNminimumValue
#define DtNminimumValue		"minimumValue"
#endif
#define XmNminimumValue		DtNminimumValue
#endif
#ifdef  XmNnumValues
#define DtNnumValues		XmNnumValues
#else
#ifndef DtNnumValues
#define DtNnumValues		"numValues"
#endif
#define XmNnumValues		DtNnumValues
#endif
#ifdef  XmNvalues
#define DtNvalues		XmNvalues
#else
#ifndef DtNvalues
#define DtNvalues		"values"
#endif
#define XmNvalues		DtNvalues
#endif

#ifndef DtNactivateCallback
#define DtNactivateCallback	XmNactivateCallback
#endif
#ifndef DtNalignment
#define DtNalignment		XmNalignment
#endif
#ifndef DtNcolumns
#define DtNcolumns		XmNcolumns
#endif
#ifndef DtNdecimalPoints
#define DtNdecimalPoints	XmNdecimalPoints
#endif
#ifndef DtNeditable
#define DtNeditable		XmNeditable
#endif
#ifndef DtNfocusCallback
#define DtNfocusCallback	XmNfocusCallback
#endif
#ifndef DtNinitialDelay
#define DtNinitialDelay		XmNinitialDelay
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
#ifndef DtNmodifyVerifyCallback
#define DtNmodifyVerifyCallback	XmNmodifyVerifyCallback
#endif
#ifndef DtNrecomputeSize
#define DtNrecomputeSize	XmNrecomputeSize
#endif
#ifndef DtNrepeatDelay
#define DtNrepeatDelay		XmNrepeatDelay
#endif
#ifndef DtNvalueChangedCallback
#define DtNvalueChangedCallback	XmNvalueChangedCallback
#endif

#ifdef  XmCArrowLayout
#define DtCArrowLayout		XmCArrowLayout
#else
#ifndef DtCArrowLayout
#define DtCArrowLayout		"ArrowLayout"
#endif
#define XmCArrowLayout		DtCArrowLayout
#endif
#ifdef  XmCArrowSensitivity
#define DtCArrowSensitivity	XmCArrowSensitivity
#else
#ifndef DtCArrowSensitivity
#define DtCArrowSensitivity	"ArrowSensitivity"
#endif
#define XmCArrowSensitivity	DtCArrowSensitivity
#endif
#ifdef  XmCSpinBoxChildType
#define DtCSpinBoxChildType	XmCSpinBoxChildType
#else
#ifndef DtCSpinBoxChildType
#define DtCSpinBoxChildType	"SpinBoxChildType"
#endif
#define XmCSpinBoxChildType	DtCSpinBoxChildType
#endif
#ifdef  XmCTextField
#define DtCTextField		XmCTextField
#else
#ifndef DtCTextField
#define DtCTextField		"TextField"
#endif
#define XmCTextField		DtCTextField
#endif
#ifdef  XmCWrap
#define DtCWrap			XmCWrap
#else
#ifndef DtCWrap
#define DtCWrap			"Wrap"
#endif
#define XmCWrap			DtCWrap
#endif
#ifdef  XmCIncrementValue
#define DtCIncrementValue	XmCIncrementValue
#else
#ifndef DtCIncrementValue
#define DtCIncrementValue	"incrementValue"
#endif
#define XmCIncrementValue	DtCIncrementValue
#endif
#ifdef  XmCMaximumValue
#define DtCMaximumValue		XmCMaximumValue
#else
#ifndef DtCMaximumValue
#define DtCMaximumValue		"maximumValue"
#endif
#define XmCMaximumValue		DtCMaximumValue
#endif
#ifdef  XmCMinimumValue
#define DtCMinimumValue		XmCMinimumValue
#else
#ifndef DtCMinimumValue
#define DtCMinimumValue		"minimumValue"
#endif
#define XmCMinimumValue		DtCMinimumValue
#endif
#ifdef  XmCNumValues
#define DtCNumValues		XmCNumValues
#else
#ifndef DtCNumValues
#define DtCNumValues		"numValues"
#endif
#define XmCNumValues		DtCNumValues
#endif
#ifdef  XmCValues
#define DtCValues		XmCValues
#else
#ifndef DtCValues
#define DtCValues		"values"
#endif
#define XmCValues		DtCValues
#endif

#ifndef DtCAlignment
#define DtCAlignment		XmCAlignment
#endif
#ifndef DtCCallback
#define DtCCallback		XmCCallback
#endif
#ifndef DtCColumns
#define DtCColumns		XmCColumns
#endif
#ifndef DtCDecimalPoints
#define DtCDecimalPoints	XmCDecimalPoints
#endif
#ifndef DtCEditable
#define DtCEditable		XmCEditable
#endif
#ifndef DtCInitialDelay
#define DtCInitialDelay		XmCInitialDelay
#endif
#ifndef DtCItems
#define DtCItems		XmCItems
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
#ifndef DtCPosition
#define DtCPosition		XmCPosition
#endif
#ifndef DtCRecomputeSize
#define DtCRecomputeSize	XmCRecomputeSize
#endif
#ifndef DtCRepeatDelay
#define DtCRepeatDelay		XmCRepeatDelay
#endif

/* Representation types */

#ifdef  XmRIncrementValue
#define DtRIncrementValue	XmRIncrementValue
#else
#ifndef DtRIncrementValue
#define DtRIncrementValue	"IncrementValue"
#endif
#define XmRIncrementValue	DtRIncrementValue
#endif
#ifdef  XmRMaximumValue
#define DtRMaximumValue		XmRMaximumValue
#else
#ifndef DtRMaximumValue
#define DtRMaximumValue		"MaximumValue"
#endif
#define XmRMaximumValue		DtRMaximumValue
#endif
#ifdef  XmRMinimumValue
#define DtRMinimumValue		XmRMinimumValue
#else
#ifndef DtRMinimumValue
#define DtRMinimumValue		"MinimumValue"
#endif
#define XmRMinimumValue		DtRMinimumValue
#endif
#ifdef  XmRNumValues
#define DtRNumValues		XmRNumValues
#else
#ifndef DtRNumValues
#define DtRNumValues		"NumValues"
#endif
#define XmRNumValues		DtRNumValues
#endif
#ifdef  XmRValues
#define DtRValues		XmRValues
#else
#ifndef DtRValues
#define DtRValues		"Values"
#endif
#define XmRValues		DtRValues
#endif
#ifdef  XmRArrowSensitivity
#define DtRArrowSensitivity	XmRArrowSensitivity
#else
#ifndef DtRArrowSensitivity
#define DtRArrowSensitivity	"ArrowSensitivity"
#endif
#define XmRArrowSensitivity	DtRArrowSensitivity
#endif
#ifdef  XmRArrowLayout
#define DtRArrowLayout		XmRArrowLayout
#else
#ifndef DtRArrowLayout
#define DtRArrowLayout		"ArrowLayout"
#endif
#define XmRArrowLayout		DtRArrowLayout
#endif
#ifdef  XmRSpinBoxChildType
#define DtRSpinBoxChildType	XmRSpinBoxChildType
#else
#ifndef DtRSpinBoxChildType
#define DtRSpinBoxChildType	"SpinBoxChildType"
#endif
#define XmRSpinBoxChildType	DtRSpinBoxChildType
#endif

/* DtNarrowLayout values */

#ifdef  XmARROWS_FLAT_BEGINNING
#define DtARROWS_FLAT_BEGINNING		XmARROWS_FLAT_BEGINNING
#else
#ifndef DtARROWS_FLAT_BEGINNING
#define DtARROWS_FLAT_BEGINNING		0
#endif
#define XmARROWS_FLAT_BEGINNING		DtARROWS_FLAT_BEGINNING
#endif
#ifdef  XmARROWS_FLAT_END
#define DtARROWS_FLAT_END		XmARROWS_FLAT_END
#else
#ifndef DtARROWS_FLAT_END
#define DtARROWS_FLAT_END		1
#endif
#define XmARROWS_FLAT_END		DtARROWS_FLAT_END
#endif
#ifdef  XmARROWS_SPLIT
#define DtARROWS_SPLIT			XmARROWS_SPLIT
#else
#ifndef DtARROWS_SPLIT
#define DtARROWS_SPLIT			2
#endif
#define XmARROWS_SPLIT			DtARROWS_SPLIT
#endif
#ifdef  XmARROWS_BEGINNING
#define DtARROWS_BEGINNING		XmARROWS_BEGINNING
#else
#ifndef DtARROWS_BEGINNING
#define DtARROWS_BEGINNING		3
#endif
#define XmARROWS_BEGINNING		DtARROWS_BEGINNING
#endif
#ifdef  XmARROWS_END
#define DtARROWS_END			XmARROWS_END
#else
#ifndef DtARROWS_END
#define DtARROWS_END			4
#endif
#define XmARROWS_END			DtARROWS_END
#endif

/* DtNarrowSensitivity values */

#ifdef  XmARROWS_SENSITIVE
#define DtARROWS_SENSITIVE		XmARROWS_SENSITIVE
#else
#ifndef DtARROWS_SENSITIVE
#define DtARROWS_SENSITIVE		0
#endif
#define XmARROWS_SENSITIVE		DtARROWS_SENSITIVE
#endif
#ifdef  XmARROWS_DECREMENT_SENSITIVE
#define DtARROWS_DECREMENT_SENSITIVE	XmARROWS_DECREMENT_SENSITIVE
#else
#ifndef DtARROWS_DECREMENT_SENSITIVE
#define DtARROWS_DECREMENT_SENSITIVE	1
#endif
#define XmARROWS_DECREMENT_SENSITIVE	DtARROWS_DECREMENT_SENSITIVE
#endif
#ifdef  XmARROWS_INCREMENT_SENSITIVE
#define DtARROWS_INCREMENT_SENSITIVE	XmARROWS_INCREMENT_SENSITIVE
#else
#ifndef DtARROWS_INCREMENT_SENSITIVE
#define DtARROWS_INCREMENT_SENSITIVE	2
#endif
#define XmARROWS_INCREMENT_SENSITIVE	DtARROWS_INCREMENT_SENSITIVE
#endif
#ifdef  XmARROWS_INSENSITIVE
#define DtARROWS_INSENSITIVE		XmARROWS_INSENSITIVE
#else
#ifndef DtARROWS_INSENSITIVE
#define DtARROWS_INSENSITIVE		3
#endif
#define XmARROWS_INSENSITIVE		DtARROWS_INSENSITIVE
#endif

/* DtNspinBoxChildType values */

#ifdef  XmNUMERIC
#define DtNUMERIC	XmNUMERIC
#else
#ifndef DtNUMERIC
#define DtNUMERIC	0
#endif
#define XmNUMERIC	DtNUMERIC
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
#ifndef DtCR_OK
#define DtCR_OK                 XmCR_OK
#endif
#ifdef  XmCR_SPIN_NEXT
#define DtCR_SPIN_NEXT		XmCR_SPIN_NEXT
#else
#ifndef DtCR_SPIN_NEXT
#define DtCR_SPIN_NEXT		100
#endif
#define XmCR_SPIN_NEXT		DtCR_SPIN_NEXT
#endif
#ifdef  XmCR_SPIN_PRIOR
#define DtCR_SPIN_PRIOR		XmCR_SPIN_PRIOR
#else
#ifndef DtCR_SPIN_PRIOR
#define DtCR_SPIN_PRIOR		101
#endif
#define XmCR_SPIN_PRIOR		DtCR_SPIN_PRIOR
#endif


/*
 * Types
 */

typedef struct {
	int		reason;
	XEvent		*event;
	Widget		widget;
	Boolean		doit;
	int		position;
	XmString	value;
	Boolean		crossed_boundary;
} DtSpinBoxCallbackStruct;

/* Widget class and instance */

typedef struct _DtSpinBoxClassRec *DtSpinBoxWidgetClass;
typedef struct _DtSpinBoxRec      *DtSpinBoxWidget;

/*
 * Data
 */

/* Widget class record */

externalref WidgetClass dtSpinBoxWidgetClass;


/*
 * Functions
 */

extern Widget DtCreateSpinBox(
		Widget		parent,
		char		*name,
		ArgList		arglist,
		Cardinal	argcount);

extern void DtSpinBoxAddItem(
		Widget		widget,
		XmString	item,
		int		pos);

extern void DtSpinBoxDeletePos(
		Widget		widget,
		int		pos);

extern void DtSpinBoxSetItem(
		Widget		widget,
		XmString	item);

#ifdef __cplusplus
}
#endif

#endif	/* _Dt_SpinBox_h */
