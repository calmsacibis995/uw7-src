/*
 *	@(#)MemConMap.c	11.1	10/28/97	14:02:51
 *
 *	Copyright (C) The Santa Cruz Operation, 1997.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 */
/*
 *	Handle all mmap requests and remember them.
 *	Some mmap requests come with conmemmap requests, do those
 *		when requested.
 *	And provide a mechanism to clear all the outstanding mmaps
 */

#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/kd.h>
#include <sys/ws/ws.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>

typedef struct	xMemMaps	{
	void * addr;
	size_t len;
} memmaps_t ;

#define	MaxMaps	64
static memmaps_t Maps[MaxMaps];
static int MapIndex = 0;
static int ServerPID = 0;

/*
 *	memconmap - consolidate all mmap and ioctl CON_MEM_MAP
 *		operations into one recording mechanism here
 *	The first bunch of arguments are just like mmap
 *	The ioctlFd is for the ioctl CON_MEM_MAP (usually /dev/video)
 *	and the DoConMemMap and DoZero flags are to turn on
 *	those two operations sometimes.
 */
void *
memconmap( void *addr, size_t len, int prot, int flags, int fd,
          off_t off, int ioctlFd, int DoConMemMap, int DoZero )
{
	int cmmRet;
	con_mem_map_req_t ConMemMap;
	void * MapAddr;

	if( ! ServerPID )
		ServerPID = getpid();

	ConMemMap.length = len;
	ConMemMap.address = (paddr_t) addr;
	ConMemMap.proc_id = ServerPID;

	if( DoConMemMap ) {
		cmmRet=ioctl(ioctlFd, CON_MEM_MAPPED, & ConMemMap );
	}

	if( DoZero ) {
		MapAddr = mmap((caddr_t) 0, ConMemMap.length,
			prot, flags, fd, off );
	} else {
		MapAddr = mmap((caddr_t) ConMemMap.address, ConMemMap.length,
			prot, flags, fd, off );
	}

/*
 *	There should never be anything over the MaxMaps limit, if
 *		so, just ignore keeping track of them as something
 *		else is probably much more wrong that just this.
 */
	if ( MapIndex < MaxMaps ) {
		Maps[MapIndex].addr = MapAddr;
		Maps[MapIndex].len = ConMemMap.length;
		++MapIndex;
	}

	return MapAddr;
}

/*
 *	xShowMaps - only for debugging purposes
 */
void
xShowMaps() {
	int i;

	for ( i = 0; i < MapIndex; ++i ) {
ErrorF( "xShowMaps: addr %#x (%d), len %d (%#x)\n", Maps[i].addr, Maps[i].addr, Maps[i].len, Maps[i].len );
	}
}

/*
 *	Call xUnMapAll on screen close to clear all maps
 */
void
xUnMapAll() {
	int i;
	int rtn;
	int videoFd;
	int cmmRet;
	con_mem_map_req_t ConMemMap;

	videoFd = open("/dev/video", O_RDWR);

/*
 *	Doesn't seem to bother anything to do the ioctl on all
 *		the maps even if it wasn't done in the first place.
 */
	if (videoFd >= 0) {
		ConMemMap.length = 0;
		ConMemMap.proc_id = ServerPID;
		for ( i = MapIndex-1; i >= 0; --i ) {
			ConMemMap.address = (paddr_t) Maps[i].addr;
			rtn = munmap( Maps[i].addr, Maps[i].len );
			cmmRet=ioctl(videoFd, CON_MEM_MAPPED, & ConMemMap );
		}
		close(videoFd);
		MapIndex = 0;	/*	All cleared	*/
	}
}	/*	xUnMapAll()	*/

