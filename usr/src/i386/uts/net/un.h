#ifndef	_NET_UN_H	/* wrapper symbol for kernel use */
#define	_NET_UN_H	/* subject to change without notice */

#ident	"@(#)un.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifdef _KERNEL_HEADERS
#include <util/types.h>
#include <net/bitypes.h>
#include <net/convsa.h>
#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/types.h>
#include <sys/bitypes.h>
#include <sys/convsa.h>
#else
#include <sys/types.h>
#include <sys/bitypes.h>
#include <sys/convsa.h>
#endif /* _KERNEL_HEADERS */

/*
 * Definitions for UNIX IPC domain.
 */
struct sockaddr_un {
#ifdef __NEW_SOCKADDR__
	sa_len_t	sun_len;	/* length of struct including null */
#endif
	sa_family_t	sun_family;	/* AF_UNIX */
	char	sun_path[108];		/* path name (gag) */
};

#ifdef _KERNEL
int	unp_discard();
#endif

/*
 * Actual length of an initialized sockaddr_un, but *NOT* including the
 * zero byte terminating.  That is SUN_LEN(sun)+1 == sun->sun_len.
 */
#define SUN_LEN(su) \
	(sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_UN_H */
