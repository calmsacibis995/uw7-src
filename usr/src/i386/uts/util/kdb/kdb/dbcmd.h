#ifndef _UTIL_KDB_KDB_DBCMD_H	/* wrapper symbol for kernel use */
#define _UTIL_KDB_KDB_DBCMD_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/kdb/kdb/dbcmd.h	1.7"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <util/kdb/db_as.h> /* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h> /* REQUIRED */
#include <sys/db_as.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * The debugger's fixed command table structure.
 */
struct cmdentry {
    char    *name;          /* command's textual name */
    void    (*func)();	    /* function to call to handle this command */
    int	    arg;	    /* argument to func */
    uchar_t stackcheck;     /* stack bounds check, see STACKCHK(d,u) below  */
    uchar_t parmcnt;        /* number of checked stack parameters for cmd   */
    uchar_t parmtypes[3];   /* allowed type mask for each parm, 3 max       */
};

/*
 * Suffix table structure.
 */
struct suffix_entry {
    char    name[4];        /* suffix's textual name */
    db_as_t as;		    /* address space to assign */
    int	    size;	    /* operand size to use */
    int     brk;	    /* breakpoint type */
    int	    flags;	    /* special flags */
};

/* Values for suffix_entry flags */
#define SFX_NUM	 0x01	    /* suffix takes a (hex) numeric argument */
#define SFX_RSET 0x02	    /* numeric argument is register set number */
#define SFX_CPU	 0x04	    /* numeric argument is cpu number */

/*
 * Parameter type checking masks.
 *  These can be used singly or in combination to specify the allowed types
 *  of stack parameters for each command.
 */
#define T_NUMBER        0x01
#define T_NAME          0x02
#define T_STRING        0x04

/*
 * Stack depth checking masks.
 *  The stackcheck member of struct cmdentry specifies the lower and upper
 *  bounds of stack growth during the command's execution.  The predefined
 *  values S_1_0, S_2_0, and S_0_1 are the only ones currently needed, but
 *  for new commands, arbitrary checks can be constructed with STACKCHK().
 */

#define S_DOWNMASK      0x0f
#define S_UPMASK        0xf0
#define S_DOWNSHIFT     0
#define S_UPSHIFT       4

#define S_DOWN(x) (((unsigned)(x)&S_DOWNMASK)>>S_DOWNSHIFT)
#define S_UP(x)   (((unsigned)(x)&S_UPMASK)>>S_UPSHIFT)

#define STACKCHK(d,u)   ((((d)<<S_DOWNSHIFT)&S_DOWNMASK) | \
					(((u)<<S_UPSHIFT)&S_UPMASK))
#define S_1_0           STACKCHK(1,0)
#define S_1_1           STACKCHK(1,1)
#define S_2_0           STACKCHK(2,0)
#define S_0_1           STACKCHK(0,1)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KDB_KDB_DBCMD_H */
