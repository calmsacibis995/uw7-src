/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)priv_prot.h	1.2"
#ident	"$Header$"

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
 * This file consists of all rpc information for lock manager
 * interface to status monitor
 */

#define PRIV_PROG 100021
#define PRIV_VERS 2
#define PRIV_CRASH 1
#define PRIV_RECOVERY 2
