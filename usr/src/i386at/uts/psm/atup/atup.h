#ifndef _PSM_ATUP_ATUP_H        /* wrapper symbol for kernel use */
#define _PSM_ATUP_ATUP_H        /* subject to change without notice */

#ident	"@(#)kern-i386at:psm/atup/atup.h	1.4"
#ident	"$Header$" 

#if defined(__cplusplus)
extern "C" {
#endif

#include <psm/toolkits/psm_i8259/psm_i8259.h>

#define AT_MAX_SLOT		((I8259_MAX_ICS*I8259_NIRQ)-1)

#define AT_TIMER_SLOT		0
#define AT_CASCADE_SLOT         2

#if defined(__cplusplus)
       }
#endif
#endif /* _PSM_ATUP_ATUP_H */
