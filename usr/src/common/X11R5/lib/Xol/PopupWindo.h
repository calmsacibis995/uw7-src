#ifndef	NOIDENT
#ident	"@(#)popupwindo:PopupWindo.h	1.4"
#endif

#ifndef _PopupWindow_h
#define _PopupWindow_h

#include <X11/Shell.h>

typedef struct _PopupWindowShellClassRec	*PopupWindowShellWidgetClass;
typedef struct _PopupWindowShellRec		*PopupWindowShellWidget;

extern WidgetClass popupWindowShellWidgetClass;

#endif
