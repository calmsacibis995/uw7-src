#ident	"@(#)kern-i386at:psm/toolkits/psm_mc146818/psm_mc146818.c	1.3.1.1"
#ident	"$Header$"

/* 
 * Driver for PC AT Calendar clock chip
 *	
 * PC-DOS compatibility requirements:
 *      1. must use local time, not GMT (sigh!)
 *      2. must use bcd mode, not binary
 *
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_mc146818/psm_mc146818.h>

ms_bool_t	psm_mc146818_initialized=MS_FALSE;

/*
 * void
 * psm_mc146818_init(void)
 *	Initialize real time clock.
 *
 * Calling/Exit State:
 *	none
 */

void
psm_mc146818_init()
{
	ms_lockstate_t  lstate;
	/*
	 * reinitialize the real time clock if it's not valid.
	 */

	if (psm_mc146818_initialized == MS_TRUE)
		return;
	psm_mc146818_initialized = MS_TRUE;
	outb(MC146818_ADDR, MC146818_D);
	if (!(inb(MC146818_DATA) & MC146818_VRT)) {
		outb(MC146818_ADDR, MC146818_A);
		outb(MC146818_DATA, MC146818_DIV2);	/* 32K xtal, no onts */
		outb(MC146818_ADDR, MC146818_B);
		outb(MC146818_DATA, MC146818_HM);	/* 24 HR mode, no ints */
		outb(MC146818_ADDR, MC146818_C);
		(void)inb(MC146818_DATA);		/* clear ints */
		outb(MC146818_ADDR, MC146818_D);
		(void)inb(MC146818_DATA);		/* set time valid */
	}
}

/*
 * void
 * psm_mc146818_init(void)
 *	Get the current value of "seconds" from the RTC.
 *
 * Calling/Exit State:
 *	none
 */


int
psm_mc146818_getsec()
{
	unsigned char	byte;

	if (psm_mc146818_initialized == MS_FALSE)
		psm_mc146818_init();

wait0:
	outb(MC146818_ADDR, 0);
	byte = inb(MC146818_DATA);
	if (byte == 0xff)
		goto wait0;
wait1:
	outb(MC146818_ADDR, MC146818_A);
	byte = inb(MC146818_DATA);
	if (byte & MC146818_UIP)
		goto wait1;
	outb(MC146818_ADDR, 0);
	byte = inb(MC146818_DATA);
	if (byte == 0xff)
		goto wait1;
	
	return((int) byte);
}




/*
 * ms_bool_t
 * psm_mc146818_rtodc(ms_daytime_t *)
 *	Read contents of real time clock to the specified buffer.
 *
 * Calling/Exit State:
 *	Returns -1 if clock not valid, else 0.
 *	Returns MC146818_NREG (which is 15) bytes of data in (*buf), as given
 *	in the technical reference data.  This data includes both the time
 *	and the status registers.
 */

ms_bool_t
psm_mc146818_rtodc(ms_daytime_t *dayp)
{
	unsigned char	*bufp;
	unsigned char	reg;
	unsigned int	i, cnt;

	if (psm_mc146818_initialized == MS_FALSE)
		psm_mc146818_init();

	/*
	 * ZZZ TODO KLUDGE - conversion routines not yet done for converting
	 * between rtc_t & ms_daytime_t. Kernel currently passes a rtc_t.
	 * Changes required here & in kernel to use new ms_daytime_t.
	 */

	outb(MC146818_ADDR, MC146818_D); /* check if clock valid */
	reg = inb(MC146818_DATA);
	if (!(reg & MC146818_VRT)) {
		return MS_FALSE;
	}

	cnt = 0;
checkuip:
	/*
	 * Check if the clock is in the middle of updating itself
	 * (for the next second).  During such a time, the clock data
	 * are not stable and may not be self-consistent.
	 */
	outb(MC146818_ADDR, MC146818_A); /* check if update in progress */
	reg = inb(MC146818_DATA);
	if (reg & MC146818_UIP) {
		goto tryagain;
	}

	bufp = (unsigned char *)dayp;
	for (i = 0; i < MC146818_NREG; i++) {
		outb(MC146818_ADDR, i);
		*bufp++ = inb(MC146818_DATA);
	}

	/*
	 * Check again and compare results, in case an update started
	 * while we were reading the data out.
	 */

	outb(MC146818_ADDR, MC146818_A); /* check if update in progress */
	reg = inb(MC146818_DATA);
	if (reg & MC146818_UIP) {
		goto tryagain;
	}

	bufp = (unsigned char *)dayp;
	for (i = 0; i < MC146818_NREG; i++) {
		outb(MC146818_ADDR, i);
		if (inb(MC146818_DATA) != *bufp++)
			goto tryagain;
	}


	return MS_TRUE;

tryagain:
	if (++cnt == 1000) {
		os_printf ("Real Time Clock not responding");
		return MS_FALSE;
	}
	goto checkuip;
}

/*
 * ms_bool_t
 * psm_mc146818_wtodc(ms_daytime_t *)
 *	This routine writes the contents of the given buffer to the real time
 *	clock.
 *
 * Calling/Exit State:
 *	Takes MC146818_NREGP bytes of data, which are the 10 bytes
 *	used to write the time and set the alarm.
 */

ms_bool_t
psm_mc146818_wtodc(ms_daytime_t *dayp)
{
	unsigned char	*bufp;
	unsigned char	reg;
	unsigned int	i;

	/*
	 * ZZZ TODO KLUDGE - conversion routines not yet done for converting
	 * between rtc_t & ms_daytime_t. Kernel currently passes a rtc_t.
	 * Changes required here & in kernel to use new ms_daytime_t.
	 */

	if (psm_mc146818_initialized == MS_FALSE)
		psm_mc146818_init();

	outb(MC146818_ADDR, MC146818_B);
	reg = inb(MC146818_DATA);
	outb(MC146818_ADDR, MC146818_B);
	outb(MC146818_DATA, reg | MC146818_SET); /* allow time set now */
	bufp = (unsigned char *)dayp;
	for (i = 0; i < MC146818_NREGP; i++) { /* set the time */
		outb(MC146818_ADDR, i);
		outb(MC146818_DATA, *bufp++);
	}
	outb(MC146818_ADDR, MC146818_B);
	outb(MC146818_DATA, reg & ~MC146818_SET); /* allow time update */

	return (MS_TRUE);
}

