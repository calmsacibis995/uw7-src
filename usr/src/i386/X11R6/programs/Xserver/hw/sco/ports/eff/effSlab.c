/*
 *	@(#) effSlab.c 11.1 97/10/22
 */
/*
 * Modification History
 *
 * S009, 13-Oct-92, mikep
 *	remove dead code
 * S008, 13-Mar-91, mikep
 *	add TransMorgrify array and start using the C version of
 *	effAPBlockOutW().  Remove dead code
 * S007, 20-Sep-91, staceyc
 * 	activate slow AP block outw for broken Matrox boards
 * S006, 30-Aug-91, staceyc
 * 	add better chip diagnostic routines
 * S005, 28-Aug-91, staceyc
 * 	code cleanup
 * S004, 20-Aug-91, staceyc
 * 	added a portable version of across plane block out, should only
 *	be used for debugging
 * S003, 26-Jun-91, staceyc
 * 	use assembler blkout
 * S002, 21-Jun-91, staceyc
 * 	cranked sanity check value - chip keeps very busy with big bitblts
 * S001, 20-Jun-91, staceyc
 * 	added code to assist with chip diagnostics and code debugging
 * S000, 17-Jun-91, staceyc
 * 	created - this will be the major hardware bottleneck and we should
 *	move to the assembler version for production
 */

#include <stdio.h>
#include "X.h"
#include "effConsts.h"
#include "effMacros.h"
#include "effDefs.h"
#include "effProcs.h"

void
effSlowBlockOutW(values, n)
unsigned short *values;
int n;
{
#if EFF_HALFQBUF != 4
	******** ERROR ********
#endif
	for (; n >= EFF_HALFQBUF; n -= EFF_HALFQBUF)
	{
		EFF_CLEAR_QUEUE(EFF_HALFQBUF);
		/* This assumes that EFF_HALFQBUF is 4 */
		EFF_OUT(EFF_VARDATA, *values++);
		EFF_OUT(EFF_VARDATA, *values++);
		EFF_OUT(EFF_VARDATA, *values++);
		EFF_OUT(EFF_VARDATA, *values++);
	}
	if (n)                                   /* Left over < EFF_HALFQBUF */
	{
		EFF_CLEAR_QUEUE(n);
		while (n--)
			EFF_OUT(EFF_VARDATA, *values++);
    	}
}

static unsigned short TransMorgrify[256];			/* S008 vvv */

#define transmogrify( byte ) \
( ( ( byte >> 3 ) | ( ( (unsigned short int) byte ) << 9 ) ) & 0x1E1E )

void
effInitTransMorgrify()
{
	register int i, j;
	register unsigned short *ap_p;
	register unsigned char ch;
	unsigned short *ap_values;

	for (i = 0; i < 256; i++)
	{
#if BITMAP_BIT_ORDER == LSBFirst
                ch = 0;
                for (j = 0; j < 8; ++j)
                        if (i & (1 << j))
                                ch |= 1 << (7 - j);
#else
		ch = i;
#endif
		TransMorgrify[i] = transmogrify(ch);
	
	}
}								/* S008 ^^^ */

void
effAPBlockOutW(values, n)
register unsigned char *values;
int n;
{
	register int i;
	register unsigned short *ap_p;
	register unsigned char tmp;
	unsigned short *ap_values;

	ap_p = ap_values = (unsigned short *)ALLOCATE_LOCAL(n * sizeof(short));
	i = n;
	while (i--)
	{
		tmp = *values++;
		*ap_p++ = TransMorgrify[tmp];			/* S008 */
	}
	effBlockOutW(ap_values, n);
	DEALLOCATE_LOCAL((char *)ap_values);
}

void
effBlockInW(data, n)
unsigned char *data;
int n;
{
	for (n++ ;--n ;data += sizeof (unsigned short int))
		*((unsigned short int *)data) = EFF_INW(EFF_VARDATA);
}

