/* copyright "%c%" */
#ident	"@(#)kern-i386at:io/cpumtr/cpumtr_p6.c	1.2.1.1"
#ident	"$Header$"


/**************************************************************
* Copyright (c) International Comptuters Ltd 1996
* Copyright (c) 1996 HAL Computer Systems
*/

/* P6-specific portions of the CPUMTR driver. */

#ifdef _KERNEL_HEADERS
# include <svc/clock.h>
# include <util/dl.h>
# include <util/types.h>
# include <util/ksynch.h>
# include <io/ddi.h>
# include <io/cpumtr/cpumtr.h>
#else /* !_KERNEL_HEADERS */
# include <sys/clock.h>
# include <sys/types.h>
# include <sys/dl.h>
# include <sys/ksynch.h>
# include <sys/ddi.h>
# ifdef HEADERS_INSTALLED
#  include <sys/cpumtr.h>
# else /* !HEADERS_INSTALLED */
#  include "cpumtr.h"
# endif /* !HEADERS_INSTALED */
#endif /* !_KERNEL_HEADERS */

/* P6 MSRs accessable by driver */
#define CPUMTR_P6REG_CLOCK    0x10
#define CPUMTR_P6REG_CONTROL0 0x186
#define CPUMTR_P6REG_CONTROL1 0x187
#define CPUMTR_P6REG_COUNTER0 0xc1
#define CPUMTR_P6REG_COUNTER1 0xc2

/*******************************************************
* cpumtr_p6_readmsr()
*
* reads the machine specific register specifed by msr and
* puts the value in counter pointed to by pointer.
*/
/* ARGSUSED */
static void
cpumtr_p6_readmsr(volatile cpumtr_uint64_t *pointer, int msr) {
  asm("movl       12(%esp), %ecx"); /* get MSR number into ECX */
  asm("rdmsr");                     /* read from MSR */
  asm("movl       8(%esp), %ecx");  /* get address of pointer */
  asm("movl       %edx, 4(%ecx)");  /* copy into pointer.dl_lop  */
  asm("movl       %eax, (%ecx)");   /* copy into pointer.dl_hop */
} /* cpumtr_p6_readmsr() */


/*******************************************************
* cpumtr_p6_setmsr()
*
* sets the bottom 32 bits of the machine specific register
* selected by msr for which mask=0 with value.
* The top 32 bits and any bits for which mask=1 retain their previous values.
*/
/* ARGSUSED */
static void
cpumtr_p6_setmsr(cpumtr_uint32_t value, int msr, cpumtr_uint32_t mask) {
  asm("movl       12(%esp), %ecx"); /* get MSR number into ECX */
  asm("rdmsr");                     /* read MSR in ECX into EDX:EAX */
  asm("movl       16(%esp), %ebx"); /* get mask into EBX */
  asm("andl       %ebx, %eax");     /* and mask into EAX */
  asm("notl       %ebx");           /* invert the mask */
  asm("andl       8(%esp), %ebx");
  asm("orl        %ebx, %eax");     /* get value for bottom 32 bits */
  asm("movl       $0,%edx");        /* clear top 32 bits */
  asm("wrmsr");                     /* write to MSR in ECX */
} /* cpumtr_p6_setmsr() */


/*******************************************************
* cpumtr_p6_readtsc()
*
* reads the timestamp counter and places it in counter
* pointed to by pointer.
*/
/* ARGSUSED */
void
cpumtr_p6_readtsc(volatile dl_t *pointer) {
  asm("rdtsc");                    /* copy timestamp into EDX:EAX */
  asm("movl       8(%esp), %ecx"); /* get address of pointer */
  asm("movl       %edx, 4(%ecx)"); /* copy EDX into pointer.dl_lop  */
  asm("movl       %eax, (%ecx)");  /* copy ECX into pointer.dl_hop */
} /* cpumtr_p6_readtsc() */


/*******************************************************
* cpumtr_p6_num_setups()
*
* returns the number of setups and sets first_setup.
* Returns -1 if the meter_id is invalid.
*/

int
cpumtr_p6_num_setups(cpumtr_meter_id_t meter_id, uint_t *first_setup) {
  switch (meter_id.cmi_type) {
    case CPUMTR_P6METERS_ALL:
      *first_setup = 0;
      return CPUMTR_P6_NUM_METERS;
    case CPUMTR_P6METERS_TSC:
      *first_setup = 0;
      return 0;
    case CPUMTR_P6METERS_CNT:
      if (meter_id.cmi_instance < CPUMTR_P6_NUM_METERS) {
        *first_setup = meter_id.cmi_instance;
        return 1;
      }
      break;
  }

  /* unknown meter type */
  return -1;
} /* cpumtr_p6_num_setups() */


/*******************************************************
* cpumtr_p6_setup()
*
* sets up the current P6 CPU.
*/
void cpumtr_p6_setup(cpumtr_sample_setup_t *setup) {
  cpumtr_uint32_t value0;  
  cpumtr_uint32_t value1;
  cpumtr_uint32_t mask0;  
  cpumtr_uint32_t mask1;

  /* Get setup values for the .cnt meters. */
  value0 = setup->css_meter_setup[CPUMTR_P6_CNT0_SETUP].p6cnt.c6c_evtsel;
  value1 = setup->css_meter_setup[CPUMTR_P6_CNT1_SETUP].p6cnt.c6c_evtsel;

  /* Always reset INT bits. */
  value0 &= ~CPUMTR_P6APICINT;
  value1 &= ~CPUMTR_P6APICINT;

  /* Never touch undefined bits. */
  mask0 = CPUMTR_P6CNT_RESERVED;
  mask1 = CPUMTR_P6CNT_RESERVED | CPUMTR_P6ENABLE;

  /* Always set EN bit of PerfEvtSel0. */
  value0 |= CPUMTR_P6ENABLE;

  cpumtr_p6_setmsr(value0, CPUMTR_P6REG_CONTROL0, mask0);
  cpumtr_p6_setmsr(value1, CPUMTR_P6REG_CONTROL1, mask1);
} /* cpumtr_p6_setup() */


/*******************************************************
* cpumtr_p6_getsample()
*
* samples the CPU meters into cc_sample and updates cc_start_*.
* The cc_lock should be held on entry and exit.
* Interrupts should be blocked before calling this function.
* Does not block.
*/
void
cpumtr_p6_getsample(cpumtr_cpu_t *cpu, cpumtr_sample_t *sample) {
  cpumtr_uint64_t *new0;
  cpumtr_uint64_t *new1;
  cpumtr_uint64_t *old0;
  cpumtr_uint64_t *old1;

  new0 = &(sample->ca_count[CPUMTR_P6_CNT0_SETUP]);
  new1 = &(sample->ca_count[CPUMTR_P6_CNT1_SETUP]);
  old0 = &(cpu->cc_meter[CPUMTR_P6_CNT0_SETUP].cm_start_count);
  old1 = &(cpu->cc_meter[CPUMTR_P6_CNT1_SETUP].cm_start_count);

  cpumtr_p6_readmsr(new0, CPUMTR_P6REG_COUNTER0);
  cpumtr_p6_readmsr(new1, CPUMTR_P6REG_COUNTER1);
  cpumtr_p6_readtsc(&(sample->ca_end_time));
  sample->ca_end_lbolt = TICKS();

  /*  Each PerfCtr is only 40 bits; check for wrap. */
  new0->dl_hop &= 0xff;
  new1->dl_hop &= 0xff;
  if (new0->dl_hop < (old0->dl_hop & 0xff))
    new0->dl_hop += (old0->dl_hop &~ 0xff) + 0xff + 1;
  else
    new0->dl_hop += (old0->dl_hop &~ 0xff);
  if (new1->dl_hop < (old1->dl_hop & 0xff))
    new1->dl_hop += (old1->dl_hop &~ 0xff) + 0xff + 1;
  else
    new1->dl_hop += (old1->dl_hop &~ 0xff);
} /* cpumtr_p6_getsample () */


/*******************************************************
* cpumtr_p6_start_count()
*
*/
void
cpumtr_p6_start_count(cpumtr_cpu_t *cpu) {
  cpumtr_uint64_t *ptr0;
  cpumtr_uint64_t *ptr1;

  ptr0 = &(cpu->cc_meter[CPUMTR_P6_CNT0_SETUP].cm_start_count);
  ptr1 = &(cpu->cc_meter[CPUMTR_P6_CNT1_SETUP].cm_start_count);

  cpumtr_p6_readmsr(ptr0, CPUMTR_P6REG_COUNTER0);
  cpumtr_p6_readmsr(ptr1, CPUMTR_P6REG_COUNTER1);

  /*  Each PerfCtr is only 40 bits; mask garbage bits. */
  ptr0->dl_hop &= 0xff;
  ptr1->dl_hop &= 0xff;
} /* cpumtr_p6_start_count() */


/*******************************************************
* cpumtr_p6_deactivate()
*
*/
/* ARGSUSED */
void
cpumtr_p6_deactivate(cpumtr_meter_setup_t *setup, uint_t isetup) {
  setup->p6cnt.c6c_evtsel = 0x0;
} /* cpumtr_p6_deactivate () */
