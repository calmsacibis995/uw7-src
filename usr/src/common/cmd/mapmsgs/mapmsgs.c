/*	copyright	"%c%"

#ident	"@(#)mapmsgs.c	1.2"

/*
 *  Utility to map the messages in a catlogue to a different code set.
 *  We cannot simply run the files trough iconv(1), as this is a binary
 *  file, and the first part is an array of pointers which don't want
 *  to be mapped.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <locale.h>
#include <pfmt.h>
#include "mapmsgs.h"

main(argc, argv)
int argc;
char *argv[];
{
	int i;			/*  Loop variable  */
	int ifd;		/*  Input file descriptor  */
	int ofd;		/*  Output file descriptor  */
	int cs_num;		/*  Index for code set conversion  */
	int hdr_size;		/*  Size of header section  */
	int msg_count;

	unsigned char *i_addr;	/*  For memory mapped input file  */
	unsigned char *o_addr;	/*  For memory mapped output file  */

	caddr_t t_addr;		/*  Temp to avoid type conflicts  */

	struct stat isb;
	struct stat osb;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxels");
	(void)setlabel("UX:mapmsgs");

	/*  Check that the usage is correct  */
	if (argc < 4)
		usage();

	/*  Check that the code set is one that is supported  */
	if (strcmp(argv[1], "437") == 0)
		cs_num = 0;
	else if (strcmp(argv[1], "850") == 0)
		cs_num = 1;
	else if (strcmp(argv[1], "860") == 0)
		cs_num = 2;
	else if (strcmp(argv[1], "863") == 0)
		cs_num = 3;
	else if (strcmp(argv[1], "865") == 0)
		cs_num = 4;
	else
	{
		pfmt(stderr, MM_ERROR, ":110:%s is an unsupported code set\n", argv[1]);
		exit(ERROR);
	}

	/*  Check that we can access the input file  */
	if (access(argv[2], R_OK) == -1)
	{
		pfmt(stderr, MM_ERROR, ":111:cannot access %s\n", argv[2]);
		exit(ERROR);
	}

	if ((ifd = open(argv[2], O_RDONLY)) < 0)
	{
		pfmt(stderr, MM_ERROR, ":112:failed to open %s\n", argv[2]);
		exit(ERROR);
	}

	/*  And the output file  */
	if ((ofd = open(argv[3], O_RDWR | O_CREAT, MODES)) < 0)
	{
		pfmt(stderr, MM_ERROR, ":113:failed to open output file %s\n", argv[3]);
		exit(ERROR);
	}

	/*  Do a stat so we know how big the file is when we memory map it
	 */
	if (fstat(ifd, &isb) < 0)
	{
		pfmt(stderr, MM_ERROR, ":114:failed to stat %s\n", argv[2]);
		exit(ERROR);
	}

	if ((t_addr = mmap(0, isb.st_size, PROT_READ, MAP_PRIVATE, ifd, 0))
	    == (caddr_t)-1)
	{
		pfmt(stderr, MM_ERROR, ":115:failed to map file\n");
		close(ifd);
		close(ofd);
		exit(ERROR);
	}

	i_addr = (unsigned char *)t_addr;
	close(ifd);

	lseek(ofd, isb.st_size-1, 0);

	if ((t_addr = mmap(0, isb.st_size, (PROT_READ|PROT_WRITE),
	    MAP_SHARED, ofd, 0)) == (caddr_t)-1)
	{
		pfmt(stderr, MM_ERROR, ":115:failed to map file\n");
		munmap((caddr_t)i_addr, isb.st_size);
		close(ofd);
		exit(ERROR);
	}

	o_addr = (unsigned char *)t_addr;
	write(ofd, "\0", 1);
	close(ofd);
	hdr_size = *((int *)(i_addr + sizeof(int)));
	msg_count = *((int *)i_addr);
	if (hdr_size >= isb.st_size || hdr_size < 0 ||
	    (msg_count+1)*sizeof(int) != hdr_size) {
		pfmt(stderr, MM_ERROR, ":116:incorrect catalog structure\n");
		exit(ERROR);
	}
	memcpy(o_addr, i_addr, hdr_size);
	o_addr += hdr_size;

	/*  Perform the actual conversion  */
	for (i = hdr_size; i < isb.st_size; i++)
		*o_addr++ = cs_map[cs_num][*(i_addr + i)];

	munmap((caddr_t)i_addr, isb.st_size);
	munmap((caddr_t)o_addr, isb.st_size);
	exit(SUCCESS);
}

void
usage()
{
	pfmt(stdout, MM_ACTION, ":117:Usage: mapmsgs <code_set> <source_file> <destination_file>\n");
	exit(ERROR);
}
