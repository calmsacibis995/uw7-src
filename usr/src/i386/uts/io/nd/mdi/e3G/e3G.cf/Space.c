/*
 *	Copyright The Windward Group, 1992.
 *      All Rights Reserved.
 *
 *	Copyright 3Com Corporation, 1992, 1993, 1994, 1995.
 *      All Rights Reserved.
 *
 *	3Com Corporation Proprietary and Confidential
 *
 *	%VER UNIX Driver for 3C59x version 1.1f
 *	Space.c file for SCO MDI
 */

/* 
 * The hardware node address can be overridden
 * for each board by placing a new node address here. 
 */
short e3G_override_node_addr [4] = {0, 0, 0, 0};  /* change to 1 to overrride */
unsigned char e3G_new_node_addr [4] [6] = {
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

