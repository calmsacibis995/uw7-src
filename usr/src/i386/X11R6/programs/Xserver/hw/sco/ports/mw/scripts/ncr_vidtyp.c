/*
 *	@(#)ncr_vidtyp.c	6.2	4/2/96	15:02:11
 *
 * Copyright (C) 1992-1996 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for 
 * purposes authorized by the license agreement provided they include
 * this notice and the associated copyright notice with any such 
 * product.  The information in this file is provided "AS IS" without 
 * warranty.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S001	Tue Apr  2 15:00:01 PST 1996	hiramc@sco.COM
 *	- can now compile on UW 2.1 - see gemini ifdefs
 *      S000    Tue May 30 12:24:06 PDT 1995    davidw@sco.com
 *      - MAP_CLASS VGA not available during installation (boot kernel).
 *	Use IOPrivl sysi86 access method.
 */

/* Copyright (c) 1992 NCR Corporation, Dayton Ohio, USA */

/*
 *
 * Description:	This program detects the type of NCR VGA
 *		Graphics chip in a system.
 *
 * Arguments:	NONE
 *
 * Returns:	0 = NCR 77C22
 *		1 = NCR 77C21
 *		2 = NCR 77C22E
 *	       -1 = ERROR
 *	       99 = NOT AN NCR
 *		* = UNKNOWN
 *
 */


#include <stdio.h>
#if	! defined(gemini)
#include <sys/console.h>
#endif
#include <sys/proc.h>
#include <sys/v86.h>
#include <sys/sysi86.h>

#if defined(gemini)
static void set_iopl( void );
#endif

char *strsearch();

main()
{
	unsigned char value;
	int fd;
	unsigned short seg, off;
	unsigned long int10;
	char buf[32];

	/*
	 * First, poke around BIOS and ROM stuff.
	 */

	if ((fd = open("/dev/mem", 0)) < 0) {
		perror("can not open /dev/mem");
		exit(-1);
	}
		
	/* get int10 vector */
	lseek(fd, 4 * 10L, 0);
	if ((read(fd, &off, sizeof off) != sizeof off) ||
	    (read(fd, &seg, sizeof seg) != sizeof seg)) {
		perror("error reading /dev/mem");
		exit(-1);
	}
	int10 = (seg << 4) + off;
	/* fprintf(stderr, "int10 = 0x%05x\n", int10); */

	/* for now, these chips are on the motherboard */
	if ((int10 < 0xF0000) || (int10 > 0xFFFFF))
		exit(99);

	/* get last 32 bytes of sytem rom */
	lseek(fd, 0x100000L - sizeof buf, 0);
	if (read(fd, buf, sizeof buf) != sizeof buf) {
		perror("error reading /dev/mem");
		exit(-1);
	}

	/* search for oem */
	if (strsearch("NCR", buf, sizeof buf) == NULL)
		exit(99);
	/* fprintf(stderr, "found NCR\n", int10); */


	/*
	 * Now, interrogate the VGA itself.
	 */
	/* get unlimited i/o privilege */
#if defined(gemini)
	set_iopl();
#else
	if (sysi86(SI86V86,V86SC_IOPL,0x3000) < 0) {
		perror("sysi86 IOPrivl failed");
		exit(-1);
	}
#endif

	/* SEQ 5 - enable extended function registers */
	outb(0x3c4, 5);
	outb(0x3c5, 1);

	/* SEQ 8 - version number register */
	outb(0x3c4, 8);
	value = inb(0x3c5);

	/* Bits 4-7 hold the NCR VGA product number */
	exit (value >> 4);

}

char *
strsearch(str, buf, len)
char *str;
char *buf;
int len;
{
	int n;

	if ((n = strlen(str)) == 0)
		return(buf);

	for (len -= n; len > 0; --len, ++buf)
		if (strncmp(str, buf, n) == 0)
			return(buf);

	return(NULL);
}

#if defined(gemini)

#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>

#define SI86IOPL           112          /* in sysi86.h, for ESMP */

static void
set_iopl()
{
        _abi_sysi86(SI86IOPL, PS_IOPL>>12);
}
#endif  /*      gemini  */
