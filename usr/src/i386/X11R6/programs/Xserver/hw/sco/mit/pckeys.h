/*
 *	@(#) pckeys.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
   
/*
   @(#)pckeys.h	1.2  6/30/89 17:46:52
*/


/*
   (c) Copyright 1988 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.  No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	??? Oct ?? ??:??:?? PST 1990	mikep@sco.com
 *	- Created file from Xsight pckeys.h
 *
 *	S001	Mon Feb 04 22:04:21 PST 1991	mikep@sco.com
 *	- Removed duplicate definitions which were already
 *	in scoModKeys.h.  Is this file really necessary?
 *
 *	S002	Thu May 09 19:11:56 PDT 1991	staceyc@sco.com
 *	- Added KEY_ALT_R to keep screen switching hackery going.
 *
 *	S003	Thu May 30 13:59:38 PDT 1991	stacey@sco.com
 *	- Changed KEY_ALT_R to KEY_RALT.
 *	- Added KEY_RCTRL.
 *
 *	S004	Tue Oct 01 14:29:14 PDT 1991	mikep@sco.com
 *	- Remove S002 and S003.   Add KEY_LEAD defines instead.
 */


/*
   pckeys.h - Definitions of AT Scan codes
*/
#define DOWN 1
#define UP 0

#define KEY_LEAD_E0     0xE0
#define KEY_LEAD_E1     0xE1

#define KEY_ESC		1
#define KEY_TR_1	2
#define KEY_TR_2	3
#define KEY_TR_3	4
#define KEY_TR_4	5
#define KEY_TR_5	6
#define KEY_TR_6	7
#define KEY_TR_7	8
#define KEY_TR_8	9
#define KEY_TR_9	10
#define KEY_TR_0	11
#define KEY_UBAR	12
#define KEY_PLUS	13
#define KEY_BS		14
#define KEY_TAB		15
#define KEY_Q		16
#define KEY_W		17
#define KEY_E		18
#define KEY_R		19
#define KEY_T		20
#define KEY_Y		21
#define KEY_U		22
#define KEY_I		23
#define KEY_O		24
#define KEY_P		25
#define KEY_LBRACE	26
#define KEY_RBRACE	27
#define KEY_RETURN	28
#define KEY_CTRL	29
#define KEY_A		30
#define KEY_S		31
#define KEY_D		32
#define KEY_F		33
#define KEY_G		34
#define KEY_H		35
#define KEY_J		36
#define KEY_K		37
#define KEY_L		38
#define KEY_SEMICOLON	39
#define KEY_QUOTE	40
#define KEY_TILDE	41
#define KEY_LSHIFT	42
#define KEY_VBAR	43
#define KEY_Z		44
#define KEY_X		45
#define KEY_C		46
#define KEY_V		47
#define KEY_B		48
#define KEY_N		49
#define KEY_M		50
#define KEY_COMMA	51
#define KEY_PERIOD	52
#define KEY_QMARK	53
#define KEY_RSHIFT	54
#define KEY_KP_ASTERISK	55
#define KEY_ALT		56
#define KEY_SPACE	57
#define KEY_LOCK	58
#define KEY_F1		59
#define KEY_F2		60
#define KEY_F3		61
#define KEY_F4		62
#define KEY_F5		63
#define KEY_F6		64
#define KEY_F7		65
#define KEY_F8		66
#define KEY_F9		67
#define KEY_F10		68
#define KEY_NUM_LOCK	69
#define KEY_SCROLL_LOCK	70
#define KEY_KP_7	71
#define KEY_KP_8	72
#define KEY_KP_9	73
#define KEY_KP_HYPHEN	74
#define KEY_KP_4	75
#define KEY_KP_5	76
#define KEY_KP_6	77
#define KEY_KP_PLUS	78
#define KEY_KP_1	79
#define KEY_KP_2	80
#define KEY_KP_3	81
#define KEY_KP_0	82
#define KEY_DEL		83
#define KEY_KP_PERIOD	83
#define KEY_SYS_REQ	84
#define	KEY_F11		87
#define	KEY_F12		88
