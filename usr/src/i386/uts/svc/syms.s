	.ident	"@(#)kern-i386:svc/syms.s	1.12.9.1"
	.ident	"$Header$"

include(KBASE/svc/asm.m4)
include(assym_include)

	.text
	.globl	stext
	.align	0x1000
	.set	stext, .

	.data
	.globl	sdata
	.align	0x1000
	.set	sdata, .

	.bss
	.globl	sbss
	.align	0x1000
	.set	sbss, .


// The following special symbols are used for interface conformance checking.
// Specially defined symbols get mapped to __OK when the right interface is
// used. The symbol _DDI_C_Defined is used to make sure only DDI providers,
// not consumers, set _DDI_C. This is accomplished by forcing use of
// "$interface base" when _DDI_C is defined.
	SYM_DEF(__OK, 1)
	SYM_DEF(_DDI_C_Defined, 1)


	SYM_DEF(l, _A_KVPLOCAL)
	SYM_DEF(cg, _A_KVCGLOCAL)
	SYM_DEF(mycgnum, _A_KVCGLOCAL+_A_CG_CG_NUM)
	SYM_DEF(os_this_cgnum, _A_KVCGLOCAL+_A_CG_CG_NUM)
	SYM_DEF(lm, _A_KVPLOCALMET)
	SYM_DEF(m, _A_KVMET)
	SYM_DEF(upointer, _A_KVPLOCAL+_A_L_USERP)
	SYM_DEF(uprocp, _A_KVPLOCAL+_A_L_CURPROCP)
	SYM_DEF(ueng, _A_KVUENG+_A_UAREA_OFFSET)
	SYM_DEF(uvwin, _A_KVUVWIN)
	SYM_DEF(plocal_intr_depth, _A_KVPLOCAL+_A_L_INTR_DEPTH)
	SYM_DEF(prmpt_state, _A_KVPLOCAL+_A_L_PRMPT_STATE)
	SYM_DEF(engine_evtflags, _A_KVPLOCAL+_A_L_EVENTFLAGS)
	SYM_DEF(using_fpu, _A_KVPLOCAL+_A_L_USINGFPU)
	SYM_DEF(myengnum, _A_KVPLOCAL+_A_L_ENG_NUM)
	SYM_DEF(cur_idtp, _A_KVPLOCAL+_A_L_IDTP)
	SYM_DEF(trap_err_code, _A_KVPLOCAL+_A_L_TRAP_ERR_CODE)
	SYM_DEF(mycpuid, _A_KVPLOCAL+_A_L_CPU_ID)
ifdef(`MERGE386', `
	SYM_DEF(vm86_idtp, _A_KVPLOCAL+_A_L_VM86_IDTP)
')

ifdef(`DEBUG', `
ifdef(`_LOCKTEST', `
	SYM_DEF(KVPLOCAL_L_HOLDFASTLOCK, _A_KVPLOCAL+_A_L_HOLDFASTLOCK)
	SYM_DEF(KVPLOCAL_L_FSPIN, _A_KVPLOCAL+_A_L_FSPIN)
')
	SYM_DEF(IS_DEBUG_BASE, 1)	/ indicate to the debugger that the
					/ base-kernel was compiled with DEBUG
')

SYM_DEF(cg_nonline, _A_KVCGLOCAL+_A_CG_NONLINE)
SYM_DEF(cg_num, _A_KVCGLOCAL+_A_CG_CG_NUM)
SYM_DEF(cg_rq, _A_KVCGLOCAL+_A_CG_CG_RQ)
SYM_DEF(cg_todo, _A_KVCGLOCAL+_A_CG_GLOBAL_TODO)
SYM_DEF(proc_sys, _A_KVCGLOCAL+_A_CG_CG_P0)
SYM_DEF(sys_cred, _A_KVCGLOCAL+_A_CG_CG_CRED)
SYM_DEF(sys_rlimits, _A_KVCGLOCAL+_A_CG_CG_RLIMIT)
SYM_DEF(vm_pagefreelockpl, _A_KVCGLOCAL+_A_CG_VM_PAGEFREELOCKPL)
SYM_DEF(mem_freemem, _A_KVCGLOCAL+_A_CG_MEM_FREEMEM)
SYM_DEF(maxfreemem, _A_KVCGLOCAL+_A_CG_MAXFREEMEM)
SYM_DEF(page_freelist, _A_KVCGLOCAL+_A_CG_PAGE_FREELIST)
SYM_DEF(page_cachelist, _A_KVCGLOCAL+_A_CG_PAGE_CACHELIST)
SYM_DEF(page_anon_cachelist, _A_KVCGLOCAL+_A_CG_PAGE_ANON_CACHELIST)	
SYM_DEF(page_dirtyflist, _A_KVCGLOCAL+_A_CG_PAGE_DIRTYFLIST)
SYM_DEF(page_dirtyalist, _A_KVCGLOCAL+_A_CG_PAGE_DIRTYALIST)
SYM_DEF(page_freelist_size, _A_KVCGLOCAL+_A_CG_PAGE_FREELIST_SIZE)
SYM_DEF(page_cachelist_size, _A_KVCGLOCAL+_A_CG_PAGE_CACHELIST_SIZE)
SYM_DEF(page_anon_cachelist_size, _A_KVCGLOCAL+_A_CG_PAGE_ANON_CACHELIST_SIZE)
ifdef(`DEBUG', `
SYM_DEF(page_dirtyflist_size, _A_KVCGLOCAL+_A_CG_PAGE_DIRTYFLIST_SIZE)
SYM_DEF(page_dirtyalist_size, _A_KVCGLOCAL+_A_CG_PAGE_DIRTYALIST_SIZE)
SYM_DEF(page_cs_table_size, _A_KVCGLOCAL+_A_CG_PAGE_CS_TABLE_SIZE)
')	
SYM_DEF(page_dirtylists_size, _A_KVCGLOCAL+_A_CG_PAGE_DIRTYLISTS_SIZE)
SYM_DEF(pslice_table, _A_KVCGLOCAL+_A_CG_PSLICE_TABLE);
SYM_DEF(pages, _A_KVCGLOCAL+_A_CG_PAGES)
SYM_DEF(epages, _A_KVCGLOCAL+_A_CG_EPAGES)
SYM_DEF(page_cs_table, _A_KVCGLOCAL+_A_CG_PAGE_CS_TABLE)
SYM_DEF(page_swapreclaim_nextpp, _A_KVCGLOCAL+_A_CG_PAGE_SWAPRECLAIM_NEXTPP)
SYM_DEF(page_swapreclaim_lck, _A_KVCGLOCAL+_A_CG_PAGE_SWAPRECLAIM_LCK)
SYM_DEF(anoninfo, _A_KVCGLOCAL+_A_CG_ANONINFO)
SYM_DEF(anon_allocated, _A_KVCGLOCAL+_A_CG_ANON_ALLOCATED)
SYM_DEF(anon_free_lck, _A_KVCGLOCAL+_A_CG_ANON_FREE_LCK)
SYM_DEF(anon_tabgrow, _A_KVCGLOCAL+_A_CG_ANON_TABGROW)
SYM_DEF(anon_max, _A_KVCGLOCAL+_A_CG_ANON_MAX)
SYM_DEF(anon_clmax, _A_KVCGLOCAL+_A_CG_ANON_CLMAX)
SYM_DEF(anon_clcur, _A_KVCGLOCAL+_A_CG_ANON_CLCUR)
SYM_DEF(anon_clfail, _A_KVCGLOCAL+_A_CG_ANON_CLFAIL)	
SYM_DEF(anon_free_listp, _A_KVCGLOCAL+_A_CG_ANON_FREE_LISTP)
SYM_DEF(vm_memlock, _A_KVCGLOCAL+_A_CG_VM_MEMLOCK)	
SYM_DEF(rmeminfo, _A_KVCGLOCAL+_A_CG_RMEMINFO)
SYM_DEF(blist, _A_KVCGLOCAL+_A_CG_BLIST)
SYM_DEF(km_grow_start, _A_KVCGLOCAL+_A_CG_KM_GROW_START)
SYM_DEF(kmlistp, _A_KVCGLOCAL+_A_CG_KMLISTP)
SYM_DEF(kmlocal_speclist, _A_KVCGLOCAL+_A_CG_KMLOCAL_SPECLIST)
SYM_DEF(kma_giveback_lock, _A_KVCGLOCAL+_A_CG_KMA_GIVEBACK_LOCK)
SYM_DEF(kma_lastgive, _A_KVCGLOCAL+_A_CG_KMA_LASTGIVE)
SYM_DEF(kma_giveback_action, _A_KVCGLOCAL+_A_CG_KMA_GIVEBACK_ACTION)
SYM_DEF(kma_giveback_sv, _A_KVCGLOCAL+_A_CG_KMA_GIVEBACK_SV)
SYM_DEF(kma_giveback_seq, _A_KVCGLOCAL+_A_CG_KMA_GIVEBACK_SEQ)
SYM_DEF(kma_completion_sv, _A_KVCGLOCAL+_A_CG_KMA_COMPLETION_SV)
SYM_DEF(kma_spawn_error, _A_KVCGLOCAL+_A_CG_KMA_SPAWN_ERROR)
SYM_DEF(kma_new_eng, _A_KVCGLOCAL+_A_CG_KMA_NEW_ENG)
SYM_DEF(kma_live_daemons, _A_KVCGLOCAL+_A_CG_KMA_LIVE_DAEMONS)
SYM_DEF(kma_targets, _A_KVCGLOCAL+_A_CG_KMA_TARGETS)
SYM_DEF(kma_shrinktime, _A_KVCGLOCAL+_A_CG_KMA_SHRINKTIME)
SYM_DEF(kma_lock, _A_KVCGLOCAL+_A_CG_KMA_LOCK)
SYM_DEF(kma_rff, _A_KVCGLOCAL+_A_CG_KMA_RFF)
SYM_DEF(kma_pageout_pool, _A_KVCGLOCAL+_A_CG_KMA_PAGEOUT_POOL)
ifdef(`DEBUG', `
SYM_DEF(kma_shrinkcount, _A_KVCGLOCAL+_A_CG_KMA_SHRINKCOUNT)
')		

SYM_DEF(pvn_pageout_markers, _A_KVCGLOCAL+_A_CG_PVN_PAGEOUT_MARKERS)
	
SYM_DEF(lwp_pageout, _A_KVCGLOCAL+_A_CG_LWP_PAGEOUT)
SYM_DEF(pushes,	_A_KVCGLOCAL+_A_CG_PUSHES)
SYM_DEF(mem_lotsfree, _A_KVCGLOCAL+_A_CG_MEM_LOTSFREE)
SYM_DEF(mem_minfree, _A_KVCGLOCAL+_A_CG_MEM_MINFREE)
SYM_DEF(mem_desfree, _A_KVCGLOCAL+_A_CG_MEM_DESFREE)
SYM_DEF(pageout_event, _A_KVCGLOCAL+_A_CG_PAGEOUT_EVENT)
	
SYM_DEF(lwp_fsflush_pagesync, _A_KVCGLOCAL+_A_CG_LWP_FSFLUSH_PAGESYNC)

SYM_DEF(myglobal_dt_info, _A_KVCGLOCAL+_A_CG_GLOBAL_DT_INFO)
SYM_DEF(mystd_idt_desc, _A_KVCGLOCAL+_A_CG_STD_IDT_DESC)
SYM_DEF(myglobal_ldt, _A_KVCGLOCAL+_A_CG_GLOBAL_LDT)
SYM_DEF(spawn_sys_lwp_spawnevent, _A_KVCGLOCAL+_A_CG_SPAWN_SYS_LWP_SPAWNEVENT)
SYM_DEF(spawn_sys_lwp_waitevent, _A_KVCGLOCAL+_A_CG_SPAWN_SYS_LWP_WAITEVENT)
SYM_DEF(spawn_mutex, _A_KVCGLOCAL+_A_CG_SPAWN_MUTEX)
SYM_DEF(spawn_sys_lwp_action, _A_KVCGLOCAL+_A_CG_SPAWN_SYS_LWP_ACTION)
SYM_DEF(spawn_sys_lwp_flags, _A_KVCGLOCAL+_A_CG_SPAWN_SYS_LWP_FLAGS)
SYM_DEF(spawn_sys_lwp_argp, _A_KVCGLOCAL+_A_CG_SPAWN_SYS_LWP_ARGP)
SYM_DEF(spawn_sys_lwp_lwpid, _A_KVCGLOCAL+_A_CG_SPAWN_SYS_LWP_LWPID)
SYM_DEF(spawn_sys_lwp_return, _A_KVCGLOCAL+_A_CG_SPAWN_SYS_LWP_RETURN)
SYM_DEF(ptpool_pl, _A_KVCGLOCAL+_A_CG_PTPOOL_PL)
SYM_DEF(mcpool_pl, _A_KVCGLOCAL+_A_CG_MCPOOL_PL)
SYM_DEF(TLBSoldpl, _A_KVCGLOCAL+_A_CG_TLBSOLDPL)
SYM_DEF(poolrefresh_event, _A_KVCGLOCAL+_A_CG_POOLREFRESH_EVENT)
SYM_DEF(poolrefresh_lasttime, _A_KVCGLOCAL+_A_CG_POOLREFRESH_LASTTIME)
SYM_DEF(poolrefresh_pending, _A_KVCGLOCAL+_A_CG_POOLREFRESH_PENDING)
SYM_DEF(fspsync_event, _A_KVCGLOCAL+_A_CG_FSPSYNC_EVENT)
SYM_DEF(fsf_toscan, _A_KVCGLOCAL+_A_CG_FSF_TOSCAN)
SYM_DEF(fsf_next_page, _A_KVCGLOCAL+_A_CG_FSF_NEXT_PAGE)
SYM_DEF(fsf_age_time, _A_KVCGLOCAL+_A_CG_FSF_AGE_TIME)
SYM_DEF(cg_nidle, _A_KVCGLOCAL+_A_CG_NIDLE)
SYM_DEF(cg_avgqlen, _A_KVCGLOCAL+_A_CG_AVGQLEN)
SYM_DEF(cg_scofree, _A_KVCGLOCAL+_A_CG_STATIC_COFREE)
SYM_DEF(cg_dcofree, _A_KVCGLOCAL+_A_CG_DYNAMIC_COFREE)
SYM_DEF(cg_ncofree, _A_KVCGLOCAL+_A_CG_NCOFREE)
SYM_DEF(cg_sncofree, _A_KVCGLOCAL+_A_CG_STATIC_NCOFREE)
SYM_DEF(cg_dncofree, _A_KVCGLOCAL+_A_CG_DYNAMIC_NCOFREE)
SYM_DEF(cg_conactive, _A_KVCGLOCAL+_A_CG_CO_NACTIVE)
SYM_DEF(cg_condead, _A_KVCGLOCAL+_A_CG_CO_NDEAD)
SYM_DEF(cg_coid_hash, _A_KVCGLOCAL+_A_CG_COID_HASH)
SYM_DEF(cg_timeid, _A_KVCGLOCAL+_A_CG_TIMEID)
SYM_DEF(cg_time_mutex, _A_KVCGLOCAL+_A_CG_TIME_MUTEX)
SYM_DEF(cg_mutex, _A_KVCGLOCAL+_A_CG_MUTEX)
SYM_DEF(cg_rescpu, _A_KVCGLOCAL+_A_CG_RESCPU)
SYM_DEF(cg_resrq, _A_KVCGLOCAL+_A_CG_RESRQ)
SYM_DEF(cg_totalqlen, _A_KVCGLOCAL+_A_CG_TOTALQLEN)
SYM_DEF(cg_mem_avail_state, _A_KVCGLOCAL+_A_CG_MEM_AVAIL_STATE)
ifdef(`DEBUG',`
SYM_DEF(page_swapreclaim_reclaims, _A_KVCGLOCAL+_A_CG_PAGE_SWAPRECLAIM_RECLAIMS)
SYM_DEF(page_swapreclaim_desperate, _A_KVCGLOCAL+_A_CG_PAGE_SWAPRECLAIM_DESPERATE)
')	
