#ident	"@(#)ccsdemos:thr_demos/sync/demo_barrier.c	1.1"
/*
 * To compile, cc -lthread -ldl demo_barrier.c -o demo_barrier
 *
 * To invoke: demo_barrier [-b Boundflag] [-t Threads] [-l LWPs]
 *    defaults: unbind; Threads = 5; LWPs = 5;
 */
/*
 * This is a demo program to illustrate the use of blocking barrier.
 * The output shows how multiple thread are synchronized by a barrier.
 */


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <thread.h>
#include <synch.h>

#define ThreadsNumber 10

thread_t t[ThreadsNumber];
barrier_t barrier;
int SpecialThreadsNumber = 5;
int LWPsNumber = 5;


void *   
ThreadsStart(void *arg)
{
	int me;

	me = thr_self();
	printf("Thread %d is blocking\n", me);
	barrier_wait(&barrier);
	printf("Thread %d passes the barrier\n", me);
}

 
main(argc, argv)
int argc;
char * argv[];
{
	int i, error;
	int lwpscreated = 0;
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
	if (boundflag == 1)
		LWPsNumber = SpecialThreadsNumber;
	
	barrier_init(&barrier, SpecialThreadsNumber, USYNC_THREAD, NULL);

	for (i = 0; i < SpecialThreadsNumber; i++) {
 	   if (boundflag == 1) {
		error = thr_create(NULL, 0, ThreadsStart, (void *) 0,
			   	THR_BOUND, &t[i]);
	   } else if (lwpscreated < LWPsNumber) {
		error = thr_create(NULL, 0, ThreadsStart, (void *)0,
					THR_NEW_LWP, &t[i]);
		lwpscreated++;
	   } else {
		error = thr_create(NULL, 0, ThreadsStart, (void *)0,
					0, &t[i]);
	   }
		if (error)
	     		printf("thr_create of parent thread failed - returned %d...FAILED\n", error);
		sleep(1);
	} /* end of threads creation */
	thr_exit(NULL);
}
