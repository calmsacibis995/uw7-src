/*		copyright	"%c%" 	*/

#ident	"@(#)profiler:prfsnap.c	1.4.5.3"
#ident	"$Header$"

/*
 *	prfsnap - dump profile data to a log file
 */

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/prf.h>

#define LOGPERM	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

main( int argc, char **argv ) {
	int 	prf_fd;
	int	log;
	time_t	tvec, log_time, prf_time;
	char	*buf;
	int	header_size;
	int	Nengine;
	int	prfmax;
	int	prfsize;
	int	namesz;
	int	need_header = 0;
	char	*namelist;

	void prf_start( const int prf_fd, const char *namelist );
	void error( const char *s );
	void *malloc();
	long lseek();

	if(argc < 2 || argc > 3)
		error("usage: prfsnap file [system_namelist]");

	namelist = NULL;
	if( argc == 3 )
		namelist = argv[2];

	if((prf_fd = open("/dev/prf", O_RDWR)) < 0)
		error("cannot open /dev/prf");

	if( (prf_time = (time_t)ioctl(prf_fd, PRF_STAT, 0)) < 0 )
		error("Cannot determine profiler status");

	if( prf_time == (time_t)0 ) {	/* profiling is off or unloaded */
		need_header = 1;
		prf_start( prf_fd, namelist );
		if( (prf_time = (time_t)ioctl(prf_fd, PRF_STAT, 0)) <= 0)
			error("Cannot determine profiler status");
	}

	if( (prfsize = ioctl(prf_fd, PRF_SIZE, 0)) <= 0 ) {
		error("Cannot determine size of profiler data");
		exit(1);
	}

	if ( (buf = malloc( prfsize ) ) == NULL) {
		error("Cannot malloc space for profiling data");
		exit(2);
	}

	time(&tvec);
	if( read(prf_fd, buf, prfsize) != prfsize )
		error("error reading /dev/prf");

	prfmax = ((struct mprf *)buf)->mprf_addr;
	namesz = ((struct mprf *)buf)->mprf_offset;
	header_size = (prfmax+1)*sizeof(struct mprf) + namesz;
	Nengine = (prfsize - header_size) / ((prfmax+1) * sizeof(int));

	if((log = open(argv[1], O_RDWR)) < 0) {
		if((log = creat(argv[1], LOGPERM)) < 0)
			error("cannot creat log file");
		else {	/* first time writing	*/
			need_header = 1;
			(void) write(log, &prf_time, sizeof(time_t) );
		}
	}
	else {	/* existing file	*/
		if( lseek(log, 0L, 2 ) == 0L) {	/* first time writing 	*/
			need_header = 1;
			(void) write(log, &prf_time, sizeof(time_t) );
		}
		else {
			(void) lseek(log, -1L * sizeof( time_t ), 2);
			if( read( log, &log_time, sizeof( time_t ) ) 
			  != sizeof( time_t ) ) {
				perror("read failed");
				error("corrupted log file");
			}
			if( log_time != prf_time ) {
				need_header = 1;
				(void) lseek(log, -1L * sizeof(time_t), 2);
				(void) write(log, &prf_time, sizeof(time_t) );
			}
		}
	}
	if( need_header ) {
		write(log, &Nengine, sizeof Nengine);
		write(log, buf, header_size);
	}
	write(log, &tvec, sizeof(time_t) );
	write(log, &buf[header_size], (prfmax+1) * Nengine * sizeof(int));
	write(log, &prf_time, sizeof(time_t) );
	exit(0);
}
