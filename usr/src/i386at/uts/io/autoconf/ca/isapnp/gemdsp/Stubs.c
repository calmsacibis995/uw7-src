/*
 *	@(#)Stubs.c	7.1	10/22/97	12:29:14
 * File Stubs.c for the Plug-and-Play driver
 *
 * @(#) Stubs.c 65.2 97/06/29 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>

#include "sys/pnp.h"

int
PnP_Bios(int Func, ...)
{
    return ENOSYS;
}

int
PnP_bus_data(PnP_busdata_t *bus)
{
    return ENOSYS;
}

int
PnP_ReadTag(PnP_TagReq_t *tagReq)
{
    return ENOSYS;
}

const char *
PnP_idStr(u_long vendor)
{
    return "pnp1234";	/* Wrong but noone will panic on it */
}

