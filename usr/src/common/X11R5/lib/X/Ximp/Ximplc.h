#pragma ident	"@(#)R5Xlib:Ximp/Ximplc.h	1.3"

/* $XConsortium: Ximplc.h,v 1.6 92/07/29 10:16:27 rws Exp $ */
/*
 * Copyright 1990, 1991, 1992 by TOSHIBA Corp.
 * Copyright 1990, 1991, 1992 by SORD Computer Corp.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that 
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA Corp. and SORD Computer Corp.
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  TOSHIBA Corp. and
 * SORD Computer Corp. make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * TOSHIBA CORP. AND SORD COMPUTER CORP. DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL TOSHIBA CORP. OR SORD COMPUTER CORP. BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *				mopi@ome.toshiba.co.jp
 */

/******************************************************************

              Copyright 1991, 1992 by FUJITSU LIMITED
	      Copyright 1991, 1992 by Sun Microsystems, Inc.
              Copyright 1991, 1992 by Sony Corporaion

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of FUJITSU LIMITED,
Sun Microsystems, Inc. and Sony Corporation not be used in advertising 
or publicity pertaining to distribution of the software without specific,
written prior permission.
FUJITSU LIMITED, Sun Microsystems, Inc. and Sony Corporation make no 
representations about the suitability of this software for any purpose.
It is provided "as is" without express or implied warranty.

FUJITSU LIMITED, SUN MICROSYSTEMS, INC. AND SONY CORPORATION DISCLAIM 
ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJITSU
LIMITED, SUN MICROSYSTEMS, INC. AND SONY CORPORATION BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  Author: Takashi Fujiwara     FUJITSU LIMITED 
          Hiromu Inukai        Sun Microsystems, Inc.
          Hideki Hiura         Sun Microsystems, Inc.
	  Makoto Wakamatsu     Sony Corporaion

******************************************************************/
/*
	HISTORY:

	An sample implementation for Xi18n function of X11R5
	based on the public review draft 
	"Xlib Changes for internationalization,Nov.1990"
	by Katsuhisa Yano,TOSHIBA Corp..

	Modification to the high level pluggable interface is done
	by Takashi Fujiwara,FUJITSU LIMITED.
*/

#define XIMP_40
#include "XIMProto.h"

#ifndef NOT_USE_SJIS
#define USE_SJIS
#endif
#ifndef NOT_XIMP_BC
#define XIMP_BC
#endif

#define	CODESET_FILE	"Codeset"
#define	COMPOSE_FILE	"Compose"

#define GL		0x00
#define GR		0x80
#define MAX_CODESET	10
#define MAX_FONTSET	50
#define XIMP_MB_CUR_MAX(lcd)   (((XimpLCd)(lcd))->locale.mb_cur_max) 
#ifndef MB_CUR_MAX
#define MB_CUR_MAX      XIMP_MB_CUR_MAX(_XlcCurrentLC())
#endif
#ifndef MB_LEN_MAX
#define MB_LEN_MAX      8
#endif

#ifndef MAXINT
#define MAXINT		(~((unsigned int)1 << (8 * sizeof(int)) - 1))
#endif /* !MAXINT */

#define LC_METHODS(lcd)	(((XimpLCd)(lcd))->lc_methods)

/*
 * XLCd dependent data
 */

typedef unsigned char         Side;
typedef struct _ParseInfoRec *ParseInfo;
typedef struct _XimpLCdRec   *XimpLCd;

typedef struct _CharSetRec {
    char	       *name;
    Side		side;
    int			length;
    char	       *encoding;	/* Compound Text encoding */
    int			gc_num;		/* num of graphic characters */
    Bool		string_encoding;
} CharSetRec, *CharSet;

typedef struct _CodeSetRec {
    int			cs_num;
    Side		side;
    int			length;
    ParseInfo		parse_info;
    unsigned long	wc_encoding;
    int			charset_num;
    CharSet	       *charset_list;
} CodeSetRec, *CodeSet;

typedef enum {
    E_GL,				/* GL encoding */
    E_GR,				/* GR encoding */
    E_SS,				/* single shift */
    E_LSL,				/* locking shift left */
    E_LSR,				/* locking shift right */
    E_LAST
} EncodingType;

typedef struct _ParseInfoRec {
    EncodingType	type;
    char	       *encoding;
    CodeSet		codeset;
} ParseInfoRec;

typedef enum {
    MBType,
    WCType
} CvtType;

typedef struct _CvtDataRec {
    CvtType		type;
    int			per_size;
    union {
	char	       *multi_byte;
	wchar_t	       *wide_char;
	XChar2b        *xchar2b;
    } string;
    int			length;
    int			cvt_length;
    CodeSet		codeset;
} CvtDataRec, *CvtData;

typedef struct _StateRec {
    XimpLCd		lcd;
    int			(*converter)();
    CodeSet		codeset;
    CodeSet		GL_codeset;
    CodeSet		GR_codeset;
    CodeSet		last_codeset;
    struct _StateRec   *next;
    Bool		is_used;
} StateRec, *State;

typedef struct _LCMethodsRec {
    void		(*destroy)();
    State		(*create_state)();
    void		(*destroy_state)();
    void		(*cnv_start)();
    void		(*cnv_end)();
    char		(*mbchar)();
    int			(*mbstocs)();
    int			(*wcstocs)();
    int			(*cstombs)();
    int			(*cstowcs)();
} LCMethodsRec, *LCMethods;

typedef struct _FontSetDataRec {
    char	       *font_name;
    Side		side;
    int			cs_num;
} FontSetDataRec, *FontSetData;

typedef struct _LocaleRec {
    char	       *name;
    char	       *language;
    char	       *territory;
    char	       *codeset;
    int			codeset_num;
    CodeSet	       *codeset_list;
    int			mb_cur_max; 
    unsigned char      *mb_parse_table;
    int			mb_parse_list_num;
    ParseInfo	       *mb_parse_list;
    unsigned long	wc_encode_mask;
    unsigned long	wc_shift_bits;
    Bool		state_dependent;
    CodeSet		initial_state_GL;
    CodeSet		initial_state_GR;
    int			fontset_data_num;
    FontSetData		fontset_data;
    XPointer		extension;
} LocaleRec, *Locale;

typedef struct _XimpLCdRec {
    XLCdMethods		methods;
    XLCdCoreRec		core;	
    LCMethods		lc_methods;
    LocaleRec		locale;
} XimpLCdRec;

/*
 * XFontSet dependent data
 */

typedef struct {
    XFontStruct	       *font;
    CodeSet		codeset;
    Side		side;
} FontSetRec;

typedef struct {
    int			fontset_num;
    FontSetRec	       *fontset;
    XPointer		extension;
} XFontSetXimpRec;

typedef struct _XimpFontSetRec {
    XFontSetMethods	methods;
    XFontSetCoreRec	core;	
    XFontSetXimpRec	ximp_fspart;
} XimpFontSetRec, *XimpFontSet;

extern Bool _XlcRegisterCharSet();
extern CharSet _XlcGetCharSetFromEncoding(), _XlcGetCharSetFromName();
extern CodeSet _XlcGetCodeSetFromCharSet();
extern Bool _XlcInsertLoader();

extern XimpLCd _XlcCreateLC();
extern Bool _XlcLoadCodeSet();
extern void _XlcDestroyLC(), _XlcDestroyState(), _XlcCnvStart(), _XlcCnvEnd();
extern State _XlcCreateState();

extern char _Xlc_mbchar();
extern int _Xlc_mbstocs(), _Xlc_wcstocs(), _Xlc_cstombs(), _Xlc_cstowcs();

extern XFontSet _XDefaultCreateFontSet();
extern XIM _Ximp_OpenIM();


/*
 * Input Method data
 */

typedef struct _Ximp_XIM	*Ximp_XIM;
typedef struct _Ximp_XIC	*Ximp_XIC;

#define XIMP_NAME	256

#define XIMP_CREATE_IC	0
#define	XIMP_SET_IC	1
#define	XIMP_START_IC	2

#define FILTERD		True
#define NOTFILTERD	False

#define XIMP_MAXBUF 1024
#define CT_MAX_IN_CM 15

/*
 * XIM Extension data
 */

typedef struct {
	int		extension_back_front_exist;
	Atom		extension_back_front_id;
	Bool		extension_conversion_exist;
	Atom		extension_conversion_id;
	Bool		extension_conversion;
	int		extension_statuswindow_exist;
	Atom		extension_statuswindow_id;
	int		extension_lookup_exist;
	Atom		extension_lookup_id;
	Atom		extension_lookup_start;
	Atom		extension_lookup_start_rep;
	Atom		extension_lookup_draw;
	Atom		extension_lookup_proc;
	Atom		extension_lookup_proc_rep;
	/* Add Extension */
} Ximp_ExtXIMRec;

/*
 * Data dtructure for local processing
 */

/*
 * Method for represent Keysequence by Tree of DefTree structure.
 *
 *  Key[:string] ->next
 *   |
 *   V succession
 *
 * <Compose><A><quotedbl>: "A_diaerasis"
 * <Compose><A><apostrophe>: "A_acute"
 * <acute><A>: "A_acute"
 *
 * is translated to
 *
 * Compose ----> acute -> NIL
 *    |            |
 *    |            V
 *    V            A:A_acute
 *    A -> NIL     |
 *    |            V
 *    |           NIL
 *    V
 * quotedbl:A_diaerasis -> apostrophe:A_acute -> NIL
 *    |                         |
 *    V                         V
 *   NIL                       NIL
 *
 * Each structure address means context
 *
 */

typedef struct _DefTree {
    struct _DefTree *next; 		/* another Key definition */
    struct _DefTree *succession;	/* successive Key Sequence */
					/* Key definitions */
    unsigned         modifier_mask;
    unsigned         modifier;
    KeySym           keysym;		/* leaf only */
    char            *mb;
    wchar_t         *wc;		/* make from mb */
#ifdef notdef
    KeySym keysym_return;
#endif
} DefTree;

/*
 * XIM dependent data
 */

typedef struct  {
	Bool		 is_local;
	int		 reconnection_mode;
	Bool		 is_connected;
	char		*im_name;
	int		 def_svr_mode;
	Ximp_KeyList	*process_start_keys;
	Bool		 use_wchar;
	XIMStyles	*delaybind_styles;

	/* for IMServer */
	Window		 fe_window;
	Window		 owner;
	Atom		 improtocol_id;
	Atom		 version_id;
	Atom		 style_id;
	Atom		 keys_id;
	Atom		 servername_id;
	Atom		 serverversion_id;
	Atom		 vendorname_id;
	Atom		 extentions_id;
	Atom		 ctext_id;
	Atom		 focus_win_id;
	Atom		 preedit_atr_id;
	Atom		 status_atr_id;
	Atom		 preeditfont_id;
	Atom		 statusfont_id;
	Atom		 preeditmaxsize_id;
	Atom		 type_id;
	long		*type_list;
	char		*im_proto_vl;
	int		 im_proto_vnum;
	XIMStyles	*im_styles;
	Ximp_KeyList	*im_keyslist;
	Ximp_KeyList	*im_offkeyslist;
	char		*im_server_name;
	char		*im_server_vl;
	char		*im_vendor_name;
	Atom		*im_ext_list;

	/* for Local Processing */
	XIC		current_ic;
	DefTree		*top;
#ifdef BACKTRACK_WHEN_UNMATCHED
	int		num_save_rooms;
#endif

	/* for Extension */
	Ximp_ExtXIMRec	*imtype;

	/* for Force Select KeyRelease */
	Bool             is_forceselectkeyrelease;
} XIMXimpRec;

/*
 * IM struct
 */

typedef struct _Ximp_XIM {
	XIMMethods	 methods;
	XIMCoreRec	 core;
	XIMXimpRec	*ximp_impart;
} Ximp_XIMRec;

/*
 * Externsion Lookup Callback
 */

typedef struct {
	XIMCallback     start;
	XIMCallback     done;
	XIMCallback     draw;
	XIMCallback     proc;
} ICExtLookupCallbacks;

 /*
  * data block describing the visual attributes associated with an input
  * context
  */

typedef struct {
	XRectangle      area;
	XRectangle      area_needed;
	XPoint          spot_location;
	Colormap        colormap;
	Atom            std_colormap;
	unsigned long   foreground;
	unsigned long   background;
	Pixmap          background_pixmap;
	XFontSet        fontset;
	int             line_space;
	Cursor          cursor;
	XPointer	draw_data;
	ICExtLookupCallbacks callbacks;
} ICExtLookupAttributes, *ICExtLookupAttributesPtr;

typedef int Ximp_CBStatus;

#define XIMPCBPREEDITACTIVE    0x00000001
#define XIMPCBSTATUSACTIVE     0x00000002
#define DEFAULTCBSTATUSSTRING "Disconnected"

typedef enum _Ximp_inputmode {
    BEING_BYPASSED  = 0,
    BEING_PREEDITED = 1
} input_mode_t ;

/*
 * IC deprndent data
 */

typedef struct {
	long			 icid;
	int			 svr_mode;
	input_mode_t		 input_mode;
	int			 filter_mode;
	long			 value_mask;
	unsigned long		 back_mask;
	Bool			 putback_key_event;
	Window			 back_focus_win;

	long			 proto3_mask;
	long			 proto4_mask;
	Ximp_PreeditPropRec4	 preedit_attr;
	char			*preedit_font;
	Ximp_StatusPropRec4	 status_attr;
	char			*status_font;
	XIMCallback		 error;
 	/* Extended Callback attribute */
	Bool			 use_lookup_choices;
	ICExtLookupAttributes	 lookup_attr;
	XIMCallback		 restart;
	XIMCallback		 destroy;

	Ximp_CBStatus		cbstatus ;
	/* for Local Processing */
	DefTree			*context;
	DefTree			*composed;
#ifdef BACKTRACK_WHEN_UNMATCHED
	XEvent			*saved_event;
	int			num_saved;
#endif

	void			*ictype;
} XICXimpRec;

/*
 * IC struct
 */

typedef struct _Ximp_XIC {
	XICMethods	 methods;
	XICCoreRec	 core;
	XICXimpRec	*ximp_icpart;
} Ximp_XICRec;

/*
 * predicate argument
 */

typedef struct {
	Atom	type;
	Window	owner;
	int	protocol;
	ICID	icid;
} XimpCMPredicateArgRec, *XimpCMPredicateArg;

typedef struct {
	Atom	type;
	Window	owner;
	ICID	icid;
	Window	window;
	Atom	atom;
} XimpPNPredicateArgRec, *XimpPNPredicateArg;

typedef struct {
	int	proto3;
	int	proto4;
} XimpChangeMaskRec, *XimpChangeaMask;

#define XIMP_INPUT_STYLE	0x0001
#define XIMP_CLIENT_WIN		0x0002
#define XIMP_RES_NAME		0x0004
#define XIMP_RES_CLASS		0x0008
#define XIMP_GEOMETRY_CB        0x0010
#define XIMP_FILTER_EV          0x0020
#define XIMP_PRE_CALLBAK        0x0040
#define XIMP_STS_CALLBAK        0x0080

#define XIMP_CHK_FOCUSWINMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_FOCUS_WIN_MASK4)
#define XIMP_CHK_PREAREAMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_PRE_AREA_MASK4)
#define XIMP_CHK_PREAREANEEDMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_PRE_AREANEED_MASK4)
#define XIMP_CHK_PRECOLORMAPMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_PRE_COLORMAP_MASK4)
#define XIMP_CHK_PRESTDCOLORMAPMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_PRE_STD_COLORMAP_MASK4)
#define XIMP_CHK_PREFGMASK(ic)			(ic->ximp_icpart->proto4_mask & XIMP_PRE_FG_MASK4)
#define XIMP_CHK_PREBGMASK(ic)			(ic->ximp_icpart->proto4_mask & XIMP_PRE_BG_MASK4)
#define XIMP_CHK_PREBGPIXMAPMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_PRE_BGPIXMAP_MASK4)
#define XIMP_CHK_PRELINESPMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_PRE_LINESP_MASK4)
#define XIMP_CHK_PRECURSORMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_PRE_CURSOR_MASK4)
#define XIMP_CHK_PRESPOTLMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_PRE_SPOTL_MASK4)
#define XIMP_CHK_STSAREAMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_STS_AREA_MASK4)
#define XIMP_CHK_STSAREANEEDMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_STS_AREANEED_MASK4)
#define XIMP_CHK_STSCOLORMAPMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_STS_COLORMAP_MASK4)
#define XIMP_CHK_STSSTDCOLORMAPMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_STS_STD_COLORMAP_MASK4)
#define XIMP_CHK_STSFGMASK(ic)			(ic->ximp_icpart->proto4_mask & XIMP_STS_FG_MASK4)
#define XIMP_CHK_STSBGMASK(ic)			(ic->ximp_icpart->proto4_mask & XIMP_STS_BG_MASK4)
#define XIMP_CHK_STSBGPIXMAPMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_STS_BGPIXMAP_MASK4)
#define XIMP_CHK_STSLINESPMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_STS_LINESP_MASK4)
#define XIMP_CHK_STSCURSORMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_STS_CURSOR_MASK4)
#define XIMP_CHK_STSWINDOWMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_STS_WINDOW_MASK4)
#define XIMP_CHK_PREFONTMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_PRE_FONT_MASK4)
#define XIMP_CHK_STSFONTMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_STS_FONT_MASK4)
#define XIMP_CHK_SERVERTYPEMASK(ic)		(ic->ximp_icpart->proto4_mask & XIMP_SERVERTYPE_MASK4)

#define XIMP_CHK_PROP_FOCUS(mask)		( (mask.proto4) & XIMP_FOCUS_WIN_MASK4 )
#define XIMP_CHK_PROP_PREEDIT(mask)		( (mask.proto4) & \
						 (  XIMP_PRE_AREA_MASK4 \
						  | XIMP_PRE_AREANEED_MASK4 \
						  | XIMP_PRE_COLORMAP_MASK4 \
						  | XIMP_PRE_STD_COLORMAP_MASK4 \
						  | XIMP_PRE_FG_MASK4 \
						  | XIMP_PRE_BG_MASK4 \
						  | XIMP_PRE_BGPIXMAP_MASK4 \
						  | XIMP_PRE_LINESP_MASK4 \
						  | XIMP_PRE_CURSOR_MASK4 \
						  | XIMP_PRE_SPOTL_MASK4 ) )
#define XIMP_CHK_PROP_STATUS(mask)		( (mask.proto4) & \
						 (  XIMP_STS_AREA_MASK4 \
						  | XIMP_STS_AREANEED_MASK4 \
						  | XIMP_STS_COLORMAP_MASK4 \
						  | XIMP_STS_STD_COLORMAP_MASK4 \
						  | XIMP_STS_FG_MASK4 \
						  | XIMP_STS_BG_MASK4 \
						  | XIMP_STS_BGPIXMAP_MASK4 \
						  | XIMP_STS_LINESP_MASK4 \
						  | XIMP_STS_CURSOR_MASK4 \
						  | XIMP_STS_WINDOW_MASK4 ) )
#define XIMP_CHK_PROP_PREFONT(mask)		( (mask.proto4) & XIMP_PRE_FONT_MASK4 )
#define XIMP_CHK_PROP_STSFONT(mask)		( (mask.proto4) & XIMP_STS_FONT_MASK4 )
#define XIMP_EQU_PRESPOTLMASK(mask)		( (mask.proto4) == XIMP_PRE_SPOTL_MASK4 )

#define XIMP_UNSET_PROPFOCUS(mask)		{ mask.proto4 &= ~(XIMP_FOCUS_WIN_MASK4); \
						  mask.proto3 &= ~(XIMP_FOCUS_WIN_MASK3); }
#define XIMP_UNSET_PROPPREEDIT(mask)		{ mask.proto4 &= ~( \
						 (  XIMP_PRE_AREA_MASK4 \
						  | XIMP_PRE_AREANEED_MASK4 \
						  | XIMP_PRE_COLORMAP_MASK4 \
						  | XIMP_PRE_STD_COLORMAP_MASK4 \
						  | XIMP_PRE_FG_MASK4 \
						  | XIMP_PRE_BG_MASK4 \
						  | XIMP_PRE_BGPIXMAP_MASK4 \
						  | XIMP_PRE_LINESP_MASK4 \
						  | XIMP_PRE_CURSOR_MASK4 \
						  | XIMP_PRE_SPOTL_MASK4 \
						  | XIMP_PRE_FONT_MASK4 ) ); \
						  mask.proto3 &= ~( \
						 (  XIMP_PRE_AREA_MASK3 \
						  | XIMP_PRE_AREANEED_MASK3 \
						  | XIMP_PRE_COLORMAP_MASK3 \
						  | XIMP_PRE_FG_MASK3 \
						  | XIMP_PRE_BG_MASK3 \
						  | XIMP_PRE_BGPIXMAP_MASK3 \
						  | XIMP_PRE_LINESP_MASK3 \
						  | XIMP_PRE_CURSOR_MASK3 \
						  | XIMP_PRE_SPOTL_MASK3 \
						  | XIMP_PRE_FONT_MASK4 ) ); }
#define XIMP_UNSET_PROPSTATUS(mask)		{ mask.proto4 &= ~( \
						 (  XIMP_STS_AREA_MASK4 \
						  | XIMP_STS_AREANEED_MASK4 \
						  | XIMP_STS_COLORMAP_MASK4 \
						  | XIMP_STS_STD_COLORMAP_MASK4 \
						  | XIMP_STS_FG_MASK4 \
						  | XIMP_STS_BG_MASK4 \
						  | XIMP_STS_BGPIXMAP_MASK4 \
						  | XIMP_STS_LINESP_MASK4 \
						  | XIMP_STS_CURSOR_MASK4 \
						  | XIMP_STS_WINDOW_MASK4 \
						  | XIMP_STS_FONT_MASK4 ) ); \
						  mask.proto3 &= ~( \
						 (  XIMP_STS_AREA_MASK3 \
						  | XIMP_STS_AREANEED_MASK3 \
						  | XIMP_STS_COLORMAP_MASK3 \
						  | XIMP_STS_FG_MASK3 \
						  | XIMP_STS_BG_MASK3 \
						  | XIMP_STS_BGPIXMAP_MASK3 \
						  | XIMP_STS_LINESP_MASK3 \
						  | XIMP_STS_CURSOR_MASK3 \
						  | XIMP_STS_WINDOW_MASK3 \
						  | XIMP_STS_FONT_MASK3 ) ); }

#define XIMP_SET_NULLMASK(mask)			{ mask.proto4                 = NULL; \
						  mask.proto3                 = NULL; }
#define XIMP_SET_NULLMASK2(mask)		{ mask->proto4                 = NULL; \
						  mask->proto3                 = NULL; }

#define XIMP_SET_FOCUSWINMASK(ic)		{ ic->ximp_icpart->proto4_mask |= XIMP_FOCUS_WIN_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_FOCUS_WIN_MASK3; }

#define XIMP_SET_FOCUSWINMASK2(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_FOCUS_WIN_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_FOCUS_WIN_MASK3; \
						  mask->proto4                 |= XIMP_FOCUS_WIN_MASK4; \
						  mask->proto3                 |= XIMP_FOCUS_WIN_MASK3; }
#define XIMP_SET_PREAREAMASK(ic, mask)		{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_AREA_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_AREA_MASK3; \
						  mask->proto4                 |= XIMP_PRE_AREA_MASK4; \
						  mask->proto3                 |= XIMP_PRE_AREA_MASK3; }
#define XIMP_SET_PREAREANEEDMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_AREANEED_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_AREANEED_MASK3; \
						  mask->proto4                 |= XIMP_PRE_AREANEED_MASK4; \
						  mask->proto3                 |= XIMP_PRE_AREANEED_MASK3; }
#define XIMP_SET_PRECOLORMAPMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_COLORMAP_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_COLORMAP_MASK3; \
						  mask->proto4                 |= XIMP_PRE_COLORMAP_MASK4; \
						  mask->proto3                 |= XIMP_PRE_COLORMAP_MASK3; }
#define XIMP_SET_PRESTDCOLORMAPMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_STD_COLORMAP_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_COLORMAP_MASK3; \
						  mask->proto4                 |= XIMP_PRE_STD_COLORMAP_MASK4; \
						  mask->proto3                 |= XIMP_PRE_COLORMAP_MASK3; }
#define XIMP_SET_PREFGMASK(ic, mask)		{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_FG_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_FG_MASK3; \
						  mask->proto4                 |= XIMP_PRE_FG_MASK4; \
						  mask->proto3                 |= XIMP_PRE_FG_MASK3; }
#define XIMP_SET_PREBGMASK(ic, mask)		{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_BG_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_BG_MASK3; \
						  mask->proto4                 |= XIMP_PRE_BG_MASK4; \
						  mask->proto3                 |= XIMP_PRE_BG_MASK3; }
#define XIMP_SET_PREBGPIXMAPMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_BGPIXMAP_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_BGPIXMAP_MASK3; \
						  mask->proto4                 |= XIMP_PRE_BGPIXMAP_MASK4; \
						  mask->proto3                 |= XIMP_PRE_BGPIXMAP_MASK3; }
#define XIMP_SET_PRELINESPMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_LINESP_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_LINESP_MASK3; \
						  mask->proto4                 |= XIMP_PRE_LINESP_MASK4; \
						  mask->proto3                 |= XIMP_PRE_LINESP_MASK3; }
#define XIMP_SET_PRECURSORMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_CURSOR_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_CURSOR_MASK3; \
						  mask->proto4                 |= XIMP_PRE_CURSOR_MASK4; \
						  mask->proto3                 |= XIMP_PRE_CURSOR_MASK3; }
#define XIMP_SET_PRESPOTLMASK(ic, mask)		{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_SPOTL_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_SPOTL_MASK3; \
						  mask->proto4                 |= XIMP_PRE_SPOTL_MASK4; \
						  mask->proto3                 |= XIMP_PRE_SPOTL_MASK3; }

#define XIMP_SET_STSAREAMASK(ic, mask)		{ ic->ximp_icpart->proto4_mask |= XIMP_STS_AREA_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_AREA_MASK3; \
						  mask->proto4                 |= XIMP_STS_AREA_MASK4; \
						  mask->proto3                 |= XIMP_STS_AREA_MASK3; }
#define XIMP_SET_STSAREANEEDMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_STS_AREANEED_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_AREANEED_MASK3; \
						  mask->proto4                 |= XIMP_STS_AREANEED_MASK4; \
						  mask->proto3                 |= XIMP_STS_AREANEED_MASK3; }
#define XIMP_SET_STSCOLORMAPMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_STS_COLORMAP_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_COLORMAP_MASK3; \
						  mask->proto4                 |= XIMP_STS_COLORMAP_MASK4; \
						  mask->proto3                 |= XIMP_STS_COLORMAP_MASK3; }
#define XIMP_SET_STSSTDCOLORMAPMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_STS_STD_COLORMAP_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_COLORMAP_MASK3; \
						  mask->proto4                 |= XIMP_STS_STD_COLORMAP_MASK4; \
						  mask->proto3                 |= XIMP_STS_COLORMAP_MASK3; }
#define XIMP_SET_STSFGMASK(ic, mask)		{ ic->ximp_icpart->proto4_mask |= XIMP_STS_FG_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_FG_MASK3; \
						  mask->proto4                 |= XIMP_STS_FG_MASK4; \
						  mask->proto3                 |= XIMP_STS_FG_MASK3; }
#define XIMP_SET_STSBGMASK(ic, mask)		{ ic->ximp_icpart->proto4_mask |= XIMP_STS_BG_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_BG_MASK3; \
						  mask->proto4                 |= XIMP_STS_BG_MASK4; \
						  mask->proto3                 |= XIMP_STS_BG_MASK3; }
#define XIMP_SET_STSBGPIXMAPMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_STS_BGPIXMAP_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_BGPIXMAP_MASK3; \
						  mask->proto4                 |= XIMP_STS_BGPIXMAP_MASK4; \
						  mask->proto3                 |= XIMP_STS_BGPIXMAP_MASK3; }
#define XIMP_SET_STSLINESPMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_STS_LINESP_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_LINESP_MASK3; \
						  mask->proto4                 |= XIMP_STS_LINESP_MASK4; \
						  mask->proto3                 |= XIMP_STS_LINESP_MASK3; }
#define XIMP_SET_STSCURSORMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_STS_CURSOR_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_CURSOR_MASK3; \
						  mask->proto4                 |= XIMP_STS_CURSOR_MASK4; \
						  mask->proto3                 |= XIMP_STS_CURSOR_MASK3; }
#define XIMP_SET_STSWINDOWMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_STS_WINDOW_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_WINDOW_MASK3; \
						  mask->proto4                 |= XIMP_STS_WINDOW_MASK4; \
						  mask->proto3                 |= XIMP_STS_WINDOW_MASK3; }

#define XIMP_SET_PREFONTMASK(ic, mask)		{ ic->ximp_icpart->proto4_mask |= XIMP_PRE_FONT_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_PRE_FONT_MASK3; \
						  mask->proto4                 |= XIMP_PRE_FONT_MASK4; \
						  mask->proto3                 |= XIMP_PRE_FONT_MASK3; }
#define XIMP_SET_STSFONTMASK(ic, mask)		{ ic->ximp_icpart->proto4_mask |= XIMP_STS_FONT_MASK4; \
						  ic->ximp_icpart->proto3_mask |= XIMP_STS_FONT_MASK3; \
						  mask->proto4                 |= XIMP_STS_FONT_MASK4; \
						  mask->proto3                 |= XIMP_STS_FONT_MASK3; }
#define XIMP_SET_SERVERTYPEMASK(ic, mask)	{ ic->ximp_icpart->proto4_mask |= XIMP_SERVERTYPE_MASK4; \
						  mask->proto4                 |= XIMP_SERVERTYPE_MASK4; }

#define XIMP_PROTO_MASK(ic, mask)	(ISXimp4(ic)?mask.proto4:mask.proto3)
#define XIMP_PROTO_MASK2(ic)		(ISXimp4(ic)?ic->ximp_icpart->proto4_mask:ic->ximp_icpart->proto3_mask)

#define XIMP_FOCUS_WIN_MASK(ic)		(ISXimp4(ic)?XIMP_FOCUS_WIN_MASK4:XIMP_FOCUS_WIN_MASK3)
#define XIMP_PRE_AREA_MASK(ic)		(ISXimp4(ic)?XIMP_PRE_AREA_MASK4:XIMP_PRE_AREA_MASK3)
#define XIMP_PRE_AREANEED_MASK(ic)	(ISXimp4(ic)?XIMP_PRE_AREANEED_MASK4:XIMP_PRE_AREANEED_MASK3)
#define XIMP_PRE_COLORMAP_MASK(ic)	(ISXimp4(ic)?XIMP_PRE_COLORMAP_MASK4:XIMP_PRE_COLORMAP_MASK3)
#define XIMP_PRE_STD_COLORMAP_MASK(ic)	(ISXimp4(ic)?XIMP_PRE_STD_COLORMAP_MASK4:XIMP_PRE_COLORMAP_MASK3)
#define XIMP_PRE_FG_MASK(ic)		(ISXimp4(ic)?XIMP_PRE_FG_MASK4:XIMP_PRE_FG_MASK3)
#define XIMP_PRE_BG_MASK(ic)		(ISXimp4(ic)?XIMP_PRE_BG_MASK4:XIMP_PRE_BG_MASK3)
#define XIMP_PRE_BGPIXMAP_MASK(ic)	(ISXimp4(ic)?XIMP_PRE_BGPIXMAP_MASK4:XIMP_PRE_BGPIXMAP_MASK3)
#define XIMP_PRE_LINESP_MASK(ic)	(ISXimp4(ic)?XIMP_PRE_LINESP_MASK4:XIMP_PRE_LINESP_MASK3)
#define XIMP_PRE_CURSOR_MASK(ic)	(ISXimp4(ic)?XIMP_PRE_CURSOR_MASK4:XIMP_PRE_CURSOR_MASK3)
#define XIMP_PRE_SPOTL_MASK(ic)		(ISXimp4(ic)?XIMP_PRE_SPOTL_MASK4:XIMP_PRE_SPOTL_MASK3)
#define XIMP_STS_AREA_MASK(ic)		(ISXimp4(ic)?XIMP_STS_AREA_MASK4:XIMP_STS_AREA_MASK3)
#define XIMP_STS_AREANEED_MASK(ic)	(ISXimp4(ic)?XIMP_STS_AREANEED_MASK4:XIMP_STS_AREANEED_MASK3)
#define XIMP_STS_COLORMAP_MASK(ic)	(ISXimp4(ic)?XIMP_STS_COLORMAP_MASK4:XIMP_STS_COLORMAP_MASK3)
#define XIMP_STS_STD_COLORMAP_MASK(ic)	(ISXimp4(ic)?XIMP_STS_STD_COLORMAP_MASK4:XIMP_STS_COLORMAP_MASK3)
#define XIMP_STS_FG_MASK(ic)		(ISXimp4(ic)?XIMP_STS_FG_MASK4:XIMP_STS_FG_MASK3)
#define XIMP_STS_BG_MASK(ic)		(ISXimp4(ic)?XIMP_STS_BG_MASK4:XIMP_STS_BG_MASK3)
#define XIMP_STS_BGPIXMAP_MASK(ic)	(ISXimp4(ic)?XIMP_STS_BGPIXMAP_MASK4:XIMP_STS_BGPIXMAP_MASK3)
#define XIMP_STS_LINESP_MASK(ic)	(ISXimp4(ic)?XIMP_STS_LINESP_MASK4:XIMP_STS_LINESP_MASK3)
#define XIMP_STS_CURSOR_MASK(ic)	(ISXimp4(ic)?XIMP_STS_CURSOR_MASK4:XIMP_STS_CURSOR_MASK3)
#define XIMP_STS_WINDOW_MASK(ic)	(ISXimp4(ic)?XIMP_STS_WINDOW_MASK4:XIMP_STS_WINDOW_MASK3)
#define XIMP_PRE_FONT_MASK(ic)		(ISXimp4(ic)?XIMP_PRE_FONT_MASK4:XIMP_PRE_FONT_MASK3)
#define XIMP_STS_FONT_MASK(ic)		(ISXimp4(ic)?XIMP_STS_FONT_MASK4:XIMP_STS_FONT_MASK3)

#define XIMP_PROP_FOCUS(ic)	( XIMP_FOCUS_WIN_MASK(ic) )
#define XIMP_PROP_PREEDIT(ic)	( XIMP_PRE_AREA_MASK(ic) \
				| XIMP_PRE_FG_MASK(ic) \
				| XIMP_PRE_BG_MASK(ic) \
				| XIMP_PRE_COLORMAP_MASK(ic) \
				| XIMP_PRE_BGPIXMAP_MASK(ic) \
				| XIMP_PRE_LINESP_MASK(ic) \
				| XIMP_PRE_CURSOR_MASK(ic) \
				| XIMP_PRE_AREANEED_MASK(ic) \
				| XIMP_PRE_SPOTL_MASK(ic) )
#define XIMP_PROP_STATUS(ic)	( XIMP_STS_AREA_MASK(ic) \
				| XIMP_STS_FG_MASK(ic) \
				| XIMP_STS_BG_MASK(ic) \
				| XIMP_STS_COLORMAP_MASK(ic) \
				| XIMP_STS_BGPIXMAP_MASK(ic) \
				| XIMP_STS_LINESP_MASK(ic) \
				| XIMP_STS_CURSOR_MASK(ic) \
				| XIMP_STS_AREANEED_MASK(ic) \
				| XIMP_STS_WINDOW_MASK(ic) )
#define XIMP_PROP_PREFONT(ic)	( XIMP_PRE_FONT_MASK(ic) )
#define XIMP_PROP_STSFONT(ic)	( XIMP_STS_FONT_MASK(ic) )

#define XIMP_NOCONNECT		0x0000
#define XIMP_DELAYBINDABLE	0x0001
#define XIMP_RECONNECTABLE	0x0002
#define XIMP_RESTARTABLE	0x0004

#define IS_FORCESELECTKEYRELEASE(im)        (((Ximp_XIM)(im))->ximp_impart->is_forceselectkeyrelease)

#define ISXimp4IM(im) 		(im->ximp_impart->im_proto_vnum >= XIMP_VERSION_NUMBER) 
#define RECONNECTION_MODE(im) 	(((Ximp_XIM)(im))->ximp_impart->reconnection_mode)
#define IS_SERVER_CONNECTED(im)	(((Ximp_XIM)(im))->ximp_impart->is_connected)
#define IS_LOCAL_PROCESSING(im) (((Ximp_XIM)(im))->ximp_impart->is_local)

#define IS_IC_CONNECTED(ic)	(ic->ximp_icpart->icid)

#define IS_RECONNECTABLE(im)   (RECONNECTION_MODE(im) & XIMP_RECONNECTABLE)
#define MAKE_RECONNECTABLE(im) (RECONNECTION_MODE(im) |= XIMP_RECONNECTABLE)
#define IS_DELAYBINDABLE(im)   (RECONNECTION_MODE(im) & XIMP_DELAYBINDABLE)
#define MAKE_DELAYBINDABLE(im) (RECONNECTION_MODE(im) |= XIMP_DELAYBINDABLE)
#define IS_RESTARTABLE(im)     (RECONNECTION_MODE(im) & XIMP_RESTARTABLE)
#define MAKE_RESTARTABLE(im)   (RECONNECTION_MODE(im) |= XIMP_RESTARTABLE)
#define IS_UNCONNECTABLE(im)   (RECONNECTION_MODE(im) == XIMP_NOCONNECT)
#define MAKE_UNCONNECTABLE(im) (RECONNECTION_MODE(im) = XIMP_NOCONNECT)
#define IS_CONNECTABLE(im)     (RECONNECTION_MODE(im) != XIMP_NOCONNECT)
#define MAKE_CONNECTABLE(im)   (RECONNECTION_MODE(im) = XIMP_RECONNECTABLE|XIMP_DELAYBINDABLE|XIMP_RESTARTABLE)

#define ISXimp4(ic)   (((Ximp_XIM)ic->core.im)->ximp_impart->im_proto_vnum >= XIMP_VERSION_NUMBER)

#define ISFE1(ic)     (((Ximp_XIC)ic)->ximp_icpart->svr_mode == XIMP_FE_TYPE1)
#define ISFE2(ic)     (((Ximp_XIC)ic)->ximp_icpart->svr_mode == XIMP_FE_TYPE2)
#define ISFE3(ic)     (((Ximp_XIC)ic)->ximp_icpart->svr_mode == XIMP_FE_TYPE3)
#define ISBE1(ic)     ((((Ximp_XIC)ic)->ximp_icpart->svr_mode & XIMP_BE_TYPE1) == XIMP_BE_TYPE1)
#define ISBE2(ic)     ((((Ximp_XIC)ic)->ximp_icpart->svr_mode & XIMP_BE_TYPE2) == XIMP_BE_TYPE2)
#define ISSYNCBE1(ic) (((Ximp_XIC)ic)->ximp_icpart->svr_mode == XIMP_SYNC_BE_TYPE1)
#define ISSYNCBE2(ic) (((Ximp_XIC)ic)->ximp_icpart->svr_mode == XIMP_SYNC_BE_TYPE2)
#define ISFRONTEND(ic) (((Ximp_XIC)ic)->ximp_icpart->svr_mode & XIMP_FRONTEND4)
#define ISBACKEND(ic) (((Ximp_XIC)ic)->ximp_icpart->svr_mode & XIMP_BACKEND4)
#define ISTYPE1(ic)   (((Ximp_XIC)ic)->ximp_icpart->svr_mode & XIMP_TYPE1)
#define ISTYPE2(ic)   (((Ximp_XIC)ic)->ximp_icpart->svr_mode & XIMP_TYPE2)
#define ISTYPE3(ic)   (((Ximp_XIC)ic)->ximp_icpart->svr_mode & XIMP_TYPE3)
#define ISSYNC(ic)    (((Ximp_XIC)ic)->ximp_icpart->svr_mode & XIMP_SYNC)

#define ISXIMP3FE(ic) ((((Ximp_XIC)ic)->ximp_icpart->svr_mode & 0x03) == XIMP_FRONTEND_BC_MASK)

#define IS_BEING_PREEDITED(ic)	(((Ximp_XIC)ic)->ximp_icpart->input_mode == BEING_PREEDITED)
#define IS_FABLICATED(ic,ev)	((ic->ximp_icpart->putback_key_event) || (ev->keycode == 0))
#define IS_USE_WCHAR(ic)	(((Ximp_XIM)ic->core.im)->ximp_impart->use_wchar)
    
#define ISCMOf(ev,n,x)		(((XEvent*)ev)->xclient.data.l[n] == (x))
#define ISXIMP_ERROR(ev)	(((XEvent*)ev)->type == ClientMessage && ((XEvent*)ev)->xclient.format == 32 \
				  && (ISCMOf(ev,0,XIMP_ERROR3) || ISCMOf(ev,0,XIMP_ERROR4)))
#define XIMP_SYNCHRONIZE(ic)	 {if(IS_SERVER_CONNECTED((ic->core.im)) && (IS_BEING_PREEDITED(ic)) &&\
				      ((((Ximp_XIC)ic)->ximp_icpart->svr_mode) & XIMP_SYNC ))\
				      _Ximp_Synchronize(ic);}

#define XIMP_PREEDIT_MAX_LONG(ic) (ISXimp4(ic)?XIMP_PREEDIT_MAX_LONG4:XIMP_PREEDIT_MAX_LONG3)
#define XIMP_STATUS_MAX_LONG(ic)  (ISXimp4(ic)?XIMP_STATUS_MAX_LONG4:XIMP_STATUS_MAX_LONG3)

#define  XIMP3_RESET_RETURN_ATOM    2
#define  XIMP4_RESET_RETURN_DETAIL  2
#define  XIMP4_RESET_RETURN_ATOM    3
#define  NO_RESET_DATA              0
#define  RESET_DATA_VIA_CM          1
#define  RESET_DATA_VIA_PROP        2
#define  XIMP_RESET_RETURN_ATOM(ic) (ISXimp4(ic)?XIMP4_RESET_RETURN_ATOM:XIMP3_RESET_RETURN_ATOM)

#define  XIMP_KEYRELEASE(ic)		XIMP_KEYRELEASE4
#define  XIMP_KEYPRESS(ic)		(ISXimp4(ic)?XIMP_KEYPRESS4:XIMP_KEYPRESS3)

#define  XIMP_CREATE(ic)		(ISXimp4(ic)?XIMP_CREATE4:XIMP_CREATE3)
#define  XIMP_DESTROY(ic)		(ISXimp4(ic)?XIMP_DESTROY4:XIMP_DESTROY3)
#define  XIMP_REG_KEY_PRESSED(ic)	(ISXimp4(ic)?XIMP_REG_KEY_PRESSED4:XIMP_BEGIN3)
#define  XIMP_SETFOCUS(ic)		(ISXimp4(ic)?XIMP_SETFOCUS4:XIMP_SETFOCUS3)
#define  XIMP_UNSETFOCUS(ic)		(ISXimp4(ic)?XIMP_UNSETFOCUS4:XIMP_UNSETFOCUS3)
#define  XIMP_CLIENT_WINDOW(ic)		XIMP_CLIENT_WINDOW4
#define  XIMP_FOCUS_WINDOW(ic)		XIMP_FOCUS_WINDOW4
#define  XIMP_MOVE(ic)			(ISXimp4(ic)?XIMP_MOVE4:XIMP_MOVE3)
#define  XIMP_RESET(ic)			(ISXimp4(ic)?XIMP_RESET4:XIMP_RESET3)
#define  XIMP_SETVALUE(ic)		(ISXimp4(ic)?XIMP_SETVALUE4:XIMP_SETVALUE3)
#define  XIMP_GETVALUE(ic)		(ISXimp4(ic)?XIMP_GETVALUE4:XIMP_GETVALUE3)

#define  XIMP_PREEDITSTART_RETURN(ic)	(ISXimp4(ic)?XIMP_PREEDITSTART_RETURN4:XIMP_PREEDITSTART_RETURN3)
#define  XIMP_PREEDITCARET_RETURN(ic)	(ISXimp4(ic)?XIMP_PREEDITCARET_RETURN4:XIMP_PREEDITCARET_RETURN3)
#define  XIMP_SPROC_STARTED(ic)		(ISXimp4(ic)?XIMP_SPROC_STARTED4:XIMP_PROCESS_BEGIN3)
#define  XIMP_SPROC_STOPPED(ic)		(ISXimp4(ic)?XIMP_SPROC_STOPPED4:XIMP_PROCESS_END3)
#define  XIMP_READPROP(ic)		(ISXimp4(ic)?XIMP_READPROP4:XIMP_READPROP3)
#define  XIMP_CLIENT_WINDOW_RETURN(ic)	XIMP_CLIENT_WINDOW_RETURN4
#define  XIMP_FOCUS_WINDOW_RETURN(ic)	XIMP_FOCUS_WINDOW_RETURN4
#define  XIMP_GETVALUE_RETURN(ic)	(ISXimp4(ic)?XIMP_GETVALUE_RETURN4:XIMP_GETVALUE_RETURN3)
#define  XIMP_RESET_RETURN(ic)		(ISXimp4(ic)?XIMP_RESET_RETURN4:XIMP_RESET_RETURN3)
#define  XIMP_CREATE_RETURN(ic)		(ISXimp4(ic)?XIMP_CREATE_RETURN4:XIMP_CREATE_RETURN3)
#define  XIMP_KEYPRESS_RETURN(ic)	XIMP_KEYPRESS_RETURN4
#define  XIMP_KEYRELEASE_RETURN(ic)	XIMP_KEYRELEASE_RETURN4
#define  XIMP_EVENTMASK_NOTIFY(ic)	XIMP_EVENTMASK_NOTIFY4
#define  XIMP_EVENTMASK_NOTIFY_RETURN(ic)	XIMP_EVENTMASK_NOTIFY_RETURN4

#define  XIMP_GEOMETRY(ic)		(ISXimp4(ic)?XIMP_GEOMETRY4:XIMP_GEOMETRY3)
#define  XIMP_PREEDITSTART(ic)		(ISXimp4(ic)?XIMP_PREEDITSTART4:XIMP_PREEDITSTART3)
#define  XIMP_PREEDITDONE(ic)		(ISXimp4(ic)?XIMP_PREEDITDONE4:XIMP_PREEDITDONE3)
#define  XIMP_PREEDITDRAW(ic)		(ISXimp4(ic)?XIMP_PREEDITDRAW4:XIMP_PREEDITDRAW3)
#define  XIMP_PREEDITDRAW_CM(ic)	(ISXimp4(ic)?XIMP_PREEDITDRAW_CM4:XIMP_PREEDITDRAW_CM3)
#define  XIMP_PREEDITDRAW_CM_TINY(ic)	(ISXimp4(ic)?XIMP_PREEDITDRAW_CM_TINY4:XIMP_PREEDITDRAW_CM_TINY3)
#define  XIMP_PREEDITCARET(ic)		(ISXimp4(ic)?XIMP_PREEDITCARET4:XIMP_PREEDITCARET3)
#define  XIMP_STATUSSTART(ic)		(ISXimp4(ic)?XIMP_STATUSSTART4:XIMP_STATUSSTART3)
#define  XIMP_STATUSDONE(ic)		(ISXimp4(ic)?XIMP_STATUSDONE4:XIMP_STATUSDONE3)
#define  XIMP_STATUSDRAW(ic)		(ISXimp4(ic)?XIMP_STATUSDRAW4:XIMP_STATUSDRAW3)
#define  XIMP_STATUSDRAW_CM(ic)		(ISXimp4(ic)?XIMP_STATUSDRAW_CM4:XIMP_STATUSDRAW_CM3)
#define  XIMP_EXTENSION(ic)		(ISXimp4(ic)?XIMP_EXTENSION4:XIMP_EXTENSION3)

