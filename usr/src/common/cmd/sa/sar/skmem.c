/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/skmem.c	1.7"
#ident	"$Header$"

/* skmem.c
 * 
 * Processes SAR_KMEM_SIZES records.  These records contain a list
 * of KMA memory pool sizes.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"

uint_t   *kmem_sizes = NULL;
static uint_t  *temp_sizes = NULL;

static int   kmem_read = FALSE;


flag
sar_skmem_init(void)
{
	if (kmem_sizes == NULL) {
		kmem_sizes = malloc(machinfo.num_kmem_sizes * sizeof(*kmem_sizes));
		temp_sizes = malloc(machinfo.num_kmem_sizes * sizeof(*temp_sizes));
		if (kmem_sizes == NULL || temp_sizes == NULL) {
			return(FALSE);
		}
	}
	return(TRUE);
}


/* ARGSUSED */

int
sar_skmem(FILE *infile, sar_header sarh, flag32 of)
{
	if (sarh.item_size != sizeof(*kmem_sizes)) {
		printf("SAR_ERROR: sizeof(*kmem_sizes) mismatch: %d %d\n",
		       sarh.item_size, sizeof(*kmem_sizes));
		sar_error(SARERR_KMEMSZ);
	}
	if (sarh.item_count != machinfo.num_kmem_sizes) {
		printf("SAR_ERROR: kmem count mismatch: %d %d\n",
		       sarh.item_count, machinfo.num_kmem_sizes);
		sar_error(SARERR_KMEMSZ);
	}
	
	if (kmem_read) {
		fread(temp_sizes, sarh.item_size, sarh.item_count, infile);
		if (memcmp(kmem_sizes, temp_sizes, sarh.item_size * sarh.item_count)) {
			puts("SAR_ERROR: kmem_sizes mismatch");
			sar_error(SARERR_KMEMSZ);
		}
	}
	else {
		fread(kmem_sizes, sarh.item_size, sarh.item_count, infile);
		kmem_read = TRUE;
	}
	return(TRUE);
}

