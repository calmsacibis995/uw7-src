#ifndef _PSM_MPS_MPS_H  /* wrapper symbol for kernel use */
#define _PSM_MPS_MPS_H  /* subject to change without notice */

#ident	"@(#)kern-i386at:psm/mps/mps.h	1.5.1.1"
#ident	"$Header$"

/*
 * Definitions for Intel PC+MP (MP Spec.) Platform Specification.
 */

#define MPS_INTS_PER_LEVEL      32
#define MPS_LEVELS              6
#define MPS_FIRST_VEC           0x20
#define MPS_TIMER_VEC           (MPS_LEVELS*MPS_INTS_PER_LEVEL + MPS_FIRST_VEC)
#define MPS_XINT_VEC            MPS_TIMER_VEC+16
#define MPS_SPUR_VEC            0xff

#define MPS_NUM_IRQS		16
/*
 * Misc.
 */
#define NULL            0

/*
 * I/O port addresses for accessing the Interrupt Mode Configuration Register.
 */
#define MPS_IMCR_ADDR       0x22
#define MPS_IMCR_DATA       0x23

#endif /* _PSM_MPS_MPS_H */
