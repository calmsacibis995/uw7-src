/* copyright "%c%" */
#ident	"@(#)kern-i386at:io/cpumtr/cpumtr.c	1.1.2.2"
#ident	"$Header$"



/**************************************************************
* Copyright (c) International Comptuters Ltd 1996
* Copyright (c) 1996 HAL Computer Systems
*/

#ifdef _KERNEL_HEADERS
# include <io/conf.h>
# include <util/types.h>
# include <io/ddi_i386at.h>
# include <io/open.h>
# include <fs/file.h>
# include <mem/kmem.h>
# include <proc/cred.h>
# include <svc/clock.h>
# include <svc/errno.h>
# include <util/cmn_err.h>
# include <util/mod/moddefs.h>
# include <io/ddi.h>
# include <io/cpumtr/cpumtr.h>
# include <proc/lwp.h>
# include <util/engine.h>
#else /* !_KERNEL_HEADERS */

# include <sys/conf.h>
# include <sys/types.h>
# include <sys/ddi_i386at.h>
# include <sys/open.h>
# include <sys/file.h>
# include <sys/kmem.h>
# include <sys/cred.h>
# include <sys/clock.h>
# include <sys/errno.h>
# include <sys/cmn_err.h>
# include <sys/moddefs.h>
# include <sys/ddi.h>
# include <sys/lwp.h>
# include <sys/engine.h>

# ifdef HEADERS_INSTALLED
#  include <sys/cpumtr.h>
# else /* !HEADERS_INSTALLED */
#  define SYSENTRY_ATTACH         1
#  define SYSENTRY_DETACH         2
#  define SYSEXIT_ATTACH          3
#  define SYSEXIT_DETACH          4
#  define TRAPENTRY_ATTACH        5
#  define TRAPENTRY_DETACH        6
#  define TRAPEXIT_ATTACH         7
#  define TRAPEXIT_DETACH         8
#  define SWTCH_OUT1_ATTACH       9
#  define SWTCH_OUT1_DETACH       10
#  define SWTCH_IN1_ATTACH        11
#  define SWTCH_IN1_DETACH        12
#  define SWTCH_OUT2_ATTACH       13
#  define SWTCH_OUT2_DETACH       14
#  define SWTCH_IN2_ATTACH        15
#  define SWTCH_IN2_DETACH        16

#  include "cpumtr.h"
# endif /* !HEADERS_INSTALLED */

#endif /* !_KERNEL_HEADERS */

#define DRVNAME "cpumtr - CPU Performance Meters Driver"
#define CPUMTR_MAX_NUM_LWPS 1024
#define CPUMTR_NUM_LWP_BUCKETS (CPUMTR_MAX_NUM_LWPS/4)

/* interval for CPU clock frequency measurement */
#define CPUMTR_START_USEC 1000000  /* 1 second */

/* interval for watchdog timer */
#define CPUMTR_WATCHDOG_USEC 10000000  /* 10 seconds */

/* forward declarations of functions:  */
static int cpumtr_load(void);
static int cpumtr_unload(void);
static void cpumtr_update_cpu(void *);

/* wrappers required for dynamic loading of driver:  */
MOD_DRV_WRAPPER(cpumtr, cpumtr_load, cpumtr_unload, NULL, DRVNAME);
int cpumtrdevflag = D_MP;

/* global variables:  */

/* lock variable to allocate CPUs to processes */
static lock_t *cpumtr_open_lock = NULL;
LKINFO_DECL(cpumtr_lkinfo_reservelock, "CPUMTR:cpumtr_open_lock", 0);

/* number of process indices allocated */
static int cpumtr_num_processes = 0;

/* flags to indicate whether this process index is in use */
static bool_t cpumtr_open_flag[CPUMTR_MAX_PROCESSES+1];


/* per-process info */
static cpumtr_process_t cpumtr_process[CPUMTR_MAX_PROCESSES+1];
LKINFO_DECL(cpumtr_lkinfo_processlock, "CPUMTR:cpumtr_process[x].cp_lock", 0);


cpumtr_lwp_t *cpumtr_lwp[CPUMTR_NUM_LWP_BUCKETS];    /* open hash table */
static cpumtr_lwp_t cpumtr_lwp_mem[CPUMTR_MAX_NUM_LWPS];
static uint_t cpumtr_num_lwps = 0;     /* cpumtr_lwp_t structs in table */
static rwlock_t *cpumtr_lwp_lock = NULL;              /* lock for table */
LKINFO_DECL(cpumtr_lkinfo_lwplock, "CPUMTR:cpumtr_lwp_lock", 0);

static uint_t cpumtr_num_cpus = 0;      /* number of CPUs in the system */
static size_t cpumtr_cpu_bytes = 0;     /* number of bytes in cpu array */
static cpumtr_cpu_t *cpumtr_cpu = NULL;
LKINFO_DECL(cpumtr_lkinfo_cpulock, "CPUMTR:cpumtr_cpu[x].cc_lock", 0);


/*******************************************************
* cpumtr_cputype()
*
* returns the CPU type.
*/
static cpumtr_cputype_t
cpumtr_cputype(void) {
  int res;    
  struct cpuparms cpuinfo;

  res = drv_gethardware(PROC_INFO, &cpuinfo);
  if (res != CPUMTR_NO_ERROR)
    return CPUMTR_NOT_SUPPORTED;

  if (cpuinfo.cpu_id == CPU_i686)
    return CPUMTR_P6;
  else
    return CPUMTR_NOT_SUPPORTED;
} /* cpumtr_cputype() */


/*******************************************************
* cpumtr_num_setups()
*
* returns the number of setups and sets first_setup.
* Returns -1 if the meter_id is invalid.
*/

static int
cpumtr_num_setups(cpumtr_meter_id_t meter_id, uint_t *first_setup) {
  cpumtr_cpu_t *cpu;

  /* Check the CPU id. */
  if (meter_id.cmi_cpu_id >= cpumtr_num_cpus)
    return -1;

  cpu = &(cpumtr_cpu[meter_id.cmi_cpu_id]);

  switch (cpu->cc_cputype) {
    case CPUMTR_P6:
      return cpumtr_p6_num_setups(meter_id, first_setup);
  }

  /* unknown CPU type */
  return -1;
} /* cpumtr_num_setups() */


/*******************************************************
* cpumtr_copy_setup()
*
* copy a portion of a sample_setup structure.
*/
static void
cpumtr_copy_setup(cpumtr_sample_setup_t *des, cpumtr_sample_setup_t *src,
    uint_t start, uint_t end) {
  uint_t isetup;
  uint_t ichar;


  for (isetup = start; isetup < end; isetup++) {
    des->css_max_interval[isetup] = src->css_max_interval[isetup];
    des->css_meter_setup[isetup] = src->css_meter_setup[isetup];
    for (ichar = 0; ichar < CPUMTR_MAX_NAME_LENGTH; ichar++) {
      char ch = src->css_name[isetup][ichar];
      des->css_name[isetup][ichar] = ch;
      if (ch == '\0')
        break;
    }
  }
} /* cpumtr_copy_setup() */


/*******************************************************
* cpumtr_deactivate_setup()
*
* deactivate a portion of a sample_setup structure.
*/
static int
cpumtr_deactivate_setup(cpumtr_sample_setup_t *des, uint_t start,
    uint_t end, cpumtr_cputype_t cputype) {
  uint_t isetup;

  for (isetup = start; isetup < end; isetup++) {
    des->css_max_interval[isetup] = (cpumtr_uint32_t)TO_PERIODIC;
    des->css_name[isetup][0] = '\0';
    switch (cputype) {
      case CPUMTR_P6:
        cpumtr_p6_deactivate(&(des->css_meter_setup[isetup]), isetup);
        break;
      default:
        cmn_err(CE_WARN, "CPUMTR:  deactivate_setup(cputype=%u)\n", cputype);
        return ENXIO;
    }
  }

  return CPUMTR_NO_ERROR;
} /* cpumtr_deactivate_setup() */


/*******************************************************
* cpumtr_lwp_alloc()
*
* allocates an lwp structure from lwp_mem.
* The cpumtr_lwp_lock should be RW_WRLOCKed on entry and exit.
* Returns NULL if no memory available.
*/
static cpumtr_lwp_t *
cpumtr_lwp_alloc (void) {
  if (cpumtr_num_lwps < CPUMTR_MAX_NUM_LWPS) {
    cpumtr_lwp_t * ptr;

    ptr = &(cpumtr_lwp_mem[cpumtr_num_lwps]);
    cpumtr_num_lwps++;
    return ptr;
  }
  
  return NULL;
} /* cpumtr_lwp_alloc() */


/*******************************************************
* cpumtr_get_lwp()
*
* gets a LWP structure for a particular pid/lwpid.
* The cpumtr_lwp_lock should be RW_WRLOCKed on entry and exit.
* Returns NULL if no memory available.  Does not sleep.
*/
static cpumtr_lwp_t *
cpumtr_get_lwp(pid_t pid, lwpid_t lwpid) {
  u_int hash;
  cpumtr_lwp_t **lwpp;
  cpumtr_lwp_t *lwp;
  u_int istate;

  hash = pid + lwpid;
  hash = hash % CPUMTR_NUM_LWP_BUCKETS;

  /* Search for an existing structure. */
  lwpp = &(cpumtr_lwp[hash]);
  while (*lwpp != NULL) {
    lwp = *lwpp;
    if (lwp->cl_pid == pid && lwp->cl_lwpid == lwpid)
      return lwp;
    lwpp = &(lwp->cl_next);
  }

  /*
   * Allocate a new structure.
   * Can't do kmem_alloc here because this might be called via swtch().
   */
  lwp = cpumtr_lwp_alloc();
  if (lwp == NULL)
    return NULL;

  /* Initialize the new structure. */
  for (istate = 0; istate < CPUMTR_NUM_LWP_STATES; istate++) {
    u_int isetup;

    for (isetup = 0; isetup < CPUMTR_NUM_METERS; isetup++) {
      lwp->cl_count[istate][isetup].dl_hop = 0;
      lwp->cl_count[istate][isetup].dl_lop = 0;
    }
    lwp->cl_time[istate].dl_hop = 0;
    lwp->cl_time[istate].dl_lop = 0;
  }
  lwp->cl_pid = pid;
  lwp->cl_lwpid = lwpid;
  lwp->cl_cpu = -1;

  /* Link it in. */
  lwp->cl_next = cpumtr_lwp[hash];
  cpumtr_lwp[hash] = lwp;

  return lwp;
} /* cpumtr_get_lwp() */


/*******************************************************
* cpumtr_lock()
*
* acquires a lock.
* This function exists to make debugging easier.
*/
static pl_t
cpumtr_lock(lock_t *lockp, pl_t pl) {
  return LOCK(lockp, pl);
} /* cpumtr_lock() */


/*******************************************************
* cpumtr_getsample()
*
* samples the CPU meters into cc_sample and updates cc_start_*.
* The cc_lock should be held on entry and exit.
*/
static int
cpumtr_getsample(processorid_t cpu_id, cpumtr_tracepoint_t tracepoint,
    ushort_t qual) {
  cpumtr_sample_t *sample;
  cpumtr_lwp_t *lwp;
  uint_t isetup;
  cpumtr_state_t state;
  cpumtr_cpu_t *cpu;
  pl_t plx;

  /* Check the CPU id. */
  if (cpu_id < 0 || cpu_id >= cpumtr_num_cpus) {
    cmn_err(CE_WARN, "CPUMTR:  getsample(cpu_id=%d)\n", cpu_id);
    return EINVAL;
  }
  cpu = &(cpumtr_cpu[cpu_id]);

  /* Check there is a current sample. */
  sample = cpu->cc_sample;
  if (sample == NULL) {
    cmn_err(CE_WARN, "CPUMTR:  getsample(sample=NULL)\n");
    return EIO;
  }

  /* Check the state. */
  state = cpu->cc_state;
  if (state >= CPUMTR_NUM_STATES) {
    cmn_err(CE_WARN, "CPUMTR:  getsample(state=%d)\n", cpu_id);
    return EINVAL;
  }

  sample->ca_start_time = cpu->cc_start_time;
  sample->ca_start_lbolt = cpu->cc_start_lbolt;
  sample->ca_pid = cpu->cc_pid;
  sample->ca_lwpid = cpu->cc_lwpid;
  sample->ca_state = state;
  sample->ca_tracepoint = tracepoint;
  sample->ca_qual = qual;

  if (cpu->cc_setup == NULL) {
    sample->ca_setup_valid_flag = FALSE;
  } else {
    sample->ca_setup_valid_flag = TRUE;
    cpumtr_copy_setup(&(sample->ca_setup), cpu->cc_setup, 0, CPUMTR_NUM_METERS);
  }

  switch (cpu->cc_cputype) {
    case CPUMTR_P6:
      cpumtr_p6_getsample(cpu, sample);
      break;

    default:
      cmn_err(CE_WARN, "CPUMTR:  getsample(cputype=%u)\n", cpu->cc_cputype);
      return ENXIO;
  }

  /* Get pointer to per-LWP meters. */
  lwp = NULL;
  if (state < CPUMTR_NUM_LWP_STATES) {
    plx = RW_WRLOCK(cpumtr_lwp_lock, plhi);
    lwp = cpumtr_get_lwp(cpu->cc_pid, cpu->cc_lwpid);
    if (lwp == NULL) {
      /* We lose this LWP sample due to memory shortage. */
      RW_UNLOCK(cpumtr_lwp_lock, plx);
      cpu->cc_lwp_samples_lost++;
    }
  }

  /* Update per-CPU and per-LWP virtual meters. */
  {
    cpumtr_uint64_t diff;

    for (isetup = 0; isetup < CPUMTR_NUM_METERS; isetup++) {
      cpumtr_meter_t *meter;

      meter = &(cpu->cc_meter[isetup]);
      diff = lsub(sample->ca_count[isetup], meter->cm_start_count);
      meter->cm_count[state] = ladd(diff, meter->cm_count[state]);
      if (lwp != NULL) {
        lwp->cl_count[state][isetup] = ladd(diff, lwp->cl_count[state][isetup]);
      }
    }

    diff = lsub(sample->ca_end_time, cpu->cc_start_time);
    cpu->cc_time[state] = ladd(diff, cpu->cc_time[state]);
    if (lwp != NULL) {
      lwp->cl_time[state] = ladd(diff, lwp->cl_time[state]);
      lwp->cl_cpu = cpu_id;
      RW_UNLOCK(cpumtr_lwp_lock, plx);
    }
  }

  /* Save ending values for use as start values in the next sample. */
  cpu->cc_start_time = sample->ca_end_time;
  cpu->cc_start_lbolt = sample->ca_end_lbolt;
  for (isetup = 0; isetup < CPUMTR_NUM_METERS; isetup++)
    cpu->cc_meter[isetup].cm_start_count = sample->ca_count[isetup];

  cpu->cc_num_samples_taken++;

  /* If tracing, advance the write pointer. */
  if (cpu->cc_trace_buffer_size > 1) {
    cpumtr_process_id_t process;

    if (cpu->cc_num_samples_taken < cpu->cc_trace_buffer_size) {
      /* not full yet */
      cpu->cc_num_samples_kept++;
    }

    cpu->cc_sample++;
    if (cpu->cc_sample == cpu->cc_trace_end) {
      /* output pointer wraps around */
      cpu->cc_sample = cpu->cc_trace;
    }

    /* If a process is waiting for the samples, consider waking it up. */
    process = cpu->cc_trace_process;
    if (process != CPUMTR_PROCESS_NONE
     && cpu->cc_num_samples_kept >= cpu->cc_block_until) {
      SV_SIGNAL(cpumtr_process[process].cp_svp, 0);
    }
  }

  return CPUMTR_NO_ERROR;
} /* cpumtr_getsample() */


/*******************************************************
* cpumtr_barrier()
*
*/
static void
cpumtr_barrier(cpumtr_process_id_t process) {
  pl_t plx;
  cpumtr_process_t *proc;

  /* Check that process is legal. */
  if (process < 0 || process > CPUMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CPUMTR:  barrier(%u)\n", process);
    return;
  }
  proc = &(cpumtr_process[process]);

  /* Get process lock. */
  plx = cpumtr_lock(proc->cp_lock, plhi);

  /* Decrement count and, if zero, wake up client thread. */
  proc->cp_count--;
  if (proc->cp_count < 0) {
    cmn_err(CE_WARN, "CPUMTR:  barrier(count = %u)\n", proc->cp_count);
    
  } else if (proc->cp_count == 0) {
    SV_SIGNAL(proc->cp_svp, 0);
  }

  /* Release process lock. */
  UNLOCK(proc->cp_lock, plx);
} /* cpumtr_barrier() */

/*******************************************************
* cpumtr_start_cpu2()
*
* timeout function used to determine CPU clock frequency.
*/
static void
cpumtr_start_cpu2(void *ptr) {
  dl_t end_time;
  uint_t cpu_cycles;
  uint_t elapsed_msec;
  cpumtr_cpu_t *cpu;

  cpu = (cpumtr_cpu_t *)ptr;


  /* Get ending timestamp. */
  switch (cpu->cc_cputype) {
    case CPUMTR_P6:
      cpumtr_p6_readtsc(&end_time);
      break;
    default:
      cmn_err(CE_WARN, "CPUMTR:  start_cpu2(cputype=%u)\n", cpu->cc_cputype);
      cpu->cc_start_timer = CPUMTR_TOID_NONE;
      cpumtr_barrier(CPUMTR_PROCESS_GLOBAL);
      return;
  }

  /* Calculate CPU clock frequency. */
  cpu_cycles = end_time.dl_lop - cpu->cc_start_time.dl_lop;
  elapsed_msec = drv_hztousec(TICKS_SINCE(cpu->cc_start_lbolt))/1000;
  if (elapsed_msec == 0) {
    cmn_err(CE_WARN, "CPUMTR:  start_cpu2(elapsed_msec=0)\n");
  } else {
    cpu->cc_hz = 1000*(cpu_cycles/elapsed_msec);
  }

  cpu->cc_start_timer = CPUMTR_TOID_NONE;
  cpumtr_barrier(CPUMTR_PROCESS_GLOBAL);
} /* cpumtr_start_cpu2() */


/*******************************************************
* cpumtr_start_cpu()
*
* determines CPU type and clock frequency.
*/
static void
cpumtr_start_cpu(void *ptr) {
  cpumtr_cpu_t *cpu;

  cpu = (cpumtr_cpu_t *)ptr;

  /* Get CPU type. */
  cpu->cc_cputype = cpumtr_cputype();

  /* Get starting timestamp. */
  switch (cpu->cc_cputype) {
    case CPUMTR_P6:
      cpumtr_p6_readtsc(&(cpu->cc_start_time));
      cpumtr_p6_start_count(cpu);
      break;
  
    default:
      cmn_err(CE_WARN, "CPUMTR:  start_cpu(cputype=%u)\n", cpu->cc_cputype);
      cpu->cc_start_timer = CPUMTR_TOID_NONE;
      cpumtr_barrier(CPUMTR_PROCESS_GLOBAL);
      return;
  }

  /* Get starting lbolt. */
  cpu->cc_start_lbolt = TICKS();

  /* Schedule second function. */
  cpu->cc_start_timer = dtimeout(cpumtr_start_cpu2, (void *)cpu,
    drv_usectohz(CPUMTR_START_USEC), plhi, cpu->cc_cpu_id);  
  if (cpu->cc_start_timer == CPUMTR_TOID_NONE) {
    cmn_err(CE_WARN,
      "CPUMTR:  start_cpu(%u) unable to schedule\n", cpu->cc_cpu_id);
  }
} /* cpumtr_start_cpu() */


/*******************************************************
* cpumtr_update_cpu()
*
* update all virtual meters on a particular CPU.
*/
static void
cpumtr_update_cpu(void *ptr) {
  processorid_t cpu_id;
  cpumtr_cpu_t *cpu;
  pl_t plx;
  cpumtr_process_id_t process;

  cpu = (cpumtr_cpu_t *)ptr;
  
  cpu_id = cpu->cc_cpu_id;
  
  /* Get CPU lock. */
  plx = cpumtr_lock(cpu->cc_lock, plhi);

  /* Save the index of the update process. */
  process = cpu->cc_update_process;

  cpu->cc_retcode = cpumtr_getsample(cpu_id, cpu->cc_update_tracepoint, 0);

  /* Release CPU lock. */
  UNLOCK(cpu->cc_lock, plx);

  cpumtr_barrier(process);
} /* cpumtr_update_cpu() */


/*******************************************************
* cpumtr_timeout_cpu()
*
* update all virtual meters on a particular CPU on timeout.
*/
static void
cpumtr_timeout_cpu(void *ptr) {
  processorid_t cpu_id;
  cpumtr_cpu_t *cpu;
  pl_t plx;

  cpu = (cpumtr_cpu_t *)ptr;
  
  cpu_id = cpu->cc_cpu_id;
  
  /* Get CPU lock. */
  plx = cpumtr_lock(cpu->cc_lock, plhi);

  (void)cpumtr_getsample(cpu_id, CPUMTR_AT_TIMEOUT, 0);

  if (cpu->cc_max_interval == 0) {
    cpu->cc_timeout_timer = CPUMTR_TOID_NONE;

  } else {
    if (cpu->cc_max_interval & TO_PERIODIC) {
      cmn_err(CE_WARN, "CPUMTR:  timeout_cpu(max_interval=%ld)\n",
        cpu->cc_max_interval);
      UNLOCK(cpu->cc_lock, plx);
      return;
    }
    /* Schedule the next update. */
    cpu->cc_timeout_timer = dtimeout(cpumtr_timeout_cpu,
        (void *)cpu, cpu->cc_max_interval, plhi, cpu_id);  
    if (cpu->cc_timeout_timer == CPUMTR_TOID_NONE) {
      cmn_err(CE_WARN, "CPUMTR:  timeout_cpu(unable to schedule timeout)\n");
    }
  }
  
  /* Release CPU lock. */
  UNLOCK(cpu->cc_lock, plx);
} /* cpumtr_timeout_cpu() */


/*******************************************************
* cpumtr_update_interval()
*
* recalculates the update interval for the CPU.
*/
static void
cpumtr_update_interval(cpumtr_cpu_t *cpu) {  
  uint_t min_usec;
  bool_t min_set;
  uint_t isetup;

  min_set = FALSE;
  for (isetup = 0; isetup < CPUMTR_NUM_METERS; isetup++) {
    uint_t usec;

    if (cpu->cc_meter[isetup].cm_setup_flags != CPUMTR_SETUP_DEACTIVATE) {
      usec = cpu->cc_setup->css_max_interval[isetup];
      
      if (!min_set || usec < min_usec) {
        min_set = TRUE;
        min_usec = usec;
      }
    }
  }

  if (min_set) {
    cpu->cc_max_interval = drv_usectohz(min_usec) - 1;
    if (cpu->cc_max_interval < 1)
      cpu->cc_max_interval = CPUMTR_ASAP;
  } else {
    cpu->cc_max_interval = 0;
  }
} /* cpumtr_update_interval() */


/*******************************************************
* cpumtr_setup_cpu_setup()
*
* sets up a cpu_setup for the current CPU.
* cc_lock is held on entry and exit.
*/
static int
cpumtr_setup_cpu_setup(cpumtr_cpu_t *cpu, cpumtr_cpu_setup_t *cpu_setup) {
  cpumtr_meter_id_t meter_id;
  int num_setups;
  uint_t first_setup;
  uint_t end_setup;
  ushort setup_flags;
  cpumtr_sample_setup_t *setup_in;
  cpumtr_sample_setup_t *setup;

  meter_id = cpu_setup->cps_meter_id;
  num_setups = cpumtr_num_setups(meter_id, &first_setup);
  end_setup = first_setup + num_setups;
  setup_flags = cpu_setup->cps_setup_flags;
  setup_in = cpu_setup->cps_sample_setup;

  setup = cpu->cc_setup;
  if (setup == NULL) {
    size_t setup_bytes;
    int retcode;

    /* Allocate sample_setup for this CPU. */
    setup_bytes = sizeof(cpumtr_sample_setup_t);
    setup = kmem_alloc(setup_bytes, KM_NOSLEEP);
    if (setup == NULL) {
      cmn_err(CE_WARN, "CPUMTR:  setup_cpu(setup_bytes=%u)\n", setup_bytes);
      return ENOMEM;
    }

    retcode = cpumtr_deactivate_setup(setup, 0,
      CPUMTR_NUM_METERS, cpu->cc_cputype);
    if (retcode != CPUMTR_NO_ERROR) {
      kmem_free(setup, setup_bytes);
      return retcode;
    }

    cpu->cc_setup_bytes = setup_bytes;
    cpu->cc_setup = setup;
  }

  /* Make changes to the CPU setup. */
  if (setup_flags != CPUMTR_SETUP_DEACTIVATE) {
    /* activate metrics */
    int isetup;

    cpumtr_copy_setup(setup, setup_in, first_setup, end_setup);

    for (isetup = first_setup; isetup < end_setup; isetup++) {
      cpumtr_meter_t *meter;

      meter = &(cpu->cc_meter[isetup]);
      meter->cm_setup_flags = setup_flags;
      meter->cm_activating_user = meter->cm_reserving_user;
    }

  } else {
    /* deactivate metrics */
    int retcode;
    int isetup;

    for (isetup = first_setup; isetup < end_setup; isetup++)
      cpu->cc_meter[isetup].cm_setup_flags = CPUMTR_SETUP_DEACTIVATE;

    retcode = cpumtr_deactivate_setup(setup, first_setup,
      end_setup, cpu->cc_cputype);
    if (retcode != CPUMTR_NO_ERROR) {
      return retcode;
    }
  }

  return CPUMTR_NO_ERROR;
} /* cpumtr_setup_cpu_setup() */


/*******************************************************
* cpumtr_setup_cpu()
*
* sets up the current CPU.
* cc_setup_process and cc_cpu_setup must be set on entry.
*/
static void
cpumtr_setup_cpu(void *ptr) {
  cpumtr_cpu_t *cpu;
  processorid_t cpu_id;
  pl_t plx;
  cpumtr_process_id_t process;
  uint_t old_trace_buffer_size;
  uint_t new_trace_buffer_size;
  cpumtr_process_t *proc;
  uint_t isetup;

  cpu = (cpumtr_cpu_t *)ptr;

  cpu_id = cpu->cc_cpu_id;
  if (cpu != &(cpumtr_cpu[cpu_id])) {
    cmn_err(CE_WARN, "CPUMTR:  setup_cpu(cpu=%lx)\n", (long)cpu);
    return;
  }

  plx = cpumtr_lock(cpu->cc_lock, plhi);
  process = cpu->cc_setup_process;
  if (process < 0 || process > CPUMTR_MAX_PROCESSES) {
    cpu->cc_retcode = EINVAL;
    UNLOCK(cpu->cc_lock, plx);
    cmn_err(CE_WARN, "CPUMTR:  setup_cpu(process=%u)\n", process);
    return;
  }

  proc = &(cpumtr_process[process]);

  /*  Revise the CPU setup by applying cpu_setups from the ioctl.  */
  for (isetup = 0; isetup < proc->cp_num_cpu_setups; isetup++) {
    cpumtr_cpu_setup_t *cpu_setup;
    cpumtr_meter_id_t meter_id;

    cpu_setup = &(proc->cp_cpu_setups[isetup]);
    meter_id = cpu_setup->cps_meter_id;
    if (meter_id.cmi_cpu_id == cpu_id) {
      int retcode;

      retcode = cpumtr_setup_cpu_setup(cpu, cpu_setup);
      if (retcode != CPUMTR_NO_ERROR) {
        cpu->cc_retcode = retcode;
        UNLOCK(cpu->cc_lock, plx);
        return;
      }
      new_trace_buffer_size = cpu_setup->cps_sampletable_size;
    }
  }

  /* Reallocate the trace buffer as necessary.  */
  old_trace_buffer_size = cpu->cc_trace_buffer_size;
  if (new_trace_buffer_size > old_trace_buffer_size) {
    cpumtr_sample_t *trace;

    trace = kmem_alloc(new_trace_buffer_size*sizeof(cpumtr_sample_t),
      KM_NOSLEEP);
    if (trace == NULL) {
      cpu->cc_retcode = ENOMEM;
      UNLOCK(cpu->cc_lock, plx);
      cmn_err(CE_WARN, "CPUMTR:  setup_cpu(trace_bytes=%u)\n",
        new_trace_buffer_size*sizeof(cpumtr_sample_t));
      return;
    }

    kmem_free(cpu->cc_trace, old_trace_buffer_size*sizeof(cpumtr_sample_t));

    cpu->cc_trace_buffer_size = new_trace_buffer_size;
    cpu->cc_trace = trace;
    cpu->cc_trace_end = trace + new_trace_buffer_size;
    cpu->cc_sample = trace;
    cpu->cc_num_samples_taken = 0;
    cpu->cc_num_samples_kept = 0;
  }

  /* Apply the revised CPU setup to the hardware. */
  switch (cpu->cc_cputype) {
    case CPUMTR_P6:
      cpumtr_p6_setup(cpu->cc_setup);
      break;

    default:
      cpu->cc_retcode = ENXIO;
      UNLOCK(cpu->cc_lock, plx);
      cmn_err(CE_WARN, "CPUMTR:  setup_cpu(cputype=%u)\n", cpu->cc_cputype);
      return;
  }

  /* Calculate and apply new update interval. */
  {
    clock_t old_interval;
    clock_t new_interval;

    old_interval = cpu->cc_max_interval;
    cpumtr_update_interval(cpu);
    new_interval = cpu->cc_max_interval;

    /* cmn_err(CE_CONT, "CPUMTR:  new interval = %ld\n", new_interval); */
    if (new_interval > 0
     && (old_interval <= 0 || new_interval < old_interval)) {
      toid_t toid;
      
      toid = cpu->cc_timeout_timer; 
      if (toid != CPUMTR_TOID_NONE) {
        cpu->cc_timeout_timer = CPUMTR_TOID_NONE;
        UNLOCK(cpu->cc_lock, plx);
        untimeout(toid); 
        plx = cpumtr_lock(cpu->cc_lock, plhi);
      } 

      /* Schedule the next timeout. */
      cpu->cc_timeout_timer = dtimeout(cpumtr_timeout_cpu,
        (void *)cpu, cpu->cc_max_interval, plhi, cpu_id);  
      if (cpu->cc_timeout_timer == CPUMTR_TOID_NONE) {
        cpu->cc_retcode = ENXIO;
        UNLOCK(cpu->cc_lock, plx);
        cmn_err(CE_WARN, "CPUMTR:  setup_cpu(unable to schedule timeout)\n");
      }
    }
  }

  /* This setup task is complete. */
  cpu->cc_setup_process = CPUMTR_PROCESS_NONE;

  UNLOCK(cpu->cc_lock, plx);
  cpumtr_barrier(process);
} /* cpumtr_setup_cpu() */


/*******************************************************
* cpumtr_watchdog()
*
* called when a task has hung because a CPU is offline.
*/
static void
cpumtr_watchdog(int process) {
  pl_t plx;
  cpumtr_process_t *proc;

  /* Check that process is legal. */
  if (process < 0 || process > CPUMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CPUMTR:  watchdog(%u)\n", process);
    return;
  }
  proc = &(cpumtr_process[process]);

  /* Get process lock. */
  plx = cpumtr_lock(proc->cp_lock, plhi);

  /* Clear watchdog. */
  proc->cp_watchdog = CPUMTR_TOID_NONE;

  /* Wake up hung process. */
  SV_SIGNAL(proc->cp_svp, 0);

  /* Release lock. */
  UNLOCK(proc->cp_lock, plx);
} /* cpumtr_watchdog() */


/*******************************************************
* cpumtr_join()
*
* join spawned tasks, with watchdog.
*
* Unlocks the cp_lock, which must be held on entry.
*/
static int
cpumtr_join(cpumtr_process_id_t process, pl_t plx,
  cpumtr_tracepoint_t point) {
  cpumtr_process_t *proc;
  processorid_t cpu_id;

  /* Check that process is legal. */
  if (process < 0 || process > CPUMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CPUMTR:  join(%u)\n", process);
    return EINVAL;
  }
  proc = &(cpumtr_process[process]);

  if (proc->cp_count < 1) {
    UNLOCK(proc->cp_lock, plx);
    return CPUMTR_NO_ERROR;
  }

  /* kick off watchdog */
  proc->cp_watchdog = itimeout(cpumtr_watchdog,
    (void *)process, drv_usectohz(CPUMTR_WATCHDOG_USEC), plhi);

  /* if watchdog not scheduled */
  if (proc->cp_watchdog == CPUMTR_TOID_NONE) {
    UNLOCK(proc->cp_lock, plx);
    cmn_err(CE_WARN, "CPUMTR:  join(cp_watchdog=0)\n");
    for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
      cpumtr_cpu_t *cpu;

      cpu = &(cpumtr_cpu[cpu_id]);
      switch (point) {
        case CPUMTR_AT_START:
          untimeout(cpu->cc_start_timer); /* must be done outside lock */
          cpu->cc_start_timer = CPUMTR_TOID_NONE;
          break;

        case CPUMTR_AT_SETUP:
          untimeout(cpu->cc_setup_timer); /* must be done outside lock */
          cpu->cc_setup_timer = CPUMTR_TOID_NONE;
          break;

        case CPUMTR_AT_UPDATE:
          untimeout(cpu->cc_update_timer); /* must be done outside lock */
          cpu->cc_update_timer = CPUMTR_TOID_NONE;
          break;

        default:
          cmn_err(CE_WARN, "CPUMTR:  join(point=%d)\n", point);
          return EINVAL;
      }
    }
    proc->cp_count = 0;
    return ENXIO;
  }

  /* Wait for tasks to complete. */
  SV_WAIT(proc->cp_svp, prilo, proc->cp_lock);

  if (proc->cp_watchdog != CPUMTR_TOID_NONE) {
    /* All CPUs beat the watchdog; cancel it. */
    untimeout(proc->cp_watchdog);
    proc->cp_watchdog = CPUMTR_TOID_NONE;
    return CPUMTR_NO_ERROR;
  } 

  for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
    cpumtr_cpu_t *cpu;

    cpu = &(cpumtr_cpu[cpu_id]);
    switch (point) {
      case CPUMTR_AT_START:
        if (cpu->cc_start_timer != CPUMTR_TOID_NONE) {
          /* cmn_err(CE_WARN, "CPUMTR:  cpu%d is unreachable, may be offline\n",
            cpu_id); */
          untimeout(cpu->cc_start_timer);
          cpu->cc_start_timer = CPUMTR_TOID_NONE;
        }
        break;

      case CPUMTR_AT_SETUP:
        if (cpu->cc_setup_timer != CPUMTR_TOID_NONE) {
          cmn_err(CE_WARN, "CPUMTR:  join(watchdog setup cpu_id=%d)\n", cpu_id);
          untimeout(cpu->cc_setup_timer);
          cpu->cc_setup_timer = CPUMTR_TOID_NONE;
        }
        break;

      case CPUMTR_AT_UPDATE:
        if (cpu->cc_update_timer != CPUMTR_TOID_NONE) {
          cmn_err(CE_WARN, "CPUMTR:  join(watchdog update cpu_id=%d)\n",
            cpu_id);
          untimeout(cpu->cc_update_timer);
          cpu->cc_update_timer = CPUMTR_TOID_NONE;
        }
        break;

      default:
        cmn_err(CE_WARN, "CPUMTR:  join(point=%d)\n", point);
    }
  }
  proc->cp_count = 0;

  return (point == CPUMTR_AT_START) ? CPUMTR_NO_ERROR : ETIME;
} /* cpumtr_join() */


/*******************************************************
* cpumtr_update()
*
* update all virtual meters.
*/
static int
cpumtr_update(cpumtr_process_id_t process, cpumtr_tracepoint_t tracepoint) {
  cpumtr_process_t *proc;
  processorid_t cpu_id;
  pl_t plx;

  proc = &(cpumtr_process[process]);
  plx = cpumtr_lock(proc->cp_lock, plhi);

  proc->cp_count = cpumtr_num_cpus;
  for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
    cpumtr_cpu_t *cpu;
    pl_t ply;

    cpu = &(cpumtr_cpu[cpu_id]);
    if (cpu->cc_cputype == CPUMTR_NOT_SUPPORTED) {
      /* skip unreachable CPU */
      proc->cp_count--;
      continue;
    }
    ply = cpumtr_lock(cpu->cc_lock, plhi);
    cpu->cc_update_process = process;
    cpu->cc_update_tracepoint = tracepoint;
    cpu->cc_update_timer = dtimeout(cpumtr_update_cpu,
      (void *)cpu, CPUMTR_ASAP, plhi, cpu_id);  
    if (cpu->cc_update_timer == CPUMTR_TOID_NONE) {
      cmn_err(CE_WARN, "CPUMTR:  update(no timer on CPU %d)\n", cpu_id);
      proc->cp_count--;
    }
    UNLOCK(cpu->cc_lock, ply);
  }

  /* Wait for CPU startups to complete, even if there were errors. */
  /* This releases the cp_lock. */
  return cpumtr_join(process, plx, CPUMTR_AT_UPDATE);
} /* cpumtr_update() */


/*******************************************************
* cpumtr_hook()
*
* reads a sample on a context switch, trap, or system call.
* The return value is always ignored.
*/
static int
cpumtr_hook(struct hook_args *args) {
  pl_t plx;
  cpumtr_cpu_t *cpu;
  processorid_t cpu_id;
  cpumtr_tracepoint_t point;

  cpu_id = args->ha_engno;
  point = (cpumtr_tracepoint_t)args->ha_args;
  
  /* Check that CPU id is legal. */
  if (cpu_id < 0 || cpu_id >= cpumtr_num_cpus) {
    cmn_err(CE_WARN, "CPUMTR:  hook(cpu_id=%d)\n", cpu_id);
    return EINVAL;
  }

  /* Get CPU lock. */
  cpu = &(cpumtr_cpu[cpu_id]);
  plx = cpumtr_lock(cpu->cc_lock, plhi);

  cpu->cc_pid = args->ha_pid;
  cpu->cc_lwpid = args->ha_lwpid;

  if (args->ha_pid == -1) {
    cpu->cc_state = CPUMTR_STATE_IDLE;

  } else switch (point) {
    case CPUMTR_AT_SYSENTRY:
    case CPUMTR_AT_TRAPENTRY:
      cpu->cc_state = CPUMTR_STATE_USER;
      break;
    case CPUMTR_AT_SYSEXIT:
    case CPUMTR_AT_TRAPEXIT:
      cpu->cc_state = CPUMTR_STATE_SYSTEM;
      break;
  }

  /* Sample the meters -- unless CPU is marked unreachable. */
  if (cpu->cc_cputype != CPUMTR_NOT_SUPPORTED) {
    (void)cpumtr_getsample(cpu_id, point, args->ha_qual);
  }

  switch (point) {
    case CPUMTR_AT_SYSENTRY:
    case CPUMTR_AT_TRAPENTRY:
      cpu->cc_state = CPUMTR_STATE_SYSTEM;
      break;
    case CPUMTR_AT_SYSEXIT:
    case CPUMTR_AT_TRAPEXIT:
      cpu->cc_state = CPUMTR_STATE_USER;
      break;
  }

  /* Release CPU lock. */
  UNLOCK(cpu->cc_lock, plx);
  
  /* The return value is ignored. */
  return CPUMTR_NO_ERROR;
} /* cpumtr_hook() */


/*******************************************************
* cpumtr_install_hooks()
*
* installs or removes the system hooks.
*/
static void
cpumtr_install_hooks(bool_t install) {
  static cpumtr_hooks_installed = FALSE;

  if (install && !cpumtr_hooks_installed) {
    cpumtr_hooks_installed = TRUE;

    lwp_callback(SWTCH_OUT2_ATTACH, cpumtr_hook,
      (void *)CPUMTR_AT_SWTCH); 
    lwp_callback(SYSENTRY_ATTACH, cpumtr_hook,
      (void *)CPUMTR_AT_SYSENTRY); 
    lwp_callback(SYSEXIT_ATTACH, cpumtr_hook,
      (void *)CPUMTR_AT_SYSEXIT); 
    lwp_callback(TRAPENTRY_ATTACH, cpumtr_hook,
      (void *)CPUMTR_AT_TRAPENTRY); 
    lwp_callback(TRAPEXIT_ATTACH, cpumtr_hook,
      (void *)CPUMTR_AT_TRAPEXIT);

  } else if (cpumtr_hooks_installed && !install) {
    cpumtr_hooks_installed = FALSE;

    lwp_callback(SWTCH_OUT2_DETACH, cpumtr_hook,
      (void *)CPUMTR_AT_SWTCH); 
    lwp_callback(SYSENTRY_DETACH, cpumtr_hook,
      (void *)CPUMTR_AT_SYSENTRY); 
    lwp_callback(SYSEXIT_DETACH, cpumtr_hook,
      (void *)CPUMTR_AT_SYSEXIT); 
    lwp_callback(TRAPENTRY_DETACH, cpumtr_hook,
      (void *)CPUMTR_AT_TRAPENTRY); 
    lwp_callback(TRAPEXIT_DETACH, cpumtr_hook,
      (void *)CPUMTR_AT_TRAPEXIT); 
  }
} /* cpumtr_install_hooks() */


/*******************************************************
* cpumtr_rdstats_cpu()
* 
* copies out the CPU stats to the user process.
*/
static int
cpumtr_rdstats_cpu(cpumtr_rdstats_t *prdstats) {
  int retcode;
  processorid_t cpu_id;
  cpumtr_cpu_stats_t *cpu_stats; /* array of CPU stats */
  size_t cpustats_bytes; /* size of cpu_stats in bytes */
  pl_t plx;

  /* Check that the number of CPUs is correct. */
  if (prdstats->cr_num_cpus != cpumtr_num_cpus) {
    cmn_err(CE_WARN, "CPUMTR:  rdstats_cpu(num_cpu=%u)\n",
      prdstats->cr_num_cpus);
    return EINVAL;
  }
    
  /* Allocate array of CPU stats. */
  cpustats_bytes = prdstats->cr_num_cpus*sizeof(cpumtr_cpu_stats_t);
  cpu_stats = kmem_alloc(cpustats_bytes, KM_SLEEP);
  if (cpu_stats == NULL) {
    cmn_err(CE_WARN, "CPUMTR:  rdstats_cpu(cpustats_bytes=%u)\n",
      cpustats_bytes);
    return ENOMEM;
  }

  /* Copy CPU stats into array. */
  for (cpu_id = 0; cpu_id < (processorid_t)prdstats->cr_num_cpus; cpu_id++) {
    cpumtr_cpu_t *cpu;
    cpumtr_cpu_stats_t *stats;
    cpumtr_state_t istate;
    u_int isetup;

    stats = &(cpu_stats[cpu_id]);
    cpu = &(cpumtr_cpu[cpu_id]);

    /* Copy constant portion of cpustats. */
    stats->ccs_cputype = cpu->cc_cputype;
    stats->ccs_hz = cpu->cc_hz;

    /* Copy variable portion of cpustats. */
    plx = cpumtr_lock(cpu->cc_lock, plhi); 
    for (istate = 0; istate < CPUMTR_NUM_STATES; istate++) {
      for (isetup = 0; isetup < CPUMTR_NUM_METERS; isetup++) {
        stats->ccs_count[istate][isetup] =
          cpu->cc_meter[isetup].cm_count[istate];
      }
      stats->ccs_time[istate] = cpu->cc_time[istate];
    }
    if (cpu->cc_setup != NULL) {
      for (isetup = 0; isetup < CPUMTR_NUM_METERS; isetup++) {
        cpumtr_meter_t *meter;

        meter = &(cpu->cc_meter[isetup]);
        stats->ccs_setup_flags[isetup] = meter->cm_setup_flags;
        stats->ccs_userid[isetup] = meter->cm_activating_user;
      }
      cpumtr_copy_setup(&(stats->ccs_setup), cpu->cc_setup,
        0, CPUMTR_NUM_METERS);
    } else {
      for (isetup = 0; isetup < CPUMTR_NUM_METERS; isetup++) {
        stats->ccs_setup_flags[isetup] = CPUMTR_SETUP_DEACTIVATE;
      }
      bzero(&(stats->ccs_setup), sizeof(cpumtr_sample_setup_t));
    }
    UNLOCK(cpu->cc_lock, plx); 
  }

  /* Copyout and free the CPU stats. */
  retcode = copyout(cpu_stats, prdstats->cr_cpu_stats, cpustats_bytes);
  kmem_free(cpu_stats, cpustats_bytes);
  if (retcode != CPUMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CPUMTR:  rdstats_cpu(cr_cpu_stats=0x%lx)\n",
      (ulong_t)prdstats->cr_cpu_stats);
    return EFAULT;
  }

  return CPUMTR_NO_ERROR;
} /* cpumtr_rdstats_cpu() */


/*******************************************************
* cpumtr_rdstats_lwp()
* 
* copies out the LWP stats to the user process.
*/
static int
cpumtr_rdstats_lwp(cpumtr_rdstats_t *prdstats) {
  int retcode;
  cpumtr_lwp_stats_t *lwp_stats; /* array of LWP stats */
  size_t lwpstats_bytes; /* size of lwp_stats in bytes */
  pl_t plx;
  cpumtr_lwp_t *lwp;
  uint_t ilwp;
  uint_t num_lost;

  /* Check that the number of LWPs is legal. */
  if (prdstats->cr_num_lwps < 1
   || prdstats->cr_num_lwps > CPUMTR_MAX_NUM_LWPS) {
    cmn_err(CE_WARN, "CPUMTR:  rdstats_lwp(num_lwp=%u)\n",
      prdstats->cr_num_lwps);
    return EINVAL;
  }
    
  /* Allocate array of LWP stats. */
  lwpstats_bytes = prdstats->cr_num_lwps*sizeof(cpumtr_lwp_stats_t);
  lwp_stats = kmem_alloc(lwpstats_bytes, KM_SLEEP);
  if (lwp_stats == NULL) {
    cmn_err(CE_WARN, "CPUMTR:  rdstats_lwp(lwpstats_bytes=%u)\n",
      lwpstats_bytes);
    return ENOMEM;
  }

  /* Copy LWP stats into array. */
  plx = RW_RDLOCK(cpumtr_lwp_lock, plhi); 
  for (ilwp = 0; ilwp < cpumtr_num_lwps; ilwp++) {
    cpumtr_lwp_stats_t *stats; /* current lwpstat */
    cpumtr_state_t istate;

    if (ilwp >= prdstats->cr_num_lwps)
      break;
    stats = &(lwp_stats[ilwp]);
    lwp = &(cpumtr_lwp_mem[ilwp]);

    /* Copy info to lwpstats. */
    stats->cls_pid = lwp->cl_pid;
    stats->cls_lwpid = lwp->cl_lwpid;
    stats->cls_cpu = lwp->cl_cpu;
    for (istate = 0; istate < CPUMTR_NUM_LWP_STATES; istate++) {
      uint_t isetup;

      for (isetup = 0; isetup < CPUMTR_NUM_METERS; isetup++) {
        stats->cls_count[istate][isetup] = lwp->cl_count[istate][isetup];
      }
      stats->cls_time[istate] = lwp->cl_time[istate];
    }
  }
  num_lost = cpumtr_num_lwps - ilwp;
  RW_UNLOCK(cpumtr_lwp_lock, plx); 

  /* Copyout and free the LWP stats. */
  retcode = copyout(lwp_stats, prdstats->cr_lwp_stats, lwpstats_bytes);
  kmem_free(lwp_stats, lwpstats_bytes);
  if (retcode != CPUMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CPUMTR:  rdstats_lwp(cr_lwp_stats=0x%lx)\n",
      (ulong_t)prdstats->cr_lwp_stats);
    return EFAULT;
  }

  prdstats->cr_num_lwps = ilwp;
  prdstats->cr_num_lwps_lost = num_lost;

  return CPUMTR_NO_ERROR;
} /* cpumtr_rdstats_lwp() */


/*******************************************************
* cpumtr_rdstats()
* 
* implements CPUMTR_CMD_RDSTATS, copying out LWP/CPU stats
* to the user process.
*/
static int
cpumtr_rdstats(caddr_t arg, cpumtr_process_id_t process) {
  int retcode;
  cpumtr_rdstats_t rdstats; /* copy of user structure */

  /* Copyin user rdstats structure. */
  retcode = copyin(arg, &rdstats, sizeof(rdstats));
  if (retcode != CPUMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CPUMTR:  rdstats(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  if (rdstats.cr_cpu_stats != NULL || rdstats.cr_lwp_stats != NULL) {
    retcode = cpumtr_update(process, CPUMTR_AT_RDSTATS);
    if (retcode != CPUMTR_NO_ERROR)
      return retcode;
  }

  if (rdstats.cr_cpu_stats == NULL) {
    /* Suggest a number of CPUs, but don't copy CPU stats. */
    rdstats.cr_num_cpus = cpumtr_num_cpus;

  } else {
    retcode = cpumtr_rdstats_cpu (&rdstats);
    if (retcode != CPUMTR_NO_ERROR)
      return retcode;
  }

  if (rdstats.cr_lwp_stats == NULL) {
    /* Suggest a number of LWPs, but don't copy LWP stats. */
    rdstats.cr_num_lwps = cpumtr_num_lwps;

  } else {
    retcode = cpumtr_rdstats_lwp (&rdstats);
    if (retcode != CPUMTR_NO_ERROR)
      return retcode;
  }

  retcode = copyout(&rdstats, arg, sizeof(rdstats));
  if (retcode != CPUMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CPUMTR:  rdstats(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  return CPUMTR_NO_ERROR;
} /* cpumtr_rdstats() */


/*******************************************************
* cpumtr_rdsamples()
* 
* implements CPUMTR_CMD_RDSAMPLES, copying out a trace of
* samples to the user process.
*/
static int
cpumtr_rdsamples(caddr_t arg, cpumtr_process_id_t process) {
  int retcode;
  cpumtr_rdsamples_t rdsamples;         /* copy of user structure */
  cpumtr_proc_rdsamples_t *cpu_samples; /* array of CPU samples */
  cpumtr_sample_t *trace;
  size_t cpusamples_bytes;      /* size of cpu_samples in bytes */
  size_t trace_bytes;      /* size of cpu_samples in bytes */
  processorid_t cpu_id;
  cpumtr_process_t *proc;

  /* Copyin user rdsamples structure. */
  retcode = copyin(arg, &rdsamples, sizeof(rdsamples));
  if (retcode != CPUMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CPUMTR:  rdsamples(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  if (rdsamples.cd_cpu_samples == NULL) {
    /* Suggest a number of CPUs. */
    rdsamples.cd_num_cpus = cpumtr_num_cpus;

    /* Copyout user arg structure. */
    retcode = copyout(&rdsamples, arg, sizeof(rdsamples));
    if (retcode != CPUMTR_NO_ERROR) {
      cmn_err(CE_WARN, "CPUMTR:  rdsamples(arg=0x%lx)\n", (ulong_t)arg);
      return EFAULT;
    }
    return CPUMTR_NO_ERROR;
  }

  /* Check that the number of CPUs is correct. */
  if (rdsamples.cd_num_cpus != cpumtr_num_cpus) {
    cmn_err(CE_WARN, "CPUMTR:  rdsamples(num_cpu=%u)\n",
      rdsamples.cd_num_cpus);
    return EINVAL;
  }
  
  /* Allocate array of CPU info and copy it in. */
  cpusamples_bytes = rdsamples.cd_num_cpus * sizeof(cpumtr_proc_rdsamples_t);
  cpu_samples = kmem_alloc(cpusamples_bytes, KM_SLEEP);
  if (cpu_samples == NULL) {
    cmn_err(CE_WARN, "CPUMTR:  rdsamples(cpusamples_bytes=%u)\n",
      cpusamples_bytes);
    return ENOMEM;
  }
  retcode = copyin(rdsamples.cd_cpu_samples, cpu_samples, cpusamples_bytes);
  if (retcode != CPUMTR_NO_ERROR) {
    kmem_free(cpu_samples, cpusamples_bytes);
    cmn_err(CE_WARN, "CPUMTR:  rdsamples(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  for (cpu_id = 0; cpu_id < rdsamples.cd_num_cpus; cpu_id++) {
    pl_t ply;
    cpumtr_cpu_t *cpu;

    cpu = &(cpumtr_cpu[cpu_id]);

    ply = cpumtr_lock(cpu->cc_lock, plhi);
    if (cpu->cc_trace_process != CPUMTR_PROCESS_NONE) {
      /* someone else is using this CPU */
      /* cmn_err(CE_WARN, "CPUMTR:  rdsamples(trace_process[%d]=%u)\n",
        cpu_id, cpu->cc_trace_process); */
      UNLOCK(cpu->cc_lock, ply);
      kmem_free(cpu_samples, cpusamples_bytes);
      /* Back out as gracefully as we can. */
      while (cpu_id > 0) {
        cpu_id--;
        cpu = &(cpumtr_cpu[cpu_id]);
        ply = cpumtr_lock(cpu->cc_lock, plhi);
        cpu->cc_trace_process = CPUMTR_PROCESS_NONE;
        UNLOCK(cpu->cc_lock, ply);
      }
      return EBUSY;
    }
    if (rdsamples.cd_block_until > cpu->cc_trace_buffer_size) {
      /* silently reduce the number of samples to a sane value */
      cpu->cc_block_until = cpu->cc_trace_buffer_size;
    } else {
      cpu->cc_block_until = rdsamples.cd_block_until;
    }
    cpu->cc_trace_process = process;
    UNLOCK(cpu->cc_lock, ply);
  }

  proc = &(cpumtr_process[process]);
  (void)cpumtr_lock(proc->cp_lock, plhi);

  /* Wait for the buffer to fill (or signal).  This releases the cp_lock.  */
  SV_WAIT_SIG(proc->cp_svp, prilo, proc->cp_lock);

  /* Cancel the trace request for each CPU. */
  for (cpu_id = 0; cpu_id < rdsamples.cd_num_cpus; cpu_id++) {
    pl_t ply;
    cpumtr_cpu_t *cpu;

    cpu = &(cpumtr_cpu[cpu_id]);

    ply = cpumtr_lock(cpu->cc_lock, plhi);
    if (cpu->cc_trace_process != process) {
      cmn_err(CE_WARN, "CPUMTR:  rdsamples(cancel trace_process[%d]=%u)\n",
        cpu_id, cpu->cc_trace_process);
    } else {
      cpu->cc_trace_process = CPUMTR_PROCESS_NONE;
    }
    UNLOCK(cpu->cc_lock, ply);
  }

  /*
   * Allocate temporary trace buffer.
   * This is necessary because we cannot hold driver spinlocks
   * across a copyout call.
   */
  trace_bytes = rdsamples.cd_max_samples * sizeof(cpumtr_sample_t);
  trace = kmem_alloc(trace_bytes, KM_SLEEP);
  if (trace == NULL) {
    cmn_err(CE_WARN, "CPUMTR:  rdsamples(trace_bytes=%u)\n", trace_bytes);
    return ENOMEM;
  }

  /* Copy each CPU's trace. */
  for (cpu_id = 0; cpu_id < rdsamples.cd_num_cpus; cpu_id++) {
    pl_t ply;
    cpumtr_cpu_t *cpu;
    uint_t samples_taken;
    uint_t samples_returned;
    cpumtr_proc_rdsamples_t *out;

    cpu = &(cpumtr_cpu[cpu_id]);

    ply = cpumtr_lock(cpu->cc_lock, plhi);
    samples_taken = cpu->cc_num_samples_taken;
    samples_returned = cpu->cc_num_samples_kept;

    if (rdsamples.cd_max_samples < samples_returned)
      samples_returned = rdsamples.cd_max_samples;
    
    if (cpu->cc_trace + samples_returned > cpu->cc_sample) {
      /* Copy two chunks of trace. */
      uint_t first;
      uint_t second;
      uint_t i;

      second = cpu->cc_sample - cpu->cc_trace;
      first = samples_returned - second;

      for (i = 0; i < first; i++)
        trace[i] = cpu->cc_trace_end[i - first];
      for (i = 0; i < second; i++)
        trace[first + i] = cpu->cc_trace[i];
      
    } else {
      /* Copy single chunk of trace. */
      uint_t i;

      for (i = 0; i < samples_returned; i++)
        trace[i] =
        cpu->cc_sample[i - samples_returned];
    }
    UNLOCK(cpu->cc_lock, ply);

    out = &(cpu_samples[cpu_id]);
    retcode = copyout(trace, out->cpd_samples,
      samples_returned * sizeof(cpumtr_sample_t));
    if (retcode != CPUMTR_NO_ERROR) {
      cmn_err(CE_WARN, "CPUMTR:  rdsamples([%d].cpd_samples=0x%lx)\n",
        cpu_id, (ulong_t)out->cpd_samples);
      kmem_free(cpu_samples, cpusamples_bytes);
      kmem_free(trace, trace_bytes);
      return EFAULT;
    }

    out->cpd_cpu_id = cpu_id;
    out->cpd_num_samples = samples_returned;
    out->cpd_num_samples_lost = samples_taken - samples_returned;
  }

  /* Free temporary trace buffer. */
  kmem_free(trace, trace_bytes);

  /* Copy out the CPU info. */
  retcode = copyout(cpu_samples, rdsamples.cd_cpu_samples, cpusamples_bytes);
  kmem_free(cpu_samples, cpusamples_bytes);
  if (retcode != CPUMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CPUMTR:  rdsamples(cd_cpu_samples=0x%lx)\n",
      (ulong_t)rdsamples.cd_cpu_samples);
    return EFAULT;
  }

  return CPUMTR_NO_ERROR;
} /* cpumtr_rdsamples() */

/*******************************************************
* cpumtr_reserve()
*
* If res_flag != FALSE reserve meters to the specified user/process.
* If res_flag == FALSE release meters reserved to the specified user/process.
*/
static int
cpumtr_reserve(caddr_t arg, cpumtr_process_id_t process, bool_t res_flag) {
  cpumtr_reserve_t reserve;
  uint_t isetup;
  int num_setups;
  uint_t first_setup;
  cpumtr_meter_t *first_meter_ptr;
  pl_t plx;
  cpumtr_meter_id_t meter_id;
  cpumtr_user_id_t user_id;
  cpumtr_cpu_t *cpu;
  int retcode;

  /* Copyin user reserve structure. */
  retcode = copyin(arg, &reserve, sizeof(reserve));
  if (retcode != CPUMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CPUMTR:  reserve(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  meter_id = reserve.cv_meter_id;
  user_id = reserve.cv_user_id;

  /* Set first_setup and num_setups. */
  num_setups = cpumtr_num_setups (meter_id, &first_setup);

  /* Check that the meter_id was valid. */
  if (num_setups == -1) {
    cmn_err(CE_WARN, "CPUMTR:  reserve(meter_id=0x%x:0x%x:0x%x)\n",
      meter_id.cmi_type, meter_id.cmi_instance, meter_id.cmi_cpu_id);
    return EINVAL;
  }
  if (first_setup >= CPUMTR_NUM_METERS) {
    cmn_err(CE_WARN, "CPUMTR:  reserve(setup=%u)\n", first_setup);
    return EINVAL;
  }
  cpu = &(cpumtr_cpu[meter_id.cmi_cpu_id]);
  first_meter_ptr = &(cpu->cc_meter[first_setup]);

  plx = cpumtr_lock(cpu->cc_lock, plhi);

  /* Check that none of the setups are reserved by another processs
     or activated by another user. */
  for (isetup = 0; isetup < num_setups; isetup++) {
    cpumtr_meter_t *meter;

    meter = &(first_meter_ptr[isetup]);
    if (meter->cm_reserving_process != CPUMTR_PROCESS_NONE
      && meter->cm_reserving_process != process)
      return EBUSY;

    if (meter->cm_setup_flags != CPUMTR_SETUP_DEACTIVATE
     && meter->cm_activating_user != user_id)
      return EBUSY;
  }

  for (isetup = 0; isetup < num_setups; isetup++) {
    cpumtr_meter_t *meter;

    meter = &(first_meter_ptr[isetup]);
    if (res_flag) {
      /* Reserve the meter. */
      meter->cm_reserving_process = process;
      meter->cm_reserving_user = user_id;
    } else {
      /* Release the meter. */
      meter->cm_reserving_process = CPUMTR_PROCESS_NONE;
    }
  }
  UNLOCK(cpu->cc_lock, plx);
    
  return CPUMTR_NO_ERROR;
} /* cpumtr_reserve() */


/*******************************************************
* cpumtr_setup()
*
* implements CPUMTR_CMD_SETUP, setting up meters.
*/
static int
cpumtr_setup(caddr_t arg, cpumtr_process_id_t process) {
  int retcode;
  int i;
  cpumtr_setup_t setup;
  uint_t num_cpu_setups;
  pl_t plx;
  cpumtr_process_t *proc;
  processorid_t cpu_id;

  cpumtr_cpu_setup_t *cpu_setups; /* array of CPU setups */
  size_t cpusetups_bytes;         /* size of array in bytes */
  cpumtr_sample_setup_t *css;     /* array of CPU sample setups */
  size_t css_bytes;               /* size of array in bytes */

  /* Copyin user structure. */
  retcode = copyin(arg, &setup, sizeof(setup));
  if (retcode != CPUMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CPUMTR:  setup(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  /* Check that number of CPU setups is legal. */
  num_cpu_setups = setup.cs_num_cpu_setups;
  if (num_cpu_setups < 1) {
    cmn_err(CE_WARN, "CPUMTR:  setup(num_cpu_setups=%u)\n", num_cpu_setups);
    return EINVAL;
  }

  /* Allocate array of CPU setups and copyin. */
  cpusetups_bytes = num_cpu_setups * sizeof(cpumtr_cpu_setup_t);
  cpu_setups = kmem_alloc(cpusetups_bytes, KM_SLEEP);
  if (cpu_setups == NULL) {
    cmn_err(CE_WARN, "CPUMTR:  setup(cpusetups_bytes=%u)\n", cpusetups_bytes);
    return ENOMEM;
  }
  retcode = copyin(setup.cs_cpu_setups, cpu_setups, cpusetups_bytes);
  if (retcode != CPUMTR_NO_ERROR) {
    kmem_free(cpu_setups, cpusetups_bytes);
    cmn_err(CE_WARN, "CPUMTR:  setup(cs_cpu_setups=0x%lx)\n",
      (ulong_t)setup.cs_cpu_setups);
    return EFAULT;
  }

  /* Allocate array of CPU sample setups and copyin. */
  css_bytes = num_cpu_setups * sizeof(cpumtr_sample_setup_t);
  css = kmem_alloc(css_bytes, KM_SLEEP);
  if (css == NULL) {
    kmem_free(cpu_setups, cpusetups_bytes);
    cmn_err(CE_WARN, "CPUMTR:  setup(css_bytes=%u)\n", css_bytes);
    return ENOMEM;
  }
  for (i = 0; i < num_cpu_setups; i++) {
    if (cpu_setups[i].cps_samplesetups_size == 1) {
      retcode = copyin(cpu_setups[i].cps_sample_setup, &(css[i]),
        sizeof(cpumtr_sample_setup_t));
      if (retcode != CPUMTR_NO_ERROR) {
        cmn_err(CE_WARN, "CPUMTR:  setup(cps_sample_setup=0x%lx)\n",
          (ulong_t)cpu_setups[i].cps_sample_setup);
        kmem_free(cpu_setups, cpusetups_bytes);
        kmem_free(css, css_bytes);
        return EFAULT;
      }
      cpu_setups[i].cps_sample_setup = &(css[i]);
    } else {
      cpu_setups[i].cps_sample_setup = NULL;
    }
  }

  /* Check each CPU setup */
  for (i = 0; i < num_cpu_setups; i++) {
    cpumtr_cpu_setup_t *cps;
    cpumtr_meter_id_t meter_id;
    int num_setups;
    uint_t first_setup;
    cpumtr_cpu_t *cpu;
    cpumtr_meter_t *first_meter_ptr;
    cpumtr_cputype_t cputype;
    uint_t setup_flags;
    uint_t num_sample_setups;
    uint_t isetup;

    cps = &(cpu_setups[i]);

    /* Check that the setup_flags are valid. */
    setup_flags = cps->cps_setup_flags;
    if (setup_flags != CPUMTR_SETUP_ACTIVATE
     && setup_flags != CPUMTR_SETUP_DEACTIVATE) {
      kmem_free(cpu_setups, cpusetups_bytes);
      kmem_free(css, css_bytes);
      cmn_err(CE_WARN, "CPUMTR:  setup([%d].cps_setup_flags=0x%x)\n",
        i, setup_flags);
      return EINVAL;
    }

    /* Set first_setup and num_setups. */
    meter_id = cps->cps_meter_id;
    num_setups = cpumtr_num_setups (meter_id, &first_setup);

    /* Check that the meter_id was valid. */
    if (num_setups == -1) {
      kmem_free(cpu_setups, cpusetups_bytes);
      kmem_free(css, css_bytes);
      cmn_err(CE_WARN, "CPUMTR:  setup([%d].cps_meter_id=0x%x:0x%x:0x%x)\n",
        i, meter_id.cmi_type, meter_id.cmi_instance, meter_id.cmi_cpu_id);
      return EINVAL;
    }
    if (first_setup >= CPUMTR_NUM_METERS) {
      kmem_free(cpu_setups, cpusetups_bytes);
      kmem_free(css, css_bytes);
      cmn_err(CE_WARN, "CPUMTR:  setup([%d].setup=%u)\n", i, first_setup);
      return EINVAL;
    }

    cpu = &(cpumtr_cpu[meter_id.cmi_cpu_id]);
    first_meter_ptr = &(cpu->cc_meter[first_setup]);

    /* Make sure the CPU type is valid. */
    cputype = cps->cps_cputype;
    if (cputype != cpu->cc_cputype
     || cputype == CPUMTR_NOT_SUPPORTED) {
      kmem_free(cpu_setups, cpusetups_bytes);
      kmem_free(css, css_bytes);
      cmn_err(CE_WARN, "CPUMTR:  setup([%d].cps_cputype=%u)\n", i, cputype);
      return EINVAL;
    }

    /* Multiple sample setups per CPU are not supported. */
    num_sample_setups = cps->cps_samplesetups_size;
    if (num_sample_setups != (cps->cps_sample_setup != NULL)) {
      kmem_free(cpu_setups, cpusetups_bytes);
      kmem_free(css, css_bytes);
      cmn_err(CE_WARN, "CPUMTR:  setup([%d].cps_samplesetups_size=%u)\n",
        i, num_sample_setups);
      return EINVAL;
    }

    /* Make sure this process has reserved the meters. */
    for (isetup = 0; isetup < num_setups; isetup++) {
      cpumtr_meter_t *meter;

      meter = &(first_meter_ptr[isetup]);
      if (meter->cm_reserving_process != process) {
        kmem_free(cpu_setups, cpusetups_bytes);
        kmem_free(css, css_bytes);
        cmn_err(CE_WARN,
          "CPUMTR:  setup(process=%u, reserving_process[%u]=%u)\n",
          process, i, meter->cm_reserving_process);
        return EBUSY;
      }
    }
  }

  for (i = 0; i < num_cpu_setups; i++) {
    cpumtr_cpu_setup_t *cps;
    cpumtr_meter_id_t meter_id;
    cpumtr_cpu_t *cpu;
    cpumtr_process_id_t setup_process;

    cps = &(cpu_setups[i]);
    meter_id = cps->cps_meter_id;
    cpu_id = meter_id.cmi_cpu_id;
    cpu = &(cpumtr_cpu[cpu_id]);

    plx = cpumtr_lock(cpu->cc_lock, plhi);
    setup_process = cpu->cc_setup_process;
    if (setup_process == CPUMTR_PROCESS_NONE
     || setup_process == process) {
      cpu->cc_setup_process = process;
      cpu->cc_retcode = CPUMTR_NO_ERROR;
      UNLOCK(cpu->cc_lock, plx);
    } else {
      int j;

      UNLOCK(cpu->cc_lock, plx);
      for (j = 0; j < i; j++) {
        cps = &(cpu_setups[i]);
        meter_id = cps->cps_meter_id;
        cpu_id = meter_id.cmi_cpu_id;
        cpu = &(cpumtr_cpu[cpu_id]);
        plx = cpumtr_lock(cpu->cc_lock, plhi);
        cpu->cc_setup_process = CPUMTR_PROCESS_NONE; 
        UNLOCK(cpu->cc_lock, plx);
      }
      kmem_free(cpu_setups, cpusetups_bytes);
      kmem_free(css, css_bytes);
      cmn_err(CE_WARN, "CPUMTR:  setup(process=%u, setup_process[%u]=%u)\n",
        process, i, setup_process);
      return EBUSY;
    }
  }

  /* Set up CPUs together, as close together in time as possible. */

  proc = &(cpumtr_process[process]);

  plx = cpumtr_lock(proc->cp_lock, plhi);

  /* Set up the meters */
  proc->cp_num_cpu_setups = num_cpu_setups;
  proc->cp_cpu_setups = cpu_setups;
  proc->cp_count = cpumtr_num_cpus;
  retcode = CPUMTR_NO_ERROR;
  for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
    cpumtr_cpu_t *cpu;

    cpu = &(cpumtr_cpu[cpu_id]);

    if (cpu->cc_setup_process != process
     || cpu->cc_cputype == CPUMTR_NOT_SUPPORTED) {
      proc->cp_count--;
    } else {
      cpu->cc_setup_timer = dtimeout(cpumtr_setup_cpu,
        (void *)cpu, CPUMTR_ASAP, plhi, cpu_id);  
      if (cpu->cc_setup_timer == CPUMTR_TOID_NONE) {
        cmn_err(CE_WARN, "CPUMTR:  setup(no timer on CPU %d)\n", cpu_id);
        proc->cp_count--;
        retcode = ENXIO;
        break;
      }
    }
  }

  /* Wait for setup to complete even if an error. */
  /* This releases the cp_lock. */
  retcode = cpumtr_join(process, plx, CPUMTR_AT_SETUP);

  /* Free the kernel memory allocated above. */
  proc->cp_cpu_setups = NULL;

  if (retcode != CPUMTR_NO_ERROR) {
    kmem_free(cpu_setups, cpusetups_bytes);
    kmem_free(css, css_bytes);
    return retcode;
  }

  /* Check for any errors. */
  for (i = 0; i < num_cpu_setups; i++) {
    cpumtr_cpu_setup_t *cps;
    cpumtr_meter_id_t meter_id;
    processorid_t cpu_id;
    cpumtr_cpu_t *cpu;

    cps = &(cpu_setups[i]);
    meter_id = cps->cps_meter_id;
    cpu_id = meter_id.cmi_cpu_id;
    cpu = &(cpumtr_cpu[cpu_id]);

    plx = cpumtr_lock(cpu->cc_lock, plhi);
    if (retcode == CPUMTR_NO_ERROR) {
      retcode = cpu->cc_retcode;
      cpu->cc_retcode = CPUMTR_NO_ERROR;
    }
    cpu->cc_setup_process = CPUMTR_PROCESS_NONE;
    UNLOCK(cpu->cc_lock, plx);
  }

  kmem_free(cpu_setups, cpusetups_bytes);
  kmem_free(css, css_bytes);
  return retcode;
} /* cpumtr_setup() */


/*********************************************
* cpumtropen()
*
* This function is called when a process opens the
* driver. It checks the open type and does appropriate
* actions
*/
/* ARGSUSED */
int
cpumtropen(dev_t *devp, int flags, int otyp, struct cred *cred_p) {
  cpumtr_process_id_t process;
  pl_t plx;

  /* Get open lock. */
  plx = cpumtr_lock(cpumtr_open_lock, plbase);

  /* Check there is a process to allocate. */
  if (cpumtr_num_processes >= CPUMTR_MAX_PROCESSES) {
    UNLOCK(cpumtr_open_lock, plx);
    return EAGAIN;
  }

  /* Find free process id (start at 1 as 0 is reserved for driver). */
  for (process = 1; process <= CPUMTR_MAX_PROCESSES; process++) {
    if (cpumtr_open_flag[process] == FALSE)
      break;
  }

  cpumtr_num_processes ++;
  cpumtr_open_flag[process] = TRUE;  

  /* Release the open lock. */
  UNLOCK(cpumtr_open_lock, plx);

  /* Change the minor number so that this process
     can be identified on ioctl and close calls. */
  *devp = makedevice(getemajor(*devp), process);

  return CPUMTR_NO_ERROR;
} /* cpumtropen() */


/*******************************************************
* cpumtrioctl()
*
* called on an ioctl() by the process.
*/
/* ARGSUSED */
int
cpumtrioctl(dev_t dev, int cmd, caddr_t arg, int mode, struct cred *cred_p,
  int *rval_p)
{
  cpumtr_process_id_t process;

  /* Get id for process calling the driver. */
  if (geteminor(dev) < 1 || geteminor(dev) > CPUMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CPUMTR:  ioctl(process=%lu)\n", geteminor(dev));
    return EINVAL;
  }
  process = geteminor(dev);

  switch (cmd) {
    case CPUMTR_CMD_SETUP:
      return cpumtr_setup(arg, process);
    
    case CPUMTR_CMD_RDSAMPLES:
      return cpumtr_rdsamples(arg, process);

    case CPUMTR_CMD_RESERVE:
      return cpumtr_reserve(arg, process, TRUE);

    case CPUMTR_CMD_RELEASE:
      return cpumtr_reserve(arg, process, FALSE);

    case CPUMTR_CMD_RDSTATS:
      return cpumtr_rdstats(arg, process);

    case CPUMTR_CMD_KON:
      return (lwp_callback_enable(SYSENTRY_ATTACH, (lwp_t *)NULL) >= 0)	
        ? CPUMTR_NO_ERROR : EINVAL;

    case CPUMTR_CMD_KOFF:
      {
        processorid_t cpu_id;
        
        for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
          cpumtr_cpu_t *cpu;
      
          cpu = &(cpumtr_cpu[cpu_id]);
          cpu->cc_state = CPUMTR_STATE_UNKNOWN;
        }
      }
      return (lwp_callback_enable(SYSENTRY_DETACH, (lwp_t *)NULL) >= 0)
        ? CPUMTR_NO_ERROR : EINVAL;

    case CPUMTR_CMD_PON:
      return (lwp_callback_enable(SWTCH_OUT2_ATTACH, (lwp_t *)NULL) >= 0)
        ? CPUMTR_NO_ERROR : EINVAL;

    case CPUMTR_CMD_POFF:
      return (lwp_callback_enable(SWTCH_OUT2_DETACH, (lwp_t *)NULL) >= 0)
        ? CPUMTR_NO_ERROR : EINVAL;
  }

  cmn_err(CE_WARN, "CPUMTR:  ioctl(cmd=0x%x)\n", cmd);
  return EINVAL;
} /* cpumtrioctl() */


/*******************************************************
* cpumtrclose()
*
* called on close of driver.
* It releases the meters that were reserved by this process.
*/
/* ARGSUSED */
int
cpumtrclose(dev_t dev, int flags, int otyp, struct cred *cred_p) {
  pl_t plx;
  cpumtr_process_id_t process;
  processorid_t cpu_id;

  /* Get id for process calling the driver. */
  if (getminor(dev) < 1 || getminor(dev) > CPUMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CPUMTR:  close(process=%lu)\n", getminor(dev));
    return EINVAL;
  }
  process = geteminor(dev);

  /* Release all meters reserved by this process. */
  for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
    uint_t i;
    cpumtr_cpu_t *cpu;
    pl_t plx;

    cpu = &(cpumtr_cpu[cpu_id]);

    plx = cpumtr_lock(cpu->cc_lock, plhi);
    for (i = 0; i < CPUMTR_NUM_METERS; i++) {
      cpumtr_meter_t *meter;

      meter = &(cpu->cc_meter[i]);
      if (meter->cm_reserving_process == process)
        meter->cm_reserving_process =
          CPUMTR_PROCESS_NONE;
    }
    UNLOCK(cpu->cc_lock, plx);
  }

  /* Deallocate the per-process structure. */
  plx = cpumtr_lock(cpumtr_open_lock, plbase);
  cpumtr_open_flag[process] = FALSE;
  cpumtr_num_processes --;
  UNLOCK(cpumtr_open_lock, plx);

  return CPUMTR_NO_ERROR;
} /* cpumtr_close() */


/*******************************************************
* cpumtr_alloc()
*
* allocate dynamic memory needed by the driver (except per-CPU memory).
*/
static int
cpumtr_alloc(int sleep_flag) {
  cpumtr_process_id_t process;
  cpumtr_process_t *proc;

  cpumtr_open_lock = LOCK_ALLOC(1, plbase, &cpumtr_lkinfo_reservelock,
    sleep_flag);
  if (cpumtr_open_lock == NULL) {
    cmn_err(CE_WARN, "CPUMTR:  alloc(open_lock=NULL)\n");
    return ENOMEM;
  }

  /* Initialize each process structure (CPUMTR_MAX_PROCESSES+1).  */
  for (process = 0; process <= CPUMTR_MAX_PROCESSES; process++) {
    proc = &(cpumtr_process[process]);
    proc->cp_lock = LOCK_ALLOC(1, plhi, &cpumtr_lkinfo_processlock, sleep_flag);
    if (proc->cp_lock == NULL) {
      cmn_err(CE_WARN, "CPUMTR:  alloc([%d].cp_lock=NULL)\n", process);
      return ENOMEM;
    }

    proc->cp_svp = SV_ALLOC(sleep_flag);
    if (proc->cp_svp == NULL) {
      cmn_err(CE_WARN, "CPUMTR:  alloc([%d].cp_svp=NULL)\n", process);
      return ENOMEM;
    }
  }

  cpumtr_lwp_lock = RW_ALLOC(3, plhi, &cpumtr_lkinfo_lwplock, sleep_flag);
  if (cpumtr_lwp_lock == NULL) {
    cmn_err(CE_WARN, "CPUMTR:  alloc(lwp_lock=NULL)\n");
    return ENOMEM;
  }

  cpumtr_cpu = kmem_alloc(cpumtr_cpu_bytes, sleep_flag);
  if (cpumtr_cpu == NULL) {
    cmn_err(CE_WARN, "CPUMTR:  alloc(cpu_bytes=%u)\n", cpumtr_cpu_bytes);
    return ENOMEM;
  }

  return CPUMTR_NO_ERROR;
} /* cpumtr_alloc() */


/*******************************************************
* cpumtr_dealloc()
*
* deallocate dynamic memory used by the driver.
*/
static void
cpumtr_dealloc(void) {
  processorid_t cpu_id;
  cpumtr_process_id_t process;
  pl_t plx;

  if (cpumtr_cpu != NULL) {
    /* Deallocate CPU structures. */
    for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
      cpumtr_cpu_t *cpu;

      cpu = &(cpumtr_cpu[cpu_id]);
      if (cpu->cc_lock != NULL)
        plx = cpumtr_lock(cpu->cc_lock, plhi);

      /* Remove setups. */
      if (cpu->cc_setup != NULL) {
        kmem_free(cpu->cc_setup, cpu->cc_setup_bytes);
        cpu->cc_setup = NULL;
      }

      /* Remove trace buffer. */
      if (cpu->cc_trace != NULL) {
        kmem_free(cpu->cc_trace,
          cpu->cc_trace_buffer_size * sizeof(cpumtr_sample_t));
      }

      if (cpu->cc_lock != NULL) {
        UNLOCK(cpu->cc_lock, plx);

        /* deallocate per-CPU lock */
        LOCK_DEALLOC(cpu->cc_lock);
      }
    }

    kmem_free(cpumtr_cpu, cpumtr_cpu_bytes);
  }

  /* Deallocate LWP lock. */
  if (cpumtr_lwp_lock != NULL)
    RW_DEALLOC(cpumtr_lwp_lock);

  /* Deallocate processes. */
  for (process = 0; process <= CPUMTR_MAX_PROCESSES; process++) {
    cpumtr_process_t *proc;

    proc = &(cpumtr_process[process]);

    /* Deallocate each per-process lock. */
    if (proc->cp_lock != NULL)
      LOCK_DEALLOC(proc->cp_lock);
    if (proc->cp_svp != NULL)
      SV_DEALLOC(proc->cp_svp);
  }

  /* Deallocate open lock. */
  if (cpumtr_open_lock != NULL)
    LOCK_DEALLOC(cpumtr_open_lock);
} /* cpumtr_dealloc() */

/*******************************************************
* cpumtr_load()
*
* dynamically loads the driver.
*/
static int
cpumtr_load(void) {
  uint_t ibucket;
  processorid_t cpu_id;
  pl_t plx;
  int retcode;
  cpumtr_process_id_t process;
  cpumtr_process_t *proc;

  /* db_breakpoint(); */
  mod_drvattach(&cpumtr_attach_info);

  cpumtr_open_lock = NULL;
  cpumtr_num_processes = 0;

  /* Initialize each process structure (CPUMTR_MAX_PROCESSES+1).  */
  for (process = 0; process <= CPUMTR_MAX_PROCESSES; process++) {
    proc = &(cpumtr_process[process]);
    proc->cp_lock = NULL;
    proc->cp_svp = NULL;
    proc->cp_watchdog = CPUMTR_TOID_NONE;
    proc->cp_count = 0;
    cpumtr_open_flag[process] = FALSE;
  }

  /* Initialize LWP hash table. */
  for (ibucket = 0; ibucket < CPUMTR_NUM_LWP_BUCKETS; ibucket++)
    cpumtr_lwp[ibucket] = NULL;
  cpumtr_num_lwps = 0;
  cpumtr_lwp_lock = NULL;

  cpumtr_num_cpus = Nengine;
  cpumtr_cpu_bytes = cpumtr_num_cpus*sizeof(cpumtr_cpu_t);
  cpumtr_cpu = NULL;

  /* Allocate the first batch of memory. */
  if (cpumtr_alloc(KM_SLEEP) == ENOMEM) {
    cpumtr_dealloc();
    return ENOMEM;
  }

  /* Initialize each CPU structure. */
  for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
    cpumtr_cpu_t *cpu;
    uint_t imeter;
    uint_t istate;

    cpu = &(cpumtr_cpu[cpu_id]);

    cpu->cc_cpu_id = cpu_id;

    cpu->cc_lock = NULL;
    cpu->cc_cputype = CPUMTR_NOT_SUPPORTED;
    cpu->cc_hz = 0;

    for (imeter = 0; imeter < CPUMTR_NUM_METERS; imeter++) {
      cpumtr_meter_t *meter;

      meter = &(cpu->cc_meter[imeter]);
      meter->cm_reserving_process = CPUMTR_PROCESS_NONE;
      meter->cm_setup_flags = CPUMTR_SETUP_DEACTIVATE;

      for (istate = 0; istate < CPUMTR_NUM_STATES; istate++) {
        meter->cm_count[istate].dl_hop = 0;
        meter->cm_count[istate].dl_lop = 0;
      }
    }

    cpu->cc_pid = -2;
    cpu->cc_lwpid = -1;
    cpu->cc_state = CPUMTR_STATE_UNKNOWN;
    cpu->cc_setup = NULL;
    cpu->cc_setup_bytes = 0;

    cpu->cc_num_samples_taken = 0;
    cpu->cc_num_samples_kept = 0;
    cpu->cc_trace_buffer_size = 1;
    cpu->cc_trace = NULL;
    cpu->cc_trace_end = NULL;
    cpu->cc_sample = NULL;

    cpu->cc_max_interval = 0;

    cpu->cc_setup_timer = CPUMTR_TOID_NONE;
    cpu->cc_setup_timer = CPUMTR_TOID_NONE;
    cpu->cc_timeout_timer = CPUMTR_TOID_NONE;
    cpu->cc_update_timer = CPUMTR_TOID_NONE;


    cpu->cc_setup_process = CPUMTR_PROCESS_NONE;

    for (istate = 0; istate < CPUMTR_NUM_STATES; istate++) {
      cpu->cc_time[istate].dl_lop = 0;
      cpu->cc_time[istate].dl_hop = 0;
    }
    cpu->cc_lwp_samples_lost = 0;
    cpu->cc_trace_process = CPUMTR_PROCESS_NONE;
    cpu->cc_retcode = CPUMTR_NO_ERROR;
  }

  /* Allocate per-CPU memory. */
  for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
    cpumtr_cpu_t *cpu;

    cpu = &(cpumtr_cpu[cpu_id]);

    cpu->cc_lock = LOCK_ALLOC(2, plhi, &cpumtr_lkinfo_cpulock, KM_SLEEP);
    if (cpu->cc_lock == NULL) {
      cmn_err(CE_WARN, "CPUMTR:  load([%d].cc_lock=NULL)\n", cpu_id);
      cpumtr_dealloc();
      return ENOMEM;
    }

    cpu->cc_trace = kmem_alloc(
      cpu->cc_trace_buffer_size*sizeof(cpumtr_sample_t), KM_SLEEP);
    if (cpu->cc_trace == NULL) {
      cmn_err(CE_WARN, "CPUMTR:  load(trace_bytes=%u)\n",
        cpu->cc_trace_buffer_size*sizeof(cpumtr_sample_t));
      cpumtr_dealloc();
      return ENOMEM;
    }
    cpu->cc_trace_end = cpu->cc_trace + cpu->cc_trace_buffer_size;
    cpu->cc_sample = cpu->cc_trace;
  }

#ifdef TEST
  for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
    cpumtr_cpu_t *cpu;

    cpu = &(cpumtr_cpu[cpu_id]);

    /* Get CPU type. */
    cpu->cc_cputype = cpumtr_cputype();
    cpu->cc_hz = 166000000; /* TODO */
  }
#else
  /* Spawn off a startup routine for each CPU. */
  process = CPUMTR_PROCESS_GLOBAL;
  proc = &(cpumtr_process[process]);

  plx = cpumtr_lock(proc->cp_lock, plhi);
  for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
    cpumtr_cpu_t *cpu;

    cpu = &(cpumtr_cpu[cpu_id]);

    cpu->cc_start_timer = dtimeout(cpumtr_start_cpu,
      (void*)cpu, CPUMTR_ASAP, plhi, cpu_id);
    if (cpu->cc_start_timer == CPUMTR_TOID_NONE) {
      cmn_err(CE_WARN, "CPUMTR:  load(no timer on CPU %d)\n", cpu_id);
    } else {
      proc->cp_count++;
    } 
  } 

  /* Wait for CPU startups to complete, even if there were errors. */
  /* This releases the cp_lock. */
  retcode = cpumtr_join(CPUMTR_PROCESS_GLOBAL, plx, CPUMTR_AT_START);
  if (retcode != CPUMTR_NO_ERROR)
    return retcode;
#endif

  /* Register callbacks with the system. */
  cpumtr_install_hooks(TRUE);

  return CPUMTR_NO_ERROR;
} /* cpumtr_load() */


/*******************************************************
* cpumtr_unload()
*
* unloads the driver. All sampling and meters are stopped.
*/
static int
cpumtr_unload(void) {
  processorid_t cpu_id;
  cpumtr_process_id_t process;
  toid_t toid;

  /* Remove the kernel hooks. */
  cpumtr_install_hooks(FALSE);

  /* Cancel any pending timeouts. */
  for (cpu_id = 0; cpu_id < cpumtr_num_cpus; cpu_id++) {
    cpumtr_cpu_t *cpu;

    cpu = &(cpumtr_cpu[cpu_id]);
    toid = cpu->cc_setup_timer;
    if (toid != CPUMTR_TOID_NONE)
      untimeout(toid);
    toid = cpu->cc_start_timer;
    if (toid != CPUMTR_TOID_NONE)
      untimeout(toid);
    toid = cpu->cc_timeout_timer;
    if (toid != CPUMTR_TOID_NONE)
      untimeout(toid);
    toid = cpu->cc_update_timer;
    if (toid != CPUMTR_TOID_NONE)
      untimeout(toid);
  }

  for (process = 0; process <= CPUMTR_MAX_PROCESSES; process++) {
    toid = cpumtr_process[process].cp_watchdog;
    if (toid != CPUMTR_TOID_NONE)
      untimeout(toid);
  }

  /* Deallocate kernel memory used by this module. */
  cpumtr_dealloc();

  mod_drvdetach(&cpumtr_attach_info);
  return CPUMTR_NO_ERROR;
} /* cpumtr_unload() */
