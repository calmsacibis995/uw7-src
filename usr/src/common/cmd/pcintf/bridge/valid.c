#ident	"@(#)pcintf:bridge/valid.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)valid.c	6.2	LCC);	/* Modified: 21:09:17 6/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*
   These are the routine used to validate the 16 byte copy protection
   structure passed during a map request.
*/

#include "sysconfig.h"

#include <memory.h>

#define UNIX
#include "pci_proto.h"
#include "pci_types.h"
#include "const.h"
#include "serial.h"
#include "flip.h"


#define	serial_alg(qq,XOP) \
{ \
	if (qq XOP 0x00000001) \
		qq = qq >> 1; \
	if (qq XOP 0x00000800) \
		qq = qq ^ 0xa247c030; \
	if (qq XOP 0x10001000) \
		qq = qq << 1; \
	if (qq XOP 0x00000010) \
		qq = qq ^ 0x5040000a; \
	if (qq XOP 0x00100008) \
		qq = qq >> 1; \
	if (qq XOP 0x00000800) \
		qq = qq ^ 0xc30055e0; \
	if (qq XOP 0x10001000) \
		qq = qq << 1; \
	if (qq XOP 0x00000010) \
		qq = qq ^ 0x83040301; \
}


LOCAL unsigned long	Encode1		PROTO((unsigned long));
LOCAL unsigned long	Encode2		PROTO((unsigned long, unsigned long));
int			validSerial	PROTO((struct seri_st *, int));


unsigned long
Encode1(snumb)                  /* calculate encode part 1 (code1) */
unsigned long snumb;            /* snumb is serial number */
{
	return (unsigned long)(rand() * snumb + snumb);
}

unsigned long
Encode2(snumb, cd1)             /* calculate encode part 2 (code2) */
unsigned long snumb, cd1;       /* snumb is serial number, cd1 is code1 */
{
extern int oldserial;
unsigned long
	ss, rr, aa, bb, jj, mm, pp, qq;

	ss = snumb;
	rr = cd1;

	for (aa = ss, jj = 23; (aa < 0x08000000) && (jj < 40); jj++)
		aa = aa * aa + jj;

	for (bb = rr; (bb < 0x08000000) && (jj < 60); jj++)
		bb = bb * bb + jj;

	mm = aa ^ bb;
	pp = ((mm >> 5) * ss) + ((mm << 6) + rr) ^ mm;
	qq = pp ^ 0x732514f6;

	if (oldserial) {
		serial_alg(qq,&&);
	} else {
		serial_alg(qq,&);
	}

	return qq;
}

int
validSerial(eSerialP, flipCode)
struct seri_st 
	*eSerialP;
int
	flipCode;
{
unsigned long
	tl;
struct seri_st 
	eSerial;

	serialCpy(&eSerial, eSerialP);
	lflipm(eSerial.c1, tl, flipCode);
	lflipm(eSerial.c2, tl, flipCode);
	lflipm(eSerial.ser_no, tl, flipCode);

	return (eSerial.c2 == Encode2(eSerial.ser_no, eSerial.c1));
}
