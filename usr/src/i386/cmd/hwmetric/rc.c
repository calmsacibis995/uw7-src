/* copyright "%c%" */
#ident	"@(#)hwmetric:rc.c	1.3"
#ident	"$Header$"


/* Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved. */

/* rc.c - info from initialization file of 'hwmetric' command */

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

#include <string.h>

#include "hwmetric.h"

#include <stdlib.h>
#include <stdio.h>

#define RC_FILE "/etc/hwmetricrc"
#define FIELD_LENGTH 500
#define MAX_NUM_METRICS 500
#define MIN_INTERVAL 10000 /* usec => 0.01 sec */

typedef struct {
  const char *name;
  int point_type;
  char *meterlist;
  meter_setup_type setup;
  unsigned long interval;
} metric_type;

static int Last_char;
static int Line_number;

static int Num_metrics = 0;
static metric_type Metric[MAX_NUM_METRICS];

/* Retrieve the point-type for a metric in the table. */
int
rc_metric_point_type (int i) {
  if (i < 0 || i >= Num_metrics) {
    pfmterr ("Internal error in rc_metric_point_type().\n");
    exit (-1);
  }

  return Metric[i].point_type;
} /* rc_metric_point_type () */

/* Retrieve the setup for a metric in the table. */
const meter_setup_type *
rc_metric_setup (int i) {
  if (i < 0 || i >= Num_metrics) {
    pfmterr ("Internal error in rc_metric_setup().\n");
    exit (-1);
  }

  return &(Metric[i].setup);
} /* rc_metric_setup () */

/* Retrieve the interval for a metric in the table. */
unsigned long
rc_metric_interval (int i) {
  if (i < 0 || i >= Num_metrics) {
    pfmterr ("Internal error in rc_metric_interval().\n");
    exit (-1);
  }

  return Metric[i].interval;
} /* rc_metric_interval () */

/* Retrieve the meterlist for a metric in the table. */
const char *
rc_meterlist (int i) {
  if (i < 0 || i >= Num_metrics) {
    pfmterr ("Internal error in rc_meterlist().\n");
    exit (-1);
  }

  return Metric[i].meterlist;
} /* rc_meterlist () */

/* Retrieve the name for a metric in the table. */
const char *
rc_metric_name (int i) {
  if (i < 0 || i >= Num_metrics) {
    pfmterr ("Internal error in rc_metric_name().\n");
    exit (-1);
  }

  return Metric[i].name;
} /* rc_metric_name () */

/* Find a metric in the table, by name. */
int
rc_find_metric (const char *metric_name) {
  int i;

  for (i = 0; i < Num_metrics; i++) {
    if (streq (metric_name, Metric[i].name))
      return i;
  }
  return -1;
} /* rc_find_metric () */

/* Read a field from the initialization file. */
static char *
rc_read_field (FILE *fp, int term_char, const char *field_name) {
  char buf[FIELD_LENGTH];
  char *cp;

  cp = &(buf[0]);
  for (;;) {
    if (Last_char == '\n')
      Line_number++;

    Last_char = getc (fp);

    if (Last_char == EOF && cp == &(buf[0]))
      return NULL;

    if (Last_char == term_char) {
      char *dup;

      *cp = '\0';
      dup = malloc (1 + strlen (buf));
      if (dup == NULL) {
        pfmterr ("Out of memory.\n");
        exit (-1);
      }
      return strcpy (dup, buf);
    }
    if (Last_char == EOF || Last_char == '\n' || Last_char == 0
     || cp + 1 == buf + FIELD_LENGTH) {
      pfmterr ("Unterminated %s field in line %d of %s.\n",
        field_name, Line_number, RC_FILE);
      exit (-1);
    }
    *cp = Last_char;
    cp++;
  }
} /* rc_read_field () */

/* Read a 'meterlist' field from the initialization file. */
static void
rc_read_meterlist (FILE * fp) {
  char *meterlist;

  /* read the meterlist */
  meterlist = rc_read_field (fp, ':', "meter list");
  if (meterlist == NULL) {
    pfmterr ("EOF in meterlist field of %s.\n", RC_FILE);
    exit (-1);

  } else {
    flag_type cpu_flag;
    flag_type cg_flag;

    pml_point_types (meterlist, &cpu_flag, &cg_flag);

    if (cpu_flag ^ cg_flag) {
      if (cpu_flag) {
        if (strlen (Metric[Num_metrics].name) > CPUMTR_MAX_NAME_LENGTH) {
          pfmterr ("Metric name too long in line %d of %s.\n",
            Line_number, RC_FILE);
          exit (-1);
        }
        Metric[Num_metrics].point_type = POINT_TYPE_CPU;

      } else {
        if (strlen (Metric[Num_metrics].name) > CGMTR_MAX_NAME_LENGTH) {
          pfmterr ("Metric name too long in line %d of %s.\n", Line_number,
            RC_FILE);
          exit (-1);
        }
        Metric[Num_metrics].point_type = POINT_TYPE_CG;
      }
  
    } else {
      pfmterr ("Invalid metric type in line %d of %s.\n", Line_number, RC_FILE);
      pfmtact ("Each metric must be either a CPU metric or a CG metric.\n");
      exit (-1);
    }

    {
      pml_type pml;
      int last_meter;
      flag_type first_meter;

      pml_init (&pml, meterlist);
  
      first_meter = TRUE;
      while (!pml_at_end (&pml)) {
        int point_type;
        int point;
        int meter;
        int instance;
  
        (void)pml_read (&pml, &point_type, &point, &meter, &instance);

        if (!first_meter && meter != last_meter) {
          pfmterr ("Invalid metric in line %d of %s.\n", Line_number, RC_FILE);
          pfmtact ("A metric cannot use more than one type of meter.\n");
          exit (-1);
        }
        first_meter = FALSE;
        last_meter = meter;
      }
    }

    Metric[Num_metrics].meterlist = meterlist;
  }
} /* rc_read_meterlist () */

/* Read an 'setups' field from the initialization file. */
static void
rc_read_setups (FILE * fp) {
  char *setups;

  /* read setup information */
  setups = rc_read_field (fp, ':', "setups");
  if (setups == NULL) {
    pfmterr ("EOF in setups field in line %d of %s.\n", Line_number, RC_FILE);
    exit (-1);

  } else {
    pml_type pml;
    int point_type;
    int point;
    int meter;
    int instance;

    pml_init (&pml, Metric[Num_metrics].meterlist);
    (void)pml_read (&pml, &point_type, &point, &meter, &instance);
    switch (point_type) {  
      case POINT_TYPE_CPU:
        cpu_parse_setups (setups, meter, &(Metric[Num_metrics].setup.cpu));
        break;
      case POINT_TYPE_CG:
        cg_parse_setups (setups, meter, &(Metric[Num_metrics].setup.cg));
        break;
      default:
        pfmterr ("Internal error in rc_read_setups().\n");
        exit (-1);
    }

    free (setups);
  }
} /* rc_read_setups () */

/* Read an 'interval' field from the initialization file. */
static void
rc_read_interval (FILE *fp) {
  char * interval;

  /* read minimum update interval, in usec */
  interval = rc_read_field (fp, '\n', "interval");
  if (interval == NULL) {
    pfmterr ("EOF in interval field of %s.\n", RC_FILE);
    exit (-1);

  } else {
    char *cp;
    unsigned long interval_long;

    interval_long = strtoul (interval, &cp, 10);
    if (cp == interval || *cp != '\0') {
      pfmterr ("Garbage in interval field, line %d of %s.\n",
        Line_number, RC_FILE);
      exit (-1);
    }

    if (interval_long < MIN_INTERVAL) {
      pfmterr ("Interval too short in line %d of %s.\n",
        Line_number, RC_FILE);
      exit (-1);
    }
    Metric[Num_metrics].interval = interval_long;
    free (interval);
  }
} /* rc_read_interval () */

/* Read one line from the initialization file. */
static void
rc_read_line (FILE *fp) {
  char *metric_name;

  /* read the metric_name */
  metric_name = rc_read_field (fp, ':', "metric-name");
  if (metric_name == NULL) {
    return;

  } else if (strlen (metric_name) == 0) {
    char *comment;

    free (metric_name);
    comment = rc_read_field (fp, '\n', "comment");
    if (comment != NULL)
      free (comment);
    return;

  } else if (rc_find_metric (metric_name) != -1) {
    pfmterr ("Duplicated metric name '%s' in line %d of %s.\n",
      metric_name, Line_number, RC_FILE);
    exit (-1);
  }
  Metric[Num_metrics].name = metric_name;

  rc_read_meterlist (fp);
  rc_read_setups (fp);
  rc_read_interval (fp);

  Num_metrics++;
} /* rc_read_line () */

/* Read the initialization file into memory. */
void
rc_read (void) {
  FILE *fp;

  fp = fopen (RC_FILE, "r");
  if (fp == NULL) {
    perror (RC_FILE);
    exit (-1);
  }

  Last_char = '\n';
  Line_number = 0;
  while (!feof (fp))
    rc_read_line (fp);

  if (fclose (fp) != 0) {
    perror (RC_FILE);
  }
} /* rc_read () */
