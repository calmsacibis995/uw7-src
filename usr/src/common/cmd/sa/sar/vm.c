/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/vm.c	1.6"
#ident "$Header$"

/* vm.c
 * 
 * Virtual memory metrics.  Processes SAR_VM_P records.
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"

static struct metp_vm   *vm = NULL;
static struct metp_vm   *old_vm = NULL;
static struct metp_vm   *first_vm = NULL;
static struct vm_info   temp;

static int  first_sample = TRUE;

flag
sar_vm_init(void)
{
	int      i;
	
	if (vm == NULL) {
		vm = malloc(machinfo.num_engines * sizeof(*vm));
		old_vm = malloc(machinfo.num_engines * sizeof(*old_vm));
		first_vm = malloc(machinfo.num_engines * sizeof(*first_vm));
		
		if (vm == NULL || old_vm == NULL || first_vm == NULL) {
			return(FALSE);
		}
	}
	
	for (i = 0; i < machinfo.num_engines; i++) {
		vm[i].mpv_atch = 0;
		vm[i].mpv_atchfree = 0;
		vm[i].mpv_atchfree_pgout = 0;
		vm[i].mpv_atchmiss = 0;
		
		vm[i].mpv_pgin = 0;
		vm[i].mpv_pgpgin = 0;
		vm[i].mpv_pgout = 0;
		vm[i].mpv_pgpgout = 0;
		
		vm[i].mpv_swpout = 0;
		vm[i].mpv_pswpout = 0;
		vm[i].mpv_vpswpout = 0;
		vm[i].mpv_swpin = 0;
		
		vm[i].mpv_virscan = 0;
		vm[i].mpv_virfree = 0;
		vm[i].mpv_physfree = 0;
		
		vm[i].mpv_pfault = 0;
		vm[i].mpv_vfault = 0;
		vm[i].mpv_sftlock = 0;
		
		old_vm[i] = vm[i];
	}
	first_sample = TRUE;
	
	return(TRUE);
}


/*ARGSUSED*/

int
sar_vm(FILE *infile, sar_header sarh, flag32 of)
{
	int      i;
	int      p;
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(temp), sarh.item_size, infile);
		p = temp.id;
		old_vm[p] = vm[p];
		vm[p] = temp.data;
	}
	if (first_sample == TRUE) {
		memcpy(first_vm, vm, machinfo.num_engines * sizeof(*first_vm));
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;
	
	return(TRUE);
}


sarout_t
sar_vm_out(int column, int mode, int devnum)
{
	struct metp_vm	*start;
	struct metp_vm	*end;
	time_t		*td;
	sarout_t	answer;

	SET_INTERVAL(mode, vm, old_vm, first_vm);

	switch (column) {
	      case VM_ATCH:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_atch);
		return(answer);
		break;

	      case VM_ATCHFREE:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_atchfree);
		return(answer);
		break;

	      case VM_ATCHFREE_P:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_atchfree_pgout);
		return(answer);
		break;

	      case VM_ATCHMISS:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_atchmiss);
		return(answer);
		break;

	      case VM_PGIN:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_pgin);
		return(answer);
		break;

	      case VM_PGPGIN:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_pgpgin);
		return(answer);
		break;

	      case VM_PGOUT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_pgout);
		return(answer);
		break;

	      case VM_PGPGOUT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_pgpgout);
		return(answer);
		break;

	      case VM_SWPOUT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_swpout);
		return(answer);
		break;

	      case VM_PSWPOUT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_pswpout);
		return(answer);
		break;

	      case VM_VPSWPOUT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_vpswpout);
		return(answer);
		break;

	      case VM_SWPIN:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_swpin);
		return(answer);
		break;

	      case VM_PSWPIN:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_pswpin);
		return(answer);
		break;

	      case VM_VIRSCAN:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_virscan);
		return(answer);
		break;

	      case VM_VIRFREE:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_virfree);
		return(answer);
		break;

	      case VM_PHYSFREE:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_physfree);
		return(answer);
		break;

	      case VM_PFLT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_pfault);
		return(answer);
		break;

	      case VM_VFLT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_vfault);
		return(answer);
		break;

	      case VM_SLOCK:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpv_sftlock);
		return(answer);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


void
sar_vm_cleanup(void)
{
	free(vm);
	free(old_vm);
	free(first_vm);

	vm = NULL;
	old_vm = NULL;
	first_vm = NULL;
}

