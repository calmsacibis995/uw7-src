/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/pml.c	1.2"
#ident "$Header$"

/* pml.c - parse meterlists for the 'sar' command */

#include <pfmt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "../sa.h"
#include "sar.h"


#define streq(a,b) (strcmp(a,b)==0)

#define FIRST_INSTANCE 0
#define FIRST_POINT 0

#define Cpu_type (hwmetric_init_info.cpu_type)
#define Cg_type (hwmetric_init_info.cg_type)

static int First_point_type = POINT_TYPE_CPU;
static int Last_point_type = POINT_TYPE_CG;

#define POINT_TYPE_NAME_LENGTH 3

/* Format and print an error message. */
static void
pfmterr (const char *fmt, ...) {
  va_list ap; 
  va_start (ap, fmt);
  
  (void)vpfmt (stderr, MM_STD|MM_NOGET|MM_ERROR, fmt, ap);

  va_end (ap);
}

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
static int
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
static int
cpu_first_meter_type (void) {
  switch (Cpu_type) {
    case CPUMTR_P6:
      return CPUMTR_P6METERS_TSC;
  }

  pfmterr ("Internal error in cpu_first_meter_type(), cputype=%d.\n", Cpu_type);
  exit (-1);
} /* cpu_first_meter_type () */

/* get the index of the last meter-type */
static int
cpu_last_meter_type (void) {
  switch (Cpu_type) {
    case CPUMTR_P6:
      return CPUMTR_P6METERS_CNT;
  }

  pfmterr ("Internal error in cpu_last_meter_type().\n");
  exit (-1);
} /* cpu_last_meter_type () */

/*  Return the index of a meter. */
int
cpu_meter_index (int meter_type, int instance) {
  if (instance > cpu_last_instance (meter_type) || instance < 0) {
    pfmterr ("Internal error in cpu_meter_index ().\n");
    exit (-1);
  }

  switch (Cpu_type) {
    case CPUMTR_P6:
      switch (meter_type) {
        case CPUMTR_P6METERS_TSC:    return 0;
        case CPUMTR_P6METERS_CNT:    return 1 + instance;
      }
      break;
  }   
  
  pfmterr ("Internal error in cpu_meter_index(meter_type=%d).\n", meter_type);
  exit (-1);
} /* cpu_meter_index () */


/*  Return the name of a meter-type. */
const char *
cg_name_meter (int meter_type) {
  switch (Cg_type) {
    case CGMTR_MCU:
      switch (meter_type) {
        case CGMTR_MCUMETERS_CYC:    return "cyc";
        case CGMTR_MCUMETERS_BUSLAT: return "buslat";
        case CGMTR_MCUMETERS_REMLAT: return "remlat";
        case CGMTR_MCUMETERS_REM:    return "rem";
        case CGMTR_MCUMETERS_BUS:    return "bus";
        case CGMTR_MCUMETERS_MSG:    return "msg";
        case CGMTR_MCUMETERS_PKT:    return "pkt";
        case CGMTR_MCUMETERS_IDWM:   return "idwm";
        case CGMTR_MCUMETERS_TBWM:   return "tbwm";
        case CGMTR_MCUMETERS_QRWM:   return "qrwm";
        case CGMTR_MCUMETERS_IHWM:   return "ihwm";
      }
      break;

    case CGMTR_PIU:
      switch (meter_type) {
        case CGMTR_PIUMETERS_PC:    return "pc";
      }
      break;

    case CGMTR_TEST:
      switch (meter_type) {
        case CGMTR_TESTMETERS_PC:    return "pc";
        case CGMTR_TESTMETERS_MHB:   return "mhb";
      }
      break;
  }

  pfmterr ("Internal error in cg_name_meter(meter_type=%d).\n", meter_type);
  exit (-1);
} /* cg_name_meter () */

/* Return the last instance of each meter type. */
static int
cg_last_instance (int meter_type) {
  switch (Cg_type) {
    case CGMTR_MCU:
      switch (meter_type) {
        case CGMTR_MCUMETERS_CYC:    return 0;
        case CGMTR_MCUMETERS_BUSLAT: return 0;
        case CGMTR_MCUMETERS_REMLAT: return 0;
        case CGMTR_MCUMETERS_REM:    return 0;
        case CGMTR_MCUMETERS_BUS:    return 3;
        case CGMTR_MCUMETERS_MSG:    return 3;
        case CGMTR_MCUMETERS_PKT:    return 5;
        case CGMTR_MCUMETERS_IDWM:   return 0;
        case CGMTR_MCUMETERS_TBWM:   return 0;
        case CGMTR_MCUMETERS_QRWM:   return 0;
        case CGMTR_MCUMETERS_IHWM:   return 0;
      }
      break;

    case CGMTR_PIU:
      switch (meter_type) {
        case CGMTR_PIUMETERS_PC:    return 1;
      }
      break;

    case CGMTR_TEST:
      switch (meter_type) {
        case CGMTR_TESTMETERS_PC:    return 1;
      }
      break;
  }

  pfmterr ("Internal error in cg_last_instance(meter_type=%d).\n", meter_type);
  exit (-1);
} /* cg_last_instance () */

/*  Return the index of a meter. */
int
cg_meter_index (int meter_type, int instance) {
  if (instance > cg_last_instance (meter_type) || instance < 0) {
    pfmterr ("Internal error in cg_meter_index ().\n");
    exit (-1);
  }

  switch (Cg_type) {
    case CGMTR_MCU:
      switch (meter_type) {
        case CGMTR_MCUMETERS_CYC:    return 0;
        case CGMTR_MCUMETERS_BUSLAT: return 1;
        case CGMTR_MCUMETERS_REMLAT: return 2;
        case CGMTR_MCUMETERS_REM:    return 3;
        case CGMTR_MCUMETERS_BUS:    return 4 + instance;
        case CGMTR_MCUMETERS_MSG:    return 8 + instance;
        case CGMTR_MCUMETERS_PKT:    return 12 + instance;
        case CGMTR_MCUMETERS_IDWM:   return 18;
        case CGMTR_MCUMETERS_TBWM:   return 19;
        case CGMTR_MCUMETERS_QRWM:   return 20;
        case CGMTR_MCUMETERS_IHWM:   return 21;
      }
      break;

    case CGMTR_PIU:
      switch (meter_type) {
        case CGMTR_PIUMETERS_PC:    return instance;
      }
      break;

    case CGMTR_TEST:
      switch (meter_type) {
        case CGMTR_TESTMETERS_PC:    return instance;
      }
      break;
  }

  pfmterr ("Internal error in cg_meter_index(meter_type=%d).\n", meter_type);
  exit (-1);
} /* cg_meter_index () */

/* Return the first meter-type. */
static int
cg_first_meter_type (void) {
  switch (Cg_type) {
    case CGMTR_MCU:
      return CGMTR_MCUMETERS_CYC;
    case CGMTR_PIU:
      return CGMTR_PIUMETERS_PC;
    case CGMTR_TEST:
      return CGMTR_TESTMETERS_PC;
  }
    
  pfmterr ("Internal error in cg_first_meter_type(void).\n");
  exit (-1);
} /* cg_first_meter_type () */

/* Return the last meter-type. */
static int
cg_last_meter_type (void) {
  switch (Cg_type) {
    case CGMTR_MCU:
      return CGMTR_MCUMETERS_IHWM;
    case CGMTR_PIU:
      return CGMTR_PIUMETERS_PC;
    case CGMTR_TEST:
      return CGMTR_TESTMETERS_PC;
  }
    
  pfmterr ("Internal error in cg_last_meter_type(void).\n");
  exit (-1);
} /* cg_last_meter_type () */


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
    pfmterr ("Malformed metername:  invalid point-type leng=%d (%s).\n", leng, start);
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
      flag found;

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
  (void)pml_parse_metername (pml);

  return 1;
} /* pml_read () */

void
pml_init (pml_type *pml, const char *text) {
#ifdef DEBUG
  printf ("pml_init (%s)\n", text);
#endif /* DEBUG */
  pml->next_metername = text;

  if (text[0] == '\0')
    pml_all (pml);
  else
    pml->point_type = Last_point_type + 1;
} /* pml_init () */

flag
pml_at_end (const pml_type *pml) {
  return pml->point_type > Last_point_type
      && pml->next_metername[0] == '\0';
} /* pml_at_end () */

int
pml_meter_index (int ptype, int mtype, int inst) {
  switch (ptype) {
    case POINT_TYPE_CPU:
      return cpu_meter_index (mtype, inst);
    case POINT_TYPE_CG:
      return cg_meter_index (mtype, inst);
  }

  pfmterr ("Internal error in pml_meter_index(%d).\n", ptype);
  exit (-1);
} /* pml_meter_index() */
