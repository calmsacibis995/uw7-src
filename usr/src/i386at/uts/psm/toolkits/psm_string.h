#ident	"@(#)kern-i386at:psm/toolkits/psm_string.h	1.1.1.1"
#ident	"$Header$"

#ifndef _PSM_STRING_H			/* wrapper symbol for kernel use */
#define _PSM_STRING_H			/* subject to change without notice */

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


char *strchr(const char *, int);
char *strcpy(char *, const char *);
char *strncat(char *, const char *, int);
char *strncpy(char *, const char *, int);

int strcmp(const char*, const char*);
int strlen(const char*);
int strncmp(const char*, const char*, int);

unsigned long strtoul (const char *, char **, int);

#if defined(__cplusplus)
	}
#endif

#endif /* _PSM_STRING_H */
