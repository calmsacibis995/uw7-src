#ident	"@(#)OSRcmds:lib/libos/libos_intl.h	1.1"
#pragma comment(exestr, "@(#) libos_intl.h 26.2 95/07/26 ")
/*
 *	Copyright (C) 1995 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */

/* MODIFICATION HISTORY
 *
 *	26 May 1995	scol!ashleyb
 *	- Created -- defines for libos internationalisation routines.
 *	24 Jul 1995	scol!ianw	[not marked]
 *	- Changed to use the renamed more generic __lib_msg() routine
 *	  (see libos_intl.c).
 */

#ifndef _LIBOS_INTL_H
#define _LIBOS_INTL_H
#ifdef INTL

/* #include <locale.h> */
#include "libos_msg.h"

extern char *__lib_msg(const char *catname,int message_id,char *default_string);
#define LMSGSTR(num,str) __lib_msg(MF_LIBOS, (num), (str))


#else /* INTL */

#define LMSGSTR(num,str) (str)


#endif /* INTL */

#endif	/* _LIBOS_INTL_H */
