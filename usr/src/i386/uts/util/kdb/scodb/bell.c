#ident	"@(#)kern-i386:util/kdb/scodb/bell.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
*	all this crap from os/bell.c
*/
# define	TONEON		0x03
# define	TONECTL		0x61
# define	TIMERCTL	0x43
# define	TONETIMER	0x42
# define	T_CTLWORD	0xB6

# define	bell_on(bf)	(				\
			iooutb(TIMERCTL, T_CTLWORD),		\
			iooutb(TONETIMER, (bf)),		\
			iooutb(TONETIMER, (bf) >> 8),		\
			iooutb(TONECTL, ioinb(TONECTL) | TONEON)\
		)

# define	bell_off()	(				 \
			iooutb(TONECTL, ioinb(TONECTL) & ~TONEON)\
		)

# define	delay(bt)	{				   \
			for (i = 0;i < (bt);i++)		   \
				;				   \
		}


NOTSTATIC
dobell() {
	int i;

#ifdef NEVER
	bell_on(0x600);
	delay(0x20000);
	bell_off();
#endif
}
