/*
 *	@(#) vbdintf.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Mon Nov 02 15:44:40 PST 1992	buckm@sco.com
 *	- Created.
 *	S001	Sun Mar 28 23:50:07 PST 1993	buckm@sco.com
 *	- Add defs for vbCallRom.
 */

/*
 * vbdintf.h	- video bios daemon interface definitions
 */

#ifndef	VBDINTF_H
#define	VBDINTF_H

/*
 * request numbers
 */
#define	VB_MEMMAP	1
#define	VB_IOENB	2
#define	VB_INT10	3
#define	VB_CALL		4

/*
 * request and status structures
 */

struct	vb_mio		{
	unsigned short	vb_req;
	unsigned short	vb_cnt;		/* currently unused */
	unsigned int	vb_base;
	unsigned int	vb_size;
};

struct	vb_int		{
	unsigned short	vb_req;
	unsigned short	vb_cnt;
	unsigned int	vb_reg[8];
};

struct	vb_int_sts	{
	unsigned int	vb_sts;
	unsigned int	vb_reg[8];
};

struct	vb_call		{
	unsigned short	vb_req;
	unsigned short	vb_seg;
	unsigned short	vb_off;
	unsigned short	vb_cnt;
	unsigned int	vb_reg[8];
};

#endif	/* VBDINTF_H */
