#ifndef _IO_STRSUBR_P_H	/* wrapper symbol for kernel use */
#define _IO_STRSUBR_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/strsubr_p.h	1.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif


#ifdef _KERNEL

/*
 * MACRO 
 * STRIOCTL_P(struct vnode *vp, int cmd, int arg, int flag, int copyflag,
 *	cred_t *crp, int *rvalp, void **iocstatep, mblk_t **mp, int *error)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Do any platform-specific ioctl processing. On an AT platform
 *	all the platform-specfic ioctl conversion and remapping are
 *	isolated in this interface. However, on the Sequent platforms
 *	this is a NULL macro.
 */

struct strioctlstate;

extern void strioctl_p(struct vnode *, int *, int *, int, int, cred_t *, int *, struct strioctlstate **, mblk_t **, int *);

#define STRIOCTL_P(vp, cmdp, argp, flag, copyflag, crp, rvalp, iocstatep, mp, error) \
		strioctl_p((vp), (cmdp), (argp), (flag), (copyflag), (crp), \
				(rvalp), (iocstatep), (mp), (error))
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_STRSUBR_P_H */
