#ident	"@(#)pcintf:bridge/swap.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)swap.c	6.3	LCC);	/* Modified: 09:36:05 2/20/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"
#include "pci_types.h"
#include "common.h"
#include "flip.h"

int	rd_flag = 0;			/* reliable delivery flag */

int	flipBytes = NOFLIP;	/* byte ordering flag (argument to byte
				   flipping macros and functions) */


/*
 * 	swap() -			auto-sense byte swapping routine.
 *					Returns how bytes were swapped.
 */

int
input_swap(ptr, pattern)
register struct input *ptr;
register long	pattern;
{
    register	int
	how;

#ifdef	VERSION_MATCHING
	register struct	connect_text *c_ptr;
#endif	/* VERSION_MATCHING */
	register struct	emhead       *e_ptr;

    short	tmpshort;

    long	tmplong;


    if ((how = FLIPHOW(pattern)) == NOFLIP)
	return how;

    if (how & SFLIP) {
#ifdef RS232PCI
        dosflipm(ptr->rs232.f_cnt, tmpshort);
        dosflipm(ptr->rs232.chks, tmpshort);
#endif /* RS232PCI */

        dosflipm(ptr->hdr.pid, tmpshort);
        dosflipm(ptr->hdr.fdsc, tmpshort);
        dosflipm(ptr->hdr.mode, tmpshort);
        dosflipm(ptr->hdr.date, tmpshort);
        dosflipm(ptr->hdr.time, tmpshort);
        dosflipm(ptr->hdr.b_cnt, tmpshort);
        dosflipm(ptr->hdr.t_cnt, tmpshort);
        dosflipm(ptr->hdr.inode, tmpshort);
        dosflipm(ptr->hdr.versNum, tmpshort);
    }
    lflipm(ptr->hdr.f_size, tmplong, how);
    lflipm(ptr->hdr.offset, tmplong, how);
    lflipm(ptr->hdr.pattern, tmplong, how);

	switch (ptr->hdr.req) {		/* process special cases */

		case CONNECT :
#ifdef	VERSION_MATCHING
				c_ptr = (struct connect_text *) ptr->text;
				if (how & SFLIP) {
				    dosflipm(c_ptr->vers_major, tmpshort);
				    dosflipm(c_ptr->vers_minor, tmpshort);
				    dosflipm(c_ptr->vers_submin, tmpshort);
				}
#endif	/* VERSION_MATCHING */
				break;

		default :
			/* This will check for emulation stream packets.
			 * unfortunately this is necessary.  We might
			 * want to change the way this is done later. - rp
			 */
				if (rd_flag) {
					if (ptr->pre.select ==  SHELL) {
				    	    e_ptr = (struct emhead *) ptr->text;
					    if (how & SFLIP) {
						dosflipm(e_ptr->dnum, tmpshort);
						dosflipm(e_ptr->anum, tmpshort);
						dosflipm(e_ptr->strsiz,tmpshort);
					    }
					}
				}
					
				break;
	}
    
	return how;
}


/*
 *	output_swap() -
 */

void
output_swap(ptr, how)
register	struct	output	*ptr;
register	int	how;
{
    short	tmpshort;

    long	tmplong;
    struct	emhead *e_ptr;

#ifndef RS232PCI
    ptr->hdr.pattern = SENSEORDER;
#endif /* !RS232PCI */

    if (how == NOFLIP)
	return;

    if (how & SFLIP) {
#ifdef RS232PCI
	dosflipm(ptr->rs232.f_cnt, tmpshort);
	dosflipm(ptr->rs232.chks, tmpshort);
#endif /* RS232PCI */

	dosflipm(ptr->hdr.pid, tmpshort);
	dosflipm(ptr->hdr.fdsc, tmpshort);
	dosflipm(ptr->hdr.mode, tmpshort);
	dosflipm(ptr->hdr.date, tmpshort);
	dosflipm(ptr->hdr.time, tmpshort);
	dosflipm(ptr->hdr.b_cnt, tmpshort);
	dosflipm(ptr->hdr.t_cnt, tmpshort);
	dosflipm(ptr->hdr.inode, tmpshort);
	dosflipm(ptr->hdr.versNum, tmpshort);
    }
    lflipm(ptr->hdr.f_size, tmplong, how);
    lflipm(ptr->hdr.offset, tmplong, how);
    lflipm(ptr->hdr.pattern, tmplong, how);

/*
 * Swap emulation headers if and only if rd_flag is TRUE
 */

	/* This will check for emulation stream packets.
	 * unfortunately this is necessary.  We might
	 * want to change the way this is done later. - rp
	 */
	if (rd_flag) {
		if (ptr->pre.select ==  SHELL) {
	    	    e_ptr = (struct emhead *) ptr->text;
		    if (how & SFLIP) {
			dosflipm(e_ptr->dnum, tmpshort);
			dosflipm(e_ptr->anum, tmpshort);
			dosflipm(e_ptr->strsiz, tmpshort);
		    }
		}
	}
				
}

/*
 *	byteorder_init:  determines machine byte ordering
 *		Sets the global variable flipBytes
 */

void
byteorder_init()
{
	int	ivar = 1;

	if (*(char *)&ivar)
		flipBytes = NOFLIP;
	else 
		flipBytes = LSFLIP;
}
