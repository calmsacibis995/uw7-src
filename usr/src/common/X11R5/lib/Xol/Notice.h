#ifndef	NOIDENT
#ident	"@(#)notice:Notice.h	1.15"
#endif

#ifndef _Notice_h
#define _Notice_h

#include <Xol/Modal.h>		/* include superclasses's header file */

/*
 * OPEN LOOK(TM) Notice Widget
 */

/* Name			Type	Default	   Meaning
 * ----			----	-------	   -------
 * XtNtextArea		widget	 NULL	   control widget ID
 * XtNcontrolArea	widget	 NULL	   text widget ID
 * XtNemanateWidget	widget	 parent	   widget from which Notice emanates
 * XtNnoticeType	OlDefine OL_ERROR  Motif mode message type
 */

/* Class record pointer */
extern WidgetClass noticeShellWidgetClass;

/* C Widget type definition */
typedef struct _NoticeShellClassRec	*NoticeShellWidgetClass;
typedef struct _NoticeShellRec		*NoticeShellWidget;

#endif /* _Notice_h */
