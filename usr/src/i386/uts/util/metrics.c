#ident	"@(#)kern-i386:util/metrics.c	1.17.3.1"
#ident	"$Header$"

/*
 * Definitions and initial assignments of system-wide metrics structures
 */

#include <mem/tuneable.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/var.h>
#include <proc/cg.h>
#include <proc/disp.h>
#include <util/inline.h>

struct mets_wait_locks	mets_wait_locks;	/* fspin locks for wait I/O metrics */
struct met_ppmets_offsets met_ppmets_offsets;	/* offsets of plocal met structs */
struct met_localdata_ptrs *met_localdata_ptrs_p;/* addresses of plocal structs */

extern int met_ds_ndisks;

/*
 * void metrics(void)
 *	Initialize various metrics structures.
 *
 * Calling/Exit State:
 *	No locks held before, during, or after.
 */
		
void
metrics()
{

	int	i;
	vaddr_t l_ptr;

	/*
	 * store system constants
	 */
	MET_HZ();
	MET_PAGESZ();
	
	/*
	 * Get system counts.
	 */
	MET_NENGINES();
	MET_NCGS();

	/*
	 * Store system maximums for resources
	 */
	MET_PROC_MAX(v.v_proc);

	/*
	 * Store the possible number of KMA pools, including oversize pool
	 */
	m.mets_kmemsizes.msk_numpools = MET_KMEM_NCLASS;

	/*
	 * Store the number of fstypes and their names
	 */
	m.mets_fstypes.msf_numtypes = MET_FSTYPES;
	strcpy(m.mets_fstypes.msf_names[MET_S5], "s5");
	strcpy(m.mets_fstypes.msf_names[MET_SFS], "sfs/ufs");
	strcpy(m.mets_fstypes.msf_names[MET_VXFS], "vxfs");
	strcpy(m.mets_fstypes.msf_names[MET_OTHER], "other");

	/*
	 * Store the plocalmet metric structure offsets
	 * We'll use our own plocalmet structure to figure them out.
	 */
	l_ptr = (vaddr_t)&lm;
	met_ppmets_offsets.metp_cpu_offset =
				(int)((vaddr_t)&lm.metp_cpu - l_ptr);

	met_ppmets_offsets.metp_cg_offset = 
				(int)((vaddr_t)&lm.metp_cg - l_ptr);

	met_ppmets_offsets.metp_sched_offset =
				(int)((vaddr_t)&lm.metp_sched - l_ptr);

	met_ppmets_offsets.metp_buf_offset =
				(int)((vaddr_t)&lm.metp_buf - l_ptr);

	met_ppmets_offsets.metp_syscall_offset =
				(int)((vaddr_t)&lm.metp_syscall - l_ptr);

	met_ppmets_offsets.metp_filelookup_offset =
				(int)((vaddr_t)&lm.metp_filelookup - l_ptr);

	met_ppmets_offsets.metp_fileaccess_offset =
				(int)((vaddr_t)&lm.metp_fileaccess - l_ptr);

	met_ppmets_offsets.metp_tty_offset =
				(int)((vaddr_t)&lm.metp_tty - l_ptr);

	met_ppmets_offsets.metp_ipc_offset =
				(int)((vaddr_t)&lm.metp_ipc - l_ptr);

	met_ppmets_offsets.metp_vm_offset =
				(int)((vaddr_t)&lm.metp_vm - l_ptr);

	met_ppmets_offsets.metp_kmem_offset =
				(int)((vaddr_t)&lm.metp_kmem - l_ptr);

	met_ppmets_offsets.metp_lwp_resrc_offset =
				(int)((vaddr_t)&lm.metp_lwp_resrc - l_ptr);

	met_ppmets_offsets.metp_str_resrc_offset =
				(int)((vaddr_t)&lm.metp_str_resrc - l_ptr);

	/*
	 * Allocate space to store the addresses of each plocal structure.
	 */
	met_localdata_ptrs_p = (struct met_localdata_ptrs *)kmem_zalloc(
						(size_t)MET_LOCALDATA_PTRS_SZ,
						 KM_SLEEP);
	met_localdata_ptrs_p->num_eng = Nengine;

	for (i = 0; i < Nengine; i++) {
		struct plocalmet *p = ENGINE_PLOCALMET_PTR(i);
		p->metp_cg.mpg_cgid = cgnum2cgid(CPUtoCG(i));
		met_localdata_ptrs_p->localdata_p[i] = (vaddr_t) p;
	}
}

void
met_once_sec()
{
	cgnum_t icg;	/* indexing cg */
	int ifs;	/* indexing file system types */
 
 	int sum_iowait = 0;
 	int sum_physio = 0;
 	    
        long sum_files_file[4] = {0, 0, 0, 0};

	long sum_inodes_inodes[MET_FSTYPES][4] = {
                   {0, 0, 0, 0}, /* MET_S5 */
                   {0, 0, 0, 0}, /* MET_SFS */
                   {0, 0, 0, 0}, /* MET_VXFS */
                   {0, 0, 0, 0}  /* MET_OTHER */
        };
#ifdef PERF
	long sum_fsinfo_getpage[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_pgin[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_pgpgin[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_sectin[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_ra[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_rapgpgin[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_rasectin[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_putpage[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_pgout[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_pgpgout[MET_FSTYPES] = {0, 0, 0, 0};
	long sum_fsinfo_sectout[MET_FSTYPES] = {0, 0, 0, 0};
#endif /* PERF */

	/* 
	 * Update global runque and swap "queue" metrics
	 */
	MET_GLOB_RUNQUE();
	MET_GLOB_SWAPQUE();
	    
	MET_FREEMEM(mem_freemem[STD_PAGE]);	/* Update freemem statistics */

	for (icg = 0; icg < Ncg; icg++) {
		struct cglocalmet *cgmp;

		if (! IsCGOnline(icg)) continue;                     

                cgmp = CGVAR(&cg.cglocalmet, struct cglocalmet, icg);

	        /* mets_wait */
                sum_iowait += (cgmp->metcg_wait).msw_iowait;
                sum_physio += (cgmp->metcg_wait).msw_physio;

                /* mets_files */
                sum_files_file[MET_INUSE] += (cgmp->metcg_files).msf_file[MET_INUSE];
                sum_files_file[MET_FAIL] += (cgmp->metcg_files).msf_file[MET_FAIL];
	        /* mets_inodes */
	        for (ifs = 0; ifs < MET_FSTYPES; ifs++) {
	                    sum_inodes_inodes[ifs][MET_INUSE] += (cgmp->metcg_inodes[ifs]).msi_inodes[MET_INUSE];
	                    sum_inodes_inodes[ifs][MET_FAIL] += (cgmp->metcg_inodes[ifs]).msi_inodes[MET_FAIL];
	                    sum_inodes_inodes[ifs][MET_CURRENT] += (cgmp->metcg_inodes[ifs]).msi_inodes[MET_CURRENT];
                }
#ifdef PERF
	        /* mets_fsinfo */
	        for (ifs = 0; ifs < MET_FSTYPES; ifs++) {
	                    sum_fsinfo_getpage[ifs] += (cgmp->metcg_fsinfo[ifs]).getpage;
	                    sum_fsinfo_pgin[ifs] += (cgmp->metcg_fsinfo[ifs]).pgin;
	                    sum_fsinfo_pgpgin[ifs] += (cgmp->metcg_fsinfo[ifs]).pgpgin;
	                    sum_fsinfo_sectin[ifs] += (cgmp->metcg_fsinfo[ifs]).sectin;
	                    sum_fsinfo_ra[ifs] += (cgmp->metcg_fsinfo[ifs]).ra;
	                    sum_fsinfo_rapgpgin[ifs] += (cgmp->metcg_fsinfo[ifs]).rapgpgin;
	                    sum_fsinfo_rasectin[ifs] += (cgmp->metcg_fsinfo[ifs]).rasectin;
	                    sum_fsinfo_putpage[ifs] += (cgmp->metcg_fsinfo[ifs]).putpage;
	                    sum_fsinfo_pgout[ifs] += (cgmp->metcg_fsinfo[ifs]).pgout;
	                    sum_fsinfo_pgpgout[ifs] += (cgmp->metcg_fsinfo[ifs]).pgpgout;
	                    sum_fsinfo_sectout[ifs] += (cgmp->metcg_fsinfo[ifs]).sectout;
	        }
#endif /* PERF */
	}

        /* update system-wide mets_wait */
	m.mets_wait.msw_iowait = sum_iowait;
	m.mets_wait.msw_physio = sum_physio;

        /* update mets_files */
	m.mets_files.msf_file[MET_INUSE] = sum_files_file[MET_INUSE];
	m.mets_files.msf_file[MET_FAIL] = sum_files_file[MET_FAIL];

        /* update mets_inodes */
	for (ifs = 0; ifs < MET_FSTYPES; ifs++) {
	            m.mets_inodes[ifs].msi_inodes[MET_INUSE] = sum_inodes_inodes[ifs][MET_INUSE];
	            m.mets_inodes[ifs].msi_inodes[MET_FAIL] = sum_inodes_inodes[ifs][MET_FAIL];
	            m.mets_inodes[ifs].msi_inodes[MET_CURRENT] = sum_inodes_inodes[ifs][MET_CURRENT];
	}

#ifdef PERF
        /* update mets_fsinfo */
	for (ifs = 0; ifs < MET_FSTYPES; ifs++) {
	            m.mets_fsinfo[ifs].getpage = sum_fsinfo_getpage[ifs];
	            m.mets_fsinfo[ifs].pgin    = sum_fsinfo_pgin[ifs];
	            m.mets_fsinfo[ifs].pgpgin  = sum_fsinfo_pgpgin[ifs];
	            m.mets_fsinfo[ifs].sectin  = sum_fsinfo_sectin[ifs];
	            m.mets_fsinfo[ifs].ra      = sum_fsinfo_ra[ifs];
	            m.mets_fsinfo[ifs].rapgpgin = sum_fsinfo_rapgpgin[ifs];
	            m.mets_fsinfo[ifs].rasectin = sum_fsinfo_rasectin[ifs];
	            m.mets_fsinfo[ifs].putpage  = sum_fsinfo_putpage[ifs];
	            m.mets_fsinfo[ifs].pgout   = sum_fsinfo_pgout[ifs];
	            m.mets_fsinfo[ifs].pgpgout = sum_fsinfo_pgpgout[ifs];
	            m.mets_fsinfo[ifs].sectout = sum_fsinfo_sectout[ifs];
	}
#endif /* PERF */
	    
	/*
	 * Update the freeswap cumulative count
	 */
	MET_FREESWAP(anoninfo.ani_kma_max - anoninfo.ani_resv);
}

void
met_init_cg ()
{
        FSPIN_INIT(&cg.cglocalmet.metcg_wait_locks.msw_iowait_fspin);
        FSPIN_INIT(&cg.cglocalmet.metcg_wait_locks.msw_physio_fspin);
        FSPIN_INIT(&cg.cglocalmet.metcg_file_list_mutex);
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void print_metrics(void)
 *	Print various metrics structures.
 *
 * Calling/Exit State:
 *	No locks held before, during, or after.
 */

void
print_metrics(void)
{
	uint_t i, j;
	struct plocalmet *plp;
	time_t cpu[4];
	ulong  pswitch = 0;
	ulong  lwpmax = 0;
	ulong  lwpinuse = 0;
	ulong  lwpfail = 0;
	ulong  runque = 0;
	ulong  runocc = 0;
	ulong  bread = 0;
	ulong  bwrite = 0;
	ulong  lread = 0;
	ulong  lwrite = 0;
	ulong  phread = 0;
	ulong  phwrite = 0;
	ulong  syscall = 0;
	ulong  fork = 0;
	ulong  lwpcreate = 0;
	ulong  exec = 0;
	ulong  read = 0;
	ulong  write = 0;
	ulong  readch = 0;
	ulong  writech = 0;
	ulong  lookup = 0;
	ulong  dnlc_hits = 0;
	ulong  dnlc_misses = 0;
	ulong  iget = 0;
	ulong  dirblk = 0;
	ulong  ipage = 0;
	ulong  inopage = 0;
	ulong  rcvint = 0;
	ulong  xmtint = 0;
	ulong  mdmint = 0;
	ulong  rawch = 0;
	ulong  canch = 0;
	ulong  outch = 0;
	ulong  msg = 0;
	ulong  sema = 0;
	long   mem = 0;
	long   balloc = 0;
	long   ralloc = 0;
	ushort fail = 0;
	extern void print_page_plocal_stats(void);

	debug_printf("System Wide Metrics:\n\n"
		     "mets_native_units:  mnu_hz  mnu_pagesz\n"
		     "                   %6d      %6d\n\n",
		m.mets_native_units.mnu_hz, m.mets_native_units.mnu_pagesz);
	debug_printf("mets_wait:         msw_iowait  msw_physio\n"
		     "                      %6d      %6d\n\n",
		m.mets_wait.msw_iowait, m.mets_wait.msw_physio);
	debug_printf("mets_sched:        mss_runque  mss_runocc  mss_swpque"
		      "  mss_swpocc\n"
		     "                      %6d      %6d      %6d      %6d\n\n",
		m.mets_sched.mss_runque, m.mets_sched.mss_runocc,
		m.mets_sched.mss_swpque, m.mets_sched.mss_swpocc);
	debug_printf("mets_proc_resrc:   msr_proc[MAX]  msr_proc[INUSE]"
		      "  msr_proc[FAIL]\n"
		     "                          %6d           %6d"
		      "          %6d\n\n",
		m.mets_proc_resrc.msr_proc[MET_MAX],
		m.mets_proc_resrc.msr_proc[MET_INUSE],
		m.mets_proc_resrc.msr_proc[MET_FAIL]);
	debug_printf("mets_files:        msf_file[MAX]  msf_file[INUSE]"
		      "  msr_file[FAIL]\n"
		     "                          %6d           %6d"
		      "          %6d\n",
		m.mets_files.msf_file[MET_MAX],
		m.mets_files.msf_file[MET_INUSE],
		m.mets_files.msf_file[MET_FAIL]);
	debug_printf("                   msf_flck[MAX]  msf_flck[INUSE]"
		      "  msr_flck[FAIL]\n"
		     "                          %6d           %6d"
		      "          %6d\n\n",
		m.mets_files.msf_flck[MET_MAX],
		m.mets_files.msf_flck[MET_INUSE],
		m.mets_files.msf_flck[MET_FAIL]);
	debug_printf("mets_inodes:   msi_inode_max  msi_inode_curr"
		      "  msi_inode_inuse  msi_inode_fail\n"
		     "         s5:          %6d          %6d"
		      "           %6d          %6d\n",
		m.mets_inodes[MET_S5].msi_inodes[MET_MAX],
		m.mets_inodes[MET_S5].msi_inodes[MET_CURRENT],
		m.mets_inodes[MET_S5].msi_inodes[MET_INUSE],
		m.mets_inodes[MET_S5].msi_inodes[MET_FAIL]);
	debug_printf("        sfs:          %6d          %6d"
		      "           %6d          %6d\n",
		m.mets_inodes[MET_SFS].msi_inodes[MET_MAX],
		m.mets_inodes[MET_SFS].msi_inodes[MET_CURRENT],
		m.mets_inodes[MET_SFS].msi_inodes[MET_INUSE],
		m.mets_inodes[MET_SFS].msi_inodes[MET_FAIL]);
 	debug_printf("        vxfs:          %6d          %6d"
 		      "           %6d          %6d\n",
 		m.mets_inodes[MET_VXFS].msi_inodes[MET_MAX],
 		m.mets_inodes[MET_VXFS].msi_inodes[MET_CURRENT],
 		m.mets_inodes[MET_VXFS].msi_inodes[MET_INUSE],
 		m.mets_inodes[MET_VXFS].msi_inodes[MET_FAIL]);
	debug_printf("      other:          %6d          %6d"
		      "           %6d          %6d\n\n",
		m.mets_inodes[MET_OTHER].msi_inodes[MET_MAX],
		m.mets_inodes[MET_OTHER].msi_inodes[MET_CURRENT],
		m.mets_inodes[MET_OTHER].msi_inodes[MET_INUSE],
		m.mets_inodes[MET_OTHER].msi_inodes[MET_FAIL]);
	debug_printf("mets_mem:          msm_freemem        msm_freeswap\n"
		     "               %6d %6d     %6d %6d\n\n",
		m.mets_mem.msm_freemem.dl_hop, m.mets_mem.msm_freemem.dl_lop,
		m.mets_mem.msm_freeswap.dl_hop, m.mets_mem.msm_freeswap.dl_lop);

	debug_printf("\nPER-PROCESSOR METRICS\n\n");
		
	debug_printf("metp_cpu:    eng     mpc_cpu[USR]  mpc_cpu[SYS]"
		      "  mpc_cpu[WAIT]  mpc_cpu[IDLE]\n");
	for (j = 0; j < 4; j++)
		cpu[j] = 0;
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("              %2d           %6d        %6d"
			      "         %6d         %6d\n",
			i, plp->metp_cpu.mpc_cpu[MET_CPU_USER],
			plp->metp_cpu.mpc_cpu[MET_CPU_SYS],
			plp->metp_cpu.mpc_cpu[MET_CPU_WAIT],
			plp->metp_cpu.mpc_cpu[MET_CPU_IDLE]);
		for (j = 0; j < 4; j++)
			cpu[j] += plp->metp_cpu.mpc_cpu[j];
		if (debug_output_aborted())
			return;
	}
	debug_printf("                           ------        ------"
		      "         ------         ------\n"
		     "                           %6d        %6d"
		      "         %6d         %6d\n\n",
		cpu[MET_CPU_USER], cpu[MET_CPU_SYS], cpu[MET_CPU_WAIT],
		cpu[MET_CPU_IDLE]);

	debug_printf("metp_lwp_resrc: eng\tmpr_lwp[MAX]   mpr_lwp[INUSE]"
		      "   mpr_lwp[FAIL]\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("               %2d\t      %6d"
			      "           %6d          %6d\n",
			i, plp->metp_lwp_resrc.mpr_lwp[MET_MAX],
			plp->metp_lwp_resrc.mpr_lwp[MET_INUSE],
			plp->metp_lwp_resrc.mpr_lwp[MET_FAIL]);
		if (plp->metp_lwp_resrc.mpr_lwp[MET_MAX] > lwpmax)
			lwpmax = plp->metp_lwp_resrc.mpr_lwp[MET_MAX];
		lwpinuse += plp->metp_lwp_resrc.mpr_lwp[MET_INUSE];
		lwpfail += plp->metp_lwp_resrc.mpr_lwp[MET_FAIL];
		if (debug_output_aborted())
			return;
	}
	debug_printf("                 \t      ------           ------"
		      "          ------\n"
		     "                 \t      %6d           %6d"
		      "          %6d\n\n",
		lwpmax, lwpinuse, lwpfail);

	debug_printf("metp_sched:   eng\tmps_pswitch  mps_runque"
		      "  mps_runocc\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("               %2d\t     %6d      %6d      %6d\n",
			i, plp->metp_sched.mps_pswitch,
			plp->metp_sched.mps_runque,
			plp->metp_sched.mps_runocc);
		pswitch += plp->metp_sched.mps_pswitch;
		runque += plp->metp_sched.mps_runque;
		runocc += plp->metp_sched.mps_runocc;
		if (debug_output_aborted())
			return;
	}
	debug_printf("                 \t     ------      ------"
		      "      ------\n"
		     "                 \t     %6d      %6d"
		      "      %6d\n\n",
		pswitch, runque, runocc);

	debug_printf("metp_buf:     eng\t bread  bwrite  lread  lwrite"
		      "  phread  phwrite\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("               %2d\t%6d  %6d %6d"
			      "  %6d  %6d   %6d\n",
			i, plp->metp_buf.mpb_bread,
			plp->metp_buf.mpb_bwrite,
			plp->metp_buf.mpb_lread,
			plp->metp_buf.mpb_lwrite,
			plp->metp_buf.mpb_phread,
			plp->metp_buf.mpb_phwrite);
		bread += plp->metp_buf.mpb_bread;
		bwrite += plp->metp_buf.mpb_bwrite;
		lread += plp->metp_buf.mpb_lread;
		lwrite += plp->metp_buf.mpb_lwrite;
		phread += plp->metp_buf.mpb_phread;
		phwrite += plp->metp_buf.mpb_phwrite;
		if (debug_output_aborted())
			return;
	}
	debug_printf("                 \t------  ------ ------"
		      "  ------  ------   ------\n"
		     "                 \t%6d  %6d %6d"
		      "  %6d  %6d   %6d\n\n",
		bread, bwrite, lread, lwrite, phread, phwrite);

	debug_printf("metp_syscall: eng syscall   fork  lwpcr"
		      "   exec   read  write  readch  writech\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("              %2d   %6d %6d"
			      " %6d %6d %6d %6d %6d   %6d\n",
			i, plp->metp_syscall.mps_syscall,
			plp->metp_syscall.mps_fork,
			plp->metp_syscall.mps_lwpcreate,
			plp->metp_syscall.mps_exec,
			plp->metp_syscall.mps_read,
			plp->metp_syscall.mps_write,
			plp->metp_syscall.mps_readch,
			plp->metp_syscall.mps_writech);
		syscall += plp->metp_syscall.mps_syscall;
		fork += plp->metp_syscall.mps_fork;
		lwpcreate += plp->metp_syscall.mps_lwpcreate;
		exec += plp->metp_syscall.mps_exec;
		read += plp->metp_syscall.mps_read;
		write += plp->metp_syscall.mps_write;
		readch += plp->metp_syscall.mps_readch;
		writech += plp->metp_syscall.mps_writech;
		if (debug_output_aborted())
			return;
	}
	debug_printf("                   ------ ------"
		      " ------ ------ ------ ------  ------   ------\n"
		     "                   %6d %6d"
		      " %6d %6d %6d %6d %6d   %6d\n\n",
		syscall, fork, lwpcreate, exec, read, write, readch, writech);

	debug_printf("filelookup:   eng\tmpf_lookup  mpf_dnlc_hits"
		      "  mpf_dnlc_misses\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("               %2d\t    %6d         %6d"
			      "           %6d\n",
			i, plp->metp_filelookup.mpf_lookup,
			plp->metp_filelookup.mpf_dnlc_hits,
			plp->metp_filelookup.mpf_dnlc_misses);
		lookup += plp->metp_filelookup.mpf_lookup;
		dnlc_hits += plp->metp_filelookup.mpf_dnlc_hits;
		dnlc_misses += plp->metp_filelookup.mpf_dnlc_misses;
		if (debug_output_aborted())
			return;
	}
	debug_printf("                 \t    ------         ------"
		      "           ------\n"
		     "                 \t    %6d         %6d"
		      "           %6d\n\n",
		lookup, dnlc_hits, dnlc_misses);

	debug_printf("fileaccess:   eng\tmpf_iget  mpf_dirblk"
		      "  mpf_ipage  mpf_inopage\n");
	debug_printf("        s5:\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("               %2d\t  %6d      %6d"
			      "     %6d       %6d\n",
			i, plp->metp_fileaccess[MET_S5].mpf_iget,
			plp->metp_fileaccess[MET_S5].mpf_dirblk,
			plp->metp_fileaccess[MET_S5].mpf_ipage,
			plp->metp_fileaccess[MET_S5].mpf_inopage);
		iget += plp->metp_fileaccess[MET_S5].mpf_iget;
		dirblk += plp->metp_fileaccess[MET_S5].mpf_dirblk;
		ipage += plp->metp_fileaccess[MET_S5].mpf_ipage;
		inopage += plp->metp_fileaccess[MET_S5].mpf_inopage;
		if (debug_output_aborted())
			return;
	}
	debug_printf("                 \t  ------      ------"
		      "     ------       ------\n"
		     "                 \t  %6d      %6d"
		      "     %6d       %6d\n\n",
		iget, dirblk, ipage, inopage);
	iget = 0;
	dirblk = 0;
	ipage = 0;
	inopage = 0;
	debug_printf("       sfs:\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("              %2d\t  %6d      %6d"
			      "     %6d       %6d\n",
			i, plp->metp_fileaccess[MET_SFS].mpf_iget,
			plp->metp_fileaccess[MET_SFS].mpf_dirblk,
			plp->metp_fileaccess[MET_SFS].mpf_ipage,
			plp->metp_fileaccess[MET_SFS].mpf_inopage);
		iget += plp->metp_fileaccess[MET_SFS].mpf_iget;
		dirblk += plp->metp_fileaccess[MET_SFS].mpf_dirblk;
		ipage += plp->metp_fileaccess[MET_SFS].mpf_ipage;
		inopage += plp->metp_fileaccess[MET_SFS].mpf_inopage;
 		if (debug_output_aborted())
 			return;
 	}
 	debug_printf("                 \t  ------      ------"
 		      "     ------       ------\n"
 		     "                 \t  %6d      %6d"
 		      "     %6d       %6d\n\n",
 		iget, dirblk, ipage, inopage);
 	iget = 0;
 	dirblk = 0;
 	ipage = 0;
 	inopage = 0;
 	debug_printf("       vxfs:\n");
 	for (i = 0; i < Nengine; i++) {
 		plp = ENGINE_PLOCALMET_PTR(i);
 		debug_printf("              %2d\t  %6d      %6d"
 			      "     %6d       %6d\n",
 			i, plp->metp_fileaccess[MET_VXFS].mpf_iget,
 			plp->metp_fileaccess[MET_VXFS].mpf_dirblk,
 			plp->metp_fileaccess[MET_VXFS].mpf_ipage,
 			plp->metp_fileaccess[MET_VXFS].mpf_inopage);
 		iget += plp->metp_fileaccess[MET_VXFS].mpf_iget;
 		dirblk += plp->metp_fileaccess[MET_VXFS].mpf_dirblk;
 		ipage += plp->metp_fileaccess[MET_VXFS].mpf_ipage;
 		inopage += plp->metp_fileaccess[MET_VXFS].mpf_inopage;
		if (debug_output_aborted())
			return;
	}
	debug_printf("                 \t  ------      ------"
		      "     ------       ------\n"
		     "                 \t  %6d      %6d"
		      "     %6d       %6d\n\n",
		iget, dirblk, ipage, inopage);
	iget = 0;
	dirblk = 0;
	ipage = 0;
	inopage = 0;
	debug_printf("     other:\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("              %2d\t  %6d      %6d"
			      "     %6d       %6d\n",
			i, plp->metp_fileaccess[MET_OTHER].mpf_iget,
			plp->metp_fileaccess[MET_OTHER].mpf_dirblk,
			plp->metp_fileaccess[MET_OTHER].mpf_ipage,
			plp->metp_fileaccess[MET_OTHER].mpf_inopage);
		iget += plp->metp_fileaccess[MET_OTHER].mpf_iget;
		dirblk += plp->metp_fileaccess[MET_OTHER].mpf_dirblk;
		ipage += plp->metp_fileaccess[MET_OTHER].mpf_ipage;
		inopage += plp->metp_fileaccess[MET_OTHER].mpf_inopage;
		if (debug_output_aborted())
			return;
	}
	debug_printf("                 \t  ------      ------"
		      "     ------       ------\n"
		     "                 \t  %6d      %6d"
		      "     %6d       %6d\n\n",
		iget, dirblk, ipage, inopage);

	debug_printf("metp_tty:     eng\trcvint  xmtint  mdmint"
		      "  rawch  canch  outch\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("               %2d\t%6d  %6d  %6d %6d %6d %6d\n",
			i, plp->metp_tty.mpt_rcvint,
			plp->metp_tty.mpt_xmtint,
			plp->metp_tty.mpt_mdmint,
			plp->metp_tty.mpt_rawch,
			plp->metp_tty.mpt_canch,
			plp->metp_tty.mpt_outch);
		rcvint += plp->metp_tty.mpt_rcvint;
		xmtint += plp->metp_tty.mpt_xmtint;
		mdmint += plp->metp_tty.mpt_mdmint;
		rawch += plp->metp_tty.mpt_rawch;
		canch += plp->metp_tty.mpt_canch;
		outch += plp->metp_tty.mpt_outch;
		if (debug_output_aborted())
			return;
	}
	debug_printf("                 \t------  ------"
		      "  ------ ------ ------ ------\n"
		     "                 \t%6d  %6d"
		      "  %6d %6d %6d %6d\n\n",
		rcvint, xmtint, mdmint, rawch, canch, outch);

	debug_printf("metp_ipc:     eng\tmpi_msg  mpi_sema\n");
	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("               %2d\t %6d    %6d\n",
			i, plp->metp_ipc.mpi_msg,
			plp->metp_ipc.mpi_sema);
		msg += plp->metp_ipc.mpi_msg;
		sema += plp->metp_ipc.mpi_sema;
		if (debug_output_aborted())
			return;
	}
	debug_printf("                 \t ------    ------\n"
		     "                 \t %6d    %6d\n\n", msg, sema);

	print_page_plocal_stats();

	debug_printf("\nmetp_kmem:    eng\tmpk_mem  mpk_balloc"
		      "  mpk_ralloc  mpk_fail\n");
	for (j = 0; j < MET_KMEM_NCLASS; j++) {
		if (j == MET_KMOVSZ)
			debug_printf("     OVSZ:\n");
		else
			debug_printf("   %6d:\n", m.mets_kmemsizes.msk_sizes[j]);
		for (i = 0; i < Nengine; i++) {
			plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("              %2d\t %6d      %6d"
			      "      %6d    %6d\n",
				i, plp->metp_kmem[j].mpk_mem,
				plp->metp_kmem[j].mpk_balloc,
				plp->metp_kmem[j].mpk_ralloc,
				plp->metp_kmem[j].mpk_fail);
			mem += plp->metp_kmem[j].mpk_mem;
			balloc += plp->metp_kmem[j].mpk_balloc;
			ralloc += plp->metp_kmem[j].mpk_ralloc;
			fail += plp->metp_kmem[j].mpk_fail;
		}
		debug_printf("                 \t ------      ------"
			      "      ------    ------\n"
			     "                 \t %6d      %6d"
			      "      %6d    %6d\n\n",
			mem, balloc, ralloc, fail);
		mem = 0;
		balloc = 0;
		ralloc = 0;
		fail = 0;
		if (debug_output_aborted())
			return;
	} /* for MET_KMEM_CLASS */
}

#endif /* DEBUG || DEBUG_TOOLS */
