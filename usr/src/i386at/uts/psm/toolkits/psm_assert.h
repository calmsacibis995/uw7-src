#ifndef _PSM_TOOLKITS_PSM_ASSERT_H
#define _PSM_TOOLKITS_PSM_ASSERT_H
#ident	"@(#)kern-i386at:psm/toolkits/psm_assert.h	1.3.2.1"
#ident	"$Header$"

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

#ifdef DEBUG

#define PSM_ASSERT(EX) ((void)((EX) || psm_assfail(#EX, __FILE__, __LINE__)))

#else

#define PSM_ASSERT(x) ((void)0)

#endif

#endif _PSM_TOOLKITS_PSM_ASSERT_H
