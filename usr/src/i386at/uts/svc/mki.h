#ifndef	_SVC_MKI_H	
#define	_SVC_MKI_H	

#ident	"@(#)kern-i386at:svc/mki.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined (_KERNEL_HEADERS)

#include <util/types.h>     		/* REQUIRED */
#include <io/stream.h>	 		/* REQUIRED */
#include <io/ws/ws.h>	 		/* REQUIRED */
#include <proc/regset.h> 		/* REQUIRED */

#elif  defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>     		/* REQUIRED */
#include <sys/stream.h>	 		/* REQUIRED */
#include <sys/ws/ws.h>	 		/* REQUIRED */
#include <sys/regset.h> 		/* REQUIRED */

#endif 
	
extern void *vm86_idtp;		/* Per-engine IDT pointer */

enum request {
	PARM_POST_COOKIE,	/* pointer to current LWP (lwp_t) */
	PARM_FRAME_BASE,	/* stack frame base */
	PARM_CPU_TYPE,		/* pointer to CPU type */
	PARM_PRIVATE,		/* pointer to the location of a scratch
				   memory pointer */
	PARM_GDTP,      	/* pointer to current GDT for this LWP */
	PARM_LDTP,      	/* pointer to current LDT for this LWP */
	PARM_IDTP,      	/* pointer to current IDT for this LWP */
	PARM_TSSP,      	/* pointer to current TSS for this LWP */
	PARM_RUID     	 	/* real UID for this process */
};

/*
 * Merge function offset into the hook fuctions table.
 */
enum hook_id {
	SWITCH_AWAY,
	SWITCH_TO,
	THREAD_EXIT,
	RET_USER,
	SIGNAL,
	QUERY,
	/* Below are UnixWare specific hooks */
	UW_PORT_ALLOC,
	UW_PORT_FREE,
	UW_COM_PPI_IOCTL,
	UW_KD_PPI_IOCTL
};

/*
 *  Index values for the os dependent portion of mki_fault_catch_t
 */
#define MKI_FC_SIZE	6

#define FC_FUNC_INDEX	0
#define FC_FLAGS_INDEX	1

typedef struct {
	int mkifc_catching_user_fault; /* Boolean */
	int mkifc_os_dependent[MKI_FC_SIZE]; /* OS dependent state */
} mki_fault_catch_t;

/*
 * MKI (merge kernel interface) Function prototypes
 */
int		mki_install_hook(enum hook_id, int (*hook_fn)());
void		mki_remove_hook(enum hook_id);

void		mki_set_idt_dpl(void);
void		mki_set_idt_entry(unsigned short, unsigned long *,
				  unsigned long *);
void		mki_set_vm86p(void *);
void		*mki_get_vm86p();
void		mki_getparm(enum request, void *);
void		mki_post_event(void *);
void		mki_mark_vm86();
boolean_t	mki_check_vm86();
void		mki_clear_vm86();
void		mki_pgfault_get_state(int *, mki_fault_catch_t *);
void		mki_pgfault_restore_state(mki_fault_catch_t *);
struct tss386	*mki_alloc_priv_tss();
int		mprotect_k(unsigned long, unsigned int, unsigned int);

/*
 * MHI (merge hook interface) Function prototypes
 */
extern struct mrg_com_data;
	
void		mhi_switch_to(void *);
void		mhi_thread_away(void *);
void		mhi_thread_exit(void *);
void		mhi_ret_user(struct regs *);
boolean_t	mhi_signal(struct regs *, int);
int		mddi_query(int, void *);
int		mhi_port_alloc(unsigned long, unsigned long);
void		mhi_port_free(unsigned long, unsigned long);
int 		mhi_com_ppi_ioctl(struct queue *, struct msgb *,
				    struct mrg_com_data *, int);
int		mhi_kd_ppi_ioctl(queue_t *, mblk_t *, struct iocblk *,
				   ws_channel_t *);
#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_MKI_H */





