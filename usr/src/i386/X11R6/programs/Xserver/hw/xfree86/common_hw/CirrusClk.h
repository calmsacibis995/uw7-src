/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/CirrusClk.h,v 3.1 1995/01/28 15:58:09 dawes Exp $ */





/* $XConsortium: CirrusClk.h /main/3 1995/11/12 19:29:50 kaleb $ */

int CirrusFindClock( 
#if NeedFunctionPrototypes
   int freq, int *num_out, int *den_out, int *usemclk_out
#endif
);     

int CirrusSetClock(
#if NeedFunctionPrototypes
   int freq
#endif
);
