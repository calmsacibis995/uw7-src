#pragma ident	"@(#)m1.2libs:Xm/_xpmI.h	1.1"
/************************************************************************* 
 **  (c) Copyright 1993, 1994 Hewlett-Packard Company
 **  (c) Copyright 1993, 1994 International Business Machines Corp.
 **  (c) Copyright 1993, 1994 Sun Microsystems, Inc.
 **  (c) Copyright 1993, 1994 Novell, Inc.
 *************************************************************************/
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$"
#endif
#endif
/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* _xpmI.h:                                                                     *
*                                                                             *
*  Special version of Xpm library for ImageCache use only		      *
*  Private Include file                                                       *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#ifndef _XPMI_h
#define _XPMI_h

#ifdef Debug
/* memory leak control tool */
#include <mnemosyne.h>
#endif

#ifdef VMS
#include "decw$include:Xlib.h"
#include "decw$include:Intrinsic.h"
#include "sys$library:stdio.h"
#else
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <stdio.h>
/* stdio.h doesn't declare popen on a Sequent DYNIX OS */
#ifdef sequent
extern FILE *popen();
#endif
#endif

#include "_xpmP.h"

/* we keep the same codes as for Bitmap management */
#ifndef _XUTIL_H_
#ifdef VMS
#include "decw$include:Xutil.h"
#else
#include <X11/Xutil.h>
#endif
#endif

#if defined(SYSV) || defined(SVR4)
#define bcopy(source, dest, count) memcpy(dest, source, count)
#endif

typedef struct {
    unsigned int type;
    union {
	FILE *file;
	char **data;
    }     stream;
    char *cptr;
    unsigned int line;
    int CommentLength;
    char Comment[BUFSIZ];
    char *Bcmt, *Ecmt, Bos, Eos;
}      xpmData;

#define XPMARRAY 0
#define XPMFILE  1
#define XPMPIPE  2

typedef unsigned char byte;

#define EOL '\n'
#define TAB '\t'
#define SPC ' '

typedef struct {
    char *type;				/* key word */
    char *Bcmt;				/* string beginning comments */
    char *Ecmt;				/* string ending comments */
    char Bos;				/* character beginning strings */
    char Eos;				/* character ending strings */
    char *Strs;				/* strings separator */
    char *Dec;				/* data declaration string */
    char *Boa;				/* string beginning assignment */
    char *Eoa;				/* string ending assignment */
}      xpmDataType;

extern xpmDataType _XmxpmDataTypes[];

/*
 * rgb values and ascii names (from rgb text file) rgb values,
 * range of 0 -> 65535 color mnemonic of rgb value
 */
typedef struct {
    int r, g, b;
    char *name;
}      xpmRgbName;

/* Maximum number of rgb mnemonics allowed in rgb text file. */
#define MAX_RGBNAMES 1024

extern char *_XmxpmColorKeys[];

#define TRANSPARENT_COLOR "None"	/* this must be a string! */

/* number of _XmxpmColorKeys */
#define NKEYS 5

/*
 * key numbers for visual type, they must fit along with the number key of
 * each corresponding element in _XmxpmColorKeys[] defined in _xpmI.h
 */
#define MONO	2
#define GRAY4	3
#define GRAY 	4
#define COLOR	5

/* structure containing data related to an Xpm pixmap */
typedef struct {
    char *name;
    unsigned int width;
    unsigned int height;
    unsigned int cpp;
    unsigned int ncolors;
    char ***colorTable;
    unsigned int *pixelindex;
    XColor *xcolors;
    char **colorStrings;
    unsigned int mask_pixel;		/* mask pixel's colorTable index */
}      xpmInternAttrib;

#define UNDEF_PIXEL 0x80000000

/* XPM private routines */

FUNC(_XmxpmCreateData, int, (char ***data_return,
		    xpmInternAttrib * attrib, XpmAttributes * attributes));

FUNC(_XmxpmCreateImage, int, (Display * display,
			   xpmInternAttrib * attrib,
			   XImage ** image_return,
			   XImage ** shapeimage_return,
			   XpmAttributes * attributes));

FUNC(_XmxpmParseData, int, (xpmData * data,
			 xpmInternAttrib * attrib_return,
			 XpmAttributes * attributes));

FUNC(_XmxpmScanImage, int, (Display * display,
			 XImage * image,
			 XImage * shapeimage,
			 XpmAttributes * attributes,
			 xpmInternAttrib * attrib));

FUNC(_XmxpmFreeColorTable, int, (char ***colorTable, int ncolors));

FUNC(_XmxpmInitInternAttrib, int, (xpmInternAttrib * dtdata));

FUNC(_XmxpmFreeInternAttrib, int, (xpmInternAttrib * dtdata));

FUNC(_XmxpmSetAttributes, int, (xpmInternAttrib * attrib,
			     XpmAttributes * attributes));

/* this part of private functions is from xpm.h */
FUNC(_XmXpmCreateDataFromImage, int, (Display * display,
                                   char ***data_return,
                                   XImage * image,
                                   XImage * shapeimage,
                                   XpmAttributes * attributes));
FUNC(_XmXpmFreeAttributes, int, (XpmAttributes * attributes));
FUNC(_XmXpmFreeExtensions, int, (XpmExtension * extensions, int nextensions));

/* I/O utility */

FUNC(_XmxpmNextString, int, (xpmData * mdata));
FUNC(_XmxpmNextUI, int, (xpmData * mdata, unsigned int *ui_return));

#define _XmxpmGetC(mdata) \
	(mdata->type ? (getc(mdata->stream.file)) : (*mdata->cptr++))

FUNC(_XmxpmNextWord, unsigned int, (xpmData * mdata, char *buf));
FUNC(_XmxpmGetCmt, int, (xpmData * mdata, char **cmt));
FUNC(_XmxpmReadFile, int, (char *filename, xpmData * mdata));
FUNC(_XmxpmOpenArray, void, (char **data, xpmData * mdata));
FUNC(_XmXpmDataClose, int, (xpmData * mdata));

/* RGB utility */

FUNC(_XmxpmReadRgbNames, int, (char *rgb_fname, xpmRgbName * rgbn));
FUNC(_XmxpmGetRgbName, char *, (xpmRgbName * rgbn, int rgbn_max,
			     int red, int green, int blue));
FUNC(_XmxpmFreeRgbNames, void, (xpmRgbName * rgbn, int rgbn_max));

FUNC(_Xmxpm_xynormalizeimagebits, void, (register unsigned char *bp,
				      register XImage * img));
FUNC(_Xmxpm_znormalizeimagebits, void, (register unsigned char *bp,
				     register XImage * img));

/*
 * Macros
 * 
 * The XYNORMALIZE macro determines whether XY format data requires 
 * normalization and calls a routine to do so if needed. The logic in
 * this module is designed for LSBFirst byte and bit order, so 
 * normalization is done as required to present the data in this order.
 *
 * The ZNORMALIZE macro performs byte and nibble order normalization if 
 * required for Z format data.
 * 
 * The XYINDEX macro computes the index to the starting byte (char) boundary
 * for a bitmap_unit containing a pixel with coordinates x and y for image
 * data in XY format.
 * 
 * The ZINDEX* macros compute the index to the starting byte (char) boundary 
 * for a pixel with coordinates x and y for image data in ZPixmap format.
 * 
 */

#define XYNORMALIZE(bp, img) \
    if ((img->byte_order == MSBFirst) || (img->bitmap_bit_order == MSBFirst)) \
	_Xmxpm_xynormalizeimagebits((unsigned char *)(bp), img)

#define ZNORMALIZE(bp, img) \
    if (img->byte_order == MSBFirst) \
	_Xmxpm_znormalizeimagebits((unsigned char *)(bp), img)

#define XYINDEX(x, y, img) \
    ((y) * img->bytes_per_line) + \
    (((x) + img->xoffset) / img->bitmap_unit) * (img->bitmap_unit >> 3)

#define ZINDEX(x, y, img) ((y) * img->bytes_per_line) + \
    (((x) * img->bits_per_pixel) >> 3)

#define ZINDEX32(x, y, img) ((y) * img->bytes_per_line) + ((x) << 2)

#define ZINDEX16(x, y, img) ((y) * img->bytes_per_line) + ((x) << 1)

#define ZINDEX8(x, y, img) ((y) * img->bytes_per_line) + (x)

#define ZINDEX1(x, y, img) ((y) * img->bytes_per_line) + ((x) >> 3)

#if __STDC__
#define Const const
#else
#define Const				/**/
#endif

/*
 * there are structures and functions related to hastable code
 */

typedef struct _xpmHashAtom {
    char *name;
    void *data;
}      *xpmHashAtom;

typedef struct {
    int size;
    int limit;
    int used;
    xpmHashAtom *atomTable;
} xpmHashTable;

FUNC(_XmxpmHashTableInit, int, (xpmHashTable *table));
FUNC(_XmxpmHashTableFree, void, (xpmHashTable *table));
FUNC(_XmxpmHashSlot, xpmHashAtom *, (xpmHashTable *table, char *s));
FUNC(_XmxpmHashIntern, int, (xpmHashTable *table, char *tag, void *data));

#define HashAtomData(i) ((void *)i)
#define HashColorIndex(slot) ((unsigned int)((*slot)->data))
#define USE_HASHTABLE (cpp > 2 && ncolors > 4)

#endif
