#ident	"@(#)ihvkit:display/vga256/newfill.h	1.1"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

/*
 * File : 
 *	newfill.h
 *
 * Description : 
 *	contains the macros from MIT's cfb server code to be used by our
 *	cfb-style stippler.
 */

extern int				V256Rrop;
extern unsigned long	V256And,V256Xor;
extern int 				cfb8StippleMode; 
extern int				cfb8StippleAlu; 
extern int				cfb8StippleRRop;
extern unsigned long	cfb8StippleFg; 
extern unsigned long	cfb8StippleBg; 
extern unsigned long	cfb8StipplePm;
extern unsigned long	cfb8StippleAnd[16]; 
extern unsigned long	cfb8StippleXor[16];



#define PPW 		4
#define PLST		3
#define PIM 		0x03
#define PWSH    	2
#define PSZ 		8
#define PMSK    	0xFF

#define cfb8PixelMasks            cfb8StippleMasks

extern int          	cfb8SetStipple (), cfb8SetOpaqueStipple();
extern int		cfbReduceRasterOp();
#define cfb8CheckOpaqueStipple(alu,fg,bg,pm) \
    ((SGOPQStipple == cfb8StippleMode && \
      (alu) == cfb8StippleAlu && \
      ((fg) & PMSK) == cfb8StippleFg && \
      ((bg) & PMSK) == cfb8StippleBg && \
      ((pm) & PMSK) == cfb8StipplePm) ? 0 : cfb8SetOpaqueStipple(alu,fg,bg,pm))

#define cfb8CheckStipple(alu,fg,pm) \
    ((SGStipple == cfb8StippleMode && \
      (alu) == cfb8StippleAlu && \
      ((fg) & PMSK) == cfb8StippleFg && \
      ((pm) & PMSK) == cfb8StipplePm) ? 0 : cfb8SetStipple(alu,fg,pm))


/*
 * Note that the shift direction is independent of the byte ordering of the
 * machine.  The following is portable code.
 */
#define PFILL(p) ( ((p)&PMSK)          | \
           ((p)&PMSK) <<   PSZ | \
           ((p)&PMSK) << 2*PSZ | \
           ((p)&PMSK) << 3*PSZ )
#define PFILL2(p, pf) { \
    pf = (p) & PMSK; \
    pf |= (pf << PSZ); \
    pf |= (pf << 2*PSZ); \
}

/*
 * VATS:
 * NOTE : This macro will change if GetFourBits macro is implemented
 */
#define GetFourPixels(x)        (cfb8StippleXor[x])


/*
 * Reduced raster op - using precomputed values, perform the above
 * in three instructions
 */

#define DoRRop(dst, and, xor)   (((dst) & (and)) ^ (xor))

#define	DoMaskRRop(dst, and, xor, mask) \
    (((dst) & ((and) | ~(mask))) ^ (xor & mask))

#define MaskRRopPixels(dst,x,mask)  (DoMaskRRop(dst,cfb8StippleAnd[x], \
									cfb8StippleXor[x], mask))

#define RRopPixels(dst,x)       (DoRRop(dst,cfb8StippleAnd[x], \
									cfb8StippleXor[x]))

#define RRopFourBits(dst,bits)                                  \
    {                                                           \
		*(dst) = RRopPixels(*(dst),bits);                           \
    }

#define WriteFourBits(dst,pixel,bits) \
        switch (bits) {                 \
        case 0:                         \
            break;                      \
        case 1:                         \
            ((char *) (dst))[0] = (pixel);      \
            break;                      \
        case 2:                         \
            ((char *) (dst))[1] = (pixel);      \
            break;                      \
        case 3:                         \
            ((short *) (dst))[0] = (pixel);     \
            break;                      \
        case 4:                         \
            ((char *) (dst))[2] = (pixel);      \
            break;                      \
        case 5:                         \
            ((char *) (dst))[0] = (pixel);      \
            ((char *) (dst))[2] = (pixel);      \
            break;                      \
        case 6:                         \
            ((char *) (dst))[1] = (pixel);      \
            ((char *) (dst))[2] = (pixel);      \
            break;                      \
        case 7:                         \
            ((short *) (dst))[0] = (pixel);     \
            ((char *) (dst))[2] = (pixel);      \
            break;                      \
        case 8:                         \
            ((char *) (dst))[3] = (pixel);      \
            break;                      \
        case 9:                         \
            ((char *) (dst))[0] = (pixel);      \
            ((char *) (dst))[3] = (pixel);      \
            break;                      \
        case 10:                        \
            ((char *) (dst))[1] = (pixel);      \
            ((char *) (dst))[3] = (pixel);      \
            break;                      \
        case 11:                        \
            ((short *) (dst))[0] = (pixel);     \
            ((char *) (dst))[3] = (pixel);      \
            break;                      \
        case 12:                        \
            ((short *) (dst))[1] = (pixel);     \
            break;                      \
        case 13:                        \
            ((char *) (dst))[0] = (pixel);      \
            ((short *) (dst))[1] = (pixel);     \
            break;                      \
        case 14:                        \
            ((char *) (dst))[1] = (pixel);      \
            ((short *) (dst))[1] = (pixel);     \
            break;                      \
        case 15:                        \
            ((long *) (dst))[0] = (pixel);      \
            break;                      \
        }


