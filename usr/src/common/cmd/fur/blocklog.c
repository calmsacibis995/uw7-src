#ident	"@(#)fur:common/cmd/fur/blocklog.c	1.1.1.1"
#ifdef KERNEL
#define _KERNEL
#endif
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#ifndef KERNEL
#include <limits.h>
#endif
#include "log.h"

static ulong Cursource = ULONG_MAX;

#define MY_BUFSIZE 4096
#define BLOCK_BUFSIZE 40960

static char Outfile[] = BLOCKLOGPREFIX ".XX";

#ifdef libc
#define open _rtopen
#define mmap _rtmmap
#define write _rtwrite
#endif

#ifndef KERNEL
static int
outfile()
{
	int i;
	int fd;

	for (i = 0; i < 99; i++) {
		Outfile[sizeof(Outfile) - 3] = '0' + i / 10;
		Outfile[sizeof(Outfile) - 2] = '0' + i % 10;
		if ((fd = open(Outfile, O_EXCL|O_RDWR|O_CREAT, 0666)) >= 0)
			return(fd);
	}
}

struct blockcount *BlockCount;

void
blockinit()
{
	int fd;
	int i;
	char buf[MY_BUFSIZE];

	for (i = 0; i < MY_BUFSIZE / sizeof(long); i++)
		((long *) buf)[i] = 0;
	if ((fd = outfile()) < 0)
		exit(1);
	for (i = 0; i < (NBLOCKS * sizeof(struct blockcount)) / MY_BUFSIZE; i++)
		write(fd, buf, MY_BUFSIZE);
	if ((NBLOCKS * sizeof(struct blockcount)) % MY_BUFSIZE)
		write(fd, buf, (NBLOCKS * sizeof(struct blockcount)) % MY_BUFSIZE);
	if ((BlockCount = (struct blockcount *) mmap(NULL, NBLOCKS * sizeof(struct blockcount), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == (struct blockcount *) -1)
		exit(1);
	close(fd);
}
#else

unsigned long BlockSize = NBLOCKS * sizeof(struct blockcount);
struct blockcount BlockCount[NBLOCKS];

#endif

#ifndef FLOW
blocklog(unsigned long blockno)
{
	static int in_here = 0;

	if (in_here)
		return;
	in_here = 1;
#ifndef KERNEL
	if (!BlockCount)
		blockinit();
#endif
	BlockCount[blockno].callcount++;
	if (Cursource != ULONG_MAX) {
		if (!BlockCount[Cursource].firstcount)
			BlockCount[Cursource].firstblock = blockno;
		if (BlockCount[Cursource].firstblock == blockno)
			BlockCount[Cursource].firstcount++;
		else
			BlockCount[Cursource].secondcount++;
	}
	Cursource = blockno;
	in_here = 0;
}
#endif
