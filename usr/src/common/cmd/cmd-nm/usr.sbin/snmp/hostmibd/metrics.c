#ident	"@(#)metrics.c	1.2"
/*
 *	metcook.c	gather a subset of the sar information,
 *			compute interval data, and print it.
 *
 *	the top level functions which may be extracted are:
 *
 *		open_mets - open met registration file and metrics
 *		  memory map everything into user space.	
 *		snap_mets - take a snapshot of the raw metric data
 *		calc_interval_data - cook the raw data and copy
 *		  the snapshot to a holding area so it can be
 *		  compared against the next sample.
 *		close_mets - close the metric registration files
 *		print_mets - print the cooked metric values
 *
 *	all of the other functions are internal to this program
 *	for supporting the above set.
 */

#include "hostmibd.h"



/*
 *	defs for the top level functs
 */
int	open_mets( void ); 
void	close_mets( void ); 
int	snap_mets( void ); 
void	calc_interval_data( void ); 
int	 get_ncpu( void ); 


/*
 *  ... and everything else
 */
static int	 get_hz( void ); 
static void	 alloc_mets( void ); 
static void	 cook_metric( struct met *metp, int count ); 
static void	 cook_dblmetric( struct dblmet *metp, int count ); 
static void	 check_resource( metid_t id, metid_t resid );
static void	 check_size( metid_t id, int size );
static void	 metalloc( struct met **metp, int count ); 
static void	 dblmetalloc( struct dblmet **metp, int count ); 
static void	 metset( struct met *metp, metid_t id, int count);
static void	 dblmetset(struct dblmet *metp, metid_t id, int count);
static void	 alloc_met( metid_t id, resource_t resource, int cnt,
			   int size, struct met **metp); 
static void	 alloc_dblmet( metid_t id, resource_t resource, int cnt, 
			      int size, struct dblmet **metp); 
static uint32	size_convert( caddr_t obj_p, uint32 *objsz );
static void	free_mets( void ); 

/*
 * timing information
 */
struct met *timeval;
static clock_t currtime;
static long freeswptime;
#define tdiff timeval->cooked
static struct tbuffer 
{	/*  need a tbuf to pass to times() */
  long user;
  long sys;
  long cuser;
  long csys;
} tbuf;

/*
 * some misc variables we need to know about
 */
static int md;			/* metric descriptor ret from mas_open	*/
int ncpu = 0;		/* number of CPUs			*/
int hz = 0;		/* native machine clock rate		*/

/*
 *	get the number of processors on the system
 */
int get_ncpu(void) 
{
  caddr_t metaddr;
  uint32 *metsz_p;

  metaddr = mas_get_met( md, NCPU, 0 );
  if( !metaddr ) 
    {
      fprintf(stderr,"number of cpus is not registered\n");
      exit(1);
    }
  metsz_p = mas_get_met_objsz( md, NCPU );
  if( !metsz_p 
     || ( *metsz_p != sizeof(short) && *metsz_p != sizeof(int)) ) 
    {
      fprintf(stderr,"can't determine size of ncpu\n");
      exit(1);
    }
  return size_convert( metaddr, metsz_p );
}

/*
 *	get the lbolt clock rate
 */
static int get_hz(void) 
{
  caddr_t metaddr;
  uint32 *metsz_p;

  metaddr = mas_get_met( md, HZ_TIX, 0 );
  if( !metaddr ) 
    {
      fprintf(stderr,"value of HZ is not registered\n");
      exit(1);
    }
  metsz_p = mas_get_met_objsz( md, HZ_TIX );
  if( !metsz_p 
     || ( *metsz_p != sizeof(short) && *metsz_p != sizeof(int)) ) 
    {
      fprintf(stderr,"can't determine size of hz\n");
      exit(1);
    }
  return size_convert( metaddr, metsz_p );
}

/*
 *	allocate some struct mets for everything we care about
 */
static void alloc_mets(void) 
{
  /* allocate timer metric */
  metalloc( &timeval, 1 );
  timeval->met_p = (caddr_t)(&currtime);
  tdiff=0;

  /* get number of cpus */
  ncpu=get_ncpu();

  /* get the value of HZ */
  hz=get_hz();

  /* per process cpu times - idle */
  alloc_met( MPC_CPU_IDLE, NCPU, ncpu, sizeof( int ), &idl_time);

  /* global freemem and freewap pages */
  alloc_dblmet( FREEMEM, MAS_SYSTEM, 1, sizeof( dl_t ), &freemem);
  alloc_dblmet( FREESWAP, MAS_SYSTEM, 1, sizeof( dl_t ), &freeswp);

  /* allocate calculated metrics, computed by calc_interval_data */
}

/*
 *	free everything allocated by alloc_mets
 */
static void free_mets(void) 
{
  free( idl_time );

  free( freemem );
  free( freeswp );
  free( timeval );
}

/*
 *	convert values into uint32 via run time size binding
 */
static uint32
size_convert( caddr_t obj_p, uint32 *objsz )
{
  uint32 sz = *objsz;
  uint32 ret;

  if( sz == sizeof(int) )
    /* LINTED pointer alignment */
    ret = ( (uint32)*((int *)obj_p) );
  else 
    if( sz == sizeof(short) )
      /* LINTED pointer alignment */
      ret = ( (uint32)*((short *)obj_p) );
    else 
      if( sz == sizeof(long) )
	/* LINTED pointer alignment */
	ret = ( (uint32)*((long *)obj_p) );
      else 
	if( sz == sizeof(char) )
	  ret = ( ((uint32)*obj_p)&0xff );
	else 
	  {
	    (void)fprintf(stderr,"unsupported object size\n");
	    exit(1);
	  }
  return( ret );
}

/*
 *	compute the traditional sar data from for the set of counters 
 * 	declared above.
 */
static void
calc_interval_data( void ) 
{
  timeval->intv = (double)(currtime - (clock_t)timeval->met);
  timeval->met = currtime;
  tdiff = timeval->intv / (double)hz;
  
  cook_metric( idl_time, ncpu );

  cook_dblmetric( freemem, 1 );
  /*
   *	freemem needs some special attention, because it is summed every
   *	clock tick.  It's already been divided by the time in seconds,
   *	we just need to divide it by hz to get the denominator to be the 
   *	number of samples.
   */

  freemem->cooked = freemem->cooked / hz;

  /*
   *	freeswap needs some attention too.  cook_metric will
   *	divide it by the time in seconds, but we want to 
   *	divide by the integer number of seconds since we
   *	last did the calculation.  This corresponds to the 
   *	sample count, since freeswp is summed every second.
   * 	However, we have to be careful not to divide by zero
   *	if the sample rate (tdiff) is less than 1 second.
   */
  {	
    int nsamples = (currtime/hz) - (freeswptime/hz);
    if( nsamples >= 1 ) 
      {
	cook_dblmetric( freeswp, 1 );
	freeswp->cooked = freeswp->intv/((double)nsamples);
	freeswptime = currtime;
      }
  }
}

/*
 *	calculate interval data for a metric and compute rate.
 */
static void
cook_metric( struct met *metp, int count ) 
{
  int i;
  
  assert(tdiff != 0.0);
  assert( metp && metp->met_p );
  
  for( i = 0; i < count ; i++ ) 
    {
      metp[i].intv = (double)(*((int *)(metp[i].met_p)) - (metp[i].met));
      metp[i].cooked = metp[i].intv/tdiff;
      metp[i].met = *((int *)(metp[i].met_p));
    }
}

/*
 *	calculate interval and rate data for double long metrics
 *	(the only double longs are freemem and freeswap)
 */
static void
cook_dblmetric( struct dblmet *metp, int count ) 
{
  int i;
  assert( metp && metp->met_p );
  assert( tdiff != 0.0 );
  assert( hz != 0 );
  
  for( i = 0; i < count ; i++ ) 
    {
      dl_t answer;
      dl_t lsub();
      
      answer = lsub(*((dl_t *)(metp[i].met_p)),metp[i].met);
      metp[i].intv = (double)answer.dl_lop;
      metp[i].cooked = metp[i].intv/tdiff;
      metp[i].met.dl_lop = ((dl_t *)(metp[i].met_p))->dl_lop;
      metp[i].met.dl_hop = ((dl_t *)(metp[i].met_p))->dl_hop;
    }
}

/*
 *	check that the resource a metric is based on is what we expect
 */
static void
check_resource( metid_t id, metid_t resid )
{
  resource_t *resource_p;

  resource_p = mas_get_met_resources( md, id );
  if( !resource_p ) 
    {
      fprintf(stderr,"can't get resource list for met id:%d\n",id);
      exit(1);
    }
  if( *resource_p != resid ) 
    {
      fprintf(stderr,"weird resource for met id:%d\n",id);
      fprintf(stderr,"expected:%d got:%d\n",resid, *resource_p);
      exit(1);
    }
}

/*
 *	verify the size of something is what we expect
 */
static void 
check_size( metid_t id, int size )
{
  uint32 *metsz_p;
  metsz_p = mas_get_met_objsz( md, id );
  if( !metsz_p ) 
    {
      fprintf(stderr,"can't get object size for met id:%d\n",id);
      exit(1);
    }
  if( *metsz_p != size ) 
    {
      fprintf(stderr,"weird object size for met id:%d\n",id);
      fprintf(stderr,"expected:%d got:%d\n",size, *metsz_p);
      exit(1);
    }
}

/*
 *	allocate a set of struct mets, one per instance
 */
static void
metalloc( struct met **metp, int count ) 
{
  if( !((*metp) = (struct met *)malloc( count * sizeof( struct met ) ))) {
    fprintf(stderr,"can't malloc\n");
    exit(1);
  }
}

/*
 *	allocate a set of dblmets, one per instance
 */
static void
dblmetalloc( struct dblmet **metp, int count ) 
{
  if(!((*metp) = (struct dblmet *)malloc( count * sizeof( struct dblmet ) ))) 
    {
      fprintf(stderr,"can't malloc\n");
      exit(1);
    }
}

/*
 *	initialize a set of struct mets with metric address
 *	within the snap buffer
 */
static void
metset( struct met *metp, metid_t id, int count) 
{
  int i;

  for( i = 0 ; i < count ; i++ ) 
    {
      metp[ i ].met_p = mas_get_met_snap( md, id, i );
      if( !(metp[ i ].met_p) ) 
	{
	  fprintf(stderr,"unregistered met id:%d inst:%d\n",
		  id, i );
	  exit(1);
	}
    }
}

/*
 *	initialize a set of struct dblmets with
 *	the address of the metric within the snap buffer
 */
static void
dblmetset(struct dblmet *metp, metid_t id, int count)
{
  int i;

  for( i = 0 ; i < count ; i++ ) 
    {
      metp[ i ].met_p = mas_get_met_snap( md, id, i );
      if( !(metp[ i ].met_p) ) 
	{
	  fprintf(stderr,"unregistered met id:%d inst:%d\n",
		  id, i );
	  exit(1);
	}
    }
}

/*
 *	allocate a single metric, which may be composed
 *	of multiple resources.  verify the resource list and size.
 *	set the metric addresses within the snap buffer.
 */
static void alloc_met( metid_t id, resource_t resource, int cnt, int size,
		      struct met **metp) 
{
  check_resource( id, resource );
  check_size( id, size );
  metalloc( metp, cnt );
  metset( *metp, id, cnt);
}

/*
 *	allocate a single double long metric, which may be composed
 *	of multiple resources.  verify the resource list and size.
 *	set the metric addresses within the snap buffer.
 */
static void alloc_dblmet( metid_t id, resource_t resource, int cnt, 
			 int size, struct dblmet **metp) 
{
  check_resource( id, resource );
  check_size( id, size );
  dblmetalloc( metp, cnt );
  dblmetset( *metp, id, cnt);
}

/*
 *	function: 	open_mets
 *
 *	args:		none
 *
 *	ret val:	non-negative int on success
 *			-1 on failure
 *
 *	Open_mets opens the metric registration file MAS_FILE
 *	which is defined in metreg.h.  The raw system metrics
 *	are memory mapped into the calling process' address space.
 *	Memory is allocated for the subset of metrics declared above.
 */
int 
open_mets( void ) 
{
  md = mas_open( MAS_FILE, MAS_MMAP_ACCESS );
  alloc_mets();
  return( md );
}

/*
 *	function: 	close_mets
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Close_mets undoes everything done by open_mets.
 *	The metric registration file and the raw system metrics
 *	are unmapped from the calling process' address space.
 *	Memory is freed for the subset of metrics declared above.
 */
void 
close_mets( void ) 
{
  free_mets();
  (void)mas_close( md );
}

/*
 *	function: 	snap_mets
 *
 *	args:		none
 *
 *	ret val:	the current value of lbolt (time in ticks)
 *
 *	Snap_mets takes a snapshot of the raw system metric data.
 *	The times system call is invoked to get the system time
 *	in ticks, and mas_snap is called to copy the memory mapped
 *	metric data to the mas snapshot buffer.  Then calc_interval_data
 *	is called to cook the raw metric.  During the interval calcs,
 *	the metric is copied to an "old" value for use in the next 
 *	iteration.
 */
int 
snap_mets( void ) 
{
  currtime = times( &tbuf );
  (void)mas_snap( md );
  if( currtime == timeval->met ) 
    {
      /*
       *	need to let clock tick, otherwise, calc_interval_data
       *	divides by tdiff of zero.  It's not likely anything
       *	changed much in 10ms, so just return
       */

      return(currtime);
    }

  if( !tdiff ) 
    {
      /*
       *	this is the first time snap_mets was called.
       *	seed some initial values into "old" time, otherwise
       *	calc_interval_data hits a divide check
       */
		
      timeval->met = 1;
      timeval->cooked = 1.0;

      /*
       *	save inital time into freeswaptime
       *	the free swap pages counter is incremented
       *	once per second ( when currtime % hz == 0 ).
       *	We will only update the cooked metric when
       *	the counter has been updated, otherwise, we
       *	may end up trying to divide by 0.
       */
      freeswptime = currtime;
      freeswp->met = *(dl_t *)(freeswp->met_p);
    }
  calc_interval_data();
  return( currtime );
}

