/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ifndef _FS_XX_XXFBLK_H	/* wrapper symbol for kernel use */
#define _FS_XX_XXFBLK_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxfblk.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef	_KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <fs/xxfs/xxfilsys.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/fs/xxfilsys.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#pragma pack(2)

typedef	struct	fblk {
	short	df_nfree;
	daddr_t	df_free[XXNICFREE];
} fblk_t;

#pragma pack()
#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XX_XXFBLK_H */
