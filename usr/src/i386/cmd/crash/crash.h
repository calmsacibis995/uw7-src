#ident	"@(#)crash:i386/cmd/crash/crash.h	1.6.3.2"

#include <sys/types.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>

typedef ullong_t phaddr_t;	/* 64-bit physical address or mem offset */
#define highlong(ll) ((long)((ll)>>32))

/* This file should include only command independent declarations */

#define NARGS     25	/* number of arguments to one function */
#define LINESIZE  1024	/* size of function input line */

extern void (*intsig)();
extern void (*pipesig)();
extern FILE *fp, *rp;	/* output file, redirect file pointer */
extern FILE *bp;	/* batch file pointer */
extern int mem;		/* file descriptor for dumpfile */
extern jmp_buf syn;	/* syntax error label */
extern struct var vbuf;	/* tunable variables buffer */
extern char *args[];	/* argument array */
extern int argcnt;	/* number of arguments */
extern int optind;	/* argument index */
extern char *optarg;	/* getopt argument */
extern char vnheading[];/* common vnode heading for inode modules */
extern int tabsize;	/* Size of function table */
extern int debugmode;	/* Set via -D or test command, usually 0-9 */
extern int hexmode;	/* Set via hexmode command: expect and display hex */
extern int active;	/* Set if crash is examining an active system */
extern int pae;		/* Set if 36-bit Page Address Extension enabled */
extern int UniProc;	/* Set if kernel built for single processor only */
extern int Nengine;
extern int Ncg;
extern int Cur_proc;
extern int Cur_lwp;
extern int Cur_eng;
extern int Cur_cg;
extern int Sav_eng;


extern	vaddr_t	kvbase;

#define CR_KADDR(vaddr)  ((vaddr_t)(vaddr) >= kvbase)

/* function definition */
struct func {
	char *name;
	char *syntax;
	int (*call)();
	char *description;
};

#define _OFFSTABLE
struct offstable {
	long	offset;
	char	*name;
};

#ifndef _SYMENT				/* symtab.c defines it for privacy */
struct syment {				/* shorter than that in syms.h */
	ushort	_private_to_symtab1;
	char	_private_to_symtab2;
	char	_private_to_symtab3;
	char 	*n_offset;		/* pointer to name as in syms.h */
	vaddr_t	n_value;		/* value of symbol as in syms.h */
};					/* other syms.h fields unused */
#endif /* _SYMENT */

#define UNKNOWN		((ulong_t) -1)	/* for stacktrace() */
#define ALLTMPBUFS	((void *)  -1)	/* for cr_free() */
#define PROCARGS	((long) -2)	/* for getargs() */

/*
 * prototypes for exported functions
 */
extern struct kcontext;
extern struct engine;
extern struct plocal;
extern struct cglocal;
extern struct proc;
extern struct lwp;
extern struct as;

#define fprintf cr_fprintf /* change decimal or octal to hex if hexmode */
/*PRINTFLIKE2*/
extern int cr_fprintf(FILE *, char *, ...);
/*PRINTFLIKE2*/
extern int raw_fprintf(FILE *, char *, ...);

extern void cr_open(int, char *, char *, char *);
extern void sigint(int);
extern void *cr_malloc(size_t, char *);
extern void cr_unfree(void *);
extern void cr_free(void *);
extern char *db_sym_off(vaddr_t);
extern struct syment *findsym(vaddr_t);
extern struct syment *symsrch(char *);
extern vaddr_t symfindval(char *);
extern struct offstable *structfind(char *, char *);
extern void *readmem(vaddr_t, int, int, void *, size_t, char *);
extern void *readmem64(phaddr_t, int, int, void *, size_t, char *);
extern char *readstr(char *, char *);
extern void readengine(int, struct engine *);
extern void readplocal(int, struct plocal *);
extern void readcglocal(int, struct cglocal *);
extern void readlwp(struct lwp *, struct lwp *);
extern long strcon(char *, char);
extern phaddr_t strcon64(char *, int);
extern boolean_t getargs(long, long *, long *);		/* on args[optind] */
extern int valproc(char *);
extern int setproc(void);				/* on optarg */
extern void get_context(int);
extern struct engine *id_to_eng(int);
extern int eng_to_id(struct engine *);
extern void check_engid(int);
extern pid_t slot_to_pid(int);
extern int pid_to_slot(pid_t);
extern struct proc *slot_to_proc(int);
extern int proc_to_slot(struct proc *);
extern struct lwp *slot_to_lwp(int, struct proc *);
extern int lwp_to_slot(struct lwp *, struct proc *);
extern int getslot(vaddr_t, vaddr_t, size_t, int, int);
extern char *date_time(long);
extern char *getfsname(int);
extern vaddr_t getublock(int, int, boolean_t);
extern void printregs(vaddr_t, unsigned long *, struct kcontext *);
extern struct as *readas(int, struct as **, boolean_t);
extern phaddr_t cr_vtop(vaddr_t, int, size_t *, boolean_t);
extern phaddr_t cr_ptof(phaddr_t, size_t *);
