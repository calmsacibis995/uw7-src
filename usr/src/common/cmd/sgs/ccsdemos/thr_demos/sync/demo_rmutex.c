#ident	"@(#)ccsdemos:thr_demos/sync/demo_rmutex.c	1.1"
/* 
 * To compile, cc -lthread -ldl demo_rmutex.c -o demo_rmutex
 *
 * To invoke, demo_rmutex [-d depth] [-b Boundflag] [-t Threads] [-l LWPs]
 *    depth is the recursion depth
 *    defaults: depth = 3; unbind; Threads = 3; LWPs = 3;
 */
/*
 * this is a demo program to illustrate the use of recursive mutex.
 * each thread created will acquire the recursive mutex 'depth'
 * number of times and then release it.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <thread.h>
#include <synch.h>

#define MaxThreadsNumber 10

thread_t t[MaxThreadsNumber];
rmutex_t rmutex;

void *   
ThreadsStart(void *arg)
{
	int me;
	int rdep = (int)arg;

	if (rdep > 0) {
		me = thr_self();
		printf("Thread %d is trying to get the rmutex\n", me);
		rmutex_lock(&rmutex);
		printf("Thread %d gets the rmutex\n", me);
		sleep(2);
		ThreadsStart((void *)(--rdep));
		printf("Thread %d is releasing the rmutex\n", me);
		rmutex_unlock(&rmutex);
	}
}

 
main(argc, argv)
int argc;
char * argv[];
{
	int i, error;
	int depth = 3;
	int ThreadsNumber = 3;
	int boundflag = 0;
	int LWPsNumber = 3;
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
			case 'd':
				if (argc >= 3) {
					argc--;
					argv++;
					depth = atoi(argv[1]);
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

	rmutex_init(&rmutex, USYNC_THREAD, NULL);

        for (i = 0; i < ThreadsNumber; i++) {
           if (boundflag == 1) {
                error = thr_create(NULL, 0, ThreadsStart, (void *) depth,
                                THR_BOUND, &t[i]);
           } else if (lwpscreated < LWPsNumber) {
                error = thr_create(NULL, 0, ThreadsStart, (void *)depth,
                                        THR_NEW_LWP, &t[i]);
                lwpscreated++;
           } else {
                error = thr_create(NULL, 0, ThreadsStart, (void *)depth,
                                        0, &t[i]);
           }
	   if (error)
	     		printf("thr_create of parent thread failed - returned %d...FAILED\n", error);
		sleep(4);
	}
	thr_exit(NULL);
}
