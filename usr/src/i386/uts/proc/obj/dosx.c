#ident	"@(#)kern-i386:proc/obj/dosx.c	1.6"
#ident	"$Header$"

#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/* DOSEXEMAGIC	0x5a4d		DOS .exe file */
/* DOSBLTMAGIC	0x5a4c		DOS builtin file */

STATIC char *mrg_dos_file = "/usr/bin/dos";


/*
 * int
 * dosx_exec(vnode_t *vp, struct uarg *args, int level, long *execsz,
 *	     exhda_t *ehdp)
 *	Exec an MS-DOS executable file.
 *
 * Calling/Exit State:
 *	Called from gexec via execsw[].  No spin locks can be held on
 *	entry, no spin locks are held on return.
 */
/* ARGSUSED */
int
dosx_exec(vnode_t	*vp,
	  struct uarg	*args,
	  int		level,
	  long		*execsz,
	  exhda_t	*ehdp)
{
	int error;

	/* If no magic number, maybe it's a .com or .bat file */
	if (args->execinfop->ei_execsw->exec_magic == NULL &&
	    !isdosexec(vp, ehdp))
		return ENOEXEC;

	/* XXX - arrange for setuid on mrg_dos_file to work? */
	return setxemulate(mrg_dos_file, args, execsz);
}
