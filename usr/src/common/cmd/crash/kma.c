#ident	"@(#)crash:common/cmd/crash/kma.c	1.2.1.1"

/*
 * This file contains code for the crash function kmastat.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include "crash.h"

static vaddr_t	Kmeminfo, Km_pools;
static void	prkmastat();

struct	kmeminfo kmeminfo;
int	km_pools[KMEM_NCLASS];

/* get arguments for kmastat function */
int
getkmastat()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	(void) redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind])
		longjmp(syn,0);
	prkmastat();
	return(0);
}

/* print kernel memory allocator statistics */
static void
prkmastat()
{
	if(!Km_pools) {
		Kmeminfo = symfindval("kmeminfo");
		Km_pools = symfindval("Km_pools");
	}

	readmem(Kmeminfo,1,-1,&kmeminfo,
		sizeof(struct kmeminfo),"kmeminfo");
	readmem(Km_pools,1,-1,&km_pools,
		sizeof(km_pools),"km_pools");

	(void) fprintf(fp,
	"                       total bytes     total bytes\n");
	(void) fprintf(fp,
	"size        # pools       in pools       allocated     # failures\n");
	(void) fprintf(fp,
	"-----------------------------------------------------------------\n");

	(void) fprintf(fp,
	"small       %3u         %10u      %10u     %3u\n",
		km_pools[KMEM_SMALL], kmeminfo.km_mem[KMEM_SMALL],
		kmeminfo.km_alloc[KMEM_SMALL], kmeminfo.km_fail[KMEM_SMALL]);
	(void) fprintf(fp,
	"big         %3u         %10u      %10u     %3u\n",
		km_pools[KMEM_LARGE], kmeminfo.km_mem[KMEM_LARGE],
		kmeminfo.km_alloc[KMEM_LARGE], kmeminfo.km_fail[KMEM_LARGE]);
	(void) fprintf(fp,
	"outsize       -                  -      %10u     %3u\n",
		kmeminfo.km_alloc[KMEM_OSIZE], kmeminfo.km_fail[KMEM_OSIZE]);
}
