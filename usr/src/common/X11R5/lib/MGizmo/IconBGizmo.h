#ifndef NOIDENT
#ident	"@(#)MGizmo:IconBGizmo.h	1.1"
#endif

#ifndef _IconBGizmo_h
#define _IconBGizmo_h

#define ICON_LABEL(label, string) \
	{ XmString str = XmStringCreateLtoR( \
		(String)(string), XmFONTLIST_DEFAULT_TAG \
	); \
	(label) = (XtArgVal)_XmStringCreate(str); \
	XmStringFree(str); \
}

typedef struct _IconBoxItem {
	_XmString	label;
	WidePosition	x;		/* Value ignored by Gizmo */
	WidePosition	y;		/* Value ignored by Gizmo */
	Dimension	icon_width;	/* Value ignored by Gizmo */
	Dimension	icon_height;	/* Value ignored by Gizmo */
	Boolean		managed;
	Boolean		select;
	Boolean		sensitive;
	XtPointer	client_data;
	DmGlyphPtr	glyph;
} IconBoxItem;

typedef struct _IconBoxGizmo {
	HelpInfo *	help;
	char *		name;
	Dimension	width;		/* Nonzero overrides resource values */
	Dimension	height;		/* Nonzero overrides resource values */
	DmItemRec *	items;		/* Array of list items */
	int		numItems;
	void		(*select)();	/* selectProc */
	void		(*dblSelect)();	/* dblSelectProc */
	void		(*draw)();	/* Icon drawing routine */
	XtPointer	clientData;
} IconBoxGizmo;

extern GizmoClassRec	IconBoxGizmoClass[];

extern Widget		GetIconBoxSW(Gizmo);
extern Dimension	GetIconBoxWidth(Gizmo);
extern Dimension	GetIconBoxHeigth(Gizmo);
extern void		ResetIconBoxGizmo(Gizmo, IconBoxGizmo *, Arg *, int);

#endif /* _IconBGizmo_h */
