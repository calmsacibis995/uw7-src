/*		copyright	"%c%" 	*/

#ident	"@(#)prtconf:common/cmd/bootparam/bootparam.c	1.2"
#ident	"$Header$"

/***************************************************************************
 * Command: bootparam
 * Inheritable Privileges: P_MACREAD,P_DACREAD
 *       Fixed Privileges: P_DEV
 * Notes:
 *
 ***************************************************************************/
/*
 * bootparam - print boot parameters
 *
 * For now, this program accesses the boot parameters via /dev/kmem.
 *
 * In the base, /dev/kmem is protected by DAC.  Therefore,
 * this program must always run with the setgid bit on.
 * With MAC installed, the program must in addition have
 * a fixed P_DEV privilege to override the private state
 * of /dev/kmem.
 *
 * Configuration:
 *	cr--r-----	sys	sys	SYS_PUBLIC /dev/kmem (private state)
 *	-r-xr-sr-x	bin	sys	SYS_PUBLIC /usr/bin/ipcs
 *							P_DEV      (fixed)
 *							P_MACREAD  (inher)
 *							P_MACWRITE (inher)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ksym.h>
#include <sys/elf.h>

static const char *kmemfile = "/dev/kmem";	/* kernel memory file */
static const char *bootstring_name = "bootstring";
static const char *bootstring_len_name = "bootstring_actual";

int
main(int argc, char **argv)
{
	struct mioc_rksym rks;
	char *bootstring;
	char buf[BUFSIZ], *p;
	ssize_t n;
	off_t off;
	int mfd;
	int len;
	unsigned char val = 0;

	if ((mfd = open(kmemfile, O_RDONLY)) == -1)
		goto error;

	/* Get length of boot string. */
	rks.mirk_symname = (char *)bootstring_len_name;
	rks.mirk_buf = &len;
	rks.mirk_buflen = sizeof len;
	if (ioctl(mfd, MIOC_READKSYM, &rks) == -1)
		goto error;

	/* Find address of boot string. */
	rks.mirk_symname = (char *)bootstring_name;
	rks.mirk_buf = &bootstring;
	rks.mirk_buflen = sizeof bootstring;
	if (ioctl(mfd, MIOC_READKSYM, &rks) == -1)
		goto error;

	/* Read boot string from kernel, and send to stdout. */
	if (lseek(mfd, (off_t)bootstring, SEEK_SET) == -1L)
		goto error;
	while ((n = read(mfd, buf, sizeof buf)) > 0) {
		for (p = buf; p < buf + n; p++) {
			if (!len--)
				return 0;

			if (*p == '\0'){
				if (val) {
					putchar('\n');
					val = 0;
				} else {
					putchar('=');
					val = 1;

				}
			} else 
				putchar(*p);
		}
	}

error:
	perror(argv[0]);
	return 1;
}
