#ifndef _PROC_IPC_IPC_F_H	/* wrapper symbol for kernel use */
#define _PROC_IPC_IPC_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/ipc/ipc_f.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Family-specific IPC definitions, i386 version.
 */

/*
 * Shared memory segment low boundary address multiple, must be a power of two.
 * Defined here to avoid rendering shm.h machine-dependent or exposing user
 * code to the contents of param.h in violation of standards.
 */
#define _SHMLBA		((ulong_t)(1) << 12)	/* ptob(1) */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_IPC_IPC_F_H */
