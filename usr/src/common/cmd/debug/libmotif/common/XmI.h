#ident	"@(#)debugger:libmotif/common/XmI.h	1.1"
#pragma ident	"@(#)m1.2libs:Xm/XmI.h	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmI_h
#define _XmI_h

#ifndef _XmNO_BC_INCL
#define _XmNO_BC_INCL
#endif

#include <Xm/XmP.h>


#ifdef __cplusplus
extern "C" {
#endif


#ifndef XM_1_1_BC

#define Max(x, y)	(((x) > (y)) ? (x) : (y))
#define Min(x, y)	(((x) < (y)) ? (x) : (y))
#define AssignMax(x, y)	if ((y) > (x)) x = (y)
#define AssignMin(x, y)	if ((y) < (x)) x = (y)

#ifndef MAX
#define MAX(x,y)	((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y)	((x) > (y) ? (y) : (x))
#endif

#define GMode(g)	    ((g)->request_mode)
#define IsX(g)		    (GMode (g) & CWX)
#define IsY(g)		    (GMode (g) & CWY)
#define IsWidth(g)	    (GMode (g) & CWWidth)
#define IsHeight(g)	    (GMode (g) & CWHeight)
#define IsBorder(g)	    (GMode (g) & CWBorderWidth)
#define IsWidthHeight(g)    ((GMode (g) & CWWidth) || (GMode (g) & CWHeight))
#define IsQueryOnly(g)      (GMode (g) & XtCWQueryOnly)

#define XmStrlen(s)      ((s) ? strlen(s) : 0)

#endif /* XM_1_1_BC */

#define MOTIF_PRIVATE_GC     INT_MAX

#define XmStackAlloc(size, stack_cache_array)     \
    ((size) <= sizeof(stack_cache_array)          \
    ?  (XtPointer)(stack_cache_array)             \
    :  XtMalloc((unsigned)(size)))

#define XmStackFree(pointer, stack_cache_array) \
    if ((pointer) != ((XtPointer)(stack_cache_array))) XtFree(pointer);


/********    Internal Types for XmString.c    ********/

/*
 * These are the fontlist structures
 */

typedef struct _XmFontListRec
{
    XtPointer    font;
    char        *tag;
    XmFontType   type;
}
    XmFontListRec;

typedef struct _XmFontListContextRec
{
    XmFontList          nextPtr;                /* next one in fontlist */
    Boolean             error;                  /* something bad */
}
    XmFontListContextRec;

/*
 * these are the structures which make up the internal version of the TCS.
 */
typedef struct
{
    char        *charset;               /* name of charset */
    short       font_index;             /* index of font to use */
    short       char_count;             /* octet count for this segment */
    char        *text;                  /* ptr to octets. If RtoL then */
                                        /* is a local flipped copy */
    XmStringDirection direction;        /* octet order of this segment */
    Dimension   pixel_width;            /* width of segment */
}
    _XmStringSegmentRec, *_XmStringSegment;

typedef struct
{
    short               segment_count;  /* segments in this line */
    _XmStringSegment    segment;        /* array of segments */
}
    _XmStringLineRec, *_XmStringLine;

#define OPTIMIZED_BITS      1

typedef struct __XmStringRec
{
    unsigned int   optimized : OPTIMIZED_BITS;/*flag to indicate whether opt.*/
	        /* "optimized" is really a Boolean, but ANSI complains about */
		/* assigning TRUE to a one-bit signed field. */
    unsigned int    line_count : 15 ;   /* lines in this _XmString */
    _XmStringLine   line ;              /* array of lines */
}
    _XmStringRec;

/*
 * these are the structures which make up the optimized
 * internal version of the TCS.
 */

#define CHARSET_INDEX_BITS  4
#define CHAR_COUNT_BITS     8
#define TEXT_BYTES_IN_STRUCT 2

typedef struct
{
    unsigned int  optimized : OPTIMIZED_BITS;/* flag to indicate whether opt.*/
    unsigned int  width_updated : 1 ;   /* flag to track update of pixel_wid.*/
	    /* "optimized" and "width_updated" are really Boolean, but */
	    /* ANSI complains about assigning TRUE to a one-bit signed field.*/
    unsigned int direction : 2 ;   /* octet order of this segment */
    unsigned int charset_index : CHARSET_INDEX_BITS ; /* index in cs cache */
    unsigned int char_count : CHAR_COUNT_BITS ; /* octet count, this seg.*/
    Dimension   pixel_width;            /* width of segment */
    char        text[TEXT_BYTES_IN_STRUCT] ; /* the string text */
}
    _XmStringOptRec, *_XmStringOpt;

/*
 * internal context data block, for read-out
 */

typedef struct __XmStringContextRec
{
    _XmString   string;			/* pointer to internal string */
    short       current_line;           /* index of current line */
    short       current_seg;            /* index of current segment */
    Boolean     optimized;              /* flags whether this is optimized */
    Boolean     error;                  /* something wrong */
}
    _XmStringContextRec;

/*
 * external context data block
 */

typedef struct _XmtStringContextRec
{
    XmString            string;
    unsigned short      offset;                 /* current place TCS */
    unsigned short      length;                 /* max length */
    XmStringCharSet     charset;                /* last charset seen */
    unsigned short      charset_length;         /* and it's length */
    XmStringDirection   direction;              /* last direction */
    Boolean             error;                  /* something bad */
}
    XmStringContextRec;


/********    Private Function Declarations for GetSecRes.c    ********/
#ifdef _NO_PROTO

extern Cardinal _XmSecondaryResourceData() ;

#else

extern Cardinal _XmSecondaryResourceData( 
                        XmBaseClassExt bcePtr,
                        XmSecondaryResourceData **secResDataRtn,
                        XtPointer client_data,
                        String name,
                        String class_name,
                        XmResourceBaseProc basefunctionpointer) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


/********        ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmI_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
