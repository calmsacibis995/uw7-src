/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cir_span.h,v 3.1 1994/12/25 12:35:11 dawes Exp $ */





/* $XConsortium: cir_span.h /main/4 1995/11/13 08:21:24 kaleb $ */
/*
 * Definitions for span functions in cir_span.s
 */

#ifdef AVOID_ASM_ROUTINES
#define __FTYPE__ static
#else
#define __FTYPE__ extern
#endif

__FTYPE__ void CirrusColorExpandWriteSpans(
#if NeedFunctionPrototypes
    void *,
    int,
    int,
    int,
    int,
    int,
    int,
    int
#endif
);

__FTYPE__ void CirrusColorExpandWriteStippleSpans(
#if NeedFunctionPrototypes
    void *,
    int,
    int,
    int,
    int,
    int,
    int,
    unsigned long,
    int
#endif
);

__FTYPE__ void CirrusLatchWriteTileSpans(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int
#endif
);

#undef __FTYPE__
