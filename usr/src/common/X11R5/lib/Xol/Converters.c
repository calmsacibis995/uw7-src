#ifndef	NOIDENT
#ident	"@(#)olmisc:Converters.c	1.48.1.31"
#endif


#include "stdio.h"
#include "ctype.h"
#include "string.h"
#include <sys/stat.h>
#include "X11/cursorfont.h"

/* for Internationalization */
#ifdef I18N

#ifndef sun	/* or other porting that does care I18N */
#include <sys/euc.h>
#endif

#endif

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"
#include "X11/CoreP.h"

#define XK_LATIN1
#include "X11/keysymdef.h"
#include "Xol/OpenLookI.h"
#include "Xol/EventObj.h"
#include "Xol/ConvertersI.h"
#include "Xol/DynamicP.h"
#include "Xol/xpm.h"

/*
 * Macros:
 */

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
# define Array(P,T,N)							\
	((N)?								\
		  ((P)?							\
			  (T *)Realloc((P), sizeof(T) * (N))		\
			: (T *)Malloc(sizeof(T) * (N))			\
		  )							\
		: ((P)? (Free(P),(T *)0) : (T *)0)			\
	)
#endif

/*
 * Delimiters for words in a resource file.
 */
#define WORD_SEPS		(OLconst char *)" \t\n"		/* I18N */

/*
 * Things that help us ignore upper/lower case.
 */
#define	CASELESS_STREQU(A,B)	(caseless_strcmp((A), (B)) == 0)
#define LOWER_IT(X) \
	if ((X >= XK_A) && (X <= XK_Z))					\
		X += (XK_a - XK_A);					\
	else if ((X >= XK_Agrave) && (X <= XK_Odiaeresis))		\
		X += (XK_agrave - XK_Agrave);				\
	else if ((X >= XK_Ooblique) && (X <= XK_Thorn))			\
		X += (XK_oslash - XK_Ooblique)


/* In regexp.c, used for SVR 3.2.2 */
extern char * strexp OL_ARGS((char *, char *,char * ));

/*
 * Local routines:
 */
static void		FetchFontGroupDef OL_ARGS((
	Widget			w,
	Cardinal *		size,
	XrmValue *		value
));
static Boolean		FetchPixmapFile OL_ARGS((
	Display *		dpy,
	Screen *		screen,
	XrmValue *		value,
	char *			buf
));
static Boolean		FetchBitmap OL_ARGS((
	Screen *		screen,
	Drawable 		d,
	char *			buf,
	int			depth,
	unsigned int *		w,
	unsigned int *		h,
        Pixmap *		p
));
static void		FreeFont OL_ARGS((
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,
	XrmValuePtr		args,
	Cardinal *		num_args
));
static void		FreeFontStruct OL_ARGS((
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,
	XrmValuePtr		args,
	Cardinal *		num_args
));
static void		FreeFontStructList OL_ARGS((
	XtAppContext		app,
	XrmValuePtr		to,
	XtPointer		closure,
	XrmValuePtr		args,
	Cardinal *		num_args
));
static Boolean		IsScaledInteger OL_ARGS((
	String			val,
	int *			pint,
	Screen *		screen
));
static Boolean		IsInteger OL_ARGS((
	String			val,
	int *			pint
));
static Boolean		IsOlBitMask OL_ARGS((
	String			val,
	OlBitMask *		mask
));
static Boolean		IsFont OL_ARGS((
	OLconst char * 		who,
	Screen *		screen,
	String			str,
	Font *			p_font,
	XtPointer *		p_converter_data,
	String	*		cset
));
static Boolean		MapAndLoadFont OL_ARGS((
	Display *		display,
	Screen *		screen,
	OLconst char *		str,
	String		*	cset,
	Font		*	p_font
));
static Font		LoadFont OL_ARGS((
	Display *		display,
	OLconst char *		name
));
static int		IgnoreBadName OL_ARGS((
	Display *		display,
	XErrorEvent *		event
));
static OlDefine		LookupOlDefine OL_ARGS((
	String			name,
	OlDefine	 	value,
	int			flag
));
static Boolean		FetchResource OL_ARGS((
	Display *		display,
	OLconst char *		name,
	OLconst char *		class,
	XrmRepresentation *	p_rep_type,
	XrmValue *		p_value
));
static int		caseless_strcmp OL_ARGS((
	OLconst char *		s1,
	OLconst char *		s2
));
static int		caseless_strncmp OL_ARGS((
	OLconst char *		s1,
	OLconst char *		s2,
	register int		n
));
static int		CompareISOLatin1 OL_ARGS((
	OLconst char *		first,
	OLconst char *		second,
	register int		count
));

static void		FreeCursor OL_ARGS((
	XtAppContext app,
	XrmValuePtr to,
	XtPointer closure,
	XrmValuePtr args,
	Cardinal * num_args
));

static void		FreePixmap OL_ARGS((
	XtAppContext app,
	XrmValuePtr to,
	XtPointer closure,
	XrmValuePtr args,
	Cardinal * num_args
));

static void		FreeImage OL_ARGS((
	XtAppContext app,
	XrmValuePtr to,
	XtPointer closure,
	XrmValuePtr args,
	Cardinal * num_args
));

/*
 * Local data:
 */

typedef struct Fraction {
	long			numerator;
	long			denominator;
}			Fraction;

#define DECPOINT	'.'		/* I18N */
#define DIGIT(x)	(long)((x)-'0')	/* I18N */
#define PLUS		'+'		/* I18N */
#define MINUS		'-'		/* I18N */

#define IsZeroFraction(F) ((F).numerator == 0)
#define IsNegativeFraction(F) ((F).numerator < 0)

static Fraction		NewFraction OL_ARGS(( int , int ));
static Fraction		StrToFraction OL_ARGS(( String , String * ));
static Fraction		MulFractions OL_ARGS(( Fraction , Fraction ));
static int		FractionToInt OL_ARGS(( Fraction ));

typedef struct Units {
	/*
	 * Multiply a number with this modifier (e.g. "1 inch")...
	 */
	OLconst char *		modifier;
	/*
	 * ...by this factor, to get the equivalent number of millimeters.
	 * [The special factor 0 means units are in pixels.]
	 */
	OLconst Fraction	factor;
	/*
	 * If this flag is set, the modifier sets the orientation.
	 */
	OLconst OlDefine	axis;
}			Units;

static OLconst Units	units[] = {
	{ "i",            254,10,    0 },	/* 25.4 */
	{ "in",           254,10,    0 },	/* 25.4 */
	{ "inch",         254,10,    0 },	/* 25.4 */
	{ "inches",       254,10,    0 },	/* 25.4 */
	{ "m",           1000,1,     0 },
	{ "meter",       1000,1,     0 },
	{ "meters",      1000,1,     0 },
	{ "c",             10,1,     0 },
	{ "cent",          10,1,     0 },
	{ "centimeter",    10,1,     0 },
	{ "centimeters",   10,1,     0 },
	{ "mm",             1,1,     0 },
	{ "millimeter",     1,1,     0 },
	{ "millimeters",    1,1,     0 },
	{ "p",           2540,7227,  0 },	/* 25.4 / 72.27 */
	{ "pt",          2540,7227,  0 },	/* 25.4 / 72.27 */
	{ "point",       2540,7227,  0 },	/* 25.4 / 72.27 */
	{ "points",      2540,7227,  0 },
	{ "pixel",          0,0,     0 },
	{ "pixels",         0,0,     0 },
	{ "hor",            0,0,     OL_HORIZONTAL },
	{ "horz",           0,0,     OL_HORIZONTAL },
	{ "horizontal",     0,0,     OL_HORIZONTAL },
	{ "ver",            0,0,     OL_VERTICAL },
	{ "vert",           0,0,     OL_VERTICAL },
	{ "vertical",       0,0,     OL_VERTICAL },
	{ 0 }
};

typedef struct _NameValue {
	OLconst char *		name;
	OLconst int		value;
}			NameValue;

OLconst NameValue	gravities[] = {
	{ "forget",             ForgetGravity             },
	{ "northwest",          NorthWestGravity          },
	{ "north",              NorthGravity              },
	{ "northeast",          NorthEastGravity          },
	{ "west",               WestGravity               },
	{ "center",             CenterGravity             },
	{ "east",               EastGravity               },
	{ "southwest",          SouthWestGravity          },
	{ "south",              SouthGravity              },
	{ "southeast",          SouthEastGravity          },
	{ "static",             StaticGravity             },
	{ "unmap",              UnmapGravity              },
	{ "all",                AllGravity                },
	{ "northsoutheastwest", NorthSouthEastWestGravity },
	{ "southeastwest",      SouthEastWestGravity      },
	{ "northeastwest",      NorthEastWestGravity      },
	{ "northsouthwest",     NorthSouthWestGravity     },
	{ "northsoutheast",     NorthSouthEastGravity     },
	{ "eastwest",           EastWestGravity           },
	{ "northsouth",         NorthSouthGravity         },
	{ 0 }
};

static int (*PrevErrorHandler) OL_ARGS(( Display * , XErrorEvent * ));

static Boolean		GotBadName;

typedef struct {
	OLconst char *	res_name;
	OLconst char *	res_class;
	OLconst char *	ol_default;
	OLconst char *	motif_default;
}	DefaultFonts;

/*  Do not move xtDefaultFont or olDefaultFont in this table.  They are
    referenced by the following defined index.  */
#define XT_DEFAULT_FONT	0
#define OL_DEFAULT_FONT	1

OLconst DefaultFonts default_fonts[] = {
	{ "xtDefaultFont", "XtDefaultFont", "-*-*-*-R-*-*-1*-120-*-*-*-*-ISO8859-1", "-*-*-*-R-*-*-1*-120-*-*-*-*-ISO8859-1" },
	{ olDefaultFont, OlDefaultFont, Nlucida, Nhelvetica },
	{ olDefaultBoldFont, OlDefaultBoldFont, NlucidaBold, NhelveticaBold },
	{ olDefaultFixedFont, OlDefaultFixedFont, NlucidaFixed, NlucidaFixed },
	{ olDefaultBoldItalicFont, OlDefaultBoldItalicFont, NlucidaBoldItalics, NhelveticaBoldItalic },
	{ olDefaultItalicFont, OlDefaultItalicFont, NlucidaItalics, NhelveticaItalic },
	{ olDefaultNoticeFont, OlDefaultNoticeFont, NlucidaBold14, Nhelvetica },
};

/*
 * OlDefine hash table:
 */

#define ADD_TO_TABLE	1
#define LOOKUP_VALUE	2

#define HASH_SIZE	127			/* value 2^n - 1	*/
#define	TABLE_SIZE	(HASH_SIZE+1)
#define HASH_QUARK(q)	((int) ((q) & (XrmQuark) HASH_SIZE))

typedef struct OlDefineNode {
	XrmQuark		quark;
	OlDefine		value;
}			OlDefineNode;

typedef struct Bucket {
	OlDefineNode *		array;
	Cardinal		elements;
}			Bucket;

static Bucket		define_table[TABLE_SIZE];

/**
 ** OlRegisterConverters()
 **/


void
OlRegisterConverters OL_NO_ARGS()
{
	/*
	 * Look in Xt/Converters.c.
	 */
	extern XtConvertArgRec OLconst	screenConvertArg[];

	static XtConvertArgRec	OLconst fontGroupConvertArg[] = {
		{ XtWidgetBaseOffset, (XtPointer)XtOffset(Widget, core.screen),
		  sizeof(Screen *) },
		{ XtProcedureArg, (XtPointer)FetchFontGroupDef, 0 },
	};

	static Boolean		registered	= False;

	if (!registered) {

		XtSetTypeConverter (
			XtRString,
			XtROlFontList,
			OlCvtFontGroupToFontStructList,
			(XtConvertArgList)fontGroupConvertArg,
			XtNumber(fontGroupConvertArg),
			XtCacheByDisplay,
			FreeFontStructList
		);

		XtSetTypeConverter (
			XtRString,
			XtRDimension,
			OlCvtStringToDimension,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRPixmap,
			OlCvtStringToPixmap,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			FreePixmap
		);

		XtSetTypeConverter (
			XtRString,
			XtRBitmap,
			OlCvtStringToBitmap,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			FreePixmap
		);

		/* This StringToImage converter is really a */
		/* StringToPointer converter.  What implications? */
		
		XtSetTypeConverter (
			XtRString,
			XtRPointer, 
			OlCvtStringToImage,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			FreeImage
		);
		XtSetTypeConverter (
			XtRString,
			XtRPosition,
			OlCvtStringToPosition,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRGravity,
			OlCvtStringToGravity,
			(XtConvertArgList)0,
			0,
			XtCacheAll,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRCardinal,
			OlCvtStringToCardinal,
			(XtConvertArgList)0,
			0,
			XtCacheAll,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtROlBitMask,
			OlCvtStringToOlBitMask,
			(XtConvertArgList)0,
			0,
			XtCacheAll,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtROlDefine,
			OlCvtStringToOlDefine,
			(XtConvertArgList)0,
			0,
			XtCacheAll,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			OlRChar,
			OlCvtStringToChar,
			(XtConvertArgList)0,
			0,
			XtCacheAll,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRModifiers,
			OlCvtStringToModifiers,
			(XtConvertArgList)0,
			0,
			XtCacheNone,
			(XtDestructor)0
		);
		XtSetTypeConverter (
			XtRString,
			XtRFont,
			OlCvtStringToFont,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			FreeFont
		);
		XtSetTypeConverter (
			XtRString,
			XtRCursor,
			OlCvtStringToCursor,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			FreeCursor
		);
		XtSetTypeConverter (
			XtRString,
			XtRFontStruct,
			OlCvtStringToFontStruct,
			(XtConvertArgList)screenConvertArg,
			1,
			XtCacheByDisplay,
			FreeFontStruct
		);

		OlRegisterColorTupleListConverter ();

		registered = True;
	}
	return;
} /* OlRegisterConverters() */

/**
 ** _OlAddOlDefineType()
 ** _OlRegisterOlDefineType() - Obsolete
 **/

void
_OlAddOlDefineType OLARGLIST(( name, value ))
	OLARG (OLconst char *,	name)
	OLGRA (OlDefine,	value)
{
	(void)LookupOlDefine ((String)name, value, ADD_TO_TABLE);
	return;
} /* _OlAddOlDefineType() */

void
_OlRegisterOlDefineType OLARGLIST(( app_context, name, value ))
	OLARG (XtAppContext,	app_context)	/* App. Context or NULL	*/
	OLARG (String,		name)		/* OlDefine's name	*/
	OLGRA (OlDefine,	value)		/* OlDefine's value	*/
{
	(void)LookupOlDefine (name, value, ADD_TO_TABLE);
} /* END OF _OlRegisterOlDefineType() */

/**
 ** _OlCvtStringToGravity() - Obsolete
 **/

void
_OlCvtStringToGravity OLARGLIST(( args, num_args, from, to ))
	OLARG (XrmValue *,	args)
	OLARG (Cardinal *,	num_args)
	OLARG (XrmValue *,	from)
	OLGRA (XrmValue *,	to)
{
	extern Display *	toplevelDisplay;

	OlCvtStringToGravity (
		toplevelDisplay,
		args,
		num_args,
		from,
		to,
		(XtPointer *)0
	);
	return;
} /* _OlCvtStringToGravity() */

/**
 ** _OlCvtStringToOlDefine() - Obsolete
 **/

void
_OlCvtStringToOlDefine OLARGLIST(( args, num_args, from, to ))
	OLARG (XrmValue *,	args)
	OLARG (Cardinal *,	num_args)
	OLARG (XrmValue *,	from)
	OLGRA (XrmValue *,	to)
{
	extern Display *	toplevelDisplay;

	OlCvtStringToOlDefine (
		toplevelDisplay,
		args,
		num_args,
		from,
		to,
		(XtPointer *)0
	);
	return;
} /* _OlCvtStringToOlDefine() */

void
OlDisplayStringConversionWarning OLARGLIST((dpy, from, toType))
	OLARG( Display *,	dpy)
	OLARG( OLconst char *,	from)
	OLGRA( int,		toType)
{
  static enum {Check, Report, Ignore} report_it = Check;
  
  if (report_it == Check) {
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
    XrmDatabase rdb = XtScreenDatabase(XDefaultScreenOfDisplay(dpy));
#else
    XrmDatabase rdb = XtDatabase(dpy);
#endif
    XrmName xrm_name[2];
    XrmClass xrm_class[2];
    XrmRepresentation rep_type;
    XrmValue value;
    xrm_name[0] = XrmStringToName( (OLconst char *)"stringConversionWarnings" );
    xrm_name[1] = NULL;
    xrm_class[0] = XrmStringToClass( (OLconst char *)"StringConversionWarnings" );
    xrm_class[1] = NULL;
    if (XrmQGetResource( rdb, xrm_name, xrm_class,
			&rep_type, &value ))
      {
	if (rep_type == XrmStringToQuark(XtRBoolean))
	  report_it = *(Boolean*)value.addr ? Report : Ignore;
	else if (rep_type == XrmStringToQuark(XtRString)) {
	  XrmValue toVal;
	  Boolean report;
	  
	  toVal.addr = (caddr_t)&report; /* Xresource.h says caddr_t */
	  toVal.size = sizeof(Boolean);
	  
/*		if (XtCallConverter(dpy, OlCvtStringToBoolean, (XrmValuePtr)NULL,
			(Cardinal)0, &value, &toVal,
			(XtCacheRef*)NULL))*/
	  if (OlCvtStringToBoolean(dpy, (XrmValuePtr)NULL, (Cardinal)0,
				 &value, &toVal, (XtCacheRef*)NULL))
	    report_it = report ? Report : Ignore;
	}
	else report_it = Report;
      }
    else report_it = Report;
  }
  
  if (report_it == Report) {
    switch (toType) {
    case BOOL:
      OlVaDisplayErrorMsg(dpy,
			  OleNfileConverters,
			  OLET(1),
			  OleCOlToolkitError,
			  OLEM(1),
			  from);
      break;
    case FONT:
      OlVaDisplayWarningMsg(dpy,
			  OleNfileConverters,
			  OLET(2),
			  OleCOlToolkitError,
			  OLEM(2),
			  from);
      break;
    }
  }
}

/**
 ** OlCvtStringToBoolean()
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToBoolean OLARGLIST((display, args, num_args, from, to, closure_ret))
    OLARG (Display *,		display)
    OLARG (XrmValue *,		args)
    OLARG (Cardinal *,		num_args)
    OLARG (XrmValue *,		from)
    OLARG (XrmValue *,		to)
    OLGRA (XtPointer *,		closure_ret)
{
    String str = (String)from->addr;
    if (*num_args != 0)
      OlVaDisplayErrorMsg(display,
			  OleNbadConversion,
			  OleTtooManyParams,
			  OleCOlToolkitError,
			  OleMbadConversion_tooManyParams,
			  (OLconst char *)"String", (OLconst char *)"Boolean");
    
            /*NOTREACHED*/

    if (   (caseless_strcmp(str, (OLconst char *)"true") == 0)
	|| (caseless_strcmp(str, (OLconst char *)"yes") == 0)
	|| (caseless_strcmp(str, (OLconst char *)"on") == 0)
	|| (caseless_strcmp(str, (OLconst char *)"1") == 0))	{
      ConversionDone( Boolean, True );
    }
    
    if (   (caseless_strcmp(str, (OLconst char *)"false") == 0)
	|| (caseless_strcmp(str, (OLconst char *)"no") == 0)
	|| (caseless_strcmp(str, (OLconst char *)"off") == 0)
	|| (caseless_strcmp(str, (OLconst char *)"0") == 0)) {
      ConversionDone( Boolean, False );
    }
    
    OlDisplayStringConversionWarning(display, str, BOOL);
    return False;
}


/**
 ** OlCvtStringToDimension()
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToDimension OLARGLIST(( display, args, num_args, from, to, converter_data ))
	OLARG (Display *,	display)
	OLARG (XrmValue *,	args)
	OLARG (Cardinal *,	num_args)
	OLARG (XrmValue *,	from)
	OLARG (XrmValue *,	to)
	OLGRA (XtPointer *,	converter_data)
{
	int			i;

	Screen *		screen;


	if (*num_args != 1)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadConversion_missingParameter,
			(OLconst char *)"String", (OLconst char *)"Dimension"
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	/*
	 * MORE: This will fail with huge input numbers.
	 */
	if (!IsScaledInteger((String)from->addr, &i, screen) || i < 0) {
		OlVaDisplayWarningMsg (
			display,
			OleNbadConversion, OleTillegalString,
			OleCOlToolkitWarning,
			OleMbadConversion_illegalString,
			(OLconst char *)"String", (OLconst char *)"Dimension",
			(String)from->addr
		);
		return (False);
	}

	ConversionDone (Dimension, (Dimension)i);
} /* OlCvtStringToDimension */

/**
 ** FetchPixmapFile () - just finds a pixmap or bitmap file
 **/

static Boolean
#if	OlNeedFunctionPrototypes
ResolveFile (
	Display *		dpy,
	String			buf,
	String			home,
	OLconst char *		loc,
	OLconst char *		type,
	String			filename,
	OLconst char *		suffix
)
#else
ResolveFile (dpy, buf, home, loc, type, filename, suffix)
	Display *		dpy;
	String			buf;
	String			home;
	OLconst char *		loc;
	OLconst char *		type;
	String			filename;
	OLconst char *		suffix;
#endif
{
	Boolean			found = True;
	struct stat		statbuf;

#define STREQU(A,B) (strcmp((A),(B)) == 0)
#define RESOLVE(DPY,TYPE,FILE,SUFF) \
		XtResolvePathname(DPY,TYPE,FILE,SUFF,NULL,NULL,0,NULL)

	sprintf (buf, (OLconst char *)"%s%s%s", home, loc, filename);
	if (stat(buf, &statbuf) != 0) {
		String fn = RESOLVE(dpy, type, filename, NULL);
		if (
		     !fn
		  && !STREQU(strchr(filename,0)-strlen(suffix), suffix)
		  && !(fn = RESOLVE(dpy, type, filename, suffix))
		)
			found = False;
		if (found) {
			strcpy (buf, fn);
			XtFree (fn);
		}
	}
	return (found);

#undef	STREQU
#undef	RESOLVE
} /* ResolveFile */

/*ARGSUSED*/
static Boolean
FetchPixmapFile OLARGLIST((dpy, screen, value, buf))
  OLARG (Display *,	dpy)
  OLARG (Screen *,	screen)
  OLARG (XrmValue*,	value)
  OLGRA (char *,	buf)
{
  String	filename = (String)value->addr;
  Boolean	found = True;
  struct stat	statbuf;
  
  /* Searching for the pixmap file will go as follows:
     (This is loosely based on the XmuConvertStringtoBitmap ordering)
     
     1) Try it as an absolute if it begins with / or ./
     2) See if it's in $XWINHOME/lib/pixmaps (place where desktop pixmaps
        are located).
     3) See if can find it in a pixmaps directory with XtResolvePathname.
     4) See if can find it with .xpm suffix with XtResolvePathname (if it
        doesn't already have a .xpm suffix).
     5) Try 2-4 above with bitmaps and .xbm.
     6) Just try the filename.

     Once the file is found, use the XPM routine XReadFileToPixmap.  If it
     fails, try XReadBitmapFile.
  */

  if ((filename[0] == '/' || (filename[0] == '.')
	&& filename[1] == '/')) { 
    strcpy(buf, filename);
  }
  else {

#define pixmaps		(OLconst char *)"pixmaps"
#define bitmaps		(OLconst char *)"bitmaps"
#define xpm		(OLconst char *)".xpm"
#define xbm		(OLconst char *)".xbm"

#define PixmapLoc	(OLconst char *)"/lib/pixmaps/"
#define BitmapLoc	(OLconst char *)"/lib/bitmaps/"

    String		home;
    OLconst char	*first_choice, *second_choice; /* can't use String */

    if ( !(home = (String) getenv((OLconst char *)"XWINHOME")) )
	home = "/usr/X";	/* fall back, watch for null pointer */

    if (DefaultDepthOfScreen(screen) > 1) {
	first_choice = PixmapLoc;
	second_choice = BitmapLoc;
    } else {
	first_choice = BitmapLoc;
	second_choice = PixmapLoc;
    }
    found = ResolveFile(dpy, buf, home, first_choice, bitmaps, filename, xbm);
    if (!found)
	found = ResolveFile(
			dpy, buf, home, second_choice, pixmaps, filename, xpm);

#undef pixmaps
#undef bitmaps
#undef xpm
#undef xbm
#undef PixmapLoc
#undef BitmapLoc
  }

  if (!found)			/* time for case #6 */
    if (stat(filename, &statbuf) == 0) {
      strcpy(buf, filename);
      found = True;
    }

  if (!found) {
    return (False);
  }

  return (True);
}

/**
 ** FetchBitmap () - read in a bitmap file and return its Pixmap
 **/
static Boolean
FetchBitmap OLARGLIST((screen, d, buf, depth, w, h, p))
  OLARG (Screen *,	screen)
  OLARG (Drawable ,	d)
  OLARG (char *,	buf)
  OLARG (int,		depth)
  OLARG (unsigned int *, w)
  OLARG (unsigned int *, h)
  OLGRA (Pixmap *,	p)
{
  Pixmap	bitmap;
  int		dummy;
  Display	*dpy = DisplayOfScreen(screen);

#ifndef NO_XMU
  if (XReadBitmapFile(dpy, d, buf, w, h, &bitmap, &dummy,
		      &dummy) != BitmapSuccess) {
    return (False);
  }
  else {
    /* using these XtDefault tends to bring up a random set of colors, */
    /* so maybe there's a bug in the intrinsics, so comment them out. */

    Pixel bg;			/* = XtDefaultBackground;*/
    Pixel fg;			/* = XtDefaultForeground;*/

/*      if (bg == fg) {*/
    fg = BlackPixelOfScreen(screen);
    bg = WhitePixelOfScreen(screen);
/*      }*/
    if (depth > 1){
	*p = XmuCreatePixmapFromBitmap(dpy, d, bitmap, *w, *h, depth, fg, bg);
	XFreePixmap(dpy, bitmap);
    }
    else
	*p = bitmap;
   

  }
#endif
}

/**
 ** OlCvtStringToPixmap()
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToPixmap OLARGLIST(( display, args, num_args, from, to,
			       converter_data ))
  OLARG (Display *,	display)
  OLARG (XrmValue *,	args)
  OLARG (Cardinal *,	num_args)
  OLARG (XrmValue *,	from)
  OLARG (XrmValue *,	to)
  OLGRA (XtPointer *,	converter_data)
{
  Pixmap	p;
  Screen	*screen;
  Display	*dpy;
  unsigned int	w, h, dummy;
  char		buf[180];	/* MAXPATHLEN is 1K, too big */
  int		ret;
  Drawable	d;
  int		depth;

  if (*num_args != 1)
    OlVaDisplayErrorMsg (
			 display,
			 OleNbadConversion, OleTmissingParameter,
			 OleCOlToolkitError,
			 OleMbadConversion_missingParameter,
			 (OLconst char *)"String", (OLconst char *)"Pixmap"
			 );
  /*NOTREACHED*/

  screen = *((Screen **)(args[0].addr));
  dpy = DisplayOfScreen(screen);
  d = RootWindowOfScreen(screen);
  depth = DefaultDepthOfScreen(screen);

  if (!FetchPixmapFile(dpy, screen, from, buf))
    return(False);
  
  /* now read in the found file */

  if (ret = XReadPixmapFile(dpy, d, DefaultColormapOfScreen(screen), buf,
			    &w, &h, depth, &p) != PixmapSuccess) {
    if (!FetchBitmap(screen, d, buf, depth, &w, &h, &p))
      return (False);
  }
  
  ConversionDone(Pixmap, p);
} /* OlCvtStringToPixmap */

/**
 ** OlCvtStringToBitmap()
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToBitmap OLARGLIST(( display, args, num_args, from, to,
			       converter_data ))
  OLARG (Display *,	display)
  OLARG (XrmValue *,	args)
  OLARG (Cardinal *,	num_args)
  OLARG (XrmValue *,	from)
  OLARG (XrmValue *,	to)
  OLGRA (XtPointer *,	converter_data)
{
  Pixmap	p;
  Screen	*screen;
  Display	*dpy;
  unsigned int	w, h, dummy;
  char		buf[180];	/* MAXPATHLEN is 1K, too big */
  int		ret;
  Drawable	d;


  if (*num_args != 1)
    OlVaDisplayErrorMsg (
			 display,
			 OleNbadConversion, OleTmissingParameter,
			 OleCOlToolkitError,
			 OleMbadConversion_missingParameter,
			 (OLconst char *)"String", (OLconst char *)"Bitmap"
			 );
  /*NOTREACHED*/

  screen = *((Screen **)(args[0].addr));
  dpy = DisplayOfScreen(screen);
  d = RootWindowOfScreen(screen);


  
  if (!FetchPixmapFile(dpy, screen, from, buf)){
      return(False);
  }
  
  /* now read in the found file */
  if (!FetchBitmap(screen, d, buf, 1/* depth */, &w, &h, &p))
      return (False);
    
  ConversionDone(Pixmap, p);
} /* OlCvtStringToBitmap */

/* Destructor for OlCvtStringToPixmap and OlCvtStringToBitmap converters. 
 */

/* ARGSUSED */
static void
FreePixmap OLARGLIST(( app, to, closure, args, num_args ))
	OLARG (XtAppContext,	app)
	OLARG (XrmValuePtr,	to)
	OLARG (XtPointer,	closure)
	OLARG (XrmValuePtr,	args)
	OLGRA (Cardinal *,	num_args)
{
    Screen *screen;

    if (*num_args != 1)
		OlVaDisplayErrorMsg (
			toplevelDisplay, /* ICK! */
			OleNbadDestruction, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadDestruction_missingParameter,
			(OLconst char *)"FreePixmap"
		);

    screen = *((Screen **)(args[0].addr));
    XFreePixmap( DisplayOfScreen(screen), *(Pixmap*)to->addr );
} /* FreePixmap */

/**
 ** OlCvtStringToImage()
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToImage OLARGLIST(( display, args, num_args, from, to,
			      converter_data ))
  OLARG (Display *,	display)
  OLARG (XrmValue *,	args)
  OLARG (Cardinal *,	num_args)
  OLARG (XrmValue *,	from)
  OLARG (XrmValue *,	to)
  OLGRA (XtPointer *,	converter_data)
{
  Pixmap	p;
  Screen	*screen;
  Display	*dpy;
  unsigned int	w, h;
  int		dummy;
  XImage	*image;
  Drawable	d;
  int		depth, ret;
  char		buf[180];	/* MAXPATHLEN is 1K, too big */

  if (*num_args != 1)
    OlVaDisplayErrorMsg (
			 display,
			 OleNbadConversion, OleTmissingParameter,
			 OleCOlToolkitError,
			 OleMbadConversion_missingParameter,
			 (OLconst char *)"String", (OLconst char *)"Image"
			 );
  /*NOTREACHED*/

  screen = *((Screen **)(args[0].addr));
  dpy = DisplayOfScreen(screen);
  d = RootWindowOfScreen(screen);
  depth = DefaultDepthOfScreen(screen);

  if (!FetchPixmapFile(dpy, screen, from, buf))
    return(False);
  
  /* now read in the found file */

  if (ret = XReadPixmapFile(dpy, d, DefaultColormapOfScreen(screen), buf,
			    &w, &h, depth, &p) != PixmapSuccess) {
    if (!FetchBitmap(screen, d, buf, depth, &w, &h, &p))
      return (False);
  }
  
  image = XGetImage(dpy, p, 0, 0, w, h, 0xffff, XYPixmap);
  
  ConversionDone(XImage *, image);
} /* OlCvtStringToImage */

/* Destructor for OlCvtStringToImage converter. 
 */

/* ARGSUSED */
static void
FreeImage OLARGLIST(( app, to, closure, args, num_args ))
	OLARG (XtAppContext,	app)
	OLARG (XrmValuePtr,	to)
	OLARG (XtPointer,	closure)
	OLARG (XrmValuePtr,	args)
	OLGRA (Cardinal *,	num_args)
{
    Screen *screen;

    if (*num_args != 1)
		OlVaDisplayErrorMsg (
			toplevelDisplay, /* ICK! */
			OleNbadDestruction, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadDestruction_missingParameter,
			(OLconst char *)"FreeImage"
		);

    XtFree((char *)*(XImage **)to->addr);
} /* FreeImage */

/**
 ** OlCvtStringToPosition()
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToPosition OLARGLIST(( display, args, num_args, from, to, converter_data ))
	OLARG (Display *,	display)
	OLARG (XrmValue *,	args)
	OLARG (Cardinal *,	num_args)
	OLARG (XrmValue *,	from)
	OLARG (XrmValue *,	to)
	OLGRA (XtPointer *,	converter_data)
{
	int			i;

	Screen *		screen;


	if (*num_args != 1)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadConversion_missingParameter,
			(OLconst char *)"String", (OLconst char *)"Position"
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (!IsScaledInteger((String)from->addr, &i, screen)) {
		OlVaDisplayWarningMsg (
			display,
			OleNbadConversion, OleTillegalString,
			OleCOlToolkitWarning,
			OleMbadConversion_illegalString,
			(OLconst char *)"String", (OLconst char *)"Position",
			(String)from->addr
		);
		return (False);
	}

	ConversionDone (Position, (Position)i);
} /* OlCvtStringToPosition */

/**
 ** OlCvtStringToGravity()
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToGravity OLARGLIST(( display, args, num_args, from, to, converter_data ))
	OLARG (Display *,	display)
	OLARG (XrmValue *,	args)
	OLARG (Cardinal *,	num_args)
	OLARG (XrmValue *,	from)
	OLARG (XrmValue *,	to)
	OLGRA (XtPointer *,	converter_data)
{
	int			i;

	OLconst NameValue *	p;


	/*
	 * This converter used to convert the list of gravities to
	 * quarks, then compare quarks instead of strings. Now that
	 * we use the XtCacheAll feature, quarkification becomes less
	 * important, as we will come here only once per gravity value
	 * (at most!)
	 */

	if (*num_args != 0)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTtooManyParams,
			OleCOlToolkitError,
			OleMbadConversion_tooManyParams,
			(OLconst char *)"String", (OLconst char *)"Gravity"
		);
		/*NOTREACHED*/

	for (p = gravities; p->name; p++)
		if (CASELESS_STREQU((String)from->addr, p->name))
			break;

	if (!p->name) {
		OlVaDisplayWarningMsg (
			display,
			OleNbadConversion, OleTillegalString,
			OleCOlToolkitWarning,
			OleMbadConversion_illegalString,
			(OLconst char *)"String", (OLconst char *)"Gravity",
			(String)from->addr
		);
		return (False);
	}

	ConversionDone (int, p->value);
} /* OlCvtStringToGravity() */

/**
 ** OlCvtStringToCardinal()
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToCardinal OLARGLIST(( display, args, num_args, from, to, converter_data ))
	OLARG (Display *,	display)
	OLARG (XrmValue *,	args)
	OLARG (Cardinal *,	num_args)
	OLARG (XrmValue *,	from)
	OLARG (XrmValue *,	to)
	OLGRA (XtPointer *,	converter_data)
{
	int			i;


	if (*num_args != 0)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTtooManyParams,
			OleCOlToolkitError,
			OleMbadConversion_tooManyParams,
			(OLconst char *)"String", (OLconst char *)"Cardinal"
		);
		/*NOTREACHED*/

	/*
	 * MORE: This will fail with huge input numbers.
	 */
	if (!IsInteger((String)from->addr, &i) || i < 0) {
		OlVaDisplayWarningMsg (
			display,
			OleNbadConversion, OleTillegalString,
			OleCOlToolkitWarning,
			OleMbadConversion_illegalString,
			(OLconst char *)"String", (OLconst char *)"Cardinal",
			(String)from->addr
		);
		return (False);
	}

	ConversionDone (Cardinal, (Cardinal)i);
} /* OlCvtStringToCardinal() */

/**
 ** OlCvtStringToOlBitMask()
 **
 ** note that, this can be a "unsigned long" converter too. I didn't
 **	bother to create XtRUnsignedLong symbol because of time
 **	constrain. We probably should have a "long" converter
 **	because I found at least one instance (textedit)
 **	is using XtRInt when it really want XtRLong.
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToOlBitMask OLARGLIST(( display, args, num_args, from, to, converter_data ))
	OLARG (Display *,	display)
	OLARG (XrmValue *,	args)
	OLARG (Cardinal *,	num_args)
	OLARG (XrmValue *,	from)
	OLARG (XrmValue *,	to)
	OLGRA (XtPointer *,	converter_data)
{
	OlBitMask		i;


	if (*num_args != 0)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTtooManyParams,
			OleCOlToolkitError,
			OleMbadConversion_tooManyParams,
			(OLconst char *)"String", (OLconst char *)"OlBitMask"
		);
		/*NOTREACHED*/

	if (!IsOlBitMask((String)from->addr, &i)) {
		OlVaDisplayWarningMsg (
			display,
			OleNbadConversion, OleTillegalString,
			OleCOlToolkitWarning,
			OleMbadConversion_illegalString,
			(OLconst char *)"String", (OLconst char *)"OlBitMask",
			(String)from->addr
		);
		return (False);
	}

	ConversionDone (OlBitMask, (OlBitMask)i);
} /* OlCvtStringToOlBitMask() */


/**
 ** OlCvtStringToOlDefine()
 **/

/*ARGSUSED5*/
extern Boolean
OlCvtStringToOlDefine OLARGLIST(( display, args, num_args, from, to, converter_data ))
	OLARG (Display *,	display)
	OLARG (XrmValue *,	args)
	OLARG (Cardinal *,	num_args)
	OLARG (XrmValue *,	from)
	OLARG (XrmValue *,	to)
	OLGRA (XtPointer *,	converter_data)
{
	OlDefine		value;

	if (*num_args != 0)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTtooManyParams,
			OleCOlToolkitError,
			OleMbadConversion_tooManyParams,
			(OLconst char *)"String", (OLconst char *)"OlDefine"
		);
		/*NOTREACHED*/


	value = LookupOlDefine((String)from->addr, (OlDefine)0, LOOKUP_VALUE);
	if (!value) {
		OlVaDisplayWarningMsg (
			display,
			OleNbadConversion, OleTillegalString,
			OleCOlToolkitWarning,
			OleMbadConversion_illegalString,
			(OLconst char *)"String", (OLconst char *)"OlDefine",
			(String)from->addr
		);
		return (False);
	}

	ConversionDone (OlDefine, value);
} /* OlCvtStringToOlDefine() */

/**
 ** OlCvtStringToChar() - this routine converts a string to a mnemonic
 **/

/* ARGSUSED */
extern Boolean
OlCvtStringToChar OLARGLIST((display, args, num_args, from, to, converter_data))
	OLARG( Display *,		display)
	OLARG( XrmValue *,		args)
	OLARG( Cardinal *,		num_args)
	OLARG( XrmValue *,		from)
	OLARG( XrmValue *,		to)
	OLGRA( XtPointer *,		converter_data)
{
	if (*num_args != 0)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTtooManyParams,
			OleCOlToolkitError,
			OleMbadConversion_tooManyParams,
			(OLconst char *)"String", (OLconst char *)"Char"
		);
		/*NOTREACHED*/

	/*  Use the first character of the string.  */
	ConversionDone(char, *((char *) from->addr));

} /* END OF OlCvtStringToChar() */

/**
 ** OlCvtStringToModifiers()
 **/

extern Boolean
OlCvtStringToModifiers OLARGLIST((display, args, num_args, from, to, converter_data))
	OLARG( Display *,		display)
	OLARG( XrmValue *,		args)
	OLARG( Cardinal *,		num_args)
	OLARG( XrmValue *,		from)
	OLARG( XrmValue *,		to)
	OLGRA( XtPointer *,		converter_data)
{
	static char		dummy_detail[]	= { LBRA , 'a', RBRA, 0 };
	static char		prefixed_key_buf[20];

	Modifiers		modifiers	= 0;


	if (*num_args)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTtooManyParams,
			OleCOlToolkitError,
			OleMbadConversion_tooManyParams,
			(OLconst char *)"String", (OLconst char *)"Modifiers"
		);
		/*NOTREACHED*/

	if (from->addr) {
		XrmValue		_from;
		XrmValue		_to;

		String			prefixed_key;

		Cardinal		len;

		Boolean			success;

		OlKeyDef *		kd;


		len = Strlen((String)from->addr) + XtNumber(dummy_detail);
		if (len > XtNumber(prefixed_key_buf))
			prefixed_key = Malloc(len);
		else
			prefixed_key = prefixed_key_buf;

		strcpy (prefixed_key, (String)from->addr);
		strcat (prefixed_key, dummy_detail);

		_from.addr = (XtPointer)prefixed_key;
		_from.size = len;
		_to.addr   = 0;
		success = XtCallConverter(
			display,
			_OlStringToOlKeyDef,
			(XrmValuePtr)0,
			(Cardinal)0,
			&_from,
			&_to,
			(XtCacheRef)0
		);

		if (prefixed_key != prefixed_key_buf)
			Free (prefixed_key);

		if (!success)
			return (False);

		kd = (OlKeyDef *)_to.addr;
		if (kd->used != 1) {
			OlVaDisplayWarningMsg (
				display,
				OleNbadConversion, OleTillegalSyntax,
				OleCOlToolkitWarning,
				OleMbadConversion_illegalSyntax,
				(OLconst char *)"String", (OLconst char *)"Modifiers"
			);
			return (False);
		}

		modifiers = kd->modifier[0];

	} else {
		OlVaDisplayWarningMsg (
			display,
			OleNbadConversion, OleTnullString,
			OleCOlToolkitWarning,
			OleMbadConversion_nullString,
			(OLconst char *)"String", (OLconst char *)"Modifiers"
		);
		return (False);
	}

	ConversionDone (Modifiers, modifiers);
} /* OlCvtStringToModifiers */

/**
 ** OlCvtStringToFont()
 ** FreeFont()
 **/

extern Boolean
OlCvtStringToFont OLARGLIST(( display, args, num_args, from, to, converter_data ))
	OLARG (Display *,	display)
	OLARG (XrmValue *,	args)
	OLARG (Cardinal *,	num_args)
	OLARG (XrmValue *,	from)
	OLARG (XrmValue *,	to)
	OLGRA (XtPointer *,	converter_data)
{
	Font			f;

	Screen *		screen;

	String			cset = NULL;


	if (*num_args != 1)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadConversion_missingParameter,
			(OLconst char *)"String", (OLconst char *)"Font"
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (!IsFont((OLconst char *)"Font", screen, (String)from->addr, &f, converter_data, &cset))
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTnoFont,
			OleCOlToolkitError,
			OleMbadConversion_noFont
		);
		/*NOTREACHED*/
	ConversionDone (Font, f);
} /* OlCvtStringToFont */

/*
 * FetchFontGroupDef - used by OlRegisterFontGroupConverter().
 */
static void
FetchFontGroupDef OLARGLIST((w, size, value))
	OLARG( Widget,		w)
	OLARG( Cardinal *,	size)
	OLGRA( XrmValue *,	value)
{
	value->size = sizeof(String);
	value->addr = (XtPointer)&ol_app_attributes.font_group_def;
} /* end of FetchFontGroupDef */

static void
FreeFont OLARGLIST(( app, to, closure, args, num_args ))
	OLARG (XtAppContext,	app)
	OLARG (XrmValuePtr,	to)
	OLARG (XtPointer,	closure)
	OLARG (XrmValuePtr,	args)
	OLGRA (Cardinal *,	num_args)
{
	Screen *		screen;

#define free_the_font	(Boolean)closure


	if (*num_args != 1)
		OlVaDisplayErrorMsg (
			toplevelDisplay, /* ICK! */
			OleNbadDestruction, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadDestruction_missingParameter,
			(OLconst char *)"FreeFont"
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (free_the_font)
		XUnloadFont (DisplayOfScreen(screen), *(Font*)to->addr);

#undef free_the_font
	return;
} /* FreeFont */

/**
 ** OlCvtStringToFontStruct()
 ** FreeFontStruct()
 **/

extern Boolean
OlCvtStringToFontStruct OLARGLIST((display, args, num_args, from, to, converter_data))
	OLARG( Display *,		display)
	OLARG( XrmValue *,		args)
	OLARG( Cardinal *,		num_args)
	OLARG( XrmValue *,		from)
	OLARG( XrmValue *,		to)
	OLGRA( XtPointer *,		converter_data)
{
	XFontStruct *		fs;

	Font			f;

	Screen *		screen;

	String			cset = NULL;


	if (*num_args != 1)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadConversion_missingParameter,
			(OLconst char *)"String", (OLconst char *)"FontStruct"
		);
		/*NOTREACHED*/
	screen = *((Screen **)(args[0].addr));

	if (!IsFont((OLconst char *)"FontStruct", screen, (String)from->addr, &f, converter_data, &cset))
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTnoFont,
			OleCOlToolkitError,
			OleMbadConversion_noFont
		);
		/*NOTREACHED*/
	/*
	 * Slight problem here: If the font ID returned is ``static''
	 * (as marked by the "converter_data"), then the following
	 * will never be freed. The problem is that Xlib doesn't
	 * provide a way to back out of the String -> Font -> FontStruct
	 * steps without trashing the Font as well as the FontStruct.
	 * But currently this will happen only once (Font can be static
	 * only for Xt default font), so not a big deal.
	 */
	if (!(fs = XQueryFont(display, f)))
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTbadQueryFont,
			OleCOlToolkitError,
			OleMbadConversion_badQueryFont
		);
		/*NOTREACHED*/

	ConversionDone (XFontStruct *, fs);
} /* OlCvtStringToFontStruct */

static void
FreeFontStruct OLARGLIST(( app, to, closure, args, num_args ))
	OLARG (XtAppContext,	app)
	OLARG (XrmValuePtr,	to)
	OLARG (XtPointer,	closure)
	OLARG (XrmValuePtr,	args)
	OLGRA (Cardinal *,	num_args)
{
	Screen *		screen;

#define free_the_font	(Boolean)closure


	if (*num_args != 1)
		OlVaDisplayErrorMsg (
			toplevelDisplay, /* ICK! */
			OleNbadDestruction, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadDestruction_missingParameter,
			(OLconst char *)"FreeFontStruct"
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (free_the_font)
		XFreeFont (DisplayOfScreen(screen), *(XFontStruct**)to->addr);

#undef free_the_font
	return;
} /* FreeFontStruct */

/* These are the defined Open Look Cursors - these defines are used
 * only as placeholders in the array below, and are not standardized
 * in Open Look.
 */
#define OL_MOVE_cursor	156
#define OL_DUPLICATE_cursor	158
#define OL_BUSY_cursor	160
#define OL_PAN_cursor	162
#define OL_QUESTION_cursor	164
#define OL_TARGET_cursor	166
#define OL_STANDARD_cursor	168

/* When naming an Open Look cursor in a resource file, the string used
 * must be specified exactly as defined in the table below -
 *
 *	OLmove_cursor	- the "Move" cursor.
 *	OLduplicate_cursor - the "Duplicate" cursor.
 *	OLbusy_cursor - the "Busy" cursor
 *	OLpan_cursor - the "Pan" cursor.
 *	OLquestion_cursor - the "Question Mark" cursor.
 *	OLtarget_cursor - the "Target" (Bullseye) cursor.
 *	OLstandard_cursor - the standard (arrow) Open Look Cursor.
 */

typedef struct _CursorName {
	OLconst char		*name;
	OLconst unsigned int	shape;
    } Cursor_Name;

/* String to OL cursor converter.  This code was (for the most part)
 * taken directly from Xt/Converters.c, but the table is my version
 * contains the Open Look cursors at the very beginning.  Search the table
 * for the user specified cursor (string), and create the cursor.
 */
extern Boolean
OlCvtStringToCursor OLARGLIST((display, args, num_args, from, to, converter_data))
	OLARG( Display *,		display)
	OLARG( XrmValue *,		args)
	OLARG( Cardinal *,		num_args)
	OLARG( XrmValue *,		from)
	OLARG( XrmValue *,		to)
	OLGRA( XtPointer *,		converter_data)
{
	Screen *screen;

static OLconst Cursor_Name cursor_names[] = {
			{"OLmove_cursor",		OL_MOVE_cursor},
			{"OLduplicate_cursor",		OL_DUPLICATE_cursor},
			{"OLbusy_cursor",		OL_BUSY_cursor},
			{"OLpan_cursor",		OL_PAN_cursor},
			{"OLquestion_cursor",		OL_QUESTION_cursor},
			{"OLtarget_cursor",		OL_TARGET_cursor},
			{"OLstandard_cursor",		OL_STANDARD_cursor},
			/* Now the X standard cursors... */
			{"X_cursor",		XC_X_cursor},
			{"arrow",		XC_arrow},
			{"based_arrow_down",	XC_based_arrow_down},
			{"based_arrow_up",	XC_based_arrow_up},
			{"boat",		XC_boat},
			{"bogosity",		XC_bogosity},
			{"bottom_left_corner",	XC_bottom_left_corner},
			{"bottom_right_corner",	XC_bottom_right_corner},
			{"bottom_side",		XC_bottom_side},
			{"bottom_tee",		XC_bottom_tee},
			{"box_spiral",		XC_box_spiral},
			{"center_ptr",		XC_center_ptr},
			{"circle",		XC_circle},
			{"clock",		XC_clock},
			{"coffee_mug",		XC_coffee_mug},
			{"cross",		XC_cross},
			{"cross_reverse",	XC_cross_reverse},
			{"crosshair",		XC_crosshair},
			{"diamond_cross",	XC_diamond_cross},
			{"dot",			XC_dot},
			{"dotbox",		XC_dotbox},
			{"double_arrow",	XC_double_arrow},
			{"draft_large",		XC_draft_large},
			{"draft_small",		XC_draft_small},
			{"draped_box",		XC_draped_box},
			{"exchange",		XC_exchange},
			{"fleur",		XC_fleur},
			{"gobbler",		XC_gobbler},
			{"gumby",		XC_gumby},
			{"hand1",		XC_hand1},
			{"hand2",		XC_hand2},
			{"heart",		XC_heart},
			{"icon",		XC_icon},
			{"iron_cross",		XC_iron_cross},
			{"left_ptr",		XC_left_ptr},
			{"left_side",		XC_left_side},
			{"left_tee",		XC_left_tee},
			{"leftbutton",		XC_leftbutton},
			{"ll_angle",		XC_ll_angle},
			{"lr_angle",		XC_lr_angle},
			{"man",			XC_man},
			{"middlebutton",	XC_middlebutton},
			{"mouse",		XC_mouse},
			{"pencil",		XC_pencil},
			{"pirate",		XC_pirate},
			{"plus",		XC_plus},
			{"question_arrow",	XC_question_arrow},
			{"right_ptr",		XC_right_ptr},
			{"right_side",		XC_right_side},
			{"right_tee",		XC_right_tee},
			{"rightbutton",		XC_rightbutton},
			{"rtl_logo",		XC_rtl_logo},
			{"sailboat",		XC_sailboat},
			{"sb_down_arrow",	XC_sb_down_arrow},
			{"sb_h_double_arrow",	XC_sb_h_double_arrow},
			{"sb_left_arrow",	XC_sb_left_arrow},
			{"sb_right_arrow",	XC_sb_right_arrow},
			{"sb_up_arrow",		XC_sb_up_arrow},
			{"sb_v_double_arrow",	XC_sb_v_double_arrow},
			{"shuttle",		XC_shuttle},
			{"sizing",		XC_sizing},
			{"spider",		XC_spider},
			{"spraycan",		XC_spraycan},
			{"star",		XC_star},
			{"target",		XC_target},
			{"tcross",		XC_tcross},
			{"top_left_arrow",	XC_top_left_arrow},
			{"top_left_corner",	XC_top_left_corner},
			{"top_right_corner",	XC_top_right_corner},
			{"top_side",		XC_top_side},
			{"top_tee",		XC_top_tee},
			{"trek",		XC_trek},
			{"ul_angle",		XC_ul_angle},
			{"umbrella",		XC_umbrella},
			{"ur_angle",		XC_ur_angle},
			{"watch",		XC_watch},
			{"xterm",		XC_xterm},
    };

    OLconst Cursor_Name *nP;
    char *name = (char *)from->addr;
    register int i;

	if (*num_args != 1) /* display parameter needed */
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadConversion_missingParameter,
			"String", "Cursor"
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));


    for (i=0, nP=cursor_names; i < XtNumber(cursor_names); i++, nP++ ) {
	if (strcmp(name, nP->name) == 0) {
	    Cursor cursor;
		switch(nP->shape) {
			case OL_MOVE_cursor:
				cursor = GetOlMoveCursor(screen);
				break;
			case OL_DUPLICATE_cursor:
				cursor = GetOlDuplicateCursor(screen);
				break;
			case OL_BUSY_cursor:
				cursor = GetOlBusyCursor(screen);
				break;
			case OL_PAN_cursor:
				cursor = GetOlPanCursor(screen);
				break;
			case OL_QUESTION_cursor:
				cursor = GetOlQuestionCursor(screen);
				break;
			case OL_TARGET_cursor:
				cursor = GetOlTargetCursor(screen);
				break;
			case OL_STANDARD_cursor:
				cursor = GetOlStandardCursor(screen);
				break;
			default:
 				cursor = XCreateFontCursor(display, nP->shape );
				break;
		} /* end switch */
	    ConversionDone (Cursor, cursor);
	    break;
	}
    }
   /* The specified cursor isn't one of the X or Open Look cursors;
    * print a warning.
    *   An alternative to the method used here for convertering a string
    * to a cursor is to put only the Open look cursors (7) into the above
    * array and not duplicate the X cursors here; then if the search fails
    * on the OL cursors, call XtCallConverter() on the Xt StringToCursor
    * converter (the Xt Converter already has the full table of X cursors
    * that we have listed above).  The problem is, the Xt converter isn't
    * public (it's static within it's file);  we can't call XtConvert() either,
    * because it takes a widget as an argument. 
    */
	OlVaDisplayWarningMsg (
			display,
			OleNbadConversion, OleTillegalString,
			OleCOlToolkitError,
			OleMbadConversion_illegalString,
			"String", "Cursor", name
		);
    return False;
}

/* Destructor for OlStringToCursor converter.  Once again, this code is
 * primarily taken from Xt/Converters.c
 */

/* ARGSUSED */
static void
FreeCursor OLARGLIST(( app, to, closure, args, num_args ))
	OLARG (XtAppContext,	app)
	OLARG (XrmValuePtr,	to)
	OLARG (XtPointer,	closure)
	OLARG (XrmValuePtr,	args)
	OLGRA (Cardinal *,	num_args)
{
    Screen *screen;

    if (*num_args != 1)
		OlVaDisplayErrorMsg (
			toplevelDisplay, /* ICK! */
			OleNbadDestruction, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadDestruction_missingParameter,
			(OLconst char *)"FreeCursor"
		);

    screen = *((Screen **)(args[0].addr));
    XFreeCursor( DisplayOfScreen(screen), *(Cursor*)to->addr );
}

/**
 ** OlContrastingColor()
 **/

Pixel
OlContrastingColor OLARGLIST((w, pixel, percent))
	OLARG( Widget,		w )
	OLARG( Pixel,		pixel )
	OLGRA( int,		percent )
{
	static Pixel		prev_pixel = 0;
	static Pixel		contrasting_pixel = 0;

	static Screen *		prev_screen = 0;

	Screen *		screen = XtScreenOfObject(w);


	if (prev_screen == screen && prev_pixel == pixel)
		return (contrasting_pixel);

	if (pixel == BlackPixelOfScreen(screen))
		contrasting_pixel = WhitePixelOfScreen(screen);

	else if (pixel == WhitePixelOfScreen(screen))
		contrasting_pixel = BlackPixelOfScreen(screen);

	else {
		Display *		display	= XtDisplayOfObject(w);

		XColor			xcolor;

		unsigned long		intensity;


		xcolor.pixel = pixel;
		XQueryColor (display, w->core.colormap, &xcolor);

#if	OlNeedFunctionPrototypes
# define F(C) (unsigned long)C ## L
#else
# define F(C) (unsigned long)C/**/L
#endif
		intensity =
		      (xcolor.flags & DoRed?   F(39) * xcolor.red   : 0)
		    + (xcolor.flags & DoGreen? F(50) * xcolor.green : 0)
		    + (xcolor.flags & DoBlue?  F(11) * xcolor.blue  : 0);
#undef F

		if (intensity > (unsigned long)(65535L * percent))
			contrasting_pixel = BlackPixelOfScreen(screen);
		else
			contrasting_pixel = WhitePixelOfScreen(screen);
	}

	prev_screen = screen;
	prev_pixel = pixel;
	return (contrasting_pixel);
} /* OlContrastingColor */

/**
 ** IsScaledInteger()
 **/

static Boolean
IsScaledInteger OLARGLIST(( val, pint, screen ))
	OLARG (String,		val)
	OLARG (int *,		pint)
	OLGRA (Screen *,	screen)
{
	Boolean			ret;

	OlDefine		axis	= OL_HORIZONTAL;

	String			pri_val	= Strdup(val);
	String			rest	= 0;
	String			word;

	Fraction		d;

	OLconst Units		*pu;


	d = StrToFraction(pri_val, &rest);
	if (rest && *rest) {

		rest += strspn(rest, WORD_SEPS);
		while ((word = strtok(rest, WORD_SEPS))) {

			for (pu = units; pu->modifier; pu++)
				if (CASELESS_STREQU(pu->modifier, word))
					break;
			if (!pu->modifier) {
				ret = FALSE;
				goto Return;
			}

			if (pu->axis)
				axis = pu->axis;
			else if (IsZeroFraction(pu->factor))
				goto Pixels;
			else
				d = MulFractions(d, pu->factor);
			rest = 0;

		}
		if (IsZeroFraction(d))
			*pint = 0;
		else {
			Fraction		mmtopixel;

			mmtopixel =
				(axis == OL_HORIZONTAL?
					NewFraction(
						WidthOfScreen(screen),
						WidthMMOfScreen(screen)
					)
				      : NewFraction(
						HeightOfScreen(screen),
						HeightMMOfScreen(screen)
					)
				);

			*pint = FractionToInt(MulFractions(d, mmtopixel));

			/*
			 * The value wasn't zero before the conversion
			 * to pixels, so don't let it be zero now. This
			 * ensures that all non-zero dimensions/positions
			 * are visible on the screen.
			 */
			if (*pint == 0)
				if (IsNegativeFraction(d))
					*pint = -1;
				else
					*pint = 1;
		}
		ret = TRUE;
	} else {
Pixels:		*pint = FractionToInt(d);
		ret = TRUE;
	}

Return:
	Free (pri_val);
	return (ret);
} /* IsScaledInteger() */

/**
 ** IsInteger()
 **/

static Boolean
IsInteger OLARGLIST(( val, pint ))
	OLARG (String,		val)
	OLGRA (int *,		pint)
{
	String			rest	= 0;


	*pint = strtol(val, &rest, 0);
	return (!(rest && *rest));
} /* IsInteger() */

/**
 ** IsOlBitMask()
 **/

static Boolean
IsOlBitMask OLARGLIST(( val, mask ))
	OLARG (String,		val)
	OLGRA (OlBitMask *,	mask)
{
	String			rest	= 0;

#ifdef sun
#define strtoul	strtol
#endif

	*mask = strtoul(val, &rest, 0);
	return (!(rest && *rest));

#ifdef sun
#undef strtoul
#endif

} /* IsOlBitMask() */


/**
 ** IsFont()
 **/

static Boolean
IsFont OLARGLIST((who, screen, str, p_font, p_converter_data, cset))
	OLARG( OLconst char *,		who)
	OLARG( Screen *,		screen)
	OLARG( String,			str)
	OLARG( Font *,			p_font)
	OLARG( XtPointer *,		p_converter_data)
	OLGRA( String	*,		cset)
{
	XrmRepresentation	type;
	XrmValue		value;
	OLconst char *	res_name;
	OLconst char *	res_class;
	OLconst char *	def_name;
	String		try;
	OLconst DefaultFonts * 	font_table_p = NULL;
	int		i;
	Display *		display = DisplayOfScreen(screen);

#define p_free_the_font (Boolean *)p_converter_data

	/*
	 * Assume we'll allocate the font.
	 */
	*p_free_the_font = True;

	/*
	 * If we were given a non-null font name other than
	 * the Xt default font, try fetching a font by that
	 * name--first, with an optimum point-size/resolution
	 * combination, then if that fails, with exactly the name given.
	 * 
	 * Added the OlDefaultFont resources that switch on the GUI.
	 * Users can specify a value for the OlDefault* resource
	 * which overrides the GUI switch, or they can specify the
	 * value of the default names (e.g. OlOpenLook* and OlMotif*)
	 * to change the GUI dependent defaults.
	 */

	if (!str || !*str)
		font_table_p = &default_fonts[OL_DEFAULT_FONT];
	else  {
		for (i = 0; i < XtNumber(default_fonts); i++)  {
			if (CASELESS_STREQU(str, default_fonts[i].res_name)) {
				font_table_p = &default_fonts[i];
				break;
			}
		}
	}

	/*  If the user passed a string that is not a font resource name,
	    then it is loaded and returned here... */
	if (!font_table_p)  {
		if (MapAndLoadFont(display, screen, str, cset, p_font))
			return (True);
		else  {
			/*  If the users font is not available, try the
			    Open Look default font. */
			font_table_p = &default_fonts[OL_DEFAULT_FONT];
		}
	}

	/*
	 * Either we were asked to fetch the Xt default font or
	 * an OlDefault font.  Try mapping the value of the
	 * given resource and class to a font.
	 */
pass2:
	if (FetchResource(display, font_table_p->res_name, font_table_p->res_class, &type, &value)) {
		if (type == XrmStringToQuark(XtRString)) {
			if (MapAndLoadFont(display, screen, value.addr, cset, p_font))
				return(True);
		} else if (type == XrmStringToQuark(XtRFont))  {
			*p_font = *(Font*)value.addr;
 			*p_free_the_font = False;
			return (True);

		} else if (type == XrmStringToQuark(XtRFontStruct))  {
			*p_font = ((XFontStruct*)value.addr)->fid;
			*p_free_the_font = False;
			return (True);
		}
	}

	/*  As a last resort, try the default font name.  This is stored
	    in a special type of string in the StringList.  */
	if (MapAndLoadFont(display, screen, (OlGetGui() == OL_MOTIF_GUI ?
		font_table_p->motif_default: font_table_p->ol_default),
		cset, p_font))
		return(True);

	/*  One final try to use the XtDefaultFont (if it was not already
	    tried). 
	*/
	if (font_table_p != &default_fonts[XT_DEFAULT_FONT])  {
		font_table_p = &default_fonts[XT_DEFAULT_FONT];
		goto pass2;
	}

	return (False);

#undef p_free_the_font
#undef display
} /* IsFont */

/**
 ** MapAndLoadFont()
 **/

static Boolean
MapAndLoadFont OLARGLIST(( display, screen, str, cset, p_font ))
	OLARG (Display *,	display)
	OLARG (Screen *,	screen)
	OLARG (OLconst char *,	str)
	OLARG (String *,	cset)
	OLGRA (Font *,		p_font)
{
	OLconst char * try;

	if (*p_font = LoadFont(display, str))  {
		return (True);
	}

	/*  Give up with the mapping.  Note that the converter does
	    not necessarily give up yet. */
	OlDisplayStringConversionWarning ( display, str, FONT );
		return(False);

}   /* end of MapAndLoadFont() */


/**
 ** LoadFont()
 **/

static Font
LoadFont OLARGLIST((display, name))
	OLARG( Display *,	display)
	OLGRA( OLconst char *,	name)
{
	Font			f	= 0;

#if	defined(THIS_WORKS)
	char **			list;

	int			nfonts	= 0;


	/*
	 * We avoid a protocol error by looking first before loading.
	 */
	list = XListFonts(display, name, 1, &nfonts);
	if (nfonts && list)
		f = XLoadFont(display, list[0]);
	if (list)
		XFreeFontNames (list);
#else
	/*  Eliminate the possibility of other errors in the queue.  */
	XSync (display, False);
	/*  Change the error handler to catch ALL errors. */
	PrevErrorHandler = XSetErrorHandler(IgnoreBadName);

	GotBadName = False;

	/*  Could cause a protocol error if the name doesn't exist or the
	    font cache is full (alloc). We use this to check for valid font
	    names in a convoluted way.  */
	f = XLoadFont(display, name);

	/*  Sync again to make sure the error handler gets called if it is
	    going to be called. */
	XSync (display, False);

	if (GotBadName)
		f = 0;

	(void)XSetErrorHandler (PrevErrorHandler);
#endif

	return (f);
} /* LoadFont */

/**
 ** IgnoreBadName()
 **/

static int
IgnoreBadName OLARGLIST(( display, event ))
	OLARG (Display *,	display)
	OLGRA (XErrorEvent *,	event)
{
/*
	if (event->error_code == BadName)  {
		GotBadName = True;
		return (0);
	} else
		return ((*PrevErrorHandler)(display, event));
*/
	GotBadName = True;
}

/**
 ** LookupOlDefine()
 **/

#define BUF_SIZE		512	/* number of characters	*/

#define LOWER_CASE(S,D) \
    {									\
	register char *	source	= (S);					\
	register char *	dest	= (D);					\
	register char	ch;						\
									\
	for (; (ch = *source) != 0; source++, dest++) {			\
		LOWER_IT (ch);						\
		*dest = ch;						\
	}								\
	*dest = 0;							\
    }

static OlDefine
LookupOlDefine OLARGLIST(( name, value, flag ))
	OLARG (String,		name)
	OLARG (OlDefine, 	value)
	OLGRA (int,		flag)
{
	char		buf[BUF_SIZE];		/* local buffer		*/
	char *		c_name;			/* lower-cased name	*/
	Cardinal	length = (Cardinal) strlen(name);
	OlDefineNode *	self;
	Bucket *	bucket;
	XrmQuark	quark;

	c_name = (char *) (length < (Cardinal)BUF_SIZE ? buf :
			XtMalloc(length * sizeof(char)));

	LOWER_CASE(name, c_name)

	quark = XrmStringToQuark( (flag == ADD_TO_TABLE &&
				c_name[0] == 'o' &&
				c_name[1] == 'l' &&
				c_name[2] == '_' ? (c_name + 3) : c_name));

	if (c_name != buf) {			/* Free the buffer	*/
		XtFree(c_name);
	}

	bucket = &define_table[HASH_QUARK(quark)];

	self = bucket->array;

	if (flag == ADD_TO_TABLE || self != (OlDefineNode *) NULL) {
		Cardinal i;
					/* Is the quark in the array ?	*/

		for (i=0; i < bucket->elements; ++i) {
			if (self[i].quark == quark) {
				return(self[i].value);
			}
		}

				/* If we're here, we didn't find the
				 * value in the table.			*/

		if (flag == ADD_TO_TABLE) {
			++bucket->elements;

			bucket->array = (OlDefineNode *)
				XtRealloc((char *)bucket->array,
				(bucket->elements * sizeof(OlDefineNode)));
			
			self	= &bucket->array[bucket->elements - 1];
			self->quark = quark;
			self->value = value;
			return(self->value);
		}
	}
	return(0);

} /* LookupOlDefine() */

/**
 ** NewFraction()
 ** StrToFraction()
 ** MulFractions()
 ** FractionToInt()
 **/

static Fraction
NewFraction OLARGLIST(( numerator, denominator ))
	OLARG (int,		numerator)
	OLGRA (int,		denominator)
{
	Fraction		ret;

	ret.numerator   = (long)numerator;
	ret.denominator = (long)denominator;
	return (ret);
}

static Fraction
StrToFraction OLARGLIST(( str, p_rest ))
	OLARG (String,		str)
	OLGRA (String *,	p_rest)
{
	Fraction		ret;

	Boolean			negative	= False;
	Boolean			in_fraction	= False;


	ret.numerator   = 0;
	ret.denominator = 1;

	/*
	 * Just in case we don't have a number here....
	 */
	if (p_rest)
		*p_rest = str;

	while (isspace(*str))
		str++;

	switch (*str) {
	case MINUS:
		negative = True;
		/*FALLTHROUGH*/
	case PLUS:
		str++;
	}

	if (!isdigit(*str) && *str != DECPOINT)
		return (ret);

	do {
		if (*str == DECPOINT)
			in_fraction = True;
		else {
			ret.numerator = ret.numerator * 10L + DIGIT(*str);
			if (in_fraction)
				ret.denominator *= 10L;
		}
	} while (isdigit(*++str) || *str == DECPOINT);

	if (p_rest)
		*p_rest = str;

	if (negative)
		ret.numerator = -ret.numerator;

	return (ret);
}

static Fraction
MulFractions OLARGLIST(( a, b ))
	OLARG (Fraction,	a)
	OLGRA (Fraction,	b)
{
	Boolean			negative = False;


	a.numerator *= b.numerator;
	a.denominator *= b.denominator;

	/*
	 * Save some grief with "long" overflow. If the denominator
	 * is really big, reduce both numerator and denominator to
	 * bring the denominator down to a reasonable size. Essentially
	 * what we're doing here is throwing away the least significant
	 * bits, leaving enough approximately 4 significant DIGITS.
	 * We check the denominator, because we don't want to reduce
	 * it to zero.
	 *
	 * ASSUMPTION: We're not chaining many calculations with these
	 * Fractions, so we don't worry about accumulated error.
	 */
	if (a.numerator < 0) {
		negative = True;
		a.numerator = -a.numerator;
	}
	while (a.denominator > 8192) {
		a.numerator   >>= 1;
		a.denominator >>= 1;
	}
	if (negative)
		a.numerator = -a.numerator;

	return (a);
}

static int
FractionToInt OLARGLIST(( a ))
	OLGRA (Fraction,	a)
{
	long			half = a.denominator / 2;

	if (a.denominator < 1)
		return (0);	/* undefined */
	if (a.numerator < half)
		return (0);
	else
		return ((a.numerator + half) / a.denominator);
}

/**
 ** FetchResource()
 **/

static Boolean
FetchResource OLARGLIST(( display, name, class, p_rep_type, p_value ))
	OLARG (Display *,	display)
	OLARG (OLconst char *,	name)
	OLARG (OLconst char *,	class)
	OLARG (XrmRepresentation *, p_rep_type)
	OLGRA (XrmValue *,	p_value)
{
	XrmName			xrm_name[2];
	XrmClass		xrm_class[2];


	xrm_name[0]  = XrmStringToName(name);
	xrm_name[1]  = 0;
	xrm_class[0] = XrmStringToClass(class);
	xrm_class[1] = 0;
	return (XrmQGetResource(
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
		XtScreenDatabase(XDefaultScreenOfDisplay(display)),
#else
		XtDatabase(display),
#endif
		xrm_name,
		xrm_class,
		p_rep_type,
		p_value
	));
}

/**
 ** caseless_strcmp()
 ** caseless_strncmp()
 **/

static int
caseless_strcmp OLARGLIST(( s1, s2 ))
	OLARG (OLconst char *,	s1)
	OLGRA (OLconst char *,	s2)
{
	if (s1 == s2)
		return (0);
	return (CompareISOLatin1(s1, s2, 0));
} /* caseless_strcmp() */

static int
caseless_strncmp OLARGLIST(( s1, s2, n ))
	OLARG (OLconst char *,	s1)
	OLARG (OLconst char *,	s2)
	OLGRA (register int,	n)
{
	if (s1 == s2)
		return (0);
	return (CompareISOLatin1(s1, s2, n));
} /* caseless_strncmp() */

/**
 ** CompareISOLatin1()
 **/

/*
 * Stolen shamelessly (but why should it have been necessary?)
 * from Xt/Converters.c. Reformatted to fit the local style,
 * to move the good stuff into the macro LOWER_IT, and added
 * the "count" argument.
 */

static int
CompareISOLatin1 OLARGLIST(( first, second, count ))
	OLARG (OLconst char *,		first)
	OLARG (OLconst char *,		second)
	OLGRA (register int,	count)
{
	register unsigned char *ap	   = (unsigned char *)first;
	register unsigned char *bp	   = (unsigned char *)second;

	register Boolean	dont_count = (count == 0);


	for (; (dont_count || --count >= 0) && *ap && *bp; ap++, bp++) {
		register unsigned char	a  = *ap;
		register unsigned char	b  = *bp;

		if (a != b) {
			LOWER_IT (a);
			LOWER_IT (b);
			if (a != b)
				break;
		}
	}
	return ((int)*bp - (int)*ap);
}

/**
 ** OlCvtFontGroupToFontStructList()
 ** FreeFontStructList()
 **/

extern Boolean
OlCvtFontGroupToFontStructList OLARGLIST((display, args, num_args, from, to, converter_data))
    OLARG( Display *,	display )
    OLARG( XrmValue *,	args )
    OLARG( Cardinal *,	num_args )
    OLARG( XrmValue *,	from )
    OLARG( XrmValue *,	to )
    OLGRA( XtPointer *,	converter_data )
{
        OlFontList *		fsl = NULL;
        Font			f;
	XFontStruct **		font;
        Screen *		screen;
	String 			tmp;
	char *			p;
	register String		fname;
	String			cset;
 	Arg	arg[4];
extern char *_OlGetFontGroupDef();
#ifdef I18N
#ifndef sun	/* or other porting that does care I18N */
	eucwidth_t		cwidth;
#endif
#endif
	
	char * fgrpdef;
	int num = 0;

        if (*num_args != 2 )
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadConversion_missingParameter,
			(OLconst char *)"FontGroup", (OLconst char *)"FontStructList"
		);
		/*NOTREACHED*/
	   
	/* retrieve args, a screen and fontGroupDef ptr. */
	screen = *((Screen **)(args[0].addr));
	p = *((char **)(args[1].addr));

	if ((p == NULL) || (!(*p)))
		{
		ConversionDone(OlFontList *, NULL);
		}
	fgrpdef = (char *)XtMalloc(strlen(p) + 1);
	strcpy(fgrpdef, p);

	/* get '/' separated font names list for given fontgroup */

#ifdef __STDC__
	tmp = strstr(fgrpdef, (String)from->addr);
#else
	/* Use strexp() from Xol/regexp.c, provide same functionality */
	if ((String)from->addr == NULL || *(String)from->addr == '\0')
		tmp = fgrpdef;
	else
		tmp = strexp(fgrpdef, fgrpdef, (String)from->addr);
#endif
	tmp = strchr(tmp, '=');
	if (tmp == NULL)
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTinvalidFormat,
			OleCOlToolkitError,
			OleMbadConversion_invalidFormat,
			p
		);
		/*NOTREACHED*/

	else  /* find end of font name list for this font group */
	   {
	   tmp++;
	   while (isspace(*tmp)) tmp++;
	   p = tmp;
	   p = strchr(p, ',');	
	   if (p != NULL)
	      *p = '\0';
	   }

	fsl = (OlFontList *) XtCalloc(1, sizeof(OlFontList));
	fsl->fontl = (XFontStruct **) XtMalloc( 4 * sizeof(XFontStruct *));
	fsl->csname = (char **) XtMalloc( 4 * sizeof(char *));
	fsl->cswidth = (int *) XtMalloc (4 * sizeof(int));

	/* font group def for this font group is needed for Front end since 
	   it is a separate process and can not be passed XFontStruct list.
	*/
	fsl->fgrpdef = (char *) XtMalloc(strlen(tmp) + 1);
	strcpy(fsl->fgrpdef, tmp);
	for(fname = strtok(tmp, (OLconst char *)"/"); fname != NULL; 
				      fname = strtok(NULL, (OLconst char *)"/"))
	  {
	  if (!IsFont((OLconst char *)"FontStruct", screen, fname, &f, converter_data, &cset))
		OlVaDisplayErrorMsg(
			display,
			OleNbadConversion, OleTinvalidFont,
			OleCOlToolkitError,
			OleMbadConversion_invalidFont
		);
		/*NOTREACHED*/

	/* The code set name may be useful for CT -> EUC and
	   EUC -> CT conversion (?). The only way we can obtain
	   code set name is; if the supplied font names are in
	   XLFD format.
	*/
	if (cset != NULL)
		{
		fsl->csname[num] = (String)XtMalloc(strlen(cset) + 1);
		(void)strcpy(fsl->csname[num], cset);
		}
	/* may not need this iff, calloc'd instead of malloc'd */
	else    fsl->csname[num] = NULL;

	if (!(fsl->fontl[num++] = XQueryFont(display, f)))
		OlVaDisplayErrorMsg (
			display,
			OleNbadConversion, OleTbadQueryFont,
			OleCOlToolkitError,
			OleMbadConversion_badQueryFont
		);
		/*NOTREACHED*/
	}

	fsl->num = num;

#ifdef I18N
	/* get width of characters in each code set */

#ifdef sun	/* or other porting that doesn't care I18N */
	(void)getwidth();
	fsl->cswidth[0] = 1;
	fsl->cswidth[1] = 1;
	fsl->cswidth[2] = 1;
	fsl->cswidth[3] = 1;
#else
	(void)getwidth(&cwidth);
	fsl->cswidth[0] = 1;
	fsl->cswidth[1] = cwidth._eucw1;	
	fsl->cswidth[2] = cwidth._eucw2;	
	fsl->cswidth[3] = cwidth._eucw3;	
#endif
#endif

    /* Now compute the font metrics and store them in the font list.
       Note the use of font->ascent/descent instead of
       font->max_bounds.ascent/descent.  These are used for line spacing
       and not strictly for the max bounding box of a theorectical char.
    */
    for (font = fsl->fontl, num = fsl->num;
	 num != 0; font++, num--)
    {
	_OlAssignMax(fsl->max_bounds.lbearing,	(*font)->max_bounds.lbearing);
	_OlAssignMax(fsl->max_bounds.rbearing,	(*font)->max_bounds.rbearing);
	_OlAssignMax(fsl->max_bounds.width,	(*font)->max_bounds.width);
	_OlAssignMax(fsl->max_bounds.ascent,	(*font)->ascent);
	_OlAssignMax(fsl->max_bounds.descent,	(*font)->descent);
    }

        ConversionDone (OlFontList *, fsl);
} /* OlCvtFontGroupToFontStructList */

static void
FreeFontStructList OLARGLIST(( app, to, closure, args, num_args ))
	OLARG (XtAppContext,	app)
	OLARG (XrmValuePtr,	to)
	OLARG (XtPointer,	closure)
	OLARG (XrmValuePtr,	args)
	OLGRA (Cardinal *,	num_args)
{
	OlFontList *fsl = (OlFontList *) to->addr;

	Screen *		screen;

	register int		i;

#define free_the_font	(Boolean)closure


	if (*num_args != 1)
		OlVaDisplayErrorMsg (
			toplevelDisplay, /* ICK! */
			OleNbadDestruction, OleTmissingParameter,
			OleCOlToolkitError,
			OleMbadDestruction_missingParameter,
			(OLconst char *)"FreeFontStructList"
		);
		/*NOTREACHED*/

	screen = *((Screen **)(args[0].addr));

	if (free_the_font)
		{
		for (i=0; i< fsl->num; i++)
			{
			XFreeFont (DisplayOfScreen(screen), fsl->fontl[i]);
			XtFree(fsl->csname[i]);
			}
		XtFree((char *)fsl->csname);
		XtFree(fsl->fgrpdef);
		XtFree((char *)fsl->fontl);
		XtFree((char *)fsl->cswidth);
		XtFree((char *)fsl);
		}

#undef free_the_font
	return;
} /* FreeFontStructList() */
