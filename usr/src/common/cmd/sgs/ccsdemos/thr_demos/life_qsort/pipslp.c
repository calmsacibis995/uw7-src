#ident	"@(#)ccsdemos:thr_demos/life_qsort/pipslp.c	1.1"
#include <stdio.h>

int num = 10000;

main(argc, argv)
int argc;
char * argv[];
{

	int fd;


	if (argc != 3) {
		fprintf(stderr,"wrong num args\n");
		exit(1);
	}

	if (open(argv[2],0) >= 0)
		exit(1);
	if (creat(argv[2], 0666) < 0) {
		fprintf(stderr,"can not create lock file\n");
		exit(1);
	}
	if ((fd = open(argv[1],2)) < 0) {
		fprintf(stderr,"can not open output file\n");
		exit(1);
	}
	sleep(num);
	unlink(argv[2]);
}
