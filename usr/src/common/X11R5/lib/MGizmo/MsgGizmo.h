#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:MsgGizmo.h	1.1"
#endif

#ifndef _MsgGizmo_h
#define _MsgGizmo_h

typedef struct	_MsgGizmo {
	HelpInfo *	help;
	char *		name;
	char *		leftMsgText;
	char *		rightMsgText;
} MsgGizmo;

/* globals... */

extern GizmoClassRec	MsgGizmoClass[];

/* function prototypes... */

extern void	SetMsgGizmoTextLeft(Gizmo, char *);
extern void	SetMsgGizmoTextRight(Gizmo, char *);
extern Widget	GetMsgGizmoWidgetRight(Gizmo);

#endif /* _MsgGizmo_h */
