/* copyright "%c%" */
#ident	"@(#)kern-i386at:io/cgmtr/cgmtr_mcu.c	1.2"
#ident	"$Header$"


/**************************************************************
* Copyright (c) International Comptuters Ltd 1996
* Copyright (c) 1996 HAL Computer Systems
*/

/* MCU-specific portions of the CGMTR driver. */

#ifdef _KERNEL_HEADERS
# include <util/dl.h>
# include <util/types.h>
# include <util/ksynch.h>
# include <io/ddi.h>
# include <svc/errno.h>
# include <io/cgmtr/cgmtr.h>
#else /* !_KERNEL_HEADERS */
# include <sys/types.h>
# include <sys/dl.h>
# include <sys/ksynch.h>
# include <sys/ddi.h>
# include <sys/errno.h>
# ifdef HEADERS_INSTALLED
#  include <sys/cgmtr.h>
# else /* !HEADERS_INSTALLED */
#  include "cgmtr.h"
# endif /* !HEADERS_INSTALLED */
#endif /* !_KERNEL_HEADERS */

/* physical address of MCU register space */
#define MCU_BASE_PA ((unsigned)0xfec09000)

/* virtual address of MCU register space */
static caddr_t Mcu_base_va = NULL;

/* Offsets of MCU registers accessable by driver */
#define CGMTR_MCUREG_MCU_BASE    0x018 /* MCU register base address */
#define CGMTR_MCUREG_CYCLE_CNT   0x058 /* free running cycle counter */ 
#define CGMTR_MCUREG_BUS_LAT_TOT 0x060 /* bus latency total */ 
#define CGMTR_MCUREG_REM_LAT_TOT 0x068 /* remote latency total */
#define CGMTR_MCUREG_REM_CNT     0x070 /* count of remote events */
#define CGMTR_MCUREG_BUS_QUAL    0x0A0 /* bus transaction qualifiers */
#define CGMTR_MCUREG_BUS_CNT     0x080 /* bus transaction counters */
#define CGMTR_MCUREG_MSG_QUAL    0x360 /* mesh message qualifiers */
#define CGMTR_MCUREG_MSG_CNT     0x380 /* mesh message counters */
#define CGMTR_MCUREG_PKT_QUAL    0x740 /* packet qualifiers */
#define CGMTR_MCUREG_PKT_CNT     0x3A0 /* packet counters */
#define CGMTR_MCUREG_RPM_WM      0x778 /* RPM watermarks */
#define CGMTR_MCUREG_IDB_WM_CNT  0x398 /* input data buffer watermark cnt */
#define CGMTR_MCUREG_TB_WM_CNT   0x39C /* transmission buffer watermark cnt */
#define CGMTR_MCUREG_QRAM_WM     0x148 /* QRAM watermark */
#define CGMTR_MCUREG_QRAM_WM_CNT 0x394 /* QRAM watermark counter */
#define CGMTR_MCUREG_IHQ_WM_CNT  0x390 /* input header queue watermark cnt */

static struct {
  int offset;
  cgmtr_uint32_t mask;
} cgmtr_mcu_regs[CGMTR_MCU_NUM_METERS] = {
    { CGMTR_MCUREG_CYCLE_CNT,      CGMTR_MCUCYCLE_CNT_COUNT },
    { CGMTR_MCUREG_BUS_LAT_TOT,    CGMTR_MCUBUS_LAT_TOT_COUNT },
    { CGMTR_MCUREG_REM_LAT_TOT,    CGMTR_MCUREM_LAT_TOT_COUNT },
    { CGMTR_MCUREG_REM_CNT,        CGMTR_MCUREM_CNT_COUNT },
    { CGMTR_MCUREG_BUS_CNT,        CGMTR_MCUBUS_CNT_COUNT },
    { CGMTR_MCUREG_BUS_CNT + 0x8,  CGMTR_MCUBUS_CNT_COUNT },
    { CGMTR_MCUREG_BUS_CNT + 0x10, CGMTR_MCUBUS_CNT_COUNT },
    { CGMTR_MCUREG_BUS_CNT + 0x18, CGMTR_MCUBUS_CNT_COUNT },
    { CGMTR_MCUREG_MSG_CNT,        CGMTR_MCUMSG_CNT_COUNT },
    { CGMTR_MCUREG_MSG_CNT + 0x4,  CGMTR_MCUMSG_CNT_COUNT },
    { CGMTR_MCUREG_MSG_CNT + 0x8,  CGMTR_MCUMSG_CNT_COUNT },
    { CGMTR_MCUREG_MSG_CNT + 0xc,  CGMTR_MCUMSG_CNT_COUNT },
    { CGMTR_MCUREG_PKT_CNT,        CGMTR_MCUPKT_CNT_COUNT },
    { CGMTR_MCUREG_PKT_CNT + 0x4,  CGMTR_MCUPKT_CNT_COUNT },
    { CGMTR_MCUREG_PKT_CNT + 0x8,  CGMTR_MCUPKT_CNT_COUNT },
    { CGMTR_MCUREG_PKT_CNT + 0xc,  CGMTR_MCUPKT_CNT_COUNT },
    { CGMTR_MCUREG_PKT_CNT + 0x10, CGMTR_MCUPKT_CNT_COUNT },
    { CGMTR_MCUREG_PKT_CNT + 0x14, CGMTR_MCUPKT_CNT_COUNT },
    { CGMTR_MCUREG_IDB_WM_CNT,     CGMTR_MCUIDB_WM_CNT_COUNT },
    { CGMTR_MCUREG_TB_WM_CNT,      CGMTR_MCUTB_WM_CNT_COUNT },
    { CGMTR_MCUREG_QRAM_WM_CNT,    CGMTR_MCUQRAM_WM_CNT_COUNT },
    { CGMTR_MCUREG_IHQ_WM_CNT,     CGMTR_MCUIHQ_WM_CNT_COUNT }
};

/*******************************************************
* cgmtr_mcu_readoffset()
*
* reads the MCU register specified by offset and
* puts the value in memory pointed to by pointer.
*/
static void
cgmtr_mcu_readoffset(volatile cgmtr_uint64_t *pointer, int offset) {
  volatile caddr_t va;

  va = Mcu_base_va + offset;
  pointer->dl_lop = *(volatile cgmtr_uint32_t *)va;
  pointer->dl_hop = 0x0;
} /* cgmtr_mcu_readoffset() */


/*******************************************************
* cgmtr_mcu_setoffset()
*
* sets the bits of the MCU register specified by offset
* for which mask=0 with value.
* Any bits for which mask=1 retain their previous values.
*/
static void
cgmtr_mcu_setoffset(cgmtr_uint32_t value, int offset, cgmtr_uint32_t mask) {
  volatile caddr_t va;
  cgmtr_uint32_t old;

  va = Mcu_base_va + offset;
  old = *(volatile cgmtr_uint32_t *)va;
  *(volatile cgmtr_uint32_t *)va = ((old & mask) | (value &~ mask));
} /* cgmtr_mcu_setoffset() */


/*******************************************************
* cgmtr_mcu_num_setups()
*
* returns the number of setups and sets first_setup.
* Returns -1 if the meter_id is invalid.
*/
int
cgmtr_mcu_num_setups(cgmtr_meter_id_t meter_id, uint_t *first_setup) {
  switch (meter_id.cgi_type) {
    case CGMTR_MCUMETERS_CYC:
    case CGMTR_MCUMETERS_BUSLAT:
    case CGMTR_MCUMETERS_REMLAT:
    case CGMTR_MCUMETERS_REM:
      if (meter_id.cgi_instance == 0) {
        *first_setup = meter_id.cgi_type;
        return 1;
      }
      break;

    case CGMTR_MCUMETERS_BUS:
      if (meter_id.cgi_instance < CGMTR_MCU_NUM_BUS) {
        *first_setup = 4 + meter_id.cgi_instance;
        return 1;
      }
      break;

    case CGMTR_MCUMETERS_MSG:
      if (meter_id.cgi_instance < CGMTR_MCU_NUM_MSG) {
        *first_setup = 8 + meter_id.cgi_instance;
        return 1;
      }
      break;

    case CGMTR_MCUMETERS_PKT:
      if (meter_id.cgi_instance < CGMTR_MCU_NUM_PKT) {
        *first_setup = 12 + meter_id.cgi_instance;
        return 1;
      }
      break;

    case CGMTR_MCUMETERS_IDWM:
    case CGMTR_MCUMETERS_TBWM:
    case CGMTR_MCUMETERS_QRWM:
    case CGMTR_MCUMETERS_IHWM:
      if (meter_id.cgi_instance == 0) {
        *first_setup = 11 + meter_id.cgi_type;
        return 1;
      }
      break;
  }

  /* unknown meter type */
  return -1;
} /* cgmtr_mcu_num_setups() */


/*******************************************************
* cgmtr_mcu_setup()
*
* sets up the local MCU
*/
void
cgmtr_mcu_setup(cgmtr_sample_setup_t *setup) {
  int i; 

  /* Setup the .bus meters. */
  for (i = 0; i < CGMTR_MCU_NUM_BUS; i++) {
    cgmtr_uint32_t value;

    value = setup->pss_meter_setup[i + 4].mcubus.cgb_qual;
    cgmtr_mcu_setoffset(value, CGMTR_MCUREG_BUS_QUAL + 8*i, CGMTR_MCUBUS_QUAL_R);
  }

  /* Setup the .msg meters. */
  for (i = 0; i < CGMTR_MCU_NUM_MSG; i++) {
    cgmtr_uint32_t value_a;
    cgmtr_uint32_t value_b;

    value_a = setup->pss_meter_setup[i + 8].mcumsg.cgm_quala;
    value_b = setup->pss_meter_setup[i + 8].mcumsg.cgm_qualb;

    cgmtr_mcu_setoffset(value_a, CGMTR_MCUREG_MSG_QUAL + 8*i,
      CGMTR_MCUMSG_QUALA_R);
    cgmtr_mcu_setoffset(value_b, CGMTR_MCUREG_MSG_QUAL + 8*i + 4,
      CGMTR_MCUMSG_QUALB_R);
  }

  /* Setup the .pkt meters. */
  for (i = 0; i < CGMTR_MCU_NUM_PKT; i++) {
    cgmtr_uint32_t value;

    value = setup->pss_meter_setup[i + 12].mcupkt.cgp_qual;
    cgmtr_mcu_setoffset(value, CGMTR_MCUREG_PKT_QUAL + 8*i, CGMTR_MCUPKT_QUAL_R);
  }

  {
    cgmtr_uint32_t wm;
    cgmtr_uint32_t mode;

    /* Setup the .idwm meter. */
    wm = setup->pss_meter_setup[18].mcuidwm.cgd_wm << 8;
    mode = setup->pss_meter_setup[18].mcuidwm.cgd_select << 31;
    cgmtr_mcu_setoffset(wm, CGMTR_MCUREG_RPM_WM, ~CGMTR_MCURPM_WM_IBUF);
    cgmtr_mcu_setoffset(mode, CGMTR_MCUREG_IDB_WM_CNT, ~CGMTR_MCUIDB_WM_CNT_MODE);

    /* Setup the .tbwm meter. */
    wm = setup->pss_meter_setup[19].mcutbwm.cgt_wm;
    cgmtr_mcu_setoffset(wm, CGMTR_MCUREG_RPM_WM, ~CGMTR_MCURPM_WM_TBUF);

    /* Setup the .qrwm meter. */
    wm = setup->pss_meter_setup[20].mcuqrwm.cgq_wm;
    cgmtr_mcu_setoffset(wm, CGMTR_MCUREG_QRAM_WM, ~CGMTR_MCUQRAM_WM_MARK);

    /* Setup the .ihwm meter. */
    wm = setup->pss_meter_setup[21].mcuihwm.cgh_wm << 28;
    cgmtr_mcu_setoffset(wm, CGMTR_MCUREG_IHQ_WM_CNT, ~CGMTR_MCUIHQ_WM_CNT_MARK);
  }
} /* cgmtr_mcu_setup() */


/*******************************************************
* cgmtr_mcu_getsample()
*
* samples the MCU meters into pc_sample.
* The pc_lock should be held on entry and exit.
* Interrupts should be blocked before calling this function.
* Does not block.
*/
void
cgmtr_mcu_getsample(cgmtr_cg_t *cg, cgmtr_sample_t *sample) {
  uint_t imeter;

  /* Sample the MCU registers. */
  for (imeter = 0; imeter < CGMTR_MCU_NUM_METERS; imeter++) {
    cgmtr_mcu_readoffset(&(sample->pa_count[imeter]), cgmtr_mcu_regs[imeter].offset);
  }

  /* Check for wrap-arounds. */
  for (imeter = 0; imeter < CGMTR_MCU_NUM_METERS; imeter++) {
    cgmtr_uint64_t *new;
    cgmtr_uint64_t *old;
    cgmtr_uint32_t mask;

    new = &(sample->pa_count[imeter]);
    old = &(cg->pc_meter[imeter].cg_start_count);
    mask = cgmtr_mcu_regs[imeter].mask;

    new->dl_lop += (old->dl_lop &~ mask);
    if (new->dl_lop < old->dl_lop) {
      new->dl_lop += mask + 1;
      if (new->dl_lop < old->dl_lop)
        new->dl_hop ++;
    }
  }
} /* cgmtr_mcu_get_sample() */

/*******************************************************
* cgmtr_mcu_start_count()
*
*/
void
cgmtr_mcu_start_count(cgmtr_cg_t *cg) {
  uint_t imeter;

  /* Sample the MCU registers. */
  for (imeter = 0; imeter < CGMTR_MCU_NUM_METERS; imeter++) {
    cgmtr_mcu_readoffset(&(cg->pc_meter[imeter].cg_start_count),
      cgmtr_mcu_regs[imeter].offset);
  }

  /* Mask off garbage. */
  for (imeter = 0; imeter < CGMTR_MCU_NUM_METERS; imeter++) {
    cgmtr_uint64_t *old;
    cgmtr_uint32_t mask;

    old = &(cg->pc_meter[imeter].cg_start_count);
    mask = cgmtr_mcu_regs[imeter].mask;

    old->dl_lop &= mask;
    old->dl_hop = 0x0;
  }
} /* cgmtr_mcu_start_count() */

/*******************************************************
* cgmtr_mcu_init()
*
*/
int
cgmtr_mcu_init(void) {
  /* Map the MCU registers into memory. */
  Mcu_base_va = physmap((paddr_t)MCU_BASE_PA, 0x1000, KM_SLEEP);
  if (Mcu_base_va == NULL) {
    return ENOMEM;
  }
  return CGMTR_NO_ERROR;
} /* cgmtr_mcu_init() */


/*******************************************************
* cgmtr_mcu_present()
*
* returns TRUE if an MCU is present.
*/
bool_t
cgmtr_mcu_present(void) {
  cgmtr_uint64_t reg;

  cgmtr_mcu_readoffset (&reg, CGMTR_MCUREG_MCU_BASE);
  return reg.dl_lop == MCU_BASE_PA;
} /* cgmtr_mcu_present() */


/*******************************************************
* cgmtr_mcu_deactivate()
*
*/
void
cgmtr_mcu_deactivate(cgmtr_meter_setup_t *setup, uint_t isetup) {
  switch (isetup) {
    case 0:
    case 1:
    case 2:
    case 3:
      break;

    case 4:
    case 5:
    case 6:
    case 7:
      setup->mcubus.cgb_qual = 0x0;
      break;

    case 8:
    case 9:
    case 10:
    case 11:
      setup->mcumsg.cgm_quala = 0x0;
      setup->mcumsg.cgm_qualb = 0x0;
      break;

    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
      setup->mcupkt.cgp_qual = 0x0;
      break;

    case 18:
      setup->mcuidwm.cgd_wm = 0x0;
      setup->mcuidwm.cgd_select = 0x0;
      break;

    case 19:
      setup->mcutbwm.cgt_wm = 0x0;
      break;

    case 20:
      setup->mcuqrwm.cgq_wm = 0x0;
      break;

    case 21:
      setup->mcuihwm.cgh_wm = 0x0;
      break;
  }
} /* cgmtr_mcu_deactivate() */
