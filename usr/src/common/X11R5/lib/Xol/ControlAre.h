#ifndef	NOIDENT
#ident	"@(#)control:ControlAre.h	1.13"
#endif

/**
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **/

#ifndef _Control_h
#define _Control_h

#include <Xol/Manager.h>	/* include superclasses' header */
#include <Xol/ChangeBar.h>

/*
**	Layouttype is one of OL_FIXEDCOLS, OL_FIXEDROWS, OL_FIXEDWIDTH,
**	OL_FIXEDHEIGHT.
**	measure is number of rows or columns for the first two cases,
**	height or width for the other two cases.
*/

/*

Name		Type		Default		Meaning
---		----		-------		-------
XtNmeasure	XtRInt		1		maximum height/width/rows/cols
XtNlayoutType	ControlLayout	OL_FIXEDROWS	Type of layout
XtNhSpace	Dimension	4		interwidget horizonal spacing
XtNvSpace	Dimension	4		interwidget vertical spacing
XtNhPad		Dimension	4		horizonal padding at edges
XtNvPad		Dimension	4		vertical padding at edges
XtNsameSize	OlSameSize	OL_NONE		Force children to be same size?
XtNalignCaptions	Boolean	FALSE		Align captions?
XtNcenter	Boolean		FALSE		Center widgets in each column?
XtNpostSelect	XtCallbackList	NULL		Callbacks for children


typedef int ControlLayout;

typedef int OlSameSize;
*/

typedef struct _ControlClassRec *ControlAreaWidgetClass;
typedef struct _ControlRec      *ControlAreaWidget;
typedef struct _ControlAreaConstraintRec *	ControlAreaConstraints;

extern WidgetClass controlAreaWidgetClass;

#endif /* _Control_h */
