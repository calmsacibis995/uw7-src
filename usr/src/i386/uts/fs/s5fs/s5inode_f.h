#ifndef _FS_S5FS_S5INODE__F_H      /* wrapper symbol for kernel use */
#define _FS_S5FS_S5INODE__F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/s5fs/s5inode_f.h	1.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define l3tolone(lp, cp) {		\
	*lp++ = *cp++;			\
	*lp++ = *cp++;			\
	*lp++ = *cp++;			\
	*lp++ = 0;			\
}

#define ltol3one(cp, lp) {		\
	*cp++ = *lp++;			\
	*cp++ = *lp++;			\
	*cp++ = *lp++;			\
	lp++;				\
}

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_S5FS_S5INODE__F_H */
