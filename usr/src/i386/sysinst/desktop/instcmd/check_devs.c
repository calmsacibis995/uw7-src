#ident	"@(#)check_devs.c	15.1"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/cram.h>
#include <sys/kd.h>
#include <sys/inline.h>
#include <sys/types.h>
#include <sys/fd.h>
#include <sys/ddi.h>

extern int errno;

static void usage();
static int get_floppy_info();
static int get_tape_info();
static int get_parallel_info();
static int get_serial_info();

static char *arg0save;

int
main(argc, argv)
int argc;
char **argv;
{
  extern int opterr;
  extern char *optarg;
  int arg;

  opterr = 0;		/* turn off the getopt error messages */
  arg0save = argv[0];
  while ((arg = getopt(argc,argv,"f:g:ps:t:")) != EOF) {
     switch (arg) {
     case 'f': { /* check for existence/type of floppy device */
	return get_floppy_info(atoi(optarg));
     }
     case 'g': { /* check for existence of generic device */
	return (open(optarg, O_RDONLY, 0) == -1) ? 99 : 0;
     }
     case 'p': { /* check for io addr of parallel port */
	return get_parallel_info();
     }
     case 's': { /* check for existence of serial port */
	return get_serial_info(optarg);
     }
     case 't': { /* check for existence of tape */
	return get_tape_info(optarg);
     }
     case '?' : /* Incorrect argument found */
	usage();
	return (99);
     } /* switch arg */
  } /* while */
  usage();
  return (99);
}

void
usage()
{
	(void) fprintf(stderr,
	  "usage: %s [-f <1|2>] [-g dev] [-p] [-s serial_dev] [-t tape_dev]\n",
	  arg0save);
}

static int
get_floppy_info(n)
int n;
{
	int	fd;
	unsigned char buf[2];

	if ((n<1) || (n>NUMDRV)) /* NUMDRV defined in fd.h */
		return (99); /* not a valid drive number */

	buf[0] = DDTB;
	if ((fd = open("/dev/cram", O_RDONLY)) == -1) {
		(void) fprintf(stderr,
			"%s: errno %d on open /dev/cram\n", arg0save, errno);
		return (99);
	}
	if (ioctl(fd, CMOSREAD, buf) == -1) {
		(void) fprintf(stderr,"%s: errno %d on ioctl of /dev/cram\n",
		   arg0save, errno);
		return (99);
	}
	(void) close(fd);

	if (n==1)
	  return ((buf[1] >> 4) & 0x0F);
	return (buf[1] & 0x0F);
	/* 				0 for nothing	*/
	/* 				2 for 5 1/4"	*/
	/* 				4 for 3 1/2"	*/
}

static int
get_tape_info(path)
char *path;
{
	if (access(path, R_OK) == -1) { /* validate path, permission to open */
		return (99);
	}
	/*
	 * Because of the access() call above, the only error we care about
	 * now is ENXIO.
	 */
	errno=0;
	(void) open(path,O_RDONLY);
	if (errno == ENXIO)
		return (99);	/* indicates no tape controller */
	return (0); /* tape controller is alive; tape may not be
		     * inserted, though
		     */
}

/*
 * determine presence of serial port given by arg path
 */
static int
get_serial_info(path)
char *path;
{
	int fd;

	/* open serial port with NDELAY turned on so we
	 * don't hang waiting for CARRIER_DETECT to be raised
	 */
	fd= open(path, O_RDONLY|O_NDELAY, 0);

	/* fail command if ENXIO -- indicates controller not there
	 * or ENOENT -- indicates file not found
	 */
	if ((fd == -1) && ((errno == ENOENT) | (errno == ENXIO)))
		return (99);
	return (0);
}

/*
 * determine io addr of parallel port from 3 possible reserved space
 */
static int
get_parallel_info()
{
	static int s_ios[3] = { 0x3BC, 0x378, 0x278 };
	int	testval;
	int	act_io;
	int	i, fd;

	if ((fd = open("/dev/console", O_RDONLY)) == -1) {
		(void) fprintf(stderr, "can't open /dev/console\n");
		return (99);
	}
	act_io = 0;
	for (i=0; i < 3; i++) {
		if (ioctl(fd, KDADDIO, s_ios[i]) == -1) { 
			(void) fprintf(stderr, "%x ioctl failed\n", s_ios[i]);
			continue;
		}
		if (ioctl(fd, KDENABIO, 0) == -1) {
			(void) fprintf(stderr, "KDENABIO ioctl failed\n");
			return(99);
		}
		outb(s_ios[i], 0x55);
		testval = inb(s_ios[i]);
		if (testval == 0x55) {
			act_io = s_ios[i];
			break;
		}
	}

	switch (act_io) {
		case 0x3BC: return 1;
		case 0x378: return 2;
		case 0x278: return 3;
		default: return -1;
	}
}
