#pragma comment(exestr, "@(#) siofifo.c 25.7 95/02/22 ")
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/iasy.h>
#include <fcntl.h>

main(int argc, char *argv[])
{

	int fd, ret, in = 0;
	int c, err = 0;
	extern char *optarg;
	extern int optind, optopt;
	char *port;

	while ((c = getopt(argc, argv, "r:")) != -1)
		switch (c) {
		case 'r':
			in = atoi(optarg);
			if (in > 16 || in <1)
				err++;
			break;
		default:
			err++;
		}

	if (err || optind == argc || in == 0 ) {
		printf("Usage: %s -r level device\n", argv[0]);
		exit(1);
	}

	port = argv[optind];

	fd = open(port, O_RDWR | O_NDELAY);
	if (fd < 0) {
	       perror("open");
	       exit(1);
	}

	if (in)
		setrxtrig(fd, &in);

	printf("%s : FIFO tx %d, rx %d\n", port, 0, in);

	exit(0);
	
}

setrxtrig(int fd, int *in)
{
struct strioctl arg;
int val;


	if (*in >= 14)
	{
		val = T_TRLVL4;
		*in = 14;
	}
	else if (*in >= 8)
	{
		val = T_TRLVL3;
		*in = 8;
	}
	else if (*in >= 4)
	{
		val = T_TRLVL2;
		*in = 4;
	}
	else 
	{
		val = T_TRLVL1;
		*in = 1;
	}
	
	arg.ic_cmd = SETRTRLVL;
	arg.ic_timout = 0;
	arg.ic_len = 4;
	arg.ic_dp = (char *)&val;

	if (ioctl(fd, I_STR, &arg) < 0) {
		perror("ioctl");
		exit(1);
	}
	
}

