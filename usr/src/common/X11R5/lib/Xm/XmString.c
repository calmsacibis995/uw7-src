#pragma ident	"@(#)m1.2libs:Xm/XmString.c	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
ifndef OSF_v1_2_4
 * Motif Release 1.2.3
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
*  (c) Copyright 1989, 1990, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
 * this enables compilation of debugging routines at the end
 */

/***********/
/* #ifndef _XM_DEBUG_XMSTRING */
/* #define _XM_DEBUG_XMSTRING */
/* #endif */
/************/
#include <stdio.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" { /* some 'locale.h' do not have prototypes (sun) */
#endif
#include <X11/Xlocale.h>
#ifdef __cplusplus
} /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#ifdef OSF_v1_2_4
#include <Xm/XmosP.h>
#endif /* OSF_v1_2_4 */
#include "XmI.h"
#ifndef OSF_v1_2_4
#include "XmosP.h"
#endif /* OSF_v1_2_4 */
#include <Xm/AtomMgr.h>

/* These are the os-specific environment variables checked for a language
** specification.
*/
#define env_variable "LANG"

struct __Xmlocale {
    char   *charset;
    int    charsetlen;
    Boolean inited;
};

#define CVT_STRING_SIZE         1000  /*  max size of a string that the */
                                      /* type convertes will handle */


/* defines for which line for calculating heights */
#define XMSTRING_FIRST_LINE	0
#define XMSTRING_MIDDLE_LINE	1
#define XMSTRING_LAST_LINE	2

#define two_byte_font(f)        (( (f)->min_byte1 != 0 || (f)->max_byte1 != 0))

/*
 * calculates the number of bytes in the header of an external compound
 * string, given the total length of the components.
 */

#define _calc_header_size(len) \
    ((((unsigned short)(len)) > MAXSHORTVALUE) ? (CSHEADERLEN + CSLONGLEN) : (CSHEADERLEN + CSSHORTLEN))

#define _asn1_size(len) \
    ((((unsigned short)(len)) > MAXSHORTVALUE) ? (ASNTAG + CSLONGLEN) : (ASNTAG + CSSHORTLEN))

#define _is_asn1_long(p) \
  ((*((unsigned char *)(p) + ASNTAG)) & ((unsigned char)CSLONGBIT))

#define Half(x)		(x >> 1)

/****************
 *
 * Because of possible byte alignment problems, we define a macro to
 * read and write the short quantities. If we are aligned, the shorts 
 * are stored low byte = low 8 bits
 *
 ****************/
#ifdef STRINGS_ALIGNED

#define WriteShort(addr, short_val)                         \
        *((unsigned char *)addr) = (unsigned char)(short_val & 0xff);       \
        *(((unsigned char *)addr) + 1) = (unsigned char)(short_val >> 8);  

#define ReadShort(addr, short_val)                                  \
        short_val = (((unsigned short) *(((unsigned char *)addr) + 1)) << 8) \
                | ((unsigned short ) *((unsigned char *)(addr)));
#else

#ifdef NON_OSF_FIX

    /* Some compilers may require that we explicitly cast between unsigned */
    /* char ptr and unsigned-short ptr, in order for this macro to work */

#define WriteShort(addr, short_val)  *((unsigned char *) addr) = short_val;
#define ReadShort(addr, short_val)   short_val = *((unsigned short *) addr);

#else

#define WriteShort(addr, short_val) *addr = short_val;
#define ReadShort(addr, short_val) short_val = *addr;        

#endif /* NON_OSF_FIX */

#endif  /* STRINGS_ALIGNED */

typedef struct _FontlistCache {
    XmFontList fontlist;
    struct _FontlistCache *next;
    int        refcount;
  } FontlistEntry;

/*
 * Data structure macros for fontlist and fontcache access
 */

#define  FontListType(f)		((XmFontList)(f))->type
#define  FontListFont(f)		((XmFontList)(f))->font
#define  FontListTag(f)			((XmFontList)(f))->tag
#define  FontListCharset		FontListTag
#define  FontCacheFontList(cache)	((FontlistEntry *)(cache))->fontlist
#define  FontCacheNext(cache)		(((struct _FontlistCache *)(cache)) \
					 ->next)
#define  FontCacheRefCount(cache)       ((FontlistEntry *)(cache))->refcount
#define  FontListContextNext(context)	(((XmFontListContextRec *)(context))-> \
					nextPtr)
#define  FontListContextError(context)	(((XmFontListContextRec *)(context))-> \
					error)

/*
 * Macros for the optimized segment data structure access
 */
#define _XmOptCharsetGet(opt)           _charset_cache[ \
                                          ((_XmStringOpt)(opt))->charset_index]
#define _XmOptCharsetIndex(opt)         ((_XmStringOpt)(opt))->charset_index
#define _XmOptWidthUpdated(opt)         ((_XmStringOpt)(opt))->width_updated
#define _XmOptPixelWidth(opt)           ((_XmStringOpt)(opt))->pixel_width
#define _XmOptCharCount(opt)            ((_XmStringOpt)(opt))->char_count
#define _XmOptText(opt)                 ((_XmStringOpt)(opt))->text
#define _XmOptDirectionGet(opt)         ((_XmStringOpt)(opt))->direction
#define _XmOptDirectionSet(opt,val)    (((_XmStringOpt)(opt))->direction = val)

/*
 * Macros for non-optimized segment data structure access
 */

#define _XmSegCharset(seg)		((_XmStringSegment)(seg))->charset
#define _XmSegFontIndex(seg)		((_XmStringSegment)(seg))->font_index
#define _XmSegCharCount(seg)		((_XmStringSegment)(seg))->char_count
#define _XmSegText(seg)			((_XmStringSegment)(seg))->text
#define _XmSegDirection(seg)		((_XmStringSegment)(seg))->direction
#define _XmSegPixelWidth(seg)		((_XmStringSegment)(seg))->pixel_width

/*
 * Macros for line data structure access
 */

#define _XmStrLineSegCount(line)        ((_XmStringLine)(line))->segment_count
#define _XmStrLineSegment(line)	        ((_XmStringLine)(line))->segment

/*
 * Macros for internal string data structure access
 */

#define _XmStrOptimized(str)		((_XmString)(str))->optimized
#define _XmStrLineCnt(str)		((_XmString)(str))->line_count
#define _XmStrLineLine(str)		((_XmString)(str))->line

/*
 * Macros for string internal context block data structure access
 */

#define _XmStrContString(cont)	  \
                 ((_XmStringContextRec *)(cont))->string
#define _XmStrContCurrLine(cont)  \
                 ((_XmStringContextRec *)(cont))->current_line
#define _XmStrContCurrSeg(cont)   \
                 ((_XmStringContextRec *)(cont))->current_seg
#define _XmStrContOpt(cont)	  \
                 ((_XmStringContextRec *)(cont))->optimized
#define _XmStrContError(cont)	((_XmStringContextRec *)(cont))->error


/*
 * Macros for string external context block data structure access
 */

#define XmStrContString(cont)	((XmStringContextRec *)(cont))->string
#define XmStrContOffset(cont)	((XmStringContextRec *)(cont))->offset
#define XmStrContLength(cont)	((XmStringContextRec *)(cont))->length
#define XmStrContCharSet(cont)	((XmStringContextRec *)(cont))->charset
#define XmStrContCharSetLen(cont)	\
                 ((XmStringContextRec *)(cont))->charset_length
#define XmStrContDirection(cont)	\
                 ((XmStringContextRec *)(cont))->direction
#define XmStrContError(cont)	((XmStringContextRec *)(cont))->error
#define XmStrContASN1(cont)	\
  _is_asn1(((XmStringContextRec *)(cont))->string)

/*
 * this set constructs external XmString format object.  The TLV version
 */

/*
 	The external version of XmString is:

	COMPOUND_STRING			4 or 6 bytes (see description below)

	    component tag		1 byte
	    length			1 or 3 bytes
	    value			n bytes

	    component tag		1 byte
	    length			1 or 3 bytes
	    value			n bytes

	eg. very simple... 
*/


/*
 * ASN.1 header for compound string - 3 byte header, followed by length
 * which is three bytes maximum, but almost always 1 byte.
 *
 * The first byte defines the ASN.1 space:  (0xdf)
 *              1 1      0      1 1 1 1 1
 *              class    form   ID code
 *
 *    class is private, form is primitive (not constructed from other
 *    forms), and the ID code value means the actual ID code value is
 *    extended into one or more octets.
 *

	      **  S U P E R S E D E D   I N   1 . 2  **

 * The second and third bytes define the actual ID code value.  The
 * value used is the original XUI value.
 *     second byte:  (0xff)
 *               1       1111111
 *              MSB      high seven bits of ID code
 *
 *     third byte:   (0x79)
 *               0       1111001
 *              LSB      low seven bits of ID code
 *

		    **  N E W   F O R   1 . 2  **

 * The second and third bytes define the actual ID code value.  The
 * value used for 1.2 is the inverse of the original XUI value.
 *     second byte:  (0x80)
 *               1       0000000
 *              MSB      high seven bits of ID code
 *
 *     third byte:   (0x06)
 *               0       0000110
 *              LSB      low seven bits of ID code
 *

 * The length field of the ASN.1 conformant compound string header
 * is dynamically constructed.  There are two possible forms depending
 * upon the length of the string.  Note that this length excludes the
 * header bytes.
 *
 *    Short Form: range 0 .. 127
 *    one byte
 *                  0         nnnnnnn
 *                 short       7 bit length
 *
 *    Long Form: range 128 .. 2**32-1
 *    three bytes
 *    first:        1         nnnnnnn
 *                 long       number of bytes to follow
 *
 *    second:
 *                  nnnnnnnn
 *                  MSB of length
 *
 *    third:
 *                  nnnnnnnn
 *                  LSB of length
 *

 * In 1.2, this process for constructing the length field will also be
 * used to construct the length field within individual tag-length-value
 * triplets. 
 */

#define CSHEADERLEN     3

#define CSHEADER1       0xdf
#define CSHEADER2       0xff
#define CSHEADER3       0x79
static unsigned char CSHeader[3] = {CSHEADER1, CSHEADER2, CSHEADER3};


#define ASNHEADERLEN     3

#define ASNHEADER1	0xdf
#define ASNHEADER2	0x80
#define ASNHEADER3	0x06
static unsigned char 	ASNHeader[3] = {ASNHEADER1, ASNHEADER2, ASNHEADER3}; 
  

#define MAXSHORTVALUE   127             /* maximum len to be used for short 
                                           length form */
#define CSLONGLEN       3
#define CSSHORTLEN      1
#define CSLONGLEN1      0x82
#define CSLONGBIT	0x80

#define ASNTAG		1
/* Num bytes for tag & length = ASNTAG + [CSSHORTLEN | CSLONGLEN] */

#define HEADER 3	/* num bytes for tag & length */


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Boolean _is_short_length() ;
static void _write_long_length() ;
static unsigned char * _write_header() ;
static unsigned char * _read_header() ;
static unsigned short _read_header_length() ;
static unsigned short _read_length();
static unsigned short _read_string_length() ;
static unsigned char * _write_component() ;
static unsigned char * _read_component() ;
static unsigned short _read_component_length() ;
static unsigned char * _copy_short_to_long() ;
static int _index_cache_charset() ;
static char * _cache_charset() ;
static void _cache_fontlist() ;
static Boolean FontListSearch() ;
static Boolean _is_compound() ;
static Boolean _is_asn1() ;
static void _XmStringOptLineExtent() ;
static void _XmStringLineExtent() ;
static Dimension _XmStringLineWidth(); 
static void _XmStringOptLineMetrics(); 
static Dimension _XmStringOptLineAscender() ;
static Dimension _XmStringFirstLineAscender() ;
static Dimension _XmStringLineAscender() ;
static Dimension _XmStringLineDescender() ;
static void _XmStringSubStringPosition() ;
static void _XmStringDrawSegment() ;
static void _XmStringDrawLine() ;
static void _calc_align_and_clip() ;
static void _draw() ;
static void new_segment() ;
static void new_line() ;
static _XmString _XmStringOptCreate() ;
static _XmString _XmStringNonOptCreate() ;
static void _update_opt() ;
static void _update_segment() ;
static void _clear_segment() ;
static void _clear_opt() ;
static void _parse_locale() ;

#else

static Boolean _is_short_length( 
                        unsigned char *p) ;
static void _write_long_length( 
                        unsigned char *p,
#if NeedWidePrototypes
                        unsigned int length) ;
#else
                        unsigned short length) ;
#endif /* NeedWidePrototypes */
static unsigned char * _write_header( 
                        unsigned char *p,
#if NeedWidePrototypes
                        unsigned int length) ;
#else
                        unsigned short length) ;
#endif /* NeedWidePrototypes */
static unsigned char * _read_header( 
                        unsigned char *p) ;
static unsigned short _read_header_length( 
                        unsigned char *p) ;
static unsigned short _read_length( 
                        unsigned char *p) ;
static unsigned short _read_string_length( 
                        unsigned char *p) ;
static unsigned char * _write_component( 
                        unsigned char *p,
#if NeedWidePrototypes
                        unsigned int tag,
                        unsigned int length,
#else
                        unsigned char tag,
                        unsigned short length,
#endif /* NeedWidePrototypes */
                        unsigned char *value,
#if NeedWidePrototypes
                        int move_by_length) ;
#else
                        Boolean move_by_length) ;
#endif /* NeedWidePrototypes */
static unsigned char * _read_component( 
                        unsigned char *p,
                        unsigned char *tag,
                        unsigned short *length,
                        unsigned char *value,
#if NeedWidePrototypes
			int asn1) ;
#else
                        Boolean asn1);
#endif /* NeedWidePrototypes */
static unsigned short _read_component_length( 
                        unsigned char *p) ;
static unsigned char * _copy_short_to_long( 
                        unsigned char *p) ;
static int _index_cache_charset( 
                        char *charset,
                        int length) ;
static char * _cache_charset( 
                        char *charset,
                        int length) ;
static void _cache_fontlist( 
                        XmFontList f) ;
static Boolean FontListSearch( 
                        XmFontList fontlist,
                        XmStringCharSet charset,
#if NeedWidePrototypes
                        int cached_charset,
#else
                        Boolean cached_charset,
#endif /* NeedWidePrototypes */
                        short *indx,
                        XmFontListEntry *entry) ;
static Boolean _is_compound( 
                        XmString string) ;
static Boolean _is_asn1( 
                        XmString string) ;
static void 	_XmStringOptLineExtent(
				       XmFontList fontlist,
				       _XmStringOpt optline,
				       Dimension *width,
				       Dimension *height );
static void 	_XmStringLineExtent(
				    XmFontList fontlist,
				    _XmStringLine line,
				    Dimension *width,
				    Dimension *height,
				    int	which);
static Dimension	_XmStringLineWidth(XmFontList fontlist,
					   _XmStringLine line);
static void	_XmStringOptLineMetrics( XmFontList fontlist,
					_XmStringOpt line,
					Dimension *ascender,
					Dimension *descender );
static Dimension _XmStringOptLineAscender( 
                        XmFontList f,
                        _XmStringOpt opt) ;
static Dimension _XmStringFirstLineAscender( 
                        XmFontList f,
                        _XmStringLine line) ;
static Dimension _XmStringLineAscender( 
                        XmFontList f,
                        _XmStringLine line) ;
static Dimension _XmStringLineDescender( 
                        XmFontList f,
                        _XmStringLine line) ;
static void _XmStringSubStringPosition( 
#if NeedWidePrototypes
                        int one_byte,
#else
                        Boolean one_byte,
#endif /* NeedWidePrototypes */
                        XmFontListEntry entry,
                        _XmStringSegment seg,
                        _XmStringSegment under_seg,
#if NeedWidePrototypes
                        int x,
#else
                        Position x,
#endif /* NeedWidePrototypes */
                        Dimension *under_begin,
                        Dimension *under_end) ;
static void _XmStringDrawSegment( 
                        Display *d,
                        Window w,
                        int x,
                        int y,
                        _XmStringSegment seg,
                        GC gc,
                        XmFontList fontlist,
#if NeedWidePrototypes
                        int image,
#else
                        Boolean image,
#endif /* NeedWidePrototypes */
                        _XmString underline,
                        Dimension *under_begin,
                        Dimension *under_end) ;
static void _XmStringDrawLine( 
                        Display *d,
                        Window w,
                        int x,
                        int y,
                        _XmStringLine line,
                        GC gc,
                        XmFontList fontlist,
#if NeedWidePrototypes
                        int image,
#else
                        Boolean image,
#endif /* NeedWidePrototypes */
                        _XmString underline,
                        Dimension *under_begin,
                        Dimension *under_end,
#if NeedWidePrototypes
                        int opt) ;
#else
                        Boolean opt) ;
#endif /* NeedWidePrototypes */
static void _calc_align_and_clip( 
                        Display *d,
                        GC gc,
                        Position *x,
#if NeedWidePrototypes
                        int y,
                        int width,
#else
                        Position y,
                        Dimension width,
#endif /* NeedWidePrototypes */
                        int line_width,
#if NeedWidePrototypes
                        unsigned int lay_dir,
#else
                        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
                        XRectangle *clip,
#if NeedWidePrototypes
                        unsigned int align,
#else
                        unsigned char align,
#endif /* NeedWidePrototypes */
                        int descender,
                        int *restore) ;
static void _draw( 
                        Display *d,
                        Window w,
                        XmFontList fontlist,
                        _XmString string,
                        GC gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        unsigned int align,
                        unsigned int lay_dir,
#else
                        Position x,
                        Position y,
                        Dimension width,
                        unsigned char align,
                        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
                        XRectangle *clip,
#if NeedWidePrototypes
                        int image,
#else
                        Boolean image,
#endif /* NeedWidePrototypes */
                        _XmString underline) ;
static void new_segment( 
                        _XmString string,
                        int line_index,
                        _XmStringSegment value) ;
static void new_line( 
                        _XmString string) ;
static _XmString _XmStringOptCreate( 
                        unsigned char *c,
                        unsigned char *end,
#if NeedWidePrototypes
                        unsigned int textlen,
                        int havecharset,
#else
                        unsigned short textlen,
                        Boolean havecharset,
#endif /* NeedWidePrototypes */
                        unsigned int charset_index) ;
static _XmString _XmStringNonOptCreate( 
                        unsigned char *c,
                        unsigned char *end,
#if NeedWidePrototypes
                        int havecharset) ;
#else
                        Boolean havecharset) ;
#endif /* NeedWidePrototypes */
static void _update_opt( 
                        XmFontList fontlist,
                        _XmStringOpt optline,
                        XmFontListEntry entry) ;
static void _update_segment( 
                        XmFontList fontlist,
                        _XmStringSegment seg) ;
static void _clear_segment( 
                        XmFontList fontlist,
                        _XmStringSegment seg) ;
static void _clear_opt( 
                        XmFontList fontlist,
                        _XmStringOpt opt) ;
static void _parse_locale( 
                        char *str,
                        int *indx,
                        int *len) ;
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static struct __Xmlocale locale;
static char **_charset_cache = NULL;
static int    _cache_count = 0;
static FontlistEntry *_fontlist_cache = NULL;


/*
 * Determines whether this string has a short or long length field
 */
static Boolean 
#ifdef _NO_PROTO
_is_short_length( p )
        unsigned char *p ;
#else
_is_short_length(
        unsigned char *p )
#endif /* _NO_PROTO */
{

    unsigned char *uchar_p = (unsigned char *) p;

    uchar_p += ASNHEADERLEN;

    if (*uchar_p & (char)CSLONGBIT)
       return (FALSE);
    else return (TRUE);
}

/*
 * Routine that writes a long length field
 */
static void 
#ifdef _NO_PROTO
_write_long_length( p, length )
        unsigned char *p ;
        unsigned short length ;
#else
_write_long_length(
        unsigned char *p,
#if NeedWidePrototypes
        unsigned int length )
#else
        unsigned short length )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{

    unsigned char   * uchar_p = (unsigned char *) p;

    /* 
     * flag the long version
     */
    *uchar_p = CSLONGLEN1;
    uchar_p++;

    /* 
     * need to pull off the high 8 bits 
     */

    *uchar_p = (unsigned char) (length >> 8);
    uchar_p++;
    *uchar_p = (unsigned char) (length & 0xff);

}
	

/*
 * Private routines for manipulating the ASN.1 header of external
 * compound strings.
 */
static unsigned char * 
#ifdef _NO_PROTO
_write_header( p, length )
        unsigned char *p ;
        unsigned short length ;
#else
_write_header(
        unsigned char *p,
#if NeedWidePrototypes
        unsigned int length )
#else
        unsigned short length )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{

    unsigned char * uchar_p = p;
    int     headlen;

    /* write the header in. */

    headlen = ASNHEADERLEN;
    memcpy( uchar_p, ASNHeader, ASNHEADERLEN);
    uchar_p += ASNHEADERLEN;

    /* short or long length value? */
 
    if (length > MAXSHORTVALUE)
    {
        _write_long_length(uchar_p, length);
        headlen += CSLONGLEN;
    }
    else {
        /* Short version */
        *uchar_p = (unsigned char) length;
        headlen += CSSHORTLEN;
    }
    return (p + headlen);
}

/*
 * extracts the ASN.1 header from the external compound string.
 */
static unsigned char * 
#ifdef _NO_PROTO
_read_header( p )
        unsigned char *p ;
#else
_read_header(
        unsigned char *p )
#endif /* _NO_PROTO */
{
    /*
     * Read past the ASN.1 header; get the first length byte and see if this
     * is a one or three byte length.
     */

    if (_is_short_length(p))
        return (p + ASNHEADERLEN + CSSHORTLEN);
    else
       return (p + ASNHEADERLEN + CSLONGLEN); 
}

/*
 * reads the length the ASN.1 header of an external
 * compound string.
 */
static unsigned short 
#ifdef _NO_PROTO
_read_header_length( p )
        unsigned char *p ;
#else
_read_header_length(
        unsigned char *p )
#endif /* _NO_PROTO */
{
    /*
     * Read past the ASN.1 header; get the first length byte and see if this
     * is a one or three byte length.
     */

    if (_is_short_length(p))
       return (ASNHEADERLEN + CSSHORTLEN);
    else
       return (ASNHEADERLEN + CSLONGLEN);

}

/*
 * calculates the length of the external compound string, excluding the
 * ASN.1 header.
 */
static unsigned short 
#ifdef _NO_PROTO
_read_string_length( p )
        unsigned char *p ;
#else
_read_string_length(
        unsigned char *p )
#endif /* _NO_PROTO */
{


    unsigned char * uchar_p = (unsigned char *) p;
    unsigned short totallen = 0;

    /*
     * Read past the ASN.1 header; get the first length byte and see if this
     * is a one or three byte length.
     */

    uchar_p += ASNHEADERLEN;

    if (_is_short_length(p))
    {
       totallen += (unsigned short) *uchar_p;
    }
    else {
       unsigned short i;

       uchar_p++;
       i = ((unsigned short) *uchar_p) << 8;
       uchar_p++;
       i |= ((unsigned short) *uchar_p);    /* Mask on the low byte */
       totallen += i;
    }
    return (totallen);
}

/*
 * calculates length of component marked by a tag-length-value triple.
 */
static unsigned short 
#ifdef _NO_PROTO
_read_asn1_length( p )
        unsigned char *p ;
#else
_read_asn1_length(
        unsigned char *p )
#endif /* _NO_PROTO */
{
    unsigned char * uchar_p = (unsigned char *) p;
    unsigned short totallen = 0;

    /*
     * Read past the tag; get the first length byte and see if this
     * is a one or three byte length.
     */

    uchar_p += ASNTAG;

    if (_is_asn1_long(p))
      {
	unsigned short i;

	uchar_p++;
	i = ((unsigned short) *uchar_p) << 8;
	uchar_p++;
	i |= ((unsigned short) *uchar_p); /* Mask on the low byte */
	totallen += i;
      }
    else 
      {
	totallen += (unsigned short) *uchar_p;
      }
    return (totallen);
}

/*
 * determines length of ASN.1 length field of component of external 
 * compound string.
 */
static unsigned short 
#ifdef _NO_PROTO
_read_length( p )
        unsigned char *p ;
#else
_read_length(
        unsigned char *p )
#endif /* _NO_PROTO */
{
    /*
     * Read past the tag field; get the first length byte and see if this
     * is a one or three byte length.
     */
    if (_is_asn1_long(p))
       return ((unsigned short)(ASNTAG + CSLONGLEN));
    else
       return ((unsigned short)(ASNTAG + CSSHORTLEN));
}

/*
 * Private routines for reading/writing the individual compound string
 * components
 */
static unsigned char * 
#ifdef _NO_PROTO
_write_component( p, tag, length, value, move_by_length )
        unsigned char *p ;
        unsigned char tag ;
        unsigned short length ;
        unsigned char *value ;
        Boolean move_by_length ;
#else
_write_component(
        unsigned char *p,
#if NeedWidePrototypes
        unsigned int tag,
        unsigned int length,
#else
        unsigned char tag,
        unsigned short length,
#endif /* NeedWidePrototypes */
        unsigned char *value,
#if NeedWidePrototypes
        int move_by_length )
#else
        Boolean move_by_length )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    unsigned char * uchar_p = p;

    *uchar_p = tag;				/* stuff tag */
    uchar_p += ASNTAG;
    
    /* short or long length value? */
    if (length > MAXSHORTVALUE)
      {
	_write_long_length(uchar_p, length);
        uchar_p += CSLONGLEN;
      }
    else {
      /* Short version */
      *uchar_p = (unsigned char) length;
      uchar_p += CSSHORTLEN;
    }

    if (value != (unsigned char *) NULL)
      memcpy((char *)uchar_p, (char *)value, (int)length);
    
    if (move_by_length)
	return (uchar_p + length);
    else
	return (uchar_p);
}

static unsigned char * 
#ifdef _NO_PROTO
_read_component( p, tag, length, value, asn1 )
        unsigned char *p ;
        unsigned char *tag ;
        unsigned short *length ;
        unsigned char *value ;
        Boolean asn1;
#else
_read_component(
        unsigned char *p,
        unsigned char *tag,
        unsigned short *length,
        unsigned char *value,
#if NeedWidePrototypes
	int asn1)
#else
        Boolean asn1)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    unsigned char * uchar_p = (unsigned char *) p;
    unsigned char * short_p = (unsigned char *) (p + 1);
    unsigned short s;
    int k;

    *tag = *uchar_p;				/* read tag */

    if (!asn1)
      {
	uchar_p += HEADER;	/* move to value */

	ReadShort(short_p, s);
	WriteShort(length, s);

	if (value != NULL) for (k=0; k<s; k++) *value++ = *uchar_p++;

	return (p + s + HEADER);
      }
    else
      {
	*length = _read_asn1_length(p);
	
	uchar_p += _read_length(p); /* move to value */
	if (value != NULL) memcpy(value, uchar_p, *length);
	
	return (uchar_p + *length);
      }
  }

static unsigned short 
#ifdef _NO_PROTO
_read_component_length( p )
        unsigned char *p ;
#else
_read_component_length(
        unsigned char *p )
#endif /* _NO_PROTO */
{
    unsigned char * short_p = (unsigned char *) (p + 1);
    unsigned short s;

    ReadShort(short_p, s)
    return (s);				/* read length */
}

/*
 * Routine to copy a short header length string to a long header
 * length version.  Used by concatenates.  The original string
 * is freed.
 */
static unsigned char * 
#ifdef _NO_PROTO
_copy_short_to_long( p )
        unsigned char *p ;
#else
_copy_short_to_long(
        unsigned char *p )
#endif /* _NO_PROTO */
{

    unsigned char    * uchar_p = (unsigned char *) p;
    unsigned char    * q;
    unsigned char    * newstring;
    unsigned short   len;
    

    len = _read_string_length(p) + ASNHEADERLEN + CSLONGLEN;

    newstring = (unsigned char *) XtMalloc(len);
    q = newstring;

    q = _write_header(q, (unsigned short) MAXSHORTVALUE+1);
    uchar_p = _read_header(uchar_p);
    len = _read_string_length(p);
    memcpy( q, uchar_p, len);

    q = newstring + ASNHEADERLEN;
    _write_long_length(q, len);
    return (newstring);
}

/*
 * build an external TCS text component
 */
XmString 
#ifdef _NO_PROTO
XmStringCreate( text, charset )
        char *text ;
        XmStringCharSet charset ;
#else
XmStringCreate(
        char *text,
        XmStringCharSet charset )
#endif /* _NO_PROTO */
{
    unsigned char    *p;
    XmString string;
    char     *curcharset = NULL; 
    int      t_length;
    int      c_length = 0;
    Boolean  is_local = FALSE;
    int      i;
    short    head_size;
    
    if (!text) return ((XmString) NULL);
    if (!charset) return ((XmString) NULL);    
#ifdef IBM_MOTIF
    if(!strcmp(charset, "Xm_FONTLIST_DEFAULT_TAG_STRING"))
        charset = XmFONTLIST_DEFAULT_TAG;
#endif /* IBM_MOTIF */

    t_length = strlen (text);

    if ((charset == XmFONTLIST_DEFAULT_TAG) || 
	(strcmp(charset, XmFONTLIST_DEFAULT_TAG) == 0))
      is_local = TRUE; 
    else {
      if ((strcmp(charset, XmSTRING_DEFAULT_CHARSET) == 0))
	curcharset = _XmStringGetCurrentCharset();
      else curcharset = charset;
      c_length = strlen(curcharset);
    }
    
    /* 
     * Add up the number of bytes to allocate.  The header length field size
     * is calculated from the number of bytes in the string.
     */

    if (is_local) i = t_length + _asn1_size(t_length);
    else i = t_length + _asn1_size(t_length) + c_length + _asn1_size(c_length);

    head_size = _calc_header_size((unsigned short)i);  
    i += head_size;

    string = (XmString) XtMalloc (i);
    p = (unsigned char *) string;

    p = _write_header (p, (unsigned short)(i - head_size));

    if (is_local)
      p = _write_component(p, (unsigned char) XmSTRING_COMPONENT_LOCALE_TEXT,
                         (unsigned short) t_length, (unsigned char *) text,
                         TRUE);
    else 
      {
      p = _write_component (p, (unsigned char) XmSTRING_COMPONENT_CHARSET, 
                            (unsigned short) c_length, 
                            (unsigned char *) curcharset, TRUE);
  
      p = _write_component (p, (unsigned char) XmSTRING_COMPONENT_TEXT, 
                            (unsigned short) t_length,
                            (unsigned char *) text, TRUE);
      }
    
    return (string);
}

/*
 * Convenience routine to create an XmString from a NULL terminated string.
 */
XmString 
#ifdef _NO_PROTO
XmStringCreateSimple( text )
        char *text ;
#else
XmStringCreateSimple(
        char *text )
#endif /* _NO_PROTO */
{
  return (XmStringCreate(text, XmSTRING_DEFAULT_CHARSET));
 }

/*
 * Convenience routine creating localized XmString from NULL terminated string.
 */
XmString 
#ifdef _NO_PROTO
XmStringCreateLocalized( text )
        String text ;
#else
XmStringCreateLocalized(
        String text )
#endif /* _NO_PROTO */
{
     char *c;
     char *start, *end;
     Boolean done;
     XmString string, next, tmp;
 
     if (!text) return (NULL);
 
     /* Copy the string because '\n' will be bashed. */
     c = (char *) strcpy ((char *) ALLOCATE_LOCAL (strlen (text) + 1), text);
     start = c;
     done = FALSE;
     string = NULL;
     
     while ( ! done)                           /* loop thu local copy */
     {                                         /* looking for \n */
       end = start;
 
       while ((*end != '\0') && (*end != '\n'))  end++;
 
       if (*end == '\n')                       /* replace \n with \0 */
           *end = '\0';
       else
           done = TRUE;                        /* we are at the end */
 
       /* Don't convert empty string unless it's an initial newline. */
       /* Done so StringHeight has clue to size of empty lines. */
       if ((start != end) || (start == c)) {
         if (string == NULL)  {
             string = XmStringCreate(start, XmFONTLIST_DEFAULT_TAG);
             }
         else {
             next = XmStringCreate(start, XmFONTLIST_DEFAULT_TAG);
             tmp = string;
             string = XmStringConcat(tmp, next);
       
             XmStringFree(tmp);
             XmStringFree(next);
             }
         }
       
       /* Make a separator if this isn't the last segment. */
       if (!done) {
         if (string == NULL)  {
             string = XmStringSeparatorCreate();
             }
         else {
             next = XmStringSeparatorCreate();
             tmp = string;
             string = XmStringConcat(tmp, next);
           
             XmStringFree(tmp);
             XmStringFree(next);
             }
 
       start = ++end;                          /* start at next char */
       }
     }
 
     DEALLOCATE_LOCAL (c);
     return(string);
}

/*
 * build an external TCS direction component
 */
XmString 
#ifdef _NO_PROTO
XmStringDirectionCreate( direction )
        XmStringDirection direction ;
#else
XmStringDirectionCreate(
#if NeedWidePrototypes
        int direction )
#else
        XmStringDirection direction )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmString string;
    unsigned char    *p;
    int      i;
    short    head_size;
    /* ANSI - direction needs to be in the lowest significant byte */
    unsigned char value = (unsigned char)direction;

    i = ASNTAG + CSSHORTLEN + 1; /* direction header and value */
    head_size = _calc_header_size((unsigned short) i);
    i += head_size;

    string = (XmString) XtMalloc (i);
    p = (unsigned char *) string;

    p = _write_header (p, (unsigned short)(i - head_size));

    p = _write_component (p, (unsigned char) XmSTRING_COMPONENT_DIRECTION, 
                                             (unsigned short) 1, &value, TRUE);
    return (string);
}

/*
 * build an external TCS separator component
 */
XmString 
#ifdef _NO_PROTO
XmStringSeparatorCreate()
#else
XmStringSeparatorCreate( void )
#endif /* _NO_PROTO */
{
    XmString string;
    unsigned char    *p;
    int      i;
    short    head_size;

    i = ASNTAG + CSSHORTLEN;
    head_size = _calc_header_size((unsigned short) i);
    i += head_size;

    string = (XmString) XtMalloc (i);
    p = (unsigned char *) string;

    p = _write_header (p, (unsigned short) (i - head_size));

    p = _write_component (p, (unsigned char) XmSTRING_COMPONENT_SEPARATOR, 
			  0, NULL, TRUE);

    return (string);
}

/*
 * build an external TCS 'segment', just a high level create
 */
XmString 
#ifdef _NO_PROTO
XmStringSegmentCreate( text, charset, direction, separator )
        char *text ;
        XmStringCharSet charset ;
        XmStringDirection direction ;
        Boolean separator ;
#else
XmStringSegmentCreate(
        char *text,
        XmStringCharSet charset,
#if NeedWidePrototypes
        int direction,
        int separator )
#else
        XmStringDirection direction,
        Boolean separator )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmString a, b, c;

    a = XmStringDirectionCreate (direction);
    b = XmStringCreate (text, charset);

    c = XmStringConcat (a, b);

    XmStringFree (a);
    XmStringFree (b);

    if (separator)
    {
	a = XmStringSeparatorCreate ();

	b = XmStringConcat (c, a);

	XmStringFree (a);
	XmStringFree (c);

	c = b;
    }

    return (c);
}

/*
 * as close as we can come to Latin1Create without knowing the charset of
 * Latin1.  This imposes the semantic of \n meaning separator.
 */
XmString 
#ifdef _NO_PROTO
XmStringLtoRCreate( text, charset )
        char *text ;
        XmStringCharSet charset ;
#else
XmStringLtoRCreate(
        char *text,
        XmStringCharSet charset )
#endif /* _NO_PROTO */
{
    char *c;
    char *start, *end;
    Boolean done;
    XmString string, next, tmp;

    if (!text) return (NULL);

    /* Copy the string because '\n' will be bashed. */
#ifndef OSF_v1_2_4
    c = (char *) strcpy ((char *) ALLOCATE_LOCAL (strlen (text) + 1), text);
#else /* OSF_v1_2_4 */
/* BEGIN OSF Fix CR 5413 */
    c = (char *)ALLOCATE_LOCAL(strlen(text) + 1);
    (void)strcpy(c, text);
/* END OSF Fix CR 5413 */
#endif /* OSF_v1_2_4 */
    start = c;
    done = FALSE;

    /* Set the direction once only at the beginning. */
    string = XmStringDirectionCreate(XmSTRING_DIRECTION_L_TO_R);
    
    while ( ! done)				/* loop thu local copy */
    {						/* looking for \n */
	end = start;

	while ((*end != '\0') && (*end != '\n'))  end++;

	if (*end == '\n')			/* replace \n with \0 */
	    *end = '\0';
	else
	    done = TRUE;			/* we are at the end */

      /* Don't convert empty string unless it's an initial newline. */
      /* Done so StringHeight has clue to size of empty lines. */
      if ((start != end) || (start == c)) {
        next = XmStringCreate(start, charset);
        tmp = string;
        string = XmStringConcat(tmp, next);
      
        XmStringFree(tmp);
        XmStringFree(next);
        }
      
      /* Make a separator if this isn't the last segment. */
      if (!done) {
        next = XmStringSeparatorCreate();
        tmp = string;
        string = XmStringConcat(tmp, next);
      
        XmStringFree(tmp);
        XmStringFree(next);

	start = ++end;				/* start at next char */
      }
    }

    DEALLOCATE_LOCAL (c);
    return (string);
}

XmString 
#ifdef _NO_PROTO
XmStringCreateLtoR( text, charset )
        char *text ;
        XmStringCharSet charset ;
#else
XmStringCreateLtoR(
        char *text,
        XmStringCharSet charset )
#endif /* _NO_PROTO */
{
#ifdef IBM_MOTIF
    if(charset && !strcmp(charset, "Xm_FONTLIST_DEFAULT_TAG_STRING"))
        charset = XmFONTLIST_DEFAULT_TAG;
#endif /* IBM_MOTIF */
    return (XmStringLtoRCreate (text, charset));
}
/*
 * this set provides access to the external TCS
 */

/*
 * set up the read-out context
 */
Boolean 
#ifdef _NO_PROTO
XmStringInitContext( context, string )
        XmStringContext *context ;
        XmString string ;
#else
XmStringInitContext(
        XmStringContext *context,
        XmString string )
#endif /* _NO_PROTO */
{
    XmStringContext q;
    
    if (!string) return(FALSE);
    
    q = (XmStringContext) XtMalloc (sizeof (XmStringContextRec));

    /*
     * read past header
     */

    XmStrContOffset(q)     = _read_header_length((unsigned char *)string);
    XmStrContLength(q)     = _read_string_length((unsigned char *)string);
    XmStrContCharSet(q)    = NULL;
    XmStrContCharSetLen(q) = 0;
    XmStrContDirection(q)  = XmSTRING_DIRECTION_L_TO_R;
    XmStrContError(q)      = FALSE;

    XmStrContString(q)     = string;
    
    *context = q;

    XmStrContError(q)      = !_XmStringIsXmString(string);

    return ( ! XmStrContError(q));
}

void 
#ifdef _NO_PROTO
XmStringFreeContext( context )
        XmStringContext context ;
#else
XmStringFreeContext(
        XmStringContext context )
#endif /* _NO_PROTO */
{
    XtFree ((char *) context);
}

/*
 * fetch the next component of the external TCS
 */
XmStringComponentType 
#ifdef _NO_PROTO
XmStringGetNextComponent( context, text, charset, direction, unknown_tag, unknown_length, unknown_value )
        XmStringContext context ;
        char **text ;
        XmStringCharSet *charset ;
        XmStringDirection *direction ;
        XmStringComponentType *unknown_tag ;
        unsigned short *unknown_length ;
        unsigned char **unknown_value ;
#else
XmStringGetNextComponent(
        XmStringContext context,
        char **text,
        XmStringCharSet *charset,
        XmStringDirection *direction,
        XmStringComponentType *unknown_tag,
        unsigned short *unknown_length,
        unsigned char **unknown_value )
#endif /* _NO_PROTO */
{
    Boolean asn1 = XmStrContASN1(context);
    unsigned char *p = ((unsigned char *) XmStrContString(context)) + 
      XmStrContOffset(context);
    XmStringComponentType answer;
    unsigned char tag;
    unsigned short length, offset;
    
    if ((XmStrContError(context)) || 
        (XmStrContOffset(context) >= 
            (XmStrContLength(context) + 
                 _read_header_length((unsigned char *)XmStrContString(context)))))
       return (XmSTRING_COMPONENT_END);
    
    length = asn1 ? _read_asn1_length(p) : _read_component_length (p);

    switch (*p)
      {
      case XmSTRING_COMPONENT_CHARSET:
      XmStrContCharSetLen(context) = length; /* update context */
      offset = asn1 ? _asn1_size(length) : HEADER;
      XmStrContCharSet(context) = (char *) (p + offset);

      *charset  = (XmStringCharSet) XtMalloc (length + 1);
      p = _read_component (p, &tag, &length, (unsigned char *)*charset, asn1);
      (*charset)[length] = '\0';
      answer = (XmStringComponentType) XmSTRING_COMPONENT_CHARSET;
      break;

      case XmSTRING_COMPONENT_TEXT:
      *text  = (char *) XtMalloc (length + 1);
      p = _read_component (p, &tag, &length, (unsigned char *)*text, asn1);
      (*text)[length] = '\0';
      answer = (XmStringComponentType) XmSTRING_COMPONENT_TEXT;
      break;

      case XmSTRING_COMPONENT_LOCALE_TEXT:
      /* Update context. */
      XmStrContCharSetLen(context) = strlen(XmFONTLIST_DEFAULT_TAG);
      XmStrContCharSet(context) = XmFONTLIST_DEFAULT_TAG;
      
      /* Return text component. */
      *text = (char *) XtMalloc(length + 1);
      p = _read_component(p, &tag, &length, (unsigned char *)*text, asn1);
      (*text)[length] = '\0';
      answer = (XmStringComponentType) XmSTRING_COMPONENT_LOCALE_TEXT;
      break;

      case XmSTRING_COMPONENT_DIRECTION:
      p = _read_component (p, &tag, &length, direction, asn1);

      XmStrContDirection(context) = *direction;
      answer = (XmStringComponentType) XmSTRING_COMPONENT_DIRECTION;
      break;

      case XmSTRING_COMPONENT_SEPARATOR:
      p = _read_component (p, &tag, &length, NULL, asn1);
      answer = (XmStringComponentType) XmSTRING_COMPONENT_SEPARATOR;
      break;

      default:
      *unknown_tag = (XmStringComponentType) *p;
      *unknown_length = length;
      *unknown_value = (unsigned char *) XtMalloc (length + 1);
      p = _read_component (p, &tag, &length, *unknown_value, asn1);
      answer = (XmStringComponentType) XmSTRING_COMPONENT_UNKNOWN;
      break;
      }

    offset = asn1 ? _asn1_size(length) : HEADER;
    
    XmStrContOffset(context) += length + offset;

    return (answer);
}

XmStringComponentType 
#ifdef _NO_PROTO
XmStringPeekNextComponent( context )
        XmStringContext context ;
#else
XmStringPeekNextComponent(
        XmStringContext context )
#endif /* _NO_PROTO */
{
    unsigned char *p = ((unsigned char *) XmStrContString(context)) + XmStrContOffset(context);
    XmStringComponentType answer;

    if ((XmStrContError(context)) ||
        (XmStrContOffset(context) >= 
            (XmStrContLength(context) + 
                 _read_header_length((unsigned char *)XmStrContString(context)))))
       return (XmSTRING_COMPONENT_END);

    switch (*p)
    {
	case XmSTRING_COMPONENT_CHARSET:
	case XmSTRING_COMPONENT_TEXT:
        case XmSTRING_COMPONENT_LOCALE_TEXT:
	case XmSTRING_COMPONENT_DIRECTION:
	case XmSTRING_COMPONENT_SEPARATOR:
	    answer = (XmStringComponentType) *p;
	    break;

	default:
	    answer = (XmStringComponentType) XmSTRING_COMPONENT_UNKNOWN;
	    break;
    }
    return (answer);
}

/*
 * fetch the next 'segment' of the external TCS
 */
Boolean 
#ifdef _NO_PROTO
XmStringGetNextSegment( context, text, charset, direction, separator )
        XmStringContext context ;
        char **text ;
        XmStringCharSet *charset ;
        XmStringDirection *direction ;
        Boolean *separator ;
#else
XmStringGetNextSegment(
        XmStringContext context,
        char **text,
        XmStringCharSet *charset,
        XmStringDirection *direction,
        Boolean *separator )
#endif /* _NO_PROTO */
{
    XmStringComponentType j;
    unsigned char  *jv;
    unsigned short jl;
    char *t;
    XmStringCharSet	 c_set = NULL;
    XmStringDirection	 dir = XmSTRING_DIRECTION_L_TO_R;

    *text      = NULL;				/* pre-condition results */
    *charset   = NULL;
    *direction = XmStrContDirection(context);
    *separator = FALSE;

    if (XmStrContError(context)) return (FALSE);

    while (TRUE)				/* return directly */
    {
     	switch (XmStringGetNextComponent (context, &t, &c_set, &dir, 
		&j, &jl, &jv))
	{
	   case XmSTRING_COMPONENT_DIRECTION:	/* just record these */
                *direction = dir;
                break;

	   case XmSTRING_COMPONENT_SEPARATOR:
		break;

	   case XmSTRING_COMPONENT_UNKNOWN:	/* skip these */
		XtFree ((char *) jv);
		break;

	   case XmSTRING_COMPONENT_CHARSET:	/* careful of memory leak */
		if (c_set != NULL) XtFree (c_set);
		break;

	   case XmSTRING_COMPONENT_TEXT:
           case XmSTRING_COMPONENT_LOCALE_TEXT: /* at last */
		*text = t;
		*separator = (XmStringPeekNextComponent( context)
                                             == XmSTRING_COMPONENT_SEPARATOR) ;
		*charset = (XmStringCharSet) XtMalloc(
                                           XmStrContCharSetLen( context) + 1) ;
                memcpy( *charset, XmStrContCharSet( context),
                                               XmStrContCharSetLen( context)) ;
		(*charset)[XmStrContCharSetLen( context)] = '\0' ;

		return( TRUE && !XmStrContError( context)) ;
		/* break ; */

	    case XmSTRING_COMPONENT_END:	/* no more */
		/* no break */

	    default:
		return (FALSE);
		/* break; */
	}
    }
}

/*
 * fetch the first text 'segment' of the external TCS that matches the given
 * char set.
 */
Boolean 
#ifdef _NO_PROTO
XmStringGetLtoR( string, charset, text )
        XmString string ;
        XmStringCharSet charset ;
        char **text ;
#else
XmStringGetLtoR(
        XmString string,
        XmStringCharSet charset,
        char **text )
#endif /* _NO_PROTO */
{
    XmStringContext context;
    char * t;
    XmStringCharSet c, curcharset = NULL; 
    XmStringDirection d;
    Boolean s, is_local = FALSE, done = FALSE, is_default = FALSE;

    if (!string) return(FALSE);
    if (!charset) return (FALSE);
#ifdef IBM_MOTIF
    if(!strcmp(charset, "Xm_FONTLIST_DEFAULT_TAG_STRING"))
        charset = XmFONTLIST_DEFAULT_TAG;
#endif /* IBM_MOTIF */

    if ((charset == XmFONTLIST_DEFAULT_TAG) || 
	(strcmp(charset, XmFONTLIST_DEFAULT_TAG) == 0))
      is_local = TRUE; 

    *text = NULL;				/* pre-condition result */

    if (!is_local)
      {
      if ((strcmp(charset, XmSTRING_DEFAULT_CHARSET) == 0))
        {
          curcharset = _XmStringGetCurrentCharset();
          is_default = TRUE;
        }
      else curcharset = charset;
      }
    
    XmStringInitContext (&context, string);

    while ( ! done)
    {
	if (XmStringGetNextSegment (context, &t, &c, &d, &s))
	{
            if (c && (d == XmSTRING_DIRECTION_L_TO_R) &&
                (((is_local || is_default) && 
                  ((c == XmFONTLIST_DEFAULT_TAG) || 
		   (strcmp(c, XmFONTLIST_DEFAULT_TAG) == 0) ||
		   (strcmp(c, _XmStringGetCurrentCharset()) == 0))) ||
		 (curcharset && (strcmp (c, curcharset) == 0))))
	    {
		*text = t;			/* OK, pass text to caller */
		done = TRUE;
	    }
	    else
		XtFree (t);			/* not this text */

   	    if (c)
                XtFree (c);                     /* always dump charset */
	}
	else
	    done = TRUE;
    }

    XmStringFreeContext (context);
    return (*text != NULL);
}

/*
 * this set is the TCS font list handling stuff
 */
static int 
#ifdef _NO_PROTO
_index_cache_charset( charset, length )
        char *charset ;
        int length ;
#else
_index_cache_charset(
        char *charset,
        int length )
#endif /* _NO_PROTO */
{   
    char *a;
    int i;

    for (i=0; i<_cache_count; i++)
    {
	int l = strlen(_charset_cache[i]);
        if (l == length && ((charset == _charset_cache[i]) ||
			    (strncmp (charset, _charset_cache[i], l) == 0) ))
	{
	    return( i) ;
            }
        }
    _charset_cache = (char **) XtRealloc ((char *) _charset_cache, 
                                        sizeof (char **) * (_cache_count + 1));

    /* Treat XmFONTLIST_DEFAULT_TAG specially; Can be used as a variable. */
    /* Can't count on "charset" being null-terminated. */
    if(    (strncmp(charset, XmFONTLIST_DEFAULT_TAG, length) == 0)
        && (length == strlen( XmFONTLIST_DEFAULT_TAG))    )
      {
	a = XmFONTLIST_DEFAULT_TAG;
      }
    else 
      {
	a = XtMalloc (length + 1);
	memcpy( a, charset, length);
	a[length] = '\0';
      }
    
    _charset_cache[_cache_count] = a;

    return( _cache_count++) ;
    } 

static char * 
#ifdef _NO_PROTO
_cache_charset( charset, length )
        char *charset ;
        int length ;
#else
_cache_charset(
        char *charset,
        int length )
#endif /* _NO_PROTO */
{
    int charset_index ;

    charset_index = _index_cache_charset( charset, length) ;
    return( _charset_cache[charset_index]) ;
    }

static void 
#ifdef _NO_PROTO
_cache_fontlist( f )
        XmFontList f ;
#else
_cache_fontlist(
        XmFontList f )
#endif /* _NO_PROTO */
{

    FontlistEntry *node = (FontlistEntry *) XtMalloc(sizeof(FontlistEntry));

    FontCacheFontList(node) = f;
    FontCacheRefCount(node) = 1;
    FontCacheNext(node) = _fontlist_cache;

    _fontlist_cache = node;
}

XmFontListEntry
#ifdef _NO_PROTO
XmFontListEntryCreate( tag, type, font )
        char *tag ;
        XmFontType type ;
        XtPointer font ;
#else
XmFontListEntryCreate(
        char *tag,
        XmFontType type ,
        XtPointer font )
#endif /* _NO_PROTO */
{
    XmFontListEntry f;
    char            *derived_tag;
    
    if ((font == NULL) || (tag == NULL) ||
        ((type != XmFONT_IS_FONTSET) && (type != XmFONT_IS_FONT)))
         return (NULL);
  
    if ((tag != XmFONTLIST_DEFAULT_TAG) &&
	(strcmp(tag, XmSTRING_DEFAULT_CHARSET) == 0))
      derived_tag = _XmStringGetCurrentCharset();
    else derived_tag = tag;

    f = (XmFontListEntry) XtMalloc (sizeof (XmFontListRec));
    FontListType(f) = type;
    FontListFont(f) = font;
    FontListTag(f) = _cache_charset (derived_tag, strlen (derived_tag));
  
    return (f);
}

void
#ifdef _NO_PROTO
XmFontListEntryFree( entry )
        XmFontListEntry  *entry ;
#else
XmFontListEntryFree(
        XmFontListEntry  *entry )
#endif /* _NO_PROTO */
{
    if (entry != NULL) 
        XtFree ((char *) *entry);
}

XtPointer
#ifdef _NO_PROTO
XmFontListEntryGetFont( entry, typeReturn )
        XmFontListEntry entry ;
        XmFontType *typeReturn ;
#else
XmFontListEntryGetFont(
        XmFontListEntry entry ,
        XmFontType *typeReturn )
#endif /* _NO_PROTO */
{
    if (entry == NULL)
       return (NULL);

    *typeReturn = FontListType(entry);
    return ( FontListFont(entry) );
}

char *
#ifdef _NO_PROTO
XmFontListEntryGetTag( entry )
        XmFontListEntry entry ;
#else
XmFontListEntryGetTag(
        XmFontListEntry entry )
#endif /* _NO_PROTO */
{
    if (entry == NULL)
       return (NULL);

    return ( XtNewString (FontListTag(entry)) );  /* pending spec change */
}

XmFontList 
#ifdef _NO_PROTO
XmFontListAppendEntry( old, entry )
        XmFontList old ;
        XmFontListEntry entry ;
#else
XmFontListAppendEntry(
        XmFontList old ,
        XmFontListEntry entry )
#endif /* _NO_PROTO */
{
    int             i, k, oldcnt;
    XmFontList      p, new_f;
    FontlistEntry  *cachePtr;
    Boolean         matched;

    if (!entry)
        return (old);

    /*
     * correctly handles old == NULL
     */

    if (!old)
    {
        oldcnt = 0;
    }
    else
    {
	/* count list */
        for (i=0, p=old; FontListFont(p) != NULL; p++, i++)
	    ;
        oldcnt = i;
    }

    /*
     * try to find a fontlist in the cache that the newly expanded fontlist
     * would match.
     */

    for (cachePtr = _fontlist_cache; cachePtr; 
                                     cachePtr = FontCacheNext(cachePtr))
    {
        /*
         * count the fontlist entries in this cached fontlist.  We're looking
         * for a fontlist with one entry more than the old fontlist.
         */

        new_f = FontCacheFontList(cachePtr);
        for (k=0; FontListFont(new_f) != NULL; new_f++, k++)
            ;

        if (k != (oldcnt+1))
           continue;                    /* wrong length */

        /* ok, this one is the right length.  loop thru the fontlist entries,
         * until they don't match or the old font ends. Skip the terminating
         * NULL entries, we know the length and it will screw up the matching.
         */

        matched = TRUE;
        new_f = FontCacheFontList(cachePtr);
	if (oldcnt)
	{
            for (p = old; FontListFont(p); p++, new_f++)
            {
                if ((FontListType(new_f) != FontListType(p)) ||
                    (FontListFont(new_f) != FontListFont(p)) ||
                    (FontListTag(new_f) != FontListTag(p)))
                {
                   /* match failed; try next fontlist in cache */
                   matched = FALSE;
                   break;
                }
            }
	}

        /*
         * That fontlist entry matched exactly what we're looking for.  So,
         * now, compare the next cached entry against the font and charset
         * we would add.
         */

        if (matched)
        {
            if ((FontListType(new_f) == FontListType(entry)) && 
                (FontListFont(new_f) == FontListFont(entry)) && 
                (FontListTag(new_f) == FontListTag(entry)))
            {

               /*
                * found it.
                */

               if (old)
	           XmFontListFree (old);
               FontCacheRefCount(cachePtr)++;
               return (FontCacheFontList(cachePtr));
            }
        }
    }

    /*
       After all this, we didn't find it.  Make a new fontlist from the
       old one and cache it.
    */

    new_f = (XmFontList) XtMalloc (sizeof (XmFontListRec) * (oldcnt+2));
    if (oldcnt)
        memcpy( new_f, old, (sizeof (XmFontListRec) * oldcnt));	/* copy over */

    FontListType(&new_f[oldcnt]) = FontListType(entry);
    FontListFont(&new_f[oldcnt]) = FontListFont(entry);
    FontListTag(&new_f[oldcnt]) = FontListTag(entry);

    oldcnt++;

    FontListFont(&new_f[oldcnt]) = NULL;
    FontListTag(&new_f[oldcnt]) = NULL;

    /* make a new cache node and insert it in the beginning. */

    _cache_fontlist(new_f);

    if (old)
	XmFontListFree (old);
    return (new_f);
}

XmFontListEntry
#ifdef _NO_PROTO
XmFontListNextEntry( context )
        XmFontContext context ;
#else
XmFontListNextEntry(
        XmFontContext context )
#endif /* _NO_PROTO */
{
    XmFontListEntry   entry;

    if (!context || context->error)
       return (NULL);

    if (!FontListContextNext(context) ||
	!FontListFont(FontListContextNext(context)))
    {
       FontListContextError(context) = TRUE;
       return (NULL);
    }

    entry = (XmFontListEntry) FontListContextNext(context);
    FontListContextNext(context)++;
    return (entry);
}

XmFontList 
#ifdef _NO_PROTO
XmFontListRemoveEntry( old, entry )
        XmFontList old ;
        XmFontListEntry entry ;
#else
XmFontListRemoveEntry(
        XmFontList old ,
        XmFontListEntry entry )
#endif /* _NO_PROTO */
{
    int              i, k, oldcnt, matchcnt;
    XmFontList       p, new_f, oldPtr;
    Boolean         *matchedEntry;
    FontlistEntry   *cachePtr;
    Boolean          matched;

    if (!old || !entry)
    {
        return (old);
    }

    /*
     *  Count the size of the original list and create an array of Booleans
     *  to indicate entry matches.
     */

    for (p=old, i=0; FontListFont(p) != NULL; p++, i++)
	;
    oldcnt = i;

    matchedEntry =
	(Boolean *) ALLOCATE_LOCAL ((unsigned) (oldcnt * sizeof(Boolean)));

    /*
     *  Find the matching entries.
     */

    for (matchcnt=0, p=old, i=0; FontListFont(p) != NULL; p++, i++)
    {
        if ((FontListType(p) == FontListType(entry)) &&
            (FontListFont(p) == FontListFont(entry)) &&
            (FontListTag(p) == FontListTag(entry)))
        {
            /* match */
            matchcnt++;
            matchedEntry[i] = TRUE;
        }
	else
	{
            matchedEntry[i] = FALSE;
	}
    }

    /*
     *  If no match, return old.
     *  If old has only matching entries return NULL.
     */

    if (!matchcnt)
    {
        DEALLOCATE_LOCAL ((char *)matchedEntry);
        return (old);
    }
    if (oldcnt == matchcnt)
    {
        DEALLOCATE_LOCAL ((char *)matchedEntry);
        XmFontListFree (old);
        return ((XmFontList) NULL);
    }

    /*
     * try to find a fontlist in the cache that the newly contracted fontlist
     * would match.
     */

    for (cachePtr = _fontlist_cache; cachePtr; 
                                     cachePtr = FontCacheNext(cachePtr))
    {
        /*
         * count the fontlist entries in this cached fontlist.  We're looking
         * for a fontlist with matchcnt entries less than the old fontlist.
         */
        p = FontCacheFontList(cachePtr);
        for (k=0; FontListFont(p) != NULL; p++, k++)
            ;

        if (k != (oldcnt-matchcnt))
           continue;                    /* wrong length */

        /* ok, this one is the right length.  loop thru the fontlist entries,
         * until they don't match or the old font ends. Skip the terminating
         * NULL entries, we know the length and it will screw up the matching.
         */

        matched = TRUE;
        i = 0;
	oldPtr = old;
	new_f = FontCacheFontList(cachePtr);

	while (FontListFont(oldPtr))  /* remaining entries */
	{
	    if (!matchedEntry[i])
	    {
                if ((FontListType(new_f) != FontListType(oldPtr)) ||
                    (FontListFont(new_f) != FontListFont(oldPtr)) ||
                    (FontListTag(new_f) != FontListTag(oldPtr)))
                {
                   /* match failed; try next fontlist in cache */
                   matched = FALSE;
                   break;
                }
                new_f++;
	    }
            i++, oldPtr++;
	}

        if (matched)
	{
           DEALLOCATE_LOCAL ((char *)matchedEntry);
           XmFontListFree (old);
           FontCacheRefCount(cachePtr)++;
           return (FontCacheFontList(cachePtr));
        }
    }

    /*
     * After all this, we didn't find it.  Make a new fontlist from the
     * old one and cache it. 
     */

    new_f = (XmFontList) XtMalloc (sizeof(XmFontListRec) * (oldcnt-matchcnt+1));

    i = 0;
    oldPtr = old;
    p = new_f;

    while (FontListFont(oldPtr))  /* remaining entries */
    {
        if (!matchedEntry[i])
	{
            FontListType(p) = FontListType(oldPtr);
            FontListFont(p) = FontListFont(oldPtr);
            FontListTag(p) = FontListTag(oldPtr);
            p++;
        }
        i++, oldPtr++;
    }
    FontListFont(p) = NULL;
    FontListTag(p) = NULL;

    /* make a new cache node and insert it in the beginning. */

    _cache_fontlist(new_f);

    DEALLOCATE_LOCAL ((char *)matchedEntry);
    XmFontListFree (old);
    return (new_f);
}

XmFontListEntry
#ifdef _NO_PROTO
XmFontListEntryLoad ( display, fontName, type, tag )
        Display *display ;
        char *fontName ;
        XmFontType type ;
        char *tag ;
#else
XmFontListEntryLoad(
        Display *display ,
        char *fontName ,
        XmFontType type ,
        char *tag )
#endif /* _NO_PROTO */
{
    static XrmString locale;
    static XtPointer font;
    XrmValue         args[2];
    Cardinal         num_args = 0;
    XrmValue         fromVal;
    XrmValue         toVal;
    Boolean          result = False;

    args[0].addr = (XPointer) &display;
    args[0].size = sizeof(Display*);
    num_args++;

    fromVal.addr = fontName;
    fromVal.size = strlen(fontName);

    toVal.addr = (XPointer) &font;
    toVal.size = sizeof (XtPointer);

    if (type == XmFONT_IS_FONT)
    {
	result = XtCallConverter (display, XtCvtStringToFontStruct, args,
				  num_args, &fromVal, &toVal, NULL);
    }
    else if (type == XmFONT_IS_FONTSET)
    {
        locale = XrmQuarkToString(XrmStringToQuark(setlocale(LC_ALL, NULL)));
        args[1].addr = (XPointer) &locale;
        args[1].size = sizeof(XrmString);
        num_args++;

	result = XtCallConverter (display, XtCvtStringToFontSet, args,
				  num_args, &fromVal, &toVal, NULL);
    }

    if ((result) && (font != NULL))
    {
        return (XmFontListEntryCreate( tag, type, font ));
    }
    else
    {
	return (NULL);
    }
}

XmFontList 
#ifdef _NO_PROTO
XmFontListCreate( font, charset )
        XFontStruct *font ;
        XmStringCharSet charset ;
#else
XmFontListCreate(
        XFontStruct *font,
        XmStringCharSet charset )
#endif /* _NO_PROTO */
{
    FontlistEntry       *cachePtr;
    XmFontList          f;
    char              	*curcharset; 

    if ((font == NULL) || (charset == NULL))
         return (NULL);
#ifdef IBM_MOTIF
    if(!strcmp(charset, "Xm_FONTLIST_DEFAULT_TAG_STRING"))
        charset = XmFONTLIST_DEFAULT_TAG;
#endif /* IBM_MOTIF */

    if ((charset != XmFONTLIST_DEFAULT_TAG) &&
	(strcmp(charset, XmSTRING_DEFAULT_CHARSET) == 0))
      curcharset = _XmStringGetCurrentCharset();
    else curcharset = charset;
    
    /*
     * See if this fontlist is cached.
     */

    for (cachePtr = _fontlist_cache; cachePtr; 
       cachePtr = FontCacheNext(cachePtr))
    {

        XmFontList    listPtr;

        listPtr = FontCacheFontList(cachePtr);
        if ((FontListType(&listPtr[0]) == XmFONT_IS_FONT) &&
            (FontListFont(&listPtr[0]) == (XtPointer) font) &&
            (FontListTag(&listPtr[0]) != NULL) &&
            (strcmp(FontListTag(&listPtr[0]),curcharset) == 0) &&
            (FontListFont(&listPtr[1]) == NULL) &&
            (FontListTag(&listPtr[1]) == NULL))
        {
            FontCacheRefCount(cachePtr)++;
            return (listPtr);
        }
    }

    /*
     * If we got here, the fontlist isn't in the cache.
     */

    f = (XmFontList) XtMalloc (sizeof (XmFontListRec) * 2);

    FontListType(&f[0]) = XmFONT_IS_FONT;
    FontListFont(&f[0]) = (XtPointer) font;
    FontListTag(&f[0]) = _cache_charset (curcharset, strlen (curcharset));

    FontListFont(&f[1]) = NULL;
    FontListTag(&f[1]) = NULL;

    _cache_fontlist(f);
    return (f);
}

XmFontList 
#ifdef _NO_PROTO
XmStringCreateFontList( font, charset )
        XFontStruct *font ;
        XmStringCharSet charset ;
#else
XmStringCreateFontList(
        XFontStruct *font,
        XmStringCharSet charset )
#endif /* _NO_PROTO */
{
    return (XmFontListCreate(font,charset));
}


/*
 * dump a font list
 */
void 
#ifdef _NO_PROTO
XmFontListFree (fontlist)
    XmFontList      fontlist;
#else
XmFontListFree (
    XmFontList      fontlist)
#endif
{

    FontlistEntry   *cachePtr;
    FontlistEntry   *lastPtr;

    if (fontlist == NULL) return;

    for (cachePtr = _fontlist_cache, lastPtr = NULL;
         cachePtr;
         lastPtr = cachePtr, cachePtr = FontCacheNext(cachePtr))
    {

        if (FontCacheFontList(cachePtr) == fontlist)
        {
            FontCacheRefCount(cachePtr)--;
            if (FontCacheRefCount(cachePtr) == 0)
            {
                if (lastPtr == NULL)
                     _fontlist_cache = FontCacheNext(cachePtr);
                else FontCacheNext(lastPtr) = FontCacheNext(cachePtr);
                XtFree((char *) cachePtr);
                XtFree((char *) fontlist);
            }
            return;
        }
    }

    /*
     *  If we got here, for some bad reason this font wasn't in the
ifndef OSF_v1_2_4
     *  cache.  Free it.
else
     *  cache.  Ignore it.
endif
     */
#ifndef OSF_v1_2_4
    if (fontlist != NULL) 
        XtFree ((char *) fontlist);
#endif /* OSF_v1_2_4 */
}
/*
 * extent a font list by one element, the old font list is gone
 */
XmFontList 
#ifdef _NO_PROTO
XmFontListAdd( old, font, charset )
        XmFontList old ;
        XFontStruct *font ;
        XmStringCharSet charset ;
#else
XmFontListAdd(
        XmFontList old,
        XFontStruct *font,
        XmStringCharSet charset )
#endif /* _NO_PROTO */
{
    int i, k, oldcnt;
    XmFontList p = old, q;
    FontlistEntry    *cachePtr;
    XmStringCharSet  curcharset; 
    Boolean          matched;

    if (!old)
        return((XmFontList) NULL);
    if (!charset || !font)
        return ((XmFontList) old);
#ifdef IBM_MOTIF
    if(!strcmp(charset, "Xm_FONTLIST_DEFAULT_TAG_STRING"))
        charset = XmFONTLIST_DEFAULT_TAG;
#endif /* IBM_MOTIF */

    if ((charset != XmFONTLIST_DEFAULT_TAG) &&
	(strcmp(charset, XmSTRING_DEFAULT_CHARSET) == 0))
      curcharset = _XmStringGetCurrentCharset();
    else curcharset = charset;
    
    for (i=0; FontListFont(p) != NULL; p++, i++)	/* count list */
	;
    oldcnt = i;

    /*
     * try to find a fontlist in the cache that the newly expanded fontlist
     * would match.
     */

    for (cachePtr = _fontlist_cache; cachePtr; 
       cachePtr = FontCacheNext(cachePtr))
    {
        XmFontList   listPtr, oldfontPtr;

        /*
         * count the fontlist entries in this cached fontlist.  We're looking
         * for a fontlist with one entry more than the old fontlist.
         */
        p = FontCacheFontList(cachePtr);
        for (k=0; FontListFont(p) != NULL; p++, k++)
            ;

        if (k != (oldcnt+1))
           continue;                    /* wrong length */

        /* ok, this one is the right length.  loop thru the fontlist entries,
         * until they don't match or the old font ends. Skip the terminating
         * NULL entries, we know the length and it will screw up the matching.
         */

        matched = TRUE;
        for (oldfontPtr = old, listPtr = FontCacheFontList(cachePtr);
             FontListFont(oldfontPtr);
             oldfontPtr++, listPtr++)
        {
            if ((FontListType(listPtr) != FontListType(oldfontPtr)) ||
                (FontListFont(listPtr) != FontListFont(oldfontPtr)) ||
                (FontListTag(listPtr) != FontListTag(oldfontPtr)))
            {
               /* match failed; try next fontlist in cache */
               matched = FALSE;
               break;
            }
        }

        /*
         * That fontlist entry matched exactly what we're looking for.  So,
         * now, compare the next cached entry against the font and charset
         * we would add.
         */

        if (matched)
        {
            if ((FontListType(listPtr) == XmFONT_IS_FONT) && 
                (FontListFont(listPtr) == (XtPointer) font) && 
                (strcmp(FontListTag(listPtr), curcharset) == 0))
            {
               /*
                * found it.
                */

               XmFontListFree (old);
               FontCacheRefCount(cachePtr)++;
               return (FontCacheFontList(cachePtr));
            }
        }
    }

    /*
       After all this, we didn't find it.  Make a new fontlist from the
       old one and cache it.
    */

    q = (XmFontList) XtMalloc (sizeof (XmFontListRec) * (i+2));

    memcpy( q, old, (sizeof (XmFontListRec) * i));	/* copy over */

    FontListType(&q[i]) = XmFONT_IS_FONT;
    FontListFont(&q[i]) = (XtPointer) font;
    FontListTag(&q[i]) = _cache_charset (curcharset, strlen (curcharset));

    i++;

    FontListFont(&q[i]) = NULL;
    FontListTag(&q[i]) = NULL;

    /* make a new cache node and insert it in the beginning. */

    _cache_fontlist(q);
    XmFontListFree (old);

    return (q);
}

/*
 * replicate a font list
 */
XmFontList 
#ifdef _NO_PROTO
XmFontListCopy( fontlist )
        XmFontList fontlist ;
#else
XmFontListCopy(
        XmFontList fontlist )
#endif /* _NO_PROTO */
{
    XmFontList p, q;
    FontlistEntry       *cachePtr;
    int i, j;

    if (!fontlist) return((XmFontList ) NULL);

    /*
     *	Find the font list in the cache, and increment the number
     *	of references.
     */
    for (cachePtr = _fontlist_cache; cachePtr; 
                                     cachePtr = FontCacheNext(cachePtr))
    {
        if (FontCacheFontList(cachePtr) == fontlist)
        {
            FontCacheRefCount(cachePtr)++;
            return (fontlist);
        }
    }

    /*
     *  If we got here, for some bad reason this font wasn't in the
     *  cache.  Make a real copy.
     */
    for (i=0, p = fontlist; FontListFont(p) != NULL; p++, i++)  /* count  list */
	;

    q = (XmFontList) XtMalloc (sizeof (XmFontListRec) * (i+1));

    for (j=0; j<i; j++)	q[j] = fontlist[j]; 		/* copy over */

    q[i] = fontlist[i];					/* copy null item too */

    return (q);
}

XFontStruct *
#ifdef _NO_PROTO
_XmGetFirstFont( entry )
	XmFontListEntry  entry ;
#else
_XmGetFirstFont(
	XmFontListEntry  entry)
#endif /* _NO_PROTO */
{
    XFontStruct *font_struct;

    if (FontListType(entry) == XmFONT_IS_FONTSET)
    {
	XFontStruct **font_struct_list;
	char **font_name_list;

	if (XFontsOfFontSet( (XFontSet) FontListFont(entry),
	                     &font_struct_list, &font_name_list))
	{
            font_struct = font_struct_list[0];
	}
	else
	{
            font_struct = NULL;
	}
    }
    else
    {
        font_struct = (XFontStruct *) FontListFont(entry);
    }
    return (font_struct);
}

/*
 * Find an entry in the fontlist which matches the current charset or
 * return the first font if none match.
 */
Boolean 
#ifdef _NO_PROTO
_XmFontListGetDefaultFont( fontlist, font_struct )
        XmFontList fontlist ;
        XFontStruct **font_struct ;
#else
_XmFontListGetDefaultFont(
        XmFontList fontlist,
        XFontStruct **font_struct )
#endif /* _NO_PROTO */
{
  XmStringCharSet       charset = XmFONTLIST_DEFAULT_TAG;
  short			indx = -1;
  Boolean		retval;
  
  retval = _XmFontListSearch (fontlist, charset, &indx, font_struct);
/*  XtFree(charset); */
  return(retval);
}

/*
 * find an entry in the font list which matches, return index (or -1) and
 * font stuct ptr (or first in list).
 */
Boolean 
#ifdef _NO_PROTO
_XmFontListSearch( fontlist, charset, indx, font_struct )
        XmFontList fontlist ;
        XmStringCharSet charset ;
        short *indx ;
        XFontStruct **font_struct ;
#else
_XmFontListSearch(
        XmFontList fontlist,
        XmStringCharSet charset,
        short *indx,
        XFontStruct **font_struct )
#endif /* _NO_PROTO */
{
    XmFontListEntry    entry;
    Boolean            success;
  
    success = FontListSearch( fontlist, charset, FALSE, indx, &entry);
    if (success) *font_struct = _XmGetFirstFont(entry);

    return(success) ;
}

/****************
 * If the cached_charset flag is true, FontListSearch assumes that the
 *   charset pointer is a pointer out of the (local) charset cache.
 *   Since XmFontListCreate also uses charset pointers out of this cache,
 *   a string compare is avoided by simply comparing pointer values.
 ****************/
static Boolean 
#ifdef _NO_PROTO
FontListSearch( fontlist, charset, cached_charset, indx, entry_ptr )
	XmFontList fontlist ;
	XmStringCharSet charset ;
	Boolean cached_charset ;
	short *indx ;
	XmFontListEntry *entry_ptr;
#else
FontListSearch(
        XmFontList fontlist,
        XmStringCharSet charset,
#if NeedWidePrototypes
        int cached_charset,
#else
        Boolean cached_charset,
#endif /* NeedWidePrototypes */
        short *indx,
	XmFontListEntry *entry_ptr )
#endif /* _NO_PROTO */
{   
  int                         i;
  XmStringCharSet     search_cset = NULL;
  
  *indx = -1 ;

  if ((charset == NULL) && (fontlist != NULL))
    {   
      /* pickup first one */
      *entry_ptr = &fontlist[0];
      *indx = 0 ;

      return( TRUE) ; 
    }
  if (fontlist != NULL)
    {   
      if (cached_charset) /* No XmSTRING_DEFAULT_CHARSET */
        {   
        for(i=0 ; FontListFont( &fontlist[i]) != NULL ; i++)
            {   
            if(FontListCharset( &fontlist[i]) == charset)
                {   
                *indx = i ;
                *entry_ptr = &fontlist[i];

                return( TRUE) ;
              }
          } 
      }
      else
      {   
        XmStringCharSet       curcharset; 

        if ((strcmp(charset, XmSTRING_DEFAULT_CHARSET) == 0))
          curcharset = _XmStringGetCurrentCharset();
        else curcharset = charset;

        for (i=0 ; FontListFont( &fontlist[i]) != NULL ; i++)
            {   
            if (strcmp (FontListCharset( &fontlist[i]), curcharset) == 0)
                {   
                *indx = i ;
                *entry_ptr = &fontlist[i];

                return( TRUE) ;
                }
          }  
      } 
  
      /* Didn't find a match.  See if charset is one of the defaults
       and search for the other. */
      if (strcmp(charset, XmSTRING_DEFAULT_CHARSET) == 0) 
      {
        search_cset = XmFONTLIST_DEFAULT_TAG;
        for (i=0; FontListFont(&fontlist[i]) != NULL; i++)
          {   
            if (FontListCharset(&fontlist[i]) == search_cset)
              {   
                *indx = i ;
                *entry_ptr = &fontlist[i];

                return( TRUE) ;
              }
          }
      }
      else if ((charset == XmFONTLIST_DEFAULT_TAG) ||
	       (strcmp(charset, XmFONTLIST_DEFAULT_TAG) == 0))
      {
        search_cset = _XmStringGetCurrentCharset();
        for (i=0; FontListFont(&fontlist[i]) != NULL; i++)
          {   
            if (strcmp(FontListCharset(&fontlist[i]), search_cset) == 0)
              {   
                *indx = i ;
                *entry_ptr = &fontlist[i];

                return( TRUE) ;
              }
          }
      }
        
      /* Otherwise pick up first one. */
      *entry_ptr = &fontlist[0];
      *indx = 0 ;

      return( TRUE) ;
    }
  else
    { /****************
         * We should create a default fontlist and return that.
         ****************/
        *entry_ptr = NULL ;                          /* or NULL */
        } 
    return( FALSE) ;
}

/*
 * Fontlist access routines
 */
Boolean 
#ifdef _NO_PROTO
XmFontListInitFontContext( context, fontlist )
        XmFontContext *context ;
        XmFontList fontlist ;
#else
XmFontListInitFontContext(
        XmFontContext *context,
        XmFontList fontlist )
#endif /* _NO_PROTO */
{

    XmFontContext p;

    if ((!fontlist) || (!context))
        return(FALSE);

    p = (XmFontContext) XtMalloc (sizeof (XmFontListContextRec));
    if (p == NULL)
       return (FALSE);
    FontListContextNext(p) = fontlist;
    FontListContextError(p) = FALSE;
    *context = p;
    return (TRUE);

}

Boolean 
#ifdef _NO_PROTO
XmFontListGetNextFont( context, charset, font )
        XmFontContext context ;
        XmStringCharSet *charset ;
        XFontStruct **font ;
#else
XmFontListGetNextFont(
        XmFontContext context,
        XmStringCharSet *charset,
        XFontStruct **font )
#endif /* _NO_PROTO */
{

    int length;

    if (!(context && charset && font))
       return (FALSE);

    if (context->error)
       return (FALSE);

    *font = _XmGetFirstFont (FontListContextNext(context));

    if (*font == NULL)
    {
       FontListContextError(context) = TRUE;
       return (FALSE);
    }
    else {
        length = strlen(FontListTag(FontListContextNext(context)));
        *charset = (XmStringCharSet) XtMalloc(length + 1);
        strcpy (*charset, FontListTag(FontListContextNext(context)));
        (*charset)[length] = '\0';
        FontListContextNext(context)++;
        return (TRUE);
    }
}

void
#ifdef _NO_PROTO
XmFontListFreeFontContext( context )
        XmFontContext context ;
#else
XmFontListFreeFontContext(
        XmFontContext context )
#endif
{

    if (context)
       XtFree ((char *) context);
}

/*
 * general external TCS utilties
 */
XmString 
#ifdef _NO_PROTO
XmStringConcat( a, b )
        XmString a ;
        XmString b ;
#else
XmStringConcat(
        XmString a,
        XmString b )
#endif /* _NO_PROTO */
{
    unsigned short  a_length, b_length;
    unsigned char   *p, *q, *c;
    int     i, j;
    short   head_size;

    if (!a && !b) return ((XmString) NULL);
    if (a == (XmString) NULL) return (XmStringCopy (b));
    if (b == (XmString) NULL) return (XmStringCopy (a));

    a_length = _read_string_length ((unsigned char *) a);
    b_length = _read_string_length ((unsigned char *) b);
    head_size = _calc_header_size(a_length + b_length);

    i = a_length + b_length		 	/* sum + outer component */
	+ head_size;

    p = c = (unsigned char *) XtMalloc (i);
    p = _write_header(p, a_length + b_length);

    q = (unsigned char *) a;				/* start at beginning */
    q = _read_header(q);
    
    for (j=0; j<(a_length); j++) 		/* copy a into new one */
	*p++ = *q++;				/* excluding outer header */

    q = ((unsigned char *) b);				/* skip b's outer HEADER */
    q = _read_header(q);

    for (j=0; j<b_length; j++)	 		/* copy b into new one */
	*p++ = *q++;

    return ((XmString) c);
}

/*
 * concat two external strings.  Only concat a component at a time
 * so that we always wind up with a meaningful string
 */
XmString 
#ifdef _NO_PROTO
XmStringNConcat( first, second, n )
        XmString first ;
        XmString second ;
        int n ;
#else
XmStringNConcat(
        XmString first,
        XmString second,
        int n )
#endif /* _NO_PROTO */
{
    unsigned char    *a = (unsigned char *) first;
    unsigned char    *b = (unsigned char *) second;
    XmString string;
    unsigned short   used, delta;
    unsigned char    *p, *new_c, *b_end;
    unsigned char    *q ;
    unsigned char    *bp;
    unsigned short   a_length, b_length, length, header, foo;
    short    head_size;
    Boolean	     asn1;
 
    if (a && !b) return (XmStringCopy(a));
    if (!a || !b) return ((XmString) NULL);
    if (n < (ASNTAG + CSSHORTLEN)) return (XmStringCopy(a));
    a_length = _read_string_length(a);
    b_length = n < _read_string_length(b) ? n : _read_string_length(b);

    head_size = _calc_header_size(a_length + b_length);

    if (_is_short_length(a) && 
        ((unsigned short)head_size > _read_header_length(a)))
    {    
    /* 
     * If the first string was short enough to use the smaller length
     * field, we have to check that we can still use it for the concatenated
     * version.  Otherwise, the whole string needs to be copied into a
     * large length version
     */
      string = (XmString) _copy_short_to_long(a);
    }
    else {
       /*
        * Whichever length type it is, we will keep it that way.
        */
      string = XmStringCopy((XmString)a);
    }

    used =  _read_string_length ((unsigned char *) string) + 
            _read_header_length ((unsigned char *) string);
    string = (XmString) XtRealloc((char *) string, used + n);

    /*
     * Point to starting location to concat second string into
     */
    p = (unsigned char *) string + used ;

    /*
     * Get a pointer to the string length for update later
     */
    q = (unsigned char *) string;

    /*
     * Point to first component of string
     */
    bp = _read_header(b);

    /*
     * Calculate pointer to end of string
     */
    b_end = ((unsigned char *) b) + _read_string_length (b) + 
      _read_header_length(b);

    /* Is b a new, asn.1 conformant string? */
    asn1 = _is_asn1(b);

    length = asn1 ? _read_asn1_length(bp) : _read_component_length (bp);
    header = asn1 ? _asn1_size(length) : HEADER;
    
    while (((length + header) < n) && (bp < b_end))
      {
	new_c = _read_component(bp, p, &foo, (p + header), asn1);
	if (header > ASNTAG + CSSHORTLEN)
	  _write_long_length((p + ASNTAG), length);
	else 
	  *(p + ASNTAG) = length;
	    
	delta = length + header;
	used += delta;
	p += delta;
	bp = new_c;
	n -= delta;
	length = asn1 ? _read_asn1_length(bp) : _read_component_length (bp);
	header = asn1 ? _asn1_size(length) : HEADER;
      }

    /*
     * Update length field of entire string
     */
    header = _read_header_length(string);
    
    if (header > (ASNHEADERLEN + CSSHORTLEN))
      _write_long_length((q + ASNHEADERLEN), used - header);
    else _write_header(q, used - header);

    string = (XmString) XtRealloc((char *) string, used);

    return (string);
}

XmString 
#ifdef _NO_PROTO
XmStringCopy( string )
        XmString string ;
#else
XmStringCopy(
        XmString string )
#endif /* _NO_PROTO */
{
    XmString 		c;
    unsigned short	length, oldlen, oldheader, len, header, foo, delta;
    unsigned short	used = 0;
    unsigned char    	*sp, *cp, *new_c, *end;

    if (string == (XmString) NULL) return ((XmString) NULL);

    oldlen = _read_string_length ((unsigned char *) string);
    oldheader =  _read_header_length ((unsigned char *) string);
    length = oldlen + oldheader;
    
    c = (XmString) XtMalloc (length);

    cp = _write_header(c, oldlen);
    
    if (_is_asn1(string)) memcpy( c, string, length);
    else 
      {
	sp = _read_header(string);
	end = string + _read_string_length(string) + 
	  _read_header_length(string);
	
	while (sp < end)
	  {
	    len = _read_component_length(sp);
	    header = _asn1_size(len);
	    
	    if ((cp + len + header) > (c + length))
	      {
		length = (cp + len + header - c);
		c = (XmString)XtRealloc((char *)c, length);
		cp = c - (len + header);
	      }
	    
	    new_c = _read_component(sp, cp, &foo, (cp + header), FALSE);
	    if (header > ASNTAG + CSSHORTLEN)
	      _write_long_length((cp + ASNTAG), len);
	    else *(cp + ASNTAG) = len;
	    
	    delta = len + header;
	    used += delta;
	    sp = new_c;
	    cp += delta;
	  }
	
	/* Reallocate string if needed and update length field. */
	if (used < oldlen) 
	  {
	    unsigned short headsize = _calc_header_size(used);
	    
	    if (headsize < oldheader)
	      {
		memcpy((c + headsize), (c + oldheader), used);
	      }
	    
	    c = (XmString)XtRealloc((char *)c, used + headsize);
	  }

	_write_header(c, used);
      }
    
    return (c);
}

/*
 * copy an external string.  Only copy a component at a time
 * so that we always wind up with a meaningful string
 */
XmString 
#ifdef _NO_PROTO
XmStringNCopy( str, n )
        XmString str ;
        int n ;
#else
XmStringNCopy(
        XmString str,
        int n )
#endif /* _NO_PROTO */
{
    unsigned char    *a = (unsigned char *) str;
    XmString string;
    unsigned short   used , delta, foo;
    unsigned char    *p, *new_c, *a_end;
    unsigned char   *q;
    unsigned char    *ap;
    short    head_size;
    int	     len, mal_size, length, header;
    Boolean  asn1;
    
    if (!a) return ((XmString) NULL);
    if (n < ASNTAG + CSSHORTLEN) return ((XmString) NULL);
    
    head_size = used = _read_header_length(a);
    len = _read_string_length(a);
    mal_size = (n < len) ? n : len;
    
    string = (XmString) XtMalloc (mal_size + head_size);
    p = (unsigned char *) string;
    q = (unsigned char *) string;
    ap = _read_header(a);

    p = _write_header(p, mal_size);

    a_end = ((unsigned char *) a) + len + head_size;

    /* Is this a new, asn.1 conformant string? */
    asn1 = _is_asn1(a);

    length = asn1 ? _read_asn1_length(ap) : _read_component_length (ap);
    header = asn1 ? _asn1_size(length) : HEADER;
    
    while (((length + header) <= (n - used)) && (ap < a_end))
      {	
	new_c = _read_component(ap, p, &foo, (p + header), asn1);
	if (header > ASNTAG + CSSHORTLEN)
	  _write_long_length((p + ASNTAG), length);
	else *(p + ASNTAG) = length;

	delta = length + header;
	used += delta;
	p += delta;
	ap = new_c;
	length = asn1 ? _read_asn1_length(ap) : _read_component_length (ap);
	header = asn1 ? _asn1_size(length) : HEADER;
      }

    _write_header(q, used - head_size);

    string = (XmString)XtRealloc((char *)string, used);
    
    return (string);
}

Boolean 
#ifdef _NO_PROTO
XmStringByteCompare( a1, b1 )
        XmString a1 ;
        XmString b1 ;
#else
XmStringByteCompare(
        XmString a1,
        XmString b1 )
#endif /* _NO_PROTO */
{
    unsigned char  *a = (unsigned char *) a1;
    unsigned char  *b = (unsigned char *) b1;
    unsigned short a_length, b_length;

    if ((a == (unsigned char *) NULL) && (b == (unsigned char *) NULL)) return (TRUE);
    if (a == (unsigned char *) NULL) return (FALSE);
    if (b == (unsigned char *) NULL) return (FALSE);

    a_length = _read_string_length (a) + _read_header_length(a);
    b_length = _read_string_length (b) + _read_header_length(b);

    if (a_length < b_length) return (FALSE);
    if (a_length > b_length) return (FALSE);

    return (!memcmp (a, b, a_length));
}


/************************************************************************
 *									*
 * XmStringCompare - compare two strings.  				*
 *									* 
 * Returns TRUE if the strings are equal, FALSE o.w.			*
 *									*
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmStringCompare( a, b )
        XmString a ;
        XmString b ;
#else
XmStringCompare(
        XmString a,
        XmString b )
#endif /* _NO_PROTO */
{
    _XmString _a, _b;
/* BEGIN OSF Fix CR 4399 */
    Boolean ret = FALSE;
/* END OSF Fix CR 4399 */

    if (!a && !b) return(TRUE);
    if (!a || !b) return(FALSE);

    _a = _XmStringCreate (a);
    _b = _XmStringCreate (b);

/* BEGIN OSF Fix CR 4399 */
    if (_a && _b)
      ret = _XmStringByteCompare(_a, _b);
/* END OSF Fix CR 4399 */
    _XmStringFree(_a);
    _XmStringFree(_b);
    return(ret);

}

int 
#ifdef _NO_PROTO
XmStringLength( string )
        XmString string ;
#else
XmStringLength(
        XmString string )
#endif /* _NO_PROTO */
{
    if (!string) return (0);
    if (!_XmStringIsXmString(string)) return (0);
    return ((int) _read_string_length ((unsigned char *) string) + 
                  _read_header_length ((unsigned char *) string));
}

Boolean 
#ifdef _NO_PROTO
XmStringEmpty( string )
        XmString string ;
#else
XmStringEmpty(
        XmString string )
#endif /* _NO_PROTO */
{
    unsigned char *c, *end;
    unsigned char tag;
    unsigned short length;
    Boolean asn1;
    
    if (!string) return (TRUE);
    if (!_XmStringIsXmString(string)) return (TRUE);
    
    end = ((unsigned char *) string) + 
      _read_string_length ((unsigned char *) string) + 
	_read_header_length ((unsigned char *) string);
    /* skip outer tag */
    c = (unsigned char *) _read_header((unsigned char *) string);	

    asn1 = _is_asn1(string);
    
    while (c < end)
      {
	c = _read_component(c, &tag, &length, NULL, asn1);

        if (((tag == XmSTRING_COMPONENT_TEXT) || 
             (tag == XmSTRING_COMPONENT_LOCALE_TEXT))
            && (length > 0)) return (FALSE);
      }

    return (TRUE);
}

Boolean 
#ifdef _NO_PROTO
XmStringHasSubstring( string, substring )
        XmString string ;
        XmString substring ;
#else
XmStringHasSubstring(
        XmString string,
        XmString substring )
#endif /* _NO_PROTO */
{
    _XmString str;
    _XmString substr;
    Boolean   retvalue;

    if ((string == NULL) || (substring == NULL) || (XmStringEmpty(substring)))
       return (FALSE);

    str = _XmStringCreate(string);
    substr = _XmStringCreate(substring);

    retvalue = _XmStringHasSubstring(str, substr);

    _XmStringFree(str);
    _XmStringFree(substr);
    return (retvalue);
}

/************************************************************************
 *                                                                      *
 * _XmStringIsXmString - returns TRUE if the parameter is an XmString.   *
 *                                                                      *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmStringIsXmString( string )
        XmString string ;
#else
_XmStringIsXmString(
        XmString string )
#endif /* _NO_PROTO */
{
    if (string == NULL) return(FALSE);
    return ((_is_asn1(string) || _is_compound(string)));
}

/*
 * determines from ASN.1 header whether this is an external compound string.
 * returns T or F.
 */
static Boolean 
#ifdef _NO_PROTO
_is_compound( string )
        XmString string ;
#else
_is_compound(
        XmString string )
#endif /* _NO_PROTO */
{
    unsigned char * uchar_p = (unsigned char *) string;

   /*
    *  Start with comparing the ASN.1 header.
    */
    return (strncmp ((char *) uchar_p, (char *) CSHeader, CSHEADERLEN) == 0);
}

/*
 * determines from ASN.1 header whether this is an ASN.1 conformant 
 * external compound string.  Returns T or F.
 */
static Boolean 
#ifdef _NO_PROTO
_is_asn1( string )
        XmString string ;
#else
_is_asn1( XmString string )
#endif /* _NO_PROTO */
{
    unsigned char * uchar_p = (unsigned char *) string;

    /*  Compare the ASN.1 header. */
    return (strncmp ((char *)uchar_p, (char *)ASNHeader, ASNHEADERLEN) == 0);
}

/*
 * internal structure access routines
 */
Boolean 
#ifdef _NO_PROTO
_XmStringInitContext( context, string )
        _XmStringContext *context ;
        _XmString string ;
#else
_XmStringInitContext(
        _XmStringContext *context,
        _XmString string )
#endif /* _NO_PROTO */
{
 
    _XmStringContext  ct;


    /* make sure there is something in the string.  we are
       going to assume a good string in the get next routine
     */
    if (!(string && context)) return (FALSE);

    ct = (_XmStringContext) XtMalloc(sizeof(_XmStringContextRec));

    _XmStrContError(ct)     = FALSE;
    _XmStrContCurrLine(ct)  = 0;
    _XmStrContCurrSeg(ct)   = 0;
    _XmStrContString(ct)    = string;
    _XmStrContOpt(ct)       = _XmStrOptimized(string);

    *context = ct;
    return (TRUE);

}

Boolean 
#ifdef _NO_PROTO
_XmStringGetNextSegment( context, charset, direction, text, char_count, separator )
        _XmStringContext context ;
        XmStringCharSet *charset ;
        XmStringDirection *direction ;
        char **text ;
        short *char_count ;
        Boolean *separator ;
#else
_XmStringGetNextSegment(
        _XmStringContext context,
        XmStringCharSet *charset,
        XmStringDirection *direction,
        char **text,
        short *char_count,
        Boolean *separator )
#endif /* _NO_PROTO */
{

    if (! (context && charset && direction && text && char_count && separator))
       return (FALSE);

    if (_XmStrContError(context))
       return(FALSE);

    if (_XmStrContOpt(context))
    {
        _XmStringOpt  str = (_XmStringOpt) (_XmStrContString(context));
        int           charsetlen = strlen(_XmOptCharsetGet(str));

        *charset = XtMalloc(charsetlen + 1);
        memcpy( *charset, _XmOptCharsetGet(str), charsetlen);
        (*charset)[charsetlen] = '\0';

        *char_count = _XmOptCharCount(str);
        *text = XtMalloc(*char_count + 1);
        memcpy( *text, _XmOptText(str), *char_count);
        (*text)[*char_count] = '\0';

        *direction = _XmOptDirectionGet(str);
        *separator = FALSE;

        /* 
         * Always set this to true.  Optimized strings only have one
         * segment, so if they try for another, it's an error.
         */        
        _XmStrContError(context) = TRUE;
        return (TRUE);
    }
       /* not optimized */
    else {

        _XmString        str = _XmStrContString(context);
        _XmStringSegment seg;
        _XmStringLine    line;

        if (_XmStrContCurrLine(context) >= _XmStrLineCnt(str))
        {
            _XmStrContError(context) = TRUE;
            return (FALSE);
        }
        else
	  {
            line = &(_XmStrLineLine(str)[_XmStrContCurrLine(context)]);
	    
	    if (_XmStrLineSegCount(line) != 0)
	      {
		seg = &(_XmStrLineSegment(line)[_XmStrContCurrSeg(context)]);

		*charset = XtMalloc(strlen(_XmSegCharset(seg)) + 1);
		memcpy(*charset, _XmSegCharset(seg), 
		       strlen(_XmSegCharset(seg)));
		(*charset)[strlen(_XmSegCharset(seg))] = '\0';

		*char_count = _XmSegCharCount(seg);
		*text = XtMalloc(*char_count + 1);
		memcpy( *text, _XmSegText(seg), *char_count);
		(*text)[*char_count] = '\0';

		*direction = _XmSegDirection(seg);
		*separator = FALSE;

		_XmStrContCurrSeg(context)++;
		if (_XmStrContCurrSeg(context) >= _XmStrLineSegCount(line))
		  {
		    *separator = TRUE;
		    _XmStrContCurrSeg(context) = 0;
		    _XmStrContCurrLine(context)++;
		  }
	      }
	    else /* Empty line, no segments, skip this line. */
	      {
		_XmStrContCurrLine(context)++;
		return(_XmStringGetNextSegment(context, charset, direction, 
					       text, char_count, separator));
	      }	    
	  }
   
    }
    return (TRUE);
}

void 
#ifdef _NO_PROTO
_XmStringFreeContext( context )
        _XmStringContext context ;
#else
_XmStringFreeContext(
        _XmStringContext context )
#endif /* _NO_PROTO */
{
    XtFree((char *) context);
}

/*
 * find the rectangle which will enclose the line
 */
static void 
#ifdef _NO_PROTO
_XmStringOptLineExtent( fontlist, optline, width, height )
        XmFontList fontlist ;
        _XmStringOpt optline ;
        Dimension *width ;
        Dimension *height ;
#else
_XmStringOptLineExtent(
        XmFontList fontlist,
        _XmStringOpt optline,
        Dimension *width,
        Dimension *height )
#endif /* _NO_PROTO */
{
  XmFontListEntry entry;
  short	font_index;
  
  FontListSearch(fontlist, _XmOptCharsetGet(optline), TRUE, &font_index, 
		 &entry) ;

  if (FontListType(entry) == XmFONT_IS_FONT)
    {
      XCharStruct char_return;
      int dir, asc, dsc;
      short bearing;
      XFontStruct *font_struct = (XFontStruct *)FontListFont(entry);
      
      if (font_index >= 0)
	if (two_byte_font(font_struct))
	  {
	    XTextExtents16(font_struct, (XChar2b *)(_XmOptText(optline)),
			   Half(_XmOptCharCount(optline)), &dir, &asc, &dsc,
			   &char_return);
	  }
	else
	  {
	    XTextExtents(font_struct, _XmOptText(optline), 
			 _XmOptCharCount(optline), &dir, &asc, &dsc,
			 &char_return);
	  }
      
      /* Minor hack here for 1.1 visual backward compatibility on
	 certain platforms (OSF1 and Sun). */
      bearing = (char_return.rbearing - char_return.lbearing);
      *width = (char_return.width > bearing) ? char_return.width : bearing;
      *height = asc + dsc;
    }
  else
    {
      XFontSet font_set = (XFontSet)FontListFont(entry);
      XRectangle ink, logical;
      
      XmbTextExtents(font_set, _XmOptText(optline), _XmOptCharCount(optline),
		     &ink, &logical);
	  
#ifdef NON_OSF_FIX
      if(logical.height == 0)
	{ XFontSetExtents *extents = XExtentsOfFontSet(font_set);
 	  logical.height = extents->max_logical_extent.height;
         }
#endif /* NON_OSF_FIX */
      *width = logical.width;
      *height = logical.height;
    }
}
	
/*
 * optimized internal TCS structure handling routines
 */
/*
 * find the ascender for the given optimized line
 */
static Dimension 
#ifdef _NO_PROTO
_XmStringOptLineAscender( f, opt )
        XmFontList f ;
        _XmStringOpt opt ;
#else
_XmStringOptLineAscender(
        XmFontList f,
        _XmStringOpt opt )
#endif /* _NO_PROTO */
{
  Dimension ascent, descent;
  
  _XmStringOptLineMetrics(f, opt, &ascent, &descent);
  
  return(ascent);
}

/*
 * Find the ascent and descent for the given optimized line. 
 */
static void
#ifdef _NO_PROTO
_XmStringOptLineMetrics( f, opt, ascent, descent )
        XmFontList f ;
        _XmStringOpt opt ;
     Dimension	*ascent;
     Dimension	*descent;
#else
_XmStringOptLineMetrics(
        XmFontList f,
        _XmStringOpt opt,
	Dimension	*ascent,
        Dimension	*descent)
#endif /* _NO_PROTO */
{
  short font_index ;
  XmFontListEntry entry;
  
  FontListSearch(f, _XmOptCharsetGet(opt), TRUE, &font_index, &entry) ;

  /* Use the raster extent for a single line. */
  if (font_index >= 0)
    {   
      if (FontListType(entry) == XmFONT_IS_FONT)
	{
	  XFontStruct *font_struct = (XFontStruct *)FontListFont(entry);
	  XCharStruct char_return;
	  int dir, asc, dsc;

	  if (two_byte_font(font_struct))
	    {
	      XTextExtents16(font_struct, (XChar2b *)(_XmOptText(opt)),
			     Half(_XmOptCharCount(opt)), &dir, &asc, &dsc,
			     &char_return);
	    }
	  else
	    {
	      XTextExtents(font_struct, _XmOptText(opt), 
			   _XmOptCharCount(opt), &dir, &asc, &dsc,
			   &char_return);
	    }

	  *ascent = asc;
	  *descent = dsc;
	} 	
      /* Not clear I completly understand this for fontsets, but I think: */
      /* baseline y for fontsets; ascender = -y, descender = height + y */
      else
	{
	  XFontSet font_set = (XFontSet)FontListFont(entry);
	  XRectangle ink, logical;
	    
	  XmbTextExtents(font_set, _XmOptText(opt), _XmOptCharCount(opt),
			 &ink, &logical);
	  
#ifdef NON_OSF_FIX 
          if(logical.height == 0){
    	    XFontSetExtents *extents = XExtentsOfFontSet(font_set);
	    *ascent = -(extents->max_logical_extent.y);
	    *descent = extents->max_logical_extent.height +
		       extents->max_logical_extent.y;
	  }
	  else{
            *ascent = -(logical.y);
            *descent = logical.height + logical.y;
	  }
#else /* NON_OSF_FIX */
	  *ascent = -(logical.y);
	  *descent = logical.height + logical.y;
#endif /* NON_OSF_FIX */
	}
    }
}

/*
 * find the rectangle which will enclose the line
 */
static void 
#ifdef _NO_PROTO
_XmStringLineExtent( fontlist, line, width, height, which )
        XmFontList fontlist ;
        _XmStringLine line ;
        Dimension *width ;
        Dimension *height ;
     int	which;
#else
_XmStringLineExtent(
        XmFontList fontlist,
        _XmStringLine line,
        Dimension *width,
        Dimension *height,
	int	which)
#endif /* _NO_PROTO */
{
  XmFontListEntry entry;
  int i, cur_width = 0, cur_height = 0;
  
  *width = 0, *height = 0;
  
  /* Height of first line is raster ascent plus font descent. */
  /* For middle lines it's font ascent plus descent. */
  /* For last line, it's font ascent plus raster descent. */
  /* Width of first segment is -leftbearing + width. */
  /* Width of last segment is rightbearing. */
  /* Width of single segment is max of width and rightbearing - leftbearing. */
  /* Width of all other segments is width. */
  for (i = 0; i < _XmStrLineSegCount(line); i++)
    {
      _XmStringSegment seg = &(_XmStrLineSegment(line)[i]);
      
      if (_XmSegFontIndex(seg) == -1) _update_segment(fontlist, seg);
      
      entry = &fontlist[_XmSegFontIndex(seg)];
      
      if (FontListType(entry) == XmFONT_IS_FONT)
	{
	  XFontStruct *font_struct = (XFontStruct *)FontListFont(entry);
	  XCharStruct char_return;
	  int dir, asc, dsc;
	  
	  if (two_byte_font(font_struct))
	    {
	      XTextExtents16(font_struct, (XChar2b *)(_XmSegText(seg)),
			     Half(_XmSegCharCount(seg)), &dir, &asc, &dsc,
			     &char_return);
	    }
	  else
	    {
	      XTextExtents(font_struct, _XmSegText(seg), 
			     _XmSegCharCount(seg), &dir, &asc, &dsc,
			     &char_return);
	    }

	  if ((i == 0) && (char_return.lbearing < 0))
	    cur_width = -(char_return.lbearing);
	  else cur_width = 0;
	  
	  if (i == (_XmStrLineSegCount(line) - 1))
	    cur_width += ((char_return.width > char_return.rbearing) ?
	                   char_return.width : char_return.rbearing);
	  else cur_width += char_return.width;

	  switch (which)
	    {
	    case XMSTRING_FIRST_LINE:
	      cur_height = font_struct->ascent + dsc;
	      break;
	    case XMSTRING_MIDDLE_LINE:
	      cur_height = asc + dsc;
	      break;
	    case XMSTRING_LAST_LINE:
	      cur_height = asc + font_struct->descent;
	      break;
	    }
	}
      else
	{
	  XFontSet font_set = (XFontSet)FontListFont(entry);
	  XRectangle ink, logical;

	  XmbTextExtents(font_set, _XmSegText(seg), _XmSegCharCount(seg),
			 &ink, &logical);
	  
	  cur_width = logical.width;
#ifdef NON_OSF_FIX
          if (logical.height == 0){
            XFontSetExtents *extents = XExtentsOfFontSet(font_set);
            logical.height = extents->max_logical_extent.height;
          }
          cur_height = logical.height;
#else /* NON_OSF_FIX */
	  switch (which)
	    {
	    case XMSTRING_FIRST_LINE:
	    case XMSTRING_LAST_LINE:
	    case XMSTRING_MIDDLE_LINE:
	      cur_height = logical.height;
	      break;
	    }
#endif /* NON_OSF_FIX */
	}

      *width += cur_width;
      if (cur_height > *height) *height = cur_height;
    }
}
	
/*
 * internal TCS structure handling routines
 */
/*
 * find biggest ascender in this line for first line
 */
static Dimension 
#ifdef _NO_PROTO
_XmStringFirstLineAscender( f, line )
        XmFontList f ;
        _XmStringLine line ;
#else
_XmStringFirstLineAscender(
        XmFontList f,
        _XmStringLine line )
#endif /* _NO_PROTO */
{
  int i, max = 0;
  
  /* First line uses TextExtents information. */
  for (i=0; i<_XmStrLineSegCount(line); i++)
    {
      _XmStringSegment seg = &(_XmStrLineSegment(line)[i]);

      if (_XmSegFontIndex(seg) == -1) _update_segment (f, seg);

      if (_XmSegFontIndex(seg) >= 0)
	{
	  XmFontListEntry entry;

	  entry = &f[_XmSegFontIndex(seg)];
 
	  if (FontListType(entry) == XmFONT_IS_FONT)
	    {
	      XFontStruct *font_struct = (XFontStruct *) FontListFont(entry);
	      XCharStruct char_return;
	      int dir, asc, dsc;

	      if (two_byte_font(font_struct))
		{
		  XTextExtents16(font_struct, (XChar2b *)(_XmSegText(seg)),
				 Half(_XmSegCharCount(seg)), &dir, &asc, &dsc,
				 &char_return);
		}
	      else
		{
		  XTextExtents(font_struct, _XmSegText(seg), 
			       _XmSegCharCount(seg), &dir, &asc, &dsc,
			       &char_return);
		}
	      
	      if (asc > max) max = asc; 
	    }
	  /* Not clear I completly understand this for fontsets.  I think: */
	  /* baseline y for fontsets; ascender = -y, descender = height+y */
	  else
	    {
	      XFontSet font_set = (XFontSet)FontListFont(entry);
	      XRectangle ink, logical;

	      XmbTextExtents(font_set, _XmSegText(seg), _XmSegCharCount(seg),
			     &ink, &logical);
	  
#ifdef NON_OSF_FIX 
	      if(logical.y == 0){
	        XFontSetExtents *extents = XExtentsOfFontSet(font_set);
	        if(-(extents->max_logical_extent.y) > max)
                  max = -(extents->max_logical_extent.y);
               }
	       else
		{
                 if(-(logical.y) > max) max = -(logical.y);
	       }
#else /* NON_OSF_FIX */
	      if (-(logical.y) > max) max = -(logical.y);
#endif /* NON_OSF_FIX */
	    }
	}
    }
  return (max);
}

/*
 * find biggest ascender in this line for rest of lines
 */
static Dimension 
#ifdef _NO_PROTO
_XmStringLineAscender( f, line )
        XmFontList f ;
        _XmStringLine line ;
#else
_XmStringLineAscender(
        XmFontList f,
        _XmStringLine line )
#endif /* _NO_PROTO */
{
    int i, max = 0;

    /* All other lines use font metric ascent. */
    for (i=0; i<_XmStrLineSegCount(line); i++)
    {
	_XmStringSegment seg = &(_XmStrLineSegment(line)[i]);

	if (_XmSegFontIndex(seg) == -1) _update_segment (f, seg);

	if (_XmSegFontIndex(seg) >= 0)
	{
	    XmFontListEntry entry;

	    entry = &f[_XmSegFontIndex(seg)];
 
	    if (FontListType(entry) == XmFONT_IS_FONT)
	    {
		XFontStruct *font_struct = (XFontStruct *) FontListFont(entry);

		if (font_struct->ascent > max)
		    max = font_struct->ascent;
	    }
	    /* Not clear I completly understand this for fontsets.  I think: */
	    /* baseline y for fontsets; ascender = -y, descender = height+y */
	    else
	    {
		XFontSet font_set = (XFontSet)FontListFont(entry);
#ifdef NON_OSF_FIX
		XRectangle ink, logical;

		XmbTextExtents(font_set, _XmSegText(seg), _XmSegCharCount(seg),
			       &ink, &logical);

		if (logical.y == 0){
		  XFontSetExtents *extents = XExtentsOfFontSet(font_set);
		  logical.y = extents->max_logical_extent.y;
		}
		if (-(logical.y) > max)
		    max = -(logical.y);
#else /* NON_OSF_FIX */
		XFontSetExtents *extents = XExtentsOfFontSet(font_set);

		if (-(extents->max_logical_extent.y) > max)
		    max = -(extents->max_logical_extent.y);
#endif /* NON_OSF_FIX */
	    }
	}
    }
    return (max);
}

/*
 * find biggest descender in this line
 */
static Dimension 
#ifdef _NO_PROTO
_XmStringLineDescender( f, line )
        XmFontList f ;
        _XmStringLine line ;
#else
_XmStringLineDescender(
        XmFontList f,
        _XmStringLine line )
#endif /* _NO_PROTO */
{
    int i, max = 0;

    for (i=0; i<_XmStrLineSegCount(line); i++)
    {
	_XmStringSegment seg = &(_XmStrLineSegment(line)[i]);

	if (_XmSegFontIndex(seg) == -1) _update_segment (f, seg);

	if (_XmSegFontIndex(seg) >= 0)
	{
	    XmFontListEntry entry;

	    entry = &f[_XmSegFontIndex(seg)];

	    if (FontListType(entry) == XmFONT_IS_FONT)
	    {
		XFontStruct *font_struct = (XFontStruct *) FontListFont(entry);

		if (font_struct->descent > max)
		    max = font_struct->descent;
	    }
	    else  
	    /* Baseline y in fontsets; ascender = -y, descender = height + y */
	    {
		XFontSet font_set = (XFontSet)FontListFont(entry);
#ifdef NON_OSF_FIX
		XRectangle ink, logical;

		XmbTextExtents(font_set, _XmSegText(seg), _XmSegCharCount(seg),
			       &ink, &logical);

		if (logical.height == 0){
		  XFontSetExtents *extents = XExtentsOfFontSet(font_set);
		  logical.height = extents->max_logical_extent.height;
		  logical.y = extents->max_logical_extent.y;
		}
		if ((logical.height + logical.y) > max)
		    max = (logical.height + logical.y);
#else /* NON_OSF_FIX */
		XFontSetExtents *extents = XExtentsOfFontSet(font_set);
		XRectangle *extent = &(extents->max_logical_extent);

		if ((extent->height + extent->y) > max)
		    max = (extent->height + extent->y);
#endif /* NON_OSF_FIX */
	    }
	}
    }
    return (max);
}

/*
 * find the total length of this line
 */
static Dimension 
#ifdef _NO_PROTO
_XmStringLineWidth( fontlist, line )
        XmFontList fontlist ;
        _XmStringLine line ;
#else
_XmStringLineWidth(
        XmFontList fontlist,
        _XmStringLine line )
#endif /* _NO_PROTO */
{
  Dimension width = 0, height;
  
  _XmStringLineExtent(fontlist, line, &width, &height, XMSTRING_MIDDLE_LINE);
  
  return(width);
}

/*
 * find width of widest line in internal TCS
 */
Dimension 
#ifdef _NO_PROTO
_XmStringWidth( fontlist, string )
        XmFontList fontlist ;
        _XmString string ;
#else
_XmStringWidth(
        XmFontList fontlist,
        _XmString string )
#endif /* _NO_PROTO */
{
  Dimension width, height;
  _XmStringExtent(fontlist, string, &width, &height);
  return(width);
}

Dimension 
#ifdef _NO_PROTO
_XmStringHeight( fontlist, string )
        XmFontList fontlist ;
        _XmString string ;
#else
_XmStringHeight(
        XmFontList fontlist,
        _XmString string )
#endif /* _NO_PROTO */
{
  Dimension width, height;
  _XmStringExtent(fontlist, string, &width, &height);
  return(height);
}

/*
 * find the rectangle which will enclose the text 
 */
void 
#ifdef _NO_PROTO
_XmStringExtent( fontlist, string, width, height )
        XmFontList fontlist ;
        _XmString string ;
        Dimension *width ;
        Dimension *height ;
#else
_XmStringExtent(
        XmFontList fontlist,
        _XmString string,
        Dimension *width,
        Dimension *height )
#endif /* _NO_PROTO */
{
  Dimension cur_width, max_width = 0, cur_height, line_height = 0;
  int j;
  
  *width = 0, *height = 0;

  if ((fontlist == NULL) || (string == NULL)) return;

  if (_XmStrOptimized(string))
    _XmStringOptLineExtent(fontlist, (_XmStringOpt)string, width, height);
  else 
    {
      int which_line;
      
      /* Height of first line is raster ascent plus font descent. */
      /* For middle lines it's font ascent plus descent. */
      /* For last line, it's font ascent plus raster descent. */
      for (j = 0; j < _XmStrLineCnt(string); j++)
	{
	  _XmStringLine line = &(_XmStrLineLine(string)[j]);
	  
	  if (j == 0) which_line = XMSTRING_FIRST_LINE;
	  else if (j == (_XmStrLineCnt(string) - 1))
	    which_line = XMSTRING_LAST_LINE;
	  else which_line = XMSTRING_MIDDLE_LINE;
	  
	  _XmStringLineExtent(fontlist, line, &cur_width, &cur_height,
			      which_line);
	  
	  /* Returned height for empty lines is zero, so go
	     with previous in that case. */
	  if (_XmStrLineSegCount(line)) line_height = cur_height;
	  *height += line_height;
	  
	  if (cur_width > max_width) max_width = cur_width;
	}
      *width = max_width;
    }
}

Boolean 
#ifdef _NO_PROTO
_XmStringEmpty( string )
        _XmString string ;
#else
_XmStringEmpty(
        _XmString string )
#endif /* _NO_PROTO */
{
    int i, j;

    if (!string) return (TRUE);

    if (_XmStrOptimized(string))
    {
        if (_XmOptCharCount((_XmStringOpt)string) > 0) 
            return FALSE;
    }
    else {
        _XmStringLine  line = _XmStrLineLine(string);

        for (i=0; i<_XmStrLineCnt(string); i++)
        {
            int segcount = _XmStrLineSegCount(&line[i]);
   	    for (j = 0; j < segcount; j++)
       	    {
	        _XmStringSegment seg = 
                   &(_XmStrLineSegment(&line[i])[j]);

	        if (_XmSegCharCount(seg) > 0) return (FALSE);
            }
        }
    }

    return (TRUE);
}

/*
 * figure out if there is sub string match, and if so the begining
 * and end of the match section in pixels.  Don't touch anything if
 * there is no match
 */
static void 
#ifdef _NO_PROTO
_XmStringSubStringPosition( one_byte, entry, seg, under_seg, x, under_begin, under_end )
	Boolean one_byte ;
	XmFontListEntry entry;
	_XmStringSegment seg ;
	_XmStringSegment under_seg ;
	Position x ;
	Dimension *under_begin ;
	Dimension *under_end ;
#else
_XmStringSubStringPosition(
#if NeedWidePrototypes
        int one_byte,
#else
        Boolean one_byte,
#endif /* NeedWidePrototypes */
	XmFontListEntry entry,
        _XmStringSegment seg,
        _XmStringSegment under_seg,
#if NeedWidePrototypes
        int x,
#else
        Position x,
#endif /* NeedWidePrototypes */
        Dimension *under_begin,
        Dimension *under_end )
#endif /* _NO_PROTO */
{
    char *a = _XmSegText(seg), *b = _XmSegText(under_seg);
    int i, j, k, begin, max;
    Boolean fail;

    if (!((_XmSegCharset(seg) == _XmSegCharset(under_seg)) ||
	  ((strcmp(_XmSegCharset(seg), XmFONTLIST_DEFAULT_TAG) == 0) &&
	   _XmStringIsCurrentCharset(_XmSegCharset(under_seg))) ||
	  ((strcmp(_XmSegCharset(under_seg), XmFONTLIST_DEFAULT_TAG) == 0) &&
	   _XmStringIsCurrentCharset(_XmSegCharset(seg)))))
      return;

    if (_XmSegCharCount(seg) < _XmSegCharCount(under_seg)) return;

    max = (_XmSegCharCount(seg) - _XmSegCharCount(under_seg));
      
    if (FontListType(entry) == XmFONT_IS_FONT)
      {
        XFontStruct *font_struct = (XFontStruct *)FontListFont(entry);
       
        if (one_byte)
        {
            for (i = 0; i <= max; i++)
            {
                fail = FALSE;
                begin = i;

                for (j = 0; j < _XmSegCharCount(under_seg); j++)
                {
                    if (a[i+j] != b[j]) 
                    {
                        fail = TRUE;
                        break;
                    }
                }
                if ( ! fail)      /* found it */
                {
                    if (begin == 0)
                        *under_begin = x;
                    else
                        *under_begin = 
                             x + abs(XTextWidth (font_struct, a, begin));
 
                    if (_XmSegPixelWidth(under_seg) == 0)
                        _XmSegPixelWidth(under_seg) = 
                            abs(XTextWidth (font_struct, b,
                                            _XmSegCharCount(under_seg)));
 
                    *under_end = *under_begin + _XmSegPixelWidth(under_seg);
 
                    return;
                }
            }
        }
        else
        {
            /*
             * If either string isn't even byte length, it can't be
             * two bytes/char.
             */
 
            if (((_XmSegCharCount(seg) % 2) != 0) || 
                ((_XmSegCharCount(under_seg) % 2) != 0))
            return;
 
            /*
             * search for the substring
             */
 
            for (i = 0; i <= max; i+=2)
            {
                fail = FALSE;
                begin = i;
 
                for (j = 0; j < _XmSegCharCount(under_seg); j+=2)
                {
                    if ((a[i+j] != b[j]) || (a[i+j+1] != b[j+1]))
                    {
                        fail = TRUE;
                        break;
                    }
                }
                if ( ! fail)      /* found it */
                {
                    if (begin == 0)
                        *under_begin = x;
                    else
                        *under_begin = 
                             x + abs(XTextWidth16 (font_struct, (XChar2b *) a, 
                                                   begin/2));
  
                    if (_XmSegPixelWidth(under_seg) == 0)
                        _XmSegPixelWidth(under_seg) = 
                             abs(XTextWidth16 (font_struct, (XChar2b *) b, 
					       _XmSegCharCount(under_seg)/2));
  
                    *under_end = *under_begin + _XmSegPixelWidth(under_seg);
  
                    return;
                }
            }
        }
    }
    else
    {
	XFontSet font_set = (XFontSet)FontListFont(entry);
	int len_a, len_b;
  
	for (i = 0; i <= max; i += len_a)
	{
	    fail = FALSE;
	    begin = i;

	    for (j = 0; j < _XmSegCharCount(under_seg); j += len_b)
            {
		len_a = mblen(&a[i+j], MB_CUR_MAX);
		if (len_a < 1) return;

		len_b = mblen(&b[j], MB_CUR_MAX);
		if (len_b < 1) return;

		if (len_b == len_a)
                {
		    for (k = 0; k < len_b; k++)
		    {
			if (a[i+j+k] != b[j+k])
			{
			    fail = TRUE;
			    break;
			}
		    }
		    if (fail == TRUE) break;
		}
		else
		{
		    fail = TRUE;
		    break;
		}
	    }

	    if (!fail)            /* found it */
	    {
		if (begin == 0) *under_begin = x;
		else *under_begin =
		    x + abs(XmbTextEscapement(font_set, a, begin));

		if (_XmSegPixelWidth(under_seg) == 0)
		    _XmSegPixelWidth(under_seg) =
			abs(XmbTextEscapement(font_set, b, 
			                      _XmSegCharCount(under_seg)));
  
		*under_end = *under_begin + _XmSegPixelWidth(under_seg);

		return;
	    }
 	}
    }
}

/*
 * draw a single internal TCS segment
 */
static void 
#ifdef _NO_PROTO
_XmStringDrawSegment( d, w, x, y, seg, gc, fontlist, image, underline, under_begin, under_end )
        Display *d ;
        Window w ;
        int x ;
        int y ;
        _XmStringSegment seg ;
        GC gc ;
        XmFontList fontlist ;
        Boolean image ;
        _XmString underline ;
        Dimension *under_begin ;
        Dimension *under_end ;
#else
_XmStringDrawSegment(
        Display *d,
        Window w,
        int x,
        int y,
        _XmStringSegment seg,
        GC gc,
        XmFontList fontlist,
#if NeedWidePrototypes
        int image,
#else
        Boolean image,
#endif /* NeedWidePrototypes */
        _XmString underline,
        Dimension *under_begin,
        Dimension *under_end )
#endif /* _NO_PROTO */
{
    XmFontListEntry entry;
    Boolean text16 = False, multibyte;
    Font    oldfont = (Font) 0;
    XGCValues xgcv;
    char *save_text = seg->text;       /* this is slimy */
    char  flip_char[100];              /* but simple */
    char *flip_char_extra = NULL;

    if (_XmSegFontIndex(seg) == -1) _update_segment (fontlist, seg);

    if ((_XmSegFontIndex(seg) == -1) || (_XmSegCharCount(seg) == 0)) return;

    entry = &fontlist[_XmSegFontIndex(seg)];
  
    multibyte = (FontListType(entry) == XmFONT_IS_FONTSET);
    
    if (!multibyte)
      {
        XFontStruct *f = (XFontStruct *)FontListFont(entry);
        XGCValues current_gcv;

	text16 = two_byte_font (f);

	XGetGCValues (d, gc, GCFont, &current_gcv) ;

	xgcv.font = f->fid;	/* get segment font */

	if (current_gcv.font != xgcv.font)	/* not right one */
	{					/* change it */
            oldfont = current_gcv.font;
            XChangeGC (d, gc, GCFont, &xgcv);
	}
    }

    if (_XmSegDirection(seg) == XmSTRING_DIRECTION_R_TO_L)
      {
      /* Flip the bytes. */
      char *p = flip_char, *q;
      int i, j;
      if (_XmSegCharCount(seg) > 100) 
        p = flip_char_extra = (char *) ALLOCATE_LOCAL (_XmSegCharCount(seg));
      
      _XmSegText(seg) =  p;   /* change seg for a while */
  
      if (multibyte) /* Have to flip a mb character at time. */
      {
        int   len;
       
        for (i = 0, q = save_text, p += _XmSegCharCount(seg); 
             i < _XmSegCharCount(seg); 
             i += len)
        {
            len = mblen(q, MB_CUR_MAX);
            if (len < 1) /* Something went wrong, just return for now. */
              return;
            
            p -= len;
            for (j = 0; j < len; j++)
              {
                p[j] = q[j];
              }
            q += len;
        }
      }
      else if (!text16)
        {
          for (i=0, q = (save_text + _XmSegCharCount(seg) - 1); 
               i < _XmSegCharCount(seg); 
               i++) 
            *p++ = *q--;
        }
      else /* Have to flip two at a time, maintaining their order. */
        {
          char tmp;
          
          for (i=0, q = (save_text + _XmSegCharCount(seg) - 1); 
               i < Half (_XmSegCharCount(seg)); 
               i++) 
            {
              tmp = *q--;
              *p++ = *q--;
              *p++ = tmp;
            }
        }
      }
  
    if ((underline != (_XmString) NULL) && (*under_begin == *under_end))
    {

        if (_XmStrOptimized(underline))
        {

            /*
             * This is an optimized string; build a segment and call the
	     * sub-string search routine.
             */

            _XmStringSegmentRec  under_seg;
            _XmStringOpt         opt = (_XmStringOpt) underline ;
            short                font_index = -1 ;
	    XmFontListEntry      fl_entry;

	    _XmSegCharset(&under_seg) = _XmOptCharsetGet(opt);
            if(    _XmOptWidthUpdated( opt)    )
            {   FontListSearch( fontlist, _XmOptCharsetGet(opt), TRUE,
                            &font_index, &fl_entry) ;
                } 
            _XmSegFontIndex(&under_seg) = font_index ;
            _XmSegCharCount(&under_seg) = _XmOptCharCount(opt);
            _XmSegText(&under_seg) = _XmOptText(opt);
            _XmSegDirection(&under_seg) = _XmOptDirectionGet(opt);
            _XmSegPixelWidth(&under_seg) = _XmOptPixelWidth(opt);
	    _XmStringSubStringPosition ((!text16), entry, seg, &under_seg, 
                                        x, under_begin, under_end);
        }
        else {
            _XmStringLine line = _XmStrLineLine(underline);
	    if ((_XmStrLineCnt(underline) > 0) && 
	        (_XmStrLineSegCount(&line[0]) > 0))
	    {
	        _XmStringSegment under_seg =  _XmStrLineSegment(line);

		_XmStringSubStringPosition ((!text16), entry, seg, under_seg,
					    x, under_begin, under_end);
	    }
        }
    }
  
    if (image)
      {
	if (text16) XDrawImageString16 (d, w, gc, x, y, 
					(XChar2b *)(_XmSegText(seg)), 
					Half (_XmSegCharCount(seg)));
	else if (multibyte) 
	  XmbDrawImageString (d, w, (XFontSet)FontListFont(entry), gc, x, y,
			      _XmSegText(seg), _XmSegCharCount(seg));
        else XDrawImageString (d, w, gc, x, y, 
			       _XmSegText(seg), _XmSegCharCount(seg));
      }
    else
      {
	if (text16) 
	  XDrawString16 (d, w, gc, x, y, (XChar2b *)(_XmSegText(seg)), 
			 Half (_XmSegCharCount(seg)));
	else if (multibyte)
	  XmbDrawString (d, w, (XFontSet)FontListFont(entry), gc, x, y,
			 _XmSegText(seg), _XmSegCharCount(seg));
        else 
	  XDrawString (d, w, gc, x, y, _XmSegText(seg), _XmSegCharCount(seg));
      }

    if ((Font)0 != oldfont)                     /* if font was changed */
    {                                           /* put it back */
        xgcv.font = oldfont;
        XChangeGC (d, gc, GCFont, &xgcv);
    }

    _XmSegText(seg) = save_text;			/* put seg back */

    if (flip_char_extra  != NULL) DEALLOCATE_LOCAL (flip_char_extra);
}

/* 
 * Draw a single internal TCS line
 */
static void 
#ifdef _NO_PROTO
_XmStringDrawLine( d, w, x, y, line, gc, fontlist, image, underline, under_begin, under_end, opt )
        Display *d ;
        Window w ;
        int x ;
        int y ;
        _XmStringLine line ;
        GC gc ;
        XmFontList fontlist ;
        Boolean image ;
        _XmString underline ;
        Dimension *under_begin ;
        Dimension *under_end ;
        Boolean opt ;
#else
_XmStringDrawLine(
        Display *d,
        Window w,
        int x,
        int y,
        _XmStringLine line,
        GC gc,
        XmFontList fontlist,
#if NeedWidePrototypes
        int image,
#else
        Boolean image,
#endif /* NeedWidePrototypes */
        _XmString underline,
        Dimension *under_begin,
        Dimension *under_end,
#if NeedWidePrototypes
        int opt )
#else
        Boolean opt )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    int i;

    if (opt)
    {
        /*
         * This is optimized; build a full segment and call the drawing
	 * routine.
         */

        _XmStringOpt         optline = (_XmStringOpt) line;
        short                font_index = -1 ;
	XmFontListEntry      entry;
        _XmStringSegmentRec  segm;

        _XmSegCharset(&segm) = _XmOptCharsetGet(optline);
        if(    _XmOptWidthUpdated( optline)    )
        {   FontListSearch( fontlist, _XmOptCharsetGet(optline), TRUE,
                        &font_index, &entry) ;
            } 
        _XmSegFontIndex(&segm) = font_index ;
        _XmSegCharCount(&segm) = _XmOptCharCount(optline);
        _XmSegText(&segm) = _XmOptText(optline);
        _XmSegDirection(&segm) = _XmOptDirectionGet(optline);
        _XmSegPixelWidth(&segm) = _XmOptPixelWidth(optline);

	_XmStringDrawSegment (d, w, x, y, &segm, gc, fontlist, image,
		underline, under_begin, under_end);

    }
    else {
        for (i=0; i<_XmStrLineSegCount(line); i++)
        {
            _XmStringSegment seg;

	    seg = &(_XmStrLineSegment(line)[i]);

	    _XmStringDrawSegment (d, w, x, y, seg, gc, fontlist, image,
		underline, under_begin, under_end);

	    x += _XmSegPixelWidth(seg);
        }
    }
}

/*
 * calculate the alignment, position and clipping for the string
 */
static void 
#ifdef _NO_PROTO
_calc_align_and_clip( d, gc, x, y, width, line_width, lay_dir, clip, align, descender, restore )
        Display *d ;
        GC gc ;
        Position *x ;
        Position y ;
        Dimension width ;
        int line_width ;
        unsigned char lay_dir ;
        XRectangle *clip ;
        unsigned char align ;
        int descender ;
        int *restore ;
#else
_calc_align_and_clip(
        Display *d,
        GC gc,
        Position *x,
#if NeedWidePrototypes
        int y,
        int width,
#else
        Position y,
        Dimension width,
#endif /* NeedWidePrototypes */
        int line_width,
#if NeedWidePrototypes
        unsigned int lay_dir,
#else
        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
        XRectangle *clip,
#if NeedWidePrototypes
        unsigned int align,
#else
        unsigned char align,
#endif /* NeedWidePrototypes */
        int descender,
        int *restore )
#endif /* _NO_PROTO */
{

    Boolean l_to_r = (lay_dir == XmSTRING_DIRECTION_L_TO_R);


    switch (align)
    {
    	case XmALIGNMENT_BEGINNING:
	    if ( ! l_to_r) *x += width - line_width;
	    break;

    	case XmALIGNMENT_CENTER:
	    *x += Half (width) - Half (line_width);
	    break;

    	case XmALIGNMENT_END :
	    if (l_to_r)
	    	*x += width - line_width;
	    break;
   }

    if ((clip != NULL) && ( ! *restore))

/* BEGIN OSF Fix CR 5106 */
        if ((line_width > clip->width) ||
/* END OSF Fix CR 5106 */
	     (y + descender) > (clip->y + clip->height))
	{
	    *restore = TRUE;
            XSetClipRectangles (d, gc, 0, 0, clip, 1, YXBanded);
	}

}

/*
 * draw a complete internal format TCS
 */
static void 
#ifdef _NO_PROTO
_draw( d, w, fontlist, string, gc, x, y, width, align, lay_dir, clip, image, underline )
        Display *d ;
        Window w ;
        XmFontList fontlist ;
        _XmString string ;
        GC gc ;
        Position x ;
        Position y ;
        Dimension width ;
        unsigned char align ;
        unsigned char lay_dir ;
        XRectangle *clip ;
        Boolean image ;
        _XmString underline ;
#else
_draw(
        Display *d,
        Window w,
        XmFontList fontlist,
        _XmString string,
        GC gc,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        unsigned int align,
        unsigned int lay_dir,
#else
        Position x,
        Position y,
        Dimension width,
        unsigned char align,
        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
        XRectangle *clip,
#if NeedWidePrototypes
        int image,
#else
        Boolean image,
#endif /* NeedWidePrototypes */
        _XmString underline )
#endif /* _NO_PROTO */
{
  Position base_x = x, draw_x;
  Dimension line_width, ascender = 0, descender = 0;
  static _XmStringLine line;
  int i;
  int restore_clip = FALSE;
  Dimension under_begin = 0, under_end = 0;

  if (!string) return;
  
  if (_XmStrOptimized(string))
    {   
	_XmStringOptLineMetrics(fontlist, (_XmStringOpt) string,
				&ascender, &descender);
	y += ascender;
	
        if (!_XmOptWidthUpdated(string))
          {
            _update_opt(fontlist, (_XmStringOpt)string, (XmFontListEntry)NULL);
          } 
        line_width = _XmOptPixelWidth( string) ;
        if (line_width != 0)
          {   
            draw_x = base_x ; /* most left position */
            _calc_align_and_clip( d, gc, &draw_x, y, width, line_width, 
                                lay_dir, clip, align, descender, 
                                &restore_clip) ;
            _XmStringDrawLine( d, w, draw_x, y, (_XmStringLine) string, 
                             gc, fontlist, image, 
                             underline, &under_begin, &under_end, TRUE);
          }
        y += descender ;      /* go to bottom of this line */

        if(    (underline != NULL) && (under_begin != under_end)    )
          {   XDrawLine( d, w, gc, under_begin, y, under_end, y) ;
          } 
    }
  else {
    for (i=0; i < _XmStrLineCnt(string); i++)
      {
      line = &(_XmStrLineLine(string)[i]);
      /* baseline, ascent, and descent of this line */
      if (_XmStrLineSegCount(line))
        {
          ascender = (i == 0) ? _XmStringFirstLineAscender (fontlist, line) 
	                      : _XmStringLineAscender (fontlist, line);

          descender = _XmStringLineDescender (fontlist, line);
        } /* Else whatever they were previously. */
      y += ascender;

      line_width = _XmStringLineWidth (fontlist, line);

      if (line_width != 0)
        {
          draw_x = base_x;    /* most left position */

          _calc_align_and_clip(d, gc, &draw_x, y, width, line_width, 
                               lay_dir, clip, align, descender, &restore_clip);

          _XmStringDrawLine (d, w, draw_x, y, line, gc, fontlist, image,
                             underline, &under_begin, &under_end,FALSE);
        }

      y += descender;         /* go to bottom of this line */

      if ((underline != NULL) && (under_begin != under_end))
      {
          underline = (_XmString) NULL; /* only once */

          XDrawLine (d, w, gc, under_begin, y, under_end, y);
      }
    }
  }
  if (restore_clip) XSetClipMask (d, gc, None); 
}
  
void 
#ifdef _NO_PROTO
_XmStringDraw( d, w, fontlist, string, gc, x, y, width, align, lay_dir, clip )
        Display *d ;
        Window w ;
        XmFontList fontlist ;
        _XmString string ;
        GC gc ;
        Position x ;
        Position y ;
        Dimension width ;
        unsigned char align ;
        unsigned char lay_dir ;
        XRectangle *clip ;
#else
_XmStringDraw(
        Display *d,
        Window w,
        XmFontList fontlist,
        _XmString string,
        GC gc,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        unsigned int align,
        unsigned int lay_dir,
#else
        Position x,
        Position y,
        Dimension width,
        unsigned char align,
        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
        XRectangle *clip )
#endif /* _NO_PROTO */
{
    _draw (d, w, fontlist, string, gc, x, y, width, 
	align, lay_dir, clip, FALSE, NULL);
}

void 
#ifdef _NO_PROTO
_XmStringDrawImage( d, w, fontlist, string, gc, x, y, width, align, lay_dir, clip )
        Display *d ;
        Window w ;
        XmFontList fontlist ;
        _XmString string ;
        GC gc ;
        Position x ;
        Position y ;
        Dimension width ;
        unsigned char align ;
        unsigned char lay_dir ;
        XRectangle *clip ;
#else
_XmStringDrawImage(
        Display *d,
        Window w,
        XmFontList fontlist,
        _XmString string,
        GC gc,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        unsigned int align,
        unsigned int lay_dir,
#else
        Position x,
        Position y,
        Dimension width,
        unsigned char align,
        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
        XRectangle *clip )
#endif /* _NO_PROTO */
{
    _draw (d, w, fontlist, string, gc, x, y, width, 
	align, lay_dir, clip, TRUE, NULL);
}

void 
#ifdef _NO_PROTO
_XmStringDrawUnderline( d, w, f, s, gc, x, y, width, align, lay_dir, clip, u )
        Display *d ;
        Window w ;
        XmFontList f ;
        _XmString s ;
        GC gc ;
        Position x ;
        Position y ;
        Dimension width ;
        unsigned char align ;
        unsigned char lay_dir ;
        XRectangle *clip ;
        _XmString u ;
#else
_XmStringDrawUnderline(
        Display *d,
        Window w,
        XmFontList f,
        _XmString s,
        GC gc,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        unsigned int align,
        unsigned int lay_dir,
#else
        Position x,
        Position y,
        Dimension width,
        unsigned char align,
        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
        XRectangle *clip,
        _XmString u )
#endif /* _NO_PROTO */
{
    _draw (d, w, f, s, gc, x, y, width, 
	align, lay_dir, clip, FALSE, u);
}

void 
#ifdef _NO_PROTO
_XmStringDrawMnemonic( d, w, fontlist, string, gc, x, y, width, align, lay_dir, clip, mnemonic, charset )
        Display *d ;
        Window w ;
        XmFontList fontlist ;
        _XmString string ;
        GC gc ;
        Position x ;
        Position y ;
        Dimension width ;
        unsigned char align ;
        unsigned char lay_dir ;
        XRectangle *clip ;
        String mnemonic ;
        XmStringCharSet charset ;
#else
_XmStringDrawMnemonic(
        Display *d,
        Window w,
        XmFontList fontlist,
        _XmString string,
        GC gc,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        unsigned int align,
        unsigned int lay_dir,
#else
        Position x,
        Position y,
        Dimension width,
        unsigned char align,
        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
        XRectangle *clip,
        String mnemonic,
        XmStringCharSet charset )
#endif /* _NO_PROTO */
{
    
    XmString  mne_string;
    _XmString underline;
 
    mne_string = XmStringCreate(mnemonic, charset);
    underline = _XmStringCreate(mne_string);
    XmStringFree(mne_string);
 
    _draw (d, w, fontlist, string, gc, x, y, width, 
	align, lay_dir, clip, FALSE, underline);
    _XmStringFree(underline);
}

/*
 * build the internal TCS given the external TCS
 */
static void 
#ifdef _NO_PROTO
new_segment( string, line_index, value )
        _XmString string ;
        int line_index ;
        _XmStringSegment value ;
#else
new_segment(
        _XmString string,
        int line_index,
        _XmStringSegment value )
#endif /* _NO_PROTO */
{
    _XmStringLine line = &(_XmStrLineLine(string)[line_index]);
    _XmStringSegment seg;
    int sc = _XmStrLineSegCount(line);

    _XmStrLineSegment(line) = 
          (_XmStringSegment) XtRealloc ((char *) _XmStrLineSegment(line), 
				sizeof (_XmStringSegmentRec) * (sc+1));

    seg = &(_XmStrLineSegment(line)[sc]);

    *seg = *value;

    _XmStrLineSegCount(line)++;
}

static void 
#ifdef _NO_PROTO
new_line( string )
        _XmString string ;
#else
new_line(
        _XmString string )
#endif /* _NO_PROTO */
{
    int lc = _XmStrLineCnt(string);

    _XmStrLineLine(string) = (_XmStringLine) 
		XtRealloc ((char *) _XmStrLineLine(string), 
                sizeof (_XmStringLineRec) * (lc + 1));

    _XmStrLineSegCount(&(_XmStrLineLine(string)[lc])) = 0;
    _XmStrLineSegment(&(_XmStrLineLine(string)[lc])) = NULL;

    _XmStrLineCnt(string)++;
}


#define _init_segment(seg)				\
{							\
    _XmSegFontIndex(seg)   = -1;			\
    _XmSegCharCount(seg)   = 0;				\
    _XmSegText(seg)         = NULL;			\
    _XmSegPixelWidth(seg)  = 0;				\
}

static _XmString 
#ifdef _NO_PROTO
_XmStringOptCreate( c, end, textlen, havecharset, charset_index )
        unsigned char *c ;
        unsigned char *end ;
        unsigned short textlen ;
        Boolean havecharset ;
        unsigned int charset_index ;
#else
_XmStringOptCreate(
        unsigned char *c,
        unsigned char *end,
#if NeedWidePrototypes
        unsigned int textlen,
        int havecharset,
#else
        unsigned short textlen,
        Boolean havecharset,
#endif /* NeedWidePrototypes */
        unsigned int charset_index )
#endif /* _NO_PROTO */
{
    _XmStringOpt  string;
    char          *charset = NULL;
    unsigned short        length;

    string = (_XmStringOpt) XtMalloc( sizeof( _XmStringOptRec)
                                            + textlen - TEXT_BYTES_IN_STRUCT) ;
    _XmStrOptimized(string) = TRUE;
    _XmOptCharCount(string) = textlen;
    _XmOptWidthUpdated(string) = FALSE ;
    _XmOptPixelWidth(string) = 0;
    if (havecharset)
    {   
        _XmOptCharsetIndex(string) = charset_index ;
    } 
    else
    {
         charset = XmFONTLIST_DEFAULT_TAG;
         _XmOptCharsetIndex(string) = _index_cache_charset((char *) charset, 
                                                strlen(charset));
    }
    _XmOptDirectionSet( string, XmSTRING_DIRECTION_L_TO_R) ;

    while (c < end)
    {
	length = _read_asn1_length (c);

	switch (*c)
	{
	    case XmSTRING_COMPONENT_TEXT:
            case XmSTRING_COMPONENT_LOCALE_TEXT:
	 	memcpy( _XmOptText(string), (c + _asn1_size(length)), textlen);
		break;

	    case XmSTRING_COMPONENT_DIRECTION:		/* record dir */
		_XmOptDirectionSet( string, ((XmStringDirection) 
					     *(c + _asn1_size(length)))) ;
		break;

	    case XmSTRING_COMPONENT_SEPARATOR:		/* start new line */
                XtFree((char *) string);
                return (NULL);
		/* break; */

	    default:
		break;
	}

	c += length + _asn1_size(length);
    }

/*    if (!havecharset)
       XtFree(charset);
*/
    return((_XmString) string);
}

static _XmString 
#ifdef _NO_PROTO
_XmStringNonOptCreate( c, end, havecharset )
        unsigned char *c ;
        unsigned char *end ;
        Boolean havecharset ;
#else
_XmStringNonOptCreate(
        unsigned char *c,
        unsigned char *end,
#if NeedWidePrototypes
        int havecharset )
#else
        Boolean havecharset )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{

    int lc;
    _XmStringSegmentRec seg;
    unsigned short length;
    _XmString string ;
    char *charset = NULL;

    string = (_XmString) XtMalloc (sizeof (_XmStringRec));

    _XmStrOptimized(string) = FALSE;
    _XmStrLineCnt(string) = 0;			/* init root */
    _XmStrLineLine(string) = (_XmStringLine) NULL;

    new_line (string);

    _init_segment (&seg);

    if (!havecharset)
    {
       charset = XmFONTLIST_DEFAULT_TAG;
       _XmSegCharset(&seg) = _cache_charset((char *) (charset), 
                                            (int) strlen(charset));
    }  
    _XmSegDirection(&seg) = XmSTRING_DIRECTION_L_TO_R;

    lc = 0;

    while (c < end)
    {
	length = _read_asn1_length (c);

	switch (*c)
	{
	    case XmSTRING_COMPONENT_CHARSET:
		_XmSegCharset(&seg) = _cache_charset 
		  ((char *) (c + _asn1_size(length)), (int) length);
		break;

            case XmSTRING_COMPONENT_LOCALE_TEXT:
                _XmSegCharset(&seg) = 
                  _cache_charset((char *) XmFONTLIST_DEFAULT_TAG,
                                 strlen(XmFONTLIST_DEFAULT_TAG));
            /* Fall through to regular text. */
	    case XmSTRING_COMPONENT_TEXT:
		_XmSegText(&seg) = XtMalloc (length);
	 	memcpy( _XmSegText(&seg), (c + _asn1_size(length)), length);

		_XmSegCharCount(&seg) = length;

/****************
 *
 * Why are they doing this?? It forces the text to be last.
 *
 ****************/
		new_segment (string, lc, &seg);		/* alloc new one and */
							/* copy static to it */
		_init_segment (&seg);  
		break;

	    case XmSTRING_COMPONENT_DIRECTION:		/* record dir */
		_XmSegDirection(&seg) = 
		  (XmStringDirection) *(c + _asn1_size(length));
		break;

	    case XmSTRING_COMPONENT_SEPARATOR:		/* start new line */
		new_line (string);
		lc++;
		break;

	    default:
		break;
	}

	c += length + _asn1_size(length);
    }

    return(string);
}

_XmString 
#ifdef _NO_PROTO
_XmStringCreate( cs )
        XmString cs ;
#else
_XmStringCreate(
        XmString cs )
#endif /* _NO_PROTO */
{
    unsigned char       *c;
    unsigned char       *c_opt;
    unsigned char       *end;
    unsigned short      length;
    unsigned short      txtlength;
    _XmString   	string ;
    Boolean     	continue_flag;
    Boolean     	optimized;
    Boolean     	havecharset;
    unsigned int 	charset_index = 0;
    Boolean 		asn1;
 
    if (!cs) return((_XmString) NULL);
    if (!_XmStringIsXmString(cs)) return ((_XmString) NULL);

    /* If cs isn't an asn.1 conformant string, convert it first. */
    asn1 = _is_asn1(cs);
    if (!asn1) cs = XmStringCopy(cs);
    
    c  = (unsigned char *) _read_header((unsigned char *) cs);
    end = c + _read_string_length ((unsigned char *) cs);
    if (c >= end) return ((_XmString) NULL);
   
    /*
     * In order to build an optimized string, we have to see if this one
     * qualifies.  Do some preprocessing to see.
     * We also need to know if this CS contains a character set component,
     * so look for that too.
     */

    c_opt = c;
    continue_flag = TRUE;
    optimized = TRUE;
    txtlength = 0;		/* For strings with no text component. */
    havecharset = FALSE;
    while (continue_flag)
    {
	length = _read_asn1_length (c_opt);

	switch (*c_opt)
        {
        case XmSTRING_COMPONENT_LOCALE_TEXT:
          /* Check the charset. */
          charset_index = 
            _index_cache_charset((char *)XmFONTLIST_DEFAULT_TAG,
                                 strlen(XmFONTLIST_DEFAULT_TAG));
          havecharset = TRUE;
          if (charset_index >= (1 << CHARSET_INDEX_BITS))
            {
              optimized = FALSE;
              txtlength = length;
              break;
            }
          /* Else fall through to text case. */
        case XmSTRING_COMPONENT_TEXT:
          if (((c_opt + length + _asn1_size(length)) < end) || 
              (length >= (1 << CHAR_COUNT_BITS)))
            {
              optimized = FALSE;
            } 
          txtlength = length;
          break;

        case XmSTRING_COMPONENT_SEPARATOR: /* start new line */
          optimized = FALSE;
          break;

        case XmSTRING_COMPONENT_CHARSET:
          charset_index = _index_cache_charset 
            ((char *) (c_opt + _asn1_size(length)), (int) length);
          if(    charset_index >= (1 << CHARSET_INDEX_BITS)    )
            {
              optimized = FALSE ;
            } 
          havecharset = TRUE;
          break;

        default:
          break;
        }

	c_opt += length + _asn1_size(length);
        if ((c_opt >= end) || (!optimized))
           continue_flag = FALSE;
    }

    if (optimized) string = (_XmString)
      _XmStringOptCreate(c, end, txtlength, havecharset, charset_index);
    else string = _XmStringNonOptCreate(c, end, havecharset);   

    if (!asn1) XmStringFree(cs);
    
    return (string);
}

/*
 * free the TCS internal data structure
 */
void 
#ifdef _NO_PROTO
_XmStringFree( string )
        _XmString string ;
#else
_XmStringFree(
        _XmString string )
#endif /* _NO_PROTO */
{
    int i, j;

    if (!string) return;

    if (!_XmStrOptimized(string))
    {
        _XmStringLine line = _XmStrLineLine(string);

        for (i=0; i<_XmStrLineCnt(string); i++)
        {
            _XmStringSegment seg = _XmStrLineSegment(&line[i]);
            int              segcount = _XmStrLineSegCount(&line[i]);

	    for (j = 0; j < segcount; j++)
                XtFree (_XmSegText(&seg[j]));
            XtFree( (char *) seg);
        }
        XtFree ((char *) _XmStrLineLine(string));
    }
    XtFree ((char *) string);
}

void 
#ifdef _NO_PROTO
XmStringFree( string )
        XmString string ;
#else
XmStringFree(
        XmString string )
#endif /* _NO_PROTO */
{
    XtFree ((char *) string);
}

static void 
#ifdef _NO_PROTO
_update_opt( fontlist, optline, entry )
	XmFontList fontlist ;
	_XmStringOpt optline ;
	XmFontListEntry entry;
#else
_update_opt(
	XmFontList fontlist,
	_XmStringOpt optline,
	XmFontListEntry entry )
#endif /* _NO_PROTO */
{
    short           font_index = 0 ;
    XmFontListEntry new_fl_entry;
    
    if (!entry)
    {   
      FontListSearch (fontlist, _XmOptCharsetGet(optline), TRUE,
		      &font_index, &new_fl_entry);
    } 
    else new_fl_entry = entry;
    
    if (font_index < 0)
    {   _XmOptWidthUpdated( optline) = FALSE ;
        _XmOptPixelWidth(optline) = 0;
    } 
    else
    {   _XmOptWidthUpdated( optline) = TRUE ;

        if (FontListType(new_fl_entry) == XmFONT_IS_FONT)
        {
            XFontStruct *font_struct = (XFontStruct *)FontListFont(new_fl_entry);
	    XCharStruct char_return;
	    int dir, asc, dsc;

            if (two_byte_font (font_struct))
            {
                if (_XmOptCharCount(optline) < 2)
                    _XmOptPixelWidth(optline) = 0;
                else
		  {
		    XTextExtents16(font_struct,
				   (XChar2b *)(_XmOptText(optline)), 
				   Half (_XmOptCharCount(optline)),
				   &dir, &asc, &dsc, &char_return);

		    _XmOptPixelWidth(optline) = 
		      abs(char_return.rbearing - char_return.lbearing);
		
		    /* pir 2967 */
		    if (_XmOptPixelWidth(optline) == 0)
		      _XmOptPixelWidth(optline) = 
			Half(_XmOptCharCount(optline)) *
			  (font_struct->max_bounds.width);
		  }
	      }	
            else
	      {
                if (_XmOptCharCount(optline) < 1)
		  _XmOptPixelWidth(optline) = 0;
                else
		  {
		    short	bearing;
		    
		    XTextExtents(font_struct,_XmOptText(optline),
				 _XmOptCharCount(optline),
				 &dir, &asc, &dsc, &char_return);
		    
		    /* Minor hack here for 1.1 visual backward compatibility on
		       certain platforms (OSF1 and Sun). */
		    bearing = abs(char_return.rbearing - char_return.lbearing);
		    _XmOptPixelWidth(optline) = (char_return.width > bearing) ?
		      char_return.width : bearing;
		    
		    /* pir 2967 */
		    if (_XmOptPixelWidth(optline) == 0)
		      _XmOptPixelWidth(optline) = _XmOptCharCount(optline) *
			(font_struct->max_bounds.width);
		  }
	      }
        }
        else
        {
	  if (_XmOptCharCount(optline) < 1) _XmOptPixelWidth(optline) = 0;
	  else 
	    /* Baseline y in fontsets; ascender = -y, descender = height + y */
	    {
	      XFontSet font_set = (XFontSet)FontListFont(new_fl_entry);
	      XRectangle ink, logical;

	      XmbTextExtents(font_set, _XmOptText(optline),
			      _XmOptCharCount(optline),
			      &ink, &logical);
	      _XmOptPixelWidth(optline) = abs(logical.width);
	    }
        }
      }
  }

static void 
#ifdef _NO_PROTO
_update_segment( fontlist, seg )
        XmFontList fontlist ;
        _XmStringSegment seg ;
#else
_update_segment(
        XmFontList fontlist,
        _XmStringSegment seg )
#endif /* _NO_PROTO */
{
    XmFontListEntry entry;

    if (_XmSegFontIndex(seg) == -1)
    {
        FontListSearch (fontlist, _XmSegCharset(seg), TRUE,
                        &(_XmSegFontIndex(seg)), &entry);
    }

    if (_XmSegFontIndex(seg) != -1)
    {
        if (FontListType(entry) == XmFONT_IS_FONT)
        {
            XFontStruct *font_struct = (XFontStruct *)FontListFont(entry);

            if (two_byte_font (font_struct))
            {
                if (_XmSegCharCount(seg) < 2)
                    _XmSegPixelWidth(seg) = 0;
                else
		  {
                    _XmSegPixelWidth(seg) = 
		      abs(XTextWidth16 (font_struct, 
					(XChar2b *)(_XmSegText(seg)), 
					Half (_XmSegCharCount(seg))));
		    /* pir 2967 */
		    if (_XmSegPixelWidth(seg) == 0)
		      _XmSegPixelWidth(seg) = Half(_XmSegCharCount(seg)) *
			(font_struct->max_bounds.width);
		  }
            }
            else
            {
                if (_XmSegCharCount(seg) < 1)
                    _XmSegPixelWidth(seg) = 0;
                else 
		  {
                    _XmSegPixelWidth(seg) = 
		      abs(XTextWidth (font_struct,(char *)(_XmSegText(seg)),
				      _XmSegCharCount(seg)));
		    /* pir 2967 */
		    if (_XmSegPixelWidth(seg) == 0)
		      _XmSegPixelWidth(seg) = _XmSegCharCount(seg) *
			(font_struct->max_bounds.width);
		  }
            }
        }
        else
        {
	  if (_XmSegCharCount(seg) < 1) _XmSegPixelWidth(seg) = 0;
	  else
	    {
	      XFontSet font_set = (XFontSet)FontListFont(entry);

	      _XmSegPixelWidth(seg) =
                abs(XmbTextEscapement(font_set, _XmSegText(seg),
                                      _XmSegCharCount(seg)));
	    }
        }
    }
}

static void 
#ifdef _NO_PROTO
_clear_segment( fontlist, seg )
        XmFontList fontlist ;
        _XmStringSegment seg ;
#else
_clear_segment(
        XmFontList fontlist,
        _XmStringSegment seg )
#endif /* _NO_PROTO */
{
    _XmSegFontIndex(seg)  = -1;
    _XmSegPixelWidth(seg)  = 0;
}

static void 
#ifdef _NO_PROTO
_clear_opt( fontlist, opt )
        XmFontList fontlist ;
        _XmStringOpt opt ;
#else
_clear_opt(
        XmFontList fontlist,
        _XmStringOpt opt )
#endif /* _NO_PROTO */
{
    _XmOptWidthUpdated(opt) = FALSE ;
    _XmOptPixelWidth(opt) = 0 ;
}

static void 
#ifdef _NO_PROTO
_parse_locale( str, indx, len )
        char *str ;
        int *indx ;
        int *len ;
#else
_parse_locale(
        char *str,
        int *indx,
        int *len )
#endif /* _NO_PROTO */
{
    char     *temp;
    int      start;
    int      end;

    /*
     *  Set the return variables to zero.  If we find what we're looking
     *  for, we reset them.
     */

    *indx = 0;
    *len = 0;

    /*
     *  The format of the locale string is:
     *          language[_territory[.codeset]]
     */

    temp = str;
    for (end = 0; (temp[end] != '.') && (temp[end] != 0); end++)
      ;

    if (temp[end] == '.')
    {
        start = end + 1;
        *indx = start;
        for (end = start; temp[end] != 0; end++)
            ;
        *len = end - start;
    }
}

 /* This function returns current default charset being used.  This is */
 /* determined from the value of the $LANG environment variable or */
 /* XmFALLBACK_CHARSET.  */
char * 
#ifdef _NO_PROTO
_XmStringGetCurrentCharset()
#else
_XmStringGetCurrentCharset( void )
#endif /* _NO_PROTO */
{
    char *str;
    char *ptr;
    int  chlen;
    int  indx;
    int  len;
   
    if (!locale.inited)
    {
        locale.charset = NULL;
        locale.charsetlen = 0;
 
        str = (char *)getenv(env_variable);

        if (str)
        {
           _parse_locale(str, &indx, &chlen);
           if (chlen > 0)
           {
               ptr = &str[indx];
	       len = chlen;
           }
           else {
               len = strlen(XmFALLBACK_CHARSET);
               ptr = XmFALLBACK_CHARSET;
           }
        }
        else {
	  len = strlen(XmFALLBACK_CHARSET);
	  ptr = XmFALLBACK_CHARSET;
        }
        locale.charset = (char *) XtMalloc(len + 1);
        strncpy(locale.charset, ptr, len);
        locale.charset[len] = '\0';
        locale.charsetlen = len;

	/* Register XmSTRING_DEFAULT_CHARSET for compound text conversion. */
	XmRegisterSegmentEncoding(XmSTRING_DEFAULT_CHARSET, 
				  XmFONTLIST_DEFAULT_TAG);
      
        locale.inited = TRUE;
    }
    return (locale.charset);
}

 /* This function compares a given charset to the current default charset
    being used.  It return TRUE if they match, FALSE otherwise.
 */
Boolean
#ifdef _NO_PROTO
_XmStringIsCurrentCharset(c)
     XmStringCharSet c;
#else
_XmStringIsCurrentCharset( XmStringCharSet c )
#endif /* _NO_PROTO */
{
  return (strcmp(c, _XmStringGetCurrentCharset()) == 0);
}

/*
 * update the font indexs and the dimensional information in the internal 
 * TCS, used when the font changes
 */
void 
#ifdef _NO_PROTO
_XmStringUpdate( fontlist, string )
        XmFontList fontlist ;
        _XmString string ;
#else
_XmStringUpdate(
        XmFontList fontlist,
        _XmString string )
#endif /* _NO_PROTO */
{
    int i, j;

    if ((fontlist == NULL) || (string == NULL)) return;

    /* optimized; clear and leave */

    if (_XmStrOptimized(string))
    {
        _clear_opt(fontlist, (_XmStringOpt) string);
        _update_opt(fontlist, (_XmStringOpt) string, (XmFontListEntry) NULL);
        return;
    }

    /* non-optimized; grind through it */

    else {
        _XmStringLine line = _XmStrLineLine(string);
        for (i=0; i<_XmStrLineCnt(string); i++)
        {
            _XmStringSegment seg = _XmStrLineSegment(&line[i]);
            int        segcount = _XmStrLineSegCount(&line[i]);

	    for (j = 0; j < segcount; j++)
	    {
	        _clear_segment (fontlist, &seg[j]);
	        _update_segment (fontlist, &seg[j]);
	    }
        }
    }
}

/*
 * duplicate an internal string
 */
_XmString 
#ifdef _NO_PROTO
_XmStringCopy( string )
        _XmString string ;
#else
_XmStringCopy(
        _XmString string )
#endif /* _NO_PROTO */
{

    _XmString new_string;

    if (string == NULL) return((_XmString)NULL);

    if (_XmStrOptimized(string))
    {
       _XmStringOpt o_string = (_XmStringOpt) string;
       _XmStringOpt n_o_string = 
              (_XmStringOpt) XtMalloc(sizeof(_XmStringOptRec) + 
                            _XmOptCharCount(o_string));
       memcpy( o_string, n_o_string, sizeof(_XmStringOptRec)
                                                 + _XmOptCharCount(o_string)) ;
       new_string = (_XmString) n_o_string;        
                
    }
    else
    {
        int i, j;
        _XmString n_string = (_XmString) XtMalloc (sizeof (_XmStringRec));

        _XmStrLineCnt(n_string) = _XmStrLineCnt(string);
	_XmStrLineLine(n_string) = (_XmStringLine) XtMalloc (
			sizeof (_XmStringLineRec) * _XmStrLineCnt(string));

        for (i=0; i<_XmStrLineCnt(string); i++)
        {
	    _XmStringLine line   = &(_XmStrLineLine(string)[i]);
	    _XmStringLine n_line = &(_XmStrLineLine(n_string)[i]);

	    _XmStrLineSegCount(n_line) = _XmStrLineSegCount(line);

	    if (_XmStrLineSegCount(line) > 0)
	    {
	        _XmStrLineSegment(n_line) = (_XmStringSegment) XtMalloc (
			  sizeof (_XmStringSegmentRec) * 
                          _XmStrLineSegCount(line));

	        for (j=0; j<_XmStrLineSegCount(line); j++)
	        {
	    	    _XmStringSegment seg   = &(_XmStrLineSegment(line)[j]);
	    	    _XmStringSegment n_seg = &(_XmStrLineSegment(n_line)[j]);

	    	    _XmSegCharset(n_seg)      = _XmSegCharset(seg);
	    	    _XmSegFontIndex(n_seg)   = -1;
	    	    _XmSegCharCount(n_seg)   = _XmSegCharCount(seg);
	    	    _XmSegText(n_seg)         = (char *) XtMalloc (_XmSegCharCount(seg));
		    memcpy(       _XmSegText(n_seg), 
                                  _XmSegText(seg), 
                                  _XmSegCharCount(seg));
	    	    _XmSegDirection(n_seg)    = _XmSegDirection(seg);
	    	    _XmSegPixelWidth(n_seg)  = 0;
	        }
	    }
	    else
	        _XmStrLineSegment(n_line) = (_XmStringSegment) NULL;
        }
        new_string = (_XmString) n_string;
    }
    return(new_string);
}

/*
 * check these two internals
 */
Boolean 
#ifdef _NO_PROTO
_XmStringByteCompare( a, b )
        _XmString a ;
        _XmString b ;
#else
_XmStringByteCompare(
        _XmString a,
        _XmString b )
#endif /* _NO_PROTO */
{
  if ((a == NULL) && (b == NULL)) return TRUE;
  if ((a == NULL) || (b == NULL)) return FALSE;

#ifndef OSF_v1_2_4
    if (_XmStrOptimized(a))
#else /* OSF_v1_2_4 */
  if (_XmStrOptimized(a) != _XmStrOptimized(b)) return FALSE;

  if (_XmStrOptimized(a))
#endif /* OSF_v1_2_4 */
    {
       _XmStringOpt a1 = (_XmStringOpt) a;
       _XmStringOpt b1 = (_XmStringOpt) b;

       if (!((_XmOptCharsetGet(a1) == _XmOptCharsetGet(b1)) ||
	     ((strcmp(_XmOptCharsetGet(a1), XmFONTLIST_DEFAULT_TAG) == 0) &&
	      _XmStringIsCurrentCharset(_XmOptCharsetGet(b1))) ||
	     ((strcmp(_XmOptCharsetGet(b1), XmFONTLIST_DEFAULT_TAG) == 0) &&
	      _XmStringIsCurrentCharset(_XmOptCharsetGet(a1)))))
	 return (FALSE);
       if (_XmOptCharCount(a1) != _XmOptCharCount(b1))
           return (FALSE);
       if (_XmOptDirectionGet(a1) != _XmOptDirectionGet(b1))
           return (FALSE);
       if (strncmp(_XmOptText(a1), _XmOptText(b1), _XmOptCharCount(a1)) != 0)
           return (FALSE);

    }
    else {
        int i, j;
        _XmStringLine line_a = _XmStrLineLine(a);
        _XmStringLine line_b = _XmStrLineLine(b);

	if (_XmStrLineCnt(a) != _XmStrLineCnt(b)) 
	  return (FALSE);

        for (i=0; i<_XmStrLineCnt(a); i++)
        {
	    if (_XmStrLineSegCount(&line_a[i]) != 
                _XmStrLineSegCount(&line_b[i])) 
	        return (FALSE);

	    for (j=0; j<_XmStrLineSegCount(&line_a[i]); j++)
	    {
	        _XmStringSegment 
                    a_seg = &(_XmStrLineSegment(&line_a[i])[j]),
                    b_seg = &(_XmStrLineSegment(&line_b[i])[j]);

		if (!((_XmSegCharset(a_seg) == _XmSegCharset(b_seg)) ||
		      ((strcmp(_XmSegCharset(a_seg), 
			       XmFONTLIST_DEFAULT_TAG) == 0) &&
		       _XmStringIsCurrentCharset(_XmSegCharset(b_seg))) ||
		      ((strcmp(_XmSegCharset(b_seg), 
			       XmFONTLIST_DEFAULT_TAG) == 0) &&
		       _XmStringIsCurrentCharset(_XmSegCharset(a_seg)))))
		  return (FALSE);
	        if (_XmSegCharCount(a_seg)   != _XmSegCharCount(b_seg))
                     return (FALSE);
	        if (_XmSegDirection(a_seg)    != _XmSegDirection(b_seg))
                     return (FALSE);
	        if (strncmp (_XmSegText(a_seg), _XmSegText(b_seg), 
                             _XmSegCharCount(a_seg)) != 0)
		    return (FALSE);
	    }
        }
    }
    return (TRUE);
}

Boolean 
#ifdef _NO_PROTO
_XmStringHasSubstring( string, substring )
        _XmString string ;
        _XmString substring ;
#else
_XmStringHasSubstring(
        _XmString string,
        _XmString substring )
#endif /* _NO_PROTO */
{
    _XmStringContext  context;
    XmStringCharSet   charset;
    XmStringDirection direction;
    char              *text;
    char              *subtext;
    short             char_count;
    short             subchar_count;
    Boolean           separator;
    Boolean           found;
    int               i, j, max;

    if ((string == NULL) || (substring == NULL) || (_XmStringEmpty(substring)))
        return (FALSE);

    /*
     * The substring must be a one line/one segment string.
     */
  
#ifndef OSF_v1_2_4
    if (!_XmStrOptimized(substring) && (_XmStrLineCnt(substring) > 0))
#else /* OSF_v1_2_4 */
    if (!_XmStrOptimized(substring)) {
      if (_XmStrLineCnt(substring) > 1)
#endif /* OSF_v1_2_4 */
        return (FALSE);
#ifdef OSF_v1_2_4
      if (_XmStrLineCnt(substring) > 0 && 
	  _XmStrLineSegCount(&(_XmStrLineLine(substring)[0])) > 1)
	return (FALSE);
    }
#endif /* OSF_v1_2_4 */


    /*
     * Get the text out of the substring.
     */

    if ((found = _XmStringInitContext(&context, substring)))
    {
        found = _XmStringGetNextSegment(context, &charset, &direction, 
                    &subtext, &subchar_count, &separator);
/* BEGIN OSF Fix CR 3279 */
	if (found) XtFree(charset);
/* END OSF Fix CR 3279 */
        _XmStringFreeContext(context);
    }

    if (!found)
	return (FALSE);

    if (_XmStringInitContext(&context, string))
    {
        while (_XmStringGetNextSegment(context, &charset, &direction, &text,
                    &char_count, &separator))
        {
/* BEGIN OSF Fix CR 3279 */
	  XtFree(charset);
/* END OSF Fix CR 3279 */

            if (char_count >= subchar_count)
            {
                max = char_count - subchar_count;
                for (i = 0; i <= max; i++)
	        {
/* BEGIN OSF Fix CR 3279 */
	            found = TRUE;
/* END OSF Fix CR 3279 */

	            for (j = 0; j < subchar_count; j++)
	            {
		        if (text[i+j] != subtext[j]) 
		        {
/* BEGIN OSF Fix CR 3279 */
		            found = FALSE;
/* END OSF Fix CR 3279 */
		            break;
		        }
	            }
/* BEGIN OSF Fix CR 3279 */
	            if (found)
/* END OSF Fix CR 3279 */
                    {
                        _XmStringFreeContext(context);
/* BEGIN OSF Fix CR 3279 */
			XtFree(subtext);
			XtFree(text);
/* END OSF Fix CR 3279 */
			
		        return(TRUE);
                    }
                }
            }
/* BEGIN OSF Fix CR 3279 */
	    XtFree(text);
/* END OSF Fix CR 3279 */
        }
        _XmStringFreeContext(context);
    }
/* BEGIN OSF Fix CR 3279 */
    XtFree(subtext);
/* END OSF Fix CR 3279 */

    return (FALSE);
}


/*
 * build the external TCS given an internal.

 * this is pretty crude yet
 * could be made a lot faster by making a pass to figure out how big it 
 * needs to be and building the external in-place rather than all the
 * concatenations...
 */
XmString 
#ifdef _NO_PROTO
_XmStringCreateExternal( fontlist, cs )
        XmFontList fontlist ;
        _XmString cs ;
#else
_XmStringCreateExternal(
        XmFontList fontlist,
        _XmString cs )
#endif /* _NO_PROTO */
{
    int i, j;
    XmString a, b, c, d, ext;
    char *x;


    if (!cs) return((XmString) NULL);

    ext = NULL;

    if (_XmStrOptimized(cs))
    {
        _XmStringOpt str = (_XmStringOpt) cs;

	a = XmStringDirectionCreate (_XmOptDirectionGet(str));

	x = (char *)ALLOCATE_LOCAL (_XmOptCharCount(str) + 1);
	memcpy( x, _XmOptText(str), _XmOptCharCount(str));
	x[_XmOptCharCount(str)] = '\0';

	b = XmStringCreate (x, _XmOptCharsetGet(str));

	c = XmStringConcat (a, b);

	d = ext;
	ext = XmStringConcat (d, c);

	XmStringFree (a); 
	XmStringFree (b);
	XmStringFree (c);
	XmStringFree (d);
        DEALLOCATE_LOCAL (x);
    }
    else {

        _XmStringLine  line = _XmStrLineLine(cs);

        for (i=0; i<_XmStrLineCnt(cs); i++)
        {

            _XmStringSegment seg = _XmStrLineSegment(&line[i]);
            int        segcount = _XmStrLineSegCount(&line[i]);

	    for (j = 0; j < segcount; j++)
	    {
	        if (_XmSegCharCount(&seg[j]) > 0)
	        {
		    a = XmStringDirectionCreate (_XmSegDirection(&seg[j]));

		    x = (char *)ALLOCATE_LOCAL (_XmSegCharCount(&seg[j]) + 1);
		    memcpy( x, _XmSegText(&seg[j]), _XmSegCharCount(&seg[j]));
		    x[_XmSegCharCount(&seg[j])] = '\0';

		    b = XmStringCreate (x, _XmSegCharset(&seg[j]));

		    c = XmStringConcat (a, b);

		    d = ext;
		    ext = XmStringConcat (d, c);

		    XmStringFree (a); 
		    XmStringFree (b);
		    XmStringFree (c);
		    XmStringFree (d);
                    DEALLOCATE_LOCAL (x);
	        }
	    }

            if (i < (_XmStrLineCnt(cs) - 1))
            {
                a = XmStringSeparatorCreate ();
	        b = ext;
	        ext = XmStringConcat (b, a);
                XmStringFree (a); 
	        XmStringFree (b);
            }
        }
    }
    return (ext);
}

Dimension 
#ifdef _NO_PROTO
_XmStringBaseline( fontlist, string )
        XmFontList fontlist ;
        _XmString string ;
#else
_XmStringBaseline(
        XmFontList fontlist,
        _XmString string )
#endif /* _NO_PROTO */
{
  if ((fontlist == NULL) || (string == NULL)) return(0);
  
    if (!_XmStrOptimized(string))
	return (_XmStringLineAscender (fontlist, 
                                       &(_XmStrLineLine(string)[0])));
    else
    {   return (_XmStringOptLineAscender(fontlist, (_XmStringOpt)string));
        } 
}

Dimension 
#ifdef _NO_PROTO
XmStringBaseline( fontlist, string )
        XmFontList fontlist ;
        XmString string ;
#else
XmStringBaseline(
        XmFontList fontlist,
        XmString string )
#endif /* _NO_PROTO */
{
    _XmString s;
    Dimension bl;

    if (!string || !fontlist) return (0);
    s = _XmStringCreate(string);

/* BEGIN OSF Fix CR 4399 */
    if (!s) return (0);
/* END OSF Fix CR 4399 */

    bl = _XmStringBaseline (fontlist, s);
    _XmStringFree(s);
    return(bl);
}

int 
#ifdef _NO_PROTO
_XmStringLineCount( string )
        _XmString string ;
#else
_XmStringLineCount(
        _XmString string )
#endif /* _NO_PROTO */
{
  if ((string == NULL)) return(0);
  
    if(    _XmStrOptimized( string)    )
    {   return( 1) ;
        } 
    return( _XmStrLineCnt( string)) ;
    }


/*
 * external TCS routines which just do on-the-fly conversion to
 * internal and then call the internal ones
 */

/*
 * find width of widest line in external TCS
 */
Dimension 
#ifdef _NO_PROTO
XmStringWidth( fontlist, string )
        XmFontList fontlist ;
        XmString string ;
#else
XmStringWidth(
        XmFontList fontlist,
        XmString string )
#endif /* _NO_PROTO */
{
    _XmString a;
    Dimension width;

    if (!string || !fontlist) return (0);

    a = _XmStringCreate (string);

/* BEGIN OSF Fix CR 4399 */
    if (!a) return (0);
/* END OSF Fix CR 4399 */

    width = _XmStringWidth (fontlist, a);
    _XmStringFree (a);

    return (width);
}

/*
 * find total height of external TCS
 */
Dimension 
#ifdef _NO_PROTO
XmStringHeight( fontlist, string )
        XmFontList fontlist ;
        XmString string ;
#else
XmStringHeight(
        XmFontList fontlist,
        XmString string )
#endif /* _NO_PROTO */
{
    _XmString a;
    Dimension height;

    if (!string || !fontlist) return (0);

    a = _XmStringCreate (string);

/* BEGIN OSF Fix CR 4399 */
    if (!a) return (0);
/* END OSF Fix CR 4399 */

    height = _XmStringHeight (fontlist, a);

    _XmStringFree (a);

    return (height);
}

/*
 * find the rectangle which will enclose the text 
 */
void 
#ifdef _NO_PROTO
XmStringExtent( fontlist, string, width, height )
        XmFontList fontlist ;
        XmString string ;
        Dimension *width ;
        Dimension *height ;
#else
XmStringExtent(
        XmFontList fontlist,
        XmString string,
        Dimension *width,
        Dimension *height )
#endif /* _NO_PROTO */
{
    _XmString a;

/* BEGIN OSF Fix CR 4399 */
    if (!string || !fontlist || !(a = _XmStringCreate (string)))
/* END OSF Fix CR 4399 */
      {
        *width = 0;
        *height = 0;
        return;
      }

/* BEGIN OSF Fix CR 4399 */
/* END OSF Fix CR 4399 */
    _XmStringExtent(fontlist, a, width, height);
    
    _XmStringFree (a);
}

/*
 * count the number of lines in an external TCS
 */
int 
#ifdef _NO_PROTO
XmStringLineCount( string )
        XmString string ;
#else
XmStringLineCount(
        XmString string )
#endif /* _NO_PROTO */
{
    int i = 1;
    unsigned char *c = (unsigned char *) _read_header((unsigned char *) string);
    unsigned char *end = c + _read_string_length ((unsigned char *) string);
    Boolean asn1 = _is_asn1(string);

    while (c < end)
    {
	switch (*c)
	{
	    case XmSTRING_COMPONENT_SEPARATOR:
		i++;
		/* no break */

	    default:
		if (asn1) 
		  {
		    int length = _read_asn1_length(c);
		    c += length + _asn1_size(length);
		  }
		else c += _read_component_length (c) + HEADER;
		break;
	}
    }

    return (i);
}



/*
 * drawing routine for external TCS
 */
void 
#ifdef _NO_PROTO
XmStringDraw( d, w, fontlist, string, gc, x, y, width, align, lay_dir, clip )
        Display *d ;
        Window w ;
        XmFontList fontlist ;
        XmString string ;
        GC gc ;
        Position x ;
        Position y ;
        Dimension width ;
        unsigned char align ;
        unsigned char lay_dir ;
        XRectangle *clip ;
#else
XmStringDraw(
        Display *d,
        Window w,
        XmFontList fontlist,
        XmString string,
        GC gc,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        unsigned int align,
        unsigned int lay_dir,
#else
        Position x,
        Position y,
        Dimension width,
        unsigned char align,
        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
        XRectangle *clip )
#endif /* _NO_PROTO */
{
  if (string) 
    {
      
      _XmString internal = _XmStringCreate (string);

      _draw (d, w, fontlist, internal, gc, x, y, width, 
	     align, lay_dir, clip, FALSE, NULL);

      _XmStringFree (internal);
    }
}

void 
#ifdef _NO_PROTO
XmStringDrawImage( d, w, fontlist, string, gc, x, y, width, align, lay_dir, clip )
        Display *d ;
        Window w ;
        XmFontList fontlist ;
        XmString string ;
        GC gc ;
        Position x ;
        Position y ;
        Dimension width ;
        unsigned char align ;
        unsigned char lay_dir ;
        XRectangle *clip ;
#else
XmStringDrawImage(
        Display *d,
        Window w,
        XmFontList fontlist,
        XmString string,
        GC gc,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        unsigned int align,
        unsigned int lay_dir,
#else
        Position x,
        Position y,
        Dimension width,
        unsigned char align,
        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
        XRectangle *clip )
#endif /* _NO_PROTO */
{
  if (string)
    {
      _XmString internal = _XmStringCreate (string);

      _draw (d, w, fontlist, internal, gc, x, y, width, 
	     align, lay_dir, clip, TRUE, NULL);

      _XmStringFree (internal);
    }
}

void 
#ifdef _NO_PROTO
XmStringDrawUnderline( d, w, fntlst, str, gc, x, y, width, align, lay_dir, clip, under )
        Display *d ;
        Window w ;
        XmFontList fntlst ;
        XmString str ;
        GC gc ;
        Position x ;
        Position y ;
        Dimension width ;
        unsigned char align ;
        unsigned char lay_dir ;
        XRectangle *clip ;
        XmString under ;
#else
XmStringDrawUnderline(
        Display *d,
        Window w,
        XmFontList fntlst,
        XmString str,
        GC gc,
#if NeedWidePrototypes
        int x,
        int y,
        int width,
        unsigned int align,
        unsigned int lay_dir,
#else
        Position x,
        Position y,
        Dimension width,
        unsigned char align,
        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
        XRectangle *clip,
        XmString under )
#endif /* _NO_PROTO */
{
  if (str)
    {
      _XmString internal = _XmStringCreate (str),
                int_under = _XmStringCreate (under);

      _draw (d, w, fntlst, internal, gc, x, y, width, 
	     align, lay_dir, clip, FALSE, int_under);

      _XmStringFree (internal);
      _XmStringFree (int_under);
    }
}


#ifdef _XM_DEBUG_XMSTRING

void 
#ifdef _NO_PROTO
_Xm_dump_fontlist( f )
        XmFontList f ;
#else
_Xm_dump_fontlist(
        XmFontList f )
#endif /* _NO_PROTO */
{
    int i = 0;

    for ( ; FontListFont(f) != NULL; f++, i++)
    {
   	printf ("fontlist[%3d] of 0x%x(%d)\n", i,f,f);
	printf ("\ttype = %d\n", FontListType(f));
	printf ("\tfont = %d\n", FontListFont(f));
	printf ("\ttag = <%s>\n", FontListTag(f));
    }
}

void 
#ifdef _NO_PROTO
_Xm_dump_fontlist_cache()
#else
_Xm_dump_fontlist_cache( void )
#endif /* _NO_PROTO */
{
    FontlistEntry *cache;

    if (_fontlist_cache == NULL)
    {
        printf("fontlist cache is empty\n");
        return;
    }

    for (cache = _fontlist_cache; cache; cache = FontCacheNext(cache))
    {
        printf("cache pointer:   %x\n", cache);
        _Xm_dump_fontlist(FontCacheFontList(cache));
        printf("refcount:      %d\n", FontCacheRefCount(cache));
        printf("next:          %x\n\n", FontCacheNext(cache));
    }
}

void 
#ifdef _NO_PROTO
_Xm_dump_external( cs )
        XmString cs ;
#else
_Xm_dump_external(
        XmString cs )
#endif /* _NO_PROTO */
{
    unsigned char *c; 
    unsigned char *end; 
    int k;
    Boolean asn1;
    
    if (_XmStringIsXmString(cs))
    {
	printf ("Compound string\n");
	printf ("overall length = %d\n", _read_string_length(cs));
	c = _read_header(cs);
    }
    else {
        printf ("Not a compound string\n");
        return;
    }

    asn1 = _is_asn1(cs);
    if (!asn1) cs = XmStringCopy(cs);
    
    c = (unsigned char *) cs;
    end = c + _read_string_length (c) + _read_header_length(c);

    while (c < end)
    {
	unsigned short length = _read_asn1_length (c);

	switch (*c)
	{
	    case XmSTRING_COMPONENT_CHARSET:
		printf ("\tCharacter set component\n");
		printf ("\tlength = %d\n", length);
	  	printf ("\tvalue  = <");
		for (k=0; k<length; k++) 
		  printf ("%c", *(c + _asn1_size(length) + k));
		printf (">\n");
		c += length + _asn1_size(length);
		break;

	    case XmSTRING_COMPONENT_TEXT:
            case XmSTRING_COMPONENT_LOCALE_TEXT:
                if (*c ==  XmSTRING_COMPONENT_TEXT)
                  printf ("\tText component\n");
                else printf ("\tLocalized text component\n");

		printf ("\tlength = %d\n", length);
	  	printf ("\tvalue  = <");
		for (k=0; k<length; k++) 
		  printf ("%c", *(c + _asn1_size(length) + k));
		printf (">\n");
		c += length + _asn1_size(length);
		break;

	    case XmSTRING_COMPONENT_DIRECTION:		/* record dir */
		printf ("\tDirection component\n");
		printf ("\tlength = %d\n", length);
	  	printf ("\tvalue  = %d\n", *(c + _asn1_size(length)));
		c += length + _asn1_size(length);
		break;

	    case XmSTRING_COMPONENT_SEPARATOR:		/* start new line */
		printf ("\tSeparator component\n");
		printf ("\tlength = %d\n", length);
		c += length + _asn1_size(length);
		break;

	    default:
		printf ("\tUnknown component\n");
		printf ("\tlength = %d\n", length);
	  	printf ("\tvalue  = <");
		for (k=0; k<length; k++)
			printf ("%3d ", (int) *(c + _asn1_size(length) + k));
		printf ("\n");
		c += length + _asn1_size(length);
		break;
	}

	printf ("\n");

    }
}

void 
#ifdef _NO_PROTO
_Xm_dump_internal( string )
        _XmString string ;
#else
_Xm_dump_internal(
        _XmString string )
#endif /* _NO_PROTO */
{
    int i, j, k;

    if (_XmStrOptimized(string))
    {
        _XmStringOpt str = (_XmStringOpt) string;
        printf ("string with 1 line\n") ;
        printf ("\tOptimized string - single segment\n");
	printf ("\t\tchar count   = %4d\n", _XmOptCharCount(str));
	printf ("\t\twidth updated   = %4d\n", _XmOptWidthUpdated(str));
	printf ("\t\tcharset index   = %4d\n", _XmOptCharsetIndex(str));
	printf ("\t\ttext         = <");
	for (k=0; k<_XmOptCharCount(str); k++) printf ("%c", _XmOptText(str)[k]);
	  printf (">\n");
	printf ("\t\tdirection    = %4d\n", _XmOptDirectionGet(str));
	printf ("\t\tpixel width  = %4d\n", _XmOptPixelWidth(str));
    }     

    else {
        _XmStringLine line = _XmStrLineLine(string);

        printf ("string with %d lines\n", _XmStrLineCnt(string));
        for (i = 0; i < _XmStrLineCnt(string); i++)
        {
	    _XmStringSegment seg = _XmStrLineSegment(&line[i]);

	    printf ("\tline [%2d] has %5d segments\n", i, 
                     _XmStrLineSegCount(&line[i]));

	    for (j = 0; j < _XmStrLineSegCount(&line[i]); j++)
	    {
	        printf ("\t\tsegment [%2d]\n", j); 
	        printf ("\t\t\tchar count   = %4d\n", _XmSegCharCount(&seg[j]));
	        printf ("\t\t\tfont index   = %4d\n", _XmSegFontIndex(&seg[j]));
	        printf ("\t\t\ttext         = <");
	        for (k=0; k<_XmSegCharCount(&seg[j]); k++) 
                    printf ("%c", _XmSegText(&seg[j])[k]);
	        printf (">\n");
	        printf ("\t\t\tdirection    = %4d\n", _XmSegDirection(&seg[j]));
	        printf ("\t\t\tpixel width  = %4d\n", _XmSegPixelWidth(&seg[j]));
	    }
        }
    }    
}

#endif /* _XM_DEBUG_XMSTRING */

/****************************************************************/
char *
#ifdef _NO_PROTO
_XmStringGetTextConcat( string)
        XmString string ;
#else
_XmStringGetTextConcat(
        XmString string)
#endif /* _NO_PROTO */
/****************
 * 
 ****************/
{
        XmStringContext context ;
        XmStringComponentType type ;
        char *text ;
        XmStringCharSet charset ;
        XmStringDirection dir ;
        XmStringComponentType tag ;
        unsigned short len ;
        unsigned char *val ;
        size_t SegLen ;
        size_t OldLen ;
        size_t OutLen = 0 ;
        char * OutStr = NULL ;
/****************/

    if(    string
        && XmStringInitContext( &context, string)    )
    {   
        while(    (type = XmStringGetNextComponent( context, &text, &charset,
                        &dir, &tag, &len, &val)) != XmSTRING_COMPONENT_END    )
        {   switch( type)
            {   
                case XmSTRING_COMPONENT_TEXT:
                case XmSTRING_COMPONENT_LOCALE_TEXT:
                {   
                    SegLen = strlen( text) ;
                    OldLen = OutLen ;
                    OutLen += SegLen ;
                    OutStr = XtRealloc( OutStr, OutLen + 1) ;
                    memcpy( &OutStr[OldLen], text, SegLen + 1) ;

                    XtFree( text) ;
                    break ;
                    }
                case XmSTRING_COMPONENT_CHARSET:
                {   XtFree((char *) charset) ;
                    break ;
                    } 
                case XmSTRING_COMPONENT_UNKNOWN:
                {   XtFree((char *) val) ;
                    break ;
                    } 
                case XmSTRING_COMPONENT_DIRECTION:
                case XmSTRING_COMPONENT_SEPARATOR:
                default:
                {   break ;
                    } 
                } 
            } 
        XmStringFreeContext( context) ;
        }
    return( OutStr) ;
    }
/****************************************************************/

/****************************************************************************
 ***									  ***
 ***  This next function is DUPLICATED in BulletinB.c!			  ***
 ***  REMOVE other copy for 1.2.1!					  ***
 ***									  ***
 ****************************************************************************/
/****************************************************************
 * Allocates a copy of the text and character set of the specified XmString
 *   if the XmString is composed of a single segment.
 * Returns TRUE if str is a single segment, FALSE otherwise.
 ****************/
Boolean
#ifdef _NO_PROTO
_XmStringSingleSegment( str, pTextOut, pCharsetOut )
        XmString str ;
        char **pTextOut ;
        XmStringCharSet *pCharsetOut ;
#else
_XmStringSingleSegment(
        XmString str,
        char **pTextOut,
        XmStringCharSet *pCharsetOut )
#endif /* _NO_PROTO */
{
            Boolean         retVal ;
            XmStringContext context ;
            char *          text ;
            XmStringCharSet charset ;
            char *          text2 ;
            XmStringCharSet charset2 ;
            XmStringDirection direction ;
            Boolean         separator ;
/****************/

    retVal = FALSE ;

    if(    XmStringInitContext( &context, str)    )
    {
        if(    XmStringGetNextSegment( context, &text, &charset,
                                                   &direction, &separator)    )
       {   if(    !XmStringGetNextSegment( context, &text2, &charset2,
                                                   &direction, &separator)    )
           {   retVal = TRUE ;
                *pTextOut = text ;
                *pCharsetOut = charset ;
                }
            else
            {   XtFree( text) ;
                XtFree( (char *) charset) ;
                XtFree( text2) ;
                XtFree( (char *) charset2) ;
                }
            }
        XmStringFreeContext( context) ;
        }
    return( retVal) ;
}

/****************************************************************************
 ***									  ***
 ***  This next function SUPERCEDES UpdateWMShellTitle() in BulletinB.c!  ***
 ***  REMOVE other copy and reuse for 1.2.1!				  ***
 ***									  ***
 ****************************************************************************/

#define STRING_CHARSET          "ISO8859-1"

void 
#ifdef _NO_PROTO
_XmStringUpdateWMShellTitle(xmstr, shell)
	XmString xmstr;
	Widget shell;
#else
_XmStringUpdateWMShellTitle(
	XmString xmstr,
	Widget shell) 
#endif
{
            char *          text ;
            XmStringCharSet charset ;
            Arg             al[10] ;
            Cardinal        ac ;
            XrmValue        from ;
            Atom            encoding = None;
            XrmValue        to ;
/****************/

    /* Set WMShell title (if present).
    */
    if(    XtIsWMShell( shell)    )
    {
        /* Shell is a Window Manager Shell, so set WMShell title
        *   from XmNdialogTitle.
        */
        text = NULL ;
        ac = 0 ;
        if (_XmStringSingleSegment(xmstr, &text, &charset))
          {
            if (!strcmp( STRING_CHARSET, charset))
              {
                /* dialog_title is a single segment of charset STRING_CHARSET,
                *   so use atom of "STRING".  Otherwise, convert to compound
                *   text and use atom of "COMPOUND_TEXT".
                */
                XtFree( (char *) charset) ;
                encoding = XmInternAtom(XtDisplay(shell), "STRING", 
					FALSE);
              }
            else if (!strcmp( XmFONTLIST_DEFAULT_TAG, charset))
              {
                /* dialog_title locale encoded so use constant of None */
                XtFree((char *)charset);
                encoding = None;
              }
            else
              {/* Don't need this, since dialog_title will be converted from
                *   original XmString to compound text.
                */
                XtFree( (char *) charset) ;
                XtFree( (char *) text) ;
                text = NULL ;
             }
          }
        if (!text)
          { from.addr = (char *) xmstr;
            if(    XmCvtXmStringToText( XtDisplay( shell), NULL, NULL,
                                                         &from, &to, NULL)    )
           {   text = to.addr ;
                encoding = XmInternAtom(XtDisplay( shell), 
					"COMPOUND_TEXT", FALSE);
                }
            }
        if(    text    )
        {
            XtSetArg( al[ac], XtNtitle, text) ;  ++ac ;
            XtSetArg( al[ac], XtNtitleEncoding, encoding) ; ++ac ;
            XtSetArg( al[ac], XtNiconName, text) ;  ++ac ;
            XtSetArg( al[ac], XtNiconNameEncoding, encoding) ; ++ac ;
            XtSetValues( shell, al, ac) ;
            XtFree( (char *) text) ;
            }
        }
    return ;
}
