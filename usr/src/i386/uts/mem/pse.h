#ifndef _MEM_PSE_H	/* wrapper symbol for kernel use */
#define _MEM_PSE_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/pse.h	1.1.1.2"
#ident	"$Header$"

#ifdef	_KERNEL

/*
 * is PSE supported on this cpu?
 */

/*
 * PSE is temporarily turned off until segment chaing development
 * is complete.
 */
#define	PSE_SUPPORTED()		(l.cpu_features[0] & CPUFEAT_PSE)

/*
 * minimum size to use PSE mappings
 */
#define	KPSE_MIN		(1024*1024)

/*
 * maximum virtual space waste
 */
#define	KPSE_WASTE		(4*1024*1024)

#endif	/* _KERNEL */

#endif /* _MEM_PSE_H */
