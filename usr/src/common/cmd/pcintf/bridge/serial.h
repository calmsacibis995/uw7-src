#ident	"@(#)pcintf:bridge/serial.h	1.1.1.2"
/* SCCSID(@(#)serial.h	6.1	LCC);	/* Modified: 14:54:38 10/15/90 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*********************************************************************/
/* serial.h       structure for serial number record
/*********************************************************************/

#define SELECTOR 1      /* the following structure is type 1 */
#define ESNSIZE  8      /* size of the encrypted serial # in bytes */

#ifdef UNIX
#define	SERI_ST	struct seri_st
struct seri_st
#else
#define	SERI_ST	seri_st
typedef struct  /* serial number assignment record structure type */
#endif
{
unsigned short selector;        /* this structure is type 1       */
unsigned short cksum;           /* one's comp checksum            */
unsigned long  ser_no;          /* serial number                  */
unsigned long  c1;              /* encrypted serial number part 1 */
unsigned long  c2;              /* encrypted serial number part 2 */
} 
#ifndef UNIX
seri_st;
#else
;
#endif

/* Used to pass the address and serial number from mapsvr to consvr */
struct seriEthSt
{
SERI_ST	serNum;
char   serAddr[DST];
};

typedef struct /* encrypted serial number in two longs */
{
unsigned long code1;            /* part 1: low 4 bytes */
unsigned long code2;            /* part 2:  hi 4 bytes */
} encr_st;
