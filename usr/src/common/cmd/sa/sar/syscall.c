/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/syscall.c	1.6.1.1"
#ident "$Header$"

/* syscall.c  
 *
 * System call metrics.  Processes SAR_SYSCALL_P records.
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"


static struct metp_syscall   *syscalls = NULL;
static struct metp_syscall   *old_syscalls = NULL;
static struct metp_syscall   *first_syscalls = NULL;
static struct syscall_info   temp;

static flag    first_sample = TRUE;


flag
sar_syscall_init(void)
{
	int      i;
	
	if (syscalls == NULL) {
		syscalls = malloc(machinfo.num_engines * sizeof(*syscalls));
		old_syscalls = malloc(machinfo.num_engines * sizeof(*old_syscalls));
		first_syscalls = malloc(machinfo.num_engines * sizeof(*first_syscalls));
		
		if (syscalls == NULL || old_syscalls == NULL || first_syscalls == NULL) {
			return(FALSE);
		}
	}
	
	for (i = 0; i < machinfo.num_engines; i++) {
		syscalls[i].mps_syscall = 0;
		syscalls[i].mps_fork = 0;
		syscalls[i].mps_lwpcreate = 0;
		syscalls[i].mps_exec = 0;
		syscalls[i].mps_read = 0;
		syscalls[i].mps_write = 0;
		syscalls[i].mps_readch = 0;
		syscalls[i].mps_writech = 0;
		
		old_syscalls[i] = syscalls[i];
	}
	
	first_sample = TRUE;
	return(TRUE);
}


/*ARGSUSED*/

int
sar_syscall_p(FILE *infile, sar_header sarh, flag32 of)
{
	int      i;
	int      p;
	
	memcpy(old_syscalls, syscalls, 
	       machinfo.num_engines * sizeof(*old_syscalls));
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(temp), sarh.item_size, infile);
		p = temp.id;
		syscalls[p] = temp.data;
	}
	
	if (first_sample == TRUE) {
		memcpy(first_syscalls, syscalls,
		       machinfo.num_engines * sizeof(*first_syscalls));
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;

	return(TRUE);
}


sarout_t
sar_syscall_out(int column, int mode, int devnum)
{
	struct metp_syscall	*start;
	struct metp_syscall	*end;
	time_t			*td;
	sarout_t		answer;

	SET_INTERVAL(mode, syscalls, old_syscalls, first_syscalls);

	switch (column) {
	      case SYSCALL_SCALL:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_syscall);
		return(answer);
		break;

	      case SYSCALL_SREAD:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_read);
		return(answer);
		break;

	      case SYSCALL_SWRIT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_write);
		return(answer);
		break;

	      case SYSCALL_FORK:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_fork);
		return(answer);
		break;

	      case SYSCALL_LWPCREATE:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_lwpcreate);
		return(answer);
		break;

	      case SYSCALL_EXEC:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_exec);
		return(answer);
		break;

	      case SYSCALL_RCHAR:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_readch);
		return(answer);
		break;

	      case SYSCALL_WCHAR:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_writech);
		return(answer);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


void
sar_syscall_cleanup(void)
{
	free(syscalls);
	free(old_syscalls);
	free(first_syscalls);

	syscalls = NULL;
	old_syscalls = NULL;
	first_syscalls = NULL;
}

