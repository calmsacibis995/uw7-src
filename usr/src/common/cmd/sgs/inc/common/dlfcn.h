#ifndef _DLFCN_H
#define _DLFCN_H
#ident	"@(#)sgs-inc:common/dlfcn.h	1.2.8.7"

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

/* structure used to get information from dladdr */
typedef struct {
	const char	*dli_fname; /* filename of containing object */
	void		*dli_fbase; /* base addr of containing object */
	const char	*dli_sname; /* name of nearest symbol */
	void 		*dli_saddr; /* address of nearest symbol */
	size_t 		dli_size;   /* size of nearest symbol */
	int 		dli_bind;   /* binding of nearest symbol */
	int 		dli_type;   /* binding of nearest symbol */
} Dl_info;

/* valid values for mode argument to dlopen */

#define RTLD_LAZY	1	/* lazy function call binding */
#define RTLD_NOW	2	/* immediate function call binding */
#define RTLD_GLOBAL	4	/* all symbols available for binding */
#define RTLD_LOCAL	8	/* only immediate dependencies available */

#define RTLD_NEXT	((char *)-1)	/* special argument to dlsym */

#ifdef __cplusplus
extern "C" {
#endif

extern void	*dlopen(const char *, int);
extern void	*dlsym(void *, const char *);
extern int	dlclose(void *);
extern char	*dlerror(void);
extern int	dladdr(void *, Dl_info *);

#ifdef __cplusplus
}
#endif

#endif /*_DLFCN_H*/
