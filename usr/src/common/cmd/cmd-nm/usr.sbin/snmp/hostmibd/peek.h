#ident	"@(#)peek.h	1.2"

#ifndef _peek_h_
#define _peek_h_

#include <nlist.h>
#include <sys/vmparam.h>
#include <sys/immu.h>
/*
**  min/max kernel virtual address, see peek.c
*/

#include <sys/sysmacros.h>

/*
**  several special devices
*/
#define VMUNIX_FILE     "/stand/unix"
#define KMEM_FILE       "/dev/kmem"
#define MEM_FILE        "/dev/mem"

#define __C(x)      ((caddr_t) x)

/*
**  kernel address to user address
*/
#define __KTU(k)    \
    ( KADDR(k) ? (__C(k) - __C(KVBASE))   + __C(pagingBase): 0 )

/*
**  user address to kernel address
*/
#define __UTK(u)    \
    ( __C(u) < __C(pagingBase) ? 0 : (__C(u) - __C(pagingBase)) + __C(KVBASE) )

/*
**  kernel address to user address with type cast
*/
#define KTUT(k,type)    ((type) __KTU(k))

/*
**  user address to kernel address with type cast
*/
#define UTKT(u,type)    ((type) __UTK(u))

#define KTU(k)          KTUT(k, caddr_t)
#define UTK(u)          UTKT(u, caddr_t)

#define LOG1024             10
#define FSCALE              (1 << 8)

#define percent_cpu(cpu)    ((double) ((double) (cpu)) / ((double) FSCALE))
#define pagetok(size)       ((size*NBPP) >> LOG1024)
#define bytestok(size)      ((size) >> LOG1024)
#define ktobytes(size)      ((size) << LOG1024)
#define bytestopage(size)   size/NBPP

/*
**  dummy entry for nlist array entries not currently used
*/
#define SYM_DUMMY       "_dummy_"

/*
**  base address of mapped kernel pages in user space
**  starting at this base address, all pages must be mapped contiguous!!
*/
extern caddr_t          pagingBase;

extern void peek_kernel_init(void);

#endif

