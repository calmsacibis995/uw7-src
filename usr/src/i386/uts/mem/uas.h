#ifndef _MEM_UAS_H	/* wrapper symbol for kernel use */
#define _MEM_UAS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/uas.h	1.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * Interfaces for accessing user address space from the kernel.
 * These routines handle just the data access, without worrying about
 * fault handling, address validation, etc.  They should only be called
 * from routines, such as copyin()/copyout(), which are handling those
 * issues.
 *
 * For implementations where the kernel can directly access user space,
 * these will just be macros to use bcopy et al. or direct pointer dereference.
 *
 * For implementations which require special mechanisms to access user
 * space, these will typically be implemented as assembly language routines.
 */


/*
 * void uas_copyin(const void *src, void *dst, size_t cnt)
 *
 * src is a user address.
 */

#define uas_copyin(src, dst, cnt)	bcopy(src, dst, cnt)

/*
 * void uas_copyout(const void *src, void *dst, size_t cnt)
 *
 * dst is a user address.
 */

#define uas_copyout(src, dst, cnt)	bcopy(src, dst, cnt)

/*
 * char uas_char_in(const char *src)
 *
 * src is a user address.
 */

#define uas_char_in(src)	(*(const char *)(src))

/*
 * void uas_char_out(char *dst, char val)
 *
 * dst is a user address.
 */

#define uas_char_out(dst, val)	(*(char *)(dst) = (val))

/*
 * char *uas_charp_in(char * const *src)
 *
 * src is a user address.
 */

#define uas_charp_in(src)	(*(char * const *)(src))

/*
 * void uas_charp_out(char **dst, char *val)
 *
 * dst is a user address.
 */

#define uas_charp_out(dst, val)	(*(char **)(dst) = (val))

/*
 * ushort_t uas_ushort_in(const ushort_t *src)
 *
 * src is a user address.
 */

#define uas_ushort_in(src)	(*(const ushort_t *)(src))

/*
 * void uas_ushort_out(ushort_t *dst, ushort_t val)
 *
 * dst is a user address.
 */

#define uas_ushort_out(dst, val)	(*(ushort_t *)(dst) = (val))

/*
 * void uas_ushort_add(ushort_t *dst, ushort_t val)
 *
 * dst is a user address.
 */

#define uas_ushort_add(dst, val) (*(ushort_t *)(dst) += (val))

/*
 * int uas_strlen(const char *s)
 *
 * s is a user address.
 */

#define uas_strlen(s)		strlen(s)

/*
 * int uas_strcpy_len(char *dst, const char *src)
 *
 * src and dst are user addresses.
 */

#define uas_strcpy_len(dst, src)		strcpy_len(dst, src)

/*
 * int uas_strcpy_max(char *dst, const char *src, size_t maxlen)
 *
 * src is a user address.
 */

#define uas_strcpy_max(dst, src, maxlen)	strcpy_max(dst, src, maxlen)

/*
 * void uas_bzero(void *dst, size_t cnt)
 *
 * dst is a user address.
 */

#define uas_bzero(dst, cnt)	bzero(dst, cnt)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_UAS_H */
