/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/mach8/mach8cache.h,v 3.4 1995/01/28 17:00:27 dawes Exp $ */






/* $XConsortium: mach8cache.h /main/3 1995/11/12 18:00:05 kaleb $ */

void mach8GlyphWrite(
#if NeedFunctionPrototypes
    int /*x*/,
    int /*y*/,
    int /*count*/,
    unsigned char */*chars*/,
    CacheFont8Ptr /*fentry*/,
    GCPtr /*pGC*/,
    BoxPtr /*pBox*/,
    int /*numRects*/
#endif
);

int mach8NoCPolyText(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char */*chars*/,
    Bool /*is8bit*/
#endif
);

int mach8NoCImageText(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char */*chars*/,
    Bool /*is8bit*/
#endif
);

void mach8CacheMoveBlock(
#if NeedFunctionPrototypes
    int /*srcx*/,
    int /*srcy*/,
    int /*dstx*/,
    int /*dsty*/,
    int /*h*/,
    int /*len*/,
    unsigned int /*id*/
#endif
);


void mach8FontOpStipple(
#if NeedFunctionPrototypes
    int /*x*/,
    int /*y*/,
    int /*w*/,
    int /*h*/,
    unsigned char */*psrc*/,
    int /*pwidth*/,
    Pixel /*id*/
#endif
);
