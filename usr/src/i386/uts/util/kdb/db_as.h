#ifndef _UTIL_KDB_DB_AS_H	/* wrapper symbol for kernel use */
#define _UTIL_KDB_DB_AS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/kdb/db_as.h	1.12.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * Address space codes for the kernel debugger.
 */

typedef unsigned int	db_as_t;

#define AS_KVIRT	1	/* Kernel virtual */
#define AS_PHYS		2	/* Physical */
#define AS_IO		3	/* I/O ports */
#define AS_UVIRT	4	/* User process virtual */

typedef struct as_addr {
	ullong_t	a_addr;		/* Address within address space */
	db_as_t	a_as;		/* Relevant address space */
	int	a_mod;		/* Numeric address space modifier */
#ifndef UNIPROC
	int	a_cpu;		/* CPU number */
#endif
} as_addr_t;

#define AS_CUR_CPU	-1	/* value of a_cpu for current cpu */
#define AS_ALL_CPU	-2	/* value of a_cpu to specify all cpu's */

#ifndef UNIPROC
#define SET_KVIRT_ADDR_CPU(as_addr, vaddr, cpu) \
		((as_addr).a_addr = (vaddr), (as_addr).a_as = AS_KVIRT, \
		 (as_addr).a_cpu = (cpu))
#define DB_KVTOP(vaddr, cpu)	db_kvtop(vaddr, cpu)
#else
#define SET_KVIRT_ADDR_CPU(as_addr, vaddr, cpu) \
		((as_addr).a_addr = (vaddr), (as_addr).a_as = AS_KVIRT)
#define DB_KVTOP(vaddr, cpu)	db_kvtop(vaddr, 0)
#endif

#define SET_KVIRT_ADDR(as_addr, vaddr) \
		SET_KVIRT_ADDR_CPU(as_addr, vaddr, l.eng_num)

struct proc;
extern paddr64_t db_kvtop(ulong_t vaddr, int cpu);
extern paddr64_t db_uvtop(ulong_t vaddr, const struct proc * proc);
extern int db_read(as_addr_t addr, void * buf, uint_t n);
extern int db_write(as_addr_t addr, const void * buf, uint_t n);
extern void *db_mapin2(paddr64_t paddr);
extern void db_mapout2(void);

extern as_addr_t dis_dot(as_addr_t,int mode16);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KDB_DB_AS_H */
