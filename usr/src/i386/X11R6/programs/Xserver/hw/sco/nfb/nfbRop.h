/*
 *	@(#) nfbRop.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Fri Dec 04 03:48:41 PST 1992	buckm@sco.com
 *	- Fix RopWith2Masks to pass arguments to RopWithMask
 *	in the correct order; ropexpr should be first, not last!
 */

extern unsigned char rop_needs_src[], rop_needs_dst[];

typedef void (*ropfcn)();

extern ropfcn nfbRopFcn[];
extern ropfcn nfbMaskRopFcn[];

#define RopWithMask(ropexpr, dst, mask) \
	(((dst) & ~(mask)) | ((ropexpr) & (mask)))

#define RopWith2Masks(ropexpr, dst, mask1, mask2) \
	(((dst) & ~(mask1)) | (RopWithMask(ropexpr, dst, mask2) & (mask1)))

#ifndef fnCLEAR
/* The following are borrowed from cfb/mfb */
#define fnCLEAR(src, dst)	(0)
#define fnAND(src, dst) 	((src) & (dst))
#define fnANDREVERSE(src, dst)	((src) & ~(dst))
#define fnCOPY(src, dst)	(src)
#define fnANDINVERTED(src, dst)	(~(src) & (dst))
#define fnNOOP(src, dst)	(dst)
#define fnXOR(src, dst)		((src) ^ (dst))
#define fnOR(src, dst)		((src) | (dst))
#define fnNOR(src, dst)		(~((src) | (dst)))
#define fnEQUIV(src, dst)	(~(src) ^ (dst))
#define fnINVERT(src, dst)	(~(dst))
#define fnORREVERSE(src, dst)	((src) | ~(dst))
#define fnCOPYINVERTED(src, dst)(~(src))
#define fnORINVERTED(src, dst)	(~(src) | (dst))
#define fnNAND(src, dst)	(~((src) & (dst)))
#define fnSET(src, dst)		(~0)
#endif

#define ClearWithMask(dst, mask) \
	((dst) & ~(mask))

#define ClearWith2Masks(dst, mask1, mask2) \
	(((dst) & ~(mask1)) | (ClearWithMask(dst, mask2) & (mask1)))

#define SetWithMask(dst, mask) \
	((dst) | (mask))

#define SetWith2Masks(dst, mask1, mask2) \
	(((dst) | (mask1)) | (SetWithMask(dst, mask2) & (mask1)))

/*
 * NfbDoRop -  modified from DoRop in cfb.h
 *		assumes that alu != GXcopy && alu != GXnoop.
 */
#define NfbDoRop(alu, src, dst) \
    (((alu) >= GXnor) ? \
     (((alu) >= GXcopyInverted) ? \
       (((alu) >= GXnand) ? \
         (((alu) == GXnand) ? ~((src) & (dst)) : ~0) : \
         (((alu) == GXcopyInverted) ? ~(src) : (~(src) | (dst)))) : \
       (((alu) >= GXinvert) ? \
	 (((alu) == GXinvert) ? ~(dst) : ((src) | ~(dst))) : \
	 (((alu) == GXnor) ? ~((src) | (dst)) : (~(src) ^ (dst)))) ) : \
     (((alu) >= GXandInverted) ? \
       (((alu) >= GXxor) ? \
	 (((alu) == GXxor) ? ((src) ^ (dst)) : ((src) | (dst))) : \
	 (~(src) & (dst))) : \
       (((alu) >= GXandReverse) ? \
	 (((alu) == GXandReverse) ? ((src) & ~(dst)) : (src)) : \
	 (((alu) == GXand) ? ((src) & (dst)) : 0))))

/*
 * NfbDoRopFull -  like NfbDoRop, but allows GXcopy.
 */
#define NfbDoRopFull(alu, src, dst) \
( ((alu) == GXcopy) ? (src) : NfbDoRop(alu, src, dst) )
