/*		copyright	"%c%" 	*/

#ident	"@(#)flop_num.c	15.1"
#ident  "$Header$"

/*
** exits the number of floppy drives present -  0, 1, or 2.
*/

#include <fcntl.h>
#include <sys/cram.h>

main()
{
	int	fd;
	unsigned char buf[2];


	buf[0] = DDTB;
	if ((fd = open("/dev/cram", O_RDONLY)) < 0)
		exit (1);
	ioctl(fd, CMOSREAD, buf);
	close(fd);
	fd = 0;

	if ((buf[1] >> 4) & 0x0F)  { /* Floppy Drive 0	*/
	/* 				0 for nothing	*/
	/* 				2 for 5 1/4"	*/
	/* 				4 for 3 1/2"	*/
#ifdef DEBUG
		printf ("Floppy drive 0 = %d\n", ((buf[1] >> 4) & 0x0F));
#endif

		fd+=1;
	}
	if (buf[1] & 0x0F) {         /* Floppy Drive 1	*/
#ifdef DEBUG
		printf ("Floppy drive 1 = %d\n", (buf[1] & 0x0F));
#endif
		fd+=1;
	}
	exit (fd);
}
