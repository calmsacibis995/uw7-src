/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)prot_time.h	1.2"
#ident  "$Header$"

/*
 *              PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *              Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *      (c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *      (c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *                All rights reserved.
 */

/*
 * This file consists of all timeout definition used by lockd.
 */

#define MAX_LM_TIMEOUT_COUNT	1
#define OLDMSG			30	/* counter to throw away old msg */

/*
 * LM_TIMEOUT is now the interval between successive xtimer() calls
 * (xtimer() is the signal handler for SIGALARM) when there are blocked
 * locks present (bloking_req != NULL). Hence it is much smaller now.
 * Note that xtimer() does not retry for the messages queued each time
 * it is called, but only the 6th time. Hence if LM_TIMEOUT is 1 second,
 * the lockd retries for the blocked locks (makes fcntl() calls) every
 * second, and for queued messages (makes nlm_calls()) every 6 seconds.
 */

#define LM_TIMEOUT_DEFAULT	1
#define LM_GRACE_DEFAULT 	3
int 	LM_TIMEOUT;
int 	LM_GRACE;
int	grace_period;
