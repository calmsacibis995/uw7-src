/* copyright "%c%" */
#ident	"@(#)hwmetric:cpumtr.c	1.3"
#ident	"$Header$"

/* Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved. */

/* cpumtr.c - CPUMTR driver interface for hwmetric command */

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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static int Cpumtr_fd = -1;

/* Open the cpumtr driver. */
int
cpumtr_open (void) {
  cpumtr_rdstats_t rdstats;
  int retcode;

  Cpumtr_fd = open (CPUMTR_DEVICEFILE, O_RDWR);
  if (Cpumtr_fd == -1) {
    if (errno != ENOENT)
      perror ("open cpumtr");
    return -1;
  }

  /* Determine the number of CPUs. */
  rdstats.cr_flags = CPUMTR_RDSTATS_FLAGS;
  rdstats.cr_num_cpus = 0;
  rdstats.cr_cpu_stats = NULL;
  rdstats.cr_num_lwps = 0;
  rdstats.cr_lwp_stats = NULL;
#ifdef DEBUG
  printf ("cpumtr ioctl rdstats 0\n");
  fflush (stdout);
#endif /* DEBUG */
  retcode = ioctl (Cpumtr_fd, CPUMTR_CMD_RDSTATS, (caddr_t)&rdstats);
  if (retcode != 0) {
    perror ("ioctl(rdstats) cpumtr in cpumtr_open");
    exit (-1);
  }
  Num_cpus = rdstats.cr_num_cpus;

  return 0;
} /* cpumtr_open () */

/* Read statistics from the cpumtr driver. */
cpumtr_rdstats_t *
cpumtr_rdstats (void) {
  cpumtr_rdstats_t *rdstats;
  int retcode;

  rdstats = malloc (sizeof (cpumtr_rdstats_t));
  if (rdstats == NULL) {
    pfmterr ("Out of memory.\n");
    exit (-1);
  }
  rdstats->cr_flags = CPUMTR_RDSTATS_FLAGS;
  rdstats->cr_num_cpus = Num_cpus;
  rdstats->cr_cpu_stats = malloc (Num_cpus * sizeof (cpumtr_cpu_stats_t));
  if (rdstats->cr_cpu_stats == NULL) {
    pfmterr ("Out of memory.\n");
    exit (-1);
  }
  rdstats->cr_num_lwps = 0;
  rdstats->cr_lwp_stats = NULL;

#ifdef DEBUG
  printf ("cpumtr ioctl rdstats %d\n", Num_cpus);
  fflush (stdout);
#endif /* DEBUG */
  retcode = ioctl (Cpumtr_fd, CPUMTR_CMD_RDSTATS, (caddr_t)rdstats);
  if (retcode != 0) {
    perror ("ioctl(rdstats) cpumtr");
    exit (-1);
  }

  return rdstats;
} /* cpumtr_rdstats () */

/* Reserve a CPU meter to this user. */
int
cpumtr_reserve (int point, int meter, int instance) {
  cpumtr_reserve_t arg;
  int retcode;

  arg.cv_meter_id.cmi_cpu_id = point;
  arg.cv_meter_id.cmi_instance = instance;
  arg.cv_meter_id.cmi_type = meter;
  arg.cv_user_id = getuid ();

#ifdef DEBUG
  printf ("cpumtr ioctl reserve cpu%d.%s%d\n",
    point, name_meter (POINT_TYPE_CPU, meter), instance);
  fflush (stdout);
#endif /* DEBUG */
  retcode = ioctl (Cpumtr_fd, CPUMTR_CMD_RESERVE, (caddr_t)&arg);
  if (retcode != 0) {
    if (errno == EBUSY)
      return EBUSY;
    perror ("ioctl(reserve) cpumtr");
    exit (-1);
  }
  return 0;
} /* cpumtr_reserve () */

/* Initialize a cpumtr setup structure. */
void
cpumtr_setup_init (cpumtr_setup_t *setup) {
  setup->cs_num_cpu_setups = 0;
  setup->cs_cpu_setups = NULL;
} /* cpumtr_setup_init () */

/* Add to a cpumtr setup structure. */
void
cpumtr_setup_add (cpumtr_setup_t *setup, int point, int meter, int instance,
   int metric_index) {
  cpumtr_cpu_setup_t *cpu_setup;
  int num_cpu_setups;

  /* Grow the array of cpu_setups. */
  num_cpu_setups = setup->cs_num_cpu_setups;
  setup->cs_cpu_setups = realloc (setup->cs_cpu_setups,
    sizeof (cpumtr_cpu_setup_t) * (num_cpu_setups + 1));

  /* Initialize the new cpu_setup. */
  cpu_setup = &(setup->cs_cpu_setups[num_cpu_setups]);
  setup->cs_num_cpu_setups = num_cpu_setups + 1;

  cpu_setup->cps_meter_id.cmi_cpu_id = point;
  cpu_setup->cps_meter_id.cmi_instance = instance;
  cpu_setup->cps_meter_id.cmi_type = meter;

  cpu_setup->cps_cputype = Cpu_type;
  cpu_setup->cps_sampletable_size = 1;

  if (metric_index == -1) {
    cpu_setup->cps_setup_flags = CPUMTR_SETUP_DEACTIVATE;
    cpu_setup->cps_samplesetups_size = 0;
    cpu_setup->cps_sample_setup = NULL;

  } else {
    cpumtr_sample_setup_t *sample_setup;

    cpu_setup->cps_setup_flags = CPUMTR_SETUP_ACTIVATE;
    cpu_setup->cps_samplesetups_size = 1;
    sample_setup = malloc (sizeof (cpumtr_sample_setup_t));
    if (sample_setup == NULL) {
      pfmterr ("Out of memory.\n");
      exit (-1);
    }
    if (instance < 0 || instance >= CPUMTR_NUM_METERS) {
      pfmterr ("Internal error in cpumtr_setup_add ().\n");
      exit (-1);
    }
    sample_setup->css_meter_setup[instance] =
      (rc_metric_setup (metric_index))->cpu;
    sample_setup->css_max_interval[instance] =
      rc_metric_interval (metric_index);
    strcpy (sample_setup->css_name[instance], rc_metric_name (metric_index));
    cpu_setup->cps_sample_setup = sample_setup;
  }
} /* cpumtr_setup_add () */

/* Apply a cpumtr setup structure. */
void
cpumtr_setup (cpumtr_setup_t *setup) {
  int retcode;

  if (setup->cs_num_cpu_setups == 0)
    return;
#ifdef DEBUG
  printf ("cpumtr ioctl setup %d\n", setup->cs_num_cpu_setups);
  fflush (stdout);
#endif /* DEBUG */
  retcode = ioctl (Cpumtr_fd, CPUMTR_CMD_SETUP, (caddr_t)setup);
  if (retcode != 0) {
    perror ("ioctl(setup) cpumtr");
    exit (-1);
  }
} /* cpumtr_setup () */

/* Capture a cpumtr trace. */
void
cpumtr_trace (unsigned long bufsize, unsigned long samples,
    cpumtr_rdsamples_t *rdsamples) {
  int retcode;
  cpumtr_setup_t setup;
  int icpu;

  /* Change the size of the trace buffer. */
  cpumtr_setup_init (&setup);
  for (icpu = 0; icpu < Num_cpus; icpu++) {
    if (!Cpu_reachable[icpu])
      continue;

    /* The setup is a no-op as far as the meters are concerned. */
    cpumtr_setup_add (&setup, icpu, CPUMTR_P6METERS_TSC, 0, -1);
    setup.cs_cpu_setups[setup.cs_num_cpu_setups - 1].cps_sampletable_size = bufsize;
  }
  cpumtr_setup (&setup);

  /* Prepare arg for RDSAMPLES ioctl. */
  rdsamples->cd_num_cpus = Num_cpus;
  rdsamples->cd_flags = CPUMTR_RDSAMPLES_FLAGS;
  rdsamples->cd_max_samples = bufsize;
  rdsamples->cd_cpu_samples =
    malloc (Num_cpus * sizeof (cpumtr_proc_rdsamples_t));
  if (rdsamples->cd_cpu_samples == NULL) {
    pfmterr ("Out of memory.\n");
    exit (-1);
  }
  rdsamples->cd_block_until = samples;

  for (icpu = 0; icpu < Num_cpus; icpu++) {
    cpumtr_proc_rdsamples_t *cpu;

    cpu = &(rdsamples->cd_cpu_samples[icpu]);
    cpu->cpd_cpu_id = icpu;
    cpu->cpd_samples = malloc (bufsize * sizeof (cpumtr_sample_t));
    if (rdsamples->cd_cpu_samples == NULL) {
      pfmterr ("Out of memory.\n");
      exit (-1);
    }
  }

#ifdef DEBUG
  printf ("cpumtr ioctl rdsamples %d\n", Num_cpus);
  fflush (stdout);
#endif /* DEBUG */
  retcode = ioctl (Cpumtr_fd, CPUMTR_CMD_RDSAMPLES, (caddr_t)rdsamples);
  if (retcode != 0) {
    perror ("ioctl(rdsamples) cpumtr");
    exit (-1);
  }
} /* cpumtr_trace () */

/* Enable or disable context-switch hooks on all CPUs. */
void
swtch_hooks (flag_type enable_flag) {
  int retcode;

  if (enable_flag) {
    retcode = ioctl (Cpumtr_fd, CPUMTR_CMD_PON, (caddr_t)0);
    if (retcode != 0) {
      perror ("ioctl(pon) cpumtr");
      exit (-1);
    }
  } else {
    retcode = ioctl (Cpumtr_fd, CPUMTR_CMD_POFF, (caddr_t)0);
    if (retcode != 0) {
      perror ("ioctl(poff) cpumtr");
      exit (-1);
    }
  }
} /* swtch_hooks () */

/* Enable or disable system-call hooks on all LWPs. */
void
syscall_hooks (flag_type enable_flag) {
  int retcode;

  if (enable_flag) {
    retcode = ioctl (Cpumtr_fd, CPUMTR_CMD_KON, (caddr_t)0);
    if (retcode != 0) {
      perror ("ioctl(kon) cpumtr");
      exit (-1);
    }
  } else {
    retcode = ioctl (Cpumtr_fd, CPUMTR_CMD_KOFF, (caddr_t)0);
    if (retcode != 0) {
      perror ("ioctl(koff) cpumtr");
      exit (-1);
    }
  }
} /* syscall_hooks () */
