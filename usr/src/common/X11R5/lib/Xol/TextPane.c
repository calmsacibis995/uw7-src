/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#include <Xol/OlMinStr.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)oldtext:TextPane.c	1.43.1.46"
#endif

/*
 *************************************************************************
 *
 * Description:  This file contains the TextPane widget code.  The
 *	TextPane widget allows editing/display of string and disk
 *	sources.
 *
 *******************************file*header*******************************
 */

/*************************************<+>*************************************
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1987, 1988 by Digital Equipment Corporation, Maynard,
 **             Massachusetts, and the Massachusetts Institute of Technology,
 **             Cambridge, Massachusetts
 **   
 *****************************************************************************
 *************************************<+>*************************************/

						/* #includes go here	*/
#include <stdio.h>
#include <fcntl.h>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/TextPaneP.h>
#include <Xol/TextP.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static void		AlterSelection();
static void		BuildLineTable() ;
static void		CalculateSize() ;
static void		CheckResizeOrOverflow() ;
static void		ClearText() ;     
static void		ClearWindow() ;
static Boolean		ConvertSelection ();
static void		DeleteOrKill();
static void		DisplayAllText() ;
static void		DisplayText() ;
static void		DoSelection();
static void		EndAction();
static void		ExtendSelection();
static void		FlushUpdate() ;
static void		ForceBuildLineTable() ;
static void		InsertCursor() ;
static Bool		IsGraphicsExpose();
static int		LineAndXYForPosition() ;
static int		LineForPosition() ;
static void		LosePrimary() ;
static void		LoseSelection() ;
static void		NewSelection();
static void		PasteTextOnClipBoard();
static OlTextPosition	PositionForXY() ;
static int		ReplaceText() ;
static void		StartAction() ;
static void		StuffFromBuffer() ;
static void		TakeFromClipboard() ;
static OlTextLineRange	UpdateLineTable() ;
static int		_OlSetCursorPos();
static void		_XtTextExecuteUpdate() ;
static void		_XtTextNeedsUpdating() ;
static void		_XtTextPrepareToUpdate() ;
static void		_OlTextScroll() ;
void			_OlTextScrollAbsolute() ;
static void		_XtTextSetNewSelection();
static void		_XtTextShowPosition() ;

					/* class procedures		*/

static void ClassInitialize();
static void ClassPartInitialize();
static void GetValuesHook();
static void HighlightHandler OL_ARGS((Widget, OlDefine));
static void Initialize();
static void InitializeHook();
static void OlTextClearBuffer();
static unsigned char * OlTextCopyBuffer();
static unsigned char * OlTextCopySelection();
static OlTextPosition OlTextGetInsertPos();
static OlTextPosition OlTextGetLastPos();
void OlTextGetSelectionPos();
static void OlTextInsert();
static int OlTextReadSubString();
static void OlTextRedraw();
static int OlTextReplace();
static void OlTextSetInsertPos();
void OlTextSetSelection();
static void OlTextSetSource();
void OlTextUnsetSelection();
static void OlTextUpdate();
static void ProcessExposeRegion();
static void Realize();
static Widget RegisterFocus();
static void Resize();
static Boolean SetValues();
static Boolean SetValuesHook();
static void TextDestroy();
static unsigned char * _OlTextCopySelection();
static void _OlTextRedraw();
static int _OlTextReplace();
static void _OlTextSetSelection();
static int _OlTextSubString();
static void _OlTextUnsetSelection();

					/* action procedures		*/

static void CopySelection();
static void DeleteBackwardChar();
static void DeleteBackwardWord();
static void DeleteCurrentSelection();
static void DeleteForwardChar();
static void DeleteForwardWord();
static void Execute();
static void ExtendAdjust();
static void ExtendEnd();
static void ExtendStart();
static void InsertChar OL_ARGS((Widget, OlVirtualEvent));
static int InsertNewLine();
static void InsertNewLineAndBackup();
static void InsertNewLineAndIndent();
static void KillBackwardWord();
static void KillCurrentSelection();
static void KillForwardWord();
static void DeleteToBeginningOfLine();
static void DeleteToEndOfLine();
static void KillToEndOfParagraph();
static void MoveBackwardChar();
static void MoveBackwardParagraph();
static void MoveBackwardWord();
static void MoveBeginningOfFile();
static void MoveEndOfFile();
static void MoveForwardChar();
static void MoveForwardParagraph();
static void MoveForwardWord();
static void MoveNextLine();
void MoveNextPage();
static int  MovePosition();
static void MovePreviousLine();
void MovePreviousPage();
static void MoveToLineEnd();
static void MoveToLineStart();
static void RedrawDisplay();
static void ScrollOneLineDown();
static void ScrollOneLineUp();
static void SelectWord();
static void SelectAll();
static void SelectStart();
static void SelectAdjust();
static void SelectEnd();
static void Stuff();
static void TextFocusOut();
static void UnKill();
static Boolean VerifyLeave();

					/* public procedures		*/
void _OlScrollText();

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define HAS_FOCUS(w)	(((TextPaneWidget)(w))->primitive.has_focus == TRUE)

static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));

static OlEventHandlerRec
event_procs[] = {
	{ KeyPress,	InsertChar	},
	{ FocusOut,	TextFocusOut	}
};

#define BufMax 1000
#define abs(x)	(((x) < 0) ? (-(x)) : (x))
#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))
#define GETLASTPOS(ctx)  ((*(ctx->text.source->getLastPos)) (ctx->text.source))

#define BUTTONMASK 0x143d

#define zeroPosition ((OlTextPosition) 0)

#define STRBUFSIZE 100
static Boolean false = (Boolean) FALSE;
static Boolean true = (Boolean) TRUE;

static XComposeStatus compose_status = {
	NULL, 0};

Boolean GotNotification = FALSE;

#define BYTE_OFFSET	XtOffsetOf(TextPaneRec, text.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
	XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_TEXTPANE_BG, NULL },
{ { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel), 0, XtRString,
	 XtDefaultForeground }, BYTE_OFFSET, OL_B_TEXTPANE_FONTCOLOR, NULL },
};
#undef BYTE_OFFSET

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* Translations */

/* Note: the initialization string below is too long for some C compilers;
 *	it must be converted using the FIXTEXT tool into a list of 
 *	comma-separated single characters for initialization
 */

char defaultTextPaneTranslations1[] =  
       "! <charFwdKey>:	forward-character()\n\
	! <charBakKey>:	backward-character()\n\
	! <rowDownKey>:	next-line()\n\
	! <rowUpKey>:	previous-line()\n\
	! <wordFwdKey>:	forward-word()\n\
	! <wordBakKey>:	backward-word()\n\
	! <lineStartKey>:	beginning-of-line()\n\
	! <lineEndKey>:	end-of-line()\n\
	  <docStartKey>:	beginning-of-file()\n\
	  <docEndKey>:	end-of-file()\n\
	! <delCharFwdKey>:	delete-next-character()\n\
	! <delCharBakKey>:	delete-previous-character()\n\
	! <delWordFwdKey>:	delete-next-word()\n\
	! <delWordBakKey>:	delete-previous-word()\n\
	! <delLineFwdKey>:	delete-to-end-of-line()\n\
	! <delLineBakKey>:	delete-to-beginning-of-line()\n\
";

char defaultTextPaneTranslations2[] =  "\
	! <Key>Return:	newline()\n\
	! Ctrl<Key>L:	redraw-display()\n\
	  <FocusIn>:	OlAction()	\n\
	  <FocusOut>:	OlAction()	\n\
	! <pasteKey>:	stuff()\n\
	! <selectBtnDown>		:	select-start()\n\
	! selectBtn <selectBtnMotion>	:	extend-adjust()\n\
	! adjustBtn <adjustBtnMotion>	:	extend-adjust()\n\
	! adjustBtn <adjustBtnUp>	:	extend-end()\n\
	! <Key>Execute:	execute()\n\
	<Key>:	OlAction()";

XtActionsRec textPaneActionsTable [] = {
	{"forward-character", 	MoveForwardChar},
	{"backward-character", 	MoveBackwardChar},
	{"forward-word", 		MoveForwardWord},
	{"backward-word", 		MoveBackwardWord},
	{"forward-paragraph", 	MoveForwardParagraph},
	{"backward-paragraph", 	MoveBackwardParagraph},
	{"beginning-of-line", 	MoveToLineStart},
	{"end-of-line", 		MoveToLineEnd},
	{"next-line", 		MoveNextLine},
	{"previous-line", 		MovePreviousLine},
	{"next-page", 		MoveNextPage},
	{"previous-page", 		MovePreviousPage},
	{"beginning-of-file", 	MoveBeginningOfFile},
	{"end-of-file", 		MoveEndOfFile},
	{"scroll-one-line-up", 	ScrollOneLineUp},
	{"scroll-one-line-down", 	ScrollOneLineDown},
	{"delete-next-character", 	DeleteForwardChar},
	{"delete-previous-character", DeleteBackwardChar},
	{"delete-next-word", 		DeleteForwardWord},
	{"delete-previous-word", 	DeleteBackwardWord},
	{"delete-to-end-of-line", 	DeleteToEndOfLine},
	{"delete-to-beginning-of-line",	DeleteToBeginningOfLine},
	{"delete-selection", 		DeleteCurrentSelection},
	{"kill-word", 		KillForwardWord},
	{"backward-kill-word", 	KillBackwardWord},
	{"kill-to-end-of-paragraph", 	KillToEndOfParagraph},
	{"unkill", 			UnKill},
	{"stuff", 			Stuff},
	{"newline-and-indent", 	InsertNewLineAndIndent},
	{"newline-and-backup", 	InsertNewLineAndBackup},
	{"newline", 			(XtActionProc)InsertNewLine},
	{"select-word", 		SelectWord},
	{"select-all", 		SelectAll},
	{"select-start", 		SelectStart},
	{"select-adjust", 		SelectAdjust},
	{"select-end", 		SelectEnd},
	{"extend-start", 		ExtendStart},
	{"extend-adjust", 		ExtendAdjust},
	{"extend-end", 		ExtendEnd},
	{"redraw-display", 		RedrawDisplay},
	{"execute",                   Execute},
	{NULL,NULL}
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] = {
					/* core resources */
  { XtNheight, XtCHeight, XtRDimension, sizeof(Dimension),
    XtOffsetOf(TextPaneRec, core.height), XtRImmediate, (XtPointer)200
  },
  { XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension),
    XtOffsetOf(TextPaneRec, core.width), XtRImmediate, (XtPointer)200
  },
					/* text pane resources */

#define OFFSET(field) XtOffsetOf(TextPaneRec, text.field)

  { XtNcursorPosition, XtCTextPosition, XtRLong, sizeof(OlTextPosition), 
    OFFSET(insertPos), XtRImmediate, 0
  },
  { XtNdisplayPosition, XtCTextPosition, XtRLong, sizeof(OlTextPosition), 
    OFFSET(lt.top), XtRImmediate, 0
  },
  { XtNexecute, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(execute), XtRCallback, NULL
  },
  { XtNfile, XtCString, XtRString, sizeof(XtPointer),
    OFFSET(file), XtRString, NULL
  },
  { XtNgrow, XtCGrow, XtROlDefine, sizeof(OlDefine),
    OFFSET(grow_mode), XtRImmediate, (XtPointer)OL_GROW_OFF
  },
  { XtNleftMargin, XtCMargin, XtRDimension, sizeof(Dimension), 
    OFFSET(leftmargin), XtRImmediate, (XtPointer)2
  },
  { XtNrightMargin, XtCMargin, XtRDimension, sizeof(Dimension), 
    OFFSET(rightmargin), XtRImmediate, (XtPointer)2
  },
  { XtNtopMargin, XtCMargin, XtRDimension, sizeof(Dimension), 
    OFFSET(topmargin), XtRImmediate, (XtPointer)2
  },
  { XtNbottomMargin, XtCMargin, XtRDimension, sizeof(Dimension), 
    OFFSET(bottommargin), XtRImmediate, (XtPointer)2
  },
  { XtNsourceType, XtCSourceType, XtROlDefine, sizeof(OlDefine),
    OFFSET(srctype), XtRImmediate, (XtPointer)OL_STRING_SOURCE
  },
  { XtNtextSource, XtCTextSource, XtRPointer, sizeof(XtPointer), 
    OFFSET(source), XtRPointer, NULL
  },
  { XtNselection, XtCSelection, XtRPointer, sizeof(XtPointer),
    OFFSET(s), XtRPointer, NULL
  },
  { XtNmark, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(mark), XtRCallback, NULL
  },
  { XtNmotionVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(motion_verification), XtRCallback, NULL
  },
  { XtNmodifyVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(modify_verification), XtRCallback, NULL
  },
  { XtNleaveVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
    OFFSET(leave_verification), XtRCallback, NULL
  },
  { XtNrecomputeSize, XtCBoolean, XtRBoolean, sizeof(Boolean),
    OFFSET(recompute_size), XtRImmediate, (XtPointer)True
  },
  { XtNscroll, XtCScroll, XtROlDefine, sizeof(OlDefine),
    OFFSET(scroll_mode), XtRImmediate, (XtPointer)OL_AUTO_SCROLL_OFF
  },
  { XtNstring, XtCString, XtRString, sizeof(XtPointer),
    OFFSET(string), XtRString, NULL
  },
  { XtNtextClearBuffer, XtCTextClearBuffer, XtRPointer, sizeof(XtPointer),
    OFFSET(text_clear_buffer), XtRPointer, NULL
  },
  { XtNtextCopyBuffer, XtCTextCopyBuffer, XtRPointer, sizeof(XtPointer),
    OFFSET(text_copy_buffer), XtRPointer, NULL
  },
  { XtNtextGetInsertPoint, XtCTextGetInsertPoint, XtRPointer, sizeof(XtPointer),
    OFFSET(text_get_insert_point), XtRPointer, NULL
  },
  { XtNtextGetLastPos, XtCTextGetLastPos, XtRPointer, sizeof(XtPointer),
    OFFSET(text_get_last_pos), XtRPointer, NULL
  },
  { XtNtextInsert, XtCTextInsert, XtRPointer, sizeof(XtPointer),
    OFFSET(text_insert), XtRPointer, NULL
  },
  { XtNtextReadSubStr, XtCTextReadSubStr, XtRPointer, sizeof(XtPointer),
    OFFSET(text_read_sub_str), XtRPointer, NULL
  },
  { XtNtextRedraw, XtCTextRedraw, XtRPointer, sizeof(XtPointer),
    OFFSET(text_redraw), XtRPointer, NULL
  },
  { XtNtextReplace, XtCTextReplace, XtRPointer, sizeof(XtPointer),
    OFFSET(text_replace), XtRPointer, NULL
  },
  { XtNtextSetInsertPoint, XtCTextSetInsertPoint, XtRPointer, sizeof(XtPointer),
    OFFSET(text_set_insert_point), XtRPointer, NULL
  },
  { XtNtextSetSource, XtCTextSetSource, XtRPointer, sizeof(XtPointer),
    OFFSET(text_set_source), XtRPointer, NULL
  },
  { XtNtextUpdate, XtCTextUpdate, XtRPointer, sizeof(XtPointer),
    OFFSET(text_update), XtRPointer, NULL
  },
/*
  { XtNtransparent, XtCBoolean, XtRBoolean, sizeof(Boolean),
    OFFSET(transparent), XtRImmediate, (XtPointer)False
  },
*/
  { XtNverticalSBWidget, XtCVerticalSBWidget, XtRWidget, sizeof(XtPointer),
    OFFSET(verticalSB), XtRWidget, NULL
  },
  { XtNwrap, XtCWrap, XtRBoolean, sizeof(Boolean),
    OFFSET(wrap_mode), XtRImmediate, (XtPointer)True
  },
  { XtNwrapForm, XtCWrapForm, XtROlDefine, sizeof(OlDefine),
    OFFSET(wrap_form), XtRImmediate, (XtPointer)OL_SOURCE_FORM
  },
  { XtNwrapBreak, XtCWrapBreak, XtROlDefine, sizeof(OlDefine),
    OFFSET(wrap_break), XtRImmediate, (XtPointer)OL_WRAP_WHITE_SPACE
  },
  { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
    XtOffsetOf(TextPaneRec, core.background_pixel), XtRString,
    (XtPointer)XtDefaultBackground
  },
  { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel),
    XtOffsetOf(TextPaneRec, primitive.font_color), XtRString,
    (XtPointer)XtDefaultForeground
  },
 };

#undef OFFSET

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

TextPaneClassRec textPaneClassRec = {
	{
	/* core fields */
	/* superclass       */      (WidgetClass) &primitiveClassRec,
	/* class_name       */      "TextPane",
	/* widget_size      */      sizeof(TextPaneRec),
	/* class_initialize */      ClassInitialize,
	/* class_part_init  */      ClassPartInitialize,
	/* class_inited     */      FALSE,
	/* initialize       */      Initialize,
	/* initialize_hook  */      InitializeHook,
	/* realize          */      Realize,
	/* actions          */      textPaneActionsTable,
	/* num_actions      */      XtNumber(textPaneActionsTable),
	/* resources        */      resources,
	/* num_ resource    */      XtNumber(resources),
	/* xrm_class        */      NULLQUARK,
	/* compress_motion  */      TRUE,
	/* compress_exposure*/      TRUE,
	/* compress_enterleave*/    TRUE,
	/* visible_interest */      FALSE,
	/* destroy          */      TextDestroy,
	/* resize           */      Resize,
	/* expose           */      ProcessExposeRegion,
	/* set_values       */      SetValues,
	/* set_values_hook  */      SetValuesHook,
	/* set_values_almost*/      XtInheritSetValuesAlmost,
	/* get_values_hook  */      GetValuesHook,
	/* accept_focus     */      XtInheritAcceptFocus,
	/* version          */      XtVersion,
	/* callback_private */      NULL,
	/* tm_table         */      NULL
	},
        {
	/* primitive class */
	/* focus_on_select  */	    True,
        /* highlight_handler*/      HighlightHandler,
        /* traversal_handler*/      NULL,
        /* register_focus   */      RegisterFocus,
	/* activate         */      ActivateWidget,
        /* event_procs      */      event_procs,
        /* num_event_procs  */      XtNumber(event_procs),
        /* version          */      OlVersion,
        /* extension        */      NULL,
	/* dyn_data	    */	    { dyn_res, XtNumber(dyn_res) },
	/* transparent_proc */	    NULL,
        },
	{
	/* TextPane fields */
	/* copy_substring    */     _OlTextCopySubString,
	/* copy_selection    */     _OlTextCopySelection,
	/* unset_selection   */     _OlTextUnsetSelection,
	/* set_selection     */     _OlTextSetSelection,
	/* replace_text      */     _OlTextReplace,
	/* redraw_text       */     _OlTextRedraw,
	}
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass textPaneWidgetClass = (WidgetClass)&textPaneClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */


/*
 *************************************************************************
 * 
 *  AlterSelection - This routine is used to perform various selection
 *	functions. The goal is to be able to specify all the more popular
 *	forms of draw-through and multi-click selection user interfaces
 *	from the outside.
 * 
 ****************************procedure*header*****************************
 */
static void
AlterSelection (ctx, mode, action)
	TextPaneWidget     ctx;
	OlSelectionMode      mode;   /* {OlsmTextSelect, OlsmTextExtend}  */
	OlSelectionAction action;/*{OlactionStart,OlactionAdjust,OlactionEnd} */
{
	OlTextPosition position;
	unsigned char   *ptr;

	position = PositionForXY (ctx, (int) ctx->text.ev_x, (int) ctx->text.ev_y);
	if (!XtOwnSelection(	(Widget)ctx,
				XA_PRIMARY,
				ctx->text.time,
				ConvertSelection,
				LosePrimary,
				NULL)) {

			OlVaDisplayWarningMsg(	(Display *)NULL,
						OleNfileTextPane,
						OleTmsg1,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg1);
	}

	if (action == OlactionStart) {
		switch (mode) {
		case OlsmTextSelect:
			DoSelection (ctx, position, ctx->text.time, FALSE);
			break;
		case OlsmTextExtend:
			ExtendSelection (ctx, position, FALSE);
			break;
		}
	}
	else {
		switch (mode) {
		case OlsmTextSelect:
			DoSelection (ctx, position, ctx->text.time, TRUE);
			break;
		case OlsmTextExtend:
			ExtendSelection (ctx, position, TRUE);
			break;
		}
	}
}	/*  AlterSelection  */


/*
 *************************************************************************
 * 
 *  BuildLineTable - This routine builds a line table. It does this by
 *	starting at the specified position and measuring text to
 *	determine the staring positio of each line to be displayed.
 *	It also determines and saves in the linetable all the required
 *	metrics for displaying a given line (e.g. x offset, y offset,
 *	line length, etc.).
 * 
 ****************************procedure*header*****************************
 */
static void
BuildLineTable(self, position)
	TextPaneWidget self;
	OlTextPosition position;
{
	OlTextPosition line, lines, lastLine ;
	Boolean     rebuild;
	int		i;
	OlLineTableEntryPtr lp ;

	rebuild = (Boolean) (position != self->text.lt.top);
/* SIZE_FIX
	if (self->core.height == 0)
		lines = self->text.source->number_of_lines;
	else
*/
		lines= applyDisplay(maxLines)(self) ;

	/****************
 *
 *  RBM 
 *
 *  Don't allow a 0-line widget - let clipping occur
 *
 *  NOTE: THE MAXINT CLIPPING IS AN UGLY HACK THAT NEEDS TO BE FIXED 
 *        WITH A CAST!!
 ****************/
	if ((lines < 1) || (lines > 32767)) lines = 1;

	if (self->text.lt.info != NULL && lines != self->text.lt.lines) {
		XtFree((char *) self->text.lt.info);
		self->text.lt.info = NULL;
	}
	if (self->text.lt.info == NULL)
	{	
		self->text.lt.info = (OlLineTableEntry *)
		    XtCalloc(lines + 1, (unsigned)sizeof(OlLineTableEntry));
		self->text.lt.lines = lines ;
		for (line = 0, lp = &(self->text.lt.info[0]) ;
		    line < lines; line++, lp++)
		{ 
			lp->position = lp->drawPos = 0 ;
			lp->x = lp->y = 0 ;
		}
		rebuild = TRUE;
	}

	self->text.lt.top = position ;
	if (rebuild) UpdateLineTable ( self
	    , position
	    , 0
	    , self->core.width
	    - self->text.leftmargin
	    - self->text.rightmargin
	    , 0
	    , FALSE
	    ) ;
}	/*  BuildLineTable  */


/*
 *************************************************************************
 * 
 *  CalculateSize - This routine calculates the size of the text window.
 * 
 ****************************procedure*header*****************************
 */
static void
CalculateSize(tew)
TextPaneWidget tew;
{
	int new_width;
	int new_height;

	/*
	 *  Calculate the width of the text window.
	 */
	if (tew->core.width == 0  || tew->text.recompute_size)
		{}

	/*
	 *  Calculate the height of the text window.
	 */
	if (tew->core.height == 0  || tew->text.recompute_size)
		{}

}	/*  CalculateSize  */


/*
 *************************************************************************
 * 
 *  CheckResizeOrOverflow - This routine checks to see if the window
 *	should be resized (grown or shrunk) or scrolled then text to
 *	be painted overflows to the right or the bottom of the window.
 *	It is used by the keyboard input routine.
 * 
 ****************************procedure*header*****************************
 */
static void
CheckResizeOrOverflow(ctx)
	TextPaneWidget ctx;
{
	OlTextPosition posToCheck;
	OlTextLineRange i, line, nLines;
	Dimension width;
	XtWidgetGeometry rbox;
	XtGeometryResult reply;
	OlLineTableEntryPtr lp;
	OlLineTablePtr      lt;
	TextPanePart   *tp;

	tp = &(ctx->text);
	lt = &(tp->lt);
	nLines = lt->lines;

	if (ctx->core.width == 0 || tp->grow_state & OL_GROW_HORIZONTAL) { 
		UpdateLineTable ( ctx,
		    lt->top,
		    0,
		    INFINITE_WIDTH,
		    0,
		    FALSE);
		width = 0;
		for (i=0, lp = &(lt->info[0]) ; i < nLines ; i++, lp++) { 
			if ((Position)width < lp->endX) width = lp->endX;
		};
		width += ctx->text.rightmargin;
		if (width > ctx->core.width) { 
			rbox.request_mode = CWWidth;
			rbox.width = width;
			reply = XtMakeGeometryRequest((Widget)ctx, &rbox, &rbox);
			if (reply == XtGeometryAlmost)
				reply = XtMakeGeometryRequest((Widget)ctx, &rbox, NULL);
			/*
			 * NOTE: following test is expected to fall-through
			 * from the previous conditional.  It should not be
			 * an else if.
			 */
			if (reply == XtGeometryYes)
				ctx->core.width = rbox.width;
			else
			/* if request not satisfied, disallow future attempts */
			{ 
				tp->grow_state &= ~OL_GROW_HORIZONTAL ;
				UpdateLineTable ( ctx
				    , lt->top
				    , 0
				    , ctx->core.width - ctx->text.leftmargin
				    - ctx->text.rightmargin
				    , 0
				    , FALSE
				    ) ;
			}
		}
	};

	if (ctx->core.height == 0 || (tp->grow_state & OL_GROW_VERTICAL)
	    && ( ! (lt->info[nLines].fit & tfEndText)
	    || (lt->info[nLines].drawPos > lt->info[nLines].position))) { 
		rbox.request_mode = CWHeight;
		rbox.height = (*(ctx->text.sink->maxHeight))
	    		(ctx, nLines + 1) +
			tp->topmargin + tp->bottommargin;
		reply = XtMakeGeometryRequest((Widget)ctx, &rbox, &rbox);
		if (reply == XtGeometryAlmost)
			reply = XtMakeGeometryRequest((Widget)ctx, &rbox, NULL);
		if (reply == XtGeometryYes)
			ctx->core.height = rbox.height;
		else
			/* if request not satisfied, disallow future attempts */
			tp->grow_state &= ~OL_GROW_VERTICAL;
	};
}	/*  CheckResizeOrOverflow  */


/*
 *************************************************************************
 * 
 * ClearText - Clear the portion of the window that the text in drawn
 *	in, or that is don't clear the margins.
 * 
 ****************************procedure*header*****************************
 */
static void
ClearText(ctx)
	TextPaneWidget ctx;
{
	register TextPanePart *text = (TextPanePart *) &(ctx->text);

#if 0
	(*(text->sink->clearToBackground))(ctx,
		text->leftmargin,
		text->topmargin,
		ctx->core.width - (text->rightmargin + text->leftmargin),
		ctx->core.height - (text->bottommargin + text->topmargin));
#endif
	(*(text->sink->clearToBackground))(ctx, (Position)0, (Position)0,
		ctx->core.width, ctx->core.height - (text->bottommargin));

}	/*  ClearText  */


/*
 *************************************************************************
 * 
 *  ClearWindow - Clear the window to background color.
 * 
 ****************************procedure*header*****************************
 */
static void
ClearWindow (ctx)
	TextPaneWidget ctx;
{
	(*(ctx->text.sink->clearToBackground))(ctx,
		0,
		0,
		ctx->core.width,
		ctx->core.height);

}	/*  ClearWindow  */


static Boolean	
ConvertSelection(w, selection, target, type_return, value_return, length_return, format_return)
	Widget	w;
	Atom	*selection, *target, *type_return;
	XtPointer	*value_return;
	unsigned long	*length_return;
	int	*format_return;
{
	TextPaneWidget	ctx = (TextPaneWidget)w;
	int	i;
	char	*str, *buffer;
	int 	*intbuffer;
	Atom	stuff;
	Display *	dpy = XtDisplay((Widget)ctx);

	if (*selection != XA_PRIMARY && *selection != XA_CLIPBOARD(dpy)) {
		return (False);
	}

	if (*target == (stuff = XA_OL_COPY(dpy))) {
		CopySelection(ctx, NULL);
		*value_return = NULL;
		*length_return = NULL;
		*format_return = NULL;
		*type_return = stuff;

		return (False);
	}

	else if (*target == (stuff = XA_OL_CUT(dpy))) {
		KillCurrentSelection(ctx, NULL);
		*value_return = NULL;
		*length_return = NULL;
		*format_return = NULL;
		*type_return = stuff;

		return (False);
	}

	else if (*target == XA_STRING) {

		if (*selection == XA_CLIPBOARD(dpy)) {
			str = ctx->text.clip_contents;
			i = strlen(str);

			buffer = XtMalloc(1 + i);
			(void) strncpy (buffer, str, i);
			buffer[i] = '\0';
		} 
		else if (*selection == XA_PRIMARY) {
			buffer = (char *) _OlTextCopySubString
				(ctx, ctx->text.s.left, ctx->text.s.right);
			i = strlen(buffer);
		}
		else {
			return (False);
		}

		*value_return = buffer;
		*length_return = i;		/* DON'T include null byte */
		*format_return = 8;
		*type_return = XA_STRING;

		return (True);
	}
/*
**  Code to cooperate with Sun clients
*/
	else if (*target == XInternAtom (dpy, "LENGTH", False)) {
		intbuffer = (int *) XtMalloc(sizeof(int));
		*intbuffer = (int) (strlen (ctx->text.clip_contents));
		*value_return = (XtPointer)intbuffer;
		*length_return = 1;
		*format_return = sizeof(int) * 8;
		*type_return = (Atom) *target;
		return (True);
	}

	return (False);
}

/*
 *************************************************************************
 * 
 *  DeleteOrKill - 
 * 
 ****************************procedure*header*****************************
 */
static void
DeleteOrKill(ctx, from, to, kill)
	TextPaneWidget	   ctx;
	OlTextPosition from, to;
	Boolean	   kill;
{
	OlTextBlock text;
	unsigned char *ptr;
	int result;

	if (kill && from < to)
		ptr = _OlTextCopySubString(ctx, from, to);

	text.length = 0;

	if (result = ReplaceText(ctx, from, to, &text, TRUE)) {
		if (result != OleditReject)
			(void) _OlBeepDisplay((Widget)ctx, 1);
		if (kill && from < to)
			XtFree((char *)ptr);
		return;
	}

	if (kill && from < to) {
#if 0
      XStoreBuffer(XtDisplay(ctx), ptr, _OlStrlen(ptr), 1);
#endif
		PasteTextOnClipBoard(ctx, ptr);
	}
	_OlSetCursorPos(ctx, from);
	ctx->text.showposition = TRUE;

	from = ctx->text.insertPos;
	_XtTextSetNewSelection(ctx, from, from);
}	/*  DeleteOrKill  */


/*
 *************************************************************************
 * 
 *  DisplayAllText - Internal redisplay entire window.
 * 
 ****************************procedure*header*****************************
 */
static void
DisplayAllText(w)
	Widget w;
{
	TextPaneWidget ctx = (TextPaneWidget) w;

	ClearText(ctx);
	_XtTextNeedsUpdating(ctx, zeroPosition, GETLASTPOS(ctx));

}	/*  DisplayAllText  */


/*
 *************************************************************************
 * 
 *  DisplayText - This routine will display text between two arbitrary
 *	source positions. In the event that this span contains
 *	highlighted text for the selection, only that portion will be
 *	displayed highlighted.  It is illegal to call this routine
 *	unless there is a valid line table.
 * 
 ****************************procedure*header*****************************
 */
static void
DisplayText(ctx, pos1, pos2)
	TextPaneWidget ctx;
	OlTextPosition pos1, pos2;
{
	Position x, y, xlimit, tempx;
	Dimension height;
	int line, i, visible;
	OlTextPosition startPos, endPos, lastpos = GETLASTPOS(ctx);

	if (pos1 < ctx->text.lt.top)
		pos1 = ctx->text.lt.top;
	if (pos2 > lastpos)
		pos2 = lastpos;
	if (pos1 >= pos2)
		return;

	visible = LineAndXYForPosition(ctx, pos1, &line, &x, &y);
	if (!visible)
		return;

	startPos = pos1;
	xlimit = ctx->core.width - ctx->text.rightmargin + 1 ;
	height = ctx->text.lt.info[1].y - ctx->text.lt.info[0].y;
	for (i = line; i < ctx->text.lt.lines; i++) {
		endPos = ctx->text.lt.info[i].drawPos + 1;
		if (endPos > pos2)
			endPos = pos2;
		if (endPos >= startPos) {
/*  We know this should not be necessary!!!!
		if (x == ctx->text.leftmargin)
			(*(ctx->text.sink->clearToBackground))(ctx,
				0,
				y,
				ctx->text.leftmargin,
				height);
*/
			if (startPos >= ctx->text.s.right ||
				endPos <= ctx->text.s.left) {
				(*(ctx->text.sink->display))(ctx, x, y,
				    startPos, endPos, FALSE);
			} else if (startPos >= ctx->text.s.left
			    && endPos <= ctx->text.s.right) { 
				(*(ctx->text.sink->display))(ctx, x, y,
				    startPos, endPos, TRUE);
			} else {
				DisplayText(ctx, startPos, ctx->text.s.left);
				DisplayText(ctx, max(startPos, ctx->text.s.left), 
				    min(endPos, ctx->text.s.right));
				DisplayText(ctx, ctx->text.s.right, endPos);
			}
		}
		startPos = ctx->text.lt.info[i + 1].position;
		height = ctx->text.lt.info[i + 1].y - ctx->text.lt.info[i].y;
		tempx = ctx->text.lt.info[i].endX;
		(*(ctx->text.sink->clearToBackground))(ctx,
		    tempx, y, xlimit - tempx, height);
		x = ctx->text.leftmargin;
		y = ctx->text.lt.info[i + 1].y;
		if ((endPos == pos2) && (endPos != lastpos))
			break;
	}
}	/*  DisplayText  */

void
_OlDisplayMark(ctx, pos, mark)
	TextPaneWidget		ctx;
	OlTextPosition		pos;
	char *			mark;
{
	/*
	 *	check visibility of pos
	 */
	if (mark && IsPositionVisible(ctx, pos)) {
		ctx->text.insert_state = OlisOff;
		InsertCursor(ctx);
		_AsciiDisplayMark(
			ctx,
			ctx->text.lt.info[LineForPosition(ctx, pos)].y,
			mark
		);
		ctx->text.insert_state = OlisOn;
		InsertCursor(ctx); 
	}
}

char *
_OlMark(ctx, pos)
	TextPaneWidget		ctx;
	OlTextPosition		pos;
{
	OlTextMark		mark;

	mark.p = NULL;
	mark.pos = pos;

	if (XtClass(XtParent(XtParent(ctx))) == textWidgetClass) {
		XtCallCallbacks(XtParent(XtParent((Widget)ctx)),
					XtNmark, &mark);
	}
	return mark.p;
}

/*
 *************************************************************************
 * 
 *  DoSelection - This routine implements multi-click selection in a
 *	hardwired manner.  It supports multi-click entity cycling
 *	(char, word, line, file) and mouse motion adjustment of the
 *	selected entitie (i.e. select a word then, with button still
 *	down, adjust wich word you really meant by moving the mouse).
 *	[NOTE: This routine is to be replaced by a set of procedures that
 *	will allows clients to implements a wide class of draw through and
 *	multi-click selection user interfaces.]
 * 
 ****************************procedure*header*****************************
 */
static void
DoSelection (ctx, position, time, motion)
	TextPaneWidget ctx;
	OlTextPosition position;
	Time time;
	Boolean motion;
{
	int     delta;
	int     line;
	OlTextPosition newLeft, newRight;
	OlSelectType newType;
	OlSelectType *sarray;

	delta = (time < ctx->text.lasttime) ?
	    ctx->text.lasttime - time : time - ctx->text.lasttime;
	if (motion)
		newType = ctx->text.s.type;
	else {/* multi-click event */
		if ((delta < 500) && ((position >= ctx->text.s.left)
		    && (position <= ctx->text.s.right))) {
			sarray = (OlSelectType *) ctx->text.sarray;
			for (sarray = (OlSelectType *) ctx->text.sarray;
			    *sarray != OlselectNull && *sarray != ctx->text.s.type;
			    sarray++) ;
			if (*sarray != OlselectNull) sarray++;
			if (*sarray == OlselectNull)
				sarray = (OlSelectType *) ctx->text.sarray;
			newType = *sarray;
		} else {			/* single-click event */
			newType = *(ctx->text.sarray);
		}
		ctx->text.lasttime = time;
	}
	switch (newType) {
	case OlselectPosition:
		newLeft = newRight = position;
		break;
	case OlselectChar:
		newLeft = position;
		newRight = (*(ctx->text.source->scan))(
		    ctx->text.source, position, OlstPositions, OlsdRight, 1, FALSE);
		break;
	case OlselectWord:
		newLeft = (*(ctx->text.source->scan))
		    (ctx->text.source, position, OlstWhiteSpace, OlsdLeft, 1, FALSE);
		newRight = (*(ctx->text.source->scan))
		    (ctx->text.source, position, OlstWhiteSpace, OlsdRight, 1, FALSE);
		break;
	case OlselectLine:
		line = LineForPosition(ctx, ctx->text.insertPos);
		newLeft = ctx->text.lt.info[line].position;
		newRight = ctx->text.lt.info[line+1].position -
			(OlTextPosition)1;
		break;
	case OlselectParagraph:  /* need "para" scan mode to implement pargraph */
		newLeft = (*(ctx->text.source->scan))(
		    ctx->text.source, position, OlstEOL, OlsdLeft, 1, FALSE);
		newRight = (*(ctx->text.source->scan))(
		    ctx->text.source, position, OlstEOL, OlsdRight, 1, FALSE);
		break;
	case OlselectAll:
		newLeft = (*(ctx->text.source->scan))(
		    ctx->text.source, position, OlstLast, OlsdLeft, 1, FALSE);
		newRight = (*(ctx->text.source->scan))(
		    ctx->text.source, position, OlstLast, OlsdRight, 1, FALSE);
		break;
	}
	if ((newLeft != ctx->text.s.left) || (newRight != ctx->text.s.right)
	    || (newType != ctx->text.s.type)) {
		_XtTextSetNewSelection(ctx, newLeft, newRight);
		ctx->text.s.type = newType;
		_OlSetCursorPos(ctx, newRight);
	}
	if (!motion) { /* setup so we can freely mix select extend calls*/
		ctx->text.origSel.type = ctx->text.s.type;
		ctx->text.origSel.left = ctx->text.s.left;
		ctx->text.origSel.right = ctx->text.s.right;
		if (position >= ctx->text.s.left +
		    ((ctx->text.s.right - ctx->text.s.left) / 2))
			ctx->text.extendDir = OlsdRight;
		else
			ctx->text.extendDir = OlsdLeft;
	}
}	/*  DoSelection  */


/*
 *************************************************************************
 * 
 *  EndAction - 
 * 
 ****************************procedure*header*****************************
 */
static void
EndAction(ctx)
	TextPaneWidget ctx;
{
	CheckResizeOrOverflow(ctx);
	_XtTextExecuteUpdate(ctx);
}	/*  EndAction  */


/*
 *************************************************************************
 * 
 *  ExtendSelection - This routine implements extension of the currently
 *	selected text in the "current" mode (i.e. char word, line, etc.).
 *	It worries about extending from either end of the selection and
 *	handles the case when you cross through the "center" of the
 *	current selection (e.g. switch which end you are extending!).
 *	[NOTE: This routine will be replaced by a set of procedures that
 *	will allows clients to implements a wide class of draw through and
 *	multi-click selection user interfaces.]
 * 
 ****************************procedure*header*****************************
 */
static void
ExtendSelection (ctx, position, motion)
	TextPaneWidget ctx;
	OlTextPosition position;
	Boolean motion;
{
	OlTextPosition newLeft, newRight;
	int line;


	/* check for change in extend direction */
	if ((ctx->text.extendDir == OlsdRight &&
	    position < ctx->text.origSel.left) ||
	    (ctx->text.extendDir == OlsdLeft &&
	    position > ctx->text.origSel.right)) {
		ctx->text.extendDir =
		    (ctx->text.extendDir == OlsdRight)? OlsdLeft : OlsdRight;
		_XtTextSetNewSelection(ctx, ctx->text.origSel.left,
		    ctx->text.origSel.right);
	}
	newLeft = ctx->text.s.left;
	newRight = ctx->text.s.right;
	switch (ctx->text.s.type) {
	case OlselectPosition:
		if (ctx->text.extendDir == OlsdRight)
			newRight = position;
		else
			newLeft = position;
		break;
	case OlselectWord:
		if (ctx->text.extendDir == OlsdRight)
			newRight = position = (*(ctx->text.source->scan))
			    (ctx->text.source, position, OlstWhiteSpace, OlsdRight, 1, FALSE);
		else
			newLeft = position = (*(ctx->text.source->scan))
			    (ctx->text.source, position, OlstWhiteSpace, OlsdLeft, 1, FALSE);
		break;
	case OlselectLine:
		line = LineForPosition(ctx, ctx->text.insertPos);
		newLeft = ctx->text.lt.info[line].position;
		position = newRight = ctx->text.lt.info[line+1].position -
				(OlTextPosition)1;
		break;
	case OlselectParagraph: /* need "para" scan mode to implement pargraph */
		if (ctx->text.extendDir == OlsdRight)
			newRight = position = (*(ctx->text.source->scan))
			    (ctx->text.source, position, OlstEOL, OlsdRight, 1, TRUE);
		else
			newLeft = position = (*(ctx->text.source->scan))
			    (ctx->text.source, position, OlstEOL, OlsdLeft, 1, FALSE);
		break;
	case OlselectAll:

		position = ctx->text.insertPos;
		break;
	}
	_XtTextSetNewSelection(ctx, newLeft, newRight);
	_OlSetCursorPos(ctx, position);
/*
	ctx->text.insertPos = position;
*/
}	/*  ExtendSelection  */


/*
 *************************************************************************
 * 
 *  FlushUpdate - This is a private utility routine used by
 *	_XtTextExecuteUpdate. It processes all the outstanding update
 *	requests and merges update ranges where possible.
 * 
 ****************************procedure*header*****************************
 */
static void
FlushUpdate(ctx)
	TextPaneWidget ctx;
{
	int     i, w;
	OlTextPosition updateFrom, updateTo;

	while (ctx->text.numranges > 0) {
		updateFrom = ctx->text.updateFrom[0];
		w = 0;
		for (i=1 ; i<ctx->text.numranges ; i++) {
			if (ctx->text.updateFrom[i] < updateFrom) {
				updateFrom = ctx->text.updateFrom[i];
				w = i;
			}
		}
		updateTo = ctx->text.updateTo[w];
		ctx->text.numranges--;
		ctx->text.updateFrom[w] = ctx->text.updateFrom[ctx->text.numranges];
		ctx->text.updateTo[w] = ctx->text.updateTo[ctx->text.numranges];
		for (i=ctx->text.numranges-1 ; i>=0 ; i--) {
			while (ctx->text.updateFrom[i] <= updateTo
			    && i < ctx->text.numranges) {
				updateTo = ctx->text.updateTo[i];
				ctx->text.numranges--;
				ctx->text.updateFrom[i] =
				    ctx->text.updateFrom[ctx->text.numranges];
				ctx->text.updateTo[i] =
				    ctx->text.updateTo[ctx->text.numranges];
			}
		}
		DisplayText(ctx, updateFrom, updateTo);
	}
}	/*  FlushUpdate  */


/*
 *************************************************************************
 * 
 *  ForceBuildLineTable - This routine is used to re-display the entire
 *	window, independent of its current state.
 * 
 ****************************procedure*header*****************************
 */
static void
ForceBuildLineTable(ctx)
	TextPaneWidget ctx;
{
	OlTextPosition position;

	position = ctx->text.lt.top;
	ctx->text.lt.top++; /* ugly, but it works */
	BuildLineTable(ctx, position);

}	/*  ForceBuildLineTable  */


/*
 *************************************************************************
 * 
 * InsertCursor - Procedure to manage insert cursor visibility for
 *	editable text.  It uses the value of ctx->insertPos and an
 *	implicit argument. In the event that position is immediately
 *	preceded by an eol graphic, then the insert cursor is displayed
 *	at the beginning of the next line.
 *
 ****************************procedure*header*****************************
 */
static void
InsertCursor (ctx)
	TextPaneWidget ctx;
{
	Position x, y;
	int dy, line, visible;
	OlTextBlock text;

	if (ctx->text.lt.lines < 1) return;
	visible = LineAndXYForPosition(ctx,
			ctx->text.insertPos,
			&line,
			&x,
			&y);
	if (line < ctx->text.lt.lines)
		dy = (ctx->text.lt.info[line + 1].y -
				ctx->text.lt.info[line].y) + 1;
	else
		dy = (ctx->text.lt.info[line].y -
				ctx->text.lt.info[line - 1].y) + 1;

	/*
	 *  If the insert position is just after eol then put
	 *  it on next line
	 */
	if (x > (Position)ctx->text.leftmargin &&
	    ctx->text.insertPos > 0 &&
	    ctx->text.insertPos >= GETLASTPOS(ctx)) {
		/* reading the source is bogus and this code should use scan */
		(*(ctx->text.source->read)) (ctx->text.source,
		    ctx->text.insertPos - 1, &text, 1);
		if (text.ptr[0] == '\n') {
			x = ctx->text.leftmargin;
			y += dy;
		}
	}
	y += dy;
	if (visible)
		(*(ctx->text.sink->insertCursor))(ctx, x, y,
						  ctx->text.insert_state);

}	/*  InsertCursor  */


/*
 *************************************************************************
 * 
 *  InsertNewLineAndBackupInternal - 
 * 
 ****************************procedure*header*****************************
 */
static int
InsertNewLineAndBackupInternal(ctx)
	TextPaneWidget ctx;
{
	OlTextBlock text;
	int result;

	text.length = 1;
	text.ptr = (unsigned char*)"\n";
	text.firstPos = 0;
	if (result =
	    ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos,
	    &text, TRUE)) {
		if (result != OleditReject)
			(void) _OlBeepDisplay((Widget)ctx, 1);
		return(result);
	}
	ctx->text.showposition = TRUE;
	return(result);
}	/*  InsertNewLineAndBackupInternal  */


static Bool
IsGraphicsExpose(d, event, arg)
  Display	*d;
  XEvent	*event;
  char		*arg;
{

  return (Boolean)(((XAnyEvent *)event)->window == (Window)arg &&
		   event->type == GraphicsExpose || event->type == NoExpose);

} /* IsGraphicsExpose() */


/*
 *************************************************************************
 * 
 *  LineAndXYForPosition - This routine maps a source position into
 *	the corresponding line number and the x, y coordinates of the
 *	text that is displayed in the window.  It is illegal to call
 *	this routine unless there is a valid line table.
 * 
 ****************************procedure*header*****************************
 */
static int
LineAndXYForPosition (ctx, pos, line, x, y)
	TextPaneWidget ctx;
	OlTextPosition pos;
	int *line;
	Position *x, *y;
{
	OlTextPosition linePos, endPos;
	int     visible, realW, realH;

	*line = 0;
	*x = ctx->text.leftmargin;
	*y = ctx->text.topmargin;
	visible = IsPositionVisible(ctx, pos);
	if (visible) {
		*line = LineForPosition(ctx, pos);
		*y = ctx->text.lt.info[*line].y;
		*x = ctx->text.lt.info[*line].x;
		linePos = ctx->text.lt.info[*line].position;
		(*(ctx->text.sink->findDistance))(ctx, linePos,
		    *x, pos, &realW, &endPos, &realH);
		*x = *x + realW;
	}
	return visible;
}	/*  LineAndXYForPisition  */


/*
 *************************************************************************
 * 
 *  LineForPosition - This routine maps a source position in to the
 *	corresponding line number of the text that is displayed in
 *	the window.  It is illegal to call this routine unless there
 *	is a valid line table.
 * 
 ****************************procedure*header*****************************
 */
static int
LineForPosition (ctx, position)
	TextPaneWidget ctx;
	OlTextPosition position;
{
	int     line;

	if (position <= ctx->text.lt.info[0].position)
		return 0;
	for (line = 0; line < ctx->text.lt.lines; line++)
		if (position < ctx->text.lt.info[line + 1].position)
			break;
	return line;
}	/*  LineForPosition  */

static void		
LoseClipboard(w, atom)
Widget	w;
Atom		*atom;
{
	TextPaneWidget	ctx = (TextPaneWidget)w;
	if (ctx->text.clip_contents) {
		XtFree(ctx->text.clip_contents);
		ctx->text.clip_contents = NULL;
	}
}

static void	
LosePrimary(ctx, atom)
TextPaneWidget	ctx;
Atom		*atom;
{
	StartAction(ctx, NULL);
	_XtTextSetNewSelection(ctx, ctx->text.insertPos, ctx->text.insertPos);
	EndAction(ctx);
}

static void
PasteTextOnClipBoard(w, str)
Widget w;
char *str;
{

	TextPaneWidget ctx = (TextPaneWidget) w;
	int	i;
	char	*buffer;

	if (ctx->text.clip_contents) {
		XtFree(ctx->text.clip_contents);
	}

	i = strlen(str);
	buffer = XtMalloc(2 + i);	/* grep ^@ for details */
	(void) strncpy (buffer, str, i);
	buffer[i] = ' ';
	buffer[i+1] = '\0';
	ctx->text.clip_contents = buffer;

	if (!XtOwnSelection(	(Widget)ctx,
				XA_CLIPBOARD(XtDisplay((Widget)ctx)),
				ctx->text.time,
				ConvertSelection,
				LoseClipboard,
				NULL)) {

			OlVaDisplayWarningMsg(	(Display *)NULL,
						OleNfileTextPane,
						OleTmsg1,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg1);
	}
}	/*  PasteTextOnClipBoard  */


/*
 *************************************************************************
 * 
 *  NewSelection - 
 * 
 ****************************procedure*header*****************************
 */
static void
NewSelection(ctx, l, r)
	TextPaneWidget ctx;
	OlTextPosition l, r;
{
	unsigned char   *ptr;
	_XtTextSetNewSelection(ctx, l, r);
	if (l < r) {
		ptr = _OlTextCopySubString(ctx, l, r);
		XStoreBuffer(XtDisplay(ctx),
			(OLconst char *)ptr,
			min((int)_OlStrlen(ptr), (int)MAXCUT),
			0);
		XtFree((char *)ptr);
	}
}	/*  NewSelection  */


/*
 *************************************************************************
 * 
 *  NextPosition - 
 * 
 ****************************procedure*header*****************************
 */
static OlTextPosition
NextPosition(ctx, position, kind, direction)
	TextPaneWidget ctx;
	OlTextPosition   position;
	OlScanType       kind;
	OlScanDirection  direction;
{
	OlTextPosition pos;

	pos = (*(ctx->text.source->scan))(
	    ctx->text.source, position, kind, direction, 1, FALSE);
	if (pos == ctx->text.insertPos)
		pos = (*(ctx->text.source->scan))(
		    ctx->text.source, position, kind, direction, 2, FALSE);
	return pos;
}	/*  NextPosition  */


/*
 *************************************************************************
 * 
 * PositionForXY - This routine maps an x and y position in a window
 *	that is displaying text into the corresponding position
 *	in the source.  It is illegal to call this routine unless there
 *	is a valid line table.
 * 
 ****************************procedure*header*****************************
 */
static OlTextPosition
PositionForXY(ctx, x, y)
	TextPaneWidget ctx;
	Position x,y;
{
	int width;
	int fromx;
	int line;
	OlTextPosition position;
	OlTextPosition resultstart;
	OlTextPosition resultend;
	OlTextPosition lastpos = GETLASTPOS(ctx);

	/* figure out what line it is on */
	for (line = 0; line < ctx->text.lt.lines - 1; line++) {
		if (y <= ctx->text.lt.info[line + 1].y)
			break;
	}

	position = ctx->text.lt.info[line].position;
	if (position >= lastpos)
		return lastpos;

	fromx = ctx->text.lt.info[line].x;	/* starting x in line */
	width = x - fromx;		/* num of pix from starting of line */

	(*(ctx->text.sink->resolve)) (ctx, position, fromx, width,
	    &resultstart, &resultend);

	if (resultstart >= ctx->text.lt.info[line + 1].position)
		resultstart = (*(ctx->text.source->scan))(ctx->text.source,
			ctx->text.lt.info[line + 1].position,
			OlstPositions,
			OlsdLeft,
			1,
			TRUE);

	return resultstart;

}	/*  PositionForXY  */



/*
 *************************************************************************
 * 
 *  ReplaceText - This internal routine deletes the text from pos1 to
 *	pos2 in a source and then inserts, at pos1, the text that was
 *	passed. As a side effect it "invalidates" that portion of the
 *	displayed text (if any).  It is illegal to call this routine
 *	unless there is a valid line table.
 * 
 ****************************procedure*header*****************************
 */
static int
ReplaceText (ctx, pos1, pos2, text, verify)
	TextPaneWidget ctx;
	OlTextPosition pos1, pos2;
	OlTextBlock *text;
	Boolean     verify;
{
	int             i, line1, line2, visible, delta;
	int    error;
	Position        x, y;
	Dimension       realW, realH, width;
	OlTextPosition  startPos, endPos, updateFrom, lastpos;
	OlTextVerifyCD  cbdata;
	OlTextBlock	    newtxtblk;

	newtxtblk.ptr = (unsigned char*) XtMalloc(text->length);
	newtxtblk.firstPos = text->firstPos;
	newtxtblk.length = text->length;
	(void) strncpy((char *)newtxtblk.ptr,
			(OLconst char *)text->ptr, text->length);
	cbdata.operation = modVerify;
	cbdata.doit = TRUE;
	cbdata.currInsert = ctx->text.insertPos;
	cbdata.newInsert = ctx->text.insertPos;
	cbdata.startPos = pos1;
	cbdata.endPos = pos2;
	cbdata.text = &newtxtblk;

	if (verify) { 
		if (XtClass(XtParent(XtParent(ctx))) == textWidgetClass)
			XtCallCallbacks((Widget)XtParent(XtParent(ctx)),
				XtNmodifyVerification, &cbdata);
		else
			XtCallCallbacks((Widget)ctx,
				XtNmodifyVerification, &cbdata);

		if (!cbdata.doit) {
			/* Necessary inorder to return to initial state */
			XtFree((char *)newtxtblk.ptr);
			text->length = 0; 
			return OleditReject;
		}
		else
			text->length = newtxtblk.length;

		/*
		 * Extract any new data changed by the verification callback.
		 * newtxtblk is used in the actual replace call later.
		 */
		pos1 = cbdata.startPos;
		pos2 = cbdata.endPos;
		ctx->text.insertPos = cbdata.newInsert;
	}

	/*
	 *  The insertPos may not always be set to the right spot
	 *  in OltextAppend
	 */
	if ((pos1 == ctx->text.insertPos) && 
	    ((*(ctx->text.source->editType))(ctx->text.source) == OL_TEXT_APPEND)) {
		ctx->text.insertPos = GETLASTPOS(ctx);
		pos2 = pos2 - pos1 + ctx->text.insertPos;
		pos1 = ctx->text.insertPos;
	}
	updateFrom = (*(ctx->text.source->scan))
	    (ctx->text.source, pos1, OlstWhiteSpace, OlsdLeft, 1, TRUE);
	updateFrom = (*(ctx->text.source->scan))
	    (ctx->text.source, updateFrom, OlstPositions, OlsdLeft, 1, TRUE);
	startPos = max(updateFrom, ctx->text.lt.top);
	visible = LineAndXYForPosition(ctx, startPos, &line1, &x, &y);
	error = (*(ctx->text.source->replace))
	    (ctx->text.source, pos1, pos2, &newtxtblk, &delta);
	XtFree((char *)newtxtblk.ptr);
	if (error) return error;
	lastpos = GETLASTPOS(ctx);
	if (ctx->text.lt.top >= lastpos) {
		BuildLineTable(ctx, lastpos);
		/* ClearWindow(ctx); */
		ClearText(ctx);
		return error;
	}
	if (delta < lastpos) {
		for (i = 0; i < ctx->text.numranges; i++) {
			if (ctx->text.updateFrom[i] > pos1)
				ctx->text.updateFrom[i] += delta;
			if (ctx->text.updateTo[i] >= pos1)
				ctx->text.updateTo[i] += delta;
		}
	}

	line2 = LineForPosition(ctx, pos1);
	/* 
	 * fixup all current line table entries to reflect edit.
	 * BUG: it is illegal to do arithmetic on positions. This code should
	 * either use scan or the source needs to provide a function for doing
	 * position arithmetic.
	 */
        for (i = line2 + 1; i <= ctx->text.lt.lines; i++) {
		ctx->text.lt.info[i].position += delta;
		ctx->text.lt.info[i].drawPos += delta;
	}

	endPos = pos1;
	/*
	 * Now process the line table and fixup in case edits caused
	 * changes in line breaks. If we are breaking on word boundaries,
	 * this code checks for moving words to and from lines.
	 */
	if (visible) {
		OlTextLineRange lastChangedLine ;
		/* force check for word moving to prev line */
		if (line1) line1-- ;
		lastChangedLine =
		    UpdateLineTable (ctx,
		    ctx->text.lt.info[line1].position,
		    pos2 + delta,
		    ctx->core.width - ctx->text.leftmargin
		    	- ctx->text.rightmargin,
		    (OlTextPosition)line1,
		    TRUE);
		endPos = ctx->text.lt.info[lastChangedLine+1].position ;
	}

	/*
	 * Now process the position table and fixup in case edits caused
	 * changes in line breaks.
	 */
	_OlPtBuild(ctx, ctx->text.lt.info[line1].position);

	lastpos = GETLASTPOS(ctx);
	if (delta >= lastpos)
		endPos = lastpos;
	if (delta >= lastpos || pos2 >= ctx->text.lt.top)
		_XtTextNeedsUpdating(ctx, updateFrom, endPos);

	if (ctx->text.verticalSB)
	  _OlUpdateVerticalSB(ctx);

	return error;

}	/*  ReplaceText  */


/*
 *************************************************************************
 * 
 *  SelectStart - 
 * 
 ****************************procedure*header*****************************
 */
static void
SelectStart(ctx, event)
	TextPaneWidget	ctx;
	XEvent *	event;
{
	Time		time = event->xbutton.time;

	if (HAS_FOCUS(ctx) ||
	    XtCallAcceptFocus((Widget)ctx, &time) == TRUE) {
		StartAction(ctx, event);

		AlterSelection(ctx, OlsmTextSelect, OlactionStart);

		EndAction(ctx);
	}
}	/*  SelectStart  */


/*
 *************************************************************************
 * 
 *  StartAction - 
 * 
 ****************************procedure*header*****************************
 */
static void
StartAction(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	_XtTextPrepareToUpdate(ctx);
	if (event) {
		ctx->text.time = event->xbutton.time;
		ctx->text.ev_x = event->xbutton.x;
		ctx->text.ev_y = event->xbutton.y;
	}
}	/*  StartAction  */


/*
 *************************************************************************
 * 
 *  StuffFromBuffer - 
 * 
 ****************************procedure*header*****************************
 */
static void
StuffFromBuffer(ctx, buffer)
TextPaneWidget ctx;
int buffer;
{
	XtGetSelectionValue(	(Widget)ctx,
				XA_CLIPBOARD(XtDisplay(ctx)),
				XA_STRING,
				TakeFromClipboard,
				NULL,
				ctx->text.time);
}

/*
**  TakeFromClipboard will be called when the clipboard owner responds
**  with the clipboard contents.
*/

static void
TakeFromClipboard(ctx, client_data, selection, type, value, length, format)
TextPaneWidget	ctx;
XtPointer	client_data;
Atom	*selection;
Atom	*type;
XtPointer	value;
unsigned long	*length;
int	*format;
{
	OlTextBlock text;
	int result;
	OlTextPosition nextcursorpos;
	static Arg	args[15];
	int i;

	if ((*length) == 0)
		return;

	text.ptr = (unsigned char *) value;
	text.length = *length - 1;	/* get rid of ^@ */
	text.firstPos = (OlTextPosition) 0;

	if (result = ReplaceText(	ctx, 
					ctx->text.insertPos, 
					ctx->text.insertPos,
					&text, 
					TRUE)) {
		if (result != OleditReject) {
			(void) _OlBeepDisplay((Widget)ctx, 1);

		}
		XtFree(value);
		return;
	}

	nextcursorpos = (*(ctx->text.source->scan)) (	ctx->text.source, 
							ctx->text.insertPos, 
							OlstPositions, 
							OlsdRight,
							text.length, 
							TRUE);
	_OlSetCursorPos(ctx, nextcursorpos);
	nextcursorpos = ctx->text.insertPos;
	_XtTextSetNewSelection(ctx, nextcursorpos, nextcursorpos);

	DisplayAllText(ctx);
	_XtTextExecuteUpdate(ctx);
	XtFree(value);
}

/*
 *************************************************************************
 * 
 *  UpdateLineTable - 
 * 
 ****************************procedure*header*****************************
 */
static OlTextLineRange
UpdateLineTable (self, pos0, posF, width, line0, updateMode)
	TextPaneWidget	self;
	OlTextPosition	pos0, posF;
	Dimension		width;
	OlTextPosition	line0;
	int		updateMode;
{
	OlLineTableEntryPtr lp, currLine;
	OlLineTablePtr      lt;
	OlTextPosition line, nLines;
	Dimension	x0, y;
	int		reqW, reqH;
	TextFit	fit;
	TextFit	(*textFitFn)();
	int		wrapEnabled, breakOnWhiteSpace;
	OlTextPosition fitPos, drawPos, nextPos;

	textFitFn =  self->text.sink->textFitFn;
	lt = &(self->text.lt);
	nLines = lt->lines;
	x0 = self->text.leftmargin;
	y = updateMode ? lt->info[line0].y : self->text.topmargin;
	reqH = 0;
	wrapEnabled = (int) self->text.wrap_mode;
	breakOnWhiteSpace =
	    wrapEnabled && (self->text.wrap_break == OL_WRAP_WHITE_SPACE);

	for (line = line0; line <= nLines; line++)
	{ 
		currLine = &(lt->info[line]);
		currLine->x = x0;
		currLine->y = y;
		currLine->position = pos0;
		if (pos0 <= GETLASTPOS(self)) { 
			currLine->fit = (*textFitFn) ( self,
			    pos0,
			    x0,
			    width,
			    wrapEnabled,
			    breakOnWhiteSpace,
			    &fitPos,
			    &drawPos,
			    &nextPos,
			    &reqW,
			    &reqH);
			currLine->drawPos = drawPos;

			currLine->endX = x0 + reqW;
			pos0 = nextPos;

			/*
			 * In update mode we must go through the last line
			 * which had a character replaced in it before
			 * terminating on a mere position match. Starting
			 * position (of replacement) would be sufficient
			 * only if we know the font is fixed width. (Good
			 * place to optimize someday, huh?)
			 */

			if (updateMode && (nextPos > posF)
			    && (nextPos == lt->info[line+1].position)) { 
				break;
			}
		}
		else { 
			currLine->endX = x0;
			currLine->fit = tfEndText ;
		}
		y += reqH;
	};

	return ((line < nLines) ? line : nLines - 1 );
}	/*  UpdateLineTable  */



/*
 *************************************************************************
 *
 *  _OlUpdateVerticalSB -
 *
 ****************************procedure*header*****************************
 */
void
_OlUpdateVerticalSB(ctx)
  TextPaneWidget	ctx;
{
	static	TextPaneWidget	OldCtx = (TextPaneWidget) 0;
	static	int	OldProportionLength = 0;
	static	int	OldSliderMax = 0;
	static	int	OldSliderValue = 0;
	int	proportionLength;
	int	sliderMax;
	int	sliderValue;
	int	n;
	Arg	arg[3];

	/********************************************************************
	 * proportionLength = number of text lines that fit in Pane
	 * sliderValue = line number of first line in Pane
	 * sliderMax = Maximum of:
	 *	line # of first line in Pane + #lines that fit in Pane  -or-
	 *	total lines in the source buffer 
	 *******************************************************************/

	proportionLength = ctx->text.lt.lines;
	sliderValue = _OlPtLineFromPos(ctx, ctx->text.lt.top);
	sliderMax = proportionLength + sliderValue;
	if (sliderMax < ctx->text.pt.lines) sliderMax = ctx->text.pt.lines;

	/*******************************************************************
	 * Performance hack:  only call XtSetValues if the widget or any 
	 * of its resources have changed since the last call 
	 ******************************************************************/

	if (ctx != OldCtx ||
			proportionLength != OldProportionLength ||
			sliderValue != OldSliderValue ||
			sliderMax != OldSliderMax) {
		
		OldCtx = ctx;
		OldProportionLength = proportionLength;
		OldSliderValue = sliderValue;
		OldSliderMax = sliderMax;

		n=0;
		XtSetArg(arg[n], XtNproportionLength, proportionLength);  n++;
		XtSetArg(arg[n], XtNsliderValue, sliderValue);		  n++;
		XtSetArg(arg[n], XtNsliderMax, sliderMax);		  n++;

		XtSetValues(ctx->text.verticalSB, arg, n);
	}

}	/* _OlUpdateVerticalSB */

/*
 *************************************************************************
 * 
 *  OlTextCopySubString - 
 * 
 ****************************procedure*header*****************************
 */
unsigned char *
OlTextCopySubString(w, left, right)
	TextPaneWidget w;
	OlTextPosition left, right;
{
	/* invoke w's copy_substring function on the given parameters */
	return (*((TextPaneClassRec *)(w->core.widget_class))->
		textedit_class.copy_substring) ((TextPaneWidget)w, left, right);
}	/*  OlTextCopySubString  */


/*
 *************************************************************************
 * 
 *  OlTextInvalidate - 
 * 
 ****************************procedure*header*****************************
 */
void
OlTextInvalidate(w, from, to)
	TextPaneWidget     w;
	OlTextPosition       from,to;
{
	_XtTextPrepareToUpdate(w);
	_XtTextNeedsUpdating(w, from, to);
	ForceBuildLineTable(w);
	_XtTextExecuteUpdate(w);
}	/*  OlTextInvalidate  */


/*
 *************************************************************************
 * 
 *  OlTextTopPosition - 
 * 
 ****************************procedure*header*****************************
 */
OlTextPosition
OlTextTopPosition(w)
	TextPaneWidget  w;
{
	return w->text.lt.top;
}	/*  OlTextTopPosition  */


/*
 *************************************************************************
 * 
 *  _OlSetCursorPos - 
 * 
 ****************************procedure*header*****************************
 */
static int
_OlSetCursorPos(ctx, newposition)
	TextPaneWidget  ctx;
	OlTextPosition  newposition;
{
	OlTextVerifyCD  cbdata;

	if (newposition == ctx->text.insertPos)
		return OleditDone;

	cbdata.operation = motionVerify;
	cbdata.doit = TRUE;
	cbdata.currInsert = ctx->text.insertPos;
	cbdata.newInsert = newposition;

	if (XtClass(XtParent(XtParent(ctx))) == textWidgetClass)
		XtCallCallbacks(XtParent(XtParent((Widget)ctx)),
			XtNmotionVerification, &cbdata);
	else
		XtCallCallbacks((Widget)ctx, XtNmotionVerification, &cbdata);

	if (!cbdata.doit)
		return OleditReject;

	ctx->text.insertPos = cbdata.newInsert;

	return OleditDone;
}	/*  _OlSetCursorPos  */


/*
 *************************************************************************
 * 
 *  _XtTextExecuteUpdate - This routine causes all batched screen
 *	updates to be performed.
 * 
 ****************************procedure*header*****************************
 */
static void
_XtTextExecuteUpdate(ctx)
TextPaneWidget ctx;
{
	if ((ctx->text.oldinsert >= 0) && ctx->text.update_flag) {
		if (ctx->text.oldinsert != ctx->text.insertPos
		    || ctx->text.showposition)
			_XtTextShowPosition(ctx);
		FlushUpdate(ctx);
		InsertCursor(ctx);
		ctx->text.oldinsert = -1;
	}
}	/*  _XtTextExecuteUpdate  */


/*
 *************************************************************************
 * 
 * _XtTextNeedsUpdating - Procedure to register a span of text that is
 *	no longer valid on the display. It is used to avoid a number
 *	of small, and potentially overlapping, screen updates.
 *	[note: this is really a private procedure but is used in
 *	 multiple modules].
 * 
 ****************************procedure*header*****************************
 */
static void
_XtTextNeedsUpdating(ctx, left, right)
	TextPaneWidget ctx;
	OlTextPosition left, right;
{
	int     i;

	if (left < right) {
		for (i = 0; i < ctx->text.numranges; i++) {
			if (left <= ctx->text.updateTo[i]
			    && right >= ctx->text.updateFrom[i])
			{ 
				ctx->text.updateFrom[i] =
					min(left, ctx->text.updateFrom[i]);
				ctx->text.updateTo[i] =
					max(right, ctx->text.updateTo[i]);
				return;
			}
		}

		ctx->text.numranges++;

		if (ctx->text.numranges > ctx->text.maxranges) {
			ctx->text.maxranges = ctx->text.numranges;
			i = ctx->text.maxranges * sizeof(OlTextPosition);
			ctx->text.updateFrom = (OlTextPosition *)
			    XtRealloc((char *)ctx->text.updateFrom,
					(unsigned) i);
			ctx->text.updateTo = (OlTextPosition *)
			    XtRealloc((char *)ctx->text.updateTo,
					(unsigned) i);
		}

		ctx->text.updateFrom[ctx->text.numranges - 1] = left;
		ctx->text.updateTo[ctx->text.numranges - 1] = right;
	}

}	/*  _XtTextNeedsUpdating  */


/*
 *************************************************************************
 * 
 *  _XtTextPrepareToUpdate - This routine does all setup required to
 *	syncronize batched screen updates.
 * 
 ****************************procedure*header*****************************
 */
static void
_XtTextPrepareToUpdate(ctx)
	TextPaneWidget ctx;
{

	if ((ctx->text.oldinsert < 0) && ctx->text.update_flag) {
		InsertCursor(ctx);
		ctx->text.numranges = 0;
		ctx->text.showposition = FALSE;
		ctx->text.oldinsert = ctx->text.insertPos;
	}
}	/*  _XtTextPrepareToUpdate  */


/*
 *************************************************************************
 * 
 *  _OlTextScroll - The routine will scroll the displayed text by lines.
 *	If the arg is positive, move up; otherwise, move down.
 *	[note: this is really a private procedure but is used in
 *	multiple modules].
 * 
 ****************************procedure*header*****************************
 */
static void
_OlTextScroll(ctx, n)
	TextPaneWidget ctx;
	int n;
{
	unsigned cur_line, top_line;

	cur_line = _OlPtLineFromPos(ctx, ctx->text.lt.info[0].position);
	top_line = cur_line + n;

	_OlTextScrollAbsolute(ctx, top_line, FALSE);
}	/* _OlTextScroll() */


/*
 *************************************************************************
 * 
 * _OlTextScrollAbsolute - The routine scrolls 'top_line' to the
 * displayed text.
 * 
 ****************************procedure*header*****************************
 */
void
_OlTextScrollAbsolute(ctx, top_line, update)
	TextPaneWidget ctx;
	unsigned top_line;
	Boolean update;
{
	register TextPanePart *text = (TextPanePart *) &(ctx->text);
	register OlLineTablePtr lt = &(text->lt);
	OlTextPosition top_pos, lastpos = GETLASTPOS(ctx);
	Dimension textwidth = ctx->core.width;
	Dimension textheight;
	Position copy_from_y, copy_to_y;
	Boolean copy_area = FALSE;
	Position clear_from_y;
	Dimension clear_height;
	OlTextPosition update_from_pos, update_to_pos;
	int n;
	XEvent expose_event;


	if (top_line < 0 || top_line > text->pt.lines - 1)
	  return;

	n = top_line - _OlPtLineFromPos(ctx, lt->info[0].position);
	top_pos = _OlPtPosFromLine(ctx, top_line);

	if (update)
		_XtTextPrepareToUpdate(ctx);

	if (n > 0 && n < lt->lines) {
		copy_from_y = lt->info[n].y;
		textheight = ctx->core.height - lt->info[n].y -
			text->bottommargin;
		copy_to_y = text->topmargin;
		copy_area = TRUE;
		clear_from_y = lt->info[lt->lines-n].y;
		clear_height = ctx->core.height - clear_from_y;
		BuildLineTable(ctx, top_pos);
		update_from_pos = lt->info[lt->lines - n].position;
		update_to_pos = lastpos;
	}
	else if (n < 0 && -n < lt->lines) {
		copy_from_y = text->topmargin;
		textheight = lt->info[lt->lines+n].y - text->topmargin;
		copy_to_y = lt->info[-n].y;
		copy_area = TRUE;
		clear_from_y = text->topmargin;
		clear_height = copy_to_y - text->topmargin;
		BuildLineTable(ctx, top_pos);
		update_from_pos = lt->info[0].position;
		update_to_pos = lt->info[-n].position;
	} else {
		clear_from_y = text->topmargin; 
		clear_height = ctx->core.height - text->topmargin -
			text->bottommargin;
		textheight = 0;
		BuildLineTable(ctx, top_pos);
		update_from_pos = lt->info[0].position;
		update_to_pos = lastpos;
	}

	if (n != 0) {
		if (copy_area)
			XCopyArea(XtDisplay(ctx),
				XtWindow(ctx),
				XtWindow(ctx),
				text->gc,
				0,
				copy_from_y,
				textwidth,
				textheight,
				0,
				copy_to_y);

		(*(text->sink->clearToBackground))(ctx,
			0,
			clear_from_y,
			textwidth,
			clear_height);

		_XtTextNeedsUpdating(ctx, update_from_pos, update_to_pos);

		if (text->verticalSB) 
			_OlUpdateVerticalSB(ctx);
	}

	if (update)
		_XtTextExecuteUpdate(ctx);

	/*
	** Block until a GraphicsExpose or NoExpose event
	*/
	if (copy_area)
	  do {
	      XIfEvent(XtDisplay(ctx), &expose_event,
		       IsGraphicsExpose, (char *)XtWindow(ctx));
	      ProcessExposeRegion((Widget)ctx,
				  &expose_event, (Region)NULL);
          } while (expose_event.type != NoExpose &&
		   ((XGraphicsExposeEvent *)&expose_event)->count != 0);

}	/*  _OlTextScrollAbsolute  */


/*
 *************************************************************************
 * 
 *  _XtTextShowPosition - This is a private utility routine used by
 *	_XtTextExecuteUpdate. This routine worries about edits causing
 *	new data or the insertion point becoming invisible (off the
 *	screen). Currently it always makes it visible by scrolling.
 *	It probably needs generalization to allow more options.
 * 
 ****************************procedure*header*****************************
 */
static void
_XtTextShowPosition(ctx)
	TextPaneWidget ctx;
{
	OlTextPosition top, first, second, insertPos ;
	OlTextPosition lastpos = GETLASTPOS(ctx);
	TextPanePart *text = &(ctx->text) ;
	OlLineTablePtr lt = &(text->lt) ;
	short hScroll ;

	/* NOTE: Following code relies on current assumption that
       horizontal scrolling will be enabled only when there is
       only one display line.
       */

	insertPos = text->insertPos ;

	if (   insertPos < lt->top
	    || insertPos >= lt->info[lt->lines].position
	    || (hScroll = ((text->scroll_state & OL_AUTO_SCROLL_HORIZONTAL)
	    && (insertPos > lt->info[0].drawPos)
	    && ( lt->info[0].drawPos + 1 < lastpos))
	    ? 1 : 0 )) {
		if (   lt->lines > 0
		    && (insertPos < lt->top 
		    || lt->info[lt->lines].position <=  lastpos
		    || hScroll)) {
			first = lt->top;
			second = lt->info[1].position;
			if (insertPos < first)
				top = (*(text->source->scan))(
				    text->source, insertPos, OlstEOL,
				    OlsdLeft, 1, FALSE);
			else
				top = (*(text->source->scan))(
				    text->source, insertPos, OlstEOL,
				    OlsdLeft, lt->lines, FALSE);
			BuildLineTable(ctx, top);
			while (insertPos >=
			    lt->info[lt->lines].position) {
				if (lt->info[lt->lines].position >
				    lastpos)
					break;
				BuildLineTable(ctx, lt->info[1].position);
			}
			if ((text->scroll_state & OL_AUTO_SCROLL_HORIZONTAL)
			    && (insertPos > lt->info[0].drawPos)
			    && ( lt->info[0].drawPos + 1 < lastpos)) {
		/* insert position is on line 0, attempt to scroll so
		   that insert pos is 2/3 of the way from left to right
		   margin.  First guess assumes fixed width font.  It
		   will iterate to guarantee visibility for proportional
		   fonts.
					   */
				OlTextPosition delta = 0 ;

				top = insertPos - 2 * (lt->info[0].drawPos
				    - lt->info[0].position) / 3 ;
				while (insertPos > lt->info[0].drawPos+1)
				{ 
					BuildLineTable (ctx, top += delta) ;
					delta = (insertPos - top) >> 2 ;
				}
				first = -1 ; /* prevent scroll down by one line */
			};
			if (lt->top == second && lt->lines > 1) {
				BuildLineTable(ctx, first);
				_OlTextScroll(ctx, 1);
			} else if (lt->info[1].position == first && lt->lines > 1) {
				BuildLineTable(ctx, first);
				_OlTextScroll(ctx, -1);
			} else {
				text->numranges = 0;
				if (lt->top != first)
					DisplayAllText(ctx);
				if (ctx->text.verticalSB)
					_OlUpdateVerticalSB(ctx);
			}
		}
	}
}	/*  _XtTextShowPosition  */


/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/****************************procedure*header*****************************
 *  ClassInitialize - This procedure registers the converters used by
 *	Text widgets and converts the virtual translations for the
 *	entire class.
 */
static void
ClassInitialize()
{
	char	buf[sizeof(defaultTextPaneTranslations1) +
			sizeof(defaultTextPaneTranslations2)];

	(void)strcpy(buf, defaultTextPaneTranslations1);
	(void)strcpy(buf + sizeof(defaultTextPaneTranslations1) - 1,
			defaultTextPaneTranslations2);

	textPaneWidgetClass->core_class.tm_table =
				OlConvertVirtualTranslation(buf);

					/* XtNscroll */
	_OlAddOlDefineType ("auto_scroll_off",        OL_AUTO_SCROLL_OFF);
	_OlAddOlDefineType ("auto_scroll_vertical",   OL_AUTO_SCROLL_VERTICAL);
	_OlAddOlDefineType ("auto_scroll_horizontal", OL_AUTO_SCROLL_HORIZONTAL);
	_OlAddOlDefineType ("auto_scroll_both",       OL_AUTO_SCROLL_BOTH);

					/* XtNwrapForm */
	_OlAddOlDefineType ("source_form",  OL_SOURCE_FORM);
	_OlAddOlDefineType ("display_form", OL_DISPLAY_FORM);

					/* XtNgrow */
	_OlAddOlDefineType ("off",          OL_GROW_OFF);
	_OlAddOlDefineType ("horizontal",   OL_GROW_HORIZONTAL);
	_OlAddOlDefineType ("vertical",     OL_GROW_VERTICAL);
	_OlAddOlDefineType ("both",         OL_GROW_BOTH);

                                        /* XtNwrapBreak */
	_OlAddOlDefineType ("wrapany",        OL_WRAP_ANY);
	_OlAddOlDefineType ("wrapwhitespace", OL_WRAP_WHITE_SPACE);

                                        /* XtNeditType */
	_OlAddOlDefineType ("textread",  OL_TEXT_READ);
	_OlAddOlDefineType ("textedit",  OL_TEXT_EDIT);

                                        /* XtNsourceType */
	_OlAddOlDefineType ("stringsrc", OL_STRING_SOURCE);
	_OlAddOlDefineType ("disksrc",   OL_DISK_SOURCE);
}	/*  ClassInitialize  */


/*
 *************************************************************************
 * 
 *  ClassPartInitialize - This procedure allows inheritance of the TextPane
 *	class procedures.
 * 
 ****************************procedure*header*****************************
 */
static void
ClassPartInitialize(class)
	WidgetClass class;
{
	TextPaneWidgetClass wc = (TextPaneWidgetClass)class;
	TextPaneWidgetClass super =
				(TextPaneWidgetClass)wc->core_class.superclass;
#ifdef SHARELIB
	void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit		(*__libXol__XtInherit)
#endif

	if (wc->textedit_class.copy_substring == XtInheritCopySubstring)
		wc->textedit_class.copy_substring =
			super->textedit_class.copy_substring;

	if (wc->textedit_class.copy_selection == XtInheritCopySelection)
		wc->textedit_class.copy_selection =
			super->textedit_class.copy_selection;

	if (wc->textedit_class.unset_selection == XtInheritUnsetSelection)
		wc->textedit_class.unset_selection =
			super->textedit_class.unset_selection;

	if (wc->textedit_class.set_selection == XtInheritSetSelection)
		wc->textedit_class.set_selection =
			super->textedit_class.set_selection;

	if (wc->textedit_class.replace_text == XtInheritReplaceText)
		wc->textedit_class.replace_text =
			super->textedit_class.replace_text;

	if (wc->textedit_class.redraw_text == XtInheritRedrawText)
		wc->textedit_class.redraw_text =
			super->textedit_class.redraw_text;

	OlClassSearchTextDB (class);

}	 /* ClassPartInitialize */



/*
 *************************************************************************
 * 
 *  GetValuesHook - 
 * 
 ****************************procedure*header*****************************
 */
static void
GetValuesHook(widget, args, num_args)
	Widget widget;
	ArgList args;
	Cardinal *num_args;
{
	OlTextSource *source = ((TextPaneWidget)widget)->text.source;
	OlTextSink *sink = ((TextPaneWidget)widget)->text.sink;

	/* Check to see if our source or since is NULL since one of
	 * our superclasses may call XtGetValues from their class's
	 * Initialize procedure -- which implies we have not yet
	 * created our sink and source yet.
	 */

	if (source != (OlTextSource*)NULL && sink != (OlTextSink*)NULL) {
	    int i = 0;

		/* This is ugly, but have to get internal buffer to
		 * initial_string storage */
	    while (i < *num_args) {
		if (strcmp(args[i].name, XtNstring) == 0) {
			((StringSourcePtr)(source->data))->initial_string =
				    OlTextCopyBuffer((TextPaneWidget)widget);
			break;
		};
		i++;
	    };

		XtGetSubvalues(source->data, source->resources,
			source->resource_num, args, *num_args);
		XtGetSubvalues(sink->data, sink->resources,
			sink->resource_num, args, *num_args);
	}
}	/*  GetValuesHook  */


static void
HighlightHandler(
#ifdef OlNeedFunctionPrototypes
    Widget w, OlDefine highlight_type)
#else
	w, highlight_type)
   Widget	w;
   OlDefine	highlight_type;
#endif
{
   TextPaneWidget	ctx = (TextPaneWidget)w;

   switch((int)highlight_type)
   {
   case OL_IN:
	ctx->text.insert_state = OlisOn;
	InsertCursor(ctx);
	break;
   case OL_OUT:
	ctx->text.insert_state = OlisOff;
	InsertCursor(ctx);
	break;
   default:
			OlVaDisplayWarningMsg(	(Display *)NULL,
						OleNfileTextPane,
						OleTmsg2,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg2);
   }
}	/* HighlightHandler() */


/*
 *************************************************************************
 * 
 *  Initialize - 
 * 
 ****************************procedure*header*****************************
 */
static void
Initialize(request, new, args, num_args)
	Widget		request;
	Widget		new;
	ArgList		args;
	Cardinal *	num_args;
{
	TextPaneWidget ctx = (TextPaneWidget)new;
	TextPanePart *text = &(ctx->text);
	XGCValues	gcv;

			/* Until we convert this widget to use the new
			 * event handling scheme
			 */
	_OlDeleteDescendant(new);

	if (text->lt.top < 0)  {
			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg3,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg3,
						XtNdisplayPosition,
						text->lt.top);
		text->lt.top = 0;
		}
	if ((Dimension)(text->leftmargin + text->rightmargin) 
						> ctx->core.width)  {
			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg4,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg4,
						XtNleftMargin,
						text->leftmargin,
						XtNrightMargin,
						text->rightmargin);
		text->leftmargin = (Dimension) 2;
		text->rightmargin = (Dimension) 2;
		}
	if ((Dimension)(text->topmargin + text->bottommargin) 
						> ctx->core.height)  {
			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg4,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg4,
						XtNtopMargin,
						text->topmargin,
						XtNbottomMargin,
						text->bottommargin);
		text->topmargin = (Dimension) 2;
		text->bottommargin = (Dimension) 2;
		}
	text->lt.lines = 0;
	text->lt.info = NULL;
	_OlPtInitialize(&(text->pt));
	text->s.left = text->s.right = 0;
	text->s.type = OlselectPosition;
	text->sarray[0] = OlselectPosition;
	text->sarray[1] = OlselectWord;
	text->sarray[2] = OlselectLine;
	text->sarray[3] = OlselectParagraph;
	text->sarray[4] = OlselectAll;
	text->sarray[5] = OlselectNull;
	text->lasttime = 0; /* ||| correct? */
	text->time = 0; /* ||| correct? */

	text->insert_state = OlisOff;
	text->oldinsert = -1 ;
	text->update_flag = FALSE;
	text->showposition = TRUE;
	text->updateFrom = (OlTextPosition *) XtMalloc(1);
	text->updateTo = (OlTextPosition *) XtMalloc(1);
	text->numranges = text->maxranges = 0;
	text->clip_contents = NULL;

	gcv.graphics_exposures = True;
	text->gc = XtGetGC(new, GCGraphicsExposures, &gcv);

	text->scroll_state = text->scroll_mode ;
	text->grow_state = text->grow_mode ;
	text->prevW = ctx->core.width ;
	text->prevH = ctx->core.height ;
	text->text_clear_buffer = OlTextClearBuffer;
	text->text_copy_buffer = OlTextCopyBuffer;
	text->text_get_insert_point = OlTextGetInsertPos;
	text->text_get_last_pos = OlTextGetLastPos;
	text->text_insert = OlTextInsert;
	text->text_read_sub_str = OlTextReadSubString;
	text->text_redraw = OlTextRedraw;
	text->text_replace = OlTextReplace;
	text->text_set_insert_point = OlTextSetInsertPos;
	text->text_set_source = OlTextSetSource;
	text->text_update = OlTextUpdate;
	if (text->grow_mode & OL_GROW_HORIZONTAL)
	{	
		text->wrap_mode = FALSE ;
	};
	if (!text->wrap_mode)
	{ 
		text->wrap_form = OL_SOURCE_FORM ;
		text->wrap_break = OL_WRAP_ANY ;
	};


#ifdef GLSDEBUG
	fprintf (stderr, "%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %x\n"
	    , "  wrap mode", text->wrap_mode
	    , "  wrap form", text->wrap_form
	    , " wrap break", text->wrap_break
	    , "scroll mode", text->scroll_mode
	    , "  grow mode", text->grow_mode
	    , "    options", text->options
	    ) ;
#endif 


}	/*  Initialize  */


/*
 *************************************************************************
 * 
 *  InitializeHook - This procedure create the source for the Text
 *	widget.  Subresources are initialized using this mechanism.
 * 
 ****************************procedure*header*****************************
 */
static void
InitializeHook(widget, args, num_args)
	Widget widget;
	ArgList args;
	Cardinal *num_args;
{ 
	TextPaneWidget ctx = (TextPaneWidget)widget;
	register TextPanePart *text = &(ctx->text);

	text->sink = OlAsciiSinkCreate(widget, args, *num_args);

	if (text->srctype == OL_STRING_SOURCE)
		text->source = OlStringSourceCreate(widget, args, *num_args);
	else if (text->srctype == OL_DISK_SOURCE)
		text->source = OlDiskSourceCreate(widget, args , *num_args);
	else if (text->srctype != OL_PROG_DEFINED_SOURCE)
	{
			OlVaDisplayWarningMsg(	(Display *)NULL,
						OleNfileTextPane,
						OleTmsg5,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg5);

		text->source = OlStringSourceCreate(widget, args, *num_args);
	};


	ForceBuildLineTable((TextPaneWidget)widget);
	_OlPtBuild((TextPaneWidget)widget, (OlTextPosition)0);

	/*
	 *  Set up the slider max on the vertical scrollbar (if there
	 *  is one).
	 */
	if (text->verticalSB)
	  _OlUpdateVerticalSB(ctx);

	if (text->lt.lines > 1)  {
		/*  
		 *  The Horizontal scroll mode is forced off if the
		 *  TextPaneWidget is more than one line.
		 */
		text->scroll_state &= ~OL_AUTO_SCROLL_HORIZONTAL ;
	}

/* SIZE_FIX
	CheckResizeOrOverflow(ctx);
*/

}	/*  InitializeHook  */


/*
 *************************************************************************
 * 
 *  ProcessExposeRegion - This routine processes all "expose region"
 *	XEvents. In general, its job is to the best job at minimal
 *	re-paint of the text, displayed in the window, that it can.
 * 
 ****************************procedure*header*****************************
 */
static void
ProcessExposeRegion(w, event, region)
	Widget w;
	XEvent *event;
  	Region region;
{
	TextPaneWidget ctx = (TextPaneWidget) w;
	OlTextPosition pos1, pos2, resultend;
	int line;
	int x;
	int y;
	int width;
	int height;
	OlLineTableEntryPtr info;


	if (region != (Region)NULL)
	  {
	    XRectangle rect;


	    XClipBox(region, &rect);
	    x = rect.x;
	    y = rect.y;
	    width = rect.width;
	    height = rect.height;
	  }
	else
	  switch (event->type)
	    {
	    case Expose:
	      x = event->xexpose.x;
	      y = event->xexpose.y;
	      width = event->xexpose.width;
	      height = event->xexpose.height;
	      break;
	    case GraphicsExpose:
	      x = event->xgraphicsexpose.x;
	      y = event->xgraphicsexpose.y;
	      width = event->xgraphicsexpose.width;
	      height = event->xgraphicsexpose.height;
	      break;
	    case NoExpose:
	  default:
	    return;
	  }
	
	_XtTextPrepareToUpdate(ctx);
	if (x < (Position)ctx->text.leftmargin) /* stomp on caret tracks */
		(*(ctx->text.sink->clearToBackground))(ctx, x, y, width, height);
	/* figure out starting line that was exposed */
	line = LineForPosition(ctx, PositionForXY(ctx, x, y));
	while (line < ctx->text.lt.lines && ctx->text.lt.info[line + 1].y < y)
		line++;
	while (line < ctx->text.lt.lines) {
		info = &(ctx->text.lt.info[line]);
		if (info->y >= y + height)
			break;
		(*(ctx->text.sink->resolve))(ctx, 
		    info->position, info->x,
		    x - info->x, &pos1, &resultend);
		(*(ctx->text.sink->resolve))(ctx, 
		    info->position, info->x,
		    x + width - info->x, &pos2, 
		    &resultend);
		pos2 = (*(ctx->text.source->scan))(ctx->text.source, pos2,
		    OlstPositions, OlsdRight, 1, TRUE);
		_XtTextNeedsUpdating(ctx, pos1, pos2);
		line++;
	}
	_XtTextExecuteUpdate(ctx);
}	/*  ProcessExposeRegion  */



/*
 *************************************************************************
 * 
 *  Realize - This procedure creates the window for the Text widget, and
 *	defines the cursor.
 * 
 ****************************procedure*header*****************************
 */
static void
Realize(ctx, valueMask, attributes )
	TextPaneWidget		ctx;
	Mask *			valueMask;
	XSetWindowAttributes *	attributes;
{
	register TextPanePart *tPart = &(ctx->text);

	*valueMask |= CWBitGravity;
	attributes->bit_gravity = NorthWestGravity;

	XtCreateWindow((Widget)ctx,
		InputOutput,
		(Visual *)CopyFromParent,
		*valueMask,
		attributes);

	XDefineCursor(XtDisplay(ctx), XtWindow(ctx),
		XCreateFontCursor(XtDisplay(ctx), XC_left_ptr));
	tPart->update_flag = TRUE;

}	/*  Realize  */


/****************************function*header*****************************
 *  RegisterFocus - return widget id to register on Shell
 */
static Widget
RegisterFocus(w)
    Widget w;
{
    return (XtParent(XtParent(w)));
}


/*
 *************************************************************************
 * 
 *  Resize - 
 * 
 ****************************procedure*header*****************************
 */
static void
Resize(w)
	Widget          w;
{
	TextPaneWidget ctx = (TextPaneWidget) w;
	TextPanePart   *tp ;
	Dimension width, height ;
	Boolean          realized = XtIsRealized(w);

	tp = &(ctx->text) ;
	width = ctx->core.width ;
	if ((width < tp->prevW)  && (tp->grow_mode & OL_GROW_HORIZONTAL))
		tp->grow_state |= OL_GROW_HORIZONTAL ;
	height = ctx->core.height ;
	if ((height < tp->prevH)  && (tp->grow_mode & OL_GROW_VERTICAL))
		tp->grow_state |= OL_GROW_VERTICAL ;

	if (realized) {
		_XtTextPrepareToUpdate(ctx);
		DisplayAllText(ctx);
        }

	ForceBuildLineTable(ctx);
	_OlPtBuild(ctx, (OlTextPosition)0);


	if (tp->lt.lines > 1)  {
		/*
	 *  the Horizontal scrolling only works for TextPaneWidgets 
	 *  with one line.  So, if the widget just grew to be 
	 *  greater than one line long, turn the Horizontal scroll
	 *  bit off in the scroll_state.
	 */
		tp->scroll_state &= ~OL_AUTO_SCROLL_HORIZONTAL ;
	}
	else {
		/*
	 *  This turns the Horizontal scroll state on if the user
	 *  set it with a SetValues.
	 */
		tp->scroll_state |=  tp->scroll_mode & OL_AUTO_SCROLL_HORIZONTAL ;
	}

	if (realized) _XtTextExecuteUpdate(ctx);
}	/*  Resize  */


static int		background_changed = FALSE;
/*
 *************************************************************************
 * 
 *  SetValues - 
 * 
 ****************************procedure*header*****************************
 */
static Boolean
SetValues(current, request, new, args, num_args)
	Widget		current;
	Widget		request;
	Widget		new;
	ArgList		args;
	Cardinal *	num_args;
{
	TextPaneWidget oldtw = (TextPaneWidget) current;
	TextPaneWidget newtw = (TextPaneWidget) new;
	TextPanePart *oldtext = &(oldtw->text);
	TextPanePart *newtext = &(newtw->text);
	Boolean realized = XtIsRealized(current);

	background_changed =
		new->core.background_pixel != current->core.background_pixel;

	/*
	 *  Setting the background_pixel in the old widget is
	 *  necessary because the SetVaulesHook is only passed
	 *  the old widget and needs the information to create
	 *  new GCs.
	 */
	oldtw->core.background_pixel = newtw->core.background_pixel;

	if ((Dimension)(newtext->leftmargin + newtext->rightmargin) 
						> newtw->core.width)  {
			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg4,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg4,
						XtNleftMargin,
						newtext->leftmargin,
						XtNrightMargin,
						newtext->rightmargin);

		newtext->leftmargin = (Dimension) 2;
		newtext->rightmargin = (Dimension) 2;
		}
	if ((Dimension)(newtext->topmargin + newtext->bottommargin) 
						> newtw->core.height)  {
			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg4,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg4,
						XtNtopMargin,
						newtext->topmargin,
						XtNbottomMargin,
						newtext->bottommargin);

		newtext->topmargin = (Dimension) 2;
		newtext->bottommargin = (Dimension) 2;
		}

	if (realized) _XtTextPrepareToUpdate(oldtw);

	if (oldtext->source != newtext->source) {
		ForceBuildLineTable(oldtw);
		if ((oldtext->s.left == newtext->s.left) &&
		    (oldtext->s.right == newtext->s.right)) {
			newtext->s.left = (OlTextPosition) 0;
			newtext->s.right = (OlTextPosition) 0;
		}
	}

	if (oldtext->insertPos != newtext->insertPos)
		oldtext->showposition = TRUE;

	if ((oldtext->s.left != newtext->s.left) ||
	    (oldtext->s.right != newtext->s.right)) {
		if (newtext->s.left > newtext->s.right) {
			OlTextPosition temp = newtext->s.right;
			newtext->s.right = newtext->s.left;
			newtext->s.left = temp;
		}
	}


	if (oldtext->text_clear_buffer != newtext->text_clear_buffer)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextClearBuffer);

		newtext->text_clear_buffer = oldtext->text_clear_buffer;
	}

	if (oldtext->text_copy_buffer != newtext->text_copy_buffer)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextCopyBuffer);

		newtext->text_copy_buffer = oldtext->text_copy_buffer;
	}

	if (oldtext->text_get_insert_point != newtext->text_get_insert_point)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextGetInsertPoint);

		newtext->text_get_insert_point = oldtext->text_get_insert_point;
	}

	if (oldtext->text_get_last_pos != newtext->text_get_last_pos)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextGetLastPos);

		newtext->text_get_last_pos = oldtext->text_get_last_pos;
	}

	if (oldtext->text_insert != newtext->text_insert)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextInsert);

		newtext->text_insert = oldtext->text_insert;
	}

	if (oldtext->text_read_sub_str != newtext->text_read_sub_str)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextReadSubStr);

		newtext->text_read_sub_str = oldtext->text_read_sub_str;
	}

	if (oldtext->text_redraw != newtext->text_redraw)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextRedraw);

		newtext->text_redraw = oldtext->text_redraw;
	}

	if (oldtext->text_replace != newtext->text_replace)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextReplace);

		newtext->text_replace = oldtext->text_replace;
	}

	if (oldtext->text_set_insert_point != newtext->text_set_insert_point) {
			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextSetInsertPoint);

		newtext->text_set_insert_point = oldtext->text_set_insert_point;
	}

	if (oldtext->text_set_source != newtext->text_set_source)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextSetSource);

		newtext->text_set_source = oldtext->text_set_source;
	}

	if (oldtext->text_update != newtext->text_update)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg6,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg6,
						XtNtextUpdate);

		newtext->text_update = oldtext->text_update;
	}
	
	if (newtext->srctype != oldtext->srctype)  {
		if (oldtext->srctype == OL_DISK_SOURCE)  {
			OlDiskSourceDestroy(oldtext->source);
			oldtext->string = (char *) NULL;
			}
		else if (oldtext->srctype == OL_STRING_SOURCE)  {
			OlStringSourceDestroy(oldtext->source);
			oldtext->file = (char *) NULL;
			}
		}

	if (newtext->file != oldtext->file)  {
		OlTextSource *ts = NULL;
		Arg args[2];

		if (oldtext->srctype == OL_DISK_SOURCE)
			OlDiskSourceDestroy(oldtext->source);

		if (newtext->srctype == OL_DISK_SOURCE)  {
			XtSetArg(args[0], XtNfile, newtext->file);
			ts = OlDiskSourceCreate(newtw, args, 1);
			OlTextSetSource(newtw, ts, 0);
			}
		}

	if (newtext->string != oldtext->string)  {
		OlTextSource *ts = NULL;
		StringSourcePtr old_data = (StringSourcePtr) NULL;
		OlTextPosition old_max_size;
		int old_max_size_flag;
		Arg args[2];

		if (oldtext->srctype == OL_STRING_SOURCE) {
			old_data = (StringSourcePtr)oldtext->source->data;
			old_max_size		= old_data->max_size;
			old_max_size_flag	= old_data->max_size_flag;
			OlStringSourceDestroy(oldtext->source);
		}

		if (newtext->srctype == OL_STRING_SOURCE)  {
			XtSetArg(args[0], XtNstring, newtext->string);
			ts = OlStringSourceCreate(newtw, args, 1);
			if (((StringSourcePtr)ts->data)->max_size ==
			    MAGICVALUE && old_data != (StringSourcePtr)NULL) {
				((StringSourcePtr)ts->data)->max_size =
					old_max_size;
			    	((StringSourcePtr)ts->data)->max_size_flag =
					old_max_size_flag;
	  		}
			OlTextSetSource(newtw, ts, 0);
			}
		}

	if (newtext->lt.top != oldtext->lt.top)  {
		if (newtext->lt.top < 0)  {

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNfileTextPane,
						OleTmsg3,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg3,
						XtNdisplayPosition,
						newtext->lt.top);

			newtext->lt.top = oldtext->lt.top;
			}
		}
	/* ||| This may be the best way to do this, as some optimizations
     *     can occur here that may be harder if we let XtSetValues
     *     call our expose proc.
     */

	return ( FALSE );
}	/*  SetValues  */



/*
 *************************************************************************
 * 
 *  SetValuesHook - This procedure is used to set values of subresources
 *	of the different sources.
 * 
 ****************************procedure*header*****************************
 */
static Boolean
SetValuesHook(widget, args, num_args)
	Widget widget;
	ArgList args;
	Cardinal *num_args;
{
	TextPaneWidget ctx = (TextPaneWidget)widget;
	OlTextSource *source = ctx->text.source;
	OlTextSink   *sink   = ctx->text.sink;
	Boolean realized = XtIsRealized(widget);
	StringSourcePtr data = (StringSourcePtr)source->data;
	OlTextPosition old_max_size;
	int old_max_size_flag;		
        Boolean is_string_source = (ctx->text.srctype == OL_STRING_SOURCE);

	if (realized) _XtTextPrepareToUpdate(ctx);

	/* This is ugly, but need to know if user set initial_string */
	if (is_string_source)
		data->initial_string = NULL;

	

	XtSetSubvalues(source->data, source->resources,
	    source->resource_num, args, *num_args);

	XtSetSubvalues(sink->data, sink->resources,
	    sink->resource_num, args, *num_args);

	if (is_string_source) {
		old_max_size = data->max_size;
		old_max_size_flag = data->max_size_flag;
	}

	(*(source->check_data))(source);

	if (is_string_source && data->max_size == MAGICVALUE) {
		data->max_size = old_max_size;
		data->max_size_flag = old_max_size_flag;
	}

	(*(sink->check_data))(sink);

	ForceBuildLineTable(ctx);
	_OlPtBuild(ctx, (OlTextPosition)0);

	if (realized && !background_changed) {
		DisplayAllText(ctx);
		_XtTextExecuteUpdate(ctx);
	}

	return( FALSE );
}	/*  SetValuesHook  */


/*
 *************************************************************************
 * 
 *  TextDestroy - 
 * 
 ****************************procedure*header*****************************
 */
static void
TextDestroy(ctx)
	TextPaneWidget ctx;
{
	Atom actual_type_return;
	int actual_format_return;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	Window *prop_return;
	Widget p;
	int get_return;

	(*(ctx->text.source->destroy))(ctx->text.source);
	(*(ctx->text.sink->destroy))(ctx->text.sink);
	XtFree((char *)ctx->text.updateFrom);
	XtFree((char *)ctx->text.updateTo);
	XtFree((char *)ctx->text.lt.info);
	_OlPtDestroy(&(ctx->text.pt));

	XtRemoveAllCallbacks((Widget)ctx, XtNmark);
	XtRemoveAllCallbacks((Widget)ctx, XtNmotionVerification);
	XtRemoveAllCallbacks((Widget)ctx, XtNmodifyVerification);
	XtRemoveAllCallbacks((Widget)ctx, XtNleaveVerification);

	XtReleaseGC((Widget)ctx, ctx->text.gc);

	if (XGetSelectionOwner(XtDisplay(ctx), XA_CLIPBOARD(XtDisplay(ctx))) == XtWindow(ctx))  {
		/*
		 *  We own the CLIPBOARD contents, so give up ownership
		 *  of the CLIPBOARD and free the contents.
		 */

		XtFree(ctx->text.clip_contents);

		XSetSelectionOwner(XtDisplay(ctx),
		    XA_CLIPBOARD(XtDisplay(ctx)),
		    None,		/* Owner */
		ctx->text.time);
	}

}	/*  TextDestroy  */


/*
 *************************************************************************
 * 
 *  _OlTextCopySelection - This procdeure calls the function registered
 *	for copying a sub-string to return a copy of the current
 *	selection.
 * 
 ****************************procedure*header*****************************
 */
static unsigned char *
_OlTextCopySelection(ctx)
	TextPaneWidget ctx;
{
	return( _OlTextCopySubString(ctx, ctx->text.s.left, ctx->text.s.right));
}	/*  _OlTextCopySelection  */


/*
 *************************************************************************
 * 
 *  _OlTextCopySubString - 
 * 
 ****************************procedure*header*****************************
 */
unsigned char *
_OlTextCopySubString(ctx, left, right)
	TextPaneWidget ctx;
	OlTextPosition left, right;
{
	unsigned char   *result, *tempResult;
	int length, resultLength;
	OlTextBlock text;
	OlTextPosition end, nend;
	int   dummy;

	resultLength = right - left + 10;	/* Bogus? %%% */
	result = (unsigned char *)XtMalloc((unsigned) resultLength);

	length = _OlTextSubString(ctx,
			left, right, result, resultLength, &dummy);

	result[length] = 0;
	return result;
}	/*  _OlTextCopySubString  */


/*
 *************************************************************************
 * 
 *  _OlTextRedraw - This procedure will redraw the text widget.
 * 
 ****************************procedure*header*****************************
 */
static void
_OlTextRedraw (ctx)
	TextPaneWidget ctx;
{
	_XtTextPrepareToUpdate(ctx);
	ForceBuildLineTable(ctx);
	DisplayAllText(ctx);
	_XtTextExecuteUpdate(ctx);
}	/*  _OlTextRedraw  */


/*
 *************************************************************************
 * 
 *  _OlTextReplace - This procedure replaces the text between the starting
 *	position and ending position given with the text.  It will call
 *	the verify callback if the verify argument is TRUE.
 * 
 ****************************procedure*header*****************************
 */
static int
_OlTextReplace(w, startPos, endPos, text, verify)
	Widget	    w;
	OlTextPosition  startPos, endPos;
	OlTextBlock     *text;
	Boolean         verify;
{
	TextPaneWidget ctx = (TextPaneWidget) w;
	int result;

	_XtTextPrepareToUpdate(ctx);
	result = ReplaceText(ctx, startPos, endPos, text, verify);
	_XtTextExecuteUpdate(ctx);
	return result;
}	/*  _OlTextReplace  */


/*
 *************************************************************************
 * 
 *  _OlTextSetSelection - This procedure set the selectio to start at
 *	the left postion given and end at the right postion given.
 * 
 ****************************procedure*header*****************************
 */
static void
_OlTextSetSelection(ctx, left, right)
	TextPaneWidget ctx;
	OlTextPosition left, right;
{
	_XtTextPrepareToUpdate(ctx);
	_XtTextSetNewSelection(ctx, left, right);
	_XtTextExecuteUpdate(ctx);
}	/*  _OlTextSetSelection  */


/*
 *************************************************************************
 * 
 *  _OlTextUnsetSelection - this funtion sets the selection to start at
 *	zero and end at zero, thus "unsetting" the selection.
 * 
 ****************************procedure*header*****************************
 */
static void
_OlTextUnsetSelection(w)
	Widget	    w;
{
	TextPaneWidget ctx = (TextPaneWidget) w;

	_XtTextPrepareToUpdate(ctx);
	_XtTextSetNewSelection(ctx, zeroPosition, zeroPosition);
	_XtTextExecuteUpdate(ctx);
}	/*  _OlTextUnsetSelection  */


/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * 
 *  CopySelection - 
 * 
 ****************************procedure*header*****************************
 */
static void
CopySelection(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	unsigned char *ptr;

	StartAction(ctx, event);
	if (ctx->text.s.left == ctx->text.s.right)  {
		(void) _OlBeepDisplay((Widget)ctx, 1);
	}
	else {
		ptr = _OlTextCopySubString (ctx,
		    ctx->text.s.left, ctx->text.s.right);
		PasteTextOnClipBoard(ctx, ptr);
		_XtTextSetNewSelection(ctx,
		    ctx->text.insertPos, ctx->text.insertPos);
	}
	EndAction(ctx);
}	/*  CopySelection  */


/*
 *************************************************************************
 * 
 *  DeleteBackwardChar - 
 * 
 ****************************procedure*header*****************************
 */
static void
DeleteBackwardChar(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition next;

	StartAction(ctx, event);
	next = (*(ctx->text.source->scan))(
	    ctx->text.source, ctx->text.insertPos, OlstPositions, 
	    OlsdLeft, 1, TRUE);
	DeleteOrKill(ctx, next, ctx->text.insertPos, FALSE);
	EndAction(ctx);
}	/*  DeleteBackwardChar  */


/*
 *************************************************************************
 * 
 *  DeleteBackwardWord - 
 * 
 ****************************procedure*header*****************************
 */
static void
DeleteBackwardWord(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition next;

	StartAction(ctx, event);
	next = NextPosition(ctx, ctx->text.insertPos, OlstWhiteSpace, OlsdLeft);
	DeleteOrKill(ctx, next, ctx->text.insertPos, FALSE);
	EndAction(ctx);
}	/*  DeleteBackwardWord  */


/*
 *************************************************************************
 * 
 *  DeleteCurrentSelection - 
 * 
 ****************************procedure*header*****************************
 */
static void
DeleteCurrentSelection(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	DeleteOrKill(ctx, ctx->text.s.left, ctx->text.s.right, FALSE);
	EndAction(ctx);
}	/*  DeleteCurrentSelection  */


/*
 *************************************************************************
 * 
 *  DeleteForwardChar - 
 * 
 ****************************procedure*header*****************************
 */
static void
DeleteForwardChar(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition next;

	StartAction(ctx, event);
	next = (*(ctx->text.source->scan))(
	    ctx->text.source, ctx->text.insertPos, OlstPositions, 
	    OlsdRight, 1, TRUE);
	DeleteOrKill(ctx, ctx->text.insertPos, next, FALSE);
	EndAction(ctx);
}	/*  DeleteForwardChar  */


/*
 *************************************************************************
 * 
 *  DeleteForwardWord - 
 * 
 ****************************procedure*header*****************************
 */
static void
DeleteForwardWord(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition next;

	StartAction(ctx, event);
	next = NextPosition(ctx,
		ctx->text.insertPos,
		OlstWhiteSpace,
		OlsdRight);
	DeleteOrKill(ctx, ctx->text.insertPos, next, FALSE);
	EndAction(ctx);
}	/*  DeleteForwardWord  */


/*
 *************************************************************************
 * 
 *  Execute - 
 * 
 ****************************procedure*header*****************************
 */
static void
Execute(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{

	XtCallCallbacks((Widget)ctx, XtNexecute, NULL);

}	/*  Execute  */


/*
 *************************************************************************
 * 
 *  ExtendAdjust - 
 * 
 ****************************procedure*header*****************************
 */
static void
ExtendAdjust(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	AlterSelection(ctx, OlsmTextExtend, OlactionAdjust);
	EndAction(ctx);
}	/*  ExtendAdjust  */


/*
 *************************************************************************
 * 
 *  ExtendEnd - 
 * 
 ****************************procedure*header*****************************
 */
static void
ExtendEnd(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	AlterSelection(ctx, OlsmTextExtend, OlactionEnd);
	EndAction(ctx);
}	/*  ExtendEnd  */


/*
 *************************************************************************
 * 
 *  ExtendStart - 
 * 
 ****************************procedure*header*****************************
 */
static void
ExtendStart(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	AlterSelection(ctx, OlsmTextExtend, OlactionStart);
	EndAction(ctx);
}	/*  ExtendStart  */

/*
 *************************************************************************
 * 
 *  InsertChar - 
 * 
 ****************************procedure*header*****************************
 */
static void
InsertChar(w, ve)
	Widget		w;
	OlVirtualEvent	ve;
{
	TextPaneWidget	ctx = (TextPaneWidget)w;
	XEvent *event = ve->xevent;
	unsigned char strbuf[STRBUFSIZE];
	OlTextPosition left, right;
	int     keycode;
	int result;
	Boolean	is_selection;
	OlTextBlock text;

		/* Even though this logic is incorrect, we do it since
		 * it's consistent with the old behavior, i.e.,
		 * the old behavior was wrong; but, this widget is
		 * obsolete, therefore, we don't bug fix it.
		 */

	if (ve->virtual_name == OL_PREVFIELD ||
	    ve->virtual_name == OL_NEXTFIELD ||
	    ve->virtual_name == OL_COPY ||
	    ve->virtual_name == OL_CUT)
	{
		return;
	} else {
		ve->consumed = TRUE;
	}

	text.length = XLookupString ((XKeyEvent *)event, (char *)strbuf,
			STRBUFSIZE, (KeySym *)&keycode, &compose_status);
	if (text.length==0) return;
	StartAction(ctx, event);
	text.ptr = &strbuf[0];

	text.firstPos = 0;
	
	is_selection = ctx->text.s.left != ctx->text.s.right;
	if (is_selection) {
		left = ctx->text.s.left;
		right = ctx->text.s.right;

	}
	else
		left = right = ctx->text.insertPos;

	if (result = ReplaceText(ctx, left, right, &text, TRUE)) {
		if (result != OleditReject)
			(void)_OlBeepDisplay((Widget)ctx, 1);
		EndAction(ctx);
		return;
	}

	_OlSetCursorPos(ctx, (*(ctx->text.source->scan))(ctx->text.source,
					left,
					OlstPositions,
					OlsdRight,
					text.length,
					TRUE));

	if (is_selection)
		_OlTextUnsetSelection(ctx);

	EndAction(ctx);
}	/*  InsertChar  */


/*
 *************************************************************************
 * 
 *  InsertNewLine - 
 * 
 ****************************procedure*header*****************************
 */
static int
InsertNewLine(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition next;
	int result;

	StartAction(ctx, event);
	if (InsertNewLineAndBackupInternal(ctx)) {
		EndAction(ctx);
		return(OleditError);
	}
	next = (*(ctx->text.source->scan))(ctx->text.source,
		ctx->text.insertPos, OlstPositions, OlsdRight, 1, TRUE);
	_XtTextSetNewSelection(ctx,
		(OlTextPosition) next,
		(OlTextPosition) next);
	ctx->text.showposition = TRUE;
	result = _OlSetCursorPos(ctx, next);
	EndAction(ctx);
	return(result);
}	/*  InsertNewLine  */


/*
 *************************************************************************
 * 
 *  InsertNewLineAndBackup - 
 * 
 ****************************procedure*header*****************************
 */
static void
InsertNewLineAndBackup(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	(void) InsertNewLineAndBackupInternal(ctx);
	EndAction(ctx);
}	/*  InsertNewLineAndBackup  */


/*
 *************************************************************************
 * 
 *  InsertNewLineAndIndent - 
 * 
 ****************************procedure*header*****************************
 */
static void
InsertNewLineAndIndent(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextBlock text;
	OlTextPosition pos1, pos2;
	int result;

	StartAction(ctx, event);
	pos1 = (*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos, 
	    OlstEOL, OlsdLeft, 1, FALSE);
	pos2 = (*(ctx->text.source->scan))(ctx->text.source, pos1, OlstEOL, 
	    OlsdLeft, 1, TRUE);
	pos2 = (*(ctx->text.source->scan))(ctx->text.source, pos2, OlstWhiteSpace, 
	    OlsdRight, 1, TRUE);
	text.ptr = _OlTextCopySubString(ctx, pos1, pos2);
	text.length = _OlStrlen(text.ptr);
	if (InsertNewLine(ctx, event)) return;
	if (result =
	    ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos,
	    &text, TRUE)) {
		if (result != OleditReject)
			(void) _OlBeepDisplay((Widget)ctx, 1);
		XtFree((char *)text.ptr);
		EndAction(ctx);
		return;
	}
	_OlSetCursorPos(ctx, (*(ctx->text.source->scan))
	    (ctx->text.source, ctx->text.insertPos, OlstPositions,
	    OlsdRight, text.length, TRUE));
	XtFree((char *)text.ptr);
	EndAction(ctx);
}	/*  InsertNewLineAndIndent  */


/*
 *************************************************************************
 * 
 *  KillBackwardWord - 
 * 
 ****************************procedure*header*****************************
 */
static void
KillBackwardWord(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition next;

	StartAction(ctx, event);
	next = NextPosition(ctx, ctx->text.insertPos, OlstWhiteSpace, OlsdLeft);
	DeleteOrKill(ctx, next, ctx->text.insertPos, TRUE);
	EndAction(ctx);
}	/*  KillBackwardWord  */


/*
 *************************************************************************
 * 
 *  KillCurrentSelection - 
 * 
 ****************************procedure*header*****************************
 */
static void
KillCurrentSelection(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	if (ctx->text.s.left == ctx->text.s.right)  {
		(void) _OlBeepDisplay((Widget)ctx, 1);
	}
	else
		DeleteOrKill(ctx, ctx->text.s.left, ctx->text.s.right, TRUE);
	EndAction(ctx);
}	/*  KillCurrentSelection  */


/*
 *************************************************************************
 * 
 *  KillForwardWord - 
 * 
 ****************************procedure*header*****************************
 */
static void
KillForwardWord(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition next;

	StartAction(ctx, event);
	next = NextPosition(ctx, ctx->text.insertPos, OlstWhiteSpace, OlsdRight);
	DeleteOrKill(ctx, ctx->text.insertPos, next, TRUE);
	EndAction(ctx);
}	/*  KillForwardWord  */


/*
 *************************************************************************
 * 
 *  DeleteToBeginningOfLine - 
 * 
 ****************************procedure*header*****************************
 */
static void
DeleteToBeginningOfLine(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	int     line;
	OlTextPosition beginning;
	StartAction(ctx, event);
	_XtTextShowPosition(ctx);
	line = LineForPosition(ctx, ctx->text.insertPos);
	beginning = ctx->text.lt.info[line].position;
	if (beginning != ctx->text.insertPos)
		DeleteOrKill(ctx, beginning, ctx->text.insertPos, FALSE);
	EndAction(ctx);
}	/*  DeleteToBeginningOfLine  */


/*
 *************************************************************************
 * 
 *  DeleteToEndOfLine - 
 * 
 ****************************procedure*header*****************************
 */
static void
DeleteToEndOfLine(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	int     line;
	OlTextPosition last, next, lastpos = GETLASTPOS(ctx);
	StartAction(ctx, event);
	_XtTextShowPosition(ctx);
	line = LineForPosition(ctx, ctx->text.insertPos);
	last = ctx->text.lt.info[line + 1].position;
	next = (*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos,
	    OlstEOL, OlsdRight, 1, FALSE);
	if (last > lastpos)
		last = lastpos;
	if (last > next && ctx->text.insertPos < next)
		last = next;
	DeleteOrKill(ctx, ctx->text.insertPos, last, FALSE);
	EndAction(ctx);
}	/*  DeleteToEndOfLine  */


/*
 *************************************************************************
 * 
 *  KillToEndOfParagraph - 
 * 
 ****************************procedure*header*****************************
 */
static void
KillToEndOfParagraph(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition next;

	StartAction(ctx, event);
	next = (*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos,
	    OlstEOL, OlsdRight, 1, FALSE);
	if (next == ctx->text.insertPos)
		next = (*(ctx->text.source->scan))(ctx->text.source, next, OlstEOL,
		    OlsdRight, 1, TRUE);
	DeleteOrKill(ctx, ctx->text.insertPos, next, TRUE);
	EndAction(ctx);
}	/*  KillToEndOfParagraph  */


/*
 *************************************************************************
 * 
 *  MoveBackwardChar - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveBackwardChar(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition position;

	StartAction(ctx, event);
	position = (*(ctx->text.source->scan)) (
			ctx->text.source,
			ctx->text.insertPos,
			OlstPositions,
			OlsdLeft,
			1,
			TRUE);
	(void) MovePosition(ctx, position);
	EndAction(ctx);

}	/*  MoveBackwardChar  */


/*
 *************************************************************************
 * 
 *  MoveBackwardParagraph - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveBackwardParagraph(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition position;

	StartAction(ctx, event);
	position = NextPosition(ctx, ctx->text.insertPos, OlstEOL, OlsdLeft);
	(void) MovePosition(ctx, position);
	EndAction(ctx);

}	/*  MoveBackwardParagraph  */


/*
 *************************************************************************
 * 
 *  MoveBackwardWord - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveBackwardWord(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition position;

	StartAction(ctx, event);
	position = 
	    NextPosition(ctx, ctx->text.insertPos, OlstWhiteSpace, OlsdLeft);
	(void) MovePosition(ctx, position);
	EndAction(ctx);

}	/*  MoveBackwardWord  */


/*
 *************************************************************************
 * 
 *  MoveBeginningOfFile - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveBeginningOfFile(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition position;

	StartAction(ctx, event);
	position = (*(ctx->text.source->scan)) (
			ctx->text.source,
			ctx->text.insertPos,
			OlstLast,
			OlsdLeft,
			1,
			TRUE);
	(void) MovePosition(ctx, position);
	EndAction(ctx);

}	/*  MoveBeginningOfFile  */


/*
 *************************************************************************
 * 
 *  MoveEndOfFile - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveEndOfFile(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition position;

	StartAction(ctx, event);
	position = (*(ctx->text.source->scan)) (
			ctx->text.source,
			ctx->text.insertPos,
			OlstLast,
			OlsdRight,
			1,
			TRUE);
	(void) MovePosition(ctx, position);
	EndAction(ctx);

}	/*  MoveEndOfFile  */


/*
 *************************************************************************
 * 
 *  MoveForwardChar - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveForwardChar(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition position;

	StartAction(ctx, event);
	position = (*(ctx->text.source->scan)) (
			ctx->text.source,
			ctx->text.insertPos,
			OlstPositions,
			OlsdRight,
			1,
			TRUE);
	(void) MovePosition(ctx, position);
	EndAction(ctx);

}	/*  MoveForwardChar  */


/*
 *************************************************************************
 * 
 *  MoveForwardParagraph - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveForwardParagraph(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition position;

	StartAction(ctx, event);
	position = NextPosition(ctx, ctx->text.insertPos, OlstEOL, OlsdRight);
	(void) MovePosition(ctx, position);
	EndAction(ctx);

}	/*  MoveForwardParagraph  */


/*
 *************************************************************************
 * 
 *  MoveForwardWord - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveForwardWord(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition position;

	StartAction(ctx, event);
	position = 
	    NextPosition(ctx, ctx->text.insertPos, OlstWhiteSpace, OlsdRight);
	(void) MovePosition(ctx, position);
	EndAction(ctx);

}	/*  MoveForwardWord  */


/*
 *************************************************************************
 * 
 *  MoveNextLine - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveNextLine(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	int     width, width2, height, line;
	OlTextPosition position, maxp, lastpos = GETLASTPOS(ctx);
	OlTextSink *sink = ctx->text.sink ;

	StartAction(ctx, event);
	_XtTextShowPosition(ctx); /* Needed if cursor goes off screen ??? */
	line = LineForPosition(ctx, ctx->text.insertPos);

	if (ctx->text.lt.info[line+1].position != (lastpos + 1)) {
		if ((line == ctx->text.lt.lines - 1) && (line > 0)) {
			_OlTextScroll(ctx, 1);
			line = LineForPosition(ctx, ctx->text.insertPos);
		}
		if (sink->LineLastPosition == ctx->text.insertPos)
			width = sink->LineLastWidth;
		else
			(*(ctx->text.sink->findDistance))
			    (ctx, ctx->text.lt.info[line].position, ctx->text.lt.info[line].x,
			    ctx->text.insertPos, &width, &position, &height);
		line++;
		if (ctx->text.lt.info[line].position > lastpos) {
			position = lastpos;
		}
		else {
			(*(ctx->text.sink->findPosition))(ctx,
			    ctx->text.lt.info[line].position, ctx->text.lt.info[line].x,
			    width, FALSE, &position, &width2, &height);
			maxp = (*(ctx->text.source->scan))(ctx->text.source,
			    ctx->text.lt.info[line+1].position,
			    OlstPositions, OlsdLeft, 1, TRUE);
			if (position > maxp)
				position = maxp;
		}
		if (MovePosition(ctx, position) == OleditDone) {
			sink->LineLastWidth = width;
			sink->LineLastPosition = position;
		}
	}
	EndAction(ctx);
}	/*  MoveNextLine  */


/*
 *************************************************************************
 * 
 *  MoveNextPage - 
 * 
 ****************************procedure*header*****************************
 */
void
MoveNextPage(ctx)
	TextPaneWidget ctx;
{
	OlLineTablePtr  lt = &(ctx->text.lt) ;
	int cur_line, line_lastpos;
	OlTextPosition line_offset, new_pos, lastpos = GETLASTPOS(ctx);

	if (lt->info[(lt->lines)].position != (lastpos + 1)) {
		cur_line = LineForPosition(ctx, ctx->text.insertPos);
		line_offset = (ctx->text.insertPos - lt->info[cur_line].position);
		_OlTextScroll(ctx, (int) max(1, lt->lines - 2));
		if (lt->info[cur_line].position > lastpos)
			cur_line = LineForPosition(ctx, lastpos);
		new_pos = (lt->info[cur_line].position + line_offset);
		new_pos = min(new_pos, lt->info[cur_line].drawPos);
		(void) MovePosition(ctx, new_pos);
	}
}	/*  MoveNextPage  */


/*
 *************************************************************************
 * 
 *  MovePosition - Move insert cursor to a new position and clear any
 *	existing selection.
 * 
 ****************************procedure*header*****************************
 */
static int
MovePosition (ctx, position)
	TextPaneWidget ctx;
	OlTextPosition position;
{
	ctx->text.lasttime = ctx->text.time;

	_XtTextSetNewSelection(ctx, position, position);

	ctx->text.s.type = OlselectPosition;

	return _OlSetCursorPos(ctx, position);

}	/*  MovePosition  */



/*
 *************************************************************************
 * 
 *  MovePreviousLine - 
 * 
 ****************************procedure*header*****************************
 */
static void
MovePreviousLine(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	int     width, width2, height, line;
	OlTextPosition position, maxp;
	OlTextSink *sink = ctx->text.sink ;

	StartAction(ctx, event);
	_XtTextShowPosition(ctx);
	line = LineForPosition(ctx, ctx->text.insertPos);
	if (line == 0) {
		_OlTextScroll(ctx, -1);
		line = LineForPosition(ctx, ctx->text.insertPos);
	}
	if (line > 0) {
		if (sink->LineLastPosition == ctx->text.insertPos)
			width = sink->LineLastWidth;
		else
			(*(ctx->text.sink->findDistance))(ctx,
			    ctx->text.lt.info[line].position, 
			    ctx->text.lt.info[line].x,
			    ctx->text.insertPos, &width, &position, &height);
		line--;
		(*(ctx->text.sink->findPosition))(ctx,
		    ctx->text.lt.info[line].position, ctx->text.lt.info[line].x,
		    width, FALSE, &position, &width2, &height);
		maxp = (*(ctx->text.source->scan))(ctx->text.source, 
		    ctx->text.lt.info[line+1].position,
		    OlstPositions, OlsdLeft, 1, TRUE);
		if (position > maxp)
			position = maxp;
		if (MovePosition(ctx, position) == OleditDone) {
			sink->LineLastWidth = width;
			sink->LineLastPosition = position;
		}
	}
	EndAction(ctx);
}	/*  MovePreviousLine  */


/*
 *************************************************************************
 * 
 *  MovePreviousPage - 
 * 
 ****************************procedure*header*****************************
 */
void
MovePreviousPage(ctx)
	TextPaneWidget ctx;
{ 
	OlLineTablePtr  lt = &(ctx->text.lt) ;
	int cur_line = LineForPosition(ctx, ctx->text.insertPos);
	OlTextPosition line_offset =
	    (ctx->text.insertPos - lt->info[cur_line].position);
	OlTextPosition new_pos;
	_OlTextScroll(ctx,(int)(-max(1, lt->lines - 2)));
	new_pos = (lt->info[cur_line].position + line_offset);
	(void) MovePosition(ctx, min(new_pos, lt->info[cur_line].drawPos));
}	/*  MovePreviousPage  */


/*
 *************************************************************************
 * 
 *  MoveToLineEnd - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveToLineEnd(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	int line;
	OlTextPosition next, lastpos = GETLASTPOS(ctx);
	StartAction(ctx, event);
	_XtTextShowPosition(ctx);
	line = LineForPosition(ctx, ctx->text.insertPos);
	next = ctx->text.lt.info[line+1].position;
	if (next > lastpos)
		next = lastpos;
	else
		next = (*(ctx->text.source->scan))
		    (ctx->text.source, next, OlstPositions, OlsdLeft, 1, TRUE);
	if (next <= ctx->text.lt.info[line].drawPos)
		next = ctx->text.lt.info[line].drawPos + 1 ;
	(void) MovePosition(ctx, next);
	EndAction(ctx);
}	/*  MoveToLineEnd  */


/*
 *************************************************************************
 * 
 *  MoveToLineStart - 
 * 
 ****************************procedure*header*****************************
 */
static void
MoveToLineStart(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	int line;
	StartAction(ctx, event);
	_XtTextShowPosition(ctx);
	line = LineForPosition(ctx, ctx->text.insertPos);
	(void) MovePosition(ctx, ctx->text.lt.info[line].position);
	EndAction(ctx);
}	/*  MoveToLineStart  */


/*
 *************************************************************************
 * 
 *  RedrawDisplay - 
 * 
 ****************************procedure*header*****************************
 */
static void
RedrawDisplay(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	ForceBuildLineTable(ctx);
	DisplayAllText(ctx);
	ClearWindow(ctx);
	EndAction(ctx);
}	/*  RedrawDisplay  */


/*
 *************************************************************************
 * 
 *  ScrollOneLineDown - 
 * 
 ****************************procedure*header*****************************
 */
static void
ScrollOneLineDown(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	_OlTextScroll(ctx, -1);
	EndAction(ctx);
}	/*  ScrollOneLineDown  */


/*
 *************************************************************************
 * 
 *  ScrollOneLineUp - 
 * 
 ****************************procedure*header*****************************
 */
static void
ScrollOneLineUp(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	_OlTextScroll(ctx, 1);
	EndAction(ctx);
}	/*  ScrollOneLineUp  */


/*
 *************************************************************************
 * 
 *  SelectAll - 
 * 
 ****************************procedure*header*****************************
 */
static void SelectAll(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	NewSelection(ctx, (OlTextPosition) 0, GETLASTPOS(ctx));
	EndAction(ctx);
}	/*  SelectAll  */


/*
 *************************************************************************
 * 
 *  SelectAdjust - 
 * 
 ****************************procedure*header*****************************
 */
static void SelectAdjust(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	AlterSelection(ctx, OlsmTextSelect, OlactionAdjust);
	EndAction(ctx);
}	/*  SelectAdjust  */


/*
 *************************************************************************
 * 
 *  SelectEnd - 
 * 
 ****************************procedure*header*****************************
 */
static void
SelectEnd(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	AlterSelection(ctx, OlsmTextSelect, OlactionEnd);
	EndAction(ctx);
}	/*  SelectEnd  */


/*
 *************************************************************************
 * 
 *  SelectWord - 
 * 
 ****************************procedure*header*****************************
 */
static void
SelectWord(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	OlTextPosition l, r;
	StartAction(ctx, event);
	l = (*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos, 
	    OlstWhiteSpace, OlsdLeft, 1, FALSE);
	r = (*(ctx->text.source->scan))(ctx->text.source, l, OlstWhiteSpace, 
	    OlsdRight, 1, FALSE);
	NewSelection(ctx, l, r);
	EndAction(ctx);
}	/*  SelectWord  */


/*
 *************************************************************************
 * 
 *  Stuff - 
 * 
 ****************************procedure*header*****************************
 */
static void
Stuff(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	StuffFromBuffer(ctx, 0);
}	/*  Stuff  */


/*
 *************************************************************************
 * 
 *  TextFocusOut - 
 * 
 ****************************procedure*header*****************************
 */
static void
TextFocusOut(w, ve)
	Widget	w;
	OlVirtualEvent	ve;
{
        /*
        ** The primitive-focus-out() action procedure may have determined
	** that we really didn't lose focus, so check first.
	** Note: we don't consume the event since we want our superclass
	** to use it also.
	*/
	if (HAS_FOCUS(w) == FALSE) {
		(void) VerifyLeave((TextPaneWidget)w, ve->xevent);

		/* Change the input cursor to the stippled diamond */
	}

}	/*  TextFocusOut  */


/*
 *************************************************************************
 * 
 *  UnKill - 
 * 
 ****************************procedure*header*****************************
 */
static void
UnKill(ctx, event)
	TextPaneWidget ctx;
	XEvent *event;
{
	StartAction(ctx, event);
	StuffFromBuffer(ctx, 1);

	EndAction(ctx);
}	/*  UnKill  */


/*
 *************************************************************************
 * 
 *  VerifyLeave - 
 * 
 ****************************procedure*header*****************************
 */
static Boolean
VerifyLeave(ctx,event)
	TextPaneWidget  ctx;
	XEvent            *event;
{
	OlTextVerifyCD  cbdata;

	StartAction(ctx, event);
	cbdata.operation = leaveVerify;
	cbdata.doit = TRUE;
	cbdata.currInsert = ctx->text.insertPos;
	cbdata.newInsert = ctx->text.insertPos;
	cbdata.startPos = ctx->text.insertPos;
	cbdata.endPos = ctx->text.insertPos;
	cbdata.text = NULL;
	cbdata.xevent = event;
	if (XtClass(XtParent(XtParent(ctx))) == textWidgetClass)
		XtCallCallbacks((Widget)XtParent(XtParent(ctx)),
			XtNleaveVerification, &cbdata);
	else
		XtCallCallbacks((Widget)ctx, XtNleaveVerification, &cbdata);
	ctx->text.insertPos = cbdata.newInsert;
	EndAction(ctx);
	return( cbdata.doit );
}	/*  VerifyLeave  */


/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * 
 *  _OlScrollText - 
 * 
 ****************************procedure*header*****************************
 */
void
_OlScrollText(ctx, relative_pos)
	TextPaneWidget ctx;
	int relative_pos;
{
	StartAction(ctx, NULL);
	_OlTextScroll(ctx, relative_pos);
	EndAction(ctx);
}	/*  _OlScrollText  */


/*
 *************************************************************************
 * 
 *  OlTextClearBuffer - 
 * 
 ****************************procedure*header*****************************
 */
static void
OlTextClearBuffer(w)
	Widget w;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {
			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextClearBuffer");

		return;
		}

	_XtTextPrepareToUpdate(tw);
	(*(tw->text.source->setLastPos))(tw->text.source, (OlTextPosition)0);
	tw->text.insertPos = 0;
	tw->text.s.left = 0;
	tw->text.s.right = 0;
	ForceBuildLineTable(tw);
	DisplayAllText(tw);
	_XtTextExecuteUpdate(tw);
}	/*  OlTextClearBuffer  */


/*
 *************************************************************************
 * 
 *  OlTextCopyBuffer - 
 * 
 ****************************procedure*header*****************************
 */
static unsigned char *
OlTextCopyBuffer(w)
	Widget w;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {
			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextCopyBuffer");
		return(NULL);
		}

        return (*((TextPaneClassRec *)(tw->core.widget_class))->textedit_class.copy_substring) ((TextPaneWidget)tw, 0, GETLASTPOS(tw));


}	/*  OlTextCopyBuffer  */


/*
 *************************************************************************
 * 
 *  OlTextCopySelection - 
 * 
 ****************************procedure*header*****************************
 */
static unsigned char *
OlTextCopySelection(w)
	Widget w;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextCopySelection");

		return(NULL);
		}

        return (*((TextPaneClassRec *)(tw->core.widget_class))->
			textedit_class.copy_selection) ((TextPaneWidget)tw);

}	/*  OlTextCopySelection  */


/*
 *************************************************************************
 * 
 *  OlTextGetInsertPos - 
 * 
 ****************************procedure*header*****************************
 */
static OlTextPosition
OlTextGetInsertPos(w)
	Widget w;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextGetInsertPos");

		return(NULL);
		}


	return(tw->text.insertPos );

}	/*  lTextGetInsertPos  */


/*
 *************************************************************************
 * 
 *  OlTextGetLastPos - 
 * 
 ****************************procedure*header*****************************
 */
static OlTextPosition
OlTextGetLastPos (w)
	Widget w;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextGetLastPos");

		return(NULL);
		}

	return( GETLASTPOS(tw) );
}	/*  OlTextGetLastPos  */


/*
 *************************************************************************
 * 
 *  OlTextGetSelectionPos - 
 * 
 ****************************procedure*header*****************************
 */
void
OlTextGetSelectionPos(w, left, right)
	Widget w;
	OlTextPosition *left, *right;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextGetSelectionPos");


		return;
		}


	*left = tw->text.s.left;
	*right = tw->text.s.right;
}	/*  OlTextGetSelectionPos  */


/*
 *************************************************************************
 * 
 *  OlTextInsert - 
 * 
 ****************************procedure*header*****************************
 */
static void
OlTextInsert(w, string)
	Widget w;
	unsigned char *string;
{
	OlTextBlock blk;
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextInsert");

		return;
		}


	blk.ptr = string;
	blk.length = _OlStrlen(string);
	blk.firstPos = (OlTextPosition) 0;

	_XtTextPrepareToUpdate(tw);
	if (ReplaceText(tw, tw->text.insertPos,
	    tw->text.insertPos, &blk, FALSE) == OleditDone) {
		tw->text.showposition = TRUE;
		tw->text.insertPos = tw->text.insertPos + blk.length;
	}
	_XtTextExecuteUpdate(tw);

}	/*  OlTextInsert  */


/*
 *************************************************************************
 * 
 *  OlTextReadSubString - 
 * 
 ****************************procedure*header*****************************
 */
static int
OlTextReadSubString( w, startpos, endpos, target, targetsize, targetused )
	Widget  w;
	OlTextPosition    startpos, endpos;
	unsigned char     *target;
	int               targetsize;
	int		*targetused;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextReadSubString");

		return(NULL);
		}

	return( _OlTextSubString(tw, startpos, endpos, target, targetsize, targetused) );

}	/*  OlTextReadSubString  */


/*
 *************************************************************************
 * 
 *  OlTextRedraw - 
 * 
 ****************************procedure*header*****************************
 */
static void
OlTextRedraw(w)
	Widget w;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextRedraw");

		return;
		}

	(*((TextPaneClassRec *)(tw->core.widget_class))->textedit_class.redraw_text)
	    ((Widget)tw);
}	/*  OlTextRedraw  */


/*
 *************************************************************************
 * 
 *  OlTextReplace - 
 * 
 ****************************procedure*header*****************************
 */
static int
OlTextReplace(w, startPos, endPos, string)
	Widget w;
	OlTextPosition   startPos, endPos;
	unsigned char    *string;
{
	OlTextBlock blk;
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextReplace");

		return(NULL);
		}


	blk.ptr = string;
	blk.length = _OlStrlen(string);
	blk.firstPos = (OlTextPosition)0;

	return (*((TextPaneClassRec *)(tw->core.widget_class))->
	textedit_class.replace_text) (tw, startPos, endPos, &blk, FALSE);
}	/*  OlTextReplace  */


/*
 *************************************************************************
 * 
 *  OlTextSetInsertPos - 
 * 
 ****************************procedure*header*****************************
 */
static void
OlTextSetInsertPos(w, position)
	Widget w;
	OlTextPosition position;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextSetInsertPos");

		return;
		}

	_XtTextPrepareToUpdate(tw);
	tw->text.insertPos = position;
	tw->text.showposition = TRUE;
	_XtTextExecuteUpdate(tw);
}	/*  OlTextSetInsertPos  */


/*
 *************************************************************************
 * 
 *  OlTextSetSelection - 
 * 
 ****************************procedure*header*****************************
 */
void
OlTextSetSelection(w, left, right)
	Widget  w;
	OlTextPosition    left, right;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextSetSelection");

		return;
		}

	(*((TextPaneClassRec *)(tw->core.widget_class))->textedit_class.set_selection)
	    (tw, left, right);
}	/*  OlTextSetSelection  */


/*
 *************************************************************************
 * 
 *  OlTextSetSource - 
 * 
 ****************************procedure*header*****************************
 */

static void
OlTextSetSource(w, source, startpos)
	Widget w;
	OlTextSourcePtr source;
	OlTextPosition startpos;
{
	TextPaneWidget tw;
	tw = (TextPaneWidget) w;

	tw->text.source = source;
	tw->text.lt.top = startpos;
	tw->text.s.left = tw->text.s.right = 0;
	tw->text.insertPos = startpos;

}



/*
 *************************************************************************
 * 
 *  OlTextUnsetSelection - 
 * 
 ****************************procedure*header*****************************
 */
void
OlTextUnsetSelection(w)
	Widget w;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextUnsetSelection");

		return;
		}

	(*((TextPaneClassRec *)(tw->core.widget_class))->textedit_class.unset_selection)
	    ((Widget)tw);
}	/*  OlTextUnsetSelection  */


/*
 *************************************************************************
 * 
 *  OlTextUpdate - 
 * 
 ****************************procedure*header*****************************
 */
static void
OlTextUpdate( w, status )
	Widget w;
	Boolean status;
{
	TextPaneWidget tw;

	/*
	 *  The widget passed in should be a TextPaneWidget or
	 *  a TextWidget.  We must find out which to call the
	 *  routine stored in the TextPaneWidget
	 */
	if (XtIsSubclass(w, textPaneWidgetClass))
		tw = (TextPaneWidget) w;
	else if (XtIsSubclass(w, textWidgetClass))
		tw = (TextPaneWidget)((TextWidget) w)->text.bb_child;
	else  {

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileTextPane,
						OleTmsg7,
						OleCOlToolkitWarning,
						OleMfileTextPane_msg7,
						"OlTextUpdate");

		return;
		}

	tw->text.update_flag = status;
	if (status) {
		_XtTextExecuteUpdate(tw);
	}
	else {
		tw->text.numranges = 0;
		tw->text.showposition = FALSE;
		tw->text.oldinsert = tw->text.insertPos;
		if ( XtIsRealized((Widget)tw) )
		   InsertCursor(tw);
	}
}	/*  OlTextUpdate  */


/*
 *************************************************************************
 * 
 *  _XtTextSetNewSelection - 
 * 
 ****************************procedure*header*****************************
 */
static void
_XtTextSetNewSelection(ctx, left, right)
	TextPaneWidget ctx;
	OlTextPosition left, right;
{
	OlTextPosition pos;

	if (left < ctx->text.s.left) {
		pos = min(right, ctx->text.s.left);
		_XtTextNeedsUpdating(ctx, left, pos);
	}
	if (left > ctx->text.s.left) {
		pos = min(left, ctx->text.s.right);
		_XtTextNeedsUpdating(ctx, ctx->text.s.left, pos);
	}
	if (right < ctx->text.s.right) {
		pos = max(right, ctx->text.s.left);
		_XtTextNeedsUpdating(ctx, pos, ctx->text.s.right);
	}
	if (right > ctx->text.s.right) {
		pos = max(left, ctx->text.s.right);
		_XtTextNeedsUpdating(ctx, pos, right);
	}

	ctx->text.s.left = left;
	ctx->text.s.right = right;
}	/*  _XtSetNewSelection  */


/*
 *************************************************************************
 * 
 *  _OlTextSubString - This method deletes the text from startPos to
 *	endPos in a source and then inserts, at startPos, the text that
 *	was passed. As a side effect it "invalidates" that portion of
 *	the displayed text (if any), so that things will be repainted
 *	properly.
 * 
 ****************************procedure*header*****************************
 */
static int
_OlTextSubString(ctx, left, right, target, size, used)
	TextPaneWidget ctx;
	OlTextPosition left, right;
	unsigned char  *target;       /* Memory to copy into */
	int            size,          /* Size of memory */
	*used;         /* Memory used by copy */
{
	unsigned char   *tempResult;
	int             length, resultLength, n;
	OlTextBlock     text;
	OlTextPosition  end, nend;

	end = (*(ctx->text.source->read))(ctx->text.source, left, &text,
	    right - left);
	n = length = min(text.length,size);
	(void) strncpy((char *)target, (OLconst char *)text.ptr, n);
	while ((end < right) && (length < size)) {
		nend = (*(ctx->text.source->read))(ctx->text.source, end, &text,
		    right - end);
		n = text.length;
		if (length + n > size) n = size - length;
		tempResult = target + length;
		(void) strncpy((char *)tempResult, (OLconst char *)text.ptr, n);
		length += n;
		end = nend;
	}
	*used = length;
	return length; /* return the number of positions transfered to target */
}	/*  _OlTextSubString  */

#ifdef SHARELIB
#define open	(*_libXol_open)
#define read	(*_libXol_read)
#define close	(*_libXol_close)
extern int open();
extern int read();
extern int close();
#endif

/*
 *************************************************************************
 * 
 *  OlTextInsertFile - Insert a file of the given name into the text.
 *	Returns 0 if file found, -1 if not.
 * 
 ****************************procedure*header*****************************
 */
static int
OlTextInsertFile(ctx, str)
	TextPaneWidget ctx;
	unsigned char *str;
{
	int fid;
	OlTextBlock text;
	unsigned char    buf[1000];
	OlTextPosition position;

	if (str == NULL || _OlStrlen(str) == 0) return -1;
	fid = open((OLconst char *)str, O_RDONLY);
	if (fid <= 0) return -1;
	_XtTextPrepareToUpdate(ctx);
	position = ctx->text.insertPos;
	while ((text.length = read(fid, buf, 512)) > 0) {
		text.ptr = buf;
		ReplaceText(ctx, position, position, &text, TRUE);
		position = (*(ctx->text.source->scan))(ctx->text.source, position, 
		    OlstPositions, OlsdRight, text.length, TRUE);
	}
	(void) close(fid);
	ctx->text.insertPos = position;
	_XtTextExecuteUpdate(ctx);
	return 0;
}	/*  OlTextInsertFile  */

static Boolean
ActivateWidget OLARGLIST((w, type, call_data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	call_data)
{
	Boolean consumed = False;

	switch (type)
	{
		case OL_COPY:
			consumed = True;
			CopySelection(w, NULL);
			break;
		case OL_CUT:
			consumed = True;
			KillCurrentSelection(w, NULL);
			break;
	}
	return (consumed);
} /* end of ActivateWidget */


#ifdef SHARELIB
#undef open
#undef read
#undef close
#endif
