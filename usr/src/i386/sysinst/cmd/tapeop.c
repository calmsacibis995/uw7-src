#ident	"@(#)tapeop.c	15.1"

#include <fcntl.h>
#include <sys/tape.h>
#include <sys/scsi.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

main(int argc, char **argv)
{
	int cmd = 0, c, fd,length = 0;
	struct	blklen bl;

	while ((c = getopt (argc, argv, "twf:")) != EOF)
		switch (c) {
		case 't':
			cmd = 1;
			break;
		case 'w':
			cmd = 2;
			break;
		case 'f':
			cmd = 3;
			length = atoi(optarg);
			break;
		}
	fd = (optind == argc) ? 0 : open(argv[optind], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "tapeop: Error: cannot open tape device.\n");
		return 1;
	}
	switch (cmd) {
	case 0:
		break;
	case 1:
		ioctl(fd, T_RETENSION, 0, 0);
		break;
	case 2:
		ioctl(fd, T_RWD, 0, 0);
		break;
	case 3:
                bl.max_blen = length;
                bl.min_blen = length;
                ioctl(fd,T_WRBLKLEN,&bl,sizeof(struct blklen));
		break;
	}
	close(fd);
	return 0;
}
