#ident "@(#)Space.c	10.1"
/*
 *	Copyright The Windward Group, 1992.
 *      All Rights Reserved.
 *
 *	Copyright 3Com Corporation, 1992, 1993, 1994, 1995.
 *      All Rights Reserved.
 *
 *	3Com Corporation Proprietary and Confidential
 *
 *	%VER UNIX Driver for 3C90x version 2.0.0d
 *	Space.c file for SCO MDI
 */

/* Node addres can be overridden by specifying new node address below */
short e3H_Node_Addr_Override[4] = {0, 0, 0, 0};  /* change to 1 to overrride */
unsigned char e3H_New_Node_Addr[4] [6] = {
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

/*
	Media type may be overridden.  Default is to let the NIC determine the
	speed and duplex mode.  
*/
#define MEDIA_OVERRIDE_DEFAULT	0
#define MEDIA_OVERRIDE_NONE	0
#define MEDIA_OVERRIDE_AUTO	1
#define MEDIA_OVERRIDE_100_HALF	2
#define MEDIA_OVERRIDE_100_FULL	3
#define MEDIA_OVERRIDE_10_HALF	4
#define MEDIA_OVERRIDE_10_FULL	5

short e3H_Media_Override[4] = {
	MEDIA_OVERRIDE_DEFAULT, 
	MEDIA_OVERRIDE_DEFAULT,
	MEDIA_OVERRIDE_DEFAULT,
	MEDIA_OVERRIDE_DEFAULT
};
