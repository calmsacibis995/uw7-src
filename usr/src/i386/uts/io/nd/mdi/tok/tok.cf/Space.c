#ident "@(#)Space.c	10.1"
#ident "$Header$"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 * 	Driver for the IBM Token Ring Adapter
 */

#define TOK_0_HWADDR		0x00,0x00,0x00,0x00,0x00,0x00
#define TOK_1_HWADDR		0x00,0x00,0x00,0x00,0x00,0x00
#define TOK_2_HWADDR		0x00,0x00,0x00,0x00,0x00,0x00
#define TOK_3_HWADDR		0x00,0x00,0x00,0x00,0x00,0x00

unsigned char tokhwaddr[][6] = {
	TOK_0_HWADDR,
	TOK_1_HWADDR,
	TOK_2_HWADDR,
	TOK_3_HWADDR,
};
