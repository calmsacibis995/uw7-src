#ifndef _UTIL_MOD_MODDRV_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MODDRV_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/mod/moddrv.h	1.6.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/conf.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/conf.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Private data for device drivers.
 */

struct	mod_drv_data	{
	/*
	 * Block device info.
	 */
	struct	bdevsw		drv_bdevsw;	/* bdevsw[] table image */
	int	bmajor_0;			/* the first block major number */
	int	bmajors;			/* number of block majors */
	/*
	 * Character device info.
	 */
	struct	cdevsw		drv_cdevsw;	/* cdevsw[] table image */
	int	cmajor_0;			/* the first character major number */
	int	cmajors;			/* the number of character majors */
};


/* Current version: */
struct	mod_drvintr	{
	ushort_t	di_magic;
	ushort_t	di_version;
	char		*di_modname;
	int		*di_devflagp;
	void		(*di_handler)();
	void		*di_hook;	/* uninitialized; used at runtime */
};

#define MOD_INTR_MAGIC	0xEB13	/* value for di_magic */
#define MOD_INTR_VER	1	/* value for di_version */

/* The old way: */

struct	o_mod_drvintr	{
	struct	intr_info	*drv_intrinfo;	/* points to an array of structures */
	int	(*ihndler)();			/* the drivers interrupt handler */
};

#define MOD_INTRVER_MASK	0xff000000
#define INTRVER(infop)	((unsigned int)((infop)->ivect_no & MOD_INTRVER_MASK))
#define INTRNO(infop)	((infop)->ivect_no & ~MOD_INTRVER_MASK)

#define MOD_INTRVER_42		0x01000000	/* 4.2 compat version number */

struct	intr_info0	{
	int	ivect_no;	/* the interrupt vector */
	int	int_pri;	/* the interrupt priority */
	int	itype;		/* type of interrupt */
};

struct	intr_info	{
	int	ivect_no;	/* the interrupt vector */
	int	int_pri;	/* the interrupt priority */
	int	itype;		/* type of interrupt */
	int	int_cpu;	/* bind to this cpu */
	int	int_mp;		/* multithreaded */
	void	*int_idata;	/* idata for the interrupt */
};

struct _Compat_intr_idata {
	void	(*ivect)();  	/* driver entry point */
	int	vec_no; 	/* vector no */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MODDRV_H */
