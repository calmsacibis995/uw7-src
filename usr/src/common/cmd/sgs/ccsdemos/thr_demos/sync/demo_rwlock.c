#ident	"@(#)ccsdemos:thr_demos/sync/demo_rwlock.c	1.1"
/* 
 * To compile, cc -lthread -ldl demo_rwlock.c -o demo_rwlock
 * To invoke: demo_rwlock
 */
/*
 * This program demostrates the use of rwlock (read/write lock).
 * The output shows that rwlock permits multiple readers or a single
 * writer to acquire the lock.  Any number of readers can hold the 
 * lock simultaneously as long as no writers are waiting.  If there is
 * any writer waiting for the lock, no more readers will be able to
 * get the lock. The writer will get the lock after all readers or
 * the writer who has got the lock release the lock.
 */ 

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <thread.h>
#include <synch.h>


int RThreadsNumber  = 5;
int WThreadsNumber = 2;

thread_t readt[10];
thread_t writet[4];
rwlock_t lock;

void *   
ReadStart(void *arg)
{
	int me;

	me = thr_self();
	printf("Read thread %d is trying to get the lock\n", me);
	rw_rdlock(&lock);
	printf("Read thread %d gets the lock\n", me);
	sleep(2);
	printf("Read thread %d is releasing the lock\n", me);
	rw_unlock(&lock);
}

void *
WriteStart(void *arg)
{
	int me;

	me = thr_self();
	printf("Write thread %d is trying to get the lock\n", me);
	rw_wrlock(&lock);
	printf("Write thread %d gets the lock\n", me);
	sleep(4);
	printf("Write thread %d is releasing the lock\n", me);
	rw_unlock(&lock);
}

 
main(argc, argv)
int argc;
char * argv[];
{
	int i, error;
	
	rwlock_init(&lock, USYNC_THREAD, NULL);

	for (i = 0; i < RThreadsNumber; i++) {
		error = thr_create(NULL, 0, ReadStart, (void *) 0,
			   	THR_NEW_LWP, &readt[i]);
		if (error)
	     		printf("thr_create of parent thread failed - returned %d...FAILED\n", error);
	}
	for (i = 0; i < WThreadsNumber; i++) {
		error = thr_create(NULL, 0, WriteStart, (void *) 0,
					0, &writet[i]);
		if (error)
			printf("thr_create of write threads failed - returned %d...FAILED\n", error);
	}
	sleep(5);
	for (i = RThreadsNumber; i < 2 * RThreadsNumber; i++) {
		error = thr_create(NULL, 0, ReadStart, (void *) 0,
			   	0, &readt[i]);
		if (error)
	     		printf("thr_create of parent thread failed - returned %d...FAILED\n", error);
	}
	for (i = WThreadsNumber; i < 2 * WThreadsNumber; i++) {
		error = thr_create(NULL, 0, WriteStart, (void *) 0,
					0, &writet[i]);
		if (error)
			printf("thr_create of write threads failed - returned %d...FAILED\n", error);
	}
	thr_exit(NULL);
}
