#pragma ident	"@(#)dtm:olwsm/wsm.h	1.38"

#ifndef _WSM_H
#define _WSM_H

#define WSM			"Workspace Manager"
#define DISPLAY			XtDisplay(InitShell)
#define SCREEN			XtScreen(InitShell)
#define ROOT			RootWindowOfScreen(SCREEN)
#define HELP(name)		name
#define MOD1			"Alt"

extern Widget		InitShell;
extern Widget		handleRoot;
extern Widget		workspaceMenu;
extern Widget		programsMenu;

extern char *		GetPath(char *);
extern void		FooterMessage(Widget, String, /* OlDefine, */ Boolean);
extern void		WSMExit( void );
extern int		ExecCommand(char * );
extern void		RefreshCB ( Widget, XtPointer, XtPointer);
extern void		CreateWorkSpaceMenu(Widget, Widget *, Widget *	);
extern void		RestartWorkspaceManager( void );
extern void		TouchPropertySheets( void );

#endif
