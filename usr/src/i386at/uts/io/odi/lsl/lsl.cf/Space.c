#ident	"@(#)Space.c	9.1"
#ident	"$Header$"

#include <sys/types.h>
#include <config.h>

/*
 * The STREAMS_LOG define determines if STREAMS tracing will be done in the
 * driver. A non-zero value will allow the strace(1M) command to follow
 * activity in the driver.  The driver ID used in the strace(1M) command is
 * equal to the ENET_ID value (generally 2101).
 *
 * NOTE: STREAMS tracing can greatly reduce the performance of the driver
 * and should only be used for trouble shooting.
 */
#define	STREAMS_LOG	0
int	lslstrlog = 0;

/*
 * to keep inet stats or not.
 */
int	lsl_keeping_inetstats = 1;

int	lsl_eth_frame_size = ETH_FRAME_SIZE;
int	lsl_tok_frame_size = TOK_FRAME_SIZE;
int	lsl_atm_frame_size = ATM_FRAME_SIZE;
int	lsl_fddi_frame_size = FDDI_FRAME_SIZE;

/*
 * ODI spec 3.1 tunables.
 */
int     lsl_31_1kchunks =       LSL_1KCHUNKS;
int     lsl_31_2kchunks =       LSL_2KCHUNKS;
int     lsl_31_6kchunks =       LSL_6KCHUNKS;
int     lsl_31_10kchunks =      LSL_10KCHUNKS;
int     lsl_31_36kchunks =      LSL_36KCHUNKS;

void    *allocd_2k_chunks[LSL_2KCHUNKS];
void    *allocd_6k_chunks[LSL_6KCHUNKS];
void    *allocd_10k_chunks[LSL_10KCHUNKS];
void    *allocd_36k_chunks[LSL_36KCHUNKS];

void    *tmp_array[LSL_2KCHUNKS];

/*
 * ODI cache tunable for > PAGESIZE physcontiguous blocks allocated
 * from the odimem module.
 */
int	lsl_odimem_max_cached =	LSL_ODIMEM_CACHESZ;

int	lsl_max_mc_ref_count  = 10;  /* max multicast ref count */

