/*
 *	@(#) scoModKeys.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright IBM Corporation 1987,1988,1989
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	??? Oct ?? ???????? PST 1990	mikep@sco.com
 *	- Created file from ibmModKeys.h
 *
 *	S001	Tue Jan 29 20:38:03 PST 1991	mikep@sco.com
 *	-  Changed NumLockMask to Mod3
 *
 *	S002	Mon Feb 04 20:13:07 PST 1991	mikep@sco.com
 *	- Changed PC_GLYPHS_PER_KEY to 4 to allow for Num Lock mode
 *	  switch column.
 *	
 *	S003	Sun Feb 17 01:46:53 PST 1991	mikep@sco.com
 *	- Changed PC_GLYPHS_PER_KEY back to 2 for new Num Lock
 *	handling.
 *      S004    Tue Sep 21 13:02:32 PDT 1993    edb@sco.com
 *	- Extend KB101Map for use with scancode API
 */

#ifndef _SCOMODKEYS_H_
#define _SCOMODKEYS_H_

#define PC_MIN_KEY  0x08
#define PC_MAX_KEY ( 144 + PC_MIN_KEY)          /* S004 */
#define PC_GLYPHS_PER_KEY 2			/* S002, S003 */

/*                                        Position
 *                          Base            Code
 *	(This info should be pulled out of the mapkey table)
 */
#define PC_Control_L    (PC_MIN_KEY   +    0x1C)
#define PC_Control_R	(PC_MIN_KEY   +    0x64)
#define PC_Shift_L      (PC_MIN_KEY   +    0x29)
#define PC_Shift_R      (PC_MIN_KEY   +    0x35)
#define PC_Caps_Lock    (PC_MIN_KEY   +    0x39)
#define PC_Alt_L        (PC_MIN_KEY   +    0x37)
#define PC_Alt_R        (PC_MIN_KEY   +    0x65)
#define PC_Delete       (PC_MIN_KEY   +    0x52)
#define PC_Num_Lock	(PC_MIN_KEY   +    0x44)

#define NumLockMask	  Mod3Mask		/* S001 */

#endif /* _SCOMODKEYS_H_ */
