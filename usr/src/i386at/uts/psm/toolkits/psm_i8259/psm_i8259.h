#ident	"@(#)kern-i386at:psm/toolkits/psm_i8259/psm_i8259.h	1.1.2.1"
#ident  "$Header$"

#ifndef _PSM_PSM_I8259_H	/* wrapper symbol for kernel use */
#define _PSM_PSM_I8259_H	/* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_PSM)
#if _PSM != 2
#error "Unsupported PSM version"
#endif
#else
#error "Header file not valid for Core OS"
#endif

#ifndef __PSM_SYMREF

struct __PSM_dummy_st {
	int *__dummy1__;
	const struct __PSM_dummy_st *__dummy2__;
};
#define __PSM_SYMREF(symbol) \
	extern int symbol; \
	static const struct __PSM_dummy_st __dummy_PSM = \
		{ &symbol, &__dummy_PSM }

#if !defined(_PSM)
__PSM_SYMREF(No_PSM_version_defined);
#pragma weak No_PSM_version_defined
#else
#define __PSM_ver(x) _PSM_##x
#define _PSM_ver(x) __PSM_ver(x)
__PSM_SYMREF(_PSM_ver(_PSM));
#endif

#endif  /* __PSM_SYMREF */

#include <svc/psm.h>



/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *     This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */

/*
 * The number of interrupt controllers in an EISA system.
 */

#define I8259_MAX_ICS  		2

/*
 * The number of IRQ lines per 8259 controller
 */

#define I8259_NIRQ             	8       	

/*
 * Vector definitions for external interrupts
 */
#define I8259_VBASE    		0x20
#define I8259_ALLVECS 		(256-I8259_VBASE)

/*
 * Definitions for 8259 Priority Interrupt Controller ports on AT 386 
 */

#define I8259_IC_1     		0x20
#define I8259_IC_2     		0xA0

/*
 * Registers to set interrupts edge or level sensitive.  Edge
 * sensitive is zero, level sensitive is one.  Set 0
 * applies to interrupts 0-7, set 1 interrupts 8-15.
 */

#define I8259_IRQ_SET_0_EDGE_LEVEL       0x04D0
#define I8259_IRQ_SET_1_EDGE_LEVEL       0x04D1
#define I8259_IRQ0	0
#define I8259_IRQ1	1
#define I8259_IRQ2	2
#define I8259_IRQ8	8
#define I8259_IRQ13	13

/*
 * Offsets for the Initialization Control Words.  Note that
 * there is a lot of overlap and the data written and the
 * order in which it is written determines where it goes.
 */

#define I8259_ICW1              0x00
#define I8259_ICW2              0x01
#define I8259_ICW3              0x01
#define I8259_ICW4              0x01

/*
 * Offsets for the Operation Control Words.
 */

#define I8259_OCW1              0x01
#define I8259_OCW3              0x00

/*
 * EOI (end-of-interrupt) command sent to the 8259 interrupt controller.
 */

#define I8259_EOI               0x20

/*
 * Contents of ICW/OCWs, used for initialization
 */

#define I8259_MASTER_ICW1       0x11            /* Edge triggered, icw4 */
#define I8259_MASTER_ICW2       I8259_VBASE
#define I8259_MASTER_ICW3       0x04            /* Master - slave on 2 */
#define I8259_MASTER_ICW4       0x01            /* Master 8086 mode */
#define I8259_MASTER_OCW1       0xFF            /* mask     */
#define I8259_MASTER_OCW3       0x0B            /* read ISR */
#define I8259_SLAVE_ICW1        0x11            /* Edge triggered, icw4 */
#define I8259_SLAVE_ICW2        (I8259_VBASE+8)
#define I8259_SLAVE_ICW3        0x02            /* Slave - slave on 2 */
#define I8259_SLAVE_ICW4        0x01            /* Slave 8086 mode */
#define I8259_SLAVE_OCW1        0xFF            /* mask     */
#define I8259_SLAVE_OCW3        0x0B            /* read ISR */

#define I8259_READIRR     	0x0A            /* read IRR */
#define I8259_READISR     	0x0B            /* read ISR */


/*
 * Function prototypes for toolkit procedures.
 */
void            i8259_init(unsigned int);
void            i8259_intr_mask(ms_islot_t);
void            i8259_intr_unmask(ms_islot_t);
void            i8259_intr_attach(ms_intr_dist_t *, unsigned int);
ms_bool_t       i8259_check_spurious(ms_ivec_t);
void            i8259_eoi(ms_islot_t);
void            i8259_deinit(void);


#if defined(__cplusplus)
	}
#endif

#endif /* _PSM_PSM_I8259_H */
