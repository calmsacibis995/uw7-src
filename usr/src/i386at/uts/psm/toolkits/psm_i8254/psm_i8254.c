#ident	"@(#)kern-i386at:psm/toolkits/psm_i8254/psm_i8254.c	1.1.2.2"
#ident	"$Header$"


/*
 * The i8254 toolkit supports the Intel i8254 family of Programmable Interval    
 * Timers (PITs).  This toolkit includes the following routines:
 *
 *  	1) A routine to initialize the i8254 machinery. 
 *	2) A routine to get the current count value of the i8254, converted to
 *		an increasing value rather than a countdown value.
 *	3) A routine to arrange for periodic tick interrupts for a given period.
 *	4) A routine to de-initialize the i8254 machinery.
 *
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_i8254/psm_i8254.h>
#include <psm/toolkits/psm_time/psm_time.h>



/*
 * void
 * i8254_tick_init(i8254_params_t*, ms_port_t, ms_time_t)
 *	Initialize the i8254.
 *	Arrange for periodic tick interrupts with the given period.
 *	Calibrate the spin_delay loop.
 *
 * Calling/Exit State:
 *
 */

void
i8254_init(i8254_params_t *params, ms_port_t port, ms_time_t period)
{
	unsigned int	count;
	unsigned int	usec;

	params->port = port;
	params->lockp = os_mutex_alloc();
	params->clkval = ((unsigned)I8254_CLKHZ*2000)/((unsigned)4 * 1000000000/(period.mst_nsec/500));

	/*
	 * Set the PIT to squarewave mode & for the maximum period.
	 *	NOTE: counter derement by 2 for each clock. We need
	 *	to multiply to account for this.
	 */
	outb(I8254_CTL|params->port, I8254_SQUAREMODE|I8254_READMODE);
	outb(params->port, 0xff);
	outb(params->port, 0xff);

	/*
	 * Now calibrate the delay loop.
	 */
	psm_time_spin(10000);
	outb(I8254_CTL|params->port, I8254_LATCH);
	(void) inb(params->port);		/* discard status byte */
	count = 0xffff - (inb(params->port) + (inb(params->port)<<8));

	usec = (count*1000) /(2*I8254_CLKHZ/1000); /* 2 since mode 3 double counts */
	psm_time_spin_adjust(10000, usec);

	/*
	 * Now set up the counter to interrupt at the correct rate.
	 */	
	outb(I8254_CTL|params->port, I8254_SQUAREMODE|I8254_READMODE);
	outb(params->port, params->clkval&0xff);
	outb(params->port, params->clkval >> 8);
}


/*
 * unsigned int
 * i8254_get_time(i8254_params_t*)
 *	Get the current count value of the i8254, converted to an
 *	increasing value.
 *	
 * Calling/Exit State:
 *
 */
unsigned int
i8254_get_time(i8254_params_t *params)
{
	unsigned int	val, status;
	ms_lockstate_t	ls;

	ls = os_mutex_lock(params->lockp);
	outb(I8254_CTL|params->port, I8254_LATCH);
	status = inb(params->port);
	val = inb(params->port) + (inb(params->port)<<8);

	if (val > params->clkval) {
		val = params->clkval;

		/* Set mode */
		outb(I8254_CTL|params->port, I8254_SQUAREMODE|I8254_READMODE);
		outb(params->port, params->clkval&0xff);
		outb(params->port, params->clkval >> 8);
	}

	if (status&I8254_OUT_HIGH)
		val = (params->clkval - val) / 2;
	else
		val = params->clkval - (val / 2);


	os_mutex_unlock(params->lockp, ls);
	return (val);

}

/*
 * void
 * i8254_deinit(void)
 *	De-initialize the i8254.
 *
 * Calling/Exit State:
 *
 */
void
i8254_deinit(i8254_params_t* params)
{
	os_mutex_free(params->lockp);
}

