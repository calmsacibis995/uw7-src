/*
 *	@(#) ddxMisc.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Sep 15 19:58:38 PDT 1992	mikep@sco.com
 *	- Add ddxBitSwap and init routine.
 *
 */
#include "misc.h"
#include "commonDDX.h"

#ifdef DEBUG
unsigned	ddxVerboseLevel=	0;

void
Notice(unsigned flags,char *format,void *a0,void *a1,void *a2,void *a3,void *a4)
{
    if (flags&ddxVerboseLevel) {
	ErrorF(format,a0,a1,a2,a3,a4);
    }
    return;
}
#endif

unsigned char ddxBitSwap[256];

void
ddxInitBitSwap()
{
        int i, j;
        unsigned char ch;

        for (i = 0; i < 256; ++i)
        {
                ch = 0;
                for (j = 0; j < 8; ++j)
                        if (i & (1 << j))
                                ch |= 1 << (7 - j);
                ddxBitSwap[i] = ch;
        }
}

