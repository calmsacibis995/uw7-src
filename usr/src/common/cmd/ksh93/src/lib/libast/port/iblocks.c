#ident	"@(#)ksh93:src/lib/libast/port/iblocks.c	1.1"
#pragma prototyped
/*
 * aux function for <ls.h> iblocks() macro
 *
 * return number of blocks, including indirect block count
 * given stat info
 *
 * for systems not supporting stat.st_blocks
 */

#include <ast_lib.h>
#include <ast_param.h>
#include <ast_fs.h>

#if !_mem_st_blocks_stat

#ifndef B_DIRECT
#define B_DIRECT	10
#endif

#ifdef BITFS

#define B_SIZE		BSIZE(st->st_dev)
#define B_INDIRECT	NINDIR(st->st_dev)

#else

#ifdef BSIZE
#define B_SIZE		BSIZE
#else
#define B_SIZE		1024
#endif

#ifdef NINDIR
#define B_INDIRECT	NINDIR
#else
#define B_INDIRECT	128
#endif

#endif

#endif

off_t
_iblocks(struct stat* st)
{
#if _mem_st_blocks_stat
	return((st->st_blocks + 1) / 2);
#else
	off_t	b;
	off_t	t;

	t = b = (st->st_size + B_SIZE - 1) / B_SIZE;
	if ((b -= B_DIRECT) > 0)
	{
		t += (b - 1) / B_INDIRECT + 1;
		if ((b -= B_INDIRECT) > 0)
		{
			t += (b - 1) / (B_INDIRECT * B_INDIRECT) + 1;
			if (b > B_INDIRECT * B_INDIRECT) t++;
		}
	}
	return(t);
#endif
}
