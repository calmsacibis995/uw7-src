#ifndef NOIDENT
#ident	"@(#)textedit:TextEdit.h	1.8"
#endif

/*
 * TextEdit.h
 *
 */

#ifndef _TextEdit_h
#define _TextEdit_h

#include <Xol/buffutil.h>
#include <Xol/textbuff.h>
#include <Xol/Dynamic.h>
#include <DnD/OlDnDVCX.h>


typedef Dimension * TabTable;

typedef struct
   {
   Boolean           ok;
   TextPosition      current_cursor;
   TextPosition      new_cursor;
   TextPosition      select_start;
   TextPosition      select_end;
   } OlTextMotionCallData, *OlTextMotionCallDataPointer;

typedef struct
   {
   Boolean           ok;
   TextPosition      current_cursor;
   TextPosition      select_start;
   TextPosition      select_end;
   TextPosition      new_cursor;
   TextPosition      new_select_start;
   TextPosition      new_select_end;
   char *            text;
   int               text_length;
   } OlTextModifyCallData, *OlTextModifyCallDataPointer;

typedef struct
   {
   Boolean           requestor;
   TextPosition      new_cursor;
   TextPosition      new_select_start;
   TextPosition      new_select_end;
   char *            inserted;
   char *            deleted;
   TextLocation      delete_start;
   TextLocation      delete_end;
   TextLocation      insert_start;
   TextLocation      insert_end;
   TextPosition      cursor_position;
   } OlTextPostModifyCallData, *OlTextPostModifyCallDataPointer;

typedef enum 
   { OL_MARGIN_EXPOSED, OL_MARGIN_CALCULATED } OlTextMarginHint;

typedef struct
   {
   OlTextMarginHint  hint;
   XRectangle *      rect;
   } OlTextMarginCallData, *OlTextMarginCallDataPointer;

#undef OL_TEXT_READ
#undef OL_TEXT_EDIT
typedef enum
   { OL_TEXT_EDIT=66, OL_TEXT_READ=67 } OlEditMode;

#undef OL_WRAP_OFF
#undef OL_WRAP_ANY
#undef OL_WRAP_WHITE_SPACE
typedef enum
   { OL_WRAP_OFF=74, OL_WRAP_ANY=75, OL_WRAP_WHITE_SPACE=76 } OlWrapMode;

#undef OL_STRING_SOURCE
#undef OL_DISK_SOURCE
#undef OL_TEXT_BUFFER_SOURCE
typedef enum
   { OL_DISK_SOURCE=15, OL_STRING_SOURCE=64, OL_TEXT_BUFFER_SOURCE=99 } OlSourceType;

typedef struct _TextEditClassRec *TextEditWidgetClass;
typedef struct _TextEditRec      *TextEditWidget;

OLBeginFunctionPrototypeBlock

extern Boolean     
OlTextEditClearBuffer OL_ARGS((
	Widget
));

extern Boolean
OlTextEditCopyBuffer OL_ARGS((
	Widget , char **
));

extern Boolean
OlTextEditCopySelection OL_ARGS((
	Widget , int
));

extern Boolean
OlTextEditReadSubString OL_ARGS((
	Widget , char ** , TextPosition , TextPosition
));

extern Boolean
OlTextEditGetLastPosition OL_ARGS((
	Widget , TextPosition *
));

extern Boolean
OlTextEditGetCursorPosition OL_ARGS((
	Widget , TextPosition * , TextPosition * , TextPosition *
));

extern Boolean
OlTextEditInsert OL_ARGS((
	Widget , String , int
));

extern Boolean
OlTextEditSetCursorPosition OL_ARGS((
	Widget , TextPosition , TextPosition , TextPosition
));

extern Boolean
OlTextEditRedraw OL_ARGS((
	Widget
));

extern void
OlTextEditResize OL_ARGS((
	Widget , int , int
));

extern Boolean
OlTextEditUpdate OL_ARGS((
	Widget , Boolean
));

extern TextBuffer *
OlTextEditTextBuffer OL_ARGS((
	Widget
));

extern Boolean 
_OlTextEditTriggerNotify OL_ARGS(( 
	Widget,
	Window,
	Position,
        Position,
	Atom,
	Time,
	OlDnDDropSiteID,
	OlDnDTriggerOperation,
	Boolean,
	Boolean,
	XtPointer
));

OLEndFunctionPrototypeBlock

extern WidgetClass textEditWidgetClass;

#endif
