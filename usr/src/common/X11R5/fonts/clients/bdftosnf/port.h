#ident	"@(#)r5fonts:clients/bdftosnf/port.h	1.1"

/* $XConsortium: port.h,v 1.2 88/09/06 16:44:49 jim Exp $ */
/*
 * The portable font format has the following properties:
 *	little-endian 16 and 32-bit integers
 *	no padding in structures
 *	lowest-addressed byte of a bitmap is leftmost on screen
 *	least-significant bit within a byte of a bitmap is leftmost on screen
 *	bitmaps are padded only to byte boundaries
 *
 * The "native" font format has the following properties:
 *	native-ended 16 and 32-bit integers
 *	padding in structures is that of the compiler with which the converter
 *		is compiled
 *	byte ordering along a scanline is set by a command-line option
 *	bit ordering within a byte of a bitmap is set by a command-line option
 *	bitmaps are padded only to byte boundaries
 */
#ifndef u_char
#define u_char	unsigned char
#endif

/*
 * put i into 32 portable bits
 */
#define p32( i, pb)	\
	(pb)[0] =  i & 0xff;	\
	(pb)[1] = (i & 0xff00) >> 8;	\
	(pb)[2] = (i & 0xff0000) >> 16;	\
	(pb)[3] = (i & 0xff000000) >> 24;

/*
 * put i into 16 portable bits
 */
#define p16( i, pb)	\
	(pb)[0] =  i & 0xff;	\
	(pb)[1] = (i & 0xff00) >> 8;

/*
 * naturalize 32 portable bits
 */
#define n32( p32)	\
	(((((u_char *)(p32))[3] << 8 | ((u_char *)(p32))[2]) << 8 | ((u_char *)(p32))[1]) << 8 | ((u_char *)(p32))[0])

/*
 * naturalize 16 portable bits
 */
#define n16( p16)	\
	(((u_char *)(p16))[1] << 8 | ((u_char *)(p16))[0])


/*
 * These increment the byte pointer as well.
 * no return value
 */
#define port32( pb, i)	\
	p32( i, pb);	\
	pb += 4;

#define port16( pb, i)	\
	p16( i, pb);	\
	pb += 2;

#define nat32( pb, i)	\
	i = n32( pb);	\
	pb += 4;

#define nat16( pb, i)	\
	i = n16( pb);	\
	pb += 2;


unsigned char	_b32[4];	/* a hidden temporary */
/*
 * These increment the file pointer as well.
 * Don't put more than one of these macros in an expression!
 * no return values
 */
#define put32( i, fp)	\
	p32( (i), _b32);	\
	fwrite( _b32, 4, 1, (fp));

#define put16( i, fp)	\
	p16( (i), _b32);	\
	fwrite( _b32, 2, 1, (fp));

#define get32( i, fp)	\
	( fread( _b32, 4, 1, (fp)), g32( _b32))

#define get16( i, fp)	\
	( fread( _b32, 2, 1, (fp)), g16( _b32))
