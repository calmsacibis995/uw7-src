#ident	"@(#)libc-i386:gen/gettimeofday.c	1.9"

#ifndef GEMINI_ON_OSR5

#ifdef __STDC__
	#pragma weak gettimeofday = _gettimeofday
#ifdef DSHLIB
	#pragma weak _abi_gettimeofday = _gettimeofday
#endif
#endif

#include "synonyms.h"
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ksym.h>
#include <sys/mman.h>
#include <sys/param.h>

static const volatile timestruc_t *mapped_hrtimep;	/* must be == 0 at start */

static caddr_t 
get_hrt_mapping(void)
{
	void *ret_addr = (caddr_t)(-1);
	unsigned long hrtaddr = 0;
	unsigned long hrtinfo;
	int sysdat_fd;
	caddr_t sysdat_pageaddr = NULL;

	/* 
	 * this function is entered only once -- the first time that a call 
	 * is made to gettimeofday() by any thread in the process. 
	 * if we can successfully map hrtrestime, then return the mapped
	 * address; otherwise return (-1);
 	 */

	if ((getksym("hrestime", &hrtaddr, &hrtinfo) == -1) ||
	    ((sysdat_fd = open("/dev/sysdat", O_RDONLY)) == -1)) {
		return(ret_addr);
	}
	if (((sysdat_pageaddr = mmap(0, PAGESIZE, PROT_READ, MAP_SHARED, 
				sysdat_fd, (off_t)hrtaddr)) != NULL) &&
            (sysdat_pageaddr != (void *)(-1))) {
		ret_addr = (caddr_t)sysdat_pageaddr + (hrtaddr % PAGESIZE);
	}
	(void)close(sysdat_fd);
	return(ret_addr);
}

/*
 * Get the time of day information.
 * BSD compatibility on top of SVr4 facilities:
 * u_sec always zero, and don't do anything with timezone pointer.
 */
int
#ifdef __STDC__
gettimeofday(struct timeval *tp, void *tzp)
#else
gettimeofday(tp, tzp)
struct timeval *tp;
void *tzp;
#endif
{
	long	tmp;
 
        if (tp == NULL)
                return (0);
again:
	if (mapped_hrtimep == (timestruc_t *)(-1)) {
		return (_sys_gettimeofday(tp));
	} else if (mapped_hrtimep != NULL) {
		tp->tv_sec = mapped_hrtimep->tv_sec;
		tp->tv_usec = (mapped_hrtimep->tv_nsec) / 1000;
		if ((tmp = mapped_hrtimep->tv_sec) > tp->tv_sec) {
			tp->tv_sec = tmp;
			tp->tv_usec = 0;
		}	
		return(0);
	} else {
		/* 
		 * we're here for the first time. this part needs 
		 * protection in a multithreaded process.
	 	 */

		/*
		 * don't worry about multiple threads updating
		 * mapped_hrtimep if NULL.
		 */
		if (mapped_hrtimep == NULL) {
			mapped_hrtimep = (timestruc_t *)get_hrt_mapping();
		}
		goto again;
	}
}
#endif
