#ifndef	NOIDENT
#ident	"@(#)oldtext:TextPane.h	1.8"
#endif

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        TextPane.h
 **
 **   Project:     X Widgets
 **
 **   Description: TextPane widget public include file
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1987, 1988 by Digital Equipment Corporation, Maynard,
 **             Massachusetts, and the Massachusetts Institute of Technology,
 **             Cambridge, Massachusetts
 **   
 *****************************************************************************
 *************************************<+>*************************************/

#ifndef _OlTextPane_h
#define _OlTextPane_h

#include <Xol/Primitive.h>		/* include superclasses' header */

#define OlSetArg(arg, n, v) \
     { Arg *_OlSetArgTmp = &(arg) ;\
       _OlSetArgTmp->name = (n) ;\
       _OlSetArgTmp->value = (XtArgVal) (v) ;}


/*************************************************************************
*
*  Structures used in TextPane function calls
*
*************************************************************************/

typedef struct {
	char *			p;
	OlTextPosition		pos;
} OlTextMark;

#define XtNmark			"mark"

extern void			_AsciiDisplayMark();
extern void			OlTextGetSelectionPos();
extern void			OlTextSetSelection();
extern void			_OlDisplayMark();
extern char *			_OlMark();

extern WidgetClass textPaneWidgetClass;

typedef struct _TextPaneClassRec *TextPaneWidgetClass;
typedef struct _TextPaneRec      *TextPaneWidget;

typedef struct {
    XtResource      *resources;
    Cardinal        resource_num;
    int		    (*read)();
    int		    (*replace)();
    OlTextPosition  (*getLastPos)();
    int		    (*setLastPos)();
    OlTextPosition  (*scan)();
    OlDefine        (*editType)();
    Boolean         (*check_data)();
    void            (*destroy)();
    int		    *data;       
    int             number_of_lines;
    } OlTextSource, *OlTextSourcePtr;

/* this wouldn't be here if source and display (still called
   sink here) were properly separated, classed and subclassed
   */

typedef short TextFit ;
#define tfNoFit			0x01
#define tfIncludeTab		0x02
#define tfEndText		0x04
#define tfNewline		0x08
#define tfWrapWhiteSpace	0x10
#define tfWrapAny		0x20

typedef struct {
    TextPaneWidget parent;
    XFontStruct *font;
    int foreground;
    XtResource *resources;
    Cardinal resource_num;
    int (*display)();
    int (*insertCursor)();
    int (*clearToBackground)();
    int (*findPosition)();
    TextFit (*textFitFn)();
    int (*findDistance)();
    int (*resolve)();
    int (*maxLines)();
    int (*maxHeight)();
    Boolean (*check_data)();
    void (*destroy)();
    int LineLastWidth ;
    OlTextPosition LineLastPosition ;
    int *data;
    } OlTextSink, *OlTextSinkPtr;

/* other stuff */

#define wordBreak		0x01
#define scrollVertical		0x02
#define scrollHorizontal	0x04
#define scrollOnOverflow	0x08
#define resizeWidth		0x10
#define resizeHeight		0x20
#define editable		0x40

extern void _OlScrollText();

/*************************************************************************
*  
*  Extern Source and Sink Create/Destroy functions
*
*************************************************************************/

extern OlTextSink *OlAsciiSinkCreate();
    /* Widget    w       */
    /* ArgList   args    */
    /* Cardinal num_args */

extern OlTextSource *OlDiskSourceCreate();
    /* Widget   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern void OlDiskSourceDestroy();
    /* OlTextSource *src */

extern OlTextSource *OlStringSourceCreate(); 
    /* Widget   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern void OlStringSourceDestroy();
    /* OlTextSource *src */

#endif
/* DON'T ADD STUFF AFTER THIS #endif */
