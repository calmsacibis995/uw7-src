/* copyright "%c%" */
#ident	"@(#)hwmetric:cpu.c	1.3"
#ident	"$Header$"

/* Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved. */

/* cpu.c - CPU-specific functions for hwmetric */

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
#include <stdio.h>
#include <stdlib.h>

flag_type *Cpu_reachable = NULL;
cpumtr_cputype_t Cpu_type = CPUMTR_NOT_SUPPORTED;

/* get name of a meter-type */
const char *
cpu_name_meter (int meter_type) {
  switch (Cpu_type) {
    case CPUMTR_P6:
      switch (meter_type) {
        case CPUMTR_P6METERS_TSC: return "tsc";
        case CPUMTR_P6METERS_CNT: return "cnt";
      }
    break;
  }

  pfmterr ("Internal error in cpu_name_meter(meter_type=%d).\n", meter_type);
  exit (-1);
} /* cpu_name_meter () */

/* get the index of the last instance of a meter-type */
int
cpu_last_instance (int meter_type) {
  switch (Cpu_type) {
    case CPUMTR_P6:
      switch (meter_type) {
        case CPUMTR_P6METERS_TSC: return 0;
        case CPUMTR_P6METERS_CNT: return 1;
      }
    break;
  }

  pfmterr ("Internal error in cpu_last_instance(meter_type=%d).\n", meter_type);
  exit (-1);
} /* cpu_last_instance () */

/* get the index of the first meter-type */
int
cpu_first_meter_type (void) {
  switch (Cpu_type) {
    case CPUMTR_P6:
      return CPUMTR_P6METERS_TSC;
  }

  pfmterr ("Internal error in cpu_first_meter_type().\n");
  exit (-1);
} /* cpu_first_meter_type () */

/* get the index of the last meter-type */
int
cpu_last_meter_type (void) {
  switch (Cpu_type) {
    case CPUMTR_P6:
      return CPUMTR_P6METERS_CNT;
  }

  pfmterr ("Internal error in cpu_last_meter_type().\n");
  exit (-1);
} /* cpu_last_meter_type () */

/* open the cpumtr driver and determine the CPU types */
int
cpu_init (void) {
  cpumtr_rdstats_t *rdstats;
  int i;

  if (cpumtr_open () == -1)
    return -1;

  rdstats = cpumtr_rdstats ();

  Cpu_reachable = malloc (sizeof (flag_type)* Num_cpus);
  if (Cpu_reachable == NULL) {
    pfmterr ("Out of memory.\n");
    exit (-1);
  }

  for (i = 0; i < Num_cpus; i++) {
    cpumtr_cputype_t cpu_type;

    cpu_type = rdstats->cr_cpu_stats[i].ccs_cputype;
    Cpu_reachable[i] = (cpu_type != CPUMTR_NOT_SUPPORTED);

    if (Cpu_type == CPUMTR_NOT_SUPPORTED) {
      Cpu_type = cpu_type;
    } else if (cpu_type != Cpu_type && cpu_type != CPUMTR_NOT_SUPPORTED) {
      pfmterr ("Multiple CPU types in system.\n");
      exit (-1);
    }
  }
  
  free (rdstats->cr_cpu_stats);
  free (rdstats);
  return 0;
} /* cpu_init () */

/* convert a hex string (read from the rc file) to a setup value */
void
cpu_parse_setups (const char *str, int meter, cpumtr_meter_setup_t *result) {
  switch (Cpu_type) {
    case CPUMTR_P6:
      switch (meter) {
        case CPUMTR_P6METERS_TSC:
          /*  Accept only an empty string.  */
          if (strlen (str) > 0) {
            pfmterr ("Garbage in setup '%s' for cpu.tsc.\n", str);
            exit (-1);
          }
          return;

        case CPUMTR_P6METERS_CNT: {
          /*  Get 32-bit hex value for EvtSel MSR.  */
          char *cp;
          unsigned long value;
          
          value = strtoul (str, &cp, 16);
          if (cp == str || *cp != '\0') {
            pfmterr ("Garbage in setup '%s' for cpu.cnt.\n", str);
            exit (-1);
          }
          result->p6cnt.c6c_evtsel = value;
        } return;
      }
      break;
  }

  pfmterr ("Internal error in cpu_parse_setups(str=%s, meter=%d).\n",
    str, meter);
  exit (-1);
} /* cpu_parse_setups () */
