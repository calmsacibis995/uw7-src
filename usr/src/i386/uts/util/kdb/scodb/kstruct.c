#ident	"@(#)kern-i386:util/kdb/scodb/kstruct.c	1.1"
#ident  "$Header$"

/*
 * This file is intended to be compiled in a kernel tree matching
 * the kernel on which scodb is running.  The following makefile
 * entry should be used:
 *
 *	kstruct.o.atup: kstruct.c
 *		$(CC) -g -W0,-d1 $(INCLIST) $(DEFLIST) -DUNIPROC -c kstruct.c
 *
 *	kstruct.o.mp: kstruct.c
 *		$(CC) -g -W0,-d1 $(INCLIST) $(DEFLIST) -UUNIPROC -c kstruct.c
 *
 * Note that '-W0,-d1' option is used to force Dwarf I debugging output format.
 */

#include <acc/audit/audit.h>

#include <proc/lwp.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/cred.h>
#include <proc/resource.h>
#include <proc/exec.h>
#include <proc/disp.h>
#include <proc/class.h>
#include <proc/acct.h>
#include <proc/session.h>
#include <proc/usync.h>
#include <proc/bind.h>

#include <mem/kmem.h>
#include <mem/ublock.h>
#include <mem/as.h>
#include <mem/anon.h>
#include <mem/page.h>
#include <mem/swap.h>
#include <mem/tuneable.h>
#include <mem/seg_map.h>

#include <util/types.h>
#include <util/ksynch.h>
#include <util/dl.h>
#include <util/plocal.h>
#include <util/engine.h>
#include <util/metrics.h>
#include <util/mod/moddefs.h>
#include <util/mod/mod_obj.h>

#include <fs/buf.h>
#include <fs/fs_hier.h>
#include <fs/vnode.h>
#include <fs/flock.h>
#include <fs/pathname.h>
#include <fs/memfs/memfs.h>

#include <io/stream.h>
#include <io/stropts.h>
#include <io/strstat.h>
#include <io/strsubr.h>
#include <io/tty.h>
#include <io/conf.h>
#include <io/iobuf.h>
#include <io/uio.h>
