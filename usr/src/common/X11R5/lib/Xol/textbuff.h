#ifndef NOIDENT
#ident	"@(#)olmisc:textbuff.h	1.17"
#endif

/*
 * textbuff.h
 *
 */

#ifndef _textbuff_h
#define _textbuff_h
#include <stdio.h>

#ifdef I18N
#ifndef MEMUTIL
#include <stdlib.h>
#endif

#ifndef sun	/* or other porting that doesn't care I18N */
#include <widec.h>
#endif

#endif

#if defined(XlibSpecificationRelease)
#include "X11/Xfuncproto.h"
#endif

/* private private private private private private private private */
/* private private private private private private private private */
/* private private private private private private private private */
/* private private private private private private private private */

/* TextBuffer table sizes and increments */

#define FTALLOC         8
#define LTALLOC         2
#define PTALLOC         4
#define BTALLOC         8
#define MAXLPP         96
#define BLOCKSIZE    2048
#define PQLIMIT        16

/* public public public public public public public public */
/* public public public public public public public public */
/* public public public public public public public public */
/* public public public public public public public public */

/* standard types               */

typedef int TextPosition;
typedef int TextLine;
typedef int TextPage;
typedef int TextBlock;

/* status of a text buffer file */

typedef enum
   {
   NOTOPEN, READWRITE, READONLY, NEWFILE
   } TextFileStatus;

/* status of edit operations    */

typedef enum
   {
   EDIT_FAILURE, EDIT_SUCCESS
   } EditResult;

/* Scan routine return values   */
/* and direction definitions    */

typedef enum
   {
   SCAN_NOTFOUND, SCAN_WRAPPED, SCAN_FOUND, SCAN_INVALID
   } ScanResult;

/* SaveTextBuffer status        */

typedef enum
   {
   SAVE_FAILURE, SAVE_SUCCESS
   } SaveResult;

/* WriteTextBuffer status       */

typedef enum
   {
   WRITE_FAILURE, WRITE_SUCCESS
   } WriteResult;

/* TextEditOperations     */

#define TEXT_BUFFER_NOP                (0)
#define TEXT_BUFFER_DELETE_START_LINE  (1L<<0)
#define TEXT_BUFFER_DELETE_START_CHARS (1L<<1)
#define TEXT_BUFFER_DELETE_END_LINE    (1L<<2)
#define TEXT_BUFFER_DELETE_END_CHARS   (1L<<3)
#define TEXT_BUFFER_DELETE_JOIN_LINE   (1L<<4)
#define TEXT_BUFFER_DELETE_SIMPLE      (1L<<5)
#define TEXT_BUFFER_INSERT_SPLIT_LINE  (1L<<6)
#define TEXT_BUFFER_INSERT_LINE        (1L<<7)
#define TEXT_BUFFER_INSERT_CHARS       (1L<<8)

typedef Bufferof(TextBlock) BlockTable;

typedef struct
   {
   TextPage      pageindex;
   unsigned long timestamp;
   } PageQueue;

typedef struct
   {
   TextPosition  chars;
   TextLine      lines;
   TextPage      qpos;
   BlockTable *  dpos;
   } Page;

typedef struct
   {
   TextPage      pageindex;
   Buffer *      buffer;
   unsigned long userData;
   } Line;

typedef Bufferof(Page) PageTable;

typedef Bufferof(Line) LineTable;

typedef struct _TextLocation
   {
   TextLine        line;
   TextPosition    offset;
   BufferElement * buffer;
   } TextLocation;

typedef int TextUndoHint;

typedef struct _TextUndoItem
   {
   BufferElement *string;
   TextLocation	start;
   TextLocation end;
   TextUndoHint hint;
   } TextUndoItem;

typedef void (*TextUpdateFunction)();
typedef struct _TextUpdateCallback
   {
   TextUpdateFunction f;
   void *            d;
   } TextUpdateCallback;

typedef struct _TextBuffer
   {
   char *        filename;
   FILE *        tempfile;
   TextBlock     blockcnt;
   TextBlock     blocksize;
   LineTable     lines;
   PageTable     pages;
   BlockTable *  free_list;
   PageQueue     pqueue[PQLIMIT];
   TextPage      pagecount;
   TextPage      pageref;
   TextPage      curpageno;
   Buffer *      buffer;
   char          dirty;
   TextFileStatus       status;
   int                  refcount;
   TextUpdateCallback * update;
   TextUndoItem         deleted;
   TextUndoItem         insert;
   } TextBuffer;

/* `executable' macros          */

#define TextBufferUserData(text,line)  text-> lines.p[line].userData
#define TextBufferName(text)           (text-> filename)
#define TextBufferModified(text)       (text-> dirty)
#define TextBufferEmpty(text)          (text-> lines.used == 1 && \
                                        text-> lines.p[0].buffer-> used == 1)
#define TextBufferNamed(text)          (text-> filename != NULL)
#define LinesInTextBuffer(text)        (text-> lines.used)
#define LastTextBufferLine(text)       (text-> lines.used - 1)
#define LastCharacterInTextBufferLine(text, line)              \
   (text-> lines.p[line].buffer-> used - 1)
#define LengthOfTextBufferLine(text, line) (text-> lines.p[line].buffer-> used)

#define SameTextLocation(x,y)         (x.line == y.line && x.offset == y.offset)

/* extern interfaces            */

	/* XlibSpecificationRelease is introduced in X11R5,
	 * so we have to define NeedFunctionPrototypes
	 * when necessary... 
	 */
#if defined(XlibSpecificationRelease)

_XFUNCPROTOBEGIN

#else

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define NeedFunctionPrototypes	1
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif /* defined(__cplusplus) || defined(c_plusplus) */
#endif /* defined (__STDC__) defined(__cplusplus) || defined(c_plusplus) */

#endif /* defined(XlibSpecificationRelease) */

#define CONST

extern TextBuffer *
AllocateTextBuffer (
#if NeedFunctionPrototypes
	CONST char * , TextUpdateFunction , caddr_t
#endif
);

extern TextBuffer *
ReadFileIntoTextBuffer (
#if NeedFunctionPrototypes
	CONST char * , TextUpdateFunction , caddr_t
#endif
);

extern TextBuffer *
ReadStringIntoTextBuffer (
#if NeedFunctionPrototypes
	char * , TextUpdateFunction , caddr_t
#endif
);

extern TextBuffer *
wcReadStringIntoTextBuffer (
#if NeedFunctionPrototypes
	BufferElement * , TextUpdateFunction , caddr_t
#endif
);

extern BufferElement *
wcGetTextBufferLocation (
#if NeedFunctionPrototypes
	TextBuffer * , TextLine , TextLocation *
#endif
);

extern char *
GetTextBufferLocation (
#if NeedFunctionPrototypes
	TextBuffer * , TextLine , TextLocation *
#endif
);

extern ScanResult
ForwardScanTextBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , CONST char * , TextLocation *
#endif
);

extern ScanResult
BackwardScanTextBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , CONST char * , TextLocation *
#endif
);

extern EditResult
ReplaceBlockInTextBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation * , TextLocation * , char * ,
	TextUpdateFunction , caddr_t
#endif
);

extern EditResult
wcReplaceBlockInTextBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation * , TextLocation * , BufferElement * ,
	TextUpdateFunction , caddr_t
#endif
);

extern EditResult
ReplaceCharInTextBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation * , int ,
	TextUpdateFunction , caddr_t
#endif
);

extern EditResult
InsertLineIntoTextBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , TextLine , Buffer *
#endif
);

extern TextLocation
IncrementTextBufferLocation (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation , TextLine , TextPosition
#endif
);

extern TextLocation
LocationOfPosition (
#if NeedFunctionPrototypes
	TextBuffer * , TextPosition
#endif
);

extern TextLine
LineOfPosition (
#if NeedFunctionPrototypes
	TextBuffer * , TextPosition
#endif
);

extern TextPosition
PositionOfLocation (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation
#endif
);

extern TextPosition
PositionOfLine (
#if NeedFunctionPrototypes
	TextBuffer * , TextLine
#endif
);

extern TextPosition
LastTextBufferPosition (
#if NeedFunctionPrototypes
	TextBuffer *
#endif
);

extern TextLocation
LastTextBufferLocation (
#if NeedFunctionPrototypes
	TextBuffer *
#endif
);

extern TextLocation
StartCurrentTextBufferWord (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation
#endif
);

extern TextLocation
EndCurrentTextBufferWord (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation
#endif
);

extern TextLocation
PreviousTextBufferWord (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation
#endif
);

extern TextLocation
NextTextBufferWord (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation
#endif
);

extern TextLocation
NextLocation (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation
#endif
);

extern TextLocation
PreviousLocation (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation
#endif
);

extern char *
GetTextBufferLine (
#if NeedFunctionPrototypes
	TextBuffer * , TextLine
#endif
);

extern int
GetTextBufferChar (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation
#endif
);

extern int
CopyTextBufferBlock (
#if NeedFunctionPrototypes
	TextBuffer * , char * , TextPosition , TextPosition
#endif
);

extern char *
GetTextBufferBlock (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation , TextLocation
#endif
);

extern Buffer *
GetTextBufferBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , TextLine
#endif
);

extern SaveResult
SaveTextBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , char *
#endif
);

extern TextBuffer *
ReadPipeIntoTextBuffer (
#if NeedFunctionPrototypes
	FILE * , TextUpdateFunction , caddr_t
#endif
);

extern WriteResult
WriteTextBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , FILE *
#endif
);

extern BufferElement *
wcGetTextBufferLine (
#if NeedFunctionPrototypes
	TextBuffer * , TextLine
#endif
);

extern int
wcCopyTextBufferBlock (
#if NeedFunctionPrototypes
	TextBuffer * , BufferElement * , TextPosition , TextPosition
#endif
);

extern BufferElement *
wcGetTextBufferBlock (
#if NeedFunctionPrototypes
	TextBuffer * , TextLocation , TextLocation
#endif
);

extern void
RegisterTextBufferUpdate (
#if NeedFunctionPrototypes
	TextBuffer * , TextUpdateFunction , caddr_t
#endif
);

extern int
UnregisterTextBufferUpdate (
#if NeedFunctionPrototypes
	TextBuffer * , TextUpdateFunction , caddr_t
#endif
);

extern void
RegisterTextBufferScanFunctions (
#if NeedFunctionPrototypes
	CONST char *(*) ( CONST char * , CONST char * , CONST char * ) ,
	CONST char *(*) ( CONST char * , CONST char * , CONST char * )
#endif
);

extern void
RegisterTextBufferWordDefinition (
#if NeedFunctionPrototypes
	int (*) ( int )
#endif
);

extern int
_StringLength (
#if NeedFunctionPrototypes
	BufferElement *
#endif
);

extern BufferElement *
_StringCopy (
#if NeedFunctionPrototypes
	BufferElement * , BufferElement *
#endif
);

extern void
FreeTextBuffer (
#if NeedFunctionPrototypes
	TextBuffer * , TextUpdateFunction , caddr_t
#endif
);

#if defined(XlibSpecificationRelease)
_XFUNCPROTOEND
#else
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* defined(XlibSpecificationRelease) */

#endif
