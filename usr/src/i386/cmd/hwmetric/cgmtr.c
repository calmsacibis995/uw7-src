/* copyright "%c%" */
#ident	"@(#)hwmetric:cgmtr.c	1.3"
#ident	"$Header$"

/* Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved. */

/* cgmtr.c - CGMTR driver interface for hwmetric command */

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

static int Cgmtr_fd = -1;

/* Open the cgmtr driver. */
int
cgmtr_open (void) {
  cgmtr_rdstats_t rdstats;
  int retcode;

  Cgmtr_fd = open (CGMTR_DEVICEFILE, O_RDWR);
  if (Cgmtr_fd == -1) {
    if (errno != ENOENT)
      perror ("open cgmtr");
    return -1;
  }

  /* Determine the number of CGs. */
  rdstats.pr_flags = CGMTR_RDSTATS_FLAGS;
  rdstats.pr_num_cgs = 0;
  rdstats.pr_cg_stats = NULL;
  retcode = ioctl (Cgmtr_fd, CGMTR_CMD_RDSTATS, (caddr_t)&rdstats);
  if (retcode != 0) {
    perror ("ioctl(rdstats) cgmtr");
    exit (-1);
  }
  Num_cgs = rdstats.pr_num_cgs;

  return 0;
} /* cgmtr_open () */

/* Read statistics from the cgmtr driver. */
cgmtr_rdstats_t *
cgmtr_rdstats (void) {
  cgmtr_rdstats_t * rdstats;
  int retcode;

  rdstats = malloc (sizeof (cgmtr_rdstats_t));
  if (rdstats == NULL) {
    pfmterr ("Out of memory.\n");
    exit (-1);
  }
  rdstats->pr_flags = CGMTR_RDSTATS_FLAGS;
  rdstats->pr_num_cgs = Num_cgs;
  rdstats->pr_cg_stats = malloc (Num_cgs * sizeof (cgmtr_cg_stats_t));
  if (rdstats->pr_cg_stats == NULL) {
    pfmterr ("Out of memory.\n");
    exit (-1);
  }

  retcode = ioctl (Cgmtr_fd, CGMTR_CMD_RDSTATS, (caddr_t)rdstats);
  if (retcode != 0) {
    perror ("ioctl(rdstats) cgmtr");
    exit (-1);
  }

  return rdstats;
} /* cgmtr_rdstats () */

/* Reserve a CG meter to this user. */
int
cgmtr_reserve (int point, int meter, int instance) {
  cgmtr_reserve_t arg;
  int retcode;

  arg.pv_meter_id.cgi_cg_num = point;
  arg.pv_meter_id.cgi_instance = instance;
  arg.pv_meter_id.cgi_type = meter;
  arg.pv_user_id = getuid ();

  retcode = ioctl (Cgmtr_fd, CGMTR_CMD_RESERVE, (caddr_t)&arg);
  if (retcode != 0) {
    if (errno == EBUSY)
      return EBUSY;
    perror ("ioctl(reserve) cgmtr");
    exit (-1);
  }
  return 0;
} /* cgmtr_reserve () */

/* Initialize a cgmtr setup structure. */
void
cgmtr_setup_init (cgmtr_setup_t *setup) {
  setup->ps_num_cg_setups = 0;
  setup->ps_cg_setups = NULL;
} /* cgmtr_setup_init () */

/* Add to cgmtr setup structure. */
void
cgmtr_setup_add (cgmtr_setup_t *setup, int point, int meter, int instance,
   int metric_index) {
  cgmtr_cg_setup_t *cg_setup;
  int num_cg_setups;

  /* Grow the array of cg_setups. */
  num_cg_setups = setup->ps_num_cg_setups;
  setup->ps_cg_setups = realloc (setup->ps_cg_setups,
    sizeof (cgmtr_cg_setup_t) * (num_cg_setups + 1));

  /* Initialize the new cg_setup. */
  cg_setup = &(setup->ps_cg_setups[num_cg_setups]);
  setup->ps_num_cg_setups = num_cg_setups + 1;

  cg_setup->pps_meter_id.cgi_cg_num = point;
  cg_setup->pps_meter_id.cgi_instance = instance;
  cg_setup->pps_meter_id.cgi_type = meter;

  cg_setup->pps_cgtype = Cg_type;
  cg_setup->pps_sampletable_size = 1;

  if (metric_index == -1) {
    cg_setup->pps_setup_flags = CGMTR_SETUP_DEACTIVATE;
    cg_setup->pps_samplesetups_size = 0;
    cg_setup->pps_sample_setup = NULL;

  } else if (metric_index == -2) {
    cg_setup->pps_setup_flags = CGMTR_SETUP_NOOP;
    cg_setup->pps_samplesetups_size = 0;
    cg_setup->pps_sample_setup = NULL;

  } else {
    cgmtr_sample_setup_t *sample_setup;
    int meter_index;

    meter_index = cg_meter_index (meter, instance);

    cg_setup->pps_setup_flags = CGMTR_SETUP_ACTIVATE;
    cg_setup->pps_samplesetups_size = 1;
    sample_setup = malloc (sizeof (cgmtr_sample_setup_t));
    if (sample_setup == NULL) {
      pfmterr ("Out of memory.\n");
      exit (-1);
    }
    sample_setup->pss_meter_setup[meter_index] =
      (rc_metric_setup (metric_index))->cg;
    sample_setup->pss_max_interval[meter_index] =
      rc_metric_interval (metric_index);
    strcpy (sample_setup->pss_name[meter_index], rc_metric_name (metric_index));
    cg_setup->pps_sample_setup = sample_setup;
  }
} /* cgmtr_setup_add () */

/* Apply a cgmtr setup structure. */
void
cgmtr_setup (cgmtr_setup_t *setup) {
  int retcode;

  if (setup->ps_num_cg_setups == 0)
    return;
  retcode = ioctl (Cgmtr_fd, CGMTR_CMD_SETUP, (caddr_t)setup);
  if (retcode != 0) {
    perror ("ioctl(setup) cgmtr");
    exit (-1);
  }
} /* cgmtr_setup () */

/* Capture a cgmtr trace. */
void
cgmtr_trace (unsigned long bufsize, unsigned long samples,
    cgmtr_rdsamples_t *rdsamples) {
  int retcode;
  int icg;
  cgmtr_setup_t setup;

  /* Change the size of the trace buffer. */
  cgmtr_setup_init (&setup);
  for (icg = 0; icg < Num_cgs; icg++) {
    /* The setup is a no-op as far as the meters are concerned. */
    cgmtr_setup_add (&setup, icg, 0, 0, -2);
    setup.ps_cg_setups[icg].pps_sampletable_size = bufsize;
  }
  cgmtr_setup (&setup);

  /* Prepare arg for RDSAMPLES ioctl. */
  rdsamples->pd_num_cgs = Num_cgs;
  rdsamples->pd_flags = CGMTR_RDSAMPLES_FLAGS;
  rdsamples->pd_max_samples = bufsize;
  rdsamples->pd_cg_samples =
    malloc (Num_cgs * sizeof (cgmtr_cg_rdsamples_t));
  if (rdsamples->pd_cg_samples == NULL) {
    pfmterr ("Out of memory.\n");
    exit (-1);
  }
  rdsamples->pd_block_until = samples;

  for (icg = 0; icg < Num_cgs; icg++) {
    cgmtr_cg_rdsamples_t *cg;

    cg = &(rdsamples->pd_cg_samples[icg]);
    cg->ppd_cg_num = icg;
    cg->ppd_samples = malloc (bufsize * sizeof (cgmtr_sample_t));
    if (rdsamples->pd_cg_samples == NULL) {
      pfmterr ("Out of memory.\n");
      exit (-1);
    }
  }

#ifdef DEBUG
  printf ("cgmtr ioctl rdsamples %d\n", Num_cgs);
  fflush (stdout);
#endif /* DEBUG */
  retcode = ioctl (Cgmtr_fd, CGMTR_CMD_RDSAMPLES, (caddr_t)rdsamples);
  if (retcode != 0) {
    perror ("ioctl(rdsamples) cgmtr");
    exit (-1);
  }
} /* cgmtr_trace () */
