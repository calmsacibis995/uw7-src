#ident	"@(#)libthread:i386/lib/libthread/archdep/symbols.c	1.1.4.10"
#include <libthread.h>

size_t __SYMBOL__t_ucontext  = offsetof(thread_desc_t, t_ucontext);
size_t __SYMBOL__t_tls  = offsetof(thread_desc_t, t_tls);
size_t __SYMBOL__t_tid  = offsetof(thread_desc_t, t_tid);
size_t __SYMBOL__t_lwpp  = offsetof(thread_desc_t, t_lwpp);
size_t __SYMBOL__t_state  = offsetof(thread_desc_t, t_state);
size_t __SYMBOL__t_usropts  = offsetof(thread_desc_t, t_usropts);
size_t __SYMBOL__t_flags  = offsetof(thread_desc_t, t_flags);
size_t __SYMBOL__t_pri  = offsetof(thread_desc_t, t_pri);
size_t __SYMBOL__t_cid  = offsetof(thread_desc_t, t_cid);
size_t __SYMBOL__t_usingfpu  = offsetof(thread_desc_t, t_usingfpu);
size_t __SYMBOL__t_suspend  = offsetof(thread_desc_t, t_suspend);
size_t __SYMBOL__t_exitval  = offsetof(thread_desc_t, t_exitval);
size_t __SYMBOL__t_idle  = offsetof(thread_desc_t, t_idle);

size_t __SYMBOL__t_lock  = offsetof(thread_desc_t, t_lock);
size_t __SYMBOL__t_join  = offsetof(thread_desc_t, t_join);
size_t __SYMBOL__t_joincount  = offsetof(thread_desc_t, t_joincount);

size_t __SYMBOL__t_nosig  = offsetof(thread_desc_t, t_nosig);
size_t __SYMBOL__t_sig  = offsetof(thread_desc_t, t_sig);
size_t __SYMBOL__t_hold  = offsetof(thread_desc_t, t_hold);
size_t __SYMBOL__t_psig  = offsetof(thread_desc_t, t_psig);
size_t __SYMBOL__t_oldmask  = offsetof(thread_desc_t, t_oldmask);

size_t __SYMBOL__t_callo_alarm  = offsetof(thread_desc_t, t_callo_alarm);
size_t __SYMBOL__t_callo_realit  = offsetof(thread_desc_t, t_callo_realit);
size_t __SYMBOL__t_callo_cv  = offsetof(thread_desc_t, t_callo_cv);

size_t __SYMBOL__t_sp  = offsetof(thread_desc_t, t_ucontext) +
			 offsetof(ucontext_t, uc_mcontext)+
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_ESP);

size_t __SYMBOL__t_pc  = offsetof(thread_desc_t, t_ucontext) +
			 offsetof(ucontext_t, uc_mcontext)+
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_EIP);
size_t __SYMBOL__t_stk  = offsetof(thread_desc_t, t_ucontext) +
			  offsetof(ucontext_t, uc_stack)+
			  offsetof(stack_t, ss_sp);

size_t __SYMBOL__t_stksize  = offsetof(thread_desc_t, t_ucontext) +
			      offsetof(ucontext_t, uc_stack)+
			      offsetof(stack_t, ss_size);

size_t __SYMBOL__lwp_id  = offsetof(__lwp_desc_t, lwp_id);
size_t __SYMBOL__lwp_thread  = offsetof(__lwp_desc_t, lwp_thread);

size_t __SYMBOL__UCTX_SIGMASK  = offsetof(ucontext_t, uc_sigmask);
size_t __SYMBOL__UCTX_STACK  = offsetof(ucontext_t, uc_stack);
size_t __SYMBOL__UCTX_MCONTEXT  = offsetof(ucontext_t, uc_mcontext);

size_t __SYMBOL__UCTX_R_SS   = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_SS);

size_t __SYMBOL__UCTX_R_ESP  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_ESP);

size_t __SYMBOL__UCTX_R_EFL  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_EFL);

size_t __SYMBOL__UCTX_R_CS   = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_CS);

size_t __SYMBOL__UCTX_R_EIP  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_EIP);

size_t __SYMBOL__UCTX_R_EAX  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_EAX);

size_t __SYMBOL__UCTX_R_ECX  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_ECX);

size_t __SYMBOL__UCTX_R_EDX  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_EDX);

size_t __SYMBOL__UCTX_R_EBX  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_EBX);

size_t __SYMBOL__UCTX_R_EBP  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_EBP);

size_t __SYMBOL__UCTX_R_ESI  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_ESI);

size_t __SYMBOL__UCTX_R_EDI  = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_EDI);

size_t __SYMBOL__UCTX_R_DS   = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_DS);

size_t __SYMBOL__UCTX_R_ES   = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_ES);

size_t __SYMBOL__UCTX_R_FS   = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_FS);

size_t __SYMBOL__UCTX_R_GS   = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, gregs) + 
			       (sizeof(long)*R_GS);

size_t __SYMBOL__UCTX_FPREGS = offsetof(ucontext_t, uc_mcontext)+ 
			       offsetof(mcontext_t, fpregs);

size_t __SYMBOL__U_FPCHIP_STATE   = offsetof(fpregset_t, fp_reg_set.fpchip_state);
size_t __SYMBOL__TS_ONPROC   = TS_ONPROC;
size_t __SYMBOL__SIG_SETMASK   = SIG_SETMASK;

size_t __SYMBOL__IDLE_THREAD_ID   = (size_t)-1;
size_t __SYMBOL__TC_SWITCH_BEGIN   = tc_switch_begin;
size_t __SYMBOL__TC_SWITCH_COMPLETE   = tc_switch_complete;
size_t __SYMBOL__t_dbg_switch  = offsetof(thread_desc_t, t_dbg_switch);
size_t __SYMBOL__UC_FP   = UC_FP;
size_t __SYMBOL__UCTX_FLAGS   = offsetof(ucontext_t, uc_flags);
size_t __SYMBOL__FPUSED_FLAG   = offsetof(__lwp_private_page_t,
					    _lwp_private_fpu_used);
