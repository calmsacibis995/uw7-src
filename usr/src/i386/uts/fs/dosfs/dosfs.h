#ifndef DOSFS_H
#define DOSFS_H

#if defined(__cplusplus)
extern "C" {
#endif

#ident	"@(#)kern-i386:fs/dosfs/dosfs.h	1.2.4.1"
#ident  "$Header$"

/*
 *
 *  Adapted for System V Release 4	(ESIX 4.0.4)	
 *
 *  Gerard van Dorth	(gdorth@nl.oracle.com)
 *  Paul Bauwens	(paul@pphbau.atr.bso.nl)
 *
 *  May 1993
 *
 *  Originally written by Paul Popelka (paulp@uts.amdahl.com)
 *
 *  You can do anything you want with this software,
 *    just don't say you wrote it,
 *    and don't remove this notice.
 *
 *  This software is provided "as is".
 *
 *  The author supplies this software to be publicly
 *  redistributed on the understanding that the author
 *  is not responsible for the correct functioning of
 *  this software in any circumstances and is not liable
 *  for any damages caused by this software.
 *
 *  October 1992
 *
 */

#include <fs/fski.h>

#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/dnlc.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/fs_hier.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/pathname.h>
#include <fs/specfs/specfs.h>
#include <fs/stat.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/acct.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>


#include "fs/fs_subr.h"

/* dosfs includes */
#include "bootsect.h"
#include "bpb.h"
#include "direntry.h"
#include "denode.h"
#include "fat.h"
#include "dosfs_data.h"
#include "dosfs_filsys.h"
#include "dosfs_hier.h"
#include "dosfs_lbuf.h"

#ifdef _KERNEL

/* dosfs prototypes */

int createde(struct denode *dep, struct vnode *dvp, struct denode **depp);
int deflush(struct vfs *vfsp, int force);
int detrunc(struct denode *dep, u_long length, int flags);
int deupdat(struct denode *dep, timestruc_t *tp, int waitfor);
int doscheckpath(struct denode *source, struct denode *target);
int dosdirempty(struct denode *dep);
int extendfile(struct denode *dep, lbuf_t **bpp, u_long *ncp);
int fillinusemap(struct dosfs_vfs *pvp);
int get_direntp(struct dosfs_vfs *pvp, u_long dirclust, u_long diroffset, dosdirent_t *dep);
int readde(struct denode *dep, lbuf_t **bpp, struct direntry **epp);
int removede(struct vnode *vp);
void cleanlocks();
void fc_purge(struct denode *dep, unsigned int frcn);
void fc_lookup(struct denode *dep, u_long findcn, u_long *frcnp, u_long *fsrcnp);
void deput(struct denode *dep);
void deunhash(struct denode *dp);
void dosfs_deflush(void);
void reinsert(struct denode *dep);

extern vnodeops_t dosfs_vnodeops;
extern int dosfs_fstype;

extern int dosfs_tflush;

#ifndef NO_GENOFF
extern int dosfs_shared;
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif

#endif /* DOSFS_H */
