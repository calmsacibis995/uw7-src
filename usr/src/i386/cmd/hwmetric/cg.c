/* copyright "%c%" */
#ident	"@(#)hwmetric:cg.c	1.2"
#ident	"$Header$"


/* Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved. */

/* cg.c - CG-specific functions */

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
#include <stdio.h>

cgmtr_cgtype_t Cg_type = CGMTR_NOT_SUPPORTED;

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
        case CGMTR_PIUMETERS_MHB:   return 2;
      }
      break;

    case CGMTR_TEST:
      switch (meter_type) {
        case CGMTR_TESTMETERS_PC:    return instance;
        case CGMTR_TESTMETERS_MHB:   return 2;
      }
      break;
  }

  pfmterr ("Internal error in cg_meter_index(meter_type=%d).\n", meter_type);
  exit (-1);
} /* cg_meter_index () */

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
        case CGMTR_PIUMETERS_MHB:   return "mhb";
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
int
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
        case CGMTR_PIUMETERS_MHB:   return 0;
      }
      break;

    case CGMTR_TEST:
      switch (meter_type) {
        case CGMTR_TESTMETERS_PC:    return 1;
        case CGMTR_TESTMETERS_MHB:   return 0;
      }
      break;
  }

  pfmterr ("Internal error in cg_last_instance(meter_type=%d).\n", meter_type);
  exit (-1);
} /* cg_last_instance () */

/* Return the first meter-type. */
int
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
int
cg_last_meter_type (void) {
  switch (Cg_type) {
    case CGMTR_MCU:
      return CGMTR_MCUMETERS_IHWM;
    case CGMTR_PIU:
      return CGMTR_PIUMETERS_MHB;
    case CGMTR_TEST:
      return CGMTR_TESTMETERS_MHB;
  }
    
  pfmterr ("Internal error in cg_last_meter_type(void).\n");
  exit (-1);
} /* cg_last_meter_type () */

int
cg_init (void) {
  cgmtr_rdstats_t *rdstats;
  int i;

  if (cgmtr_open () == -1) {
    return -1;
  }

  rdstats = cgmtr_rdstats ();

  for (i = 0; i < Num_cgs; i++) {
    cgmtr_cgtype_t cg_type;

    cg_type = rdstats->pr_cg_stats[i].pcs_cgtype;
    if (i == 0) {
      Cg_type = cg_type;
    } else if (cg_type != Cg_type) {
      pfmterr ("Multiple CG types in system.\n");
      return -1;
    }
  }

  free (rdstats->pr_cg_stats);
  free (rdstats);

  return 0;
} /* cg_init () */

void
cg_parse_setups (const char *str, int meter, cgmtr_meter_setup_t *result) {
  
  switch (Cg_type) {
    case CGMTR_MCU:
      switch (meter) {
        case CGMTR_MCUMETERS_CYC:
        case CGMTR_MCUMETERS_BUSLAT:
        case CGMTR_MCUMETERS_REMLAT:
        case CGMTR_MCUMETERS_REM:
          /*  Accept only an empty string.  */
          if (strlen (str) > 0) {
            pfmterr ("Garbage in setup '%s' for cg.%s.\n",
              str, cg_name_meter (meter));
            exit (-1);
          }
          return;

        case CGMTR_MCUMETERS_BUS: {
          /* Get 32-bit hex value for BUS_QUAL register.  */
          char *cp;
          unsigned long value;

          value = strtoul (str, &cp, 16);
          if (cp == str || *cp != '\0') {
            pfmterr ("Garbage in setup '%s' for cg.bus.\n", str);
            exit (-1);
          }
          result->mcubus.cgb_qual = value;
        } return;

        case CGMTR_MCUMETERS_MSG: {
          /* Get 64-bit hex value for MSG_QUAL register.  */
          char *cp;
          char *cp2;
          unsigned long value;
          unsigned long value2;

          value = strtoul (str, &cp, 16);
          if (cp == str || *cp != ',') {
            pfmterr ("Garbage in setup '%s' for cg.msg.\n", str);
            exit (-1);
          }
          value2 = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != '\0') {
            pfmterr ("Garbage in setup '%s' for cg.msg.\n", str);
            exit (-1);
          }

          result->mcumsg.cgm_quala = value;
          result->mcumsg.cgm_qualb = value2;
        } return;

        case CGMTR_MCUMETERS_PKT: {
          /* Get 32-bit hex value for PKT_QUAL register.  */
          char *cp;
          unsigned long value;

          value = strtoul (str, &cp, 16);
          if (cp == str || *cp != '\0') {
            pfmterr ("Garbage in setup '%s' for cg.pkt.\n", str);
            exit (-1);
          }
          result->mcupkt.cgp_qual = value;
        } return;

        case CGMTR_MCUMETERS_IDWM: {
          /* Get 8-bit hex value for RPM_WM register IBUF wm 
             and 1-bit hex value for IDB_WM_CNT register  */
          char *cp;
          char *cp2;
          unsigned long value;
          unsigned value2;

          value = strtoul (str, &cp, 16);
          if (cp == str || *cp != ',' || value <= (1<<8)) {
            pfmterr ("Garbage in setup '%s' for cg.idwm.\n", str);
            exit (-1);
          }
          value2 = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != '\0') {
            pfmterr ("Garbage in setup '%s' for cg.idwm.\n", str);
            exit (-1);
          }

          result->mcuidwm.cgd_wm = value;
          result->mcuidwm.cgd_select = value2;
        } return;

        case CGMTR_MCUMETERS_TBWM: {
          /* Get 8-bit hex value for RPM_WM register TBUF wm.  */
          char *cp;
          unsigned long value;

          value = strtoul (str, &cp, 16);
          if (cp == str || *cp != '\0' || value <= (1<<8)) {
            pfmterr ("Garbage in setup '%s' for cg.tbwm.\n", str);
            exit (-1);
          }
          result->mcutbwm.cgt_wm = value;
        } return;

        case CGMTR_MCUMETERS_QRWM: {
          /* Get 8-bit hex value for QRAM_WM register.  */
          char *cp;
          unsigned long value;

          value = strtoul (str, &cp, 16);
          if (cp == str || *cp != '\0' || value <= (1<<8)) {
            pfmterr ("Garbage in setup '%s' for cg.qrwm.\n", str);
            exit (-1);
          }
          result->mcuqrwm.cgq_wm = value;
        } return;

        case CGMTR_MCUMETERS_IHWM: {
          /* Get 3-bit hex value for IHQ_WM_CNT regsiter wm.  */
          char *cp;
          unsigned long value;

          value = strtoul (str, &cp, 16);
          if (cp == str || *cp != '\0' || value <= (1<<3)) {
            pfmterr ("Garbage in setup '%s' for cg.ihwm.\n", str);
            exit (-1);
          }
          result->mcuihwm.cgh_wm = value;
        } return;

      }
      break;

    case CGMTR_PIU:
      switch (meter) {
        case CGMTR_PIUMETERS_PC: {
          /* Get 64-bit hex values for _COMPARE and _MASK registers.  */
          char *cp;
          char *cp2;
          unsigned long compare_hi;
          unsigned long compare_lo;
          unsigned long mask_hi;
          unsigned long mask_lo;

          compare_hi = strtoul (str, &cp, 16);
          if (cp == str || *cp != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          compare_lo = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          mask_hi = strtoul (cp2 + 1, &cp, 16);
          if (cp == cp2 + 1 || *cp != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          mask_lo = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != '\0') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }

          result->piupc.ppp_compare.dl_hop = compare_hi;
          result->piupc.ppp_compare.dl_lop = compare_lo;
          result->piupc.ppp_mask.dl_hop = mask_hi;
          result->piupc.ppp_mask.dl_lop = mask_lo;
        } return;

        case CGMTR_PIUMETERS_MHB: {
          /* Get 64-bit hex values for METER_COMPARE and METER_MASK regs.  */
          char *cp;
          char *cp2;
          unsigned long compare_hi;
          unsigned long compare_lo;
          unsigned long mask_hi;
          unsigned long mask_lo;

          compare_hi = strtoul (str, &cp, 16);
          if (cp == str || *cp != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          compare_lo = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          mask_hi = strtoul (cp2 + 1, &cp, 16);
          if (cp == cp2 + 1 || *cp != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          mask_lo = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != '\0') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }

          result->piumhb.pcg_compare.dl_hop = compare_hi;
          result->piumhb.pcg_compare.dl_lop = compare_lo;
          result->piumhb.pcg_mask.dl_hop = mask_hi;
          result->piumhb.pcg_mask.dl_lop = mask_lo;
        } return;
      }
      break;

    case CGMTR_TEST:
      switch (meter) {
        case CGMTR_TESTMETERS_PC: {
          /* Get 64-bit hex values for _COMPARE and _MASK registers.  */
          char *cp;
          char *cp2;
          unsigned long compare_hi;
          unsigned long compare_lo;
          unsigned long mask_hi;
          unsigned long mask_lo;

          compare_hi = strtoul (str, &cp, 16);
          if (cp == str || *cp != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          compare_lo = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          mask_hi = strtoul (cp2 + 1, &cp, 16);
          if (cp == cp2 + 1 || *cp != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          mask_lo = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != '\0') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }

          result->testpc.ppp_compare.dl_hop = compare_hi;
          result->testpc.ppp_compare.dl_lop = compare_lo;
          result->testpc.ppp_mask.dl_hop = mask_hi;
          result->testpc.ppp_mask.dl_lop = mask_lo;
        } return;

        case CGMTR_TESTMETERS_MHB: {
          /* Get 64-bit hex values for METER_COMPARE and METER_MASK regs.  */
          char *cp;
          char *cp2;
          unsigned long compare_hi;
          unsigned long compare_lo;
          unsigned long mask_hi;
          unsigned long mask_lo;

          compare_hi = strtoul (str, &cp, 16);
          if (cp == str || *cp != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          compare_lo = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          mask_hi = strtoul (cp2 + 1, &cp, 16);
          if (cp == cp2 + 1 || *cp != ',') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }
          mask_lo = strtoul (cp + 1, &cp2, 16);
          if (cp2 == cp + 1 || *cp2 != '\0') {
            pfmterr ("Garbage in setup '%s' for cg.pc.\n", str);
            exit (-1);
          }

          result->testmhb.pcg_compare.dl_hop = compare_hi;
          result->testmhb.pcg_compare.dl_lop = compare_lo;
          result->testmhb.pcg_mask.dl_hop = mask_hi;
          result->testmhb.pcg_mask.dl_lop = mask_lo;
        } return;
      }
      break;
  }

  pfmterr ("Internal error in cg_parse_setups().\n");
  exit (-1);
} /* cg_parse_setups () */
