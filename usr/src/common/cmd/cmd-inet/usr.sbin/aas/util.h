/*	"@(#)util.h	1.2"	*/
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#define UTIL_ERROR_CODE		99

#ifdef __STDC__
#define P(s)	s
#else
#define P(s)	()
#endif

char *read_password P((void));
void error P((int code, char *fmt, ...));

#undef P
