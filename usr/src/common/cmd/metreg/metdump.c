#ident	"@(#)metreg:metdump.c	1.1"

/*
 *	dump the contents of a metric access support file
 *	and all of its associated metrics
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/dl.h>
#include <mas.h>
#include <assert.h>

#define MAS_FILE	"/var/adm/metreg.data"
#define MAX_RES		9

void	dump_mas_head(int md);
void	dump_mrt(int md, uint32 acc);
void	dump_met(int md, metid_t id);
void	qdump_met(int md, metid_t id);
void	dump_res_met(int md, metid_t id, resource_t *resources, int level);
uint32	size_convert( caddr_t obj_p, uint32 *objsz );

int
main( int argc, char **argv ) 
{
	char *masfile;		/* metric access file to open		*/
	int md;			/* metric descriptor ret from mas_open	*/
	uint32 tst;		/* set of acc methods to run (read|mmap)*/
	uint32 acc;		/* access method requested (read|mmap)	*/
	extern char *optarg;	/* optional argument (for getopt)	*/
	extern int optind;	/* opt arg index (for getopt)		*/
	int c;			/* option char returned from getopt	*/
	int rflg, mflg;		/* flags for whether to do read | mmap	*/
	int errflg;		/* flag whether error was found in args	*/
	int i;			/* loop variable			*/

	rflg = mflg = errflg = tst = 0;

	while(( c = getopt( argc, argv, "rm" ) ) != EOF ) {
		switch( c ) {
		case 'r':
			if( rflg++ )
				errflg++;
			tst |= MAS_READ_ACCESS;
			break;
		case 'm':
			tst |= MAS_MMAP_ACCESS;
			if( rflg++ )
				errflg++;
			break;
		default:
			errflg++;
			break;
		}
	}

	if( !rflg && !mflg )
		tst = MAS_READ_ACCESS | MAS_MMAP_ACCESS;
			
	if( errflg || optind < argc-1 ) {
		(void)fprintf(stderr,"usage: mapex [-rm] [file]\n");
		exit(1);
	}

	if( optind < argc )
		masfile = argv[optind];
	else
		masfile = MAS_FILE;

	while( tst ) {
		if( tst & MAS_MMAP_ACCESS ) {
			(void)printf("Testing metric access support via mmap\n");
			tst &= ~MAS_MMAP_ACCESS;
			acc = MAS_MMAP_ACCESS;
		} else if( tst & MAS_READ_ACCESS ){
			(void)printf("Testing metric access support via read\n");
			tst &= ~MAS_READ_ACCESS;
			acc = MAS_READ_ACCESS;
		}
/*
 *		open (and possibly map in) the metric access file
 */
		if( ( md = mas_open( masfile, acc ) ) < 0 ) {
			(void)fprintf(stderr,"mas_open failed\n");
			mas_perror();
			exit(1);
		}
/*
 *		print information from the mas header structure
 */
		dump_mas_head( md );
/*
 *		print all of the metrics
 */
		dump_mrt( md, acc );
/*
 *		close (and possibly unmap) the metric access file
 */
		if( mas_close( md ) < 0 ) {
			(void)fprintf(stderr,"mas_close failed\n");
			mas_perror();
			exit(1);
		}
	}
	(void)printf("done\n");
	exit(0);
}

/*
 *	dump_mrt:	print the metadata and value(s) for every 
 *			metric registered in the metric registration table
 *			
 *	args:		md - metric descriptor returned from mas_open
 *			acc - access method requested
 */
void
dump_mrt( int md, uint32 acc ) {

	int i;			/* loop variable			*/
	uint32 *nmrt_p;		/* ptr to num of entries in met reg tbl	*/
	metid_t *id_p;		/* pointer to metric id number 		*/
/*
 *	if read access, read the metrics
 */
	if( acc == MAS_READ_ACCESS ) {
		if( mas_snap( md ) < 0 ) {
			(void)fprintf(stderr,"mas_snap failed\n");
			mas_perror();
			exit(1);
		}
	}
/*
 *	get the number of metrics that have been registered
 */
	if( !(nmrt_p = mas_get_nmrt(md) ) ) {
		(void)fprintf(stderr,"mas_get_nmrt failed\n");
		mas_perror();
		exit(1);
	}
/*
 *	For each metric in the registration table, print
 *	out its descriptive information and value.  If
 *	using mmap access, the value will be instantaneous.
 *	If using read access, the value will be from the time 
 *	that mas_snap() was issued.
 *
 *	If a snapshot of the metrics is desired while using 
 *	mmap access, mas_snap() can be used copy metrics to the 
 *	snap buffer.  The addresses of the metrics within the
 *	snap buffer can be determined from mas_get_met_snap().
 */
	for( i = 0; i < *nmrt_p ; i++ ) {
/*
 *		get the id number for the metric in the i'th slot
 *		within the metric registration table.
 */
		if( !(id_p = mas_get_met_id(md,i) ) ) {
			(void)fprintf(stderr,"mas_get_med_id failed\n");
			mas_perror();
			exit(1);
		}
/*
 *		print everything there is to know about this metric
 */
		dump_met( md, *id_p );
		(void)printf("\n");
	}
}

/*
 *	dump_met:	print all of the available metric information
 *			for a single metric id
 *			
 *	args:		md - metric descriptor returned from mas_open
 *			id - id number of the metric to print
 */
void
dump_met(int md, metid_t id)
{
	char *name;			/* the name of the metric 	*/
	units_t units, *units_p;	/* the units of the metric	*/
	name_t unitsnm;			/* units descriptive text	*/
	type_t mettype, *mettype_p;	/* type field of the metric	*/
	resource_t *resource;		/* the resource list of the met	*/
	uint32 objsz, *objsz_p;		/* size of each element in met 	*/
	uint32 nobj, *nobj_p;		/* num of elements >1 then array*/
	uint32 nlocs, *nlocs_p;		/* total number of instances	*/
	uint32 status, *status_p;	/* status word (update|avail)	*/
	int i;				/* loop variable		*/
	int ninstance;			/* count of instances		*/
	resource_t res;			/* a resource			*/
	caddr_t resource_p;		/* ptr to the resource metric	*/
	uint32 ressz, *ressz_p;		/* size of the resource met	*/

	(void)printf("\nid:        %d\n", id );

	if( !(name = mas_get_met_name( md, id ) )) {
		(void)fprintf(stderr,"mas_get_met_name failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("name:      %s\n", name );

	if( !(status_p = mas_get_met_status( md, id ))) {
		(void)fprintf(stderr,"mas_get_met_status failed\n");
		mas_perror();
		exit(1);
	}
	status = *status_p;
	(void)printf("status:    %d\n", status );

	if( !(units_p = mas_get_met_units( md, id ))) {
		(void)fprintf(stderr,"mas_get_met_units failed\n");
		mas_perror();
		exit(1);
	}
	units = *units_p;

	if( !(unitsnm = mas_get_met_unitsnm( md, id ))) {
		(void)fprintf(stderr,"mas_get_met_unitsnm failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("units:     %d (%s)\n", units, unitsnm );

	if( !(mettype_p = mas_get_met_type( md, id ))) {
		(void)fprintf(stderr,"mas_get_met_type failed\n");
		mas_perror();
		exit(1);
	}
	mettype = *mettype_p;
	(void)printf("type:      %d\n", mettype );

	if( !(objsz_p = mas_get_met_objsz( md, id ))) {
		(void)fprintf(stderr,"mas_get_met_objsz failed\n");
		mas_perror();
		exit(1);
	}
	objsz = *objsz_p;
	(void)printf("objsz:     %d\n", objsz );

	if( !(nobj_p = mas_get_met_nobj( md, id ))) {
		(void)fprintf(stderr,"mas_get_met_nobj failed\n");
		mas_perror();
		exit(1);
	}
	nobj = *nobj_p;
	(void)printf("nobj:      %d\n", nobj );

/*
 *	get the number of instances that libmas thinks it knows about
 */
 	if( !(nlocs_p = mas_get_met_nlocs( md, id ))) {
		(void)fprintf(stderr,"mas_get_met_nlocs failed\n");
		mas_perror();
		exit(1);
	}
	nlocs = *nlocs_p;
	(void)printf("nlocs:     %d\n", nlocs );
/*
 *	get the resource list for the metric
 */
	if( !(resource = mas_get_met_resources( md, id ))) {
		(void)fprintf(stderr,"mas_get_met_resources failed\n");

		mas_perror();
		exit(1);
	}
/*
 *	for each resource in the resource list, get the value and size
 *	of the resource.  calculate the total number of instances based
 *	on res[1] * res[2] * ... * res[n].  This should match nlocs above.
 */
	for( i=0, ninstance = 1; *resource; i++, resource++ ) {
/*
 *		get the address of the resource
 */
		if( !(resource_p = mas_get_met( md, *resource, 0 ))) {
			(void)fprintf(stderr,"mas_get_met of resource failed\n");
			mas_perror();
			exit(1);
		}
/*
 *		get the size of the resource
 */
	 	if( !(ressz_p = mas_get_met_objsz( md, (metid_t)(*resource) ))) {
			(void)fprintf(stderr,"mas_get_met_objsz of resource failed\n");
			mas_perror();
			exit(1);
		}
/*
 *		assign the resource based on its size
 */
		res = (resource_t)size_convert( resource_p, ressz_p );
		(void)printf("resrc[%02d]: %d\n", i, res );
/*
 *		get the name of the resource
 */
		if( !(name = mas_get_met_name( md, *resource ))) {
			(void)fprintf(stderr,"mas_get_met_name failed\n");
			mas_perror();
			exit(1);
		}
		(void)printf("resrc_nm:  %s\n",name);
		ninstance *= (int)res;
	}
	(void)printf("ninstance: %d instances\n",ninstance);
	if( ninstance != nlocs )
		(void)printf(">>> resource counts do not match\n");

/*
 *	print out all of the instances of the metric based on the 
 *	resource list.  This is done with a recursive function,
 *	since the number of resources can vary from metric to metric.
 */
	if( !(resource = mas_get_met_resources( md, id ))) {
		(void)fprintf(stderr,"mas_get_met_resources failed\n");
		mas_perror();
		exit(1);
	}
	dump_res_met( md, id, resource, 0 );
}

/*
 *	dump_res_met:	recursively run through resource list and get
 *			all possible values for a single metric id
 *			
 *	args:		md - metric descriptor returned from mas_open
 *			id - id number of the metric to print
 *			resources - resource list for the metric
 *			level - depth of recursion
 */
void
dump_res_met(int md, metid_t id, resource_t *resources, int level)
{
	static uint32 ulst[ MAX_RES ];		/* resource values	*/
	static char unames[ MAX_RES ][ 64 ];	/* resource names	*/
	int i;					/* loop variable	*/
	name_t name;				/* name of the metric	*/
	units_t *units_p;			/* units of the metric	*/
	char *ustr;				/* units string		*/
	caddr_t metric_p;			/* ptr to the metric	*/
	uint32 sz, *sz_p;			/* size of the metric	*/
	uint32 nobj, *nobj_p;			/* num of elements	*/
	int met;				/* int for printing met	*/
	char *resname;				/* resource name	*/
	resource_t res;				/* resource id number	*/
/*
 *	check to make sure we are not in too deep
 */
	if( level >= MAX_RES )  {
		(void)fprintf(stderr,"too many resources, quitting.\n");
		exit(1);
	}
/*
 *	check to see if we are at the end of the resource list.
 *	if so, print the information for this instance of the metric.
 */
	if( !mas_resource_cmp( resources, MAS_NATIVE ) 
	  || ((level == 0) && !mas_resource_cmp( resources, MAS_SYSTEM ) ) ) {
/*
 *		print the name of the metric
 */
		if( !(name = mas_get_met_name( md, id ))) {
			(void)fprintf(stderr,"mas_get_met_name failed\n");
			mas_perror();
			exit(1);
		}
		(void)printf("%s:",name);
/*
 *		for each resource in the list, print the resource
 *		name with a suffix to indicate the resource number.
 *		for a per filesys and per cpu metric, this might
 *		yield something like:
 *
 *			[filesys_2][cpu_1]:
 */
		ulst[ level ] = 0;
		for( i = 0 ; i < level ; i++ ) {
			(void)printf("[%s_%d]",unames[i],ulst[i]);
			if( (i+1) < level )
				(void)printf(",");
		}
		if( level ) (void)printf(": ");
/*
 *		get the units of the metric
 */
		if( !(units_p = mas_get_met_units( md, id ))) {
			(void)fprintf(stderr,"mas_get_met_units failed\n");
			mas_perror();
			exit(1);
		}
/*
 *		get the address of the metric.  if the metric is a text
 *		string, print it as such.  otherwise detremine how to
 *		print it based on it's size.
 */
		assert( MAX_RES <= 9 ); /* 8 args to mas_get_met below */

		metric_p = mas_get_met( md, id, ulst[0], ulst[1], ulst[2],
		  ulst[3], ulst[4], ulst[5], ulst[6], ulst[7], ulst[8] );

		if( !metric_p ) {
			(void)printf("<unregistered instance> ");
		}
		else {
		 	if( !(sz_p = mas_get_met_objsz( md, id ))){
				(void)fprintf(stderr,
				  "mas_get_met_objsz failed\n");
				mas_perror();
				exit(1);
			}
			sz = *sz_p;

			if( !(nobj_p = mas_get_met_nobj( md, id ))) {
				(void)fprintf(stderr,"mas_get_met_nobj failed\n");
				mas_perror();
				exit(1);
			}
			nobj = *nobj_p;

			if( sz == 1 && nobj > 1) {
/*
 *				assume it's a string
 */
				(void)printf("%s ",metric_p);
			}
			else {
/*
 *				assume it's numeric and fits in an int
 */
				met = (int)size_convert( metric_p, sz_p );
				(void)printf("%d ", met );
			}
		}

		ustr = mas_get_met_unitsnm( md, id );
		if( ustr && *ustr )
			(void)printf("(%s)", ustr );
		(void)printf("\n");
		return;
	}
	else {
		if( !(sz_p = mas_get_met_objsz( md, (metid_t)(*resources) ))) {
			(void)fprintf(stderr,"mas_get_met_objsz failed\n");
			mas_perror();
			exit(1);
		}

		if( !(metric_p = mas_get_met( md, *resources, 0 ))) {
			(void)fprintf(stderr,"mas_get_met of resource failed\n");
			mas_perror();
			exit(1);
		}
/*
 *		assign the resource based on its object size
 */
		res = (resource_t)size_convert( metric_p, sz_p );
/*
 *		get the resource name
 */
		if( !(resname = mas_get_met_name(md, *resources))) {
			(void)fprintf(stderr,
			  "mas_get_met_name of resource failed\n");
			mas_perror();
			exit(1);
		}
		(void)strcpy(unames[ level ], resname);

		resources++;
		for( i=0; i < res; i++ ) {
			ulst[ level ] = i;
			dump_res_met( md, id, resources, level+1 );
		}
	}
}
/*
 *	size_convert:	convert metric values into uint32
 *			
 *	args:		obj_p - pointer to the metric
 *			objsz - pointer to the size of the metric
 *
 *	return value:	metric converted to unit32
 */
uint32
size_convert( caddr_t obj_p, uint32 *objsz )
{
	uint32 sz = *objsz;
	uint32 ret;

	if( sz == sizeof(int) )
		/* LINTED pointer alignment */
		ret = ( (uint32)*((int *)obj_p) );
	else if( sz == sizeof(short) )
		/* LINTED pointer alignment */
		ret = ( (uint32)*((short *)obj_p) );
	else if( sz == sizeof(long) )
		/* LINTED pointer alignment */
		ret = ( (uint32)*((long *)obj_p) );
	else if( sz == sizeof(char) )
		ret = ( ((uint32)*obj_p)&0xff );
	else if( sz == sizeof(dl_t) )	/* double long, take low part */
		/* LINTED pointer alignment */
		ret = (uint32)(((dl_t *)obj_p)->dl_lop);
	else {
		(void)fprintf(stderr,"unsupported object size\n");
		exit(1);
	}
	return( ret );
}

/*
 *	dump_mas_head:	print all of the information in the 
 *			mas header struct and the mrt header struct
 *			all possible values for a single metric id
 *			
 *	args:		md - metric descriptor returned from mas_open
 */
void
dump_mas_head(int md)
{
	int i;
	name_t name;
	uint32 *u_p;
	caddr_t addr;
	uint32 nseg;

	if( !(name = mas_get_mas_filename(md))) {
		(void)fprintf(stderr,"mas_get_mas_filename failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas file:%s\n",name);

	if( !(u_p = mas_get_mas_magic(md))) {
		(void)fprintf(stderr,"mas_get_mas_magic failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas magic:%d\n",*u_p);

	if( !(u_p = mas_get_mas_status(md))) {
		(void)fprintf(stderr,"mas_get_mas_status failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas status:%d\n",*u_p);

	if( !(u_p = mas_get_bpw(md))) {
		(void)fprintf(stderr,"mas_get_bpw failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas bpw:%d\n",*u_p);

	if( !(u_p = mas_get_byte_order(md))) {
		(void)fprintf(stderr,"mas_get_byte_order failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas byteorder:0x%x\n",*u_p);

	if( !(u_p = mas_get_head_sz(md))) {
		(void)fprintf(stderr,"mas_get_head_sz failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas head sz:%d\n",*u_p);

	if( !(u_p = mas_get_access_methods(md))) {
		(void)fprintf(stderr,"mas_get_access_methods failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas access methods:%d\n",*u_p);

	if( !(addr = mas_get_mas_start_addr(md))) {
		(void)fprintf(stderr,"mas_get_mas_start_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas start addr:%p\n",(void *)addr);

	if( !(addr = mas_get_mas_end_addr(md))) {
		(void)fprintf(stderr,"mas_get_mas_end_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas end addr:%p\n",(void *)addr);

	if( !(u_p = mas_get_nseg(md))) {
		(void)fprintf(stderr,"mas_get_nseg failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas metrics segments:%d\n",*u_p);

	for( nseg = *u_p, i = 0; i < nseg; i++ ) {

		if( !(name = mas_get_metrics_filename(md,i))) {
			(void)fprintf(stderr,
			  "mas_get_metrics_filename failed\n");
			mas_perror();
			exit(1);
		}
		(void)printf("mas metrics file[%d]:%s\n",i,name);

		if( !(addr = mas_get_metrics_start_addr(md,i))) {
			(void)fprintf(stderr,
			  "mas_get_metrics_start_addr failed\n");
			mas_perror();
			exit(1);
		}
		(void)printf("mas metrics start addr[%d]:%p\n",i,(void *)addr);

		if( !(addr = mas_get_metrics_end_addr(md,i))) {
			(void)fprintf(stderr,
			  "mas_get_metrics_end_addr failed\n");
			mas_perror();
			exit(1);
		}
		(void)printf("mas metrics end addr[%d]:%p\n",i,(void *)addr);
	}

	if( !(name = mas_get_strings_filename(md))) {
		(void)fprintf(stderr,"mas_get_strings_filename failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas strings file:%s\n",name);

	if( !(addr = mas_get_strings_start_addr(md))) {
		(void)fprintf(stderr,"mas_get_strings_start_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas strings start addr:%p\n",(void *)addr);

	if( !(addr = mas_get_strings_end_addr(md))) {
		(void)fprintf(stderr,"mas_get_strings_end_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas strings end addr:%p\n",(void *)addr);

	if( !(addr = mas_get_mrt_hdr_start_addr(md))) {
		(void)fprintf(stderr,"mas_get_mrt_hdr_start_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas mrt_hdr start addr:%p\n",(void *)addr);

	if( !(addr= mas_get_mrt_hdr_end_addr(md))) {
		(void)fprintf(stderr,"mas_get_mrt_hdr_end_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas mrt_hdr end addr:%p\n",(void *)addr);

	if( !(name = mas_get_mr_tbl_filename(md))) {
		(void)fprintf(stderr,"mas_get_mr_tbl_filename failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas mr_tbl file:%s\n",name);

	if( !(addr = mas_get_mr_tbl_start_addr(md))) {
		(void)fprintf(stderr,"mas_get_mr_tbl_start_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas mr_tbl start addr:%p\n",(void *)addr);

	if( !(addr = mas_get_mr_tbl_end_addr(md))) {
		(void)fprintf(stderr,"mas_get_mr_tbl_end_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas mr_tbl end addr:%p\n",(void *)addr);

	if( !(name = mas_get_metadata_filename(md))) {
		(void)fprintf(stderr,"mas_get_metadata_filename failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas metadata file:%s\n",name);

	if( !(addr = mas_get_metadata_start_addr(md))) {
		(void)fprintf(stderr,"mas_get_metadata_start_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas metadata start addr:%p\n",(void *)addr);

	if( !(addr = mas_get_metadata_end_addr(md))) {
		(void)fprintf(stderr,"mas_get_metadata_end_addr failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("mas metadata end addr:%p\n",(void *)addr);

	if( !(u_p = mas_get_mrt_sz(md))) {
		(void)fprintf(stderr,"mas_get_mrt_sz failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("size of mrt entry:%d\n",*u_p);

	if( !(u_p = mas_get_mrt_hdr_sz(md))) {
		(void)fprintf(stderr,"mas_get_mrt_hdr_sz failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("size of mrt_hdr:%d\n",*u_p);

	if( !(u_p = mas_get_nmrt(md))) {
		(void)fprintf(stderr,"mas_get_nmrt failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("number of entries in mrt:%d\n",*u_p);

	if( !(u_p = mas_get_id_sz(md))) {
		(void)fprintf(stderr,"mas_get_id_sz failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("size of metid_t:%d\n",*u_p);

	if( !(u_p = mas_get_units_sz(md))) {
		(void)fprintf(stderr,"mas_get_units_sz failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("size of units:%d\n",*u_p);

	if( !(u_p = mas_get_resource_sz(md))) {
		(void)fprintf(stderr,"mas_get_resource_sz failed\n");
		mas_perror();
		exit(1);
	}
	(void)printf("size of resource_t:%d\n",*u_p);
}
