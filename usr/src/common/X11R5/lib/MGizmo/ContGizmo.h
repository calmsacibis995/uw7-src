#ifndef NOIDENT
#ident	"@(#)MGizmo:ContGizmo.h	1.1"
#endif

#ifndef _ContGizmo_h
#define _ContGizmo_h

typedef enum {
	G_CONTAINER_SW,
	G_CONTAINER_BB,
	G_CONTAINER_RC,
	G_CONTAINER_FORM,
	G_CONTAINER_FRAME,
	G_CONTAINER_PANEDW
} ContainerType;

typedef struct _ContainerGizmo {
	HelpInfo *	help;
	char *		name;
	ContainerType	type;
	int		width;
	int		height;
	GizmoArray	gizmos;
	int		numGizmos;
} ContainerGizmo;

extern GizmoClassRec	ContainerGizmoClass[];

#endif /* _ContGizmo_h */
