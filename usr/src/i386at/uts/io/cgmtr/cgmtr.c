/* copyright "%c%" */
#ident	"@(#)kern-i386at:io/cgmtr/cgmtr.c	1.1.3.1"
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
# include <proc/lwp.h>
# include <io/cgmtr/cgmtr.h>
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

# ifdef HEADERS_INSTALLED
#  include <sys/cgmtr.h>
# else /* !HEADERS_INSTALLED */
#  include "cgmtr.h"
# endif /* !HEADERS_INSTALLED */
#endif /* !_KERNEL_HEADERS */

extern cgnum_t cg_numcg(int);

#ifndef NUMA
#define cg_dtimeout dtimeout  /* TODO take this out someday */
#endif /* NUMA */

#define DRVNAME "cgmtr - CG Performance Meters Driver"

/* interval for watchdog timer */
#define CGMTR_WATCHDOG_USEC 10000000  /* 10 seconds */

/* forward declarations of functions:  */
static int cgmtr_load(void);
static int cgmtr_unload(void);
static void cgmtr_update_cg(void *);

/* wrappers required for dynamic loading of driver:  */
MOD_DRV_WRAPPER(cgmtr, cgmtr_load, cgmtr_unload, NULL, DRVNAME);
int cgmtrdevflag = D_MP;

/* global variables:  */

/* lock variable to allocate CGs to processes */
static lock_t *cgmtr_open_lock = NULL;
LKINFO_DECL(cgmtr_lkinfo_reservelock, "CGMTR:cgmtr_open_lock", 0);

/* number of process indices allocated */
static int cgmtr_num_processes = 0;

/* flags to indicate whether this process index is in use */
static bool_t cgmtr_open_flag[CGMTR_MAX_PROCESSES+1];


/* per-process info */
static cgmtr_process_t cgmtr_process[CGMTR_MAX_PROCESSES+1];
LKINFO_DECL(cgmtr_lkinfo_processlock, "CGMTR:cgmtr_process[x].pp_lock", 0);


static cgnum_t cgmtr_num_cgs = 0;	/* number of CGs in the system */
static size_t cgmtr_cg_bytes = 0;	/* number of bytes in cg array */
static cgmtr_cg_t *cgmtr_cg = NULL;
LKINFO_DECL(cgmtr_lkinfo_cglock, "CGMTR:cgmtr_cg[x].pc_lock", 0);


/*******************************************************
* cgmtr_cgtype()
*
* returns the CG type.
*/
static cgmtr_cgtype_t
cgmtr_cgtype(void) {
  if (cgmtr_mcu_present()) {
    return CGMTR_MCU;
  } else if (cgmtr_piu_present()) {
    return CGMTR_PIU;
  }
  return CGMTR_TEST;
} /* cgmtr_cgtype() */


/*******************************************************
* cgmtr_num_setups()
*
* returns the number of setups and sets first_setup.
* Returns -1 if the meter_id is invalid.
*/

static int
cgmtr_num_setups(cgmtr_meter_id_t meter_id, uint_t *first_setup) {
  cgmtr_cg_t *cg;

  /* Check the CG id. */
  if (meter_id.cgi_cg_num >= cgmtr_num_cgs)
    return -1;

  cg = &(cgmtr_cg[meter_id.cgi_cg_num]);

  switch (cg->pc_cgtype) {
    case CGMTR_MCU:
      return cgmtr_mcu_num_setups(meter_id, first_setup);
    case CGMTR_PIU:
      return cgmtr_piu_num_setups(meter_id, first_setup);
    case CGMTR_TEST:
      return cgmtr_test_num_setups(meter_id, first_setup);
  }

  /* unknown CG type */
  return -1;
} /* cgmtr_num_setups() */


/*******************************************************
* cgmtr_copy_setup()
*
* copy a portion of a sample_setup structure.
*/
static void
cgmtr_copy_setup(cgmtr_sample_setup_t *des, cgmtr_sample_setup_t *src,
    uint_t start, uint_t end) {
  uint_t isetup;
  uint_t ichar;

  for (isetup = start; isetup < end; isetup++) {
    des->pss_max_interval[isetup] = src->pss_max_interval[isetup];
    des->pss_meter_setup[isetup] = src->pss_meter_setup[isetup];
    for (ichar = 0; ichar < CGMTR_MAX_NAME_LENGTH; ichar++) {
      char ch = src->pss_name[isetup][ichar];
      des->pss_name[isetup][ichar] = ch;
      if (ch == '\0')
        break;
    }
  }
} /* cgmtr_copy_setup() */


/*******************************************************
* cgmtr_deactivate_setup()
*
* deactivate a portion of a sample_setup structure.
*/
static int
cgmtr_deactivate_setup(cgmtr_sample_setup_t *des, uint_t start,
    uint_t end, cgmtr_cgtype_t cgtype) {
  uint_t isetup;

  for (isetup = start; isetup < end; isetup++) {
    des->pss_max_interval[isetup] = (cgmtr_uint32_t)TO_PERIODIC;
    des->pss_name[isetup][0] = '\0';
    switch (cgtype) {
      case CGMTR_MCU:
        cgmtr_mcu_deactivate(&(des->pss_meter_setup[isetup]), isetup);
        break;
      case CGMTR_PIU:
        cgmtr_piu_deactivate(&(des->pss_meter_setup[isetup]), isetup);
        break;
      case CGMTR_TEST:
        cgmtr_test_deactivate(&(des->pss_meter_setup[isetup]), isetup);
        break;
      default:
        cmn_err(CE_WARN, "CGMTR:  deactivate_setup(cgtype=%u)\n", cgtype);
        return ENXIO;
    }
  }

  return CGMTR_NO_ERROR;
} /* cgmtr_deactivate_setup() */


/*******************************************************
* cgmtr_lock()
*
* acquires a lock.
* This function exists to make debugging easier.
*/
static pl_t
cgmtr_lock(lock_t *lockp, pl_t pl) {
  return LOCK(lockp, pl);
} /* cgmtr_lock() */


/*******************************************************
* cgmtr_getsample()
*
* samples the CG meters into pc_sample.
* The pc_lock should be held on entry and exit.
*/
static int
cgmtr_getsample(cgnum_t cgnum, cgmtr_tracepoint_t tracepoint) {
  cgmtr_sample_t *sample;
  uint_t isetup;
  cgmtr_cg_t *cg;
  uint_t num_counters;

  /* Check the CG id. */
  if (cgnum >= cgmtr_num_cgs) {
    cmn_err(CE_WARN, "CGMTR:  getsample(cgnum=%d)\n", cgnum);
    return EINVAL;
  }
  cg = &(cgmtr_cg[cgnum]);

  switch (cg->pc_cgtype) {
    case CGMTR_MCU:
      num_counters = CGMTR_MCU_NUM_METERS;
      break;
    case CGMTR_PIU:
    case CGMTR_TEST:
      num_counters = CGMTR_PIU_NUM_METERS - 1; /* exclude the MHB */
      break;
  }

  /* Check there is a current sample. */
  sample = cg->pc_sample;
  if (sample == NULL) {
    cmn_err(CE_WARN, "CGMTR:  getsample(sample=NULL)\n");
    return EIO;
  }

  sample->pa_start_lbolt = cg->pc_start_lbolt;
  sample->pa_tracepoint = tracepoint;

  if (cg->pc_setup == NULL) {
    sample->pa_setup_valid_flag = FALSE;
  } else {
    sample->pa_setup_valid_flag = TRUE;
    cgmtr_copy_setup(&(sample->pa_setup), cg->pc_setup, 0, CGMTR_NUM_METERS);
  }

  switch (cg->pc_cgtype) {
    case CGMTR_MCU:
      cgmtr_mcu_getsample(cg, sample);
      break;

    case CGMTR_PIU:
      cgmtr_piu_getsample(cg, sample);
      break;

    case CGMTR_TEST:
      cgmtr_test_getsample(cg, sample);
      break;

    default:
      cmn_err(CE_WARN, "CGMTR:  getsample(cgtype=%u)\n", cg->pc_cgtype);
      return ENXIO;
  }

  /* Update per-CG virtual meters. */
  {
    cgmtr_uint64_t diff;

    for (isetup = 0; isetup < num_counters; isetup++) {
      cgmtr_meter_t *meter;

      meter = &(cg->pc_meter[isetup]);
      diff = lsub(sample->pa_count[isetup], meter->cg_start_count);
      meter->cg_count = ladd(diff, meter->cg_count);
    }
  }

  /* Save ending values for use as start values in the next sample. */
  cg->pc_start_lbolt = sample->pa_end_lbolt;
  for (isetup = 0; isetup < num_counters; isetup++)
    cg->pc_meter[isetup].cg_start_count = sample->pa_count[isetup];

  cg->pc_num_samples_taken++;

  /* If tracing, advance the write pointer. */
  if (cg->pc_trace_buffer_size > 1) {
    cgmtr_process_id_t process;

    if (cg->pc_num_samples_taken < cg->pc_trace_buffer_size) {
      /* not full yet */
      cg->pc_num_samples_kept++;
    }

    cg->pc_sample++;
    if (cg->pc_sample == cg->pc_trace_end) {
      /* output pointer wraps around */
      cg->pc_sample = cg->pc_trace;
    }

    /* If a process is waiting for the samples, wake it up. */
    process = cg->pc_trace_process;
    if (process != CGMTR_PROCESS_NONE
     && cg->pc_num_samples_kept >= cg->pc_block_until) {
      SV_SIGNAL(cgmtr_process[process].pp_svp, 0);
    }
  }

  return CGMTR_NO_ERROR;
} /* cgmtr_getsample() */


/*******************************************************
* cgmtr_barrier()
*
*/
static void
cgmtr_barrier(cgmtr_process_id_t process) {
  pl_t plx;
  cgmtr_process_t *proc;

  /* Check that process is legal. */
  if (process < 0 || process > CGMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CGMTR:  barrier(%u)\n", process);
    return;
  }
  proc = &(cgmtr_process[process]);

  /* Get process lock. */
  plx = cgmtr_lock(proc->pp_lock, plhi);

  /* Decrement count and, if zero, wake up client thread. */
  proc->pp_count--;
  if (proc->pp_count < 0) {
    cmn_err(CE_WARN, "CGMTR:  barrier(count = %u)\n", proc->pp_count);

  } else if (proc->pp_count == 0) {
    SV_SIGNAL(proc->pp_svp, 0);
  }

  /* Release process lock. */
  UNLOCK(proc->pp_lock, plx);
} /* cgmtr_barrier() */

/*******************************************************
* cgmtr_start_cg()
*
* determines CG type.
*/
static void
cgmtr_start_cg(void *ptr) {
  cgmtr_cg_t *cg;

  cg = (cgmtr_cg_t *)ptr;

  /* Get CG type. */
  cg->pc_cgtype = cgmtr_cgtype();

  switch (cg->pc_cgtype) {
    case CGMTR_MCU:
      cgmtr_mcu_start_count(cg);
      break;
    case CGMTR_PIU:
      cgmtr_piu_start_count(cg);
      break;
    case CGMTR_TEST:
      cgmtr_test_start_count(cg);
      break;

   default:
    cmn_err(CE_WARN, "CGMTR:  start_cg(cgtype=%u)\n", cg->pc_cgtype);
    cg->pc_start_timer = CGMTR_TOID_NONE;
    cgmtr_barrier(CGMTR_PROCESS_GLOBAL);
    return;
  }

  /* Get starting lbolt. */
  cg->pc_start_lbolt = TICKS();

  cg->pc_start_timer = CGMTR_TOID_NONE;
  cgmtr_barrier(CGMTR_PROCESS_GLOBAL);
} /* cgmtr_start_cg() */


/*******************************************************
* cgmtr_update_cg()
*
* update all virtual meters on a particular CG.
*/
static void
cgmtr_update_cg(void *ptr) {
  cgnum_t cgnum;
  cgmtr_cg_t *cg;
  pl_t plx;
  cgmtr_process_id_t process;

  cg = (cgmtr_cg_t *)ptr;
  
  cgnum = cg->pc_cg_num;
  
  /* Get CG lock. */
  plx = cgmtr_lock(cg->pc_lock, plhi);

  /* Save the index of the update process. */
  process = cg->pc_update_process;

  cg->pc_retcode = cgmtr_getsample(cgnum, cg->pc_update_tracepoint);

  /* Release CG lock. */
  UNLOCK(cg->pc_lock, plx);

  cgmtr_barrier(process);
} /* cgmtr_update_cg() */


/*******************************************************
* cgmtr_timeout_cg()
*
* update all virtual meters on a particular CG on timeout.
*/
static void
cgmtr_timeout_cg(void *ptr) {
  cgnum_t cgnum;
  cgmtr_cg_t *cg;
  pl_t plx;

  cg = (cgmtr_cg_t *)ptr;
  
  cgnum = cg->pc_cg_num;
  
  /* Get CG lock. */
  plx = cgmtr_lock(cg->pc_lock, plhi);

  (void)cgmtr_getsample(cgnum, CGMTR_AT_TIMEOUT);

  if (cg->pc_max_interval == 0) {
    cg->pc_timeout_timer = CGMTR_TOID_NONE;

  } else {
    if (cg->pc_max_interval & TO_PERIODIC) {
      cmn_err(CE_WARN, "CGMTR:  timeout_cg(max_interval=%ld)\n",
        cg->pc_max_interval);
      UNLOCK(cg->pc_lock, plx);
      return;
    }

    /* Schedule the next update. */
    cg->pc_timeout_timer = cg_dtimeout(cgmtr_timeout_cg,
        (void *)cg, cg->pc_max_interval, plhi, cgnum);  
    if (cg->pc_timeout_timer == CGMTR_TOID_NONE) {
      cmn_err(CE_WARN, "CGMTR:  timeout_cg(unable to schedule timeout)\n");
    }
  }
  
  /* Release CG lock. */
  UNLOCK(cg->pc_lock, plx);
} /* cgmtr_timeout_cg() */


/*******************************************************
* cgmtr_update_interval()
*
* recalculates the update interval for the CG.
*/
static void
cgmtr_update_interval(cgmtr_cg_t *cg) {  
  uint_t min_usec;
  bool_t min_set;
  uint_t isetup;

  min_set = FALSE;
  for (isetup = 0; isetup < CGMTR_NUM_METERS; isetup++) {
    uint_t usec;

    if (cg->pc_meter[isetup].cg_setup_flags != CGMTR_SETUP_DEACTIVATE) {
      usec = cg->pc_setup->pss_max_interval[isetup];
      
      if (!min_set || usec < min_usec) {
        min_set = TRUE;
        min_usec = usec;
      }
    }
  }

  if (min_set) {
    cg->pc_max_interval = drv_usectohz(min_usec) - 1;
    if (cg->pc_max_interval < 1)
      cg->pc_max_interval = CGMTR_ASAP;
  } else {
    cg->pc_max_interval = 0;
  }
} /* cgmtr_update_interval() */


/*******************************************************
* cgmtr_setup_cg_setup()
*
* sets up a cg_setup for the current CG.
* pc_lock is held on entry and exit.
*/
static int
cgmtr_setup_cg_setup(cgmtr_cg_t *cg, cgmtr_cg_setup_t *cg_setup) {
  cgmtr_meter_id_t meter_id;
  uint_t num_setups;
  uint_t first_setup;
  uint_t end_setup;
  ushort setup_flags;
  cgmtr_sample_setup_t *setup_in;
  cgmtr_sample_setup_t *setup;

  meter_id = cg_setup->pps_meter_id;
  num_setups = cgmtr_num_setups(meter_id, &first_setup);
  end_setup = first_setup + num_setups;
  setup_flags = cg_setup->pps_setup_flags;
  setup_in = cg_setup->pps_sample_setup;

  setup = cg->pc_setup;
  if (setup == NULL) {
    size_t setup_bytes;
    int retcode;

    /* Allocate sample_setup for this CG. */
    setup_bytes = sizeof(cgmtr_sample_setup_t);
    setup = kmem_alloc(setup_bytes, KM_NOSLEEP);
    if (setup == NULL) {
      cmn_err(CE_WARN, "CGMTR:  setup_cg(setup_bytes=%u)\n", setup_bytes);
      return ENOMEM;
    }

    retcode = cgmtr_deactivate_setup(setup, 0, CGMTR_NUM_METERS, cg->pc_cgtype);
    if (retcode != CGMTR_NO_ERROR) {
      kmem_free(setup, setup_bytes);
      return retcode;
    }

    cg->pc_setup_bytes = setup_bytes;
    cg->pc_setup = setup;
  }

  /* Make changes to the CG setup. */
  if (setup_flags == CGMTR_SETUP_NOOP) {
    /* don't touch the metrics */

  } else if (setup_flags != CGMTR_SETUP_DEACTIVATE) {
    /* activate metrics */
    int isetup;

    cgmtr_copy_setup(setup, setup_in, first_setup, end_setup);

    for (isetup = first_setup; isetup < end_setup; isetup++) {
      cgmtr_meter_t *meter;

      meter = &(cg->pc_meter[isetup]);
      meter->cg_setup_flags = setup_flags;
      meter->cg_activating_user = meter->cg_reserving_user;
    }

  } else {
    /* deactivate metrics */
    int retcode;
    int isetup;

    for (isetup = first_setup; isetup < end_setup; isetup++)
      cg->pc_meter[isetup].cg_setup_flags = CGMTR_SETUP_DEACTIVATE;

    retcode = cgmtr_deactivate_setup(setup, first_setup,
      end_setup, cg->pc_cgtype);
    if (retcode != CGMTR_NO_ERROR) {
      return retcode;
    }
  }

  return CGMTR_NO_ERROR;
} /* cgmtr_setup_cg_setup() */


/*******************************************************
* cgmtr_setup_cg()
*
* sets up the current CG.
* pc_setup_process and pc_cg_setup must be set on entry.
*/
static void
cgmtr_setup_cg(void *ptr) {
  cgmtr_cg_t *cg;
  cgnum_t cgnum;
  pl_t plx;
  cgmtr_process_id_t process;
  uint_t old_trace_buffer_size;
  uint_t new_trace_buffer_size;
  cgmtr_process_t *proc;
  uint_t isetup;

  cg = (cgmtr_cg_t *)ptr;

  cgnum = cg->pc_cg_num;
  if (cg != &(cgmtr_cg[cgnum])) {
    cmn_err(CE_WARN, "CGMTR:  setup_cg(cg=%lx)\n", (long)cg);
    return;
  }

  plx = cgmtr_lock(cg->pc_lock, plhi);
  process = cg->pc_setup_process;
  if (process < 0 || process > CGMTR_MAX_PROCESSES) {
    cg->pc_retcode = EINVAL;
    UNLOCK(cg->pc_lock, plx);
    cmn_err(CE_WARN, "CGMTR:  setup_cg(process=%u)\n", process);
    return;
  }

  proc = &(cgmtr_process[process]);

  /*  Revise the CG setup by applying cg_setups from the ioctl.  */
  for (isetup = 0; isetup < proc->pp_num_cg_setups; isetup++) {
    cgmtr_cg_setup_t *cg_setup;
    cgmtr_meter_id_t meter_id;

    cg_setup = &(proc->pp_cg_setups[isetup]);
    meter_id = cg_setup->pps_meter_id;
    if (meter_id.cgi_cg_num == cgnum) {
      int retcode;

      retcode = cgmtr_setup_cg_setup(cg, cg_setup);
      if (retcode != CGMTR_NO_ERROR) {
        cg->pc_retcode = retcode;
        UNLOCK(cg->pc_lock, plx);
        return;
      }
      new_trace_buffer_size = cg_setup->pps_sampletable_size;
    }
  }

  /* Reallocate the trace buffer as necessary.  */
  old_trace_buffer_size = cg->pc_trace_buffer_size;
  if (new_trace_buffer_size > old_trace_buffer_size) {
    cgmtr_sample_t *trace;

    trace = kmem_alloc(new_trace_buffer_size*sizeof(cgmtr_sample_t),
      KM_NOSLEEP);
    if (trace == NULL) {
      cg->pc_retcode = ENOMEM;
      UNLOCK(cg->pc_lock, plx);
      cmn_err(CE_WARN, "CGMTR:  setup_cg(trace_bytes=%u)\n",
        new_trace_buffer_size*sizeof(cgmtr_sample_t));
      return;
    }

    kmem_free(cg->pc_trace, old_trace_buffer_size*sizeof(cgmtr_sample_t));

    cg->pc_trace_buffer_size = new_trace_buffer_size;
    cg->pc_trace = trace;
    cg->pc_trace_end = trace + new_trace_buffer_size;
    cg->pc_sample = trace;
    cg->pc_num_samples_taken = 0;
    cg->pc_num_samples_kept = 0;
  }

  /* Apply the revised CG setup to the hardware. */
  switch (cg->pc_cgtype) {
   case CGMTR_MCU:
    cgmtr_mcu_setup(cg->pc_setup);
    break;

   case CGMTR_PIU:
    cgmtr_piu_setup(cg->pc_setup);
    break;

   case CGMTR_TEST:
    cgmtr_test_setup(cg->pc_setup);
    break;

   default:
    cg->pc_retcode = ENXIO;
    UNLOCK(cg->pc_lock, plx);
    cmn_err(CE_WARN, "CGMTR:  setup_cg(cgtype=%u)\n", cg->pc_cgtype);
    return;
  }

  /* Calculate and apply new update interval. */
  {
    clock_t old_interval;
    clock_t new_interval;

    old_interval = cg->pc_max_interval;
    cgmtr_update_interval(cg);
    new_interval = cg->pc_max_interval;

    if (new_interval > 0
     && (old_interval <= 0 || new_interval < old_interval)) {
      toid_t toid;

      toid = cg->pc_timeout_timer; 
      if (toid != CGMTR_TOID_NONE) {
        cg->pc_timeout_timer = CGMTR_TOID_NONE;
        UNLOCK(cg->pc_lock, plx);
        untimeout(toid); 
        plx = cgmtr_lock(cg->pc_lock, plhi);
      } 

      /* Schedule the next timout. */
      cg->pc_timeout_timer = cg_dtimeout(cgmtr_timeout_cg,
        (void *)cg, cg->pc_max_interval, plhi, cgnum);  
      if (cg->pc_timeout_timer == CGMTR_TOID_NONE) {
        cg->pc_retcode = ENXIO;
        UNLOCK(cg->pc_lock, plx);
        cmn_err(CE_WARN, "CGMTR:  setup_cg(unable to schedule timeout)\n");
      }
    }
  }

  /* This setup task is complete. */
  cg->pc_setup_process = CGMTR_PROCESS_NONE;

  UNLOCK(cg->pc_lock, plx);
  cgmtr_barrier(process);
} /* cgmtr_setup_cg() */


/*******************************************************
* cgmtr_watchdog()
*
* called when a task has hung because a CG is offline.
*/
static void
cgmtr_watchdog(int process) {
  pl_t plx;
  cgmtr_process_t *proc;

  /* Check that process is legal. */
  if (process < 0 || process > CGMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CGMTR:  watchdog(%u)\n", process);
    return;
  }
  proc = &(cgmtr_process[process]);

  /* Get process lock. */
  plx = cgmtr_lock(proc->pp_lock, plhi);

  /* Clear watchdog. */
  proc->pp_watchdog = CGMTR_TOID_NONE;

  /* Wake up hung process. */
  SV_SIGNAL(proc->pp_svp, 0);

  /* Release lock. */
  UNLOCK(proc->pp_lock, plx);
} /* cgmtr_watchdog() */


/*******************************************************
* cgmtr_join()
*
* join spawned tasks, with watchdog.
*
* Unlocks the pp_lock, which must be held on entry.
*/
static int
cgmtr_join(cgmtr_process_id_t process, pl_t plx,
  cgmtr_tracepoint_t point) {
  cgmtr_process_t *proc;
  cgnum_t cgnum;

  /* Check that process is legal. */
  if (process < 0 || process > CGMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CGMTR:  join(%u)\n", process);
    return EINVAL;
  }
  proc = &(cgmtr_process[process]);

  if (proc->pp_count < 1) {
    UNLOCK(proc->pp_lock, plx);
    return CGMTR_NO_ERROR;
  }

  /* kick off watchdog */
  proc->pp_watchdog = itimeout(cgmtr_watchdog,
    (void *)process, drv_usectohz(CGMTR_WATCHDOG_USEC), plhi);

  /* if watchdog not scheduled */
  if (proc->pp_watchdog == CGMTR_TOID_NONE) {
    UNLOCK(proc->pp_lock, plx);
    cmn_err(CE_WARN, "CGMTR:  join(pp_watchdog=0)\n");
    for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
      cgmtr_cg_t *cg;

      cg = &(cgmtr_cg[cgnum]);
      switch (point) {
        case CGMTR_AT_START:
          untimeout(cg->pc_start_timer); /* must be done outside lock */
          cg->pc_start_timer = CGMTR_TOID_NONE;
          break;

        case CGMTR_AT_SETUP:
          untimeout(cg->pc_setup_timer); /* must be done outside lock */
          cg->pc_setup_timer = CGMTR_TOID_NONE;
          break;

        case CGMTR_AT_UPDATE:
          untimeout(cg->pc_update_timer); /* must be done outside lock */
          cg->pc_update_timer = CGMTR_TOID_NONE;
          break;

        default:
          cmn_err(CE_WARN, "CGMTR:  join(point=%d)\n", point);
          return EINVAL;
      }
    }
    proc->pp_count = 0;
    return ENXIO;
  }

  /* Wait for tasks to complete. */
  SV_WAIT(proc->pp_svp, prilo, proc->pp_lock);

  if (proc->pp_watchdog != CGMTR_TOID_NONE) {
    /* All CGs beat the watchdog; cancel it. */
    untimeout(proc->pp_watchdog);
    proc->pp_watchdog = CGMTR_TOID_NONE;
    return CGMTR_NO_ERROR;
  } 

  cmn_err(CE_WARN, "CGMTR:  join(watchdog went off)\n");

  for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
    cgmtr_cg_t *cg;

    cg = &(cgmtr_cg[cgnum]);
    switch (point) {
      case CGMTR_AT_START:
        if (cg->pc_start_timer != CGMTR_TOID_NONE) {
          cmn_err(CE_WARN, "CGMTR:  join(watchdog start cgnum=%d)\n", cgnum);
          untimeout(cg->pc_start_timer);
          cg->pc_start_timer = CGMTR_TOID_NONE;
        }
        break;

      case CGMTR_AT_SETUP:
        if (cg->pc_setup_timer != CGMTR_TOID_NONE) {
          cmn_err(CE_WARN, "CGMTR:  join(watchdog setup cgnum=%d)\n", cgnum);
          untimeout(cg->pc_setup_timer);
          cg->pc_setup_timer = CGMTR_TOID_NONE;
        }
        break;

      case CGMTR_AT_UPDATE:
        if (cg->pc_update_timer != CGMTR_TOID_NONE) {
          cmn_err(CE_WARN, "CGMTR:  join(watchdog update cgnum=%d)\n", cgnum);
          untimeout(cg->pc_update_timer);
          cg->pc_update_timer = CGMTR_TOID_NONE;
        }
        break;

      default:
        cmn_err(CE_WARN, "CGMTR:  join(point=%d)\n", point);
    }
  }
  proc->pp_count = 0;

  return ETIME;
} /* cgmtr_join() */


/*******************************************************
* cgmtr_update()
*
* update all virtual meters.
*/
static int
cgmtr_update(cgmtr_process_id_t process, cgmtr_tracepoint_t tracepoint) {
  cgmtr_process_t *proc;
  cgnum_t cgnum;
  pl_t plx;

  proc = &(cgmtr_process[process]);
  plx = cgmtr_lock(proc->pp_lock, plhi);

  proc->pp_count = cgmtr_num_cgs;
  for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
    cgmtr_cg_t *cg;
    pl_t ply;

    cg = &(cgmtr_cg[cgnum]);
    ply = cgmtr_lock(cg->pc_lock, plhi);
    cg->pc_update_process = process;
    cg->pc_update_tracepoint = tracepoint;
    cg->pc_update_timer = cg_dtimeout(cgmtr_update_cg,
      (void *)cg, CGMTR_ASAP, plhi, cgnum);
    if (cg->pc_update_timer == CGMTR_TOID_NONE) {
      cmn_err(CE_WARN, "CGMTR:  update(no timer on CG %d)\n", cgnum);
      proc->pp_count--;
    }
    UNLOCK(cg->pc_lock, ply);
  }

  /* Wait for CG startups to complete, even if there were errors. */
  /* This releases the pp_lock. */
  return cgmtr_join(process, plx, CGMTR_AT_UPDATE);
} /* cgmtr_update() */


/*******************************************************
* cgmtr_rdstats_cg()
* 
* copies out the CG stats to the user process.
*/
static int
cgmtr_rdstats_cg(cgmtr_rdstats_t *prdstats) {
  int retcode;
  cgnum_t cgnum;
  cgmtr_cg_stats_t *cg_stats; /* array of CG stats */
  size_t cgstats_bytes; /* size of cg_stats in bytes */
  pl_t plx;

  /* Check that the number of CGs is correct. */
  if (prdstats->pr_num_cgs != cgmtr_num_cgs) {
    cmn_err(CE_WARN, "CGMTR:  rdstats_cg(num_cg=%u)\n", prdstats->pr_num_cgs);
    return EINVAL;
  }
    
  /* Allocate array of CG stats. */
  cgstats_bytes = prdstats->pr_num_cgs*sizeof(cgmtr_cg_stats_t);
  cg_stats = kmem_alloc(cgstats_bytes, KM_SLEEP);
  if (cg_stats == NULL) {
    cmn_err(CE_WARN, "CGMTR:  rdstats_cg(cgstats_bytes=%u)\n",
      cgstats_bytes);
    return ENOMEM;
  }

  /* Copy CG stats into array. */
  for (cgnum = 0; cgnum < prdstats->pr_num_cgs; cgnum++) {
    cgmtr_cg_t *cg;
    cgmtr_cg_stats_t *stats;
    u_int isetup;

    stats = &(cg_stats[cgnum]);
    cg = &(cgmtr_cg[cgnum]);

    /* Copy constant portion of cgstats. */
    stats->pcs_cgtype = cg->pc_cgtype;

    /* Copy variable portion of cgstats. */
    plx = cgmtr_lock(cg->pc_lock, plhi); 
    for (isetup = 0; isetup < CGMTR_NUM_METERS; isetup++) {
      stats->pcs_count[isetup] = cg->pc_meter[isetup].cg_count;
    }

    if (cg->pc_setup != NULL) {
      for (isetup = 0; isetup < CGMTR_NUM_METERS; isetup++) {
        cgmtr_meter_t *meter;

        meter = &(cg->pc_meter[isetup]);
        stats->pcs_setup_flags[isetup] = meter->cg_setup_flags;
        stats->pcs_userid[isetup] = meter->cg_activating_user;
      }
      cgmtr_copy_setup(&(stats->pcs_setup), cg->pc_setup,
        0, CGMTR_NUM_METERS);
    } else {
      for (isetup = 0; isetup < CGMTR_NUM_METERS; isetup++) {
        stats->pcs_setup_flags[isetup] = CGMTR_SETUP_DEACTIVATE;
      }
      bzero(&(stats->pcs_setup), sizeof(cgmtr_sample_setup_t));
    }
    UNLOCK(cg->pc_lock, plx); 
  }

  /* Copyout and free the CG stats. */
  retcode = copyout(cg_stats, prdstats->pr_cg_stats, cgstats_bytes);
  kmem_free(cg_stats, cgstats_bytes);
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  rdstats_cg(pr_cg_stats=0x%lx)\n",
      (ulong_t)prdstats->pr_cg_stats);
    return EFAULT;
  }

  return CGMTR_NO_ERROR;
} /* cgmtr_rdstats_cg() */


/*******************************************************
* cgmtr_rdstats()
* 
* implements CGMTR_CMD_RDSTATS, copying out LWP/CG stats
* to the user process.
*/
static int
cgmtr_rdstats(caddr_t arg, cgmtr_process_id_t process) {
  int retcode;
  cgmtr_rdstats_t rdstats; /* copy of user structure */

  /* Copyin user rdstats structure. */
  retcode = copyin(arg, &rdstats, sizeof(rdstats));
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  rdstats(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  if (rdstats.pr_cg_stats != NULL) {
    retcode = cgmtr_update(process, CGMTR_AT_RDSTATS);
    if (retcode != CGMTR_NO_ERROR)
      return retcode;
  }

  if (rdstats.pr_cg_stats == NULL) {
    /* Suggest a number of CGs, but don't copy CG stats. */
    rdstats.pr_num_cgs = cgmtr_num_cgs;

  } else {
    retcode = cgmtr_rdstats_cg (&rdstats);
    if (retcode != CGMTR_NO_ERROR)
      return retcode;
  }

  retcode = copyout(&rdstats, arg, sizeof(rdstats));
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  rdstats(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  return CGMTR_NO_ERROR;
} /* cgmtr_rdstats() */


/*******************************************************
* cgmtr_rdsamples()
* 
* implements CGMTR_CMD_RDSAMPLES, copying out a trace of
* samples to the user process.
*/
static int
cgmtr_rdsamples(caddr_t arg, cgmtr_process_id_t process) {
  cgmtr_rdsamples_t rdsamples;         /* copy of user structure */
  cgmtr_cg_rdsamples_t *cg_samples; /* array of CG samples */
  cgmtr_sample_t *trace;
  size_t cgsamples_bytes;      /* size of cg_samples in bytes */
  size_t trace_bytes;      /* size of cg_samples in bytes */
  cgnum_t cgnum;
  cgmtr_process_t *proc;
  int retcode;

  /* Copyin user rdsamples structure. */
  retcode = copyin(arg, &rdsamples, sizeof(rdsamples));
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  rdsamples(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  if (rdsamples.pd_cg_samples == NULL) {
    /* Suggest a number of CGs. */
    rdsamples.pd_num_cgs = cgmtr_num_cgs;

    /* Copyout user arg structure. */
    retcode = copyout(&rdsamples, arg, sizeof(rdsamples));
    if (retcode != CGMTR_NO_ERROR) {
      cmn_err(CE_WARN, "CGMTR:  rdsamples(arg=0x%lx)\n", (ulong_t)arg);
      return EFAULT;
    }
    return CGMTR_NO_ERROR;
  }

  /* Check that the number of CGs is correct. */
  if (rdsamples.pd_num_cgs != cgmtr_num_cgs) {
    cmn_err(CE_WARN, "CGMTR:  rdsamples(num_cg=%u)\n", rdsamples.pd_num_cgs);
    return EINVAL;
  }
  
  /* Allocate array of CG info and copy it in. */
  cgsamples_bytes = rdsamples.pd_num_cgs * sizeof(cgmtr_cg_rdsamples_t);
  cg_samples = kmem_alloc(cgsamples_bytes, KM_SLEEP);
  if (cg_samples == NULL) {
    cmn_err(CE_WARN, "CGMTR:  rdsamples(cgsamples_bytes=%u)\n",
      cgsamples_bytes);
    return ENOMEM;
  }
  retcode = copyin(rdsamples.pd_cg_samples, cg_samples, cgsamples_bytes);
  if (retcode != CGMTR_NO_ERROR) {
    kmem_free(cg_samples, cgsamples_bytes);
    cmn_err(CE_WARN, "CGMTR:  rdsamples(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  for (cgnum = 0; cgnum < rdsamples.pd_num_cgs; cgnum++) {
    pl_t ply;
    cgmtr_cg_t *cg;

    cg = &(cgmtr_cg[cgnum]);

    ply = cgmtr_lock(cg->pc_lock, plhi);
    if (cg->pc_trace_process != CGMTR_PROCESS_NONE) {
      cmn_err(CE_WARN, "CGMTR:  rdsamples(trace_process[%d]=%u)\n",
        cgnum, cg->pc_trace_process);
      UNLOCK(cg->pc_lock, ply);
      kmem_free(cg_samples, cgsamples_bytes);
      /* Back out as gracefully as we can. */
      while (cgnum != 0) {
        cgnum--;
        cg = &(cgmtr_cg[cgnum]);
        ply = cgmtr_lock(cg->pc_lock, plhi);
        cg->pc_trace_process = CGMTR_PROCESS_NONE;  
        UNLOCK(cg->pc_lock, ply);
      }
      return EBUSY;
    }
    if (rdsamples.pd_block_until > cg->pc_trace_buffer_size) {
      cmn_err(CE_WARN, "CGMTR:  rdsamples(trace_buffer_size[%d]=%u)\n",
        cgnum, cg->pc_trace_buffer_size);
      cg->pc_block_until = cg->pc_trace_buffer_size;
    } else {
      cg->pc_block_until = rdsamples.pd_block_until;
    }
    cg->pc_trace_process = process;
    UNLOCK(cg->pc_lock, ply);
  }

  proc = &(cgmtr_process[process]);
  (void)cgmtr_lock(proc->pp_lock, plhi);

  /* Wait for the buffer to fill.  This releases the pp_lock. */
  SV_WAIT(proc->pp_svp, prilo, proc->pp_lock);

  /* Cancel the trace request for each CG. */
  for (cgnum = 0; cgnum < rdsamples.pd_num_cgs; cgnum++) {
    pl_t ply;
    cgmtr_cg_t *cg;

    cg = &(cgmtr_cg[cgnum]);

    ply = cgmtr_lock(cg->pc_lock, plhi);
    if (cg->pc_trace_process != process) {
      cmn_err(CE_WARN, "CGMTR:  rdsamples(cancel trace_process[%d]=%u)\n",
        cgnum, cg->pc_trace_process);
    } else {
      cg->pc_trace_process = CGMTR_PROCESS_NONE;
    }
    UNLOCK(cg->pc_lock, ply);
  }

  /*
   * Allocate temporary trace buffer.
   * This is necessary because we cannot hold driver spinlocks
   * across a copyout call.
   */
  trace_bytes = rdsamples.pd_max_samples * sizeof(cgmtr_sample_t);
  trace = kmem_alloc(trace_bytes, KM_SLEEP);
  if (trace == NULL) {
    cmn_err(CE_WARN, "CGMTR:  rdsamples(trace_bytes=%u)\n", trace_bytes);
    return ENOMEM;
  }

  /* Copy each CG's trace. */
  for (cgnum = 0; cgnum < rdsamples.pd_num_cgs; cgnum++) {
    pl_t ply;
    cgmtr_cg_t *cg;
    uint_t samples_taken;
    uint_t samples_returned;
    cgmtr_cg_rdsamples_t *out;

    cg = &(cgmtr_cg[cgnum]);

    ply = cgmtr_lock(cg->pc_lock, plhi);
    samples_taken = cg->pc_num_samples_taken;
    samples_returned = cg->pc_num_samples_kept;

    if (rdsamples.pd_max_samples < samples_returned)
      samples_returned = rdsamples.pd_max_samples;
    
    if (cg->pc_trace + samples_returned > cg->pc_sample) {
      /* Copy two chunks of trace. */
      uint_t first;
      uint_t second;
      uint_t i;

      second = cg->pc_sample - cg->pc_trace;
      first = samples_returned - second;

      for (i = 0; i < first; i++)
        trace[i] = cg->pc_trace_end[i - first];
      for (i = 0; i < second; i++)
        trace[first + i] = cg->pc_trace[i];
      
    } else {
      /* Copy single chunk of trace. */
      uint_t i;

      for (i = 0; i < samples_returned; i++)
        trace[i] = cg->pc_sample[i - samples_returned];
    }
    UNLOCK(cg->pc_lock, ply);

    out = &(cg_samples[cgnum]);
    retcode = copyout(trace, out->ppd_samples,
      samples_returned * sizeof(cgmtr_sample_t));
    if (retcode != CGMTR_NO_ERROR) {
      kmem_free(cg_samples, cgsamples_bytes);
      kmem_free(trace, trace_bytes);
      cmn_err(CE_WARN, "CGMTR:  rdsamples([%d].ppd_samples=0x%lx)\n",
        cgnum, (ulong_t)out->ppd_samples);
      return EFAULT;
    }

    out->ppd_cg_num = cgnum;
    out->ppd_num_samples = samples_returned;
    out->ppd_num_samples_lost = samples_taken - samples_returned;
  }

  /* Free temporary trace buffer. */
  kmem_free(trace, trace_bytes);

  /* Copy out the CG info. */
  retcode = copyout(cg_samples, rdsamples.pd_cg_samples, cgsamples_bytes);
  kmem_free(cg_samples, cgsamples_bytes);
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  rdsamples(pd_cg_samples=0x%lx)\n",
      (ulong_t)rdsamples.pd_cg_samples);
    return EFAULT;
  }

  return CGMTR_NO_ERROR;
} /* cgmtr_rdsamples() */

/*******************************************************
* cgmtr_reserve()
*
* If res_flag != FALSE reserve meters to the specified user/process.
* If res_flag == FALSE release meters reserved to the specified user/process.
*/
static int
cgmtr_reserve(caddr_t arg, cgmtr_process_id_t process, bool_t res_flag) {
  cgmtr_reserve_t reserve;
  uint_t isetup;
  int num_setups;
  uint_t first_setup;
  cgmtr_meter_t *first_meter_ptr;
  pl_t plx;
  cgmtr_meter_id_t meter_id;
  cgmtr_user_id_t user_id;
  cgmtr_cg_t *cg;
  int retcode;

  /* Copyin user reserve structure. */
  retcode = copyin(arg, &reserve, sizeof(reserve));
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  reserve(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  meter_id = reserve.pv_meter_id;
  user_id = reserve.pv_user_id;

  /* Set first_setup and num_setups. */
  num_setups = cgmtr_num_setups (meter_id, &first_setup);

  /* Check that the meter_id was valid. */
  if (num_setups == -1) {
    cmn_err(CE_WARN, "CGMTR:  reserve(meter_id=%x:%x:%x)\n",
      meter_id.cgi_type, meter_id.cgi_instance, meter_id.cgi_cg_num);
    return EINVAL;
  }
  if (first_setup >= CGMTR_NUM_METERS) {
    cmn_err(CE_WARN, "CGMTR:  reserve(setup=%u)\n", first_setup);
    return EINVAL;
  }
  cg = &(cgmtr_cg[meter_id.cgi_cg_num]);
  first_meter_ptr = &(cg->pc_meter[first_setup]);

  plx = cgmtr_lock(cg->pc_lock, plhi);

  /* Check that none of the setups are reserved by another processs
     or activated by another user. */
  for (isetup = 0; isetup < num_setups; isetup++) {
    cgmtr_meter_t *meter;

    meter = &(first_meter_ptr[isetup]);
    if (meter->cg_reserving_process != CGMTR_PROCESS_NONE
      && meter->cg_reserving_process != process)
      return EBUSY;

    if (meter->cg_setup_flags != CGMTR_SETUP_DEACTIVATE
     && meter->cg_activating_user != user_id)
      return EBUSY;
  }

  for (isetup = 0; isetup < num_setups; isetup++) {
    cgmtr_meter_t *meter;

    meter = &(first_meter_ptr[isetup]);
    if (res_flag) {
      /* Reserve the meter. */
      meter->cg_reserving_process = process;
      meter->cg_reserving_user = user_id;
    } else {
      /* Release the meter. */
      meter->cg_reserving_process = CGMTR_PROCESS_NONE;
    }
  }
  UNLOCK(cg->pc_lock, plx);
    
  return CGMTR_NO_ERROR;
} /* cgmtr_reserve() */


/*******************************************************
* cgmtr_setup()
*
* implements CGMTR_CMD_SETUP, setting up meters.
*/
static int
cgmtr_setup(caddr_t arg, cgmtr_process_id_t process) {
  int retcode;
  int i;
  cgmtr_setup_t setup;
  uint_t num_cg_setups;
  pl_t plx;
  cgmtr_process_t *proc;
  cgnum_t cgnum;

  cgmtr_cg_setup_t *cg_setups; /* array of CG setups */
  size_t cgsetups_bytes;         /* size of array in bytes */
  cgmtr_sample_setup_t *css;     /* array of CG sample setups */
  size_t pss_bytes;               /* size of array in bytes */

  /* Copyin user structure. */
  retcode = copyin(arg, &setup, sizeof(setup));
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  setup(arg=0x%lx)\n", (ulong_t)arg);
    return EFAULT;
  }

  /* Check that number of CG setups is legal. */
  num_cg_setups = setup.ps_num_cg_setups;
  if (num_cg_setups < 1) {
    cmn_err(CE_WARN, "CGMTR:  setup(num_cg_setups=%u)\n", num_cg_setups);
    return EINVAL;
  }

  /* Allocate array of CG setups and copyin. */
  cgsetups_bytes = num_cg_setups * sizeof(cgmtr_cg_setup_t);
  cg_setups = kmem_alloc(cgsetups_bytes, KM_SLEEP);
  if (cg_setups == NULL) {
    cmn_err(CE_WARN, "CGMTR:  setup(cgsetups_bytes=%u)\n", cgsetups_bytes);
    return ENOMEM;
  }
  retcode = copyin(setup.ps_cg_setups, cg_setups, cgsetups_bytes);
  if (retcode != CGMTR_NO_ERROR) {
    kmem_free(cg_setups, cgsetups_bytes);
    cmn_err(CE_WARN, "CGMTR:  setup(ps_cg_setups=0x%lx)\n",
      (ulong_t)setup.ps_cg_setups);
    return EFAULT;
  }

  /* Allocate array of CG sample setups and copyin. */
  pss_bytes = num_cg_setups * sizeof(cgmtr_sample_setup_t);
  css = kmem_alloc(pss_bytes, KM_SLEEP);
  if (css == NULL) {
    kmem_free(cg_setups, cgsetups_bytes);
    cmn_err(CE_WARN, "CGMTR:  setup(pss_bytes=%u)\n", pss_bytes);
    return ENOMEM;
  }
  for (i = 0; i < num_cg_setups; i++) {
    if (cg_setups[i].pps_samplesetups_size == 1) {
      retcode = copyin(cg_setups[i].pps_sample_setup, &(css[i]),
        sizeof(cgmtr_sample_setup_t));
      if (retcode != CGMTR_NO_ERROR) {
        cmn_err(CE_WARN, "CGMTR:  setup(pps_sample_setup=0x%lx)\n",
          (ulong_t)cg_setups[i].pps_sample_setup);
        kmem_free(cg_setups, cgsetups_bytes);
        kmem_free(css, pss_bytes);
        return EFAULT;
      }
      cg_setups[i].pps_sample_setup = &(css[i]);
    } else {
      cg_setups[i].pps_sample_setup = NULL;
    }
  }

  /* Check each CG setup */
  for (i = 0; i < num_cg_setups; i++) {
    cgmtr_cg_setup_t *cps;
    cgmtr_meter_id_t meter_id;
    int num_setups;
    uint_t first_setup;
    cgmtr_cg_t *cg;
    cgmtr_meter_t *first_meter_ptr;
    cgmtr_cgtype_t cgtype;
    uint_t setup_flags;
    uint_t num_sample_setups;
    uint_t isetup;

    cps = &(cg_setups[i]);

    /* Check that the setup_flags are valid. */
    setup_flags = cps->pps_setup_flags;
    if (setup_flags != CGMTR_SETUP_ACTIVATE
     && setup_flags != CGMTR_SETUP_NOOP
     && setup_flags != CGMTR_SETUP_DEACTIVATE) {
      kmem_free(cg_setups, cgsetups_bytes);
      kmem_free(css, pss_bytes);
      cmn_err(CE_WARN, "CGMTR:  setup([%d].pps_setup_flags=0x%x)\n",
        i, setup_flags);
      return EINVAL;
    }

    /* Set first_setup and num_setups. */
    meter_id = cps->pps_meter_id;
    num_setups = cgmtr_num_setups (meter_id, &first_setup);

    /* Check that the meter_id was valid. */
    if (num_setups == -1) {
      kmem_free(cg_setups, cgsetups_bytes);
      kmem_free(css, pss_bytes);
      cmn_err(CE_WARN, "CGMTR:  setup([%d].pps_meter_id=%x:%x:%x)\n",
         i, meter_id.cgi_type, meter_id.cgi_instance, meter_id.cgi_cg_num);
      return EINVAL;
    }
    if (first_setup >= CGMTR_NUM_METERS) {
      kmem_free(cg_setups, cgsetups_bytes);
      kmem_free(css, pss_bytes);
      cmn_err(CE_WARN, "CGMTR:  setup([%d].setup=%u)\n", i, first_setup);
      return EINVAL;
    }

    cg = &(cgmtr_cg[meter_id.cgi_cg_num]);
    first_meter_ptr = &(cg->pc_meter[first_setup]);

    /* Make sure the CG type is valid. */
    cgtype = cps->pps_cgtype;
    if (cgtype != cg->pc_cgtype || cgtype == CGMTR_NOT_SUPPORTED) {
      kmem_free(cg_setups, cgsetups_bytes);
      kmem_free(css, pss_bytes);
      cmn_err(CE_WARN, "CGMTR:  setup([%d].pps_cgtype=%u)\n", i, cgtype);
      return EINVAL;
    }

    /* Multiple sample setups per CG are not supported. */
    num_sample_setups = cps->pps_samplesetups_size;
    if (num_sample_setups != (cps->pps_sample_setup != NULL)) {
      kmem_free(cg_setups, cgsetups_bytes);
      kmem_free(css, pss_bytes);
      cmn_err(CE_WARN, "CGMTR:  setup([%d].pps_samplesetups_size=%u)\n",
        i, num_sample_setups);
      return EINVAL;
    }

    /* Make sure this process has reserved the meters. */
    for (isetup = 0; isetup < num_setups; isetup++) {
      cgmtr_meter_t *meter;

      meter = &(first_meter_ptr[isetup]);
      if (meter->cg_reserving_process != process
       && setup_flags != CGMTR_SETUP_NOOP) {
        kmem_free(cg_setups, cgsetups_bytes);
        kmem_free(css, pss_bytes);
        cmn_err(CE_WARN,
          "CGMTR:  setup(process=%u, reserving_process[%u]=%d)\n",
          process, i, meter->cg_reserving_process);
        return EBUSY;
      }
    }
  }

  for (i = 0; i < num_cg_setups; i++) {
    cgmtr_cg_setup_t *cps;
    cgmtr_meter_id_t meter_id;
    cgmtr_cg_t *cg;
    cgmtr_process_id_t setup_process;

    cps = &(cg_setups[i]);
    meter_id = cps->pps_meter_id;
    cgnum = meter_id.cgi_cg_num;
    cg = &(cgmtr_cg[cgnum]);

    plx = cgmtr_lock(cg->pc_lock, plhi);
    setup_process = cg->pc_setup_process;
    if (setup_process == CGMTR_PROCESS_NONE || setup_process == process) {
      cg->pc_setup_process = process;
      cg->pc_retcode = CGMTR_NO_ERROR;
      UNLOCK(cg->pc_lock, plx);
    } else {
      int j;

      UNLOCK(cg->pc_lock, plx);
      for (j = 0; j < i; j++) {
        cps = &(cg_setups[i]);
        meter_id = cps->pps_meter_id;
        cgnum = meter_id.cgi_cg_num;
        cg = &(cgmtr_cg[cgnum]);
        plx = cgmtr_lock(cg->pc_lock, plhi);
        cg->pc_setup_process = CGMTR_PROCESS_NONE; 
        UNLOCK(cg->pc_lock, plx);
      }
      kmem_free(cg_setups, cgsetups_bytes);
      kmem_free(css, pss_bytes);
      cmn_err(CE_WARN, "CGMTR:  setup(process=%u, setup_process[%u]=%u)\n",
        process, i, setup_process);
      return EBUSY;
    }
  }

  /* Set up CGs together, as close together in time as possible. */

  proc = &(cgmtr_process[process]);

  plx = cgmtr_lock(proc->pp_lock, plhi);

  /* Set up the meters */
  proc->pp_num_cg_setups = num_cg_setups;
  proc->pp_cg_setups = cg_setups;
  proc->pp_count = cgmtr_num_cgs;
  retcode = CGMTR_NO_ERROR;
  for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
    cgmtr_cg_t *cg;

    cg = &(cgmtr_cg[cgnum]);

    if (cg->pc_setup_process != process) {
      proc->pp_count--;
    } else {
      cg->pc_setup_timer = cg_dtimeout(cgmtr_setup_cg,
        (void *)cg, CGMTR_ASAP, plhi, cgnum);  
      if (cg->pc_setup_timer == CGMTR_TOID_NONE) {
        cmn_err(CE_WARN, "CGMTR:  setup(no timer on CG %d)\n", cgnum);
        proc->pp_count--;
        retcode = ENXIO;
        break;
      }
    }
  }

  /* Wait for setup to complete even if an error. */
  /* This releases the pp_lock. */
  retcode = cgmtr_join(process, plx, CGMTR_AT_SETUP);

  /* Free the kernel memory allocated above. */
  proc->pp_cg_setups = NULL;

  if (retcode != CGMTR_NO_ERROR) {
    kmem_free(cg_setups, cgsetups_bytes);
    kmem_free(css, pss_bytes);
    return retcode;
  }

  /* Check for any errors. */
  for (i = 0; i < num_cg_setups; i++) {
    cgmtr_cg_setup_t *cps;
    cgmtr_meter_id_t meter_id;
    cgnum_t cgnum;
    cgmtr_cg_t *cg;

    cps = &(cg_setups[i]);
    meter_id = cps->pps_meter_id;
    cgnum = meter_id.cgi_cg_num;
    cg = &(cgmtr_cg[cgnum]);

    plx = cgmtr_lock(cg->pc_lock, plhi);
    if (retcode == CGMTR_NO_ERROR) {
      retcode = cg->pc_retcode;
      cg->pc_retcode = CGMTR_NO_ERROR;
    }
    cg->pc_setup_process = CGMTR_PROCESS_NONE;
    UNLOCK(cg->pc_lock, plx);
  }

  kmem_free(cg_setups, cgsetups_bytes);
  kmem_free(css, pss_bytes);
  return retcode;
} /* cgmtr_setup() */


/*********************************************
* cgmtropen()
*
* This function is called when a process opens the
* driver. It checks the open type and does appropriate
* actions
*/
/* ARGSUSED */
int
cgmtropen(dev_t *devp, int flags, int otyp, struct cred *cred_p) {
  cgmtr_process_id_t process;
  pl_t plx;

  /* Get open lock. */
  plx = cgmtr_lock(cgmtr_open_lock, plbase);

  /* Check there is a process to allocate. */
  if (cgmtr_num_processes >= CGMTR_MAX_PROCESSES) {
    UNLOCK(cgmtr_open_lock, plx);
    return EAGAIN;
  }

  /* Find free process id (start at 1 as 0 is reserved for driver). */
  for (process = 1; process <= CGMTR_MAX_PROCESSES; process++) {
    if (cgmtr_open_flag[process] == FALSE)
      break;
  }

  cgmtr_num_processes ++;
  cgmtr_open_flag[process] = TRUE;  

  /* Release the open lock. */
  UNLOCK(cgmtr_open_lock, plx);

  /* Change the minor number so that this process
     can be identified on ioctl and close calls. */
  *devp = makedevice(getemajor(*devp), process);

  return CGMTR_NO_ERROR;
} /* cgmtropen() */


/*******************************************************
* cgmtrioctl()
*
* called on an ioctl() by the process.
*/
/* ARGSUSED */
int
cgmtrioctl(dev_t dev, int cmd, caddr_t arg, int mode, struct cred *cred_p,
  int *rval_p)
{
  cgmtr_process_id_t process;

  /* Get id for process calling the driver. */
  if (geteminor(dev) < 1 || geteminor(dev) > CGMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CGMTR:  ioctl(process=%lu)\n", geteminor(dev));
    return EINVAL;
  }
  process = geteminor(dev);

  switch (cmd) {
   case CGMTR_CMD_SETUP:
    return cgmtr_setup(arg, process);
    
   case CGMTR_CMD_RDSAMPLES:
    return cgmtr_rdsamples(arg, process);

   case CGMTR_CMD_RESERVE:
    return cgmtr_reserve(arg, process, TRUE);

   case CGMTR_CMD_RELEASE:
    return cgmtr_reserve(arg, process, FALSE);

   case CGMTR_CMD_RDSTATS:
    return cgmtr_rdstats(arg, process);
  }

  cmn_err(CE_WARN, "CGMTR:  ioctl(cmd=%x)\n", cmd);
  return EINVAL;
} /* cgmtrioctl() */


/*******************************************************
* cgmtrclose()
*
* called on close of driver.
* It releases the meters that were reserved by this process.
*/
/* ARGSUSED */
int
cgmtrclose(dev_t dev, int flags, int otyp, struct cred *cred_p) {
  pl_t plx;
  cgmtr_process_id_t process;
  cgnum_t cgnum;

  /* Get id for process calling the driver. */
  if (geteminor(dev) < 1 || geteminor(dev) > CGMTR_MAX_PROCESSES) {
    cmn_err(CE_WARN, "CGMTR:  close(process=%lu)\n", geteminor(dev));
    return EINVAL;
  }
  process = geteminor(dev);

  /* Release all meters reserved by this process. */
  for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
    uint_t i;
    cgmtr_cg_t *cg;
    pl_t plx;

    cg = &(cgmtr_cg[cgnum]);

    plx = cgmtr_lock(cg->pc_lock, plhi);
    for (i = 0; i < CGMTR_NUM_METERS; i++) {
      cgmtr_meter_t *meter;

      meter = &(cg->pc_meter[i]);
      if (meter->cg_reserving_process == process)
        meter->cg_reserving_process = CGMTR_PROCESS_NONE;
    }
    UNLOCK(cg->pc_lock, plx);
  }

  /* Deallocate the per-process structure. */
  plx = cgmtr_lock(cgmtr_open_lock, plbase);
  cgmtr_open_flag[process] = FALSE;
  cgmtr_num_processes --;
  UNLOCK(cgmtr_open_lock, plx);

  return CGMTR_NO_ERROR;
} /* cgmtr_close() */


/*******************************************************
* cgmtr_alloc()
*
* allocate dynamic memory needed by the driver (except per-CG memory).
*/
static int
cgmtr_alloc(int sleep_flag) {
  cgmtr_process_id_t process;
  cgmtr_process_t *proc;

  cgmtr_open_lock = LOCK_ALLOC(1, plbase, &cgmtr_lkinfo_reservelock,
    sleep_flag);
  if (cgmtr_open_lock == NULL) {
    cmn_err(CE_WARN, "CGMTR:  alloc(open_lock=NULL)\n");
    return ENOMEM;
  }

  /* Initialize each process structure (CGMTR_MAX_PROCESSES+1).  */
  for (process = 0; process <= CGMTR_MAX_PROCESSES; process++) {
    proc = &(cgmtr_process[process]);
    proc->pp_lock = LOCK_ALLOC(1, plhi, &cgmtr_lkinfo_processlock, sleep_flag);
    if (proc->pp_lock == NULL) {
      cmn_err(CE_WARN, "CGMTR:  alloc([%d].pp_lock=NULL)\n", process);
      return ENOMEM;
    }

    proc->pp_svp = SV_ALLOC(sleep_flag);
    if (proc->pp_svp == NULL) {
      cmn_err(CE_WARN, "CGMTR:  alloc([%d].pp_svp=NULL)\n", process);
      return ENOMEM;
    }
  }

  cgmtr_cg = kmem_alloc(cgmtr_cg_bytes, sleep_flag);
  if (cgmtr_cg == NULL) {
    cmn_err(CE_WARN, "CGMTR:  alloc(cg_bytes=%u)\n", cgmtr_cg_bytes);
    return ENOMEM;
  }

  return CGMTR_NO_ERROR;
} /* cgmtr_alloc() */


/*******************************************************
* cgmtr_dealloc()
*
* deallocate dynamic memory used by the driver.
*/
static void
cgmtr_dealloc(void) {
  cgnum_t cgnum;
  cgmtr_process_id_t process;
  pl_t plx;

  if (cgmtr_cg != NULL) {
    /* Deallocate CG structures. */
    for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
      cgmtr_cg_t *cg;

      cg = &(cgmtr_cg[cgnum]);
      if (cg->pc_lock != NULL)
        plx = cgmtr_lock(cg->pc_lock, plhi);

      /* Remove setups. */
      if (cg->pc_setup != NULL) {
        kmem_free(cg->pc_setup, cg->pc_setup_bytes);
        cg->pc_setup = NULL;
      }

      /* Remove trace buffer. */
      if (cg->pc_trace != NULL) {
        kmem_free(cg->pc_trace,
          cg->pc_trace_buffer_size * sizeof(cgmtr_sample_t));
      }

      if (cg->pc_lock != NULL) {
        UNLOCK(cg->pc_lock, plx);

        /* deallocate per-CG lock */
        LOCK_DEALLOC(cg->pc_lock);
      }
    }

    kmem_free(cgmtr_cg, cgmtr_cg_bytes);
  }

  /* Deallocate processes. */
  for (process = 0; process <= CGMTR_MAX_PROCESSES; process++) {
    cgmtr_process_t *proc;

    proc = &(cgmtr_process[process]);

    /* Deallocate each per-process lock. */
    if (proc->pp_lock != NULL)
      LOCK_DEALLOC(proc->pp_lock);
    if (proc->pp_svp != NULL)
      SV_DEALLOC(proc->pp_svp);
  }

  /* Deallocate open lock. */
  if (cgmtr_open_lock != NULL)
    LOCK_DEALLOC(cgmtr_open_lock);
} /* cgmtr_dealloc() */

/*******************************************************
* cgmtr_load()
*
* dynamically loads the driver.
*/
static int
cgmtr_load(void) {
  cgnum_t cgnum;
  pl_t plx;
  int retcode;
  cgmtr_process_id_t process;
  cgmtr_process_t *proc;

  mod_drvattach(&cgmtr_attach_info);

  cgmtr_open_lock = NULL;
  cgmtr_num_processes = 0;

  /* Initialize each process structure (CGMTR_MAX_PROCESSES+1).  */
  for (process = 0; process <= CGMTR_MAX_PROCESSES; process++) {
    proc = &(cgmtr_process[process]);
    proc->pp_lock = NULL;
    proc->pp_svp = NULL;
    proc->pp_watchdog = CGMTR_TOID_NONE;
    proc->pp_count = 0;
    cgmtr_open_flag[process] = FALSE;
  }

  cgmtr_num_cgs = cg_numcg(0x0);
  cgmtr_cg_bytes = cgmtr_num_cgs*sizeof(cgmtr_cg_t);
  cgmtr_cg = NULL;

  /* Allocate the first batch of memory. */
  if (cgmtr_alloc(KM_SLEEP) == ENOMEM) {
    cgmtr_dealloc();
    return ENOMEM;
  }

  /* Initialize each CG structure. */
  for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
    cgmtr_cg_t *cg;
    uint_t imeter;

    cg = &(cgmtr_cg[cgnum]);

    cg->pc_cg_num = cgnum;

    cg->pc_lock = NULL;
    cg->pc_cgtype = CGMTR_NOT_SUPPORTED;

    for (imeter = 0; imeter < CGMTR_NUM_METERS; imeter++) {
      cgmtr_meter_t *meter;

      meter = &(cg->pc_meter[imeter]);
      meter->cg_reserving_process = CGMTR_PROCESS_NONE;
      meter->cg_setup_flags = CGMTR_SETUP_DEACTIVATE;
    }

    cg->pc_setup = NULL;
    cg->pc_setup_bytes = 0;

    cg->pc_num_samples_taken = 0;
    cg->pc_num_samples_kept = 0;
    cg->pc_trace_buffer_size = 1;
    cg->pc_trace = NULL;
    cg->pc_trace_end = NULL;
    cg->pc_sample = NULL;

    cg->pc_max_interval = 0;

    cg->pc_setup_timer = CGMTR_TOID_NONE;
    cg->pc_setup_timer = CGMTR_TOID_NONE;
    cg->pc_timeout_timer = CGMTR_TOID_NONE;
    cg->pc_update_timer = CGMTR_TOID_NONE;

    cg->pc_setup_process = CGMTR_PROCESS_NONE;

    cg->pc_trace_process = CGMTR_PROCESS_NONE;
    cg->pc_retcode = CGMTR_NO_ERROR;
  }

  /* Allocate per-CG memory. */
  for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
    cgmtr_cg_t *cg;

    cg = &(cgmtr_cg[cgnum]);

    cg->pc_lock = LOCK_ALLOC(2, plhi, &cgmtr_lkinfo_cglock, KM_SLEEP);
    if (cg->pc_lock == NULL) {
      cmn_err(CE_WARN, "CGMTR:  load([%d].pc_lock=NULL)\n", cgnum);
      cgmtr_dealloc();
      return ENOMEM;
    }

    cg->pc_trace = kmem_alloc(
      cg->pc_trace_buffer_size*sizeof(cgmtr_sample_t), KM_SLEEP);
    if (cg->pc_trace == NULL) {
      cmn_err(CE_WARN, "CGMTR:  load(trace_bytes=%u)\n",
        cg->pc_trace_buffer_size*sizeof(cgmtr_sample_t));
      cgmtr_dealloc();
      return ENOMEM;
    }
    cg->pc_trace_end = cg->pc_trace + cg->pc_trace_buffer_size;
    cg->pc_sample = cg->pc_trace;
  }

  retcode = cgmtr_mcu_init();
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  mcu_init failed\n");
    cgmtr_dealloc();
    return retcode;
  }
  retcode = cgmtr_piu_init();
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  piu_init failed\n");
    cgmtr_dealloc();
    return retcode;
  }
  retcode = cgmtr_test_init();
  if (retcode != CGMTR_NO_ERROR) {
    cmn_err(CE_WARN, "CGMTR:  test_init failed\n");
    cgmtr_dealloc();
    return retcode;
  }

  /* Spawn off a startup routine for each CG. */
  process = CGMTR_PROCESS_GLOBAL;
  proc = &(cgmtr_process[process]);

  plx = cgmtr_lock(proc->pp_lock, plhi);
  for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
    cgmtr_cg_t *cg;

    cg = &(cgmtr_cg[cgnum]);

    cg->pc_start_timer = cg_dtimeout(cgmtr_start_cg,
      (void*)cg, CGMTR_ASAP, plhi, cgnum);  
    if (cg->pc_start_timer == CGMTR_TOID_NONE) {
      cmn_err(CE_WARN, "CGMTR:  load(no timer on CG %d)\n", cgnum);
      cgmtr_num_cgs = cgnum;
      break;
    } else {
      proc->pp_count++;
    }
  }

  /* Wait for CG startups to complete, even if there were errors. */
  /* This releases the pp_lock. */
  retcode = cgmtr_join(CGMTR_PROCESS_GLOBAL, plx, CGMTR_AT_START);
  if (retcode != CGMTR_NO_ERROR)
    return retcode;

  return CGMTR_NO_ERROR;
} /* cgmtr_load() */


/*******************************************************
* cgmtr_unload()
*
* unloads the driver. All sampling and meters are stopped.
*/
static int
cgmtr_unload(void) {
  cgnum_t cgnum;
  cgmtr_process_id_t process;
  toid_t toid;

  /* Cancel any pending timeouts. */
  for (cgnum = 0; cgnum < cgmtr_num_cgs; cgnum++) {
    cgmtr_cg_t *cg;

    cg = &(cgmtr_cg[cgnum]);
    toid = cg->pc_setup_timer;
    if (toid != CGMTR_TOID_NONE)
      untimeout(toid);
    toid = cg->pc_start_timer;
    if (toid != CGMTR_TOID_NONE)
      untimeout(toid);
    toid = cg->pc_timeout_timer;
    if (toid != CGMTR_TOID_NONE)
      untimeout(toid);
    toid = cg->pc_update_timer;
    if (toid != CGMTR_TOID_NONE)
      untimeout(toid);
  }

  for (process = 0; process <= CGMTR_MAX_PROCESSES; process++) {
    toid = cgmtr_process[process].pp_watchdog;
    if (toid != CGMTR_TOID_NONE)
      untimeout(toid);
  }

  /* Deallocate kernel memory used by this module. */
  cgmtr_dealloc();

  mod_drvdetach(&cgmtr_attach_info);
  return CGMTR_NO_ERROR;
} /* cgmtr_unload() */
