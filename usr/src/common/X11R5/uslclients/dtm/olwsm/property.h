#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/property.h	1.28"
#endif

#ifndef _PROPERTY_H
#define _PROPERTY_H

#include <MGizmo/Gizmo.h>	/* For HelpInfo */
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/ModalGizmo.h>

typedef struct Notice {
	String		title;
	String		message;
	void		(*callback)();	/* Routine to call for ok button */
	Gizmo		g;	/* holds the Modal Gizmo */
} Notice;

typedef enum ApplyReturnType 
{
	APPLY_OK,
	APPLY_RESTART,
	APPLY_REFRESH,
	APPLY_ERROR,
	APPLY_NOTICE
}			ApplyReturnType;

typedef struct ApplyReturn 
{
	ApplyReturnType		reason;
	union {
		Notice *		notice;
		String			message;
	}			u;
	struct Property *	bad_sheet;
}			ApplyReturn;

typedef struct Property 
{
	char *		name;
	ArgList		args;	/* Resources set in Widget */
	Cardinal	num_args;
	HelpInfo 	*help;
	char		mnemonic;
	void		(*import)  ( XtPointer );
	void		(*export)  ( XtPointer );
	void		(*create)  ( Widget , XtPointer );
	ApplyReturn *	(*apply)   ( Widget , XtPointer );
	void		(*reset)   ( Widget , XtPointer );
	void		(*factory) ( Widget , XtPointer );
	void		(*popdown) ( Widget , XtPointer );
	void		(*exit)    ( XtPointer );
	XtPointer	closure;
	String		footer;
	String		pLabel;	/* page label */
	Widget		w;
	Widget		pb;
	Boolean		changed;
	void		(*change)  ( int , int );
	Dimension	width;
	Dimension	height;
}			Property;
 
extern List		global_resources;

extern Property *	*PropertyList;
extern int		numKbdSheets;

extern Property		desktopProperty;
extern Property		windowProperty;
extern Property		settingsProperty;
extern Property		localeProperty;

extern void		WSMExitProperty(Display* );
extern void		InitProperty(Display* );
extern void		UpdateResources (void);
extern void		MergeResources (char *);
extern void		DeleteResources(char *);
extern void		PropertySheetByName(String);
extern void		PropertyCB( Widget, XtPointer, XtPointer);
extern void		PropertyApplyCB(Widget, XtPointer, XtPointer);
extern void		DestroyPropertyPopup(Widget, XtPointer, XtPointer);
extern void		HelpCB(Widget, XtPointer, XtPointer);
extern void		PopupMenuCB(Widget, XtPointer, XtPointer);

#if	defined(_EXCLUSIVE_H)
extern ExclusiveItem *	ResourceItem (Exclusive *, char *);
#endif

#if	defined(_NONEXCLU_H)
extern NonexclusiveItem * NonexclusiveResourceItem (Nonexclusive *, char *);
#endif

#endif
