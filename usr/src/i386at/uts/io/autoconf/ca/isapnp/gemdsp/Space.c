/*
 *	@(#)Space.c	7.1	10/22/97	12:29:13
 * File Space.c for the Plug-and-Play driver
 *
 * @(#) Space.c 65.7 97/07/12 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>

#include "sys/pnp.h"

/*
 * If the value of PnP_auto is FALSE, only devices configured by
 * the BIOS or with specific assignments on the boot line will
 * be configured.  The value of PnP_auto may also be altered at
 * boot time by providing a boot string option.
 */

int	PnP_auto = 1;

/*
 * If the value of PnP_warn is TRUE and the device is not
 * configured a kernel NOTICE message is displayed. The value of
 * PnP_warn may also be altered at boot time by providing a boot
 * string option.
 */

int	PnP_warn = 1;

