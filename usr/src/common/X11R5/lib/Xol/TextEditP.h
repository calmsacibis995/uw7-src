#ifndef NOIDENT
#ident	"@(#)textedit:TextEditP.h	1.29"
#endif

/*
 * TextEditP.h
 *
 */

#ifndef _TextEditP_h
#define _TextEditP_h

#define XVIEW_CLICK_MODE	1


typedef struct 
   {
   OlIm *   	im;
   int		im_key_index;
   int 		click_mode;
} TextEditClassPart;

#include <Xol/TextEdit.h>
#include <Xol/PrimitiveP.h>
#include <X11/keysym.h>
#include <DnD/OlDnDVCX.h>	/* don't include Xol because DnD is	*/
				/* GUI independent...			*/
#include <Xol/Util.h>
#include <Xol/OlCursors.h>
#include <X11/Xatom.h>

typedef struct _TextEditClassRec 
   {
   CoreClassPart        core_class;
   PrimitiveClassPart   primitive_class;
   TextEditClassPart    textedit_class;
   } TextEditClassRec;

typedef enum 
   {
   OlselectNull, OlselectPosition, OlselectChar, OlselectWord,
   OlselectLine, OlselectParagraph, OlselectAll
   } OlSelectType;

typedef enum
   {
   OlCopyDrag, OlMoveDrag
   } OlDragMode;

typedef enum
   {
   OlCursorOn, OlCursorOff, OlCursorBlinkOff, OlCursorBlinkOn
   } OlCursorState;

typedef struct _TextCursor
   {
   int             limit;
   int             width;
   int             height;
   int             xoffset;
   int             baseline;
   unsigned char * inbits;
   unsigned char * outbits;
   } TextCursor;

typedef Bufferof(int) WrapLocation;
typedef Bufferof(WrapLocation *) WrapTable;

typedef struct _DisplayTable
   {
   BufferElement *       p;
   int                   used;
   int                   size;
   char                  flag;
   TextLocation          wraploc;
   } DisplayTable;

typedef struct
   {
   TextPosition    displayPosition;
   TextPosition    cursorPosition;
   TextPosition    selectStart;
   TextPosition    selectEnd;
   TextPosition    anchor;
   Dimension       leftMargin;
   Dimension       rightMargin;
   Dimension       topMargin;
   Dimension       bottomMargin;
   OlEditMode      editMode;
   OlWrapMode      wrapMode;
   OlSourceType    sourceType;
   char *          source;
   XtCallbackList  motionVerification;
   XtCallbackList  modifyVerification;
   XtCallbackList  postModifyNotification;
   XtCallbackList  margin;
   XtCallbackList  buttons;
   XtCallbackList  keys;
   long            blinkRate;
   Boolean         updateState;
   Boolean         shouldBlink;
   Boolean 	   insertTab;
   Boolean         insertReturn;
   TabTable        tabs;
   OlRegisterFocusFunc register_focus;
   Boolean	   controlCaret;	/* display ASCII nonPrintables with caret? */
   Boolean	   preselect;		/* select all on (non-button) FOCUS_IN? */


   /* not in resource table */

   TextLocation    displayLocation;
   TextLocation    cursorLocation;

   /* private information */

   char *          clip_contents;
   char *          drag_contents;
   DisplayTable *  DT;
   int             DTsize;
   XtIntervalId    blink_timer;
   int             lineCount;
   int             linesVisible;
   int             charsVisible;
   int             lineHeight;
   int             charWidth;
   int             xOffset;
   int             maxX;
   Widget          vsb;
   Widget          hsb;
   TextBuffer *    textBuffer;
   GC              gc;
   GC              invgc;
   GC              insgc;
   int             save_offset;
   char            selectMode;
   unsigned char   dyn_flags;
   WrapTable *     wrapTable;
   unsigned int    mask;
   unsigned long   dynamic;
   Pixmap          CursorIn;
   Pixmap          CursorOut;
   TextCursor *    CursorP;
   OlCursorState   cursor_state;
   Position        cursor_x;
   Position        cursor_y;
   char            need_vsb;
   char            need_hsb;
   Dimension       prev_width;
   Dimension       prev_height;
#ifdef I18N
   OlIc *          ic;
#endif
   unsigned short  m_index;		/* for i18n */
   unsigned short  a_index;		/* for i18n */
   unsigned short  im_key_index;	/* for i18n */
   Cursor	   flyingCursor;
   OlDnDDropSiteID dropsiteid;		/* new - for drag and drop support */
   Atom		   transient;		/* private			   */
   int 		PanX;
   int 		PanY;
   } TextEditPart;

typedef struct _TextEditRec
   {
   CorePart           core;
   PrimitivePart      primitive;
   TextEditPart       textedit;
   } TextEditRec;

typedef struct _PasteSelectionRec{
    TextPosition	destination;	
    Boolean		cut;		/* Delete original selection and*/
                                        /* make new text primary selection*/
    XtPointer		text;		/* private Copy of text for cut */
} PasteSelectionRec;

#define ASCENT(widget)	OlFontAscent(widget->primitive.font, \
				      widget->primitive.font_list)

#define DESCENT(widget)	OlFontDescent(widget->primitive.font, \
				      widget->primitive.font_list)

#define FONTWID(widget) OlFontWidth(widget->primitive.font, \
				    widget->primitive.font_list)

#define HORIZONTAL_SHIFT(ctx) (8 * ENSPACE(ctx))
#define ENSPACE(widget)       _CharWidth(widget, 'n',               \
                                 widget->primitive.font_list, \
                                 widget->primitive.font, \
                                 widget->primitive.font->per_char, \
                                 widget->primitive.font->min_char_or_byte2, \
                                 widget->primitive.font->max_char_or_byte2, \
                                 widget->primitive.font->max_bounds.width)

#define FONTHT(widget)	OlFontHeight(widget->primitive.font, \
				      widget->primitive.font_list)

#define PAGE_T_GAP(widget)    ((int)(widget-> textedit.topMargin))
#define PAGE_B_GAP(widget)    ((int)(widget-> textedit.bottomMargin))
#define PAGE_R_GAP(widget)    ((int)(widget-> textedit.rightMargin))
#define PAGE_L_GAP(widget)    ((int)(widget-> textedit.leftMargin))
#define PAGE_T_MARGIN(widget) ((int)(widget-> textedit.topMargin))
#define PAGE_L_MARGIN(widget) ((int)(widget-> textedit.leftMargin))
#define PAGE_R_MARGIN(widget) ((int)(widget-> core.width) - PAGE_R_GAP(widget))
#define PAGE_B_MARGIN(widget) ((int)(widget-> core.height) - PAGE_B_GAP(widget))
#define PAGEHT(widget)        (PAGE_B_MARGIN(widget) - PAGE_T_MARGIN(widget))
#define PAGEWID(widget)       (PAGE_R_MARGIN(widget) - PAGE_L_MARGIN(widget))
#define PAGE_LINE_HT(widget)  (PAGEHT(widget) / FONTHT(widget))
#define LINES_VISIBLE(widget) (widget-> textedit.linesVisible)
#define HGAP(ctx)             (0)

#define PRINTLOC(L,T)   \
(void)fprintf(stderr,"%s:(%5d, %5d)\n", T, L.line, L.offset)

#define PRINTPOS(P,T)   \
(void)fprintf(stderr,"%s:(%5d)\n", T, P)

#define PRINTRECT(rect, T) \
(void)fprintf(stderr,"%s: %d,%d %d,%d\n",T,rect.x,rect.y,rect.width,rect.height)

#define PRINTSELECT(text, T) \
(void)fprintf(stderr,"%s: %d to %d\n",T,text-> selectStart, text-> selectEnd)

/* dynamics resources bit masks */
#define OL_B_TEXTEDIT_BG		(1 << 0)
#define OL_B_TEXTEDIT_FONTCOLOR		(1 << 1)

#define HAS_FOCUS(w)	(((TextEditWidget)(w))->primitive.has_focus == TRUE)

extern TextEditClassRec textEditClassRec;

extern void _OlTEPollPan OL_ARGS((XtPointer, 
				  XtIntervalId *));
extern void _OlTESelect OL_ARGS((TextEditWidget, 
				 XEvent *));
extern void _OlTEAdjust OL_ARGS((TextEditWidget,
				 XEvent *));
extern void _OlTEDragText OL_ARGS((TextEditWidget, 
				   TextEditPart *, 
				   TextPosition, 
				   OlDragMode, 
				   Boolean));
extern void _OlTEPaste OL_ARGS((TextEditWidget,
				XEvent *, 
				Atom,
				PasteSelectionRec *));
extern void PopupTextEditMenu OL_ARGS((TextEditWidget, OlVirtualName, Position,
				       Position, Position, Position));



extern void _OlmTECreateTextCursors OL_ARGS((TextEditWidget));
extern void _OloTECreateTextCursors OL_ARGS((TextEditWidget));
extern void _OlmTEButton OL_ARGS((Widget, OlVirtualEvent));
extern void _OloTEButton OL_ARGS((Widget, OlVirtualEvent));
extern void _OlmTEKey OL_ARGS((Widget, OlVirtualEvent));
extern void _OloTEKey OL_ARGS((Widget, OlVirtualEvent));

#endif
