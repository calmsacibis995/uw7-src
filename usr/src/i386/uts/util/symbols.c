#ident	"@(#)kern-i386:util/symbols.c	1.52.8.4"
#ident	"$Header$"

/*
 * Generate symbols for use by assembly language files in kernel.
 *
 * This file is compiled using "-S"; "symbols.awk" walks over the assembly
 * file and extracts the symbols.
 */

#include <mem/faultcatch.h>
#include <mem/vmmeter.h>
#include <mem/vmparam.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/regset.h>
#include <proc/seg.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <svc/creg.h>
#include <svc/fp.h>
#include <svc/reg.h>
#include <svc/trap.h>
#include <svc/time.h>
#include <svc/v86bios.h>
#include <util/cglocal.h>
#include <util/cmn_err.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#ifdef DEBUG_TRACE
#include <util/nuc_tools/trace/nwctrace.h>
#endif /* DEBUG_TRACE */

#define	offsetof(x, y)	((int)&((x *)0)->y)
#define	OFFSET(s, st, m) \
	size_t __SYMBOL___A_##s = offsetof(st, m)

#define	DEFINE(s, e) \
	size_t __SYMBOL___A_##s = (size_t)(e)

/*
 * Control Register 0 (CR0) and 4 (CR4) definitions.
 */

DEFINE(CR0_PG, CR0_PG);
DEFINE(CR0_CD, CR0_CD);
DEFINE(CR0_NW, CR0_NW);
DEFINE(CR0_AM, CR0_AM);
DEFINE(CR0_WP, CR0_WP);
DEFINE(CR0_NE, CR0_NE);
DEFINE(CR0_TS, CR0_TS);
DEFINE(CR0_EM, CR0_EM);
DEFINE(CR0_MP, CR0_MP);
DEFINE(CR0_PE, CR0_PE);
DEFINE(CR4_PSE, CR4_PSE);
DEFINE(CR4_PAE, CR4_PAE);

/*
 * Misc kernel virtual addresses and constants.
 */

DEFINE(KVPLOCAL, KVPLOCAL);
DEFINE(KVCGLOCAL, KVCGLOCAL);
DEFINE(KVPLOCALMET, KVPLOCALMET);
DEFINE(KVMET, KVMET);
DEFINE(KVUENG, KVUENG);
DEFINE(KVUVWIN, KVUVWIN);
DEFINE(KVENG_L1PT, KVENG_L1PT);
DEFINE(KVSYSDAT, KVSYSDAT);
DEFINE(TIMESTR_SIZ, sizeof(timestruc_t));

/*
 * Offset within an LWP ublock where the uarea begins.
 */

DEFINE(SIZEOF_PTESHIFT, 2);
DEFINE(PS_IE, PS_IE);
DEFINE(UAREA_OFFSET, UAREA_OFFSET);

/*
 * Processor local fields ("l.").
 */

OFFSET(L_ARGSAVE, struct plocal, argsave);
OFFSET(L_FPUON, struct plocal, fpuon);
OFFSET(L_FPUOFF, struct plocal, fpuoff);
OFFSET(L_USINGFPU, struct plocal, usingfpu);
OFFSET(L_HATOPS, struct plocal, hatops);

OFFSET(HOP_ASLOAD, struct hatops, hat_asload);
OFFSET(HOP_ASUNLOAD, struct hatops, hat_asunload);

/*
 * Processor local metric fields ("lm.").
 */

OFFSET(LM_CNT, struct plocalmet, cnt);

OFFSET(L_EVENTFLAGS, struct plocal, eventflags);
DEFINE(EVT_RUNRUN, EVT_RUNRUN);
DEFINE(EVT_KPRUNRUN, EVT_KPRUNRUN);
DEFINE(EVT_UPREEMPT, EVT_UPREEMPT);

OFFSET(L_IDTP, struct plocal, idtp);
OFFSET(L_TSS, struct plocal, tss);
OFFSET(L_TRAP_ERR_CODE, struct plocal, trap_err_code);
#if defined DEBUG && defined _LOCKTEST
OFFSET(L_HOLDFASTLOCK, struct plocal, holdfastlock);
OFFSET(L_FSPIN, struct plocal, fspin);
#endif /* DEBUG && _LOCKTEST */
OFFSET(L_INTR_DEPTH, struct plocal, intr_depth);
OFFSET(L_ENG, struct plocal, eng);
OFFSET(L_ENG_NUM, struct plocal, eng_num);
OFFSET(L_KVPTE, struct plocal, kvpte);
OFFSET(L_KVPTE64, struct plocal, kvpte64);
OFFSET(L_KPD0, struct plocal, kpd0);
OFFSET(L_NMI_HANDLER, struct plocal, nmi_handler);
OFFSET(L_USERP, struct plocal, userp);
OFFSET(L_CURPROCP, struct plocal, procp);
OFFSET(L_CPU_VENDOR, struct plocal, cpu_vendor);
OFFSET(L_CPU_FAMILY, struct plocal, cpu_family);
OFFSET(L_CPU_MODEL, struct plocal, cpu_model);
OFFSET(L_CPU_STEPPING, struct plocal, cpu_stepping);
OFFSET(L_CPU_FEATURES, struct plocal, cpu_features);
OFFSET(L_CPU_ID, struct plocal, cpu_id);
OFFSET(L_PRMPT_STATE, struct plocal, prmpt_state);
#ifdef _MPSTATS
OFFSET(L_UPRMPTCNT, struct plocal, prmpt_user);
OFFSET(L_KPRMPTCNT, struct plocal, prmpt_kern);
#endif /* _MPSTATS */
OFFSET(L_KSE_PTE, struct plocal, kse_pte);
OFFSET(L_SPECIAL_LWP, struct plocal, special_lwp);
OFFSET(L_FPE_KSTATE, struct plocal, fpe_kstate);
#ifdef MERGE386
OFFSET(L_VM86_IDTP, struct plocal, vm86_idtp);
#endif /* MERGE386 */

/*
 * FP emul struct offsets
 */

OFFSET(FPE_STATE, struct fpemul_kstate, fpe_state);

/*
 * lock_t member offsets
 */

OFFSET(SP_LOCK, lock_t, sp_lock);
#if (defined DEBUG || defined SPINDEBUG)
OFFSET(SP_FLAGS, lock_t, sp_flags);
OFFSET(SP_MINIPL, lock_t, sp_minipl);
OFFSET(SP_HIER, lock_t, sp_hier);
OFFSET(SP_VALUE, lock_t, sp_value);
OFFSET(SP_LKSTATP, lock_t, sp_lkstatp);
OFFSET(SP_INFOP, lock_t, sp_lkinfop);
#endif /* (DEBUG || SPINDEBUG)  */

/*
 * rwlock_t member offsets
 */

OFFSET(RWS_FSPIN, rwlock_t, rws_fspin);
OFFSET(RWS_LOCK, rwlock_t, rws_lock);
OFFSET(RWS_RDCOUNT, rwlock_t, rws_rdcount);
OFFSET(RWS_STATE, rwlock_t, rws_state);
#if (defined DEBUG || defined SPINDEBUG) 
OFFSET(RWS_FLAGS, rwlock_t, rws_flags);
OFFSET(RWS_HIER, rwlock_t, rws_hier);
OFFSET(RWS_MINIPL, rwlock_t, rws_minipl);
OFFSET(RWS_VALUE, rwlock_t, rws_value);
OFFSET(RWS_LKSTATP, rwlock_t, rws_lkstatp);
OFFSET(RWS_INFOP, rwlock_t, rws_lkinfop);
#endif /* (DEBUG || SPINDEBUG)  */

/*
 * sleep and rwsleep member offsets
 */
OFFSET(SL_AVAIL, sleep_t, sl_avail);
OFFSET(RW_AVAIL, rwsleep_t, rw_avail);

/*
 * lkstat_t member offsets
 */

OFFSET(LKS_INFOP, lkstat_t, lks_infop);
OFFSET(LKS_WRCNT, lkstat_t, lks_wrcnt);
OFFSET(LKS_RDCNT, lkstat_t, lks_rdcnt);
OFFSET(LKS_SOLORDCNT, lkstat_t, lks_solordcnt);
OFFSET(LKS_FAIL, lkstat_t, lks_fail);
OFFSET(LKS_STIME, lkstat_t, lks_stime);
OFFSET(LKS_NEXT, lkstat_t, lks_next);
OFFSET(LKS_WTIME, lkstat_t, lks_wtime);
OFFSET(LKS_HTIME, lkstat_t, lks_htime);


/*
 * offsets within the lwp structure.
 */

OFFSET(LWP_MUTEX, lwp_t, l_mutex);
OFFSET(LWP_PROCP, lwp_t, l_procp);
OFFSET(LWP_TRAPEVF, lwp_t, l_trapevf);
OFFSET(LWP_TSSP, lwp_t, l_tssp);
OFFSET(LWP_UP, lwp_t, l_up);
OFFSET(LWP_BPT, lwp_t, l_beingpt);
OFFSET(LWP_SPECIAL, lwp_t, l_special);
OFFSET(LWP_START, lwp_t, l_start);
#ifdef MERGE386
OFFSET(LWP_IDTP, lwp_t, l_idtp);
OFFSET(LWP_DESCTABP, lwp_t, l_desctabp);
#endif

/*
 * TSS-related offsets.
 */

OFFSET(ST_TSS, struct stss, st_tss);
OFFSET(ST_TSSDESC, struct stss, st_tssdesc);
OFFSET(T_ESP0, struct tss386, t_esp0);

/*
 * offsets within the proc structure
 */

OFFSET(P_AS, proc_t, p_as);

/*
 * Defines for user structure innards.
 */

OFFSET(U_LWPP, struct user, u_lwpp);
OFFSET(U_PROCP, struct user, u_procp);
OFFSET(U_PRIVATEDATAP, struct user, u_privatedatap);
OFFSET(U_FP_USED, struct user, u_fp_used);
OFFSET(U_FAULT_CATCH, struct user, u_fault_catch);
OFFSET(U_KSE_PTEP, struct user, u_kse_ptep);
OFFSET(U_GDT_INFOP, struct user, u_dt_infop[DT_GDT]);
OFFSET(U_GDT_DESC, struct user, u_gdt_desc);
OFFSET(U_LDT_INFOP, struct user, u_dt_infop[DT_LDT]);
OFFSET(U_LDT_DESC, struct user, u_ldt_desc);
OFFSET(U_AR0, struct user, u_ar0);
OFFSET(U_FPE_RESTART, struct user, u_fpe_restart);
DEFINE(SIZEOF_FPE_RESTART, sizeof u.u_fpe_restart);
#ifdef MERGE386
OFFSET(U_VM86P, struct user, u_vm86p);
#endif /* MERGE386 */
DEFINE(SIZEOF_USER, sizeof(struct user));

/*
 * Offsets within the uvwindow structure.
 */

OFFSET(UV_PRIVATEDATAP, struct uvwindow, uv_privatedatap);
OFFSET(UV_FP_USED, struct uvwindow, uv_fp_used);

/*
 * Defines for kcontext innards.
 */

OFFSET(U_KCONTEXT, struct user, u_kcontext);

OFFSET(KCTX_EBX, kcontext_t, kctx_ebx);
OFFSET(KCTX_EBP, kcontext_t, kctx_ebp);
OFFSET(KCTX_EDI, kcontext_t, kctx_edi);
OFFSET(KCTX_ESI, kcontext_t, kctx_esi);
OFFSET(KCTX_EIP, kcontext_t, kctx_eip);
OFFSET(KCTX_ESP, kcontext_t, kctx_esp);
OFFSET(KCTX_EAX, kcontext_t, kctx_eax);
OFFSET(KCTX_ECX, kcontext_t, kctx_ecx);
OFFSET(KCTX_EDX, kcontext_t, kctx_edx);
OFFSET(KCTX_FS, kcontext_t, kctx_fs);
OFFSET(KCTX_GS, kcontext_t, kctx_gs);
OFFSET(KCTX_FPREGS, kcontext_t, kctx_fpregs);
OFFSET(FPCHIP_STATE, fpregset_t, fp_reg_set.fpchip_state);
OFFSET(FP_EMUL_SPACE, fpregset_t, fp_reg_set.fp_emul_space);
OFFSET(KCTX_DEBUGON, kcontext_t, kctx_debugon);
OFFSET(KCTX_DBREGS, kcontext_t, kctx_dbregs);

DEFINE(SIZEOF_FP_EMUL, sizeof(struct fpemul_state));

/*
 * defines for fault_catch_t
 */

OFFSET(FC_FLAGS, fault_catch_t, fc_flags);
OFFSET(FC_FUNC, fault_catch_t, fc_func);
DEFINE(CATCH_ALL_FAULTS, CATCH_ALL_FAULTS);

/*
 * Offsets within desctab_info
 */

OFFSET(DI_TABLE, struct desctab_info, di_table);

/*
 * Masks used to initialized the FPU after an fninit.
 */

DEFINE(FPU_INIT_OLD_ANDMASK, ~(FPINV|FPZDIV|FPOVR|FPPC));
DEFINE(FPU_INIT_OLD_ORMASK, FPSIG53|FPA);
DEFINE(FPU_INIT_ORMASK, FPA);

/*
 * Selector definitions.
 */

DEFINE(KTSSSEL, KTSSSEL);
DEFINE(KPRIVLDTSEL, KPRIVLDTSEL);
DEFINE(KPRIVTSSSEL, KPRIVTSSSEL);
DEFINE(KCSSEL, KCSSEL);
DEFINE(KDSSEL, KDSSEL);
DEFINE(KLDTSEL, KLDTSEL);
DEFINE(FPESEL, FPESEL);
DEFINE(SEL_RPL, SEL_RPL);

DEFINE(TSS3_KACC1, TSS3_KACC1);

/*
 * vmmeter l.cnt fields
 */

OFFSET(V_INTR, struct vmmeter, v_intr);

/*
 * Register offsets in stack locore frames.
 */

DEFINE(INTR_SP_CS, INTR_SP_CS*sizeof(int));
DEFINE(INTR_SP_IP, INTR_SP_IP*sizeof(int));
DEFINE(SP_CS, SP_CS*sizeof(int));
DEFINE(SP_EIP, SP_EIP*sizeof(int));
DEFINE(SP_EFL, SP_EFL*sizeof(int));
DEFINE(T_CS, T_CS);

/*
 * Trap type mnemonics.
 */

DEFINE(DIVERR, DIVERR);
DEFINE(SGLSTP, SGLSTP);
DEFINE(NMIFLT, NMIFLT);
DEFINE(BPTFLT, BPTFLT);
DEFINE(INTOFLT, INTOFLT);
DEFINE(BOUNDFLT, BOUNDFLT);
DEFINE(INVOPFLT, INVOPFLT);
DEFINE(NOEXTFLT, NOEXTFLT);
DEFINE(DBLFLT, DBLFLT);
DEFINE(EXTOVRFLT, EXTOVRFLT);
DEFINE(INVTSSFLT, INVTSSFLT);
DEFINE(SEGNPFLT, SEGNPFLT);
DEFINE(STKFLT, STKFLT);
DEFINE(GPFLT, GPFLT);
DEFINE(PGFLT, PGFLT);
DEFINE(EXTERRFLT, EXTERRFLT);
DEFINE(ALIGNFLT, ALIGNFLT);
DEFINE(MCEFLT, MCEFLT);
DEFINE(TRP_PREEMPT, TRP_PREEMPT);
DEFINE(TRP_UNUSED, TRP_UNUSED);

/*
 * CPU/FPU type defines.
 */

DEFINE(CPU_386, CPU_386);
DEFINE(CPU_486, CPU_486);
DEFINE(CPU_P5, CPU_P5);
DEFINE(CPU_P6, CPU_P6);
DEFINE(FP_NO, FP_NO);
DEFINE(FP_SW, FP_SW);
DEFINE(FP_287, FP_287);
DEFINE(FP_387, FP_387);

/*
 * Misc defines.
 */

DEFINE(UVBASE, UVBASE);
DEFINE(CE_PANIC, CE_PANIC);
DEFINE(SP_LOCKED, SP_LOCKED);
DEFINE(SP_UNLOCKED, SP_UNLOCKED);
DEFINE(KS_LOCKTEST, KS_LOCKTEST);
DEFINE(KS_MPSTATS, KS_MPSTATS);
DEFINE(RWS_READ, RWS_READ);
DEFINE(RWS_WRITE, RWS_WRITE);
DEFINE(RWS_UNLOCKED, RWS_UNLOCKED);
DEFINE(B_TRUE, B_TRUE);
DEFINE(B_FALSE, B_FALSE);
DEFINE(TRAPEXIT_FLAGS, TRAPEXIT_FLAGS);
DEFINE(RWS_LOCKTYPE, RWS_LOCKTYPE);
DEFINE(SP_LOCKTYPE, SP_LOCKTYPE);
DEFINE(NBPW, NBPW);
DEFINE(PAGESIZE, PAGESIZE);
DEFINE(SPECF_PRIVGDT, SPECF_PRIVGDT);
DEFINE(SPECF_PRIVLDT, SPECF_PRIVLDT);
DEFINE(SPECF_PRIVTSS, SPECF_PRIVTSS);
DEFINE(SPECF_SWTCH1_CALLBACK, SPECF_SWTCH1_CALLBACK);
DEFINE(SPECF_SWTCH2_CALLBACK, SPECF_SWTCH2_CALLBACK);
/* For V86BIOS */
DEFINE(KV86TSSSEL, KV86TSSSEL);
DEFINE(V86INT, V86INT);

#ifdef MERGE386
DEFINE(SPECF_VM86, SPECF_VM86);
DEFINE(SPECF_PRIVIDT, SPECF_PRIVIDT);
#endif /* MERGE386 */
DEFINE(KSTACK_RESERVE, KSTACK_RESERVE);

#ifdef DEBUG_TRACE_LOCKS

DEFINE(NVLTT_rwLockReadWait, NVLTT_rwLockReadWait);
DEFINE(NVLTT_rwLockReadGet, NVLTT_rwLockReadGet);
DEFINE(NVLTT_rwLockWriteGet, NVLTT_rwLockWriteGet);
DEFINE(NVLTT_rwLockWriteWait, NVLTT_rwLockWriteWait);
DEFINE(NVLTT_rwTryReadGet, NVLTT_rwTryReadGet);
DEFINE(NVLTT_rwTryReadFail, NVLTT_rwTryReadFail);
DEFINE(NVLTT_rwTryWriteGet, NVLTT_rwTryWriteGet);
DEFINE(NVLTT_rwTryWriteFail, NVLTT_rwTryWriteFail);
DEFINE(NVLTT_rwLockFree, NVLTT_rwLockFree);

OFFSET(LK_NAME, lkinfo_t, lk_name);
DEFINE(KS_NVLTTRACE, KS_NVLTTRACE);

#endif /* DEBUG_TRACE_LOCKS */

/*
 * Cpu-group Module local fields ("cg.").
 */
OFFSET(CG_CG_NUM, struct cglocal, cg_num);
OFFSET(CG_CG_P0, struct cglocal, proc_sys);
OFFSET(CG_CG_CRED, struct cglocal, sys_cred);
OFFSET(CG_CG_RLIMIT, struct cglocal, sys_rlimits);
OFFSET(CG_CG_RQ, struct cglocal, cg_rq);
OFFSET(CG_ENGINE, struct cglocal, cg_feng);
OFFSET(CG_GLOBAL_TODO, struct cglocal, cg_todo);
OFFSET(CG_NONLINE, struct cglocal, cg_nonline);
OFFSET(CG_STRNSCHED, struct cglocal, strnsched);
OFFSET(CG_QSVC, struct cglocal, qsvc);
OFFSET(CG_NSCHED, struct cglocal, Nsched);
OFFSET(CG_LASTPICK, struct cglocal, lastpick);
OFFSET(CG_BCALL, struct cglocal, bcall);
OFFSET(CG_STRBCID, struct cglocal, strbcid);
OFFSET(CG_VM_PAGEFREELOCKPL, struct cglocal, vm_pagefreelockpl);
OFFSET(CG_MEM_FREEMEM, struct cglocal, mem_freemem);
OFFSET(CG_MAXFREEMEM, struct cglocal, maxfreemem);
OFFSET(CG_PAGE_FREELIST, struct cglocal, page_freelist);
OFFSET(CG_PAGE_CACHELIST, struct cglocal, page_cachelist);
OFFSET(CG_PAGE_ANON_CACHELIST, struct cglocal, page_anon_cachelist);
OFFSET(CG_PAGE_DIRTYFLIST, struct cglocal, page_dirtyflist);
OFFSET(CG_PAGE_DIRTYALIST, struct cglocal, page_dirtyalist);
OFFSET(CG_PAGE_FREELIST_SIZE, struct cglocal, page_freelist_size);
OFFSET(CG_PAGE_CACHELIST_SIZE, struct cglocal, page_cachelist_size);
OFFSET(CG_PAGE_ANON_CACHELIST_SIZE, struct cglocal, page_anon_cachelist_size);
#ifdef DEBUG
OFFSET(CG_PAGE_DIRTYFLIST_SIZE, struct cglocal, page_dirtyflist_size);
OFFSET(CG_PAGE_DIRTYALIST_SIZE, struct cglocal, page_dirtyalist_size);
#endif
OFFSET(CG_PAGE_DIRTYLISTS_SIZE, struct cglocal, page_dirtylists_size);
OFFSET(CG_PSLICE_TABLE, struct cglocal, pslice_table);
OFFSET(CG_PAGES, struct cglocal, pages);
OFFSET(CG_EPAGES, struct cglocal, epages);
OFFSET(CG_PAGE_CS_TABLE, struct cglocal, page_cs_table);
#ifdef DEBUG
OFFSET(CG_PAGE_CS_TABLE_SIZE, struct cglocal, page_cs_table_size);
#endif
OFFSET(CG_PAGE_SWAPRECLAIM_NEXTPP, struct cglocal, page_swapreclaim_nextpp);
OFFSET(CG_PAGE_SWAPRECLAIM_LCK, struct cglocal, page_swapreclaim_lck);
#ifdef DEBUG
OFFSET(CG_PAGE_SWAPRECLAIM_RECLAIMS, struct cglocal, page_swapreclaim_reclaims);
OFFSET(CG_PAGE_SWAPRECLAIM_DESPERATE, struct cglocal, page_swapreclaim_desperate);
#endif
OFFSET(CG_ANONINFO, struct cglocal, anoninfo);
OFFSET(CG_ANON_ALLOCATED, struct cglocal, anon_allocated);
OFFSET(CG_ANON_FREE_LCK, struct cglocal, anon_free_lck);
OFFSET(CG_ANON_TABGROW, struct cglocal, anon_tabgrow);
OFFSET(CG_ANON_MAX, struct cglocal, anon_max);
OFFSET(CG_ANON_CLMAX, struct cglocal, anon_clmax);
OFFSET(CG_ANON_CLCUR, struct cglocal, anon_clcur);
OFFSET(CG_ANON_CLFAIL, struct cglocal, anon_clfail);
OFFSET(CG_ANON_FREE_LISTP, struct cglocal, anon_free_listp);
OFFSET(CG_VM_MEMLOCK, struct cglocal, vm_memlock);
OFFSET(CG_RMEMINFO, struct cglocal, rmeminfo);

OFFSET(CG_BLIST, struct cglocal, blist);
OFFSET(CG_KM_GROW_START, struct cglocal, km_grow_start);
OFFSET(CG_KMLISTP, struct cglocal, kmlistp);
OFFSET(CG_KMLOCAL_SPECLIST, struct cglocal, kmlocal_speclist);
OFFSET(CG_KMA_GIVEBACK_LOCK, struct cglocal, kma_giveback_lock);
OFFSET(CG_KMA_LASTGIVE, struct cglocal, kma_lastgive);
OFFSET(CG_KMA_GIVEBACK_ACTION, struct cglocal, kma_giveback_action);
OFFSET(CG_KMA_GIVEBACK_SV, struct cglocal, kma_giveback_sv);
OFFSET(CG_KMA_GIVEBACK_SEQ, struct cglocal, kma_giveback_seq);
OFFSET(CG_KMA_COMPLETION_SV, struct cglocal, kma_completion_sv);
OFFSET(CG_KMA_SPAWN_ERROR, struct cglocal, kma_spawn_error);
OFFSET(CG_KMA_NEW_ENG, struct cglocal, kma_new_eng);
OFFSET(CG_KMA_LIVE_DAEMONS, struct cglocal, kma_live_daemons);
OFFSET(CG_KMA_TARGETS, struct cglocal, kma_targets);
#ifdef DEBUG
OFFSET(CG_KMA_SHRINKCOUNT, struct cglocal, kma_shrinkcount);
#endif
OFFSET(CG_KMA_SHRINKTIME, struct cglocal, kma_shrinktime);
OFFSET(CG_KMA_LOCK, struct cglocal, kma_lock);
OFFSET(CG_KMA_RFF, struct cglocal, kma_rff);
OFFSET(CG_KMA_PAGEOUT_POOL, struct cglocal, kma_pageout_pool);

OFFSET(CG_PVN_PAGEOUT_MARKERS, struct cglocal, pvn_pageout_markers);

OFFSET(CG_LWP_PAGEOUT, struct cglocal, lwp_pageout);
OFFSET(CG_PUSHES, struct cglocal, pushes);
OFFSET(CG_MEM_LOTSFREE, struct cglocal, mem_lotsfree);
OFFSET(CG_MEM_MINFREE, struct cglocal, mem_minfree);
OFFSET(CG_MEM_DESFREE, struct cglocal, mem_desfree);
OFFSET(CG_PAGEOUT_EVENT, struct cglocal, pageout_event);
OFFSET(CG_LWP_FSFLUSH_PAGESYNC, struct cglocal, lwp_fsflush_pagesync);
OFFSET(CG_GLOBAL_DT_INFO, struct cglocal, global_dt_info);
OFFSET(CG_STD_IDT_DESC, struct cglocal, std_idt_desc);
OFFSET(CG_GLOBAL_LDT, struct cglocal, global_ldt);
OFFSET(CG_SPAWN_SYS_LWP_SPAWNEVENT, struct cglocal, spawn_sys_lwp_spawnevent);
OFFSET(CG_SPAWN_SYS_LWP_WAITEVENT, struct cglocal, spawn_sys_lwp_waitevent);
OFFSET(CG_SPAWN_MUTEX, struct cglocal, spawn_mutex);
OFFSET(CG_SPAWN_SYS_LWP_ACTION, struct cglocal, spawn_sys_lwp_action);
OFFSET(CG_SPAWN_SYS_LWP_FLAGS, struct cglocal, spawn_sys_lwp_flags);
OFFSET(CG_SPAWN_SYS_LWP_ARGP, struct cglocal, spawn_sys_lwp_argp);
OFFSET(CG_SPAWN_SYS_LWP_LWPID, struct cglocal, spawn_sys_lwp_lwpid);
OFFSET(CG_SPAWN_SYS_LWP_RETURN, struct cglocal, spawn_sys_lwp_return);
OFFSET(CG_PTPOOL_PL, struct cglocal, ptpool_pl);
OFFSET(CG_MCPOOL_PL, struct cglocal, mcpool_pl);
OFFSET(CG_TLBSOLDPL, struct cglocal, TLBSoldpl);
OFFSET(CG_POOLREFRESH_EVENT, struct cglocal, poolrefresh_event);
OFFSET(CG_POOLREFRESH_LASTTIME, struct cglocal, poolrefresh_lasttime);
OFFSET(CG_POOLREFRESH_PENDING, struct cglocal, poolrefresh_pending);
OFFSET(CG_FSPSYNC_EVENT, struct cglocal, fspsync_event);
OFFSET(CG_FSF_TOSCAN, struct cglocal, fsf_toscan);
OFFSET(CG_FSF_NEXT_PAGE, struct cglocal, fsf_next_page);
OFFSET(CG_FSF_AGE_TIME, struct cglocal, fsf_age_time);
OFFSET(CG_NIDLE, struct cglocal, cg_nidle);
OFFSET(CG_AVGQLEN, struct cglocal, cg_avgqlen);
OFFSET(CG_STATIC_COFREE, struct cglocal, cg_scofree);
OFFSET(CG_DYNAMIC_COFREE, struct cglocal, cg_dcofree);
OFFSET(CG_NCOFREE, struct cglocal, cg_ncofree);
OFFSET(CG_STATIC_NCOFREE, struct cglocal, cg_sncofree);
OFFSET(CG_DYNAMIC_NCOFREE, struct cglocal, cg_dncofree);
OFFSET(CG_CO_NACTIVE, struct cglocal, cg_conactive);
OFFSET(CG_CO_NDEAD, struct cglocal, cg_condead);
OFFSET(CG_COID_HASH, struct cglocal, cg_coid_hash);
OFFSET(CG_TIMEID, struct cglocal, cg_timeid);
OFFSET(CG_TIME_MUTEX, struct cglocal, cg_time_mutex);
OFFSET(CG_MUTEX, struct cglocal, cg_mutex);
OFFSET(CG_RESCPU, struct cglocal, cg_rescpu);
OFFSET(CG_RESRQ, struct cglocal, cg_resrq);
OFFSET(CG_TOTALQLEN, struct cglocal, cg_totalqlen);
OFFSET(CG_MEM_AVAIL_STATE, struct cglocal, cg_mem_avail_state);

/*
 * Misc defs for P5 bugs (intr.s)
 */
DEFINE(PF_ERR_PROT, PF_ERR_PROT);
DEFINE(PF_ERR_WRITE, PF_ERR_WRITE);
DEFINE(PF_ERR_USER, PF_ERR_USER);

