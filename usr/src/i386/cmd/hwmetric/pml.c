/* copyright "%c%" */
#ident	"@(#)hwmetric:pml.c	1.2"
#ident	"$Header$"


/* Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved. */

/* pml.c - parse meterlists for the 'hwmetric' command */

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

#include <stdlib.h>

#define FIRST_INSTANCE 0
#define FIRST_POINT 0
#define POINT_TYPE_CPU 0
#define POINT_TYPE_CG 1

static flag_type Initialized = FALSE;
static int First_point_type;
static int Last_point_type;

#define POINT_TYPE_NAME_LENGTH 3

const char *
name_point (int point_type) {
  switch (point_type) {
    case POINT_TYPE_CPU:  return "cpu";
    case POINT_TYPE_CG:  return "cg";
  }

  pfmterr ("Internal error in name_point(%d).\n", point_type);
  exit (-1);
} /* name_point () */

const char *
name_meter (int point_type, int meter_type) {
  switch (point_type) {
    case POINT_TYPE_CPU:
      return cpu_name_meter (meter_type);
    case POINT_TYPE_CG:
      return cg_name_meter (meter_type);
  }

  pfmterr ("Internal error in name_meter(point_type=%d).\n",
    point_type, meter_type);
  exit (-1);
} /* name_meter () */

static int
last_instance (int point_type, int meter_type) {
  switch (point_type) {
    case POINT_TYPE_CPU:
      return cpu_last_instance (meter_type);

    case POINT_TYPE_CG:
      return cg_last_instance (meter_type);
  }

  pfmterr ("Internal error in last_instance(point_type=%d, meter_type=%d).\n",
    point_type, meter_type);
  exit (-1);
} /* last_instance () */

static int
first_meter_type (int point_type) {
  switch (point_type) {
    case POINT_TYPE_CPU:  return cpu_first_meter_type ();
    case POINT_TYPE_CG:  return cg_first_meter_type ();
  }
  pfmterr ("Internal error in first_meter_type.\n");
  exit (-1);
} /* first_meter_type () */

static int
last_meter_type (int point_type) {
  switch (point_type) {
    case POINT_TYPE_CPU:  return cpu_last_meter_type ();
    case POINT_TYPE_CG:  return cg_last_meter_type ();
  }
  pfmterr ("Internal error in last_meter_type.\n");
  exit (-1);
} /* last_meter_type () */

static int
last_point (int point_type) {
  switch (point_type) {
    case POINT_TYPE_CPU:  return Num_cpus - 1;
    case POINT_TYPE_CG:  return Num_cgs - 1;
  }
  pfmterr ("Internal error in last_point.\n");
  exit (-1);
} /* last_point () */

static int
get_point_type (const char *name) {
  int pt;

  for (pt = First_point_type; pt <= Last_point_type; pt++) {
    if (streq (name, name_point (pt)))
      return pt;
  }
  pfmterr ("Malformed metername:  invalid point-type \"%s\".\n", name);
  exit (-1);
} /* get_point_type () */

static void
pml_set_point_type (pml_type *pml, const char *start, int leng) {
  if (leng == 0) {
    /* no point_type specified */
    pml->all_point_type = TRUE;
    pml->point_type = First_point_type;

  } else if (leng < 0 || leng > POINT_TYPE_NAME_LENGTH) {
    pfmterr ("Malformed metername:  invalid point-type.\n");
    exit (-1);

  } else {
    char tmp[POINT_TYPE_NAME_LENGTH + 1];

    pml->all_point_type = FALSE;
    strncpy (tmp, start, leng);
    tmp[leng] = '\0';
    pml->point_type = get_point_type (tmp);
  }
} /* pml_set_point_type () */

static void
pml_set_point (pml_type *pml, const char *start, int leng) {
  if (leng == 0) {
    /* no point specified */
    pml->all_point = TRUE;
    pml->point = FIRST_POINT;

  } else {
    char *ptr;

    pml->all_point = FALSE;
    pml->point = (int)strtol (start, &ptr, 10);
    if (ptr < start + leng) {
      pfmterr ("Malformed metername:  garbage after point.\n");
      exit (-1);
    }
    if (pml->point < FIRST_POINT
     || pml->point > last_point (pml->point_type)) {
      pfmterr ("Point index (%d) out of range.\n", pml->point);
      exit (-1);
    }
  }
} /* pml_set_point () */

static void
pml_set_meter_type (pml_type *pml, const char *start, int leng) {
  if (leng == 0) {
    /* no meter_type specified */
    pml->all_meter_type = TRUE;
    pml->meter_type_name[leng] = '\0';
    pml->meter_type = first_meter_type (pml->point_type);

  } else if (leng < 1 || leng > METER_TYPE_NAME_LENGTH) {
    pfmterr ("Malformed metername:  invalid meter-type.\n");
    exit (-1);
    
  } else {
    pml->all_meter_type = FALSE;
    strncpy (pml->meter_type_name, start, leng);  
    pml->meter_type_name[leng] = '\0';

    while (pml->point_type <= Last_point_type) {
      int mt;

      for (mt = first_meter_type (pml->point_type);
           mt <= last_meter_type (pml->point_type); mt++) {
        if (streq (name_meter (pml->point_type, mt),
                   pml->meter_type_name)) {
          pml->meter_type = mt; 
          return;
        }
      }
      pml->point_type++;
    }

    if (pml->all_point_type) {
      pfmterr ("Malformed metername:  invalid meter-type \"%s\".\n",
        pml->meter_type_name);
    } else {
      pfmterr (
      "Malformed metername:  invalid meter-type \"%s\" for point-type %s.\n",
        pml->meter_type_name, name_point (pml->point_type));
    }
    exit (-1);
  }
} /* pml_set_meter_type () */

static void
pml_set_instance (pml_type *pml, const char *start, int leng) {
  if (leng == 0) {
    /* no instance specified */
    pml->all_instance = TRUE;
    pml->instance = FIRST_INSTANCE;

  } else {
    char *ptr;

    pml->all_instance = FALSE;
    pml->instance = (int)strtol (start, &ptr, 10);
    if (ptr < start + leng) {
      pfmterr ("Malformed metername:  garbage after instance.\n");
      exit (-1);
    }
    if (pml->instance < FIRST_INSTANCE
     || pml->instance > last_instance (pml->point_type, pml->meter_type)) {
      pfmterr ("Instance index (%d) out of range.\n", pml->instance);
      exit (-1);
    }
  }
} /* pml_set_instance () */

static void
pml_all (pml_type *pml) {
  pml_set_point_type (pml, "", 0);
  pml_set_point (pml, "", 0);
  pml_set_meter_type (pml, "", 0);
  pml_set_instance (pml, "", 0);
} /* pml_all () */

static int
pml_parse_metername (pml_type *pml) {
  const char *base;
  int comma;
  int dot;

  base = pml->next_metername;
  /*  printf ("parse_metername ('%s')\n", base);  */

  /* Skip over any commas left over from previous parse. */
  if (base[0] == ',') {
    base++;
    pml->next_metername = base;
  }

  if (base[0] == '\0') {
    /* We're at the end of the list. */
    pml->point_type = Last_point_type + 1;
    return 0;
  }

  comma = strcspn (base, ",");
  if (comma == 0) {
    pml_all (pml);
    return 1;
  }

  pml->next_metername += comma;

  dot = strcspn (base, ".,");
  if (dot == 0) {
    /* begins with a dot -- certain meters on all points. */
    int mdigit;
 
    pml_set_point_type (pml, "", 0);
    pml_set_point (pml, "", 0);

    mdigit = strcspn (base, "0123456789,");
    if (mdigit <= 1) {
      pfmterr ("Malformed metername \"%s\".\n", base);
      exit (-1);
    } else {
      pml_set_meter_type (pml, base + 1, mdigit - 1);
      pml_set_instance (pml, base + mdigit, comma - mdigit);
    }

  } else if (dot == comma) {
    /* The name does not contain a dot -- all meters on certain points. */
    int pdigit;

    pdigit = strcspn (base, "0123456789,");

    if (pdigit <= 1) {
      pfmterr ("Malformed metername \"%s\".\n", base);
      exit (-1);
    } else {
      pml_set_point_type (pml, base, pdigit);
      pml_set_point (pml, base + pdigit, comma - pdigit);
    }

    pml_set_meter_type (pml, "", 0);
    pml_set_instance (pml, "", 0);

   } else {
    /* The name contains an internal dot -- certain meters on certain points. */
    int pdigit;
    int mdigit;

    pdigit = strcspn (base, "0123456789,.");
    mdigit = strcspn (base + dot + 1, "0123456790,");

    if (pdigit <= 1) {
      pfmterr ("Malformed metername \"%s\".\n", base);
      exit (-1);
    } else {
      pml_set_point_type (pml, base, pdigit);
      pml_set_point (pml, base + pdigit, dot - pdigit);
    }

    if (mdigit <= 1) {
      pfmterr ("Malformed metername \"%s\".\n");
      exit (-1);
    } else {
      pml_set_meter_type (pml, base + dot + 1, mdigit);
      pml_set_instance (pml, base + dot + 1 + mdigit, comma - dot - 1 - mdigit);
    }
  }
  return 1;
} /* parse_metername () */

int
pml_read (pml_type *pml, int *point_type, int *point, int *meter_type,
		int *instance) {

  if (pml->point_type > Last_point_type) {
    /*
     * Nothing left from previous metername.
     * Attempt to parse a new metername.
     */

    if (pml_parse_metername (pml) == 0)
      return 0;
  }

  /* We've got some info ready to go. */
  *point_type = pml->point_type;
  *point = pml->point;
  *meter_type = pml->meter_type;
  *instance = pml->instance;

  /*  Figure out what to do next time.  */

  /*  Consider next instance.  */
  if (pml->all_instance
   && pml->instance < last_instance (pml->point_type, pml->meter_type)) {
    pml->instance ++;
    return 1;
  }

  /*  Consider next meter-type.  */
  if (pml->all_meter_type
   && pml->meter_type < last_meter_type (pml->point_type)) {
    pml->meter_type ++;
    if (pml->all_instance)
      pml->instance = FIRST_INSTANCE;
    return 1;
  }

  /*  Consider next point.  */
  if (pml->all_point
   && pml->point < last_point (pml->point_type)) {
    pml->point ++;
    if (pml->all_meter_type)
      pml->meter_type = first_meter_type (pml->point_type);
    if (pml->all_instance)
      pml->instance = FIRST_INSTANCE;
    return 1;
  }

  /*  Consider next point-type.  */
  if (pml->all_point_type
   && pml->point_type < Last_point_type) {
    pml->point_type ++;
    if (pml->all_point)
      pml->point = FIRST_POINT;
    if (pml->all_meter_type)
      pml->meter_type = first_meter_type (pml->point_type);
    else {
      flag_type found;

      found = FALSE;
      while (pml->point_type <= Last_point_type && !found) {
        int mt;

        for (mt = first_meter_type (pml->point_type);
             mt <= last_meter_type (pml->point_type); mt++) {
          if (streq (name_meter (pml->point_type, mt),
                     pml->meter_type_name)) {
            pml->meter_type = mt; 
          }
        }
        pml->point_type++;
      }
    }
    if (pml->all_instance)
      pml->instance = FIRST_INSTANCE;
    if (pml->point_type <= Last_point_type)
      return 1;
  }

  /*  Consider next metername.  */
  pml_parse_metername (pml);

  return 1;
} /* pml_read () */

void
pml_init (pml_type *pml, const char *text) {
  if (!Initialized) {
    if (cpu_init () == -1) {
      First_point_type = POINT_TYPE_CG;
    } else {
      First_point_type = POINT_TYPE_CPU;
    }
    if (cg_init () == -1) {
      Last_point_type = POINT_TYPE_CPU;
    } else {
      Last_point_type = POINT_TYPE_CG;
    }

    if (First_point_type > Last_point_type) {
      pfmterr ("No hardware meter drivers are installed.\n");
      exit (-1);
    }
    Initialized = TRUE;
  }

  pml->next_metername = text;

  if (text[0] == '\0')
    pml_all (pml);
  else
    pml->point_type = Last_point_type + 1;
} /* pml_init () */

flag_type
pml_at_end (const pml_type *pml) {
  return pml->point_type > Last_point_type
      && pml->next_metername[0] == '\0';
} /* pml_at_end () */

void
pml_point_types (const char *meterlist, flag_type *cpu_flag,
		flag_type *cg_flag) {
  pml_type pml;

  pml_init (&pml, meterlist);

  *cpu_flag = FALSE;
  *cg_flag = FALSE;
  while (!pml_at_end (&pml)) {
    int point_type;
    int point;
    int meter;
    int instance;

    (void)pml_read (&pml, &point_type, &point, &meter, &instance);
    if (point_type == POINT_TYPE_CPU)
      *cpu_flag = TRUE;
    if (point_type == POINT_TYPE_CG)
      *cg_flag = TRUE;
  }
} /* pml_point_types () */
