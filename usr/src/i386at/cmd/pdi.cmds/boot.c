/*	copyright	"%c%"	*/

#ident	"@(#)pdi.cmds:boot.c	1.3.5.1"
#ident	"$Header$"

/*
 * This file copies a hard disk boot image to the boot slice. The first
 * 28 sectors of the boot slice are available.
 */

#include <fcntl.h>
#include <malloc.h>
#include <sys/vtoc.h>
#include <stdio.h>
#include <pfmt.h>

/* structures to be used for loading the boot */
extern  struct	absio  absio;
extern  struct	disk_parms dp;
extern  daddr_t unix_base;
extern  int 	bootfd;
extern  int 	diskfd;


void
loadboot(void)
{
	char		*buf;
	long            fsz, len;
	int             i;

	/* Get length of boot image. */
	fsz = lseek(bootfd, 0L, 2);
	if (fsz > VTOC_SEC * dp.dp_secsiz) {
		(void) pfmt(stderr, MM_ERROR,
			":393:The size of boot code is too large\n");
		exit(43);
	}
	lseek(bootfd, 0L, 0);

	/* Round length to a sector boundary. */
	len = ((fsz + dp.dp_secsiz - 1) / dp.dp_secsiz) * dp.dp_secsiz;

	/* Read the whole thing in one fell swoop. */
	buf = malloc(len);
	if (read(bootfd, buf, fsz) != fsz) {
		(void) pfmt(stderr, MM_ERROR,
			":16:Error reading boot program\n");
		exit(42);
	}

	/* Write out the boot to the beginning of the unix partition. */
	set_sig_off();
	for (i = 0; i < len / dp.dp_secsiz; i++){
		absio.abs_sec = unix_base + i;
		absio.abs_buf = (buf + (i * dp.dp_secsiz));
		if (ioctl(diskfd, V_WRABS, &absio) != 0) {
			(void) pfmt(stderr, MM_ERROR,
				":13:Error writing boot to disk!");
			(void) pfmt(stderr, MM_ERROR,
				":14:Successful completion is\n"
				"required to allow boot from hard disk!\n");
			exit(43);
	    	}
	}
	set_sig_on();
}
