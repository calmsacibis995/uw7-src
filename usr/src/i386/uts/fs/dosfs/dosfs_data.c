#ident	"@(#)kern-i386:fs/dosfs/dosfs_data.c	1.1"

#include "dosfs.h"

struct denode *denode;		 	/* the inode table itself */
long int ndenode;

/*
 * Inode Hash List/Free List
 */
lock_t dosfs_denode_table_mutex;
LKINFO_DECL(dosfs_denode_table_lkinfo, "FS:DOSFS:dosfs denode table lock", 0);

/*
 * Update lock
 */
sleep_t dosfs_updlock;
LKINFO_DECL(dosfs_updlock_lkinfo, "FS:DOSFS:dosfs update lock", 0);

LKINFO_DECL(dosfs_deno_lock_lkinfo, "FS:DOSFS:per-denode sleep lock", 0);
LKINFO_DECL(dosfs_rename_lkinfo, "FS:DOSFS: rename lock", 0);
