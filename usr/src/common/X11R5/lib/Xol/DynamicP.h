/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#ident	"@(#)olmisc:DynamicP.h	1.22"
#endif

#ifndef __DynamicP_h__
#define __DynamicP_h__

#include <Xol/Dynamic.h>
#include <Xol/VendorI.h>

#define XtROlKeyDef		"OlKeyDef"
#define XtROlBtnDef		"OlBtnDef"

#define MAXDEFS			2
#define MORESLOTS               4
#define BUFFER_SIZE		64

#ifndef MEMUTIL
#define Xfree(x)		{ FREE(x); x = NULL; }
#endif /* MEMUTIL */
#define ABS_DELTA(x1, x2)	(x1 < x2 ? x2 - x1 : x1 - x2)

#define IsDampableKey(flag, k)	(flag == True && IsCursorKey(k))

#define CanBeBound(flag,keysym,modifier) \
				(flag == False || keysym >= 0x1000 || \
				(modifier & ~(ShiftMask | LockMask)))

#define DEF_SEPS		","	/* I18N */
#define MOD_SEPS		"!~ "	/* I18N */
#define LBRA			'<'	/* I18N */
#define RBRA			'>'	/* I18N */

#if	!defined(STREQU)
# define STREQU(A,B)	(strcmp((A),(B)) == 0)
#endif

#if	!defined(Malloc)
# define Malloc(N) XtMalloc(N)
#endif

#if	!defined(Realloc)
# define Realloc(P,N) XtRealloc(P,N)
#endif

#if	!defined(New)
# define New(M) XtNew(M)
#endif

#if	!defined(Free)
# define Free(M) XtFree(M)
#endif

#if	!defined(Strlen)
# define Strlen(S) ((S) && *(S)? strlen((S)) : 0)
#endif

#if	!defined(Strdup)
# define Strdup(S) strcpy(Malloc((unsigned)Strlen(S) + 1), S)
#endif

#if	!defined(Array)
# define Array(P,T,N) \
	((N)? \
		  ((P)? \
			  (T *)Realloc((char *)(P), sizeof(T) * (N)) \
			: (T *)Malloc(sizeof(T) * (N)) \
		  ) \
		: ((P)? (Free((char *)P),(T *)0) : (T *)0) \
	)
#endif

typedef unsigned int	BtnSym;

typedef struct _OlKeyDef {
	int		used;
	Modifiers	modifier[MAXDEFS];
	KeySym		keysym[MAXDEFS];
} OlKeyDef;

typedef struct _OlBtnDef {
	int		used;
	Modifiers	modifier[MAXDEFS];
	BtnSym		button[MAXDEFS];
} OlBtnDef;

typedef struct _OlKeyBinding {
	OLconst char *	name;           /* XtN string */
	OLconst char *	default_value;  /* `,' sperated string, two most */
	OlInputEvent	ol_event;
	OlKeyDef	def;
} OlKeyBinding;

typedef struct _OlBtnBinding {
	OLconst char *	name;           /* XtN string */
	OLconst char *	default_value;  /* `,' sperated string, two most */
	OlInputEvent	ol_event;
	OlBtnDef	def;
} OlBtnBinding;

typedef struct mapping {
	OLconst String			s;
	OLconst unsigned long		m;
} mapping;

typedef struct _btn_mapping {
	OLconst unsigned long		button;
	OLconst unsigned long		button_mask;
} btn_mapping;

typedef struct _Token {
	short			i;
	short			j;
} Token;

typedef struct _OlVirtualEventInfo {
	OlKeyBinding *		key_bindings;
	OlBtnBinding *		btn_bindings;
	char			num_key_bindings;
	char			num_btn_bindings;
	Token *			sorted_key_db;
} OlVirtualEventInfo;

typedef struct _OlClassSearchRec {
	WidgetClass		wc;
	OlVirtualEventInfo *	db;
} OlClassSearchRec, *OlClassSearchInfo;

typedef struct _OlWidgetSearchRec {
	Widget			w;
	OlVirtualEventInfo *	db;
} OlWidgetSearchRec, *OlWidgetSearchInfo;

typedef struct GrabbedVirtualKey {
	Widget			w;
	OlVirtualName		vkey;
	OlKeyBinding *		kb;
	OlKeyDef		as_grabbed;
	Boolean			grabbed;
	Boolean			owner_events;
	int			pointer_mode;
	int			keyboard_mode;
} GrabbedVirtualKey;

typedef struct {
	Cardinal		mouse_damping_factor;
	Cardinal		multi_click_timeout;
	Modifiers		dont_care_bits;
	Cardinal		key_remap_timeout;
	Modifiers		key_dont_care_bits;
} LocalData;

typedef struct _DynamicCallback {
	OlDynamicCallbackProc	CB;
	XtPointer		data;
} DynamicCallback;

typedef struct CharKeysymMap {
	OLconst char			single;
	OLconst KeySym			keysym;
} CharKeysymMap;

OLBeginFunctionPrototypeBlock

extern void OlGetOlKeysForIm OL_ARGS((OlMAndAList **, int *, int *));

extern void OlGetMAndAList OL_ARGS((OlVendorPartExtension, OlMAndAList **,
					int  *, Boolean));

extern OlVirtualName _OlKeySymToVirtualKeyName OL_ARGS((KeySym,
					Modifiers, String *));

OLEndFunctionPrototypeBlock

#endif /* __DynamicP_h__ */
