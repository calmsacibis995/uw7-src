#ident	"@(#)ccsdemos:thr_demos/sync/demo_sbarrier.c	1.1"
/*
 * To compile, cc -lthread -ldl demo_sbarrier.c -o demo_sbarrier
 *
 * To invoke: demo_sbarrier [-b Boundflag] [-t Threads] 
 *    defaults: unbind; Threads = 5;
 *
 * Not recommend to run on uniprocessor machines
 */
/*
 * This is a demo program to illustrate the use of spin barrier.
 * The output shows how multiple thread are synchronized by a barrier.
 * On a time sharing system, the number of LWPs should be greater than
 * the number of threads spinning to avoid deadlock. Otherwise, 
 * the number of processors should be greater than the number
 * of threads spinning.
 * Spin barriers should only be used when all participating threads will
 * reach the barrier at approximately the same time.
 */


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <thread.h>
#include <synch.h>

#define ThreadsNumber 10

thread_t t[ThreadsNumber];
barrier_spin_t barrier;
int SpecialThreadsNumber = 5;


void *   
ThreadsStart(void *arg)
{
	int me;

	me = thr_self();
	printf("Thread %d is blocking\n", me);
	_barrier_spin(&barrier);
	printf("Thread %d passes the barrier\n", me);
}

 
main(argc, argv)
int argc;
char * argv[];
{
	int i, error;
	int boundflag = 0;	/* no binding by default */
	
	while((argc > 1) && (argv[1][0] == '-')) {
		switch(argv[1][1]) {
			case 'b':
				boundflag = 1;
				break;
			case 't':
				if (argc >= 3) {
					argc--;
					argv++;
					SpecialThreadsNumber = atoi(argv[1]);
					if (SpecialThreadsNumber > 
						ThreadsNumber) {
					   SpecialThreadsNumber =
						ThreadsNumber;
					   printf("Threads Number too big, set to the maximum number, which is %d\n", ThreadsNumber);
					}
				}
				break;
			default:
				fprintf(stderr, "invalid option: %s\n",argv[1]);
				exit(1);
		}
		argc--;
		argv++;
	}
	
	_barrier_spin_init(&barrier, SpecialThreadsNumber, NULL);

	for (i = 0; i < SpecialThreadsNumber; i++) {
 	   if (boundflag == 1) {
		error = thr_create(NULL, 0, ThreadsStart, (void *) 0,
			   	THR_BOUND, &t[i]);
	   } else
		error = thr_create(NULL, 0, ThreadsStart, (void *)0,
					THR_NEW_LWP, &t[i]);
	   if (error)
	     		printf("thr_create of parent thread failed - returned %d...FAILED\n", error);
	} /* end of threads creation */
	thr_exit(NULL);
}
