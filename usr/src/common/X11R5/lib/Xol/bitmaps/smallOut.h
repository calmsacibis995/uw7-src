#ifndef NOIDENT
#ident	"@(#)olbitmaps:bitmaps/smallOut.h	1.2"
#endif

#define smallOut_width 5
#define smallOut_height 5

#ifdef INACTIVE_CURSOR
static unsigned char smallOut_bits[] = {
   0x00,
   0x00,
   0x04, 
   0x0a, 
   0x04};
#else
static unsigned char smallOut_bits[] = {
   0x00,
   0x00,
   0x00,
   0x00,
   0x00
   };
#endif
