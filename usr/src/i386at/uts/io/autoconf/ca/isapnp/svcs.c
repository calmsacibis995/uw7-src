/*
 *	@(#)svcs.c	7.1	10/22/97	12:29:11
 * File svcs.c for the Plug-and-Play driver
 *
 * @(#) svcs.c 65.4 97/07/11 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/cmn_err.h"
#include "sys/errno.h"
#include "sys/param.h"
#include "stdarg.h"

#ifdef UNIXWARE
# include "sys/cred.h"
# include "sys/confmgr.h"
# include "sys/cm_i386at.h"
# include "sys/conf.h"
# include "sys/ddi.h"
#else
# include "sys/arch.h"
# include "sys/ci/cilock.h"
#endif

#include "sys/pnp.h"
#include "space.h"
#include "pnp_private.h"
#include "pnp_bios.h"

STATIC int PnP_GetNode(u_char *Node, u_char *Buffer);
STATIC int PnP_SetNode(u_char *Node, u_char *Buffer, short Ctrl);
STATIC u_long PnP_xVal(u_char c);
STATIC u_long PnP_aVal(u_char c);

int
PnP_Bios(int Func, ...)
{
    va_list	args;
    int		err;

    PnPinit();
    if (!PnP_Alive)
	return ENOSYS;

    switch (Func)
    {
	/*
	 * System Configuration Interface
	 */

	case 0x00:	/* Get Number of System Device Nodes */
	    {
		u_char	*NumNodes;
		u_short	*NodeSize;

		va_start(args, Func);
		NumNodes = va_arg(args, u_char *);
		NodeSize = va_arg(args, u_short *);
		va_end(args);

		if (!NumNodes || !NodeSize)
		    return EINVAL;

		*NumNodes = (u_char)PnP_NumNodes;
		*NodeSize = (u_short)PnP_NodeSize;
	    }
	    return 0;

	case 0x01:	/* Get System Device Node */
	    {
		u_char		*Node;
		u_char		*Buffer;

		va_start(args, Func);
		Node = va_arg(args, u_char *);
		Buffer = va_arg(args, u_char *);
		va_end(args);

		if ((err = PnP_GetNode(Node, Buffer)) != 0)
		    return err;

		if (*Node < PnP_NumNodes)
		    ++*Node;		/* Indicate the next value */
		else
		    *Node = 0xff;	/* No more */
	    }
	    return 0;

	case 0x02:	/* Set System Device Node */
	    {
		u_char		*Node;
		u_char		*Buffer;
		short		Ctrl;

		va_start(args, Func);
		Node = va_arg(args, u_char *);
		Buffer = va_arg(args, u_char *);
		Ctrl = va_arg(args, short);
		va_end(args);

		if ((err = PnP_SetNode(Node, Buffer, Ctrl)) != 0)
		    return err;

		if (*Node < PnP_NumNodes)
		    ++*Node;		/* Indicate the next value */
		else
		    *Node = 0xff;	/* No more */
	    }
	    return 0;

	/*
	 * Event Notification Interface
	 */

	case 0x03:	/* Get Event */
	case 0x04:	/* Send Message */
	case 0x05:	/* Get Docking Station Information */
	    break;	/* Not yet implemented */

	case 0x06:	/* Reserved */
	case 0x07:
	case 0x08:
	    break;

	/*
	 * Extended Configuration Services
	 */

	case 0x09:	/* Set Statically Allocated Resource Information */
	case 0x0a:	/* Get Statically Allocated Resource Information */
	    break;

	case 0x40:	/* Get Plug & Play ISA Configuration Structure */
	    {
		PnP_ISA_Config_t	*Cfg;

		va_start(args, Func);
		Cfg = va_arg(args, PnP_ISA_Config_t *);
		va_end(args);

		if (!Cfg)
		    return EINVAL;

		bzero(Cfg, sizeof *Cfg);
		Cfg->Rev = 0x01;
		Cfg->NumNodes = PnP_NumNodes;
		Cfg->ReadDataPort = PnP_dataPort;
	    }
	    return 0;

	case 0x41:	/* Get Extended System Configuration Data Info */
	case 0x42:	/* Read Extended System Configuration Data */
	case 0x43:	/* Write Extended System Configuration Data */
	    break;

	/*
	 * Power Management Services
	 */

	case 0x0b:	/* Get APM ID Table */
	    break;
    }

    return ENOSYS;
}

STATIC int
PnP_GetNode(u_char *Node, u_char *Buffer)
{
    PnP_unitID_t	unit;
    const PnP_device_t	*dp;
    int			s;
    u_short		n;
    u_long		resNum;

    if (!Node || !Buffer)
	return EINVAL;

    if (*Node == 0)
	*Node = 1;

    unit.vendor = PNP_ANY_VENDOR;
    unit.serial = PNP_ANY_SERIAL;
    unit.unit = PNP_ANY_UNIT;
    unit.node = *Node;

    PNP_LOCK(s);
    if (!(dp = PnP_FindUnit(&unit)))
    {
	PNP_UNLOCK(s);
	return ENODEV;
    }

    /*
     * Wake up the card so that we can read from it and
     * run thru the Serial Identification bytes
     */

    PnP_Wake(dp->id.CSN);

    for (n = 0; n < 9; ++n)
	PnP_readResByte();

    /*
     * Get the resource
     */

    for (resNum = 0; resNum < dp->ResCnt; ++resNum)
    {
	u_short	len;
	u_char	resByte;

	*Buffer++ = resByte = PnP_readResByte();
	if (!(resByte & 0x80U))
	    len = resByte & 0x07U;
	else
	    len = (*Buffer++ = PnP_readResByte()) |
		 ((*Buffer++ = PnP_readResByte()) << 8);

	for (n = 0; n < len; ++n)
	    *Buffer++ = PnP_readResByte();
    }

    PnP_SetRunState();
    PNP_UNLOCK(s);
    return 0;
}

STATIC int
PnP_SetNode(u_char *Node, u_char *Buffer, short Ctrl)
{
    PnP_unitID_t	unit;
    const PnP_device_t	*dp;
    int			s;
    u_short		n;
    u_long		resNum;

    if (!Node || !Buffer)
	return EINVAL;

    if (*Node == 0)
	*Node = 1;

    unit.vendor = PNP_ANY_VENDOR;
    unit.serial = PNP_ANY_SERIAL;
    unit.unit = PNP_ANY_UNIT;
    unit.node = *Node;

    PNP_LOCK(s);
    if (!(dp = PnP_FindUnit(&unit)))
    {
	PNP_UNLOCK(s);
	return ENODEV;
    }

    /*
     * Wake up the card so that we can read from it and
     * run thru the Serial Identification bytes
     */

    PnP_Wake(dp->id.CSN);

    for (n = 0; n < 9; ++n)
	PnP_readResByte();

    /*
     * Set the resource data only
     */

    for (resNum = 0; resNum < dp->ResCnt; ++resNum)
    {
	u_short	len;
	u_char	resByte;

	resByte = PnP_readResByte();
	++Buffer;
	if (!(resByte & 0x80U))
	    len = resByte & 0x07U;
	else
	{
	    len = PnP_readResByte() | (PnP_readResByte() << 8);
	    Buffer += 2;
	}

	for (n = 0; n < len; ++n)
	    PnP_writeResByte(*Buffer++);
    }

    PnP_SetRunState();
    PNP_UNLOCK(s);
    return 0;
}

int
PnP_bus_data(PnP_busdata_t *bus)
{
    PnPinit();
    if (!PnP_Alive)
	return ENOSYS;

    if (!bus)
	return EINVAL;

    if (PnP_BIOS_entry)
	bus->BusType = pnp_bus_bios;
    else if (ARCH & (AT | EISA))
	bus->BusType = pnp_bus_os;
    else
	bus->BusType = pnp_bus_none;

    bus->NodeSize = PnP_NodeSize;
    bus->NumNodes = PnP_NumNodes;
    return 0;
}

/*
 * Some PnP resources are vendor specific and can only be parsed
 * by the driver.
 */

int
PnP_ReadTag(PnP_TagReq_t *tagReq)
{
    const PnP_device_t	*dp;
    u_long		resNum;
    u_short		n;
    u_short		len;
    u_long		bufIx;
    int			s;

    PnPinit();
    if (!PnP_Alive)
	return ENOSYS;

    if (!tagReq)
	return EINVAL;

    if (!tagReq->tagPtr || (tagReq->tagLen < 1))
	return EINVAL;	/* We need at least one byte */

    PNP_LOCK(s);
    if (!(dp = PnP_FindUnit(&tagReq->unit)))
    {
	PNP_UNLOCK(s);
	return ENODEV;
    }

    if (tagReq->resNum >= dp->ResCnt)
    {
	PNP_UNLOCK(s);
	return ERANGE;
    }

    /*
     * Wake up the card so that we can read from it and
     * run thru the Serial Identification bytes
     */

    PnP_Wake(dp->id.CSN);

    for (n = 0; n < 9; ++n)
	PnP_readResByte();

    /*
     * Run thru the bytes preceeding the requested resource.
     */

    for (resNum = 0; resNum < tagReq->resNum; ++resNum)
    {
	u_char	resByte;

	if (!((resByte = PnP_readResByte()) & 0x80U))
	    len = resByte & 0x07U;
	else
	    len = PnP_readResByte() | (PnP_readResByte() << 8);

	for (n = 0; n < len; ++n)
	    PnP_readResByte();
    }

    /*
     * If we found the tag, copy it out
     */

    if (resNum != tagReq->resNum)
    {
	PnP_SetRunState();
	PNP_UNLOCK(s);
	return EINVAL;
    }

    if (!((tagReq->tagPtr[0] = PnP_readResByte()) & 0x80U))
    {
	len = tagReq->tagPtr[0] & 0x07U;
	bufIx = 1;			/* We have sent one byte */
    }
    else
    {
	if (tagReq->tagLen < 3)
	{
	    /* No valuable data to return */
	    PnP_SetRunState();
	    PNP_UNLOCK(s);
	    return EINVAL;
	}

	tagReq->tagPtr[1] = PnP_readResByte();
	tagReq->tagPtr[2] = PnP_readResByte();
	bufIx = 3;			/* We have sent three bytes */

	len = (tagReq->tagPtr[2] << 8) | tagReq->tagPtr[1];
    }

    /*
     * Read the lesser of all we have or all the caller can take.
     */

    if ((len + bufIx) > tagReq->tagLen)
	len = tagReq->tagLen - bufIx;

    for (n = 0; n < len; ++n)
	tagReq->tagPtr[bufIx++] = PnP_readResByte();

    tagReq->tagLen = bufIx;
    PnP_SetRunState();
    PNP_UNLOCK(s);
    return 0;
}

/*
 * The vendor ID is encoded into a u_long in the same manner as
 * an EISA ID.  This function generates a printable ID string
 * from the encoded vendor ID.  Since the returned string is
 * stored in a private static buffer, the contents may be
 * overwritten by subsequent invocations.
 */

const char *
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


/*
 * "ctl109d"
 *
 * 0x9d108c0e
 * 9999 dddd 1111 0000 tttl llll 0ccc cctt
 *
 * 0x0e 0x8c 0x10 0x9d
 * 0ccc cctt   tttl llll   1111 0000   9999 dddd
 */

u_long
PnP_idVal(const char *idStr)
{
    u_long	value;

    if (!idStr)
	return ~0UL;

    return  (PnP_xVal(idStr[6]) << 24) |
	    (PnP_xVal(idStr[5]) << 28) |
	    (PnP_xVal(idStr[4]) << 16) |
	    (PnP_xVal(idStr[3]) << 20) |

	    (PnP_aVal(idStr[2]) << 8) |
	    (PnP_aVal(idStr[1]) >> 3) |
	    ((PnP_aVal(idStr[1]) & 0x07U) << 13) |
	    (PnP_aVal(idStr[0]) << 2);
}

STATIC u_long
PnP_xVal(u_char c)
{
    if ((c >= '0') && (c <= '9'))
	return (u_long)c - (u_long)'0';

    c |= 0x20U;		/* c = tolower(c) */

    if ((c >= 'a') && (c <= 'f'))
	return (u_long)c - (u_long)('a' - 10);

    return 0;
}

STATIC u_long
PnP_aVal(u_char c)
{
    c |= 0x20U;

    if ((c >= 'a') && (c <= 'z'))
	return (u_long)c - (u_long)ID_CVT;

    return 0;
}
