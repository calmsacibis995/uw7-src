#ident	"@(#)timer.c	1.5"

#include <assert.h>
#include <synch.h>
#include <thread.h>
#include <sys/time.h>
#include <signal.h>

#include "ppp_type.h"
#include "psm.h"

#define TIMERDEBUG

struct time_o_s {
	struct time_o_s *to_next;	/* Next event in list */
	struct time_o_s *to_prev;	/* Prev event in list */
	struct timeval 	to_due;		/* Time event is due */
	int		to_flags;	/* Flags */
	int 		(*to_fn)();	/* Function to call */
	caddr_t		to_arg1;	/* Argument to pass to to_fn */
	caddr_t		to_arg2;	/* Argument to pass to to_fn */
	void		*to_id;		/* ID - used to cancel the timeout */
};


#define TOF_NONBLOCK	0x0001	/* Event doesn't block .. or take long */
	
/*
 * Initialise globals 
 */
thread_t timer_tid;
static struct time_o_s *time_o_h_p;
static struct time_o_s *time_o_t_p;
static mutex_t time_o_mutex;

timer_init()
{
	mutex_init(&time_o_mutex,USYNC_PROCESS, NULL);
	time_o_h_p = NULL;
	time_o_t_p = NULL;
}

/*
 * Schedule a timeout event.
 *
 * tim is the number of 100ths of a second until the event should be called.
 */
STATIC void *
timeout_cmn(int tim, int (*func)(), int flags, caddr_t arg1, caddr_t arg2)
{
	struct time_o_s *new, *listp;
	struct timeval due;
	int sec, usec;

	ASSERT(tim >= 10);

	gettimeofday(&due, NULL);

	sec = tim / 100;
	usec = (tim % 100) * 10000;
	due.tv_usec += usec;
	due.tv_sec += sec + (due.tv_usec / 1000000);
	due.tv_usec %= 1000000;

	/*
	 * Allocate and fill time_o_s
	 */
	new = (struct time_o_s *)malloc(sizeof(struct time_o_s));
	if (!new) {
		ppplog(MSG_ERROR, 0, "timeout: Out of memory\n");
		return(NULL);
	}

	new->to_next = NULL;
	new->to_prev = NULL;
	new->to_due = due;
	new->to_fn = func;
	new->to_arg1 = arg1;
	new->to_arg2 = arg2;
	new->to_id = (void *)new;
	new->to_flags = flags;

	/* Lock the timeout event list */

	MUTEX_LOCK(&time_o_mutex);

	/* Find the place this should go in the list */

	listp = time_o_h_p;
	while (listp) {
		if (listp->to_due.tv_sec > due.tv_sec)
			break;

		if (listp->to_due.tv_sec == due.tv_sec &&
		    listp->to_due.tv_usec > due.tv_usec)
			break;

		listp = listp->to_next;
	}

	/* Here's where */

	if (!listp) {
		/* At tail */
		new->to_prev = time_o_t_p;

		if (time_o_t_p)
			time_o_t_p->to_next = new;

		time_o_t_p = new;

		/* Check if at head */
		if (!time_o_h_p)
			time_o_h_p = new;
	} else {
		/* Insert in the list just before the listp element*/

		new->to_prev = listp->to_prev;
		new->to_next = listp;

		if (listp->to_prev)
			listp->to_prev->to_next = new;
		else
			time_o_h_p = new;

		listp->to_prev = new;
	}

	/*
	 * Signal the timer thread to indicate that we
	 * have modified the list
	 */
	thr_kill(timer_tid, SIGALRM);
	MUTEX_UNLOCK(&time_o_mutex);
#ifdef TIMEREVENTDEBUG
	ppplog(MSG_DEBUG, 0, "timeout: id %d, time due %d\n", new->to_id, due);
#endif
	return(new->to_id);
}


/*
 * The function specified by 'func' will be called with 'arg1' and 'arg2'
 * as arguments after 'tim' 100ths of a second have elapsed.
 *
 * 'func' may not sleep/block.
 */
void *
timeout(int tim, int (*func)(), caddr_t arg1, caddr_t arg2)
{
	return timeout_cmn(tim, func, TOF_NONBLOCK, arg1, arg2);
}

/*
 * The function specified by 'func' will be called with 'arg1' and 'arg2'
 * as arguments after 'tim' 100ths of a second have elapsed.
 *
 * 'func' may sleep/block, it will be executed by its' own 
 * LWP associted.
 */
void *
timeout_l(int tim, int (*func)(), caddr_t arg1, caddr_t arg2)
{
	return timeout_cmn(tim, func, 0, arg1, arg2);
}


/*
 * Cancel a previous timeout event
 */
void
untimeout(void *id)
{
	struct time_o_s *listp;

	/*
	 * No need to signal timer thread ... it will findout about our changes
	 * soon enough
	 */
#ifdef TIMEREVENTDEBUG
	ppplog(MSG_DEBUG, 0, "untimeout: id %d\n", id);
#endif
	MUTEX_LOCK(&time_o_mutex);

	listp = time_o_h_p;

	while (listp && listp->to_id != id)
		listp = listp->to_next;

	if (listp) {
		
		/* Remove this item from the callout list */

		if (listp->to_next) 
			listp->to_next->to_prev = listp->to_prev;
		else
			time_o_t_p = listp->to_prev;

		if (listp->to_prev)
			listp->to_prev->to_next = listp->to_next;
		else
			time_o_h_p = listp->to_next;
		free(listp);

#ifdef TIMERDEBUG
		/* Check the lists integrity */
		listp = time_o_h_p;
		while (listp)
			listp = listp->to_next;

		listp = time_o_t_p;
		while (listp)
			listp = listp->to_prev;
#endif
	}
	MUTEX_UNLOCK(&time_o_mutex);
}

/*
 * When events occur, if they will take a significant time to execute,
 * then a new thread is created to service the event.
 */
static void *
callout(void *argp)
{
	struct time_o_s *time_o_p = (struct time_o_s *)argp;

	(time_o_p->to_fn)(time_o_p->to_arg1, time_o_p->to_arg2);
	free(time_o_p);
	thr_exit(NULL);
}

void *
timer_thread(void *argp)
{
	struct timeval now, *tv_p;
	struct time_o_s *listp;
	struct itimerval interval;
	sigset_t sig_mask = *(sigset_t*)argp;
	int ret;

	interval.it_interval.tv_sec = 0;
	interval.it_interval.tv_usec = 0;
	tv_p = &interval.it_value;

	for (;;) {

		/* Check if any events are due */

		MUTEX_LOCK(&time_o_mutex);

#ifdef TIMERDEBUG
		/* Check the lists integrity */
		listp = time_o_h_p;
		while (listp)
			listp = listp->to_next;

		listp = time_o_t_p;
		while (listp)
			listp = listp->to_prev;

#endif
		gettimeofday(&now, NULL);

		/* Search the list for times less than the current */

		if (time_o_h_p &&
		    !(time_o_h_p->to_due.tv_sec > now.tv_sec) &&
		    !(time_o_h_p->to_due.tv_sec == now.tv_sec &&
		      time_o_h_p->to_due.tv_usec > now.tv_usec)) {

			/* 
			 * Okay, the time this ievent is due is less than
			 * or equal to the current time ... so call it
			 */
			listp = time_o_h_p;

			if (listp->to_next) 
				listp->to_next->to_prev = listp->to_prev;
			else
				time_o_t_p = listp->to_prev;

			if (listp->to_prev)
				listp->to_prev->to_next = listp->to_next;
			else
				time_o_h_p = listp->to_next;
#ifdef TIMERDEBUG
			{
				struct time_o_s *listp;
				/* Check the lists integrity */
				listp = time_o_h_p;
				while (listp)
					listp = listp->to_next;

				listp = time_o_t_p;
				while (listp)
					listp = listp->to_prev;
			}
#endif

			MUTEX_UNLOCK(&time_o_mutex);

#ifdef TIMEREVENTDEBUG
			ppplog(MSG_DEBUG, 0, "timer_thread: id %d - start\n", listp->to_id);
#endif

			if (listp->to_flags & TOF_NONBLOCK) {
				(*listp->to_fn)(listp->to_arg1,
						listp->to_arg2);
				free(listp);
			} else
				thr_create(NULL, 0, callout, (void *)listp,
					   THR_DETACHED | THR_BOUND, NULL);
#ifdef TIMEREVENTDEBUG
			ppplog(MSG_DEBUG, 0, "timer_thread: id %d - done\n", listp->to_id);
#endif
			continue;
		}


		/* Calculate time until next event is due */

		if (time_o_h_p) {

			tv_p->tv_sec =
				(time_o_h_p->to_due.tv_sec - now.tv_sec);

			tv_p->tv_usec =
				(time_o_h_p->to_due.tv_usec - now.tv_usec);

			if (tv_p->tv_usec < 0) {
				tv_p->tv_usec += 1000000;
				tv_p->tv_sec--;
			}

			/* Set timer */

			setitimer(ITIMER_REAL, &interval, NULL);
		}

		MUTEX_UNLOCK(&time_o_mutex);

		/* Wait for signal */
		if ((ret = sigwait(&sig_mask)) < 0) {
			ppplog(MSG_FATAL, 0, 
			       "timer_thread: sigwait failure\n");
		}

		switch (ret) {
		case SIGALRM:
			break;
		case SIGTERM:
			ppplog(MSG_INFO, 0, "Received SIGTERM. Exiting.\n");
			exit(1);
		default:
			ppplog(MSG_WARN, 0, "Received signal %d - ignored\n",
			       ret);
			break;
		}
	}
}
