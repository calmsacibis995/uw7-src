#ident	"@(#)pcintf:bridge/rs232.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)rs232.c	6.2	LCC);	/* Modified: 21:08:49 6/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#ifdef RS232PCI

#include <memory.h>
#include <stdio.h>

#include "pci_types.h"
#include "common.h"


/*
 *	unstuff()	-	Scans an input frame and unstuffs the
 *				"unprotected" portion of the data frame
 *				by removing a SYNC from the data when
 *				it encounters two adjacent SYNC's.
 */

int 
unstuff(iptr, optr, len)
register char	*iptr;
register char	*optr;
int len;
{
    register int 
	i, 
	cnt;

/* Copy "protected" header: do not unstuff */
    for (i = 0; i < PROTECTED; i++) 
	*optr++ = *iptr++;

    cnt = PROTECTED;

/* UNSTUFF frame */
    for (; i < len; i++) {
	if (cnt < sizeof(struct input)) {
		if (*iptr != SYNC)
		    *optr++ = *iptr++;
		else if (*(iptr+1) == SYNC) {
		    *optr++ = SYNC;
		    iptr += 2;
		    ++i;
		} else
			cnt--;
	}
	cnt++;
    }
    return cnt;
}


/*
 *	stuff() -		Character stuffs an output stream.
 */

int
stuff(ptr, length)
register char *ptr;
register int length;
{
    register char
	*usrbufptr,		/* Pointer to begining of user buffer */
	*tmptr;

    char
	tmpbuf[MAX_READ];

/* If the data contains a SYNC2 stuff an extra one */
    for (usrbufptr = ptr, tmptr = tmpbuf; length > 0; length--, ptr++) {
	if (*ptr == SYNC_2) {
	    *tmptr++ = SYNC_2;
	    *tmptr++ = SYNC_2;
	} 
	else
	    *tmptr++ = *ptr;
    }

/* Repack data and return new length */
    length = tmptr - tmpbuf;
    memcpy(usrbufptr, tmpbuf, length);
    return length;
}


/*
 *	chksum() -		Calculates a sort checksum on data frames.
 */

unsigned short 
chksum(ptr, len)
register char *ptr;
register int len;
{
    register int 
	chks;

    /* len used to be declared as a short; now make it explicitly short */
    len = (short)len;

    ptr += 4;
    for (chks = 0; len > 4; len--)
	chks += (0x00ff & *ptr++);
    return (unsigned short)chks;
}


#ifdef RS232_7BIT
/*
 *  convert_to_7_bits() - convert an 8 bit buffer into a 7 bit buffer
 */

int
convert_to_7_bits(bufp, len)
unsigned char *bufp;
int len;
{
	register unsigned char *bp, *bp7;
	register int i, l;
	unsigned char tmpbuf[((MAX_OUTPUT+HEADER)*8+6)/7 + 1];

	bp = bufp;
	bp7 = tmpbuf;

	/* Copy "protected" header: do not convert */
	for (i = 0; i < PROTECTED; i++) 
		*bp7++ = *bp++;
	len -= PROTECTED;

	l = HEADER - PROTECTED;
	while (l >= 7) {
		*bp7 = 0;
		for (i = 0; i < 7; i++) {
			if (bp[i] & 0x80)
				*bp7 |= 1 << i;
		}
		bp7++;
		for (i = 0; i < 7; i++)
			*bp7++ = *bp++ & 0x7f;
		len -= 7;
		l -= 7;
	}
	if (l > 0) {
		*bp7 = 0;
		for (i = 0; i < l; i++) {
			if (bp[i] & 0x80)
				*bp7 |= 1 << i;
		}
		bp7++;
		for (i = 0; i < l; i++)
			*bp7++ = *bp++ & 0x7f;
		len -= l;
	}
	while (len >= 7) {
		*bp7 = 0;
		for (i = 0; i < 7; i++) {
			if (bp[i] & 0x80)
				*bp7 |= 1 << i;
		}
		bp7++;
		for (i = 0; i < 7; i++)
			*bp7++ = *bp++ & 0x7f;
		len -= 7;
	}
	if (len > 0) {
		*bp7 = 0;
		for (i = 0; i < len; i++) {
			if (bp[i] & 0x80)
				*bp7 |= 1 << i;
		}
		bp7++;
		for (i = 0; i < len; i++)
			*bp7++ = *bp++ & 0x7f;
	}

	/* Repack data and return new length */
	i = bp7 - tmpbuf;
	memcpy(bufp, tmpbuf, i);
	return i;
}


/*
 *  convert_from_7_bits() - convert a 7 bit buffer into an 8 bit buffer
 */

int
convert_from_7_bits(bufp, len)
unsigned char *bufp;
int len;
{
	register unsigned char *bp7, *bp;
	register int i, l;

	bp = bp7 = &bufp[PROTECTED];

	/* Skip "protected" header: do not convert */
	len -= PROTECTED;

	l = HEADER - PROTECTED;
	while (l >= 7) {
		for (i = 0; i < 7; i++) {
			if (*bp7 & (1 << i))
				bp7[i+1] |= 0x80;
		}
		bp7++;
		for (i = 0; i < 7 && l > 0; i++)
			*bp++ = *bp7++;
		len -= 8;
		l -= 7;
	}
	if (l > 0) {
		for (i = 0; i < l; i++) {
			if (*bp7 & (1 << i))
				bp7[i+1] |= 0x80;
		}
		bp7++;
		for (i = 0; i < l; i++)
			*bp++ = *bp7++;
		len -= 1 + l;
	}
	while (len >= 8) {
		for (i = 0; i < 7; i++) {
			if (*bp7 & (1 << i))
				bp7[i+1] |= 0x80;
		}
		bp7++;
		for (i = 0; i < 7; i++)
			*bp++ = *bp7++;
		len -= 8;
	}
	if (len > 0) {
		for (i = 0; i < len-1; i++) {
			if (*bp7 & (1 << i))
				bp7[i+1] |= 0x80;
		}
		bp7++;
		for (i = 0; i < len-1; i++)
			*bp++ = *bp7++;
	}

	/* return new length */
	return bp - bufp;
}
#endif /* RS232_7BIT */

#endif /* RS232PCI */
