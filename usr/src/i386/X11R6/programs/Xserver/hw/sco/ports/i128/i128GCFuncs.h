/*
 * @(#) i128GCFuncs.h 11.1 97/10/22
 *
 * Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */
/*
 * i128GCFuncs.h
 */

#ifdef I128_FAST_GC_OPS
#ifndef I128_GC_FUNCS_INCLUDE
#define I128_GC_FUNCS_INCLUDE

#include "i128Defs.h"

Bool i128CreateGC(GCPtr);

#endif /* #ifndef I128_GC_FUNCS_INCLUDE */
#endif /* #ifdef I128_FAST_GC_OPS */
