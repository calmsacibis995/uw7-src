/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)MGizmo:_wchar.h	1.1"
#endif

#define H_EUCMASK 0x8080
#define H_P11     0x8080          /* Code Set 1 */
#define H_P01     0x0080          /* Code Set 2 */
#define H_P10     0x8000          /* Code Set 3 */
#define EUCMASK 0xf0000000
#define P11     0x30000000          /* Code Set 1 */
#define P01     0x10000000          /* Code Set 2 */
#define P10     0x20000000          /* Code Set 3 */
#ifdef __STDC__
#define _ctype __ctype
#endif
#ifndef _ctype 		/* if still can't find _ctype, then... */
#define _ctype _ctype_
#endif
#define multibyte       (_ctype[520]>1)
#define eucw1   _ctype[514]
#define eucw2   _ctype[515]
#define eucw3   _ctype[516]
#define scrw1   _ctype[517]
#define scrw2   _ctype[518]
#define scrw3   _ctype[519]
