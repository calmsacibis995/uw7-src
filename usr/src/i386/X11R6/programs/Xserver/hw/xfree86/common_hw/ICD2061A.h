/* $XFree86: $ */
/* Based on the number 9 Inc code */
/* Copyright (c) 1992, Number Nine Computer Corp.  All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Number Nine Computer Corp not be used 
 * in advertising or publicity pertaining to distribution of the software 
 * without specific, written prior permission.  Number Nine Computer Corp 
 * makes no representations about the suitability of this software for any 
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * NUMBER NINE COMPUTER CORP DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, 
 * IN NO EVENT SHALL NUMBER NINE COMPUTER CORP BE LIABLE FOR ANY SPECIAL, 
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING 
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: ICD2061A.h /main/2 1995/11/12 19:30:05 kaleb $ */

extern long ICD2061ACalcClock();
extern long ICD2061AGCD();
extern void ICD2061ASetClock();
extern long ICD2061AGetClock();

#define CRT_INDEX               0x03D4
#define CRT_DATA                0x03D5
#define MODE_CTRL_INDEX         0x42
#define SSW_WRITE_INDEX         0x5C
#define SSW_READ_ENBL_INDEX     0x55
#define LOCK_INDEX              0x39
#define UNLOCK_PATTERN          0xA0

#define CRYSTAL_FREQUENCY       (14318180L * 2)
#define MIN_VCO_FREQUENCY       50000000L
#define MAX_POST_SCALE          285000000L
