#ifndef _UTIL_KDB_DB_SLAVE_H	/* wrapper symbol for kernel use */
#define _UTIL_KDB_DB_SLAVE_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/kdb/db_slave.h	1.10"
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
 * Master-slave interaction for multi-processor systems.
 */

#ifndef UNIPROC

struct slave_cmd {		/* Slave command information */
	int		cmd;
	int		rval;
	as_addr_t	addr;
	void		*buf;
	uint_t		n;
};

/* Commands sent from the master to the slave */

#define DBSCMD_EXIT	  1	/* Exit from debugger */
#define DBSCMD_SYNC	  2	/* No-op command to sync with slaves */
#define DBSCMD_GET_STACK  3	/* Get starting point for stack trace */
#define DBSCMD_AS_READ	  4	/* Read from a given address space */
#define DBSCMD_AS_WRITE	  5	/* Write to a given address space */
#define DBSCMD_CR0	  6	/* Get the value of %cr0 */
#define DBSCMD_CR2	  7	/* Get the value of %cr2 */
#define DBSCMD_CR3	  8	/* Get the value of %cr3 */
#define DBSCMD_CR4	  9	/* Get the value of %cr4 */

/* Master/slave communication area */

volatile struct slave_cmd db_slave_command;

#endif /* _KERNEL */

#endif /* not UNIPROC */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KDB_DB_SLAVE_H */
