/*		copyright	"%c%" 	*/

#ident "@(#)profiler:prfdc.c	1.4.5.3"
#ident "$Header$"

/*
 *	prfdc - profiler data collector
 */

#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/prf.h>

void
sigalrm( const int sig ) {
	void sigalrm( const int sig );

	signal( SIGALRM, sigalrm );
}

main( int argc, char **argv ) {
	int	prf;
	int	log;
	int	rate = 10, toff = 17;
	time_t	tvec;		/* the time				*/
	char	*buf;		/* Symbols, Kernel ctrs, and User ctr	*/
	int	prfmax;		/* number of text addresses		*/
	int	namesz;		/* size of name table			*/
	int	header_size;	/* size of header information		*/
	int	Nengine;	/* number of processes			*/
	time_t	prf_time;	/* the time profiling was activated	*/
	time_t	this_time;	/* if more recent, prf was turned off	*/
	int	prfsize;	/* size of entire prf data structure	*/
	char 	*namelist;	/* /stand/unix or other static kernel	*/

	void sigalrm( const int sig );

	void	*malloc();
	struct	tm	*localtime();

	namelist = NULL;

	switch(argc) {
		default:
			error("usage: prfdc file [period [off_hour [system_namelist]]]");
		case 5:
			namelist = argv[4];
		case 4:
			toff = atoi(argv[3]);
			if( toff < 0 || toff > 24 )
				error("invalid off hour, must be 0-24");
		case 3:
			rate = atoi(argv[2]);
		case 2:
			;
	}
	if(rate <= 0)
		error("invalid sampling period");

	if((prf = open("/dev/prf", O_RDWR)) < 0)
		error("cannot open /dev/prf");

	if(open(argv[1], O_RDONLY) >= 0)
		error("existing file would be truncated");

	if((log = creat(argv[1],
	  (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))) < 0)
		error("cannot creat log file");

	prf_start( prf, namelist );

	if((prf_time = (time_t) ioctl(prf, PRF_STAT, 0)) <= 0 )
		error("cannot determine profiling status");

	if( (prfsize = ioctl(prf, PRF_SIZE, 0)) < 0 )
		error("Cannot determine size of profiler data");

	if ( (buf = malloc( prfsize ) ) == NULL) 
		error("Cannot malloc space for profiling data");

	if(fork())
		exit(0);
	setpgrp();
	sigalrm( 0 );

new_header:
	read( prf, buf, prfsize );
	prfmax = ((struct mprf *)buf)->mprf_addr;
	namesz = ((struct mprf *)buf)->mprf_offset;
	header_size = (prfmax+1)*sizeof(struct mprf) + namesz;
	Nengine = (prfsize - header_size) / ((prfmax+1)*sizeof(int));
	write(log, &prf_time, sizeof tvec);
	write(log, &Nengine, sizeof Nengine);
	write(log, buf, header_size);
	time(&tvec);
	write(log, &prf_time, sizeof tvec);
	write(log, &buf[header_size], (prfmax + 1) * Nengine 
	  * sizeof (int));

	for(;;) {
		alarm(60 * rate);
		pause();
		time(&tvec);
		this_time = (time_t)ioctl(prf, PRF_STAT, 0);
		if( prf_time != this_time ) {
			if( this_time <= 0 ) {
				prf_start( prf, namelist );
				this_time = (time_t)ioctl(prf, PRF_STAT, 0);
			}
			prf_time = this_time;
			free(buf);
			prfsize = ioctl(prf, PRF_SIZE, 0);
			buf = malloc( prfsize );
			goto new_header;
		}
		read( prf, buf, prfsize );
		write(log, &prf_time, sizeof tvec);
		write(log, &tvec, sizeof tvec);
		write(log, &buf[header_size], (prfmax + 1) * Nengine 
		  * sizeof (int));
		if(localtime(&tvec)->tm_hour == toff)
			exit(0);
	}
}
