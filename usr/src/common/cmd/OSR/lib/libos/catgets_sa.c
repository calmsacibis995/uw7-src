#ident	"@(#)OSRcmds:lib/libos/catgets_sa.c	1.1"
#pragma comment(exestr, "@(#) catgets_sa.c 25.1 92/08/12 ")
/*
 *	Copyright (C) 1992 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 * The code marked with symbols from the list below, is owned
 * by The Santa Cruz Operation Inc., and represents SCO value
 * added portions of source code requiring special arrangements
 * with SCO for inclusion in any product.
 *  Symbol:		 Market Module:
 * SCO_BASE 		Platform Binding Code
 * SCO_ENH 		Enhanced Device Driver
 * SCO_ADM 		System Administration & Miscellaneous Tools
 * SCO_C2TCB 		SCO Trusted Computing Base-TCB C2 Level
 * SCO_DEVSYS 		SCO Development System Extension
 * SCO_INTL 		SCO Internationalization Extension
 * SCO_BTCB 		SCO Trusted Computing Base TCB B Level Extension
 * SCO_REALTIME 	SCO Realtime Extension
 * SCO_HIGHPERF 	SCO High Performance Tape and Disk Device Drivers
 * SCO_VID 		SCO Video and Graphics Device Drivers (2.3.x)
 * SCO_TOOLS 		SCO System Administration Tools
 * SCO_FS 		Alternate File Systems
 * SCO_GAMES 		SCO Games
 */

/* BEGIN SCO_BASE */

/* MODIFICATION HISTORY
 *
 *	11 Aug 92	scol!harveyt	created
 *	- A version of catgets() which does not change the value of errno.
 *	  This is generally used by MSGSTR() macros defined by mkcatdefs,
 *	  and is used in psyserrorl() calls, which usually take errno as
 *	  a first argument.  If catgets() were used in the MSGSTR() macro
 *	  then errno may be changed, to EBADF for example.
 */

#include <nl_types.h>
#include <errno.h>

char *
catgets_safe (nl_catd catd, int set_id, int msg_id, char *defmsg)
{
	int	errnosave;
	char	*ret;

	errnosave = errno;
	ret = catgets (catd, set_id, msg_id, defmsg);
	errno = errnosave;

	return ret;
}
/* END SCO_BASE */
