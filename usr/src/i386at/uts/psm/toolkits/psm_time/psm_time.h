#ifndef _PSM_PSM_TIME_H	/* wrapper symbol for kernel use */
#define _PSM_PSM_TIME_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:psm/toolkits/psm_time/psm_time.h	1.1.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_PSM)
#if _PSM != 2
#error "Unsupported PSM version"
#endif
#else
#error "Header file not valid for Core OS"
#endif

#ifndef __PSM_SYMREF

struct __PSM_dummy_st {
	int *__dummy1__;
	const struct __PSM_dummy_st *__dummy2__;
};
#define __PSM_SYMREF(symbol) \
	extern int symbol; \
	static const struct __PSM_dummy_st __dummy_PSM = \
		{ &symbol, &__dummy_PSM }

#if !defined(_PSM)
__PSM_SYMREF(No_PSM_version_defined);
#pragma weak No_PSM_version_defined
#else
#define __PSM_ver(x) _PSM_##x
#define _PSM_ver(x) __PSM_ver(x)
__PSM_SYMREF(_PSM_ver(_PSM));
#endif

#endif  /* __PSM_SYMREF */

/*
 * Function prototypes for toolkit procedures & variables.
 */
void		psm_time_spin(unsigned int);
void		psm_time_spin_init();
void		psm_time_spin_adjust(unsigned int, unsigned int);

#if defined(__cplusplus)
	}
#endif

#endif /* _PSM_PSM_TIME_H */
