#ident	"@(#)get_ngm.c	1.2"
#ident  "$Header$"

#include	<sys/param.h>
#include	<unistd.h>

static int ngm = -1;

/*
	read the value of NGROUPS_MAX from the kernel 
*/
int
get_ngm()
{
	if (ngm != -1)
		return ngm;

	ngm = sysconf(_SC_NGROUPS_MAX);

	return ngm;
}
