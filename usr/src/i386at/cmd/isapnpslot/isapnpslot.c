/*
 *	@(#)isapnpslot.c	6.1	9/9/97	15:36:37
 * File isapnpslot.c
 * Cmd-line util. to print out info on current ISA Plug & Play devices.
 *
 * @(#) isapnpslot.c 65.5 97/07/23 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "sys/pnp.h"
#include <malloc.h>

static int		pnpFd = -1;
static PnP_busdata_t	pnpBus;

static int	open_PnP(void);
static int	have_isa(void);
static const char *PnP_idStr(u_long vendor);


int
main(int argc, char **argv)
{
    u_int		node;

    if (!have_isa())
	return 0;		/* don't do anything */

    for (node = 1; node <= (u_int)pnpBus.NumNodes; ++node)
    {
	u_long		resNum;
	PnP_findUnit_t	up;
	u_long		totalLength = 0;
	u_char		chkSum = 0;
	PnP_Active_t	act;
	char		activestr[256];

	up.unit.vendor = PNP_ANY_VENDOR;
	up.unit.serial = PNP_ANY_SERIAL;
	up.unit.unit = PNP_ANY_UNIT;
	up.unit.node = node;

	if (ioctl(pnpFd, PNP_FIND_UNIT, &up) == -1)
	{
	    fprintf(stderr, "PNP_FIND_UNIT fail: %s\n", strerror(errno));
	    return errno;
	}

	act.unit.vendor = up.unit.vendor;
	act.unit.serial = up.unit.serial;
	act.unit.unit = up.unit.unit;
	act.unit.node = node;

	/*
	** Note: This loop will hang if there are actually
	** 256 logical devices on the card because act.device
	** will overflow.
	*/
	for(act.device=0; ioctl(pnpFd, PNP_READ_ACTIVE, &act)!=-1; ++act.device)
		activestr[act.device]=(act.active) ? 'Y' : 'N';
	activestr[act.device]='\0';

	/* Print out the data */
	/*	node	unit	vendor	serial	active	*/
	/*	(dec)	(dec)	(str)	(hex)	(str)	*/
	printf("%d %d %s %8.8lx %s\n",
					up.unit.node,
					up.unit.unit,
					PnP_idStr(up.unit.vendor),
					up.unit.serial,
					activestr);
    }

    return 0;
}

static int
have_isa(void)
{
    static int	gotBus = 0;

    if (!gotBus)
    {
	memset(&pnpBus, 0, sizeof(pnpBus));
	pnpBus.BusType = pnp_bus_none;
	gotBus = 1;

	if (open_PnP())
	    return 0;

	if (ioctl(pnpFd, PNP_BUS_PRESENT, &pnpBus) == -1)
	{
	    fprintf(stderr, "PNP_BUS_PRESENT fail: %s\n", strerror(errno));
	    return 0;
	}
    }

    return pnpBus.BusType != pnp_bus_none;
}

static int
open_PnP()
{
    static const char	pnpDev[] = "/dev/pnp";

    if (pnpFd != -1)
	return 0;

    if ((pnpFd = open(pnpDev, O_RDONLY)) == -1)
    {
	fprintf(stderr, "Cannot open %s: %s\n", pnpDev, strerror(errno));
	return errno;
    }

    return 0;
}


static const char *
PnP_idStr(u_long vendor)
{
    static char		idBuf[] = "pnp1234";
    static const char	xdigit[] = "0123456789abcdef";
    const u_char	*id;
#   define ID_CVT	('a' - 1)	/* Must be lower case for getbsword() */


    id = (u_char *)&vendor;
    idBuf[0] = ID_CVT | ((u_int)(id[0] & 0x7cU) >> 2);
    idBuf[1] = ID_CVT | ((u_int)(id[0] & 0x03U) << 3) |
			((u_int)(id[1] & 0xe0U) >> 5);
    idBuf[2] = ID_CVT |  (id[1] & 0x1f);

    idBuf[3] = xdigit[(id[2] >> 4) & 0x0f];
    idBuf[4] = xdigit[id[2] & 0x0f];

    idBuf[5] = xdigit[(id[3] >> 4) & 0x0f];
    idBuf[6] = xdigit[id[3] & 0x0f];

    return idBuf;
}

