#ifndef NOIDENT
#ident	"@(#)textedit:TextUtil.h	1.4"
#endif

/*
 * TextUtil.h
 *
 */

#ifndef _TextUtil_h
#define _TextUtil_h

#include <Xol/OpenLook.h>
#include <Xol/buffutil.h>      /* for BufferElement */

extern Bool _IsGraphicsExpose();
extern Cursor _CreateCursor();
extern Cursor _CreateCursorFromBitmaps();
extern Time   _XtLastTimestampProcessed();

#ifdef I18N
extern int    _mbCopyOfwcString  OL_ARGS((char **, BufferElement *));
extern int    _mbCopyOfwcSegment OL_ARGS((char **, BufferElement *, int));
#endif

#endif
