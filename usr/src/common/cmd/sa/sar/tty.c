/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/tty.c	1.7"
#ident	"$Header$"

/* tty.c
 * 
 * TTY metrics.  Processes SAR_TTY_P records.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"


static struct metp_tty  *tty = NULL;
static struct metp_tty  *old_tty = NULL;
static struct metp_tty  *first_tty = NULL;
static struct tty_info  temp;

static flag   first_sample;

flag
sar_tty_init(void)
{
	int   i;
	
	if (tty == NULL) {
		tty = malloc(machinfo.num_engines * sizeof(*tty));
		old_tty = malloc(machinfo.num_engines * sizeof(*old_tty));
		first_tty = malloc(machinfo.num_engines * sizeof(*first_tty));
		
		if (tty == NULL || old_tty == NULL || first_tty == NULL) {
			return(FALSE);
		}
	}
	
	for (i = 0; i < machinfo.num_engines; i++) {
		tty[i].mpt_rcvint = 0;
		tty[i].mpt_xmtint = 0;
		tty[i].mpt_mdmint = 0;
		tty[i].mpt_rawch = 0;
		tty[i].mpt_canch = 0;
		tty[i].mpt_outch = 0;
		
		old_tty[i] = tty[i];
	}

	first_sample = TRUE;
	return(TRUE);
}


/*ARGSUSED*/

int
sar_tty(FILE *infile, sar_header sarh, flag32 of)
{
	int   i;
	int   p;
	
	memcpy(old_tty, tty, machinfo.num_engines * sizeof(*old_tty));
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(temp), sarh.item_size, infile);
		p = temp.id;
		tty[p] = temp.data;
	}
	
	if (first_sample) {
		memcpy(first_tty, tty, machinfo.num_engines * sizeof(*first_tty));
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;
		
	return(TRUE);
}


sarout_t
sar_tty_out(int column, int mode, int devnum)
{
	struct metp_tty	*start;
	struct metp_tty	*end;
	time_t		*td;
	sarout_t	answer;

	SET_INTERVAL(mode, tty, old_tty, first_tty);

	switch(column) {
	      case TTY_RAWCH:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpt_rawch);
		return(answer);
		break;

	      case TTY_CANCH:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpt_canch);
		return(answer);
		break;

	      case TTY_OUTCH:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpt_outch);
		return(answer);
		break;

	      case TTY_RCVINT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpt_rcvint);
		return(answer);
		break;

	      case TTY_XMTINT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpt_xmtint);
		return(answer);
		break;

	      case TTY_MDMINT:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpt_mdmint);
		return(answer);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}



void
sar_tty_cleanup(void)
{
	free(tty);
	free(old_tty);
	free(first_tty);

	tty = NULL;
	old_tty = NULL;
	first_tty = NULL;
}

