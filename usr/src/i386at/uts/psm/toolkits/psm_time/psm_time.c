#ident	"@(#)kern-i386at:psm/toolkits/psm_time/psm_time.c	1.1.1.2"
#ident	"$Header$"

/*
 * PSM_TIME - contain a collection routnes for spin-waits & time-of-day
 *	      routines.
 *
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_time/psm_time.h>



#define SPIN_DEFAULT	256			/* correct for ~100 Mhz pentium */
#define SPIN_MIN	(SPIN_DEFAULT/20)	/* For sanity checking */
#define SPIN_MAX	(SPIN_DEFAULT*20)	/* For sanity checking */

STATIC int	psm_time_spin_calibrated=0;	/* Multiplier has been calibrated */
STATIC int	psm_time_spin_mul;	 	/* Multipliers used to convert usec to a spin
					 	 * count for the asm spin delay loop.
					 	 * Usec is converted to a spin count by:
					 	 *	count = (usec*multiplier)/16;
					 	 */
/*
 * void
 * psm_time_spin_init
 *	Initialization for time spin routines
 *
 */
void
psm_time_spin_init()
{
	psm_time_spin_mul = SPIN_DEFAULT;
	psm_time_spin_calibrated = 0;
}


/*
 * void
 * psm_time_spin_adjust(unsigned int, unsigned int)
 *	Adjust the spin delay multiplier for actual cpu speed.
 *	If multiple cpus, make sure multiplier is correct for the
 *	fastest cpu.
 *
 * NOTE: a value of zero for actual_usec is allowed and means the calibration failed.
 *
 */
void
psm_time_spin_adjust(unsigned int req_usec, unsigned int actual_usec)
{
	unsigned int	mul=0;


#ifdef DELAY_DEBUG
	os_printf("SPIN ADJUST, CPU:%d. Req:%d, Actual:%d. Omul:%d, Nmul:%d\n",
		os_this_cpu, req_usec, actual_usec, psm_time_spin_mul, mul);
#endif

	if (actual_usec)			/* dont divide by zero */
		mul = (psm_time_spin_mul*req_usec)/actual_usec;
	if (mul >SPIN_MAX || mul < SPIN_MIN) {
		os_printf ("psm_time_spin_adjust: invalid delay factor(%d %d %d)\n",
			req_usec, actual_usec, psm_time_spin_mul);
		mul = psm_time_spin_mul;
		return;
	}

	if (!psm_time_spin_calibrated || mul > psm_time_spin_mul)
		psm_time_spin_mul = mul;

	psm_time_spin_calibrated = 1;
}


/*
 * void
 * psm_asm_loop(int)
 *	Inline delay loop for spin_wait routine
 */
asm void
psm_time_asm_loop(int n)
{
%reg n; lab lp;
	movl	n,%ecx
lp:	loop	lp
%mem n; lab lp;
	movl	n,%ecx
lp:	loop	lp
} 
#pragma asm partial_optimization psm_time_asm_loop
	

/*
 * STATIC
 * psm_time_loop(int)
 *	Loop for specified number of iterations.
 *
 * Note:
 *	We want only 1 copy of the inline routine. Some architectures
 * 	have different timing depending on where an instruction
 *	lies in a cache line.
 */
STATIC void
psm_time_loop(int n)
{
	psm_time_asm_loop(n);
} 
	

/*
 * void
 * psm_time_spin(ms_time_t *)
 *      Spin for the specified amount of time.
 *
 * Calling/Exit State:
 *
 */
void
psm_time_spin(unsigned int usec)
{
	int	mul;

	mul = psm_time_spin_mul;
        while (usec > 1000000) {
                psm_time_loop((1000000/16)*mul);
		usec -= 1000000;
	}
        psm_time_loop((15+usec*mul) >> 4);
}
