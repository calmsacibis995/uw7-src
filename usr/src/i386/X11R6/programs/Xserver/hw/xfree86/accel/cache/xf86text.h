/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/cache/xf86text.h,v 3.2 1995/01/28 16:57:46 dawes Exp $ */

/*
 * Function prototypes for the text output functions.
 */

/* $XConsortium: xf86text.h /main/4 1995/11/13 10:11:12 kaleb $ */

#ifndef _XF86_TEXT_H
#define _XF86_TEXT_H

/*
 * Entrypoints into text code.
 */

void xf86InitText(
#if NeedFunctionPrototypes
    void (*/*GlyphWriteFunc*/)(int, int, int, unsigned char *,
			       CacheFont8Ptr, GCPtr, BoxPtr, int),
    int (*/*NoCPolyTextFunc*/)(DrawablePtr, GCPtr, int, int, int, char*, Bool),
    int (*/*NoCImageTextFunc*/)(DrawablePtr, GCPtr, int, int, int,
				 char *, Bool)
#endif
);

int xf86PolyText8(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char * /*chars*/
#endif
);

int xf86PolyText16(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    unsigned short * /*chars*/
#endif
);

void xf86ImageText8(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char * /*chars*/
#endif
);

void xf86ImageText16(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    unsigned short * /*chars*/
#endif
);

#endif /* _XF86_TEXT_H */
