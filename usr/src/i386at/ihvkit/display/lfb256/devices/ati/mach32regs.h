#ident	"@(#)ihvkit:display/lfb256/devices/ati/mach32regs.h	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#ifndef _MACH32REGS_H_
#define _MACH32REGS_H_

/*
 * ATI Mach32 registers
 *
 * All the information needed to initialize the Mach32 to a given
 * resolution and pixel size.
 *
 * These registers are used to initialize the Mach32.  The names are
 * from the Mach32 spec.
 *
 */

typedef struct {
    unsigned int width, height, freq;
    unsigned long
	clock_sel,
	disp_cntl,
	h_disp,
	h_sync_strt,
	h_sync_wid,
	h_total,
	v_disp,
	v_sync_strt,
	v_sync_wid,
	v_total,
	interlace;
} Mach32DispRegs;

typedef struct {

#if (BPP == 16)
    int weighting;
#elif (BPP == 24)
    char weighting[10];
#endif

    int def_ext_ge_config;
} Mach32WeightRegs;

#endif /* _MACH32REGS_H_ */
