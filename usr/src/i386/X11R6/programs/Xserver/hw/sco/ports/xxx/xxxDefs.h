/*
 * @(#) xxxDefs.h 11.1 97/10/22
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

typedef struct _xxxPrivate {
    unsigned char *fbBase;
} xxxPrivate, * xxxPrivatePtr;

extern unsigned int xxxScreenPrivateIndex;

#define XXX_PRIVATE_DATA(pScreen) \
((xxxPrivatePtr)((pScreen)->devPrivates[xxxScreenPrivateIndex].ptr))


