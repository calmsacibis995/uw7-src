#ident	"@(#)libc-i386:gen/mpsys.c	1.3"

#ifdef __STDC__
	#pragma weak mpsys = _mpsys
#endif

#include "synonyms.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/prosrfs.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>

#define	MPSYS_ONLINE	1
#define	MPSYS_BIND	2
#define	MPSYS_INFO	20

int
mpsys(int op, int arg1, int arg2, int arg3)
{
        switch (op) {
        case MPSYS_ONLINE:
	        return(online((int)arg1, (int)arg2));
        case MPSYS_BIND:
		return(processor_bind(P_PID,(id_t)arg2,(processorid_t)arg1,
				     (processorid_t *)arg3));
        case MPSYS_INFO:
		return(processor_info((processorid_t)arg1,
				      (processor_info_t *)arg2));
        default:
                return (EINVAL);
        }
}
