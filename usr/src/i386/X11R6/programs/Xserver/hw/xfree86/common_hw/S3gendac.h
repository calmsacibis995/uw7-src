/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/S3gendac.h,v 3.4 1995/10/21 11:43:06 dawes Exp $ */


/* Jon Tombs <jon@esix2.us.es>  */


/* $XConsortium: S3gendac.h /main/5 1995/11/12 19:30:20 kaleb $ */

#define GENDAC_INDEX	     0x3C8
#define GENDAC_DATA	     0x3C9
#define BASE_FREQ         14.31818   /* MHz */

int S3gendacSetClock( 
#if NeedFunctionPrototypes
   long freq, int clock
#endif
);     

int ET4000gendacSetClock( 
#if NeedFunctionPrototypes
   long freq, int clock
#endif
);     

int ET4000gendacSetpixmuxClock( 
#if NeedFunctionPrototypes
   long freq, int clock
#endif
);     

int ICS5342SetClock( 
#if NeedFunctionPrototypes
   long freq, int clock
#endif
);     

int S3TrioSetClock( 
#if NeedFunctionPrototypes
   long freq, int clock
#endif
);     

int commonCalcClock(
#if NeedFunctionPrototypes
   long freq,
   int min_n2,
   long freq_min, long freq_max,
   unsigned char *mdiv, *ndiv
#endif
);

