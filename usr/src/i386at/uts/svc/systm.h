#ifndef _SVC_SYSTM_H	/* wrapper symbol for kernel use */
#define _SVC_SYSTM_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/systm.h	1.65.9.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Random set of variables and functions
 * used by more than one routine.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <svc/time.h>   /* REQUIRED */
#include <util/dl.h>    /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/time.h>   /* REQUIRED */
#include <sys/dl.h>     /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

extern struct vnode *rootdir;	/* pointer to vnode of root directory */

extern int	rstchown;	/* 1 ==> restrictive chown(2) semantics */

/* Enhanced Application Compatibility Support */
extern int	poll_delay_compatibility; /* 0=OSR, poll(2) ret on Nfd=0, 1=
					   * honor the poll delay */
/* End Enhanced Application Compatibility Support */

extern dev_t	rootdev;	/* device of the root */
extern dev_t    dumpdev;    	/* dump device */

extern int	cpurate;	/* cpu rate in Mhz */
extern int	lcpuspeed;	/* aprox. VAX MIPS (normalized to 100 Mhz) */
extern int	i486_lcpuspeed;	/* ditto for 25 Mhz 486 */
extern int	upyet;		/* non-zero when system is initialized */
extern boolean_t in_shutdown;	/* non-zero when system being shut down */
extern char	etext[];	/* end of kernel text */

extern int nodev(void);
extern int nulldev(void);
extern dev_t getudev(void);
extern void putudev(dev_t);
extern int stoi(char **);
extern void numtos(u_long, char *);
extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, size_t);
extern char *strcat(char *, const char *);
extern char *strncat(char *, const char *, size_t);
extern char *strpbrk(const char *, const char *);
extern char *strchr(const char *, int);
extern int strcmp(const char *, const char *);
extern int strlen(const char *);
extern int strncmp(const char *, const char *, size_t);
extern unsigned long strtoul(const char *, char **, int);
extern void ticks_to_timestruc(timestruc_t *, dl_t *);
extern int strcpy_len(char *, const char *);
extern int strcpy_max(char *, const char *, size_t);
extern int bcmp(const void *, const void *, size_t);

extern int ucopyin(const void *, void *, size_t, uint_t);
extern int copyin(const void *, void *, size_t);
extern int ucopyout(const void *, void *, size_t, uint_t);
extern int copyout(const void *, void *, size_t);
extern int fubyte(const char *);
extern int fushort(const ushort_t *, ushort_t *);
extern int fuword(const int *);
extern int subyte(char *, char);
extern int sushort(ushort_t *, ushort_t);
extern int suword(int *, int);
extern int copyinstr(const char *, char *, size_t, size_t *);
extern int copystr(const char *, char *, size_t, size_t *);
extern int uzero(void *, size_t);
extern int kzero(void *, size_t);

extern void bcopy(const void *, void *, size_t);
extern void ovbcopy(void *, void *, size_t);
extern void bzero(void *, size_t);
extern void struct_zero(void *, size_t);
extern void bscan(void *, size_t);

extern int setjmp(label_t *);
extern void longjmp(label_t *);

extern int arglistsz(vaddr_t, int *, size_t *, int);
extern int copyarglist(int, vaddr_t, int, vaddr_t, vaddr_t, boolean_t);

extern caddr_t physmap(paddr_t, ulong_t, uint_t);
extern caddr_t physmap64(paddr64_t, ulong_t, uint_t);
extern void physmap_free(caddr_t, ulong_t, uint_t);

extern pl_t spl0(void);
extern pl_t spl1(void);
extern pl_t spl4(void);
extern pl_t spl5(void);
extern pl_t spl6(void);
extern pl_t spl7(void);
extern pl_t spltty(void);
extern pl_t splhi(void);
extern pl_t spldisk(void);
extern pl_t splstr(void);
extern void splx(pl_t);

#undef min
#undef max
extern int min(uint, uint);
extern int max(uint, uint);

extern void si_add_bustype(const char *bustype);
extern void call_demon(void);
extern void loadldt(ushort);
extern void setup_seg_regs(void);
extern void enable_nmi(void);
extern void bootarg_parse(void);
extern void conf_mem(void);
extern int calc_delay(int);
extern boolean_t mainstore_memory(paddr64_t);
extern void drv_usecwait(clock_t);
extern void fpu_error(void);
extern void save_fpu(void);
extern void configure(void);
extern void init_fpu(void);
extern void t_diverr(void);
extern void t_dbg(void);
extern void t_nmi(void);
extern void t_int3(void);
extern void t_into(void);
extern void t_check(void);
extern void t_und(void);
extern void t_dna(void);
extern void t_syserr(void);
extern void t_extovr(void);
extern void t_res(void);
extern void t_badtss(void);
extern void t_notpres(void);
extern void t_stkflt(void);
extern void t_gpflt(void);
extern void t_pgflt(void);
extern void t_coperr(void);
extern void t_alignflt(void);
extern void t_mceflt(void);
extern void selfinit(int);
extern void sys_call(void);
extern void sig_clean(void);
extern void yield(void);
extern void cl_trapret(void);
extern void devint20(void);
extern void devint21(void);
extern void devint22(void);
extern void devint23(void);
extern void devint24(void);
extern void devint25(void);
extern void devint26(void);
extern void devint27(void);
extern void devint28(void);
extern void devint29(void);
extern void devint2a(void);
extern void devint2b(void);
extern void devint2c(void);
extern void devint2d(void);
extern void devint2e(void);
extern void devint2f(void);
extern void devint30(void);
extern void devint31(void);
extern void devint32(void);
extern void devint33(void);
extern void devint34(void);
extern void devint35(void);
extern void devint36(void);
extern void devint37(void);
extern void devint38(void);
extern void devint39(void);
extern void devint3a(void);
extern void devint3b(void);
extern void devint3c(void);
extern void devint3d(void);
extern void devint3e(void);
extern void devint3f(void);
extern void devint40(void);
extern void devint41(void);
extern void devint42(void);
extern void devint43(void);
extern void devint44(void);
extern void devint45(void);
extern void devint46(void);
extern void devint47(void);
extern void devint48(void);
extern void devint49(void);
extern void devint4a(void);
extern void devint4b(void);
extern void devint4c(void);
extern void devint4d(void);
extern void devint4e(void);
extern void devint4f(void);
extern void devint50(void);
extern void devint51(void);
extern void devint52(void);
extern void devint53(void);
extern void devint54(void);
extern void devint55(void);
extern void devint56(void);
extern void devint57(void);
extern void devint58(void);
extern void devint59(void);
extern void devint5a(void);
extern void devint5b(void);
extern void devint5c(void);
extern void devint5d(void);
extern void devint5e(void);
extern void devint5f(void);
extern void devint60(void);
extern void devint61(void);
extern void devint62(void);
extern void devint63(void);
extern void devint64(void);
extern void devint65(void);
extern void devint66(void);
extern void devint67(void);
extern void devint68(void);
extern void devint69(void);
extern void devint6a(void);
extern void devint6b(void);
extern void devint6c(void);
extern void devint6d(void);
extern void devint6e(void);
extern void devint6f(void);
extern void devint70(void);
extern void devint71(void);
extern void devint72(void);
extern void devint73(void);
extern void devint74(void);
extern void devint75(void);
extern void devint76(void);
extern void devint77(void);
extern void devint78(void);
extern void devint79(void);
extern void devint7a(void);
extern void devint7b(void);
extern void devint7c(void);
extern void devint7d(void);
extern void devint7e(void);
extern void devint7f(void);
extern void devint80(void);
extern void devint81(void);
extern void devint82(void);
extern void devint83(void);
extern void devint84(void);
extern void devint85(void);
extern void devint86(void);
extern void devint87(void);
extern void devint88(void);
extern void devint89(void);
extern void devint8a(void);
extern void devint8b(void);
extern void devint8c(void);
extern void devint8d(void);
extern void devint8e(void);
extern void devint8f(void);
extern void devint90(void);
extern void devint91(void);
extern void devint92(void);
extern void devint93(void);
extern void devint94(void);
extern void devint95(void);
extern void devint96(void);
extern void devint97(void);
extern void devint98(void);
extern void devint99(void);
extern void devint9a(void);
extern void devint9b(void);
extern void devint9c(void);
extern void devint9d(void);
extern void devint9e(void);
extern void devint9f(void);
extern void devinta0(void);
extern void devinta1(void);
extern void devinta2(void);
extern void devinta3(void);
extern void devinta4(void);
extern void devinta5(void);
extern void devinta6(void);
extern void devinta7(void);
extern void devinta8(void);
extern void devinta9(void);
extern void devintaa(void);
extern void devintab(void);
extern void devintac(void);
extern void devintad(void);
extern void devintae(void);
extern void devintaf(void);
extern void devintb0(void);
extern void devintb1(void);
extern void devintb2(void);
extern void devintb3(void);
extern void devintb4(void);
extern void devintb5(void);
extern void devintb6(void);
extern void devintb7(void);
extern void devintb8(void);
extern void devintb9(void);
extern void devintba(void);
extern void devintbb(void);
extern void devintbc(void);
extern void devintbd(void);
extern void devintbe(void);
extern void devintbf(void);
extern void devintc0(void);
extern void devintc1(void);
extern void devintc2(void);
extern void devintc3(void);
extern void devintc4(void);
extern void devintc5(void);
extern void devintc6(void);
extern void devintc7(void);
extern void devintc8(void);
extern void devintc9(void);
extern void devintca(void);
extern void devintcb(void);
extern void devintcc(void);
extern void devintcd(void);
extern void devintce(void);
extern void devintcf(void);
extern void devintd0(void);
extern void devintd1(void);
extern void devintd2(void);
extern void devintd3(void);
extern void devintd4(void);
extern void devintd5(void);
extern void devintd6(void);
extern void devintd7(void);
extern void devintd8(void);
extern void devintd9(void);
extern void devintda(void);
extern void devintdb(void);
extern void devintdc(void);
extern void devintdd(void);
extern void devintde(void);
extern void devintdf(void);
extern void devinte0(void);
extern void devinte1(void);
extern void devinte2(void);
extern void devinte3(void);
extern void devinte4(void);
extern void devinte5(void);
extern void devinte6(void);
extern void devinte7(void);
extern void devinte8(void);
extern void devinte9(void);
extern void devintea(void);
extern void devinteb(void);
extern void devintec(void);
extern void devinted(void);
extern void devintee(void);
extern void devintef(void);
extern void devintf0(void);
extern void devintf1(void);
extern void devintf2(void);
extern void devintf3(void);
extern void devintf4(void);
extern void devintf5(void);
extern void devintf6(void);
extern void devintf7(void);
extern void devintf8(void);
extern void devintf9(void);
extern void devintfa(void);
extern void devintfb(void);
extern void devintfc(void);
extern void devintfd(void);
extern void devintfe(void);
extern void devintff(void);

extern void drv_suspend_all(void);
extern void shutdown_drv(void);
#ifdef UNIPROC
/* UP machine xcall functions are no-ops */
#define xcall(targets, responders, func, arg) \
		((responders) ? EMASK_CLRALL((emask_t *)(responders)) : 0)
#define xcall_all(responders, timed, func, arg) \
		EMASK_CLRALL((emask_t *)(responders))
#else
struct emask;
extern void xcall_init(void);
extern void xcall(struct emask *, struct emask *, void (*)(), void *);
extern void xcall_all(struct emask *, boolean_t, void (*)(), void *);
extern int xcall_intr(void);
extern void xcall_softint(void *);
#endif /* UNIPROC */

#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Structure of the system-entry table.
 */
struct sysent {
	char	sy_narg;		/* total number of arguments */
	int	(*sy_call)();		/* handler */
};

#ifdef _KERNEL
extern struct sysent sysent[];
extern unsigned	sysentsize;
#endif

/*
 * Structure of the return-value parameter passed by reference to
 * system entries.
 */
union rval {
	struct	{
		int	r_v1;
		int	r_v2;
	} r_v;
	off32_t	r_off;
	off64_t r_off64;	/* lseek64 needs this */
	time_t	r_time;
	cgid_t	r_cgid;		/* cg_current needs this */
};

#define r_val1	r_v.r_v1
#define r_val2	r_v.r_v2
	
typedef union rval rval_t;

struct panic_data {
	struct engine *pd_engine;	/* Panicking engine */
	struct lwp *pd_lwp;		/* LWP running at time of panic */
	struct kcontext *pd_rp;		/* saved regs in panic frame */
	struct kcontext *pd_dblrp;	/* saved regs in dblpanic frame */
};
 
#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern uint_t plocal_intr_depth;

#define servicing_interrupt()		plocal_intr_depth
#define was_servicing_interrupt()	(plocal_intr_depth > 1)

#ifdef MERGE386
extern boolean_t mki_uaddr_mapped(vaddr_t);
extern boolean_t mki_upageflt(vaddr_t, int);
#endif /* MERGE386 */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_SYSTM_H */
