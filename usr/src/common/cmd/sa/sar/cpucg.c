/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/cpucg.c	1.3"
#ident "$Header$"

/* cpucg.c 
 * Processes SAR_CPUCG_P records to build the cpuToCG translation
 * array.
 */

  
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <string.h>

#include "../sa.h"
#include "sar.h"

#include <unistd.h>

cgid_t *cpu_to_cgid = NULL;
int *cpu_to_fakeid = NULL;
static int cpucg_read = FALSE;

static int create_fakeids(void);

int
sar_cpusg(FILE *infile, sar_header sarh, flag32 of) 
{
  if (sarh.item_size != sizeof(*cpu_to_cgid)) {
    metric_warn[sarh.item_code] = TRUE;
  }

  if (sarh.item_count != Num_cpus) {
    printf("SAR_ERROR: cg count mismatch: %d %d\n", 
	   sarh.item_count, Num_cpus);
    sar_error(SARERR_CPUCG);
  }

  if (cpucg_read) {
    cgid_t *temp_map = malloc(sarh.item_count * sarh.item_size);
    if (!temp_map) {
      return FALSE;
    }
    fread(temp_map, sarh.item_size, sarh.item_count, infile);
    if (!memcpy(cpu_to_cgid, temp_map, sarh.item_size * sarh.item_count)) {
      puts("SAR_ERROR: cpu to cg translation mismatch");
      sar_error(SARERR_CPUCG);
    }
    free(temp_map);
  }
  else {
    fread(cpu_to_cgid, sarh.item_size, sarh.item_count, infile);
    cpucg_read = TRUE;
    return create_fakeids();
  }
  return TRUE;
}

struct idmap {
  cgid_t real_id;
  int fake_id;
};


static int
idmap_compare(const void *p1, const void *p2) 
{
  struct idmap *im1 = (struct idmap *) p1;
  struct idmap *im2 = (struct idmap *) p2;

  if (im1->real_id < im2->real_id)
    return -1;
  else if (im1->real_id > im2->real_id)
    return 1;
  else 
    return 0;
}

static int
create_fakeids(void)
{
  struct idmap *sorted_ids;

  int i;

  sorted_ids = malloc(Num_cpus * sizeof(*sorted_ids));
  if (!sorted_ids) {
    return FALSE;
  }

  for (i = 0; i < Num_cpus; ++i) { 
    sorted_ids[i].real_id = cpu_to_cgid[i];
  }

  qsort(sorted_ids, Num_cpus, sizeof(*sorted_ids), idmap_compare);
  
  sorted_ids[0].fake_id = 0;
  for (i = 1; i < Num_cpus; ++i) {
    if (sorted_ids[i].real_id == sorted_ids[i - 1].real_id) {
      sorted_ids[i].fake_id = sorted_ids[i - 1].fake_id;
    }
    else {
      sorted_ids[i].fake_id = sorted_ids[i - 1].fake_id + 1;
    }
  }

  for (i = 0; i < Num_cpus; ++i) {
    int j;
    cgid_t rid = cpu_to_cgid[i];
    for (j = 0; j < Num_cpus; ++j) {
      if (rid != sorted_ids[j].real_id) {
	continue;
      }
      else {
	cpu_to_fakeid[i] = sorted_ids[j].fake_id;
	break;
      }
    }
  }				  

  return TRUE;
}



/*
 * Find out which node a processor is on.
 */

int
which_node(int cpu) {
	if (cpu >= Num_cpus || Num_cgs == 0 || (Num_cgs > Num_cpus)) {
		printf("ERROR in which_node()\n");
		return 0;
	}
	return cpu_to_fakeid[cpu];
} 
