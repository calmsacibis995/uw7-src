#pragma ident	"@(#)m1.2libs:Xm/ResConvert.c	1.8"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
ifndef OSF_v1_2_4
 * Motif Release 1.2.3 -- font conversion message fixed
else
 * Motif Release 1.2.4
endif
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifdef OSF_v1_2_4
#include <Xm/XmosP.h>
#endif /* OSF_v1_2_4 */
#include <Xm/GadgetP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#include <stdio.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include <string.h>
#include <ctype.h>
#include "XmI.h"
#include "RepTypeI.h"
#include <Xm/ExtObjectP.h>
#include <Xm/VendorS.h>
#include <Xm/VendorSP.h>
#include "MessagesI.h"
#include <Xm/AtomMgr.h>
#ifndef OSF_v1_2_4
#include <Xm/XmosP.h>
#endif /* OSF_v1_2_4 */
#ifdef USE_FONT_OBJECT
#include <Xm/FontObj.h>
#endif /* USE_FONT_OBJECT */


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MSG1	catgets(Xm_catd,MS_ResCvt,MSG_RSC_1,_XmMsgResConvert_0000)
#define MSG2	catgets(Xm_catd,MS_ResCvt,MSG_RSC_1,_XmMsgResConvert_0001)
#else
#define MSG1	_XmMsgResConvert_0000
#define MSG2    _XmMsgResConvert_0001
#endif



#define done( to_rtn, type, value, failure )            \
    {                                                   \
        static type buf ;                               \
                                                        \
        if(    to_rtn->addr    )                        \
        {                                               \
            if(    to_rtn->size < sizeof( type)    )    \
            {                                           \
                failure                                 \
                to_rtn->size = sizeof( type) ;          \
                return( FALSE) ;                        \
                }                                       \
            else                                        \
            {   *((type *) (to_rtn->addr)) = value ;    \
                }                                       \
            }                                           \
        else                                            \
        {   buf = value ;                               \
            to_rtn->addr = (XPointer) &buf ;           \
            }                                           \
        to_rtn->size = sizeof( type) ;                  \
        return( TRUE) ;                                 \
        } 

typedef unsigned char Octet;
typedef Octet *OctetPtr;

typedef enum {
    ct_Dir_StackEmpty,
    ct_Dir_Undefined,
    ct_Dir_LeftToRight,
    ct_Dir_RightToLeft
} ct_Direction;

/*
** ct_Charset is used in the xmstring_to_text conversion to keep track
** of the prevous character set.  The order is not important.
*/
typedef enum {
    cs_none,
    cs_Hanzi,
    cs_JapaneseGCS,
    cs_Katakana,
    cs_KoreanGCS,
    cs_Latin1,
    cs_Latin2,
    cs_Latin3,
    cs_Latin4,
    cs_Latin5,
    cs_LatinArabic,
    cs_LatinCyrillic,
    cs_LatinGreek,
    cs_LatinHebrew,
    cs_NonStandard
} ct_Charset; 

/* Internal context block */
typedef struct _ct_context {
    OctetPtr	    octet;		/* octet ptr into compound text stream */
    OctetPtr	    lastoctet;		/* ptr to last octet in stream */
    struct {				/* flags */
	unsigned    dircs	: 1;	/*   direction control seq encountered */
	unsigned    gchar	: 1;	/*   graphic characters encountered */
	unsigned    ignext	: 1;	/*   ignore extensions */
	unsigned    gl		: 1;	/*   text is for gl */
	unsigned    text	: 1;	/*   current item is a text seq */
    } flags;
    ct_Direction    *dirstack;		/* direction stack pointer */
    unsigned int    dirsp;		/* current dir stack index */
    unsigned int    dirstacksize;	/* size of direction stack */
    OctetPtr	    item;		/* ptr to current item */
    unsigned int    itemlen;		/* length of current item */
    unsigned int    version;		/* version of compound text */
    String	    gl_charset;	/* ptr to GL character set */
    String	    gr_charset;	/* ptr to GR character set */
    unsigned char   gl_charset_size;	/* # of chars in GL charset */
    unsigned char   gr_charset_size;	/* # of chars in GR charset */
    unsigned char   gl_octets_per_char;	/* # of octets per GL char */
    unsigned char   gr_octets_per_char;	/* # of octets per GR char */
    XmString	    xmstring;		/* compound string to be returned */
    XmString	    xmsep;		/* compound string separator segment */
} ct_context;

/*
 *    Segment Encoding Registry datatype and macros
 */

typedef struct _EncodingRegistry {
  char                                *fontlist_tag;
  char                                *ct_encoding;
  struct _EncodingRegistry    *next;
} SegmentEncoding;

#define EncodingRegistryTag(er)       ((SegmentEncoding *)(er))->fontlist_tag
#define EncodingRegistryEncoding(er)  ((SegmentEncoding *)(er))->ct_encoding
#define EncodingRegistryNext(er)      ((SegmentEncoding *)(er))->next

/*
** Define standard character set strings
*/

static char CS_ISO8859_1[] = "ISO8859-1" ;
static char CS_ISO8859_2[] = "ISO8859-2" ;
static char CS_ISO8859_3[] = "ISO8859-3" ;
static char CS_ISO8859_4[] = "ISO8859-4" ;
static char CS_ISO8859_5[] = "ISO8859-5" ;
static char CS_ISO8859_6[] = "ISO8859-6" ;
static char CS_ISO8859_7[] = "ISO8859-7" ;
static char CS_ISO8859_8[] = "ISO8859-8" ;
static char CS_ISO8859_9[] = "ISO8859-9" ;
static char CS_JISX0201[] = "JISX0201.1976-0" ;
static char CS_GB2312_0[] = "GB2312.1980-0" ;
static char CS_GB2312_1[] = "GB2312.1980-1" ;
static char CS_JISX0208_0[] = "JISX0208.1983-0" ;
static char CS_JISX0208_1[] = "JISX0208.1983-1" ;
static char CS_KSC5601_0[] = "KSC5601.1987-0" ;
static char CS_KSC5601_1[] = "KSC5601.1987-1" ;


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void FetchUnitType() ;
static void FetchDisplayArg() ;
static Boolean _StringToEntity() ;
static Boolean _XmCvtStringToWidget() ;
static Boolean _XmCvtStringToWindow() ;
static Boolean _XmCvtStringToChar() ;
static Boolean _XmCvtStringToKeySym() ;
static void _XmCvtStringToXmStringDestroy() ;
static Boolean _XmCvtStringToXmString() ;
static void _XmCvtStringToXmFontListDestroy() ;
static Boolean _XmCvtStringToXmFontList() ;
static Boolean GetNextFontListEntry() ;
static Boolean GetFontName() ;
static Boolean GetFontTag() ;
static Boolean GetNextXmString() ;
static Boolean _XmCvtStringToXmStringTable() ;
static void _XmXmStringCvtDestroy() ;
static Boolean _XmCvtStringToStringTable() ;
static void _XmStringCvtDestroy() ;
static Boolean _XmCvtStringToHorizontalPosition() ;
static Boolean _XmCvtStringToHorizontalDimension() ;
static Boolean _XmCvtStringToVerticalPosition() ;
static Boolean _XmCvtStringToVerticalDimension() ;
static void _XmConvertStringToButtonTypeDestroy() ;
static Boolean _XmConvertStringToButtonType() ;
static void _XmCvtStringToKeySymTableDestroy() ;
static Boolean _XmCvtStringToKeySymTable() ;
static void _XmCvtStringToCharSetTableDestroy() ;
static Boolean _XmCvtStringToCharSetTable() ;
static Boolean _XmCvtStringToBooleanDimension() ;
static SegmentEncoding * _find_encoding() ;
static Boolean processCharsetAndText(); 
static Boolean processESCHack() ;
static Boolean processExtendedSegmentsHack() ;
static Boolean cvtTextToXmString() ;
static void outputXmString() ;
static XmString concatStringToXmString() ;
static Boolean processESC() ;
static Boolean processCSI() ;
static Boolean processExtendedSegments() ;
static Boolean process94n() ;
static Boolean process94GL() ;
static Boolean process94GR() ;
static Boolean process96GR() ;
static Boolean cvtXmStringToText() ;
static OctetPtr ctextConcat() ;
static Boolean _XmCvtStringToAtomList() ;
static void _XmSimpleDestructor() ;
static Boolean OneOf() ;
static char * GetNextToken() ;
static Boolean _XmCvtStringToCardinal() ;
static Boolean _XmCvtStringToTextPosition();
static Boolean _XmCvtStringToTopItemPosition();
static Boolean isInteger() ;
#else

static void FetchUnitType( 
                        Widget widget,
                        Cardinal *size,
                        XrmValue *value) ;
static void FetchDisplayArg(
                        Widget widget,
                        Cardinal *size,
                        XrmValue *value) ;
static Boolean _StringToEntity( 
                        Display *disp,
                        XrmValue *args,
                        Cardinal *n_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToWidget( 
                        Display *disp,
                        XrmValue *args,
                        Cardinal *n_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToWindow( 
                        Display *disp,
                        XrmValue *args,
                        Cardinal *n_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToChar( 
                        Display *disp,
                        XrmValue *args,
                        Cardinal *n_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToKeySym( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static void _XmCvtStringToXmStringDestroy( 
                        XtAppContext app,
                        XrmValue *to,
                        XtPointer converter_data,
                        XrmValue *args,
                        Cardinal *num_args) ;
static Boolean _XmCvtStringToXmString( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static void _XmCvtStringToXmFontListDestroy( 
                        XtAppContext app,
                        XrmValue *to,
                        XtPointer converter_data,
                        XrmValue *args,
                        Cardinal *num_args) ;
static Boolean _XmCvtStringToXmFontList( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean GetNextFontListEntry( 
                        char **s,
                        char **fontNameRes,
                        char **fontTagRes,
                        XmFontType *fontTypeRes,
                        char *delim) ;
static Boolean GetFontName( 
                        char **s,
                        char **name,
                        char *delim) ;
static Boolean GetFontTag( 
                        char **s,
                        char **tag,
                        char *delim) ;
static Boolean GetNextXmString( 
                        char **s,
                        char **cs) ;
static Boolean _XmCvtStringToXmStringTable( 
                        Display *dpy,
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val,
                        XtPointer *data) ;
static void _XmXmStringCvtDestroy( 
                        XtAppContext app,
                        XrmValue *to,
                        XtPointer data,
                        XrmValue *args,
                        Cardinal *num_args) ;
static Boolean _XmCvtStringToStringTable( 
                        Display *dpy,
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val,
                        XtPointer *data) ;
static void _XmStringCvtDestroy( 
                        XtAppContext app,
                        XrmValue *to,
                        XtPointer data,
                        XrmValue *args,
                        Cardinal *num_args) ;
static Boolean _XmCvtStringToHorizontalPosition( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToHorizontalDimension( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToVerticalPosition( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToVerticalDimension( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static void _XmConvertStringToButtonTypeDestroy( 
                        XtAppContext app,
                        XrmValue *to,
                        XtPointer converter_data,
                        XrmValue *args,
                        Cardinal *num_args) ;
static Boolean _XmConvertStringToButtonType( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static void _XmCvtStringToKeySymTableDestroy( 
                        XtAppContext app,
                        XrmValue *to,
                        XtPointer converter_data,
                        XrmValue *args,
                        Cardinal *num_args) ;
static Boolean _XmCvtStringToKeySymTable( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static void _XmCvtStringToCharSetTableDestroy( 
                        XtAppContext app,
                        XrmValue *to,
                        XtPointer converter_data,
                        XrmValue *args,
                        Cardinal *num_args) ;
static Boolean _XmCvtStringToCharSetTable( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToBooleanDimension( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static SegmentEncoding * _find_encoding( 
                        char *fontlist_tag) ;
static Boolean processCharsetAndText(XmStringCharSet tag,
				     OctetPtr	ctext,
#if NeedWidePrototypes
				     int 	separator,
#else
				     Boolean	separator,
#endif /* NeedWidePrototypes */
				     OctetPtr	*outc,
				     unsigned int	*outlen,
				     ct_Charset	*prev);
static Boolean processESCHack( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int final) ;
#else
                        Octet final) ;
#endif /* NeedWidePrototypes */
static Boolean processExtendedSegmentsHack( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int final) ;
#else
                        Octet final) ;
#endif /* NeedWidePrototypes */
static Boolean cvtTextToXmString( 
                        XrmValue *from,
                        XrmValue *to) ;
static void outputXmString( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int separator) ;
#else
                        Boolean separator) ;
#endif /* NeedWidePrototypes */
static XmString concatStringToXmString( 
                        XmString compoundstring,
                        char *textstring,
                        char *charset,
#if NeedWidePrototypes
                        int direction,
                        int separator) ;
#else
                        XmStringDirection direction,
                        Boolean separator) ;
#endif /* NeedWidePrototypes */
static Boolean processESC( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int final) ;
#else
                        Octet final) ;
#endif /* NeedWidePrototypes */
static Boolean processCSI( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int final) ;
#else
                        Octet final) ;
#endif /* NeedWidePrototypes */
static Boolean processExtendedSegments( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int final) ;
#else
                        Octet final) ;
#endif /* NeedWidePrototypes */
static Boolean process94n( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int final) ;
#else
                        Octet final) ;
#endif /* NeedWidePrototypes */
static Boolean process94GL( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int final) ;
#else
                        Octet final) ;
#endif /* NeedWidePrototypes */
static Boolean process94GR( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int final) ;
#else
                        Octet final) ;
#endif /* NeedWidePrototypes */
static Boolean process96GR( 
                        ct_context *ctx,
#if NeedWidePrototypes
                        int final) ;
#else
                        Octet final) ;
#endif /* NeedWidePrototypes */
static Boolean cvtXmStringToText( 
                        XrmValue *from,
                        XrmValue *to) ;
static OctetPtr ctextConcat( 
                        OctetPtr str1,
                        unsigned int str1len,
                        OctetPtr str2,
                        unsigned int str2len) ;
static Boolean _XmCvtStringToAtomList( 
                        Display *dpy,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static void _XmSimpleDestructor( 
                        XtAppContext app,
                        XrmValue *to,
                        XtPointer data,
                        XrmValue *args,
                        Cardinal *num_args) ;
static Boolean OneOf( 
#if NeedWidePrototypes
                        int c,
#else
                        char c,
#endif /* NeedWidePrototypes */
                        char *set) ;
static char * GetNextToken( 
                        char *src,
                        char *delim) ;
static Boolean _XmCvtStringToCardinal( 
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToTextPosition(
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean _XmCvtStringToTopItemPosition(
                        Display *display,
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean isInteger(
                        String string,
                        int *value) ;
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/
  
static SegmentEncoding _encoding_registry = 
{ XmFONTLIST_DEFAULT_TAG, XmFONTLIST_DEFAULT_TAG, NULL};
static SegmentEncoding *_encoding_registry_ptr = &_encoding_registry;

static void 
#ifdef _NO_PROTO
FetchUnitType( widget, size, value )
        Widget widget ;
        Cardinal *size ;
        XrmValue *value ;
#else
FetchUnitType(
        Widget widget,
        Cardinal *size,
        XrmValue *value )
#endif /* _NO_PROTO */
{

    if (widget == NULL) {
	XtErrorMsg("missingWidget", "fetchUnitType", "XtToolkitError",
                   "FetchUnitType called without a widget to reference",
                   (String*)NULL, (Cardinal*)NULL);
    }

    if (XmIsExtObject(widget))
      {
	  widget = ((XmExtObject)widget)->ext.logicalParent;
      }

    if (XmIsGadget(widget))
      {
	  XmGadget	gadget = (XmGadget) widget;

	  value->addr = (XPointer)&(gadget->gadget.unit_type);
      }

    else if (XmIsManager(widget))
      {
	  XmManagerWidget	manager = (XmManagerWidget) widget;

	  value->addr = (XPointer)&(manager->manager.unit_type);
      }
    else if (XmIsPrimitive(widget))
      {
	  XmPrimitiveWidget	primitive = (XmPrimitiveWidget) widget;

	  value->addr = (XPointer)&(primitive->primitive.unit_type);
      }
    else 
      _XmWarning(NULL, MSG1);

    value->size = sizeof(unsigned char);
}

static XtConvertArgRec resIndConvertArgs[] = {
    { XtProcedureArg, 
      (XtPointer)FetchUnitType, 
      0
    },
    { XtWidgetBaseOffset,
        (XtPointer) XtOffsetOf( struct _WidgetRec, core.screen),
        sizeof (Screen*)
    }
};

static XtConvertArgRec parentConvertArgs[] = {
    { XtBaseOffset, 
      (XtPointer) XtOffsetOf(WidgetRec, core.parent), 
      sizeof(Widget),
    }
};

/*ARGSUSED*/
static void
#ifdef _NO_PROTO
FetchDisplayArg(widget, size, value)
    Widget widget;
    Cardinal *size;
    XrmValue* value;
#else
FetchDisplayArg(
    Widget widget,
    Cardinal *size,
    XrmValue *value)
#endif /* _NO_PROTO */
{
    if (widget == NULL)
        XtErrorMsg("missingWidget", "fetchDisplayArg", "XtToolkitError",
                   "FetchDisplayArg called without a widget to reference",
                   (String*)NULL, (Cardinal*)NULL);
        /* can't return any useful Display and caller will de-ref NULL,
           so aborting is the only useful option */

    value->size = sizeof(Display*);
    value->addr = (XPointer)&(XtScreenOfObject(widget))->display;
}

static XtConvertArgRec displayConvertArg[] = {
    {XtProcedureArg, (XtPointer)FetchDisplayArg, 0},
};


/* Motif widget set version number.  Accessable by application - externed   */
/* in Xm.h.  Set to the value returned by XmVersion when RegisterConverters */
/* is called.                                                               */

externaldef(xmuseversion) int xmUseVersion;



/************************************************************************
 *
 *  _XmRegisterConverters
 *	Register all of the Xm resource type converters.  Retain a
 *	flag indicating whether the converters have already been
 *	registered.
 *
 ************************************************************************/
void
#ifdef _NO_PROTO
_XmRegisterConverters()
#else
_XmRegisterConverters( void )
#endif /* _NO_PROTO */
{
    static Boolean registered = False ;

    if(    !registered    )
    {
        xmUseVersion = XmVersion;

        _XmRepTypeInstallConverters() ;

        XtSetTypeConverter( XmRString, XmRWidget, _XmCvtStringToWidget, 
                            parentConvertArgs, XtNumber(parentConvertArgs),
                            XtCacheNone, (XtDestructor) NULL) ;
        XtSetTypeConverter( XmRString, XmRWindow, _XmCvtStringToWindow, 
                            parentConvertArgs, XtNumber(parentConvertArgs),
                            XtCacheNone, (XtDestructor) NULL) ;
        XtSetTypeConverter( XmRString, XmRChar, _XmCvtStringToChar, NULL, 0,
                                                           XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRString, XmRFontList, _XmCvtStringToXmFontList,
                            displayConvertArg, XtNumber(displayConvertArg),
			    XtCacheByDisplay, _XmCvtStringToXmFontListDestroy);
        XtSetTypeConverter( XmRString, XmRXmString, _XmCvtStringToXmString,
			    NULL, 0, (XtCacheNone | XtCacheRefCount), 
			    _XmCvtStringToXmStringDestroy ) ;
        XtSetTypeConverter( XmRString, XmRKeySym, _XmCvtStringToKeySym,
                                                  NULL, 0, XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRString, XmRHorizontalPosition,
                           _XmCvtStringToHorizontalPosition, resIndConvertArgs,
                             XtNumber( resIndConvertArgs), XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRString, XmRHorizontalDimension,
                          _XmCvtStringToHorizontalDimension, resIndConvertArgs,
                             XtNumber( resIndConvertArgs), XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRString, XmRVerticalPosition,
                             _XmCvtStringToVerticalPosition, resIndConvertArgs,
                             XtNumber( resIndConvertArgs), XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRString, XmRVerticalDimension,
                            _XmCvtStringToVerticalDimension, resIndConvertArgs,
                             XtNumber( resIndConvertArgs), XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRString, XmRBooleanDimension, 
                             _XmCvtStringToBooleanDimension, resIndConvertArgs,
                             XtNumber( resIndConvertArgs), XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRCompoundText, XmRXmString, XmCvtTextToXmString,
                               resIndConvertArgs, XtNumber( resIndConvertArgs),
                                                           XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRXmString, XmRCompoundText, XmCvtXmStringToText,
                               resIndConvertArgs, XtNumber( resIndConvertArgs),
                                                           XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRString, XmRCharSetTable,
                      _XmCvtStringToCharSetTable, NULL, 0, XtCacheNone,
                                           _XmCvtStringToCharSetTableDestroy) ;
        XtSetTypeConverter( XmRString, XmRKeySymTable,
                               _XmCvtStringToKeySymTable, NULL, 0, XtCacheNone,
                                            _XmCvtStringToKeySymTableDestroy) ;
        XtSetTypeConverter( XmRString, XmRButtonType, 
                    _XmConvertStringToButtonType, NULL, 0, XtCacheNone, 
                                         _XmConvertStringToButtonTypeDestroy) ;
        XtSetTypeConverter( XmRString, XmRXmStringTable, 
                    _XmCvtStringToXmStringTable, NULL, 0,
                      (XtCacheNone | XtCacheRefCount), _XmXmStringCvtDestroy) ;
        XtSetTypeConverter (XmRString, XmRStringTable,
                      _XmCvtStringToStringTable, NULL, 0,
                        (XtCacheNone | XtCacheRefCount), _XmStringCvtDestroy) ;
        XtSetTypeConverter( XmRString, XmRAtomList, 
                    _XmCvtStringToAtomList, NULL, 0,
                      (XtCacheNone | XtCacheRefCount), _XmSimpleDestructor) ;
        XtSetTypeConverter( XmRString, XmRCardinal,
                            _XmCvtStringToCardinal, NULL, 0,
			    XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRString, XmRTextPosition,
                            _XmCvtStringToTextPosition, NULL, 0,
                            XtCacheNone, NULL) ;
        XtSetTypeConverter( XmRString, XmRTopItemPosition,
                            _XmCvtStringToTopItemPosition, NULL, 0,
                            XtCacheNone, NULL) ;
        registered = True;
        }
    return ;
    }

/* For binary compatibility only: */
void XmRegisterConverters() { _XmRegisterConverters() ; } 


/************************************************************************
 *
 *  _XmWarning
 *	Build up a warning message and call XtWarning to get it displayed.
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmWarning( w, message )
        Widget w ;
        char *message ;
#else
_XmWarning(
        Widget w,
        char *message )
#endif /* _NO_PROTO */
{
   char buf[1024];
   register int pos;
   char * newline_pos;


   pos = 0;

   if (w != NULL)
   {
      strcpy (&buf[pos], "\n    Name: ");
      pos += 11;
      strcpy (&buf[pos], XrmQuarkToString (w->core.xrm_name));
      pos += strlen (XrmQuarkToString (w->core.xrm_name));
      strcpy (&buf[pos], "\n    Class: ");
      pos += 12;
      strcpy (&buf[pos], w->core.widget_class->core_class.class_name);
      pos += strlen (w->core.widget_class->core_class.class_name);
   }

   strcpy (&buf[pos], "\n");
   pos++;

   do
   {
      strcpy (&buf[pos], "    ");
      pos += 4;

      newline_pos = strchr (message, '\n');

      if (newline_pos == NULL)
      {
         strcpy (&buf[pos], message);
         pos += strlen (message);
         break;
      }
      else
      {
         strncpy (&buf[pos], message, (int) (newline_pos - message + 1));
         pos +=  (int) (newline_pos - message + 1);
         message +=  (int) (newline_pos - message + 1);
      }
   }
   while (newline_pos != NULL);

   strcpy (&buf[pos], "\n");

   XtWarning (buf);
}



/************************************************************************
 *
 *  _XmStringsAreEqual
 *	Compare two strings and return true if equal.
 *	The comparison is on lower cased strings.  It is the callers
 *	responsibility to ensure that test_str is already lower cased.
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmStringsAreEqual( in_str, test_str )
        register char *in_str ;
        register char *test_str ;
#else
_XmStringsAreEqual(
        register char *in_str,
        register char *test_str )
#endif /* _NO_PROTO */
{
        register char i ;

    if(    ((in_str[0] == 'X') || (in_str[0] == 'x'))
        && ((in_str[1] == 'M') || (in_str[1] == 'm'))    )
    {   
        in_str +=2;
        } 
    do
    {
 /*
  * Fix for 5330 - For OS compatibility with old operating systems, always
  *                check a character with isupper before using tolower on it.
  */
        if (isupper((unsigned char)*in_str))
            i = (char) tolower((unsigned char) *in_str) ;
        else
            i = *in_str;
        in_str++;

        if(    i != *test_str++    )
        {   
            return( False) ;
            } 
    }while(    i    ) ;

    return( True) ;
    }

/************************************************************************
 *
 *  _StringToEntity
 *    Allow widget or window to be specified by name
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
_StringToEntity( disp, args, n_args, from, to, converter_data )
        Display *disp ;
        XrmValue *args ;
        Cardinal *n_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_StringToEntity(
        Display *disp,
        XrmValue *args,
        Cardinal *n_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    static Widget  itsChild;
    Widget         theParent;
    Boolean        success;

    if (*n_args != 1) 
      XtAppWarningMsg (
            XtDisplayToApplicationContext(disp),
            "wrongParameters", "cvtStringToWidget", "XtToolkitError",
            "Cannot convert widget name to Widget because parent is unknown.",
            (String*)NULL, (Cardinal*)NULL );

    theParent = *(Widget*) args[0].addr;
    itsChild  = XtNameToWidget( theParent, (String) from->addr );

    success   = !( itsChild == NULL );

    if ( success ) 
    { 
        if (to->addr == NULL)
            to->addr = (XPointer) &itsChild;

        else if (to->size < sizeof(Widget))  
            success  = FALSE;

        else
            *(Widget*) to->addr = itsChild;

        to->size = sizeof(Widget);    
    } 
    else
        XtDisplayStringConversionWarning(disp, from->addr, "Widget");     

    return ( success );
}

/************************************************************************
 *
 *  _XmCvtStringToWidget
 *    Allow widget to be specified by name
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
_XmCvtStringToWidget( disp, args, n_args, from, to, converter_data )
        Display *disp ;
        XrmValue *args ;
        Cardinal *n_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToWidget(
        Display *disp,
        XrmValue *args,
        Cardinal *n_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    return (_StringToEntity( disp, args, n_args, from, to, converter_data ) );
}

/************************************************************************
 *
 *  _XmCvtStringToWindow
 *    Allow widget(Window) to be specified by name
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
_XmCvtStringToWindow( disp, args, n_args, from, to, converter_data )
        Display *disp ;
        XrmValue *args ;
        Cardinal *n_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToWindow(
        Display *disp,
        XrmValue *args,
        Cardinal *n_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    return (_StringToEntity( disp, args, n_args, from, to, converter_data ) );
}



/************************************************************************
 *
 *  _XmCvtStringToChar
 *	Convert string to a single character (a mnemonic)
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
_XmCvtStringToChar( disp, args, n_args, from, to, converter_data )
        Display *disp ;
        XrmValue *args ;
        Cardinal *n_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToChar(
        Display *disp,
        XrmValue *args,
        Cardinal *n_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
   unsigned char in_char = *((unsigned char *) (from->addr)) ;

   done( to, unsigned char, in_char, ; )
   }

/************************************************************************
 *
 *  XmCvtStringToUnitType
 *	Convert a string to resolution independent unit type.
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmCvtStringToUnitType( args, num_args, from_val, to_val )
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
XmCvtStringToUnitType(
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
   char *in_str = (char *) (from_val->addr);
   static unsigned char i;

   to_val->size = sizeof (unsigned char);
   to_val->addr = (XPointer) &i;

   if (_XmStringsAreEqual (in_str, "pixels")) 
      i = XmPIXELS;
   else if (_XmStringsAreEqual (in_str, "100th_millimeters")) 
      i = Xm100TH_MILLIMETERS;
   else if (_XmStringsAreEqual (in_str, "1000th_inches")) 
      i = Xm1000TH_INCHES;
   else if (_XmStringsAreEqual (in_str, "100th_points")) 
      i = Xm100TH_POINTS;
   else if (_XmStringsAreEqual (in_str, "100th_font_units")) 
      i = Xm100TH_FONT_UNITS;
   else
   {
      to_val->size = 0;
      to_val->addr = NULL;
      XtStringConversionWarning ((char *)from_val->addr, XmRUnitType);
   }
}

/************************************************************************
 *
 *   _XmCvtStringToKeySym
 *	Convert a string to a KeySym
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
_XmCvtStringToKeySym( display, args, num_args, from, to, converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToKeySym(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
        KeySym tmpKS = XStringToKeysym( (char *) (from->addr)) ;

    if(    tmpKS != NoSymbol    )
    {   
        done( to, KeySym, tmpKS, ; )
        } 
    XtStringConversionWarning( (char *) from->addr, XmRKeySym) ;

    return( FALSE) ;
    }

static void
#ifdef _NO_PROTO
_XmCvtStringToXmStringDestroy( app, to, converter_data, args, num_args )
        XtAppContext app ;
        XrmValue *to ;
        XtPointer converter_data ;
        XrmValue *args ;
        Cardinal *num_args ;
#else
_XmCvtStringToXmStringDestroy(
        XtAppContext app,
        XrmValue *to,
        XtPointer converter_data,
        XrmValue *args,
        Cardinal *num_args)
#endif /* _NO_PROTO */
{   
    XmStringFree( *((XmString *) to->addr)) ;
    return ;
    } 

/************************************************************************
 *
 *  _XmCvtStringToXmString
 *	Convert an ASCII string to a XmString.
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
_XmCvtStringToXmString( display, args, num_args, from, to, converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToXmString(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
        XmString tmpStr ;

    if(    from->addr    )
    {   
        tmpStr = XmStringCreateLocalized( (char *) from->addr);
        if(    tmpStr    )
        {   
            done( to, XmString, tmpStr, XmStringFree( tmpStr) ; )
            } 
        } 
    XtStringConversionWarning( ((char *) from->addr), XmRXmString) ;

    return( FALSE) ;
    }

static void
#ifdef _NO_PROTO
_XmCvtStringToXmFontListDestroy( app, to, converter_data, args, num_args )
        XtAppContext app ;
        XrmValue *to ;
        XtPointer converter_data ;
        XrmValue *args ;
        Cardinal *num_args ;
#else
_XmCvtStringToXmFontListDestroy(
        XtAppContext app,
        XrmValue *to,
        XtPointer converter_data,
        XrmValue *args,
        Cardinal *num_args)
#endif /* _NO_PROTO */
{   
    XmFontListFree( *((XmFontList *) to->addr)) ;

    return ;
    }

/************************************************************************
 *
 *  _XmCvtStringToXmFontList
 *	Convert a string to a fontlist.  This is in the form :
 *  
 *  <XmFontList>	::=	<fontlistentry> { ',' <fontlistentry> }
 *  
 *  <fontlistentry>	::=	<fontset> | <font>
 *  
 *  <fontset>		::=	<fontname> { ';' <fontname> } ':' [ <tag> ]
 *  
 *  <font>		::=	<fontname> [ '=' <tag> ]
 *  
 *  <fontname>		::=	<XLFD String>
 *  
 *  <tag>		::=	<characters from ISO646IRV except newline>
 *  
 *  
 *  Additional syntax is allowed for compatibility with Xm1.1:
 *  
 *  1. The fontlistentries may be separated by whitespace, rather than ','.
 *  2. Empty fontlistentries are ignored.
 *
 ************************************************************************/

static Boolean
#ifdef _NO_PROTO
_XmCvtStringToXmFontList( dpy, args, num_args, from, to, converter_data )
        Display *dpy ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToXmFontList(
        Display *dpy,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    Boolean got_it = FALSE ;
    char *s;
    char *newString;
    char *sPtr;
    char *fontName;
    char *fontTag;
    XmFontType fontType;
    char delim;
    XmFontListEntry fontListEntry;
    XmFontList      fontList = NULL;
    Display *display;

    display = *(Display**)args[0].addr;

    if (from->addr)
    {   
#ifdef USE_FONT_OBJECT
	Widget font_obj = XtNameToWidget((Widget)XmGetXmDisplay(display), "fontObject");

	if (font_obj)  {
		if (strcmp(from->addr, XmNserifFamilyFontList) == 0)
			XtVaGetValues(font_obj, XmNserifFamilyFontList,
				&fontList, NULL);
		else if (strcmp(from->addr, XmNsansSerifFamilyFontList) == 0)
			XtVaGetValues(font_obj, XmNsansSerifFamilyFontList,
				&fontList, NULL);
		else if (strcmp(from->addr, XmNmonospacedFamilyFontList) == 0)
			XtVaGetValues(font_obj, XmNmonospacedFamilyFontList,
				&fontList, NULL);
		if (fontList != NULL) {
			fontList = XmFontListCopy(fontList);
        		done( to, XmFontList, fontList, XmFontListFree( fontList) ; )
		}
	}
#endif /* USE_FONT_OBJECT */
        /*
         *  Copy the input string.
         */

        s = (char *) from->addr ;
        newString = XtNewString (s);
        sPtr = newString;

        /*
	 * Parse the fontlistentries.
         */

        if (!GetNextFontListEntry (&sPtr, &fontName, &fontTag,
				   &fontType, &delim))
        {
            /* Begin fixing OSF 4735 */
 
              XtFree (newString);
              s = (char *) XmDEFAULT_FONT;
              newString = XtNewString (s);
              sPtr = newString;


              if (!GetNextFontListEntry (&sPtr, &fontName, &fontTag,
                                     &fontType, &delim))
               {
               XtFree (newString);
               _XmWarning(NULL, MSG2) ;
               exit( 1) ;
               }
 
               do
                 {
                   if (*fontName) {
                         fontListEntry = XmFontListEntryLoad (display, fontName,
                                                           fontType, fontTag);
                   if (fontListEntry != NULL)
                      {
                          got_it = TRUE ;
                          fontList =
                              XmFontListAppendEntry (fontList, fontListEntry);
                          XmFontListEntryFree (&fontListEntry);
                      }
                   else
                          XtStringConversionWarning(fontName, XmRFontList);
 
                      }
                  }
                while ((delim == ',') && *++sPtr &&
                            GetNextFontListEntry (&sPtr, &fontName, &fontTag,
                                           &fontType, &delim));
                XtFree (newString);
          }
            /* End fixing OSF 4735 */

        else
        {
	    do
	    {
                if (*fontName) {
		    fontListEntry = XmFontListEntryLoad (display, fontName,
						         fontType, fontTag);
		    if (fontListEntry != NULL)
                    {
                        got_it = TRUE ;
                        fontList =
			    XmFontListAppendEntry (fontList, fontListEntry);
                        XmFontListEntryFree (&fontListEntry);
                    }

		    /* CR4721 - If fontListEntry not loaded, inform user */
		    else
		      XtStringConversionWarning(fontName, XmRFontList);
		}
	    }
            while ((delim == ',') && *++sPtr &&
                   GetNextFontListEntry (&sPtr, &fontName, &fontTag,
					 &fontType, &delim));
            XtFree (newString);
        }
    } 

    if (got_it)
    {
        done( to, XmFontList, fontList, XmFontListFree( fontList) ; )
    }
    XtStringConversionWarning( (char *) from->addr, XmRFontList) ;
    return( FALSE) ;
}

/************************************************************************
 *
 *  GetNextFontListEntry
 *  
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
GetNextFontListEntry (s, fontNameRes, fontTagRes, fontTypeRes, delim)
    char **s ;			/* return:  at delimiter found */
    char **fontNameRes ;	/* return:  fontname */
    char **fontTagRes ;		/* return:  font or fontset tag */
    XmFontType *fontTypeRes ;	/* return:  font type */
    char *delim ;		/* return:  delimiter found */
#else
GetNextFontListEntry (
    char **s ,
    char **fontNameRes ,
    char **fontTagRes ,
    XmFontType *fontTypeRes ,
    char *delim )
#endif /* _NO_PROTO */
{
    char *fontName;
    char *fontTag;
    char *fontPtr;
    String params[2];
    Cardinal num_params;

    *fontTypeRes = XmFONT_IS_FONT;

    /*
     * Parse the fontname or baselist.
     */

    if (!GetFontName(s, &fontName, delim))
    {
	return (FALSE);
    }

    while (*delim == ';')
    {
        *fontTypeRes = XmFONT_IS_FONTSET;

        **s = ',';
        (*s)++;

        if (!GetFontName(s, &fontPtr, delim))
        {
	    return (FALSE);
        }
    }

    /*
     * Parse the fontsettag or fonttag.
     */

    if (*delim == ':')
    {
        *fontTypeRes = XmFONT_IS_FONTSET;

	(*s)++;
        if (!GetFontTag(s, &fontTag, delim))
        {
	    fontTag = XmFONTLIST_DEFAULT_TAG;
        }
    }
    else
    {
	if (*fontTypeRes == XmFONT_IS_FONTSET)
	{
	    /* CR4721 */
            params[0] = fontName;
	    num_params = 1;
	    XtWarningMsg("conversionWarning", "string", "XtToolkitError",
			 "Missing colon in font string \"%s\", any remaining fonts in list unparsed",
			 params, &num_params);

	    return (FALSE);
	}

        if (*delim == '=')
        {
	    (*s)++;
            if (!GetFontTag(s, &fontTag, delim))
            {
	        return (FALSE);
            }
        }
	else if ((*delim == ',') || *delim == '\0')
	{
	    fontTag = XmFONTLIST_DEFAULT_TAG;
	}
	else
        {
	    /* CR4721 */
	    params[0] = fontTag;
	    num_params = 1;
	    XtWarningMsg("conversionWarning", "string", "XtToolkitError",
			 "Invalid delimeter in tag \"%s\", any remaining fonts in list unparsed",
			 params, &num_params);

	    return (FALSE);
        }
    }
    *fontNameRes = fontName;
    *fontTagRes = fontTag;
    return (TRUE);
}

/************************************************************************
 *
 *  GetFontName
 *  
 *
 *  May return null string as fontname (Xm1.1 compatibility).
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
GetFontName (s, name, delim)
    char **s;		/* return:  at delimiter found */
    char **name;	/* return:  fontname */
    char *delim;	/* return:  delimiter found */
#else
GetFontName (
    char **s,
    char **name,
    char *delim )
#endif /* _NO_PROTO */
{
    String params[2];
    Cardinal num_params;

    /*
     * Skip any leading whitespace.
     */

    while (**s != '\0' && isspace((unsigned char)**s))
    {
	(*s)++;
    }
    if (**s == '\0')
    {
        return (FALSE);
    }

    /*
     * Have nonspace.  Find the end of the name.
     */

    *name = *s;
    if (**s == '"')
    {
        (*name)++;
        (*s)++;
        while (**s != '\0' && (**s != '"'))
	{
            (*s)++;
	}
        if (**s == '\0')
        {
	  /* CR4721 */
	  params[0] = --(*name);
	  num_params = 1;
	  XtWarningMsg("conversionWarning", "string", "XtToolkitError",
		       "Unmatched quotation marks in string \"%s\", any remaining fonts in list unparsed",
		       params, &num_params);

            return (FALSE);
        }
        **s = '\0';
        (*s)++;
	*delim = **s;
    }
    else
    {
        while ((**s != '\0') &&
	       (**s != ',') && (**s != ':') && (**s != ';') && (**s != '='))
	{
	      (*s)++;
	}
	*delim = **s;
        **s = '\0';
    }

    return (TRUE);
}

/************************************************************************
 *
 *  GetFontTag
 *  
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
GetFontTag (s, tag, delim)
    char **s;		/* return:  at delimiter found */
    char **tag;		/* return:  tag */
    char *delim;	/* return:  delimiter found */
#else
GetFontTag (
    char **s,
    char **tag,
    char *delim )
#endif /* _NO_PROTO */
{
    String params[2];
    Cardinal num_params;
#ifdef OSF_v1_2_4
    Boolean needs_tag = (*delim == '=');
 
#endif /* OSF_v1_2_4 */
    /*
     * Skip any leading whitespace.
     */

    while (**s != '\0' && isspace((unsigned char)**s))
    {
#ifndef OSF_v1_2_4
        /* CR4721 */
        params[0] = --(*tag);
	num_params = 1;
	XtWarningMsg("conversionWarning", "string", "XtToolkitError",
		     "Unmatched quotation marks in tag \"%s\", any remaining fonts in list unparsed",
		     params, &num_params);

#endif /* OSF_v1_2_4 */
	(*s)++;
    }
    if (**s == '\0')
    {
        return (FALSE);
    }

    /*
     * Have nonspace.  Find the end of the tag.
     */

    *tag = *s;
    if (**s == '"')
    {
        (*tag)++;
        (*s)++;
        while (**s != '\0' && (**s != '"'))
	{
            (*s)++;
	}
        if (**s == '\0')
        {
#ifdef OSF_v1_2_4
	    /* CR4721 */
        params[0] = --(*tag);
	num_params = 1;
	XtWarningMsg("conversionWarning", "string", "XtToolkitError",
		     "Unmatched quotation marks in tag \"%s\", any remaining fonts in list unparsed",
		     params, &num_params);
#endif /* OSF_v1_2_4 */
            return (FALSE);
        }
        **s = '\0';
        (*s)++;
	*delim = **s;
    }
    else
    {
        while (!isspace((unsigned char)**s) && (**s != ',') && (**s != '\0'))
	{
	    (*s)++;
	}
	/* Xm1.1 compatibility */
	*delim = isspace ((unsigned char)**s) ? ',' : **s;	
        **s = '\0';
    }

    /* Null tags are not accepted. */

    if (*s == *tag)
    {
#ifndef OSF_v1_2_4
        /* CR4721 */
#else /* OSF_v1_2_4 */
      if (needs_tag) {
       /* CR4721 */
#endif /* OSF_v1_2_4 */
        params[0] = XmRFontList;
	num_params = 1;
	XtWarningMsg("conversionWarning", "string", "XtToolkitError",
		     "Null tag found when converting to type %s, any remaining fonts in list unparsed", params, &num_params);
#ifndef OSF_v1_2_4

        return (FALSE);
#else /* OSF_v1_2_4 */
      }
      return (FALSE);
#endif /* OSF_v1_2_4 */
    }

    return (TRUE);
}

/************************************************************************
 *									*
 * GetNextXmString - return a pointer to a null-terminated string.	*
 *                   The pointer is passed in cs. Up to the caller to   *
 *    		     free that puppy. Returns FALSE if end of string.	*
 *									*
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
GetNextXmString( s, cs )
        char **s ;
        char **cs ;
#else
GetNextXmString(
        char **s,
        char **cs )
#endif /* _NO_PROTO */
{
   char *tmp;
   int csize;

   if (**s == '\0')
      return(FALSE);


   /*  Skip any leading whitespace.  */

   while(isspace((unsigned char)**s) && **s != '\0') (*s)++;

   if (**s == '\0')
      return(FALSE);
  

   /* Found something. Allocate some space (ugh!) and start copying  */
   /* the next string                                                */

   *cs = XtMalloc(strlen(*s) + 1);
   tmp = *cs;

   while((**s) != '\0') 
   {
      if ((**s) == '\\' && *((*s)+1) == ',')	/* Quoted comma */
      {
         (*s)+=2;
         *tmp = ',';
         tmp++;
      }
      else
      {
         if((**s) == ',')			/* End of a string */
         {
            *tmp = '\0';
            (*s)++;
            return(TRUE);
         }
         else
         {
	    if (MB_CUR_MAX > 1) {
	      if ((csize = mblen(*s, MB_CUR_MAX)) < 0)
	        break;
	      strncpy(tmp, *s, csize);
	      tmp += csize;
	      (*s) += csize;
	    } else {
	      *tmp = **s;
	      tmp++; 
	      (*s)++;
	    }
         }
       }
    }

    *tmp = '\0';
    return(TRUE);
}

/************************************************************************
 *
 * _XmCvtStringToXmStringTable
 *
 * Convert a string table to an array of XmStrings.This is in the form :
 *
 *       		String [, String2]*
 *
 * The comma delimeter can be  quoted by a \
 *
 ************************************************************************/
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
_XmCvtStringToXmStringTable( dpy, args, num_args, from_val, to_val, data )
        Display *dpy ;
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
        XtPointer *data ;
#else
_XmCvtStringToXmStringTable(
        Display *dpy,
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val,
        XtPointer *data )
#endif /* _NO_PROTO */
{
   char  *s, *cs;
   XmString *table;
   static  XmString *tblptr;
   int	  i, j;

   if (from_val->addr == NULL)
      return(FALSE);

   s = (char *) from_val->addr;
   j = 100;
   table = (XmString *) XtMalloc(sizeof(XmString) * j);
   for (i = 0; GetNextXmString (&s, &cs); i++)
   {
      if (i >= j)
       {
           j *= 2;
           table = (XmString *)XtRealloc((char *)table, (sizeof(XmString) * j));
       }
      table[i] = XmStringCreateLocalized (cs);
      XtFree(cs);
   }
/****************
 *
 * NULL terminate the array...
 *
 ****************/
   i ++;
   table =  (XmString *)XtRealloc((char *) table, (sizeof(XmString) * i));
   table[i-1] = (XmString ) NULL;
   tblptr = table;

   if (to_val->addr != NULL) 
   {
	if (to_val->size < sizeof(XtPointer)) 
        {
	    to_val->size = sizeof(XtPointer);	
	    return False;
	}		
        *(XmString **)(to_val->addr) = table;
    }
    else 
    {
	to_val->addr = (XPointer)&tblptr;
    }				
    to_val->size = sizeof(XtPointer);
    return(TRUE);
}

/****************
 *
 * _XmXmStringCvtDestroy - free up the space allocated by the converter
 *
 ****************/
static void 
#ifdef _NO_PROTO
_XmXmStringCvtDestroy( app, to, data, args, num_args )
        XtAppContext app ;
        XrmValue *to ;
        XtPointer data ;
        XrmValue *args ;
        Cardinal *num_args ;
#else
_XmXmStringCvtDestroy(
        XtAppContext app,
        XrmValue *to,
        XtPointer data,
        XrmValue *args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
   int i;
   XmString *table = *(XmString **)(to->addr);
   for (i = 0; table[i] != NULL; i++)
       XmStringFree(table[i]);       
   XtFree((char*)table);
}

static Boolean
#ifdef _NO_PROTO
_XmCvtStringToStringTable( dpy, args, num_args, from_val, to_val, data)
        Display *dpy ;
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
        XtPointer *data ;
#else
_XmCvtStringToStringTable(
        Display *dpy,
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val,
        XtPointer *data)
#endif /* _NO_PROTO */
{   
    register char *p ;
            char *top ;
            String *table ;
    static String *tblptr ;
            int size = 50 ;
            int i, len ;
            int csize;

    if(    (p = from_val->addr) == NULL    )
    {   return( False) ;
        } 
    table = (String *) XtMalloc( sizeof( String) * size) ;

    for(    i = 0 ; *p ; i++    )
    {   
        while(    isspace((unsigned char) *p) && *p != '\0'    )
        {   p++ ;
            } 
        if(    *p == '\0'    )
        {   
            if(    i == size    )
            {   
                size++ ;
                table = (String *)XtRealloc( (char *) table,
                                                      sizeof( String) * size) ;
                }
            table[i] = XtMalloc( sizeof( char)) ;
            *(table[i]) = '\0' ;

            break ;
            }
        for(    top = p ; *p != ',' && *p != '\0' ; p+=csize    )
        {   
            if(    *p == '\\' && *(p + 1) == ','    )
            {   p++ ;
                } 
	    if((csize = mblen(p, MB_CUR_MAX)) < 0)
 	      break;
            } 
        if(    i == size    )
        {   
            size *= 2 ;
            table = (String *)XtRealloc( (char *) table,
                                                      sizeof( String) * size) ;
            }
        len = p - top ;
        table[i] = XtMalloc( len + 1) ;
        strncpy( table[i], top, len) ;
	(table[i])[len] = '\0' ;
        if (*p != '\0') p++ ;
        }
    table = (String *)XtRealloc( (char *) table, sizeof( String) * (i + 1)) ;
    table[i] = NULL ;

    if(    to_val->addr != NULL    )
    {   
        if(    to_val->size < sizeof( XPointer)    )
        {   
            to_val->size = sizeof( XPointer) ;
            return( False) ;
            }
        *(String **)(to_val->addr) = table ;
        }
    else
    {   tblptr = table ;
        to_val->addr = (XPointer)&tblptr ;
        }
    to_val->size = sizeof( XPointer) ;
    return( True) ;
    }
 
static void
#ifdef _NO_PROTO
_XmStringCvtDestroy( app, to, data, args, num_args)
        XtAppContext app;
        XrmValue *to;
        XtPointer data;
        XrmValue *args;
        Cardinal *num_args;
#else
_XmStringCvtDestroy(
        XtAppContext app,
        XrmValue *to,
        XtPointer data,
        XrmValue *args,
        Cardinal *num_args)
#endif /* _NO_PROTO */
{   
            int i ;
            String *table = * (String **) (to->addr) ;

    for(    i = 0 ; table[i] != NULL ; i++    )
    {   XtFree( (char *) table[i]) ;
        } 
    XtFree( (char *) table) ;

    return ;
    }
 
static Boolean
#ifdef _NO_PROTO
_XmCvtStringToHorizontalPosition( display, args, num_args, from, to,
                                                               converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToHorizontalPosition(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
        unsigned char unitType = *((unsigned char *) args[0].addr) ;
        Screen * screen = *((Screen **) args[1].addr) ;
        int intermediate;
        Position tmpPix;

    if (!isInteger(from->addr,&intermediate))
        {
        XtStringConversionWarning((char *)from->addr, XmRHorizontalPosition);
        return False;
        }

	tmpPix = (Position) _XmConvertUnits( screen, XmHORIZONTAL,
                                      (int) unitType, intermediate, XmPIXELS) ;
    done( to, Position, tmpPix, ; )
    }

static Boolean
#ifdef _NO_PROTO
_XmCvtStringToHorizontalDimension( display, args, num_args, from, to,
                                                               converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToHorizontalDimension(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
        unsigned char unitType = *((unsigned char *) args[0].addr) ;
        Screen * screen = *((Screen **) args[1].addr) ;
        int intermediate;
        Dimension tmpPix ;

    if (!isInteger(from->addr,&intermediate) || intermediate < 0)
        {
        XtStringConversionWarning((char *)from->addr, XmRHorizontalDimension);
        return False;
        }

    tmpPix = (Dimension) _XmConvertUnits( screen, XmHORIZONTAL,
                                      (int) unitType, intermediate, XmPIXELS) ;
    done( to, Dimension, tmpPix, ; )
    }

static Boolean
#ifdef _NO_PROTO
_XmCvtStringToVerticalPosition( display, args, num_args, from, to,
                                                               converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToVerticalPosition(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
        unsigned char unitType = *((unsigned char *) args[0].addr) ;
        Screen * screen = *((Screen **) args[1].addr) ;
        int intermediate;
        Position tmpPix;

    if (!isInteger(from->addr,&intermediate))
        {
        XtStringConversionWarning((char *)from->addr, XmRVerticalPosition);
        return False;
	}

        tmpPix = (Position) _XmConvertUnits( screen, XmVERTICAL,
                                      (int) unitType, intermediate, XmPIXELS) ;
    done( to, Position, tmpPix, ; )
    }

static Boolean
#ifdef _NO_PROTO
_XmCvtStringToVerticalDimension( display, args, num_args, from, to,
                                                               converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToVerticalDimension(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
        unsigned char unitType = *((unsigned char *) args[0].addr) ;
        Screen * screen = *((Screen **) args[1].addr) ;
        int intermediate;
        Dimension tmpPix ;

    if (!isInteger(from->addr,&intermediate) || intermediate < 0)
        {
        XtStringConversionWarning((char *)from->addr, XmRVerticalDimension);
        return False;
        }
    tmpPix = (Dimension) _XmConvertUnits( screen, XmVERTICAL,
                                      (int) unitType, intermediate, XmPIXELS) ;
    done( to, Dimension, tmpPix, ; )
    }


/************************************************************************
 *
 *  _XmGetDefaultFontList
 *       This function is called by a widget to initialize it's fontlist
 *   resource with a default, when it is NULL. This is done by checking to
 *   see if any of the widgets, in the widget's parent hierarchy is a
 *   subclass of BulletinBoard or the VendorShell widget class, and if it
 *   is, returning the BulletinBoard or VendorShell fontlist. 
 *
 *************************************************************************/
XmFontList 
#ifdef _NO_PROTO
_XmGetDefaultFontList( w, fontListType )
        Widget w ;
        unsigned char fontListType ;
#else
_XmGetDefaultFontList(
        Widget w,
#if NeedWidePrototypes
        unsigned int fontListType )
#else
        unsigned char fontListType )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Arg al[1];
    XmFontList data = NULL;
    Widget origw = w;
    XmFontListEntry fontListEntry;
    char *s;
    char *newString;
    char *sPtr;
    char *fontName;
    char *fontTag;
    XmFontType fontType;
    char delim;

    if(    fontListType    )
    while ((w = XtParent(w)) != NULL)
    {
        if (XmIsBulletinBoard(w) || XmIsVendorShell (w) || XmIsMenuShell (w) )
        {
            if (fontListType == XmLABEL_FONTLIST)
                XtSetArg(al[0], XmNlabelFontList, & data);
	    else if (fontListType == XmTEXT_FONTLIST)
	        XtSetArg(al[0], XmNtextFontList, & data);
	    else if (fontListType == XmBUTTON_FONTLIST)
	        XtSetArg(al[0], XmNbuttonFontList, & data);

	    XtGetValues (w, al, 1);
            break ;  /* If NULL font is returned, will use XmDEFAULT_FONT.*/
        }
    }

#ifdef USE_FONT_OBJECT
    data = (XmFontList)_XmFontObjectGetDefaultFontList(origw, data);
#endif /* USE_FONT_OBJECT */
    if (!data)
    {
          /* Begin fixing OSF 4735 */
            s = (char *) XmDEFAULT_FONT;
            newString = XtNewString (s);
            sPtr = newString;

            if (!GetNextFontListEntry (&sPtr, &fontName, &fontTag,
                                   &fontType, &delim))
             {
              XtFree (newString);
              _XmWarning(NULL, MSG2);
                exit( 1) ;
             }

             do
               {
               if (*fontName) {
                        fontListEntry = XmFontListEntryLoad (XtDisplay(origw), fontName,
                                                         fontType, fontTag);
               if (fontListEntry != NULL)
                    {
                        data =
                            XmFontListAppendEntry (data, fontListEntry);
                        XmFontListEntryFree (&fontListEntry);
                    }
               else
                        XtStringConversionWarning(fontName, XmRFontList);

                    }
               }
               while ((delim == ',') && *++sPtr && !data &&
                          GetNextFontListEntry (&sPtr, &fontName, &fontTag,
                                         &fontType, &delim));
               XtFree (newString);

         /* End fixing OSF 4735 */
    }
    return (data);
} 

static void
#ifdef _NO_PROTO
_XmConvertStringToButtonTypeDestroy( app, to, converter_data, args, num_args )
        XtAppContext app ;
        XrmValue *to ;
        XtPointer converter_data ;
        XrmValue *args ;
        Cardinal *num_args ;
#else
_XmConvertStringToButtonTypeDestroy( 
        XtAppContext app,
        XrmValue *to,
        XtPointer converter_data,
        XrmValue *args,
        Cardinal *num_args)
#endif /* _NO_PROTO */
{   
    XtFree( *((char **) to->addr)) ;

    return ;
    } 

static Boolean
#ifdef _NO_PROTO
_XmConvertStringToButtonType( display, args, num_args, from, to,
                                                               converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmConvertStringToButtonType(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    String in_str = (String) from->addr ;
    unsigned int in_str_size = 0 ;
    XmButtonTypeTable buttonTable ;
    int i, comma_count ;
    String work_str, btype_str ;
    
    comma_count = 0 ;
    while(    in_str[in_str_size]    )
    {   if(    in_str[in_str_size++] == ','    )
        {   ++comma_count ;
            } 
        } 
    ++in_str_size ;

    buttonTable = (XmButtonTypeTable) XtMalloc( 
                                   sizeof( XmButtonType) * (comma_count + 2)) ;
    buttonTable[comma_count+1] = (XmButtonType)NULL;
    work_str = (String) XtMalloc( in_str_size) ;
    strcpy( work_str, in_str) ;

    for(    i = 0, btype_str = (char *) strtok( work_str, ",") ;
            btype_str ;
            btype_str = (char *) strtok( NULL, ","), ++i)
    {
        while (*btype_str && isspace((unsigned char)*btype_str)) btype_str++;
        if (*btype_str == '\0')
            break;
        if (_XmStringsAreEqual(btype_str, "pushbutton"))
            buttonTable[i] = XmPUSHBUTTON;
        else if (_XmStringsAreEqual(btype_str, "togglebutton"))
            buttonTable[i] = XmTOGGLEBUTTON;
        else if (_XmStringsAreEqual(btype_str, "cascadebutton"))
            buttonTable[i] = XmCASCADEBUTTON;
        else if (_XmStringsAreEqual(btype_str, "separator"))
            buttonTable[i] = XmSEPARATOR;
        else if (_XmStringsAreEqual(btype_str, "double_separator"))
            buttonTable[i] = XmDOUBLE_SEPARATOR;
        else if (_XmStringsAreEqual(btype_str, "title"))
            buttonTable[i] = XmTITLE;
        else
        {
            XtStringConversionWarning( (char *) btype_str, XmRButtonType) ;
            XtFree( (char *) buttonTable) ;
            XtFree( (char *) work_str) ;

            return( FALSE) ;
            }
        }
    XtFree( work_str) ;

    done( to, XmButtonTypeTable, buttonTable, XtFree( (char *) buttonTable) ; )
    }

static void
#ifdef _NO_PROTO
_XmCvtStringToKeySymTableDestroy( app, to, converter_data, args, num_args )
        XtAppContext app ;
        XrmValue *to ;
        XtPointer converter_data ;
        XrmValue *args ;
        Cardinal *num_args ;
#else
_XmCvtStringToKeySymTableDestroy( 
        XtAppContext app,
        XrmValue *to,
        XtPointer converter_data,
        XrmValue *args,
        Cardinal *num_args)
#endif /* _NO_PROTO */
{   
    XtFree( *((char **) to->addr)) ;

    return ;
    } 

static Boolean
#ifdef _NO_PROTO
_XmCvtStringToKeySymTable( display, args, num_args, from, to, converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToKeySymTable(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    String in_str = (String) from->addr ;
    unsigned int in_str_size = 0 ;
    XmKeySymTable keySymTable ;
    int i, comma_count ;
    String work_str, ks_str ;
    KeySym ks ;

    comma_count = 0 ;
    while(    in_str[in_str_size]    )
    {   if(    in_str[in_str_size++] == ','    )
        {   ++comma_count ;
            } 
        } 
    ++in_str_size ;

    keySymTable = (XmKeySymTable) XtMalloc(
                                         sizeof( KeySym) * (comma_count + 2)) ;
    keySymTable[comma_count+1] = (KeySym)NULL;
    work_str = XtNewString(in_str);

    for(    ks_str = (char *) strtok( work_str, ","), i = 0 ;
            ks_str ;
            ks_str = (char *) strtok( NULL, ","), i++)
    {
        if(    !*ks_str    )
        {   keySymTable[i] = NoSymbol ;
            } 
        else
        {   if(    (ks = XStringToKeysym( ks_str)) == NoSymbol)
            {   
                XtStringConversionWarning( ks_str, XmRKeySym);
                XtFree( (char *) work_str) ;
                XtFree( (char *) keySymTable) ;

                return( FALSE) ;
                } 
            keySymTable[i] = ks ;
            }
        }
    XtFree( (char *) work_str) ;

    done( to, XmKeySymTable, keySymTable, XtFree( (char *) keySymTable) ; )
    }

static void
#ifdef _NO_PROTO
_XmCvtStringToCharSetTableDestroy( app, to, converter_data, args, num_args )
        XtAppContext app ;
        XrmValue *to ;
        XtPointer converter_data ;
        XrmValue *args ;
        Cardinal *num_args ;
#else
_XmCvtStringToCharSetTableDestroy( 
        XtAppContext app,
        XrmValue *to,
        XtPointer converter_data,
        XrmValue *args,
        Cardinal *num_args)
#endif /* _NO_PROTO */
{   
    XtFree( *((char **) to->addr)) ;

    return ;
    } 

static Boolean
#ifdef _NO_PROTO
_XmCvtStringToCharSetTable( display, args, num_args, from, to, converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToCharSetTable(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    String in_str = (String) from->addr ;
    XmStringCharSetTable charsetTable ;
    unsigned int numCharsets = 0 ;
    unsigned int strDataSize = 0 ;
    char * dataPtr ;
    int i ;
    String work_str, cs_str ;

    work_str = XtNewString( in_str) ;

    for(    cs_str = (char *) strtok( work_str, ",") ;
            cs_str ;
            cs_str = (char *) strtok( NULL, ","))
    {   if(    *cs_str    )
        {
            strDataSize += strlen( cs_str) + 1 ;
            } 
        ++numCharsets ;
        }
    charsetTable = (XmStringCharSetTable) XtMalloc( strDataSize +
                                      sizeof( XmStringCharSet) * 
					(numCharsets+1)) ;
    charsetTable[numCharsets] = (XmStringCharSet)NULL;
    dataPtr = (char *) &charsetTable[numCharsets+1] ;
    strcpy( work_str, in_str) ;

    for(    i = 0, cs_str = (char *) strtok( work_str, ",") ;
            cs_str ;
            cs_str = (char *) strtok( NULL, ","), ++i)
    {   
        if(    *cs_str    )
        {
            charsetTable[i] = dataPtr ;
            strcpy( dataPtr, cs_str) ;
            dataPtr += strlen( cs_str) + 1 ;
            }
        else
        {   charsetTable[i] = NULL ;
            } 
        }
    XtFree( (char *) work_str) ;

    done( to, XmStringCharSetTable, charsetTable,
                                             XtFree( (char *) charsetTable) ; )
    }

/************************************************************************
 *
 *  _XmCvtStringToBooleanDimension
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
_XmCvtStringToBooleanDimension( display, args, num_args, from, to,
                                                               converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToBooleanDimension(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
        char *in_str = (char *) from->addr ;
        Dimension outVal ;
        int intermediate;

    if (isInteger(from->addr, &intermediate))
    {   
        /* Is numeric argument, so convert to horizontal dimension.  This is
        *   to preserve 1.0 compatibility (the resource actually behaves like
        *   a boolean in version 1.1).
        */
        unsigned char unitType = *((unsigned char *) args[0].addr) ;
        Screen * screen = *((Screen **) args[1].addr) ;

        if(    intermediate < 0    )
        {   XtStringConversionWarning( (char *)from->addr,
                                                         XmRBooleanDimension) ;
            return( FALSE) ;
            } 
        outVal = (Dimension) _XmConvertUnits( screen, XmHORIZONTAL,
                                      (int) unitType, intermediate, XmPIXELS) ;
        } 
    else
    {   /* Presume Boolean (version 1.1).
        */
        if(    _XmStringsAreEqual( in_str, "true")    )
        {   outVal = (Dimension) 1 ;
            } 
        else
        {   if(    _XmStringsAreEqual( in_str, "false")    )
            {   outVal = (Dimension) 0 ;
                } 
            else
            {   XtStringConversionWarning( in_str, XmRBooleanDimension) ;
                return( FALSE) ;
                } 
            } 
        } 
    done( to, Dimension, outVal, ; )
    }



/* Define handy macros (note: these constants are in OCTAL) */
#define EOS	00
#define STX	02
#define HT	011
#define NL	012
#define ESC	033
#define CSI	0233

#define NEWLINESTRING			"\012"
#define NEWLINESTRING_LEN		sizeof(NEWLINESTRING)-1

#define CTEXT_L_TO_R			"\233\061\135"
#define CTEXT_L_TO_R_LEN		sizeof(CTEXT_L_TO_R)-1

#define CTEXT_R_TO_L			"\233\062\135"
#define CTEXT_R_TO_L_LEN		sizeof(CTEXT_R_TO_L)-1

#define CTEXT_SET_ISO8859_1		"\033\050\102\033\055\101"
#define CTEXT_SET_ISO8859_1_LEN		sizeof(CTEXT_SET_ISO8859_1)-1

#define CTEXT_SET_ISO8859_2		"\033\050\102\033\055\102"
#define CTEXT_SET_ISO8859_2_LEN		sizeof(CTEXT_SET_ISO8859_2)-1

#define CTEXT_SET_ISO8859_3		"\033\050\102\033\055\103"
#define CTEXT_SET_ISO8859_3_LEN		sizeof(CTEXT_SET_ISO8859_3)-1

#define CTEXT_SET_ISO8859_4		"\033\050\102\033\055\104"
#define CTEXT_SET_ISO8859_4_LEN		sizeof(CTEXT_SET_ISO8859_4)-1

#define CTEXT_SET_ISO8859_5		"\033\050\102\033\055\114"
#define CTEXT_SET_ISO8859_5_LEN		sizeof(CTEXT_SET_ISO8859_5)-1

#define CTEXT_SET_ISO8859_6		"\033\050\102\033\055\107"
#define CTEXT_SET_ISO8859_6_LEN		sizeof(CTEXT_SET_ISO8859_6)-1

#define CTEXT_SET_ISO8859_7		"\033\050\102\033\055\106"
#define CTEXT_SET_ISO8859_7_LEN		sizeof(CTEXT_SET_ISO8859_7)-1

#define CTEXT_SET_ISO8859_8		"\033\050\102\033\055\110"
#define CTEXT_SET_ISO8859_8_LEN		sizeof(CTEXT_SET_ISO8859_8)-1

#define CTEXT_SET_ISO8859_9		"\033\050\102\033\055\115"
#define CTEXT_SET_ISO8859_9_LEN		sizeof(CTEXT_SET_ISO8859_9)-1

#define CTEXT_SET_JISX0201		"\033\050\112\033\051\111"
#define CTEXT_SET_JISX0201_LEN		sizeof(CTEXT_SET_JISX0201)-1

#define CTEXT_SET_GB2312_0		"\033\044\050\101\033\044\051\101"
#define CTEXT_SET_GB2312_0_LEN		sizeof(CTEXT_SET_GB2312_0)-1

#define CTEXT_SET_JISX0208_0		"\033\044\050\102\033\044\051\102"
#define CTEXT_SET_JISX0208_0_LEN	sizeof(CTEXT_SET_JISX0208_0)-1

#define CTEXT_SET_KSC5601_0		"\033\044\050\103\033\044\051\103"
#define CTEXT_SET_KSC5601_0_LEN		sizeof(CTEXT_SET_KSC5601_0)-1


#define CTVERSION 1
#define _IsValidC0(ctx, c)	(((c) == HT) || ((c) == NL) || ((ctx)->version > CTVERSION)) 
#define _IsValidC1(ctx, c)	((ctx)->version > CTVERSION)
 
#define _IsValidESCFinal(c)	(((c) >= 0x30) && ((c) <= 0x7e))
#define _IsValidCSIFinal(c)	(((c) >= 0x40) && ((c) <= 0x7e))

#define _IsInC0Set(c)		((c) <= 0x1f)
#define _IsInC1Set(c)		(((c) >= 0x80) && ((c) <= 0x9f))
#define _IsInGLSet(c)		(((c) >= 0x20) && ((c) <= 0x7f))
#define _IsInGRSet(c)		((c) >= 0xa0)
#define _IsInColumn2(c)		(((c) >= 0x20) && ((c) <= 0x2f))
#define _IsInColumn3(c)		(((c) >= 0x30) && ((c) <= 0x3f))
#define _IsInColumn4(c)		(((c) >= 0x40) && ((c) <= 0x4f))
#define _IsInColumn5(c)		(((c) >= 0x50) && ((c) <= 0x5f))
#define _IsInColumn6(c)		(((c) >= 0x60) && ((c) <= 0x6f))
#define _IsInColumn7(c)		(((c) >= 0x70) && ((c) <= 0x7f))
#define _IsInColumn4or5(c)	(((c) >= 0x40) && ((c) <= 0x5f))


#define _SetGL(ctx, charset, size, octets)\
    (ctx)->flags.gl = True;\
    (ctx)->gl_charset = (charset);\
    (ctx)->gl_charset_size = (size);\
    (ctx)->gl_octets_per_char = (octets)

#define _SetGR(ctx, charset, size, octets)\
    (ctx)->flags.gl = False;\
    (ctx)->gr_charset = (charset);\
    (ctx)->gr_charset_size = (size);\
    (ctx)->gr_octets_per_char = (octets)

#define _PushDir(ctx, dir)\
    if ( (ctx)->dirsp == ((ctx)->dirstacksize - 1) ) {\
	(ctx)->dirstacksize += 8;\
	(ctx)->dirstack = \
	    (ct_Direction *)XtRealloc((char *)(ctx)->dirstack,\
				(ctx)->dirstacksize * sizeof(ct_Direction));\
    }\
    (ctx)->dirstack[++((ctx)->dirsp)] = dir;\
    (ctx)->flags.dircs = True

#define _PopDir(ctx)	((ctx)->dirsp)--

#define _CurDir(ctx)	(ctx)->dirstack[(ctx)->dirsp]

/************************************************************************
 *
 *  _find_encoding
 *    Find the SegmentEncoding with fontlist_tag.  Return NULL if no
 *    such SegmentEncoding exists.  As a side effect, free any encodings
 *    encountered that have been unregistered.
 *
 ************************************************************************/
static SegmentEncoding *
#ifdef _NO_PROTO
_find_encoding(fontlist_tag)
     char *fontlist_tag;
#else
_find_encoding(char *fontlist_tag)
#endif /* _NO_PROTO */ 
{
  SegmentEncoding     *prevPtr, *encodingPtr = _encoding_registry_ptr;
  String              encoding = NULL;

  if (encodingPtr)
    {
      if (strcmp(fontlist_tag, EncodingRegistryTag(encodingPtr)) == 0)
      {
        encoding = EncodingRegistryEncoding(encodingPtr);
        
        /* Free unregistered encodings. */
        if (encoding == NULL)
          {
            _encoding_registry_ptr = EncodingRegistryNext(encodingPtr);
            XtFree( (char *) encodingPtr);
            encodingPtr = NULL;
          }
        
        return(encodingPtr);
      }
    }
  else return(encodingPtr);
  
  for (prevPtr = encodingPtr, encodingPtr = EncodingRegistryNext(encodingPtr);
       encodingPtr != NULL;
       prevPtr = encodingPtr, encodingPtr = EncodingRegistryNext(encodingPtr))
    {
      if (strcmp(fontlist_tag, EncodingRegistryTag(encodingPtr)) == 0)
      {
        encoding = EncodingRegistryEncoding(encodingPtr);
        
        /* Free unregistered encodings. */
        if (encoding == NULL)
          {
            EncodingRegistryNext(prevPtr) = 
              EncodingRegistryNext(encodingPtr);
            XtFree( (char *) encodingPtr);
            encodingPtr = NULL;
          }
        
        return(encodingPtr);
      }
      /* Free unregistered encodings. */
      else if (EncodingRegistryEncoding(encodingPtr) == NULL)
      {
        EncodingRegistryNext(prevPtr) = EncodingRegistryNext(encodingPtr);
        XtFree( (char *) encodingPtr);
      }
    }
  
  return(NULL);
}


/************************************************************************
 *
 *  XmRegisterSegmentEncoding
 *    Register a compound text encoding format for a specified font list
 *    element tag.  Returns NULL for a new tag or a copy of the old encoding
 *    for an already registered tag.
 *
 ************************************************************************/
char *
#ifdef _NO_PROTO
XmRegisterSegmentEncoding(fontlist_tag, ct_encoding)
     char     *fontlist_tag;
     char     *ct_encoding;
#else     
XmRegisterSegmentEncoding(
     char     *fontlist_tag,
     char     *ct_encoding)
#endif /* _NO_PROTO */
{
  SegmentEncoding     *encodingPtr = NULL;
  String              ret_val = NULL;
  
  encodingPtr = _find_encoding(fontlist_tag);

  if (encodingPtr)
    {
      ret_val = XtNewString(EncodingRegistryEncoding(encodingPtr));
      EncodingRegistryEncoding(encodingPtr) = 
      ct_encoding ? XtNewString(ct_encoding) : (String)NULL;
    }
  else if (ct_encoding != NULL)
    {
      encodingPtr = 
      (SegmentEncoding *)XtMalloc((Cardinal)sizeof(SegmentEncoding));
      EncodingRegistryTag(encodingPtr) = XtNewString(fontlist_tag);
      EncodingRegistryEncoding(encodingPtr) = XtNewString(ct_encoding);
      
      EncodingRegistryNext(encodingPtr) = _encoding_registry_ptr;
      _encoding_registry_ptr = encodingPtr;
    }
  
  return(ret_val);
}

/************************************************************************
 *
 *  XmMapSegmentEncoding
 *    Returns the compound text encoding format associated with the
 *    specified font list element tag.  Returns NULL if not found.
 *
 ************************************************************************/
char *
#ifdef _NO_PROTO
XmMapSegmentEncoding(fontlist_tag)
     char     *fontlist_tag;
#else     
XmMapSegmentEncoding(char        *fontlist_tag)
#endif /* _NO_PROTO */
{
  SegmentEncoding     *encodingPtr = NULL;
  String              ret_val = NULL;

  encodingPtr = _find_encoding(fontlist_tag);

  if (encodingPtr) 
    ret_val = XtNewString(EncodingRegistryEncoding(encodingPtr));
  
  return(ret_val);
}


/************************************************************************
 *
 *  XmCvtCTToXmString
 *	Convert a compound text string to a XmString.  This is the public
 *	version which takes only a compound text string as an argument.
 *	Note: processESC and processExtendedSegments have to be hacked
 *	for this to work.
 *
 ************************************************************************/
XmString 
#ifdef _NO_PROTO
XmCvtCTToXmString( text )
        char *text ;
#else
XmCvtCTToXmString(
        char *text )
#endif /* _NO_PROTO */
{
    ct_context	    *ctx;		/* compound text context block */
    Boolean	    ok = True;
    Octet	    c;
    XmString	    xmsReturned;	/* returned Xm string */

    ctx = (ct_context *) XtMalloc(sizeof(ct_context));

/* initialize the context block */
    ctx->octet = (OctetPtr)text;
    ctx->flags.dircs = False;
    ctx->flags.gchar = False;
    ctx->flags.ignext = False;
    ctx->flags.gl = False;
    ctx->flags.text = False;
    ctx->dirstacksize = 8;
    ctx->dirstack = (ct_Direction *)
            XtMalloc(ctx->dirstacksize*sizeof(ct_Direction));
    ctx->dirstack[0] = ct_Dir_StackEmpty;
    ctx->dirstack[1] = ct_Dir_LeftToRight;
    ctx->dirsp = 1;
    ctx->item = NULL;
    ctx->itemlen = 0;
    ctx->version = CTVERSION;
    ctx->gl_charset = CS_ISO8859_1;
    ctx->gl_charset_size = 94;
    ctx->gl_octets_per_char = 1;
    ctx->gr_charset = CS_ISO8859_1;
    ctx->gr_charset_size = 96;
    ctx->gr_octets_per_char = 1;
    ctx->xmstring = NULL;
    ctx->xmsep = NULL;

/*
** check for version/ignore extensions sequence (must be first if present)
**  Format is:	ESC 02/03 V 03/00   ignoring extensions is OK
**		ESC 02/03 V 03/01   ignoring extensions is not OK
**  where V is in the range 02/00 thru 02/15 and represents versions 1 thru 16
*/
    if (    (ctx->octet[0] == ESC)
	&&  (ctx->octet[1] == 0x23)
	&&  (_IsInColumn2(ctx->octet[2])
	&&  ((ctx->octet[3] == 0x30) || ctx->octet[3] == 0x31))
       ) {
	ctx->version = ctx->octet[2] - 0x1f;	/* 0x20-0x2f => version 1-16 */
	if (ctx->octet[3] == 0x30)		/* 0x30 == can ignore extensions */
	    ctx->flags.ignext = True;
	ctx->octet += 4;			/* advance ptr to next seq */
    }


    while (ctx->octet[0] != 0) {
    switch (*ctx->octet) {			/* look at next octet in seq */
	case ESC:
	    /* %%% TEMP
	    ** if we have any text to output, do it
	    ** this section needs to be optimized so that it handles
	    ** paired character sets without outputting a new segment.
	    */
	    if (ctx->flags.text) {
		outputXmString(ctx, False);	/* with no separator */
	    }
	    ctx->flags.text = False;
	    ctx->item = ctx->octet;		/* remember start of this item */
	    ctx->itemlen = 0;

	    ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */

	    /* scan for final char */
	    while (	(ctx->octet[0] != 0)
		     && (_IsInColumn2(*ctx->octet)) ) {
		ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    }

	    if (ctx->octet[0] == 0) {	/* if nothing after this, it's an error */
		ok = False;
		break;
	    }

	    c = *ctx->octet;			/* get next char in seq */
	    ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    if (_IsValidESCFinal(c)) {
		/* we have a valid ESC sequence - handle it */
		ok = processESCHack(ctx, c);
	    } else {
		ok = False;
	    }
	    break;

	case CSI:
            /*
            ** CSI format is: CSI 03/01 05/13 - begin left to right text
            **                CSI 03/02 05/13 - begin right to left text
            **                CSI 05/13       - end of string
            **
            **              OR for Extensions:
            **
            */
	    /*
	    ** CSI format is:	CSI P I F   where
	    **	    03/00 <= P <= 03/15
	    **	    02/00 <= I <= 02/15
	    **	    04/00 <= F <= 07/14
	    */
	    /* %%% TEMP
	    ** if we have any text to output, do it
	    ** This may need optimization.
	    */
	    if (ctx->flags.text) {
		/* check whether we have a specific direction set */
                if (((ctx->octet[1] == 0x31) && (ctx->octet[2] == 0x5d))||
                    ((ctx->octet[1] == 0x32) && (ctx->octet[2] == 0x5d))||
                    (ctx->octet[1] == 0x5d))
                        outputXmString(ctx, False);    /* without a separator*/
                else
			outputXmString(ctx, True);	/* with a separator */
	    }
	    ctx->flags.text = False;
	    ctx->item = ctx->octet;		/* remember start of this item */
	    ctx->itemlen = 0;

	    ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */

	    /* scan for final char */
	    while (	(ctx->octet[0] != 0)
		    &&	_IsInColumn3(*ctx->octet)  ) {
		ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    }
	    while (	(ctx->octet[0] != 0)
		    && _IsInColumn2(*ctx->octet)   ) {
		ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    }

	    /* if nothing after this, it's an error */
	    if (ctx->octet[0] == 0) {
		ok = False;
		break;
	    }

	    c = *ctx->octet;			/* get next char in seq */
	    ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    if (_IsValidCSIFinal(c)) {
		/* we have a valid CSI sequence - handle it */
		ok = processCSI(ctx, c);
	    } else {
		ok = False;
	    }
	    break;

	case NL:			    /* new line */
	    /* if we have any text to output, do it */
	    if (ctx->flags.text) {
		outputXmString(ctx, True);	/* with a separator */
		ctx->flags.text = False;
	    } else {
		XmString    save;
		if (ctx->xmsep == NULL) {
		    ctx->xmsep = XmStringSeparatorCreate();
		}
		save = ctx->xmstring;
		ctx->xmstring = XmStringConcat(ctx->xmstring,
						ctx->xmsep);
		XmStringFree(save);		/* free original xmstring */
	    }
	    ctx->octet++;			/* advance ptr to next char */
	    break;

	case HT:
	    /* Tab has no meaning in an XmString, so just ignore it. */
	    ctx->octet++;			/* advance ptr to next char */
	    break;

	default:			    /* just 'normal' text */
	    ctx->item = ctx->octet;		/* remember start of this item */
	    ctx->itemlen = 0;
	    ctx->flags.text = True;
#ifdef OSF_v1_2_4
	    /* RtoL nesting bug fix */
            ctx->flags.dircs = False;       /* clear indicator if text found */

#endif /* OSF_v1_2_4 */
	    while (ctx->octet[0] != 0) {
		c = *ctx->octet;
		if ((c == ESC) || (c == CSI) || (c == NL) || (c == HT)) {
		    break;
		}
		if (	(_IsInC0Set(c) && (!_IsValidC0(ctx, c)))
		    ||	(_IsInC1Set(c) && (!_IsValidC1(ctx, c))) ) {
		    ok = False;
		    break;
		}
		ctx->flags.gchar = True;	/* We have a character! */

                /*
                 *  We should look at the actual character to
                 *  decide whether it's a gl or gr character.
                 *
                 *  We'll hit the problem if we get a CT that
                 *  isn't generated by Motif.
                 */
                if (isascii((unsigned char)c)) {
		    ctx->octet += ctx->gl_octets_per_char;
		    ctx->itemlen += ctx->gl_octets_per_char;
		} else {
		    ctx->octet += ctx->gr_octets_per_char;
		    ctx->itemlen += ctx->gr_octets_per_char;
		}
	    } /* end while */
	    break;
	} /* end switch */
    if (!ok) break;
    } /* end while */

/* if we have any text left to output, do it */
    if (ctx->flags.text) {
	outputXmString(ctx, False);		/* with no separator */
    }

    XtFree((char *) ctx->dirstack);
    if (ctx->xmsep != NULL) XmStringFree(ctx->xmsep);
    xmsReturned = (XmString)ctx->xmstring;
#ifdef OSF_v1_2_4

    if (ctx)
    {
       ctx->dirstack = NULL;
       ctx->xmsep = NULL;
    }

#endif /* OSF_v1_2_4 */
    XtFree((char *) ctx);

    if (ok)
      return ( xmsReturned );
    else 
	return ( (XmString)NULL );
    
}


/***********************************************************************
 *
 * Hacked procedures to work with XmCvtCTToXmString.
 *
 ***********************************************************************/

/* processESCHack - handle valid ESC sequences */
static Boolean 
#ifdef _NO_PROTO
processESCHack( ctx, final )
        ct_context *ctx ;
        Octet final ;
#else
processESCHack(
        ct_context *ctx,
#if NeedWidePrototypes
        int final )
#else
        Octet final )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Boolean	    ok;

    switch (ctx->item[1]) {
    case 0x24:			/* 02/04 - invoke 94(n) charset into GL or GR */
	ok = process94n(ctx, final);
	break;
    case 0x25:			/* 02/05 - extended segments */
	/* if we have any text to output, do it */
	if (ctx->flags.text) {
	    outputXmString(ctx, False);	/* with no separator */
	    ctx->flags.text = False;
	}
	ok = processExtendedSegmentsHack(ctx, final);
	break;
    case 0x28:			/* 02/08 - invoke 94 charset into GL */
	ok = process94GL(ctx, final);
	break;
    case 0x29:			/* 02/09 - invoke 94 charset into GR */
	ok = process94GR(ctx, final);
	break;
    case 0x2d:			/* 02/13 - invoke 96 charset into GR */
	ok =  process96GR(ctx, final);
	break;
    default:
	ok = False;
	break;
    }
    return(ok);
}


static Boolean 
#ifdef _NO_PROTO
processExtendedSegmentsHack( ctx, final )
        ct_context *ctx ;
        Octet final ;
#else
processExtendedSegmentsHack(
        ct_context *ctx,
#if NeedWidePrototypes
        int final )
#else
        Octet final )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    OctetPtr	    esptr;			/* ptr into ext seg */
    unsigned int    seglen;			/* length of ext seg */
    unsigned int    len;			/* length */
    String	    charset_copy;		/* ptr to NULL-terminated copy of ext seg charset */
    OctetPtr	    text_copy;			/* ptr to NULL-terminated copy of ext seg text */
    XmString	    tempxm1, tempxm2;
    Boolean	    ok = True;

    /* Extended segments
    **  01/11 02/05 02/15 03/00 M L	    variable # of octets/char
    **  01/11 02/05 02/15 03/01 M L	    1 octet/char
    **  01/11 02/05 02/15 03/02 M L	    2 octets/char
    **  01/11 02/05 02/15 03/03 M L	    3 octets/char
    **  01/11 02/05 02/15 03/04 M L	    4 octets/char
    */
    if (	(ctx->itemlen == 4)
	&&	(ctx->item[2] == 0x2f)
	&&	(_IsInColumn3(final))
       ) 
      {
	if (    (ctx->octet[0] < 0x80)
	    ||  (ctx->octet[1] < 0x80)
	   )	
	  {
	    return(False);
	  }

	/*
	** The most significant bit of M and L are always set to 1
	** The number is computed as ((M-128)*128)+(L-128)
	*/
	seglen = *ctx->octet - 0x80;
	ctx->octet++; ctx->itemlen++;		/* advance pointer */
	seglen = (seglen << 7) + (*ctx->octet - 0x80);
	ctx->octet++; ctx->itemlen++;		/* advance pointer */
	
	/* Check for premature end. */
	for (esptr = ctx->octet; esptr < (ctx->octet + seglen); esptr++) 
	  {
	    if (*esptr == 0) 
	      {
		return(False);
	      }
	  }	

        esptr = ctx->octet;			/* point to charset */
	ctx->itemlen += seglen;			/* advance pointer over segment */
	ctx->octet += seglen;

	switch (final) {
	case 0x30:				/* variable # of octets per char */
	case 0x31:				/* 1 octet per char */
	case 0x32:				/* 2 octets per char */
	    /* scan for STX separator between charset and text */
	    len = 0;
	    while (esptr[len] != STX)
		len++;
	    if (len > ctx->itemlen) {		/* if we ran off the end, error */
		ok = False;
		break;
	    }
	    charset_copy = XtMalloc(len + 1);
	    strncpy(charset_copy, (char *) esptr, len);
	    charset_copy[len] = EOS;
	    esptr += len + 1;			/* point to text part */
	    len = seglen - len - 1;		/* calc length of text part */

	    /* For two-octets charsets, make sure the text
	     * contains an integral number of characters. */
            if (final == 0x32 && len % 2) {
	      XtFree(charset_copy);
	      return (False);
            }
	    
	    text_copy = (unsigned char *) XtMalloc(len + 1);
	    memcpy( text_copy, esptr, len);
	    text_copy[len] = EOS;
	    tempxm1 = XmStringSegmentCreate (	(char *) text_copy,
						charset_copy,
						(unsigned char ) (_CurDir(ctx) == ct_Dir_LeftToRight ?
							XmSTRING_DIRECTION_L_TO_R :
							XmSTRING_DIRECTION_R_TO_L ),
						False );
	    tempxm2 = ctx->xmstring;
	    ctx->xmstring = XmStringConcat(ctx->xmstring, tempxm1);
	    XtFree((char *) text_copy);
	    XtFree((char *) charset_copy);
	    XmStringFree(tempxm1);		/* free xm string */
	    XmStringFree(tempxm2);		/* free original xm string */
	    ok = True;
	    break;
	    
	case 0x33:				/* 3 octets per char */
	case 0x34:				/* 4 octets per char */
	    /* not supported */
	    ok = False;
	    break;

	default:
	    /* reserved for future use */
	    ok = False;
	    break;
	} /* end switch */
    } /* end if */

    return(ok);
}
  

/************************************************************************
 *
 *  XmCvtTextToXmString
 *	Convert a compound text string to a XmString.
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmCvtTextToXmString( display, args, num_args, from_val, to_val, converter_data )
        Display *display ;
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
        XtPointer *converter_data ;
#else
XmCvtTextToXmString(
        Display *display,
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val,
        XtPointer *converter_data )
#endif /* _NO_PROTO */
{
    Boolean		ok;

    if (from_val->addr == NULL)
	return( FALSE);

    ok = cvtTextToXmString(from_val, to_val);

    if (!ok)
    {
	to_val->addr = NULL;
	to_val->size = 0;
	XtAppWarningMsg(
			XtDisplayToApplicationContext(display),
			"conversionError","compoundText", "XtToolkitError",
			"Cannot convert compound text string to type XmString",
			(String *)NULL, (Cardinal *)NULL);
    }
    return(ok);
}


static Boolean 
#ifdef _NO_PROTO
cvtTextToXmString( from, to )
        XrmValue *from ;
        XrmValue *to ;
#else
cvtTextToXmString(
        XrmValue *from,
        XrmValue *to )
#endif /* _NO_PROTO */
{
    ct_context	    *ctx;		/* compound text context block */
    Boolean	    ok = True;
    Octet	    c;

    ctx = (ct_context *) XtMalloc(sizeof(ct_context));

/* initialize the context block */
    ctx->octet = (OctetPtr)from->addr;
    ctx->lastoctet = ctx->octet + strlen((char *)ctx->octet);
    ctx->flags.dircs = False;
    ctx->flags.gchar = False;
    ctx->flags.ignext = False;
    ctx->flags.gl = False;
    ctx->flags.text = False;
    ctx->dirstacksize = 8;
    ctx->dirstack = (ct_Direction *)
            XtMalloc(ctx->dirstacksize*sizeof(ct_Direction));
    ctx->dirstack[0] = ct_Dir_StackEmpty;
    ctx->dirstack[1] = ct_Dir_LeftToRight;
    ctx->dirsp = 1;
    ctx->item = NULL;
    ctx->itemlen = 0;
    ctx->version = CTVERSION;
    ctx->gl_charset = CS_ISO8859_1;
    ctx->gl_charset_size = 94;
    ctx->gl_octets_per_char = 1;
    ctx->gr_charset = CS_ISO8859_1;
    ctx->gr_charset_size = 96;
    ctx->gr_octets_per_char = 1;
    ctx->xmstring = NULL;
    ctx->xmsep = NULL;

/*
** check for version/ignore extensions sequence (must be first if present)
**  Format is:	ESC 02/03 V 03/00   ignoring extensions is OK
**		ESC 02/03 V 03/01   ignoring extensions is not OK
**  where V is in the range 02/00 thru 02/15 and represents versions 1 thru 16
*/
    if (    (from->size >= 4)
	&&  (ctx->octet[0] == ESC)
	&&  (ctx->octet[1] == 0x23)
	&&  (_IsInColumn2(ctx->octet[2])
	&&  ((ctx->octet[3] == 0x30) || ctx->octet[3] == 0x31))
       ) {
	ctx->version = ctx->octet[2] - 0x1f;	/* 0x20-0x2f => version 1-16 */
	if (ctx->octet[3] == 0x30)		/* 0x30 == can ignore extensions */
	    ctx->flags.ignext = True;
	ctx->octet += 4;			/* advance ptr to next seq */
    }


    while (ctx->octet < ctx->lastoctet) {
    switch (*ctx->octet) {			/* look at next octet in seq */
	case ESC:
	    /* %%% TEMP
	    ** if we have any text to output, do it
	    ** this section needs to be optimized so that it handles
	    ** paired character sets without outputting a new segment.
	    */
	    if (ctx->flags.text) {
		outputXmString(ctx, False);	/* with no separator */
	    }
	    ctx->flags.text = False;
	    ctx->item = ctx->octet;		/* remember start of this item */
	    ctx->itemlen = 0;

	    ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */

	    /* scan for final char */
	    while (	(ctx->octet != ctx->lastoctet)
		     && (_IsInColumn2(*ctx->octet)) ) {
		ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    }

	    if (ctx->octet == ctx->lastoctet) {	/* if nothing after this, it's an error */
		ok = False;
		break;
	    }

	    c = *ctx->octet;			/* get next char in seq */
	    ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    if (_IsValidESCFinal(c)) {
		/* we have a valid ESC sequence - handle it */
		ok = processESC(ctx, c);
	    } else {
		ok = False;
	    }
	    break;

	case CSI:
	    /*
	    ** CSI format is:	CSI P I F   where
	    **	    03/00 <= P <= 03/15
	    **	    02/00 <= I <= 02/15
	    **	    04/00 <= F <= 07/14
	    */
	    /* %%% TEMP
	    ** if we have any text to output, do it
	    ** This may need optimization.
	    */
	    if (ctx->flags.text) {
		/* check whether we have a specific direction set */
                if (((ctx->octet[1] == 0x31) && (ctx->octet[2] == 0x5d))||
                    ((ctx->octet[1] == 0x32) && (ctx->octet[2] == 0x5d))||
                    (ctx->octet[1] == 0x5d))
                        outputXmString(ctx, False);    /* without a separator*/
                else
			outputXmString(ctx, True);	/* with a separator */
	    }
	    ctx->flags.text = False;
	    ctx->item = ctx->octet;		/* remember start of this item */
	    ctx->itemlen = 0;

	    ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */

	    /* scan for final char */
	    while (	(ctx->octet != ctx->lastoctet)
		    &&	_IsInColumn3(*ctx->octet)  ) {
		ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    }
	    while (	(ctx->octet != ctx->lastoctet)
		    && _IsInColumn2(*ctx->octet)   ) {
		ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    }

	    /* if nothing after this, it's an error */
	    if (ctx->octet == ctx->lastoctet) {
		ok = False;
		break;
	    }

	    c = *ctx->octet;			/* get next char in seq */
	    ctx->octet++; ctx->itemlen++;	/* advance ptr to next char */
	    if (_IsValidCSIFinal(c)) {
		/* we have a valid CSI sequence - handle it */
		ok = processCSI(ctx, c);
	    } else {
		ok = False;
	    }
	    break;

	case NL:			    /* new line */
	    /* if we have any text to output, do it */
	    if (ctx->flags.text) {
		outputXmString(ctx, True);	/* with a separator */
		ctx->flags.text = False;
	    } else {
		XmString    save;
		if (ctx->xmsep == NULL) {
		    ctx->xmsep = XmStringSeparatorCreate();
		}
		save = ctx->xmstring;
		ctx->xmstring = XmStringConcat(ctx->xmstring,
						ctx->xmsep);
		XmStringFree(save);		/* free original xmstring */
	    }
	    ctx->octet++;			/* advance ptr to next char */
	    break;

	case HT:
	    /* Tab has no meaning in an XmString, so just ignore it. */
	    ctx->octet++;			/* advance ptr to next char */
	    break;

	default:			    /* just 'normal' text */
	    ctx->item = ctx->octet;		/* remember start of this item */
	    ctx->itemlen = 0;
	    ctx->flags.text = True;
	    while (ctx->octet < ctx->lastoctet) {
		c = *ctx->octet;
		if ((c == ESC) || (c == CSI) || (c == NL) || (c == HT)) {
		    break;
		}
		if (	(_IsInC0Set(c) && (!_IsValidC0(ctx, c)))
		    ||	(_IsInC1Set(c) && (!_IsValidC1(ctx, c))) ) {
		    ok = False;
		    break;
		}
		ctx->flags.gchar = True;	/* We have a character! */

                /*
                 *  We should look at the actual character to
                 *  decide whether it's a gl or gr character.
                 *
                 *  We'll hit the problem if we get a CT that
                 *  isn't generated by Motif.
                 */
                if (isascii((unsigned char)c)) {
		    ctx->octet += ctx->gl_octets_per_char;
		    ctx->itemlen += ctx->gl_octets_per_char;
		} else {
		    ctx->octet += ctx->gr_octets_per_char;
		    ctx->itemlen += ctx->gr_octets_per_char;
		}
		if (ctx->octet > ctx->lastoctet) {
		    ok = False;
		    break;
		}
	    } /* end while */
	    break;
	} /* end switch */
    if (!ok) break;
    } /* end while */

/* if we have any text left to output, do it */
    if (ctx->flags.text) {
	outputXmString(ctx, False);		/* with no separator */
    }

    XtFree((char *) ctx->dirstack);
    if (ctx->xmstring != NULL) {
	to->addr = (char *) ctx->xmstring;
	to->size = XmStringLength(ctx->xmstring);
    }

    if (ctx->xmsep != NULL) XmStringFree(ctx->xmsep);
    XtFree((char *) ctx);

    return (ok);
}

/* outputXmString */
static void 
#ifdef _NO_PROTO
outputXmString( ctx, separator )
        ct_context *ctx ;
        Boolean separator ;
#else
outputXmString(
        ct_context *ctx,
#if NeedWidePrototypes
        int separator )
#else
        Boolean separator )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    OctetPtr		tempstring;
    XmString		savedxm;

/*
**  If the GL charset is ISO8859-1, and the GR charset is any ISO8859
**  charset, then they're a pair, so we can create a single segment using
**  just the GR charset.
**
**  If GL and GR are multibyte charsets and they match (both GB2312 or both
**  KSC5601) except for JISX0208, then we can create a single segment using
**  just the GR charset.  If GL and GR are multibyte charsets and they DON'T
**  match, or if GL or GR is multibyte and the other is singlebyte, then
**  there's no way to tell which characters belong to GL and which to GR,
**  so treat it like a non-Latin1 in GL - 7 bit characters go to GL, 8 bit
**  characters to to GR.  *** THIS APPEARS TO BE A HOLE IN THE COMPOUND
**  TEXT SPEC ***.
**
**  Otherwise the charsets are not a pair and we will switch between GL
**  and GR segments each time the high bit changes.
*/
    if (    (	(ctx->gl_charset == CS_ISO8859_1)
	        &&	
	        (   (ctx->gr_charset == CS_ISO8859_1)
		||  (ctx->gr_charset == CS_ISO8859_2)
		||  (ctx->gr_charset == CS_ISO8859_3)
		||  (ctx->gr_charset == CS_ISO8859_4)
		||  (ctx->gr_charset == CS_ISO8859_5)
		||  (ctx->gr_charset == CS_ISO8859_6)
		||  (ctx->gr_charset == CS_ISO8859_7)
		||  (ctx->gr_charset == CS_ISO8859_8)
		||  (ctx->gr_charset == CS_ISO8859_9)
		)
	    )
	    ||
	    (	(ctx->gl_charset == CS_GB2312_0) && 
	        (ctx->gr_charset == CS_GB2312_1))
	    ||
	    (	(ctx->gl_charset == CS_KSC5601_0) && 
	        (ctx->gr_charset == CS_KSC5601_1))
	)
	{
	/* OK to do single segment output but always use GR charset */
	tempstring = (unsigned char *) XtMalloc(ctx->itemlen+1);
	strncpy((char *) tempstring, (char *) ctx->item, ctx->itemlen);
	tempstring[ctx->itemlen] = EOS;
	ctx->xmstring = concatStringToXmString
			    (	ctx->xmstring,
				(char *) tempstring,
				(char *) ctx->gr_charset,
				(XmStringDirection)
				    ((_CurDir(ctx) == ct_Dir_LeftToRight) ?
					XmSTRING_DIRECTION_L_TO_R :
					XmSTRING_DIRECTION_R_TO_L ),
				separator );
	XtFree((char *) tempstring);			/* free text version */
	}
    else
	{
	/* have to create a new segment everytime the highbit changes */
	unsigned int	i = 0, j = 0;
	Octet		c;
	Boolean		curseg_is_gl;

	curseg_is_gl = isascii((unsigned char)ctx->item[0]);

	tempstring = (unsigned char *) XtMalloc(ctx->itemlen+1);

	while (j < ctx->itemlen)
	    {
	    c = ctx->item[j];
	    if (isascii((unsigned char)c))
		{
		if (!curseg_is_gl)
		    {
		    /* output gr string */
		    tempstring[i] = EOS;
		    ctx->xmstring = concatStringToXmString
					(   ctx->xmstring,
					    (char *) tempstring,
					    (char *) ctx->gr_charset,
					    (XmStringDirection)
						((_CurDir(ctx) == ct_Dir_LeftToRight) ?
						    XmSTRING_DIRECTION_L_TO_R :
						    XmSTRING_DIRECTION_R_TO_L ),
					    False );
		    i = 0;			/* reset tempstring ptr */
		    curseg_is_gl = True;	/* start gl segment */
		    };
		tempstring[i++] = c;		/* copy octet to temp */
		j++;
		}
	    else
		{
		if (curseg_is_gl)
		    {
		    /* output gl string */
		    tempstring[i] = EOS;
		    ctx->xmstring = concatStringToXmString
					(   ctx->xmstring,
					    (char *) tempstring,
					    (char *) ctx->gl_charset,
					    (XmStringDirection)
						((_CurDir(ctx) == ct_Dir_LeftToRight) ?
						    XmSTRING_DIRECTION_L_TO_R :
						    XmSTRING_DIRECTION_R_TO_L ),
					    False );
		    i = 0;			/* reset tempstring ptr */
		    curseg_is_gl = False;	/* start gr segment */
		    };
		tempstring[i++] = c;		/* copy octet to temp */
		j++;
		}; /* end if */
	    }; /* end while */

	/* output last segment */
	tempstring[i] = EOS;
	ctx->xmstring = concatStringToXmString
				(   ctx->xmstring,
				    (char *) tempstring,
				    (char *)
					((curseg_is_gl) ?
					    ctx->gl_charset :
					    ctx->gr_charset ),
				    (XmStringDirection)
					((_CurDir(ctx) == ct_Dir_LeftToRight) ?
					    XmSTRING_DIRECTION_L_TO_R :
					    XmSTRING_DIRECTION_R_TO_L ),
				    False );
	XtFree((char *) tempstring);			/* free text version */
	if (separator)
	    {
	    if (ctx->xmsep == NULL)
		{
		ctx->xmsep = XmStringSeparatorCreate();
		};
	    savedxm = ctx->xmstring;
	    ctx->xmstring = XmStringConcat(ctx->xmstring, ctx->xmsep);
	    XmStringFree(savedxm);		/* free original xmstring */
	    };
	}; /* end if paired */
    return;
}

static XmString 
#ifdef _NO_PROTO
concatStringToXmString( compoundstring, textstring, charset, direction, separator )
        XmString compoundstring ;
        char *textstring ;
        char *charset ;
        XmStringDirection direction ;
        Boolean separator ;
#else
concatStringToXmString(
        XmString compoundstring,
        char *textstring,
        char *charset,
#if NeedWidePrototypes
        int direction,
        int separator )
#else
        XmStringDirection direction,
        Boolean separator )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmString	tempxm1, tempxm2;

    tempxm1 = XmStringSegmentCreate (   textstring,
					charset,
					direction,
					separator );
    tempxm2 = compoundstring;
    compoundstring = XmStringConcat(compoundstring, tempxm1);
    XmStringFree(tempxm1);			/* free xm version */
    XmStringFree(tempxm2);			/* free original xm string */
    return (compoundstring);
}


/* processESC - handle valid ESC sequences */
static Boolean 
#ifdef _NO_PROTO
processESC( ctx, final )
        ct_context *ctx ;
        Octet final ;
#else
processESC(
        ct_context *ctx,
#if NeedWidePrototypes
        int final )
#else
        Octet final )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Boolean	    ok;

    switch (ctx->item[1]) {
    case 0x24:			/* 02/04 - invoke 94(n) charset into GL or GR */
	ok = process94n(ctx, final);
	break;
    case 0x25:			/* 02/05 - extended segments */
	/* if we have any text to output, do it */
	if (ctx->flags.text) {
	    outputXmString(ctx, False);	/* with no separator */
	    ctx->flags.text = False;
	}
	ok = processExtendedSegments(ctx, final);
	break;
    case 0x28:			/* 02/08 - invoke 94 charset into GL */
	ok = process94GL(ctx, final);
	break;
    case 0x29:			/* 02/09 - invoke 94 charset into GR */
	ok = process94GR(ctx, final);
	break;
    case 0x2d:			/* 02/13 - invoke 96 charset into GR */
	ok =  process96GR(ctx, final);
	break;
    default:
	ok = False;
	break;
    }
    return(ok);
}


/*
**  processCSI - handle valid CSI sequences
**	CSI sequences
**	09/11 03/01 05/13   begin left-to-right text
**	09/11 03/02 05/13   begin right-to-left text
**	09/11 05/13	    end of string
**	09/11 P I F	    reserved for use in future extensions
*/
static Boolean 
#ifdef _NO_PROTO
processCSI( ctx, final )
        ct_context *ctx ;
        Octet final ;
#else
processCSI(
        ct_context *ctx,
#if NeedWidePrototypes
        int final )
#else
        Octet final )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Boolean	    ok = True;

    switch (final) {
    case 0x5d:				/* end of direction sequence */
	switch (ctx->item[1]) {
	case 0x31:			/* start left to right */
	    if (ctx->flags.gchar && ctx->dirsp == 0) {
		ok = False;
	    } else {
		_PushDir(ctx, ct_Dir_LeftToRight);
	    }
	    break;
	case 0x32:			/* start right to left */
	    if (ctx->flags.gchar && ctx->dirsp == 0) {
		ok = False;
	    } else {
		_PushDir(ctx, ct_Dir_RightToLeft);
	    }
	    break;
	case 0x5d:			/* Just CSI EOS - revert */
	    if (ctx->dirsp > 0) {
		_PopDir(ctx);
		
	    } else {
		ok = False;
	    }
	    break;
	default:			/* anything else is an error */
	    ok = False;
	}
	break;

    default:				/* reserved for future extensions */
	ok = False;
	break;
    }
    return(ok);
}



static Boolean 
#ifdef _NO_PROTO
processExtendedSegments( ctx, final )
        ct_context *ctx ;
        Octet final ;
#else
processExtendedSegments(
        ct_context *ctx,
#if NeedWidePrototypes
        int final )
#else
        Octet final )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    OctetPtr	    esptr;			/* ptr into ext seg */
    unsigned int    seglen;			/* length of ext seg */
    unsigned int    len;			/* length */
    String	    charset_copy;		/* ptr to NULL-terminated copy of ext seg charset */
    OctetPtr	    text_copy;			/* ptr to NULL-terminated copy of ext seg text */
    XmString	    tempxm1, tempxm2;
    Boolean	    ok = True;

    /* Extended segments
    **  01/11 02/05 02/15 03/00 M L	    variable # of octets/char
    **  01/11 02/05 02/15 03/01 M L	    1 octet/char
    **  01/11 02/05 02/15 03/02 M L	    2 octets/char
    **  01/11 02/05 02/15 03/03 M L	    3 octets/char
    **  01/11 02/05 02/15 03/04 M L	    4 octets/char
    */
    if (	(ctx->itemlen == 4)
	&&	(ctx->item[2] == 0x2f)
	&&	(_IsInColumn3(final))
       ) {
	if (    ((ctx->lastoctet - ctx->octet) < 2)
	    ||  (ctx->octet[0] < 0x80)
	    ||  (ctx->octet[1] < 0x80)
	   ) {
	    return(False);
	}

	/*
	** The most significant bit of M and L are always set to 1
	** The number is computed as ((M-128)*128)+(L-128)
	*/
	seglen = *ctx->octet - 0x80;
	ctx->octet++; ctx->itemlen++;		/* advance pointer */
	seglen = (seglen << 7) + (*ctx->octet - 0x80);
	ctx->octet++; ctx->itemlen++;		/* advance pointer */
	if ((ctx->lastoctet - ctx->octet) < seglen) {
	    return(False);
	}
	esptr = ctx->octet;			/* point to charset */
	ctx->itemlen += seglen;			/* advance pointer over segment */
	ctx->octet += seglen;

	switch (final) {
	case 0x30:				/* variable # of octets per char */
	case 0x31:				/* 1 octet per char */
	case 0x32:				/* 2 octets per char */
	    /* scan for STX separator between charset and text */
	    len = 0;
	    while (esptr[len] != STX)
		len++;
	    if (len > ctx->itemlen) {		/* if we ran off the end, error */
		ok = False;
		break;
	    }
	    charset_copy = XtMalloc(len + 1);
	    strncpy(charset_copy, (char *) esptr, len);
	    charset_copy[len] = EOS;
	    esptr += len + 1;			/* point to text part */
	    len = seglen - len - 1;		/* calc length of text part */
	    text_copy = (unsigned char *) XtMalloc(len + 1);
	    memcpy( text_copy, esptr, len);
	    text_copy[len] = EOS;
	    tempxm1 = XmStringSegmentCreate (	(char *) text_copy,
						charset_copy,
						(unsigned char ) (_CurDir(ctx) == ct_Dir_LeftToRight ?
							XmSTRING_DIRECTION_L_TO_R :
							XmSTRING_DIRECTION_R_TO_L ),
						False );
	    tempxm2 = ctx->xmstring;
	    ctx->xmstring = XmStringConcat(ctx->xmstring, tempxm1);
	    XtFree((char *) text_copy);
	    XtFree(charset_copy);
	    XmStringFree(tempxm1);		/* free xm string */
	    XmStringFree(tempxm2);		/* free original xm string */
	    ok = True;
	    break;
	    
	case 0x33:				/* 3 octets per char */
	case 0x34:				/* 4 octets per char */
	    /* not supported */
	    ok = False;
	    break;

	default:
	    /* reserved for future use */
	    ok = False;
	    break;
	} /* end switch */
    } /* end if */

    return(ok);
}


static Boolean 
#ifdef _NO_PROTO
process94n( ctx, final )
        ct_context *ctx ;
        Octet final ;
#else
process94n(
        ct_context *ctx,
#if NeedWidePrototypes
        int final )
#else
        Octet final )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    if (ctx->itemlen > 3) {
	switch (ctx->item[2]) {
	case 0x28:				/* into GL */
	    switch (final) {
	    case 0x41:				/* 04/01 - China (PRC) Hanzi */
		_SetGL(ctx, CS_GB2312_0, 94, 2);
		break;
	    case 0x42:				/* 04/02 - Japanese GCS, level 2 */
		_SetGL(ctx, CS_JISX0208_0, 94, 2);
		break;
	    case 0x43:				/* 04/03 - Korean GCS */
		_SetGL(ctx, CS_KSC5601_0, 94, 2);
		break;
	    default:
		/* other character sets are not supported */
		return False;
	    } /* end switch (final) */
	    break;

	case 0x29:				/* into GR */
	    switch (final) {
	    case 0x41:				/* 04/01 - China (PRC) Hanzi */
		_SetGR(ctx, CS_GB2312_1, 94, 2);
		break;
	    case 0x42:				/* 04/02 - Japanese GCS, level 2 */
		_SetGR(ctx, CS_JISX0208_1, 94, 2);
		break;
	    case 0x43:				/* 04/03 - Korean GCS */
		_SetGR(ctx, CS_KSC5601_1, 94, 2);
		break;
	    default:
		/* other character sets are not supported */
		return False;
	    } /* end switch (final) */
	    break;

	default:
	    /* error */
	    return False;
	} /* end switch item[2] */
    }
    else {
	/* error */
	return False;
    } /* end if */
    return True;
}



static Boolean 
#ifdef _NO_PROTO
process94GL( ctx, final )
        ct_context *ctx ;
        Octet final ;
#else
process94GL(
        ct_context *ctx,
#if NeedWidePrototypes
        int final )
#else
        Octet final )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    switch (final) {
    case 0x42:				/* 04/02 - Left half, ISO8859* (ASCII) */
	_SetGL(ctx, CS_ISO8859_1,  94, 1);
	break;
    case 0x4a:				/* 04/10 - Left half, Katakana */
	_SetGL(ctx, CS_JISX0201, 94, 1);
	break;
    default:
	return False;
    }

    return(True);
}


static Boolean 
#ifdef _NO_PROTO
process94GR( ctx, final )
        ct_context *ctx ;
        Octet final ;
#else
process94GR(
        ct_context *ctx,
#if NeedWidePrototypes
        int final )
#else
        Octet final )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    switch (final) {
    case 0x49:				/* 04/09 - Right half, Katakana */
	_SetGR(ctx, CS_JISX0201, 94, 1);
	break;
    default:
	return False;
    }

    return(True);
}



static Boolean 
#ifdef _NO_PROTO
process96GR( ctx, final )
        ct_context *ctx ;
        Octet final ;
#else
process96GR(
        ct_context *ctx,
#if NeedWidePrototypes
        int final )
#else
        Octet final )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    switch (final) {
    case 0x41:				/* 04/01 - Right half, Latin 1 */
	_SetGR(ctx, CS_ISO8859_1, 96, 1);
	break;
    case 0x42:				/* 04/02 - Right half, Latin 2 */
	_SetGR(ctx, CS_ISO8859_2, 96, 1);
	break;
    case 0x43:				/* 04/03 - Right half, Latin 3 */
	_SetGR(ctx, CS_ISO8859_3, 96, 1);
	break;
    case 0x44:				/* 04/04 - Right half, Latin 4 */
	_SetGR(ctx, CS_ISO8859_4, 96, 1);
	break;
    case 0x46:				/* 04/06 - Right half, Latin/Greek */
	_SetGR(ctx, CS_ISO8859_7, 96, 1);
	break;
    case 0x47:				/* 04/07 - Right half, Latin/Arabic */
	_SetGR(ctx, CS_ISO8859_6, 96, 1);
	break;
    case 0x48:				/* 04/08 - Right half, Latin/Hebrew */
	_SetGR(ctx, CS_ISO8859_8, 96, 1);
	break;
    case 0x4c:				/* 04/12 - Right half, Latin/Cyrillic */
	_SetGR(ctx, CS_ISO8859_5, 96, 1);
	break;
    case 0x4d:				/* 04/13 - Right half, Latin 5 */
	_SetGR(ctx, CS_ISO8859_9, 96, 1);
	break;
    default:
	return False;
    }

    return(True);
}


/************************************************************************
 *
 *  XmCvtXmStringToCT
 *	Convert an XmString to a compound text string directly.
 *	This is the public version of the resource converter and only
 *	requires the XmString as an argument.
 *
 ************************************************************************/
char * 
#ifdef _NO_PROTO
XmCvtXmStringToCT( string )
        XmString string ;
#else
XmCvtXmStringToCT(
        XmString string )
#endif /* _NO_PROTO */
{
  Boolean	ok;
  /* Dummy up some XrmValues to pass to cvtXmStringToText. */
  XrmValue	from_val;
  XrmValue	to_val;
  
  if (string == NULL)
    return ( (char *) NULL );
  
  from_val.addr = (char *) string;
  
  ok = cvtXmStringToText(&from_val, &to_val);
  
  if (!ok)
  {
    XtWarningMsg( "conversionError","compoundText", "XtToolkitError",
          "Cannot convert XmString to type compound text string", NULL, NULL) ;
    return( (char *) NULL ) ;
    }
  return( (char *) to_val.addr) ;
  }

/***************************************************************************
 *                                                                       *
 * _XmConvertCSToString - Converts compound string to corresponding      * 
 *   STRING if it can be fully converted.  Otherwise returns NULL.       *
 *                                                                       *
 ***************************************************************************/
char *
#ifdef _NO_PROTO
_XmConvertCSToString(cs)
     XmString cs;
#else
_XmConvertCSToString(XmString cs)
#endif /* _NO_PROTO */
{
  return((char *)NULL);
  
}


/***************************************************************************
 *									   *
 * _XmCvtXmStringToCT - public wrapper for the widgets to use.	  	   *
 *   This returns the length info as well - critical for the list widget   *
 * 									   *
 ***************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmCvtXmStringToCT( from, to )
        XrmValue *from ;
        XrmValue *to ;
#else
_XmCvtXmStringToCT(
        XrmValue *from,
        XrmValue *to )
#endif /* _NO_PROTO */
{
    return (cvtXmStringToText( from, to ));
}

/************************************************************************
 *
 *  XmCvtXmStringToText
 *	Convert an XmString to an ASCII string.
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmCvtXmStringToText( display, args, num_args, from_val, to_val, converter_data )
        Display *display ;
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
        XtPointer *converter_data ;
#else
XmCvtXmStringToText(
        Display *display,
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val,
        XtPointer *converter_data )
#endif /* _NO_PROTO */
{
    Boolean		ok;

    if (from_val->addr == NULL)
	return( FALSE) ;

    ok = cvtXmStringToText(from_val, to_val);

    if (!ok)
    {
	XtAppWarningMsg(
			XtDisplayToApplicationContext(display),
			"conversionError","compoundText", "XtToolkitError",
			"Cannot convert XmString to type compound text string",
			(String *)NULL, (Cardinal *)NULL);
    }
    return(ok);
}
  

/************************************************************************
 *
 *  cvtXmStringToText
 *    Convert an XmString to a compound text string.  This is the 
 *    underlying conversion routine for XmCvtXmStringToCT, 
 *    _XmCvtXmStringToCT, and XmCvtXmStringToText.
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
cvtXmStringToText( from, to )
        XrmValue *from ;
        XrmValue *to ;
#else
cvtXmStringToText(
        XrmValue *from,
        XrmValue *to )
#endif /* _NO_PROTO */
{
  Boolean			ok;
  OctetPtr			outc = NULL;
#ifndef OSF_v1_2_4
  unsigned int		outlen = 0;
#else /* OSF_v1_2_4 */
  unsigned int			outlen = 0;
#endif /* OSF_v1_2_4 */
  OctetPtr			ctext = NULL;
  XmStringContext		context = NULL;
#ifndef OSF_v1_2_4
  XmStringCharSet		charset = NULL, ct_encoding = NULL;
#else /* OSF_v1_2_4 */
  XmStringCharSet		charset = NULL, ct_encoding = NULL, cset_save = NULL;
#endif /* OSF_v1_2_4 */
  XmStringDirection		direction;
#ifndef OSF_v1_2_4
  Boolean			separator;
  ct_Direction		prev_direction = ct_Dir_LeftToRight;
#else /* OSF_v1_2_4 */
  ct_Direction			prev_direction = ct_Dir_LeftToRight;
#endif /* OSF_v1_2_4 */
  ct_Charset			prev_charset = cs_Latin1;
#ifdef OSF_v1_2_4
  XmStringComponentType		comp, utag;
  unsigned short		ulen;
  unsigned char			*uval;
#endif /* OSF_v1_2_4 */

  /* Initialize the return parameters. */
  to->addr = (XPointer) NULL;
  to->size = 0;

  ok = XmStringInitContext(&context, (XmString) from->addr);
  if (!ok) return(False);

#ifndef OSF_v1_2_4
  while (XmStringGetNextSegment (context, (char **) &ctext, &charset,
				 &direction, &separator)) {
#else /* OSF_v1_2_4 */
/* BEGIN OSF Fix CR 7403 */
  while ((comp = XmStringGetNextComponent(context, 
					  (char **)&ctext, &charset, &direction,
					  &utag, &ulen, &uval))
	  != XmSTRING_COMPONENT_END) {
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
    /* First output the direction, if changed */
    if (direction == XmSTRING_DIRECTION_L_TO_R) {
      if (prev_direction != ct_Dir_LeftToRight) {
	outc = ctextConcat(outc, outlen, 
			   (unsigned char *) CTEXT_L_TO_R,
			   (unsigned int)CTEXT_L_TO_R_LEN);
	outlen += CTEXT_L_TO_R_LEN;
	prev_direction = ct_Dir_LeftToRight;
      }
    }
    else {
      if (prev_direction != ct_Dir_RightToLeft) {
	outc = ctextConcat(outc, outlen, 
			   (unsigned char *) CTEXT_R_TO_L,
			   (unsigned int)CTEXT_R_TO_L_LEN);
	outlen += CTEXT_R_TO_L_LEN;
	prev_direction = ct_Dir_RightToLeft;
      }
    };

    /* Check Registry */
    ct_encoding = XmMapSegmentEncoding(charset);
    
    if (ct_encoding) {		/* We have a mapping. */
      XtFree(charset);
      ok = processCharsetAndText(ct_encoding, ctext, separator, 
				 &outc, &outlen, &prev_charset);
    }
    else 
#else /* OSF_v1_2_4 */
    switch (comp)
#endif /* OSF_v1_2_4 */
      {
#ifndef OSF_v1_2_4
	/* No mapping.  Vendor dependent. */
	ok = _XmOSProcessUnmappedCharsetAndText(charset, ctext, separator, 
						&outc, &outlen, &prev_charset);
	XtFree(charset);
      }
#else /* OSF_v1_2_4 */
      case XmSTRING_COMPONENT_LOCALE_TEXT:
	cset_save = XmFONTLIST_DEFAULT_TAG;
	/* Fall through */
      case XmSTRING_COMPONENT_TEXT:
	if (cset_save != NULL)
	  /* Check Registry */
	  ct_encoding = XmMapSegmentEncoding(cset_save);
    
	if (ct_encoding != NULL) {		/* We have a mapping. */
	  ok = processCharsetAndText(ct_encoding, ctext, FALSE, 
				     &outc, &outlen, &prev_charset);
	}
	else 
	  {
	    /* No mapping.  Vendor dependent. */
	    ok = _XmOSProcessUnmappedCharsetAndText(cset_save, ctext, FALSE, 
						    &outc, &outlen, &prev_charset);
	  }
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
      XtFree((char *)ctext);

      if (!ok)
	{
	  XmStringFreeContext(context);
	  return(False);
#else /* OSF_v1_2_4 */
	XtFree((char *)ctext);
	ctext = NULL;
	
	if (!ok)
	  {
	    XmStringFreeContext(context);
	    return(False);
	  }
	break;
	
      case XmSTRING_COMPONENT_CHARSET:
	if (cset_save == NULL)
	  cset_save = charset;
	else if (strcmp(cset_save, charset) != 0)
	  {
            if(    cset_save != XmFONTLIST_DEFAULT_TAG    )
              XtFree(cset_save);
	    cset_save = charset;
	  }
	break;
	
      case XmSTRING_COMPONENT_DIRECTION:
	/* Output the direction, if changed */
	if (direction == XmSTRING_DIRECTION_L_TO_R) {
	  if (prev_direction != ct_Dir_LeftToRight) {
	    outc = ctextConcat(outc, outlen, 
			       (unsigned char *) CTEXT_L_TO_R,
			       (unsigned int)CTEXT_L_TO_R_LEN);
	    outlen += CTEXT_L_TO_R_LEN;
	    prev_direction = ct_Dir_LeftToRight;
	  }
#endif /* OSF_v1_2_4 */
	}
#ifndef OSF_v1_2_4
  }				/* end while */
#else /* OSF_v1_2_4 */
	else {
	  if (prev_direction != ct_Dir_RightToLeft) {
	    outc = ctextConcat(outc, outlen, 
			       (unsigned char *) CTEXT_R_TO_L,
			       (unsigned int)CTEXT_R_TO_L_LEN);
	    outlen += CTEXT_R_TO_L_LEN;
	    prev_direction = ct_Dir_RightToLeft;
	  }
	};
	break;
	
      case XmSTRING_COMPONENT_SEPARATOR:
	if (ct_encoding != NULL) {		/* We have a mapping. */
	  ok = processCharsetAndText(ct_encoding, NULL, TRUE, 
				     &outc, &outlen, &prev_charset);
	}
	else 
	  {
	    /* No mapping.  Vendor dependent. */
	    ok = _XmOSProcessUnmappedCharsetAndText(cset_save, NULL, TRUE, 
						    &outc, &outlen, &prev_charset);
	  }
#endif /* OSF_v1_2_4 */

#ifdef OSF_v1_2_4
	if (!ok)
	  {
	    XmStringFreeContext(context);
	    return(False);
	  }
	break;
	
      default:
	/* Just ignore it. */
	break;
      }
  } /* end while */

    if ((cset_save != NULL) &&
	(cset_save != XmFONTLIST_DEFAULT_TAG))
      XtFree(cset_save);

/* END OSF Fix CR 7403 */
#endif /* OSF_v1_2_4 */
  if (outc != NULL) {
    to->addr = (char *) outc;
    to->size = outlen;
  }

  XmStringFreeContext(context);

  return(True);
}

static Boolean
#ifdef _NO_PROTO
processCharsetAndText(tag, ctext, separator, outc, outlen, prev)
     XmStringCharSet	tag;
     OctetPtr		ctext;
     Boolean		separator;
     OctetPtr		*outc;
     unsigned int	*outlen;
     ct_Charset		*prev;
#else
processCharsetAndText(XmStringCharSet tag,
		      OctetPtr		ctext,
#if NeedWidePrototypes
		      int		separator,
#else
		      Boolean		separator,
#endif /* NeedWidePrototypes */
		      OctetPtr		*outc,
		      unsigned int	*outlen,
		      ct_Charset	*prev)
#endif /* _NO_PROTO */
{
  unsigned int		ctlen = 0, len;

  if (strcmp(tag, XmFONTLIST_DEFAULT_TAG) == 0)
    {
      XTextProperty	prop_rtn;
      int		ret_val;
      String		msg;
	
#ifndef OSF_v1_2_4
      /* Call XmbTextListToTextProperty */
      ret_val = 
	XmbTextListToTextProperty(_XmGetDefaultDisplay(), (char **)&ctext,
				  1, XCompoundTextStyle, &prop_rtn);

      if (ret_val)
#else /* OSF_v1_2_4 */
      if (ctext != NULL)
#endif /* OSF_v1_2_4 */
	{
#ifndef OSF_v1_2_4
	  switch (ret_val)
#else /* OSF_v1_2_4 */
	  /* Call XmbTextListToTextProperty */
	  ret_val = 
	    XmbTextListToTextProperty(_XmGetDefaultDisplay(), (char **)&ctext,
				      1, XCompoundTextStyle, &prop_rtn);

	  if (ret_val)
#endif /* OSF_v1_2_4 */
	    {
#ifndef OSF_v1_2_4
	    case XNoMemory:
	      msg = "Insufficient memory for XmbTextListToTextProperty";
	      break;
	    case XLocaleNotSupported:
	      msg = "Locale not supported for XmbTextListToTextProperty";
	      break;
	    default:
	      msg = "XmbTextListToTextProperty failed";
	      break;
	    }
#else /* OSF_v1_2_4 */
	      switch (ret_val)
		{
		case XNoMemory:
		  msg = "Insufficient memory for XmbTextListToTextProperty";
		  break;
		case XLocaleNotSupported:
		  msg = "Locale not supported for XmbTextListToTextProperty";
		  break;
		default:
		  msg = "XmbTextListToTextProperty failed";
		  break;
		}
#endif /* OSF_v1_2_4 */
	    
#ifndef OSF_v1_2_4
	  XtWarningMsg("conversionError", "textProperty", "XtToolkitError",
		       msg, NULL, 0);
#else /* OSF_v1_2_4 */
	      XtWarningMsg("conversionError", "textProperty", "XtToolkitError",
			   msg, NULL, 0);
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
	  return(False);
	}
#else /* OSF_v1_2_4 */
	      return(False);
	    }
#endif /* OSF_v1_2_4 */
	
#ifndef OSF_v1_2_4
      ctlen = strlen((char *)prop_rtn.value);
#else /* OSF_v1_2_4 */
	  ctlen = strlen((char *)prop_rtn.value);
#endif /* OSF_v1_2_4 */
	
#ifndef OSF_v1_2_4
      /* Now copy in the text */
      if (ctlen > 0) {
	*outc = ctextConcat(*outc, *outlen, prop_rtn.value, ctlen);
	*outlen += ctlen;
      };
#else /* OSF_v1_2_4 */
	  /* Now copy in the text */
	  if (ctlen > 0) {
	    *outc = ctextConcat(*outc, *outlen, prop_rtn.value, ctlen);
	    *outlen += ctlen;
	  };
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
      XFree(prop_rtn.value);

#else /* OSF_v1_2_4 */
	  XFree(prop_rtn.value);
	}
      
#endif /* OSF_v1_2_4 */
      /* Finally, add the separator if any */
      if (separator) {
	*outc = ctextConcat(*outc, *outlen, 
			    (unsigned char *)NEWLINESTRING, 
			    (unsigned int)NEWLINESTRING_LEN);
	(*outlen)++;
      };
      return(True);
    }
	  
  /* Next output the charset */
  if (strcmp(tag, CS_ISO8859_1) == 0) {
    if (*prev != cs_Latin1) {
      *outc = ctextConcat(*outc, *outlen, 
			  (unsigned char *)CTEXT_SET_ISO8859_1, 
			  (unsigned int)CTEXT_SET_ISO8859_1_LEN);
      *outlen += CTEXT_SET_ISO8859_1_LEN;
      *prev = cs_Latin1;
    };
  }
  else if (strcmp(tag, CS_ISO8859_2) == 0) {
    if (*prev != cs_Latin2) {
      *outc = ctextConcat(*outc, *outlen, 
			  (unsigned char *)CTEXT_SET_ISO8859_2, 
			  (unsigned int)CTEXT_SET_ISO8859_2_LEN);
      *outlen += CTEXT_SET_ISO8859_2_LEN;
      *prev = cs_Latin2;
    };
  }
  else if (strcmp(tag, CS_ISO8859_3) == 0) {
    if (*prev != cs_Latin3) {
      *outc = ctextConcat(*outc, *outlen, 
			  (unsigned char *)CTEXT_SET_ISO8859_3, 
			  (unsigned int)CTEXT_SET_ISO8859_3_LEN);
      *outlen += CTEXT_SET_ISO8859_3_LEN;
      *prev = cs_Latin3;
    };
  }
  else if (strcmp(tag, CS_ISO8859_4) == 0) {
    if (*prev != cs_Latin4) {
      *outc = ctextConcat(*outc, *outlen, 
			  (unsigned char *)CTEXT_SET_ISO8859_4, 
			  (unsigned int)CTEXT_SET_ISO8859_4_LEN);
      *outlen += CTEXT_SET_ISO8859_4_LEN;
      *prev = cs_Latin4;
    };
  }
  else if (strcmp(tag, CS_ISO8859_5) == 0) {
    if (*prev != cs_LatinCyrillic) {
      *outc = ctextConcat(*outc, *outlen, 
			  (unsigned char *)CTEXT_SET_ISO8859_5, 
			  (unsigned int)CTEXT_SET_ISO8859_5_LEN);
      *outlen += CTEXT_SET_ISO8859_5_LEN;
      *prev = cs_LatinCyrillic;
    };
  }
  else if (strcmp(tag, CS_ISO8859_6) == 0) {
    if (*prev != cs_LatinArabic) {
      *outc = ctextConcat(*outc, *outlen, 
			  (unsigned char *)CTEXT_SET_ISO8859_6, 
			  (unsigned int)CTEXT_SET_ISO8859_6_LEN);
      *outlen += CTEXT_SET_ISO8859_6_LEN;
      *prev = cs_LatinArabic;
    };
  }
  else if (strcmp(tag, CS_ISO8859_7) == 0) {
    if (*prev != cs_LatinGreek) {
      *outc = ctextConcat(*outc, *outlen, 
			  (unsigned char *)CTEXT_SET_ISO8859_7, 
			  (unsigned int)CTEXT_SET_ISO8859_7_LEN);
      *outlen += CTEXT_SET_ISO8859_7_LEN;
      *prev = cs_LatinGreek;
    };
  }
  else if (strcmp(tag, CS_ISO8859_8) == 0) {
    if (*prev != cs_LatinHebrew) {
      *outc = ctextConcat(*outc, *outlen, 
			  (unsigned char *)CTEXT_SET_ISO8859_8, 
			  (unsigned int)CTEXT_SET_ISO8859_8_LEN);
      *outlen += CTEXT_SET_ISO8859_8_LEN;
      *prev = cs_LatinHebrew;
    };
  }
  else
    if (strcmp(tag, CS_ISO8859_9) == 0) {
      if (*prev != cs_Latin5) {
	*outc = ctextConcat(*outc, *outlen, 
			    (unsigned char *)CTEXT_SET_ISO8859_9, 
			    (unsigned int)CTEXT_SET_ISO8859_9_LEN);
	*outlen += CTEXT_SET_ISO8859_9_LEN;
	*prev = cs_Latin5;
      };
    }
    else if (strcmp(tag, CS_JISX0201) == 0) {
      if (*prev != cs_Katakana) {
	*outc = ctextConcat(*outc, *outlen, 
			    (unsigned char *)CTEXT_SET_JISX0201, 
			    (unsigned int)CTEXT_SET_JISX0201_LEN);
	*outlen += CTEXT_SET_JISX0201_LEN;
	*prev = cs_Katakana;
      };
    }
    else if ((strcmp(tag, CS_GB2312_0) == 0) ||
	     (strcmp(tag, CS_GB2312_1) == 0)) {
      if (*prev != cs_Hanzi) {
	*outc = ctextConcat(*outc, *outlen, 
			    (unsigned char *)CTEXT_SET_GB2312_0, 
			    (unsigned int)CTEXT_SET_GB2312_0_LEN);
	*outlen += CTEXT_SET_GB2312_0_LEN;
	*prev = cs_Hanzi;
      };
    }
    else if ((strcmp(tag, CS_JISX0208_0) == 0) ||
	     (strcmp(tag, CS_JISX0208_1) == 0)) {
      if (*prev != cs_JapaneseGCS) {
	*outc = ctextConcat(*outc, *outlen, 
			    (unsigned char *)CTEXT_SET_JISX0208_0, 
			    (unsigned int)CTEXT_SET_JISX0208_0_LEN);
	*outlen += CTEXT_SET_JISX0208_0_LEN;
	*prev = cs_JapaneseGCS;
      };
    }
    else if ((strcmp(tag, CS_KSC5601_0) == 0) ||  
	     (strcmp(tag, CS_KSC5601_1) == 0)) {
      if (*prev != cs_KoreanGCS) {
	*outc = ctextConcat(*outc, *outlen, 
			    (unsigned char *)CTEXT_SET_KSC5601_0, 
			    (unsigned int)CTEXT_SET_KSC5601_0_LEN);
	*outlen += CTEXT_SET_KSC5601_0_LEN;
	*prev = cs_KoreanGCS;
      };
    }
    else {
      /* Must be a non-standard character set! */
      OctetPtr        temp;

      len = strlen(tag);
      temp = (unsigned char *) XtMalloc(*outlen + 6 + len + 2);
      /* orig + header + tag + STX + EOS */
      memcpy( temp, *outc, *outlen);
      XtFree((char *) *outc);
      *outc = temp;
      temp = &(*outc[*outlen]);
      /*
       ** Format is:
       **     01/11 02/05 02/15 03/nn M L tag 00/02 text
       */
      *temp++ = 0x1b;
      *temp++ = 0x25;
      *temp++ = 0x2f;
      /*
       ** HACK!  The next octet in the sequence is the # of octets/char.
       ** XmStrings don't have this information, so just set it to be
       ** variable # of octets/char, and hope the caller knows what to do.
       */
      *temp++ = 0x30;
      /* encode len in next 2 octets */
      *temp++ = 0x80 + (len+ctlen+1)/128; 
      *temp++ = 0x80 + (len+ctlen+1)%128;
      strcpy((char *) temp, tag);
      temp += len;
      *temp++ = STX;
      *temp = EOS;		/* make sure there's a \0 on the end */
      *prev = cs_NonStandard;
      *outlen += 6 + len + 1;
    };
      
#ifndef OSF_v1_2_4
  ctlen = strlen((char *)ctext);
#else /* OSF_v1_2_4 */
  if (ctext != NULL)
    {
      ctlen = strlen((char *)ctext);
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
  /* Now copy in the text */
  if (ctlen > 0) {
    *outc = ctextConcat(*outc, *outlen, ctext, ctlen);
    *outlen += ctlen;
  };

#else /* OSF_v1_2_4 */
      /* Now copy in the text */
      if (ctlen > 0) {
	*outc = ctextConcat(*outc, *outlen, ctext, ctlen);
	*outlen += ctlen;
      };
    }
  
#endif /* OSF_v1_2_4 */
  /* Finally, add the separator if any */
  if (separator) {
    *outc = ctextConcat(*outc, *outlen, 
			(unsigned char *)NEWLINESTRING, 
			(unsigned int)NEWLINESTRING_LEN);
    (*outlen)++;
  }
  return(True);
}
  

static OctetPtr 
#ifdef _NO_PROTO
ctextConcat( str1, str1len, str2, str2len )
        OctetPtr str1 ;
        unsigned int str1len ;
        OctetPtr str2 ;
        unsigned int str2len ;
#else
ctextConcat(
        OctetPtr str1,
        unsigned int str1len,
        OctetPtr str2,
        unsigned int str2len )
#endif /* _NO_PROTO */
{

	str1 = (OctetPtr)XtRealloc((char *)str1, (str1len + str2len + 1));
	memcpy( &str1[str1len], str2, str2len);
	str1[str1len+str2len] = EOS;
	return(str1);
}

/************************************************************************
 *
 *  XmCvtStringToAtomList
 *	Convert a string to an array of atoms.  Atoms within the string
 *  are delimited by commas.  If the comma is preceded by a backslash,
 *  it is considered to be part of the atom.
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
_XmCvtStringToAtomList(dpy, args, num_args,
	from, to, converter_data)
Display *dpy;
XrmValue *args;
Cardinal *num_args;
XrmValue *from;
XrmValue *to;
XtPointer *converter_data;
#else
_XmCvtStringToAtomList(
	Display *dpy,
	XrmValue *args,
	Cardinal *num_args,
	XrmValue *from,
	XrmValue *to,
	XtPointer *converter_data )
#endif /* _NO_PROTO */
{
	static char *delimiter_string = ",";
	char *atom_string;
	char *src_string;
	Atom stack_atoms[128];
	int num_stack_atoms = XtNumber(stack_atoms);
	Atom *atom_list = stack_atoms;
	int max_atoms = num_stack_atoms;
	int atom_count;
	Atom *ret_list;

	if (from->addr == NULL)
		return(False);
	
	src_string = (char *) from->addr;
	
	atom_count = 0;
	for (atom_string = GetNextToken(src_string, delimiter_string);
		atom_string != NULL;
		atom_string = GetNextToken(NULL, delimiter_string))
	{
		if (atom_count == max_atoms)
		{
			max_atoms *= 2;

			if (atom_list == stack_atoms)
			{
				Atom *new_list;

				new_list = (Atom *) XtMalloc(sizeof(Atom) * max_atoms);
				memcpy((char *)new_list, (char *)atom_list,
					(sizeof(Atom) * atom_count));
				atom_list = new_list;
			}
			else
				atom_list = (Atom *) XtRealloc((char *)atom_list,
					max_atoms);
		}

		atom_list[atom_count++] = XmInternAtom(dpy, atom_string,
			False);
		XtFree(atom_string);
	}

	/*
	 * Since the atom array is painfully persistent, we return the
	 * smallest one we can.
	 */
	ret_list = (Atom *) XtMalloc(sizeof(Atom) * atom_count);
	memcpy( ret_list, atom_list, sizeof(Atom) * atom_count);

	if (atom_list != stack_atoms)
		XtFree((char *) atom_list);

	{
		static Atom *buf;

		if(to->addr)
		{
			if(to->size < sizeof(Atom *))
			{
				XtFree((char *) ret_list);
				to->size = sizeof(Atom *);
				return(False);
			}
			else
				*((Atom **) (to->addr)) = ret_list;
		}
		else
		{
			buf = ret_list;
			to->addr = (XPointer) &buf;
		}

		to->size = sizeof(Atom *);
		return(True);
	}
}

static void 
#ifdef _NO_PROTO
_XmSimpleDestructor( app, to, data, args, num_args )
        XtAppContext app ;
        XrmValue *to ;
        XtPointer data ;
        XrmValue *args ;
        Cardinal *num_args ;
#else
_XmSimpleDestructor(
        XtAppContext app,
        XrmValue *to,
        XtPointer data,
        XrmValue *args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
   char *simple_struct = *(char **)(to->addr);

   XtFree(simple_struct);
}

/*
 *
 * GetNextToken
 *
 * This should really be in some sort of utility library.
 * This function is supposed to behave a bit like strtok in that it
 * saves a context which is used if src is NULL.  We'd like to use
 * strok, but strok can't handle backslashes.
 *
 * A token is the contiguous substring of src which begins with either
 * a backslashed space character or a non-space character and
 * terminates with occurance of a non-backslashed delimiter character
 * or the character before the last non-backshashed space character.
 *
 * Caller is responsible to free the returned string.
 *
 * Example A:
 *    The delimiter string is ","   The src is
 *           "   \ token  token\ , next token"
 *    The token is
 *           " token token "
 *
 * Example B:
 *
 *    The delimiter string is
 *        ".:"
 *    The src is 
 *        "   \: the \t token \. \    : next token  "
 *    The token returned is
 *        ": the \t token .  "
 *
 */

static Boolean
#ifdef _NO_PROTO
OneOf(c, set)
	char c;
	char *set;
#else
OneOf(
#if NeedWidePrototypes
        int c,
#else
        char c,
#endif /* NeedWidePrototypes */
	char *set )
#endif /* _NO_PROTO */
{
	char *p;

	for (p = set; *p != 0; p++)
		if (*p == c)
			return(True);
	
	return(False);
}

static char * 
#ifdef _NO_PROTO
GetNextToken(src, delim)
	char *src;
	char *delim;
#else
GetNextToken(
	char *src,
	char *delim )
#endif /* _NO_PROTO */
{
	static char *context;
	Boolean terminated = False;
	char *s, *e, *p;
	char *next_context;
	char *buf = NULL;
	int len;

	if (src != NULL)
		context = src;

	if (context == NULL)
		return(NULL);

	s = context;

	/* find the end of the token */
	for (e = s = context; (!terminated) && (*s != '\0'); e = s++)
	{
		if ((*s == '\\') && (*(s+1) != '\0'))
			s++;
		else if (OneOf(*s, delim))
			terminated = True;
	}

	/* assert (OneOf(*e,delim) || (*e == '\0')) */
	if (terminated)
	{
		next_context = (e + 1);
		e--;
	}
	else
		next_context = NULL;
	
	/* Strip out non-backslashed leading and trailing whitespace */
	s = context;
	while ((s != e) && isspace((unsigned char)*s))
		s++;
	while ((e != s) && isspace((unsigned char)*e) && ((*e-1) != '\\'))
		e--;

	if (e == s)
	{
		/*
		 * Only white-space between the delimiters,
		 * if we're at the end of the string anyway, indicate
		 * that we're done, otherwise return an empty string.
		 */
		if (terminated)
		{
			buf = (char *) XtMalloc(1);
			*buf = '\0';
			return(buf);
		}
		else
			return(NULL);
	}
	
	/*
	 * Copy into buffer.  Swallow any backslashes which precede
	 * delimiter characters or spaces.  It would be great if we had
	 * time to implement full C style backslash processing...
	 */
	len = (e - s) + 1;

	p = buf = XtMalloc(len + 1);
	while (s != e)
	{
		if ((*s == '\\') && 
		    (OneOf(*(s+1), delim) || isspace((unsigned char)*(s+1))))
			s++;
		
		*(p++) = *(s++);
	}
	*(p++) = *(s++);
	*p = '\0';

	context = next_context;

	return(buf);
}

static Boolean
#ifdef _NO_PROTO
_XmCvtStringToCardinal( display, args, num_args, from, to, converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToCardinal(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    Cardinal value;
    int intermediate;
    if (!isInteger(from->addr,&intermediate) || intermediate < 0)
	{
	XtStringConversionWarning((char *)from->addr, XmRCardinal);
	return False;
	}

    value = (Cardinal) intermediate;
    done( to, Cardinal, value, ; )
}


static Boolean
#ifdef _NO_PROTO
_XmCvtStringToTextPosition( display, args, num_args, from, to, converter_data )
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToTextPosition(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    XmTextPosition value;
    int intermediate;
    if (!isInteger(from->addr,&intermediate) || intermediate < 0)
        {
        XtStringConversionWarning((char *)from->addr, XmRTextPosition);
        return False;
        }

    value = (XmTextPosition) intermediate;
    done( to, XmTextPosition, value, ; )
}


static Boolean
#ifdef _NO_PROTO
_XmCvtStringToTopItemPosition(display, args, num_args, from, to, converter_data)
        Display *display ;
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
_XmCvtStringToTopItemPosition(
        Display *display,
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{
    int value;
    int intermediate;
    if (!isInteger(from->addr,&intermediate) || intermediate < 0)
        {
        XtStringConversionWarning((char *)from->addr, XmRTopItemPosition);
        return False;
        }

    value = intermediate - 1;
    done( to, int, value, ; )
}


static Boolean 
#ifdef _NO_PROTO
isInteger(string, value)
    String string;
    int *value;		/* RETURN */
#else
isInteger(
    String string,
    int *value)		/* RETURN */
#endif /* _NO_PROTO */
{
    Boolean foundDigit = False;
    Boolean isNegative = False;
    Boolean isPositive = False;
    int val = 0;
    char ch;
    /* skip leading whitespace */
    while ((ch = *string) == ' ' || ch == '\t') string++;
    while ((ch = *string++) != '\0') {
	if (ch >= '0' && ch <= '9') {
	    val *= 10;
	    val += ch - '0';
	    foundDigit = True;
	    continue;
	}
	if (ch == ' ' || ch == '\t') {
	    if (!foundDigit) return False;
	    /* make sure only trailing whitespace */
	    while ((ch = *string++) != '\0') {
		if (ch != ' ' && ch != '\t')
		    return False;
	    }
	    break;
	}
	if (ch == '-' && !foundDigit && !isNegative && !isPositive) {
	    isNegative = True;
	    continue;
	}
	if (ch == '+' && !foundDigit && !isNegative && !isPositive) {
	    isPositive = True;
	    continue;
	}
	return False;
    }
    if (ch == '\0') {
	if (isNegative)
	    *value = -val;
	else
	    *value = val;
	return True;
    }
    return False;
}
#ifdef IBM_MOTIF /* FOR BINARY COMPATIBILITY */

XmFontList
#ifdef _NO_PROTO
XmStringLoadQueryFont(display, fontlist)
    Display *display;
    char *fontlist;
#else
XmStringLoadQueryFont(
    Display *display,
    char *fontlist)
#endif
{
    XrmValue args[1];
    Cardinal num_args;
    XrmValue from, to;

    args[0].addr = (XtPointer)&display;
    args[0].size = sizeof(Display *);
    num_args = 1;

    from.addr = fontlist;
    from.size = strlen(fontlist);
    to.addr = NULL;
    to.size = sizeof(XmFontList);

    if(_XmCvtStringToXmFontList(display, args, &num_args, &from, &to, NULL))
        return(*(XmFontList *)to.addr);
    else
        return(NULL);
}
#endif /* IBM_MOTIF */
