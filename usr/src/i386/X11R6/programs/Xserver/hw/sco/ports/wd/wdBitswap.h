/*
 *  @(#) wdBitswap.h 11.1 97/10/22
 *
 * Copyright (C) 1992-1993 The Santa Cruz Operation, Inc.
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
 * wdBitswap.h
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *
 *	S001	Tue 05-Jan-1993	buckm@sco.com
 *		Use ddxBitSwap[] instead of our own private table.
 *              
 */
/*
 *    Table and macro to convert mono images from
 *    MSBfirst to LSBfirst bitorder and back
 */

extern unsigned char ddxBitSwap[];

#define BITSWAP( byte ) ddxBitSwap[byte]

