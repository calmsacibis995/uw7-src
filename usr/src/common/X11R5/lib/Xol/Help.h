#ifndef	NOIDENT
#ident	"@(#)olhelp:Help.h	1.6"
#endif

#ifndef _Ol_Help_h_
#define _Ol_Help_h_

/*
 *************************************************************************
 *
 * Description:
 *		This is the "public" include file for the Help Widget
 *
 *****************************file*header********************************
 */

#include <Xol/ControlAre.h>

typedef struct _HelpClassRec *HelpWidgetClass;
typedef struct _HelpRec      *HelpWidget;

extern WidgetClass helpWidgetClass;
#endif /* _Ol_Help_h_ */
