#ifndef _externs_h_
#define _externs_h_
#ident	"@(#)rtld:i386/externs.h	1.29"

#include "machdep.h"
#include "rtld.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <priv.h>

#include "stdlock.h"

/* declarations of external symbols used in rtld */

/* data symbols */

extern rt_map	*_rt_map_head;	/* head of rt_map chain */
extern rt_map	*_rt_map_tail;	/* tail of rt_map chain */
extern rt_map	*_rtld_map;	/* run-time linker rt_map */

extern struct r_debug	_r_debug;/* debugging information */
extern char	*_rt_error;	/* string describing last error */
extern char	*_rt_proc_name;	/* file name of executing process */
extern mlist	*_rt_fini_list; /* fini list for startup objects */
extern CONST char _rt_name[];	/* name of the dynamic linker */

extern size_t	_rt_syspagsz;	/* system page size */
extern ulong_t 	_rt_flags;	/* machine specific file flags */
extern ulong_t	_rt_control;	/* rtld control flags */
extern int	_rt_dlsym_order_compat;  /* set compatibility mode
					   * for dlsym searches
					   */

extern int	_nd;		/* store value of _end */

extern struct rel_copy *_rt_copy_entries;	/* head of the copy relocations list */

#ifdef DEBUG
extern int	_rt_debugflag;
#endif

/* functions */

extern void	_rt_lasterr ARGS((CONST char *fmt, ...));
extern void	_rt_setaddr ARGS((void));
extern int	_rtld ARGS((Elf32_Dyn *interface, Elf32_Dyn *rt_ret));
extern void	_r_debug_state ARGS((void));
extern void	_rtfprintf ARGS((int fd, CONST char *fmt, ...));
extern Elf32_Sym *_rt_lookup ARGS((CONST char *symname, rt_map *first, 
			rt_map *lm_list, rt_map *ref_lm, 
			rt_map **rlm, int flag));
extern rt_map	*_rt_map_so ARGS((CONST object_id *, ulong_t *, char **,				int));
extern int	_rt_unmap_so ARGS((rt_map *));
extern void	_rt_do_exit ARGS((void));
extern int	_rt_relocate ARGS((rt_map *lm, int mode));
extern void	_rt_process_delayed_relocations	ARGS((rt_map *));
extern int	_rt_process_lazy_bindings	ARGS((rt_map *));
extern void	_rtbinder ARGS((void));
extern int	_rt_setpath ARGS((CONST char *envdirs,
			CONST char *rundirs, int use_ld_lib_path));
extern int	_rt_set_protect ARGS((rt_map *lm, int permission));
extern int	_rt_flag_error ARGS((ulong_t eflags, CONST char *pathname));
extern rt_map	*_rt_new_lm ARGS((CONST object_id *, Elf32_Dyn *,
			ulong_t, ulong_t, Elf32_Phdr *, 
			ulong_t, ulong_t, Elf32_Phdr *, char **));
extern void	_rt_cleanup ARGS((rt_map *lm));
extern void	_rt_call_init ARGS((mlist  **flist));
extern int	_rt_build_init_list ARGS((rt_map *lm, mlist  **flist));
extern void	_rt_process_fini ARGS((mlist *list, int about_to_exit));
extern void	_rt_dl_do_exit ARGS((void));
extern int	_rt_so_find ARGS((CONST char *, object_id *));

#ifdef GEMINI_ON_OSR5
#define _rtstat(p,b)	_rtxstat(_SCO_STAT_VER,p,b)
#define _rtfstat(p,b)	_rtfxstat(_SCO_STAT_VER,p,b)
#else
#define _rtstat(p,b)	_rtxstat(_STAT_VER,p,b)
#define _rtfstat(p,b)	_rtfxstat(_STAT_VER,p,b)
#endif

/* system calls */
extern int	_rtwrite ARGS((int, CONST char *, uint_t));
extern int	_rtopen ARGS((CONST char *,int, ...));
extern void	_rtexit ARGS((int));
#ifdef GEMINI_ON_OSR5
extern int	_rtfxstat ARGS((int, int, struct osr5_stat32 *));
extern int	_rtxstat ARGS((int, CONST char *, struct osr5_stat32 *));
#else
extern int	_rtfxstat ARGS((int, int, struct stat *));
extern int	_rtxstat ARGS((int, CONST char *, struct stat *));
#endif
extern int	_rtclose ARGS((int));
extern int	_rtkill ARGS((int, int));
extern int	_rtgetpid ARGS((void));
extern ushort_t _rtcompeuid ARGS((void));
extern ushort_t _rtcompegid ARGS((void));
extern caddr_t	_rtmmap ARGS((caddr_t, int, int, int, int, off_t));
extern int	_rtmunmap ARGS((caddr_t, int));
extern int	_rtmprotect ARGS((caddr_t, int, int));
extern int	_rtprocpriv ARGS((int, priv_t *, int));
extern int	_rtsecsys ARGS((int, caddr_t));
extern long	_rtsysconfig ARGS((int));
extern int	_rtsysi86 ARGS((int, int, int));

/* utility functions */
extern VOID	*_rtalloc ARGS((enum rttype));
extern void	_rtfree ARGS((VOID *, enum rttype));
extern VOID	*_rtmalloc ARGS((uint_t));
extern CONST char *_rt_readenv ARGS((CONST char **,int *));
extern int	_rtstrlen ARGS((register CONST char *));
extern char	*_rtstrcpy ARGS((register char *, register CONST char *));
extern int	_rtstrcmp ARGS((register CONST char *, register CONST char *));
extern int	_rtstrncmp ARGS((register CONST char *, register CONST char *,size_t));
extern void	_rtclear ARGS((VOID *, size_t));
extern void	_rt_memcpy ARGS((VOID *, CONST VOID *, uint_t));
extern int	_rt_ismember ARGS((rt_map *, rt_map *));
extern int	_rt_hasgroup ARGS((ulong_t, rt_map *));
extern int	_rt_setgroup ARGS((ulong_t, rt_map *, uint_t));
extern int	_rt_addset ARGS((ulong_t, rt_map *, uint_t));
extern void	_rt_delset ARGS((ulong_t, rt_map *));
extern int	_rt_isglobal ARGS((rt_map *));
extern int	_rt_add_ref ARGS((rt_map *, rt_map *));
extern int	_rt_add_needed ARGS((rt_map *, rt_map *));
extern void	_rtmkspace ARGS((char *, size_t)); 
extern void	_rt_close_devzero ARGS((void));
extern char	*_rt_map_zero ARGS((ulong_t, size_t, int, int));
extern void	(*_rt_event) ARGS((ulong_t));
extern void	_rt_fatal_error ARGS((void));
extern char	*_dlerror ARGS((void));
extern VOID	*_rt_real_dlsym ARGS((ulong_t caller, VOID *handle, CONST char *name));
extern void	_rt_special_order_inits();

#ifdef _REENTRANT
extern void	_rt_rlock ARGS((rtld_rlock *));
extern void	_rt_runlock ARGS((rtld_rlock *));
extern	StdLock	_rtld_lock;
#endif

#endif
