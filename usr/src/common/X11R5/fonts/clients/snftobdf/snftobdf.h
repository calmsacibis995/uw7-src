#ident	"@(#)r5fonts:clients/snftobdf/snftobdf.h	1.1"
/*

Copyright 1991 by Mark Leisher (mleisher@nmsu.edu)

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  I make no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.

*/

#ifndef _snftobdf_h
#define _snftobdf_h

/* Note: n2dChars is a macro defined in
         mit/server/include/font.h

         the file fontutil.c only used for
         bit and byte ordering functions   */

#include <X11/Xos.h>
#include <X11/Xmd.h>
#include <X11/X.h>
#include <X11/Xproto.h>

#include "misc.h"
#include "fontstruct.h"
/* #include "snfstruct.h" */
#include "snfstr.h"
#include "font.h"

#define SNFTOBDF_VERSION 1

#define BDF_VERSION "2.1"

/*
 * START: Section taken from the original "bdftosnf.h" file from the
 *        X11R4 sources.
 */

/*
 * a structure to hold all the pointers to make it easy to pass them all
 * around. Much like the FONT structure in the server.
 */

typedef struct _TempFont {
    snfFontInfoPtr pFI;
    snfCharInfoPtr pCI;
    unsigned char *pGlyphs;
    snfFontPropPtr pFP;
    CharInfoPtr pInkCI;
    CharInfoPtr pInkMin;
    CharInfoPtr pInkMax;
} TempFont; /* not called font since collides with type in X.h */

#ifdef vax

#	define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	define DEFAULTBITORDER 	LSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER LSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#else
# ifdef sun

#  if (sun386 || sun5)
#	define DEFAULTGLPAD 	4		/* default padding for glyphs */
#	define DEFAULTBITORDER 	LSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER LSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#  else
#	define DEFAULTGLPAD 	4		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#  endif

# else
#  ifdef apollo

#	define DEFAULTGLPAD 	2		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#  else
#   ifdef ibm032

#	define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#   else
#    ifdef hpux

#	define DEFAULTGLPAD 	2		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#    else
#     ifdef pegasus

#	define DEFAULTGLPAD 	4		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#     else
#      ifdef macII

#       define DEFAULTGLPAD     4               /* default padding for glyphs */
#       define DEFAULTBITORDER  MSBFirst        /* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#      else
#       ifdef mips
#        ifdef MIPSEL

#	  define DEFAULTGLPAD 	  4		/* default padding for glyphs */
#	  define DEFAULTBITORDER  LSBFirst	/* default bitmap bit order */
#	  define DEFAULTBYTEORDER LSBFirst	/* default bitmap byte order */
#	  define DEFAULTSCANUNIT  1		/* default bitmap scan unit */

#        else
#	  define DEFAULTGLPAD 	  4		/* default padding for glyphs */
#	  define DEFAULTBITORDER  MSBFirst	/* default bitmap bit order */
#	  define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	  define DEFAULTSCANUNIT  1		/* default bitmap scan unit */
#        endif
#       else

#	 define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	 define DEFAULTBITORDER MSBFirst	/* default bitmap bit order */
#	 define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	 define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#       endif
#      endif
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#define GLWIDTHBYTESPADDED(bits,nbytes) \
	((nbytes) == 1 ? (((bits)+7)>>3)	/* pad to 1 byte */ \
	:(nbytes) == 2 ? ((((bits)+15)>>3)&~1)	/* pad to 2 bytes */ \
	:(nbytes) == 4 ? ((((bits)+31)>>3)&~3)	/* pad to 4 bytes */ \
	:(nbytes) == 8 ? ((((bits)+63)>>3)&~7)	/* pad to 8 bytes */ \
	: 0)

/*
 * END: Section taken from the original "bdftosnf.h" file from the
 *      X11R4 sources.
 */

#define IsLinear(pfi) (((pfi)->firstRow == 0) && ((pfi)->lastRow == 0))

#define ReallyNonExistent(pci) \
  ((((pci).metrics.leftSideBearing + (pci).metrics.rightSideBearing) == 0) && \
   (((pci).metrics.ascent + (pci).metrics.descent) == 0) && \
   (pci).metrics.characterWidth == 0)


TempFont *
GetSNFInfo();
/*
char *path;
int bitOrder, byteOrder, scanUnit;
*/

unsigned char *
GetSNFBitmap();
/*
TempFont *tf;
unsigned int charnum;
int glyphPad;
*/

void
BDFHeader();
/*
TempFont *tf;
*/

void
BDFBitmaps();
/*
TempFont *tf;
int glyphPad;
*/

void
BDFTrailer();
/*
TempFont *tf;
*/

#endif /* _snftobdf_h */
