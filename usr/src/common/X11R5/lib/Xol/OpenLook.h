#ifndef	NOIDENT
#ident	"@(#)olmisc:OpenLook.h	1.134"
#endif

#ifndef __Ol_OpenLook_h__
#define __Ol_OpenLook_h__

/*
 *************************************************************************
 *
 * Description:
 *		This file contains all the things necessary to compile
 *	an OPEN LOOK (tm) application.
 *
 *******************************file*header*******************************
 */

#include "limits.h"	/* for SHRT_MIN, and perhaps others */

#ifdef SHARELIB
#include <Xol/OlXlibExt.h>
#endif
#include <Xol/OlClients.h>

#ifdef _XtIntrinsic_h			/* for Xwin 1.0 compatibility */

#include <Xol/OlStrings.h>

/*
 ************************************************************************
 *	Define some ANSI C and C++ stuff
 ************************************************************************
 */

			/* Define `OLconst' as `const'			*/
			/*	if ANSI C or C++, otherwise define	*/
			/*	it as ` ' (i.e nothing).		*/
			/* The toolkit developers should always use	*/
			/*	`OLconst' to avoid the compilation	*/
			/*	problems on a non-ANSI C or non-C++	*/
			/*	environment				*/

#if !defined(__STDC__) && !defined(__cplusplus) && !defined(c_plusplus)
#define OLconst
#else
#define OLconst	const
#endif

/* Define two macros for function Prototypes.
 * NOTE: since ANSI C and C++ treat externs for parameterless functions
 * differently, declare a parameter function in the following way:
 *
 * For functions with parameters:
 *	fooWithParams OL_ARGS((String, int));
 * For functions with no parameters:
 *	fooNoParams OL_NO_ARGS();
 *
 * If token OlNeedFunctionPrototypes is defined and is zero, we undefine.
 * Else we set redefine it to have the value of one.
 */

#if !defined(OlNeedFunctionPrototypes) || (OlNeedFunctionPrototypes != 0)

#ifdef OlNeedFunctionPrototypes
#undef OlNeedFunctionPrototypes
#endif /* OlNeedFunctionPrototypes */

#if defined(__cplusplus) || defined(c_plusplus)

#define OlNeedFunctionPrototypes 1
#define OL_ARGS(x)	x
#define OL_NO_ARGS()	()

#else /* __cpluslus || c_plusplus */
#if defined(__STDC__)

#define OlNeedFunctionPrototypes 1
#define OL_ARGS(x)	x
#define OL_NO_ARGS()	(void)

#else /* defined(__STDC__) */

#undef OlNeedFunctionPrototypes
#define OL_ARGS(x)	()
#define OL_NO_ARGS()	()

#endif /* defined(__STDC__) */
#endif /* __cpluslus || c_plusplus */
#else /* !defined(OlNeedFunctionPrototypes)||(OlNeedFunctionPrototypes != 0) */

#undef OlNeedFunctionPrototypes
#define OL_ARGS(x)	()
#define OL_NO_ARGS()	()

#endif /* !defined(OlNeedFunctionPrototypes)||(OlNeedFunctionPrototypes != 0) */

	/* Now add three new macros to be used when defining functions.
	 * These macros look at the OlNeedFunctionPrototype token to
	 * determine how the macro is expanded.  Below illustrates how
	 * to use the macro.
	 *
	 *	FooBar OLARGLIST((num1, string, flag))
	 *		OLARG(int,	num1)
	 *		OLARG(String,	string)
	 *		OLGRA(int,	flag)
	 *	{
	 *		body of FooBar...
	 *	}
	 *
	 * For variable argument functions use the following syntax:
	 *
	 *	FooBar OLARGLIST((num1, string, OLVARGLIST))
	 *		OLARG(int,	num1)
	 *		OLARG(String,	string)
	 *		OLVARGS
	 *	{
	 *		body of FooBar...
	 *	}
	 *
	 * Also add two macros for enveloping function prototype sections.
	 */

#if OlNeedFunctionPrototypes

#define OLARGLIST(list)	(
#define OLARG(t,a)	t a,
#define OLGRA(t,a)	t a)
#define OLVARGLIST	...			/* junk parameter	*/
#define OLVARGS		...)

#if defined(__cplusplus) || defined(c_plusplus)
#define OLBeginFunctionPrototypeBlock	extern "C" {
#define OLEndFunctionPrototypeBlock	}
#else /* defined(__cplusplus) || defined(c_plusplus) */
#define OLBeginFunctionPrototypeBlock
#define OLEndFunctionPrototypeBlock
#endif /* defined(__cplusplus) || defined(c_plusplus) */

#else /* OlNeedFunctionPrototypes */

#define OLARGLIST(list)	list
#define OLARG(t,a)	t a;
#define OLGRA(t,a)	t a;
#define OLVARGLIST	va_alist
#define OLVARGS		va_dcl

#define OLBeginFunctionPrototypeBlock
#define OLEndFunctionPrototypeBlock

#endif /* OlNeedFunctionPrototypes */



/*
 ************************************************************************
 *	Define True Constant Tokens
 *		Tokens that appear in this section should not have
 *		their values changed.
 ************************************************************************
 */

#define OL_IGNORE		( ~0 )
#define OL_NO_ITEM		( (Cardinal)( ~0 ) )

/*
 ************************************************************************
 *	Define Tokens that have arbitary value assignments.  (Though
 *	their values must not change since it would break backwards
 *	compatibility.)
 *	New tokens should be added to the end of the list and their
 *	values must be unique.
 *	WARNING: do not remove or renumber any of the values in this
 *	list.
 ************************************************************************
 */

#define OleditDone		0
#define OleditError		1
#define OleditPosError		2
#define OleditReject		3

#define OL_ABSENT_PAIR		0
#define OL_ALL			1
#define OL_ALWAYS		2
#define OL_ATOM_HELP		3
#define OL_BOTH			4
#define OL_BOTTOM		5
#define OL_BUTTONSTACK		6
#define OL_CENTER		7
#define OL_CLASS_HELP		8
#define OL_COLUMNS		9
#define OL_COPY_MASK_VALUE	10
#define OL_COPY_SIZE		11
#define OL_COPY_SOURCE_VALUE	12
#define OL_CURRENT		13
#define OL_DEFAULT_PAIR		14
#define OL_DISK_SOURCE		15
#define OL_DISPLAY_FORM		16
#define OL_DOWN			17
#define OL_EXISTING_SOURCE	18
#define OL_FIXEDCOLS		19
#define OL_FIXEDHEIGHT		20
#define OL_FIXEDROWS		21
#define OL_FIXEDWIDTH		22
#define OL_FLAT_BUTTON		23
#define OL_FLAT_CHECKBOX	24
#define OL_FLAT_CONTAINER	25
#define OL_FLAT_EXCLUSIVES	26
#define OL_FLAT_HELP		27
#define OL_FLAT_NONEXCLUSIVES	28
#define OL_HALFSTACK		29
#define OL_HORIZONTAL		30
#define OL_IMAGE		31
#define OL_IN			32
#define OL_INDIRECT_SOURCE	33
#define OL_LABEL		34
#define OL_LEFT			35
#define OL_MASK_PAIR		36
#define OL_MAXIMIZE		37
#define OL_MILLIMETERS		38
#define OL_MINIMIZE		39
#define OL_NEVER		40
#define OL_NEXT			41
#define OL_NONE			42
#define OL_NONEBOTTOM		43
#define OL_NONELEFT		44
#define OL_NONERIGHT		45
#define OL_NONETOP		46
#define OL_NOTICES		47
#define OL_NO_VIRTUAL_MAPPING	48
#define OL_OBLONG		49
#define OL_OUT			50
#define OL_OVERRIDE_PAIR	51
#define OL_PIXELS		52
#define OL_POINTS		53
#define OL_POPUP		54
#define OL_PREVIOUS		55
#define OL_PROG_DEFINED_SOURCE	56
#define OL_RECTBUTTON		57
#define OL_RIGHT		58
#define OL_ROWS			59
#define OL_SOURCE_FORM		60
#define OL_SOURCE_PAIR		61
#define OL_STAYUP		62
#define OL_STRING		63
#define OL_STRING_SOURCE	64
#define OL_TEXT_APPEND		65
#define OL_TEXT_EDIT		66
#define OL_TEXT_READ		67
#define OL_TOP			68
#define OL_TRANSPARENT_SOURCE	69
#define OL_VERTICAL		70
#define OL_VIRTUAL_BUTTON	71
#define OL_VIRTUAL_KEY		72
#define OL_WIDGET_HELP		73
#define OL_WINDOW_HELP		74
#define OL_WRAP_ANY		75
#define OL_WRAP_WHITE_SPACE	76
#define OL_CONTINUOUS		77
#define OL_GRANULARITY		78
#define OL_RELEASE		79
#define OL_TICKMARK		80
#define OL_PERCENT		81
#define OL_SLIDERVALUE		82
#define OL_WT_BASE		83
#define OL_WT_CMD		84
#define OL_WT_NOTICE		85
#define OL_WT_HELP		86
#define OL_WT_OTHER		87
#define OL_SUCCESS		88
#define OL_DUPLICATE_KEY	89
#define OL_DUPLICATEKEY		89
#define OL_BAD_KEY		90
#define OL_MENU_FULL		91
#define OL_MENU_LIMITED		92
#define OL_MENU_CANCEL		93
#define OL_SELECTKEY		94
#define OL_MENUKEY		95
#define OL_MENUDEFAULT		96
#define OL_MENUDEFAULTKEY	97
#define OL_HSBMENU		98
#define OL_VSBMENU		99
#define OL_ADJUSTKEY		100
#define OL_NEXTAPP		101
#define OL_NEXTWINDOW		102
#define OL_PREVAPP		103
#define OL_PREVWINDOW		104
#define OL_WINDOWMENU		105
#define OL_WORKSPACEMENU	106
#define OL_DEFAULTACTION	108
#define OL_DRAG			109
#define OL_DROP			110
#define OL_TOGGLEPUSHPIN	111
#define OL_PAGELEFT		112
#define OL_PAGERIGHT		113
#define OL_SCROLLBOTTOM		114
#define OL_SCROLLTOP		115
#define OL_MULTIRIGHT		116
#define OL_MULTILEFT		117
#define OL_MULTIDOWN		118
#define OL_MULTIUP		119
#define OL_IMMEDIATE		120
#define OL_MOVEUP		121
#define OL_MOVEDOWN		122
#define OL_MOVERIGHT		123
#define OL_MOVELEFT		124
#define OL_CLICK_TO_TYPE	125
#define OL_REALESTATE		126
#define OL_UNDERLINE		127
#define OL_HIGHLIGHT		128
#define OL_INACTIVE		129
#define OL_DISPLAY		130
#define OL_PROC			131
#define OL_SIZE_PROC		132
#define OL_DRAW_PROC		133
#define OL_PINNED_MENU		134
#define OL_PRESS_DRAG_MENU	135
#define OL_STAYUP_MENU		136
#define OL_POINTER		137
#define OL_INPUTFOCUS		138
#define OL_QUIT			142
#define OL_DESTROY		143
#define OL_DISMISS		144
#define OL_PRE			145
#define OL_POST			146

	/* mooLIT extensions...			*/
#define OL_MENUBARKEY		147
#define OL_SHADOW_OUT		148
#define OL_SHADOW_IN		149
#define OL_SHADOW_ETCHED_OUT	150
#define OL_SHADOW_ETCHED_IN	151
#define OL_MOTIF_GUI		152
#define OL_OPENLOOK_GUI		153

	/* Notice types  - Notice widget	*/
#define OL_ERROR		154
#define OL_INFORMATION		155
#define OL_QUESTION		156
#define OL_WARNING		157
#define OL_WORKING 		158

	/* Flat Button Types...			*/
#define OL_RECT_BTN		159
#define OL_OBLONG_BTN		160
#define OL_CHECKBOX		161

	/* Popup Menu Shell ...			*/
#define OL_BUSY			162

	/* Abbreviated Button...		*/
#define OL_MENU_BTN		163
#define OL_WINDOW_BTN		164

	/* Motif bindings that do not have equivalent O/L bindings...	*/
#define OLM_BExtend		165
#define OLM_BPrimaryPaste	166
#define OLM_BPrimaryCopy	167
#define OLM_BPrimaryCut		168

#define OLM_BDrag		OLM_BPrimaryPaste
#define OLM_BQuickPaste		OLM_BPrimaryPaste
#define OLM_BQuickCopy		OLM_BPrimaryCopy
#define OLM_BQuickCut		OLM_BPrimaryCut

#define OLM_KAddMode		169
#define OLM_KClear		170
#define OLM_KNextPane		171
#define OLM_KPrevPane		172
#define OLM_KReselect		173
#define OLM_KRestore		174
#define OLM_KDeselectAll	175
#define OLM_KNextPara		176
#define OLM_KPrevPara		177
#define OLM_KPrimaryCopy	178
#define OLM_KPrimaryCut		179
#define OLM_KPrimaryPaste	180
#define OLM_KQuickCopy		181
#define OLM_KQuickCut		182
#define OLM_KQuickExtend	183
#define OLM_KQuickPaste		184
#define OLM_KSelectAll		185

	/* Motif bindings that do have equivalent O/L bindings...	*/
#define OLM_BToggle		OL_ADJUST
#define OLM_BMenu		OL_MENU
#define OLM_BSelect		OL_SELECT

#define OLM_KCancel		OL_CANCEL
#define OLM_KCopy		OL_COPY
#define OLM_KCut		OL_CUT
#define OLM_KActivate		OL_DEFAULTACTION
#define OLM_KBackSpace		OL_DELCHARBAK
#define OLM_KDelete		OL_DELCHARFWD
#define OLM_KEraseEndLine	OL_DELLINEFWD
#define OLM_KEndData		OL_DOCEND
#define OLM_KBeginData		OL_DOCSTART
#define OLM_KDown		OL_MOVEDOWN
#define OLM_KHelp		OL_HELP
#define OLM_KLeft		OL_MOVELEFT
#define OLM_KEndLine		OL_LINEEND
#define OLM_KBeginLine		OL_LINESTART
#define OLM_KMenu		OL_MENUKEY
#define OLM_KNextFamilyWindow	OL_NEXTAPP
#define OLM_KNextField		OL_NEXT_FIELD
#define OLM_KNextWindow		OL_NEXTWINDOW
#define OLM_KPageDown		OL_PAGEDOWN
#define OLM_KPageLeft		OL_PAGELEFT
#define OLM_KPageRight		OL_PAGERIGHT
#define OLM_KPageUp		OL_PAGEUP
#define OLM_KPaste		OL_PASTE
#define OLM_KPrevFamilyWindow	OL_PREVAPP
#define OLM_KPrevField		OL_PREV_FIELD
#define OLM_KPrevWindow		OL_PREVWINDOW
#define OLM_KEnter		OL_RETURN
#define OLM_KRight		OL_MOVERIGHT
#define OLM_KSelect		OL_SELECTKEY
#define OLM_KUndo		OL_UNDO
#define OLM_KUp			OL_MOVEUP
#define OLM_KWindowMenu		OL_WINDOWMENU
#define OLM_KPrevWord		OL_WORDBAK
#define OLM_KNextWord		OL_WORDFWD
#define OLM_KNextMenuDown	OL_MULTIDOWN
#define OLM_KPrevMenuLeft	OL_MULTILEFT
#define OLM_KNextMenuRight	OL_MULTIRIGHT
#define OLM_KPrevMenuUp		OL_MULTIUP
#define OLM_KMenuBar		OL_MENUBARKEY

#define OL_PARENT		186
#define OL_CHILD		187

	/* Link modifier for mouse buttons			*/
#define OL_LINK			188
#define OL_LINKKEY		189

	/* Another source type for OlRegisterHelp()		*/
#define OL_DESKTOP_SOURCE	190

	/* Motif bindings that do not have equivalent O/L bindings, cont... */
#define OLM_KExtend		191

#define OL_PANE			192
#define OL_HANDLES		193
#define OL_DECORATION		194
#define OL_CONSTRAINING_DECORATION 195
#define OL_SPANNING_DECORATION	196
#define OL_OTHER		197

#define OL_ACCELERATORKEY	198
#define OL_MNEMONICKEY		199

/*
 * For ChangeBar stuff. Numbers are historical (sorry!)
 */
#define OL_DIM			1000
#define OL_NORMAL		1001

/*
 * An OlDefine that shall never conflict with any other.
 */
#define OL_UNSPECIFIED		32767

/* These are bit masks used in Vendor's XtNwmProtocolMask resource.*/
#if !defined(__STDC__) && !defined(__cplusplus) && !defined(c_plusplus)
#include <limits.h>
#define _OL_CHAR_BIT	CHAR_BIT
#else
#define _OL_CHAR_BIT	8
#endif

#define _OL_HI_BIT()	(unsigned long)((unsigned long)1 << (sizeof(long) * _OL_CHAR_BIT - 1))
#define _OL_WM_GROUP(X)	(_OL_HI_BIT() >> (X))
#define _OL_WM_GROUPALL()	(_OL_WM_GROUP(0))
#define _OL_WM_BITALL()		(~(_OL_WM_GROUP(0)))
#define OL_WM_DELETE_WINDOW	(_OL_WM_GROUP(0) | ((unsigned long)1 << 0))
#define OL_WM_SAVE_YOURSELF	(_OL_WM_GROUP(0) | ((unsigned long)1 << 1))
#define OL_WM_TAKE_FOCUS	(_OL_WM_GROUP(0) | ((unsigned long)1 << 2))

#define _OL_WM_TESTBIT(X, B)	(((X & _OL_WM_GROUPALL()) & \
				 (B & _OL_WM_GROUPALL())) ? \
				 ((X & _OL_WM_BITALL()) & \
				 (B & _OL_WM_BITALL())) : 0)

/* OPEN LOOK Virtual Event (Open Look commands),
 * To maintain the backward compatiability, the following "#define"s
 * are not sorted. As a reminder, the first 59 (i.e., 0-58)
 * entries are according to the OlInputEvent stuff from Dynamic.h
 * NOTE:  DO NOT ADD ANY VALUES TO THE END OF THIS LIST -- ADD THEM TO
 * TO THE END OF THE ABOVE LIST SO THAT THE NUMBER OF COLLISIONS
 * FOR THE #define SYMBOLS' NUMERIC VALUES IS MINIMIZED!!!!
 */
#define OL_UNKNOWN_INPUT	0
#define OL_UNKNOWN_BTN_INPUT	1
#define OL_SELECT		2
#define OL_ADJUST		3
#define OL_MENU			4
#define OL_CONSTRAIN		5
#define OL_DUPLICATE		6
#define OL_PAN			7
#define OL_UNKNOWN_KEY_INPUT	8
#define OL_CUT			9
#define OL_COPY			10
#define OL_PASTE		11
#define OL_HELP			12
#define OL_CANCEL		13
#define OL_PROP			14
#define OL_PROPERTY		OL_PROP
#define OL_STOP			15
#define OL_UNDO			16
#define OL_NEXTFIELD		17
#define OL_NEXT_FIELD		OL_NEXTFIELD
#define OL_PREVFIELD		18
#define OL_PREV_FIELD		OL_PREVFIELD
#define OL_CHARFWD		19
#define OL_CHARBAK		20
#define OL_ROWDOWN		21
#define OL_ROWUP		22
#define OL_WORDFWD		23
#define OL_WORDBAK		24
#define OL_LINESTART		25
#define OL_LINEEND		26
#define OL_DOCSTART		27
#define OL_DOCEND		28
#define OL_PANESTART		29
#define OL_PANEEND		30
#define OL_DELCHARFWD		31
#define OL_DELCHARBAK		32
#define OL_DELWORDFWD		33
#define OL_DELWORDBAK		34
#define OL_DELLINEFWD		35
#define OL_DELLINEBAK		36
#define OL_DELLINE		37
#define OL_SELCHARFWD		38
#define OL_SELCHARBAK		39
#define OL_SELWORDFWD		40
#define OL_SELWORDBAK		41
#define OL_SELLINEFWD		42
#define OL_SELLINEBAK		43
#define OL_SELLINE		44
#define OL_SELFLIPENDS		45
#define OL_REDRAW		46
#define OL_RETURN		47
#define OL_PAGEUP		48
#define OL_PAGEDOWN		49
#define OL_HOME			50
#define OL_END			51
#define OL_SCROLLUP		52
#define OL_SCROLLDOWN		53
#define OL_SCROLLLEFT		54
#define OL_SCROLLRIGHT		55
#define OL_SCROLLLEFTEDGE	56
#define OL_SCROLLRIGHTEDGE	57
#define OL_PGM_GOTO		58	/* LAST NUMBER IN THIS LIST	*/
/*
 * NOTE:  DO NOT ADD ANY VALUES TO THE END OF THE ABOVE LIST -- ADD THEM TO
 * TO THE END OF THE ABOVE THIS LIST SO THAT THE NUMBER OF COLLISIONS
 * FOR THE #define SYMBOLS' NUMERIC VALUES IS MINIMIZED!!!!
 */

/*
 ************************************************************************
 *	Define Bitwise Tokens
 *		This are easily identified by the naming convention
 ************************************************************************
 */
#define OL_B_OFF		0
#define OL_B_HORIZONTAL		(1<<0)
#define OL_B_VERTICAL		(1<<1)
#define OL_B_BOTH		(OL_B_VERTICAL|OL_B_HORIZONTAL)

/*
 ************************************************************************
 *    Define pseudo-gravity values. Real gravity values currently
 *    stop with 10, but we'll leave some room for official growth.
 ************************************************************************
 */
#define AllGravity			21  /* attach all four sides    */
#define NorthSouthEastWestGravity	22  /* attach all four sides    */
#define SouthEastWestGravity		23  /* attach bottom,right,left */
#define NorthEastWestGravity		24  /* attach top,right,left    */
#define NorthSouthWestGravity		25  /* attach top,bottom,left   */
#define NorthSouthEastGravity		26  /* attach top,bottom,right  */
#define EastWestGravity			27  /* attach left,right        */
#define NorthSouthGravity		28  /* attach top,bottom        */

/************************************************************************
 *	Define Constant tokens that are being maintained for source
 *	compatibility with older versions of the toolkit.
 */
#define OL_BEEP_NEVER			OL_NEVER
#define OL_BEEP_NOTICES			OL_NOTICES
#define OL_BEEP_NOTICES_AND_FOOTERS	OL_ALWAYS
#define OL_BEEP_ALWAYS			OL_ALWAYS
#define OL_AUTO_SCROLL_OFF		OL_B_OFF
#define OL_AUTO_SCROLL_HORIZONTAL	OL_B_HORIZONTAL
#define OL_AUTO_SCROLL_VERTICAL		OL_B_VERTICAL
#define OL_AUTO_SCROLL_BOTH		OL_B_BOTH
#define OL_GROW_OFF			OL_B_OFF
#define OL_GROW_HORIZONTAL		OL_B_HORIZONTAL
#define OL_GROW_VERTICAL		OL_B_VERTICAL
#define OL_GROW_BOTH			OL_B_BOTH

/*
 ************************************************************************
 *	Define Constant token for naming the locale definition file.
 ************************************************************************
 */
#define OL_LOCALE_DEF			"ol_locale_def"

/************************************************************************
 *	Define Pixmap type
 *	Note: On some systems, only one of the following types would 
 *	work properly, this is why this macro is introduced.  Later
 *	on this macro might not be supported.
 ************************************************************************
 */
#ifdef USE_XYPIXMAP
#define PixmapType      XYPixmap
#else
#define PixmapType      ZPixmap
#endif

/*
 ************************************************************************
 *	Define public structures
 ************************************************************************
 */

/*
 * This structure is used to register help using OlRegisterHelp() for
 * the OL_DESKTOP_SOURCE source type.
 */
typedef struct _OlDtHelpInfo {
   char *app_title;
   char *title;
   char *filename;
   char *section;
   char *path;
} OlDtHelpInfo;

/*
 *************************************************************************
 *	Macros
 *************************************************************************
 */
			/* The following three macros would normally
			 * appear in the private header file, but we
			 * need them for the metric macros		*/

#define _OlFloor(value)		((int) (value))
#define _OlCeiling(value)	((int) (_OlFloor(value) == (value) ? \
					(value) : ((value) + 1)))
						/* Metric macros	*/

extern Display * toplevelDisplay;

#define OlDefaultDisplay toplevelDisplay
#define OlDefaultScreen  DefaultScreenOfDisplay(toplevelDisplay)

					/* Pixels to Millimeters */

#define Ol_ScreenPixelToMM(direction, value, screen) \
	((double) (value) * (double)(direction == OL_HORIZONTAL ? \
	((double)WidthMMOfScreen(screen) / (double)WidthOfScreen(screen))  :\
	((double)HeightMMOfScreen(screen) / (double)HeightOfScreen(screen))))

#define OlScreenPixelToMM(direction, value, screen) \
		OlRound(Ol_ScreenPixelToMM(direction, value, screen))

#define Ol_PixelToMM(direction, value) \
		Ol_ScreenPixelToMM(direction, value, OlDefaultScreen)

#define OlPixelToMM(direction, value) \
		OlScreenPixelToMM(direction, value, OlDefaultScreen)

					/* Millimeters to Pixels */

#define Ol_ScreenMMToPixel(direction, value, screen) \
	((double) (value) * (double)(direction == OL_HORIZONTAL ? \
	((double)WidthOfScreen(screen) / (double)WidthMMOfScreen(screen))  :\
	((double)HeightOfScreen(screen) / (double)HeightMMOfScreen(screen))))

#define OlScreenMMToPixel(direction, value, screen) \
		OlRound(Ol_ScreenMMToPixel(direction, value, screen))

#define Ol_MMToPixel(direction, value) \
		Ol_ScreenMMToPixel(direction, value, OlDefaultScreen)

#define OlMMToPixel(direction, value) \
		OlScreenMMToPixel(direction, value, OlDefaultScreen)

					/* Pixels to Points */

#define Ol_ScreenPixelToPoint(direction, value, screen) \
	(2.834645669 * (double)(value) * (double)(direction == OL_HORIZONTAL ? \
	((double)WidthMMOfScreen(screen) / (double)WidthOfScreen(screen))  :\
	((double)HeightMMOfScreen(screen) / (double)HeightOfScreen(screen))))

#define OlScreenPixelToPoint(direction, value, screen) \
		OlRound(Ol_ScreenPixelToPoint(direction, value, screen))

#define Ol_PixelToPoint(direction, value) \
		Ol_ScreenPixelToPoint(direction, value, OlDefaultScreen)

#define OlPixelToPoint(direction, value) \
		OlScreenPixelToPoint(direction, value, OlDefaultScreen)

					/* Points to Pixels */


#define OlScreenPointToPixel(direction, value, screen) \
		OlRound(Ol_ScreenPointToPixel(direction, (double)(value), screen))

#define Ol_PointToPixel(direction, value) \
		Ol_ScreenPointToPixel(direction, (double)value, OlDefaultScreen)

#define OlPointToPixel(direction, value) \
		OlScreenPointToPixel(direction, value, OlDefaultScreen)


/*
 * Macro to perform overlapping memory moves:
 */
#if	defined(SVR4_0) || defined(SVR4) || defined(__hpux)
# define OlMemMove(Type,p1,p2,n) \
	(void)memmove((XtPointer)(p1), (XtPointer)(p2), (n) * sizeof(Type))
#else
# define OlMemMove(Type,p1,p2,n) \
	if (p1 && p2) {						\
		register Type *		from	= p2;		\
		register Type *		to	= p1;		\
		register Cardinal	count	= n;		\
								\
		if (from < to) {				\
			from += count;				\
			to += count;				\
			while (count--)				\
				*--to = *--from;		\
		} else {					\
			while (count--)				\
				*to++ = *from++;		\
		}						\
	} else
#endif

    /* I18N convenience macros for font and font list metrics.
       Note the use of 'font->ascent' instead of 'font->max_bounds.ascent':
       This is used to calculate line spacing not strictly the bounding box
       for the largest theoretical character.
    */
#ifdef I18N
#define OlFontAscent(font, font_list)	( ((font_list) == NULL) ? \
	    (font)->ascent : (font_list)->max_bounds.ascent )

#define OlFontDescent(font, font_list)	( ((font_list) == NULL) ? \
	    (font)->descent : (font_list)->max_bounds.descent )

#define OlFontHeight(font, font_list) ( ((font_list) == NULL) ? \
	    (font)->ascent + (font)->descent : \
	    (font_list)->max_bounds.ascent + (font_list)->max_bounds.descent )

#define OlFontMaxBounds(font, font_list) ( ((font_list) == NULL) ? \
	    (font)->max_bounds : (font_list)->max_bounds )

		/* Can't define as ( OlFontMaxBounds(font, font_list).width )
		 * because it can upset some compilers, (sun_port notes).
		 */
#define OlFontWidth(font, font_list) ( ((font_list) == NULL) ? \
		((font)->max_bounds).width : ((font_list)->max_bounds).width )
#else
#define OlFontAscent(font, font_list)	( (font)->ascent )
#define OlFontDescent(font, font_list)	( (font)->descent )
#define OlFontMaxBounds(font, font_list) ( (font)->max_bounds )
#define OlFontHeight(font, font_list) ( (font)->ascent + (font)->descent )
#define OlFontWidth(font, font_list) ( ((font)->max_bounds).width )
#endif

/*
 * Macros to make it a little easier to use XtAppAddTimeOut.
 */
#define OlAddTimeOut(W,I,C,D) \
	XtAppAddTimeOut(XtWidgetToApplicationContext(W),(I),(C),(D))

/*
 * Defines for dealing with unspecified values:
 */
#define XtUnspecifiedOlDefine  OL_UNSPECIFIED
#define XtUnspecifiedDimension (Dimension)65535 /* Xt documents 2^16-1 */
#define XtUnspecifiedShort     SHRT_MIN  /* e.g. negative 2^16 */
#define XtUnspecifiedCardinal  UINT_MAX
#define XtUnspecifiedBoolean   2


/*************************************************************************
 *
 *	typedef's, enum's, struct's
 */

				/* Add a function Prototype for
				 * Dynamic callbacks		*/

typedef void	(*OlDynamicCallbackProc) OL_ARGS((XtPointer));
typedef short		OlDefine;	/* OPEN LOOK non-bitmask #defines */
typedef OlDefine	OlVirtualName;
typedef unsigned long	OlBitMask;	/* OPEN LOOK bitmask #defines	*/

		/* Declare a function pointer type for the variable
		 * argument error handlers.  Note: since this header
		 * does not include stdarg.h (or varargs.h), we'll
		 * only specify function prototypes only if the macros
		 * va_start and va_arg are defined (which would be
		 * the case if stdarg.h (or varargs.h) were included).	*/

#if defined(va_start) && defined(va_arg)
typedef void	(*OlVaDisplayErrorMsgHandler) OL_ARGS((
	Display *,	/* display;	display pointer or NULL		*/
	OLconst char *,	/* name;	message's resource name - 	*/
			/*		concatenated with the type	*/
	OLconst char *,	/* type;	message's resource type - 	*/
			/*		concatenated to the name	*/
	OLconst char *,	/* class;	class of message		*/
	OLconst char *,	/* message;	composed message created from	*/
	va_list		/* vargs	variable arguments		*/
));
typedef OlVaDisplayErrorMsgHandler	OlVaDisplayWarningMsgHandler;

#else /* defined(va_start) && defined(va_arg) */

typedef void				(*OlVaDisplayErrorMsgHandler)();
typedef OlVaDisplayErrorMsgHandler	OlVaDisplayWarningMsgHandler;

#endif /* defined(va_start) && defined(va_arg) */

typedef void	(*OlErrorHandler) OL_ARGS((
	OLconst char *	/* message;	simple error message string	*/
));

typedef void	(*OlWarningHandler) OL_ARGS((
	OLconst char * 	/* message;	simple warning message string	*/
));

				/* Include some things that should have
				 * been in the R3 Intrinsics but were
				 * not.  These must be removed for R4	*/

#ifndef XtSpecificationRelease
typedef caddr_t		XtPointer;
#define XtRWidget	"Widget"
#endif /* XtSpecificationRelease */

typedef struct {
	String		name;		/* XtN string			*/
	String		default_value;	/* `,' separated string		*/
	OlVirtualName	virtual_name;	/* ol command value		*/
} OlKeyOrBtnRec, *OlKeyOrBtnInfo;

typedef struct {
	Widget		widget;		/* new id to be filled in	*/
	String		name;		/* name of new widget		*/
	WidgetClass *	class_ptr;	/* addr. of widget class	*/
	Widget *	parent_ptr;	/* addr. of parent id		*/
	String		descendant;	/* resource names used in GetValues
                                         * call on parent to get real
                                         * parent			*/
	ArgList		resources;	/* resources for new widget	*/
	Cardinal	num_resources;	/* number of resources		*/
	Boolean		managed;	/* should widget be managed ??? */
} OlPackedWidget, *OlPackedWidgetList;

/*
 * Support for Text Verification Callbacks
 */
typedef enum {motionVerify, modVerify, leaveVerify} OlVerifyOpType;
typedef enum {OlsdLeft, OlsdRight} OlScanDirection;
typedef enum {
    OlstPositions, OlstWhiteSpace, OlstEOL, OlstParagraph, OlstLast
    } OlScanType;

typedef long OlTextPosition;
typedef struct {
	int           firstPos;
	int           length;
	unsigned char *ptr;
} OlTextBlock, *OlTextBlockPtr;

typedef struct {
	XEvent		*xevent;
	OlVerifyOpType	operation;
	Boolean		doit;
	OlTextPosition	currInsert, newInsert;
	OlTextPosition	startPos, endPos;
	OlTextBlock	*text;
} OlTextVerifyCD, *OlTextVerifyPtr;

typedef char		OlMnemonic;

typedef struct {
	Boolean		consumed;
	XEvent *	xevent;
	Modifiers	dont_care;
	OlVirtualName	virtual_name;
	KeySym		keysym;
	String 		buffer;
	Cardinal	length;
	Cardinal	item_index;
} OlVirtualEventRec, *OlVirtualEvent;

typedef struct _OlVirtualEventInfo *OlVirtualEventTable; /* an opque defn */

typedef void	(*OlEventHandlerProc) OL_ARGS((Widget, OlVirtualEvent));

typedef struct {
	int			type;	/* XEvent type		*/
	OlEventHandlerProc	handler;/* handling routine	*/
} OlEventHandlerRec, *OlEventHandlerList;

typedef struct {
	int		msgtype;	/* type of WM msg */
	XEvent		*xevent;
} OlWMProtocolVerify;

typedef enum { 
	NOT_DETERMINED, MOUSE_CLICK, MOUSE_MOVE, MOUSE_MULTI_CLICK 
} ButtonAction;

typedef void	(*OlMouseActionCallback) OL_ARGS((
	Widget			w,
	ButtonAction		action,
	Cardinal		count,
	Time			time,
	XtPointer		client_data
));

/*
 * For the 3D coloration:
 */

typedef struct _OlColorTuple {
	Pixel		bg0;	/* ``white''          */
	Pixel		bg1;	/* XtNbackground      */
	Pixel		bg2;	/* bg1 + 10-15% black */
	Pixel		bg3;	/* bg1 + 50% black    */
}		OlColorTuple;

typedef struct _OlColorTupleList {
	OlColorTuple *	list;
	Cardinal	size;
}		OlColorTupleList;

/* for Internationalization */

#define MAXFONTS	4		/* max. # of fonts (code sets) */
#define	NUM_IM_CALLBACKS	7

#define	OlImNeedNothing		000
#define	OlImPreEditArea		001
#define	OlImPreEditCallbacks	002
#define	OlImPreEditPosition	004
#define	OlImStatusArea		010
#define	OlImStatusCallbacks	020
#define	OlImFocusTracks		040	/* this is not needed for OL */

typedef struct _OlMAndAList {
	KeySym	keysym;
	unsigned int	modifier;
	} OlMAndAList;
typedef struct _MandAList {
	OlMAndAList	* ol_keys;
	int		num_ol_keys;
	OlMAndAList	*accelerators;
	int		num_accelerators;
	OlMAndAList	*mnemonics;
	int		num_mnemonics;
	} MAndAList;
	
typedef struct _OlIcValues {
	String	attr_name;		/* attribute name */
	void	*attr_value;		/* attribute value */
	} OlImValues, OlIcValues ;

typedef OlIcValues * OlIcValuesList;
typedef OlImValues * OlImValuesList;
typedef void	(*OlImProc)();

typedef struct _OlFontList {
	int		num;		/* num. XFontstruct */
	XFontStruct ** fontl;		/* fonts in 'fontgroup' */
	int *		cswidth;	/* width of char in code sets */
	String *	csname;		/* code set names - NEEDED ? */
	String		fgrpdef;
	XCharStruct	max_bounds;	/* max bounds over all fonts */
	} OlFontList;

typedef struct _OlStrSegment {
	unsigned short code_set;	/* code set that string belongs to */
	int		len;		/* length of parsed string */
	unsigned char	* str;		/* parsed string */
	} OlStrSegment;

	/* Obsolete: this information is now part of the _OlFontList */
typedef struct _OlFontInfo {
	int	ascent;
	int	descent;
	int	width;
	int	height;
	} OlFontInfo;

#define NUM_SUPPORT_STYLES	1
typedef unsigned short	OlImStyle;

/* this should be used as a returned value of OlLookupImString() in 
   next load
*/
/*  In X11R5 these status symbols are defined in Xlib.h and
    XlibSpecificationRelease is first introduced in X11R5.
 */
#if defined(XlibSpecificationRelease)
typedef int OlImStatus;
#else
typedef enum _OlImStatus {
	XBufferOverflow,
	XLookupNone,
	XLookupChars,
	XLookupKeySym,
	XLookupBoth
} OlImStatus;
#endif

typedef struct	_OlImStyles {
	short		styles_count;
	OlImStyle	*supported_styles;
	} OlImStyles;

typedef struct _OlImCallback {
	OlImValues	client_data;
	OlImProc	callback;
	} OlImCallback;

typedef struct _OlIcWindowAttr {
	Pixel		background;	/* background pixel */
	Pixel		foreground;	/* background pixel */
	Colormap	colormap;	/* colormap for pre-edit to use */
	Colormap	std_colormap;	
	Pixmap		back_pixmap;	/* background pixmap for IM to use */
	OlFontList	fontlist;	/* font list for text drawing */
	int		spacing;	/* line spacing for text lines in PE */
	Cursor		cursor;		/* cursor to use for PE window */	
	OlImCallback	callback[NUM_IM_CALLBACKS];
	} OlIcWindowAttr;

typedef struct _OlIm {
	struct _OlIc *	iclist;		/* Input context list */
	OlImStyles 	im_styles;	/* supported pre-edit types */
	OlImValuesList	imvalues;
	String		appl_name;	/* application name */
	String		appl_class;	/* application class */
	long		version;	/* OPEN LOOK version */
	void *		imtype;		/* hook for IM specific data */
	} OlIm;

typedef struct _OlIc {
	Window  	cl_win;		/* client window ID */
	XRectangle	cl_area;	/* client area for pre-edit window */
	Window  	focus_win;	/* focus window ID */
	OlIcWindowAttr  s_attr;	/* attributes for status window */
	XRectangle	s_area;		/* area for status window */
	OlIcWindowAttr  pre_attr;	/* attributes for pre-edit window */
	XRectangle	pre_area;	/* area for pre-edit window */
	OlImStyle	style;		/* styles needed for this IC */
	XPoint		spot;		/* cur. cursor location in text win. */
	struct _OlIm	* im;		/* ptr. to OlIm structure */
	struct _OlIc	*nextic;	/* ptr. to next in list */
	void		*ictype;	/* IM specific hook */
	} OlIc;	

typedef struct _OlImFunctions {
	OlIm *		(*OlOpenIm)();
	void		(*OlCloseIm)();
	OlIc *		(*OlCreateIc)();
	int		(*OlLookupImString)();
	void		(*OlDestroyIc)();
	void		(*OlSetIcFocus)();
	void		(*OlUnsetIcFocus)();
	String		(*OlGetIcValues)();
	String		(*OlSetIcValues)();
	String		(*OlResetIc)();
	OlIm *		(*OlImOfIc)();
	Display *	(*OlDisplayOfIm)();
	String		(*OlLocaleOfIm)();
	void		(*OlGetImValues)();	
	} OlImFunctions;

extern OlImFunctions *  OlImFuncs;

/*
 *************************************************************************
 *	Public extern declarations
 *************************************************************************
 */

OLBeginFunctionPrototypeBlock

extern void
OlAction OL_ARGS((
	Widget,		/* w;		*/
	XEvent *,	/* xevent;	*/
	String *,	/* params;	*/
	Cardinal *	/* num_params;	*/
));

extern Boolean
OlActivateWidget OL_ARGS((
	Widget,		/* widget;		widget to activate	*/
	OlVirtualName,	/* activation_type;	type of activation	*/
	XtPointer	/* activate_data;	widget specific data	*/	
));

extern void
OlAddCallback OL_ARGS((
	Widget,
	String,
	XtCallbackProc,
	XtPointer
));

extern Boolean
OlAssociateWidget OL_ARGS((
	Widget,		/* leader widget */
	Widget,		/* follower widget */
	Boolean		/* disable traversal */
));

extern Boolean
OlCallAcceptFocus OL_ARGS((
	Widget,
	Time
));

extern void
OlCallCallbacks OL_ARGS((
	Widget,
	OLconst char *,
	XtPointer
));

extern void
OlCallDynamicCallbacks OL_NO_ARGS();

extern Boolean
OlCanAcceptFocus OL_ARGS((
	Widget,
	Time
));

extern void
OlClassSearchIEDB OL_ARGS((
	WidgetClass,		/* wc;		*/
	OlVirtualEventTable	/* db;		*/
));

extern void
OlClassSearchTextDB OL_ARGS((
	WidgetClass	/* wc;		- the target */
));

extern void
OlClassUnsearchIEDB OL_ARGS((
	WidgetClass,		/* wc; - remove all references to db if NULL */
	OlVirtualEventTable	/* db; the target			*/
));

extern OlVirtualEventTable
OlCreateInputEventDB OL_ARGS((
	Widget,		/* w;				*/
	OlKeyOrBtnInfo,	/* key_info;			*/
	int,		/* num_key_info;		*/
	OlKeyOrBtnInfo,	/* btn_info;			*/
	int		/* num_btn_info;		*/
));

extern void
OlDestroyInputEventDB OL_ARGS((
	OlVirtualEventTable	/* db;			*/
));

	/* Convert a string containing virtual expressions into
	 * into a standard Xt translation string		*/
extern String
OlConvertVirtualTranslation OL_ARGS((
	String		/* translation;	- virtual translation	*/
));

extern Boolean
OlDragAndDrop OL_ARGS((
	Widget,		/* widget which started drag operation	*/
	Window *,	/* window where drop occurred		*/
	Position *,	/* x location in window of drop		*/
	Position *	/* y location in window of drop		*/
));

				/* OPEN LOOK toolkit error	*/
extern void
OlError OL_ARGS((
	OLconst char *		/* error_msg;	- message string */
));

extern Widget
OlFetchMneOrAccOwner OL_ARGS((
	Widget,		/* widget - owner of ve	*/
	OlVirtualEvent	/* ve - virtual event	*/
));

extern String
OlFindHelpFile OL_ARGS((
	Widget,		/* Widget id		*/
	OLconst char *		/* Help filename	*/
));

extern OlDefine
OlGetGui OL_NO_ARGS();

extern String
OlGetMessage OL_ARGS((
	Display *,	/* Display			*/
	String,		/* buffer to place message in	*/
	int,		/* size of buffer		*/
	OLconst char *,	/* message name (OleN...)	*/
	OLconst char *,	/* message type (OleT...)	*/
	OLconst char *,	/* message class (OleC...)	*/
	OLconst char *,	/* the message (OleM...)	*/
	XrmDatabase));	/* message database, or NULL	*/

extern XtCallbackStatus
OlHasCallbacks OL_ARGS((
	Widget,
	OLconst char *
));

/* Note: this function is obsolete.  Use OlGetCurrentFocusWidget() */
extern Boolean
OlHasFocus OL_ARGS((
	Widget		/* widget - widget to query	*/
));

extern Widget
OlInitialize OL_ARGS((
	OLconst char *,	/* shell_name;	- initial shell instance name	*/
	OLconst char *,	/* classname;	- application class		*/
	XrmOptionDescRec *, /* urlist;	- options list			*/
	Cardinal,	/* num_urs;	- number of options		*/
	int *,		/* argc;	- pointer to argc		*/
	String *	/* argv;	- argv				*/
));

extern Bool
OlIsFMRunning OL_ARGS((
	Display *,
	Screen *
));

extern Bool
OlIsWMRunning OL_ARGS((
	Display *,
	Screen *
));

extern Bool
OlIsWSMRunning OL_ARGS((
	Display *,
	Screen *
));

#define OL_CORE_IE		(XtPointer) 1
#define OL_TEXT_IE		(XtPointer) 2
#define OL_DEFAULT_IE		(XtPointer) 3

extern void
OlLookupInputEvent OL_ARGS((
	Widget,		/* widget;		- widget getting xevent	*/
	XEvent *,	/* xevent;		- xevent to look at	*/
	OlVirtualEvent,	/* virtual_event_ret;	- returned virtual event*/
	XtPointer	/* db_flag;		- search flag		*/
));

typedef void    (*OlMenuPositionProc) OL_ARGS((
	Widget,         /* menu;         - menu shell widget id		*/
	Widget,         /* emanate;      - the emanate widget		*/
	Cardinal,	/* emanate_index - index to flat item		*/
	OlDefine,       /* state;        - menu's state          	*/
	Position *,     /* mx;           - moving position       	*/
	Position *,     /* my;           - moving position       	*/
	Position *,     /* px;           - pointer position      	*/
	Position *      /* py;           - pointer position      	*/
));

extern void
OlMenuPopdown OL_ARGS((
	Widget,			/* w;		- menu's widget id 	*/
	Boolean			/* pinned_also; - pinned or not 	*/
));

extern void
OlMenuPopup OL_ARGS((
	Widget,			/* w;		 - menu's widget id	*/
	Widget,                 /* emanate;      - emanate widget       */
	Cardinal,		/* emanate_index - index to flat item	*/
	OlDefine,		/* state;	 - menu's state		*/
	Boolean,		/* setpos;	 - set position 	*/
	Position,		/* x;		 - pointer position 	*/
	Position,		/* y;		 - pointer position 	*/
	OlMenuPositionProc	/* pos_proc;	 - procedure or NULL 	*/
));

extern void
OlMenuPost OL_ARGS((
	Widget			/* w;		- Menu's Widget id */
));

extern void
OlMenuUnpost OL_ARGS((
	Widget			/* w;		- menu's widget id */
));

extern Widget
OlMoveFocus OL_ARGS((
	Widget,		/* widget - Starting point */
	OlDefine,	/* direction - direction of movement */
	Time		/* time - time of request */
));

					/* Toolkit post-initialization	*/
extern void
OlPostInitialize OL_ARGS((
	OLconst char *,	/* classname;	- application class		*/
	XrmOptionDescRec *, /* urlist;	- options list			*/
	Cardinal,	/* num_urs;	- number of options		*/
	int *,		/* argc;	- pointer to argc		*/
	String *	/* argv;	- argv				*/
));

					/* Toolkit pre-initialization	*/
extern void
OlPreInitialize OL_ARGS((
	OLconst char *,	/* classname;	- application class		*/
	XrmOptionDescRec *, /* urlist;	- options list			*/
	Cardinal,	/* num_urs;	- number of options		*/
	int *,		/* argc;	- pointer to argc		*/
	String *	/* argv;	- argv				*/
));

extern void
OlGetApplicationResources OL_ARGS((
	Widget			w,
	XtPointer		base,
	XtResource *		resources,
	int			num_resources,
	ArgList			args,
	Cardinal		num_args
));

extern void
OlGetApplicationValues OL_ARGS((
	Widget,		/* widget;	- widget to get display */
	ArgList,	/* args;	- args to be fetched	*/
	Cardinal	/* num_args;	- number of args	*/
));

extern Widget
OlGetCurrentFocusWidget OL_ARGS((
	Widget		/* widget - of the Base window for this widget */
));

extern void
OlGrabDragPointer OL_ARGS((
	Widget,		/* widget wanting to begin draa operation	*/
	Cursor,		/* cursor used during drag operation		*/
	Window		/* confine to window or None			*/
));

extern void
OlGrabVirtualKey OL_ARGS((
	Widget			w,
	OlVirtualName		vkey,
	Boolean			owner_events,
	int			pointer_mode,
	int			keyboard_mode
));

extern OlDefine
OlQueryAcceleratorDisplay OL_ARGS((
	Widget		/* any widget	*/
));

extern OlDefine
OlQueryMnemonicDisplay OL_ARGS((
	Widget		/* any widget	*/
));

extern void
OlRegisterDynamicCallback OL_ARGS((
	OlDynamicCallbackProc	proc,
	XtPointer		data
));

						/* Register help	*/
extern void
OlRegisterHelp OL_ARGS((
	OlDefine,	/* id_type;	- type of help being registered	*/
	XtPointer,	/* id;		- id of object registering help	*/
	String,		/* tag;		- string tag for the help msg.	*/
	OlDefine,	/* source_type;	- type of help message source	*/
	XtPointer	/* source;	- pointer to the message source	*/
));

extern void
OlRemoveCallback OL_ARGS((
	Widget,
	String,
	XtCallbackProc,
	XtPointer
));

extern Boolean
OlSetAppEventProc OL_ARGS((
	Widget,			/* widget of the application		*/
	OlDefine,		/* type:	OL_PRE or OL_POST	*/
	OlEventHandlerList,	/* list 				*/
	Cardinal		/* length of list			*/
));

extern void
OlSetApplicationValues OL_ARGS((
	Widget,		/* widget;	- widget to get display */
	ArgList,	/* args;	- args to be set	*/
	Cardinal	/* num_args;	- number of args	*/
));

extern void
OlSetGui OL_ARGS((
	OlDefine	/* type; 	OPENLOOK or MOTIF 	*/
));

extern Boolean
OlSetInputFocus OL_ARGS((
	Widget,		/* widget;	widget to get focus	*/
	int,		/* revert_to;	See XLib documentation	*/
	Time		/* time;	time or CurrentTime	*/
));

			/* This procedure is used to override the default
			 * fatal error message procedure.		*/

extern OlVaDisplayErrorMsgHandler
OlSetVaDisplayErrorMsgHandler OL_ARGS((
	OlVaDisplayErrorMsgHandler	/* new handler or NULL	*/
));

			/* This procedure is used to override the default
			 * non-fatal error message procedure.		*/

extern OlVaDisplayWarningMsgHandler
OlSetVaDisplayWarningMsgHandler OL_ARGS((
	OlVaDisplayWarningMsgHandler	/* new handler or NULL	*/
));

	/* To register a procedure to be called on fatal errors:	*/
extern OlErrorHandler
OlSetErrorHandler OL_ARGS((
	OlErrorHandler		/* handler;	- handler to be called	*/
));

	/* To register a procedure to be called on non-fatal errors:	*/
extern OlWarningHandler
OlSetWarningHandler OL_ARGS((
	OlWarningHandler	/* handler;	- handler to be called	*/
));

extern void
OlUnassociateWidget OL_ARGS((
	Widget			/* follower widget */
));

extern void
OlUngrabDragPointer OL_ARGS((
	Widget			/* widget which started the drag operation */
));

extern void
OlUngrabVirtualKey OL_ARGS((
	Widget			w,
	OlVirtualName		vkey
));

extern int
OlUnregisterDynamicCallback OL_ARGS((
	OlDynamicCallbackProc	proc,
	XtPointer		data
));

extern void
OlUpdateDisplay OL_ARGS((
	Widget			w
));

			/* The OPEN LOOK toolkit's fatal error routine.
			 * This is a variable argument list routine
			 * where the first 5 parameters are mandatory	*/

/* PRINTFLIKE5 */
extern void
OlVaDisplayErrorMsg OL_ARGS((
	Display *,	/* display;	Display pointer or NULL		*/
	OLconst char *,	/* name;	message's resource name - 
					concatenated with the type	*/
	OLconst char *,	/* type;	message's resource type - 
					concatenated to the name	*/
	OLconst char *,	/* class;	class of message		*/
	OLconst char *,	/* message;	the message string		*/
	...		/* parameters;	variable arguments to satisfy
					the message parameters		*/
));

			/* The OPEN LOOK toolkit's non-fatal error routine.
			 * This is a variable argument list routine
			 * where the first 5 parameters are mandatory	*/

/* PRINTFLIKE5 */
extern void
OlVaDisplayWarningMsg OL_ARGS((
	Display *,	/* display;	Display pointer or NULL		*/
	OLconst char *,	/* name;	message's resource name - 
					concatenated with the type	*/
	OLconst char *,	/* type;	message's resource type - 
					concatenated to the name	*/
	OLconst char *,	/* class;	class of message		*/
	OLconst char *,	/* message;	the message string		*/
	...		/* parameters;	variable arguments to satisfy
					the message parameters		*/
));

					/* OPEN LOOK toolkit warning	*/
extern void
OlWarning OL_ARGS((
	OLconst char *		/* error_msg;	- message string	*/
));

extern String
OlWidgetClassToClassName OL_ARGS((
	WidgetClass		/* wc;		*/
));

extern void
OlWidgetSearchIEDB OL_ARGS((
	Widget,			/* w;		*/
	OlVirtualEventTable	/* db;		*/
));

extern void
OlWidgetSearchTextDB OL_ARGS((
	Widget		/* widget;	- the target */
));

extern void
OlWidgetUnsearchIEDB OL_ARGS((
	Widget,			/* w; - remove all references to db if NULL*/
	OlVirtualEventTable	/* db; - the target			*/
));

extern String
OlWidgetToClassName OL_ARGS((
	Widget			/* w;		*/
));

extern void
OlWMProtocolAction OL_ARGS((
	Widget,
	OlWMProtocolVerify *,
	OlDefine		/* action to be taken */
));

extern void
OlLocaleInitialize OL_ARGS((
	Widget
));

extern int
OlDrawString OL_ARGS((
	Display *,
	Drawable,
	OlFontList *,
	GC,
	int,
	int,
	unsigned char *,
	int
));

extern int
OlDrawImageString OL_ARGS((
	Display *,
	Drawable,
	OlFontList *,
	GC,
	int,
	int,
	unsigned char *,
	int
));

extern int
OlTextWidth OL_ARGS((
	OlFontList *,
	unsigned char *,
	int
));

	/* Obsolete: this info is now contained within the OlFontList */
extern OlFontInfo *
OlMaxFontInfo OL_ARGS((
	OlFontList *
));

extern int
OlGetNextStrSegment OL_ARGS((
	OlFontList *,
	OlStrSegment *,
	unsigned char **,
	int *
));

extern OlIm *  
OlOpenIm OL_ARGS((
	Display *, 
	XrmDatabase, 
	String, 
	String
));

extern void    
OlCloseIm OL_ARGS((
	OlIm *
));

extern OlIc *  
OlCreateIc OL_ARGS((
	OlIm *, 
	OlIcValues *
));

extern void    
OlDestroyIc OL_ARGS((
	OlIc *
));

extern int  
OlLookupImString OL_ARGS((
	XKeyEvent *, 
	OlIc *, 
	String, 
	int,
	KeySym *, 
	OlImStatus *
));

extern void    
OlSetIcFocus OL_ARGS((
	OlIc *
));

extern void    
OlUnsetIcFocus OL_ARGS((
	OlIc *
));

extern String  
OlGetIcValues OL_ARGS((
	OlIc *, 
	OlIcValues *
));

extern String  
OlSetIcValues OL_ARGS((
	OlIc *, 
	OlIcValues *
));

extern String
OlResetIc OL_ARGS((
	OlIc *
));

extern OlIm *
OlImOfIc OL_ARGS((
	OlIc *
));

extern Display *
OlDisplayOfIm OL_ARGS((
	OlIm *
));

extern String
OlLocaleOfIm OL_ARGS((
	OlIm *
));

extern void
OlGetImValues OL_ARGS((
	OlIm *,
	OlImValues *
));

extern void
OlSetShellPosition OL_ARGS((
	Widget			w,
	Position		default_x,
	Position		default_y
));

extern double
Ol_ScreenPointToPixel OL_ARGS((
        int,
        double,
        Screen *
));

extern double
OlRound OL_ARGS((
        double
));

extern void
OlToolkitInitialize OL_ARGS((
			     int *,	/* argc */
			     String *,	/* argv */
			     XtPointer	/* other */
			     ));

extern void
OlCheckReadOnlyResources OL_ARGS((
	Widget			/* new */,
	Widget			current,
	ArgList			args,
	Cardinal		num_args
));
extern void
OlCheckReadOnlyConstraintResources OL_ARGS((
	Widget			/* new */,
	Widget			current,
	Arg *			args,
	Cardinal		num_args
));
extern void
OlCheckUnsettableResources OL_ARGS((
	Widget			/* new */,
	Widget			current,
	ArgList			args,
	Cardinal		num_args,
	...
));
extern void
OlCheckUnsettableConstraintResources OL_ARGS((
	Widget			/* new */,
	Widget			current,
	Arg *			args,
	Cardinal		num_args,
	...
));

extern Boolean
OlRectInRegion OL_ARGS((
	Region			region,
	XRectangle *		src_rect,
	XRectangle *		dst_rect
));
extern void
OlIntersectRectWithRegion OL_ARGS((
	XRectangle *		rectangle,
	Region			source,
	Region			destination
));

/*
 * WARNING: The following two routines assume a non-null ref_name
 * is allocated, and will free it and set it to null if in error.
 */

#define OlReferenceManaged	0x100
typedef enum OlReferenceScope {
	OlReferenceSibling                = 0,
	OlReferenceParentOrSibling        = 1,
	OlReferenceAny                    = 2,
	OlReferenceManagedSibling         = 0 | OlReferenceManaged,
	OlReferenceManagedParentOrSibling = 1 | OlReferenceManaged,
	OlReferenceAnyManaged             = 2 | OlReferenceManaged
}			OlReferenceScope;

extern void
OlResolveReference OL_ARGS((
	Widget			w,
	Widget *		ref_widget,
	String *		ref_name,
	OlReferenceScope	scope,
	Widget			(*find_widget) OL_ARGS((
		Widget			root,
		OLconst char *		name
	)),
	Widget			root
));
extern void
OlCheckReference OL_ARGS((
	Widget			w,
	Widget *		ref_widget,
	String *		ref_name,
	OlReferenceScope	scope,
	Widget			(*find_widget) OL_ARGS((
		Widget			root,
		OLconst char *		name
	)),
	Widget			root
));
extern Pixel
OlContrastingColor OL_ARGS((
      Widget                  w,
      Pixel                   pixel,
      int                     percent
));

extern XrmDatabase
OlOpenDatabase OL_ARGS((
			Display *, 
			OLconst char *
));
extern void
OlCloseDatabase OL_ARGS((
			 XrmDatabase
));

extern void
OlGetColorTupleList OL_ARGS((
	Widget			w,
	OlColorTupleList **	list,
	Boolean *		use
));
extern void
OlSetColorTupleList OL_ARGS((
	Widget			w,
	OlColorTupleList *	list,
	Boolean			use
));

extern void
OlResetMouseAction OL_ARGS((
	Widget			w
));
extern ButtonAction
OlDetermineMouseAction OL_ARGS((
	Widget			w,
	XEvent *		event
));
extern ButtonAction
OlDetermineMouseActionWithCount OL_ARGS((
	Widget			w,
	XEvent *		event,
	Cardinal *		count
));
extern ButtonAction
OlDetermineMouseActionEntirely OL_ARGS((
	Widget			w,
	XEvent *		event,
	Cursor			cursor,
	OlMouseActionCallback	callback,
	XtPointer		client_data
));

OLEndFunctionPrototypeBlock

#endif /* _XtIntrinsic_h */

#ifdef	sun
extern void _XtInherit();
static void (*OlInherit)() = _XtInherit; /* needed to pull in libXt.sa */
#endif

#endif /* __Ol_OpenLook_h__ */
