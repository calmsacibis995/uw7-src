#ident	"@(#)rtpm:ether.c	1.5"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/sysmacros.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <sys/strlog.h>
#include <sys/ksynch.h>   /* get def for lock_t - used in dpli_ether.h */
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <curses.h>
#include <mas.h>
#include "rtpm.h"

#define NDEVICES	16
#define CONF_FILE	"/etc/confnet.d/netdrivers"


#define METBUFSZ	(sizeof( DL_mib_t ))
int nether_devs = 0;
char *ether_nm[NDEVICES] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };
DL_mib_t *etherstat[NDEVICES] =  { NULL,NULL,NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

char *get_device( FILE *fp, char *buf );
int getstat( int fd, char *metbuf );
int net_stat();

/*
 * flag to indicate whether curses has been started
 */
extern int in_curses;

/*
 * parse file for device names as first thing on line.
 */
char *get_device( FILE *fp, char *buf ) {
	char line[ 256 ];
	char *p;

	do {	
		p = line;
		if( !fgets( line, 256, fp ) )
			return( NULL );
		while( *p && (isalnum(*p) || *p == '_' ) ) 
			p++;
		*p = '\0';
	} while( p == line );
	strcpy( buf, line );
	return(buf);
}
/*
 * get the stats for all of the drivers we know about
 */
ether_stat() {
	static int fd[NDEVICES] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	int i;
	char *device, *get_device( FILE *cfp, char *buf );
	FILE *cfp;
	char *buf;

	if( !nether_devs ) {
		if( !(cfp = fopen( CONF_FILE, "r" ))) {
			return(-1);
		}
		if( !(buf = (char *)malloc( 1024 )) ) {
			if( in_curses ) 
				endwin();
			fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
			exit(1);
		}
		strcpy( buf, "/dev/" );
		while( device = get_device( cfp, buf + 5 ) ) {
			if ((fd[nether_devs] = open(buf, O_RDWR)) >= 0) {
				if( !etherstat[nether_devs]
				  && !(etherstat[nether_devs] = 
				  (DL_mib_t *)histalloc( METBUFSZ )) ) {
					if( in_curses ) 
						endwin();
					fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
					exit(1);
				}
				if(getstat( fd[nether_devs], (char *)etherstat[nether_devs])) {
/*
 *					etherstat[nether_devs] could be
 *					freed here if we used malloc.  
 *					instead we're using histalloc, so 
 *					save it for the next iteration.
 */
					close( fd[nether_devs] );
					continue;
				}
				if(!(ether_nm[nether_devs] = (char *)histalloc( 
				  strlen( device +1 ) ))) {
					if( in_curses ) 
						endwin();
					fprintf(stderr, gettxt("RTPM:7", "out of memory\n"));
					exit(1);
				}
				strcpy( ether_nm[nether_devs], device );
/*
 *				Make sure there aren't more than
 *				NDEVICES ethernet cards.
 */
				if( ++nether_devs >= NDEVICES )
					break;
			}
		}
		free(buf);
		fclose( cfp );
	}
	else {
		for( i = 0 ; i < nether_devs ; i++ )
			getstat( fd[i], (char *)etherstat[i] );
	}
	return(0);
}
/*
 * call ioctl for the driver stats
 */
int
getstat( int fd, char *metbuf ) {
	struct strioctl strioctl;

	strioctl.ic_cmd	= DLIOCGMIB;
	strioctl.ic_timout = -1;
	strioctl.ic_len	= METBUFSZ;
	strioctl.ic_dp = metbuf;
	if ( ioctl(fd, I_STR, &strioctl) < 0)
		return(-1);
	return(0);
}

