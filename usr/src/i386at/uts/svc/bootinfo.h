#ifndef _SVC_BOOTINFO_H	/* wrapper symbol for kernel use */
#define _SVC_BOOTINFO_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/bootinfo.h	1.25.6.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

struct bootinfo {
	ushort_t	bootflags;	/* miscellaneous flags */
	ushort_t	machflags;	/* machine-type flags */

	struct hdparams { 		/* hard disk parameters */
		ushort	hdp_ncyl;	/* # cylinders (0 = no disk) */
		unchar	hdp_nhead;	/* # heads */
		unchar	hdp_nsect;	/* # sectors per track */
		ushort	hdp_precomp;	/* write precomp cyl */
		ushort	hdp_lz;		/* landing zone */
	} hdparams[2];			/* hard disk parameters */
};

/* Flag definitions for bootflags */

#define BF_FLOPPY	0x01		/* booted from floppy */

/* Flag definitions for machflags */

#define MC_BUS		0x10	/* Machine has micro-channel bus */
#define AT_BUS		0x20	/* Machine has ISA bus */
#define EISA_IO_BUS	0x40	/* Machine has EISA bus */
#define BUS_BRIDGED	0x100   /* Machine has a bridge to other bus(es) */

#ifdef _KERNEL

extern struct bootinfo bootinfo;

extern char *bs_getval(const char *);
extern void bs_scan(int (*func)(const char *, const char *, void *), void *arg);
extern int bs_lexparms(const char *, long *, int);
extern char *bs_lexnum(const char *, long *);
extern char *bs_strchr(const char *, char);
extern char *bs_stratoi(const char *, ulong_t *, int);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_BOOTINFO_H */
