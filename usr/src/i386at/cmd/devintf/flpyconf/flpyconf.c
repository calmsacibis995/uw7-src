#ident	"@(#)flpyconf.c	1.2"

#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/cram.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

static int flpconf(void);

static const char *floppyconf = "/var/adm/floppyconf";
static char errbuf[256], *cmd;

void
main(int argc, char **argv)
{
	cmd = argv[0];
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", cmd);
		exit(1);
	}
	if (flpconf()) {
		exit(1);
	}
	exit(0);
}

static int
flpconf(void)
{
	int fd, res = 0;
	unsigned char buf[2], conf;

	if ((fd = open("/dev/cram",O_RDONLY)) < 0) {
		sprintf(errbuf, "%s cannot open /dev/cram", cmd);
		perror(errbuf);
		return -1;
	}
	buf[0] = DDTB;
	if (ioctl(fd, CMOSREAD, buf) < 0) {
		sprintf(errbuf, "%s cannot read /dev/cram", cmd);
		perror(errbuf);
		close(fd);
		return -1;
	}
	close(fd);
	if ((fd = open(floppyconf, O_RDWR | O_CREAT, 0600)) < 0) {
		sprintf(errbuf, "%s cannot open %s", cmd, floppyconf);
		perror(errbuf);
		return -1;
	}
	if (read(fd, &conf, sizeof(conf)) != sizeof(conf) || buf[1] != conf) {

		/* Floppy configuration has changed.  Write new one. */

		(void)lseek(fd, 0, 0);
		(void)write(fd, &buf[1], sizeof(buf[1]));
		res = -1;
	}
	(void)close(fd);
	return res;
}
