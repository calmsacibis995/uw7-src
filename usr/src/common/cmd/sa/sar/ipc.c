/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/ipc.c	1.6"
#ident "$Header$"


/* ipc.c
 * 
 * Inter-process communication metrics.  Processes SAR_IPC_P records.
 *
 */

#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"


static struct metp_ipc     *ipc;
static struct metp_ipc     *old_ipc;
static struct metp_ipc     *first_ipc;
static struct ipc_info     temp;

static int  first_sample = TRUE;


flag
sar_ipc_init(void)
{
	int   i;
	
	if (ipc == NULL) {
		ipc = malloc(machinfo.num_engines * sizeof(*ipc));
		old_ipc = malloc(machinfo.num_engines * sizeof(*old_ipc));
		first_ipc = malloc(machinfo.num_engines * sizeof(*first_ipc));
		
		if (ipc == NULL || old_ipc == NULL || first_ipc == NULL) {
			return(FALSE);
		}
	}
	
	for (i = 0; i < machinfo.num_engines; i++) {
		ipc[i].mpi_msg = 0;
		ipc[i].mpi_sema = 0;
		
		old_ipc[i] = ipc[i];
	}
	
	first_sample = TRUE;
	
	return(TRUE);
}


/*ARGSUSED*/

int
sar_ipc(FILE *infile, sar_header sarh, flag32 of)
{
	int   p;
	int   i;

	if (sarh.item_size != sizeof(struct ipc_info)) {
		metric_warn[sarh.item_code] = TRUE;
	}
	
	memcpy(old_ipc, ipc, machinfo.num_engines * sizeof(*old_ipc));
	
	for (i = 0; i < machinfo.num_engines; i++) {
		get_item(&temp, sizeof(struct ipc_info), sarh.item_size, infile);
		p = temp.id;
		ipc[p] = temp.data;
	}
	
	if (first_sample == TRUE) {
		memcpy(first_ipc, ipc, machinfo.num_engines * sizeof(*first_ipc));
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;
	
	return(TRUE);
}


sarout_t
sar_ipc_out(int column, int mode, int devnum)
{
	struct metp_ipc	*start;
	struct metp_ipc	*end;
	time_t		*td;
	sarout_t	answer;

	SET_INTERVAL(mode, ipc, old_ipc, first_ipc);

	switch (column) {
	      case IPC_MSG:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpi_msg);
		return(answer);
		break;

	      case IPC_SEMA:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpi_sema);
		return(answer);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}
	

void
sar_ipc_cleanup(void)
{
	free(ipc);
	free(old_ipc);
	free(first_ipc);

	ipc = NULL;	
	old_ipc = NULL;
	first_ipc = NULL;
}
