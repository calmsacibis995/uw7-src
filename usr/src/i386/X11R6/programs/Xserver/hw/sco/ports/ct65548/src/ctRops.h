/*
 *	@(#)ctRops.h	11.1	10/22/97	12:34:12
 *	@(#) ctRops.h 58.1 96/10/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ifndef _CT_ROPS_H
#define _CT_ROPS_H

#ident "@(#) $Id: ctRops.h 58.1 96/10/09 "

/*
 * MS Windows Raster Operation Codes (Source and Destination bitmaps):
 * from Programmer's Reference Volume 3, Appendix A
 */
#define CT_BLACKNESSROP		0x00000000L	/* 0 */
#define CT_NOTSRCERASEROP	0x00000011L	/* NOT src AND NOT dst */
#define CT_NOTSRCANDROP		0x00000022L	/* NOT src AND dst */
#define CT_NOTSRCCOPYROP	0x00000033L	/* NOT src */
#define CT_SRCERASEROP		0x00000044L	/* src AND NOT dst */
#define CT_DSTINVERTROP		0x00000055L	/* NOT dst */
#define CT_SRCINVERTROP		0x00000066L	/* src XOR dst */
#define CT_NANDROP		0x00000077L	/* NOT src OR NOT dst */
#define CT_SRCANDROP		0x00000088L	/* src AND dst */
#define CT_EQUIVROP		0x00000099L	/* NOT src XOR dst ??? */
#define CT_NOOPROP		0x000000aaL	/* dst */
#define CT_MERGEPAINTROP	0x000000bbL	/* NOT src OR dst */
#define CT_SRCCOPYROP		0x000000ccL	/* src */
#define CT_ORREVERSEROP		0x000000ddL	/* src OR NOT dst */
#define CT_SRCPAINTROP		0x000000eeL	/* src OR dst */
#define CT_WHITENESSROP		0x000000ffL	/* 1 */

/*
 * MS Windows Raster Operation Codes (Patterns):
 * from Programmer's Reference Volume 3, Appendix A
 */
#define CT_PATINVERTROP		0x0000005aL
#define CT_MERGECOPYROP		0x000000c0L
#define CT_PATCOPYROP		0x000000f0L
#define CT_PATPAINTROP		0x000000fbL

#endif /* _CT_ROPS_H */
