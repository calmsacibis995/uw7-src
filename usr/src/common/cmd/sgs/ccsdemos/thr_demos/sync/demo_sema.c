#ident	"@(#)ccsdemos:thr_demos/sync/demo_sema.c	1.1"
/* 
 * To compile, cc -lthread -ldl demo_sema.c -o demo_sema
 * To invoke: demo_sema [-b Boundflag] [-t Threads] [-l LWPs]
 *    defaults: unbind; Threads = 10; LWPs = 2;
 */
/*
 * This demo program illustrates the use of semaphore.
 * the semaphore count is initialized to zero, and a number of
 * child threads will block when they try to acquire the semaphore.
 * The parent thread will then post a number of resources, allowing
 * some child threads to enter their critical section.  These
 * child threads will release the semaphore at the end of the critical
 * section, allowing other child threads to proceed.
 */


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <thread.h>
#include <synch.h>


#define MaxThreadsNumber 20

thread_t t[MaxThreadsNumber];
sema_t sema;

void *   
ThreadsStart(void *arg)
{
	int me;

	me = thr_self();
	printf("Thread %d is trying to get the semaphore\n", me);
	sema_wait(&sema);
	printf("Thread %d gets the semaphore\n", me);
	sleep(8);
	printf("Thread %d is releasing the semaphore\n", me);
	sema_post(&sema);
}

 
main(argc, argv)
int argc;
char * argv[];
{
	int i, error;
	int ThreadsNumber = 10; 
	int LWPsNumber = 2;
	int boundflag = 0;	/* no binding by default */
	int lwpscreated = 0;

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
                LWPsNumber = ThreadsNumber;

	/* the semaphore count is initialized to 0 */
	sema_init(&sema, 0, USYNC_THREAD, NULL);

        for (i = 0; i < ThreadsNumber; i++) {
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
                        printf("thr_create of parent thread failed - returned %d...FAILED\n",error);
	}
	for (i = 0; i < ThreadsNumber / 2; i++) {
		sleep(1);
		printf("posting resource\n");
		sema_post(&sema);
	}
	thr_exit(NULL);
}
