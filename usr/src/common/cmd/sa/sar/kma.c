/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/kma.c	1.7"
#ident	"$Header$"

/* kma.c
 *
 * KMA metrics.  Processes SAR_KMEM_P records.
 *
 * Note that the KMA metrics are collected per-processor, but are
 * only reported on a system-wide basis.  The code in sar_kmem
 * allows sadc to output the per-processor data, though it is
 * permissible for an implementation of sadc to combine the data
 * an output system wide totals.
 */


#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include "../sa.h"
#include "sar.h"

static struct metp_kmem     *kmem_temp = NULL;
static struct metp_kmem     *kmem = NULL;
static struct metp_kmem     *old_kmem = NULL;

/*  acc_kmem holds cumulative counts for everthing EXCEPT the 
 *  mpk_fail field.  This field holds the total delta since
 *  the first sample.
 */


static struct metp_kmem     *acc_kmem = NULL;

static unsigned int  num_samples = 0;
static int  first_sample = TRUE;

extern uint_t num_mclass;
extern uint_t  *kmem_sizes;

flag
sar_kmem_init(void)
{
        int      i;
        
        if (kmem == NULL) {
/* 
 * Allocate arrays with (num_mclass + 1) elements.  The extra
 * element is used to accumulate totals of the first num_mclass 
 * elements
 */
                kmem_temp = malloc((num_mclass + 1) * sizeof(*kmem_temp));
                kmem = malloc((num_mclass + 1) * sizeof(*kmem));
                old_kmem = malloc((num_mclass + 1) * sizeof(*old_kmem));
                acc_kmem = malloc((num_mclass + 1) * sizeof(*acc_kmem));
                
                if (kmem_temp == NULL || kmem == NULL 
                    || old_kmem == NULL 
                    || acc_kmem == NULL) {
                        return(FALSE);
                }
        }
        
        for (i = 0; i <= num_mclass; i++) {
                kmem[i].mpk_mem = 0;
                kmem[i].mpk_balloc = 0;
                kmem[i].mpk_ralloc = 0;
                kmem[i].mpk_fail = 0;
                
                old_kmem[i] = kmem[i];
                acc_kmem[i] = kmem[i];
        }
        
        first_sample = TRUE;
        num_samples = 0;
        
        return(TRUE);
}


/*ARGSUSED*/

int
sar_kmem(FILE *infile, sar_header sarh, flag32 of)
{
        int      i;
        int      class;

	if (sarh.item_size != num_mclass * sizeof(struct metp_kmem)) {
		metric_warn[sarh.item_code] = TRUE;
	}
        
        for (i = 0; i <= num_mclass; i++) {
                old_kmem[i] = kmem[i];
                kmem[i].mpk_mem = 0;
                kmem[i].mpk_balloc = 0;
                kmem[i].mpk_ralloc = 0;
                kmem[i].mpk_fail = 0;
        }
        
        for (i = 0; i < sarh.item_count; i++) {
                get_item(kmem_temp, num_mclass * sizeof(*kmem_temp), sarh.item_size, infile);
                for (class = 0; class < num_mclass; class++) {
                        kmem[class].mpk_mem += kmem_temp[class].mpk_mem;
                        kmem[class].mpk_balloc += kmem_temp[class].mpk_balloc;
                        kmem[class].mpk_ralloc += kmem_temp[class].mpk_ralloc;
                        kmem[class].mpk_fail += kmem_temp[class].mpk_fail;

		}
	}

	for (class = 0; class < num_mclass; class++) {
		/* Only add active pool sizes */
		if (kmem_sizes[i] != 0) {
			kmem[num_mclass].mpk_mem += kmem[class].mpk_mem;
			kmem[num_mclass].mpk_balloc += kmem[class].mpk_balloc;
			kmem[num_mclass].mpk_ralloc += kmem[class].mpk_ralloc;
			kmem[num_mclass].mpk_fail += kmem[class].mpk_fail;
		}
	}

	if (first_sample == FALSE) {
		for (class = 0; class <= num_mclass; class++) {
			acc_kmem[class].mpk_mem += kmem[class].mpk_mem;
			acc_kmem[class].mpk_balloc += kmem[class].mpk_balloc;
			acc_kmem[class].mpk_ralloc += kmem[class].mpk_ralloc;
			acc_kmem[class].mpk_fail += kmem[class].mpk_fail - 
			old_kmem[class].mpk_fail;
		}
		num_samples++;
	}
	else {
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;
        
        return(TRUE);
}


sarout_t
sar_kmem_out(int column, int mode, int class)
{
        sarerrno = 0;
        
        if (mode == OUTPUT_DATA || mode == OUTPUT_RAW) {
                switch (column) {
                      case KMA_MEM:
			if (mode == OUTPUT_DATA) {
				return((sarout_t) kmem[class].mpk_mem * (sarout_t) machinfo.mets_native_units.mnu_pagesz);
			}
			else {
				return(kmem[class].mpk_mem);
			}
                        break;
                        
                      case KMA_BALLOC:
                        return(kmem[class].mpk_balloc);
                        break;
                        
                      case KMA_RALLOC:
                        return(kmem[class].mpk_ralloc);
                        break;
                        
                      case KMA_FAIL:
                        return((sarout_t) kmem[class].mpk_fail 
			       - (sarout_t) old_kmem[class].mpk_fail);
                        break;
                        
                      default:
                        sarerrno = SARERR_OUTFIELD;
                        return(-1);
                }
        }
        else if (mode == OUTPUT_FINAL_DATA || mode == OUTPUT_FINAL_RAW) {
                switch (column) {
                      case KMA_MEM:
			if (mode == OUTPUT_FINAL_DATA) {
				return(((sarout_t) acc_kmem[class].mpk_mem * (sarout_t) machinfo.mets_native_units.mnu_pagesz)/(sarout_t) num_samples);
			}
			else {
				return(acc_kmem[class].mpk_mem);
			}
                        break;
                        
                      case KMA_BALLOC:
                        return((sarout_t) acc_kmem[class].mpk_balloc/(sarout_t) num_samples);
                        break;
                        
                      case KMA_RALLOC:
                        return(acc_kmem[class].mpk_ralloc/num_samples);
                        break;
                        
                      case KMA_FAIL:
                        return((sarout_t) acc_kmem[class].mpk_fail/num_samples);
                        break;
                        
                      default:
                        sarerrno = SARERR_OUTFIELD;
                        return(-1);
                }
        }
        
        sarerrno = SARERR_OUTMODE;
        return(-1);
}


void
sar_kmem_cleanup(void)
{
	free(kmem);
	free(kmem_temp);
	free(old_kmem);
	free(acc_kmem);

	kmem = NULL;
	kmem_temp = NULL;
	old_kmem = NULL;
	acc_kmem = NULL;
}
