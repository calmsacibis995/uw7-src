#ident	"@(#)metreg:mettbl.h	1.6.1.1"

#define PT_SYS_STRUCT	0
#define PT_PROC_STRUCT	1    /* implies by_processor metrics */
#define PT_DISK_STRUCT	2    /* implies by_disk metrics */

#define PT_NATIVE	MAS_NATIVE
#define PT_SYSTEM	MAS_SYSTEM
#define PT_BY_PROC	0x2
#define PT_BY_FS	0x4
#define PT_BY_MEMSIZE	0x8
#define PT_BY_DISK	0x10


typedef struct {
	metid_t	id;
	name_t 	name;
	units_t	units;
	name_t	unitsnm;
	type_t  mettype;
	uint32	obj_sz;
	uint32	nobj;	
	uint32	obj_off;  /* offset from start of containing structure */
	uint32	repflags;  /* field is actually an array per fs, disk, etc */
} finfo_t;


typedef struct {
	int	container; /* which structure this substructure is in */
	uint32	size;
	uint32	offset;
	uint32	repflags;
	uint32	nfields;
	finfo_t	*fields;
} metinfo_t;

/* 
 * for address computations in macros.
 *
 */
 

struct mets dummy_met;
struct plocalmet dummy_lm;
struct met_disk_stats dummy_ds;



#define GENERIC_FIELD(id, name, units, unitsnm, mettype, container, field, rep, bigstruct) \
{ id, name, units, unitsnm, mettype, sizeof(bigstruct.container.field), 1,\
  (caddr_t) &(bigstruct.container.field) - (caddr_t) &(bigstruct.container), \
  rep }

#define SYS_FIELD(id, name, units, unitsnm, mettype, container, field, rep) \
GENERIC_FIELD(id, name, units, unitsnm, mettype, container, field, rep, dummy_met)

#define PROC_FIELD(id, name, units, unitsnm, mettype, container, field, rep) \
GENERIC_FIELD(id, name, units, unitsnm, mettype, container, field, rep, dummy_lm)

#define DISK_FIELD(id, name, units, unitsnm, mettype, field, rep) \
{ id, name, units, unitsnm, mettype, sizeof(dummy_ds.field), 1, \
  (caddr_t) &(dummy_ds.field) - (caddr_t) &(dummy_ds), rep }


#define SYSMET_INFO(group)\
{ PT_SYS_STRUCT, \
  sizeof(dummy_met.group), \
  (caddr_t) &(dummy_met.group) - (caddr_t) &(dummy_met), \
  0, \
  sizeof(group##_info)/sizeof(*group##_info), \
  group##_info \
}


#define SYSMET_INFO_ARR(group, repflags) \
{ PT_SYS_STRUCT, \
  sizeof(dummy_met.group[0]), \
  (caddr_t) &(dummy_met.group[0]) - (caddr_t) &(dummy_met), \
  repflags, \
  sizeof(group##_info)/sizeof(*group##_info), \
  group##_info \
}


#define PROCMET_INFO(group) \
{ PT_PROC_STRUCT, \
  sizeof(dummy_lm.group), \
  (caddr_t) &(dummy_lm.group) - (caddr_t) &(dummy_lm), \
  0, \
  sizeof(group##_info)/sizeof(*group##_info), \
  group##_info \
}


#define PROCMET_INFO_ARR(group, repflags) \
{ PT_PROC_STRUCT, \
  sizeof(dummy_lm.group[0]), \
  (caddr_t) &(dummy_lm.group[0]) - (caddr_t) &(dummy_lm), \
  repflags, \
  sizeof(group##_info)/sizeof(*group##_info), \
  group##_info \
}


finfo_t mets_native_units_info[] = 
{
	SYS_FIELD(HZ_TIX, "hz", TIX, "ticks", CONSTANT, mets_native_units, mnu_hz, PT_NATIVE),
	SYS_FIELD(PGSZ, "pgsz", BYTES, "bytes", CONSTANT, mets_native_units, mnu_pagesz, 
		  PT_NATIVE)
};

finfo_t mets_counts_info[] = 
{
        SYS_FIELD(NDISK, "ndisk", DISKS, "disks", CONFIGURABLE, mets_counts, mc_ndisks, PT_SYSTEM),
	SYS_FIELD(NCPU, "ncpu", CPUS, "cpus", CONFIGURABLE, mets_counts, mc_nengines, PT_SYSTEM),
	SYS_FIELD(NCG, "ncg", CGS, "cgs", CONFIGURABLE, mets_counts, mc_ncgs, PT_SYSTEM)
};

finfo_t mets_wait_info[] = 
{
	SYS_FIELD(FSWIO, "fswio", JOBS, "jobs", COUNT, mets_wait, msw_iowait, PT_SYSTEM),
	SYS_FIELD(PHYSWIO, "physwio", JOBS, "jobs", COUNT, mets_wait, msw_physio, PT_SYSTEM)
};

finfo_t mets_sched_info[] = 
{
	SYS_FIELD(RUNQUE, "runque", RUNQ_SECS, "runque-secs", SUM_OVER_TIME, mets_sched, mss_runque, 
		  PT_SYSTEM),
	SYS_FIELD(RUNOCC, "runocc", SECS, "secs", PROFILE, mets_sched, mss_runocc,
		  PT_SYSTEM),
	SYS_FIELD(SWPQUE, "swpque", SWPQ_SECS, "swapque-secs", SUM_OVER_TIME, mets_sched, mss_swpque, 
		  PT_SYSTEM),
	SYS_FIELD(SWPOCC, "swpocc", SECS, "secs", PROFILE, mets_sched, mss_swpocc, 
		  PT_SYSTEM)
};


finfo_t mets_proc_resrc_info[] =
{
	SYS_FIELD(PROCFAIL, "procfail", PROCESSES, "processes", SUM, mets_proc_resrc, 
		   msr_proc[MET_FAIL], PT_SYSTEM),
	SYS_FIELD(PROCUSE, "procuse", PROCESSES, "processes", COUNT, mets_proc_resrc, 
		   msr_proc[MET_INUSE], PT_SYSTEM),
	SYS_FIELD(PROCMAX, "procmax", PROCESSES, "processes", CONFIGURABLE, mets_proc_resrc, 
		   msr_proc[MET_MAX], PT_SYSTEM)
};

finfo_t mets_fstypes_info[] = 
{
	SYS_FIELD(NFSTYP, "nfstypes", FILE_SYSTEMS, "file systems", CONFIGURABLE, mets_fstypes, 
		  msf_numtypes, PT_NATIVE),

/*
	SYS_FIELD(FSNAMES, "fsnames", TEXT, "", DESCRIPTIVE, mets_fstypes, msf_names[0], 
		  PT_BY_FS)
*/
	{ FSNAMES, "fsnames", TEXT, "", DESCRIPTIVE, sizeof(dummy_met.mets_fstypes.msf_names[0][0]),
	  sizeof(dummy_met.mets_fstypes.msf_names[0]), 
	  (caddr_t) &(dummy_met.mets_fstypes.msf_names) - (caddr_t) &(dummy_met.mets_fstypes),
	  PT_BY_FS }
};

finfo_t mets_files_info[] = 
{
	SYS_FIELD(FILETBLINUSE, "filetblinuse", FILE_TBLS, "file tables", COUNT, mets_files, 
		  msf_file[MET_INUSE], 0),
	SYS_FIELD(FILETBLFAIL, "filetblfail", FILE_TBLS, "file tables", SUM, mets_files, 
		  msf_file[MET_FAIL], 0),
	SYS_FIELD(FLCKTBLMAX, "flcktblmax", FLOCK_TBLS, "flock tables", CONFIGURABLE, mets_files, 
		  msf_flck[MET_MAX], 0),
	SYS_FIELD(FLCKTBLINUSE, "flcktblinuse", FLOCK_TBLS, "flock tables", COUNT, mets_files, 
		  msf_flck[MET_INUSE], 0),
	SYS_FIELD(FLCKTBLFAIL, "flcktblfail", FLOCK_TBLS, "flock tables", SUM, mets_files, 
		  msf_flck[MET_FAIL], 0),
	SYS_FIELD(FLCKTBLTOTAL, "flcktbltotal", FLOCK_TBLS, "flock tables", SUM, mets_files, 
		  msf_flck[MET_TOTAL], 0)
};

finfo_t mets_inodes_info[] = 
{
	SYS_FIELD(MAXINODE, "maxinode", INODES, "inodes", CONFIGURABLE, mets_inodes[0], 
		  msi_inodes[MET_MAX], 0),
	SYS_FIELD(CURRINODE, "currinode", INODES, "inodes", COUNT, mets_inodes[0], 
		  msi_inodes[MET_CURRENT], 0),
	SYS_FIELD(INUSEINODE, "inuseinode", INODES, "inodes", COUNT, mets_inodes[0],
		  msi_inodes[MET_INUSE], 0),
	SYS_FIELD(FAILINODE, "failinode", INODES, "inodes", SUM, mets_inodes[0],
		  msi_inodes[MET_FAIL], 0)
};

finfo_t mets_mem_info[] = 
{
	SYS_FIELD(FREEMEM, "freemem", PAGE_SECS, "page-secs", SUM_OVER_TIME, mets_mem, 
		  msm_freemem, PT_SYSTEM),
	SYS_FIELD(FREESWAP, "freeswap", PAGE_SECS, "page-secs", SUM_OVER_TIME, mets_mem, 
		  msm_freeswap, PT_SYSTEM),
};

finfo_t mets_kmemsizes_info[] = 
{
	SYS_FIELD(KMPOOLS, "kmapools", POOLS, "pools", CONFIGURABLE, mets_kmemsizes, 
		  msk_numpools, PT_NATIVE),
	SYS_FIELD(KMASIZE, "kmasize", BYTES, "bytes", CONFIGURABLE, mets_kmemsizes,
		  msk_sizes[0], PT_BY_MEMSIZE)
};

#ifdef PERF
finfo_t mets_fsinfo_info[] = 
{
	SYS_FIELD(FSGETPG, "fsgetpg", CALLS, "calls", SUM, mets_fsinfo[0], getpage, 0),
	SYS_FIELD(FSPGIN, "fspgin", CALLS, "calls", SUM, mets_fsinfo[0], pgin, 0),
	SYS_FIELD(FSPGPGIN, "fspgpgin", PAGES, "pages", SUM, mets_fsinfo[0], pgpgin, 0),
	SYS_FIELD(FSSECTPGIN, "fssectpgin", SECTORS, "sectors", SUM, mets_fsinfo[0], 
		  sectin, 0),
	SYS_FIELD(FSRDA, "fsrda", CALLS, "calls", SUM, mets_fsinfo[0], ra, 0),
	SYS_FIELD(FSRDAPGIN, "fsrdapgin", PAGES, "pages", SUM, mets_fsinfo[0], rapgpgin, 0),
	SYS_FIELD(FSRDASECTIN, "fsrdasectin", SECTORS, "sectors", SUM, mets_fsinfo[0], 
		  rasectin, 0),
	SYS_FIELD(FSPUTPG, "fsputpg", CALLS, "calls", SUM, mets_fsinfo[0], putpage, 0),
	SYS_FIELD(FSPGOUT, "fspgout", CALLS, "calls", SUM, mets_fsinfo[0], pgout, 0),
	SYS_FIELD(FSPGPGOUT, "fspgpgout", PAGES, "pages", SUM, mets_fsinfo[0], pgpgout, 0),
	SYS_FIELD(FSSECTOUT, "fssectout", SECTORS, "sectors", SUM, mets_fsinfo[0], sectout, 0)
};
#endif

	
finfo_t cnt_info[] = 
{
	PROC_FIELD(V_SWTCH, "v_swtch", CALLS, "calls", SUM, cnt, v_swtch, 0),
	PROC_FIELD(V_TRAP, "v_trap", CALLS, "calls", SUM, cnt, v_trap, 0),
	PROC_FIELD(V_SYSCALL, "v_syscall", CALLS, "calls", SUM, cnt, v_syscall, 0),
	PROC_FIELD(V_INTR, "v_intr", CALLS, "calls", SUM, cnt, v_intr, 0),
	PROC_FIELD(V_PDMA, "v_pdma", CALLS, "calls", SUM, cnt, v_pdma, 0),
	PROC_FIELD(V_PSWPIN, "v_pswpin", PAGES, "pages", SUM, cnt, v_pswpin, 0),
	PROC_FIELD(V_PSWPOUT, "v_pswpout", PAGES, "pages", SUM, cnt, v_pswpout, 0),
	PROC_FIELD(V_PGIN, "v_pgin", CALLS, "calls", SUM, cnt, v_pgin, 0),
	PROC_FIELD(V_PGOUT, "v_pgout", CALLS, "calls", SUM, cnt, v_pgout, 0),
	PROC_FIELD(V_PGPGIN, "v_pgpgin", PAGES, "pages", SUM, cnt, v_pgpgin, 0),
	PROC_FIELD(V_PGPGOUT, "v_pgpgout", PAGES, "pages", SUM, cnt, v_pgpgout, 0),
	PROC_FIELD(V_INTRANS, "v_intrans", CALLS, "calls", SUM, cnt, v_intrans, 0),
	PROC_FIELD(V_PGREC, "v_pgrec", PAGES, "pages", SUM, cnt, v_pgrec, 0),	
	PROC_FIELD(V_XSFREC, "v_xsfrec", PAGES, "pages", SUM, cnt, v_xsfrec, 0),
	PROC_FIELD(V_XIFREC, "v_xifrec", PAGES, "pages", SUM, cnt, v_xifrec, 0),
	PROC_FIELD(V_ZFOD, "v_zfod", PAGES, "pages", SUM, cnt, v_zfod, 0),
	PROC_FIELD(V_PGFREC, "v_pgfrec", PAGES, "pages", SUM, cnt, v_pgfrec, 0),
	PROC_FIELD(V_FAULTS, "v_faults", CALLS, "calls", SUM, cnt, v_faults, 0),
	PROC_FIELD(V_SCAN, "v_scan", PAGES, "pages", SUM, cnt, v_scan, 0),
	PROC_FIELD(V_REV, "v_rev", CALLS, "calls", SUM, cnt, v_rev, 0),
	PROC_FIELD(V_DFREE, "v_dfree", PAGES, "pages", SUM, cnt, v_dfree, 0),
	PROC_FIELD(V_FASTPGREC, "v_fastpgrec", CALLS, "calls", SUM, cnt, v_fastpgrec, 0),
	PROC_FIELD(V_SWPIN, "v_swpin", CALLS, "calls", SUM, cnt, v_swpin, 0),
	PROC_FIELD(V_SWPOUT, "v_swpout", CALLS, "calls", SUM, cnt, v_swpout, 0)
};

finfo_t metp_cpu_info[] = 
{
	PROC_FIELD(MPC_CPU_IDLE, "mpc_cpu[0]", TIX, "ticks", PROFILE, metp_cpu, 
		   mpc_cpu[MET_CPU_IDLE], 0),
	PROC_FIELD(MPC_CPU_WIO, "mpc_cpu[1]", TIX, "ticks", PROFILE, metp_cpu, 
		   mpc_cpu[MET_CPU_WAIT], 0),
	PROC_FIELD(MPC_CPU_USR, "mpc_cpu[2]", TIX, "ticks", PROFILE, metp_cpu, 
		   mpc_cpu[MET_CPU_USER], 0),
	PROC_FIELD(MPC_CPU_SYS, "mpc_cpu[3]", TIX, "ticks", PROFILE, metp_cpu, 
		   mpc_cpu[MET_CPU_SYS], 0)
};

finfo_t metp_cg_info[] = 
{
	PROC_FIELD(MPG_CGID, "mpg_cgid[0]", CGID, "cgid", CONFIGURABLE, metp_cg, 
		   mpg_cgid, 0),
};

finfo_t metp_sched_info[] = 
{
	PROC_FIELD(MPS_PSWITCH, "mps_pswitch", CALLS, "calls", SUM, metp_sched, 
		 mps_pswitch, 0),
	PROC_FIELD(MPS_RUNQUE, "mps_runque", RUNQ_SECS, "runque-secs", SUM_OVER_TIME, metp_sched, 
		 mps_runque, 0),
	PROC_FIELD(MPS_RUNOCC, "mps_runocc", SECS, "secs", PROFILE, metp_sched, 
		 mps_runocc, 0),
};

finfo_t metp_buf_info[] = 
{
	PROC_FIELD(MPB_BREAD, "mpb_bread", CALLS, "calls", SUM, metp_buf, mpb_bread, 0),
	PROC_FIELD(MPB_BWRITE, "mpb_bwrite", CALLS, "calls", SUM, metp_buf, mpb_bwrite, 0),
	PROC_FIELD(MPB_LREAD, "mpb_lread", CALLS, "calls", SUM, metp_buf, mpb_lread, 0),
	PROC_FIELD(MPB_LWRITE, "mpb_lwrite", CALLS, "calls", SUM, metp_buf, mpb_lwrite, 0),
	PROC_FIELD(MPB_PHREAD, "mpb_phread", CALLS, "calls", SUM, metp_buf, mpb_phread, 0),
	PROC_FIELD(MPB_PHWRITE, "mpb_phwrite", CALLS, "calls", SUM, metp_buf, 
		   mpb_phwrite, 0),
};

finfo_t metp_syscall_info[] = 
{
	PROC_FIELD(MPS_SYSCALL, "mps_syscall", CALLS, "calls", SUM, metp_syscall, 
		 mps_syscall, 0),
	PROC_FIELD(MPS_FORK, "mps_fork", CALLS, "calls", SUM, metp_syscall, mps_fork, 0),
	PROC_FIELD(MPS_LWPCREATE, "mps_lwpcreate", CALLS, "calls", SUM, metp_syscall, 
		   mps_lwpcreate, 0),
	PROC_FIELD(MPS_EXEC, "mps_exec", CALLS, "calls", SUM, metp_syscall, mps_exec, 0),
	PROC_FIELD(MPS_READ, "mps_read", CALLS, "calls", SUM, metp_syscall, mps_read, 0),
	PROC_FIELD(MPS_WRITE, "mps_write", CALLS, "calls", SUM, metp_syscall, mps_write, 0),
	PROC_FIELD(MPS_READCH, "mps_readch", CALLS, "calls", SUM, metp_syscall,
		   mps_readch, 0),
	PROC_FIELD(MPS_WRITECH, "mps_writech", CALLS, "calls", SUM, metp_syscall, 
		   mps_writech, 0)
};

finfo_t metp_filelookup_info[] = 
{
	PROC_FIELD(MPF_LOOKUP, "mpf_lookup", CALLS, "calls", SUM, metp_filelookup,
		   mpf_lookup, 0),
	PROC_FIELD(MPF_DNLC_HITS, "mpf_dnlc_hits", CALLS, "calls", SUM, metp_filelookup,
		   mpf_dnlc_hits, 0),
	PROC_FIELD(MPF_DNLC_MISSES, "mpf_dnlc_misses", CALLS, "calls", SUM, metp_filelookup,
		   mpf_dnlc_misses, 0)
};

finfo_t metp_fileaccess_info[] = 
{
	PROC_FIELD(MPF_IGET, "mpf_iget", CALLS, "calls", SUM, metp_fileaccess[0],
		   mpf_iget, 0),
	PROC_FIELD(MPF_DIRBLK, "mpf_dirblk", DISK_BLOCKS, "disk blocks", SUM, metp_fileaccess[0],
		   mpf_dirblk, 0),
	PROC_FIELD(MPF_IPAGE, "mpf_ipage", INODES, "inodes", COUNT, metp_fileaccess[0], 
		   mpf_ipage, 0),
	PROC_FIELD(MPF_INOPAGE, "mpf_inopage", INODES, "inodes", COUNT, metp_fileaccess[0],
		   mpf_inopage, 0)
};

finfo_t metp_tty_info[] = 
{
	PROC_FIELD(MPT_RCVINT, "mpt_rcvint", CALLS, "calls", SUM, metp_tty, mpt_rcvint, 0),
	PROC_FIELD(MPT_XMTINT, "mpt_xmtint", CALLS, "calls", SUM, metp_tty, mpt_xmtint, 0),
	PROC_FIELD(MPT_MDMINT, "mpt_mdmint", CALLS, "calls", SUM, metp_tty, mpt_mdmint, 0),
	PROC_FIELD(MPT_RAWCH, "mpt_rawch", CALLS, "calls", SUM, metp_tty, mpt_rawch, 0),
	PROC_FIELD(MPT_CANCH, "mpt_canch", CALLS, "calls", SUM, metp_tty, mpt_canch, 0),
	PROC_FIELD(MPT_OUTCH, "mpt_outch", CALLS, "calls", SUM, metp_tty, mpt_outch, 0)
};

finfo_t metp_ipc_info[] = 
{
	PROC_FIELD(MPI_MSG, "mpi_msg", MESSAGES, "messages", SUM, metp_ipc, mpi_msg, 0),
	PROC_FIELD(MPI_SEMA, "mpi_sema", SEMAPHORES, "semaphores", SUM, metp_ipc, mpi_sema, 0),
};

finfo_t metp_vm_info[] = 
{
	PROC_FIELD(MPV_PREATCH, "mpv_preatch", PAGES, "pages", SUM, metp_vm, mpv_preatch, 0),
	PROC_FIELD(MPV_ATCH, "mpv_atch", CALLS, "calls", SUM, metp_vm, mpv_atch, 0),
	PROC_FIELD(MPV_ATCHFREE, "mpv_atchfree", CALLS, "calls", SUM, metp_vm, 
		   mpv_atchfree, 0),
	PROC_FIELD(MPV_ATCHFREE_PGOUT, "mpv_atchfree_pgout", CALLS, "calls", SUM, metp_vm, 
		   mpv_atchfree_pgout, 0),	
	PROC_FIELD(MPV_ATCHMISS, "mpv_atchmiss", CALLS, "calls", SUM, metp_vm, mpv_atchmiss, 0),
	PROC_FIELD(MPV_PGIN, "mpv_pgin", CALLS, "calls", SUM, metp_vm, mpv_pgin, 0),
	PROC_FIELD(MPV_PGPGIN, "mpv_pgpgin", PAGES, "pages", SUM, metp_vm, mpv_pgpgin, 0),
	PROC_FIELD(MPV_PGOUT, "mpv_pgout", CALLS, "calls", SUM, metp_vm, mpv_pgout, 0),
	PROC_FIELD(MPV_PGPGOUT, "mpv_pgpgout", PAGES, "pages", SUM, metp_vm, mpv_pgpgout, 0),
	PROC_FIELD(MPV_SWPOUT, "mpv_swpout", CALLS, "calls", SUM, metp_vm, mpv_swpout, 0),
	PROC_FIELD(MPV_PSWPOUT, "mpv_pswpout", PAGES, "pages", SUM, metp_vm, mpv_pswpout, 0),
	PROC_FIELD(MPV_VPSWPOUT, "mpv_vpswpout", PAGES, "pages", SUM, metp_vm, 
		   mpv_vpswpout, 0),
	PROC_FIELD(MPV_SWPIN, "mpv_swpin", CALLS, "calls", SUM, metp_vm, mpv_swpin, 0),
	PROC_FIELD(MPV_PSWPIN, "mpv_pswpin", PAGES, "pages", SUM, metp_vm, mpv_pswpin, 0),
	PROC_FIELD(MPV_VIRSCAN, "mpv_virscan", PAGES, "pages", SUM, metp_vm, mpv_virscan, 0),
	PROC_FIELD(MPV_VIRFREE, "mpv_virfree", PAGES, "pages", SUM, metp_vm, mpv_virfree, 0),
     	PROC_FIELD(MPV_PHYSFREE, "mpv_physfree", PAGES, "pages", SUM, metp_vm, 
		   mpv_physfree, 0),
     	PROC_FIELD(MPV_PFAULT, "mpv_pfault", CALLS, "calls", SUM, metp_vm, mpv_pfault, 0),
     	PROC_FIELD(MPV_VFAULT, "mpv_vfault", CALLS, "calls", SUM, metp_vm, mpv_vfault, 0),
     	PROC_FIELD(MPV_SFTLOCK, "mpv_sftlock", CALLS, "calls", SUM, metp_vm, mpv_sftlock, 0),
};

finfo_t metp_kmem_info[] = 
{
	PROC_FIELD(MPK_MEM, "mpk_mem", BYTES, "bytes", COUNT, metp_kmem[0], mpk_mem, 0),
	PROC_FIELD(MPK_BALLOC, "mpk_balloc", BYTES, "bytes", COUNT, metp_kmem[0], 
		   mpk_balloc, 0),


	PROC_FIELD(MPK_RALLOC, "mpk_ralloc", BYTES, "bytes", COUNT, metp_kmem[0], 
		   mpk_ralloc, 0),
	PROC_FIELD(MPK_FAIL, "mpk_fail", FAILURES, "failures", SUM, metp_kmem[0], 
		   mpk_fail, 0)
};

finfo_t metp_lwp_resrc_info[] =
{
	PROC_FIELD(MPR_LWP_FAIL, "mpr_lwp[0]", LWPS, "lwps", SUM, metp_lwp_resrc,
		  mpr_lwp[MET_FAIL], 0),
	PROC_FIELD(MPR_LWP_USE, "mpr_lwp[1]", LWPS, "lwps", COUNT, metp_lwp_resrc,
		  mpr_lwp[MET_INUSE], 0),
	PROC_FIELD(MPR_LWP_MAX, "mpr_lwp[2]", LWPS, "lwps", CONFIGURABLE, metp_lwp_resrc,
		  mpr_lwp[MET_MAX], 0),
};


finfo_t metp_str_resrc_info[] = 
{
	PROC_FIELD(STR_STREAM_INUSE, "str_stream[INUSE]", STREAMS, "streams", 
		   COUNT, metp_str_resrc, str_stream[MET_INUSE], 0),
	PROC_FIELD(STR_STREAM_TOTAL, "str_stream[TOTAL]", STREAMS, "streams", 
		   SUM, metp_str_resrc, str_stream[MET_TOTAL], 0),
	PROC_FIELD(STR_QUEUE_INUSE, "str_queue[INUSE]", QUEUES, "queues", 
		   COUNT, metp_str_resrc, str_queue[MET_INUSE], 0),
	PROC_FIELD(STR_QUEUE_TOTAL, "str_queue[TOTAL]", QUEUES, "queues", 
		   SUM, metp_str_resrc, str_queue[MET_TOTAL], 0),
	PROC_FIELD(STR_MDBBLK_INUSE, "str_mdbblk[INUSE]", MDBBLKS, "mdbblks", 
		   COUNT, metp_str_resrc, str_mdbblock[MET_INUSE], 0),
	PROC_FIELD(STR_MDBBLK_TOTAL, "str_mdbblk[TOTAL]", MDBBLKS, "mdbblks", 
		   SUM, metp_str_resrc, str_mdbblock[MET_TOTAL], 0),
	PROC_FIELD(STR_MSGBLK_INUSE, "str_msgblk[INUSE]", MSGBLKS, "msgblks", 
		   COUNT, metp_str_resrc, str_msgblock[MET_INUSE], 0),
	PROC_FIELD(STR_MSGBLK_TOTAL, "str_msgblk[TOTAL]", MSGBLKS, "msgblks", 
		   SUM, metp_str_resrc, str_msgblock[MET_TOTAL], 0),
	PROC_FIELD(STR_LINK_INUSE, "str_link[INUSE]", LINKS, "links", 
		   COUNT, metp_str_resrc, str_linkblk[MET_INUSE], 0),
	PROC_FIELD(STR_LINK_TOTAL, "str_link[TOTAL]", LINKS, "links", 
		   SUM, metp_str_resrc, str_linkblk[MET_TOTAL], 0),
	PROC_FIELD(STR_EVENT_INUSE, "str_strevent[INUSE]", EVENTS, "events", 
		   COUNT, metp_str_resrc, str_strevent[MET_INUSE], 0),
	PROC_FIELD(STR_EVENT_TOTAL, "str_strevent[TOTAL]", EVENTS, "events", 
		   SUM, metp_str_resrc, str_strevent[MET_TOTAL], 0),
	PROC_FIELD(STR_EVENT_FAIL, "str_strevent[FAIL]", EVENTS, "events", 
		   SUM, metp_str_resrc, str_strevent[MET_FAIL], 0),
};



finfo_t ds_info[] =
{
/*
	DISK_FIELD(DS_NAME, "ds_name", TEXT, "", DESCRIPTIVE, ds_name, 0),
*/

	{ DS_NAME, "ds_name", TEXT, "", DESCRIPTIVE, sizeof(dummy_ds.ds_name[0]),
	  sizeof(dummy_ds.ds_name), 
	  (caddr_t) &(dummy_ds.ds_name) - (caddr_t) &(dummy_ds), 
	  0 },


	DISK_FIELD(DS_CYLS, "ds_cyls", CYLINDERS, "cylinders", CONSTANT, ds_cyls, 0),
	DISK_FIELD(DS_FLAGS, "ds_flags", BITS, "bits", BITFLAG, ds_flags, 0),
	DISK_FIELD(DS_QLEN, "ds_qlen", JOBS, "jobs", COUNT, ds_qlen, 0),
	DISK_FIELD(DS_LASTTIME, "ds_lasttime", TIX, "ticks", TIMESTAMP, ds_lasttime, 0),
	DISK_FIELD(DS_RESP, "ds_resp", USECS, "usecs", SUM, ds_resp, 0),
	DISK_FIELD(DS_ACTIVE, "ds_active", USECS, "usecs", SUM, ds_active, 0),
	DISK_FIELD(DS_READ, "ds_reads", CALLS, "calls", SUM, ds_op[1], 0),
	DISK_FIELD(DS_WRITE, "ds_writes", CALLS, "calls", SUM, ds_op[0], 0),
/*
 *	Not implemented
 *
 *	DISK_FIELD(DS_MISC, "ds_miscs", CALLS, "calls", SUM, ds_op[2], 0),
 */
	DISK_FIELD(DS_READBLK, "ds_readblks", CALLS, "calls", SUM, ds_opblks[1], 0),
	DISK_FIELD(DS_WRITEBLK, "ds_writeblks", CALLS, "calls", SUM, ds_opblks[0], 0),
/*
 *	Not implemented
 *
 *	DISK_FIELD(DS_MISCBLK, "ds_miscblks", CALLS, "calls", SUM, ds_opblks[2], 0),
 */
};


metinfo_t metinfo[] = 
{
	SYSMET_INFO(mets_native_units),
	SYSMET_INFO(mets_counts),
	SYSMET_INFO(mets_wait),
	SYSMET_INFO(mets_sched),
	SYSMET_INFO(mets_proc_resrc),
	SYSMET_INFO(mets_fstypes),
	SYSMET_INFO(mets_files),
	SYSMET_INFO_ARR(mets_inodes, PT_BY_FS),
	SYSMET_INFO(mets_mem),
	SYSMET_INFO(mets_kmemsizes),
#ifdef PERF
	SYSMET_INFO_ARR(mets_fsinfo, PT_BY_FS),
#endif
	PROCMET_INFO(cnt),
	PROCMET_INFO(metp_cpu),
	PROCMET_INFO(metp_sched),
	PROCMET_INFO(metp_buf),
	PROCMET_INFO(metp_syscall),
	PROCMET_INFO(metp_filelookup),
	PROCMET_INFO_ARR(metp_fileaccess, PT_BY_FS),
	PROCMET_INFO(metp_tty),
	PROCMET_INFO(metp_ipc),
	PROCMET_INFO(metp_vm),
	PROCMET_INFO_ARR(metp_kmem, PT_BY_MEMSIZE),
	PROCMET_INFO(metp_lwp_resrc),
	PROCMET_INFO(metp_str_resrc),
	PROCMET_INFO(metp_cg),
	{ PT_DISK_STRUCT, sizeof(dummy_ds), 0, 0, 
	  sizeof(ds_info)/sizeof(*ds_info), ds_info }
};
	
int	metinfo_size = sizeof(metinfo)/sizeof(*metinfo);

