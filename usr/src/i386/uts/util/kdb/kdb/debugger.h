#ifndef _UTIL_KDB_KDB_DEBUGGER_H	/* wrapper symbol for kernel use */
#define _UTIL_KDB_KDB_DEBUGGER_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/kdb/kdb/debugger.h	1.21.2.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/kdb/db_as.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/db_as.h> /* REQUIRED */
#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#define DBSTKSIZ    64
#define VARTBLSIZ   64
#define LINBUFSIZ   161
#define MAXSTRSIZ   256
#define STRSPCSIZ   4096

#define EOF         -1
#define EOL	    -2

struct item {
    union {
	uchar_t byte[4];
	ushort_t word[2];
	ullong_t number;
	char *string;
    } value;
    unsigned type : 3;
};

/* item types */

/* #define EOF         -1 */
/* #define NULL         0 */

#define NUMBER          1
#define STRING          2
#define NAME            3

#define TYPEMAX         3

struct variable {
    char *name;
    struct item item;
    unsigned type;
};

/* Variable types */
#define VAR_VAR		1	/* normal variable */
#define VAR_MACRO	2	/* command macro */


/* Breakpoint info */
#define MAX_BRKNUM	20	/* Highest logical breakpoint number */
#define MAX_UBRKNUM	19	/* Highest user-visible breakpoint number */
#define MAX_HBRKNUM	3	/* Highest hardware breakpoint number */
#define TMP_BRKNUM	(MAX_UBRKNUM + 1)

struct brkinfo {
	unsigned char	state;		/* breakpoint state */
	unsigned char	type;		/* breakpoint type */
	unsigned char	saved_opc;	/* saved opcode byte */
	unsigned short	tcount;		/* trace count */
	unsigned long	addr;		/* breakpoint address */
	char		*cmds;		/* commands to execute, if any */
};

/* breakpoint types (these must match hardware bit values) */
#define BRK_INST	0	/* instruction execute */
#define BRK_ACCESS	3	/* memory access */
#define BRK_MODIFY	1	/* memory modify */
#define BRK_IO		2	/* I/O port access */

/* breakpoint states */
#define BRK_CLEAR	0
#define BRK_ENABLED	1
#define BRK_DISABLED	2
#define BRK_LOADED	3

/* c_brk flags       */
#define BRK_ANYNUM	0
#define BRK_MYNUM	1
#define BRK_RETNUM	2

/* register types */
#define REG_TYPE	0x3000
#define REG_E		0x0000	/* full 32-bit register */
#define REG_X		0x1000	/* low 16 bits of register */
#define REG_H		0x2000	/* next-to-lowest 8 bits of register */
#define REG_L		0x3000	/* low 8 bits of register */

/* register indexes used internaly by KDB */
#define DB_EAX		0
#define DB_EBX		1
#define DB_ECX		2
#define DB_EDX		3
#define DB_EDI		4
#define DB_ESI		5
#define DB_EBP		6
#define DB_ESP		7
#define DB_EIP		8
#define DB_EFL		9
#define DB_CS		10
#define DB_DS		11
#define DB_ES		12
#define DB_TRAPNO	13

#define LCASEBIT    0x20
#define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')
#define issym(c)   (((c) >= 'A' && (c) <= 'Z') || \
			((c) >= 'a' && (c) <= 'z') || \
			((c) >= '0' && (c) <= '9') || \
			(c) == '_' || (c) == '.' || (c) == '%' || (c) == '?')

#ifndef UNIPROC

#include <util/engine.h>
#include <util/plocal.h>
/* MP-specific variables */

	/* Non-zero if slave has command to process */
volatile int db_slave_flag[MAXNUMCPU];

	/* Non-zero if a cpu is currently active in KDB */
volatile int db_cp_active[MAXNUMCPU];

	/* Current lwp for a particular cpu */
struct lwp * volatile db_lwp[MAXNUMCPU];

#endif /* not UNIPROC */

extern char dbverbose;
extern ushort_t dbibase;
extern ushort_t dbtos;
extern struct item dbstack[];

extern void dbputc(int);
extern void dbprintf(const char *, ...);
extern int dbgetchar(void);
extern void dbunget(int);
extern char *dbpeek(void);
extern char *dbgets(char *, int);
extern void db_brk_msg(int);
extern void db_flush_input(void);
extern void dbcmdline(char *);
extern void dberror(char *);

extern char *dbstrdup();
extern char *dbstralloc();
extern void dbstrfree();

extern short dbgetitem(struct item *);
extern int dbextname(char *);
extern int dbstackcheck();
extern int dbtypecheck();
extern void dbtellsymname(char *, vaddr_t, vaddr_t);

extern ulong_t db_safe_cr4(void);

extern void dbinterp(char *);
extern void db_dis(as_addr_t, uint_t);
extern void db_pvfs(as_addr_t);
extern void db_pfile(as_addr_t);
extern void db_pvnode(as_addr_t);
extern void db_pinode(as_addr_t);
extern void db_puinode(as_addr_t);
extern void db_pprnode(as_addr_t);
extern void db_psnode(as_addr_t);
extern void db_ps(void);
extern void db_dump(as_addr_t, ulong_t);
extern void db_fdump(as_addr_t *, ulong_t, char *);

struct lwp;
struct proc;
extern void db_lstack(struct lwp *, void (*)());
extern void db_pstack(struct proc *, k_lwpid_t, void (*)());
extern void db_tstack(ulong_t, void (*)());

/* Platform-dependent processing hooks */
extern void kdb_pdep_enter(void);
extern void kdb_pdep_exit(void);

#ifndef UNIPROC

extern void db_slave_cmd(int slave, int cmd);

#endif /* !UNIPROC */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KDB_KDB_DEBUGGER_H */
