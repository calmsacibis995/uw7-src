#ident	"@(#)OSRcmds:ksh/include/timeout.h	1.1"
#pragma comment(exestr, "@(#) timeout.h 25.3 93/01/20 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992.			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 * Modification History
 *
 *	L000	scol!markhe	11 Nov 92
 *	- changed type of message variables from 'const char' to MSG
 */

/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	AT&T Bell Laboratories
 *
 */

#define TGRACE		60	/* grace period before termination */
				/* The time_warn message contains this number */
extern long		sh_timeout;
extern MSG	e_timeout;					/* L000 */
extern MSG	e_timewarn;					/* L000 */
