#ifndef _I_87FP_H_
#define _I_87FP_H_

#ident	"@(#)debugger:inc/i386/i_87fp.h	1.5"

/* 64-bit significand, 16-bit exponent */
/* floating-point registers saved in the 387 "temporary real"
 * format of 10 bytes for an extended precision floating point
 * value, rather than the memory format of 12 bytes;
 * we use the floating-point emulation package format and the
 * debugger routines to convert to/from the two formats
 */

#ifndef EXTENDED_SIZE
#define EXTENDED_SIZE	10
#endif

typedef struct {
    unsigned char ary[EXTENDED_SIZE];
} fp_x_t;

/* Save state for both the extended word 80287 and 80387 */
struct fpstate_t {
	unsigned int fp_control;
	unsigned int fp_status;
	unsigned int fp_tag;
	unsigned int fp_ip;
	unsigned short fp_cs;
	unsigned short fp_ds;
	unsigned long fp_data_addr;
	unsigned long fp_unknown;
	fp_x_t fp_stack[8];
	short new_fp_status;
	short fp_padding; /* for even longword alignment */
};

#endif
