#ifndef	NOIDENT
#ident	"@(#)notice:Modal.h	1.1"
#endif

#ifndef _Modal_h
#define _Modal_h

#include <X11/Shell.h>		/* include superclasses's header file */

/*
 * OPEN LOOK(TM) Modal Shell Widget
 */

/* Class record pointer */
extern WidgetClass modalShellWidgetClass;

/* C Widget type definition */
typedef struct _ModalShellClassRec	*ModalShellWidgetClass;
typedef struct _ModalShellRec		*ModalShellWidget;

#endif /* _Modal_h */
