/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/psdata.c	1.1"
#ident "$Header$"



/*
 * psdata.c
 *
 * psdata metrics - processes SAR_PS records.
 *
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include "../sa.h"
#include "sar.h"

static struct ps_info *psdata = NULL;
static int psdata_size = 0;
static int psdata_valid = 0;
static struct ps_info  temp;


/*ARGSUSED*/

int
sar_psdata_p(FILE *infile, sar_header sarh, flag32 of) {
        int     i;
        
#ifdef DEBUG
	printf("sar_psdata_p()\n");
	fflush(stdout);
#endif /* DEBUG */
        if (sarh.item_size != sizeof(struct ps_info)) {
                metric_warn[sarh.item_code] = TRUE;
        }
        
	if (psdata_size < sarh.item_count) {
		psdata_size = sarh.item_count;
		psdata = realloc(psdata, psdata_size * sizeof(*psdata));
	}
	psdata_valid = sarh.item_count;

        for (i = 0; i < sarh.item_count; i++) {
                get_item(&temp, sizeof(struct ps_info), sarh.item_size, infile);
		psdata[i] = temp;
        }
	collected[sarh.item_code] = TRUE;
        
        return TRUE;
} /* sar_psdata_p() */

void
sar_psdata_cleanup(void) {
        free(psdata);
	psdata_size = 0;

	psdata = NULL;
} /* sar_psdata_cleanup() */

/*
 * Return the command and args for a given process.
 * The returned text is stored in a private buffer which is
 * overwritten on the next call.  Copy it or lose it!
 */
char *
sar_psdata_desc(int pid, int max_length) {
	int i;
	static char buf[PRFNSZ + PRARGSZ];

	/* Find the corresponding entry in the latest ps data. */
	for (i = 0; i < psdata_valid; i++) {
		if (psdata[i].pi_pid == pid) {
			strcpy(buf, psdata[i].pi_fname);
			strcat(buf, " ");
			strcat(buf, psdata[i].pi_psargs);
			break;
		}
	}
	if (i >= psdata_valid)
		strcpy(buf, "(unknown command)");

	if (max_length < PRFNSZ + PRARGSZ)
		buf[max_length] = '\0';

	return buf;
} /* sar_psdata_desc() */
