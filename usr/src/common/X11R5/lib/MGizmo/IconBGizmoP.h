#ifndef NOIDENT
#ident	"@(#)MGizmo:IconBGizmoP.h	1.1"
#endif

#ifndef _IconBGizmoP_h
#define _IconBGizmoP_h

#include "IconBGizmo.h"
#include <DtI.h>
#include <../dtm/olwsm/dtprop.h>

typedef void	(*PFV)();

typedef struct Options {
	Dimension           gridWidth;
	Dimension           gridHeight;
	u_char              folderCols;
	u_char              folderRows;
} Options;

typedef struct _IconBoxGizmoP {
	char *		name;
	Widget		swin;
	Widget		box;
	Options		opt;
	DmItemRec *	items;		/* List of Items */
	Cardinal	numItems;	/* Number of items in list */
	Dimension	width;
	Dimension	height;
	PFV		select;		/* Single select callback */
	PFV		dblSelect;	/* Double select callback */
	PFV		draw;		/* Icon drawing routine */
} IconBoxGizmoP;

static char     XtCCols[] = "Cols";
static char     XtCRows[] = "Rows";

#endif /* _IconBGizmoP_h */
