#ident	"@(#)kern-i386:util/kdb/scodb/memory.c	1.1.1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	L000	scol!nadeem	23apr92
 *	- ensure that scodb can be used with MPX kernels.  Make use of the
 *	  _gdt() function in getlinear() to pick up the correct GDT of the 
 *	  current cpu (change propagated from mskdb).
 */

#include <util/engine.h>
#include <util/kdb/db_as.h>
#include <util/kdb/db_slave.h>
#include <util/kdb/kdebugger.h>

#include	"dbg.h"


NOTSTATIC
unsigned
getlinear(seg, off)
	unsigned long seg, off;
{
	/* TBD */
	return off;
}

NOTSTATIC
db_getbyte(seg, off, bt)
	unsigned seg, off;
	unsigned char *bt;
{
	unsigned la;
	as_addr_t as;

	la = getlinear(seg, off);
	if (!validaddr(la))
		return 0;

	SET_KVIRT_ADDR_CPU(as, la, myengnum);

	if (db_read(as, bt, 1) == -1)
		return 0;
	else
		return 1;
}

NOTSTATIC
db_getshort(seg, off, sp)
	unsigned seg, off;
	unsigned short *sp;
{
	unsigned la;
	as_addr_t as;

	la = getlinear(seg, off);
	if (!validaddr(la) || !validaddr(la + sizeof(*sp) - 1))
		return 0;

	SET_KVIRT_ADDR_CPU(as, la, myengnum);

	if (db_read(as, sp, 2) == -1)
		return 0;
	else
		return 1;
}

NOTSTATIC
db_getlong(seg, off, lp)
	unsigned seg, off;
	unsigned *lp;
{
	unsigned la;
	as_addr_t as;

	la = getlinear(seg, off);
	if (!validaddr(la) || !validaddr(la + sizeof(*lp) - 1))
		return 0;

	SET_KVIRT_ADDR_CPU(as, la, myengnum);

	if (db_read(as, lp, 4) == -1)
		return 0;
	else
		return 1;
}

NOTSTATIC
db_getmem(seg, off, bf, len)
	unsigned seg, off;
	char *bf;
	int len;
{
	unsigned la;
	as_addr_t as;

	la = getlinear(seg, off);
	if (!validaddr(la) || !validaddr(la + len - 1))
		return 0;

	SET_KVIRT_ADDR_CPU(as, la, myengnum);

	if (db_read(as, bf, len) == -1)
		return 0;
	else
		return 1;
}

NOTSTATIC
db_putbyte_all(seg, off, v)
	unsigned seg, off;
	unsigned char v;
{
	unsigned la;
	as_addr_t as;

	la = getlinear(seg, off);
	if (!validaddr(la))
		return 0;

	SET_KVIRT_ADDR_CPU(as, la, AS_ALL_CPU);

	if (db_write(as, &v, 1) == -1)
		return 0;
	else
		return 1;
}

NOTSTATIC
db_putbyte(seg, off, v)
	unsigned seg, off;
	unsigned char v;
{
	unsigned la;
	as_addr_t as;

	la = getlinear(seg, off);
	if (!validaddr(la))
		return 0;

	SET_KVIRT_ADDR_CPU(as, la, myengnum);

	if (db_write(as, &v, 1) == -1)
		return 0;
	else
		return 1;
}

NOTSTATIC
db_putshort(seg, off, v)
	unsigned seg, off;
	unsigned short v;
{
	unsigned la;
	as_addr_t as;

	la = getlinear(seg, off);
	if (!validaddr(la) || !validaddr(la + sizeof(v) - 1))
		return 0;

	SET_KVIRT_ADDR_CPU(as, la, myengnum);

	if (db_write(as, &v, 2) == -1)
		return 0;
	else
		return 1;
}

NOTSTATIC
db_putlong(seg, off, v)
	unsigned seg, off;
	unsigned long v;
{
	unsigned la;
	as_addr_t as;

	la = getlinear(seg, off);
	if (!validaddr(la) || !validaddr(la + sizeof(v) - 1))
		return 0;

	SET_KVIRT_ADDR_CPU(as, la, myengnum);

	if (db_write(as, &v, 4) == -1)
		return 0;
	else
		return 1;
}

NOTSTATIC
validaddr(la)
	long la;
{
	/* TBD */
	return 1;
}

NOTSTATIC
badaddr(seg, off)
	long seg, off;
{
	printf("Address ");
	pnz(seg, 4);
	putchar(':');
	pnz(off, 8);
	printf(" not present.\n");
}

