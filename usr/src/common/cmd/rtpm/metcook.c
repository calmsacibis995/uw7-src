#ident	"@(#)rtpm:metcook.c	1.5.2.1"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <mas.h>
#include <metreg.h>
#include <sys/dl.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <sys/strlog.h>
#include <sys/ksynch.h>   /* get def for lock_t - used in dpli_ether.h */
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/socket.h>
#include <sys/stream.h>
#include <sys/protosw.h>
#include <netinet/in.h>
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netdb.h>
#include <sys/sockio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/times.h>
#include <sys/cpqw.h>

#include "rtpm.h"
#include "mtbl.h"
/*
 * timing information
 */
/*
 * the requested sleep/alarm interval
 */
extern int interval;
/*
 * the current lbolt time
 */
long currtime;
/*
 * previous timestamp
 */
long oldtime;
/* 
 * flag the first data collection
 */
long first_time = 1;
/*
 *  a struct tms to pass to times()
 */
struct tms tbuf;
/*
 * time stamps for freemem, freeswap, and eisa bus calcs, since they are 
 * summed every second, they need to have a one-sec increment in between 
 * samples.  Otherwise we take a divide check.
 */
long freememtime;
long freeswptime;
long eisabustime;
/*
 * time difference in seconds between the current and previous samples
 */
float tdiff;
/*
 * total ticks accumulated by all cpus during interval, may not be
 * the same as tdiff * ncpu, since a cpu may go on/off line.  Used
 * for calculating %usr, %sys, %wio and %idl for system as a whole.
 */
float tot_tix;
/*
 * the current position in the metric buffers
 */
extern int curcook;
/*
 * metric descriptor ret from mas_open
 */
extern int md;
/*
 * memory and swap stats
 */
extern size_t totalmem;			/* physical memory */
extern uint_t mem_swappg;		/* virtual swap memory	*/
extern ulong_t dsk_swappg;		/* swap disk	*/
extern ulong_t dsk_swappgfree;		/* swap disk	*/
/*
 * eisa bus stats - not available on all systems
 */
extern eisa_bus_util_t eisa_bus_util;
extern eisa_bus_util_flg;
extern dl_t eisa_bus_util_sum;
/*
 * lwp statistics
 */
extern struct lwpstat lwp_count;
/*
 * ethernet stats
 */
extern struct net_total net_total;
/*
 * number of ethernet devices
 */
extern int nether_devs;
/*
 * names of ethernet devices
 */
extern char *ether_nm[];
/*
 * ethernet stats
 */
extern DL_mib_t *etherstat[];
/*
 * tcp, udp, ip, icmp stats
 */
extern struct tcpstat  tcpstat;
extern struct udpstat  udpstat;
extern struct ipstat   ipstat;
extern struct icmpstat icmpstat;

extern struct metric mettbl[];		/* metric table */
/*
 * the number of metrics in mettbl
 */
extern int nmets;
/*
 * max spx connections to date - to limit work cooking metrics
 */
extern int spx_mused_conn;

void cook_met( struct metric *mp );

/*
 *	function: 	calc_intv
 *
 *	args:		pointer to an entry in mettbl
 *			pointer to a metval
 *			the size of the object to convert
 *
 *	ret val:	none
 *
 *	convert values into uint32 via run time size binding and then
 *	scale the result according to the cooking method specified in 
 *	mettbl.  PARANOID covers up a bug in a particular vendor's 
 *	hardware where the metric page is a stale one from several 
 *	samples ago, which may result in very large rates.
 */
void
calc_intv( struct metric *metp, struct metval *mp, uint32 sz )
{
	caddr_t obj_p = mp->met_p;
	uint32 itmp;
	dl_t dtmp;
	dl_t answer;
	uint32 action;

	assert( mp );
	assert( metp );

	if( !obj_p ) { /* instance not registered */
		mp->met.dbl.dl_lop = 0;
		mp->met.dbl.dl_hop = 0;
		mp->intv = 0.0;
		mp->cooked[ curcook ] = 0.0;
		return;
	}
	assert( tdiff > 0.0 );
	action = metp->action;
/*
 *	a switch statement won't work here 
 *	(sizeof(int) may be the same as the sizeof(long))
 */
	if( sz == sizeof(int) ) {
		/* LINTED pointer alignment */
		itmp = ( (uint32)*((uint32 *)obj_p) );
		mp->intv = (float)(itmp - mp->met.sngl);
#ifdef PARANOID
		if((action==RATE || action==MEAN ) && itmp < mp->met.sngl)
			mp->intv = 0.0;
		else
#endif
		mp->met.sngl = itmp;
	} else if( sz == sizeof(short) ) {
		/* LINTED pointer alignment */
		itmp = ( (uint32)*((short *)obj_p) );
		mp->intv = (float)(itmp - mp->met.sngl);
#ifdef PARANOID
		if((action==RATE || action==MEAN ) && itmp < mp->met.sngl)
			mp->intv = 0.0;
		else
#endif
		mp->met.sngl = itmp;
	} else if( sz == sizeof(long) ) {
		/* LINTED pointer alignment */
		itmp = ( (uint32)*((long *)obj_p) );
		mp->intv = (float)(itmp - mp->met.sngl);
#ifdef PARANOID
		if((action==RATE || action==MEAN ) && itmp < mp->met.sngl)
			mp->intv = 0.0;
		else
#endif
		mp->met.sngl = itmp;
	} else if( sz == sizeof(char) ) {
		itmp = ( ((uint32)*obj_p)&0xff );
		mp->intv = (float)(itmp - mp->met.sngl);
#ifdef PARANOID
		if((action==RATE || action==MEAN ) && itmp < mp->met.sngl)
			mp->intv = 0.0;
		else
#endif
		mp->met.sngl = itmp;
	} else if( sz == sizeof( dl_t ) ) {
		/* LINTED pointer cast may result in improper alignment */
		dtmp.dl_lop = ( ((dl_t *)obj_p)->dl_lop );
		/* LINTED pointer cast may result in improper alignment */
		dtmp.dl_hop = ( ((dl_t *)obj_p)->dl_hop );
		answer = lsub( dtmp, mp->met.dbl );
		mp->intv = (float)(answer.dl_lop);
		if(answer.dl_hop) 
			mp->intv += 4294967296.0*(float)(answer.dl_hop);
#ifdef PARANOID
		if( (action==RATE || action==MEAN ) 
		  && ( (mp->met.dbl.dl_hop > dtmp.dl_hop)
		  || ( (mp->met.dbl.dl_hop == dtmp.dl_hop) 
		  && (mp->met.dbl.dl_lop > dtmp.dl_lop) ) ) )
			mp->intv = 0.0;
		else {
#endif
		mp->met.dbl.dl_lop = dtmp.dl_lop;
		mp->met.dbl.dl_hop = dtmp.dl_hop;
#ifdef PARANOID
		}
#endif
	}
#ifdef DEBUG
	else {
/*
 *		don't have to call endwin here, since if this is going 
 *		to fail, it will do so on the first call
 */
		(void)fprintf(stderr,"DEBUG unsupported object size in calc_intv\n");
		exit(1);
	}
#endif
	switch( action ) {
	case NONE:
	case INSTANT:
		mp->cooked[ curcook ] = (float)(int)mp->met.sngl*mp->scaleval;
		break;
	case RATE:
		mp->cooked[ curcook ] = mp->scaleval * mp->intv / tdiff;
		break;
	case MEAN:
		mp->cooked[ curcook ] = mp->scaleval * mp->intv / tdiff;
/*
 *		Sometimes there's a little slew in the clocks on MP
 * 		systems.  The local clock handler can be delayed during 
 *		one sample interval, which looks like there's an extra 
 *		tick in the following interval.  The result is a 
 *		percentage slightly higher than 100%.  This throws off
 *		the plots scales and the bargraph, so put an 1176 on it
 *		to ensure it doesn't pin the meters...
 *
 *		If we're calculating a percentage, make sure it 
 *		does not exceed 100%.
 */
		if( mp->cooked[ curcook ] > 100.0 )
			mp->cooked[ curcook ] = 100.0;
		break;
	}
}

/*
 *	function: 	calc_interval_data
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	cook all of the metrics in mettbl, handling some special cases
 */
void
calc_interval_data( void ) {
	int i, j;
	int nsamples;
	float usr_tix, sys_tix, wio_tix, idl_tix;
	float tmp, tmp2;
	static float lastfreememcook = 0.0;
	static float lastfreeswpcook = 0.0;
	static float lasteisabuscook = 0.0;


	tdiff = (float)( currtime - oldtime ) / (float)hz;
	oldtime = currtime;

/*
 *	These have to done first, since some metrics are based on the 
 *	total number of cpu tix.  These are special cased since one or 
 *	more cpus may be offline,  which throws the total percentage 
 *	calcs off proportionally.
 */
	cook_met( &mettbl[USR_TIME_IDX] );
	cook_met( &mettbl[SYS_TIME_IDX] );
	cook_met( &mettbl[WIO_TIME_IDX] );
	cook_met( &mettbl[IDL_TIME_IDX] );
/*
 *	sum up the total cpu tix
 */
	usr_tix = sys_tix = wio_tix = idl_tix = tot_tix = 0.0;
	for( i = 0; i < ncpu ; i++ ) {
		usr_tix += mettbl[USR_TIME_IDX].metval[i].intv;
		sys_tix += mettbl[SYS_TIME_IDX].metval[i].intv;
		idl_tix += mettbl[IDL_TIME_IDX].metval[i].intv;
		wio_tix += mettbl[WIO_TIME_IDX].metval[i].intv;
	}
	tot_tix = usr_tix + sys_tix + wio_tix + idl_tix;
/*
 *	Cook the user, sys, wio, and idl percentages.
 *	Because there may be some variation on when the local
 *	clock handler is called on mp systems, limit these
 *	to 100%.  This is done mainly because the plotting
 *	functions will autoscale and we don't want the scale
 *	to get bumped up because a value came in at 101%.
 */
	mettbl[USR_TIME_IDX].metval[ncpu].cooked[curcook] = 
		min( 100.0, 100.0 * (float)usr_tix/tot_tix );
	mettbl[SYS_TIME_IDX].metval[ncpu].cooked[curcook] = 
		min( 100.0, 100.0 * (float)sys_tix/tot_tix );
	mettbl[IDL_TIME_IDX].metval[ncpu].cooked[curcook] = 
		min( 100.0, 100.0 * (float)idl_tix/tot_tix );
	mettbl[WIO_TIME_IDX].metval[ncpu].cooked[curcook] = 
		min(100.0, 100.0 * (float)wio_tix/tot_tix );

	mettbl[SPX_max_connections_IDX].metval->cooked[curcook] =
		mettbl[SPX_max_connections_IDX].metval->met.sngl;

/*
 *	main loop for cooking everything.  Starts at METSTART,
 *	since things before METSTART are resources that are
 *	constant values, so there's no sense burning cpu cycles
 *	to cook them.
 */
	for( i = METSTART; i < nmets; i++ ) {
		switch( mettbl[i].id ) {

#ifdef FREEMEM_EVERY_TICK
		case FREEMEM:
/*
 *		in pre-sbird/q5 systems freemem is 
 *		summed every clock tick.  cook_met will divide it by 
 *		the time in seconds, we just need to divide it by hz
 *		to get the denominator to be the number of samples.
 */
			cook_met( &mettbl[i] );
			mettbl[i].metval->cooked[ curcook ] =
			  mettbl[i].metval->cooked[ curcook ] / hz;
			break;
#endif
/*
 *		freemem and freeswap need some attention.  cook_met will 
 *		divide them by the time in seconds, but we want to divide 
 *		by the integer number of seconds since we last did the 
 *		calculation.  This corresponds to the sample count, 
 * 		since they are summed every second.  However, we have 
 *		to be careful not to divide by zero if the sample rate 
 *		(tdiff) is less than 1 second.  To protect against this,
 *		the time of the last sample is kept in freememtime and
 *		freeswptime.
 */
		case FREESWAP: 
			nsamples = (currtime/hz) - (freeswptime/hz);
			if( nsamples >= 1 ) {
				cook_met( &mettbl[i] );
				mettbl[i].metval->cooked[ curcook ] =
				  mettbl[i].metval->intv/(float)nsamples;
				freeswptime = currtime;
				lastfreeswpcook = mettbl[i].metval->cooked[curcook];
			}
			else mettbl[i].metval->cooked[curcook] = lastfreeswpcook;
		break;

		case FREEMEM: 
			nsamples = (currtime/hz) - (freememtime/hz);
			if( nsamples >= 1 ) {
				cook_met( &mettbl[i] );
				mettbl[i].metval->cooked[ curcook ] =
				  mettbl[i].metval->intv/(float)nsamples;
				freememtime = currtime;
				lastfreememcook = mettbl[i].metval->cooked[curcook];



			}
			else mettbl[i].metval->cooked[curcook] = lastfreememcook;
			break;
/*
 *		calculate the percentage of eisa bus that is in use
 */
		case EISA_BUS_UTIL_PERCENT:
/*
 *			The eisa_bus_util structure contains three
 *			elements: interval, period, and count
 *			These are reset with the eisa bus idl count
 *			every interval.  The interval is in ticks
 *			and is usually 1 second.  There is a second 
 *			eisa bus idl count, eisa_bus_util_sum, which
 *			is a double long, that is the sum of the idl 
 *			counts.  eisa_bus_util_sum is updated every 
 *			interval.  The count of the number of times
 *			eisa_bus_util_sum has been updated is in
 *			eisa_bus_util_sumcnt.  The interval is in tix,
 *			while period is in ns.  To determine the bus 
 *			idle time:
 *
 *			sum_difference*period/(sumcnt*interval)
 */
			if( eisa_bus_util_flg ) {
				nsamples = mettbl[EISA_BUS_UTIL_SUMCNT_IDX]
				  .metval->intv;
				if( nsamples >= 1 ) {
					double t;
					double c;
					double b;

					b = (double)eisa_bus_util.idletime_hz;
					t = nsamples*eisa_bus_util.interval_hz
					  *(1000000000.0/hz);
					cook_met( &mettbl[i] );
					c = mettbl[i].metval->intv;
					 lasteisabuscook = 
					 mettbl[i].metval->cooked[curcook]=
					  (1.0 - (c/t)*b) * 100.0;
					eisabustime = currtime;
				}
				else mettbl[i].metval->cooked[curcook] =
				  lasteisabuscook;

			}
			break;
/*
 *		response time and active time are kept in usec.
 *		The disk statistics calculated are:
 *
 *			Average disk wait time in ms:
 *			  ((resp_diff-active_diff)/(total disk ops))/1000.0
 *
 *			Average disk service time in ms:
 *			  (active_diff/(total disk ops))/1000.0
 *
 *			Percent of time disk was busy:
 *			  100.0% * active_diff/(1000000.0*tdiff)
 *
 *			Average queue length:
 *			  response_diff / active_diff
 */
		case DS_ACTIVE:
			assert(mettbl[i].reslist[0] == NDISK);
			cook_met( &mettbl[i] );
			mettbl[i].metval[ndisk].cooked[ curcook ] = 0.0;
			for( j = 0; j < ndisk; j++ ) {
				mettbl[i].metval[j].cooked[ curcook ] = 
				  mettbl[i].metval[j].intv/(tdiff*10000.0);
				mettbl[i].metval[ndisk].cooked[ curcook ] += 
				  mettbl[i].metval[j].cooked[ curcook ];
			}
			mettbl[i].metval[ndisk].cooked[ curcook ] /= (float)ndisk;
			break;

		case DS_RESP:
			assert(mettbl[i].reslist[0] == NDISK);
			cook_met( &mettbl[i] );
			mettbl[i].metval[ndisk].cooked[ curcook ] = 0.0;
			for( j = 0; j < ndisk; j++ ) {
				if( mettbl[DS_ACTIVE_IDX].metval[j].intv
				  > 0.0 )
					mettbl[i].metval[j].cooked[ curcook ] = 
					  mettbl[i].metval[j].intv/
					  mettbl[DS_ACTIVE_IDX].metval[j].intv;
				else 
					mettbl[i].metval[j].cooked[ curcook ] = 0.0;
				mettbl[i].metval[ndisk].cooked[ curcook ] += 
				  mettbl[i].metval[j].cooked[ curcook ];
			}
			mettbl[i].metval[ndisk].cooked[ curcook ] /= (float)ndisk;
			break;
/*
 *		Cook_met works fine if all processors are active.
 *		However, if a processor is offline, the total percentage 
 *		falls off accordingly.  This was handled above, but we
 *		have to make sure we don't re-cook these.
 */
		case MPC_CPU_USR:
		case MPC_CPU_SYS:
		case MPC_CPU_WIO:
		case MPC_CPU_IDLE:
			break;
/*
 *		Total cpu is a calculated from usr+sys time, 
 *		set limit to 100.0%
 */
		case TOT_CPU:
			for( j = 0; j <= ncpu ; j++ )
				mettbl[i].metval[j].cooked[ curcook ] = 
				min( mettbl[USR_TIME_IDX].metval[j].cooked[ curcook ] 
				   + mettbl[SYS_TIME_IDX].metval[j].cooked[ curcook ], 100.0 );
			break;
/*
 *		Total idle is a calculated from wio+idl time, 
 *		set limit to 100.0%
 */
		case TOT_IDL:
			for( j = 0; j <= ncpu ; j++ )
				mettbl[i].metval[j].cooked[ curcook ] = 
				min( mettbl[IDL_TIME_IDX].metval[j].cooked[ curcook ] 
				   + mettbl[WIO_TIME_IDX].metval[j].cooked[ curcook ], 100.0 ) ;
			break;
/*
 *		Calculate total reads+writes
 */
		case TOT_RW:
			for( j = 0; j <= ncpu ; j++ )
				mettbl[i].metval[j].cooked[ curcook ] = 
				mettbl[READ_IDX].metval[j].cooked[ curcook ] 
				   + mettbl[WRITE_IDX].metval[j].cooked[ curcook ] ;
			break;
/*
 *		Calculate total Kbytes read+written
 */
		case TOT_KRWCH:
			for( j = 0; j <= ncpu ; j++ )
				mettbl[i].metval[j].cooked[ curcook ] = 
				 (float)(mettbl[READCH_IDX].metval[j].cooked[ curcook ] 
				   + mettbl[WRITECH_IDX].metval[j].cooked[ curcook ] ) / 1024.0 ;
			break;
/*
 *		Calculate the percentage of directory name lookup cache 
 *		hits.  If there was no dnlc activity, set the percentage
 *		to 100%.
 */
		case DNLC_PERCENT:
			for( j = 0; j <= ncpu ; j++ ) {
				tmp = mettbl[DNLCHITS_IDX].metval[j].cooked[ curcook ] 
				   + mettbl[DNLCMISS_IDX].metval[j].cooked[ curcook ];
				if( tmp < 0.5 )
					mettbl[i].metval[j].cooked[ curcook ] = 100.0;
				else
					mettbl[i].metval[j].cooked[ curcook ] = 
					  100.0 * mettbl[DNLCHITS_IDX].metval[j].cooked[ curcook ] / tmp;
			}
			break;
/*
 *		Calculate the read cache hit ratio
 *		If there was no activity, set the percentage to 100%.
 */
		case RCACHE_PERCENT:
			for( j = 0; j <= ncpu ; j++ ) {
				tmp = mettbl[MPB_BREAD_IDX].metval[j]
				  .cooked[ curcook ];
				if( tmp < 0.5 )
					mettbl[i].metval[j].cooked[ curcook ] = 100.0;
				else {
					tmp2 = mettbl[MPB_LREAD_IDX]
					  .metval[j].cooked[ curcook ];
					mettbl[i].metval[j].cooked[curcook] = 
					  100.0 * (tmp2-tmp) / tmp2;
					if( mettbl[i].metval[j]
					  .cooked[ curcook ] > 100.0 )
						mettbl[i].metval[j].cooked
						  [ curcook ] = 100.0;
				}
			}
			break;
/*
 *		Calculate the write cache hit ratio
 *		If there was no activity, set the percentage to 100%.
 */
		case WCACHE_PERCENT:
			for( j = 0; j <= ncpu ; j++ ) {
				tmp = mettbl[MPB_BWRITE_IDX].metval[j]
				  .cooked[ curcook ];
				if( tmp < 0.5 )
					mettbl[i].metval[j].cooked[ curcook ] = 100.0;
				else {
					tmp2 = mettbl[MPB_LWRITE_IDX]
					  .metval[j].cooked[ curcook ];
					mettbl[i].metval[j].cooked[curcook] = 
					  100.0 * (tmp2-tmp) / tmp2;
					if( mettbl[i].metval[j]
					  .cooked[ curcook ] > 100.0 )
						mettbl[i].metval[j].cooked
						  [ curcook ] = 100.0;
				}
			}
			break;
/*
 *		Calculate the total number pages in use by
 *		the Kernel Memory Allocator by totaling up 
 *		the pool sizes and the ovsz allocation and 
 *		dividing by the page size.
 */
		case TOT_KMA_PAGES:
			mettbl[i].metval->cooked[curcook] =
				mettbl[ KMEM_MEM_IDX ].
				  metval[(ncpu+1)*(nkmpool+1)-1].
				  cooked[curcook]/pgsz + 
				  mettbl[ KMEM_BALLOC_IDX ].
				  metval[(ncpu)*(nkmpool+1)+nkmpool-1].
				  cooked[curcook]/pgsz;
			break;
/*
 *		calculate the percentage of memory that is in use
 */
		case MEM_PERCENT:
			{
			float mempg = (float) totalmem / pgsz;
			mettbl[i].metval->cooked[curcook] =
			  100.0*(mempg - mettbl[FREEMEM_IDX].metval
			  ->cooked[ curcook ])/mempg;
			}
			break;
/*
 *		calculate the percentage of swap memory that is in use
 */
		case MEM_SWAP_PERCENT:
			mettbl[i].metval->cooked[curcook] =
			  100.0*(mem_swappg - mettbl[FREESWAP_IDX]
			  .metval->cooked[curcook])/(float)mem_swappg;
			break;
/*
 *		calculate the percentage of swap disk that is in use
 */
		case DSK_SWAP_PERCENT:
			mettbl[i].metval->cooked[curcook] =
			  100.0*(dsk_swappg - dsk_swappgfree)/dsk_swappg;
			break;
/*
 *		this one can get into negative numbers in short range
 */
		case SPXCON_connection_id:
			for( j = 0 ; j < maxspxconn; j++ )
				mettbl[i].metval[j].cooked[curcook] =
				  /* LINTED pointer cast may result in improper alignment */
				  *(unsigned short *)mettbl[i].metval[j].met_p;
			break;


/*
 *		This is an easy one, just call cook_met
 */
		default:
			cook_met( &mettbl[i] );
		}
	}
}
/*
 *	function: 	cook_met
 *
 *	args:		pointer to an entry in mettbl
 *
 *	ret val:	none
 *
 *	Calculate interval data for each instance of a metric and cook.
 *	For metrics that have more than one instance, calculate totals
 *	in the last element of the instance array.  For two dimensional
 *	metrics (eg. based on two resources) calculate totals for both
 *	the rows and columns.
 */
void
cook_met( struct metric *mp ) {
	int i,j;
	struct metval *metp;
	int dimx;	/* size in x dimension */
	int dimy;	/* size in y dimension */
	int xflg = 0;	/* doing a mean on the rows */
	int yflg = 0;	/* doing a mean on the cols */
	int zflg = 0;	/* cut corners on cooking per minor spx mets */
	int kludge;

	assert( tdiff > 0.0 );	/* catch divide check it happens	*/
	assert( mp );		/* make sure we have a metric...	*/
	assert( mp->title );	/* and a title...			*/
	assert( mp->metval );	/* and a value to cook.			*/
	if( mp->action == MEAN )/* make a note if we're doing a mean	*/
		xflg = yflg = 1;
	if( mp->reslist[0] == SPX_max_connections )
		zflg = 1;
	metp = mp->metval;
	dimx = mp->resval[0]+1;	/* x/y dims are resource vals + one	*/
	dimy = mp->resval[1]+1; /* extra slot for the totals		*/

	switch( mp->ndim ) {
	case 2:
		metp[ subscript(dimx-1, dimy-1) ].cooked[ curcook ] = 0.0;
		for( i = 0; i < dimx-1; i++ ) {
			metp[ subscript( i, dimy-1 ) ].cooked[ curcook ] = 0.0;
			for( j = 0; j < dimy-1; j++ ) {
				if( !metp[ subscript( i, j ) ].met_p )
					continue;
				calc_intv( mp, &metp[ subscript( i, j ) ],
				  mp->objsz );
				metp[ subscript(i, dimy-1) ].cooked[ curcook ]
				  += metp[ subscript(i, j) ].cooked[ curcook ];

			}
			metp[ subscript(dimx-1, dimy-1) ].cooked[ curcook ] += 
			  metp[ subscript(i, dimy-1) ].cooked[ curcook ];
			if( xflg )
				metp[ subscript(i,dimy-1) ].cooked[curcook] /= 
				  (float)(dimx-1);
		}
		for( j = 0; j < (dimy-1) ; j++ ) {
			metp[ subscript( dimx-1, j) ].cooked[ curcook ] = 0.0;
			for( i = 0; i < dimx-1; i++ ) {
				metp[ subscript(dimx-1,j) ].cooked[curcook] += 
				  metp[ subscript( i, j ) ].cooked[ curcook ]; 
			}
			if( yflg )
				metp[ subscript(dimx-1,j) ].cooked[ curcook ] 
				  /= (dimy-1);
		}
		if( yflg )
			metp[ subscript(dimx-1,dimy-1) ].cooked[ curcook ] 
			  /= (dimy-1);
		if( xflg )
			metp[ subscript(dimx-1,dimy-1) ].cooked[ curcook ] 
			  /= (dimx-1);

		break;
	case 1:
		metp[ dimx-1 ].cooked[ curcook ] = 0.0;

		if( zflg )
			kludge = spx_mused_conn;
		else
			kludge = dimx-1;
		for( i = 0; i < kludge; i++ ) {
			if( !metp[ i ].met_p )
				continue;
			calc_intv( mp, &metp[ i ], mp->objsz);
			metp[ dimx-1 ].cooked[ curcook ] 
			  += metp[i].cooked[ curcook ];
		}
		if( xflg ) {
			metp[ dimx-1 ].cooked[ curcook ] /= (float)(dimx-1);
		}
		break;
	case 0: 
		if( !metp[ 0 ].met_p )
			break;
		calc_intv( mp, &metp[ 0 ], mp->objsz );
		break;
	default:
#ifdef DEBUG
/*
 *		don't have to call endwin here, since if this is going 
 *		to fail, it will do so on the first call
 */
		fprintf(stderr,"DEBUG unsupported resource count in mettbl\n");
		exit(1);
#else
		break;
#endif
	}
}

/*
 *	function: 	snap_mets
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Snap_mets takes a snapshot of the raw system metric data.
 *	The times system call is invoked to get the system time
 *	in ticks, and mas_snap is called to copy the memory mapped
 *	metric data to the mas snapshot buffer.  Then calc_interval_data
 *	is called to cook the raw metric.  During the interval calcs,
 *	the metric is copied to an "old" value for use in the next 
 *	iteration.
 */
void
snap_mets( void ) {
	int i;
	int retry = 0;

	do{
/*
 *		need to let clock tick, otherwise, calc_interval_data
 *		divides by tdiff of zero.  It's unlikely anything
 *		changed much in less than 1 tick :-)
 */
		do {
			currtime = times(&tbuf);
		} while ( currtime == oldtime );
/*
 *		copy the metrics
 */
		mas_snap( md );
/*
 *	check to see if a clock tick occurred while copying the metrics
 *	if so, try again.
 */
	} while( (currtime != times(&tbuf)) && (retry++ < 10)) ;

	if( first_time ) {
		first_time = 0;
/*
 *		this is the first time snap_mets was called.
 *		save initial time into freememtime and freeswptime.
 *		the freemem and freeswp pages counters are incremented
 *		once per second ( when currtime % hz == 0 ).
 *		We will only update the cooked metric when
 *		the counter has been updated, otherwise, we
 *		may end up trying to divide by 0.
 */
		freememtime = currtime;
		freeswptime = currtime;
		eisabustime = currtime;
		for( i = 0 ; i < nmets ; i++ ) {
			if( !mas_id_cmp( &mettbl[i].id, FREEMEM ) 
			  || !mas_id_cmp( &mettbl[i].id, FREESWAP ) 
			  || !mas_id_cmp( &mettbl[i].id, EISA_BUS_UTIL_PERCENT ) ) {
				mettbl[i].metval->met.dbl.dl_lop = 
				  /* LINTED pointer cast may result in improper alignment */
				  ((dl_t *)(mettbl[i].metval->met_p))->dl_lop;
				mettbl[i].metval->met.dbl.dl_hop = 
				  /* LINTED pointer cast may result in improper alignment */
				  ((dl_t *)(mettbl[i].metval->met_p))->dl_hop;
			}
		}
	}


/*
 *	get statistics for the following:
 */
	read_proc();		/* processes and lwps			*/
	ether_stat();		/* raw data from ethernet cards		*/
	net_stat();		/* tcp/ip/udp/icmp network stats	*/
	netware_stat();		/* spx/ipx/sap/rip network stats	*/
	get_mem_and_swap();	/* kernel mem and swap stats that 	*/
				/* are not yet in MAS			*/
}
/*
 * set initial value for previous time stamp to current time - 1 second
 */
void
set_oldtime( void ) {
	oldtime = times(&tbuf) - 100;
}
