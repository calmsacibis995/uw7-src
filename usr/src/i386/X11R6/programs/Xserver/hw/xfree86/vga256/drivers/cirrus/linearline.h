/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/linearline.h,v 3.1 1995/01/28 16:12:08 dawes Exp $ */





/* $XConsortium: linearline.h /main/2 1995/11/13 08:21:38 kaleb $ */

/* linearline.c */
extern void LinearFramebufferVerticalLine(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferDualVerticalLine(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferSlopedLineLeft(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferSlopedLineRight(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferSlopedLineVerticalLeft(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferSlopedLineVerticalRight(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int,
    int
#endif
);
