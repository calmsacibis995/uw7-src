/*
 *  @(#) wdBankMap.h 11.1 97/10/22
 *
 * Copyright (C) 1994 The Santa Cruz Operation, Inc.
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
 *     wdBankMap.h     WD90C31 bank mapping macros
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 08-Mar-1993	buckm@sco.com
 *              created.
 *
 *	S001	Thu Apr 14 10:24:36 PDT 1994	hiramc@sco.COM
 *		Let's not have cast's to the left of = signs
 *		fixed WD_REMAP, WD_MAP_AHEAD, and WD_MAP_BEHIND
 */

/*
 * Bank mapping - 1 64k bank on 4k boundaries
 *
 * 'addr's are frame buffer offsets;
 * 'vaddr's are pointers in the virtual address range of the frame buffer
 */


/*
 * internal defs and macros; not for outside use
 */

#define	BANKSZ		0x10000			/* 64k banks */
#define	PGSZ		0x1000			/*  4K pages */
#define	PGMASK		(PGSZ - 1)
#define	PGSHFT		12
#define	PGNO(addr)	((addr) >> PGSHFT)	/* page number */
#define	PGOFF(addr)	((addr) & PGMASK)	/* page offset */
#define	PGBASE(addr)	((addr) & ~PGMASK)	/* page base */

/* what is 'addr's offset from the current bank ? */

#define WD_BANKOFF(wdPriv, addr) \
	( (addr) - (wdPriv)->bankBase )

/* what is 'vaddr's offset from the current bank ? */

#define WD_VBANKOFF(wdPriv, vaddr) \
	( (int)(vaddr) - (int)((wdPriv)->fbBase) )

/* is the range [bankoff::bankoff+size-1] currently mapped ? */

#define	WD_ISMAPPED(bankoff, size) \
	( ((bankoff) >= 0) && ((bankoff) <= (BANKSZ - (size))) )

/* is the range [bankoff-size::bankoff-1] currently mapped ? */

#define	WD_ISMAPPEDBACK(bankoff, size) \
	( ((bankoff) >= (size)) && ((bankoff) <= BANKSZ) )

/* set bank to map 'addr'; the bank extends mostly ahead of 'addr' */

#define	WD_DOMAP(wdPriv, addr) \
	{ \
	    outw(0x3CE, (PGNO(addr) << 8) | 0x09); \
	    (wdPriv)->bankBase = PGBASE(addr); \
	}

/* slide bank up or down by 'delta' bytes, adjusting 'vaddr';	*/
/*  'delta' must be a multiple of PGSZ				*/ 

#define	WD_REMAP(wdPriv, vaddr, delta) \
	{ \
	    outw(0x3CE, (PGNO((wdPriv)->bankBase += (delta)) << 8) | 0x09); \
	    (vaddr) = (pointer) ( (int)(vaddr) - (delta) ); \
	}


/*
 * macros for external use
 *
 * use WD_REMAP_AHEAD calls after an initial WD_MAP_AHEAD call
 * when moving forward through the frame buffer.
 *
 * use WD_REMAP_BEHIND calls after an initial WD_MAP_BEHIND call
 * when moving backward through the frame buffer.
 */


/*
 * put mapping into a known state
 */

#define	WD_MAP_RESET(wdPriv)	WD_DOMAP(wdPriv, 0)

/* 
 * make sure the 'size' bytes ahead starting at 'addr' are mapped,
 * i.e. the range [addr::addr+size-1].
 * set the pointer 'vaddr' to the virtual address of 'addr'.
 */

#define	WD_MAP_AHEAD(wdPriv, addr, size, vaddr) \
	{ \
	    int bankoff = WD_BANKOFF(wdPriv, addr); \
	    if (!WD_ISMAPPED(bankoff, size)) \
	    { \
		WD_DOMAP(wdPriv, addr); \
		bankoff = WD_BANKOFF(wdPriv, addr); \
	    } \
	    (vaddr) = (pointer) ( (int)((wdPriv)->fbBase) + bankoff ); \
	}

/* 
 * check 'vaddr' to see if it has been _incremented_ too far;
 * make sure the 'size' bytes ahead starting at 'vaddr' are mapped.
 * adjust 'vaddr' if necessary.
 */

#define	WD_REMAP_AHEAD(wdPriv, vaddr, size) \
	{ \
	    int bankoff = WD_VBANKOFF(wdPriv, vaddr); \
	    if (bankoff > (BANKSZ - (size))) \
	    { \
		int delta = PGBASE(bankoff) - PGSZ; \
		WD_REMAP(wdPriv, vaddr, delta); \
	    } \
	}

/* 
 * make sure the 'size' bytes back from 'vaddr' are mapped,
 * i.e. the range ([size-vaddr::vaddr-1]).
 * set the pointer 'vaddr' to the virtual address of 'addr'.
 * NOTE: 'vaddr' may not be mapped; use only 'vaddr'-1 and lower address.
 */
#define	WD_MAP_BEHIND(wdPriv, addr, size, vaddr) \
	{ \
	    int bankoff = WD_BANKOFF(wdPriv, addr); \
	    if (!WD_ISMAPPEDBACK(bankoff, size)) \
	    { \
		int backaddr = (addr) + PGMASK - BANKSZ; \
		WD_DOMAP(wdPriv, backaddr); \
		bankoff = WD_BANKOFF(wdPriv, addr); \
	    } \
	    (vaddr) = (pointer) ( (int)((wdPriv)->fbBase) + bankoff ); \
	}

/* 
 * check 'vaddr' to see if it has been _decremented_ too far;
 * make sure the 'size' bytes back from 'vaddr' are mapped.
 * adjust 'vaddr' if necessary.
 */

#define	WD_REMAP_BEHIND(wdPriv, vaddr, size) \
	{ \
	    int bankoff = WD_VBANKOFF(wdPriv, vaddr); \
	    if (bankoff < (size)) \
	    { \
		int delta = PGBASE(bankoff) + 2 * PGSZ - BANKSZ; \
		WD_REMAP(wdPriv, vaddr, delta); \
	    } \
	}
