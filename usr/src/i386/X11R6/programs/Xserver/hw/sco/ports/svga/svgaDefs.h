/*
 * @(#)svgaDefs.h 11.1
 *
 * Copyright (C) 1993-1994 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */

typedef struct _svgaPrivate {
  unsigned char *fbBase;
  int fbStride;                 /* bytes/scanline */
  int width, height;            /* width and height in pixels */
  int depth;
} svgaPrivate, * svgaPrivatePtr;

extern unsigned int svgaScreenPrivateIndex;

#define SVGA_PRIVATE_DATA(pScreen) \
((svgaPrivatePtr)((pScreen)->devPrivates[svgaScreenPrivateIndex].ptr))


