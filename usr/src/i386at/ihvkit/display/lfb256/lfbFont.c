#ident	"@(#)ihvkit:display/lfb256/lfbFont.c	1.1"

/*
 * Copyright (c) 1990, 1991, 1992, 1993 UNIX System Laboratories, Inc.
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	Copyright (c) 1993  Intel Corporation
 *	  All Rights Reserved
 */

#include <lfb.h>

/*	HARDWARE FONT CONTROL		*/
/*		OPTIONAL		*/

SIBool lfbCheckDLFont(fontnum, fontinfo)
SIint32 fontnum;
SIFontInfoP fontinfo;

{
    return(SI_FAIL);
}

SIBool lfbDownLoadFont(fontnum, fontinfo, glyphlist)
SIint32 fontnum;
SIFontInfoP fontinfo;
SIGlyphP glyphlist;

{
    return(SI_FAIL);
}

SIBool lfbFreeFont(fontnum)
SIint32 fontnum;

{
    return(SI_FAIL);
}

SIvoid lfbFontClip(x1, y1, x2, y2)
SIint32 x1, y1, x2, y2;

{
}

SIBool lfbStplbltFont(fontnum, x, y, count, glyphs, forcetype)
SIint32 fontnum, x, y, count, forcetype;
SIint16 *glyphs;

{
    return(SI_FAIL);
}
