#ifndef _FS_PROCFS_PROCFS_F_H	/* wrapper symbol for kernel use */
#define _FS_PROCFS_PROCFS_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/procfs/procfs_f.h	1.2"

/*
 * Family-specific fields of /proc status files.
 * i386 version
 */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Family-specific ctl commands.
 */
#define	PCSDBREG		1000	/* set debug registers */

/*
 * Family-specific part of status file.
 */
typedef struct pfamily {
	long	pf_flags;	/* Flags */
	dbregset_t pf_dbreg;	/* General registers */
	ulong_t pf__pad[16];	/* Room to grow without breaking common part */
} pfamily_t;

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_PROCFS_PROCFS_F_H */
