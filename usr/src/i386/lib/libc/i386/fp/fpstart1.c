#ident	"@(#)libc-i386:fp/fpstart1.c	1.7"

/* establish the default settings for the floating-point state
 * for a C language program:
 *	rounding mode		-- round to nearest default by OS,
 *	exceptions enabled	-- all masked
 *	sticky bits		-- all clear by default by OS.
 *      precision control       -- double extended
 * set the variable _fp_hw according to what floating-point hardware
 * is available.
 */

#include	<sys/sysi86.h>	/* for SI86FPHW definition	*/
#include "synonyms.h"

long      			_fp_hw; /* default: bss: 0 == no hardware  */

 void
_fpstart()
{
	extern int __flt_rounds;
	extern int	sysi86(); /* avoid external refs */
	long	cw = 0;  /* coprocessor control word - used as -4(%ebp) */

#ifdef DSHLIB
	if (_fp_hw == -1){ /* _fp_hw will be -1 if not set by the kernel/rtld */
#endif
		(void)sysi86( SI86FPHW, &_fp_hw ); /* query OS for HW status*/
		_fp_hw &= 0xff;  /* mask off all but last byte */
#ifdef DSHLIB
	}
#endif
	
#ifdef DSHLIB
	__flt_rounds = 1;   /* default, round to nearest */
#endif
	
#if !defined(DSHLIB) || defined(GEMINI_ON_OSR5)
		/* In the dynamically linked case the kernel
		  * will handle setting up the floating point state
		  */

	/* At this point the hardware environment (established by UNIX) is:
	 * round to nearest, all sticky bits clear,
    	 * divide-by-zero, overflow and invalid op exceptions enabled.
	 * Precision control is set to double.
	 * We will disable all exceptions and set precision control
 	 * to double extended.
	 */
	asm("	fstcw	-4(%ebp)");
	asm("	orl	$0x33f,-4(%ebp)");
	asm("	fldcw	-4(%ebp)");

#endif	/* DSHLIB */

	return;
}
