#ifndef _FS_UFS_UFS_QUOTA_H	/* wrapper symbol for kernel use */
#define _FS_UFS_UFS_QUOTA_H	/* subject to change without notice */

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

#ident	"@(#)kern-i386:fs/ufs/ufs_quota.h	1.3"
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
 * The UFS header files are kept for command source
 * compatibility. The kernel source references SFS
 * header files.
 */
#ifdef _KERNEL_HEADERS

#ifndef _FS_SFS_SFS_QUOTA_H
#include <fs/sfs/sfs_quota.h>
#endif

#elif defined(_KERNEL)

#include <sys/fs/sfs_quota.h>

#else

#include <sys/fs/sfs_quota.h>

#endif /* _KERNEL_HEADERS */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_UFS_UFS_QUOTA_H */
