#ident	"@(#)ccsdemos:thr_demos/life_qsort/quicksort/qsort.c	1.1"
/*
June 1, 1993.
Invoke by
cc -lthread -ldl quickdemo2.c -o quick2
*/

/*
 * START QUICKSORT TEST DESCRIPTION
 * This code was adapted from code
 * provided by the Rochester group of Prof. Michael Scott and his
 * group who kindly allowed us to use it here.
 *
 * There are three stages to the algorithm.
 * 1. Generate a number of pivots equal to 
 *    one less than the the number of threads available.
 *    Sort them.
 *    The interval between two successive pivots is called a "bucket" and
 *    will ultimately be sorted by a single thread.
 *
 * 2. Each thread  (routine NormalStart) takes an equi-length portion of the 
 *    original array and puts each item
 *    into an appropriate bucket, labelled index using routine findpart.
 *    To avoid contention, each thread j has its own array for bucket index
 *    called myarray[index][j].
 *    After copying to myarray[][j], thread j barrier synchronizes 
 *    with the other threads using routine BarrierSync.
 *    Then all threads cooperate to copy from each myarray[index][j]
 *    to finalarray[index].
 *
 * 3. Each thread j does a local quicksort on finalarray[j] 
 *    using routine quicksort.
 *
 *  Steps two and three are repeated MaxStep numbers of time to get performance
 *  figures.
 *
 * END TEST DESCRIPTION
 */


#include <stdio.h>
#include <ctype.h>
#include <thread.h>
#include <synch.h>
#include <sys/times.h>

#define PRINT 0 /* if 1, then print, else don't */
int NormalThreadsNumber = 1;
int ProblemSize = 1000; 
int threshflag = 1; /* only print first THRESH of each finalarray */
int LWPsNumber = 1;
int MaxStep = 1; /* number of times to do the sort */
int scale = 1;
#define THRESH 8 /* at 8 or fewer, do insertion sort */
/* how much space is allocated; ProblemSize
 * is the area that is actually worked on.
*/
#define ThreadAlloc 4

char *test_name = "SortTest";

thread_t Normal[ThreadAlloc];
cond_t condbarrier; /* condition variable for barrier */
mutex_t lockbarrier; /* lock for barrier */
int countbarrier; /* counter for barrier */
int threadcount = 1; /*Number of threads in the simulation */
int lwpcount = 1; /*Number of LWPs in the simulation */

int fd;

/* Allocate big arrays --- for illustration purposes only */

#define ARRAYSIZE 100000
int startarray[ARRAYSIZE];
int myarray[ThreadAlloc][ThreadAlloc][ARRAYSIZE];
int count[ThreadAlloc][ThreadAlloc];
int finalarray[ThreadAlloc][ARRAYSIZE];
int finalcount[ThreadAlloc];
int pivotarray[ThreadAlloc];


/* Swaps what's in locations i and j */
swap(int i, int j, int qs_array[] ) 
{
	int temp = qs_array[i];
	qs_array[i] = qs_array[j];
	qs_array[j] = temp;
}


/* Performs the partitioning.
 * This means putting all values above pivot from the returned
 * index on up to the right.
 * This returns the index of the location whose value
 * is at least as large as the pivot.
*/
int
qs_part(i, j, pivot, qs_array )
int i;
int j;
int pivot;
int qs_array[];
{

        int piv = pivot;
        int left = i;
        int right = j;
        for ( ; ; ) {
		while (qs_array[right] > piv) { right -= 1;}
                while ( qs_array[left] < piv ) {
                        left += 1;
                } 
                if ( left < right ) { /* indexes in wrong order */
                        swap(left, right, qs_array);
			left++;
			right--;
		}
                else
                {
                        if ( left == i ) return left + 1; else return left;
                }
        }
}



/*
//
// return median index
//
*/
qs_findpivot(i, j)
int i, j;
{

	return ((i + j)/2);
}
			


/*
//
// Resort to insertion sort when we get down to small array sizes.
//
*/
qs_insort(i, j, qs_array)
int i;
int j;
int qs_array[];
{
        register 	int 	k;
        register 	int 	z;
        register 	int 	temp;
        for ( k = i + 1; k <= j; k++ )
        {
                temp = qs_array[k];
                z = k;
                while ( qs_array[z-1] > temp && z >= i )
                {
                        qs_array[z] = qs_array[z-1];
                        z --;
                }
                qs_array[z] = temp;
        }
}

/* Create the array to sort. */
/* Can decide on a uniform or skewed distribution */
CreateArray(qs_array)
int qs_array[];
{
	int i;
	if (ProblemSize > ARRAYSIZE) {printf("Problem Size too big for array");
	} else {
		for (i=0; i < ProblemSize; i++) {
		  qs_array[i] = 10*(ProblemSize -i);
		}
	}
}

/* Print the various finalarrays sorted. */
/* If threshflag is 1, then print only THRESH of each array */
void 
PrintArray()
{
	int i, j, temp;
	for (i=0; i < NormalThreadsNumber; i++){
		printf("\n");
		temp = finalcount[i];
		if ((threshflag == 1) && (THRESH < temp)) { temp = THRESH;}
		for(j = 0; j < temp; j++) {
			printf("%d ", finalarray[i][j]);
		}
	}
	printf("\n");
}


/*
// findpart(item, pivotsize ) returns 
// pivot partition of item in the range
// from 0 to pivotsize.
// if item <= ith item then it is put in ith partition.
// if item > pivotsize-1th item then it is put in pivotsize's partition.
//
// invariant item <= pivotarray[high] && item > pivotarray[low-1]
*/
findpart(item, pivotsize)
int item,pivotsize;
{
  int low;
  int high = pivotsize-1;
  int m;
  if (item <= pivotarray[0]) return 0;
  else if (item > pivotarray[pivotsize-1]) return pivotsize;
  else {
    low = 0;
    while (low < high) {
      m = (low + high)/2;
      if (item > pivotarray[m]) low = m+1;
      else if (item <= pivotarray[m]) high = m;
    }
    return low;
  }
}




/* Performs a quicksort of elements i through j inclusive
  * of argument qs_array.
  * THRESH is the size below which an insertion sort is performed.
*/
quicksort(i, j, qs_array)
int i, j;
int qs_array[];
{
   int newhigh;
   int pivot = qs_array[qs_findpivot(i,j)];
   if (i >= j) { /* done */
   } else if ((j - i) < THRESH) {
       qs_insort(i, j, qs_array);
   } else {
        newhigh = qs_part(i, j, pivot, qs_array);
        quicksort(i, newhigh-1, qs_array);
        quicksort(newhigh, j, qs_array);        
   }
}


/* Waits for all threads to finish a step part
 * then broadcasts to all of them.
 * In this implementation, all threads reacquire the mutex
 * after the broadcast.
 *
*/
BarrierSync(int stage) 
{
	mutex_lock(&lockbarrier);
	countbarrier++;
	if (countbarrier == NormalThreadsNumber ) {
	    countbarrier = 0; /* reset for next barrier */
	    cond_broadcast(&condbarrier); /*release everyone */
	} else {
	    cond_wait(&condbarrier, &lockbarrier);
	}
	mutex_unlock(&lockbarrier);
}


/* Performs Steps 2 and 3 of algorithm as described in TEST DESCRIPTION
 * in preamble: use pivot array to partition a portion of the original
 * array into buckets, then does a local sort.
*/
NormalStart(void * arg)
{
	int step ;
	int index = (int) arg;
	int lower = (ProblemSize/NormalThreadsNumber) * index;
	int upper = (ProblemSize/NormalThreadsNumber) * (index+1);
	int tempbucket, tempindex, temp;
	int i, j, k, item;
	int pivotsize = NormalThreadsNumber -1;
	clock_t before, after;
	struct tms bfr;
	char res[8];

	/* step 2 */
	/* reinitialize the counts. */
	if (index == 0)
		before = times(&bfr);
    for(step=0; step < MaxStep; step++){
	for (i = 0; i < NormalThreadsNumber; i++) {
		count[index][i] = 0;
		finalcount[i] = 0;
	}
	/* do the partition with respect to the pivot array */
	for(i=lower; i < upper; i++) {
		item = startarray[i]; /* item from original array */
		tempbucket = findpart(item, pivotsize); 
			/* bucket where data belongs */
		tempindex = count[index][tempbucket]; /* index in myarray */
		myarray[index][tempbucket][tempindex] = item;
		count[index][tempbucket]++;
	}
	BarrierSync(2);
	/* step 3 */
	/* copy the data to final array. */
	tempindex = 0;
	for (i = 0; i <NormalThreadsNumber; i++)  {
		temp = count[i][index];
		for (j=0; j < temp; j++) {
			finalarray[index][tempindex] = myarray[i][index][j];
			tempindex++;
		}
	}
	/* Perform local sort. */
	finalcount[index] = tempindex;
	quicksort(0, tempindex-1, finalarray[index]);
	BarrierSync(2);
	if (index == 0) {
		after = times(&bfr);
		sprintf(res, "%d\n",  ((after - before) / scale));
		write(fd, res, strlen(res));
		before = times(&bfr);
	}
     }
}




main(argc, argv)
int argc;
char * argv[];
{
	int i, error, temp, step;
	int lwpscreated = 0;
	int boundflag = 0; /* no binding  by default */
	char *pout;

	pout = test_name;
	while ((argc > 1) && (argv[1][0] == '-')) {
	   switch (argv[1][1]) {
		case 'b':
		   boundflag = 1;
		   break;
		case 'f':
		   if (argc >= 3) {
			argc--;
			argv++;
			pout = argv[1];
		   }
		   break;
		case 'a':
		   threshflag = 0; /* print out entire result */
		   break;
		case 'p':
		  if (argc >=3) { 
		   argc--;
		   argv++;
		   ProblemSize = atoi(argv[1]);
		  }
		   break;
		case 't':
		  if (argc >=3) { 
		   argc--;
		   argv++;
		   NormalThreadsNumber = atoi(argv[1]);
		  }
		   break;
		case 'i':
		  if (argc >=3) { 
		   argc--;
		   argv++;
		   MaxStep = atoi(argv[1]);
		  }
		   break;
		case 'l':
		  if (argc >=3) { 
		   argc--;
		   argv++;
		   LWPsNumber = atoi(argv[1]);
		  }
		   break;
		case 's':
		   if (argc >= 3) {
			argc--;
			argv++;
			scale = atoi(argv[1]);
		   }
		   break;
		default:
		   fprintf(stderr, "Format is:\n");  
		   fprintf(stderr, "quick [-p ProblemSize]  [-t NormalThreads] [-i MaxStep] [-all] [[-l LWPs] [-bind]]\n");
		   fprintf(stderr, "defaults: ProblemSize = 60, NormalThreads = 1; LWPs = 1; MaxStep = 1; threshhold only, unbound\n");
		   exit(1);
	   }
  	   argc--;	
  	   argv++;	
	} 
	if ((fd = open(pout, 1)) < 0) {
		fprintf(stderr, "cannot open output file: <%s>\n", pout);
		exit(1);
	}
	if ((NormalThreadsNumber) > ThreadAlloc){
		fprintf(stderr, "More threads allowed than ThreadAlloc which is %d\n", ThreadAlloc);
		exit(1);
	}
	if (boundflag == 1) {LWPsNumber = NormalThreadsNumber;}
/*
	fprintf(stderr, "Parameter settings: ProblemSize = %d, MaxStep = %d, ThreshFlag = %d,\n", ProblemSize, MaxStep, threshflag);
	fprintf(stderr, " NormalThreads = %d, Number of LWP's = %d, Boundflag = %d\n", NormalThreadsNumber, LWPsNumber, boundflag);
*/
	CreateArray(startarray);
	/* Put pivots in pivot array and sort */
	for (i=0; i<NormalThreadsNumber-1; i++) {
		temp = ProblemSize/NormalThreadsNumber;
		pivotarray[i] = startarray[temp*(i+1)];
	}
	qs_insort(0, NormalThreadsNumber-2, pivotarray);
	if (PRINT == 1) {
	 printf("\n Pivot array: \n");
	 for (i=0; i<NormalThreadsNumber-1; i++) 
	  { printf("%d ", pivotarray[i]); }
	 printf("\n");
	}
	/* Initialize barrier */
	mutex_init(&lockbarrier, USYNC_THREAD, (void *) NULL);
	cond_init(&condbarrier, USYNC_THREAD, (void *) NULL);
	/* Initialize threads */
	for (i = 0; i<NormalThreadsNumber; i++) {
	  if (boundflag == 1) {
	    error = thr_create( NULL, 0, (void *(*)())NormalStart, (void *) i, 
		THR_BOUND, &Normal[i]);
	   } else if (lwpscreated < LWPsNumber) {
	    error = thr_create( NULL, 0, (void *(*)())NormalStart, (void *) i, 
		THR_NEW_LWP, &Normal[i]);
	    lwpscreated++;
	  } else {
	    error = thr_create( NULL, 0, (void *(*)())NormalStart, (void *) i, 
		0, &Normal[i]); /* don't create an LWP*/
	  }
	  if (error != 0) {
		printf("thr_create of normal threadfailed - returned %d...FAILED\n", error);
	  }
	} /*end of normal thread creation */
	/* Thread join */
	/* Wait for threads to complete their iterations, then exit */
	for (i=0;i<NormalThreadsNumber;i++){
		thr_join((thread_t) 0,NULL, NULL);
	}
}










