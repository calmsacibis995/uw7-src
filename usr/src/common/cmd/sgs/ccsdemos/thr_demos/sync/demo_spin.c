#ident	"@(#)ccsdemos:thr_demos/sync/demo_spin.c	1.1"
/*
 * To compile, cc -lthread -ldl demo_spin.c -o demo_spin
 * To invoke: demo_spin [-b Boundflag] [-t ThreadsNumber]
 * not recommend to run on a uniprocessor
 */
/*
 * This program illustrates the use of spin lock.
 * The output shows how spin lock serialize the execution of threads
 * On a time sharing system, the number of LWPs should be greater 
 * than the number of threads spinning to avoid dead lock.  
 * Otherwise, the number of processor should be greater
 * than the number of threads spinning.
 * Spin lock should only be used when the critical section is short.
 */


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <thread.h>
#include <synch.h>


#define MaxThreadsNumber 10

thread_t t[MaxThreadsNumber];
spin_t lock;
int counter = 0;
int ThreadsNumber = 5;
barrier_t barrier;

void *   
ThreadsStart(void *arg)
{
	int me, i;

	me = thr_self();
	printf("Thread %d is trying to get the lock\n", me);
	barrier_wait(&barrier);
	_spin_lock(&lock);
	printf("Thread %d gets the lock\n", me);
	for (i = 1; i < ThreadsNumber+1; i++)
		counter += i;
	printf("Thread %d is releasing the lock\n", me);
	_spin_unlock(&lock);
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
                                        ThreadsNumber = atoi(argv[1]);
                                        if (ThreadsNumber > MaxThreadsNumber) {
                                           ThreadsNumber = MaxThreadsNumber;
                                           printf("Threads Number too big, set to the maximum number, which is %d\n", MaxThreadsNumber);
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
	
	_spin_init(&lock, NULL);
	barrier_init(&barrier, ThreadsNumber, USYNC_THREAD, NULL);

	for (i = 0; i < ThreadsNumber; i++) {
	   if (boundflag == 1) 
		error = thr_create(NULL, 0, ThreadsStart, (void *) 0,
				THR_BOUND, &t[i]);
	   else
		error = thr_create(NULL, 0, ThreadsStart, (void *) 0,
			   	THR_NEW_LWP, &t[i]);
	   if (error)
	     		printf("thr_create of parent thread failed - returned %d...FAILED\n", error);
	}
	thr_exit(NULL);
}
