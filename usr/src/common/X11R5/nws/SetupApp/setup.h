#ident	"@(#)setup.h	1.2"
#ifndef SETUP_H
#define SETUP_H

/*
//  This file contains the structures needed at the application-level (for
//  each invocation of the generic setup application).
*/


#include	<Xm/Xm.h>



/*
//  Used by setCursor(), which sets the pointer or wait (watch) cursors
*/

#define		C_WAIT		1	//  cursor "wait" font (watch cursor)
#define		C_POINTER	2	//  cursor "pointer" font (arrow)

#define		C_NO_FLUSH	0	//  used by setCursor(), don't XFlush()
#define		C_FLUSH		1	//  used by setCursor(), do an XFlush()



/*
//  Resource value data that are used by the SetupApp client,
//  and that can be changed by the user.
*/

typedef struct _rData
{
	int		cDebugLevel;	//  debug level for SetupApp client (us)
	String		cLogFile;	//  filename for the debugging stmts
	int		sDebugLevel;	//  setup API debug level
} RData, *RDataPtr;




/*  SetupApp-specific variables.					     */

struct AppStruct
{
	RData		rData;		//  resource data used by this app
	char		*realName;	//  real resolved path basename
	XtAppContext	appContext;	//  our application context
	Display		*display;	//  the display this app is running on
	Cursor		cursorWait;	//  the wait (watch) cursor
};


#include	"cDebug.h"		//  for SetupApp debugging definitions
					//  (needs to be after AppStruct def.)

/* 
//		Functions that may be used by others
*/

#include	<stdio.h>		//  setupDef.h needs it, but doesn't
					//  include it (SCOTT ERROR)
#include	"setupAPIs.h"		//  for setup API definitions

extern void	newSetupWin (setupObject_t *web);
extern void	setCursor (int cursorType, Window window, Boolean flush);
extern void	shellWidgetDestroy (Widget w,XtPointer clientData,XmAnyCallbackStruct *cbs);
extern void     exitSetupWin (setupObject_t *web);
extern void     destroySetupWin (setupObject_t *web);
extern void	shellWidgetDestroy(Widget w,XtPointer clientData,XmAnyCallbackStruct *cbs);

#endif	//  SETUP_H
