
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:olksh.h	1.2"

/* CONSTANT Character Strings, saves memory */

#ifdef MOOLIT
#define str_XtRString		XtRString
#define str_XtRCardinal		XtRCardinal
#define str_XtRWidgetList	XtRWidgetList
#define str_XtRPointer		XtRPointer
#define str_XtRBitmap		XtRBitmap
#define str_XtRPixmap		XtRPixmap
#define str_XtRWidget		XtRWidget
#define str_XtRInt		XtRInt
#define str_XtRBoolean		XtRBoolean
#define str_XtRPosition		XtRPosition
#define str_XtRBool		XtRBool
#define str_XtRDimension	XtRDimension
#define str_XtRPixel		XtRPixel
#define str_XtRFontStruct	XtRFontStruct
#define str_XtRCallback		XtRCallback
#else
extern const char str_XtRString[];
extern const char str_XtROlVirtualName[];
extern const char str_XtRPointer[];
extern const char str_XtRBitmap[];
extern const char str_XtRPixmap[];
extern const char str_XtRWidget[];
extern const char str_XtRInt[];
extern const char str_XtRBoolean[];
extern const char str_XtRPosition[];
extern const char str_XtRBool[];
extern const char str_XtRDimension[];
extern const char str_XtRPixel[];
extern const char str_XtRFontStruct[];
extern const char str_XtRCardinal[];
extern const char str_XtRCallback[];
#endif

extern const char str_Function[];
extern const char str_XtROlDefineInt[];
extern const char str_XtROlDefine[];
extern const char str_XtROlVirtualName[];
extern const char str_XtROlChangeBarDefine[];

extern const char str_wksh[];
extern const char str_s_eq_s[];
extern const char str_s_eq[];
extern const char str_nill[];
extern const char str_bad_argument_s[];
extern const char str_unknown_pointer_type[];
extern const char str_Pointer[];
extern const char str_cannot_parse_s[];
extern const char str_0123456789[];
extern const char str_left_over_points_ignored[];
extern const char str_polygon_not_supported[];
extern const char str_opt_rect[];
extern const char str_opt_fillrect[];
extern const char str_opt_fillpoly[];
extern const char str_opt_line[];
extern const char str_opt_text[];
extern const char str_opt_arc[];
extern const char str_opt_fillarc[];
extern const char str_opt_point[];
extern const char usage_draw[];
extern const char str_could_not_init[];

typedef struct {
	char *envar;
        int maxitems, lastitem;
        OlListToken *items;
} SListInfo_t;

#define FLAT_SELECT	0
#define FLAT_UNSELECT	1
#define FLAT_DBLSELECT	2
#define FLAT_DROP	3

typedef struct {
	char *selectProcCommand;
        char *unselectProcCommand;
#ifdef MOOLIT
	char *dblSelectProcCommand;
	char *dropProcCommand;
#endif
} FlatInfo_t;


typedef struct {
	Pixel focuscolor, nofocuscolor;
	Pixel cursorcolor, nocursorcolor;
	Widget textfield, textedit;
	Boolean autotraversal;
	Boolean autoblank;
	Boolean active;
	Boolean autoreturn;
	Boolean clipwhite;
	TextPosition maxcursor;
	char cvtcase;		/* 'u', 'l', or 'i' for upper, lower, or initial caps. '\0' for none */
	char *charset;		/* a regular expression */
} textfield_op_t;
