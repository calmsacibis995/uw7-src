/*	copyright	"%c%"	*/

#ident	"@(#)crash:i386at/cmd/crash/memsize.c	1.1.1.8"

#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>
#include <sys/kcore.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <macros.h>

/*
 *	memsize -- print the memory size of the active system or a core dump
 *
 *	memsize takes an optional argument which is the name of a dump file;
 *	by default it uses /dev/mem (for active system).
 *
 *	On a live system memsize uses the sysconf(_SC_TOTAL_MEMORY) info
 *	to get the number of pages. 
 *
 *	From a dump file, the header of the core file is read to ascertain the
 *	size.
 */

void
main(argc, argv)
	int	argc;
	char	*argv[];
{
	char	*fname = "/dev/mem";
	int	fd, i, err;
	struct 	bootmem *memavail;
	int	memavailcnt, pagesize;
	int	numpages;
	unsigned long long	memsize; 
	unsigned long	chunk_size;
	kcore_t header;
	char	*buf;
	mreg_chunk_t mchunk;
	size_t len;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmemsize");
	(void)setlabel("UX:memsize");

	if (argc > 2) {
		pfmt(stderr, MM_ACTION, ":1:Usage:  memsize [ dumpfile ]\n");
		exit(1);
	}
	if (argc == 2)
		fname = argv[1];

	if ((fd = open(fname, O_RDONLY)) < 0) {
		pfmt(stderr, MM_ERROR, ":2:Cannot open %s\n", fname);
		exit(1);
	}

	pagesize = sysconf(_SC_PAGESIZE);
	len = max(pagesize, DEV_BSIZE);

	buf = (char *)malloc(len);
	if (buf == NULL) {
		pfmt(stderr, MM_ERROR, ":4:malloc failed\n");
		exit(1);
	}
	/*
	 * getting size on a live system?
	 */
	if (argc < 2) {
		if((numpages = sysconf(_SC_TOTAL_MEMORY)) <= 0) {
			pfmt(stderr, MM_WARNING, 
				":5:Cannot get sysconf memory size\n");
			exit(1);
		} else
			memsize = (unsigned long long)numpages * 
				(unsigned long long)pagesize;
			
	} else { 	/* reading from a dump file */
		if (read(fd, buf, DEV_BSIZE) != DEV_BSIZE) {
			pfmt(stderr, MM_ERROR,
				":6:cannot read header\n"); 
			exit(1);
		}
		memsize = DEV_BSIZE;
		memcpy(&header, buf, sizeof(header));
		if (header.k_magic != KCORMAG) {
			pfmt(stderr, MM_ERROR, 
				":7:memsize: wrong magic in dumpfile\n");
			exit(1);
		}
		/* 
		 * If the k_size field in the dump is non-zero, then
		 * dumpcheck has already recorded the size of the dump
		 * in the header.
		 */
		if (header.k_size != 0) { 
			 memsize = header.k_size;
			 goto bye;
		}
		if (read(fd, &mchunk, sizeof(mchunk)) != sizeof(mchunk)) {
			pfmt(stderr, MM_ERROR,
				":8:failed to read chunk header 1\n");
			exit(1);
		}
		if (mchunk.mc_magic != MRMAGIC) {
			pfmt(stderr, MM_ERROR, ":10:chunk magic not found\n");
			exit(1);
		}
		memsize += DEV_BSIZE;
		while (MREG_LENGTH(mchunk.mc_mreg[0])) {
			for (chunk_size = 0, i = 0; i < NMREG; i++) {
				if (MREG_LENGTH(mchunk.mc_mreg[i]) == 0)
					break;
				if (MREG_TYPE(mchunk.mc_mreg[i]) != MREG_IMAGE)
					continue;
				chunk_size += MREG_LENGTH(mchunk.mc_mreg[i]);
			}
			if (chunk_size % header.k_align != 0) {
				chunk_size = (chunk_size + header.k_align) & 
						~(header.k_align -1);
			}
			memsize += chunk_size;
			while (chunk_size) {
				len = min(len, chunk_size);
				if (read(fd, buf, len) != len) {
					pfmt(stderr, MM_ERROR,
						":3:Read of %s failed\n",fname);
					exit(1);
				}
				chunk_size -= len;
			}
			/* 
			 * read next chunk
			 */
			if (read(fd, &mchunk, sizeof(mchunk)) != 
							sizeof(mchunk)) {
				pfmt(stderr, MM_ERROR,
					":9:failed to read chunk header 2\n");
				exit(1);
			}
			if (mchunk.mc_magic != MRMAGIC) {
				pfmt(stderr, MM_ERROR,
					":10:chunk magic not found\n");
				exit(1);
			}
			memsize += DEV_BSIZE;
		}
	}
bye:
	close(fd);
	printf("%llu\n", memsize);
	exit(0);
}
