#ident	"@(#)big_file.c	15.1"

#include <sys/types.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DIR "/tmp"

extern char *optarg;

/*
 * Make a big empty file, typically used during installation as a swap file.
 *
 * Input: -m margin or -s file_size
 * Output (on stdout): The name of the large file and its
 *   size in 512-byte blocks
 */ 

int
main(int argc, char **argv)
{
	struct statvfs vfs_buf;
	char *tmpfile;
	int fd, c;
	ulong file_size = 0; /* The size of the big file in bytes */
	int margin = 0; /*
			 * The variable "margin" tells us how many free blocks
			 * to leave.  That is, the big file will be as large
			 * as the number of free blocks minus margin.  The
			 * unit for margin is "f_bsize bytes" (see statvfs(2)).
			 */

	while ((c = getopt(argc, argv, "m:s:")) != EOF) {
		switch (c) {
		case 'm':
			margin = atoi(optarg);
			break;
		case 's':
			file_size = strtoul(optarg, (char **)NULL, 0);
			break;
		case '?':
			return 1;
		default:
			(void) fprintf(stderr, "%s: Internal Error during getopt.\n", argv[0]);
			return 1;
		}
	}
	if (file_size && margin) {
		(void) fprintf(stderr, "%s: Error: Cannot give both -m and -s options.\n", argv[0]);
		return 1;
	}
	if (!file_size && !margin) {
		(void) fprintf(stderr, "%s: Error: Must give either -m or -s option.\n", argv[0]);
		return 1;
	}
	if ((tmpfile = tempnam(DIR, "swap")) == NULL)
		return 2;
	if ((fd = creat(tmpfile, 0777)) == -1)
		return 3;
	if (fstatvfs(fd, &vfs_buf) == -1) {
		(void) close(fd);
		(void) unlink(tmpfile);
		return 4;
	}
	if (!file_size)
		file_size = (vfs_buf.f_bfree - margin) * vfs_buf.f_bsize;
	if (ftruncate(fd, file_size) == -1) {
		(void) close(fd);
		(void) unlink(tmpfile);
		return 5;
	}
	(void) close(fd);
	(void) printf("%s %lu\n", tmpfile, file_size / 512);
	return 0;
}
