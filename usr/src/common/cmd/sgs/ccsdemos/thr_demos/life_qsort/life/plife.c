#ident	"@(#)ccsdemos:thr_demos/life_qsort/life/plife.c	1.1"
#include <stdio.h>
#include <ctype.h>
#include <sys/times.h>
int GridSize = 960; 
int MaxStep = 1;
int scale = 1;
#define GridAlloc 1000 
/* how much space is allocated; GridSize
 * is the area that is actually worked on.
*/
int board[GridAlloc][GridAlloc];
int copyboard[GridAlloc][GridAlloc];

char pfile[] = "/tmp/poutA6X2";
int fd2;


/* Create the board of life. */
void 
CreateBoard()
{
	int row, col, temp;
	for (row=0; row<GridSize; row++)
		for (col=0; col<GridSize; col++) {
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
EvaluateCell(int row, int col)
{
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

void
SpecialStart()
{
	int lower = 0;
	int upper = GridSize;
	int step, row, col;
	clock_t before, after;
	struct tms bfr;
	char res[8];

	before = times(&bfr);
	for(step = 0; step < MaxStep; step++) {
	  /* part a of each step */
		for(row = lower; row < upper; row++)
	    		for(col = 0; col < GridSize; col++)
				EvaluateCell(row,col);
	  	for(row = lower; row < upper; row++)
	    		for(col = 0; col < GridSize; col++)
				board[row][col] = copyboard[row][col];
		after = times(&bfr);
		sprintf(res,"%d\n", (after - before) / scale);
		write(fd2,res,strlen(res));
		before = times(&bfr);
	} /* end step */
}


main(argc, argv)
int argc;
char * argv[];
{

	char *pout;
	int boundflag = 0;

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
		case 'g':
		  	if (argc >=3) {
		   		argc--;
		   		argv++;
		   		GridSize = atoi(argv[1]);
		  	}
		   	break;
		case 'p':
			boundflag = 1;
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
		case 'l':
			if (argc >= 3) {
				argc--;
				argv++;
			}
			break;
		default:
		   	fprintf(stderr, "invalid option:<%s>\n",argv[1]);
		   	exit(1);
	   }
  	   argc--;	
  	   argv++;	
	} 
	if ((fd2 = open(pout,1)) < 0) {
		fprintf(stderr,"can not open output file:<%s>\n",pout);
		exit(1);
	}
	CreateBoard();
	SpecialStart();
}
