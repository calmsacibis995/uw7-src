#ifndef	NOIDENT
#ident	"@(#)menu:Menu.h	1.9"
#endif

#ifndef _Ol_Menu_h
#define _Ol_Menu_h

/*************************************************************************
 *
 * Description:
 *		This is the "public" include file for the Menu Widget.
 *	This menu widget belongs to the OPEN LOOK (Tm - AT&T) Toolkit.
 *
 *****************************file*header********************************/

#include <X11/Shell.h>

extern WidgetClass			menuShellWidgetClass;
typedef struct _MenuShellClassRec *	MenuShellWidgetClass;
typedef struct _MenuShellRec *		MenuShellWidget;

#endif /* _Ol_Menu_h */
