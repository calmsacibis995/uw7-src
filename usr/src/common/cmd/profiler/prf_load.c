/*		copyright	"%c%" 	*/

#ident	"@(#)profiler:prf_load.c	1.2.1.1"
#ident	"$Header$"

/*
 *	prf_load.c - subroutine for loading profiler with 
 *		sorted kernel text addresses
 */

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/prf.h>

static int prf_fd_for_sig;

#ifdef sgn
#undef sgn
#endif

#define sgn( x )	( (x) ? ( (x) > 0 ? 1 : -1 ) : 0 )

void
prf_start( const int prf_fd, const char *namelist ) {
	void prf_load( const int prf_fd, const char *namelist );
	void error( const char *s );

	prf_load( prf_fd, namelist );
	if( ioctl( prf_fd, PRF_ENABLE, 0 ) < 0 )
		error("Can't enable profiler");
}
void
prf_load( const int prf_fd, const char *namelist ) { 
/*
 *	top level module information - read from /dev/prf after locking
 */
	struct mprf firstbuf;	/* size of module data			*/
	char *prfbuf;		/* buffer to read everything in		*/
	struct mprf *prf;	/* array of module addr/offset's	*/
	char *names;		/* module path's			*/
	int prfsize;		/* size of module information		*/
	int prfmax;		/* number of modules			*/
	int namesz;		/* size of buffer holding module paths	*/

/*
 *	per module information - returned from getsym()
 */
	struct mprf **mprf_p;	/* array of *mprf's, one for ea. module	*/
	char *mprfbuf;		/* big buffer for all symbols/addrs	*/
	struct mprf *mprf;	/* array of sym/addr's			*/
	char *mnames;		/* symbol names				*/
	char *mnms;		/* tmp for copying symbol names		*/
	int mprfsize;		/* size of symbols+offsets+names	*/
	int mprfmax;		/* total number of symbols		*/
	int mnamesz;		/* total size of symbol names		*/

	int i,j;

	struct mprf *getsym(const char *filename, unsigned long addr);
	int mprf_cmp( const void *a, const void *b );
	void catch_sig();
	void error( const char *s );
	void error1( const char *s );

	prfmax = ioctl(prf_fd, PRF_MAX, 0);

	switch( sgn( prfmax ) ) {
	case -1:
		error("Cannot determine profiler status");
		break;
	case  0:
		/* profiler symbols not loaded */	
		break;
	case  1:
		/* profiler symbols already loaded */
		return;
	}
	prf_fd_for_sig = prf_fd;

	signal( SIGHUP, catch_sig );
	signal( SIGINT, catch_sig );
	signal( SIGQUIT, catch_sig );

	if( ioctl(prf_fd, PRF_LOAD, 0) < 0 ) 
		error("Cannot lock modules in kernel memory");

	if( read(prf_fd, &firstbuf, sizeof( struct mprf )) 
	  != sizeof( struct mprf) )
		error1("error reading /dev/prf");

	prfsize = sizeof(struct mprf) * (firstbuf.mprf_addr + 1)
	   + firstbuf.mprf_offset;
	if ( (prfbuf = malloc( prfsize ) ) == NULL)
		error1("Cannot malloc space for profiling data");

	if( read(prf_fd, prfbuf, prfsize) != prfsize )
		error1("error reading /dev/prf");

	prfmax = ((struct mprf *)prfbuf)->mprf_addr;
	namesz = ((struct mprf *)prfbuf)->mprf_offset;
	mprf_p = (struct mprf **)malloc(prfmax * sizeof(struct mrpf *));
	if( mprf_p == NULL )
		error1("Cannot malloc space for profiling data");

	prf = ((struct mprf *)prfbuf)+1;
	names = prfbuf+(sizeof(struct mprf)*(prfmax+1));

	mprfmax = 0;
	mnamesz = 0;
	for( i = 0 ; i < prfmax ; i++ ) {
		char *filename = names + prf->mprf_offset;
		long addr = prf->mprf_addr;
		if( i == 0 && *filename == '\0' )
			if( namelist != NULL )
				filename = (char *)namelist;
			else
				filename = "/stand/unix";
		if( (mprf_p[i] = getsym(filename, addr)) == NULL )
			error1("Cannot determine symbols");
		mnamesz += mprf_p[i]->mprf_offset;
		mprfmax += mprf_p[i]->mprf_addr;
		prf++;
	}

	mprfsize = (mprfmax+1) * sizeof( struct mprf ) + mnamesz;
	if( (mprfbuf = malloc( mprfsize )) == NULL )
		error1("cannot malloc space for profiling data");

	mprf = (struct mprf *)mprfbuf;
	mprf->mprf_addr = mprfmax;
	mprf->mprf_offset = mnamesz;
	mnames = (char *)(&mprf[mprfmax+1]);
	mnms = mnames;
	mprf++;
/*
 *	could optimize by installing in order of module address 
 */
	for( i = 0 ; i < prfmax ; i++ ) {
		int modmax = mprf_p[i]->mprf_addr;
		char *src = (char *)(&mprf_p[i][modmax+1]);
		for( j = 1 ; j <= modmax ; j++ ) {
			mprf->mprf_addr = mprf_p[i][j].mprf_addr;
			mprf->mprf_offset = mprf_p[i][j].mprf_offset
			  + (mnms - mnames);
			mprf++;
		}
		memcpy( mnms, src, mprf_p[i]->mprf_offset );
		mnms += mprf_p[i]->mprf_offset;
		free( mprf_p[i] );
	}

	mprf = (struct mprf *)mprfbuf;
	qsort((char *)(mprf+1),mprfmax,sizeof(struct mprf),mprf_cmp);
	if( write(prf_fd, mprfbuf, mprfsize) != mprfsize )
		error1("error writing symbol addresses to /dev/prf");

	free( mprfbuf );
	free( prfbuf );

	signal( SIGHUP, SIG_DFL );
	signal( SIGINT, SIG_DFL );
	signal( SIGQUIT, SIG_DFL );
}

void
catch_sig() {
	void error1( const char *s );

	error1("caught signal");
	exit(1);
}
int
mprf_cmp( const void *a, const void *b )
{

	unsigned long av = ((struct mprf *) a)->mprf_addr;
	unsigned long bv = ((struct mprf *) b)->mprf_addr;
	if(av < bv)
		return(-1);
	if(av > bv)
		return(1);
	return(0);
}

void
warning( const char *s) {
	fprintf( stderr, "warning: %s\n", s);
}

void
error( const char *s ) {
	fprintf(stderr, "error: %s\n", s);
	exit(1);
}

void
error1( const char *s ) {
	printf("error: %s\n", s);
	(void)ioctl(prf_fd_for_sig, PRF_UNLOAD, 0);
	exit(1);
}
