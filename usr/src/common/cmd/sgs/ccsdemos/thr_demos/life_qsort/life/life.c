#ident	"@(#)ccsdemos:thr_demos/life_qsort/life/life.c	1.1"
/* To compile, cc -lthread -ldl source.c -o life
*/
/*
 * This is a benchmark that does a grid calculation, specifically by 
 * simulating the evolution of Conway's game of life.
 *
 * The basic algorithm is to set up a grid of size GridSize by GridSize
 * SpecialThreadsNumber number of special threads 
 * and LWPNumber number of LWP's.
 *
 * The program works by evolving
 * from a random initial state.
 * In each step, there are two parallel parts: a. figure out the
 * next state of each grid cell, and
 * b. install those results onto the grid cells.
 * There is a barrier sync before each step and between parts a and b.
 *
 * Our work would have been made more efficient if cond_wait had had an option
 * not to reacquire the mutex after it is released.
 *
 * Limits: number of threads should not exceed 1000.
 *	size of board should not exceed 1000 by 1000
 *
 * 	To invoke: life [-g Gridsize] [-i MaxStep] 
 *			[-t SpecialThreads] [-l LWPs] [-f fifofile]
 *			[-s scale_factor] [-b] [-p]
 *		defaults: GridSize = 960, MaxStep = 1;
 *			SpecialThreads = 1; LWPs = 1;
 *			scale = 1; unbind; unbind-processor
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <thread.h>
#include <synch.h>
#include <sys/times.h>
int SpecialThreadsNumber  = 1;
int GridSize = 960; 
int LWPsNumber = 1;
int MaxStep = 1;
int scale = 1;
#define GridAlloc 1000 
/* how much space is allocated; GridSize
 * is the area that is actually worked on.
*/
#define ThreadAlloc 1000
int board[GridAlloc][GridAlloc];
int copyboard[GridAlloc][GridAlloc];

thread_t Special[ThreadAlloc];
cond_t	condbarrier; /* condition variable for barrier */
mutex_t	lockbarrier; /* lock for variable */
int countbarrier; /* Variable that counts threads that have reached barrier. */

char pfile[] = "/tmp/poutA6X2";
int fd2;

/* Create the board of life. */
void 
CreateBoard() {

	int row, col, temp;

	for (row=0; row<GridSize; row++)
		for (col=0; col<GridSize; col++){
			temp = rand() % 3;
			if (temp >= 1) board[row][col] = 1; /* Mostly ones*/
			else board[row][col] = 0;
	   	}
}

/*Figures out whether a cell will suffocate or procreate. 
 * Procreates if empty and 2 neighbors.
 * Survives if alive and 1 or 2 neighbors.
 * Otherwise cell dies from loneliness or overpopulation.
*/
void
EvaluateCell(int row, int col) {

	int countneighbors = 0;
	if (row > 0) countneighbors += board[row-1][col];
	if (col > 0) countneighbors += board[row][col-1];
	if (row < GridSize-1) countneighbors += board[row+1][col];
	if (col < GridSize-1) countneighbors += board[row][col+1];
	countneighbors += board[row][col]; /*neighbor of itself */
	if (
		(countneighbors == 0) ||
		(countneighbors == 1) ||
		(countneighbors == 4) ||
		(countneighbors == 5)
  	)
		copyboard[row][col] = 0; /* A temporary side board. */
   	else
		copyboard[row][col] = 1;
}

/* Waits for all threads to finish a step part
 * then broadcasts to all of them.
*/
void
BarrierSync() {

	mutex_lock(&lockbarrier);
	countbarrier +=1;
	if (countbarrier == SpecialThreadsNumber) {
		countbarrier = 0; /* reset for next barrier */
		cond_broadcast(&condbarrier); /*release everyone */
	} else
		cond_wait(&condbarrier, &lockbarrier);
	mutex_unlock(&lockbarrier);
}

void *
SpecialStart(void *arg) {

	clock_t before, after;
	struct tms bfr;
	int index = (int) arg;
	int lower = (GridSize/SpecialThreadsNumber) * index;
	int upper = (GridSize/SpecialThreadsNumber) * (index+1);
	int step, row, col, temp, j;
	char res[8];

	if (index == 0)
		before = times(&bfr);
	for(step = 0; step < MaxStep; step++) {
	  /* wait for all threads to reach barrier */
	  /* replace this and following Barrier call and BarrierSync
             routine when Barrier primitive in library is delivered */
		 BarrierSync(); 
	  /* part a of each step */
		 for(row = lower; row < upper; row++)
	    		for(col = 0; col < GridSize; col++)
				EvaluateCell(row,col);
	  /* wait for barrier */
	  	BarrierSync(); /* not new step. */
		for(row = lower; row < upper; row++)
	    		for(col = 0; col < GridSize; col++)
				board[row][col] = copyboard[row][col];
		if (index == 0) {
			after = times(&bfr);
			sprintf(res,"%d\n", - ((after - before) / scale));
			write(fd2,res,strlen(res));
			before = times(&bfr);
		}
	} /* end steps */
}

main(argc, argv)
int argc;
char * argv[];
{
	int i, error;
	int lwpscreated = 0;
	int boundflag = 0; /* no binding  by default */
	int procboundflag = 0; /* no processor binding by default */
	char *pout;

	pout = pfile;
	while ((argc > 1) && (argv[1][0] == '-')) {
	   switch (argv[1][1]) {
		case 'f':
			if (argc >= 3) {
				argc--;
				argv++;
				pout = argv[1];
			}
			break;
		case 'b':
		   boundflag = 1;
		   break;
		case 'g':
		  if (argc >=3) { 
		  	argc--;
		  	argv++;
		  	GridSize = atoi(argv[1]);
		  }
		  break;
		case 'p':
		  procboundflag = 1;
		  break;
		case 'i':
		  if (argc >=3) { 
		  	argc--;
		  	argv++;
		  	MaxStep = atoi(argv[1]);
		  }
		  break;
		case 's':
		  if (argc >=3) { 
		  	argc--;
		  	argv++;
		  	scale = atoi(argv[1]);
		  }
		  break;
		case 't':
		  if (argc >=3) { 
		  	argc--;
		  	argv++;
		  	SpecialThreadsNumber = atoi(argv[1]);
		  }
		  break;
		case 'l':
		  if (argc >=3) { 
		  	argc--;
		  	argv++;
		  	LWPsNumber = atoi(argv[1]);
		  }
		  break;
		default:
			fprintf(stderr, "invalid option: %s\n", argv[1]);
		  	exit(1);
	   }
  	   argc--;	
  	   argv++;	
	} 
	if ((fd2 = open(pout, 1)) < 0) {
		fprintf(stderr, "can not open output file: %s\n", pout);
		exit(1);
	}
	if (boundflag == 1) 
		LWPsNumber = SpecialThreadsNumber;
	CreateBoard();
	/* Initialize barrier */
	mutex_init(&lockbarrier, USYNC_THREAD, (void *) NULL);
	cond_init(&condbarrier, USYNC_THREAD, (void *) NULL);

	/* Initialize thread */

	for (i = 0; i<SpecialThreadsNumber; i++) {
		if (boundflag == 1) {
			error = thr_create( NULL, 0, SpecialStart,
				(void *) i, THR_BOUND, &Special[i]);
	  	} else if (lwpscreated < LWPsNumber) {
	    		error = thr_create( NULL, 0, SpecialStart,
				(void *) i, THR_NEW_LWP, &Special[i]);
	    		lwpscreated++;
	  	} else {
	    		error = thr_create( NULL, 0, SpecialStart, (void *) i, 
				0, &Special[i]); /* don't create an LWP*/
	  	}
	  	if (error != 0)
		     printf("thr_create of special thread failed - returned %d...FAILED\n", error);
	} /*end of thread creation */

	/* wait for threads to complete their iterations, then exit */
	for (i=0;i<SpecialThreadsNumber;i++)
		thr_join((thread_t) 0,NULL, NULL);
}
