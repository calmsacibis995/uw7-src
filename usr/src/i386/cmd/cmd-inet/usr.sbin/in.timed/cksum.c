#ident	"@(#)cksum.c	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

#include <sys/types.h>
#include <sys/byteorder.h>

in_cksum(mp, len)
	char		*mp;
	int             len;
{
	register u_long sum;	/* Assume long > 16 bits */
	register int    mlen;
	register u_short fswap;
	register int    nwords;

	sum = 0L;
	fswap = 0;

	if (len > 0) {
		if ((int) mp & 1) {
			fswap = ~fswap;
			sum = ((sum & 0xff) << 8) | sum >> 8;
			sum += *mp;
			sum = (sum & 0xffff) + (sum >> 16);
			len--;
			mp++;
		}

		nwords = len >> 1;
		while (nwords--) {
			sum += *(ushort *) mp;
			mp += 2;
		}
		while (sum & ~0xffff) {
			sum = (sum & 0xffff) + (sum >> 16);
		}
		if (len & 1) {
			fswap = ~fswap;
			sum = ((sum & 0xff) << 8) | sum >> 8;
			sum += *mp;
			sum = (sum & 0xffff) + (sum >> 16);
		}
	}
	if (fswap)
		sum = ((sum & 0xff) << 8) | sum >> 8;

	/*
	 * The following is necessary for Intel byte order machines.
	 */
	sum = ((sum & 0xff) << 8) | sum >> 8;

	return ((u_short) ~ sum);
}
