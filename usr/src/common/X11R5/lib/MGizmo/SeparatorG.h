#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:SeparatorG.h	1.1"
#endif

#ifndef _SeparatorG_h
#define _SeparatorG_h

typedef struct _SeparatorGizmo {
	HelpInfo *	help;
	char *		name;
	int		type;		/* XmNseparatorType */
	int		orientation;	/* XmVERTICAL or XmHORIZONTAL */
	Dimension	height;		/* In millimeters */
	Dimension	width;		/* In millimeters */
} SeparatorGizmo;

extern GizmoClassRec SeparatorGizmoClass[];

#endif /* _SeparatorG_h */
