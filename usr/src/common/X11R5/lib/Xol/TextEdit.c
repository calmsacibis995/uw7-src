#ifndef NOIDENT
#ident	"@(#)textedit:TextEdit.c	1.150"
#endif

/*
 * Description:  This file contains the TextEdit widget code.  The
 *               TextEdit widget allows editing/display of string and disk
 *               sources.
 */

#define WORKS_SORT_OF_LIKE_XVIEW
/* #undef WORKS_SORT_OF_LIKE_XVIEW */

#include <X11/memutil.h>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/buffutil.h>
#include <Xol/textbuff.h>		/* must follow IntrinsicP.h */

#include <Xol/OpenLookP.h>
#include <DnD/OlDnDVCX.h>	/* don't include Xol because DnD is	*/

#include <Xol/DynamicP.h>
#include <Xol/TextField.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>
#ifndef WORKS_SORT_OF_LIKE_XVIEW
#include <Xol/TextField.h>
#endif
#include <Xol/PopupMenu.h>
#include <Xol/FButtons.h>
#include <Xol/Stub.h>
#include <Xol/FooterPane.h>

#include <Xol/TextEditP.h>
#include <Xol/TextEPos.h>
#include <Xol/TextUtil.h>
#include <Xol/TextDisp.h>
#include <Xol/TextWrap.h>

#define ClassName TextEdit
#include <Xol/NameDefs.h>

/* get this from ConvertersI.h...	*/
#define ConversionDone(type,value) \
{								\
	if (to->addr) {						\
		if (to->size < sizeof(type)) {			\
			to->size = sizeof(type);		\
			return (False);				\
		}						\
		*(type *)(to->addr) = (value);			\
	} else {						\
		static type		static_value;		\
								\
		static_value = (value);				\
		to->addr = (XtPointer)&static_value;		\
	}							\
	to->size = sizeof(type);				\
} return (True)

#define APPL_FG      (1L<<0)
#define APPL_BG      (1L<<1)
#define APPL_FOCUS   (1L<<2)
#define VORDER(x,y)  ((x) ? (x) : (y))

typedef struct _item
   {
   XtArgVal	label;
   XtArgVal	sensitive;
   XtArgVal	mnemonic;
   } item;

#define UNDO_ITEM   0
#define CUT_ITEM    1
#define COPY_ITEM   2
#define PASTE_ITEM  3
#define DELETE_ITEM 4


static void		Delete OL_ARGS((
 TextEditWidget ctx,
 OlVirtualName keyarg,
 int unused
));
static Boolean	_CheckForScrollbarStateChange OL_ARGS((
 TextEditWidget ctx,
 TextEditPart * text
));

static Boolean	_AdjustDisplayLocation OL_ARGS((
 TextEditWidget ctx,
 TextEditPart * text,
 WrapTable *    wrapTable,
 TextLocation * currentDP,
 int            page
));
static void	UpdateDisplay OL_ARGS((
 TextEditWidget ctx,
 TextBuffer * textBuffer,
 int requestor
));
static TextPosition ValidatePosition OL_ARGS((
 TextPosition min,
 TextPosition pos,
 TextPosition last,
 char *       name
));
static void	ValidatePositions OL_ARGS((
TextEditPart *text
));
static void	ValidateMargins OL_ARGS((
 TextEditWidget ctx,
 TextEditPart * text
));
static void	SetDefaultLeftRightMargin OL_ARGS((
 Widget			w,
 int			offset,		/*NOTUSED*/
 XrmValue *		value
));
static void	SetDefaultBottomMargin OL_ARGS((
 Widget			w,
 int			offset,		/*NOTUSED*/
 XrmValue *		value
));
static void	GetGCs OL_ARGS((
 TextEditWidget ctx,
 TextEditPart * text
));
static void	ScrolledWindowInterface OL_ARGS((
 TextEditWidget ctx,
 OlSWGeometries * gg
));
static void	ResizeScrolledWindow OL_ARGS((
 TextEditWidget ctx,
 ScrolledWindowWidget sw,
 OlSWGeometries * gg,
 Dimension width,
 Dimension height,
 int resize
));
static void SetScrolledWindowScrollbars OL_ARGS((
 TextEditWidget ctx,
 Dimension      width,
 Dimension      height
));
static void VSBCallback OL_ARGS((
 Widget w,
 XtPointer client_data,
 XtPointer call_data
));
static void HSBCallback OL_ARGS((
 Widget w,
 XtPointer client_data,
 XtPointer call_data
));
static void ClassInitialize OL_NO_ARGS();
static void ClassPartInitialize OL_ARGS((
 WidgetClass class
));
static void Initialize OL_ARGS((
 Widget request,
 Widget new,
 ArgList args,
 Cardinal * num_args
));
static void Realize OL_ARGS((
 Widget w,
 XtValueMask * valueMask,
 XSetWindowAttributes * attributes
));
static Widget RegisterFocus OL_ARGS((
 Widget
));
static void Resize OL_ARGS((
 Widget          w
));
static void Redisplay OL_ARGS((
 Widget w,
 XEvent * event,
 Region region
));
static void Destroy OL_ARGS((
 Widget w
));
static Boolean SetValues OL_ARGS((
 Widget current,
 Widget request,
 Widget new,
 ArgList args,
 Cardinal * num_args
));
static Boolean ActivateWidget OL_ARGS((
 Widget w,
 OlVirtualName type,
 XtPointer call_data
));
static void FocusHandler OL_ARGS((
 Widget w,
 OlDefine highlight_type
));
static void LoseClipboard OL_ARGS((
 Widget w,
 Atom * atom
));
static Boolean ConvertClipboard OL_ARGS((
 Widget w,
 Atom * selection,
 Atom * target,
 Atom * type_return,
 XtPointer * value_return,
 unsigned long * length_return,
 int *           format_return
));
static void Paste2 OL_ARGS((
 Widget w,
 XtPointer client_data,
 Atom * selection,
 Atom * type,
 XtPointer value,
 unsigned long * length,
 int * format
));
static void PrimaryPaste OL_ARGS((
 Widget w,
 XtPointer client_data,
 Atom * selection,
 Atom * type,
 XtPointer value,
 unsigned long * length,
 int * format
));
#ifdef NOT_USED
static void 
PastePrimaryText OL_ARGS((
  TextEditWidget ctx,
  String         buffer,
  int            length,
  TextPosition   destination			  
));
#endif
static void PrimaryCut OL_ARGS((
 Widget w,
 XtPointer client_data,
 Atom * selection,
 Atom * type,
 XtPointer value,
 unsigned long * length,
 int * format
));
static void PollMouse OL_ARGS((
 XtPointer	client_data,
 XtIntervalId *	id
));
static void TextDropOnWindow OL_ARGS((
 Widget ctx,
 TextEditPart * text,
 OlDragMode drag_mode,
 OlDnDDropStatus drop_status,
 OlDnDDestinationInfoPtr dst_info,
 OlDnDDragDropInfoPtr	root_info
));
static void Message OL_ARGS((
 Widget w,
 OlVirtualEvent ve
));
static Boolean InArgList OL_ARGS((
String	resource,
ArgList	arglist,
Cardinal num_args
));

      /* dynamically linked procedures */
static void (*_olmTECreateTextCursors) OL_ARGS((TextEditWidget));


#ifdef NOT_USE
static void             DynamicInitialize();
static XtArgsFunc       DynamicHook();
static void             DynamicHandler();
#endif


static void ReceivePasteMessage OL_ARGS((Widget,
					 XEvent *,
					 String *,
					 Cardinal *
));
static void SendPasteMessage OL_ARGS((TextEditWidget,
				      Window,
				      Position,
				      Position,
				      int
));
static void PopdownTextEditMenuCB OL_ARGS((Widget,
					 XtPointer,
					 XtPointer
));
static void MenuUndo OL_ARGS((Widget,
			      XtPointer,
			      XtPointer
));
static void MenuCut OL_ARGS((Widget,
		     XtPointer,
		     XtPointer
));
static void MenuCopy OL_ARGS((Widget,
		      XtPointer,
		      XtPointer
));
static void MenuPaste OL_ARGS((Widget,
			       XtPointer,
			       XtPointer
));
static void MenuDelete OL_ARGS((Widget,
				XtPointer,
				XtPointer
));
static void IgnorePropertyNotify OL_ARGS((Widget,
					  XEvent *,
					  String *,
					  Cardinal *
));
static void SelectAll OL_ARGS((Widget));
static void DeselectAll OL_ARGS((Widget));


   /* OPEN LOOK mode resource defaults -- 
		see ClassInitialize for Motif defaults */
static char DefaultShadowThickness[] = "0 points";
static int DefaultBlinkRate = 1000;
#define DefaultMotifShadowThickness '2'

#ifdef I18N
static char defaultTextEditTranslations[] = "\
   <FocusIn>:  OlAction()  \n\
   <FocusOut>: OlAction()  \n\
   <Key>:      OlAction()  \n\
   <BtnDown>:  OlAction()  \n\
   <BtnUp>: OlAction()  \n\
   <Message>:  OlAction()  \n\
   <Prop>:     property()  \n\
";
#else
static char defaultTextEditTranslations[] = "\
   <FocusIn>:  OlAction()  \n\
   <FocusOut>: OlAction()  \n\
   <Key>:      OlAction()  \n\
   <BtnDown>:  OlAction()  \n\
   <BtnUp>: OlAction()  \n\
   <Message>:  message()   \n\
   <Prop>:     property()  \n\
";
#endif

static XtActionsRec textEditActionsTable [] = 
   {
      {"property",  IgnorePropertyNotify},
#ifndef I18N
       "message",   ReceivePasteMessage},
#endif
      {NULL, NULL}
   };

static OlEventHandlerRec
event_procs[] = {
	{ KeyPress,	NULL	},	/* see ClassInitialize */
	{ ButtonPress,	NULL	}	/* see ClassInitialize */
#ifdef I18N
   ,{ ClientMessage,Message  }
#endif
};

#define BYTE_OFFSET	XtOffsetOf(TextEditRec, textedit.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_TEXTEDIT_BG, NULL },
{ { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_TEXTEDIT_FONTCOLOR, 
    NULL },
};
#undef BYTE_OFFSET

#define OFFSET(field)    XtOffsetOf(TextEditRec, textedit.field)

static XtResource resources[] = 
{
  { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
    XtOffsetOf(TextEditRec, core.background_pixel), XtRString,
    XtDefaultBackground},

  { XtNtabTable, XtCTabTable, XtRPointer, sizeof(TabTable),
    OFFSET(tabs), XtRPointer, NULL},

  { XtNcharsVisible, XtCCharsVisible, XtRInt, sizeof(int),
    OFFSET(charsVisible), XtRImmediate, (XtPointer)50},

  { XtNcontrolCaret, XtCControlCaret, XtRBoolean, sizeof(Boolean),
    OFFSET(controlCaret), XtRImmediate, (XtPointer) TRUE},

  { XtNlinesVisible, XtCLinesVisible, XtRInt, sizeof(int),
    OFFSET(linesVisible), XtRImmediate, (XtPointer)16},

  { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel),
    XtOffsetOf(TextEditRec, primitive.font_color), XtRString, 
    XtDefaultForeground},

  { XtNdisplayPosition, XtCTextPosition, XtRInt, sizeof (TextPosition), 
    OFFSET(displayPosition), XtRImmediate, (XtPointer)0},

  { XtNcursorPosition, XtCTextPosition, XtRInt, sizeof(TextPosition), 
    OFFSET(cursorPosition), XtRImmediate, (XtPointer)0},

  { XtNselectStart, XtCTextPosition, XtRInt, sizeof(TextPosition), 
    OFFSET(selectStart), XtRImmediate, (XtPointer)0},

  { XtNselectEnd, XtCTextPosition, XtRInt, sizeof(TextPosition), 
    OFFSET(selectEnd), XtRImmediate, (XtPointer)0},

  { XtNleftMargin, XtCMargin, XtRDimension, sizeof (Dimension), 
    OFFSET(leftMargin), XtRCallProc, (XtPointer)SetDefaultLeftRightMargin},

  { XtNrightMargin, XtCMargin, XtRDimension, sizeof (Dimension), 
    OFFSET(rightMargin), XtRCallProc, (XtPointer)SetDefaultLeftRightMargin},

  { XtNtopMargin, XtCMargin, XtRDimension, sizeof (Dimension), 
    OFFSET(topMargin), XtRImmediate, (XtPointer)4},

  { XtNbottomMargin, XtCMargin, XtRDimension, sizeof (Dimension), 
    OFFSET(bottomMargin), XtRCallProc, (XtPointer)SetDefaultBottomMargin},

  { XtNeditType, XtCEditType, XtROlEditMode, sizeof (OlEditMode),
    OFFSET(editMode), XtRImmediate, (XtPointer)OL_TEXT_EDIT},

  { XtNwrapMode, XtCWrapMode, XtROlWrapMode, sizeof (OlWrapMode),
    OFFSET(wrapMode), XtRImmediate, (XtPointer)OL_WRAP_WHITE_SPACE},

  { XtNsourceType, XtCSourceType, XtROlSourceType, sizeof(OlSourceType),
    OFFSET(sourceType), XtRImmediate, (XtPointer)OL_STRING_SOURCE},

  { XtNsource, XtCSource, XtRString, sizeof (String), 
    OFFSET(source), XtRString, NULL},

  { XtNbuttons, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(buttons), XtRCallback, (XtPointer) NULL},

  { XtNkeys, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(keys), XtRCallback, (XtPointer) NULL},

  { XtNmotionVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(motionVerification), XtRCallback, (XtPointer) NULL},

  { XtNmodifyVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(modifyVerification), XtRCallback, (XtPointer) NULL},

  { XtNpostModifyNotification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(postModifyNotification), XtRCallback, (XtPointer) NULL},

  { XtNblinkRate, XtCBlinkRate, XtRInt, sizeof (long),  /* NOTE: no XtRLong!!! */
    OFFSET(blinkRate), XtRInt, (XtPointer)&DefaultBlinkRate},

  { XtNmargin, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(margin), XtRCallback, (XtPointer) NULL},

  { XtNregisterFocusFunc, XtCRegisterFocusFunc, XtRFunction, sizeof(OlRegisterFocusFunc),
    OFFSET(register_focus), XtRFunction, (XtPointer) NULL },

  { XtNinsertTab, XtCInsertTab, XtRBoolean, sizeof(Boolean),
    OFFSET(insertTab), XtRImmediate, (XtPointer) TRUE},

  /*
   * XtNinsertReturn is a private resource that is used only by TextField
   * to tell textEdit widget not to consume or beep on a OL_RETURN key.
   */
  { XtNinsertReturn, XtCInsertReturn, XtRBoolean, sizeof(Boolean),
    OFFSET(insertReturn), XtRImmediate, (XtPointer) TRUE},

  { XtNdragCursor, XtCCursor, XtRCursor, sizeof(Cursor),
    OFFSET(flyingCursor), XtRImmediate, (XtPointer) None},
 
  { XtNdropSiteID, XtCReadOnly, XtRPointer, sizeof(OlDnDDropSiteID),
    OFFSET(dropsiteid), XtRImmediate, (XtPointer)NULL},
  { XtNshadowThickness, XtCShadowThickness, XtRDimension, sizeof(Dimension),
    XtOffsetOf(TextEditRec, primitive.shadow_thickness),
	 XtRString, (XtPointer) &DefaultShadowThickness
  },
  { XtNpreselect, XtCPreselect, XtRBoolean, sizeof(Boolean),
    OFFSET(preselect), XtRImmediate, (XtPointer) FALSE},

};
#undef OFFSET

TextEditClassRec textEditClassRec = {
        {
        /* superclass          */      (WidgetClass) &primitiveClassRec,
        /* class_name          */      "TextEdit",
        /* widget_size         */      sizeof(TextEditRec),
        /* class_initialize    */      ClassInitialize,
        /* class_part_init     */      ClassPartInitialize,
        /* class_inited        */      FALSE,
        /* initialize          */      Initialize,
        /* initialize_hook     */      NULL,
        /* realize             */      Realize,
        /* actions             */      textEditActionsTable,
        /* num_actions         */      XtNumber(textEditActionsTable),
        /* resources           */      resources,
        /* num_ resource       */      XtNumber(resources),
        /* xrm_class           */      NULLQUARK,
        /* compress_motion     */      TRUE,
        /* compress_exposure   */      TRUE,
        /* compress_enterleave */      TRUE,
        /* visible_interest    */      FALSE,
        /* destroy             */      Destroy,
        /* resize              */      Resize,
        /* expose              */      Redisplay,
        /* set_values          */      SetValues,
        /* set_values_hook     */      NULL,
        /* set_values_almost   */      XtInheritSetValuesAlmost,
        /* get_values_hook     */      NULL,
        /* accept_focus        */      XtInheritAcceptFocus,
        /* version             */      XtVersion,
        /* callback_private    */      NULL,
        /* tm_table            */      defaultTextEditTranslations
        },
        {
        /* primitive class 	*/
	/* focus_on_select	*/	False,
        /* highlight_handler	*/	FocusHandler,
        /* traversal_handler	*/	NULL,
        /* register_focus	*/	RegisterFocus,
        /* activate		*/	ActivateWidget,
        /* event_procs		*/	event_procs,
        /* num_event_procs	*/      XtNumber(event_procs),
        /* version		*/	OlVersion,
        /* extension		*/	NULL,
	/* dyn_data		*/	{ dyn_res, XtNumber(dyn_res) },
	/* transparent_proc	*/	NULL,
        },
        {
        /* TextEdit fields */
        NULL,
        }
};

WidgetClass textEditWidgetClass = (WidgetClass)&textEditClassRec;


#ifdef DEBUG
static void
PrintArgs(arg, n)
Arg arg[];
int n;
{
int i;

for (i = 0; i < n; i++)
   fprintf(stderr,"name = %s value = %d\n", arg[i].name, arg[i].value);
} /* end of PrintArgs */
static void
PrintWrapTable(textBuffer, wrapTable)
TextBuffer * textBuffer;
WrapTable * wrapTable;
{
TextLocation loc;
int len;

fprintf(stderr,"------WRAP TABLE-----\n");
fprintf(stderr,"wrapTable.esize = %d\n", wrapTable-> esize);
fprintf(stderr,"wrapTable.size  = %d\n", wrapTable-> size);
fprintf(stderr,"wrapTable.used  = %d\n", wrapTable-> used);

fprintf(stderr,"line   offset OFFSET length end    length\n");

for (loc.line = 0; loc.line < wrapTable-> used; loc.line++)
   {
   len = LastCharacterInTextBufferLine(textBuffer, loc.line);
   for (loc.offset = 0; loc.offset < wrapTable-> p[loc.line]-> used; loc.offset++)
      fprintf(stderr, "%6d %6d %6d %6d %6d %6d\n",
      loc.line, loc.offset, 
      _WrapLineOffset(wrapTable, loc), 
      _WrapLineLength(textBuffer, wrapTable, loc),
      _WrapLineEnd(textBuffer, wrapTable, loc), len);
   }

} /* end of PrintWrapTable */
#endif
/*
 * Delete
 *
 */

static void
Delete OLARGLIST((ctx, keyarg, unused))
    OLARG(TextEditWidget, ctx)
    OLARG(OlVirtualName, keyarg)
    OLGRA(int, unused)
{
TextEditPart * text = &ctx-> textedit;
TextBuffer * textBuffer = text-> textBuffer;
TextPosition position;
TextLocation loc;
TextLocation newloc;
TextLocation oldloc;
TextPosition newpos;

if (text-> selectStart < text-> selectEnd)
   OlTextEditInsert((Widget)ctx, "", 0);
else
   {
   switch(keyarg)
      {
      case OL_DELCHARFWD:
         position = text-> selectEnd + 1;
         if (position <= LastTextBufferPosition(textBuffer))
            {
            text-> selectEnd = position;
            OlTextEditInsert((Widget)ctx, "", 0);
            }
         break;
      case OL_DELCHARBAK:
         position = text-> selectStart - 1;
         if (position >= 0)
            {
            text-> selectStart = position;
            OlTextEditInsert((Widget)ctx, "", 0);
            }
         break;
      case OL_DELWORDFWD:
         oldloc = LocationOfPosition(textBuffer, text-> selectEnd);
         newloc = EndCurrentTextBufferWord(textBuffer, oldloc);
         if (SameTextLocation(newloc, oldloc))
            {
            newloc = NextTextBufferWord(textBuffer, newloc);
            newloc = EndCurrentTextBufferWord(textBuffer, newloc);
            }
         newpos = PositionOfLocation(textBuffer, newloc);
         if (newpos > text-> selectEnd)
            {
            text-> selectEnd = PositionOfLocation(textBuffer, newloc);
            OlTextEditInsert((Widget)ctx, "", 0);
            }
         break;
      case OL_DELWORDBAK:
         oldloc = LocationOfPosition(textBuffer, text-> selectStart);
         newloc = StartCurrentTextBufferWord(textBuffer, oldloc);
         if (SameTextLocation(newloc, oldloc))
            newloc = PreviousTextBufferWord(textBuffer, newloc);
         newpos = PositionOfLocation(textBuffer, newloc);
         if (newpos < text-> selectStart)
            {
            text-> selectStart = PositionOfLocation(textBuffer, newloc);
            OlTextEditInsert((Widget)ctx, "", 0);
            }
         break;
      case OL_DELLINEBAK:
         loc = LocationOfPosition(textBuffer, text-> selectStart);
         loc.offset = 0;
         position = PositionOfLocation(textBuffer, loc);
         if (position < text-> selectStart)
            {
            text-> selectStart = position;
            OlTextEditInsert((Widget)ctx, "", 0);
            }
         break;
      case OL_DELLINEFWD:
         loc = LocationOfPosition(textBuffer, text-> selectEnd);
         loc.offset = LastCharacterInTextBufferLine(textBuffer, loc.line);
         position = PositionOfLocation(textBuffer, loc);
         if (position > text-> selectEnd)
            {
            text-> selectEnd = position;
            OlTextEditInsert((Widget)ctx, "", 0);
            }
         break;
      case OL_DELLINE:
         if (!TextBufferEmpty(textBuffer))
            {
            loc = LocationOfPosition(textBuffer, text-> selectStart);
            loc.offset = 0;
            text-> selectStart = PositionOfLocation(textBuffer, loc);

            loc = LocationOfPosition(textBuffer, text-> selectEnd);
            loc.offset = LastCharacterInTextBufferLine(textBuffer, loc.line);
            text-> selectEnd = PositionOfLocation(textBuffer, loc);

            /*
             * include the newline (if it's there)
             */

            position = text-> selectEnd + 1;
            if (position <= LastTextBufferPosition(textBuffer))
               text-> selectEnd = position;

            OlTextEditInsert((Widget)ctx, "", 0);
            }
         break;
      default:
         return;
      }
   }
   
} /* end of Delete */
/*
 * AdjustPosition
 *
 */

static TextPosition
AdjustPosition(current, dlen, ilen, editstart, delend)
TextPosition current;
int dlen;
int ilen;
TextPosition editstart;
TextPosition delend;
{

if (current < editstart)
   ;
else
   {
   if (current > delend)
      current -= dlen;
   else
      current = editstart;
   current += ilen;
   }

return (current);

} /* end of AdjustPosition */
/*
 * _AdjustDisplayLocation
 *
 */

static Boolean
_AdjustDisplayLocation(ctx, text, wrapTable, currentDP, page)
TextEditWidget ctx;
TextEditPart * text;
WrapTable *    wrapTable;
TextLocation * currentDP;
int            page;
{
int            diff;
TextLocation   min;
TextLocation   max;
Boolean        retval;

if (text-> lineCount <= text-> linesVisible && 
    currentDP-> line == 0 &&
    currentDP-> offset == 0)
   {
   text-> displayLocation = *currentDP;
   text-> displayPosition = 0;
   retval =  FALSE;
   }
else
   {
   min = _FirstWrapLine(wrapTable);
   max = _LastWrapLine(wrapTable);

   (void) _IncrementWrapLocation(wrapTable, *currentDP, page + 0, max, &diff);

   if ((diff = diff - page + 1) < 0)
      *currentDP = _IncrementWrapLocation(wrapTable, *currentDP, diff, min, NULL);

   text-> displayLocation = _LocationOfWrapLocation(wrapTable, *currentDP);
   text-> displayPosition = 
      PositionOfLocation(text-> textBuffer, text-> displayLocation);

   retval = (diff < 0);
   }

return (retval);

} /* end of _AdjustDisplayLocation */
/*
 * _CheckForScrollbarStateChange
 *
 */

static Boolean
_CheckForScrollbarStateChange(ctx, text)
TextEditWidget ctx;
TextEditPart * text;
{
Boolean      saveUpdateState;
int          vsb_state_change;
int          hsb_state_change;
Boolean      retval;

if (text-> vsb == NULL)
   retval = 0;
else
   {
   vsb_state_change = 
      (text-> need_vsb == 2 && text-> linesVisible >= text-> lineCount) || 
      (text-> need_vsb == 0 && text-> linesVisible <  text-> lineCount);
   hsb_state_change = 
      (text-> need_hsb == 2 && PAGEWID(ctx) >= text-> maxX) ||
      (text-> need_hsb == 0 && PAGEWID(ctx) <  text-> maxX && 
       text-> wrapMode == OL_WRAP_OFF);

   if (retval = (hsb_state_change || vsb_state_change))
      {
      saveUpdateState = text-> updateState;
      text-> updateState = FALSE;

      OlLayoutScrolledWindow((ScrolledWindowWidget) 
									  XtParent(XtParent(ctx)), 0);

      text-> updateState = saveUpdateState;
      }
   }

return (retval);

} /* end of _CheckForScrollbarStateChange */
/*
 * UpdateDisplay
 *
 */

static void
UpdateDisplay(ctx, textBuffer, requestor)
TextEditWidget ctx;
TextBuffer * textBuffer;
int requestor;
{
TextEditPart * text    = &ctx-> textedit;
int            page;
Boolean        moved   = FALSE;

TextPosition editStart;
TextPosition delEnd;
TextPosition position;
TextLocation currentDP;
TextLocation currentIP;
WrapTable *  wrapTable;  /* set AFTER _UpdateWrapTable stuff is done */
int          diff;
int          i;
int          row;
int          xoffset;
int          space_needed;
int          LastValidLine;
int          start;

XRectangle rect;

OlTextMotionCallData     motion_call_data;
OlTextPostModifyCallData post_modify_call_data;

if (textBuffer-> insert.hint == 0 &&
    textBuffer-> deleted.hint == 0)
   return; /* nothing to do !!! */

editStart = PositionOfLocation(textBuffer, textBuffer-> deleted.start);

if (textBuffer-> insert.string == NULL || textBuffer-> insert.string[0] == '\0')
   {
   position = editStart;
   /* AKA: position = PositionOfLocation(textBuffer, textBuffer-> deleted.start); */
   }
else
   position = PositionOfLocation(textBuffer, textBuffer-> insert.end);

space_needed = 
   LinesInTextBuffer(text-> textBuffer) + 1 - text-> wrapTable-> size;

LastValidLine = textBuffer-> deleted.end.line + 1;

start = textBuffer-> deleted.start.line;

/* }page break{ */
if (space_needed != 0)
   {
   for (i = start; i < LastValidLine; i++)
      FreeBuffer((Buffer *) text-> wrapTable-> p[i]);

   if (space_needed > 0)
      {
      GrowBuffer((Buffer *)text-> wrapTable, space_needed);
      memmove(&text-> wrapTable-> p[LastValidLine + space_needed], 
              &text-> wrapTable-> p[LastValidLine], 
              sizeof(text-> wrapTable-> p[0]) * 
              (text-> wrapTable-> size - (LastValidLine + space_needed)));
      text-> wrapTable-> used = LinesInTextBuffer(text-> textBuffer);
      }
   else
      if (space_needed < 0)
         {
         memmove(&text-> wrapTable-> p[LastValidLine + space_needed], 
                 &text-> wrapTable-> p[LastValidLine], 
                 sizeof(text-> wrapTable-> p[0]) * 
                 (text-> wrapTable-> size - (LastValidLine + space_needed)));
         GrowBuffer((Buffer *) text-> wrapTable, space_needed);
         text-> wrapTable-> used = LinesInTextBuffer(text-> textBuffer);
         }
   }
   /*
    * Update the wraptable for the newly inserted lines
    */
_UpdateWrapTable(ctx, start, textBuffer->insert.end.line);
wrapTable = text-> wrapTable;

/* }page break{ */

if (requestor)
   {
   _TurnTextCursorOff(ctx);

   if (text-> displayPosition > editStart)
      {
      moved = TRUE;
#ifdef DEBUG
fprintf(stderr,"setting moved to %d at %d\n", moved, __LINE__);
#endif
      text-> displayPosition = editStart;
      text-> displayLocation = textBuffer-> deleted.start;
      }

   page = LINES_VISIBLE(ctx);
   currentDP = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
   moved |= _AdjustDisplayLocation(ctx, text, wrapTable, &currentDP, page);
#ifdef DEBUG
fprintf(stderr,"setting moved to %d at %d\n", moved, __LINE__);
#endif
   moved |= _CheckForScrollbarStateChange(ctx, text);
#ifdef DEBUG
fprintf(stderr,"setting moved to %d at %d\n", moved, __LINE__);
#endif
   page = LINES_VISIBLE(ctx);

   text-> cursorPosition           = 
   text-> selectStart              =
   text-> selectEnd                = 
   motion_call_data.current_cursor =
   motion_call_data.new_cursor     =
   motion_call_data.select_start   =
   motion_call_data.select_end     = position;
   motion_call_data.ok             = TRUE;

   XtCallCallbacks((Widget)ctx, XtNmotionVerification, &motion_call_data);

   text-> cursorLocation= LocationOfPosition(textBuffer, text-> cursorPosition);
   currentIP = _WrapLocationOfLocation(wrapTable, text-> cursorLocation);

   _CalculateCursorRowAndXOffset(ctx, &row, &xoffset, currentDP, currentIP);

   if (xoffset)
      {
      moved = TRUE;
#ifdef DEBUG
fprintf(stderr,"setting moved to %d at %d\n", moved, __LINE__);
#endif
      text-> xOffset -= xoffset;
      }

   if (row < 0 || page <= row)
      {
      moved = TRUE;
#ifdef DEBUG
fprintf(stderr,"setting moved to %d at %d row = %d page = %d\n", 
        moved, __LINE__, row, page);
#endif
      currentDP = (row >= page) ?
      _IncrementWrapLocation(wrapTable, currentDP, row - (page - 1), 
         _LastDisplayedWrapLine(wrapTable,page), &row):
      _IncrementWrapLocation(wrapTable, currentDP, row, 
         _FirstWrapLine(wrapTable), &row);
      text-> displayLocation = _LocationOfWrapLocation(wrapTable, currentDP);
      text-> displayPosition = 
         PositionOfLocation(text-> textBuffer, text-> displayLocation);
      }
   }
else /* non-requestor */
   {
   int Dlen = textBuffer-> deleted.end.offset - textBuffer-> deleted.start.offset;
   int Ilen = textBuffer-> insert.end.offset - textBuffer-> insert.start.offset;

   delEnd   = PositionOfLocation(textBuffer, textBuffer-> deleted.end);

   /* Adjust the Cursor Position */
   text-> displayPosition = AdjustPosition
      (text-> displayPosition, Dlen, Ilen, editStart, delEnd);
   text-> cursorPosition = AdjustPosition
      (text-> cursorPosition, Dlen, Ilen, editStart, delEnd);
   text-> selectEnd = AdjustPosition
      (text-> selectEnd, Dlen, Ilen, editStart, delEnd);
   text-> selectStart = AdjustPosition
      (text-> selectStart, Dlen, Ilen, editStart, delEnd);

   text-> cursorLocation = 
      LocationOfPosition(textBuffer, text-> cursorPosition);
   text-> displayLocation = 
      LocationOfPosition(textBuffer, text-> displayPosition);

   currentDP = _WrapLocationOfLocation(wrapTable, text-> displayLocation);
   page = LINES_VISIBLE(ctx);
   moved = _AdjustDisplayLocation(ctx, text, wrapTable, &currentDP, page);
#ifdef DEBUG
fprintf(stderr,"setting moved to %d at %d\n", moved, __LINE__);
#endif
   moved = _CheckForScrollbarStateChange(ctx, text);
#ifdef DEBUG
fprintf(stderr,"setting moved to %d at %d\n", moved, __LINE__);
#endif
   }
/* }page break{ */

#ifdef DEBUG
fprintf(stderr,"moved set to %d at %d\n", moved, __LINE__);
#endif
if (moved)
   {
   rect.x      = 0;
   rect.y      = 0;
   rect.width  = ctx-> core.width;
   rect.height = ctx-> core.height;
   if (XtIsRealized((Widget)ctx) && text-> updateState)
      XClearArea(XtDisplay(ctx), XtWindow(ctx), 
                 rect.x, rect.y, rect.width, rect.height, False);
   }
else
   {
   rect.x      = 0;
   rect.y      = 0;
   rect.width  = 0;
   rect.height = 0;
   }

_DisplayText(ctx, &rect);
SetScrolledWindowScrollbars(ctx, ctx-> core.width, ctx-> core.height);

if (text-> postModifyNotification != NULL)
   {
#ifdef I18N
   char *  mb_inserted = NULL;
   char *  mb_deleted  = NULL;

   (void) _mbCopyOfwcString(&mb_inserted, textBuffer->insert.string);
   (void) _mbCopyOfwcString(&mb_deleted, textBuffer->deleted.string);
   post_modify_call_data.inserted         = mb_inserted;
   post_modify_call_data.deleted          = mb_deleted;
#else
   post_modify_call_data.inserted         = textBuffer-> insert.string;
   post_modify_call_data.deleted          = textBuffer-> deleted.string;
#endif
   post_modify_call_data.requestor        = requestor;
   post_modify_call_data.new_cursor       = text-> cursorPosition;
   post_modify_call_data.new_select_start = text-> selectStart;
   post_modify_call_data.new_select_end   = text-> selectEnd;
   post_modify_call_data.delete_start     = textBuffer-> deleted.start;
   post_modify_call_data.delete_end       = textBuffer-> deleted.end;
   post_modify_call_data.insert_start     = textBuffer-> insert.start;
   post_modify_call_data.insert_end       = textBuffer-> insert.end;
   post_modify_call_data.cursor_position  = text-> cursorPosition;
   XtCallCallbacks((Widget)ctx, XtNpostModifyNotification, &post_modify_call_data);

#ifdef I18N
   if (mb_inserted)
      FREE(mb_inserted);
   if (mb_deleted)
      FREE(mb_deleted);
#endif
   }

} /* end of UpdateDisplay */
/*
 * UndoUpdate
 *
 */

static void
UndoUpdate(ctx)
TextEditWidget ctx;
{
TextEditPart * text = &ctx-> textedit;
TextBuffer * textBuffer = text-> textBuffer;
char * buffer;
char empty = '\0';
int length;

if (textBuffer-> insert.string == NULL && textBuffer-> deleted.string == NULL)
   XBell(XtDisplay(ctx), 0);
else
   {
   if (textBuffer-> deleted.string != NULL)
      {
      length = _mbCopyOfwcString(&buffer, textBuffer-> deleted.string);
      }
   else
      {
      length = 0;
      buffer = &empty;
      }

   if (_MoveSelection
      (ctx, PositionOfLocation(textBuffer, textBuffer-> insert.start), 0, 0, 0))
      {
      text-> selectEnd = PositionOfLocation(textBuffer, textBuffer-> insert.end);
      OlTextEditInsert((Widget)ctx, buffer, length);
      if (length > 0)
         FREE(buffer);
      }
   }

} /* end of UndoUpdate */
/*
 * ValidatePosition
 *
 */

static TextPosition
ValidatePosition(min, pos, last, name)
TextPosition min;
TextPosition pos;
TextPosition last;
char *       name;
{
TextPosition retval;

if (min <= pos && pos <= last)
   retval = pos;
else
   {

   retval = min;
   OlVaDisplayWarningMsg((Display *)NULL,
			 OleNinvalidResource,
			 OleTtextEdit,
			 OleCOlToolkitWarning,
			 OleMinvalidResource_textEdit,
			 name,
			 pos,
			 min);
   }

return (retval);
   
} /* end of ValidatePosition */
/*
 * ValidatePositions
 *
 */

static void
ValidatePositions(text)
TextEditPart * text;
{
TextPosition last = LastTextBufferPosition(text-> textBuffer);

text-> displayPosition = 
   ValidatePosition(0, text-> displayPosition, last, "XtNdisplayPosition");

text-> cursorPosition = 
   ValidatePosition(0, text-> cursorPosition, last, "XtNcursorPosition");

text-> selectStart = 
   ValidatePosition(0, text-> selectStart, last, "XtNselectStart");

text-> selectEnd   = 
   ValidatePosition(0, text-> selectEnd, last, "XtNselectEnd");

if (text-> cursorPosition != text-> selectStart &&
    text-> cursorPosition != text-> selectEnd)
   text-> selectStart = text-> selectEnd = text-> cursorPosition;

text-> displayLocation = 
   LocationOfPosition(text-> textBuffer, text-> displayPosition);

text-> cursorLocation = 
   LocationOfPosition(text-> textBuffer, text-> cursorPosition);

} /* end of ValidatePositions */
/*
 * ValidateMargins
 *
 */

static void
ValidateMargins(ctx, text)
TextEditWidget ctx;
TextEditPart * text;
{
	Dimension	top_shadow_max, side_shadow_max, shadow_max;

	   /* force shadow thickness to fit in Margin */
	side_shadow_max = MIN (text->leftMargin, text->rightMargin);
	top_shadow_max = MIN (text->topMargin, text->bottomMargin);
	shadow_max = MIN(side_shadow_max, top_shadow_max);
	ctx->primitive.shadow_thickness = MIN(ctx->primitive.shadow_thickness, 
													  shadow_max);
} /* end of ValidateMargins */

/**
 ** SetDefaultLeftRightMargin()
 **/

static void
#if	OlNeedFunctionPrototypes
SetDefaultLeftRightMargin (
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
)
#else
SetDefaultLeftRightMargin (w, offset, value)
	Widget			w;
	int			offset;
	XrmValue *		value;
#endif
{
	static Dimension	min_margin;

	min_margin = FONTWID(((TextEditWidget)w)) / 2 + 1;
	if (min_margin < 4)
		min_margin = 4;
	value->addr = (XtPointer)&min_margin;
	return;
} /* SetDefaultLeftRightMargin */

/**
 ** SetDefaultBottomMargin()
 **/

static void
#if	OlNeedFunctionPrototypes
SetDefaultBottomMargin (
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
)
#else
SetDefaultBottomMargin (w, offset, value)
	Widget			w;
	int			offset;
	XrmValue *		value;
#endif
{
	static Dimension	min_margin;

	min_margin = FONTHT(((TextEditWidget)w)) / 3 + 1;
	if (min_margin < 4)
		min_margin = 4;
	value->addr = (XtPointer)&min_margin;
	return;
} /* SetDefaultBottomMargin */

/*
 * GetGCs
 * NOTE: ctx and text must refer to the same widget.
 *
 */

static void
GetGCs(ctx, text)
TextEditWidget ctx;
TextEditPart * text;
{
XGCValues   values;
XtGCMask gc_mask          = (ctx->primitive.font_list ?
                            ((unsigned) GCGraphicsExposures | 
                            GCForeground | GCBackground | 
                            GCTileStipXOrigin | GCTileStipYOrigin |
                            GCStipple | GCFillStyle) :
                            ((unsigned) GCGraphicsExposures | 
                            GCForeground | GCBackground | 
                            GCFont |
                            GCTileStipXOrigin | GCTileStipYOrigin |
                            GCStipple | GCFillStyle ));
XtGCMask inv_gc_mask      = (ctx->primitive.font_list ?
                            ((unsigned) GCGraphicsExposures | 
                            GCForeground | GCBackground | 
                            GCStipple | GCFillStyle) :
                            ((unsigned) GCGraphicsExposures | 
                            GCForeground | GCBackground | 
                            GCFont |
                            GCStipple | GCFillStyle));

values.graphics_exposures = TRUE;
values.background         = ctx-> core.background_pixel;
values.foreground         = ctx->primitive.font_color;
values.font               = ctx->primitive.font-> fid;
values.stipple            = OlGet50PercentGrey(XtScreen(ctx));
values.ts_x_origin        = 0;
values.ts_y_origin        = 0;
values.fill_style         = FillSolid;
   
                            

text-> gc = XtGetGC ( (Widget)ctx, gc_mask, &values);

values.foreground         = ctx-> core.background_pixel;
values.background         = ctx->primitive.font_color;
text-> invgc = XtGetGC ( (Widget)ctx, inv_gc_mask, &values);

values.graphics_exposures = FALSE;
values.foreground         = ctx->primitive.font_color;
values.background         = ctx-> core.background_pixel;
values.function           = GXxor;
text-> insgc = XtGetGC
   ( (Widget)ctx, 
     (unsigned) GCGraphicsExposures | GCForeground | GCBackground | GCFunction,
     &values);

(*_olmTECreateTextCursors)(ctx);
} /* end of GetGCs */
/*
 * EditModeConverter
 *
 */

static Boolean
EditModeConverter OLARGLIST((dpy, args, num_args, from, to, converter_data))
	OLARG( Display *,	dpy)
	OLARG( XrmValue *,	args)
	OLARG( Cardinal *,	num_args)
	OLARG( XrmValue *,	from)
	OLARG( XrmValue *,	to)
	OLGRA( XtPointer *,	converter_data)
{
OlEditMode mode;
char * p = from-> addr;

if (0 == strcmp(p, "textread") || 0 == strcmp(p, "oltextread"))
   mode = OL_TEXT_READ;
else
   if (0 == strcmp(p, "textedit") || 0 == strcmp(p, "oltextedit"))
      mode = OL_TEXT_EDIT;
   else
      {
      mode = OL_WRAP_OFF;
      }

ConversionDone (OlEditMode, mode);

} /* end of EditModeConverter */
/*
 * WrapModeConverter
 *
 */

static Boolean
WrapModeConverter OLARGLIST((dpy, args, num_args, from, to, converter_data))
	OLARG( Display *,	dpy)
	OLARG( XrmValue *,	args)
	OLARG( Cardinal *,	num_args)
	OLARG( XrmValue *,	from)
	OLARG( XrmValue *,	to)
	OLGRA( XtPointer *,	converter_data)
{
OlWrapMode mode;
char * p = from-> addr;

if (0 == strcmp(p, "wrapany") || 0 == strcmp(p, "olwrapany"))
   mode = OL_WRAP_ANY;
else
   if (0 == strcmp(p, "wrapwhitespace") || 0 == strcmp(p, "olwrapwhitespace"))
      mode = OL_WRAP_WHITE_SPACE;
   else
      if (0 == strcmp(p, "wrapoff") || 0 == strcmp(p, "olwrapoff"))
         mode = OL_WRAP_OFF;
      else
         {
         mode = OL_WRAP_OFF;
         }

ConversionDone (OlWrapMode, mode);
} /* end of WrapModeConverter */
/*
 * SourceTypeConverter
 *
 */

static Boolean
SourceTypeConverter OLARGLIST((dpy, args, num_args, from, to, converter_data))
	OLARG( Display *,	dpy)
	OLARG( XrmValue *,	args)
	OLARG( Cardinal *,	num_args)
	OLARG( XrmValue *,	from)
	OLARG( XrmValue *,	to)
	OLGRA( XtPointer *,	converter_data)
{
OlSourceType mode;
char * p = from-> addr;

if (0 == strcmp(p, "stringsource") || 0 == strcmp(p, "olstringsource"))
   mode = OL_STRING_SOURCE;
else
   if (0 == strcmp(p, "disksource") || 0 == strcmp(p, "oldisksource"))
      mode = OL_DISK_SOURCE;
   else
      {
      mode = OL_STRING_SOURCE;
      }

ConversionDone (OlSourceType, mode);
} /* end of SourceTypeConverter */
/*
 * ScrolledWindowInterface
 *
 */

static void
ScrolledWindowInterface(ctx, gg)
TextEditWidget ctx;
OlSWGeometries * gg;
{
TextEditPart * text       = &ctx-> textedit;
Boolean force_vsb         = gg-> force_vsb;
Boolean force_hsb         = gg-> force_hsb;

ResizeScrolledWindow(ctx, (ScrolledWindowWidget) XtParent(XtParent(ctx)), gg, 
                     gg-> sw_view_width, gg-> sw_view_height, False);

gg-> bbc_real_width  = text-> wrapMode != OL_WRAP_OFF ? gg-> bbc_width :
                       text-> maxX;
gg-> bbc_real_height = text-> lineHeight * text-> lineCount;

text-> need_vsb      = force_vsb ? 1 : (gg-> force_vsb ? 2 : 0);
text-> need_hsb      = force_hsb ? 1 : (gg-> force_hsb ? 2 : 0);

if (text-> need_vsb || text-> need_hsb)
   SetScrolledWindowScrollbars(ctx, gg-> bbc_width, gg-> bbc_height);

} /* end of ScrolledWindowInterface */
/*
 * ResizeScrolledWindow
 *
 */

static void
ResizeScrolledWindow OLARGLIST((ctx, sw, gg, width, height, resize))
	OLARG(TextEditWidget, ctx)
	OLARG(ScrolledWindowWidget, sw)
	OLARG(OlSWGeometries *, gg)
	OLARG(Dimension, width)
	OLARG(Dimension, height)
	OLGRA(int, resize)
{
TextEditPart * text        = &ctx-> textedit;
int            vsb_width   = resize ? 0 : gg-> vsb_width;
int            hsb_height  = resize ? 0 : gg-> hsb_height;
Arg            arg[5];
TextBuffer *   textBuffer  = text-> textBuffer;
Dimension      save_width  = ctx-> core.width;
Dimension      save_height = ctx-> core.height;
TextLocation   currentDP;

ctx-> core.width = width;
ctx-> core.height = height;

if (text-> wrapTable != NULL)
   text-> displayPosition = 
      PositionOfLocation(text-> textBuffer, text-> displayLocation);

if (gg-> force_vsb && gg-> force_hsb)
   {
   ctx-> core.width = width - vsb_width;
   ctx-> core.height = height - hsb_height;
   _BuildWrapTable(ctx);
   }
else
   if (gg-> force_vsb || PAGE_LINE_HT(ctx) < LinesInTextBuffer(textBuffer))
      {
      gg-> force_vsb = TRUE;
      ctx-> core.width -= vsb_width;
      _BuildWrapTable(ctx);
      if (text-> wrapMode == OL_WRAP_OFF && text-> maxX > PAGEWID(ctx))
         {
         gg-> force_hsb = TRUE;
         ctx-> core.height = height - hsb_height;
         }
      }
   else
      if (gg-> force_hsb)
         {
         ctx-> core.height = height - hsb_height;
         _BuildWrapTable(ctx);
         gg-> force_vsb = text-> lineCount < text-> linesVisible;
         }
      else
         {
         if (text-> wrapMode == OL_WRAP_OFF)
            {
            _BuildWrapTable(ctx);
            if (text-> maxX > PAGEWID(ctx))
               {
               gg-> force_hsb = TRUE;
               ctx-> core.height = height - hsb_height;
               }
            text-> linesVisible = PAGE_LINE_HT(ctx);
            if (text-> linesVisible < text-> lineCount)
               {
               gg-> force_vsb = TRUE;
               ctx-> core.width -= vsb_width;
               }
            }
         else
            {
            _BuildWrapTable(ctx);
            if (text-> linesVisible < text-> lineCount)
               {
               gg-> force_vsb = TRUE;
               ctx-> core.width -= vsb_width;
               _BuildWrapTable(ctx);
               }
            }
         }

text-> linesVisible = PAGE_LINE_HT(ctx);
text-> displayLocation = LocationOfPosition(textBuffer, text-> displayPosition);
if (save_width != ctx-> core.width  || 
    save_height != ctx-> core.height)
   text-> xOffset = 0;

currentDP = _WrapLocationOfLocation(text-> wrapTable, text-> displayLocation);
(void) _AdjustDisplayLocation(ctx, text, text-> wrapTable, 
                              &currentDP, text-> linesVisible);

width  = gg-> bbc_width  = ctx-> core.width;
height = gg-> bbc_height = ctx-> core.height;

ctx-> core.width  = save_width;
ctx-> core.height = save_height;

if (resize)
   {
   if (gg-> force_vsb)
      width += gg-> vsb_width;
   if (gg-> force_hsb)
      height += gg-> hsb_height;
   XtSetArg(arg[0], XtNwidth,  2 * gg-> bb_border_width + width);
   XtSetArg(arg[1], XtNheight, 2 * gg-> bb_border_width + height);
   XtSetValues((Widget)sw, arg, 2);
   }

} /* end of ResizeScrolledWindow */
/*
 * SetScrolledWindowScrollbars
 *
 */

static void
SetScrolledWindowScrollbars OLARGLIST((ctx, width, height))
	OLARG(TextEditWidget, ctx)
	OLARG(Dimension,      width)
	OLGRA(Dimension,      height)
{
TextEditPart * text  = &ctx-> textedit;
Arg arg[10];
int pagewid          = width - PAGE_L_GAP(ctx) - PAGE_R_GAP(ctx);
int proportionLength;
int position         = 
   _LineNumberOfWrapLocation (ctx-> textedit.wrapTable, 
   _WrapLocationOfLocation(ctx-> textedit.wrapTable, ctx-> textedit.displayLocation));

if (text-> need_vsb)
   {
   proportionLength = MIN(text-> lineCount, text-> linesVisible);
   XtSetArg(arg[0], XtNsliderMax,        text-> lineCount);
   XtSetArg(arg[1], XtNproportionLength, proportionLength);
   XtSetArg(arg[2], XtNsliderValue,      position);
   XtSetValues(ctx-> textedit.vsb, arg, 3);
   }

if (text-> need_hsb)
   {
   proportionLength = MIN(pagewid, ctx-> textedit.maxX);

   if (text-> xOffset && (text-> maxX + text-> xOffset < proportionLength))
      text-> xOffset = proportionLength - text-> maxX;

   XtSetArg(arg[0], XtNsliderMax,        ctx-> textedit.maxX);
   XtSetArg(arg[1], XtNproportionLength, proportionLength);
   XtSetArg(arg[2], XtNsliderValue,      -ctx-> textedit.xOffset);
   XtSetValues(ctx-> textedit.hsb, arg, 3);
   }

} /* end of SetScrolledWindowScrollbars */
/*
 * VSBCallback
 *
 */

static void
VSBCallback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
TextEditWidget      ctx = (TextEditWidget)client_data;
OlScrollbarVerify * p   = (OlScrollbarVerify *)call_data;

if (p-> ok)
   (void)_MoveDisplayPosition(ctx, NULL, OL_PGM_GOTO, p-> delta);

} /* end of VSBCallback */
/*
 * HSBCallback
 *
 */

static void
HSBCallback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
TextEditWidget      ctx = (TextEditWidget)client_data;
OlScrollbarVerify * p = (OlScrollbarVerify *)call_data;

if (p-> ok)
   _MoveDisplayLaterally(ctx, &ctx-> textedit, p-> delta, FALSE);

} /* end of HSBCallback */

#define TECPART(w)	((TextEditWidgetClass)XtClass(w))->textedit_class

/*
 * ClassInitialize 
 * 
 * Register the converters used by TextEdit widget.
 * Set up resource defaults if in Motif mode
 * Resolve dynamic symbols from GUI-specific library
 * 
 */

static void
ClassInitialize()
{ 
    int i;

    XtSetTypeConverter(
	XtRString, XtROlEditMode, EditModeConverter, 
	(XtConvertArgList)NULL, (Cardinal)0,
	XtCacheAll, (XtDestructor)0);

    XtSetTypeConverter(
	XtRString, XtROlWrapMode, WrapModeConverter, 
	(XtConvertArgList)NULL, (Cardinal)0,
	XtCacheAll, (XtDestructor)0);

    XtSetTypeConverter(
	XtRString, XtROlSourceType, SourceTypeConverter, 
	(XtConvertArgList)NULL, (Cardinal)0,
	XtCacheAll, (XtDestructor)0);

      /* Note that the textedit_class part 'im' is initialized
       * in the Realize procedure, because OlOpenIm() requires
       * a window, which is not available until Realize() time.
       */

   /* Initialize Motif resource defaults if running in Motif mode */
    if (OlGetGui() == OL_MOTIF_GUI)  {
	DefaultShadowThickness[0] = DefaultMotifShadowThickness;
	DefaultBlinkRate = 500;
    }

    ((TextEditClassRec *)textEditWidgetClass)->textedit_class.click_mode = XVIEW_CLICK_MODE;

    /* resolve dynamic symbols from GUI-specific library */
    OLRESOLVESTART
    OLRESOLVE(TEKey, event_procs[0].handler)
    OLRESOLVE(TEButton, event_procs[1].handler)
    OLRESOLVEEND(TECreateTextCursors, _olmTECreateTextCursors)

} /* end of ClassInitialize */

/*
 * ClassPartInitialize
 *
 * This routine allows inheritance of the TextEdit class procedure
*/

static void
ClassPartInitialize(class)
	WidgetClass class;
{
#ifdef SHARELIB
	void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit		(*__libXol__XtInherit)
#endif


	OlClassSearchTextDB (class);

	return;
}
/*
 *
 * Initialize
 * 
 *
 */


/* ARGSUSED */
static void
Initialize(request, new, args, num_args)
Widget request;
Widget new;
ArgList args;
Cardinal * num_args;
{
    TextEditWidget ctx  = (TextEditWidget)new;
    TextEditPart * text = &(ctx-> textedit);
    Dimension      width;
    Dimension      height;
    Arg            arg[5];
    BufferElement  empty = (BufferElement) '\0';
    Widget	p;
    
    if (text-> linesVisible <= 0)
	text-> linesVisible = 1;
    if (text-> charsVisible <= 0)
	text-> charsVisible = 1;
    
    switch(text-> sourceType)
    {
    case OL_DISK_SOURCE:
	text-> textBuffer = 
	    ReadFileIntoTextBuffer(text-> source, UpdateDisplay, (XtPointer)ctx);
	if (text-> textBuffer == NULL){
	    text-> textBuffer = 
		wcReadStringIntoTextBuffer(&empty, UpdateDisplay, (XtPointer)ctx);
	}
	break;
    case OL_TEXT_BUFFER_SOURCE:
	text-> textBuffer = (TextBuffer *)text-> source;
	RegisterTextBufferUpdate(text-> textBuffer, UpdateDisplay, (XtPointer)ctx);
	break;
    case OL_STRING_SOURCE:
    default:
	
#ifdef DEBUG
	printf("Initialize: string source:%s\n",text->source);
#endif
	
	text-> textBuffer = 
	    ReadStringIntoTextBuffer(text-> source, UpdateDisplay, (XtPointer)ctx);
	break;
    }
    text-> source = (char *)text-> textBuffer;
    
    ValidatePositions(text);
    ValidateMargins(ctx, text);
    
    text-> clip_contents= NULL;
    text-> drag_contents= NULL;
    text-> shouldBlink  = TRUE;
    text-> blink_timer  = (XtIntervalId) NULL;
    text-> lineHeight   = FONTHT(ctx) + 0;
    text-> charWidth    = ENSPACE(ctx);
    text-> xOffset      = 0;
    text-> maxX         = 0;
    text-> vsb          = (Widget)NULL;
    text-> hsb          = (Widget)NULL;
    text-> need_vsb     = FALSE;
    text-> need_hsb     = FALSE;
    text-> save_offset  = -1;
    text-> selectMode   = 0;
    text-> wrapTable    = (WrapTable *)NULL;
    text-> mask         = 0;
    text-> dynamic      = 0;
    text-> CursorIn     = (Pixmap) 0;
    text-> CursorOut    = (Pixmap) 0;
    text-> cursor_state = OlCursorOff;
    text-> updateState  = TRUE;
    text-> DT           = (DisplayTable *)NULL;
    text-> DTsize       = 0;
    text-> anchor       = -1;
    text-> prev_width   = text-> prev_height = 0;
    text-> a_index    = 0;		/* accelerator update index - i18n */
    text-> m_index    = 0;		/* mnemonic undate index - i18n     */
    text-> im_key_index    = 0;		/* global keys undate index - i18n   */
    
    width  = text-> charWidth * text-> charsVisible + 
	PAGE_L_GAP(ctx) + PAGE_R_GAP(ctx);
    height = text-> lineHeight * text-> linesVisible + 
	PAGE_T_GAP(ctx) + PAGE_B_GAP(ctx);
    
    if (ctx-> core.width == 0)
	ctx-> core.width  = MAX(ctx-> core.width, width);
    if (ctx-> core.height == 0)
	ctx-> core.height = MAX(ctx-> core.height, height);
    
    if (XtIsSubclass(XtParent((Widget)ctx), scrolledWindowWidgetClass))
    {
	ScrolledWindowWidget sw = (ScrolledWindowWidget)XtParent((Widget)ctx);
	OlSWGeometries geometries;
	geometries = GetOlSWGeometries(sw);
	
	ctx-> core.border_width = 0;
	
	XtSetArg(arg[0], XtNcomputeGeometries, ScrolledWindowInterface);
	XtSetArg(arg[1], XtNvAutoScroll, FALSE);
	XtSetArg(arg[2], XtNhAutoScroll, FALSE);
	XtSetValues((Widget)sw, arg, 3);
	
	XtAddCallback((Widget)sw, XtNvSliderMoved, VSBCallback, (XtPointer)ctx);
	XtAddCallback((Widget)sw, XtNhSliderMoved, HSBCallback, (XtPointer)ctx);
	
	text-> vsb = (Widget)geometries.vsb;
	text-> hsb = (Widget)geometries.hsb;
	
	ResizeScrolledWindow(ctx, sw, &geometries, 
			     ctx-> core.width, ctx-> core.height, True);
	
	XtSetArg(arg[0], XtNgranularity,      1);
	XtSetArg(arg[1], XtNsliderMin,        0);
	XtSetValues(text-> vsb, arg, 2);
	
	XtSetArg(arg[0], XtNgranularity,      text-> charWidth);
	XtSetArg(arg[1], XtNsliderMin,        0);
	XtSetValues(text-> hsb, arg, 2);
	SetScrolledWindowScrollbars(ctx, ctx-> core.width, ctx-> core.height);
    }
    else
    {
	if (XtGeometryAlmost == 
	    XtMakeResizeRequest((Widget)ctx, 
				ctx-> core.width, ctx-> core.height, 
				&ctx-> core.width, &ctx-> core.height))
	    XtMakeResizeRequest ((Widget)ctx, 
				 ctx-> core.width, ctx-> core.height, 
				 NULL, NULL);
	_BuildWrapTable(ctx);
    }
    
    text-> linesVisible = PAGEHT(ctx) / text-> lineHeight;
    
    text->dropsiteid = (OlDnDDropSiteID)NULL;
    text->transient = (Atom)None;
} /* end of Initialize */
/*
 * Redisplay 
 * 
 * This routine processes all "expose region" XEvents. In general, its 
 * job is to the best job at minimal re-paint of the text, displayed in 
 * the window, that it can.
 * 
 */

static void
Redisplay(w, event, region)
Widget w;
XEvent * event;
Region region;
{
TextEditWidget ctx = (TextEditWidget)w;
TextEditPart * text = &ctx-> textedit;
XRectangle rect;

if (region != (Region)NULL)
   {
   XClipBox(region, &rect);
   }
else
   {
   switch (event-> type)
      {
      case Expose:
         rect.x      = event-> xexpose.x;
         rect.y      = event-> xexpose.y;
         rect.width  = event-> xexpose.width;
         rect.height = event-> xexpose.height;
         break;
      case GraphicsExpose:
         rect.x      = event-> xgraphicsexpose.x;
         rect.y      = event-> xgraphicsexpose.y;
         rect.width  = event-> xgraphicsexpose.width;
         rect.height = event-> xgraphicsexpose.height;
         break;
      case NoExpose:
      default:
         return;
      }
   }

if (text-> cursor_state == OlCursorOn)
   {
   if (region != (Region)NULL)
      XSetRegion(XtDisplay(ctx), text-> insgc, region);
   else
      XSetClipRectangles(XtDisplay(ctx), text->insgc, 0, 0, &rect, 1, Unsorted);
   _TurnTextCursorOff(ctx);
   text-> cursor_state = OlCursorOn;
   XSetClipMask(XtDisplay(ctx), text->insgc, None);
   }

_DisplayText(ctx, &rect);
   
} /* end of Redisplay */

/*
 * AckDel
 */
static void
AckDel OLARGLIST((w, client_data, selection, type, value, length, format))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)
	OLARG( Atom *,		selection)
	OLARG( Atom *,		type)
	OLARG( XtPointer,	value)
	OLARG( unsigned long *, length)
	OLGRA( int *,		format)
{
	if ((Boolean)client_data) {
		OlDnDDragNDropDone(w, *selection, 
			XtLastTimestampProcessed(XtDisplay(w)), NULL, NULL);
	}
} /* end of AckDel */

/*
 * _OlTextEditTriggerNotify
 */
extern Boolean
_OlTextEditTriggerNotify OLARGLIST(( widget, window, x, y, selection, timestamp, drop_site, operation, send_done, forwarded, closure))
	OLARG(Widget,		     widget)
	OLARG(Window,		     window)
	OLARG(Position,		     x)
	OLARG(Position,		     y)
	OLARG(Atom,		     selection)
	OLARG(Time,		     timestamp)
	OLARG(OlDnDDropSiteID,	     drop_site)
	OLARG(OlDnDTriggerOperation, operation)
	OLARG(Boolean,		     send_done)
	OLARG(Boolean,		     forwarded)	/* not used */
	OLGRA(XtPointer,	     closure)
{
    Window child;
    int new_x, new_y;
    TextPosition pos;

    /* convert x and y to coordinates of textedit window */
    (void) XTranslateCoordinates(XtDisplay(widget), 
				 RootWindow(XtDisplay(widget), 0),
				 window,
				 x, y,
				 &new_x, &new_y,
				 &child);
    pos = _PositionFromXY((TextEditWidget)widget, new_x, new_y);
    /* set CursorPosition to mouse position so text is inserted
     * there instead of at current insertion point
     */
    OlTextEditSetCursorPosition (widget, pos, pos, pos);

    XtGetSelectionValue(widget, selection,
			XInternAtom(XtDisplay(widget), "COMPOUND_TEXT", False),
			Paste2,
			operation == OlDnDTriggerCopyOp ?
					(XtPointer)send_done :(XtPointer)False,
					timestamp);
    if (operation == OlDnDTriggerMoveOp) {
	XtGetSelectionValue(widget, selection, OL_XA_DELETE(XtDisplay(widget)),
			    AckDel, (XtPointer)send_done , timestamp);
    }
    return(True);
} /* end of _OlTextEditTriggerNotify */

/*
 * Realize
 * 
 *		Creates window for the TextEdit widget
 *		Defines the mouse cursor
 *		Creates the GCs and Text cursor
 *		Establishes a connection with the input method (if necessary)
 *		Puts passive grabs on all button events
 *		Registers Drop site for text drag and drop of text 	
 */
static void
Realize OLARGLIST((www, valueMask, attributes ))
	OLARG( Widget,			www)
	OLARG( XtValueMask *,		valueMask)
	OLGRA( XSetWindowAttributes *,	attributes)
{
    TextEditWidget         ctx = (TextEditWidget)www;
    
#define NUM_ICV      11
#define NUM_WATT  9
    OlIm *      im = ((TextEditWidgetClass) XtClass(ctx))->textedit_class.im;
    OlIcValues  icvalues[NUM_ICV+1];
    OlIcValues  p_icvalues[NUM_WATT+1];
    int         n;
    Widget      toplevel;
    XPoint      spot;
    static OlImStyle style = OlImPreEditPosition|OlImStatusArea;
    Window      c_win;
    Window      f_win;
    Arg         args[1];
    String      input_method;
    XWindowAttributes	win_attr;
    Widget		wid = (Widget)ctx;
    
    ((TextEditWidgetClass) XtClass(ctx))->textedit_class.im_key_index = 0;
    XtSetArg(args[0], XtNinputMethod, &input_method); 
    OlGetApplicationValues((Widget) ctx, args, 1);
    
    if (im==NULL && input_method && *input_method != NULL)
    {
	im = 
	    ((TextEditWidgetClass) XtClass(ctx))-> textedit_class.im = 
		OlOpenIm(XtDisplay(ctx), NULL/*rdb*/, NULL, NULL);
    }
    
    
    /* DynamicInitialize(ctx); */
    GetGCs(ctx, &ctx-> textedit);
    
    *valueMask |= CWBitGravity;
    *valueMask |= CWBackPixel;
    attributes-> bit_gravity = NorthWestGravity;
    attributes-> background_pixel = ctx-> core.background_pixel;
    
    XtCreateWindow(wid, InputOutput, (Visual *)CopyFromParent,
		   *valueMask, attributes);
    
    /* Current OlGetStandardCursor is macro for GetOl, will change
       with code from Sun */
    XDefineCursor(XtDisplay(wid), XtWindow(wid), OlGetStandardCursor(wid));
    
    
    XGrabButton(XtDisplay(wid),
		Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask,
		AnyModifier,
		XtWindow(ctx), False,
		ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
		GrabModeAsync, GrabModeAsync, None, None);
    
    if (ctx-> textedit.vsb != NULL) /* must be in a scrolled window */
    {
	Arg arg[1];
	XtSetArg(arg[0], XtNbackground, ctx-> core.background_pixel);
	XtSetValues(XtParent(wid), arg, 1);
    }
    else
	_BuildWrapTable(ctx);
    
    {
	OlDnDSiteRect rect;
	
	rect.x = 0;
	rect.y = 0;
	rect.width = ctx->core.width;	
	rect.height = ctx->core.height;
    
	ctx->textedit.dropsiteid = 
	    OlDnDRegisterWidgetDropSite((Widget)ctx,
					OlDnDSitePreviewNone,
					&rect, 1,
					_OlTextEditTriggerNotify,
					NULL,
					(ctx->textedit.editMode == OL_TEXT_EDIT) ? 
					True : False,
					(XtPointer) NULL);
    }


    /* Create an input context for the TextEdit widget
     * if an input method is being used and the widget
     * accepts input.
     */
    if (im && ctx-> textedit.editMode == OL_TEXT_EDIT
	&& ctx->primitive.ic == NULL){
	
	toplevel = (Widget)ctx;
	while (!XtIsShell(toplevel))
	    toplevel = XtParent(toplevel);
	n = 0;
	icvalues[n].attr_name = OlNclientWindow;
	icvalues[n].attr_value = (void *)&c_win;
	c_win = XtWindow(toplevel);
	n++;
	icvalues[n].attr_name = OlNfocusWindow;;
	icvalues[n].attr_value = (void *)&f_win;
	f_win = XtWindow(ctx);
	n++;
	
	icvalues[n].attr_name = OlNpreeditAttributes;
	icvalues[n].attr_value = (void *)p_icvalues;
	n++;
	icvalues[n].attr_name = OlNstatusAttributes;
	icvalues[n].attr_value = (void *)p_icvalues;
	n++;
	
	icvalues[n].attr_name = OlNinputStyle;
	icvalues[n].attr_value = (void *)&style;
	n++;
	icvalues[n].attr_name = OlNspotLocation;
	icvalues[n].attr_value = (void *)&spot;
	spot.x = 8;
	spot.y = 15;
	n++;
	icvalues[n].attr_name = NULL;
	icvalues[n].attr_value = NULL;
	n = 0;
	p_icvalues[n].attr_name = OlNbackground;
	p_icvalues[n].attr_value = (void *)&ctx->core.background_pixel;
	n++;
	p_icvalues[n].attr_name = OlNforeground;
	p_icvalues[n].attr_value = (void *)&ctx->primitive.font_color;
	n++;
	p_icvalues[n].attr_name = OlNfontSet;
	p_icvalues[n].attr_value = (void *)ctx->primitive.font_list;
	n++;
	p_icvalues[n].attr_name = (char *)NULL;
	p_icvalues[n].attr_value = (void *)NULL;
	
	/* create the input context */
	ctx-> primitive.ic = OlCreateIc(im, icvalues);
	if (ctx-> primitive.ic==NULL) {
	    /* Error */
	    
	    
	    OlVaDisplayWarningMsg( XtDisplay(ctx),
				  OleNnoInputContext,
				  OleTtextEdit,
				  OleCOlToolkitWarning,
				  OleMnoInputContext_textEdit,
				  input_method);
	}
	else{
	    /* Register the IC with the vendor shell if the
	     * input method uses a status area.
	     * The shell will provide room for the status area.
	     * In response to our specified width of 0,
	     * the shell will use its whole width as the status
	     * area width.  (We have no good way of guessing
	     * an appropriate size.)   Use the maximum font height
	     * for the status area height.
	     */
	    if (_OlGetStatusArea())
		_OlRegisterIc((Widget) ctx, ctx->primitive.ic, 0, 
			      ctx->primitive.font_list->max_bounds.ascent +
			      ctx->primitive.font_list->max_bounds.descent);
	}
    }
    else
	ctx->primitive.ic = NULL;
} /* end of Realize */

/*
 * RegisterFocus
 *
 */
static Widget
RegisterFocus(w)
    Widget	w;
{
    OlRegisterFocusFunc focus_func =
			((TextEditWidget)w)->textedit.register_focus;

    return ( (focus_func == NULL) ? 
	    ((TextEditWidget)w)->primitive.traversal_on ? w : NULL
	    : (*focus_func)(w));
}

/*
 * Resize
 * 
 */

static void
Resize(w)
	Widget          w;
{
    TextEditWidget ctx = (TextEditWidget)w;
    TextEditPart * text = &(ctx-> textedit);
    OlDnDSiteRect dsrect;
    
    if (XtIsRealized((Widget)ctx)) 
    {
	text->charsVisible = ((int)(ctx->core.width -
			 (PAGE_L_GAP(ctx) + PAGE_R_GAP(ctx))))/text->charWidth;
	_BuildWrapTable(ctx);
	SetScrolledWindowScrollbars(ctx, ctx-> core.width, ctx-> core.height);
	if (text->updateState)
	{
	    XRectangle	rect;
	    
	    rect.x = 0;
	    rect.y = 0;
	    rect.width = ctx-> core.width;
	    rect.height = ctx-> core.height;
	    _TurnTextCursorOff(ctx);
	    XClearArea(XtDisplay(ctx), XtWindow(ctx), 0, 0, 0, 0, False);
	    _DisplayText(ctx, &rect);
	    
	    /* If the widget has registered a drop-site, update its
	     * geometry.  Read-only TextEdit widgets do not register
	     * a drop site
	     */
	    dsrect.x = 0;
	    dsrect.y = 0;
	    dsrect.width = ctx->core.width;
	    dsrect.height = ctx->core.height;
	    OlDnDUpdateDropSiteGeometry(text->dropsiteid, 
					&dsrect, 1);

	}
	
    }
    
} /* end of Resize */
/*
 * SetValues
 * 
 */

/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
Widget current;
Widget request;
Widget new;
ArgList args;
Cardinal * num_args;
{
    TextEditWidget oldtw   = (TextEditWidget) current;
    TextEditWidget newtw   = (TextEditWidget) new;
    TextEditPart * oldtext = &(oldtw-> textedit);
    TextEditPart * newtext = &(newtw-> textedit);
    Boolean realized       = XtIsRealized(current);
    Boolean redisplay      = FALSE;
    OlCursorState save_cursorState = oldtext-> cursor_state;
    Arg            arg[9];
    XWindowAttributes	win_attr;
    
#ifdef I18N
    OlIcValues  icvalues[NUM_ICV+1];
    OlIcValues  p_icvalues[NUM_WATT+1];
    int      n;
    Widget      toplevel;
    XPoint      spot;
    static OlImStyle style = OlImPreEditPosition|OlImStatusArea;
    Window      c_win;
    Window      f_win;
#endif
    
    _TurnTextCursorOff(oldtw);

    /*
     *	prevent changes to charsVisible and linesVisible
     */
    newtext->linesVisible = oldtext->linesVisible;
    newtext->charsVisible = oldtext->charsVisible;

    /*
     *	Handle resources which affect geometry
     *
     */
    if (oldtw->primitive.font != newtw->primitive.font ||
	oldtw->primitive.font_list != newtw->primitive.font_list ||
	oldtw->core.height    != newtw->core.height    ||
	oldtw->core.width     != newtw->core.width){
	
	newtext->lineHeight   = FONTHT(newtw);
	newtext->charWidth    = ENSPACE(newtw);
	newtext->charsVisible = ((int)(newtw->core.width -
			 (PAGE_L_GAP(newtw) + PAGE_R_GAP(newtw))))/newtext->charWidth;
	/*
	 *  Make sure we can handle at least one character
	 */
	if (newtext->charsVisible <= 0){
	    newtext->charsVisible = 1;
	    newtw->core.width  = newtext-> charWidth + PAGE_L_GAP(newtw)
		+ PAGE_R_GAP(newtw);
	}
	/*
	 * Make sure that we are large enough to display at least
	 * a single line.
	 */
	if (PAGE_LINE_HT(newtw) <= 0)
	    newtw->core.height = newtext-> lineHeight + PAGE_T_GAP(newtw) 
		+ PAGE_B_GAP(newtw);
    }
    
    /*
     * Handle resources which affect GCs
     */
    
    if (oldtw-> core.background_pixel != newtw-> core.background_pixel ||
	oldtw->primitive.font_color   != newtw->primitive.font_color   ||
	oldtw->primitive.input_focus_color != newtw->primitive.input_focus_color ||
	oldtw->primitive.font         != newtw->primitive.font)
    {
	XtReleaseGC((Widget)newtw, newtext-> gc);
	XtReleaseGC((Widget)newtw, newtext-> invgc);
	XtReleaseGC((Widget)newtw, newtext-> insgc);
	GetGCs(newtw, newtext);
	if (((oldtw-> primitive.font != newtw-> primitive.font)
	     || (oldtw->primitive.font_list != newtw->primitive.font_list))
	    && newtext->hsb != NULL){
	    XtSetArg(arg[0], XtNgranularity, newtext-> charWidth);
	    XtSetValues(newtext->hsb, arg, 1);
	}
	if ((oldtw-> core.background_pixel != newtw-> core.background_pixel) 
	    && (newtw-> textedit.vsb != NULL))
	{
	    XtSetArg(arg[0], XtNbackground, newtw-> core.background_pixel);
	    XtSetValues(XtParent(newtw), arg, 1);
	}
    }
    
    /* prevent any changes to XtNcontrolCaret */
    newtext-> controlCaret = oldtext-> controlCaret;
    
    if ((newtext-> sourceType != oldtext-> sourceType) ||
	(newtext-> source     != oldtext-> source))
    {
	FreeTextBuffer(newtext-> textBuffer, UpdateDisplay, (XtPointer)newtw);
	switch(newtext-> sourceType)
	{
	case OL_DISK_SOURCE:
	    newtext-> textBuffer = 
		ReadFileIntoTextBuffer(newtext-> source, UpdateDisplay, (XtPointer)newtw);
	    if (newtext-> textBuffer == NULL)
		newtext-> textBuffer = 
		    ReadStringIntoTextBuffer("", UpdateDisplay, (XtPointer)newtw);
	    break;
	case OL_TEXT_BUFFER_SOURCE:
	    newtext-> textBuffer = (TextBuffer *)newtext-> source;
	    RegisterTextBufferUpdate(newtext-> textBuffer, UpdateDisplay, (XtPointer)newtw);
	    break;
	case OL_STRING_SOURCE:
	default:
	    newtext-> textBuffer = ReadStringIntoTextBuffer
		(newtext-> source, UpdateDisplay, (XtPointer)newtw);
	    break;
	}
	newtext-> source = (char *)newtext-> textBuffer;
	newtext-> xOffset = 0;
	
	newtext-> selectMode = 0;
	/*
	 *  reset the cursorPosition, displayPosition, selectStart, and
	 *  selectEnd.  The application may be setting these to specific
	 *  values, so we only reset them if the application has not
	 *  specified them in the SetValues call.  (InArgList catches
	 *  the case where the application-specified value is the
	 *  same as the current value.)
	 */
	if ((newtext->displayPosition == oldtext->displayPosition) &&
	    !InArgList(XtNdisplayPosition, args, *num_args))
	    newtext->displayPosition = 0;
	if ((newtext->cursorPosition == oldtext->cursorPosition) &&
	    !InArgList(XtNcursorPosition, args, *num_args))
	    newtext->cursorPosition = 0;
	if ((newtext->selectStart == oldtext->selectStart) &&
	    !InArgList(XtNselectStart, args, *num_args))
	    newtext->selectStart = 0;
	if ((newtext->selectEnd == oldtext->selectEnd) &&
	    !InArgList(XtNselectEnd, args, *num_args))
	    newtext->selectEnd = 0;

	redisplay = TRUE;
    }
    else
    {
	if ((oldtext-> leftMargin          != newtext-> leftMargin)          ||
	    (oldtext-> topMargin           != newtext-> topMargin)           ||
	    (oldtext-> rightMargin         != newtext-> rightMargin)         ||
	    (oldtext-> bottomMargin        != newtext-> bottomMargin)        ||
	    (oldtext-> wrapMode            != newtext-> wrapMode)            ||
	    (oldtw->primitive.font         != newtw->primitive.font)         ||
	    (oldtw->primitive.font_list    != newtw->primitive.font_list)    ||
	    (oldtext-> displayPosition     != newtext-> displayPosition)     ||
	    (oldtext-> cursorPosition      != newtext-> cursorPosition)      ||
	    (oldtext-> selectStart         != newtext-> selectStart)         ||
	    (oldtext-> selectEnd           != newtext-> selectEnd)           ||
	    (oldtw-> core.background_pixel != newtw-> core.background_pixel) ||
	    (XtIsSensitive((Widget)oldtw)  != XtIsSensitive((Widget)newtw))  ||
        (oldtw->primitive.shadow_thickness != newtw->primitive.shadow_thickness) ||
	    (oldtw->primitive.font_color   != newtw->primitive.font_color))
	{
	    newtext-> selectMode = 0;
	    redisplay = TRUE;
	    
	    if (oldtext-> wrapMode     != newtext-> wrapMode) 
		newtext-> xOffset = 0;
	}
    }
    
    if (redisplay)
    {
	Boolean save_updateState = newtext-> updateState;
	TextLocation currentDP;
	
	newtext-> prev_width = newtext-> prev_height = 0;
	newtext-> cursor_state = OlCursorOff;
	newtext-> updateState = FALSE;
	
	
	ValidatePositions(newtext);
	ValidateMargins(newtw, newtext);


	
	if (newtext-> vsb != NULL)
	    OlLayoutScrolledWindow((ScrolledWindowWidget)
				   XtParent(XtParent(newtw)), 0);
	else
	    _BuildWrapTable(newtw);
	

	currentDP = _WrapLocationOfLocation(newtext-> wrapTable, 
					    newtext-> displayLocation);
	(void)_AdjustDisplayLocation(newtw, newtext, newtext-> wrapTable, 
				     &currentDP, LINES_VISIBLE(newtw));
	newtext-> save_offset = -1;
	newtext-> updateState = save_updateState;
    }
    else
    {
	newtext-> cursor_state = save_cursorState;
	_TurnTextCursorOff(newtw);
	newtext-> cursor_state = save_cursorState;
    }
    
#ifdef I18N
    /* update the preedit and status colors used by input method
     * if font color or background have changed
     */
    if (newtw->primitive.ic && 
	(oldtw-> core.background_pixel != newtw-> core.background_pixel) ||
	(oldtw->primitive.font_color   != newtw->primitive.font_color)){
	n = 0;
	p_icvalues[n].attr_name = OlNbackground;
	p_icvalues[n].attr_value = (void *)&newtw->core.background_pixel;
	n++;
	p_icvalues[n].attr_name = OlNforeground;
	p_icvalues[n].attr_value = (void *)&newtw->primitive.font_color;
	n++;
	p_icvalues[n].attr_name = (char *)NULL;
	p_icvalues[n].attr_value = (void *)NULL;
	n = 0;
	icvalues[n].attr_name = OlNpreeditAttributes;
	icvalues[n].attr_value = (void *)p_icvalues;
	n++;
	icvalues[n].attr_name = OlNstatusAttributes;
	icvalues[n].attr_value = (void *)p_icvalues;
	n++;
	icvalues[n].attr_name = NULL;
	icvalues[n].attr_value = NULL;
	
	OlSetIcValues(newtw->primitive.ic, icvalues);
    }

     if (newtext-> editMode != oldtext-> editMode){
     /*		Notify drop-site manager that widget has changed interest
      *		in receipt of drops.
      */
	     OlDnDSetDropSiteInterest(newtext->dropsiteid,
				      (newtext->editMode == OL_TEXT_EDIT) ?
				      True : False);
     } 

      /* We need to create or destroy an Input Context if 
       * the edit mode changes
       */
if (newtext-> editMode != oldtext-> editMode)
   {
   if (newtext-> editMode == OL_TEXT_EDIT){
      OlIm *im = ((TextEditWidgetClass) XtClass(newtw))->textedit_class.im;
      if (im){

         toplevel = (Widget)newtw;
         while (!XtIsShell(toplevel))
            toplevel = XtParent(toplevel);
         n = 0;
         icvalues[n].attr_name = OlNclientWindow;
         icvalues[n].attr_value = (void *)&c_win;
         c_win = XtWindow(toplevel);
         n++;
         icvalues[n].attr_name = OlNfocusWindow;
         icvalues[n].attr_value = (void *)&f_win;
         f_win = XtWindow(newtw);
         n++;
         icvalues[n].attr_name = OlNpreeditAttributes;
         icvalues[n].attr_value = (void *)p_icvalues;
         n++;
         icvalues[n].attr_name = OlNstatusAttributes;
         icvalues[n].attr_value = (void *)p_icvalues;
         n++;
         icvalues[n].attr_name = OlNinputStyle;
         icvalues[n].attr_value = (void *)&style;
         n++;
         icvalues[n].attr_name = OlNspotLocation;
         icvalues[n].attr_value = (void *)&spot;
         spot.x = newtw->textedit.cursor_x;
         spot.y = newtw->textedit.cursor_y;
         n++;
         icvalues[n].attr_name = NULL;
         icvalues[n].attr_value = NULL;
         n = 0;
         p_icvalues[n].attr_name = OlNbackground;
         p_icvalues[n].attr_value = (void *)&newtw->core.background_pixel;
         n++;
         p_icvalues[n].attr_name = OlNforeground;
         p_icvalues[n].attr_value = (void *)&newtw->primitive.font_color;
         n++;
         p_icvalues[n].attr_name = OlNfontSet;
         p_icvalues[n].attr_value = (void *)newtw->primitive.font_list;
         n++;
         p_icvalues[n].attr_name = (char *)NULL;
         p_icvalues[n].attr_value = (void *)NULL;

         newtw-> primitive.ic = OlCreateIc(im, icvalues);
	  /* Register the IC with the vendor shell if the
	   * input method uses a status area.
	   * The shell will provide room for the status area.
	   * In response to our specified width of 0,
	   * the shell will use its whole width as the status
	   * area width.  (We have no good way of guessing
	   * an appropriate size.)   Use the maximum font height
	   * for the status area height.
	   */
	if (_OlGetStatusArea())
	    _OlRegisterIc((Widget) newtw, newtw->primitive.ic, 0, 
			  newtw->primitive.font_list->max_bounds.ascent +
			      newtw->primitive.font_list->max_bounds.descent);

	 redisplay = TRUE;
      }
   }
   else
      if (newtw-> primitive.ic){
	  /* unregister the IC from the shell */
	 if (_OlGetStatusArea())
	     _OlUnregisterIc((Widget) newtw, newtw->primitive.ic);
         OlDestroyIc(newtw-> primitive.ic);
         newtw-> primitive.ic = (OlIc *) NULL;
      }
}
#endif
#ifdef DO_YOUR_OWN_REFRESH
if (realized && 
    redisplay && 
    newtext-> updateState && 
    newtw-> core.width  == oldtw-> core.width && 
    newtw-> core.height == oldtw-> core.height)
   {
   TextEditWidget ctx = newtw;
   XRectangle rect;
   rect.x = 0;
   rect.y = 0;
   rect.width = ctx-> core.width;
   rect.height = ctx-> core.height;
   _TurnTextCursorOff(ctx);
   XClearArea(XtDisplay(ctx), XtWindow(ctx), 0, 0, 0, 0, False);
   _DisplayText(ctx, &rect);
   }
#else
return ( realized && redisplay && newtext-> updateState && 
         newtw-> core.width  == oldtw-> core.width && 
         newtw-> core.height == oldtw-> core.height );
#endif

} /* end of SetValues */


/*
 *
 *	InArgList: check if a resource or resource class is specified
 *	in an argument list.
 */
static Boolean
InArgList OLARGLIST((resource, arglist, num_args))
    OLARG(String,	resource)
    OLARG(ArgList,	arglist)
    OLGRA(Cardinal,	num_args)
{
    int i;

    for (i=0; i<num_args; i++){
	if (!strcmp(arglist[i].name, resource))
	    return(True);
    }
    return(False);

}  /* end of InArgList */

/*
 * Destroy
 * 
 */

static void
Destroy OLARGLIST((w))
	OLGRA( Widget,	w)
{
TextEditWidget ctx = (TextEditWidget)w;
Display * dpy = XtDisplay(w);
Window    win = XtWindow(w);
int       i;

FreeTextBuffer(ctx-> textedit.textBuffer, UpdateDisplay, (XtPointer)ctx);

for (i = 0; i <= ctx-> textedit.wrapTable-> used; i++)
	if (ctx-> textedit.wrapTable-> p[i])
      FreeBuffer((Buffer *) ctx-> textedit.wrapTable-> p[i]);
if (ctx-> textedit.wrapTable)
   FreeBuffer((Buffer *) ctx-> textedit.wrapTable);

for (i = 0; i < ctx-> textedit.DTsize; i++)
   if (ctx-> textedit.DT[i].p)
      FREE(ctx-> textedit.DT[i].p);
if (ctx-> textedit.DT)
   FREE(ctx-> textedit.DT);

if (ctx-> textedit.blink_timer != NULL)
   XtRemoveTimeOut(ctx-> textedit.blink_timer);

/* OlUnregisterDynamicCallback(DynamicHandler, (XtPointer)ctx); */

XtRemoveAllCallbacks((Widget)ctx, XtNmotionVerification);
XtRemoveAllCallbacks((Widget)ctx, XtNmodifyVerification);
XtRemoveAllCallbacks((Widget)ctx, XtNpostModifyNotification);
XtRemoveAllCallbacks((Widget)ctx, XtNmargin);
XtRemoveAllCallbacks((Widget)ctx, XtNkeys);
XtRemoveAllCallbacks((Widget)ctx, XtNbuttons);

/*
 *  If we own the CLIPBOARD contents, give up ownership and free the contents.
 */

if (XtIsRealized((Widget)ctx) && XGetSelectionOwner(dpy, XA_CLIPBOARD(dpy)) == win)  
   {
   XSetSelectionOwner(dpy, XA_CLIPBOARD(dpy), 
                      None, _XtLastTimestampProcessed(ctx));  
   FREE(ctx-> textedit.clip_contents);
   }

if (XtIsRealized((Widget)ctx) && ctx->textedit.transient != (Atom)None && 
    XGetSelectionOwner(dpy, ctx->textedit.transient) == win) 
   {
        OlDnDDisownSelection((Widget)ctx, ctx->textedit.transient, CurrentTime);
        if (ctx-> textedit.drag_contents)
	   FREE(ctx-> textedit.drag_contents);
   }

if (ctx->textedit.transient != (Atom)None) {
	   OlDnDFreeTransientAtom((Widget)ctx, ctx->textedit.transient);
	ctx->textedit.transient = (Atom)None;
}

#ifdef I18N
      /* Unregister the IC from the shell.
       * Free the input context for the input method
       */
   if (ctx-> primitive.ic){
       if (_OlGetStatusArea())
	   _OlUnregisterIc((Widget) ctx, ctx->primitive.ic);
      OlDestroyIc(ctx-> primitive.ic);
  }
#endif

} /* end of Destroy */

/*
 * FocusHandler
 */

static void
FocusHandler OLARGLIST((w, highlight_type))
	OLARG( Widget,		w)
	OLGRA( OlDefine,	highlight_type)
{
	Widget			vsw;
	OlVendorPartExtension	pe;
	OlIcValues		icvalues[2];
	OlMAndAList	*	a_m_list;
	int			im_key_index;
	int			num_keys;
	int			i;
	static MAndAList*	merged_list = NULL;
	Boolean			changed = False;

	TextEditWidget ctx = (TextEditWidget)w;

if (merged_list == NULL)
	{
   	merged_list = (MAndAList *) XtMalloc(sizeof(MAndAList));
	merged_list->ol_keys = merged_list-> accelerators = 
	merged_list->mnemonics = NULL;
	merged_list->num_ol_keys = merged_list->num_mnemonics = 
	merged_list->num_accelerators = 0;
	}
switch((int)highlight_type)
   {
   case OL_IN:
      if (HAS_FOCUS(ctx))
         _ChangeTextCursor(ctx);

      /* For editable TextEdit widgets, if preselect resource is on, 
       * select all text upon receipt
       * of focus by all means other than mouse click. Mask is
       * the state of the buttons from a mouse press.
       */
      if (ctx->textedit.preselect  && (ctx-> textedit.editMode == OL_TEXT_EDIT)
	  && !ctx->textedit.mask){
	  SelectAll((Widget) ctx);
	  ctx->textedit.mask = 0;
      }

       /*
	* if using an input method, update the input method about
	* the current set of mouseless key bindings.
	*/
      if (((TextEditWidgetClass) XtClass(ctx))->textedit_class.im && 
          ctx-> primitive.ic) 
	{

	/* get those global keys which are not stored in Vendor shell */
	im_key_index = ctx->textedit.im_key_index;
	a_m_list = NULL;
	num_keys = 0;
	(void)OlGetOlKeysForIm(&a_m_list, &im_key_index, &num_keys);
	if (a_m_list != NULL)
		{
		if (merged_list->ol_keys != NULL)
			XtFree((char *)merged_list->ol_keys);
		merged_list->ol_keys = a_m_list;
		merged_list->num_ol_keys = num_keys;

		ctx->textedit.im_key_index =  im_key_index;
		changed = True;
		}
	/* then, get the list of mnemonics only if they are active ! */
	if (OlQueryMnemonicDisplay(w) != OL_INACTIVE)
	{
	vsw = _OlFindVendorShell(w, True);
	pe = _OlGetVendorPartExtension(vsw);
	if (pe->a_m_index != ctx->textedit.m_index)
		{
		num_keys = 0;
		a_m_list = NULL;
		(void)OlGetMAndAList(pe, &a_m_list, &num_keys, True);
		if (merged_list->mnemonics != NULL)
			XtFree((char *)merged_list->mnemonics);
		merged_list->mnemonics = a_m_list;
		merged_list->num_mnemonics = num_keys;
		changed = True;
		ctx->textedit.m_index = pe->a_m_index;
		}
	}

	/* and now, for the global accelerators (if active) - make sure 
	   no mnemonics are picked up
	*/
	if (OlQueryAcceleratorDisplay(w) != OL_INACTIVE)
	{
	vsw = _OlFindVendorShell(w, False);
	if (vsw)
	   {
	   pe = _OlGetVendorPartExtension(vsw);
	   if (pe->a_m_index != ctx->textedit.a_index)
		{
		num_keys = 0;
		a_m_list = NULL;
		(void)OlGetMAndAList(pe, &a_m_list, &num_keys, False);
		if (merged_list->accelerators != NULL)
			XtFree((char *)merged_list->accelerators);
		merged_list->accelerators = a_m_list;
		merged_list->num_accelerators = num_keys;
		changed = True;
		ctx->textedit.a_index = pe->a_m_index;
		}
          }
	}
	if (changed)
		{
		icvalues[0].attr_name = OlNacceleratorList;
		icvalues[0].attr_value = (void *)merged_list;
		icvalues[1].attr_name = NULL;
		icvalues[1].attr_value = NULL;
		OlSetIcValues(ctx->primitive.ic, icvalues);
		}

        OlSetIcFocus(ctx->primitive.ic);
	}
      break;
   case OL_OUT:
      /* clear the button-press mask for next FOCUS_IN */
      ctx->textedit.mask = 0;

      if (!HAS_FOCUS(ctx))
         _ChangeTextCursor(ctx);
      if (((TextEditWidgetClass) XtClass(ctx))->textedit_class.im && 
          ctx-> primitive.ic) 
         OlUnsetIcFocus(ctx->primitive.ic);
      break;
   default:
      OlVaDisplayWarningMsg(XtDisplay(ctx),
			    OleNfileTextEdit,
			    OleTmsg3,
			    OleCOlToolkitWarning,
			    OleMfileTextEdit_msg3);
   }
	   /* show/hide location cursor in Motif mode */
	if (OlGetGui() == OL_MOTIF_GUI)
	{
#define SUPERCLASS	\
	((TextEditClassRec *)textEditClassRec.core_class.superclass)

		(*SUPERCLASS->primitive_class.highlight_handler)(
					w, highlight_type);

#undef SUPERCLASS
	}

} /* end of FocusHandler */
/*
 * ConvertClipboard: selection conversion routine that is used for
 *	CLIPBOARD selectionand DRAG-n-DROP selection.
 *
 */

static Boolean
ConvertClipboard OLARGLIST((w, selection, target, type_return, value_return, length_return, format_return))
	OLARG( Widget,		w)
	OLARG( Atom *,          selection)
	OLARG( Atom *,          target)
	OLARG( Atom *,          type_return)
	OLARG( XtPointer *,       value_return)
	OLARG( unsigned long *, length_return)
	OLGRA( int *,           format_return)
{
    TextEditWidget  ctx = (TextEditWidget)w;
    Atom * atoms = NULL;
    int	   i;
    char * str;
    char * buffer;
    Boolean retval = False;

    
    Atom TARGETS = OL_XA_TARGETS(XtDisplay(w));
    Atom DELETE = OL_XA_DELETE(XtDisplay(w));

    
    if (*selection == XA_CLIPBOARD(XtDisplay(w)) ||
	*selection == ctx->textedit.transient)
    {
	/* determine what buffer to used based on the selection atom */
	str = (*selection == ctx->textedit.transient) ? 
	    ctx-> textedit.drag_contents : ctx-> textedit.clip_contents;
	if (*target == TARGETS)
	{
	    *format_return = 32;
	    *length_return = (unsigned long)4;
	    atoms = (Atom *)MALLOC((*length_return) * (*format_return));
	    atoms[0] = TARGETS;
	    atoms[1] = XA_STRING;
	    atoms[2] = DELETE;
	    atoms[3] = XInternAtom (XtDisplay(ctx), "LENGTH", False);
	    *value_return = (XtPointer)atoms;
	    *type_return = XA_ATOM;
	    retval = True;
	}
	else if (*target == XA_STRING ||
		 *target == XInternAtom(XtDisplay(ctx), "COMPOUND_TEXT", False))
	{
	    i = strlen(str);
	    
	    buffer = MALLOC(1 + i);
	    memcpy(buffer, str, i);
	    buffer[i] = '\0';
	    
	    *format_return = 8;
	    *length_return = i;
	    *value_return = buffer;
	    *type_return = XA_STRING;
	    
	    retval = True;
	}
	else if (*target == DELETE)
	{
	    /* let CleanupTransaction handle disowning the transient
             * selection and cleaning up the data for Drag and Drop.
	     * Here, just delete data if it is actually the clipboard
	     * that is being deleted
	     */
	    if (*selection == ctx->textedit.transient){
		/* second part of a MOVE operation for Drag
		 * and Drop.  un-select and lose the PRIMARY selection.
		 * Let CleanupTransaction take care of losing
		 * the transient selection and its associated data.
		 */
		OlTextEditInsert(w,"",0);
	    }
	    else{
		/* delete the clipboard data and lose the CLIPBOARD
		 * selection now so future conversion requests
		 * will not come to this widget
		 */
		XtDisownSelection((Widget)ctx, *selection, CurrentTime);
		LoseClipboard((Widget)ctx, selection);
	    }
	    
	    *format_return = 8;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = DELETE;
	    retval = True;
	}
#ifdef sun	/* JMK */
	else if (*target == XInternAtom (XtDisplay(w), "LENGTH", False)) {
	    int *intbuffer;
	    intbuffer = (int *) XtMalloc(sizeof(int));
	    *intbuffer = (int) (strlen (str));
	    *value_return = (XtPointer)intbuffer;
	    *length_return = 1;
	    *format_return = sizeof(int) * 8;
	    *type_return = (Atom) *target;
	    retval = True;
	}
	else {
	    char	*atom;
	    static char prefix[] = "_SUN_SELN";
	    
	    atom = XGetAtomName(XtDisplay(w), *target);
	    if (strncmp(prefix, atom, strlen(prefix)) != 0)
	    retval = False;
	    XFree(atom);
	}
#endif
    }

#ifdef DEBUG_SELECTION
    if (retval == False)
	OlVaDisplayWarningMsg(XtDisplay(w),
			      OleNfileTextEdit,
			      OleTmsg2,
			      OleCOlToolkitWarning,
			      OleMfileTextEdit_msg2);
#endif
    
    return (retval);
    
} /* end of ConvertClipboard */
/*
 * LoseClipboard
 *
 */

static void
LoseClipboard OLARGLIST((w, atom))
	OLARG( Widget,  w)
	OLGRA( Atom *,  atom)
{
	TextEditWidget  ctx = (TextEditWidget)w;

	if (ctx-> textedit.clip_contents != NULL)
	{
	   FREE(ctx-> textedit.clip_contents);
	   ctx-> textedit.clip_contents = NULL;
	}

	if (ctx->textedit.transient != (Atom)None &&
	    ctx->textedit.transient == *atom)
	{
	    OlDnDFreeTransientAtom((Widget)ctx, ctx->textedit.transient);
	    ctx->textedit.transient = (Atom)None;
	}

} /* LoseClipboard */
/*
 * _OlTEPaste
 *
 */

void
_OlTEPaste OLARGLIST((ctx, event, selection, paste_info))
    OLARG(TextEditWidget, ctx)
    OLARG(XEvent *, event)
    OLARG(Atom, selection)
    OLGRA(PasteSelectionRec, *paste_info)
{
    if (paste_info)

	/* for primary selections ask for COMPOUND_TEXT target.  If selection */
	/* is some multi-byte characters coming from Motif, they would get    */
	/* converted into COMPOUND TEXT.  PrimaryPaste() would then convert   */
	/* them to EUC.	 If selection is coming from MoOLIT, COMPOUND_TEXT    */
	/* targets would be treated as XA_STRINGS.  See comment in TextEPos.c */
	/* in the ConvertPrimary() routine.				      */

	XtGetSelectionValue((Widget)ctx, selection,
		        XInternAtom(XtDisplay(ctx), "COMPOUND_TEXT", False),
			PrimaryPaste, 
			(XtPointer)paste_info, _XtLastTimestampProcessed(ctx));
    else
	XtGetSelectionValue((Widget)ctx, selection, XA_STRING, Paste2, 
			NULL, _XtLastTimestampProcessed(ctx));

	
} 
/*end of _OlTEPaste */
/*
 * Paste2: Used for Drag and Drop and Clipboard Paste
 *
 */

static void
Paste2 OLARGLIST((w, client_data, selection, type, value, length, format))
	OLARG( Widget,			w)
	OLARG( XtPointer,		client_data)
	OLARG( Atom *,			selection)
	OLARG( Atom *,			type)
	OLARG( XtPointer,		value)
	OLARG( unsigned long *,		length)
	OLGRA( int *,			format)
{
    TextEditWidget ctx = (TextEditWidget)w;
    char *copy_value;
    Display *dpy = XtDisplay(w);
    Atom COMPOUND_TEXT = XInternAtom (dpy, "COMPOUND_TEXT", False);

	if (*type == COMPOUND_TEXT) {
                int cvt_status = 0;
                XTextProperty text_prop;
                char **cvt_text;
                int cvt_num;

                text_prop.value = value;
                text_prop.encoding = COMPOUND_TEXT;
                text_prop.format = 8;
                text_prop.nitems = *length;

                cvt_status = XmbTextPropertyToTextList(dpy, &text_prop, &cvt_text,
                                                &cvt_num);
                if (cvt_status == Success || cvt_status > 0){
                        value = cvt_text[0];
                        XtFree((char *) cvt_text);
                        *length = strlen(value);
                }
                else
                        *length =  0;
        }
	if ((*length) != 0)
	{
		Boolean send_done = (Boolean)client_data;

		/*
		 * OlTextEditInsert requires a NULL-terminated buffer
		 * Xt doesn't guarantee that there is enough space for the
		 * extra NULL, so we must make a copy.
		 */
		copy_value = XtMalloc(*length +1);
		strncpy(copy_value, (char *)value, (size_t)*length);
		copy_value[*length] = NULL;
		OlTextEditInsert(w, copy_value, (int)*length);
		FREE(value);
		FREE(copy_value);

		if (send_done)
		{
			OlDnDDragNDropDone(
				(Widget)ctx, *selection,
				XtLastTimestampProcessed(XtDisplay(ctx)),
				NULL, NULL);
		}
	}

} /* end of Paste2 */

static void
PrimaryPaste OLARGLIST((w, client_data, selection, type, value, length, format))
	OLARG( Widget,			w)
	OLARG( XtPointer,		client_data)
	OLARG( Atom *,			selection)
	OLARG( Atom *,			type)
	OLARG( XtPointer,		value)
	OLARG( unsigned long *,		length)
	OLGRA( int *,			format)
{
    TextEditWidget            ctx = (TextEditWidget)w;
    PasteSelectionRec *paste_info = (PasteSelectionRec *) client_data;
    TextEditPart            *text = &(ctx-> textedit);
    char *copy_value;
    Display *dpy 		= XtDisplay((Widget)ctx);
    Atom COMPOUND_TEXT 		= XInternAtom (dpy, "COMPOUND_TEXT", False);

   
    if ((*length) <= 0)
	 /*	no selection to copy or cut, do nothing */
	return;

    /*
     *	The buffer will eventually be passed to OlTextEditInsert,
     *	which requires a NULL terminated string.  Since Xt
     *  doesn't guarantee that the buffer will contain an extra
     *  byte, we must make our own copy here.
     */
    copy_value = XtMalloc(*length + 1);

    /* SS: if COMPOUND_TEXT encoding, convert to EUC first */

    if (*type == COMPOUND_TEXT) {
 	int cvt_status = 0;
    	XTextProperty text_prop;
    	char **cvt_text;
    	int cvt_num;
	
    	text_prop.value = value;
    	text_prop.encoding = COMPOUND_TEXT;
    	text_prop.format = 8;
    	text_prop.nitems = *length;

    	cvt_status = XmbTextPropertyToTextList(dpy, &text_prop, &cvt_text,
                                           &cvt_num);
    	if (cvt_status == Success || cvt_status > 0){
        	copy_value = cvt_text[0];
        	XtFree((char *) cvt_text);
        	*length = strlen(copy_value);
    	}
    	else
		*length =  0;
    }
    else
        strncpy(copy_value, (char *)value, (size_t)*length);
    copy_value[*length] = NULL;
    FREE(value);
    /* If this is a cut, we must delete the existing selection
     * before inserting it at the new position
     */
    if (paste_info->cut){
	int select_length = text->selectEnd - text->selectStart;
	TextPosition end = (text->selectStart < text->selectEnd) ?
			    text->selectEnd : (TextPosition) -1;

	/*  If this widget holds the Primary selection, adjust the
	 *  destination for changes due to the cut.  We know that
	 *  the original destination was outside of the Primary selection.
	 */
	if ((select_length > 0)  && (paste_info->destination > end))
	    paste_info->destination -= select_length;
	/* 
	 * save the text from the primary selection
	 */
	paste_info->text = copy_value;
	XtGetSelectionValue((Widget)ctx, *selection,
			    OL_XA_DELETE(dpy), PrimaryCut, 
			(XtPointer)paste_info, _XtLastTimestampProcessed(ctx));
    }
    else{
	/* 
	 * 	Move cursor to insert point and insert text.
	 *	Restore original selection (if any) and cursor
	 *	position.
	 */
	TextPosition saveStart = text->selectStart;
	TextPosition saveEnd = text->selectEnd;
	TextPosition saveCursor = text->cursorPosition;

	/* set cursor to insert point */
	OlTextEditSetCursorPosition (w, 
				     paste_info->destination,
				     paste_info->destination,
				     paste_info->destination);
	/* insert the text */
	OlTextEditInsert(w, copy_value, (*length));
	/* Restore the original selection.  If text was 
	 * inserted before the selection, adjust
	 * position values to accommodate
	 * newly inserted text.
	 */
	if (paste_info->destination < saveStart){
	    saveStart += (*length);
	    saveEnd += (*length);
	    saveCursor += (*length);
	}
	OlTextEditSetCursorPosition (w, saveStart, saveEnd, saveCursor);
	XtFree((char *)paste_info);
	FREE(copy_value);	 
    }
} /* end of PrimaryPaste */

static void
PrimaryCut OLARGLIST((w, client_data, selection, type, value, length, format))
	OLARG( Widget,			w)
	OLARG( XtPointer,		client_data)
	OLARG( Atom *,			selection)
	OLARG( Atom *,			type)
	OLARG( XtPointer,		value)
	OLARG( unsigned long *,		length)
	OLGRA( int *,			format)
{
    TextEditWidget		ctx = (TextEditWidget)w;
    PasteSelectionRec *paste_info = (PasteSelectionRec *) client_data;
 
    OlTextEditSetCursorPosition (w, 
				 paste_info->destination,
				 paste_info->destination,
				 paste_info->destination);
    /*
     * paste_info->text has already been NULL-terminated for us
     */
    OlTextEditInsert(w, paste_info->text, strlen(paste_info->text));
    /*
     *	Make sure that the new selection becomes the current
     *	selection.
     */
    OlTextEditSetCursorPosition (w, 
				 paste_info->destination,
				 paste_info->destination + _mbstrlen(paste_info->text),
				 paste_info->destination + _mbstrlen(paste_info->text));
    XtFree(paste_info->text);
    XtFree((char *)paste_info);
    if ((*length) > 0)
	FREE(value);
} /* end of PrimaryCut */
static Boolean
ActivateWidget OLARGLIST((w, type, call_data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	call_data)
{
    Boolean consumed = True;
    TextEditWidget ctx = (TextEditWidget) w;


    switch (type)
    {
    case OL_COPY:
	(void) OlTextEditCopySelection(w, False);
	break;
    case OL_CUT:
	(void) OlTextEditCopySelection(w, True);
	break;
    case OL_DELCHARFWD:
    case OL_DELCHARBAK:
    case OL_DELWORDFWD:
    case OL_DELWORDBAK:
    case OL_DELLINEFWD:
    case OL_DELLINEBAK:
    case OL_DELLINE:
	Delete(ctx, type, 0);
	break;
    case OL_PASTE:
	_OlTEPaste(ctx, NULL, XA_CLIPBOARD(XtDisplay(w)), NULL);
	break;
    case OLM_KSelectAll:
	SelectAll(w);
	break;
    case OLM_KDeselectAll:
	DeselectAll(w);
	break;
    case OL_UNDO:
	UndoUpdate(w);
	break;
    default:
	consumed = False; /* oops, wrong guess */
	break;
    }
    return (consumed);
} /* end of ActivateWidget */

/*
 * _OlSetClickMode
 *
 */
extern void _OlSetClickMode(mode)
int mode;
{
   ((TextEditClassRec *)textEditWidgetClass)->textedit_class.click_mode = mode;
} /* end of _OlSetClickMode */
/*
 * _OlTESelect
 *
 * The \fI_OlTESelect\fR procedure is called when a SELECT mouse press is
 * discovered.	The routine determines what kind of SELECT activity
 * the user is attempting: a simple click - to extend the selection
 * to the point of the click, a double click  - to select the next
 * larger unit of text (chr, word, line, paragraph, document, char...),
 * a drag and drop operation - to copy a selection,
 * or if the user is attempting to alter the selection by dragging SELECT.  
 * In the latter case the selection is selection is initiated here and 
 * a poll loop is established using PollMouse.	The poll loop will 
 * continue to extend the selection until the state of the mouse 
 * changes (i.e., the user releases the SELECT button).
 *
 */

void
_OlTESelect(ctx, event)
TextEditWidget ctx;
XEvent *       event;
{
TextEditPart * text = &ctx-> textedit;
TextPosition position;

switch(OlDetermineMouseAction((Widget)ctx, event))
   {
   case MOUSE_MOVE:
      position = _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y);
      if (text-> selectStart <= position && position <= text-> selectEnd - 1)
	 _OlTEDragText(ctx, text, position, OlMoveDrag, False);
      else
	 {
	 _MoveSelection(ctx, position, 0, 0, 0);
	 text-> shouldBlink = FALSE;
	 text-> mask = 
	    event-> xbutton.state | 1<<(event-> xbutton.button+7);
	 _TurnTextCursorOff(ctx);
	 OlAddTimeOut((Widget)ctx, /* DELAY */ 0, PollMouse, (XtPointer)ctx);
	 }
      break;
   case MOUSE_CLICK:
      _MoveSelection
	 (ctx, _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y), 0, 0, 0);
      break;
   case MOUSE_MULTI_CLICK:
      if (++text-> selectMode == 5)
	 text-> selectMode = 0;
      _MoveSelection
	 (ctx, _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y), 0, 0, text-> selectMode);
      break;
   default:
      break;
   }

} /* end of _OlTESelect */
/*
 * _OlTEAdjust
 *
 * The \fI_OlTEAdjust\fR procedure is called when a ADJUST mouse press is
 * discovered.	The routine determines what kind of ADJUST activity
 * the user is attempting: a simple click - to extend the selection
 * to the point of the click (double click is considered the same as
 * two sequential single clicks) or if the user is attempting to
 * alter the selection by dragging the ADJUST.	In the latter case
 * the slection is adjusted here and a poll loop is established using
 * PollMouse.  The poll loop will continue to extend the selection
 * until the state of the mouse changes (i.e., the user releases the
 * ADJUST button).
 *
 */

void
_OlTEAdjust(ctx, event)
TextEditWidget ctx;
XEvent *       event;
{
TextEditPart * text = &ctx-> textedit;

switch(OlDetermineMouseAction((Widget)ctx, event))
   {
   case MOUSE_MOVE:
      _MoveSelection
	 (ctx, _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y), 0, 0, 5);
      text-> shouldBlink = FALSE;
      text-> mask = 
	 event-> xbutton.state | 1<<(event-> xbutton.button+7);
      _TurnTextCursorOff(ctx);
      OlAddTimeOut((Widget)ctx, /* DELAY */ 0, PollMouse, (XtPointer)ctx);
      break;
   case MOUSE_MULTI_CLICK:
   case MOUSE_CLICK:
      _MoveSelection
	 (ctx, _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y), 0, 0, 5);
      _TurnTextCursorOff(ctx);
      text-> cursor_state = OlCursorBlinkOff;	/* force cursor to blink */
      break;
   default:
      break;
   }

} /* end of _OlTEAdjust */
/*
 * _OlTEPollPan
 *
 * The \fI_OlTEPollPan\fR procedure is used to poll the mouse until the press
 * which initiated the pan is released.	 
 *
 * Note: This routine currently constrains panning to vertical movement.
 * Care must be taken to intelligently support horizontal panning
 * since it is unlike a user will very often want to use it and a fine
 * grain will cause lots of activity (i.e., redraws and copy areas)
 *
 */

void
_OlTEPollPan OLARGLIST((client_data, id))
	OLARG( XtPointer,	client_data)
	OLGRA( XtIntervalId *,	id)
{
TextEditWidget ctx  = (TextEditWidget)client_data;
TextEditPart * text = &ctx-> textedit;

/* for query pointer */
Window		  root;
Window		  child;
int		  rootx, rooty, winx, winy;
unsigned int	  mask;

XQueryPointer(XtDisplay(ctx), XtWindow(ctx), 
	      &root, &child, &rootx, &rooty, &winx, &winy, &mask);

if (mask != text-> mask)
   {
   text-> shouldBlink = TRUE; /* let the timer expire */
   OlUngrabDragPointer((Widget)ctx);
   }
else
   {
   int vdelta = (text->PanY - (winy - (int)PAGE_T_MARGIN(ctx))) / FONTHT(ctx);

#ifdef SUPPORT_HORIZONTAL_PAN
   int hdelta = text-> PanX - winx;
   TextEditPart * text = &ctx-> textedit;
   if (vdelta && text-> wrapMode == OL_WRAP_OFF && text-> maxX > PAGEWID(ctx))
      _MoveDisplayLaterally(ctx, &ctx-> textedit, vdelta, TRUE);
#endif

   if (vdelta)
      text-> PanY-= (_MoveDisplayPosition(ctx, id, OL_PGM_GOTO, vdelta) * FONTHT(ctx));

   OlAddTimeOut((Widget)ctx, /* DELAY */ 0, _OlTEPollPan, (XtPointer)ctx);
   }

} /* end of _OlTEPollPan */
/*
 * PollMouse
 *
 * The \fIPollMouse\fR procedure is used to poll the mouse until
 * the state of the mouse changes relative to when the poll was started
 * (e.g., as a result of a Select or Adjust drag).  If the state
 * remains the same the selection is extended (by calling _MoveSelection)
 * and another poll is scheduled.  If the state of the mouse changes
 * the poll is ignored and the polling is stopped.
 *
 */

static void
PollMouse OLARGLIST((client_data, id))
	OLARG( XtPointer,	client_data)
	OLGRA( XtIntervalId *,	id)
{
TextEditWidget ctx  = (TextEditWidget)client_data;
TextEditPart * text = &ctx-> textedit;

/* for query pointer */
Window		  root;
Window		  child;
int		  rootx, rooty, winx, winy;
unsigned int	  mask;

XQueryPointer(XtDisplay((Widget)ctx), XtWindow((Widget)ctx), 
	      &root, &child, &rootx, &rooty, &winx, &winy, &mask);

if (mask != text-> mask)
   {
   text-> shouldBlink = TRUE; /* let the timer expire */
   XUngrabPointer(XtDisplay((Widget)ctx), CurrentTime);
   text-> selectMode = 0;
   text-> cursor_state = OlCursorOn;
   _TurnTextCursorOff(ctx);
   text-> cursor_state = OlCursorOn;
   }
else
   {
   _MoveSelection(ctx, _PositionFromXY(ctx, winx, winy), 0, 0, 6);
   OlAddTimeOut((Widget)ctx, /* DELAY */ 0, PollMouse, (XtPointer)ctx);
   }

} /* end of PollMouse */

static void
CleanupTransaction OLARGLIST(( widget, selection, state, timestamp, closure))
		OLARG(Widget,		      widget)
		OLARG(Atom,		      selection)
		OLARG( OlDnDTransactionState, state)
		OLARG( Time,		      timestamp)
		OLGRA( XtPointer,	      closure)
{
    TextEditWidget	ctx = (TextEditWidget)widget;
    

    switch (state) {
    case OlDnDTransactionDone:
    case OlDnDTransactionRequestorError:
    case OlDnDTransactionRequestorWindowDeath:
	if (selection != ctx->textedit.transient)
	    break;
	OlDnDDisownSelection(widget, selection, CurrentTime);
	OlDnDFreeTransientAtom(widget, ctx->textedit.transient);
	ctx->textedit.transient = (Atom)NULL;
	if (ctx->textedit.drag_contents != (char *)NULL) {
	       /* free local copy of the PRIMARY selection */
	    FREE(ctx->textedit.drag_contents);
	    ctx->textedit.drag_contents = (char *)NULL;
	}
	   /* If selection was not deleted (by a MOVE) operation
	    * unhighlight it.
	    */
	if (ctx-> textedit.selectStart != ctx-> textedit.selectEnd){
	    OlTextEditSetCursorPosition (widget,
					 ctx-> textedit.cursorPosition, 
					 ctx-> textedit.cursorPosition,
					 ctx-> textedit.cursorPosition);
	    XtDisownSelection((Widget)ctx, XA_PRIMARY, CurrentTime);
	}
	break;
    case OlDnDTransactionEnds:
    case OlDnDTransactionBegins:
	;
    }
} /* end of CleanupTransaction */

/*
 * _OlTEDragText
 *
 * The \fI_OlTEDragText\fR procedure handles the drag-and-drop operation.
 * It creates the cursor to be used during the drag and calls the utility
 * drag and drop functions to monitor the drag.	 Once the user has dropped
 * the text the _OlTEDragText procedure determines where the drop was made.
 * If the drop occurs on the widget where the drag initiated then either
 * the drop point was in the midst of the selection - indicating that
 * the user wished to abort the drop or the drop was outside the selection
 * indicating that the text is to be copied at the drop point.
 * If the drop occurs on another window the SendPasteMessage function
 * is called to tell the drop window that text had been dropped on
 * it - leaving the transfer of data to the dropee.
 */

void 
_OlTEDragText OLARGLIST((ctx, text, position, drag_mode, from_kbd))
    OLARG(TextEditWidget,	ctx)
    OLARG(TextEditPart *,	text)
    OLARG(TextPosition,	position)
    OLARG(OlDragMode,	drag_mode)
    OLGRA(Boolean,		from_kbd)	/* True means OL_DRAG	*/
{
    Display * dpy = XtDisplay(ctx);
    Window	  win = RootWindowOfScreen(XtScreen(ctx));
    char      buffer[4];
    int	      len;
    int	      size;
    static GC	  CursorGC;
    static Cursor DragCursor = NULL;
    
    OlDnDDragDropInfo	root_info;
    OlDnDAnimateCursors	cursors;
    OlDnDDestinationInfo	dst_info;
    OlDnDDropStatus		status;
    
    static Pixmap CopySource;
    static Pixmap CopyMask;
    static Pixmap MoveSource;
    static Pixmap MoveMask;
    static Pixmap MoreArrow;
    
#include <morearrow.h>
#include <copysrc.h>
#include <copymsk.h>
#include <movesrc.h>
#include <movemsk.h>
    
    
    if (text->flyingCursor == (Cursor)NULL) {
	if (DragCursor != NULL)
	    XFreeCursor(XtDisplay(ctx), DragCursor);
	else
	{
	    XRectangle	 rect;
	    
	    rect.x	= 13;
	    rect.y	= 10;
	    rect.width	= 33;
	    rect.height = 12;
	    
	    MoreArrow  = XCreateBitmapFromData(dpy, win,  
					       (OLconst char *)morearrow_bits,
					       morearrow_width, 
					       morearrow_height);
	    CopySource = XCreateBitmapFromData(dpy, win,
					       (OLconst char *)copysrc_bits,
					       copysrc_width, 
					       copysrc_height);
	    CopyMask   = XCreateBitmapFromData(dpy, win,
					       (OLconst char *)copymsk_bits,
					       copymsk_width, 
					       copymsk_height);
	    MoveSource = XCreateBitmapFromData(dpy, win,
					       (OLconst char *)movesrc_bits,
					       movesrc_width, 
					       movesrc_height);
	    MoveMask   = XCreateBitmapFromData(dpy, win,
					       (OLconst char *)movemsk_bits,
					       movemsk_width, 
					       movemsk_height);
	    CursorGC = XCreateGC(dpy, CopySource, 0L, NULL);
	    XSetClipRectangles(dpy, CursorGC, 0, 0, &rect, 1, Unsorted);
	}
	
	len = CopyTextBufferBlock(text-> textBuffer, buffer, 
				  text-> selectStart, 
				  _OlMin(text-> selectStart + 3, text-> selectEnd) - 1);
	size = text-> selectEnd - text-> selectStart;
	
	/* SUN changed _CreateCursorFromBitmaps interface */
	/* 2nd parameter from Screen * to Widget...	  */
	if (drag_mode == OlCopyDrag)
	    DragCursor = _CreateCursorFromBitmaps (dpy, XtScreen(ctx), 
						   CopySource, CopyMask, 
						   CursorGC, "black", "white",
						   buffer, len, 13, 19, 1, 1, 
						   size < 4 ? 0 : MoreArrow);
	else
	    DragCursor = _CreateCursorFromBitmaps (dpy, XtScreen(ctx), 
						   MoveSource, MoveMask,  
						   CursorGC, "black", "white",
						   buffer, len, 13, 19, 1, 1, 
						   size < 4 ? 0 : MoreArrow);
    }
    cursors.yes_cursor = DragCursor;
    cursors.no_cursor = OlGetNoCursor(XtScreenOfObject((Widget)ctx));
    
    if ((status = OlDnDTrackDragCursor((Widget)ctx, &cursors, &dst_info, 
				       &root_info) != OlDnDDropCanceled)) {
	TextDropOnWindow((Widget)ctx, text, drag_mode, status, &dst_info, 
			 &root_info);
    }
} /* end of _OlTEDragText */

/*
 *	TextDropOnWindow: current widget is the source in a drag/drop
 * operation.
 */
static void
TextDropOnWindow OLARGLIST((ctx, text, drag_mode, drop_status, dst_info, root_info))
    OLARG(Widget, ctx)
    OLARG(TextEditPart *, text)
    OLARG(OlDragMode, drag_mode)
    OLARG(OlDnDDropStatus, drop_status)
    OLARG(OlDnDDestinationInfoPtr, dst_info)
    OLGRA(OlDnDDragDropInfoPtr, root_info)
{
#define DROP_WINDOW		dst_info->window
#define X			dst_info->x
#define Y			dst_info->y
    
    Widget		drop_widget;
    Display *		dpy = XtDisplay(ctx);
    Window		win = DefaultRootWindow(dpy);
    Widget		shell;
    TextPosition	position;
    char *		buf;
    
    drop_widget = XtWindowToWidget(dpy, DROP_WINDOW);
    
    shell  = (	drop_widget == NULL || 
	      DROP_WINDOW == RootWindowOfScreen(XtScreen(ctx))) ? 
		  NULL : _OlGetShellOfWidget(drop_widget);

    if (drop_widget == ctx)
    {
	/* Optimization: When the Desitination widget is the same as the
	 * source widget, we can bypass use of the transient selection and
	 * use the PRIMARY selection directly.
	 */
	position = _PositionFromXY(ctx, X, Y);
	if (text-> selectStart <= position && position <= text-> selectEnd - 1)
		/* can't drop selection into selection */
	    ;
	else
	{
	       /* make a copy of the current selection */
	    OlTextEditReadSubString(ctx, &buf, text-> selectStart, 
				    text-> selectEnd - 1);
	    if (drag_mode == OlMoveDrag){
		position = AdjustPosition(position, 
					  text->selectEnd - text-> selectStart,
					  0, 
					  text->selectStart, text-> selectEnd);
		
		/* delete the selection if doing a Move */
		OlTextEditInsert(ctx, "", 0);
	    }
	       /* set the cursor position to based on the drop position */
	    OlTextEditSetCursorPosition (ctx, position, position, position);
	       /* copy the text to the position */
	    OlTextEditInsert(ctx, buf, strlen(buf));
	    XtFree(buf);
	}
    }
    else  
    {
	Boolean got_selection;
	
	/* Sam C.						*/
	if (drop_status == OlDnDDropFailed)
	{ 
	/*FLH should also send out Open Window 2.0 DnD...		*/
	    /* FLH this is copying to the clipboard, but should use 
	       another selection
	       */
	    OlTextEditCopySelection(ctx, drag_mode == OlCopyDrag ? False : True);
	    SendPasteMessage((TextEditWidget)ctx, DROP_WINDOW, X, Y, drag_mode);
	    return;
	}
	
	if (text->transient == (Atom)NULL) {
	    text->transient = OlDnDAllocTransientAtom(ctx);
	    got_selection = False;
	} 
	else 
	    got_selection = True;
	
	if (!got_selection)
	    got_selection = OlDnDOwnSelection((Widget)ctx, text->transient,
					      root_info->drop_timestamp,
					      ConvertClipboard, LoseClipboard, 
					      (XtSelectionDoneProc)NULL, 
					      CleanupTransaction, NULL);
	if (got_selection) {
	    char ** ptr = &text-> drag_contents;
	    if (text-> drag_contents != NULL) {
		FREE(text-> drag_contents);
		text->drag_contents = (char *)NULL;
	    }
	    /* copy the selected text into buffer */
	    OlTextEditReadSubString(ctx, ptr, text-> selectStart,
				    text-> selectEnd - 1);


	    
	    /* send trigger message to destination (drop site) client */
	    if (!OlDnDDeliverTriggerMessage(ctx, root_info->root_window,
					    root_info->root_x,
					    root_info->root_y,
					    text->transient,
					    (drag_mode == OlMoveDrag ?
					 OlDnDTriggerMoveOp: 
					     OlDnDTriggerCopyOp),
					    root_info->drop_timestamp))
	    {
		/* something is wrong...	*/
		/* FLH we should probably free the transient
		 *	atom and clipboard contents
		 */
	    }
	}
    } /* else */
    
#undef DROP_WINDOW
#undef X
#undef Y
} /* end of TextDropOnWindow */

/*
 * ReceivePasteMessage
 *
 */

static void
ReceivePasteMessage OLARGLIST((w, event, params, num_params))
    OLARG(Widget,   w)
    OLARG(XEvent *, event)
    OLARG(String *, params)
    OLGRA(Cardinal *, num_params)
{
TextEditWidget ctx  = (TextEditWidget) w;
TextEditPart * text = &ctx-> textedit;
Position       x;
Position       y;
int	       operation;
Atom OL_PASTE_MSG = XInternAtom(XtDisplay(ctx), "OL_PASTE_MSG", False);

if (event-> xclient.message_type == OL_PASTE_MSG)
   {
   x = (Position)event-> xclient.data.l[0];
   y = (Position)event-> xclient.data.l[1];
   operation = (int)event-> xclient.data.l[2];

   if (_MoveSelection(ctx, _PositionFromXY(ctx, x, y), 0, 0, 0))
      _OlTEPaste(ctx, event, XA_CLIPBOARD(XtDisplay(w)), NULL);
   }

} /* end of ReceivePasteMessage */
/*
 * SendPasteMessage
 *
 */

static void
SendPasteMessage OLARGLIST((ctx, window, x, y, operation))
    OLARG(TextEditWidget, ctx)
    OLARG(Window, window)
    OLARG(Position, x)
    OLARG(Position, y)
    OLGRA(int, operation)
{
XEvent	       event;
Status	       Result;
Atom OL_PASTE_MSG = XInternAtom(XtDisplay(ctx), "OL_PASTE_MSG", False);

event.xclient.type	   = ClientMessage;
event.xclient.display	   = XtDisplay(ctx);
event.xclient.window	   = window;
event.xclient.message_type = OL_PASTE_MSG;
event.xclient.format	   = 32;
event.xclient.data.l[0]	   = (long)x;
event.xclient.data.l[1]	   = (long)y;
event.xclient.data.l[2]	   = (long)operation;

Result = XSendEvent(XtDisplay(ctx), window, False, NoEventMask, &event);

/*
 * check the result and report failure !!!
 */

XSync(XtDisplay(ctx), False);

} /* end of SendPasteMessage */
/*
 * PopdownTextEditMenuCB (XtNpopdownCallback)
 */

static void
PopdownTextEditMenuCB OLARGLIST((w, client_data, call_data))
    OLARG(Widget, w)
    OLARG(XtPointer, client_data)
    OLGRA(XtPointer, call_data)
{
	XtDestroyWidget(w);
	return;
} /* end of PopdownTextEditMenuCB */
/*
 * MenuSelect
 */

static void MenuSelect(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
OlFlatCallData * p = (OlFlatCallData *)call_data;
item * q = (item *)p-> items;
item * r = &q[p-> item_index];

switch (p-> item_index)
   {
   case UNDO_ITEM:
      MenuUndo(w, client_data, NULL);
      break;
   case CUT_ITEM:
      MenuCut(w, client_data, NULL);
      break;
   case COPY_ITEM:
      MenuCopy(w, client_data, NULL);
      break;
   case PASTE_ITEM:
      MenuPaste(w, client_data, NULL);
      break;
   case DELETE_ITEM:
      MenuDelete(w, client_data, NULL);
      break;
   }

} /* end of MenuSelect */
/*
 * _OlTEPopupTextEditMenu
 *
 */

void
_OlTEPopupTextEditMenu OLARGLIST((ctx, activation_type, root_x, root_y, x, y))
    OLARG(TextEditWidget, ctx)
    OLARG(OlVirtualName, activation_type)
    OLARG(Position, root_x)
    OLARG(Position, root_y)
    OLARG(Position, x)
    OLGRA(Position, y)
{
TextEditPart * text	  = &ctx-> textedit;
int	       canedit	  = text-> editMode == OL_TEXT_EDIT; 
TextBuffer *   textBuffer = text-> textBuffer;
Arg	       arg[10];
Widget	       MenuShell;
Widget	       FlatButtons;
Display *      dpy = XtDisplay(ctx);

static char * fields[] = { XtNlabel, XtNmnemonic, XtNsensitive };

#define SOMETHING_IS_SELECTED (text-> selectEnd != text-> selectStart)
#define SOMETHING_TO_PASTE    (1) /* FIX: determine is clipboard has anything */
#define SOMETHING_TO_UNDO \
   (textBuffer-> insert.string != NULL || textBuffer-> deleted.string != NULL)

MenuShell =
   XtCreatePopupShell("olTextEditMenu", popupMenuShellWidgetClass, (Widget)ctx, NULL, 0);

XtAddCallback(MenuShell, XtNpopdownCallback, PopdownTextEditMenuCB, NULL);

XtSetArg(arg[0], XtNitemFields, fields);
XtSetArg(arg[1], XtNnumItemFields, XtNumber(fields));
XtSetArg(arg[2], XtNselectProc, MenuSelect);
XtSetArg(arg[3], XtNclientData, ctx);
FlatButtons = XtCreateManagedWidget("pane",
   flatButtonsWidgetClass, MenuShell, arg, 4);

#define SET_SENSITIVE(N,COND) \
	OlVaFlatSetValues(FlatButtons,N,XtNsensitive, \
	(XtArgVal)((N == COPY_ITEM || canedit) && COND),(String)0)

SET_SENSITIVE (UNDO_ITEM,   SOMETHING_TO_UNDO);
SET_SENSITIVE (CUT_ITEM,    SOMETHING_IS_SELECTED);
SET_SENSITIVE (COPY_ITEM,   SOMETHING_IS_SELECTED);
SET_SENSITIVE (PASTE_ITEM,  SOMETHING_TO_PASTE);
SET_SENSITIVE (DELETE_ITEM, SOMETHING_IS_SELECTED);
#undef	SET_SENSITIVE

OlPostPopupMenu((Widget)ctx, MenuShell, activation_type,
		(OlPopupMenuCallbackProc)NULL,
		root_x, root_y, x, y);

} /* end of _OlTEPopupTextEditMenu */
/*
 * MenuUndo
 *
 */

static void
MenuUndo OLARGLIST((w, client_data, call_data))
    OLARG(Widget, w)
    OLARG(XtPointer, client_data)
    OLGRA(XtPointer, call_data)
{
TextEditWidget ctx = (TextEditWidget)client_data;

UndoUpdate(ctx);

} /* end of MenuUndo */
/*
 * MenuCut
 *
 */

static void
MenuCut OLARGLIST((w, client_data, call_data))
    OLARG(Widget, w)
    OLARG(XtPointer, client_data)
    OLGRA(XtPointer, call_data)
{
(void) OlTextEditCopySelection((Widget)client_data, True);
} /* end of MenuCut */
/*
 * MenuCopy
 *
 */

static void
MenuCopy OLARGLIST((w, client_data, call_data))
    OLARG(Widget, w)
    OLARG(XtPointer, client_data)
    OLGRA(XtPointer, call_data)
{
(void) OlTextEditCopySelection((Widget)client_data, False);
} /* end of MenuCopy */
/*
 * MenuPaste
 *
 */

static void
MenuPaste OLARGLIST((w, client_data, call_data))
    OLARG(Widget, w)
    OLARG(XtPointer, client_data)
    OLGRA(XtPointer, call_data)
{
TextEditWidget ctx = (TextEditWidget)client_data;
Display *      dpy = XtDisplay(ctx);

XtGetSelectionValue((Widget)ctx, XA_CLIPBOARD(dpy), 
		    XA_STRING, Paste2, NULL, _XtLastTimestampProcessed(ctx));

} /* end of MenuPaste */
/*
 * MenuDelete
 *
 */

static void
MenuDelete OLARGLIST((w, client_data, call_data))
    OLARG(Widget, w)
    OLARG(XtPointer, client_data)
    OLGRA(XtPointer, call_data)
{
OlTextEditInsert((Widget)client_data, "", 0);
} /* end of MenuDelete */
/*
 * OlTextEditClearBuffer
 *
 * The \fIOlTextEditClearBuffer\fR function is used to delete all of the text
 * associated with the TextEdit widget \fIctx\fR.
 * 
 * Return value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget or
 * if the clear operation fails; otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditUpdate(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditClearBuffer(w)
Widget w;
{
TextEditWidget ctx = (TextEditWidget)w;
Boolean retval;

if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
   retval = FALSE;
else
   {
   ctx-> textedit.selectStart = 0;
   ctx-> textedit.selectEnd = LastTextBufferPosition(ctx->textedit.textBuffer);
   retval = OlTextEditInsert(w, "", 0);
   }

return (retval);

} /* end of OlTextEditClearBuffer */
/*
 * OlTextEditCopyBuffer
 *
 * The \fIOlTextEditCopyBuffer\fR function is used to retrieve a copy 
 * of the TextBuffer associated with the TextEdit Widget
 * \fIctx\fR.  The storage required for the copy is allocated
 * by this routine; it is the responsibility of the caller to
 * free this storage when appropriate.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget or
 * if the buffer cannot be read; otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditReadSubString(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditCopyBuffer(w, buffer)
Widget          w;
char **		buffer;
{
TextEditWidget	ctx = (TextEditWidget)w;
Boolean retval;

if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
   retval = FALSE;
else
   {
   TextBuffer * textBuffer = ctx-> textedit.textBuffer;

   if (TextBufferEmpty(textBuffer))
      {
      retval = (Boolean)TRUE;
      *buffer = (char *)strdup("");
      }
   else
      retval = 
	 OlTextEditReadSubString(w, buffer,
	   0, LastTextBufferPosition(textBuffer) - 1);
   }

return (retval);

} /* end of OlTextEditCopyBuffer */
/*
 * OlTextEditCopySelection
 *
 * The \fIOlTextEditCopySelection\fR function is used to Copy or Cut the current
 * selection in the TextEdit \fIctx\fR.	 If no selection exists or if the
 * TextEdit cannot acquire the CLIPBOARD FALSE is returned.
 * Otherwise, the selection is copied to the CLIPBOARD, then
 * if the \fIdelete\fR flag 
 * is non-zero, the text is then deleted from the TextBuffer associated 
 * with the TextEdit widget (i.e., a CUT operation is performed). Finally, 
 * TRUE is returned.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget or
 * if the operation fails; otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditUpdate(3), OlTextEditGetCursorPosition(3), 
 * OlTextEditSetCursorPosition(3), OlTextEditReadSubString(3),
 * OlTextEditCopyBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditCopySelection(w, delete)
Widget         w;
int	       delete;
{
TextEditWidget ctx    = (TextEditWidget)w;
TextEditPart * text   = &ctx-> textedit;
char **	       ptr    = &text-> clip_contents;
Boolean	       retval = False;
Display *      dpy    = XtDisplay(ctx);

if (text-> selectStart == text-> selectEnd)
   {
   XBell(dpy, 0);
   }
else 
   {
   if (!XtOwnSelection((Widget)ctx, XA_CLIPBOARD(dpy), 
       _XtLastTimestampProcessed(ctx), ConvertClipboard, LoseClipboard, NULL)) 
     OlVaDisplayWarningMsg(XtDisplay(ctx),
			   OleNfileTextEdit,
			   OleTmsg1,
			   OleCOlToolkitWarning,
			   OleMfileTextEdit_msg1,
			   "Copy Selection");
   else
      {
      if (text-> clip_contents != NULL)
      {
	 FREE(text-> clip_contents);
	 text->clip_contents = (char *)NULL;
      }

      OlTextEditReadSubString(w, ptr, text-> selectStart, text-> selectEnd - 1);

      if (delete)
	 OlTextEditInsert(w, "", 0);
      else
	 OlTextEditSetCursorPosition (w,
	   text-> cursorPosition, text-> cursorPosition, text-> cursorPosition);
      retval = True;
      }
   }

return (retval);

} /* end of OlTextEditCopySelection */
/*
 * OlTextEditGetCursorPosition
 *
 * The \fIOlTextEditGetCursorPosition\fR function is used to retrieve the
 * current selection \fIstart\fR and \fIend\fR and \fIcursorPosition\fR.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget; 
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditSetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditGetCursorPosition(w, start, end, cursorPosition)
Widget         w;
TextPosition * start;
TextPosition * end;
TextPosition * cursorPosition;
{
TextEditWidget ctx = (TextEditWidget)w;
Boolean retval;

if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
   retval = FALSE;
else
   {
   TextEditPart * text = &ctx-> textedit;

   *start	   = text-> selectStart;
   *end		   = text-> selectEnd;
   *cursorPosition = text-> cursorPosition;
   retval = TRUE;
   }

return (retval);

} /* end of OlTextEditGetCursorPosition */
/*
 * OlTextEditGetLastPosition
 *
 * The \fIOlTextEditGetLastPosition\fR function is used to retrieve the
 * \fIposition\fR of the last character in the TextBuffer associated with
 * the TextEdit widget \fIctx\fR.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget; 
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditGetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditGetLastPosition(w, position)
Widget         w;
TextPosition * position;
{
TextEditWidget ctx = (TextEditWidget)w;
Boolean retval;

if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
   retval = FALSE;
else
   {
   TextBuffer * textBuffer = ctx-> textedit.textBuffer;

   *position = LastTextBufferPosition(textBuffer);
   retval = TRUE;
   }

return (retval);

} /* end of OlTextEditGetLastPosition */
/*
 * OlTextEditInsert
 *
 * The \fIOlTextEditInsert\fR function is used to insert a \fIbuffer\fR
 * containing \fIlength\fR bytes in the TextBuffer associated with
 * the TextEdit widget \fIctx\fR.  The inserted text replaces the
 * current (if any) selection. The \fIbuffer\fR is assumbed to be
 * null terminated.
 *
 * Return Value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget or
 * if the insert operation fails; otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditGetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */
extern Boolean
OlTextEditInsert(w, buffer, length)
Widget         w;
String	       buffer;
int	       length;
{
TextEditWidget       ctx = (TextEditWidget)w;
TextEditPart *	     text	= &ctx-> textedit;
TextBuffer *	     textBuffer = text-> textBuffer;
TextLocation	     startloc;
TextLocation	     endloc;
OlTextModifyCallData call_data;
Boolean		     retval	= FALSE;
TextPosition	     char_count = length;

#ifdef I18N
    /* if multibyte characters are used, the *character*
     * count will differ from the *byte* count
     */
char_count = _mbstrlen(buffer);
#endif

if (text-> editMode == OL_TEXT_READ)
   XBell(XtDisplay(w), 0);
else
   {
   call_data.ok = TRUE;
   call_data.current_cursor   = text-> cursorPosition;
   call_data.select_start     = text-> selectStart;
   call_data.select_end	      = text-> selectEnd;
   call_data.new_cursor	      =
   call_data.new_select_start =
   call_data.new_select_end   = text-> cursorPosition + char_count - 
				(text-> selectEnd - text-> selectStart);
   call_data.text	      = buffer;
   call_data.text_length      = length;
   
   XtCallCallbacks(w, XtNmodifyVerification, &call_data);

   if (call_data.ok == TRUE)
      {
      startloc = LocationOfPosition(textBuffer, text-> selectStart);
      endloc   = LocationOfPosition(textBuffer, text-> selectEnd);

      retval = EDIT_SUCCESS ==
      ReplaceBlockInTextBuffer
	 (textBuffer, &startloc, &endloc, buffer, UpdateDisplay, (XtPointer)w);
      }
   }

return (retval);
} /* end of OlTextEditInsert */
/*
 * OlTextEditPaste
 *
 * The \fIOlTextEditPaste\fR function is used to programmatically paste
 * the contents of the CLIPBOARD into the TextEdit widget \fIctx\fR.
 * The current (if any) selection is replaced by the contents of the
 * CLIPBOARD,
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget; 
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditCopySelection(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */
 
extern Boolean
OlTextEditPaste(ctx)
TextEditWidget ctx;
{
Boolean retval = TRUE;

if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
   retval = FALSE;
else
   _OlTEPaste(ctx, NULL, XA_CLIPBOARD(XtDisplay((Widget)ctx)), NULL);

return (retval);

} /* end of OlTextEditPaste */
/*
 * OlTextEditReadSubString
 *
 * The \fIOlTextEditReadSubString\fR function is used to retreive a copy 
 * of a substring from the TextBuffer associated with the TextEdit Widget
 * \fIctx\fR between positions \fIstart\fR through \fIend\fR inclusive.
 * The storage required for the copy is allocated by this routine; it 
 * is the responsibility of the caller to free this storage when appropriate.
 *
 * Return value:
 *
 * FALSE is returned is the widget supplied in not a TextEdit Widget; 
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditCopyBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditReadSubString(w, buffer, start, end)
Widget w;
char ** buffer;
TextPosition start;
TextPosition end;
{
TextEditWidget ctx = (TextEditWidget)w;
Boolean retval;

if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
   retval = FALSE;
else
   {
   TextBuffer * textBuffer = ctx-> textedit.textBuffer;
   TextLocation startloc;
   TextLocation endloc;

   if (start > end || buffer == NULL)
      retval = FALSE;
   else
      {
      startloc = LocationOfPosition(textBuffer, start);
      endloc   = LocationOfPosition(textBuffer, end);
      if (startloc.buffer == NULL || endloc.buffer == NULL)
	 retval = FALSE;
      else
	 {
		*buffer = GetTextBufferBlock(textBuffer, startloc, endloc);
	 retval = (*buffer != NULL);
	 }
      }
   }

return (retval);

} /* end of OlTextEditReadSubString */
/*
 * OlTextEditRedraw
 *
 * The \fIOlTextEditRedraw\fR function is used to force a complete refresh
 * of the TextEdit widget display.  This routine does nothing if the
 * TextEdit widget is not realized or if the update state is set to FALSE.
 *
 * Return value:
 *
 * FALSE is returned is the widget supplied in not a TextEdit Widget or
 * if the widget is not realized or if the update state is FALSE; 
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditUpdate(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditRedraw(w)
Widget w;
{
TextEditWidget ctx = (TextEditWidget)w;
Boolean retval;
XRectangle rect;

if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
   retval = FALSE;
else
   {
   if (XtIsRealized((Widget)ctx) && ctx-> textedit.updateState)
      {
      rect.x = 0;
      rect.y = 0;
      rect.width = ctx-> core.width;
      rect.height = ctx-> core.height;
      XClearArea(XtDisplay(ctx), XtWindow(ctx), 
		 rect.x, rect.y, rect.width, rect.height, FALSE);
      _DisplayText(ctx, &rect);
      retval = TRUE;
      }
   else
      retval = FALSE;
   }

return (retval);

} /* end of OlTextEditRedraw */
/*
 * OlTextEditSetCursorPosition
 *
 * The \fIOlTextEditSetCursorPosition\fR function is used to change the
 * current selection \fIstart\fR and \fIend\fR and \fIcursorPosition\fR.
 * The function does NOT check (for efficiency) the validity of 
 * the positions.  If invalid values are given results are unpredictable.
 * The function attempts to ensure that the cursorPosition is visible.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget; 
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditGetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditSetCursorPosition(w, start, end, cursorPosition)
Widget w;
TextPosition start;
TextPosition end;
TextPosition cursorPosition;
{
TextEditWidget ctx = (TextEditWidget)w;
Boolean retval;

if (!XtIsSubclass((Widget) ctx, textEditWidgetClass) 
|| (start != cursorPosition && end != cursorPosition)
||  start == -1 || end == -1 || cursorPosition == -1)
   retval = FALSE;
else
   {
   TextEditPart * text = &ctx-> textedit;

   _MoveSelection(ctx, cursorPosition, start, end, 7);

   retval = TRUE;
   }

return (retval);

} /* end of OlTextEditSetCursorPosition */
/*
 * OlTextEditTextBuffer
 *
 * The \fIOlTextEditTextBuffer\fR function is used to retrieve the 
 * TextBuffer pointer associated with the TextEdit widget \fIctx\fR.
 * This pointer can be used to access the facilities provided by
 * the Text Buffer Utilities module.
 *
 * See also:
 *
 * Text Buffer Utilities(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern TextBuffer *
OlTextEditTextBuffer(w)
Widget w;
{
TextEditWidget ctx = (TextEditWidget)w;

return (ctx-> textedit.textBuffer);

} /* end of OlTextEditTextBuffer */
/*
 * OlTextEditUpdate
 *
 * The \fIOlTextEditUpdate\fR function is used to set the \fIupdateState\fR
 * of a TextEdit Widget.  Setting the state to FALSE turns screen update
 * off; setting the state to TRUE turns screen updates on and refreshes
 * the display.
 * .P
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget; 
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditRedraw(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditUpdate OLARGLIST((w, update_state))
	OLARG( Widget,	w)
	OLGRA( Boolean, update_state)
{
TextEditWidget ctx = (TextEditWidget)w;
Boolean retval;

if (!XtIsSubclass(w, textEditWidgetClass))
   retval = FALSE;
else
   {
   ctx-> textedit.updateState = update_state;
   if (ctx-> textedit.updateState)
      OlTextEditRedraw(w);
   else
      _TurnTextCursorOff(ctx);
   retval = TRUE;
   }

return (retval);

} /* end of OlTextEditUpdate */
/*
 * OlTextEditResize
 *
 * The \fIOlTextEditResize\fR procedure is used to request a size change
 * of a TextEdit widget \fIctx\fR to display \fIrequest_linesVisible\fR and
 * \fIrequest_charsVisible\fR.	This routine calculates the appropriate
 * geometry, then requests a resize of its parent.  Note: the new size may
 * or may not be honored by the widget'''s parent, therefore the outcome is
 * non-deterministic.
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern void
OlTextEditResize(w, request_linesVisible, request_charsVisible)
Widget         w;
int	       request_linesVisible;
int	       request_charsVisible;
{
TextEditWidget ctx = (TextEditWidget)w;
TextEditPart * text = &ctx-> textedit;
Dimension      request_height;
Dimension      request_width;

request_height = text-> lineHeight * request_linesVisible + 
		 PAGE_T_GAP(ctx) + PAGE_B_GAP(ctx);
request_width  = ENSPACE(ctx) * request_charsVisible +
		 PAGE_L_GAP(ctx) + PAGE_R_GAP(ctx);
request_width  = VORDER(request_width, ctx-> core.width);
request_height = MAX(text-> lineHeight, (int)request_height);

if (text-> vsb != NULL) /* must be in a scrolled window */
   {
   ScrolledWindowWidget sw = (ScrolledWindowWidget)XtParent(XtParent(ctx));
   OlSWGeometries geometries;

   geometries = GetOlSWGeometries(sw);
   ResizeScrolledWindow(ctx, sw, &geometries, request_width, request_height, TRUE);
   }
else
   {
   if (XtMakeResizeRequest((Widget)ctx, request_width, request_height, 
			   &ctx-> core.width, &ctx-> core.height)
       == XtGeometryAlmost)
      XtMakeResizeRequest((Widget) ctx, ctx-> core.width, 
			  ctx-> core.height, NULL, NULL);
   }

} /* end of OlTextEditResize */
/*
 * IgnorePropertyNotify
 *
 */

static void
IgnorePropertyNotify OLARGLIST((w, event, params, num_params))
    OLARG(Widget, w)
    OLARG(XEvent *, event)
    OLARG(String *, params)
    OLGRA(Cardinal *, num_params)
{


/*
 *
 *	     this space intentionally left blank
 *
 */



} /* end of IgnorePropertyNotify */


/* 
 * Message
 *
 * The \fIMessage\fR procedure is called whenever a ClientMessage event occurs
 * within the TextEdit window.	The procedure is called indirectly by
 * OlAction().
 *
 */

static void
Message OLARGLIST((w, ve))
OLARG (Widget,	 w)
OLGRA (OlVirtualEvent, ve)
{
    
    Atom OL_PASTE_MSG = XInternAtom(XtDisplay(w), "OL_PASTE_MSG", False);
    
#ifdef I18N
    Atom message_type = ve-> xevent-> xclient.message_type;
    if (message_type == _OlGetFrontEndAtom()){
	TextEditWidget ctx = (TextEditWidget) w;
	char *	    buffer = XtMalloc(BUFSIZ+1);	/* +1 for NULL terminator */
	int	  len;
	KeySym	  keysym;
	OlImStatus  status;
	
	
	if (ctx->primitive.ic){ 
	    len = OlLookupImString((XKeyEvent *) ve-> xevent, 
				   ctx-> primitive.ic,
				   (char*)buffer, BUFSIZ, &keysym, &status);
	    if (status == XBufferOverflow){
		buffer = XtRealloc(buffer,len+1);
		len = OlLookupImString((XKeyEvent *) ve-> xevent, 
				       ctx-> primitive.ic,
				       (char*)buffer, len, &keysym, &status);
	    }
	    if (len > 0){
		buffer[len] = 0;
		OlTextEditInsert(w, buffer, len);
	    }
	}
	XtFree(buffer);		
    }
    
#endif
    if (message_type == OL_PASTE_MSG)
	ReceivePasteMessage(w, ve->xevent, NULL, 0);
}


/*
 *	SelectAll
 *
 */
static void
SelectAll OLARGLIST((w))
    OLGRA(Widget, w)
{
    TextEditWidget ctx = (TextEditWidget) w;
    TextPosition last;

    (void) OlTextEditGetLastPosition(w, &last);
    (void) OlTextEditSetCursorPosition(w, (TextPosition) 0, last, 
				       (TextPosition) 0);

} /* end of SelectAll */

/*
 *	DeselectAll
 *
 */
static void
DeselectAll OLARGLIST((w))
    OLGRA(Widget, w)
{
    TextEditWidget ctx = (TextEditWidget) w;
    TextPosition current, dummy;

    (void) OlTextEditGetCursorPosition(w, &dummy, &dummy, &current);
    (void) OlTextEditSetCursorPosition(w, current, current, current);

} /* end of DeselectAll */

#ifdef NOT_USED

/*
 * Paste Primary Text at the given position without affecting the current
 * selection.
 */

static void
PastePrimaryText OLARGLIST((ctx, buffer, length, dest))
    OLARG(TextEditWidget, ctx)
    OLARG(String,	  buffer)
    OLARG(int,		  length)
    OLGRA(TextPosition,	  dest)
{
    TextEditPart *	     text	= &ctx-> textedit;
    TextBuffer *	     textBuffer = text-> textBuffer;
    TextLocation	     startloc;
    TextLocation	     endloc;
    OlTextModifyCallData call_data;
    Boolean		     retval	= FALSE;
    TextPosition	     char_count = length;

#ifdef I18N
    /* if multibyte characters are used, the *character*
     * count will differ from the *byte* count
     */
    char_count = _mbstrlen(buffer);
#endif

   call_data.ok = TRUE;
   call_data.current_cursor   = text-> cursorPosition;
   call_data.select_start     = text-> selectStart;
   call_data.select_end       = text-> selectEnd;
   call_data.new_cursor       =
   call_data.new_select_start =
   call_data.new_select_end   = text-> cursorPosition + char_count - 
                                (text-> selectEnd - text-> selectStart);
   call_data.text             = buffer;
   call_data.text_length      = length;
   
   XtCallCallbacks((Widget)ctx, XtNmodifyVerification, &call_data);

   if (call_data.ok == TRUE)
      {
      startloc = LocationOfPosition(textBuffer, (TextPosition)dest);
      endloc   = LocationOfPosition(textBuffer, (TextPosition)dest);

      retval = EDIT_SUCCESS ==
      ReplaceBlockInTextBuffer
         (textBuffer, &startloc, &endloc, buffer, UpdateDisplay, (XtPointer)ctx);
      }
} /* end of PastePrimaryText */
#endif

