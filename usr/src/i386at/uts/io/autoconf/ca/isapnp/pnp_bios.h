/*
 *	@(#)pnp_bios.h	7.1	10/22/97	12:29:03
 *	Copyright (C) The Santa Cruz Operation, 1984-1997.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

#ifndef _pnp_bios_h_
#define _pnp_bios_h_

#pragma comment(exestr, "@(#) pnp_bios.h 65.2 97/07/11 ")

extern caddr_t	PnP_BIOS_entry;

int PnP_bios_call(u_long Func, ...);

#endif	/* _pnp_bios_h_ */

