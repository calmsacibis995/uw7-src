#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:MsgGizmoP.h	1.1"
#endif

#ifndef _MsgGizmoP_h
#define _MsgGizmoP_h

#include "MsgGizmo.h"

typedef struct	_MsgGizmoP {
	char *	name;
	Widget	msgForm;
	Widget	leftMsgLabel;
	Widget	rightMsgLabel;
} MsgGizmoP;

#endif /* _MsgGizmoP_h */
