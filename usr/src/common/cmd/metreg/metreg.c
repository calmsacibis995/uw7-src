#ident	"@(#)metreg:metreg.c	1.7.1.2"

/*
 * metreg: This utility creates a metrics registration file using
 * getksym() and the structure layouts from metrics.h, plocal.h, 
 * and metdisk.h.  Application programs should use the MAS library
 * and this registration file to access metrics.  
 *
 * Note that the code below may be more complex than necessary
 * for application programs.  It consists of a loop that processes
 * tables defined in mettbl.h to build a call to mas_register_met()
 * for each metric.  Functionally, this is the same as a sequence
 * of calls to mas_register_met(), i.e.,
 *
 *	mas_register_met(ID1, "metric1", UNITS1, UNITSNM1, TYPE1, SIZE1,
 *			NUMOBJ1, ADDRESS1, [resource-unit pairs...]);
 *
 *	mas_register_met(ID2, "metric2", UNITS2, UNITSNM2, TYPE2, SIZE2,
 *			NUMOBJ2, ADDRESS2, [resource-unit pairs...]);
 *
 *	...
 *
 * This simpler form may be more appropriate for some applications.
 *
 */

#include <sys/types.h>
#include <vm/kma_p.h>

#include <sys/metrics.h>
#include <sys/plocal.h>
#include <sys/metdisk.h>
#include <sys/ksym.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/kmem.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/mman.h>

#include <mas.h>

#include <metreg.h>
#include "mettbl.h"

#define max( x, y )	((x)>(y)?(x):(y))

#define SAFE_LSEEK(fd, loc, mode) \
do { \
     if (lseek((fd), (loc), (mode)) == -1) { \
	(void)fprintf(stderr, "%s: seek error seeking /dev/kmem %s: %d\n", \
		progname, __FILE__, __LINE__); \
	exit(1); \
     } \
} while(0)

#define SAFE_READ(fd, buf, size) \
do { \
     if (read((fd), (buf), (size)) != (size)) { \
           (void)fprintf(stderr, "%s: read error reading /dev/kmem %s: %d\n", \
		   progname, __FILE__, __LINE__); \
	   exit(1); \
   } \
} while(0)
							   

char *progname;

extern int errno;

struct mets *z;
struct plocalmet **lm;
struct plocalmet *pmet;
struct met_disk_stats **ds;
met_disk_stats_list_t ds_list;
int *lm_off;
caddr_t *segaddr;
uint32 *segsz;
uint32 *segoff;
char **segfn;
int nseg;

static void reg_group(metinfo_t metinfo);
static void reg_metric(finfo_t field, caddr_t loc, 
		       resource_t container_res, units_t container_group,
		       resource_t group_res, units_t group_unit);
static void set_resource(int code, resource_t *res, int *num, 
			 resource_t def_res, int def_num);

/* ARGSUSED */
int
main(int argc, char **argv)
{
	int i, j, fd;
	int nengine, ndisks, pgsz;
	int met_size, dsk_size, ndsk_seg;
	struct mets sys_info;
	unsigned long dummytype;
	unsigned long met_loc = 0;
	unsigned long procaddr_loc_p = 0;
	unsigned long procaddr_loc = 0;
	caddr_t *procaddr = 0;
	unsigned long disk_loc = 0;
	unsigned long temp_disk_loc = 0;
	int	disknum;

	setbuf(stdout, NULL);
	progname = argv[0];
	
	if (getksym("m", &met_loc, &dummytype) == -1) {
		(void)fprintf(stderr, "%s: getksym error = %d\n", progname,errno);
		exit(1);
	}

	if (getksym("met_localdata_ptrs_p", &procaddr_loc_p, &dummytype) == -1) {
		(void)fprintf(stderr, "%s: getksym error = %d\n", progname, errno);
		exit(1);
	}

	if (getksym("met_ds_list", &disk_loc, &dummytype) == -1) {
		(void)fprintf(stderr, "%s: getksym error = %d\n", progname, errno);
		exit(1);
	}

	if ((fd = open(DEVMET, O_RDONLY)) < 0 ) {
		(void)fprintf(stderr,"%s: cannot open %s\n", progname, DEVMET);
		exit(1);
	}

	SAFE_LSEEK(fd, met_loc, SEEK_SET);
	if (read(fd, &sys_info, sizeof(sys_info)) != sizeof(sys_info)) {
		(void)fprintf(stderr,"%s: Couldn't read system information\n", progname);
		exit(1);
	}

	nengine = sys_info.mets_counts.mc_nengines;
	ndisks = sys_info.mets_counts.mc_ndisks;
	pgsz = sys_info.mets_native_units.mnu_pagesz;
	met_size = sizeof(struct mets);
	dsk_size = sizeof(struct met_disk_stats);
	ndsk_seg = ndisks; /* maximum number of disk segments */

	z = malloc(met_size);  /* to make address comparisons in MAS work */
	if (z == NULL) {
		(void)fprintf(stderr, "%s: Couldn't allocate met structure\n", progname);
		exit(1);
	}

	SAFE_LSEEK(fd, met_loc, SEEK_SET);
	SAFE_READ(fd, z, sizeof(*z));

	segfn = malloc((1 + nengine + ndsk_seg) * sizeof(*segfn));
	if( segfn == NULL ) {
		(void)fprintf(stderr,"%s: can't malloc\n", progname);
		exit(1);
	}

	segaddr = malloc((1 + nengine + ndsk_seg) * sizeof(*segaddr));
	if( segaddr == NULL ) {
		(void)fprintf(stderr,"%s: can't malloc\n", progname);
		exit(1);
	}

	segsz = malloc((1 + nengine + ndsk_seg) * sizeof(*segsz));
	if( segsz == NULL ) {
		(void)fprintf(stderr,"%s: can't malloc\n", progname);
		exit(1);
	}

	segoff = malloc((1 + nengine + ndsk_seg) * sizeof(*segoff));
	if( segoff == NULL ) {
		(void)fprintf(stderr,"%s: can't malloc\n", progname);
		exit(1);
	}

	nseg = 1;
	segaddr[0] = (caddr_t) z;
	segsz[0] = met_size;
	segoff[0] = met_loc;

	lm = malloc(nengine * sizeof(*lm));
	if( lm == NULL ) {
		(void)fprintf(stderr,"%s: can't malloc\n", progname);
		exit(1);
	}
	lm_off = malloc(nengine * sizeof(int));
	if( lm_off == NULL ) {
		(void)fprintf(stderr,"%s: can't malloc\n", progname);
		exit(1);
	}

	SAFE_LSEEK(fd, procaddr_loc_p, SEEK_SET);
	SAFE_READ(fd, &procaddr_loc, sizeof(procaddr_loc));

	pmet = malloc(nengine * sizeof(*pmet));
	if (pmet == NULL) {
		(void)fprintf(stderr,"%s: can't malloc\n", progname);
		exit(1);
	}

	procaddr = malloc(nengine * sizeof(*procaddr));
	if (procaddr == NULL) {
		(void)fprintf(stderr,"%s: can't malloc\n", progname);
		exit(1);
	}
		
	SAFE_LSEEK(fd, procaddr_loc + sizeof(int), SEEK_SET);
        SAFE_READ(fd, procaddr, nengine * sizeof(*procaddr));
	for (i = 0; i < (int) z->mets_counts.mc_nengines; i++) {
		segoff[nseg+i] = (uint32) procaddr[i];
		SAFE_LSEEK(fd, (off_t) procaddr[i], SEEK_SET);
		SAFE_READ(fd, pmet + i, sizeof(pmet));
		lm[i] = pmet + i;
		segaddr[nseg+i] = (caddr_t)lm[i];
		segsz[nseg+i] = sizeof(*pmet);
	}

	nseg += (int)z->mets_counts.mc_nengines;

	/* ds contains user addresses for each disk stat and is passed to met_register function */ 
	ds = malloc(ndisks * sizeof(*ds));
	if( ds == NULL ) {
		(void)fprintf(stderr,"%s: can't malloc\n", progname);
		exit(1);
	}

	/* get address of first header of disk structures */
	SAFE_LSEEK(fd, disk_loc, SEEK_SET);
	SAFE_READ(fd, &temp_disk_loc, sizeof(temp_disk_loc));

	disknum = 0;

	while ((temp_disk_loc != 0) && (disknum != ndisks)) {
		vaddr_t	p;
		uint32 endpg;
		struct met_disk_stats *ds_met;
		uint32 pgmsk=~(pgsz-1);

		/* read first header structure */
		SAFE_LSEEK(fd, temp_disk_loc, SEEK_SET);
		SAFE_READ(fd, &ds_list, sizeof(ds_list));

		/* if no disk stats in this block then skip it */
		temp_disk_loc = (unsigned long) ds_list.dl_next;
		if (ds_list.dl_used == 0) {	/* nothing here */
			continue;
		}

		/* in order to map the kernel stats into mas we need to ensure that
		   there is a copy of each page of kernel stats in user space. The easy way is
		   to provide a complete copy of each kernel page in a user page */

		/* set segoff to start of 1st page of stats */
		segoff[nseg]=(uint32)(ds_list.dl_stats)&pgmsk;

		/* end page is the page in which stats finish */
		endpg= ((uint32)ds_list.dl_stats+(ds_list.dl_num*sizeof(*ds_met)))&pgmsk;

		/* segment size is the number of whole pages in which the stats are contained */
		segsz[nseg]=endpg-segoff[nseg]+pgsz;

		/* allocate the user page(s) to hold the stats */
		segaddr[nseg]=memalign(pgsz,segsz[nseg]);
		if (segaddr[nseg] == NULL) {
			(void)fprintf(stderr,"%s: can't malloc\n", progname);
			exit(1);
		}

		/* set ds_met to the same offset as the start of stats in the kernel page */
		ds_met = (struct met_disk_stats*)(segaddr[nseg]+((uint32)(ds_list.dl_stats)&(pgsz-1)));

		nseg++;

		/* seek to start of the disk stats in the kernel */
		SAFE_LSEEK(fd, (off_t) ds_list.dl_stats, SEEK_SET);
		p = (vaddr_t) ds_list.dl_stats;
		for (i = 0; i < ds_list.dl_num; i++) {
			/* read the disk stat into the user area */
			SAFE_READ(fd, ds_met + i, sizeof(*ds_met));

			/* check the disk stat is used */
			if (ds_met[i].ds_name[0] != '\0') {
				ds[disknum] = ds_met + i;
				if (++disknum == ndisks) {
				  	break;
				}
			}
			p += sizeof(*ds_met);
		}
	}

/*
 *	set segment file names
 */
	for( i = 0 ; i < nseg ; i++ )
		segfn[i] = DEVMET;

	(void)unlink(MAS_FILE);

	if (mas_init(MODE, MAS_FILE, nseg, segfn, segaddr, segsz, segoff, 
		     (char *) 0, (char *) 0, (char *) 0) < 0) {
		(void)fprintf(stderr,"%s: mas_init failed\n", progname);
		exit(1);
	}
	
	for (i = 0; i < metinfo_size; i++) {
		reg_group(metinfo[i]);
	}

	if (mas_put() != 0) {
		(void)fprintf(stderr, "%s: Error return from mas_put()\n", progname);
		exit(1);
	}
	return(0);
}


static void
reg_group(metinfo_t metinfo)
{
	int	container_limit;
	int	group_limit;
	resource_t	container_res;
	resource_t	group_res;
	caddr_t	base;
	int	i;
	int	j;
	int	k;

	switch (metinfo.container) {
	      case PT_SYS_STRUCT:
		container_limit = 1;
		container_res = (resource_t) 0;
		break;
	      case PT_PROC_STRUCT:
		container_limit = z->mets_counts.mc_nengines;
		container_res = (resource_t) NCPU;
		break;
	      case PT_DISK_STRUCT:
		container_limit = z->mets_counts.mc_ndisks;
		container_res = (resource_t) NDISK;
		break;
	      default:
		(void)fprintf(stderr, "%s: Unknown container code %d\n", progname, 
			metinfo.container);
		exit(1);
		break;	
	}

	set_resource(metinfo.repflags, &group_res, &group_limit,
		     (resource_t) 0, 1);

	for (i = 0; i < container_limit; i++) {
		switch (metinfo.container) {	
		      case PT_SYS_STRUCT:
			base = (caddr_t) z;
			break;
		      case PT_PROC_STRUCT:
			base = (caddr_t) lm[i];
			break;
		      case PT_DISK_STRUCT:
			base = (caddr_t) ds[i];
			break;
		}

		for (j = 0; j < group_limit; j++) {
			for (k = 0; k < metinfo.nfields; k++) {
				reg_metric(metinfo.fields[k], 
					   (caddr_t) base + metinfo.offset
					   + j * metinfo.size,
					   container_res, (units_t) i,
					   group_res, (units_t) j);
			}
		}
	}
}


#define SWAP_RESP(x, y) { resource_t *tmp = x; x = y; y = tmp; }
#define SWAP_UNITP(x, y) { units_t *tmp = x; x = y; y = tmp; }

static void
reg_metric(finfo_t field, caddr_t loc, 
	   resource_t container_res, units_t container_unit,
	   resource_t group_res, units_t group_unit)
{
	resource_t field_res;
	int field_limit;
	int i;
	resource_t *resource[3];
	units_t	*unit[3];

	set_resource(field.repflags, &field_res, &field_limit, 
		     (resource_t) field.repflags, 1);


	/* 
	 * Rearrange pairs so that resource = 0 pairs are last.
	 * One of the unit numbers is the index 'i' of the for loop
	 * loop below.  Because of this, resource[] and unit[] are
	 * arrays of pointers so that the pairs may be rearranged
	 * outside the for loop.
	 * We could do the container/group swap in reg_group,
	 * but since we would have to check against the field_resource
	 * it wouldn't buy us much (and would introduce rearranging
	 * in two places).
	 *
	 */

	resource[0] = &container_res;
	resource[1] = &group_res;
	resource[2] = &field_res;
	unit[0] = &container_unit;
	unit[1] = &group_unit;
	unit[2] = &i;

	if (*resource[0] == 0 && *resource[1] != 0) {
		SWAP_RESP(resource[0], resource[1]);
		SWAP_UNITP(unit[0], unit[1]);
	}
	if (*resource[1] == 0 && *resource[2] != 0) {
		SWAP_RESP(resource[1], resource[2]);
		SWAP_UNITP(unit[1], unit[2]);
		if (*resource[0] == 0 && *resource[1] != 0) {
			SWAP_RESP(resource[0], resource[1]);
			SWAP_UNITP(unit[0], unit[1]);
		}
	}

	for (i = 0; i < field_limit; i++) {
		(void)mas_register_met(field.id, field.name, field.units,
				 field.unitsnm, field.mettype, 
				 field.obj_sz, field.nobj,
				 loc + field.obj_off + i * field.obj_sz *
				 field.nobj, 
				 *resource[0], *unit[0],
				 *resource[1], *unit[1],
				 *resource[2], *unit[2], 0, 0);
	}
}



static void
set_resource(int code, resource_t *res, int *num, 
	     resource_t def_res, int def_num)
{
	switch (code) {
	      case PT_BY_PROC:
		*num = (resource_t) z->mets_counts.mc_nengines;
		*res = NCPU;
		break;
	      case PT_BY_DISK:
		*num = (resource_t) z->mets_counts.mc_ndisks;
		*res = NDISK;
		break;
	      case PT_BY_MEMSIZE:
		*num = (resource_t) z->mets_kmemsizes.msk_numpools;
		*res = KMPOOLS;
		break;
	      case PT_BY_FS:
		*num = z->mets_fstypes.msf_numtypes; 
		*res = (resource_t) NFSTYP;
		break;
	      default:
		*num = def_num;
		*res = def_res;
		break;
	}
}
