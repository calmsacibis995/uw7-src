/* copyright "%c%" */
#ident	"@(#)hwmetric:do.c	1.3"
#ident	"$Header$"

/* Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved. */

/* do.c - top-level code for 'hwmetric' options */

#include <sys/types.h>
#include <sys/dl.h>
#include <sys/ksynch.h>
#include <sys/ddi.h>

#ifdef HEADERS_INSTALLED
# include <sys/cpumtr.h>
# include <sys/cgmtr.h>
#else /* !HEADERS_INSTALLED */
# include "../cpumtr/cpumtr.h"
# include "../cgmtr/cgmtr.h"
#endif /* !HEADERS_INSTALLED */


#include "hwmetric.h"

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Match a meterlist against a particular instance. */
static flag_type
match (const char *meterlist, int point_type, int point, int meter,
  int instance) {
  pml_type pml;

  pml_init (&pml, meterlist);
  while (!pml_at_end (&pml)) {
    int pt2;
    int p2;
    int m2;
    int i2;

    (void)pml_read (&pml, &pt2, &p2, &m2, &i2);
    if (pt2 == point_type && p2 == point && m2 == meter && i2 == instance) {
      return TRUE;
    }
  }
  return FALSE;
} /* match () */
    
/* Get CPU metric info. */
static void
cpu_do_info (const cpumtr_rdstats_t *cpu_rdstats, int point, int meter,
	int instance) {
  cpumtr_cpu_stats_t *cpu;

  if (!Cpu_reachable[point]) {
    printf ("is unreachable.\n");
    return;
  }

  cpu = &(cpu_rdstats->cr_cpu_stats[point]);

  if (meter == CPUMTR_P6METERS_TSC) {
    printf ("\n   user time= 0x%08lx%08lx",
      cpu->ccs_time[CPUMTR_STATE_USER].dl_hop,
      cpu->ccs_time[CPUMTR_STATE_USER].dl_lop);
    printf ("       system time= 0x%08lx%08lx",
      cpu->ccs_time[CPUMTR_STATE_SYSTEM].dl_hop,
      cpu->ccs_time[CPUMTR_STATE_SYSTEM].dl_lop);
    printf ("\n   idle time= 0x%08lx%08lx",
      cpu->ccs_time[CPUMTR_STATE_IDLE].dl_hop,
      cpu->ccs_time[CPUMTR_STATE_IDLE].dl_lop);
    printf ("  unaccounted time= 0x%08lx%08lx",
      cpu->ccs_time[CPUMTR_STATE_UNKNOWN].dl_hop,
      cpu->ccs_time[CPUMTR_STATE_UNKNOWN].dl_lop);

  } else if (meter != CPUMTR_P6METERS_CNT) {
    pfmterr ("Internal error in cpu_do_info ().\n");
    exit (-1);

  } else {
    if (cpu->ccs_setup_flags[instance] == CPUMTR_SETUP_DEACTIVATE) {
      printf ("is idle.");

    } else {
      uid_t user_id;
      struct passwd *pwe;
  
      printf ("metric \"%s\" was activated by ",
        cpu->ccs_setup.css_name[instance]);
  
      user_id = cpu->ccs_userid[instance];
      pwe = getpwuid (user_id);
      if (pwe != NULL) {
        printf ("%s.", pwe->pw_name);
      } else {
        /* invalid uid */
        printf ("user%ld.", user_id);
      }
    }
  
    printf ("\n  user count= 0x%08lx%08lx",
      cpu->ccs_count[CPUMTR_STATE_USER][instance].dl_hop,
      cpu->ccs_count[CPUMTR_STATE_USER][instance].dl_lop);
    printf ("      system count= 0x%08lx%08lx",
      cpu->ccs_count[CPUMTR_STATE_SYSTEM][instance].dl_hop,
      cpu->ccs_count[CPUMTR_STATE_SYSTEM][instance].dl_lop);
    printf ("\n  idle count= 0x%08lx%08lx",
      cpu->ccs_count[CPUMTR_STATE_IDLE][instance].dl_hop,
      cpu->ccs_count[CPUMTR_STATE_IDLE][instance].dl_lop);
    printf (" unaccounted count= 0x%08lx%08lx",
      cpu->ccs_count[CPUMTR_STATE_UNKNOWN][instance].dl_hop,
      cpu->ccs_count[CPUMTR_STATE_UNKNOWN][instance].dl_lop);
  }

  printf ("\n");
} /* cpu_do_info () */

/* Get CG metric info. */
static void
cg_do_info (const cgmtr_rdstats_t *cg_rdstats, int point, int meter,
	int instance) {
  cgmtr_cg_stats_t *cg;
  int meter_index;

  cg = &(cg_rdstats->pr_cg_stats[point]);

  meter_index = cg_meter_index (meter, instance);
  if (cg->pcs_setup_flags[meter_index] == CGMTR_SETUP_DEACTIVATE) {
    printf ("is idle.");

  } else {
    uid_t user_id;
    struct passwd *pwe;
    int meter_index;
    
    meter_index = cg_meter_index (meter, instance);
    printf ("metric \"%s\" was activated by ",
      cg->pcs_setup.pss_name[meter_index]);

    user_id = cg->pcs_userid[meter_index];
    pwe = getpwuid (user_id);
    if (pwe != NULL) {
      printf ("%s.", pwe->pw_name);
    } else {
      /* invalid uid */
      printf ("user%ld.", user_id);
    }
  }

  printf ("\n  count=0x%08lx%08lx",
    cg->pcs_count[instance].dl_hop,
    cg->pcs_count[instance].dl_lop);
  printf ("\n");
} /* cg_do_info () */

/* Get hardware metric info. */
void
do_info (const char *meterlist) {
  pml_type pml;
  flag_type cpu_stats_flag;
  flag_type cg_stats_flag;
  cpumtr_rdstats_t * cpu_rdstats;
  cgmtr_rdstats_t * cg_rdstats;

  rc_read ();

  pml_point_types (meterlist, &cpu_stats_flag, &cg_stats_flag);

  /* Interrogate the drivers as needed. */
  if (cpu_stats_flag) {
    cpu_rdstats = cpumtr_rdstats ();
  } else {
    cpu_rdstats = NULL;
  }

  if (cg_stats_flag) {
    cg_rdstats = cgmtr_rdstats ();
  } else {
    cg_rdstats = NULL;
  }

  pml_init (&pml, meterlist);
  while (!pml_at_end (&pml)) {
    int point_type;
    int point;
    int meter;
    int instance;

    (void)pml_read (&pml, &point_type, &point, &meter, &instance);
    printf ("%s%d.%s%d ", name_point (point_type), point,
      name_meter (point_type, meter), instance);

    switch (point_type) {
      case POINT_TYPE_CPU:
        cpu_do_info (cpu_rdstats, point, meter, instance);
        break;

      case POINT_TYPE_CG:
        cg_do_info (cg_rdstats, point, meter, instance);
        break;

      default:
        pfmterr ("Internal error in do_info ().\n");
        exit (-1);
    }
  }
} /* do_info () */

/* Activate CPU metrics. */
static void
cpu_do_activate (flag_type override_flag, int metric_index,
    const char *meterlist) {
  pml_type pml;
  cpumtr_rdstats_t *cpu_rdstats;
  cpumtr_setup_t setup;
  int last_point;
  flag_type covered_flag;
  int point = -1;

  /* Interrogate the driver. */
  cpu_rdstats = cpumtr_rdstats ();

  cpumtr_setup_init (&setup);

  last_point = -1;
  covered_flag = TRUE;
  pml_init (&pml, meterlist);
  while (!pml_at_end (&pml)) {
    int point_type;
    int meter;
    int instance;

    (void)pml_read (&pml, &point_type, &point, &meter, &instance);
    if (point_type == POINT_TYPE_CPU) {
      if (point != last_point) {
        if (covered_flag) {
          last_point = point;
          covered_flag = FALSE;
        } else if (last_point >= 0 && !Cpu_reachable[last_point]) {
          pfmtinf ("Skipping unreachable cpu%d.\n", last_point);
          last_point = point;
          covered_flag = FALSE;
        } else {
          pfmterr (
  "The metric '%s' cannot be activated at cpu%d using the available meters.\n",
            rc_metric_name (metric_index), last_point);
          exit (-1);
        }
      }
  
      if (!covered_flag && Cpu_reachable[point] && match (
          rc_meterlist (metric_index), point_type, point, meter, instance)) {
        int setup_flags;
        cpumtr_cpu_stats_t *cpu;
        
        cpu = &(cpu_rdstats->cr_cpu_stats[point]);
        setup_flags = cpu->ccs_setup_flags[instance];
        if (setup_flags == CPUMTR_SETUP_DEACTIVATE
         || override_flag && cpu->ccs_userid[instance] == getuid ()) {
          if (cpumtr_reserve (point, meter, instance) == 0) {
            covered_flag = TRUE;
            cpumtr_setup_add (&setup, point, meter, instance, metric_index);
          }
        }
      }
    }
  } /* while !at_end */
  
  if (!covered_flag) {
    if (last_point >= 0 && !Cpu_reachable[last_point]) {
      pfmtinf ("Skipping unreachable cpu%d.\n", last_point);
    } else {
      pfmterr (
"The metric '%s' cannot be activated at cpu%d using the available meters.\n",
      rc_metric_name (metric_index), last_point);
      exit (-1);
    }
  }
  cpumtr_setup (&setup);
} /* cpu_do_activate () */

/* Activate CG metrics. */
static void
cg_do_activate (flag_type override_flag, int metric_index,
    const char *meterlist) {
  pml_type pml;
  cgmtr_rdstats_t *cg_rdstats;
  cgmtr_setup_t setup;
  int last_point;
  flag_type covered_flag;
  int point = -1;

  /* Interrogate the driver. */
  cg_rdstats = cgmtr_rdstats ();

  cgmtr_setup_init (&setup);

  last_point = -1;
  covered_flag = TRUE;
  pml_init (&pml, meterlist);
  while (!pml_at_end (&pml)) {
    int point_type;
    int meter;
    int instance;

    (void)pml_read (&pml, &point_type, &point, &meter, &instance);
    if (point_type == POINT_TYPE_CG) {
      if (point != last_point) {
        if (covered_flag) {
          last_point = point;
          covered_flag = FALSE;
        } else {
          pfmterr (
  "The metric '%s' cannot be activated at cg%d using the available meters.\n",
            rc_metric_name (metric_index), last_point);
          exit (-1);
        }
      }
  
      if (!covered_flag && match (
          rc_meterlist (metric_index), point_type, point, meter, instance)) {
        int setup_flags;
        cgmtr_cg_stats_t *cg;
        int meter_index;
  
        meter_index = cg_meter_index (meter, instance);
        cg = &(cg_rdstats->pr_cg_stats[point]);
        setup_flags = cg->pcs_setup_flags[meter_index];
        if (setup_flags == CGMTR_SETUP_DEACTIVATE
         || override_flag && cg->pcs_userid[meter_index] == getuid ()) {
          if (cgmtr_reserve (point, meter, instance) == 0) {
            covered_flag = TRUE;
            cgmtr_setup_add (&setup, point, meter, instance, metric_index);
          }
        }
      }
    }
  } /* while !at_end */

  if (!covered_flag) {
    pfmterr (
"The metric '%s' cannot be activated at cg%d using the available meters.\n",
      rc_metric_name (metric_index), last_point);
    exit (-1);
  }
  cgmtr_setup (&setup);
} /* cg_do_activate () */

/* Activate hardware metrics. */
void
do_activate (flag_type override_flag, const char *metric_name,
    const char *meterlist) {
  flag_type cpu_meter_flag;
  flag_type cg_meter_flag;
  int metric_index;

  rc_read ();

  pml_point_types (meterlist, &cpu_meter_flag, &cg_meter_flag);

  metric_index = rc_find_metric (metric_name);
  if (metric_index == -1) {
    pfmterr ("Unknown metric '%s'.\n", metric_name);
    exit (-1);
  }

  switch (rc_metric_point_type (metric_index)) {
    case POINT_TYPE_CPU:
      if (!cpu_meter_flag) {
        pfmterr ("Metric '%s' is incompatible with meterlist '%s'.\n",
          metric_name, meterlist);
        exit (-1);
      }
      cpu_do_activate (override_flag, metric_index, meterlist);
      break;

    case POINT_TYPE_CG:
      if (!cg_meter_flag) {
        pfmterr ("Metric '%s' is incompatible with meterlist '%s'.\n",
          metric_name, meterlist);
        exit (-1);
      }
      cg_do_activate (override_flag, metric_index, meterlist);
      break;

    default:
      pfmterr ("Internal error in do_activate().\n");
      exit (-1);
  }
} /* do_activate () */

/* Deactivate hardware meters. */
void
do_deactivate (const char *meterlist) {
  pml_type pml;
  flag_type cpu_meter_flag;
  flag_type cg_meter_flag;
  cpumtr_rdstats_t *cpu_rdstats;
  cgmtr_rdstats_t *cg_rdstats;
  cpumtr_setup_t cpu_setup;
  cgmtr_setup_t cg_setup;

  rc_read ();

  pml_point_types (meterlist, &cpu_meter_flag, &cg_meter_flag);

  /* Interrogate the drivers as needed. */
  if (cpu_meter_flag) {
    cpu_rdstats = cpumtr_rdstats ();
    cpumtr_setup_init (&cpu_setup);
  } else {
    cpu_rdstats = NULL;
  }

  if (cg_meter_flag) {
    cg_rdstats = cgmtr_rdstats ();
    cgmtr_setup_init (&cg_setup);
  } else {
    cg_rdstats = NULL;
  }

  pml_init (&pml, meterlist);
  while (!pml_at_end (&pml)) {
    int point_type;
    int point;
    int meter;
    int instance;

    (void)pml_read (&pml, &point_type, &point, &meter, &instance);
    switch (point_type) {
      case POINT_TYPE_CPU:
        if (meter == CPUMTR_P6METERS_CNT) {
          cpumtr_cpu_stats_t *cpu;
      
          cpu = &(cpu_rdstats->cr_cpu_stats[point]);
          if (cpu->ccs_setup_flags[instance] == CPUMTR_SETUP_ACTIVATE
           && cpu->ccs_userid[instance] == getuid ()) {
            if (cpumtr_reserve (point, meter, instance) == 0) {
              cpumtr_setup_add (&cpu_setup, point, meter, instance, -1);
            } else {
              pfmterr ("cpu%d.%s%u is busy - not deactivated\n",
                point, name_meter(point_type, meter), instance);
            }
          }
        }
        break;

      case POINT_TYPE_CG: {
        cgmtr_cg_stats_t *cg;
        int meter_index;

        meter_index = cg_meter_index (meter, instance);
        cg = &(cg_rdstats->pr_cg_stats[point]);
        if (cg->pcs_setup_flags[meter_index] == CGMTR_SETUP_ACTIVATE
         && cg->pcs_userid[meter_index] == getuid ()) {
          if (cgmtr_reserve (point, meter, instance) == 0) {
            cgmtr_setup_add (&cg_setup, point, meter, instance, -1);
          } else {
            pfmterr ("cg%d.%s%u is busy - not deactivated\n",
              point, name_meter(point_type, meter), instance);
          }
        }
      } break;

      default:
        pfmterr ("Internal error in do_deactivate().\n");
        exit (-1);
    }
  }
  if (cpu_meter_flag)
    cpumtr_setup (&cpu_setup);
  if (cg_meter_flag)
    cgmtr_setup (&cg_setup);
} /* do_deactivate () */

/* Deactivate a hardware metric. */
void
do_deactivate_metric (const char *metric_name, const char *meterlist) {
  pml_type pml;
  flag_type cpu_meter_flag;
  flag_type cg_meter_flag;
  cpumtr_rdstats_t *cpu_rdstats;
  cgmtr_rdstats_t *cg_rdstats;
  cpumtr_setup_t cpu_setup;
  cgmtr_setup_t cg_setup;

  rc_read ();

  pml_point_types (meterlist, &cpu_meter_flag, &cg_meter_flag);

  /* Interrogate the drivers as needed. */
  if (cpu_meter_flag) {
    cpu_rdstats = cpumtr_rdstats ();
    cpumtr_setup_init (&cpu_setup);
  } else {
    cpu_rdstats = NULL;
  }

  if (cg_meter_flag) {
    cg_rdstats = cgmtr_rdstats ();
    cgmtr_setup_init (&cg_setup);
  } else {
    cg_rdstats = NULL;
  }

  pml_init (&pml, meterlist);
  while (!pml_at_end (&pml)) {
    int point_type;
    int point;
    int meter;
    int instance;

    (void)pml_read (&pml, &point_type, &point, &meter, &instance);
    switch (point_type) {
      case POINT_TYPE_CPU:
        if (meter == CPUMTR_P6METERS_CNT) {
          cpumtr_cpu_stats_t *cpu;
      
          cpu = &(cpu_rdstats->cr_cpu_stats[point]);
          if (cpu->ccs_setup_flags[instance] == CPUMTR_SETUP_ACTIVATE
           && streq (cpu->ccs_setup.css_name[instance], metric_name)
           && cpu->ccs_userid[instance] == getuid ()) {
            if (cpumtr_reserve (point, meter, instance) == 0) {
              cpumtr_setup_add (&cpu_setup, point, meter, instance, -1);
            } else {
              pfmterr ("cpu%d.%s%u is busy - not deactivated\n",
                point, name_meter(point_type, meter), instance);
            }
          }
        }
        break;

      case POINT_TYPE_CG: {
        cgmtr_cg_stats_t *cg;
        int meter_index;
      
        meter_index = cg_meter_index (meter, instance);
        cg = &(cg_rdstats->pr_cg_stats[point]);
        if (cg->pcs_setup_flags[meter_index] == CGMTR_SETUP_ACTIVATE
         && streq (cg->pcs_setup.pss_name[meter_index], metric_name)
         && cg->pcs_userid[meter_index] == getuid ()) {
          if (cgmtr_reserve (point, meter, instance) == 0) {
            cgmtr_setup_add (&cg_setup, point, meter, instance, -1);
          } else {
            pfmterr ("cg%d.%s%u is busy - not deactivated\n",
              point, name_meter(point_type, meter), instance);
          }
        }
      } break;

      default:
        pfmterr ("Internal error in do_deactivate_metric ().\n");
        exit (-1);
    }
  }
  if (cpu_meter_flag)
    cpumtr_setup (&cpu_setup);
  if (cg_meter_flag)
    cgmtr_setup (&cg_setup);
} /* do_deactivate_metric () */

/* Generate a trace of CPU meter samples. */
void
do_cpu_trace (const char *bufsize_str, const char *samples_str,
  const char *meterlist) {
  flag_type cpu_meter_flag;
  flag_type cg_meter_flag;
  char *ptr;
  unsigned long bufsize;
  unsigned long samples;
  cpumtr_rdsamples_t rdsamples;
  int icpu;

  pml_point_types (meterlist, &cpu_meter_flag, &cg_meter_flag);

  if (!cpu_meter_flag) {
    pfmterr ("No CPU meters specified.\n");
    exit (-1); 
  }

  bufsize = strtoul (bufsize_str, &ptr, 10);
  if (ptr == bufsize_str || *ptr != '\0') {
    pfmterr ("Malformed argument for bufsize.\n");
    exit (-1);
  }

  samples = strtoul (samples_str, &ptr, 10);
  if (ptr == samples_str || *ptr != '\0') {
    pfmterr ("Malformed argument for samples.\n");
    exit (-1);
  }

  cpumtr_trace (bufsize, samples, &rdsamples);
  
  /* Write data to stdout in CSV format. */
  for (icpu = 0; icpu < Num_cpus; icpu++) {
    cpumtr_proc_rdsamples_t *cpu;
    int isample;

    cpu = &(rdsamples.cd_cpu_samples[icpu]);
    for (isample = 0; isample < cpu->cpd_num_samples; isample++) {
      cpumtr_sample_t *sample;
      pml_type pml;

      sample = &(cpu->cpd_samples[isample]);
      printf ("%d,%d,", icpu, isample);
      printf ("%08lx%08lx,",
        sample->ca_start_time.dl_hop, sample->ca_start_time.dl_lop);
      printf ("%08lx%08lx,",
        sample->ca_end_time.dl_hop, sample->ca_end_time.dl_lop);
      printf ("%ld,%ld,%u,%u,%u", 
        sample->ca_pid, sample->ca_lwpid, sample->ca_state,
        sample->ca_tracepoint, sample->ca_qual);

      if (sample->ca_setup_valid_flag) {
        pml_init (&pml, meterlist);
        while (!pml_at_end (&pml)) {
          int point_type;
          int point;
          int meter;
          int instance;
  
          (void)pml_read (&pml, &point_type, &point, &meter, &instance);
          if (point_type == POINT_TYPE_CPU && point == icpu) {
            if (meter == CPUMTR_P6METERS_CNT) {
              printf (",%s%d", name_meter (point_type, meter), instance);
              printf (",\"%s\"", sample->ca_setup.css_name[instance]);
              printf (",%08lx%08lx",
                sample->ca_count[instance].dl_hop,
                sample->ca_count[instance].dl_lop);
            }
          }
        }
      }
      printf ("\n");
    }
  }
} /* do_cpu_trace () */

/* Generate a trace of CG meter samples. */
void
do_cg_trace (const char *bufsize_str, const char *samples_str,
  const char *meterlist) {
  flag_type cpu_meter_flag;
  flag_type cg_meter_flag;
  char *ptr;
  unsigned long bufsize;
  unsigned long samples;
  cgmtr_rdsamples_t rdsamples;
  int icg;

  pml_point_types (meterlist, &cpu_meter_flag, &cg_meter_flag);

  if (!cg_meter_flag) {
    pfmterr ("No CG meters specified.\n");
    exit (-1); 
  }

  bufsize = strtoul (bufsize_str, &ptr, 10);
  if (ptr == bufsize_str || *ptr != '\0') {
    pfmterr ("Malformed argument for bufsize.\n");
    exit (-1);
  }

  samples = strtoul (samples_str, &ptr, 10);
  if (ptr == samples_str || *ptr != '\0') {
    pfmterr ("Malformed argument for samples.\n");
    exit (-1);
  }

  cgmtr_trace (bufsize, samples, &rdsamples);
  
  /* Write data to stdout in CSV format. */
  for (icg = 0; icg < Num_cgs; icg++) {
    cgmtr_cg_rdsamples_t *cg;
    int isample;

    cg = &(rdsamples.pd_cg_samples[icg]);
    for (isample = 0; isample < cg->ppd_num_samples; isample++) {
      cgmtr_sample_t *sample;
      pml_type pml;

      sample = &(cg->ppd_samples[isample]);
      printf ("%d,%d,%u", icg, isample, sample->pa_tracepoint);

      if (sample->pa_setup_valid_flag) {
        pml_init (&pml, meterlist);
        while (!pml_at_end (&pml)) {
          int point_type;
          int point;
          int meter;
          int instance;
  
          (void)pml_read (&pml, &point_type, &point, &meter, &instance);
          if (point_type == POINT_TYPE_CG && point == icg) {
            int meter_index;

            meter_index = cg_meter_index (meter, instance);
            printf (",%s%d", name_meter (point_type, meter), instance);
            printf (",\"%s\"", sample->pa_setup.pss_name[meter_index]);
            printf (",%08lx%08lx",
                  sample->pa_count[meter_index].dl_hop,
                  sample->pa_count[meter_index].dl_lop);
          }
        }
      }
      printf ("\n");
    }
  }
} /* do_cg_trace () */
