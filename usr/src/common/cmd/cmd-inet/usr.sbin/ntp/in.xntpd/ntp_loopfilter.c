#ident "@(#)ntp_loopfilter.c	1.2"

/* wjm 17-aug-1995: Computations here should be aware of `sys_poll' being
 *		reset by NTP_PROTO occasionally. As a preliminary fix,
 *		make sure that such an event at least does not cause 
 *		an unwarranted update to `drift_comp' (==> "#if 1" below).
 *
 *		Optionally, #ifdef VMS_DEBUG, print(!) some debugging data,
 *		independent of the `debug' setting.
 *
 * wjm 15-aug-1995: #ifdef VMS_FIX1, increment `sys_poll' more slowly,
 *		in order to re-gain some of xntp3.3's tracking stability.
 *
 *		(Actually I don't see a reason why this fix should be
 *		 limited to VMS, but anyway it's _required_ in my environment.)
 *
 *		The fix assumes a CLOCK_LIMIT of 30, as e.g. in xntp3.4v.
 *
 *		It seems that `sys_poll' should not increment before
 *		(many?) more cycles than a clock filter length (i.e. 8)
 *		have passed.
 *
 *		Decrement of `sys_poll' may be slower than expected,
 *		because on a run-away condition due to bad `drift_comp',
 *		the dispersion (=> `smax') tends to grow as well.
 *
 */
#ifdef VMS	/*wjm*/
#define VMS_FIX1 1	/* at least with VMS NTP clients, this is REQUIRED */
#endif		/*wjm*/

/*
 * ntp_loopfilter.c - implements the NTP loop filter algorithm
 */
#include <stdio.h>
#include <ctype.h>
#ifndef SYS_WINNT
#include <sys/time.h>
#endif /* SYS_WINNT */
#include <signal.h>
#include <errno.h>

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_unixtime.h"
#include "ntp_stdlib.h"

#if defined(VMS) && defined(VMS_LOCALUNIT)	/*wjm*/
#include "ntp_refclock.h"
#endif

#ifdef SYSCALL_BUG
#include <nlist.h>

struct nlist nl[2];
#endif /* SYSCALL_BUG */

#ifdef KERNEL_PLL
#include <sys/timex.h>
#ifndef NTP_SYSCALLS_LIBC
#define ntp_gettime(t)  syscall(SYS_ntp_gettime, (t))
#define ntp_adjtime(t)  syscall(SYS_ntp_adjtime, (t))
#endif /* NTP_SYSCALLS_LIBC */
#endif /* KERNEL_PLL */

/*
 * The loop filter is implemented in slavish adherence to the
 * specification (Section 5), except that for consistency we
 * mostly carry the quantities in the same units as appendix G.
 *
 * Note that the long values below are the fractional portion of
 * a long fixed-point value.  This limits these values to +-0.5
 * seconds.  When adjustments are capped inside this range (see
 * CLOCK_MAX_{I,F}) both the clock_adjust and the compliance
 * registers should be fine. (When the compliance is above 16, it
 * will at most accumulate 2 ** CLOCK_MULT times the maximum offset,
 * which means it fits in a s_fp.)
 *
 * The skew compensation is a special case. In version 2, it was
 * kept in ms / 4s (i.e., CLOCK_FREQ was 10). In version 3 (Section 5)
 * it seems to be 2 ** -16ms / 4s in a s_fp for a maximum of +-125ppm
 * (stated maximum 100ppm). Since this seems about to change to a
 * larger range, it will be kept in units of 2 ** -20 (CLOCK_DSCALE)
 * in an s_fp (mainly because that's nearly the same as parts per
 * million). Note that this is ``seconds per second'', whereas a
 * clock adjustment is a 32-bit fraction of a second to be applied
 * every 2 ** CLOCK_ADJ seconds; to find it, shift the drift right by
 * (CLOCK_DSCALE - 16 - CLOCK_ADJ). When updating the drift, on the
 * other hand, the CLOCK_FREQ factor from the spec assumes the value to
 * be in ``seconds per 4 seconds''; to get our units, CLOCK_ADJ must be
 * added to the shift.
 *
 * Kernel PLL/PPS state machine
 *
 * The following state machine is used when the kernel PLL modifications
 * described in the README.kernel file are present.
 *
 * Each update to a prefer peer sets pps_update true if it survives the
 * intersection algorithm and its time is within range. The PPS time
 * discipline is enabled (STA_PPSTIME bit set in the status word) when
 * pps_update is true and the PPS frequency discipline is enabled. If
 * the PPS time discipline is enabled and the kernel reports a PPS
 * signal is present, the pps_control variable is set to the current
 * time. If the current time is later than pps_control by PPS_MAXAGE
 * (120 s), this variable is set to zero.
 *
 * The pll_enable switch can be set both at configuration time and at
 * run time using xntpdc. If true, the kernel modifications are active
 * as described above; if false, the kernel is bypassed entirely (except
 * for the PPS frequency update, if enabled) and the daemon PLL used
 * instead. 
 */
#define RSH_DRIFT_TO_FRAC (CLOCK_DSCALE - 16)
#define RSH_DRIFT_TO_ADJ (RSH_DRIFT_TO_FRAC - CLOCK_ADJ)
#define RSH_FRAC_TO_FREQ (CLOCK_FREQ + CLOCK_ADJ - RSH_DRIFT_TO_FRAC)
#define PPS_MAXAGE 120		/* kernel pps signal timeout (s) */

/*
 * Program variables
 */
	l_fp last_offset;	/* last clock offset */
	u_long last_time;	/* time of last clock update (s) */
	u_fp clock_stability;	/* clock stability (ppm) */
 	s_fp clock_frequency;	/* clock frequency error (ppm) */
	s_fp drift_comp;	/* pll frequency (ppm) */
static	long clock_adjust;	/* clock adjust (fraction only) */
static	s_fp max_comp;		/* max frequency offset (ppm) */
	int tc_counter;		/* poll-adjust counter */
	int pll_status;		/* status bits for kernel pll */
	int pll_control;	/* true if working kernel pll */
	int pll_enable;		/* true if pll enabled */
	u_long pps_control;	/* last pps sample time */
	int pps_update;		/* pps update valid */
	int fdpps = -1;		/* pps file descriptor */
#if defined(KERNEL_PLL) && defined(PPS)
	int pps_enable = 1;	/* pps enabled */
#else
	int pps_enable;		/* pps disabled */
#endif /* KERNEL_PLL PPS */

/*
 * Imported from the ntp_proto module
 */
extern s_fp sys_rootdelay;	/* root delay */
extern u_fp sys_rootdispersion;	/* root dispersion */
extern struct peer *sys_peer;	/* system peer pointer */
extern u_char sys_poll;		/* log2 of system poll interval */
extern u_char sys_leap;		/* system leap bits */
extern l_fp sys_refskew;	/* accumulated skew since last update */
extern u_fp sys_maxd;		/* max dispersion of survivor list */

/*
 * Imported from ntp_io.c
 */
extern struct interface *loopback_interface;

/*
 * Imported from ntpd module
 */
extern int debug;		/* global debug flag */
extern int correct_any;
/*
 * Imported from timer module
 */
extern u_long current_time;	/* like it says, in seconds */

/*
 * Imported from leap module
 */
extern u_char leapbits;		/* sanitized leap bits */

#if defined(KERNEL_PLL)
#define MOD_BITS (MOD_OFFSET | MOD_MAXERROR | MOD_ESTERROR | \
    MOD_STATUS | MOD_TIMECONST)
#ifndef NTP_SYSCALLS_LIBC
#ifndef SYS_NETBSD
extern int syscall	P((int, void *, ...));
#endif /* SYS_NETBSD */
#endif /* NTP_SYSCALLS_LIBC */
void pll_trap		P((int));

static struct sigaction sigsys;	/* current sigaction status */
static struct sigaction newsigsys;	/* new sigaction status */
#endif /* KERNEL_PLL */

#if defined(GDT_SURVEYING)
extern long sys_clock;		/* imported from ntp_proto */

extern l_fp gdt_rsadj;		/* running sum of adjustments to time */
#endif /* GDT_SURVEYING */

/*
 * init_loopfilter - initialize loop filter data
 */
void
init_loopfilter()
{
	extern u_long tsf_maxslew;
	u_long tsf_limit;

	/*
	 * Limit for drift_comp, minimum of two values. The first is to
	 * avoid signed overflow, the second to keep within 75% of the
	 * maximum adjustment possible in adj_systime().
	 */
	max_comp = 0x7fff0000;
	tsf_limit = ((tsf_maxslew >> 1) + (tsf_maxslew >> 2));
	if ((max_comp >> RSH_DRIFT_TO_ADJ) > tsf_limit)
		max_comp = tsf_limit << RSH_DRIFT_TO_ADJ;

	/*
	 * Reset clockworks
	 */
	drift_comp = 0;
	clock_adjust = 0;
	tc_counter = 0;
	sys_poll = NTP_MINPOLL;

	L_CLR(&last_offset);
	last_time = 0;
	clock_frequency = 0;
	clock_stability = 0;
	pps_update = pps_control = 0;
}

/*
 * local_clock - the NTP logical clock loop filter.  Returns 1 if the
 *	clock was stepped, 0 if it was slewed and -1 if it is hopeless.
 */
int
local_clock(fp_offset, peer, fastset)
	l_fp *fp_offset;		/* best offset estimate */
	struct peer *peer;		/* from peer - for messages */
	int fastset;                    /* from ntp_proto - just left unsynch
					   state */
{
	long offset;
	long tmp;
	int return_code;
	l_fp ftmp;
	s_fp stmp;
	u_fp smax;
	long allan;
	long interval;
#if defined(KERNEL_PLL)
	struct timex ntv;
#endif /* KERNEL_PLL */

	/*
	 * Initialize estimated measurement error and Allan variance
	 * intercept point. The measurement error is assumed the sum of
	 * the peer dispersion plus select dispersion, which seems
	 * reasonable. The Allan variance intercept point is assumed
	 * at MAXSEC for reference clocks and twice that for peer
	 * clocks, which seems cowardly.
	 */
	if (peer->refclktype)
		allan = CLOCK_MAXSEC;
	else
		allan = CLOCK_MAXSEC << 1;

	if (!last_time)
		last_time = current_time;
	interval = (long)(current_time - last_time);
	clock_adjust = 0;
	offset = fp_offset->l_f;
	smax = peer->dispersion + peer->selectdisp;
	return_code = 0;

#ifdef DEBUG
	if (debug > 1)
		printf(
	   "local_clock: offset %s peer %s watch %ld)\n",
		    lfptoa(fp_offset, 6), ntoa(&peer->srcadr),
		    interval);
#endif

	/*
	 * If the clock is way off, don't tempt fate by correcting it.
	 */
	ftmp = *fp_offset;
	if (L_ISNEG(&ftmp))
		L_NEG(&ftmp);
	if ((ftmp.l_ui >= CLOCK_WAYTOOBIG) && (!correct_any)) {
		syslog(LOG_ERR,
		"time error %s is way too large (set clock manually)",
		    lfptoa(fp_offset, 6));
#ifndef BIGTIMESTEP
		return (-1);
#endif /* BIGTIMESTEP */

	/*
	 * If the magnitude of the offset is greater than CLOCK.MAX
	 * (128 ms), reset the time and frequency. We are quite
	 * aggresive here, since the intrinsic clock oscillator
	 * frequency error can be quite large, sometimes over +-300 ppm.
	 * With something this large and a noisy peer, the casual time
	 * updates wander right through the acceptable range, causing
	 * this section to trigger.
	 */
	} else if (ftmp.l_ui > CLOCK_MAX_I || (ftmp.l_ui == CLOCK_MAX_I
	    && ftmp.l_uf >= CLOCK_MAX_F)) {
		tc_counter = 0;
		sys_poll = peer->minpoll;

		/*
		 * Either we are not in synchronization, or we have gone
		 * CLOCK_MINSTEP (900 s) since the last acceptable
		 * update. We step the clock and leave the frequency
		 * alone. Since the clock filter has been reset, the
		 * dispersions will be high upon recovery and the quick-
		 * march code below will trigger to keep the clock in
		 * bounds.
		 */
		if (fastset || interval >
		    CLOCK_MINSTEP) {
			step_systime(fp_offset);
			NLOG(NLOG_SYNCEVENT|NLOG_SYSEVENT) /* conditional if clause for conditional syslog */
			  syslog(LOG_NOTICE,

		    "time reset (%s) %s s",
#ifdef SLEWALWAYS
			    "slew",
#else
			    "step",
#endif
			    lfptoa(fp_offset, 6));
			return_code = 1;

		/*
		 * The local clock is out of range, but we haven't
		 * allowed enough time for the peer (usually a radio
		 * clock) to recover after a leap second. Pretend we wuz
		 * never here.
		 */
		} else {
			return (return_code);
                }

	/*
	 * This code segment works when the clock-adjustment code is
	 * implemented in the kernel, which at present is only in the
	 * (modified) SunOS 4.1, Ultrix 4.3 and OSF/1 kernels. In the
	 * case of the DECstation 5000/240 and Alpha AXP, additional
	 * kernel modifications provide a true microsecond clock. We
	 * know the scaling of the frequency variable (s_fp) is the same
	 * as the kernel variable (1 << SHIFT_USEC = 16).
	 */
#if defined(KERNEL_PLL)
	} else if (pll_control && pll_enable) {
		l_fp pps_offset;
		u_fp pps_dispersion;

		/*
		 * We initialize the structure for the ntp_adjtime()
		 * system call. We have to convert everything to
		 * microseconds first. Afterwards, remember the
		 * frequency offset for the drift file.
		 */
		memset((char *)&ntv,  0, sizeof ntv);
		ntv.modes = MOD_BITS;
		if (offset >= 0) {
			TSFTOTVU(offset, ntv.offset);
		} else {
			TSFTOTVU(-offset, ntv.offset);
			ntv.offset = -ntv.offset;
		}
		ntv.esterror = sys_rootdispersion << 4;
		ntv.maxerror = ntv.esterror + (sys_rootdelay << 2);
		ntv.constant = min(peer->ppoll, sys_poll) - NTP_MINPOLL;
		ntv.status = STA_PLL;
		if (pps_enable)
			ntv.status |= STA_PPSFREQ;
		if (pps_update && pps_enable)
			ntv.status |= STA_PPSTIME;

		/*
		 * Set the leap bits in the status word.
		 */
		if (sys_leap & LEAP_ADDSECOND &&
		     sys_leap & LEAP_DELSECOND)
			ntv.status |= STA_UNSYNC;
		else if (sys_leap & LEAP_ADDSECOND)
			ntv.status |= STA_INS;
		else if (sys_leap & LEAP_DELSECOND)
			ntv.status |= STA_DEL;

		/*
		 * This astonishingly intricate wonder juggles the
		 * status bits so that the kernel loop behaves as the
		 * daemon loop; viz., selects the FLL when necessary,
		 * etc. See the comments following the #endif for
		 * explanation.
		 */
		if (smax > CLOCK_MAX_FP)
			ntv.status |= STA_FLL | STA_FREQHOLD;
		else if (smax > sys_maxd << 3)
			return (0);
		else if (interval >= allan)
			ntv.status |= STA_FLL;
		if (ntp_adjtime(&ntv) == TIME_ERROR)
			if (ntv.status != pll_status)
				syslog(LOG_ERR, "kernel pll status change %x",
				    ntv.status);
		drift_comp = ntv.freq;
		pll_status = ntv.status;

		/*
		 * If the kernel pps discipline is working, monitor its
		 * performance.
		 */
		if (pll_status & STA_PPSTIME && pll_status &
		    STA_PPSSIGNAL && ntv.shift) {
			if (ntv.offset >= 0)
				TVUTOTSF(ntv.offset, offset);
			else {
				TVUTOTSF(-ntv.offset, offset);
				offset = -offset;
			}
			L_CLR(&pps_offset);
			L_ADDF(&pps_offset, offset);
			TVUTOTSF(ntv.jitter, tmp);
			pps_dispersion = (tmp >> 16) & 0xffff;
			if (!pps_control)
				NLOG(NLOG_SYSEVENT) /* conditional syslog */
				    syslog(LOG_INFO,
				    "pps sync enabled");
			pps_control = current_time;
			record_peer_stats(&loopback_interface->sin,
			    ctlsysstatus(), fp_offset, 0,
			    pps_dispersion);
		}
#endif /* KERNEL_PLL */

	/*
	 * If the dispersion exceeds 128 ms, we need to quick-march it
	 * to nominal zero offset and wait for the next update. This is
	 * necessary when the intrinsic frequency error is large and the
	 * clock has drifted during the interval the clock filter was
	 * stabilizing. Note that, if unsynchronized, the dispersion is
	 * always greater than 128 ms, so we don't need a check for
	 * that.
	 */
	} else if (smax > CLOCK_MAX_FP) {
		clock_adjust = offset;

	/*
	 * If the dispersion has increased substantially over the
	 * previous value, we have a spike which probably should be
	 * suppressed. A factor of eight has been found reasonable by
	 * simulation.
	 */
	} else if (smax > sys_maxd << 3) {
		return (0);

 	/*
	 * If the interval between corrections is less than the Allan
	 * variance intercept point, we use a phase-lock loop to compute
	 * new values of time and frequency. The bandwidth is controlled
	 * by the time constant, which is adjusted in response to the
	 * phase error and dispersion. Note the frequency is not changed
	 * if the local clock driver is in control.
	 */ 
	} else if (interval < allan) {
		int time_constant = min(peer->ppoll, sys_poll) -
		    NTP_MINPOLL;
		int ltmp = interval;

		if (offset < 0)
			clock_adjust = -(-offset >> time_constant);
		else
			clock_adjust = offset >> time_constant;
#if defined(VMS) && defined(VMS_LOCALUNIT)
		/* wjm: update `drift_comp' when synchronized to VMS_LOCALUNIT,
		 *      unless it's PREFERred (cf. the "intricate wrinkle" in
		 *	`adj_host_clock()' below)
                 */
		if (interval &&
		    !(peer->refclktype == REFCLK_LOCALCLOCK && 
		      (REFCLOCKUNIT(&peer->srcadr) != VMS_LOCALUNIT ||
		       (peer->flags & FLAG_PREFER)))) {
#else	/* VMS && VMS_LOCALUNIT */
		if (interval && !(peer->refclktype ==
		    REFCLK_LOCALCLOCK)) {
#endif	/* VMS && VMS_LOCALUNIT */
			tmp = peer->maxpoll;
			while (ltmp < (1 << peer->maxpoll)) {
				tmp--;
				ltmp <<= 1;
			}
#if 1 	/* wjm - The update to `drift_comp' comes out 
	 *	way too large if `sys_poll' was large previously,
	 *	and just has been reset by NTP_PROTO 
	 *	due to a change in sync source.
	 *	For now, act as if `interval' never was 
	 *	much longer than 1 << sys_poll.
	 *
	 * At this point, 
	 *	tmp == min(floor(log2(interval)),maxpoll)
	 */
			if(tmp > sys_poll) tmp = sys_poll;
#endif	/*wjm*/
			tmp = (RSH_FRAC_TO_FREQ - tmp) + time_constant +
			    time_constant;
			if (offset < 0)
				tmp = -(-offset >> tmp);
			else
				tmp = offset >> tmp;
			drift_comp += tmp;
		}

	/*
	 * If the interval between corrections is greater than the Allan
	 * variance intercept point, we use a hybrid frequency-lock loop
	 * to compute new values of phase and frequency. The following
	 * code is based on ideas suggested by Judah Levine of NIST and
	 * used in his "lockclock" implementation of ACTS. The magic
	 * factor of 4 in the left shift is to convert from s_fp to ppm.
	 */
	} else {
		clock_adjust = offset;
		stmp = (offset / interval) << 4;
		if (stmp < 0)
			drift_comp -= -stmp >> CLOCK_G;
		else
			drift_comp += stmp >> CLOCK_G;
	}

	/*
	 * As a sanity check, we clamp the frequency not to exceed the
	 * slew rate of the stock Unix adjtime() system call. Finally,
	 * do a little housekeeping.
	 */
	if (drift_comp > max_comp)
		drift_comp = max_comp;
	else if (drift_comp < -max_comp)
		drift_comp = -max_comp;
#if defined(VMS_DEBUG)	/*wjm*/
	printf("DBG: poll %d (%d..%d) freq %s DT %d err %s off %s",
		sys_poll,peer->minpoll,peer->maxpoll,
		fptoa(drift_comp,2),interval,fptoa(smax,6),
		lfptoa(fp_offset,6));
#endif
	if (interval > (1 << (peer->minpoll - 1))) {

		/*
		 * Determine when to adjust the poll interval. We do
		 * this regardless of what source controls the loop,
		 * since we might flap back and forth between sources.
		 */
#if defined(VMS_DEBUG) 	/*wjm*/
		printf(" tccnt %d",tc_counter);
#endif
		stmp = LFPTOFP(fp_offset);
		if (stmp < 0)
			stmp = -stmp;
		if (stmp > smax) {
			tc_counter -= (int)sys_poll << 1;
			if (tc_counter < -CLOCK_LIMIT) {
				tc_counter = -CLOCK_LIMIT;
				if (sys_poll > peer->minpoll) {
					sys_poll--;
					tc_counter = 0;
				}
			}
		} else {
#if defined(VMS_FIX1)	/* wjm: increment `tc_counter' more slowly,
			 * i.e. by 1 for sys_poll < 8, else by 2
			 */
			tc_counter += (int)max(1,sys_poll >> 2);
#else
			tc_counter += (int)sys_poll;
#endif
			if (tc_counter > CLOCK_LIMIT) {
				tc_counter = CLOCK_LIMIT;
				if (sys_poll < peer->maxpoll) {
					sys_poll++;
					tc_counter = 0;
				}
			}
		}

#if defined(VMS_DEBUG)	/*wjm*/
		printf(" => %d",tc_counter);
#endif
		/*
		 * Calculate the frequency offset and frequency
		 * stability. These are useful for performance
		 * monitoring, but do not affect the loop variables. The
		 * results are scaled as a s_fp in ppm, because we know
		 * more than we should.
		 */
		ftmp = *fp_offset;
		L_SUB(&ftmp, &last_offset);
		clock_frequency = (LFPTOFP(&ftmp) / interval) << 20;
		if (clock_frequency < -max_comp)
			clock_frequency = -max_comp;
		else if (clock_frequency > max_comp)
			clock_frequency = max_comp;
		stmp = clock_frequency;
		if (stmp < 0)
			stmp = -stmp;
		stmp -= clock_stability;
		if (stmp < 0)
			clock_stability -= -stmp >> NTP_MAXD;
		else
			clock_stability += stmp >> NTP_MAXD;
	}
	last_offset = *fp_offset;
	last_time = current_time;
#if defined(VMS_DEBUG)	/*wjm*/
	printf("\n");
#endif

#if defined(GDT_SURVEYING)
	  syslog(LOG_DEBUG,
		 "local_clock: phase %s freq %s err %s allan %ld poll %d time %lu rsadj %s peer %s %s\n",
		 mfptoa((clock_adjust < 0 ? -1 : 0), clock_adjust, 6),
		 fptoa(drift_comp, 3), fptoa(smax, 6), allan,
		 sys_poll,
		 sys_clock, lfptoa(&gdt_rsadj, 6),
		 ntoa(&peer->srcadr),
		 "SLEW");
#ifdef DEBUG
	  if (debug > 1)
	    printf(
		   "local_clock: phase %s freq %s err %s allan %ld poll %d time %lu rsadj %s\n",
		   mfptoa((clock_adjust < 0 ? -1 : 0), clock_adjust,
			  6), fptoa(drift_comp, 3), fptoa(smax, 6), allan,
		   sys_poll,
		   sys_clock, lfptoa(&gdt_rsadj, 6));
#endif /* DEBUG */
#else
#ifdef DEBUG
	if (debug > 1)
		printf(
	    "local_clock: phase %s freq %s err %s allan %ld poll %d\n",
	    	    mfptoa((clock_adjust < 0 ? -1 : 0), clock_adjust,
		    6), fptoa(drift_comp, 3), fptoa(smax, 6), allan,
		    sys_poll);
#endif /* DEBUG */
#endif /* GDT_SURVEYING */

	(void) record_loop_stats(fp_offset, drift_comp, sys_poll);
	
	/*
	 * Whew.  I've had enough.
	 */
	return (return_code);
}


/*
 * adj_host_clock - Called every 1 << CLOCK_ADJ seconds to update host
 *	 clock
 */
void
adj_host_clock()
{
	register long adjustment;
	l_fp offset;

	/*
	 * Update the dispersion since the last update. Don't allow
	 * frequency measurements over periods longer than NTP_MAXAGE
	 * (86400 s = one day).
	 */
	if (current_time - last_time > NTP_MAXAGE)
		last_time = 0;
	L_ADDUF(&sys_refskew, NTP_SKEWINC);

	/*
	 * Declare PPS kernel unsync if the pps signal has not been heard
	 * for a few minutes.
	 */
	if (pps_control && current_time - pps_control > PPS_MAXAGE) {
		if (pps_control)
			NLOG(NLOG_SYSEVENT) /* conditional if clause for conditional syslog */
			  syslog(LOG_INFO, "pps sync disabled");
		pps_control = 0;
	}

	/*
	 * If the phase-lock loop is not implemented in the kernel, we
	 * do it the hard way using incremental adjustments and the
	 * adjtime() system call.
	 */
	if (pll_control && pll_enable)
		return;
	adjustment = clock_adjust;
	if (adjustment < 0)
		adjustment = -(-adjustment >> CLOCK_PHASE);
	else
		adjustment >>= CLOCK_PHASE;

	clock_adjust -= adjustment;
	if (drift_comp < 0)
		adjustment -= -drift_comp >> RSH_DRIFT_TO_ADJ;
	else
		adjustment += drift_comp >> RSH_DRIFT_TO_ADJ;

	/*
	 * Intricate wrinkle. If the local clock driver is in use and
	 * selected for synchronization, somebody else may be tinker the
	 * adjtime() syscall. In this case we have to avoid calling
	 * adjtime(), since that may truncate the other guy's requests.
	 * That means the local clock fudge time and frequency
	 * adjustments don't work in that case. Caveat empty.
	 */
	if (sys_peer) {
		if (sys_peer->refclktype == REFCLK_LOCALCLOCK &&
		    sys_peer->flags & FLAG_PREFER)
		return;
	}
	L_CLR(&offset);
	L_ADDF(&offset, adjustment);
	adj_systime(&offset);
}


/*
 * adj_frequency - adjust local clock frequency
 */
void
adj_frequency(freq)
	s_fp freq;		/* frequency (ppm) */
{
#if defined(KERNEL_PLL)
	struct timex ntv;
#endif /* KERNEL_PLL */

	/*
	 * This routine adjusts the frequency offset. It is used by the
	 * local clock driver to adjust frequency when no external
	 * discipline source is available and by the acts driver when
	 * the interval between updates is greater than 1 <<
	 * NTP_MAXPOLL. Note that the maximum offset is limited by
	 * max_comp when the daemon pll is used, but the maximum may be
	 * different when the kernel pll is used.
	 */
	drift_comp += freq;
	if (drift_comp > max_comp)
		drift_comp = max_comp;
	else if (drift_comp < -max_comp)
		drift_comp = -max_comp;
#if defined(KERNEL_PLL)
	/*
	 * If the phase-lock code is implemented in the kernel, set the
	 * kernel frequency as well, but be sure to set drift_comp to
	 * the actual frequency.
	 */
	if (!(pll_control && pll_enable))
		return;
	memset((char *)&ntv, 0, sizeof ntv);
	ntv.modes = MOD_FREQUENCY;
	ntv.freq = freq + drift_comp;
	(void)ntp_adjtime(&ntv);
	drift_comp = ntv.freq;
#endif /* KERNEL_PLL */
}


/*
 * loop_config - configure the loop filter
 */
void
loop_config(item, lfp_value)
	int item;
	l_fp *lfp_value;
{
#if defined(KERNEL_PLL)
	struct timex ntv;
#endif /* KERNEL_PLL */

#ifdef DEBUG
	if (debug)
		printf("loop_config %d %s\n", item, lfptoa(lfp_value, 3));
#endif
	switch (item) {

		case LOOP_DRIFTCOMP:
		drift_comp = LFPTOFP(lfp_value);
		if (drift_comp > max_comp)
			drift_comp = max_comp;
		if (drift_comp < -max_comp)
			drift_comp = -max_comp;

#if defined(KERNEL_PLL)
		/*
		 * If the phase-lock code is implemented in the kernel,
		 * give the time_constant and saved frequency offset to
		 * the kernel. If not, no harm is done. We do this
		 * whether or not the use of the kernel mods is
		 * requested, in order to clear out the trash from
		 * possible prior customers.
 		 */
		memset((char *)&ntv, 0, sizeof ntv);
		pll_control = 1;
		ntv.modes = MOD_BITS | MOD_FREQUENCY;
		ntv.freq = drift_comp;
		ntv.maxerror = NTP_MAXDISPERSE;
		ntv.esterror = NTP_MAXDISPERSE;
		ntv.status = STA_PLL | STA_UNSYNC;
		ntv.constant = sys_poll - NTP_MINPOLL;
		newsigsys.sa_handler = pll_trap;
		newsigsys.sa_flags = 0;
		if ((sigaction(SIGSYS, &newsigsys, &sigsys)))
			syslog(LOG_ERR,
			    "sigaction() fails to save SIGSYS trap: %m");

#ifdef SYSCALL_BUG
		/*
		 * Solaris is broken wrt syscalls that it doesn't catch
		 * the trap properly causing it to loop indefinitely here
		 * so a hack is necessary to determine if the syscall exists
		 * in the kernel
		 */
		nl[0].n_name = "ntp_adjtime";
		nlist("/dev/ksyms", nl);
		if (nl[0].n_type == 0) {
			syslog(LOG_NOTICE,
			       "NTP user interface routines %s",
			       "not configured in this kernel.\n");
			pll_control = 0;
		}

		/* do the ntp_adjtime() only if the syscall exists */
		if (pll_control > 0)
#endif /* SYSCALL_BUG */
			
		/*
		 * Note ntp_adjtime() normally fails on the first call,
		 * since we deliberately set the clock unsynchronized.
		 */
		(void)ntp_adjtime(&ntv);
		if ((sigaction(SIGSYS, &sigsys,
		    (struct sigaction *)NULL)))
			syslog(LOG_ERR,
		    "sigaction() fails to restore SIGSYS trap: %m");
		if (pll_control)
			NLOG(NLOG_SYSEVENT) /* conditional if clause for conditional syslog */
			  syslog(LOG_NOTICE,
			    "using kernel phase-lock loop %04x",
			    ntv.status);
		else
			NLOG(NLOG_SYSEVENT) /* conditional if clause for conditional syslog */
			  syslog(LOG_NOTICE,
			    "using xntpd phase-lock loop");
#endif /* KERNEL_PLL */
		break;

		default:
		/* sigh */
		break;
	}
}


#if defined(KERNEL_PLL)
/*
 * _trap - trap processor for undefined syscalls
 *
 * This nugget is called by the kernel when the SYS_ntp_adjtime()
 * syscall bombs because the silly thing has not been implemented in
 * the kernel. In this case the phase-lock loop is emulated by
 * the stock adjtime() syscall and a lot of indelicate abuse.
 */
RETSIGTYPE
pll_trap(arg)
 int arg;
{
	pll_control = 0;
}
#endif /* KERNEL_PLL */
