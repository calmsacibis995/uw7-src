#ident	"@(#)rtpm:nswap.c	1.3.1.1"

#include <sys/types.h>
#include <sys/ksym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/kmem.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <vm/anon.h>
#include <mas.h>
#include <sys/conf.h>
#include <sys/dl.h>
#include <sys/nvm.h>
#include <sys/cpqw.h>

#include "rtpm.h"
#include "mtbl.h"

/*
 * the metric table
 */
extern struct metric mettbl[];
/*
 * EISA bus utilization structure - may not be present on all machines
 */
eisa_bus_util_t eisa_bus_util = { 0, 0, 0 };
dl_t eisa_bus_util_sum;
int eisa_bus_util_sumcnt;
int run_eisa_bus_util_mon;
int eisa_bus_util_flg = 1;

int dsk_swappg;
int mem_swappg;
int dsk_swappgfree;
int totalmem;

int getkval( int fd, char *name, int *retval, size_t size );

void
get_mem_and_swap() {
	static int fd = -2;
	anoninfo_t anoninfo;

	if (fd == -2) 
		fd = open("/dev/kmem", O_RDONLY);
	if( fd < 0 )
		return;
	getkval( fd, "nswappg", &dsk_swappg, sizeof( dsk_swappg ));
	getkval( fd, "nswappgfree", &dsk_swappgfree, sizeof( dsk_swappgfree ));
	getkval( fd, "totalmem", &totalmem, sizeof( totalmem ) );
	getkval( fd, "anoninfo", (int *)&anoninfo, sizeof( anoninfo ) );
	mem_swappg = anoninfo.ani_kma_max;
        if( !eisa_bus_util_flg )
		return;
	if( getkval( fd, "eisa_bus_util_sum", (int *)&eisa_bus_util_sum, sizeof( eisa_bus_util_sum ) ) < 0 ) {
		eisa_bus_util_flg = 0;
	}
	if( getkval( fd, "eisa_bus_util_sumcnt", (int *)&eisa_bus_util_sumcnt, sizeof( eisa_bus_util_sumcnt ) ) < 0 ) {
		eisa_bus_util_flg = 0;
	}
	if( eisa_bus_util.interval_hz == 0 ) {
		if( getkval( fd, "eisa_bus_util", (int *)&eisa_bus_util, sizeof( eisa_bus_util_t )) < 0 )
			eisa_bus_util_flg = 0;
		if( getkval( fd, "run_eisa_bus_util_mon", (int *)&run_eisa_bus_util_mon, sizeof( run_eisa_bus_util_mon )) < 0 )
			eisa_bus_util_flg = 0;
		if( run_eisa_bus_util_mon != ENABLED )
			eisa_bus_util_flg = 0;
	}
}
int
getkval( int fd, char *name, int *retval, size_t size ) {

	struct mioc_rksym rks;

	rks.mirk_symname = name;
	rks.mirk_buf = retval;
        rks.mirk_buflen = size;

	return( ioctl( fd, MIOC_READKSYM, &rks ) );
}
