/* copyright "%c%" */
#ident	"@(#)kern-i386at:io/cgmtr/cgmtr_piu.c	1.2"
#ident	"$Header$"


/**************************************************************
* Copyright (c) International Comptuters Ltd 1996
* Copyright (c) 1996 HAL Computer Systems
*/

/* PIU-specific portions of the CGMTR driver. */

#ifdef _KERNEL_HEADERS
# include <util/dl.h>
# include <util/types.h>
# include <util/ksynch.h>
# include <io/ddi.h>
# include <io/cgmtr/cgmtr.h>
#else /* !_KERNEL_HEADERS */
# include <sys/types.h>
# include <sys/dl.h>
# include <sys/ksynch.h>
# include <sys/ddi.h>
# ifdef HEADERS_INSTALLED
#  include <sys/cgmtr.h>
# else /* !HEADERS_INSTALLED */
#  include "cgmtr.h"
# endif /* !HEADERS_INSTALLED */
#endif /* !_KERNEL_HEADERS */

static caddr_t Piu_base_va = NULL;

/* Offsets of PIU registers accessable by driver */
#define CGMTR_PIUREG_METER_COMPARE         0x0A0 /* qualifiers for sample */ 
#define CGMTR_PIUREG_METER_MASK            0x0A8 /* mask for sample */ 
#define CGMTR_PIUREG_METER_HISTORY_BUFFER  0x0B0 /* captured sample */
#define CGMTR_PIUREG_PERF_COUNTER0         0x0C8 /* count of events */
#define CGMTR_PIUREG_PERF_COUNTER0_COMPARE 0x0B8 /* qualifiers for events */
#define CGMTR_PIUREG_PERF_COUNTER0_MASK    0x0C0 /* mask for events */
#define CGMTR_PIUREG_PERF_COUNTER1         0x0CC /* count of events */
#define CGMTR_PIUREG_PERF_COUNTER1_COMPARE 0x0D0 /* qualifiers for events */
#define CGMTR_PIUREG_PERF_COUNTER1_MASK    0x0D4 /* mask for events */

/*******************************************************
* cgmtr_piu_readoffset()
*
* reads the PIU register specified by offset and
* puts the value in memory pointed to by pointer.
*/
static void
cgmtr_piu_readoffset(volatile cgmtr_uint64_t *pointer, int offset) {
  volatile caddr_t va;

  va = Piu_base_va + offset;
  pointer->dl_lop = ((volatile cgmtr_uint32_t *)va)[0];
  if (offset == CGMTR_PIUREG_METER_HISTORY_BUFFER)
    pointer->dl_hop = ((volatile cgmtr_uint32_t *)va)[1];
  else
    pointer->dl_hop = 0x0;
} /* cgmtr_piu_readoffset() */


/*******************************************************
* cgmtr_piu_setoffset()
*
* sets the bits of the PIU register specified by offset with value.
*/
static void
cgmtr_piu_setoffset(cgmtr_uint32_t value, int offset) {
  volatile caddr_t va;

  va = Piu_base_va + offset;
  *(volatile cgmtr_uint32_t *)va = value;
} /* cgmtr_piu_setoffset() */


/*******************************************************
* cgmtr_piu_num_setups()
*
* returns the number of setups and sets first_setup.
* Returns -1 if the meter_id is invalid.
*/

int
cgmtr_piu_num_setups(cgmtr_meter_id_t meter_id, uint_t *first_setup) {
  switch (meter_id.cgi_type) {
    case CGMTR_PIUMETERS_PC:
      if (meter_id.cgi_instance < CGMTR_PIU_NUM_PC) {
        *first_setup = meter_id.cgi_instance;
        return 1;
      }
      break;

    case CGMTR_PIUMETERS_MHB:
      if (meter_id.cgi_instance == 0) {
        *first_setup = 2;
        return 1;
      }
      break;
  }

  /* unknown meter type */
  return -1;
} /* cgmtr_piu_num_setups() */


/*******************************************************
* cgmtr_piu_setup()
*
* sets up the local PIU
*/
void
cgmtr_piu_setup(cgmtr_sample_setup_t *setup) {
  cgmtr_piumhb_setup_t *pcg;

  {
    cgmtr_piupc_setup_t *ppp;

    /* Setup the .pc0 meter. */
    ppp = &(setup->pss_meter_setup[0].piupc);
    cgmtr_piu_setoffset(ppp->ppp_compare.dl_hop,
      CGMTR_PIUREG_PERF_COUNTER0_COMPARE+4);
    cgmtr_piu_setoffset(ppp->ppp_compare.dl_lop,
      CGMTR_PIUREG_PERF_COUNTER0_COMPARE);
    cgmtr_piu_setoffset(ppp->ppp_mask.dl_hop,
      CGMTR_PIUREG_PERF_COUNTER0_MASK+4);
    cgmtr_piu_setoffset(ppp->ppp_mask.dl_lop,
      CGMTR_PIUREG_PERF_COUNTER0_MASK);

    /* Setup the .pc1 meter. */
    ppp = &(setup->pss_meter_setup[1].piupc);
    cgmtr_piu_setoffset(ppp->ppp_compare.dl_hop,
      CGMTR_PIUREG_PERF_COUNTER1_COMPARE+4);
    cgmtr_piu_setoffset(ppp->ppp_compare.dl_lop,
      CGMTR_PIUREG_PERF_COUNTER1_COMPARE);
    cgmtr_piu_setoffset(ppp->ppp_mask.dl_hop,
      CGMTR_PIUREG_PERF_COUNTER1_MASK+4);
    cgmtr_piu_setoffset(ppp->ppp_mask.dl_lop,
      CGMTR_PIUREG_PERF_COUNTER1_MASK);
  }

  /* Setup the .pc1 meter. */
  pcg = &(setup->pss_meter_setup[2].piumhb);
  cgmtr_piu_setoffset(pcg->pcg_compare.dl_hop, CGMTR_PIUREG_METER_COMPARE+4);
  cgmtr_piu_setoffset(pcg->pcg_compare.dl_lop, CGMTR_PIUREG_METER_COMPARE);
  cgmtr_piu_setoffset(pcg->pcg_mask.dl_hop, CGMTR_PIUREG_METER_MASK+4);
  cgmtr_piu_setoffset(pcg->pcg_mask.dl_lop, CGMTR_PIUREG_METER_MASK);
} /* cgmtr_piu_setup() */


/*******************************************************
* cgmtr_piu_getsample()
*
* samples the PIU meters into pc_sample.
* The pc_lock should be held on entry and exit.
* Interrupts should be blocked before calling this function.
* Does not block.
*/
void
cgmtr_piu_getsample(cgmtr_cg_t *cg, cgmtr_sample_t *sample) {
  cgmtr_uint64_t *new;
  cgmtr_uint64_t *old;

  /* Sample the PIU registers. */
  cgmtr_piu_readoffset(&(sample->pa_count[0]), CGMTR_PIUREG_PERF_COUNTER0);
  cgmtr_piu_readoffset(&(sample->pa_count[1]), CGMTR_PIUREG_PERF_COUNTER1);
  cgmtr_piu_readoffset(&(sample->pa_count[2]),
    CGMTR_PIUREG_METER_HISTORY_BUFFER);

  /* Check for wrap-arounds. */
  new = &(sample->pa_count[0]);
  old = &(cg->pc_meter[0].cg_start_count);
  new->dl_hop = old->dl_hop;
  if (new->dl_lop < old->dl_lop) {
    new->dl_hop ++;
  }

  new = &(sample->pa_count[1]);
  old = &(cg->pc_meter[1].cg_start_count);
  new->dl_hop = old->dl_hop;
  if (new->dl_lop < old->dl_lop) {
    new->dl_hop ++;
  }
} /* cgmtr_get_sample() */


/*******************************************************
* cgmtr_piu_start_count()
*
*/
void
cgmtr_piu_start_count(cgmtr_cg_t *cg) {
  /* Sample the PIU registers. */
  cgmtr_piu_readoffset(&(cg->pc_meter[0].cg_start_count),
    CGMTR_PIUREG_PERF_COUNTER0);
  cgmtr_piu_readoffset(&(cg->pc_meter[1].cg_start_count),
    CGMTR_PIUREG_PERF_COUNTER1);
  cgmtr_piu_readoffset(&(cg->pc_meter[2].cg_start_count),
    CGMTR_PIUREG_METER_HISTORY_BUFFER);
} /* cgmtr_piu_start_count() */

/*******************************************************
* cgmtr_piu_init()
*
*/
int
cgmtr_piu_init(void) {
  /* Map the PIU registers into memory. */
  /* TODO */
  return CGMTR_NO_ERROR;
} /* cgmtr_piu_init() */


/*******************************************************
* cgmtr_piu_present()
*
* returns TRUE if a PIU is present.
*/
bool_t
cgmtr_piu_present(void) {
  /* TODO */
  return FALSE;
} /* cgmtr_piu_present() */


/*******************************************************
* cgmtr_piu_deactivate()
*
*/
void cgmtr_piu_deactivate(cgmtr_meter_setup_t *setup, uint_t isetup) {
  switch (isetup) {
    case 0:
    case 1:
      setup->piupc.ppp_compare.dl_lop = 0x30;
      setup->piupc.ppp_compare.dl_hop = 0x0;
      setup->piupc.ppp_mask.dl_lop = (unsigned)0xffffffff;
      setup->piupc.ppp_mask.dl_hop = (unsigned)0xffffffff;
      break;

    case 2:
      setup->piumhb.pcg_compare.dl_lop = 0x30;
      setup->piumhb.pcg_compare.dl_hop = 0x0;
      setup->piumhb.pcg_mask.dl_lop = (unsigned)0xffffffff;
      setup->piumhb.pcg_mask.dl_hop = (unsigned)0xffffffff;
      break;
  }
} /* cgmtr_piu_deactivate() */
