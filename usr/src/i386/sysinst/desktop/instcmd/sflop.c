#ident	"@(#)sflop.c	15.1"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/vtoc.h>
#include <sys/fd.h>

static char *command;
static uchar_t buf[SECSIZE];
static int floppy_fd, floppy_num, floppy_size;
static char *floppy_dev;

static void usage(void);
static int get_floppy_num(char);
static int get_floppy_size(void);
static int read_serial(void);
static int write_serial(char *, int);

/*
 * This program reads or writes a serial number to a floppy disk.
 * The last sector of the floppy is reserved for the serial number.
 */
int
main(int argc, char **argv)
{
	int arg, readnum = 0, writenum = 0, overwrite = 0;

	command = argv[0];
	while ((arg = getopt(argc, argv, "r:w:o")) != EOF) {
		switch (arg) {
		case 'r':
			readnum++;
			floppy_num = get_floppy_num(optarg[0]);
			break;
		case 'w':
			writenum++;
			floppy_num = get_floppy_num(optarg[0]);
			break;
		case 'o':
			overwrite++;
			break;
		case '?':
			usage();
			return 1;
		default:
			fprintf(stderr, "%s: Internal error during getopt.\n", command);
			return 1;
		}
	}
	if (readnum && writenum) {
		fprintf(stderr, "%s: ERROR: Cannot give both -r and -w options.\n", command);
		return 1;
	}
	if (!readnum && !writenum) {
		fprintf(stderr, "%s: ERROR: Must give either -r or -w option.\n", command);
		return 1;
	}
	if (overwrite && readnum) {
		fprintf(stderr, "%s: ERROR: Cannot give -o with the -r option.\n", command);
		return 1;
	}
	argc -= optind;
	if ((readnum && argc != 0) || (writenum && argc != 1)) {
		usage();
		return 1;
	}
	floppy_dev = (floppy_num == 0) ? "/dev/rdsk/f0t" : "/dev/rdsk/f1t";
	floppy_size = get_floppy_size();
	if (readnum)
		return read_serial();
	else if (writenum)
		return write_serial(argv[optind], overwrite);
	else {
		fprintf(stderr, "%s: Internal error: readnum and writenum are both false.\n", command);
		return 2;
	}
}

static void
usage()
{
	fprintf(stderr, "usage:");
	fprintf(stderr, "\t%s -w (a|b) [-o] serial_number\n", command);
	fprintf(stderr, "\t%s -r (a|b)\n", command);
	fprintf(stderr, "\t-w: write the serial number\n");
	fprintf(stderr, "\t-r: read the serial number\n");
	fprintf(stderr, "\t-o: overwrite data (if any) in the reserved section of the floppy\n");
}

static int
get_floppy_num(char floppy_letter)
{
	switch (floppy_letter) {
	case 'a':
	case 'A':
		return 0;
	case 'b':
	case 'B':
		return 1;
	default:
		fprintf(stderr, "%s: ERROR: Invalid drive letter: %c\n", command, floppy_letter);
		usage();
		exit(1);
		/* NOTREACHED */
	}
}

/* get_floppy_size returns the number of sectors on the floppy. */
static int
get_floppy_size(void)
{
	struct disk_parms parms;

	if ((floppy_fd = open(floppy_dev, O_RDONLY)) == -1) {
		fprintf(stderr, "%s: ERROR: Cannot open floppy for reading: %s.\n", command, strerror(errno));
		exit(1);
	}
	if (ioctl(floppy_fd, V_GETPARMS, &parms) == -1) {
		fprintf(stderr, "%s: ERROR: Cannot get floppy parameters: %s.\n", command, strerror(errno));
		exit(1);
	}
	switch (parms.dp_pnumsec) {
	case 2400:
	case 2880:
		return parms.dp_pnumsec;
	default:
		fprintf(stderr, "%s: ERROR: Floppy format is neither 1.2MB nor 1.44MB.\n", command);
		exit(1);
		/* NOTREACHED */
	}
}

/*
 * read_serial reads the serial number from the last sector of the floppy and
 * writes it to stdout.
 */
static int
read_serial(void)
{
	if (lseek(floppy_fd, (floppy_size - 1) * SECSIZE, SEEK_SET) == (off_t)-1) {
		fprintf(stderr, "%s: ERROR: Seek on floppy failed: %s.\n", command, strerror(errno));
		exit(1);
	}
	if (read(floppy_fd, buf, SECSIZE) != SECSIZE) {
		fprintf(stderr, "%s: ERROR: Read on floppy failed: %s.\n", command, strerror(errno));
		exit(1);
	}
	buf[SECSIZE - 1] = '\0'; /* Make sure the buffer is null-terminated */
	fprintf(stdout, "%s\n", buf);
	return 0;
}

/*
 * write_serial writes the serial number into the last sector of the floppy.
 */
static int
write_serial(char *serial_num, int overwrite)
{
	int i;

	if (strlen(serial_num) >= SECSIZE) {
		fprintf(stderr, "%s: ERROR: Serial number is too long.\n", command);
		exit(1);
	}
	(void) close(floppy_fd);
	if ((floppy_fd = open(floppy_dev, O_RDWR)) == -1) {
		fprintf(stderr, "%s: ERROR: Cannot open floppy for writing: %s.\n", command,
		(errno == EROFS) ? "Floppy is write-protected" : strerror(errno));
		exit(1);
	}
	if (lseek(floppy_fd, (floppy_size - 1) * SECSIZE, SEEK_SET) == (off_t)-1) {
		fprintf(stderr, "%s: ERROR: Seek on floppy failed: %s.\n", command, strerror(errno));
		exit(1);
	}
	/*
	 * When a floppy is first formatted, it consists entirely of the
	 * character 0xf6.  When a floppy is first erased, it consists entirely
	 * of the null character.  Therefore, if the last sector of the floppy
	 * consists entirely of 0xf6 or 0x00, we assume it is safe to overwrite it.
	 */
	if (!overwrite) {
		if (read(floppy_fd, buf, SECSIZE) != SECSIZE) {
			fprintf(stderr, "%s: ERROR: Read on floppy failed: %s.\n", command, strerror(errno));
			exit(1);
		}
		for (i = 0; i < SECSIZE; i++) {
			switch (buf[i]) {
			case 0x00:
			case 0xf6:
				break;
			default:
				fprintf(stderr, "The floppy appears to contain a serial number or data in the section\n");
				fprintf(stderr, "reserved for the serial number.  Use the -o option to overwrite.\n");
				exit(1);
				/* NOTREACHED */
			}
		}
		if (lseek(floppy_fd, (floppy_size - 1) * SECSIZE, SEEK_SET) == (off_t)-1) {
			fprintf(stderr, "%s: ERROR: Seek on floppy failed: %s.\n", command, strerror(errno));
			exit(1);
		}
	}
	(void) strncpy((char *)buf, serial_num, (size_t)SECSIZE);
	if (write(floppy_fd, buf, SECSIZE) != SECSIZE) {
		fprintf(stderr, "%s: ERROR: Write to floppy failed: %s.\n", command, strerror(errno));
		exit(1);
	}
	return 0;
}
