#ident	"@(#)ccsdemos:thr_demos/sync/demo_cond.c	1.1"
/* 
 * To compile, cc -lthread -ldl demo_cond.c -o demo_cond
 * To invoke: demo_cond [-b Boundflag] [-t Threads] [-l LWPs]
 *    defaults: unbind; Threads = 5; LWPs = 5;
 */
/*
 * This demo program illustrates the use of mutex and condition variable
 * The parent thread creates a number of child threads and waits for all
 * of them to block.  It then wakes up all of them at once using broadcast.
 * Each child thread will go to sleep again and when all of them are blocked,
 * the parent thread will wake up one child at a time till all children are
 * waken up.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <thread.h>
#include <synch.h>

#define MaxThreadsNumber 10

thread_t Child[MaxThreadsNumber];
thread_t Parent;
cond_t	condbarrier; 	/* condition variable for barrier */
cond_t  _condbarrier; 	/* condition variable for the parent thread */
cond_t  cbarrier;     	/* condition variable for barrier */
mutex_t	lockbarrier; 	/* lock for variable */
int countbarrier = 0; 	/* Variable that counts threads that have reached barrier. */
int parentcount = 1;
int count1barrier = 0;
int ChildNumber = 5;
int LWPsNumber = 5;
int boundflag = 0;

void
BarrierSync() {
	thread_t me;

	mutex_lock(&lockbarrier);
	me = thr_self();
	countbarrier += 1;
	printf("child thread %d is blocking\n", (int)me);
	sleep(1);
	/* if I'm the last child thread to block, wake up the parent thread */
	if (countbarrier == ChildNumber) {
		parentcount = 0;
		cond_signal(&_condbarrier);
	}
	/* blocking */
	while(countbarrier != 0)
		cond_wait(&condbarrier, &lockbarrier);
	printf("child thread %d is waking up and ready to go to sleep again\n", 
		(int)me);
	mutex_unlock(&lockbarrier);
}
void
_BarrierSync() {
	thread_t me;

	mutex_lock(&lockbarrier);
	me = thr_self();
	count1barrier += 1;
	/* wake up parent */
	if (count1barrier == ChildNumber) {
		parentcount = 0;
		cond_signal(&_condbarrier);
	}
	while (count1barrier != 0)
		cond_wait(&cbarrier, &lockbarrier);
	printf("child thread %d is awakened a second time\n", (int)me);
	parentcount = 0;
	/* wake up parent */
	cond_signal(&_condbarrier);
	mutex_unlock(&lockbarrier);
}

void *   
ChildStart(void *arg)
{

	BarrierSync();
	_BarrierSync();
	thr_exit(NULL);
}

void *
ParentStart(void *arg)
{
	int i, error;
	int lwpcreated = 0;

	if (boundflag == 1)
		LWPsNumber = ChildNumber;

	for (i = 0; i < ChildNumber; i++) {
	   if (boundflag == 1) {
		error = thr_create(NULL, 0, ChildStart, (void *)0,
				   THR_BOUND, &Child[i]);
           } else if (lwpcreated < LWPsNumber) {
		error = thr_create(NULL, 0, ChildStart, (void *)0,
				   THR_NEW_LWP, &Child[i]);
		lwpcreated++;
	   } else {
		error = thr_create(NULL, 0, ChildStart, (void *)0,
				   0, &Child[i]);
	   }
	   if (error)
		    printf("thr_create of child thread failed - returned %d\n",
			    error);
       	}
	mutex_lock(&lockbarrier);	

	/* 
	 * wait for all child threads to block, the last child thread
 	 * to block will set the parentcount to 0 and send a signal to
	 * the parent thread. We loop here to make sure that the signal
	 * received was sent by the last child thread
 	 */
	while (parentcount)
		cond_wait(&_condbarrier, &lockbarrier);	
        countbarrier = 0;
   	printf("waking up all child threads\n");
	sleep(1);
	parentcount = 1;
	/* waking up all children at once */
	cond_broadcast(&condbarrier);
	mutex_unlock(&lockbarrier);
 
	mutex_lock(&lockbarrier);
	/* wait for child threads to block again */
	while (parentcount)
		cond_wait(&_condbarrier, &lockbarrier);

	/* waking up one child thread at a time */
	for(i = 0; i < ChildNumber; i++) {
		sleep(1);
		printf("waking up one child thread\n");
		count1barrier = 0;
		parentcount  = 1;
		cond_signal(&cbarrier);
		while (parentcount)
			cond_wait(&_condbarrier, &lockbarrier);
    	}
	printf("Parent done\n");
	mutex_unlock(&lockbarrier);	
	thr_exit(NULL);
}

 
main(argc, argv)
int argc;
char * argv[];
{
	int i, error;

        while((argc > 1) && (argv[1][0] == '-')) {
                switch(argv[1][1]) {
                        case 'b':
                                boundflag = 1;
                                break;
                        case 't':
                                if (argc >= 3) {
                                        argc--;
                                        argv++;
                                        ChildNumber = atoi(argv[1]);
                                        if (ChildNumber > MaxThreadsNumber) {
                                           ChildNumber = MaxThreadsNumber;
                                           printf("Threads Number too big, set to the maximum number, which is %d\n", MaxThreadsNumber);
                                        }
                                }
                                break;
                        case 'l':
                                if (argc >= 3) {
                                        argc--;
                                        argv++;
                                        LWPsNumber = atoi(argv[1]);
                                }
                                break;
                        default:
                                fprintf(stderr, "invalid option: %s\n",argv[1]);
                                exit(1);
                }
                argc--;
                argv++;
        }

	/* Initialize barrier */
	mutex_init(&lockbarrier, USYNC_THREAD, (void *) NULL);
	cond_init(&condbarrier, USYNC_THREAD, (void *) NULL);
	cond_init(&_condbarrier, USYNC_THREAD, (void *)NULL);
	cond_init(&cbarrier, USYNC_THREAD, (void *)NULL);
	/* Create parent thread */
	error = thr_create(NULL, 0, ParentStart, (void *) 0,
			   THR_NEW_LWP, &Parent);
	if (error)
	     printf("thr_create of parent thread failed - returned %d...FAILED\n", error);
	for (i = 0; i <= ChildNumber; i++)
		thr_join((thread_t) 0, NULL, NULL);
}
