#ident	"@(#)kern-i386:proc/obj/i286x.c	1.1"
#ident	"$Header$"

#include <fs/vnode.h>
#include <proc/exec.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define I286EMUL	"/usr/bin/i286emul"

MOD_EXEC_WRAPPER(i286x_, NULL, NULL, "i286x - exec module");

/*
 * int
 * i286x_exec(vnode_t *vp, struct uarg *args, int level, long *execsz,
 *		exhda_t *ehdp)
 *	Exec a i286x executable file.
 *
 * Calling/Exit State:
 *	Called from gexec via execsw[].
 */
/* ARGSUSED */
int
i286x_exec(vnode_t *vp, struct uarg *args, int level, long *execsz,
	   exhda_t *ehdp)
{
	struct exdata	*edp = &args->execinfop->ei_exdata;
	int	error;

	if ((error = setxemulate(I286EMUL, args, execsz)) != 0)
		return error;

	/*
	 * For a 286 emulator, we set RE_EMUL to allow the emulator to
	 * read or map a 286 binary for which it doesn't have read access.
	 * The emulator should clear this bit (by making the appropriate
	 * sysi86() call) as soon as it has opened or mapped the binary.
	 */
	edp->ex_renv |= RE_EMUL;

	return 0;
}
